#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QRectF>
#include <QStringList>
#include <QTextOption>
#include <QUuid>

#include <cstdio>

namespace {

QString loadFamily(const QString& path)
{
    const int id = QFontDatabase::addApplicationFont(path);
    if (id < 0) {
        return {};
    }
    const QStringList families = QFontDatabase::applicationFontFamilies(id);
    return families.isEmpty() ? QString{} : families.first();
}

void drawRtlLine(QPainter& painter,
                 const QRectF& bounds,
                 const QString& text,
                 const QString& family,
                 qreal pointSize)
{
    QFont font(family);
    font.setPointSizeF(pointSize);
    font.setStyleStrategy(QFont::PreferAntialias);
    painter.setFont(font);

    // AlignRight is direction-relative unless AlignAbsolute is present. Without
    // it Qt mirrored the alignment for RTL text and placed the paragraphs at
    // the physical left edge even though glyph/word order remained RTL.
    QTextOption option(Qt::AlignRight | Qt::AlignVCenter | Qt::AlignAbsolute);
    option.setTextDirection(Qt::RightToLeft);
    option.setWrapMode(QTextOption::NoWrap);
    painter.drawText(bounds, text, option);
}

void drawLtrLine(QPainter& painter,
                 const QRectF& bounds,
                 const QString& text,
                 qreal pointSize)
{
    QFont font(QStringLiteral("Arial"));
    font.setPointSizeF(pointSize);
    painter.setFont(font);

    QTextOption option(Qt::AlignLeft | Qt::AlignVCenter);
    option.setTextDirection(Qt::LeftToRight);
    option.setWrapMode(QTextOption::NoWrap);
    painter.drawText(bounds, text, option);
}

} // namespace

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    if (argc != 4) {
        std::fprintf(stderr, "usage: atlas_a014_fixture_generator <arabic-font> <urdu-font> <output.pdf>\n");
        return 64;
    }

    const QString arabicFontPath = QString::fromLocal8Bit(argv[1]);
    const QString urduFontPath = QString::fromLocal8Bit(argv[2]);
    const QString outputPath = QString::fromLocal8Bit(argv[3]);
    if (!QFileInfo::exists(arabicFontPath) || !QFileInfo::exists(urduFontPath)) {
        std::fprintf(stderr, "required font file is missing\n");
        return 66;
    }

    const QString arabicFamily = loadFamily(arabicFontPath);
    const QString urduFamily = loadFamily(urduFontPath);
    if (arabicFamily.isEmpty() || urduFamily.isEmpty()) {
        std::fprintf(stderr, "failed to load an application font\n");
        return 65;
    }

    QPdfWriter writer(outputPath);
    writer.setResolution(72);
    writer.setPageSize(QPageSize(QSizeF(600.0, 480.0), QPageSize::Point, QStringLiteral("A014")));
    writer.setPageMargins(QMarginsF(0.0, 0.0, 0.0, 0.0), QPageLayout::Point);
    writer.setTitle(QStringLiteral("A014 readable Arabic and Urdu visual fidelity"));
    writer.setCreator(QStringLiteral("Atlas N2 Qt text-layout fixture generator"));
    writer.setDocumentId(QUuid(QStringLiteral("{7e59e077-c37f-4ae4-b964-a01400000002}")));

    QPainter painter(&writer);
    if (!painter.isActive()) {
        std::fprintf(stderr, "failed to initialize PDF painter\n");
        return 74;
    }
    painter.fillRect(QRectF(0.0, 0.0, 600.0, 480.0), Qt::white);
    painter.setPen(Qt::black);

    drawRtlLine(painter,
                QRectF(30.0, 15.0, 540.0, 70.0),
                QStringLiteral("مرحبا بالعالم"),
                arabicFamily,
                30.0);
    drawRtlLine(painter,
                QRectF(30.0, 95.0, 540.0, 100.0),
                QStringLiteral("بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ"),
                arabicFamily,
                30.0);
    drawRtlLine(painter,
                QRectF(30.0, 200.0, 540.0, 120.0),
                QStringLiteral("یہ اردو متن ہے"),
                urduFamily,
                30.0);

    drawLtrLine(painter, QRectF(45.0, 345.0, 190.0, 55.0), QStringLiteral("Atlas PDF 123"), 27.0);
    drawRtlLine(painter, QRectF(250.0, 345.0, 155.0, 55.0), QStringLiteral("العربية"), arabicFamily, 27.0);
    drawLtrLine(painter, QRectF(440.0, 345.0, 115.0, 55.0), QStringLiteral("English"), 27.0);

    painter.end();
    return 0;
}
