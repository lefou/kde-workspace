/***************************************************************************
 *   Copyright (C) 2007 by Alexis Ménard <darktears31@gmail.com>           *
 *   Copyright 2009 by Giulio Camuffo <giuliocamuffo@gmail.com>            *
 *   Copyright 2009 by Jacopo De Simoi <wilderkde@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "devicenotifier.h"

//Qt
#include <QGraphicsSceneContextMenuEvent>

//KDE
#include <KConfigDialog>
#include <KGlobal>
#include <KStandardDirs>
#include <KCModuleProxy>
#include <KCModuleInfo>
#include <kdesktopfileactions.h>

//Plasma
#include <Plasma/ToolTipManager>

//solid
#include <Solid/Device>
#include <Solid/StorageDrive>
#include <Solid/OpticalDisc>
#include <Solid/OpticalDrive>

//Own
#include "notifierdialog.h"
#include "deviceitem.h"
#include <Plasma/Containment>
using namespace Plasma;
using namespace Notifier;

static const char DEFAULT_ICON_NAME[] = "device-notifier";
static const int NOTIFICATION_TIMEOUT = 10000;

K_EXPORT_PLASMA_APPLET(devicenotifier, DeviceNotifier)

DeviceNotifier::DeviceNotifier(QObject *parent, const QVariantList &args)
    : Plasma::PopupApplet(parent, args),
      m_solidEngine(0),
      m_solidDeviceEngine(0),
      m_dialog(0),
      m_numberItems(0),
      m_itemsValidity(0),
      m_globalVisibility(false),
      m_checkHiddenDevices(true),
      m_autoMountingWidget(0),
      m_deviceActionsWidget(0)
{
    setBackgroundHints(StandardBackground);
    setAspectRatioMode(IgnoreAspectRatio);
    KGlobal::locale()->insertCatalog("solid_qt");

    // let's initialize the widget
    setMinimumSize(graphicsWidget()->minimumSize());
}

DeviceNotifier::~DeviceNotifier()
{
    delete m_dialog;
}

void DeviceNotifier::init()
{
    configChanged();
    
    m_solidEngine = dataEngine("hotplug");
    m_solidDeviceEngine = dataEngine("soliddevice");

    connect(m_dialog, SIGNAL(deviceSelected()), this, SLOT(showPopup()));

    Plasma::ToolTipManager::self()->registerWidget(this);

    setPopupIcon(DEFAULT_ICON_NAME);

    //connect to engine when a device is plugged in
    connect(m_solidEngine, SIGNAL(sourceAdded(const QString&)),
            this, SLOT(onSourceAdded(const QString&)));
    connect(m_solidEngine, SIGNAL(sourceRemoved(const QString&)),
            this, SLOT(onSourceRemoved(const QString&)));

    //feed the list with what is already reported by the engine
    fillPreviousDevices();

    if (m_lastPlugged.count() == 0) {
        setStatus(Plasma::PassiveStatus);
    } else {
        setStatus(Plasma::ActiveStatus);
    }
}

void DeviceNotifier::configChanged()
{
    KConfigGroup cg = config();
    m_numberItems = cg.readEntry("NumberItems", 4);
    m_itemsValidity = cg.readEntry("ItemsValidity", 5);
    m_showDevices = cg.readEntry("ShowDevices", (int)RemovableOnly);
}

QGraphicsWidget *DeviceNotifier::graphicsWidget()
{
    if (!m_dialog) {
        m_dialog = new NotifierDialog(this);
        connect(m_dialog, SIGNAL(actionSelected()), this, SLOT(hidePopup()));
        connect(m_dialog, SIGNAL(globalVisibilityChanged(bool)), this, SLOT(setGlobalVisibility(bool)));
    }

    return m_dialog->dialog();
}

void DeviceNotifier::fillPreviousDevices()
{
    m_fillingPreviousDevices = true;

    QList<Solid::Device> list = Solid::Device::listFromType(Solid::DeviceInterface::StorageVolume);
    foreach (const Solid::Device &device, list) {
        if (device.as<Solid::StorageVolume>()->isIgnored()) {
            deviceAdded(device, false);
        }
    }

    foreach (const QString &udi, m_solidEngine->sources()) {
        onSourceAdded(udi);
    }

    m_fillingPreviousDevices = false;
}

void DeviceNotifier::changeNotifierIcon(const QString& name)
{
    setPopupIcon(name.isNull() ? DEFAULT_ICON_NAME : name);
}

void DeviceNotifier::popupEvent(bool show)
{
    if (show) {
        Plasma::ToolTipManager::self()->clearContent(this);
    } else if (status() == Plasma::NeedsAttentionStatus) {
        setStatus(Plasma::ActiveStatus);
    } else {
        m_dialog->collapseDevices();
    }
    changeNotifierIcon();
}

void DeviceNotifier::dataUpdated(const QString &udi, Plasma::DataEngine::Data data)
{
    if (data.isEmpty()) {
        return;
    }

    //data from hotplug engine
    //kDebug() << data["udi"] << data["predicateFiles"].toStringList() << data["Device Types"].toStringList();

    //FIXME: here we rely on the fact that the hotplug engine gives a "text" field (and the soliddevice one does not)
    // to distinguish between data from the two engines. This is really not nice
    if (data["text"].isValid()) {
        //kDebug() << "adding" << data["udi"];
        int nb_actions = 0;
        QString lastActionLabel;
        QStringList currentActions = m_dialog->deviceActions(udi);
        QStringList newActions = data["predicateFiles"].toStringList();
        foreach (const QString &desktop, newActions) {
            QString filePath = KStandardDirs::locate("data", "solid/actions/" + desktop);
            QList<KServiceAction> services = KDesktopFileActions::userDefinedServices(filePath, true);
            nb_actions += services.size();

            if (!currentActions.contains(desktop)) {
                m_dialog->insertAction(udi, desktop);
            }
            if (services.size() > 0) {
                lastActionLabel = QString(services[0].text());
            }
        }
        foreach (const QString &action, currentActions) {
            if (!newActions.contains(action)) {
                m_dialog->removeAction(udi, action);
            }
        }

        m_dialog->setDeviceData(udi, data["text"], Qt::DisplayRole);
        m_dialog->setDeviceData(udi, data["isEncryptedContainer"], NotifierDialog::IsEncryptedContainer);

        if (nb_actions > 1) {
            QString s = i18np("1 action for this device",
                    "%1 actions for this device",
                    nb_actions);
            m_dialog->setDeviceData(udi, s, NotifierDialog::DescriptionRole);
        } else {
            m_dialog->setDeviceData(udi, lastActionLabel, NotifierDialog::DescriptionRole);
        }

    //data from soliddevice engine
    } else {

        //icon name
        m_dialog->setDeviceData(udi, data["Icon"], NotifierDialog::IconNameRole);
        m_dialog->setDeviceData(udi, KIcon(data["Icon"].toString(), NULL, data["Emblems"].toStringList()), Qt::DecorationRole);

        bool isOpticalMedia = data["Device Types"].toStringList().contains("OpticalDisc");

        m_dialog->setDeviceData(udi, isOpticalMedia, NotifierDialog::IsOpticalMedia);

        if (data["Device Types"].toStringList().contains("Storage Access")) {
            //kDebug() << "DeviceNotifier::solidDeviceEngine updated" << udi;
            if (data["Accessible"].toBool()) {
                m_dialog->setMounted(true, udi);
            } else {
                m_dialog->setMounted(false, udi);
            }

            if (data["Ignored"].toBool()) {
                m_dialog->setDeviceData(udi, data["File Path"], Qt::DisplayRole);

                const QString desktop("test-predicate-openinwindow.desktop");
                QString filePath = KStandardDirs::locate("data", "solid/actions/" + desktop);
                QList<KServiceAction> services = KDesktopFileActions::userDefinedServices(filePath, true);
                if (services.size() > 0) { //in case there is no action at all
                    m_dialog->insertAction(udi, desktop);
                    m_dialog->setDeviceData(udi, services[0].text(), NotifierDialog::DescriptionRole);
                }

                m_dialog->setDeviceLeftAction(udi, DeviceItem::Nothing);
            }
        } else if (data["Device Types"].toStringList().contains("Storage Volume")) {
            if (isOpticalMedia) {
                m_dialog->setMounted(true, udi);
            }
        }
    }
}

void DeviceNotifier::notifyDevice(const QString &udi)
{
    m_lastPlugged << udi;

    setStatus(Plasma::NeedsAttentionStatus);

    if (!m_fillingPreviousDevices) {
        emit activate();
        showPopup(NOTIFICATION_TIMEOUT);
        changeNotifierIcon("preferences-desktop-notification");
        update();
        QTimer::singleShot(NOTIFICATION_TIMEOUT, m_dialog, SLOT(resetNotifierIcon()));
    } else {
        setStatus(Plasma::ActiveStatus);
    }
}

void DeviceNotifier::toolTipAboutToShow()
{
    Plasma::ToolTipContent toolTip;
    if (m_lastPlugged.isEmpty()) {
        toolTip.setSubText(i18n("No devices available."));
        toolTip.setImage(KIcon("device-notifier"));
    } else {
        Solid::Device device(m_lastPlugged.last());
        toolTip.setSubText(i18n("Most recent device: %1", device.description()));
        toolTip.setImage(KIcon(device.icon()));
    }

    Plasma::ToolTipManager::self()->setContent(this, toolTip);
}

void DeviceNotifier::toolTipHidden()
{
    Plasma::ToolTipManager::self()->clearContent(this);
}

void DeviceNotifier::removeLastDeviceNotification(const QString &udi)
{
    m_lastPlugged.removeAll(udi);
}

void DeviceNotifier::onSourceAdded(const QString &udi)
{
    DataEngine::Data data = m_solidEngine->query(udi);
    Solid::Device device = Solid::Device(udi);
    deviceAdded(device, data["added"].toBool());
}

void DeviceNotifier::deviceAdded(const Solid::Device &device, bool hotplugged)
{
    const QString udi = device.udi();
    if (m_showDevices == NonRemovableOnly) {
        Solid::Device parentDevice = device.parent();
        Solid::StorageDrive *drive = parentDevice.as<Solid::StorageDrive>();
        if (drive && (drive->isHotpluggable() || drive->isRemovable())) {
            return;
        }
    } else if (m_showDevices == RemovableOnly) {
        Solid::Device device = Solid::Device(udi);
        Solid::Device parentDevice = device.parent();
        Solid::StorageDrive *drive = parentDevice.as<Solid::StorageDrive>();
        if (drive && (!drive->isHotpluggable() && !drive->isRemovable())) {
            return;
        }
    }

    kDebug() << "DeviceNotifier:: source added" << udi;
    KConfigGroup cg = config();
    bool visibility = cg.readEntry(udi, true);

    if (visibility || m_globalVisibility) {
        m_dialog->insertDevice(udi);

        if (hotplugged) {
            notifyDevice(udi);
        }

        m_dialog->setDeviceData(udi, visibility, NotifierDialog::VisibilityRole);

        m_solidEngine->connectSource(udi, this);
        m_solidDeviceEngine->connectSource(udi, this);

        m_lastPlugged << udi;
    }

    if (!visibility) {
        m_hiddenDevices << udi;
    }
}

void DeviceNotifier::onSourceRemoved(const QString &udi)
{
    m_solidEngine->disconnectSource(udi, this);
    m_solidDeviceEngine->disconnectSource(udi, this);

    m_dialog->removeDevice(udi);
    removeLastDeviceNotification(udi);
    if (m_checkHiddenDevices) {
        m_hiddenDevices.removeAll(udi);
    } else {
        m_checkHiddenDevices = false;
    }
    if (m_lastPlugged.count() == 0) {
        setStatus(Plasma::PassiveStatus);
    } else {
        setStatus(Plasma::ActiveStatus);
    }
}

void DeviceNotifier::resetDevices()
{
    while (m_lastPlugged.count() > 0) {
        QString udi = m_lastPlugged.takeAt(0);
        onSourceRemoved(udi);
    }

    fillPreviousDevices();
}

void DeviceNotifier::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *configurationWidget = new QWidget();
    m_configurationUi.setupUi(configurationWidget);
    m_deviceActionsWidget = new KCModuleProxy("solid-actions");
    m_autoMountingWidget = new KCModuleProxy("device_automounter_kcm");

    parent->addPage(configurationWidget, i18n("Display"), icon());
    parent->addPage(m_deviceActionsWidget, m_deviceActionsWidget->moduleInfo().moduleName(), 
                    m_deviceActionsWidget->moduleInfo().icon());
    parent->addPage(m_autoMountingWidget, i18n("Automounting"),
                    m_autoMountingWidget->moduleInfo().icon());

    parent->setButtons( KDialog::Ok | KDialog::Cancel);
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));

    switch (m_showDevices) {
        case RemovableOnly:
            m_configurationUi.removableDevices->setChecked(true);
            break;
        case NonRemovableOnly:
            m_configurationUi.nonRemovableDevices->setChecked(true);
            break;
        case AllDevices:
            m_configurationUi.allDevices->setChecked(true);
            break;
    }
}

void DeviceNotifier::configAccepted()
{
    KConfigGroup cg = config();

    if (m_configurationUi.allDevices->isChecked()) {
        m_showDevices = AllDevices;
    } else if (m_configurationUi.nonRemovableDevices->isChecked()) {
        m_showDevices = NonRemovableOnly;
    } else {
        m_showDevices = RemovableOnly;
    }

    cg.writeEntry("ShowDevices", m_showDevices);

    //Save the configurations of the embedded KCMs
    m_deviceActionsWidget->save();
    m_autoMountingWidget->save();

    emit configNeedsSaving();

    resetDevices();
}

void DeviceNotifier::setDeviceVisibility(const QString &udi, bool visibility)
{
    m_dialog->setDeviceData(udi, visibility, NotifierDialog::VisibilityRole);
    m_checkHiddenDevices = false;
    if (visibility) {
        m_hiddenDevices.removeAll(udi);
    } else {
        m_hiddenDevices << udi;
    }

    if (!visibility && !m_globalVisibility) {
        onSourceRemoved(udi);
    }

    KConfigGroup cg = config();
    cg.writeEntry(udi, visibility);
}

void DeviceNotifier::setGlobalVisibility(bool visibility)
{
    m_globalVisibility = visibility;
    resetDevices();
}

void DeviceNotifier::showErrorMessage(const QString &message, const QString &details)
{
    m_dialog->showStatusBarMessage(message, details);
    showPopup(NOTIFICATION_TIMEOUT);
    changeNotifierIcon("dialog-error");
    update();
    QTimer::singleShot(NOTIFICATION_TIMEOUT, m_dialog, SLOT(resetNotifierIcon()));
}

bool DeviceNotifier::areThereHiddenDevices()
{
    return (m_hiddenDevices.count() > 0);
}

void DeviceNotifier::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    m_dialog->setMenuActionsAt(event->scenePos());

    PopupApplet::contextMenuEvent(event);
}

QList<QAction *> DeviceNotifier::contextualActions()
{
    return m_dialog->contextualActions();
}

#include "devicenotifier.moc"
