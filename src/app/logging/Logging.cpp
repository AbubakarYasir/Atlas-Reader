#include "app/logging/Logging.h"

#include <QLoggingCategory>

namespace atlas::logging {

Q_LOGGING_CATEGORY(startup, "atlas.startup")
Q_LOGGING_CATEGORY(ui, "atlas.ui")
Q_LOGGING_CATEGORY(performance, "atlas.performance")

void configureLogging() {
    // Keep ordinary alpha builds useful without making verbose debug logging the
    // default. Individual categories can still be enabled with QT_LOGGING_RULES.
    QLoggingCategory::setFilterRules(QStringLiteral(
        "atlas.startup.info=true\n"
        "atlas.ui.info=true\n"
        "atlas.performance.info=true\n"
        "atlas.*.debug=false\n"));

    qSetMessagePattern(
        QStringLiteral("%{time yyyy-MM-ddTHH:mm:ss.zzz} %{type} %{category} %{message}"));
}

} // namespace atlas::logging
