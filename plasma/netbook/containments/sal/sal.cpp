/*
 *   Copyright 2009 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2009 by Artur Duque de Souza <asouza@kde.org>
 *   Copyright 2009 by Marco Martin <notmart@gmail.com>
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

#include "sal.h"
#include "stripwidget.h"
#include "itembackground.h"

#include <QPainter>
#include <QAction>
#include <QTimer>

#include <KDebug>
#include <KIcon>
#include <KIconLoader>

#include <Plasma/Theme>
#include <Plasma/Frame>
#include <Plasma/Corona>
#include <Plasma/FrameSvg>
#include <Plasma/LineEdit>
#include <Plasma/IconWidget>
#include <Plasma/RunnerManager>
#include <Plasma/QueryMatch>
#include <Plasma/ScrollWidget>


SearchLaunch::SearchLaunch(QObject *parent, const QVariantList &args)
    : Containment(parent, args),
      m_homeButton(0),
      m_maxColumnWidth(0),
      m_viewMainWidget(0),
      m_gridScroll(0)
{
    setContainmentType(Containment::CustomContainment);
}

SearchLaunch::~SearchLaunch()
{
    KConfigGroup cg = config();
    m_stripWidget->save(cg);

    delete m_runnermg;
    delete m_background;
}

void SearchLaunch::init()
{
    Containment::init();
    connect(this, SIGNAL(appletAdded(Plasma::Applet*,QPointF)),
            this, SLOT(layoutApplet(Plasma::Applet*,QPointF)));
    connect(this, SIGNAL(appletRemoved(Plasma::Applet*)),
            this, SLOT(appletRemoved(Plasma::Applet*)));

    m_runnermg = new Plasma::RunnerManager(this);
    connect(m_runnermg, SIGNAL(matchesChanged(const QList<Plasma::QueryMatch>&)),
            this, SLOT(setQueryMatches(const QList<Plasma::QueryMatch>&)));

    m_relayoutTimer = new QTimer(this);
    m_relayoutTimer->setSingleShot(true);
    connect(m_relayoutTimer, SIGNAL(timeout()), this, SLOT(relayout()));

    Plasma::Svg *borderSvg = new Plasma::Svg(this);
    borderSvg->setImagePath("newspaper/border");

    m_background = new Plasma::FrameSvg(this);
    m_background->setImagePath("widgets/translucentbackground");

    Plasma::DataEngine *engine = dataEngine("searchlaunch");
    engine->connectSource("query", this);
}

void SearchLaunch::themeUpdated()
{
    qreal left, top, right, bottom;
    m_background->getMargins(left, top, right, bottom);
    m_mainLayout->setContentsMargins(left, top, right, bottom);
}

void SearchLaunch::doSearch(const QString query)
{
    m_queryCounter = 0;

    foreach (Plasma::IconWidget *icon, m_items) {
        m_launchGrid->removeAt(0);
        icon->deleteLater();
    }
    m_items.clear();
    m_maxColumnWidth = 0;

    m_items.clear();
    m_matches.clear();
    m_runnermg->reset();

    if (m_gridScroll && query.isEmpty()) {
        QList<Plasma::QueryMatch> fakeMatches;
        Plasma::QueryMatch match(0);
        match.setType(Plasma::QueryMatch::ExactMatch);
        match.setRelevance(0.8);

        //FIXME: awfully hardcoded, find a way nicer way
        match.setId("bookmarks");
        match.setIcon(KIcon("bookmarks"));
        match.setText(i18n("Bookmarks"));
        match.setData("bookmarks");
        fakeMatches.append(match);

        match.setId("contacts");
        match.setIcon(KIcon("view-pim-contacts"));
        match.setText(i18n("Contacts"));
        match.setData("contacts");
        fakeMatches.append(match);

        match.setId("Network");
        match.setIcon(KIcon("applications-internet"));
        match.setText(i18n("Internet"));
        match.setData("Network");
        fakeMatches.append(match);

        match.setId("AudioVideo");
        match.setIcon(KIcon("applications-multimedia"));
        match.setText(i18n("Multimedia"));
        match.setData("AudioVideo");
        fakeMatches.append(match);

        match.setId("Education");
        match.setIcon(KIcon("applications-education"));
        match.setText(i18n("Education"));
        match.setData("Education");
        fakeMatches.append(match);

        match.setId("Game");
        match.setIcon(KIcon("applications-games"));
        match.setText(i18n("Games"));
        match.setData("Game");
        fakeMatches.append(match);

        match.setId("Graphics");
        match.setIcon(KIcon("applications-graphics"));
        match.setText(i18n("Graphics"));
        match.setData("Graphics");
        fakeMatches.append(match);

        match.setId("Office");
        match.setIcon(KIcon("applications-office"));
        match.setText(i18n("Office"));
        match.setData("Office");
        fakeMatches.append(match);

        setQueryMatches(fakeMatches);
        m_homeButton->hide();
    } else {
        m_runnermg->launchQuery(query);
        if (m_homeButton) {
            m_homeButton->show();
        }
    }
}

void SearchLaunch::reset()
{
    doSearch(QString());
}

void SearchLaunch::setQueryMatches(const QList<Plasma::QueryMatch> &m)
{
    if (m.isEmpty()) {
        return;
    }

    int iconSize = KIconLoader::SizeHuge;

    // just add new QueryMatch
    int i;
    for (i = m_queryCounter; i < m.size(); i++) {
        Plasma::QueryMatch match = m[i];

        // create new IconWidget with information from the match
        Plasma::IconWidget *icon = new Plasma::IconWidget(m_viewMainWidget);
        icon->setText(match.text());
        icon->setIcon(match.icon());
        icon->setMinimumSize(icon->sizeFromIconSize(iconSize));
        icon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        connect(icon, SIGNAL(activated()), this, SLOT(launch()));
        icon->installEventFilter(this);

        if (icon->size().width() > m_maxColumnWidth) {
            m_maxColumnWidth = icon->size().width();
        }

        // create action to add to favourites strip
        QAction *action = new QAction(icon);
        action->setIcon(KIcon("favorites"));
        icon->addIconAction(action);
        connect(action, SIGNAL(triggered()), this, SLOT(addFavourite()));

        // add to layout and data structures

        m_items.insert(1/match.relevance(), icon);
        m_matches.insert(icon, match);

        m_relayoutTimer->start(400);
    }
    m_queryCounter = i;
}

void SearchLaunch::relayout()
{
    QList<Plasma::IconWidget *>orderedItems = m_items.values();
    int validIndex = 0;

    foreach (Plasma::IconWidget *icon, orderedItems) {
        if (m_launchGrid->itemAt(validIndex) == icon) {
            ++validIndex;
        } else {
            break;
        }
    }

    for (int i = validIndex; i < m_launchGrid->count(); ++i) {
        m_launchGrid->removeAt(validIndex);
    }

    int nColumns = qMax(1, int(m_gridScroll->size().width() / m_maxColumnWidth));
    int i = 0;

    foreach (Plasma::IconWidget *icon, orderedItems) {
        if (i < validIndex) {
            ++i;
            continue;
        }

        m_launchGrid->addItem(icon, i / nColumns, i % nColumns);
        m_launchGrid->setAlignment(icon, Qt::AlignHCenter);
        ++i;
    }
    m_viewMainWidget->resize(0,0);
}

void SearchLaunch::launch()
{
    Plasma::IconWidget *icon = static_cast<Plasma::IconWidget*>(sender());
    Plasma::QueryMatch match = m_matches.value(icon, Plasma::QueryMatch(0));
    if (m_runnermg->searchContext()->query().isEmpty()) {
        doSearch(match.data().toString());
    } else {
        m_runnermg->run(match);
    }
}

void SearchLaunch::addFavourite()
{
    Plasma::IconWidget *icon = static_cast<Plasma::IconWidget*>(sender()->parent());
    Plasma::QueryMatch match = m_matches.value(icon, Plasma::QueryMatch(0));
    m_stripWidget->add(match, m_runnermg->searchContext()->query());
}

QList<QAction*> SearchLaunch::contextualActions()
{
    QList<QAction*> actions;
    return actions;
}

void SearchLaunch::layoutApplet(Plasma::Applet* applet, const QPointF &pos)
{
    Q_UNUSED(pos);

    // this gets called whenever an applet is added, and we add it to our layout
    QGraphicsLinearLayout *lay = dynamic_cast<QGraphicsLinearLayout*>(layout());

    if (!lay) {
        return;
    }

    lay->addItem(applet);
    applet->setBackgroundHints(NoBackground);
    connect(applet, SIGNAL(sizeHintChanged(Qt::SizeHint)), this, SLOT(updateSize()));
}

void SearchLaunch::appletRemoved(Plasma::Applet* applet)
{
    //shrink the SearchLaunch if possible
    if (formFactor() == Plasma::Horizontal) {
        resize(size().width() - applet->size().width(), size().height());
    } else {
        resize(size().width(), size().height() - applet->size().height());
    }
    layout()->setMaximumSize(size());
}

void SearchLaunch::updateSize()
{
    Plasma::Applet *applet = qobject_cast<Plasma::Applet *>(sender());

    if (applet) {
        if (formFactor() == Plasma::Horizontal) {
            const int delta = applet->preferredWidth() - applet->size().width();
            //setting the preferred width when delta = 0 and preferredWidth() < minimumWidth()
            // leads to the same thing as setPreferredWidth(minimumWidth())
            if (delta != 0) {
                setPreferredWidth(preferredWidth() + delta);
            }
        } else if (formFactor() == Plasma::Vertical) {
            const int delta = applet->preferredHeight() - applet->size().height();
            if (delta != 0) {
                setPreferredHeight(preferredHeight() + delta);
            }
        }

        resize(preferredSize());
    }
}


void SearchLaunch::constraintsEvent(Plasma::Constraints constraints)
{
    kDebug() << "constraints updated with" << constraints << "!!!!!!";

    if (constraints & Plasma::FormFactorConstraint ||
        constraints & Plasma::StartupCompletedConstraint) {

        Plasma::FormFactor form = formFactor();
        Qt::Orientation layoutDirection = form == Plasma::Vertical ? \
            Qt::Vertical : Qt::Horizontal;
        Qt::Orientation layoutOtherDirection = form == Plasma::Vertical ? \
            Qt::Horizontal : Qt::Vertical;

        // create our layout!
        if (!layout()) {
            // create main layout
            m_mainLayout = new QGraphicsLinearLayout();
            m_mainLayout->setOrientation(layoutOtherDirection);
            m_mainLayout->setSpacing(4);
            m_mainLayout->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
                                                    QSizePolicy::Expanding));
            setLayout(m_mainLayout);

            // create launch grid and make it centered
            QGraphicsLinearLayout *gridLayout = new QGraphicsLinearLayout(Qt::Vertical);

            Plasma::Frame *gridBackground = new Plasma::Frame(this);
            gridBackground->setFrameShadow(Plasma::Frame::Plain);
            gridBackground->setAcceptHoverEvents(true);
            gridBackground->installEventFilter(this);
            m_viewMainWidget = new QGraphicsWidget(this);
            QGraphicsLinearLayout *mwLay = new QGraphicsLinearLayout(m_viewMainWidget);
            mwLay->addStretch();
            mwLay->addItem(gridBackground);
            mwLay->addStretch();

            m_hoverIndicator = new ItemBackground(gridBackground);
            m_hoverIndicator->hide();

            m_gridScroll = new Plasma::ScrollWidget(this);
            m_gridScroll->setWidget(m_viewMainWidget);
            m_gridScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_gridScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
            gridLayout->addItem(m_gridScroll);

            m_launchGrid = new QGraphicsGridLayout();
            gridBackground->setLayout(m_launchGrid);

            QGraphicsLinearLayout *m_favourites = new QGraphicsLinearLayout();
            m_favourites->setOrientation(layoutDirection);
            m_favourites->addStretch();
            m_favourites->addStretch();

            m_stripWidget = new StripWidget(m_runnermg, this);
            m_favourites->insertItem(1, m_stripWidget);
            KConfigGroup cg = config();
            m_stripWidget->restore(cg);

            // add our layouts to main vertical layout
            m_mainLayout->addItem(m_favourites);
            m_mainLayout->addItem(gridLayout);

            // correctly set margins
            themeUpdated();
            m_mainLayout->activate();
            m_mainLayout->updateGeometry();

            m_homeButton = new Plasma::IconWidget(this);
            m_homeButton->setIcon(KIcon("go-home"));
            m_homeButton->setText(i18n("Home"));
            connect(m_homeButton, SIGNAL(activated()), this, SLOT(reset()));
            //FIXME: do it for each theme change, another place where anchorlayout would shine, now there is an hardcoded value, not acceptable
            m_homeButton->setPos(QPoint(0, 32) + m_mainLayout->contentsRect().topLeft());
            reset();
        }
    }

    if (constraints & Plasma::LocationConstraint) {
        setFormFactorFromLocation(location());
    }

    if (constraints & Plasma::SizeConstraint) {
        m_relayoutTimer->start();
    }

}

void SearchLaunch::paintInterface(QPainter *painter,
                                 const QStyleOptionGraphicsItem *option,
                                 const QRect& contentsRect)
{
    Q_UNUSED(contentsRect)
    Containment::paintInterface(painter, option, contentsRect);
    m_background->resizeFrame(contentsRect.size());
    m_background->paintFrame(painter, contentsRect.topLeft());
}

void SearchLaunch::setFormFactorFromLocation(Plasma::Location loc) {
    switch (loc) {
    case Plasma::BottomEdge:
    case Plasma::TopEdge:
        //kDebug() << "setting horizontal form factor";
        setFormFactor(Plasma::Horizontal);
        break;
    case Plasma::RightEdge:
    case Plasma::LeftEdge:
        //kDebug() << "setting vertical form factor";
        setFormFactor(Plasma::Vertical);
        break;
    case Plasma::Floating:
        setFormFactor(Plasma::Planar);
        kDebug() << "Floating is unimplemented.";
        break;
    default:
        kDebug() << "invalid location!!";
    }
}

void SearchLaunch::dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data)
{
    Q_UNUSED(sourceName);

    const QString query(data["query"].toString());
    //Take ownership of the screen if we don't have one
    //FIXME: hardcoding 0 is bad: maybe pass the screen from the dataengine?
    if (!query.isEmpty()) {
        if (screen() < 0) {
            setScreen(0);
        }
        emit activate();
    }

    doSearch(query);
}

bool SearchLaunch::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::GraphicsSceneHoverEnter) {
        Plasma::IconWidget *icon = qobject_cast<Plasma::IconWidget *>(watched);
        if (icon) {
            m_hoverIndicator->show();
            m_hoverIndicator->animatedGeometryTransform(icon->geometry());
        }
    } else if (event->type() == QEvent::GraphicsSceneHoverLeave &&
               qobject_cast<Plasma::Frame *>(watched)) {
               m_hoverIndicator->hide();
    }

    return false;
}

K_EXPORT_PLASMA_APPLET(sal, SearchLaunch)

#include "sal.moc"

