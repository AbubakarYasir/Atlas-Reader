#include "app/diagnostics/AccessibilityDump.h"

#include <QAccessible>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QQuickItem>
#include <QQuickWindow>

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
    return count > 0;
}

} // namespace atlas::diagnostics
