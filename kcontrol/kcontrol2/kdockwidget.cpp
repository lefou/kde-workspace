/*

  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/                                                                            


#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qpainter.h>
#include <qiconset.h>


#include <ktoolboxmgr.h>
#include <kwm.h>
#include <kglobal.h>
#include <kiconloader.h>


#include "kdockwidget.h"
#include "kdockwidget.moc"


KDockContainer::KDockContainer(QWidget *parent, const char *name)
  : QWidget(parent, name)
{
  _tabs = new QTabWidget(this);
}


void KDockContainer::addWidget(QWidget *widget, QPixmap icon)
{
  _tabs->addTab(widget, QIconSet(icon), widget->caption());
}


void KDockContainer::addDockWidget(KDockWidget *widget, QPixmap icon)
{
  _children.append(widget);

  _tabs->addTab(widget, QIconSet(icon), widget->caption());
  _tabs->showPage(widget);
}


void KDockContainer::removeDockWidget(KDockWidget *widget)
{
  _children.remove(widget);
}


void KDockContainer::resizeEvent(QResizeEvent *)
{
  _tabs->setGeometry(0,0,width(),height());
}


void KDockContainer::dragChild(KDockWidget *child)
{
  // create a tool box manager
  _tbManager = new KToolBoxManager(child);

  // compute the docking area
  QPoint topLeft = mapToGlobal(QPoint(0,0));
  QSize size    = _tabs->childrenRect().size();
  if (_tabs->currentPage())
    {
      size = _tabs->currentPage()->size();
      topLeft += QPoint(0,_tabs->childrenRect().height()-_tabs->currentPage()->height());
    }
  _tbManager->addHotSpot(QRect(topLeft, size));  
  connect(_tbManager, SIGNAL(onHotSpot(int)), this, SLOT(onHotSpot(int)));

  _dockSpot = false;

  // do the magic
  _tbManager->doMove(true);

  if (_dockSpot) 
    {
      if (!child->isDocked())
	{
	  // dock the window
	  child->recreate(this, 0, QPoint(0,0), false);
	  child->dock();	  
	}
    }
  else 
    {
      if (child->isDocked())
	{
	  // reparent the child
	  child->recreate((QWidget*)0, 0, QPoint(_tbManager->x(), _tbManager->y()), true);
	  KWM::setDecoration(child->winId(), KWM::tinyDecoration);
	  QApplication::syncX();
	  child->undock();
	}
    }

  child->show();
  repaint(true);
  
  delete _tbManager;
  _tbManager = 0;
}


void KDockContainer::onHotSpot(int index)
{
  if ((index > -1) && _tbManager)
    {
      _tbManager->setGeometry(index);
      _dockSpot = true;
    }
  else
    _dockSpot = false;
}


const int KDockWidget::DOCKBAR_HEIGHT = 10;


KDockWidget::KDockWidget(QString caption, QPixmap icon, KDockContainer *parent, const char *name)
  : QWidget(parent, name), _container(parent), _docked(true), _icon(icon)
{
  // set the caption immediately, as it will be used
  // for the tab in the KDockContainer!
  setCaption(caption);

  // create the close button
  _closeButton = new QPushButton(this);
  _closeButton->setFocusPolicy(QWidget::ClickFocus);
  _closeButton->setPixmap( KGlobal::iconLoader()->loadIcon( "close", false ) );

  connect(_closeButton, SIGNAL(clicked()), this, SLOT(slotCloseClicked()));

  parent->addDockWidget(this,icon);

  setMouseTracking(true);
}


KDockWidget::~KDockWidget()
{
  _container->removeDockWidget(this);
}


void KDockWidget::undock()
{
  _docked = false;
  _container->removeDockWidget(this);
  repaint(true);
}


void KDockWidget::dock()
{
  _docked = true;
  _container->addDockWidget(this, _icon);
  repaint(true);
}


void KDockWidget::resizeEvent(QResizeEvent *)
{
  _closeButton->setGeometry(width()-DOCKBAR_HEIGHT,0,DOCKBAR_HEIGHT,DOCKBAR_HEIGHT);
}


void KDockWidget::paintEvent(QPaintEvent *)
{
  QPainter paint;
  QPixmap  drawBuffer(width(), DOCKBAR_HEIGHT);

  paint.begin(&drawBuffer);
  drawBuffer.fill(colorGroup().background());
 
  paint.setPen(white);
  paint.drawLine(1, 3, 1, 2);
  paint.drawLine(1, 2, width()-12, 2);
 
  paint.setPen(colorGroup().mid());
  paint.drawLine(1, 4, width()-12, 4);
  paint.drawLine(width()-12, 4, width()-12, 3);
  
  paint.setPen(white);
  paint.drawLine(1, 6, 1, 5);
  paint.drawLine(1, 5, width()-12, 5);
  
  paint.setPen(colorGroup().mid());
  paint.drawLine(1, 7, width()-12, 7);
  paint.drawLine(width()-12, 7, width()-12, 6);
  
  bitBlt(this,0,0,&drawBuffer,0,0,width(),DOCKBAR_HEIGHT);
  paint.end();
}


void KDockWidget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == LeftButton)
    if (event->pos().y() < DOCKBAR_HEIGHT)
      _container->dragChild(this);
}


QRect KDockWidget::clientRect()
{
  return QRect(0,DOCKBAR_HEIGHT,width(),height()-DOCKBAR_HEIGHT);
}


void KDockWidget::slotCloseClicked()
{
  emit closeClicked();
}
