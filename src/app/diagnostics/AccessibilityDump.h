#pragma once

#include <QObject>
#include <QString>

namespace atlas::diagnostics {

// QA (--show a11y:<file>): writes the window's accessibility tree, as screen
// readers see it through QAccessible (UI Automation on Windows), as JSON Lines:
// depth, role, name, description and the states that matter to Narrator.
// tools/a11y/check_accessibility.py then fails on unnamed controls.
class AccessibilityDump final : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    Q_INVOKABLE bool write(QObject* root, const QString& path) const;
};

} // namespace atlas::diagnostics
