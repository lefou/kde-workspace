/***************************************************************************
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


#include "deviceitem.h"

//Qt
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsLinearLayout>
#include <QLabel>

//KDE
#include <KDebug>
#include <KIcon>
#include <KIconLoader>
#include <KStandardDirs>
#include <KDesktopFile>
#include <KGlobalSettings>
#include <kdesktopfileactions.h>

//Plasma
#include <Plasma/PaintUtils>
#include <Plasma/IconWidget>
#include <Plasma/BusyWidget>
#include <Plasma/ItemBackground>
#include <Plasma/Label>
#include <Plasma/Meter>
#include <Plasma/Animator>
#include <Plasma/Animation>

//Own
#include "notifierdialog.h"

using namespace Notifier;

DeviceItem::DeviceItem(const QString &udi, bool unpluggable, QGraphicsWidget *parent)
    : QGraphicsWidget(parent),
      m_udi(udi),
      m_hovered(false),
      m_mounted(false),
      m_safelyRemovable(true),
      m_unpluggable(unpluggable),
      m_state(DeviceItem::Idle),
      m_labelFade(0),
      m_barFade(0)
{
    setAcceptHoverEvents(true);
    setCacheMode(DeviceCoordinateCache);
    setZValue(0);
    setContentsMargins(MARGIN, MARGIN, MARGIN, MARGIN);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    m_treeLayout = new QGraphicsLinearLayout(Qt::Vertical, this);

    m_mainLayout = new QGraphicsLinearLayout(Qt::Horizontal);
    m_treeLayout->addItem(m_mainLayout);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);

    m_actionsWidget = new QGraphicsWidget(this);

    m_actionsLayout = new QGraphicsLinearLayout(Qt::Vertical, m_actionsWidget);
    m_actionsLayout->setContentsMargins(30, 0, 0, 0);
    m_actionsWidget->hide();

    m_deviceIcon = new Plasma::IconWidget(this);
    m_deviceIcon->setAcceptHoverEvents(false);
    m_deviceIcon->setContentsMargins(0, 0, 0, 0);
    m_deviceIcon->setMinimumSize(m_deviceIcon->sizeFromIconSize(KIconLoader::SizeMedium));
    m_deviceIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);
    m_deviceIcon->setAcceptedMouseButtons(Qt::NoButton);
    m_deviceIcon->setFocusPolicy(Qt::NoFocus);

    QGraphicsLinearLayout *info_layout = new QGraphicsLinearLayout(Qt::Vertical);
    info_layout->setContentsMargins(0, 0, 0, 0);
    info_layout->setSpacing(0);
    m_nameLabel = new Plasma::Label(this);
    m_nameLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_nameLabel->setPreferredWidth(0);
    m_nameLabel->nativeWidget()->setWordWrap(false);
    m_nameLabel->setAcceptedMouseButtons(0);

    m_descriptionLabel = new Plasma::Label(this);
    m_descriptionLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_descriptionLabel->setPreferredWidth(0);
    m_descriptionLabel->nativeWidget()->setWordWrap(false);
    m_descriptionLabel->setAcceptedMouseButtons(0);
    QFont font = m_descriptionLabel->font();
    font.setPointSize(KGlobalSettings::smallestReadableFont().pointSize());
    font.setItalic(true);
    m_descriptionLabel->setFont(font);
    m_descriptionLabel->setOpacity(0);
    updateColors();


    m_freeSpaceBar = new Plasma::Meter();
    m_freeSpaceBar->setMeterType(Plasma::Meter::BarMeterHorizontal);
    m_freeSpaceBar->setLabelAlignment(0, Qt::AlignCenter);
    m_freeSpaceBar->setOpacity(0);
    m_freeSpaceBar->setMaximumHeight(12);

    info_layout->addItem(m_nameLabel);
    info_layout->addItem(m_descriptionLabel);
    info_layout->addItem(m_freeSpaceBar);

    m_leftActionIcon = new Plasma::IconWidget(this);
    m_leftActionIcon->setMaximumSize(m_leftActionIcon->sizeFromIconSize(LEFTACTION_SIZE));
    m_leftActionIcon->setSizePolicy(QSizePolicy::Fixed,  QSizePolicy::Fixed);
    connect(m_leftActionIcon, SIGNAL(clicked()), this, SLOT(leftActionClicked()));

    m_mainLayout->addItem(m_deviceIcon);
    m_mainLayout->setAlignment(m_deviceIcon, Qt::AlignVCenter);
    m_mainLayout->addItem(info_layout);
    m_mainLayout->setAlignment(info_layout, Qt::AlignVCenter);
    m_mainLayout->addItem(m_leftActionIcon);
    m_mainLayout->setAlignment(m_leftActionIcon, Qt::AlignVCenter);
    m_mainLayout->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    m_busyWidget = new Plasma::BusyWidget(this);
    m_busyWidget->setMaximumSize(LEFTACTION_SIZE, LEFTACTION_SIZE);
    m_busyWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_busyWidget->hide();
    m_busyWidgetTimer.setSingleShot(true);
    connect(&m_busyWidgetTimer, SIGNAL(timeout()), this,  SLOT(triggerBusyWidget()));

    setLeftAction(Nothing);
}

DeviceItem::~DeviceItem()
{
}

void DeviceItem::collapse()
{
    if (!isCollapsed()) {
        m_treeLayout->removeAt(1);
        m_actionsWidget->hide();

        update();
    }
}

void DeviceItem::expand()
{
    if (isCollapsed()) {
        m_treeLayout->addItem(m_actionsWidget);
        m_actionsWidget->show();

        update();
    }
}

void DeviceItem::addAction(const QString &action)
{
    for (int i = 0; i < m_actionsLayout->count(); ++i) {
        QGraphicsLayoutItem *item = m_actionsLayout->itemAt(i);
        if (item->graphicsItem()->data(NotifierDialog::ActionRole).toString() == action) {
            return;
        }
    }

    QString actionUrl = KStandardDirs::locate("data", "solid/actions/" + action);
    QList<KServiceAction> services = KDesktopFileActions::userDefinedServices(actionUrl, true);

    //Sanity check
    if (services.count()>0) {
        Plasma::IconWidget *actionItem = new Plasma::IconWidget(m_actionsWidget);
        actionItem->installEventFilter(this);
        actionItem->setContentsMargins(3, 3, 3, 3);
        actionItem->setData(NotifierDialog::ActionRole, action);

        QColor color;
        color.setAlpha(255);
        actionItem->setTextBackgroundColor(color);
        actionItem->setText(services[0].text());
        actionItem->setIcon(services[0].icon());
        actionItem->setOrientation(Qt::Horizontal);
        actionItem->setPreferredHeight(KIconLoader::SizeMedium+3+3);
        actionItem->setPreferredWidth(0);
        actionItem->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

        m_actionsLayout->addItem(actionItem);
    }
}

QStringList DeviceItem::actions() const
{
    QStringList list;
    for (int i = 0; i < m_actionsLayout->count(); ++i) {
        QGraphicsLayoutItem *item = m_actionsLayout->itemAt(i);
        list << item->graphicsItem()->data(NotifierDialog::ActionRole).toString();
    }

    return list;
}

void DeviceItem::removeAction(const QString &action)
{
    for (int i = 0; i < m_actionsLayout->count(); ++i) {
        QGraphicsLayoutItem *item = m_actionsLayout->itemAt(i);
        if (item->graphicsItem()->data(NotifierDialog::ActionRole).toString() == action) {
            m_actionsLayout->removeAt(i);
            delete item;
        }
    }
}

QString DeviceItem::udi() const
{
    return data(NotifierDialog::SolidUdiRole).toString();
}

QString DeviceItem::name() const
{
    return data(Qt::DisplayRole).toString();
}

QString DeviceItem::description() const
{
    return data(NotifierDialog::DescriptionRole).toString();
}

QIcon DeviceItem::icon() const
{
    return data(Qt::DecorationRole).value<QIcon>();
}

QString DeviceItem::iconName() const
{
    return data(NotifierDialog::IconNameRole).toString();
}

bool DeviceItem::isCollapsed() const
{
    return (m_treeLayout->count() == 1);
}

bool DeviceItem::isMounted() const
{
    return m_mounted;
}

bool DeviceItem::hovered() const
{
    return m_hovered;
}

bool DeviceItem::allowsCapacityBar() const
{
    return !(data(NotifierDialog::IsOpticalMedia).toBool() || data(NotifierDialog::IsEncryptedContainer).toBool());
}

bool DeviceItem::safelyRemovable() const
{
    return m_safelyRemovable;
}

void DeviceItem::setSafelyRemovable(const bool safe)
{
    if (m_safelyRemovable == safe) {
        return;
    }
    m_safelyRemovable = safe;
    updateTooltip();
}

bool DeviceItem::unpluggable()
{
    return m_unpluggable;
}

void DeviceItem::updateTooltip()
{
    if (m_mounted) {
        if (data(NotifierDialog::IsOpticalMedia).toBool()) {
            m_leftActionIcon->setToolTip(i18n("Click to eject this disc."));
        } else {
            if (unpluggable()) {
                m_leftActionIcon->setToolTip(i18n("Click to safely remove this device."));
            } else {
                m_leftActionIcon->setToolTip(i18n("Click to unmount this device."));
            }
        }
        if (unpluggable()) {
            m_deviceIcon->setToolTip(i18n("It is currently <b>not safe</b> to remove this device: applications may be accessing it. Click the eject button to safely remove this device."));
        } else {
            m_deviceIcon->setToolTip(i18n("This device is currently accessible."));
        }
    } else {
        m_leftActionIcon->setToolTip(i18n("Click to access this device from other applications."));
        if (unpluggable()) {
            if (safelyRemovable()) {
                m_deviceIcon->setToolTip(i18n("It is currently safe to remove this device."));
            } else {
                m_deviceIcon->setToolTip(i18n("It is currently <b>not safe</b> to remove this device: applications may be accessing other volumes on this device. Click the eject button on these other volumes to safely remove this device."));
            }
        } else {
            m_deviceIcon->setToolTip(i18n("This device is currently not accessible."));
        }
    }
}

void DeviceItem::setMounted(const bool mounted)
{
    m_mounted = mounted;

    updateTooltip();

    if (data(NotifierDialog::IsEncryptedContainer).toBool()) {
        if (m_mounted) {
            setLeftAction(Lock);
        } else {
            setLeftAction(Unlock);
        }
    } else {
        if (m_mounted) {
            setLeftAction(Umount);
        } else {
            setLeftAction(Mount);
        }
    }

    const bool barVisible = m_freeSpaceBar->isVisible();
    m_freeSpaceBar->setVisible(m_mounted && allowsCapacityBar());
    if (!barVisible && m_freeSpaceBar->isVisible()) {
        // work around for a QGraphicsLayout bug when used with proxy widgets
        m_mainLayout->invalidate();
    }
}

void DeviceItem::setLeftAction(DeviceItem::LeftActions action)
{
    kDebug() << "setting to" << action;
    m_leftAction = action;
    if (m_leftAction == Umount) {
        m_leftActionIcon->setIcon("media-eject");
    } else if (m_leftAction == Mount) {
        m_leftActionIcon->setIcon("emblem-mounted");
    } else if (m_leftAction == Unlock) {
        m_leftActionIcon->setIcon("emblem-unlocked");
    } else if (m_leftAction == Lock) {
        m_leftActionIcon->setIcon("emblem-locked");
    } else {
        m_leftActionIcon->setIcon("");
    }
}

DeviceItem::LeftActions DeviceItem::leftAction()
{
    return m_leftAction;
}

void DeviceItem::setHovered(const bool hovered)
{
    if (hovered == m_hovered) {
        return;
    }

    m_hovered = hovered;
    if (!hovered) {
        if (!m_labelFade) {
            m_labelFade = Plasma::Animator::create(Plasma::Animator::FadeAnimation, this);
            m_barFade = Plasma::Animator::create(Plasma::Animator::FadeAnimation, this);

            m_labelFade->setTargetWidget(m_descriptionLabel);
            m_barFade->setTargetWidget(m_freeSpaceBar);

            m_labelFade->setProperty("targetOpacity", 0);
            m_barFade->setProperty("targetOpacity", 0);
        }
        qreal currentOpacity = m_descriptionLabel->opacity();

        m_labelFade->setProperty("startOpacity", currentOpacity);
        m_barFade->setProperty("startOpacity", currentOpacity);

        m_labelFade->start();
        m_barFade->start();
    }
}

void DeviceItem::setHoverDisplayOpacity(qreal opacity)
{
    m_descriptionLabel->setOpacity(opacity);
    m_freeSpaceBar->setOpacity(opacity);
}

void DeviceItem::setFreeSpace(qulonglong freeSpace, qulonglong size)
{
    qulonglong usedSpace = size - freeSpace;

    m_freeSpaceBar->setToolTip(i18nc("@info:status Free disk space", "%1 free", KGlobal::locale()->formatByteSize(freeSpace)));
    m_freeSpaceBar->setValue(size > 0 ? (usedSpace * 100) / size : 0);
}

void DeviceItem::leftActionClicked()
{
    emit leftActionActivated(this);
}

void DeviceItem::setData(int key, const QVariant & value)
{
    QGraphicsItem::setData(key, value);
    switch (key) {
        case Qt::DecorationRole:
            m_icon = value.value<QIcon>();
            m_deviceIcon->setIcon(m_icon);
            break;
        case Qt::DisplayRole:
            m_nameLabel->setText(value.toString());
            m_nameLabel->setMinimumWidth(50);
            break;
        case NotifierDialog::DescriptionRole:
            m_descriptionLabel->setText(value.toString());
            m_descriptionLabel->setMinimumWidth(50);
//            m_descriptionLabel->setPreferredWidth(50);
            break;
    }
}

void DeviceItem::setState(DeviceItem::State state)
{
    m_state = state;

    if (state == Idle) {
        m_descriptionLabel->setText(description());

        if (m_busyWidgetTimer.isActive()) {
            m_busyWidgetTimer.stop();
        }

        if (m_busyWidget->isVisible()) {
            m_busyWidget->hide();
            m_mainLayout->removeItem(m_busyWidget);
            m_mainLayout->addItem(m_leftActionIcon);
            m_mainLayout->setAlignment(m_leftActionIcon, Qt::AlignVCenter);
            m_leftActionIcon->show();
        }
    } else {
        if (m_busyWidgetTimer.isActive()) {
            return;
        }
        m_busyWidgetTimer.start(300);

        if (state == Mounting) {
            m_descriptionLabel->setText(i18nc("Accessing is a less technical word for Mounting; translation should be short and mean \'Currently mounting this device\'", "Accessing..."));
        } else {
            collapse();
            m_descriptionLabel->setText(i18nc("Removing is a less technical word for Unmounting; translation shoud be short and mean \'Currently unmounting this device\'", "Removing..."));
        }
    }
}

DeviceItem::State DeviceItem::state() const
{
    return m_state;
}

void DeviceItem::triggerBusyWidget()
{
    m_mainLayout->removeItem(m_leftActionIcon);
    m_leftActionIcon->hide();
    m_mainLayout->addItem(m_busyWidget);
    m_mainLayout->setAlignment(m_busyWidget, Qt::AlignVCenter);
    m_busyWidget->show();
}

bool DeviceItem::eventFilter(QObject* obj, QEvent *event)
{
    Plasma::IconWidget *item = qobject_cast<Plasma::IconWidget *>(obj);
    if (item) {
        switch (event->type()) {
            case QEvent::GraphicsSceneHoverLeave:
                emit highlightActionItem(0);
                break;
            case QEvent::GraphicsSceneHoverEnter:
                emit highlightActionItem(item);
                break;
            case QEvent::GraphicsSceneMouseRelease: {
                QGraphicsSceneMouseEvent *e = static_cast<QGraphicsSceneMouseEvent *>(event);
                if (e->button() == Qt::LeftButton &&
                    item->geometry().contains(item->mapToParent(e->pos()))) {
                    actionClicked(item);
                    return true;
                }
                break;
                }
            default:
                break;
        }
    }

    return false;
}

bool DeviceItem::selectNextAction(Plasma::IconWidget *currentAction)
{
    if (!currentAction) {
        emit highlightActionItem(dynamic_cast<QGraphicsItem*>(m_actionsLayout->itemAt(0)));
        return true;
    } else {
        int i=0;
        while (m_actionsLayout->itemAt(i) != currentAction) {
            i++;
        }
        if (m_actionsLayout->count() > i+1) {
            emit highlightActionItem(dynamic_cast<QGraphicsItem*>(m_actionsLayout->itemAt(i+1)));
            return true;
        }
    }
    emit highlightActionItem(0);
    return false;
}

bool DeviceItem::selectPreviousAction(Plasma::IconWidget *currentAction , bool forceLast)
{
    if (forceLast) {
        emit highlightActionItem(dynamic_cast<QGraphicsItem*>(m_actionsLayout->itemAt(m_actionsLayout->count()-1)));
        return true;
    }
    if (!currentAction) {
        emit highlightActionItem(0);
        return false;
    } else {
        int i=0;
        while (m_actionsLayout->itemAt(i) != currentAction) {
            i++;
        }
        if (i > 0) {
            emit highlightActionItem(dynamic_cast<QGraphicsItem*>(m_actionsLayout->itemAt(i-1)));
            return true;
        }
    }
    emit highlightActionItem(0);
    return true;
}


void DeviceItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
}

void DeviceItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && boundingRect().contains(event->pos())) {
        clicked();
    }
}

void DeviceItem::clicked()
{
    if ((m_actionsLayout->count() == 0) || (m_state == Umounting)) {
        return;
    }

    if (m_actionsLayout->count() == 1) {
        emit actionActivated(this, udi(), m_actionsLayout->itemAt(0)->graphicsItem()->data(NotifierDialog::ActionRole).toString());
    } else {
        if (isCollapsed()) {
            expand();
            emit activated(this);
        } else {
            emit collapsed(this);
            collapse();
        }
    }
}

void DeviceItem::actionClicked(Plasma::IconWidget* item)
{
    QString action = item->data(NotifierDialog::ActionRole).toString();
    emit actionActivated(this, udi(), action);
}

void DeviceItem::updateColors()
{
    QPalette p = m_descriptionLabel->nativeWidget()->palette();
    QColor color = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    color.setAlphaF(0.6);
    p.setColor(QPalette::Normal, QPalette::WindowText, color);
    p.setColor(QPalette::Inactive, QPalette::WindowText, color);
    m_descriptionLabel->nativeWidget()->setPalette(p);
    m_descriptionLabel->update();
}

#include "deviceitem.moc"
