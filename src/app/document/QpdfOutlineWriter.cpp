#include "app/document/QpdfOutlineWriter.h"

#include "core/bookmarks/LocationEnvelope.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonValue>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryFile>

#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>

namespace atlas::app {
namespace {

constexpr int kQpdfTimeoutMs = 120000;

struct Run final {
    int exitCode{-1};
    QByteArray output;
    QByteArray errors;
};

// The password goes through standard input, never the command line.
Run runQpdf(const QString& qpdf, QStringList arguments, const QStringList& files, const QString& password) {
    Run result;
    if (!password.isEmpty()) arguments << QStringLiteral("--password-file=-");
    arguments << QStringLiteral("--");
    for (const auto& file : files) arguments << QDir::toNativeSeparators(file);
    QProcess process;
    process.start(qpdf, arguments, QIODevice::ReadWrite);
    if (!process.waitForStarted(5000)) return result;
    if (!password.isEmpty()) process.write(password.toUtf8() + '\n');
    process.closeWriteChannel();
    if (!process.waitForFinished(kQpdfTimeoutMs)) {
        process.kill();
        process.waitForFinished(2000);
        return result;
    }
    result.exitCode = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
    result.output = process.readAllStandardOutput();
    result.errors = process.readAllStandardError();
    return result;
}

bool isRef(const QJsonValue& value) {
    static const QRegularExpression pattern{QStringLiteral("^\\d+ \\d+ R$")};
    return value.isString() && pattern.match(value.toString()).hasMatch();
}

// Object references are renumbered when qpdf writes; compare structure, not
// numbers. Page references become their page index so destinations compare.
QJsonValue normalized(const QJsonValue& value, const QHash<QString, int>& pageIndex) {
    if (value.isDouble()) return std::round(value.toDouble() * 1000.0) / 1000.0; // 72 and 72.0 are one number
    if (isRef(value)) {
        const auto found = pageIndex.find(value.toString());
        return found == pageIndex.end() ? QJsonValue{QStringLiteral("<ref>")}
                                        : QJsonValue{QStringLiteral("<page %1>").arg(found.value())};
    }
    if (value.isArray()) {
        QJsonArray out;
        for (const auto& item : value.toArray()) out.append(normalized(item, pageIndex));
        return out;
    }
    if (value.isObject()) {
        QJsonObject out;
        const QJsonObject in = value.toObject();
        for (auto it = in.begin(); it != in.end(); ++it) out.insert(it.key(), normalized(it.value(), pageIndex));
        return out;
    }
    return value;
}

const QStringList& linkKeys() {
    static const QStringList keys{QStringLiteral("/Parent"), QStringLiteral("/First"), QStringLiteral("/Last"),
                                  QStringLiteral("/Next"), QStringLiteral("/Prev"), QStringLiteral("/Count")};
    return keys;
}

QJsonObject withoutLinks(QJsonObject dict) {
    for (const auto& key : linkKeys()) dict.remove(key);
    return dict;
}

struct Objects final {
    QJsonObject map;
    [[nodiscard]] QJsonObject dict(const QString& ref) const {
        const QJsonObject wrapper = map.value(QStringLiteral("obj:") + ref).toObject();
        if (wrapper.contains(QStringLiteral("value"))) return wrapper.value(QStringLiteral("value")).toObject();
        return wrapper.value(QStringLiteral("stream")).toObject().value(QStringLiteral("dict")).toObject();
    }
    [[nodiscard]] QJsonValue resolve(const QJsonValue& value) const {
        if (!isRef(value)) return value;
        const QJsonObject wrapper = map.value(QStringLiteral("obj:") + value.toString()).toObject();
        if (wrapper.contains(QStringLiteral("value"))) return wrapper.value(QStringLiteral("value"));
        return wrapper.value(QStringLiteral("stream")).toObject().value(QStringLiteral("dict"));
    }
};

// A page attribute, inherited through /Parent when the page does not set it.
QJsonValue inherited(const Objects& objects, QJsonObject node, const QString& key) {
    for (int depth = 0; depth < 64; ++depth) {
        if (node.contains(key)) return objects.resolve(node.value(key));
        const QJsonValue parent = node.value(QStringLiteral("/Parent"));
        if (!isRef(parent)) break;
        node = objects.dict(parent.toString());
    }
    return {};
}

std::optional<bookmarks::PageGeometry> geometryOf(const QJsonValue& box, const QJsonValue& rotate) {
    const QJsonArray numbers = box.toArray();
    if (numbers.size() != 4) return std::nullopt;
    double v[4];
    for (int i = 0; i < 4; ++i) {
        if (!numbers.at(i).isDouble()) return std::nullopt;
        v[i] = numbers.at(i).toDouble();
    }
    bookmarks::PageGeometry page;
    page.left = std::min(v[0], v[2]);
    page.right = std::max(v[0], v[2]);
    page.bottom = std::min(v[1], v[3]);
    page.top = std::max(v[1], v[3]);
    page.rotate = rotate.isDouble() ? static_cast<int>(rotate.toDouble()) : 0;
    if (!(page.right > page.left) || !(page.top > page.bottom)) return std::nullopt;
    return page;
}

void collectItems(const QJsonArray& list, const QString& parentKey, const Objects& objects,
                  QVector<PdfOutlineItem>& out, int depth) {
    for (int i = 0; i < list.size(); ++i) {
        const QJsonObject entry = list.at(i).toObject();
        PdfOutlineItem item;
        item.key = parentKey.isEmpty() ? QString::number(i) : parentKey + u'/' + QString::number(i);
        item.parentKey = parentKey;
        item.title = entry.value(QStringLiteral("title")).toString();
        item.objectRef = entry.value(QStringLiteral("object")).toString();
        item.dict = objects.dict(item.objectRef);
        item.destPage = entry.value(QStringLiteral("destpageposfrom1")).toInt(0) - 1;
        item.open = entry.value(QStringLiteral("open")).toBool(false);
        out.push_back(item);
        if (depth < 256) collectItems(entry.value(QStringLiteral("kids")).toArray(), item.key, objects, out, depth + 1);
    }
}

QString utf8(const std::string& text) { return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size())); }

double rounded(double value) { return std::round(value * 1000.0) / 1000.0; }

// An Atlas destination as a PDF destination array on `pageRef`.
std::optional<QJsonArray> destinationArray(const std::string& location, const PdfSourceOutline& source) {
    const auto decoded = bookmarks::decodeLocation(location);
    if (decoded.status != bookmarks::LocationDecodeStatus::ok) return std::nullopt;
    const auto& dest = decoded.envelope->pdf;
    if (dest.pageIndex >= static_cast<std::uint32_t>(source.pageRefs.size())) return std::nullopt;
    const QString ref = source.pageRefs.at(static_cast<qsizetype>(dest.pageIndex));
    const auto& page = source.pages.at(static_cast<qsizetype>(dest.pageIndex));
    using K = bookmarks::DestinationKind;
    const bool upright = ((page.rotate % 360) + 360) % 360 == 0;
    const auto point = bookmarks::pdfPointFromFractions(page, dest.left.value_or(0.0), dest.top.value_or(0.0));
    switch (dest.kind) {
    case K::fit: return QJsonArray{ref, QStringLiteral("/Fit")};
    case K::fitB: return QJsonArray{ref, QStringLiteral("/FitB")};
    case K::fitH:
    case K::fitBH:
        if (upright) return QJsonArray{ref, dest.kind == K::fitH ? QStringLiteral("/FitH") : QStringLiteral("/FitBH"),
                                      dest.top ? QJsonValue{rounded(point.y)} : QJsonValue{}};
        break;
    case K::fitV:
    case K::fitBV:
        if (upright) return QJsonArray{ref, dest.kind == K::fitV ? QStringLiteral("/FitV") : QStringLiteral("/FitBV"),
                                      dest.left ? QJsonValue{rounded(point.x)} : QJsonValue{}};
        break;
    case K::fitR:
        if (upright && dest.right && dest.bottom) {
            const auto corner = bookmarks::pdfPointFromFractions(page, *dest.right, *dest.bottom);
            return QJsonArray{ref, QStringLiteral("/FitR"), rounded(point.x), rounded(corner.y), rounded(corner.x),
                              rounded(point.y)};
        }
        break;
    case K::xyz: break;
    }
    // XYZ (also the faithful form of the other kinds on a rotated page).
    return QJsonArray{ref, QStringLiteral("/XYZ"), dest.left ? QJsonValue{rounded(point.x)} : QJsonValue{},
                      dest.top ? QJsonValue{rounded(point.y)} : QJsonValue{},
                      dest.zoom ? QJsonValue{*dest.zoom} : QJsonValue{}};
}

} // namespace

PdfSourceOutline QpdfOutlineWriter::parse(const QByteArray& json) {
    PdfSourceOutline out;
    QJsonParseError error;
    const QJsonObject root = QJsonDocument::fromJson(json, &error).object();
    if (error.error != QJsonParseError::NoError || !root.contains(QStringLiteral("qpdf"))) {
        out.error = QStringLiteral("unreadable");
        return out;
    }
    const QJsonArray qpdf = root.value(QStringLiteral("qpdf")).toArray();
    if (qpdf.size() < 2) {
        out.error = QStringLiteral("unreadable");
        return out;
    }
    out.maxObjectId = qpdf.at(0).toObject().value(QStringLiteral("maxobjectid")).toInt();
    Objects objects{qpdf.at(1).toObject()};
    const QJsonObject trailer = objects.map.value(QStringLiteral("trailer")).toObject().value(QStringLiteral("value")).toObject();
    out.rootRef = trailer.value(QStringLiteral("/Root")).toString();
    out.rootDict = objects.dict(out.rootRef);
    if (out.rootRef.isEmpty() || out.rootDict.isEmpty()) {
        out.error = QStringLiteral("unreadable");
        return out;
    }
    out.outlinesDict = objects.resolve(out.rootDict.value(QStringLiteral("/Outlines"))).toObject();

    QHash<QString, int> pageIndex;
    QJsonArray pages;
    for (const auto& value : root.value(QStringLiteral("pages")).toArray()) {
        const QString ref = value.toObject().value(QStringLiteral("object")).toString();
        const QJsonObject dict = objects.dict(ref);
        QJsonValue box = inherited(objects, dict, QStringLiteral("/CropBox"));
        const QJsonValue media = inherited(objects, dict, QStringLiteral("/MediaBox"));
        if (!box.isArray()) box = media;
        const QJsonValue rotate = inherited(objects, dict, QStringLiteral("/Rotate"));
        const auto geometry = geometryOf(box, rotate);
        if (!geometry) {
            out.error = QStringLiteral("pageGeometry");
            return out;
        }
        pageIndex.insert(ref, static_cast<int>(out.pageRefs.size()));
        out.pageRefs.push_back(ref);
        out.pages.push_back(*geometry);
        QJsonObject summary = dict;
        summary.remove(QStringLiteral("/Parent"));
        summary.insert(QStringLiteral("=MediaBox"), media);
        summary.insert(QStringLiteral("=CropBox"), box);
        summary.insert(QStringLiteral("=Rotate"), rotate);
        summary.insert(QStringLiteral("=Resources"), inherited(objects, dict, QStringLiteral("/Resources")));
        pages.append(summary);
    }
    collectItems(root.value(QStringLiteral("outlines")).toArray(), {}, objects, out.items, 0);

    // What a save must leave alone (references normalized).
    QJsonObject rootRest = out.rootDict;
    rootRest.remove(QStringLiteral("/Outlines"));
    QJsonObject preservation;
    preservation.insert(QStringLiteral("pages"), normalized(pages, pageIndex));
    preservation.insert(QStringLiteral("root"), normalized(rootRest, pageIndex));
    preservation.insert(QStringLiteral("info"), normalized(objects.resolve(trailer.value(QStringLiteral("/Info"))), pageIndex));
    // Encryption compared by what it means: qpdf writes the implicit /Length 40
    // of V1 explicitly, which changes nothing.
    QJsonObject encrypt = objects.resolve(trailer.value(QStringLiteral("/Encrypt"))).toObject();
    if (encrypt.value(QStringLiteral("/V")).toInt(0) <= 1 && encrypt.value(QStringLiteral("/Length")).toInt(40) == 40)
        encrypt.remove(QStringLiteral("/Length"));
    preservation.insert(QStringLiteral("encrypt"), normalized(encrypt, pageIndex));
    preservation.insert(QStringLiteral("acroform"), normalized(objects.resolve(out.rootDict.value(QStringLiteral("/AcroForm"))), pageIndex));
    preservation.insert(QStringLiteral("names"), normalized(objects.resolve(out.rootDict.value(QStringLiteral("/Names"))), pageIndex));
    out.preservation = preservation;
    out.ok = true;
    return out;
}

PdfSourceOutline QpdfOutlineWriter::read(const QString& qpdf, const QString& path, const QString& password) {
    const Run run = runQpdf(qpdf,
                            {QStringLiteral("--json=2"), QStringLiteral("--json-key=outlines"),
                             QStringLiteral("--json-key=pages"), QStringLiteral("--json-key=qpdf")},
                            {path}, password);
    if (run.exitCode != 0 && run.exitCode != 3) {
        PdfSourceOutline out;
        out.error = run.exitCode < 0 ? QStringLiteral("qpdfUnavailable") : QStringLiteral("unreadable");
        return out;
    }
    return parse(run.output);
}

std::optional<QByteArray> QpdfOutlineWriter::buildUpdate(const PdfSourceOutline& source,
                                                         const bookmarks::BaseSnapshot& base,
                                                         const bookmarks::SavePlan& plan, QString* reason) {
    const auto fail = [reason](const char* why) -> std::optional<QByteArray> {
        if (reason) *reason = QString::fromLatin1(why);
        return std::nullopt;
    };
    // The PDF must be the one the base snapshot was read from, item for item.
    if (!source.ok || source.items.size() != static_cast<qsizetype>(base.nodes.size())) return fail("outlineMismatch");
    QHash<QString, const PdfOutlineItem*> byKey;
    for (const auto& item : source.items) byKey.insert(item.key, &item);
    for (const auto& node : base.nodes) {
        const auto found = byKey.find(utf8(node.key));
        if (found == byKey.end() || found.value()->title != utf8(node.title) ||
            found.value()->parentKey != (node.parentKey ? utf8(*node.parentKey) : QString{}))
            return fail("outlineMismatch");
    }

    int next = source.maxObjectId + 1;
    const QString outlinesRef = QStringLiteral("%1 0 R").arg(next++);
    QVector<QString> refs;
    QVector<QJsonObject> dicts;
    QVector<QVector<int>> children(plan.items.size());
    QVector<int> tops;
    QVector<bool> open;
    for (qsizetype i = 0; i < static_cast<qsizetype>(plan.items.size()); ++i) {
        const auto& item = plan.items[static_cast<std::size_t>(i)];
        refs.push_back(QStringLiteral("%1 0 R").arg(next++));
        QJsonObject dict;
        bool isOpen = false;
        if (item.baseKey) {
            const PdfOutlineItem* original = byKey.value(utf8(*item.baseKey));
            dict = withoutLinks(original->dict);
            isOpen = original->open;
        }
        if (item.titleChanged) dict.insert(QStringLiteral("/Title"), QStringLiteral("u:") + utf8(item.title));
        if (item.locationChanged) {
            dict.remove(QStringLiteral("/Dest"));
            dict.remove(QStringLiteral("/A"));
            if (!item.location.empty()) {
                const auto dest = destinationArray(item.location, source);
                if (!dest) return fail("unreadableLocation");
                dict.insert(QStringLiteral("/Dest"), *dest);
            }
        }
        dicts.push_back(dict);
        open.push_back(isOpen);
        if (item.parent >= 0) children[item.parent].push_back(static_cast<int>(i));
        else tops.push_back(static_cast<int>(i));
    }
    // /Count: descendants visible when this item's ancestors are open.
    std::function<int(int)> visible = [&](int index) {
        int count = 0;
        for (const int child : children[index]) count += 1 + (open[child] ? visible(child) : 0);
        return count;
    };
    const auto link = [&](const QVector<int>& list, const QString& parentRef) {
        for (qsizetype k = 0; k < list.size(); ++k) {
            QJsonObject& dict = dicts[list[k]];
            dict.insert(QStringLiteral("/Parent"), parentRef);
            if (k > 0) dict.insert(QStringLiteral("/Prev"), refs[list[k - 1]]);
            if (k + 1 < list.size()) dict.insert(QStringLiteral("/Next"), refs[list[k + 1]]);
        }
    };
    link(tops, outlinesRef);
    for (qsizetype i = 0; i < children.size(); ++i) {
        const auto& kids = children[i];
        link(kids, refs[i]);
        if (kids.isEmpty()) continue;
        dicts[i].insert(QStringLiteral("/First"), refs[kids.front()]);
        dicts[i].insert(QStringLiteral("/Last"), refs[kids.back()]);
        const int count = visible(static_cast<int>(i));
        dicts[i].insert(QStringLiteral("/Count"), open[i] ? count : -count);
    }

    QJsonObject objects;
    QJsonObject rootDict = source.rootDict;
    if (tops.isEmpty()) {
        rootDict.remove(QStringLiteral("/Outlines"));
    } else {
        QJsonObject outlines = withoutLinks(source.outlinesDict);
        outlines.insert(QStringLiteral("/Type"), QStringLiteral("/Outlines"));
        outlines.insert(QStringLiteral("/First"), refs[tops.front()]);
        outlines.insert(QStringLiteral("/Last"), refs[tops.back()]);
        int total = 0;
        for (const int top : tops) total += 1 + (open[top] ? visible(top) : 0);
        outlines.insert(QStringLiteral("/Count"), total);
        objects.insert(QStringLiteral("obj:") + outlinesRef, QJsonObject{{QStringLiteral("value"), outlines}});
        rootDict.insert(QStringLiteral("/Outlines"), outlinesRef);
    }
    for (qsizetype i = 0; i < refs.size(); ++i)
        objects.insert(QStringLiteral("obj:") + refs[i], QJsonObject{{QStringLiteral("value"), dicts[i]}});
    objects.insert(QStringLiteral("obj:") + source.rootRef, QJsonObject{{QStringLiteral("value"), rootDict}});

    const QJsonObject header{{QStringLiteral("jsonversion"), 2}, {QStringLiteral("maxobjectid"), source.maxObjectId}};
    return QJsonDocument{QJsonObject{{QStringLiteral("qpdf"), QJsonArray{header, objects}}}}.toJson(QJsonDocument::Compact);
}

QString QpdfOutlineWriter::write(const QString& qpdf, const QString& source, const QString& password,
                                 const QByteArray& update, const QString& output) {
    QTemporaryFile json;
    json.setFileTemplate(QDir::temp().filePath(QStringLiteral("atlas-outline-XXXXXX.json")));
    if (!json.open() || json.write(update) != update.size() || !json.flush()) return QStringLiteral("tempFile");
    json.close();
    const Run written = runQpdf(qpdf, {QStringLiteral("--update-from-json=") + QDir::toNativeSeparators(json.fileName())},
                                {source, output}, password);
    if (written.exitCode < 0) return QStringLiteral("qpdfUnavailable");
    if (written.exitCode != 0 && written.exitCode != 3) return QStringLiteral("writeFailed");
    const Run check = runQpdf(qpdf, {QStringLiteral("--check")}, {output}, password);
    if (check.exitCode == 0) return {};
    if (check.exitCode == 3) { // warnings: acceptable only if the source has them too
        const Run sourceCheck = runQpdf(qpdf, {QStringLiteral("--check")}, {source}, password);
        if (sourceCheck.exitCode == 3) return {};
    }
    return QStringLiteral("validation");
}

QString QpdfOutlineWriter::verify(const PdfSourceOutline& before, const PdfSourceOutline& after,
                                  const bookmarks::SavePlan& plan) {
    if (!after.ok) return QStringLiteral("validation");
    if (after.preservation != before.preservation) return QStringLiteral("preservation");
    if (after.items.size() != static_cast<qsizetype>(plan.items.size())) return QStringLiteral("outlineDiffers");

    QHash<QString, int> beforePages;
    for (qsizetype i = 0; i < before.pageRefs.size(); ++i) beforePages.insert(before.pageRefs[i], static_cast<int>(i));
    QHash<QString, int> afterPages;
    for (qsizetype i = 0; i < after.pageRefs.size(); ++i) afterPages.insert(after.pageRefs[i], static_cast<int>(i));
    QHash<QString, const PdfOutlineItem*> beforeByKey;
    for (const auto& item : before.items) beforeByKey.insert(item.key, &item);

    // Keys the plan's tree should have, in the same depth-first order.
    QVector<QString> keys;
    QVector<int> childCount(static_cast<qsizetype>(plan.items.size()), 0);
    int topCount = 0;
    for (const auto& item : plan.items) {
        const int position = item.parent >= 0 ? childCount[item.parent]++ : topCount++;
        keys.push_back(item.parent >= 0 ? keys[item.parent] + u'/' + QString::number(position) : QString::number(position));
    }
    for (qsizetype i = 0; i < keys.size(); ++i) {
        const auto& planned = plan.items[static_cast<std::size_t>(i)];
        const auto& written = after.items[i];
        if (written.key != keys[i] || written.title != utf8(planned.title)) return QStringLiteral("outlineDiffers");
        const QJsonObject writtenDict = withoutLinks(written.dict);
        if (planned.baseKey && !planned.titleChanged && !planned.locationChanged) {
            // Unchanged items keep every key exactly (references compared by page).
            const PdfOutlineItem* original = beforeByKey.value(utf8(*planned.baseKey));
            if (!original || normalized(withoutLinks(original->dict), beforePages) != normalized(writtenDict, afterPages))
                return QStringLiteral("outlineDiffers");
        }
        if (planned.locationChanged) {
            if (planned.location.empty()) {
                if (writtenDict.contains(QStringLiteral("/Dest")) || writtenDict.contains(QStringLiteral("/A")))
                    return QStringLiteral("outlineDiffers");
                continue;
            }
            const auto expected = destinationArray(planned.location, before);
            const QJsonArray actual = writtenDict.value(QStringLiteral("/Dest")).toArray();
            if (!expected || normalized(*expected, beforePages) != normalized(actual, afterPages))
                return QStringLiteral("outlineDiffers");
        }
    }
    return {};
}

} // namespace atlas::app
