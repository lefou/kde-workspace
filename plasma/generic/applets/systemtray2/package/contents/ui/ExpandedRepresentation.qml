/***************************************************************************
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.private.systemtray 2.0 as SystemTray

Item {

    function checkTask(task) {
        if (task.taskItemExpanded == null) return;
        var isthis = (task.taskId == root.currentTask);
        if (!isthis) {
            task.expanded = false;
        }
    }

    function clearExpanded() {
        var _hidden = host.hiddenTasks;
        for (var i = 0; i < _hidden.length; i++) {
            checkTask(_hidden[i]);
        }
        var _shown = host.shownTasks;
        for (i = 0; i < _shown.length; i++) {
            checkTask(_shown[i]);
        }
    }

    Connections {
        target: root
        onExpandedItemChanged: {
            if (root.expandedItem != null) {
                print("not null");

                root.expandedItem.parent = expandedItemContainer;
                root.expandedItem.anchors.fill = expandedItemContainer;
                expandedItemContainer.replace(root.expandedItem);
            } else {
                if (expandedItemContainer.currentPage != null) {
                    expandedItemContainer.clear();
                }
                root.currentTask = "";
            }
            clearExpanded();
        }
    }
    /* This mechanism hides inactive items until the layout figures out that it has to
     * collapse the Plasmoid.
     *
     * It also takes care of delaying the loading of hidden items (they're hidden, and
     * can be loaded later as to not block other, visible components from loading). It
     * does so by simply pushing them out of the viewport, which means the ListView won't
     * render the TaskDelegates until then.
     */
    Item {
        id: loadingItem
        anchors.fill: parent
        anchors.margins: height
        Timer {
            running: true
            interval: 4000
            onTriggered: loadingItem.visible = false
        }
    }

    MouseArea {
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
            right: expandedItemContainer.left
        }
        onClicked: {
            clearExpanded();
            expandedItemContainer.clear();
            root.currentTask = ""
            root.expandedItem = null;
        }
    }

    ListView {
        id: hiddenView
        objectName: "hiddenView"
        clip: true
        interactive: (contentHeight > height)
        width: parent.width

        anchors {
            top: (loadingItem.visible && !plasmoid.expanded) ? loadingItem.bottom : parent.top
            bottom: (loadingItem.visible && !plasmoid.expanded) ? undefined : parent.bottom
            left: parent.left
        }
        spacing: 4

        model: host.hiddenTasks

        delegate: TaskListDelegate {}
    }

    PlasmaCore.SvgItem {
        id: separator

        width: lineSvg.elementSize("vertical-line").width;
        height: parent.width;
        visible: root.expandedItem != null

        anchors {
            right: expandedItemContainer.left;
            top: parent.right;
            bottom: parent.bototm;
            rightMargin: root.smallSpacing;
        }
        elementId: "vertical-line";

        svg: PlasmaCore.Svg {
            id: lineSvg;
            imagePath: "widgets/line";
        }
    }

    Connections {
        target: plasmoid
        onExpandedChanged: {
            if (!plasmoid.expanded) {
                root.expandedItem = null;
                expandedItemContainer.clear();
                root.currentTask = "";
            }
            clearExpanded();
        }
    }

    PlasmaComponents.PageStack {
        id: expandedItemContainer
        anchors {
            left: parent.left
            leftMargin: root.itemSize + root.largeSpacing * 2
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }
    }
}