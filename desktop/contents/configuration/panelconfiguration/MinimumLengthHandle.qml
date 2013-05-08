/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.configuration 2.0
import org.kde.qtextracomponents 2.0 as QtExtras


PlasmaCore.SvgItem {
    id: root
    svg: containmentControlsSvg
    state: parent.state
    width: naturalSize.width
    height: naturalSize.height

    property int value
    onValueChanged: {
        if (panel.location == 5 || panel.location == 6) {
            y = panel.minimumLength + panel.offset
        } else {
            x = panel.minimumLength + panel.offset
        }
    }

    MouseArea {
        drag {
            target: parent
            axis: (panel.location == 5 || panel.location == 6) ? Drag.YAxis : Drag.XAxis
        }
        anchors.fill: parent
        onPositionChanged: {
            if (panel.location == 5 || panel.location == 6) {
                panel.minimumLength = parent.y - panel.offset
            } else {
                panel.minimumLength = parent.x - panel.offset
            }
        }
        Component.onCompleted: {
            if (panel.location == 5 || panel.location == 6) {
                parent.y = panel.minimumLength + panel.offset
            } else {
                parent.x = panel.minimumLength + panel.offset
            }
        }
    }

    states: [
        State {
            name: "TopEdge"
            PropertyChanges {
                target: root
                elementId: "north-minslider"
            }
        },
        State {
            name: "BottomEdge"
            PropertyChanges {
                target: root
                elementId: "south-minslider"
            }
        },
        State {
            name: "LeftEdge"
            PropertyChanges {
                target: root
                elementId: "west-minslider"
            }
        },
        State {
            name: "RightEdge"
            PropertyChanges {
                target: root
                elementId: "east-minslider"
            }
        }
    ]
}
