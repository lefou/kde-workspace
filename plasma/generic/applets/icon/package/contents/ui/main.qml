/*
 * Copyright 2013  Bhushan Shah <bhush94@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components
import org.kde.qtextracomponents 2.0
import org.kde.plasma.icon 1.0

MouseArea {
    id:root

    height: 50
    width: 50

    property int minimumWidth: formFactor == PlasmaCore.Types.Horizontal ? height : 1
    property int minimumHeight: formFactor == PlasmaCore.Types.Vertical ? width  : 1
    property int formFactor: plasmoid.formFactor
    property bool constrained: formFactor==PlasmaCore.Types.Vertical||formFactor==PlasmaCore.Types.Horizontal
    hoverEnabled: true
    onClicked: Qt.openUrlExternally(plasmoid.configuration.url);

    Component.onCompleted: {
        plasmoid.backgroundHints = 0;
        plasmoid.popupIcon = plasmoid.configuration.iconName;
        plasmoid.aspectRatioMode = IgnoreAspectRatio;
    }

    PlasmaCore.IconItem {
        id:icon
        source: plasmoid.configuration.iconName
        anchors{
            left : parent.left
            right : parent.right
            top : parent.top
            bottom : constrained ? parent.bottom : text.top
        }
        active: root.containsMouse
    }

    Components.Label {
        id : text
        text : plasmoid.configuration.applicationName
        anchors {
            left : parent.left
            bottom : parent.bottom
            right : parent.right
        }
        horizontalAlignment : Text.AlignHCenter
        opacity : constrained ? 0 : 1
    }

    PlasmaCore.ToolTip {
        target : root
        mainText : plasmoid.configuration.applicationName
        subText : plasmoid.configuration.genericName
        image : plasmoid.configuration.iconName
    }

    Logic {
	id: logic
    }

    Connections {
	target: plasmoid
	onExternalData: {
	    print("************************************************************************************************");
	    print(data);
	    plasmoid.configuration.url = data;
	    plasmoid.configuration.applicationName = logic.getName(data);
	    plasmoid.configuration.iconName = logic.getIcon(data);
	    plasmoid.configuration.genericName = logic.getGenericName(data);
	    print(plasmoid.configuration.iconName);
	    print("************************************************************************************************");
	}
    }
}