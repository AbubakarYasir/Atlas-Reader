#pragma once

#include <QObject>

namespace atlas::accessibility {

// Native expand/collapse for QML tree rows on Qt 6.10 (QML gains
// Accessible.expanded only in Qt 6.12). A row opts in with
//     property bool a11yTreeRow: true
// plus `hasChildren` and `expanded` properties and a function a11yToggle().
// Its accessible interface then reports expandable/expanded/collapsed, so
// UI Automation exposes the ExpandCollapse pattern Narrator announces, and
// the pattern's Expand/Collapse call a11yToggle().
// Also: a focused ListView/GridView reports its current item as the focused
// child and announces each move (Qt Quick otherwise reports only the view).
void installTreeItemAccessibility();

// Context object `atlasAccessibility`: rows call expandedChanged(row) when
// `expanded` changes so screen readers hear the new state. keyboardInUse is
// true after a key press and false after a mouse or touch press, so views can
// draw focus rings only for keyboard users (Windows convention) while keeping
// real focus for Narrator.
class AccessibilityNotifier final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool keyboardInUse READ keyboardInUse NOTIFY keyboardInUseChanged)
public:
    explicit AccessibilityNotifier(QObject* parent = nullptr);
    Q_INVOKABLE void expandedChanged(QObject* item) const;
    bool keyboardInUse() const { return keyboardInUse_; }

signals:
    void keyboardInUseChanged();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    bool keyboardInUse_ = false;
};

} // namespace atlas::accessibility
