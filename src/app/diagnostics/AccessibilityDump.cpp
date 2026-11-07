#include "app/diagnostics/AccessibilityDump.h"

#include <QAccessible>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRect>

namespace atlas::diagnostics {

namespace {

QString roleName(QAccessible::Role role) {
    const QMetaEnum meta = QMetaEnum::fromType<QAccessible::Role>();
    const char* key = meta.valueToKey(role);
    return key ? QString::fromLatin1(key) : QString::number(static_cast<int>(role));
}

void walk(QAccessibleInterface* node, int depth, QFile& out, int& count) {
    if (!node || !node->isValid() || depth > 64 || count > 20000) return;
    const auto state = node->state();
    if (state.invisible) return; // what a screen reader cannot reach
    QJsonObject line{
        {QStringLiteral("depth"), depth},
        {QStringLiteral("role"), roleName(node->role())},
        {QStringLiteral("name"), node->text(QAccessible::Name)},
        {QStringLiteral("description"), node->text(QAccessible::Description)},
        {QStringLiteral("focusable"), bool(state.focusable)},
        {QStringLiteral("focused"), bool(state.focused)},
        {QStringLiteral("selected"), bool(state.selected)},
        {QStringLiteral("checked"), bool(state.checked)},
        {QStringLiteral("disabled"), bool(state.disabled)},
        {QStringLiteral("expandable"), bool(state.expandable)},
        {QStringLiteral("expanded"), bool(state.expanded)},
        {QStringLiteral("collapsed"), bool(state.collapsed)},
        {QStringLiteral("x"), node->rect().x()},
        {QStringLiteral("y"), node->rect().y()},
        {QStringLiteral("w"), node->rect().width()},
        {QStringLiteral("h"), node->rect().height()},
        {QStringLiteral("class"), node->object() ? QString::fromLatin1(node->object()->metaObject()->className()) : QString{}},
    };
    out.write(QJsonDocument{line}.toJson(QJsonDocument::Compact));
    out.write("\n");
    ++count;
    for (int i = 0; i < node->childCount(); ++i) walk(node->child(i), depth + 1, out, count);
}

} // namespace

bool AccessibilityDump::write(QObject* root, const QString& path) const {
    QObject* target = root;
    if (auto* item = qobject_cast<QQuickItem*>(root); item && item->window()) target = item->window();
    QAccessibleInterface* top = QAccessible::queryAccessibleInterface(target);
    QFile out{path};
    if (!top || !out.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    int count = 0;
    walk(top, 0, out, count);
    // What a screen reader treats as focused (UI Automation GetFocus asks the
    // window for its focus child, as here).
    QJsonObject focus{{QStringLiteral("focusReport"), true}};
    if (auto* window = qobject_cast<QQuickWindow*>(target); window && window->activeFocusItem()) {
        QQuickItem* item = window->activeFocusItem();
        focus.insert(QStringLiteral("item"), QString::fromLatin1(item->metaObject()->className()));
        if (QQuickItem* parent = item->parentItem())
            focus.insert(QStringLiteral("itemParent"), QString::fromLatin1(parent->metaObject()->className()));
    }
    if (QAccessibleInterface* focused = top->focusChild()) {
        focus.insert(QStringLiteral("role"), roleName(focused->role()));
        focus.insert(QStringLiteral("name"), focused->text(QAccessible::Name));
        const QRect r = focused->rect();
        focus.insert(QStringLiteral("rect"), QStringLiteral("%1,%2 %3x%4").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height()));
    }
    out.write(QJsonDocument{focus}.toJson(QJsonDocument::Compact));
    out.write("\n");
    return count > 0;
}

} // namespace atlas::diagnostics
