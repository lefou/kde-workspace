/*
 *   Copyright 2011 Sebastian Kügler <sebas@kde.org>
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2013 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 1.1
import org.kde.plasma.core 0.1 as PlasmaCore
import "plasmapackage:/code/logic.js" as Logic
import "plasmapackage:/code/platform.js" as Platform

Item {
    id: batterymonitor
    property int minimumWidth: 450
    property int minimumHeight: dialogItem.actualHeight

    property bool show_remaining_time: false
    property bool show_suspend_buttons: false

    PlasmaCore.Theme { id: theme }

    Component.onCompleted: {
        plasmoid.aspectRatioMode = IgnoreAspectRatio
        Logic.updateCumulative();
        Logic.updateTooltip();
        Logic.updateBrightness();
        plasmoid.addEventListener('ConfigChanged', configChanged);
    }

    function configChanged() {
        show_remaining_time = plasmoid.readConfig("showRemainingTime");
        show_suspend_buttons = plasmoid.readConfig("showSuspendButtons");
    }

    property Component compactRepresentation: CompactRepresentation {
        hasBattery: pmSource.data["Battery"]["Has Battery"]
        pmSource: plasmoid.rootItem.pmSource
        batteries: plasmoid.rootItem.batteries
        singleBattery: isConstrained() || !hasBattery
    }

    property QtObject pmSource: PlasmaCore.DataSource {
        id: pmSource
        engine: "powermanagement"
        connectedSources: sources
        onDataChanged: {
            Logic.updateTooltip();
            Logic.updateBrightness();
        }
        onSourceAdded: {
            if (source == "Battery0") {
                disconnectSource(source);
                connectSource(source);
            }
        }
        onSourceRemoved: {
            if (source == "Battery0") {
                disconnectSource(source);
            }
        }
    }

    property QtObject batteries: PlasmaCore.DataModel {
        id: batteries
        dataSource: pmSource
        sourceFilter: "Battery[0-9]+"

        onDataChanged: {
            Logic.updateCumulative();

            var status = "PassiveStatus";
            if (batteries.cumulativePluggedin) {
                if (batteries.cumulativePercent <= 10) {
                    status = "NeedsAttentionStatus";
                } else if (!batteries.allCharged) {
                    status = "ActiveStatus";
                }
            }
            plasmoid.status = status;
        }

        property int cumulativePercent
        property bool cumulativePluggedin: count > 0
        // true  --> all batteries charged
        // false --> one of the batteries charging/discharging
        property bool allCharged
        property string tooltipText
    }

    PopupDialog {
        id: dialogItem
        property bool disableBrightnessUpdate: false
        model: batteries
        anchors.fill: parent
        //hasBattery: batteries.cumulativePluggedin
        showRemainingTime: show_remaining_time
        showSuspendButtons: show_suspend_buttons

        isBrightnessAvailable: pmSource.data["PowerDevil"]["Screen Brightness Available"]
        isKeyboardBrightnessAvailable: pmSource.data["PowerDevil"]["Keyboard Brightness Available"]

        pluggedIn: pmSource.data["AC Adapter"]["Plugged in"]
        remainingMsec: Number(pmSource.data["Battery"]["Remaining msec"])

        offerSuspend: true//Platform.shouldOfferSuspend(pmSource)
        offerHibernate: true//Platform.shouldOfferHibernate(pmSource)

        onSuspendClicked: {
            plasmoid.togglePopup();
            service = pmSource.serviceForSource("PowerDevil");
            var operationName = Logic.callForType(type);
            operation = service.operationDescription(operationName);
            service.startOperationCall(operation);
        }
        onBrightnessChanged: {
            if (disableBrightnessUpdate) {
                return;
            }
            service = pmSource.serviceForSource("PowerDevil");
            operation = service.operationDescription("setBrightness");
            operation.brightness = screenBrightness;
            service.startOperationCall(operation);
        }
        onKeyboardBrightnessChanged: {
            if (disableBrightnessUpdate) {
                return;
            }
            service = pmSource.serviceForSource("PowerDevil");
            operation = service.operationDescription("setKeyboardBrightness");
            operation.brightness = keyboardBrightness;
            service.startOperationCall(operation);
        }
        property int cookie1: -1
        property int cookie2: -1
        onPowermanagementChanged: {
            service = pmSource.serviceForSource("PowerDevil");
            if (checked) {
                var op1 = service.operationDescription("stopSuppressingSleep");
                op1.cookie = cookie1;
                var op2 = service.operationDescription("stopSuppressingScreenPowerManagement");
                op2.cookie = cookie2;

                var job1 = service.startOperationCall(op1);
                job1.finished.connect(function(job) {
                    cookie1 = -1;
                });

                var job2 = service.startOperationCall(op2);
                job2.finished.connect(function(job) {
                    cookie2 = -1;
                });
            } else {
                var reason = i18n("The battery applet has enabled system-wide inhibition");
                var op1 = service.operationDescription("beginSuppressingSleep");
                op1.reason = reason;
                var op2 = service.operationDescription("beginSuppressingScreenPowerManagement");
                op2.reason = reason;

                var job1 = service.startOperationCall(op1);
                job1.finished.connect(function(job) {
                    cookie1 = job.result;
                });

                var job2 = service.startOperationCall(op2);
                job2.finished.connect(function(job) {
                    cookie2 = job.result;
                });
            }
        }
    }
}
