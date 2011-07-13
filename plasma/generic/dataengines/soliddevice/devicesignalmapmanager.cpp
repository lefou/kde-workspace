/*
 *   Copyright (C) 2007 Christopher Blauvelt <cblauvelt@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#include "devicesignalmapmanager.h"

DeviceSignalMapManager::DeviceSignalMapManager(QObject *parent) : QObject(parent)
{
    user = parent;
}

DeviceSignalMapManager::~DeviceSignalMapManager()
{
}

void DeviceSignalMapManager::mapDevice(Solid::AcAdapter *ac, const QString &udi)
{
    AcAdapterSignalMapper *map=0;
    if (!signalmap.contains(Solid::DeviceInterface::AcAdapter)) {
        signalmap[Solid::DeviceInterface::AcAdapter] = new AcAdapterSignalMapper(this);
    }
    map = (AcAdapterSignalMapper*)signalmap[Solid::DeviceInterface::AcAdapter];

    connect(ac, SIGNAL(plugStateChanged(bool, const QString &)), map, SLOT(plugStateChanged(bool)));
    connect(map, SIGNAL(deviceChanged(const QString&, const QString &, QVariant)), user, SLOT(deviceChanged(const QString&, const QString &, QVariant)));
    map->setMapping(ac, udi);
}

void DeviceSignalMapManager::mapDevice(Solid::Button *button, const QString &udi)
{
    ButtonSignalMapper *map=0;
    if (!signalmap.contains(Solid::DeviceInterface::Button)) {
        signalmap[Solid::DeviceInterface::Button] = new ButtonSignalMapper(this);
    }
    map = (ButtonSignalMapper*)signalmap[Solid::DeviceInterface::Button];

    connect(button, SIGNAL(pressed(Solid::Button::ButtonType, const QString &)), map, SLOT(pressed(Solid::Button::ButtonType)));
    connect(map, SIGNAL(deviceChanged(const QString&, const QString &, QVariant)), user, SLOT(deviceChanged(const QString&, const QString &, QVariant)));
    map->setMapping(button, udi);
}

void DeviceSignalMapManager::mapDevice(Solid::Battery *battery, const QString &udi)
{
    BatterySignalMapper *map=0;
    if (!signalmap.contains(Solid::DeviceInterface::Battery)) {
        signalmap[Solid::DeviceInterface::Battery] = new BatterySignalMapper(this);
    }
    map = (BatterySignalMapper*)signalmap[Solid::DeviceInterface::Battery];

    connect(battery, SIGNAL(chargePercentChanged(int, const QString &)), map, SLOT(chargePercentChanged(int)));
    connect(map, SIGNAL(deviceChanged(const QString&, const QString &, QVariant)), user, SLOT(deviceChanged(const QString&, const QString &, QVariant)));
    connect(battery, SIGNAL(chargeStateChanged(int, const QString &)), map, SLOT(chargeStateChanged(int)));
    connect(map, SIGNAL(deviceChanged(const QString&, const QString &, QVariant)), user, SLOT(deviceChanged(const QString&, const QString &, QVariant)));
    connect(battery, SIGNAL(plugStateChanged(bool, const QString &)), map, SLOT(plugStateChanged(bool)));
    connect(map, SIGNAL(deviceChanged(const QString&, const QString &, QVariant)), user, SLOT(deviceChanged(const QString&, const QString &, QVariant)));
    map->setMapping(battery, udi);
}

void DeviceSignalMapManager::mapDevice(Solid::StorageAccess *storageaccess, const QString &udi)
{
    StorageAccessSignalMapper *map=0;
    if (!signalmap.contains(Solid::DeviceInterface::StorageAccess)) {
        signalmap[Solid::DeviceInterface::StorageAccess] = new StorageAccessSignalMapper(this);
    }
    map = (StorageAccessSignalMapper*)signalmap[Solid::DeviceInterface::StorageAccess];

    connect(storageaccess, SIGNAL(accessibilityChanged(bool, const QString &)), map, SLOT(accessibilityChanged(bool)));
    connect(map, SIGNAL(deviceChanged(const QString&, const QString &, QVariant)), user, SLOT(deviceChanged(const QString&, const QString &, QVariant)));
    map->setMapping(storageaccess, udi);
}

void DeviceSignalMapManager::unmapDevice(Solid::AcAdapter *ac)
{
    AcAdapterSignalMapper *map = (AcAdapterSignalMapper*)signalmap.value(Solid::DeviceInterface::AcAdapter);
    if (!map) {
        return;
    }

    disconnect(ac, SIGNAL(plugStateChanged(bool, const QString &)), map, SLOT(plugStateChanged(bool)));
    disconnect(map, SIGNAL(deviceChanged(const QString&, const QString &, QVariant)), user, SLOT(deviceChanged(const QString&, const QString &, QVariant)));
}

void DeviceSignalMapManager::unmapDevice(Solid::Button *button)
{
    ButtonSignalMapper *map = (ButtonSignalMapper*)signalmap.value(Solid::DeviceInterface::Button);
    if (!map) {
        return;
    }

    disconnect(button, SIGNAL(pressed(Solid::Button::ButtonType, const QString &)), map, SLOT(pressed(Solid::Button::ButtonType)));
    disconnect(map, SIGNAL(deviceChanged(const QString&, const QString &, QVariant)), user, SLOT(deviceChanged(const QString&, const QString &, QVariant)));
}

void DeviceSignalMapManager::unmapDevice(Solid::Battery *battery)
{
    BatterySignalMapper *map = (BatterySignalMapper*)signalmap.value(Solid::DeviceInterface::Battery);
    if (!map) {
        return;
    }

    disconnect(battery, SIGNAL(chargePercentChanged(int, const QString &)), map, SLOT(chargePercentChanged(int)));
    disconnect(map, SIGNAL(deviceChanged(const QString&, const QString &, QVariant)), user, SLOT(deviceChanged(const QString&, const QString &, QVariant)));
    disconnect(battery, SIGNAL(chargeStateChanged(int, const QString &)), map, SLOT(chargeStateChanged(int)));
    disconnect(map, SIGNAL(deviceChanged(const QString&, const QString &, QVariant)), user, SLOT(deviceChanged(const QString&, const QString &, QVariant)));
    disconnect(battery, SIGNAL(plugStateChanged(bool, const QString &)), map, SLOT(plugStateChanged(bool)));
    disconnect(map, SIGNAL(deviceChanged(const QString&, const QString &, QVariant)), user, SLOT(deviceChanged(const QString&, const QString &, QVariant)));
}

void DeviceSignalMapManager::unmapDevice(Solid::StorageAccess *storageaccess)
{
    StorageAccessSignalMapper *map = (StorageAccessSignalMapper*)signalmap.value(Solid::DeviceInterface::StorageAccess);
    if (!map) {
        return;
    }

    disconnect(storageaccess, SIGNAL(accessibilityChanged(bool, const QString &)), map, SLOT(accessibilityChanged(bool)));
    disconnect(map, SIGNAL(deviceChanged(const QString&, const QString &, QVariant)), user, SLOT(deviceChanged(const QString&, const QString &, QVariant)));
}

#include "devicesignalmapmanager.moc"
