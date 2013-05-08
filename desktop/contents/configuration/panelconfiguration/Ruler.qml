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


PlasmaCore.FrameSvgItem {
    id: root
    imagePath: "widgets/containment-controls"
    state: "BottomEdge"
    implicitWidth: offsetButton.implicitwidth + minimumLengthHandle.implicitwidth
    implicitHeight: 32//offsetButton.implicitheight + minimumLengthHandle.implicitheight
    PlasmaCore.Svg {
        id: containmentControlsSvg
        imagePath: "widgets/containment-controls"
    }
    OffsetHandle {
        id: offsetButton
    }
    MinimumLengthHandle {
        id: minimumLengthHandle
    }
    MaximumLengthHandle {
        id: maximumLengthHandle
    }
    
    states: [
        State {
            name: "TopEdge"
            PropertyChanges {
                target: root
                prefix: "north"
            }
            AnchorChanges {
                target: root
                anchors {
                    top: root.parent.top
                    bottom: undefined
                    left: root.parent.left
                    right: root.parent.right
                }
            }
            AnchorChanges {
                target: offsetHandle
                anchors {
                    top: undefined
                    bottom: root.bottom
                    left: undefined
                    right: undefined
                }
            }
            AnchorChanges {
                target: minimumLengthHandle
                anchors {
                    top: root.top
                    bottom: undefined
                    left: undefined
                    right: undefined
                }
            }
            AnchorChanges {
                target: maximumLengthHandle
                anchors {
                    top: undefined
                    bottom: root.bottom
                    left: undefined
                    right: undefined
                }
            }
        },
        State {
            name: "BottomEdge"
            PropertyChanges {
                target: root
                prefix: "south"
            }
            AnchorChanges {
                target: root
                anchors {
                    top: undefined
                    bottom: root.parent.bottom
                    left: root.parent.left
                    right: root.parent.right
                }
            }
            AnchorChanges {
                target: offsetHandle
                anchors {
                    top: root.top
                    bottom: undefined
                    left: undefined
                    right: undefined
                }
            }
            AnchorChanges {
                target: minimumLengthHandle
                anchors {
                    top: undefined
                    bottom: root.bottom
                    left: undefined
                    right: undefined
                }
            }
            AnchorChanges {
                target: maximumLengthHandle
                anchors {
                    top: root.top
                    bottom: undefined
                    left: undefined
                    right: undefined
                }
            }
        },
        State {
            name: "LeftEdge"
            PropertyChanges {
                target: root
                prefix: "west"
            }
            AnchorChanges {
                target: root
                anchors {
                    top: root.parent.top
                    bottom: root.parent.bottom
                    left: root.parent.left
                    right: undefined
                }
            }
            AnchorChanges {
                target: offsetHandle
                anchors {
                    top: undefined
                    bottom: undefined
                    left: undefined
                    right: root.right
                }
            }
            AnchorChanges {
                target: minimumLengthHandle
                anchors {
                    top: undefined
                    bottom: undefined
                    left: root.left
                    right: undefined
                }
            }
            AnchorChanges {
                target: maximumLengthHandle
                anchors {
                    top: undefined
                    bottom: undefined
                    left: undefined
                    right: root.right
                }
            }
        },
        State {
            name: "RightEdge"
            PropertyChanges {
                target: root
                prefix: "east"
            }
            AnchorChanges {
                target: root
                anchors {
                    top: root.parent.top
                    bottom: root.parent.bottom
                    left: undefined
                    right: root.parent.right
                }
            }
            AnchorChanges {
                target: offsetHandle
                anchors {
                    top: undefined
                    bottom: undefined
                    left: parent.left
                    right: undefined
                }
            }
            AnchorChanges {
                target: minimumLengthHandle
                anchors {
                    top: undefined
                    bottom: undefined
                    left: undefined
                    right: parent.right
                }
            }
            AnchorChanges {
                target: maximumLengthHandle
                anchors {
                    top: undefined
                    bottom: undefined
                    left: parent.left
                    right: undefined
                }
            }
        }
    ]
}
