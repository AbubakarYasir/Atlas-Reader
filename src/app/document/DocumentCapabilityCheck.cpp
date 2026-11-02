#include "app/document/DocumentCapabilityCheck.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace atlas::app {
namespace {

constexpr int kQpdfTimeoutMs = 20000;

struct ProcessResult {
    int exitCode{-1};
    QByteArray output;
    QByteArray errors;
    bool finished{false};
};

ProcessResult runQpdf(const QString& qpdf, QStringList arguments, const QString& path, const QString& password) {
    ProcessResult result;
    QProcess process;
    if (!password.isEmpty()) arguments << QStringLiteral("--password-file=-");
    arguments << QStringLiteral("--") << QDir::toNativeSeparators(path);
    process.start(qpdf, arguments, QIODevice::ReadWrite);
    if (!process.waitForStarted(5000)) return result;
    if (!password.isEmpty()) process.write(password.toUtf8() + '\n');
    process.closeWriteChannel();
    result.finished = process.waitForFinished(kQpdfTimeoutMs);
    if (!result.finished) {
        process.kill();
        process.waitForFinished(2000);
        return result;
    }
    result.exitCode = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
    result.output = process.readAllStandardOutput();
    result.errors = process.readAllStandardError();
    return result;
}

QString objectNumber(const QString& reference) { // "12 0 R" → "12 0"
    const auto parts = reference.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    return parts.size() >= 2 ? parts.at(0) + QLatin1Char(',') + parts.at(1) : QString{};
}

} // namespace

QString QpdfInspector::locate() {
    const QString overridden = QProcessEnvironment::systemEnvironment().value(QStringLiteral("ATLAS_QPDF"));
    if (!overridden.isEmpty() && QFileInfo::exists(overridden)) return overridden;
    const QDir appDir{QCoreApplication::applicationDirPath()};
#ifdef Q_OS_WIN
    const QString bundled = appDir.filePath(QStringLiteral("qpdf/qpdf.exe"));
#else
    const QString bundled = appDir.filePath(QStringLiteral("qpdf/qpdf"));
#endif
    if (QFileInfo::exists(bundled)) return bundled;
    return QStandardPaths::findExecutable(QStringLiteral("qpdf"));
}

QString QpdfInspector::rootReference(const QByteArray& trailerJson) {
    const QJsonArray objects = QJsonDocument::fromJson(trailerJson).object().value(QStringLiteral("qpdf")).toArray();
    for (const auto& entry : objects) {
        const QJsonObject trailer = entry.toObject().value(QStringLiteral("trailer")).toObject();
        const QString root = trailer.value(QStringLiteral("value")).toObject().value(QStringLiteral("/Root")).toString();
        if (!root.isEmpty()) return root;
    }
    return {};
}

document::PdfFacts QpdfInspector::parse(const QByteArray& encryptAcroform, const QByteArray& rootObject, int exitCode,
                                        const QByteArray& errorOutput) {
    using document::Encryption;
    document::PdfFacts facts;
    const QString errors = QString::fromUtf8(errorOutput).toLower();
    // qpdf: 0 ok, 3 ok with warnings (repaired), 2 error.
    if (exitCode != 0 && exitCode != 3) {
        facts.inspected = true;
        if (errors.contains(QStringLiteral("invalid password"))) {
            facts.encryption = Encryption::openPasswordRequired;
        } else if (errors.contains(QStringLiteral("unsupported")) || errors.contains(QStringLiteral("not supported"))) {
            facts.encryption = Encryption::unsupportedHandler;
        } else {
            facts.readable = false;
        }
        return facts;
    }
    const QJsonObject json = QJsonDocument::fromJson(encryptAcroform).object();
    if (json.isEmpty()) return facts; // not inspected
    facts.inspected = true;
    facts.malformed = exitCode == 3;

    const QJsonObject encrypt = json.value(QStringLiteral("encrypt")).toObject();
    if (encrypt.value(QStringLiteral("encrypted")).toBool()) {
        facts.encryption = encrypt.value(QStringLiteral("ownerpasswordmatched")).toBool()
                               ? Encryption::openedWithOwnerPassword
                               : Encryption::openedWithUserPassword;
        const QJsonObject caps = encrypt.value(QStringLiteral("capabilities")).toObject();
        auto flag = [&caps](const char* name) { return caps.value(QLatin1String(name)).toBool(true); };
        facts.permissions.modifyContents = flag("modify") || flag("modifyother");
        facts.permissions.modifyAnnotations = flag("modifyannotations");
        facts.permissions.fillForms = flag("modifyforms");
        facts.permissions.assemble = flag("modifyassembly");
        facts.permissions.extract = flag("extract");
        facts.permissions.print = flag("printlow") || flag("printhigh");
    }

    const QJsonArray fields = json.value(QStringLiteral("acroform")).toObject().value(QStringLiteral("fields")).toArray();
    for (const auto& field : fields) {
        const QJsonObject f = field.toObject();
        if (f.value(QStringLiteral("fieldtype")).toString() == QStringLiteral("/Sig") &&
            !f.value(QStringLiteral("value")).isNull()) {
            facts.hasSignatures = true;
        }
    }

    // Certification: /Perms /DocMDP in the document catalog.
    const QJsonArray objects = QJsonDocument::fromJson(rootObject).object().value(QStringLiteral("qpdf")).toArray();
    for (const auto& entry : objects) {
        const QJsonObject map = entry.toObject();
        for (auto it = map.begin(); it != map.end(); ++it) {
            const QJsonObject value = it.value().toObject().value(QStringLiteral("value")).toObject();
            const QJsonValue perms = value.value(QStringLiteral("/Perms"));
            if (perms.isObject() && perms.toObject().contains(QStringLiteral("/DocMDP"))) facts.certified = true;
            if (perms.isString()) facts.certified = true; // indirect /Perms: treat as certified (safe side)
        }
    }
    return facts;
}

document::PdfFacts QpdfInspector::inspect(const QString& qpdf, const QString& path, const QString& password) {
    if (qpdf.isEmpty()) return {}; // not inspected: never claims writability
    const auto main = runQpdf(qpdf, {QStringLiteral("--json=2"), QStringLiteral("--json-key=encrypt"),
                                     QStringLiteral("--json-key=acroform")},
                              path, password);
    if (!main.finished) return {};
    QByteArray rootJson;
    if (main.exitCode == 0 || main.exitCode == 3) {
        const auto trailer = runQpdf(qpdf, {QStringLiteral("--json=2"), QStringLiteral("--json-key=qpdf"),
                                            QStringLiteral("--json-object=trailer")},
                                     path, password);
        const QString root = objectNumber(rootReference(trailer.output));
        if (!root.isEmpty()) {
            rootJson = runQpdf(qpdf, {QStringLiteral("--json=2"), QStringLiteral("--json-key=qpdf"),
                                      QStringLiteral("--json-object=") + root},
                               path, password)
                           .output;
        }
    }
    return parse(main.output, rootJson, main.exitCode, main.errors);
}

document::FileFacts FileFactsProbe::probe(const QString& path) {
    document::FileFacts facts;
    const QFileInfo info{path};
    facts.exists = info.exists() && info.isFile();
    if (!facts.exists) return facts;
#ifdef Q_OS_WIN
    const std::wstring native = QDir::toNativeSeparators(info.absoluteFilePath()).toStdWString();
    const DWORD attributes = GetFileAttributesW(native.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        facts.readOnlyAttribute = (attributes & FILE_ATTRIBUTE_READONLY) != 0;
        // Files Explorer shows with a cloud icon: contents are fetched on access.
        facts.cloudPlaceholder = (attributes & (FILE_ATTRIBUTE_OFFLINE | 0x00400000 /*RECALL_ON_DATA_ACCESS*/)) != 0;
    }
    // Another program holds the file open for writing if we cannot deny write sharing.
    const HANDLE handle = CreateFileW(native.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        facts.sharingLocked = GetLastError() == ERROR_SHARING_VIOLATION;
    } else {
        CloseHandle(handle);
    }
    // Safe save writes a temporary file in the folder; check the folder's ACL without creating anything.
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        QNtfsPermissionCheckGuard permissionGuard;
#endif
        facts.folderWritable = QFileInfo{info.absolutePath()}.isWritable();
        if (!facts.readOnlyAttribute) facts.readOnlyAttribute = !info.isWritable();
    }
#else
    facts.readOnlyAttribute = !info.isWritable();
    facts.folderWritable = QFileInfo{info.absolutePath()}.isWritable();
#endif
    return facts;
}

RevisionStamp FileFactsProbe::stamp(const QString& path) {
    const QFileInfo info{path};
    if (!info.exists()) return {};
    return {info.size(), info.lastModified().toUTC()};
}

CapabilityCheckResult checkDocumentCapabilities(const QString& path, const QString& password,
                                                const RevisionStamp& baseline) {
    CapabilityCheckResult result;
    result.stamp = FileFactsProbe::stamp(path);
    const document::FileFacts file = FileFactsProbe::probe(path);
    document::PdfFacts pdf;
    if (file.exists && !file.cloudPlaceholder && !file.offline) {
        pdf = QpdfInspector::inspect(QpdfInspector::locate(), path, password);
    }
    document::RevisionFacts revision;
    revision.changedExternally = baseline.valid() && result.stamp.valid() && !(baseline == result.stamp);
    result.assessment = document::assessCapabilities(file, pdf, revision);
    return result;
}

QVariantMap capabilityMap(const document::CapabilityAssessment& assessment) {
    using document::MutationSafety;
    QString state;
    switch (assessment.capabilities.mutation) {
    case MutationSafety::writable: state = QStringLiteral("writable"); break;
    case MutationSafety::permissionRestricted: state = QStringLiteral("restricted"); break;
    case MutationSafety::filesystemReadOnly: state = QStringLiteral("readOnly"); break;
    case MutationSafety::sharingLocked: state = QStringLiteral("locked"); break;
    case MutationSafety::signedOrCertified: state = QStringLiteral("signed"); break;
    case MutationSafety::externalConflict: state = QStringLiteral("changed"); break;
    case MutationSafety::unsupported: state = QStringLiteral("unknown"); break;
    }
    QStringList reasons;
    for (const auto reason : assessment.reasons) reasons.append(QString::fromStdString(document::reasonName(reason)));
    return {{QStringLiteral("checked"), true},
            {QStringLiteral("state"), state},
            {QStringLiteral("canEmbedOutline"), assessment.capabilities.canEmbedOutline},
            {QStringLiteral("canEmbedAnnotations"), assessment.capabilities.canEmbedAnnotations},
            {QStringLiteral("canKeepLocalResearch"), assessment.canKeepLocalResearch},
            {QStringLiteral("reasons"), reasons}};
}

} // namespace atlas::app
