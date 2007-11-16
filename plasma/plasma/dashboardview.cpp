/*
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2007 Matt Broadstone <mbroadst@gmail.com>
 *   Copyright 2007 André Duffeck <duffeck@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
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

#include "dashboardview.h"

#include "plasma/applet.h"
#include "plasma/corona.h"
#include "plasma/containment.h"
#include "plasma/svg.h"
#include "plasma/appletbrowser.h"

#include "plasmaapp.h"

#include <KWindowSystem>

DashBoardView::DashBoardView(int screen, QWidget *parent)
    : Plasma::View(screen, PlasmaApp::self()->corona(), parent), 
      m_appletBrowserWidget( 0 )
{
    setContextMenuPolicy(Qt::NoContextMenu);
    setWindowFlags( Qt::FramelessWindowHint );
    setWindowOpacity( 0.9 );
    setWindowState( Qt::WindowFullScreen );
    KWindowSystem::setState(winId(), NET::KeepAbove);

    //FIXME: this OUGHT to be true if we don't have composite, probably
    setDrawWallpaper(false);
    hide();

    connect( scene(), SIGNAL(launchActivated()), SLOT(hideView()) );
}

DashBoardView::~DashBoardView()
{
    delete m_appletBrowserWidget;
}

void DashBoardView::drawBackground(QPainter * painter, const QRectF & rect)
{
    if (PlasmaApp::hasComposite()) {
        painter->setCompositionMode(QPainter::CompositionMode_Source);
        painter->fillRect(rect, QColor(0, 0, 0, 125));
    } else {
        Plasma::View::drawBackground(painter, rect);
    }
}

void DashBoardView::showAppletBrowser()
{
    if (!m_appletBrowserWidget) {
        m_appletBrowserWidget = new Plasma::AppletBrowserWidget(qobject_cast<Plasma::Corona *>(scene()), true, this, Qt::FramelessWindowHint );
        m_appletBrowserWidget->setApplication();
        m_appletBrowserWidget->setWindowTitle(i18n("Add Widgets"));
        KWindowSystem::setState(m_appletBrowserWidget->winId(), NET::KeepAbove);
        //TODO: provide a nice unobtrusive way to access the browser
        m_appletBrowserWidget->move( 0, 0 );
    }

    m_appletBrowserWidget->show();
}

void DashBoardView::toggleVisibility()
{
    if (isHidden()) {
        show();
        raise();
        showAppletBrowser();
    } else {
        hideView();
    }
}

void DashBoardView::hideView()
{
    if (m_appletBrowserWidget) {
        m_appletBrowserWidget->hide();
    }
    hide();
}

#include "dashboardview.moc"

