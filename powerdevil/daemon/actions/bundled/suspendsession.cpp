/***************************************************************************
 *   Copyright (C) 2010 by Dario Freddi <drf@kde.org>                      *
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

#include "suspendsession.h"

#include "powerdevilbackendinterface.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KJob>
#include <KDebug>

#include <kworkspace/kworkspace.h>

#include "screensaver_interface.h"
#include <PowerDevilSettings.h>

namespace PowerDevil
{
namespace BundledActions
{

SuspendSession::SuspendSession(QObject* parent)
    : Action(parent)
{
    setRequiredPolicies(PowerDevil::PolicyAgent::InterruptSession);
}

SuspendSession::~SuspendSession()
{

}

void SuspendSession::onProfileUnload()
{
    // Nothing to do
}

void SuspendSession::onWakeupFromIdle()
{
    // Nothing to do
}

void SuspendSession::onIdleTimeout(int msec)
{
    Q_UNUSED(msec);
    QVariantMap args;
    args.insert("Type", m_autoType);
    trigger(args);
}

void SuspendSession::onProfileLoad()
{
    // Nothing to do
}

void SuspendSession::triggerImpl(const QVariantMap& args)
{
    kDebug() << "Triggered with " << args["Type"].toString();

    KJob *suspendJob = 0;
    if (args["Type"].toString() == "Suspend") {
        // Do we want to lock the screen?
        if (PowerDevilSettings::configLockScreen()) {
            // Yeah, we do.
            QVariantMap args;
            args["Type"] = "LockScreen";
            triggerImpl(args);
        }
        suspendJob = backend()->suspend(PowerDevil::BackendInterface::ToRam);
    } else if (args["Type"].toString() == "ToDisk") {
        // Do we want to lock the screen?
        if (PowerDevilSettings::configLockScreen()) {
            // Yeah, we do.
            QVariantMap args;
            args["Type"] = "LockScreen";
            triggerImpl(args);
        }
        suspendJob = backend()->suspend(PowerDevil::BackendInterface::ToDisk);
    } else if (args["Type"].toString() == "SuspendHybrid") {
        // Do we want to lock the screen?
        if (PowerDevilSettings::configLockScreen()) {
            // Yeah, we do.
            QVariantMap args;
            args["Type"] = "LockScreen";
            triggerImpl(args);
        }
        suspendJob = backend()->suspend(PowerDevil::BackendInterface::HybridSuspend);
    } else if (args["Type"].toString() == "Shutdown") {
        KWorkSpace::requestShutDown(KWorkSpace::ShutdownConfirmNo, KWorkSpace::ShutdownTypeHalt);
    } else if (args["Type"].toString() == "Restart") {
        KWorkSpace::requestShutDown(KWorkSpace::ShutdownConfirmNo, KWorkSpace::ShutdownTypeReboot);
    } else if (args["Type"].toString() == "Logout") {
        KWorkSpace::requestShutDown(KWorkSpace::ShutdownConfirmNo, KWorkSpace::ShutdownTypeLogout);
    } else if (args["Type"].toString() == "LogoutDialog") {
        KWorkSpace::requestShutDown(KWorkSpace::ShutdownConfirmYes);
    } else if (args["Type"].toString() == "LockScreen") {
        OrgFreedesktopScreenSaverInterface iface("org.freedesktop.ScreenSaver",
                                                 "/ScreenSaver",
                                                 QDBusConnection::sessionBus());
        QDBusPendingReply< void > reply = iface.Lock();
        reply.waitForFinished();
    }

    if (suspendJob) {
        suspendJob->start();
    }
}

bool SuspendSession::loadAction(const KConfigGroup& config)
{
    if (config.isValid() && config.hasKey("idleTime") && config.hasKey("suspendType")) {
        // Add the idle timeout
        registerIdleTimeout(config.readEntry<int>("idleTime", 0));
        m_autoType = config.readEntry<QString>("suspendType", QString());
    }

    return true;
}

}
}

#include "suspendsession.moc"
