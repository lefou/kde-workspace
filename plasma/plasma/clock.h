/***************************************************************************
 *   Copyright (C) 2005,2006,2007 by Siraj Razick                          *
 *   siraj@kdemail.net                                                     *
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

#ifndef CLOCK
#define CLOCK

#include <QImage>
#include <QPaintDevice>
#include <QLabel>
#include <QPixmap>
#include <QTimer>
#include <QPaintEvent>
#include <QPainter>
#include <QTime>
#include <QX11Info>
#include <QWidget>
#include <QGraphicsItem>
#include <QColor>

#include <kdemacros.h>

#include "datavisualization.h"
#include "dataengine.h"

class QTimer;

namespace Plasma
{
    class Svg;
}

class Clock : public Plasma::DataVisualization,
              public QGraphicsItem
{
        Q_OBJECT
    public:
        Clock(QGraphicsItem *parent = 0);
        ~Clock();

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget =0);
        void setPath(const QString&);
        QRectF boundingRect() const;

    public slots:
        void updated(const QString &name, const Plasma::DataEngine::Data &data);

    protected:
//         QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    private:
        int m_pixelSize;
        QTimer *m_timer;
        Plasma::Svg* m_theme;
        QTime m_time;
};

#endif
