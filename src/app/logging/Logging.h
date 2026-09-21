#pragma once

#include <QLoggingCategory>

namespace atlas::logging {

Q_DECLARE_LOGGING_CATEGORY(startup)
Q_DECLARE_LOGGING_CATEGORY(ui)
Q_DECLARE_LOGGING_CATEGORY(performance)

// Installs the N1 console/debugger logging policy. Product code must never log
// document text, passwords, bookmark descriptions, or other research content
// by default. Persistent/rotating file logging belongs to a later checkpoint.
void configureLogging();

} // namespace atlas::logging
