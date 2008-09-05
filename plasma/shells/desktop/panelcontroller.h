/*
 *   Copyright 2008 Marco Martin <notmart@gmail.com>
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

#ifndef PANELCONTROLLER_H
#define PANELCONTROLLER_H 

#include <QWidget>

#include <plasma/plasma.h>

#include "panelview.h"



namespace Plasma
{
    class Containment;
}

class PanelController : public QWidget
{
Q_OBJECT
public:

    PanelController(QWidget* parent = 0);
    ~PanelController();

    QSize sizeHint() const;

    QPoint positionForPanelGeometry(const QRect &panelGeom) const;
    void setContainment(Plasma::Containment *containment);
    void resizePanel(const QSizeF newSize);

    void setLocation(const Plasma::Location &loc);
    Plasma::Location location() const;

    void setOffset(int newOffset);
    int offset() const;

    void setAlignment(const Qt::Alignment &newAlignment);
    Qt::Alignment alignment() const;

    void setPanelMode(PanelView::PanelMode);
    PanelView::PanelMode panelMode() const;

public Q_SLOTS:
    void setPalette();

protected:
    void paintEvent(QPaintEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void focusOutEvent(QFocusEvent * event);

Q_SIGNALS:
    /**
     * Emitted when the controller requests an add widgets dialog is shown.
     */
     void offsetChanged(int offset);
     void alignmentChanged(Qt::Alignment);
     void locationChanged(Plasma::Location);
     void panelModeChanged(PanelView::PanelMode mode);

private:
    Q_PRIVATE_SLOT(d, void rulersMoved(int offset, int minLength, int minLength))
    Q_PRIVATE_SLOT(d, void alignToggled(bool toggle))
    Q_PRIVATE_SLOT(d, void panelModeChanged(bool toggle))
    Q_PRIVATE_SLOT(d, void settingsPopup())

    class ButtonGroup;
    class ResizeHandle;
    class Private;
    Private *d;
};


#endif // multiple inclusion guard

