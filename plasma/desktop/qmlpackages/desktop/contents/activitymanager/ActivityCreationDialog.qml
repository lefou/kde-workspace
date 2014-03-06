/*   vim:set foldmethod=marker:
 *
 *   Copyright (C) 2014 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.2
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.activities 0.1 as Activities

PlasmaCore.FrameSvgItem {
    id: root

    signal accepted()
    signal canceled()

    imagePath: "widgets/extender-background"

    property alias activityName: activityNameText.text

    // titleText: i18n("Create a new activity")
    // titleIcon: "preferences-activities"

    // location: PlasmaCore.Types.LeftEdge

    height: content.height + margins.top + margins.bottom

    Column {
        id: content

        anchors {
            left: parent.left
            right: parent.right

            topMargin: root.margins.top
            leftMargin: root.margins.left
            bottomMargin: root.margins.bottom
            rightMargin: root.margins.right
        }

        PlasmaComponents.Label {
            text: i18n("Activity name:")

            elide: Text.ElideRight

            width: parent.width
        }

        PlasmaComponents.TextField {
            id: activityNameText

            width:  parent.width

            // text: ""
        }

        Item {
            height: units.largeSpacing
            width: parent.width
        }

        PlasmaComponents.ButtonRow {
            exclusive: false

            PlasmaComponents.Button {
                text: i18n("Create")
                iconSource: "list-add"
                onClicked: {
                    root.visible = false;
                    root.accepted();
                }
            }
            PlasmaComponents.Button {
                text: i18n("Cancel")
                iconSource: "dialog-close"
                onClicked: {
                    root.visible = false;
                    root.canceled();
                }
            }

        }

    }

    // buttonTexts: [i18n("Create"), i18n("Dismiss")]
}
