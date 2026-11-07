#include "app/accessibility/TreeItemAccessible.h"

#include <QAccessible>
#include <QCoreApplication>
#include <QEvent>
#include <QQuickItem>
#include <QtQuick/private/qaccessiblequickitem_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemview_p.h>

namespace atlas::accessibility {
namespace {

// Qt Quick Controls' own interfaces report the control's locale, which
// Narrator uses to choose an Arabic or Urdu voice; keep that.
class ControlAccessible : public QAccessibleQuickItem, public QAccessibleAttributesInterface {
public:
    using QAccessibleQuickItem::QAccessibleQuickItem;
    void* interface_cast(QAccessible::InterfaceType type) override {
        if (type == QAccessible::AttributesInterface && hasLocale())
            return static_cast<QAccessibleAttributesInterface*>(this);
        return QAccessibleQuickItem::interface_cast(type);
    }
    QList<QAccessible::Attribute> attributeKeys() const override {
        return {QAccessible::Attribute::Locale};
    }
    QVariant attributeValue(QAccessible::Attribute key) const override {
        if (key == QAccessible::Attribute::Locale) return object()->property("locale");
        return {};
    }

private:
    bool hasLocale() const { return object() && object()->property("locale").isValid(); }
};

bool isTreeRow(const QObject* object) {
    return object && object->property("a11yTreeRow").toBool();
}

class TreeItemAccessible final : public ControlAccessible {
public:
    using ControlAccessible::ControlAccessible;

    QAccessible::State state() const override {
        QAccessible::State result = ControlAccessible::state();
        if (expandable()) {
            const bool open = object()->property("expanded").toBool();
            result.expandable = true;
            result.expanded = open;
            result.collapsed = !open;
        }
        return result;
    }

    QStringList actionNames() const override {
        QStringList names = ControlAccessible::actionNames();
        if (expandable() && !names.contains(showMenuAction())) names.append(showMenuAction());
        return names;
    }

    void doAction(const QString& actionName) override {
        // UI Automation's Expand/Collapse map to the show-menu action.
        if (actionName == showMenuAction() && expandable()) {
            QMetaObject::invokeMethod(object(), "a11yToggle");
            return;
        }
        ControlAccessible::doAction(actionName);
    }

private:
    bool expandable() const {
        return object() && object()->property("hasChildren").toBool();
    }
};

// A focused list or grid: Qt Quick reports the view itself as focused, so
// Narrator would stay silent while arrow keys move the current item. The view
// reports its current item as the focused child and announces each move.
class ItemViewAccessible final : public QAccessibleQuickItem {
public:
    explicit ItemViewAccessible(QQuickItemView* view) : QAccessibleQuickItem(view) {
        QObject::connect(view, &QQuickItemView::currentItemChanged, view, [view] {
            if (!QAccessible::isActive() || !view->hasActiveFocus()) return;
            if (QQuickItem* current = view->currentItem()) {
                if (QAccessible::queryAccessibleInterface(current)) {
                    QAccessibleEvent event(current, QAccessible::Focus);
                    QAccessible::updateAccessibility(&event);
                }
            }
        });
    }

    QAccessibleInterface* focusChild() const override {
        auto* view = static_cast<QQuickItemView*>(item());
        if (!view->hasActiveFocus() || !view->currentItem()) return nullptr;
        return QAccessible::queryAccessibleInterface(view->currentItem());
    }
};

// A popup's own container is what receives focus when a dialog opens, but
// Qt names it only if accessibility was already on when the popup was built.
// Name it after the Dialog it contains, so Narrator says "<title>, dialog".
QString innerDialogName(QQuickItem* root) {
    QList<QQuickItem*> queue = root->childItems();
    for (int i = 0; i < queue.size() && i < 64; ++i) {
        QQuickItem* child = queue.at(i);
        if (QAccessibleInterface* inner = QAccessible::queryAccessibleInterface(child)) {
            if (inner->role() == QAccessible::Dialog) {
                const QString name = inner->text(QAccessible::Name);
                if (!name.isEmpty()) return name;
            }
        }
        queue.append(child->childItems());
    }
    return {};
}

class PopupAccessible final : public ControlAccessible {
public:
    using ControlAccessible::ControlAccessible;
    QString text(QAccessible::Text kind) const override {
        QString value = ControlAccessible::text(kind);
        if (kind == QAccessible::Name && value.isEmpty() && role() == QAccessible::Dialog)
            value = innerDialogName(item());
        return value;
    }
};

bool isAccessibleItem(QQuickItem* item) {
    return QQuickItemPrivate::get(item)->isAccessible;
}

QAccessibleInterface* atlasFactory(const QString&, QObject* object) {
    auto* item = qobject_cast<QQuickItem*>(object);
    if (!item) return nullptr;
    // Qt Quick Controls give every popup container an interface.
    if (item->inherits("QQuickPopupItem")) return new PopupAccessible(item);
    if (!isAccessibleItem(item)) return nullptr; // Qt Quick's own rule
    if (isTreeRow(object)) return new TreeItemAccessible(item);
    if (auto* view = qobject_cast<QQuickItemView*>(item)) return new ItemViewAccessible(view);
    return nullptr;
}

} // namespace

void installTreeItemAccessibility() {
    // Factories are asked newest first. Qt Quick installs its own when its
    // libraries load, before main(); install this before any QML is created
    // so no interface is cached from Qt's factory first.
    QAccessible::installFactory(atlasFactory);
}

AccessibilityNotifier::AccessibilityNotifier(QObject* parent) : QObject(parent) {
    if (QCoreApplication::instance()) QCoreApplication::instance()->installEventFilter(this);
}

bool AccessibilityNotifier::eventFilter(QObject* watched, QEvent* event) {
    bool keyboard = keyboardInUse_;
    switch (event->type()) {
    case QEvent::KeyPress: keyboard = true; break;
    case QEvent::MouseButtonPress:
    case QEvent::TouchBegin:
    case QEvent::TabletPress: keyboard = false; break;
    default: break;
    }
    if (keyboard != keyboardInUse_) {
        keyboardInUse_ = keyboard;
        emit keyboardInUseChanged();
    }
    return QObject::eventFilter(watched, event);
}

void AccessibilityNotifier::expandedChanged(QObject* item) const {
    if (!item || !QAccessible::isActive()) return;
    QAccessible::State changed;
    changed.expanded = true;
    changed.collapsed = true;
    QAccessibleStateChangeEvent event(item, changed);
    QAccessible::updateAccessibility(&event);
}

} // namespace atlas::accessibility
