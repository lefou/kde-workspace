/*  This file is part of the KDE project
    Copyright (C) 2007 Will Stephenson <wstephenson@kde.org>
    Copyright (C) 2007 Daniel Gollub <dgollub@suse.de>
    Copyright (C) 2008 Tom Patzig <tpatzig@suse.de>


    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef BLUEZ_BLUETOOTH_MANAGER_H
#define BLUEZ_BLUETOOTH_MANAGER_H

#include <QtDBus>
#include <QObject>
#include <qdbusextratypes.h>
#include <QStringList>

#include <kdemacros.h>

#include <solid/control/ifaces/bluetoothmanager.h>

//class BluezBluetoothSecurity;
class BluezBluetoothManagerPrivate;
class KDE_EXPORT BluezBluetoothManager : public Solid::Control::Ifaces::BluetoothManager
{
    Q_OBJECT

public:
    BluezBluetoothManager(QObject * parent, const QStringList  & args);
    virtual ~BluezBluetoothManager();
    QStringList bluetoothInterfaces() const;
    QObject * createInterface(const QString &);
//  QStringList bluetoothInputDevices() const;
    QString defaultInterface() const;
    QString findInterface(const QString &) const;

//  QObject * createBluetoothInputDevice(const QString &);
//  KJob *setupInputDevice(const QString &);
//  Solid::Control::Ifaces::BluetoothSecurity* security(const QString &interface);
public Q_SLOTS:
//  void removeInputDevice(const QString &);

protected Q_SLOTS:

    void slotDeviceAdded(const QDBusObjectPath &);
    void slotDeviceRemoved(const QDBusObjectPath &);
    void slotDefaultDeviceChanged(const QDBusObjectPath &);

//  void slotInputDeviceCreated(const QString &);
//  void slotInputDeviceRemoved(const QString &);

private:
    BluezBluetoothManagerPrivate * d;
//  QString m_inputManagerDest;
};

#endif

