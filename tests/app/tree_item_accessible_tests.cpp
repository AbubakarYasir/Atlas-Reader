// Screen-reader contracts from src/app/accessibility (N4-04): a bookmark-style
// tree row reports a native expandable/expanded/collapsed state and expands
// through the show-menu action UI Automation uses; a focused list reports its
// current item as the focused child; a popup's container is named after the
// dialog it holds. Runs offscreen with small inline QML.
#include "app/accessibility/TreeItemAccessible.h"

#include <QAccessible>
#include <QElapsedTimer>
#include <QAccessibleActionInterface>
#include <QAccessibleAttributesInterface>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void settle(int ms = 150) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
}

QQuickWindow* load(QQmlEngine& engine, const QByteArray& qml) {
    QQmlComponent component(&engine);
    component.setData(qml, QUrl(QStringLiteral("inline.qml")));
    QObject* root = component.create();
    if (!root) {
        for (const auto& error : component.errors()) std::cerr << error.toString().toStdString() << '\n';
        throw std::runtime_error("inline QML did not load");
    }
    auto* window = qobject_cast<QQuickWindow*>(root);
    require(window != nullptr, "inline QML root is a Window");
    window->show();
    window->requestActivate();
    settle(300);
    return window;
}

const QByteArray treeQml = R"(
import QtQuick
import QtQuick.Controls.Basic
Window {
    width: 300; height: 200
    property alias list: list
    ListModel {
        id: rows
        ListElement { title: "Chapter 1"; hasChildren: true; expanded: true }
        ListElement { title: "Section 1.1"; hasChildren: false; expanded: false }
    }
    ListView {
        id: list
        objectName: "tree"
        anchors.fill: parent
        focus: true
        model: rows
        Accessible.role: Accessible.Tree
        Accessible.name: "Bookmarks"
        delegate: AbstractButton {
            id: row
            required property int index
            required property string title
            required property bool hasChildren
            required property bool expanded
            width: ListView.view.width; height: 24
            readonly property bool a11yTreeRow: true
            function a11yToggle() { rows.setProperty(index, "expanded", !expanded) }
            Accessible.role: Accessible.TreeItem
            Accessible.name: title
        }
    }
}
)";

const QByteArray popupQml = R"(
import QtQuick
import QtQuick.Controls.Basic
Window {
    width: 400; height: 300
    property alias popup: popup
    Popup {
        id: popup
        modal: true; focus: true
        contentItem: Column {
            Accessible.role: Accessible.Dialog
            Accessible.name: "About Atlas Reader"
            Label { text: "Version" }
        }
    }
}
)";

void checkTreeRow(QQmlEngine& engine) {
    QQuickWindow* window = load(engine, treeQml);
    auto* list = window->findChild<QQuickItem*>(QStringLiteral("tree"));
    require(list != nullptr, "tree list exists");
    list->forceActiveFocus();
    settle();
    QQuickItem* first = qvariant_cast<QQuickItem*>(list->property("currentItem"));
    require(first != nullptr, "list has a current row");

    QAccessibleInterface* row = QAccessible::queryAccessibleInterface(first);
    require(row && row->role() == QAccessible::TreeItem, "row is a TreeItem");
    require(row->text(QAccessible::Name) == QStringLiteral("Chapter 1"), "row is named");
    QAccessible::State state = row->state();
    require(state.expandable && state.expanded && !state.collapsed, "open parent row reports expandable + expanded");

    auto* actions = row->actionInterface();
    require(actions && actions->actionNames().contains(QAccessibleActionInterface::showMenuAction()),
            "expandable row offers the action UI Automation's Expand/Collapse call");
    actions->doAction(QAccessibleActionInterface::showMenuAction());
    settle();
    state = row->state();
    require(state.expandable && state.collapsed && !state.expanded, "the action closes the row");

    auto* attributes = static_cast<QAccessibleAttributesInterface*>(row->interface_cast(QAccessible::AttributesInterface));
    require(attributes && attributes->attributeValue(QAccessible::Attribute::Locale).isValid(),
            "row keeps the control locale (Narrator chooses the voice from it)");

    // The focused list reports its current row as the focused child.
    QAccessibleInterface* view = QAccessible::queryAccessibleInterface(list);
    require(view && view->focusChild() == row, "focused list reports the current row as focused");
    list->setProperty("currentIndex", 1);
    settle();
    QAccessibleInterface* second = view->focusChild();
    require(second && second->text(QAccessible::Name) == QStringLiteral("Section 1.1"),
            "focus follows the current row");
    require(!second->state().expandable, "leaf row is not expandable");
    delete window;
}

void checkPopupName(QQmlEngine& engine) {
    QQuickWindow* window = load(engine, popupQml);
    QObject* popup = qvariant_cast<QObject*>(window->property("popup"));
    require(popup != nullptr, "popup exists");
    QMetaObject::invokeMethod(popup, "open");
    settle(400);
    QQuickItem* focused = window->activeFocusItem();
    require(focused != nullptr, "opening the popup gives it focus");
    QAccessibleInterface* iface = QAccessible::queryAccessibleInterface(focused);
    require(iface && iface->role() == QAccessible::Dialog, "focus lands on the dialog container");
    require(iface->text(QAccessible::Name) == QStringLiteral("About Atlas Reader"),
            "the dialog container is named after its dialog");
    delete window;
}

} // namespace

int main(int argc, char** argv) {
    QGuiApplication app{argc, argv};
    atlas::accessibility::installTreeItemAccessibility();
    try {
        QQmlEngine engine;
        checkTreeRow(engine);
        checkPopupName(engine);
    } catch (const std::exception& error) {
        std::cerr << "Accessibility test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Tree row, list focus and dialog name accessibility contracts pass.\n";
    return 0;
}
