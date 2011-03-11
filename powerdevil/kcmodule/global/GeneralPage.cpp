/***************************************************************************
 *   Copyright (C) 2008 by Dario Freddi <drf@kde.org>                      *
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

#include "GeneralPage.h"

#include "PowerDevilSettings.h"

#include <Solid/Device>
#include <Solid/DeviceInterface>
#include <Solid/Battery>
#include <Solid/PowerManagement>

#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMetaType>

#include <KNotifyConfigWidget>
#include <KPluginFactory>
#include <KAboutData>

K_PLUGIN_FACTORY(PowerDevilGeneralKCMFactory,
                 registerPlugin<GeneralPage>();
                )
K_EXPORT_PLUGIN(PowerDevilGeneralKCMFactory("powerdevilglobalconfig","powerdevil"))

typedef QMap< QString, QString > StringStringMap;
Q_DECLARE_METATYPE(StringStringMap)

GeneralPage::GeneralPage(QWidget *parent, const QVariantList &args)
        : KCModule(PowerDevilGeneralKCMFactory::componentData(), parent, args)
{
    qDBusRegisterMetaType< StringStringMap >();

    setButtons(Apply | Help);

    KAboutData *about =
        new KAboutData("powerdevilglobalconfig", "powerdevilglobalconfig", ki18n("Global Power Management Configuration"),
                       "", ki18n("A global power management configurator for KDE Power Management System"),
                       KAboutData::License_GPL, ki18n("(c), 2010 Dario Freddi"),
                       ki18n("From this module, you can configure the main Power Management daemon, assign profiles to "
                             "states, and do some advanced fine tuning on battery handling"));

    about->addAuthor(ki18n("Dario Freddi"), ki18n("Maintainer") , "drf@kde.org",
                     "http://drfav.wordpress.com");

    setAboutData(about);

    setupUi(this);

    fillUi();

    // Connect to daemon's signal
    QDBusConnection::sessionBus().connect("org.kde.Solid.PowerManagement", "/org/kde/Solid/PowerManagement",
                                          "org.kde.Solid.PowerManagement", "configurationReloaded",
                                          this, SLOT(reloadAvailableProfiles()));
}

GeneralPage::~GeneralPage()
{
}

void GeneralPage::fillUi()
{
    int batteryCount = 0;

    foreach(const Solid::Device &device, Solid::Device::listFromType(Solid::DeviceInterface::Battery, QString())) {
        const Solid::Battery *b = qobject_cast<const Solid::Battery*> (device.asDeviceInterface(Solid::DeviceInterface::Battery));
        if(b->type() != Solid::Battery::PrimaryBattery && b->type() != Solid::Battery::UpsBattery) {
            continue;
        }
        ++batteryCount;
    }

    eventsIconLabel->setPixmap(KIcon("preferences-desktop-notification").pixmap(24));
    profileIconLabel->setPixmap(KIcon("preferences-system-power-management").pixmap(24));

    reloadAvailableProfiles();

    tabWidget->setTabIcon(0, KIcon("preferences-other"));
    tabWidget->setTabIcon(1, KIcon("battery"));

    QSet< Solid::PowerManagement::SleepState > methods = Solid::PowerManagement::supportedSleepStates();

    BatteryCriticalCombo->addItem(KIcon("dialog-cancel"), i18n("Do nothing"), 0);
    if (methods.contains(Solid::PowerManagement::SuspendState)) {
        BatteryCriticalCombo->addItem(KIcon("system-suspend"), i18n("Sleep"), 1);
    }
    if (methods.contains(Solid::PowerManagement::HibernateState)) {
        BatteryCriticalCombo->addItem(KIcon("system-suspend-hibernate"), i18n("Hibernate"), 2);
    }
    BatteryCriticalCombo->addItem(KIcon("system-shutdown"), i18n("Shutdown"), 8);

    notificationsButton->setIcon(KIcon("preferences-desktop-notification"));

    // modified fields...

    connect(lockScreenOnResume, SIGNAL(stateChanged(int)), SLOT(changed()));

    connect(notificationsButton, SIGNAL(clicked()), SLOT(configureNotifications()));

    connect(lowSpin, SIGNAL(valueChanged(int)), SLOT(changed()));
    connect(warningSpin, SIGNAL(valueChanged(int)), SLOT(changed()));
    connect(criticalSpin, SIGNAL(valueChanged(int)), SLOT(changed()));

    connect(BatteryCriticalCombo, SIGNAL(currentIndexChanged(int)), SLOT(changed()));

    connect(acProfile, SIGNAL(currentIndexChanged(int)), SLOT(changed()));
    connect(lowProfile, SIGNAL(currentIndexChanged(int)), SLOT(changed()));
    connect(warningProfile, SIGNAL(currentIndexChanged(int)), SLOT(changed()));
    connect(batteryProfile, SIGNAL(currentIndexChanged(int)), SLOT(changed()));

    // Disable stuff, eventually
    if (batteryCount == 0) {
        batteryProfile->setEnabled(false);
        lowProfile->setEnabled(false);
        warningProfile->setEnabled(false);
        tabWidget->setTabEnabled(1, false);
    }
}

void GeneralPage::load()
{
    lockScreenOnResume->setChecked(PowerDevilSettings::configLockScreen());

    lowSpin->setValue(PowerDevilSettings::batteryLowLevel());
    warningSpin->setValue(PowerDevilSettings::batteryWarningLevel());
    criticalSpin->setValue(PowerDevilSettings::batteryCriticalLevel());

    BatteryCriticalCombo->setCurrentIndex(BatteryCriticalCombo->findData(PowerDevilSettings::batteryCriticalAction()));

    acProfile->setCurrentIndex(acProfile->findData(PowerDevilSettings::aCProfile()));
    lowProfile->setCurrentIndex(lowProfile->findData(PowerDevilSettings::lowProfile()));
    warningProfile->setCurrentIndex(warningProfile->findData(PowerDevilSettings::warningProfile()));
    batteryProfile->setCurrentIndex(batteryProfile->findData(PowerDevilSettings::batteryProfile()));
}

void GeneralPage::configureNotifications()
{
    KNotifyConfigWidget::configure(this, "powerdevil");
}

void GeneralPage::save()
{
    PowerDevilSettings::setConfigLockScreen(lockScreenOnResume->isChecked());

    PowerDevilSettings::setBatteryLowLevel(lowSpin->value());
    PowerDevilSettings::setBatteryWarningLevel(warningSpin->value());
    PowerDevilSettings::setBatteryCriticalLevel(criticalSpin->value());

    PowerDevilSettings::setBatteryCriticalAction(BatteryCriticalCombo->itemData(BatteryCriticalCombo->currentIndex()).toInt());

    PowerDevilSettings::setACProfile(acProfile->itemData(acProfile->currentIndex()).toString());
    PowerDevilSettings::setLowProfile(lowProfile->itemData(lowProfile->currentIndex()).toString());
    PowerDevilSettings::setWarningProfile(warningProfile->itemData(warningProfile->currentIndex()).toString());
    PowerDevilSettings::setBatteryProfile(batteryProfile->itemData(batteryProfile->currentIndex()).toString());

    PowerDevilSettings::self()->writeConfig();

    // Notify Daemon
    QDBusMessage call = QDBusMessage::createMethodCall("org.kde.Solid.PowerManagement", "/org/kde/Solid/PowerManagement",
                                                       "org.kde.Solid.PowerManagement", "refreshStatus");

    // Perform call
    QDBusConnection::sessionBus().asyncCall(call);

    // And now we are set with no change
    emit changed(false);
}

void GeneralPage::reloadAvailableProfiles()
{
    KSharedConfigPtr profilesConfig = KSharedConfig::openConfig("powerdevil2profilesrc", KConfig::SimpleConfig);

    // Request profiles to the daemon
    QDBusMessage call = QDBusMessage::createMethodCall("org.kde.Solid.PowerManagement", "/org/kde/Solid/PowerManagement",
                                                       "org.kde.Solid.PowerManagement", "availableProfiles");
    QDBusPendingReply< StringStringMap > reply = QDBusConnection::sessionBus().asyncCall(call);
    reply.waitForFinished();

    if (!reply.isValid()) {
        kDebug() << "Error contacting the daemon!";
        return;
    }

    StringStringMap profiles = reply.value();

    if (profiles.isEmpty()) {
        kDebug() << "No available profiles!";
        return;
    }

    acProfile->clear();
    batteryProfile->clear();
    lowProfile->clear();
    warningProfile->clear();

    if (profilesConfig->groupList().isEmpty()) {
        kDebug() << "No available profiles!";
        return;
    }

    for (StringStringMap::const_iterator i = profiles.constBegin(); i != profiles.constEnd(); ++i) {
        KConfigGroup group(profilesConfig, i.key());

        acProfile->addItem(KIcon(group.readEntry("icon")), i.value(), i.key());
        batteryProfile->addItem(KIcon(group.readEntry("icon")), i.value(), i.key());
        lowProfile->addItem(KIcon(group.readEntry("icon")), i.value(), i.key());
        warningProfile->addItem(KIcon(group.readEntry("icon")), i.value(), i.key());
    }

    acProfile->setCurrentIndex(acProfile->findData(PowerDevilSettings::aCProfile()));
    lowProfile->setCurrentIndex(acProfile->findData(PowerDevilSettings::lowProfile()));
    warningProfile->setCurrentIndex(acProfile->findData(PowerDevilSettings::warningProfile()));
    batteryProfile->setCurrentIndex(acProfile->findData(PowerDevilSettings::batteryProfile()));

}

void GeneralPage::defaults()
{
    KCModule::defaults();
}

#include "GeneralPage.moc"
