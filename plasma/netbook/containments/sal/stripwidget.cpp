/*
 *   Copyright 2009 by Artur Duque de Souza <asouza@kde.org>
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2,
 *   or (at your option) any later version.
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

#include "stripwidget.h"

#include <Plasma/Frame>
#include <Plasma/PushButton>
#include <Plasma/IconWidget>
#include <Plasma/QueryMatch>
#include <Plasma/AbstractRunner>
#include <Plasma/RunnerManager>
#include <Plasma/ItemBackground>

#include <KIcon>
#include <KPushButton>

#include <QAction>


StripWidget::StripWidget(Plasma::RunnerManager *rm, QGraphicsItem *parent)
    : QGraphicsWidget(parent),
      m_runnermg(rm),
      m_shownIcons(5),
      m_offset(0),
      m_currentIcon(0),
      m_currentIconIndex(-1)
{
    m_background = new Plasma::Frame();
    m_background->setFrameShadow(Plasma::Frame::Raised);
    /*m_background->setMinimumSize(QSize(600, 115));
    m_background->setMaximumSize(QSize(600, 115));*/
    setMaximumHeight(115);

    // mainLayout to correctly setup the m_background
    QGraphicsLinearLayout *mainLayout = new QGraphicsLinearLayout(this);
    mainLayout->addItem(m_background);

    m_arrowsLayout = new QGraphicsLinearLayout(m_background);
    m_stripLayout = new QGraphicsLinearLayout();
    m_hoverIndicator = new Plasma::ItemBackground(m_background);
    m_hoverIndicator->hide();
    m_hoverIndicator->setZValue(-100);
    setAcceptHoverEvents(true);
    setFocusPolicy(Qt::StrongFocus);

    m_leftArrow = new Plasma::PushButton(this);
    m_leftArrow->nativeWidget()->setIcon(KIcon("arrow-left"));
    m_leftArrow->setMaximumSize(IconSize(KIconLoader::Panel),
                              IconSize(KIconLoader::Panel));
    connect(m_leftArrow, SIGNAL(clicked()), this, SLOT(goLeft()));

    m_rightArrow = new Plasma::PushButton(this);
    m_rightArrow->nativeWidget()->setIcon(KIcon("arrow-right"));
    m_rightArrow->setMaximumSize(IconSize(KIconLoader::Panel),
                               IconSize(KIconLoader::Panel));
    connect(m_rightArrow, SIGNAL(clicked()), this, SLOT(goRight()));

    m_leftArrow->setEnabled(false);
    m_rightArrow->setEnabled(false);

    QGraphicsWidget *leftSpacer = new QGraphicsWidget(this);
    QGraphicsWidget *rightSpacer = new QGraphicsWidget(this);
    leftSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    rightSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    /*leftSpacer->setPreferredSize(0, 0);
    rightSpacer->setPreferredSize(0, 0);*/

    m_arrowsLayout->addItem(m_leftArrow);
    m_arrowsLayout->addItem(leftSpacer);
    //m_arrowsLayout->addStretch();
    m_arrowsLayout->addItem(m_stripLayout);
    //m_arrowsLayout->setStretchFactor(m_stripLayout, 8);
    m_arrowsLayout->addItem(rightSpacer);
    //m_arrowsLayout->addStretch();
    m_arrowsLayout->addItem(m_rightArrow);
}

StripWidget::~StripWidget()
{
}

void StripWidget::createIcon(Plasma::QueryMatch *match, int idx)
{
    // create new IconWidget for favourite strip

    Plasma::IconWidget *fav = new Plasma::IconWidget(this);
    fav->setSizePolicy(QSizePolicy::MinimumExpanding,
                          QSizePolicy::MinimumExpanding);
    fav->installEventFilter(this);
    fav->setText(match->text());
    fav->setIcon(match->icon());
    fav->setMinimumSize(fav->sizeFromIconSize(KIconLoader::SizeHuge));
    fav->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(fav, SIGNAL(activated()), this, SLOT(launchFavourite()));

    fav->setMinimumSize(fav->size().height(), fav->size().height());
    fav->setPos(fav->size().width()/2-fav->size().width()/2, fav->size().height()/2-fav->size().height()/2);

    // set an action to be able to remove from favourites
    QAction *action = new QAction(fav);
    action->setIcon(KIcon("list-remove"));
    fav->addIconAction(action);
    connect(action, SIGNAL(triggered()), this, SLOT(removeFavourite()));

    m_favouritesIcons.insert(fav, match);
    m_stripLayout->insertItem(idx, fav);
    m_stripLayout->setMaximumSize((fav->size().width())*m_stripLayout->count(), fav->size().height());
    m_stripLayout->setMinimumSize(m_stripLayout->maximumSize());
}

void StripWidget::add(Plasma::QueryMatch match, const QString &query)
{
    // add to layout and data structures
    Plasma::QueryMatch *newMatch = new Plasma::QueryMatch(match);
    m_favouritesMatches.append(newMatch);
    m_favouritesQueries.insert(newMatch, query);

    int idx = m_stripLayout->count();
    if (idx > m_shownIcons - 1) {
        m_leftArrow->setEnabled(true);
        m_rightArrow->setEnabled(true);
    } else {
        createIcon(newMatch, idx);
    }
}

void StripWidget::remove(Plasma::IconWidget *favourite)
{
    Plasma::QueryMatch *match = m_favouritesIcons.value(favourite);
    m_favouritesMatches.removeOne(match);
    m_favouritesQueries.remove(match);
    m_favouritesIcons.remove(favourite);

    // must be deleteLater because the IconWidget will return from the action?
    favourite->deleteLater();
    delete match;

    // the IconWidget was not removed yet
    if (m_favouritesMatches.size() <= m_shownIcons) {
        m_leftArrow->setEnabled(false);
        m_rightArrow->setEnabled(false);
    }

    if (m_favouritesMatches.size() >= m_shownIcons) {
        // adds the new item to the end of the list
        int idx = m_favouritesMatches.indexOf(match);
        int newpos = (idx + m_shownIcons + m_offset) % (m_favouritesMatches.size()-1);

        match = m_favouritesMatches[newpos];
        // must be m_shownIcons here because at this point the icon was not deleted yet
        createIcon(match, m_shownIcons);
    }
}

void StripWidget::removeFavourite()
{
    Plasma::IconWidget *icon = static_cast<Plasma::IconWidget*>(sender()->parent());

    if (icon) {
        m_stripLayout->setMinimumSize(icon->size().width()*(m_stripLayout->count()-2), icon->size().height());
        m_stripLayout->setMaximumSize(m_stripLayout->minimumSize());

        remove(icon);
    }
}

void StripWidget::launchFavourite()
{
    Plasma::IconWidget *icon = static_cast<Plasma::IconWidget*>(sender());
    Plasma::QueryMatch *match = m_favouritesIcons.value(icon);

    Plasma::RunnerContext context;
    context.setQuery(m_favouritesQueries.value(match));
    match->run(context);
}

void StripWidget::launchFavourite(Plasma::IconWidget *icon)
{
    Plasma::QueryMatch *match = m_favouritesIcons.value(icon);

    Plasma::RunnerContext context;
    context.setQuery(m_favouritesQueries.value(match));
    match->run(context);
}

void StripWidget::goRight()
{
    // discover the item that will be removed
    Plasma::IconWidget *icon = static_cast<Plasma::IconWidget*>(m_stripLayout->itemAt(0));
    Plasma::QueryMatch *match = m_favouritesIcons.value(icon);

    // removes the first item
    m_favouritesIcons.remove(icon);
    icon->hide();
    delete icon;

    // adds the new item to the end of the list
    int idx = m_favouritesMatches.indexOf(match);
    int size = m_favouritesMatches.size();
    int newpos = (idx + m_shownIcons) % size;
    m_offset = (m_offset + 1) % size;

    match = m_favouritesMatches[newpos];
    createIcon(match, m_shownIcons-1);
}

void StripWidget::goLeft()
{
    // discover the item that will be removed
    Plasma::IconWidget *icon = static_cast<Plasma::IconWidget*>(m_stripLayout->itemAt(m_shownIcons - 1));
    Plasma::QueryMatch *match = m_favouritesIcons.value(icon);

    // removes the first item
    m_favouritesIcons.remove(icon);
    icon->hide();
    delete icon;

    // adds the new item to the end of the list
    int idx = m_favouritesMatches.indexOf(match);
    int size = m_favouritesMatches.size();
    int newpos = (idx + size - m_shownIcons) % size;
    m_offset = newpos;

    match = m_favouritesMatches[newpos];
    createIcon(match, 0);
}

void StripWidget::save(KConfigGroup &cg)
{
    kDebug() << "----------------> Saving Stuff...";

    // erase the old stuff before saving the new one
    KConfigGroup oldGroup(&cg, "stripwidget");
    oldGroup.deleteGroup();

    KConfigGroup stripGroup(&cg, "stripwidget");

    int id = 0;
    foreach(Plasma::QueryMatch *match, m_favouritesMatches) {
        // Write now just saves one for tests. Later will save
        // all the strip
        KConfigGroup config(&stripGroup, QString(id));
        config.writeEntry("runnerid", match->runner()->id());
        config.writeEntry("query", m_favouritesQueries.value(match));
        config.writeEntry("matchId", match->id());
        id++;
    }
}

void StripWidget::restore(KConfigGroup &cg)
{
    kDebug() << "----------------> Restoring Stuff...";

    KConfigGroup stripGroup(&cg, "stripwidget");

    // get all the favourites
    QList<KConfigGroup> favouritesConfigs;
    foreach (const QString &favouriteGroup, stripGroup.groupList()) {
        KConfigGroup favouriteConfig(&stripGroup, favouriteGroup);
        favouritesConfigs.append(favouriteConfig);
    }


    QMutableListIterator<KConfigGroup> it(favouritesConfigs);
    while (it.hasNext()) {
        KConfigGroup &favouriteConfig = it.next();

        QString runnerId = favouriteConfig.readEntry("runnerid");
        QString query = favouriteConfig.readEntry("query");
        QString matchId = favouriteConfig.readEntry("matchId");

        // This is the "lazy mode". We do the queries again just for
        // that runner (should be fast). This way we are able to put
        // contacts and nepomuk stuff on the favourites strip.
        // Later we can improve load time for specific runners (services)
        // so we do not have to query again.

        // perform the query
        if (m_runnermg->execQuery(query, runnerId)) {
            // find our match
            Plasma::QueryMatch match(m_runnermg->searchContext()->match(matchId));

            // we should verify some other saved information to avoid putting the
            // wrong item if the search result is different!

            add(match, query);
        }
    }
}

bool StripWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::GraphicsSceneHoverEnter) {
        Plasma::IconWidget *icon = qobject_cast<Plasma::IconWidget *>(watched);
        if (icon) {
            m_hoverIndicator->show();
            m_hoverIndicator->setTargetItem(icon);
        }
    }

    return false;
}

void StripWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left: {
        m_currentIconIndex = (m_stripLayout->count() + m_currentIconIndex - 1) % m_stripLayout->count();
        m_currentIcon = static_cast<Plasma::IconWidget *>(m_stripLayout->itemAt(m_currentIconIndex));
        m_hoverIndicator->setTargetItem(m_currentIcon);
        break;
    }
    case Qt::Key_Right: {
        m_currentIconIndex = (m_currentIconIndex + 1) % m_stripLayout->count();
        m_currentIcon = static_cast<Plasma::IconWidget *>(m_stripLayout->itemAt(m_currentIconIndex));
        m_hoverIndicator->setTargetItem(m_currentIcon);
        break;
    }
    case Qt::Key_Enter:
    case Qt::Key_Return:
        launchFavourite(m_currentIcon);
    default:
        break;
    }
}

void StripWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    m_hoverIndicator->hide();
}

void StripWidget::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    Plasma::IconWidget *icon = static_cast<Plasma::IconWidget*>(m_stripLayout->itemAt(0));
    m_hoverIndicator->show();
    m_hoverIndicator->setTargetItem(icon);
    m_currentIconIndex = 0;
}

void StripWidget::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    m_hoverIndicator->hide();
    m_currentIconIndex = -1;
}

void StripWidget::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    int newShownIcons = qMax(1, (int)((event->newSize().width() - m_leftArrow->size().width() - m_rightArrow->size().width())/m_background->size().height()));

    int effectiveShownIcons = qMin(m_shownIcons, m_favouritesMatches.count());

    if (newShownIcons > effectiveShownIcons) {
        int newEffectiveShownIcons = qMin(newShownIcons, m_favouritesMatches.count());
        for (int i = effectiveShownIcons; i < newEffectiveShownIcons; ++i) {
            Plasma::QueryMatch *match = m_favouritesMatches[i];
            createIcon(match, i);
        }
    } else if (newShownIcons < effectiveShownIcons) {
        for (int i = effectiveShownIcons; i > newShownIcons && m_stripLayout->count() > 0; --i) {
            Plasma::IconWidget *icon = static_cast<Plasma::IconWidget*>(m_stripLayout->itemAt(m_stripLayout->count()-1));

            m_favouritesIcons.remove(icon);
            icon->hide();
            QSizeF widgetSize = icon->size();
            delete icon;
            //FIXME here as well
            m_stripLayout->setMinimumSize(widgetSize.width()*(m_stripLayout->count()-1), widgetSize.height());
            m_stripLayout->setMaximumSize(m_stripLayout->minimumSize());
        }
    }

    m_shownIcons = newShownIcons;
}
