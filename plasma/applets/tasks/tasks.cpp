/***************************************************************************
 *   Copyright (C) 2007 by Robert Knight                                   *
 *   robertknight@gmail.com                                                *
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

// Own
#include "tasks.h"
#include "taskgroupitem.h"
#include "startuptaskitem.h"
#include "windowtaskitem.h"

// Qt
#include <QGraphicsSceneWheelEvent>
#include <QTimeLine>

// Plasma
#include <plasma/layouts/boxlayout.h>
#include <plasma/layouts/layoutanimator.h>

Tasks::Tasks(QObject* parent , const QVariantList &arguments)
 : Plasma::Applet(parent,arguments)
{
}

void Tasks::init()
{
    Plasma::BoxLayout *layout = new Plasma::BoxLayout(Plasma::BoxLayout::LeftToRight, this);
    layout->setMargin(0);
    _rootTaskGroup = new TaskGroupItem(this, this);

    // testing
        Plasma::LayoutAnimator* animator = new Plasma::LayoutAnimator;
        animator->setAutoDeleteOnRemoval(true);
        animator->setEffect(Plasma::LayoutAnimator::InsertedState,
                            Plasma::LayoutAnimator::FadeInMoveEffect);
        animator->setEffect(Plasma::LayoutAnimator::StandardState,
                            Plasma::LayoutAnimator::MoveEffect);
        animator->setEffect(Plasma::LayoutAnimator::RemovedState,
                            Plasma::LayoutAnimator::FadeOutMoveEffect);
        animator->setTimeLine(new QTimeLine(200, this));
        _rootTaskGroup->layout()->setAnimator(animator);

    layout->addItem(_rootTaskGroup);

    // testing
        _rootTaskGroup->setBorderStyle(TaskGroupItem::NoBorder);
       // _rootTaskGroup->setColor( QColor(100,120,130) );
        _rootTaskGroup->setText("Root Group");

    // add representations of existing running tasks
    registerWindowTasks();
    registerStartingTasks();
}

void Tasks::registerStartingTasks()
{
    // listen for addition and removal of starting tasks
    connect(TaskManager::self(), SIGNAL(startupAdded(Startup::StartupPtr)),
            this, SLOT(addStartingTask(Startup::StartupPtr)) );
    connect(TaskManager::self(), SIGNAL(startupRemoved(Startup::StartupPtr)),
            this, SLOT(removeStartingTask(Startup::StartupPtr)));
}

void Tasks::addStartingTask(Startup::StartupPtr task)
{
    StartupTaskItem* item = new StartupTaskItem(_rootTaskGroup, _rootTaskGroup);
    _startupTaskItems.insert(task, item);

    addItemToRootGroup(item);
}

void Tasks::removeStartingTask(Startup::StartupPtr task)
{
    removeItemFromRootGroup(_startupTaskItems[task]);
}

void Tasks::registerWindowTasks()
{
    TaskManager *manager = TaskManager::self();

    Task::Dict tasks = manager->tasks();
    QMapIterator<WId,Task::TaskPtr> iter(tasks);

    while (iter.hasNext())
    {
        iter.next();
        addWindowTask(iter.value());
    }

    // listen for addition and removal of window tasks
    connect(TaskManager::self(), SIGNAL(taskAdded(Task::TaskPtr)),
            this, SLOT(addWindowTask(Task::TaskPtr)));
    connect(TaskManager::self(), SIGNAL(taskRemoved(Task::TaskPtr)),
            this, SLOT(removeWindowTask(Task::TaskPtr)));
}

void Tasks::addItemToRootGroup(AbstractTaskItem *item)
{
    item->setFlag(QGraphicsItem::ItemIsSelectable);
    _rootTaskGroup->insertTask(item);
}

void Tasks::removeItemFromRootGroup(AbstractTaskItem *item)
{
    Q_ASSERT( item );

    _rootTaskGroup->removeTask(item);

// TEMPORARY
//      scene()->removeItem(item);
//    item->deleteLater();
}

void Tasks::addWindowTask(Task::TaskPtr task)
{
    if (!task->showInTaskbar()) {
        return;
    }

    WindowTaskItem *item = new WindowTaskItem(_rootTaskGroup, _rootTaskGroup);
    item->setWindowTask(task);
    _windowTaskItems.insert(task,item);

    addItemToRootGroup(item);
}

void Tasks::removeWindowTask(Task::TaskPtr task)
{
    if (_windowTaskItems.contains(task)) {
        removeItemFromRootGroup(_windowTaskItems[task]);
    }
}

void Tasks::constraintsUpdated(Plasma::Constraints constraints)
{
    if (constraints & Plasma::LocationConstraint) {
        foreach (AbstractTaskItem *taskItem, _rootTaskGroup->tasks()) {
            //TODO: Update this if/when tasks() returns other types
            WindowTaskItem *windowTaskItem = dynamic_cast<WindowTaskItem *>(taskItem);
            if (windowTaskItem) {
                windowTaskItem->publishIconGeometry();
            }
        }
    }
}

void Tasks::wheelEvent(QGraphicsSceneWheelEvent *e)
{
     _rootTaskGroup->cycle(e->delta());
}

#include "tasks.moc"
