/*****************************************************************************
 *  Copyright (C) 2012 by Shaun Reich <shaun.reich@kdemail.net>               *
 *                                                                            *
 *  This library is free software; you can redistribute it and/or modify      *
 *  it under the terms of the GNU Lesser General Public License as published  *
 *  by the Free Software Foundation; either version 2 of the License or (at   *
 *  your option) any later version.                                           *
 *                                                                            *
 *  This library is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 *  Library General Public License for more details.                          *
 *                                                                            *
 *  You should have received a copy of the GNU Lesser General Public License  *
 *  along with this library; see the file COPYING.LIB.                        *
 *  If not, see <http://www.gnu.org/licenses/>.                               *
 *****************************************************************************/

#include "svgviewer.h"
#include "plasmaview.h"

#include <QApplication>
#include <QDir>
#include <QLabel>
#include <QStandardItemModel>
#include <QScrollArea>
#include <QTreeView>
#include <QTimer>
#include <QVBoxLayout>

#include <KIO/DeleteJob>
#include <KDebug>
#include <KDirWatch>
#include <KComboBox>
#include <KGlobal>
#include <KStandardDirs>
#include <KIconLoader>
#include <KIconTheme>
#include <KMenu>
#include <KStandardAction>
#include <KStringHandler>
#include <KAction>
#include <KTabWidget>

#include <Plasma/Svg>
#include <Plasma/Theme>
#include <Plasma/Corona>
#include <Plasma/ContainmentActions>
#include <Plasma/Containment>

SvgViewer::SvgViewer(QWidget* parent)
    : KDialog(parent)
    , m_themeSelector(0)
    , m_tabWidget(0)
    , m_shellContainer(0)
    , m_svgFilesTree(0)
    , m_scrollAreaContainer(0)
    , m_scrollArea(0)
    , m_dataModel(0)
    , m_currentTheme(0)
    , m_currentSvg(0)
    , m_svgPreviewImage(0)
    , m_svgFilesLabel(0)
    , m_corona(0)
    , m_plasmaView_desktop(0)
    , m_plasmaView_panel(0)
    , m_dirWatch(0)
{
    setWindowTitle(i18n("Plasma SVG Viewer"));
    setWindowState(Qt::WindowMaximized);
    QWidget* mainWidget = new QWidget(this);
    setMainWidget(mainWidget);

    m_themeSelector = new KComboBox(this);
    m_svgFilesLabel = new QLabel(this);
    m_svgFilesLabel->setText(i18n("SVG files for theme:"));

    m_svgFilesTree = new QTreeView(this);

    m_tabWidget = new KTabWidget(this);

    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->addWidget(m_themeSelector);
    leftLayout->addWidget(m_svgFilesLabel);
    leftLayout->addWidget(m_svgFilesTree);

    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->addLayout(leftLayout, 0);
    mainLayout->addWidget(m_tabWidget, 1);

    m_shellContainer = new QWidget(this);
    m_scrollArea = new QScrollArea(this);

    m_tabWidget->addTab(m_shellContainer, i18n("Plasma Preview Shell"));
    m_tabWidget->addTab(m_scrollArea, i18n("SVG Preview"));

    mainWidget->setLayout(mainLayout);

    m_currentTheme = new Plasma::Theme(this);

    m_currentSvg = new Plasma::Svg(this);
    m_currentSvg->setUsingRenderingCache(false);

    m_svgPreviewImage = new QLabel(this);
    //set "NO" icon if no icon is loaded.

    m_scrollArea->setWidget(m_svgPreviewImage);

    m_dataModel = new QStandardItemModel(this);

    connect(m_themeSelector, SIGNAL(currentIndexChanged(QString)), this, SLOT(loadTheme(QString)));

    m_svgFilesTree->setModel(m_dataModel);
    m_svgFilesTree->setWordWrap(true);

    connect(m_svgFilesTree, SIGNAL(activated(QModelIndex)), SLOT(modelIndexChanged(QModelIndex)));

    connect(m_svgFilesTree->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            SLOT(modelSelectionChanged(QModelIndex,QModelIndex)));

    setButtons(KDialog::Close);

    addAction(KStandardAction::quit(qApp, SLOT(quit()), this));

    const QStringList watchedDirs = KGlobal::dirs()->findDirs("data", "desktoptheme/");

    m_dirWatch = new KDirWatch(this);
    foreach (const QString& dir, watchedDirs) {
        m_dirWatch->addDir(dir, KDirWatch::WatchSubDirs);
    }

    connect(m_dirWatch, SIGNAL(dirty(QString)), SLOT(themesDirty(QString)));

    //TODO: connect to a signal if themes are changed?
    reloadThemeList();

    // find all theme names we know, populate combobox
    m_themeSelector->addItems(m_themeMap.keys());

//    m_shellContainer->resize(size());

    m_corona = new Plasma::Corona(this);

    connect(m_corona, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(sceneRectChanged(QRectF)));

    Plasma::ContainmentActionsPluginsConfig containmentActionPlugins;
    containmentActionPlugins.addPlugin(Qt::NoModifier, Qt::RightButton, "contextmenu");

    m_corona->setContainmentActionsDefaults(Plasma::Containment::DesktopContainment, containmentActionPlugins);
    m_corona->setContainmentActionsDefaults(Plasma::Containment::CustomContainment, containmentActionPlugins);
    m_corona->setContainmentActionsDefaults(Plasma::Containment::PanelContainment, containmentActionPlugins);
    m_corona->setContainmentActionsDefaults(Plasma::Containment::CustomPanelContainment, containmentActionPlugins);

    m_shellContainer->resize(1300,1300);

    // create one of desktop containment and also a panel containment
    m_plasmaView_desktop = new PlasmaView(m_corona->addContainment("desktop"), m_shellContainer);
    m_plasmaView_desktop->setScene(m_corona);
    m_plasmaView_desktop->setMaximumSize(1300, 1300);
    m_plasmaView_desktop->resize(1300, 1300);

    m_plasmaView_panel = new PlasmaView(m_corona->addContainment("panel"), m_shellContainer);
    m_plasmaView_panel->setScene(m_corona);
    m_plasmaView_panel->resize(1300, 40);
    // yes. hardcoded dragons.
    m_plasmaView_panel->move(0, 740);

    // HACK another sizing one...size() of anything returns something small
    // for some reason. maybe because it's a kdialog? i don't know..
//    m_plasmaView_desktop->resize(1300,1000);
}

SvgViewer::~SvgViewer()
{
    delete m_currentTheme;
    delete m_currentSvg;
}

void SvgViewer::reloadThemeList()
{
    m_themeMap.clear();

    const KPluginInfo::List& infoList = Plasma::Theme::listThemeInfo();
    foreach(const KPluginInfo& info, infoList) {
        m_themeMap.insert(info.pluginName(), info);
//        kDebug() << "added theme name and kplugininfo to hash: " << info.name();
    }

    //kDebug() <<    KGlobal::dirs()->findDirs("data", "desktoptheme");
    //KGlobal::dirs()->findAllResources("data", "desktoptheme/*");
}

void SvgViewer::clearThemeCache()
{
    kDebug() << "wiping theme cache";
    const QStringList caches = KGlobal::dirs()->findAllResources("cache", "*.kcache", KStandardDirs::Recursive);
    kDebug() << "ALLRESOURCES: " << caches;
    foreach (const QString& cache, caches) {
        kDebug() << "DFAURE: deleting theme cache file: " << cache;
        KIO::del(KUrl(cache), KIO::HideProgressInfo);
    }
}

void SvgViewer::themesDirty(const QString& file)
{
    //suspend dir watching, otherwise we get infinite requests
    m_dirWatch->stopScan();

    kDebug() << "themes dir has been dirtied. cache cleared, reloading themeName: " << m_currentTheme->themeName();
    //yeah, not the cleanest of approaches. couldn't find a way to get it to reload it without loading a different one, though.
    foreach (const QString& theme, m_themeMap.keys()) {
        kDebug() << "SEARCHING for theme that does not match ours, so we can switch to it, clear cache, then go back to the one we want";
        if (theme != m_currentTheme->themeName()) {
            Plasma::Theme::defaultTheme()->setThemeName(theme);
            goto done;
        }
    }
    //catching if we run out of non-current themes
    Q_ASSERT(0);
done:

    loadTheme(m_currentTheme->themeName());
    m_dirWatch->startScan();
}

void SvgViewer::loadTheme(const QString& themeName)
{
    kDebug() << "begin loading theme: " << themeName;


    // we're loading a new theme, wipe the old cache
    clearThemeCache();

    m_currentTheme->setThemeName(themeName);
    m_currentSvg->setTheme(m_currentTheme);

    Plasma::Theme::defaultTheme()->setThemeName(themeName);

    // wipe model prepare to reload
    m_dataModel->clear();
    m_dataModel->setColumnCount(1);

    QStringList headers;
    headers << i18n("SVG Elements");
    m_dataModel->setHorizontalHeaderLabels(headers);

    QString filePath = themeName;

 //   kDebug() << "loaded theme has entry path: " << filePath;

    // only interested in the parent dir's name.
    // e.g. ascii, Air, etc.
    QFileInfo fileInfo = QFileInfo(filePath);
    QString directoryName = fileInfo.dir().dirName();

  //  kDebug() << "searching for resources/elements in dir: " << directoryName;

    QStringList themeElementList =
        KGlobal::dirs()->findAllResources("data", "desktoptheme/" + directoryName + "/*/*.svgz", KStandardDirs::Recursive);

//    kDebug() << "theme element list, begin populating: " << themeElementList;

    themeElementList.sort();

//    kDebug() << "theme element list after qsort: " << themeElementList;

    QString previousDirName;
    QStandardItem *parentItem = 0;
    int currentRow = 0;

    for (int i = 0; i < themeElementList.length(); ++i) {
        QFileInfo file = QFileInfo(themeElementList.at(i));

        const QString& dirName = file.dir().dirName() + '/';

        if (previousDirName != dirName) {
   //         kDebug() << "creating new dir parent node; previousDirName: " << previousDirName << " dirName: " << dirName;
            //we're on a new dir set, create new parent node
            parentItem = new QStandardItem(dirName);
            m_dataModel->appendRow(parentItem);

            // reset row count since it's a new parent node
            currentRow = 0;
        }

    //    kDebug() << "creating child item: " << file.baseName();
        // produces rows looking like "widgets/viewitem.svgz", "lancelot/..." etc.
        QStandardItem *childItem = new QStandardItem(file.baseName());
        childItem->setData(QVariant(dirName + file.baseName()), Qt::UserRole);
        parentItem->setChild(currentRow, childItem);

        previousDirName = dirName;
        ++currentRow;
    }

}

void SvgViewer::restartDirWatch()
{
}

void SvgViewer::modelIndexChanged(const QModelIndex& index)
{
    // root entry, can't load anything
    if (!index.isValid() || !index.parent().isValid()) {
        return;
    }

    // 'widgets/'
//    QStandardItem *parent = m_dataModel->item(index.parent().row());
//index.child(index.row(), 0)
    // 'viewitem'
    QStandardItem *item =  m_dataModel->itemFromIndex(index);

    const QString& elementPath = item->data(Qt::UserRole).toString();

    kDebug() << "modelIndexChanged loading svg theme: " << m_currentTheme->themeName();
    m_currentSvg->setTheme(m_currentTheme);
//    m_currentSvg->resize(m_svgFilesTree->size());

//    kDebug() << "modelIndexChanged, loading svg elementPath: " << elementPath;
    m_currentSvg->setImagePath(elementPath);

//    kDebug() << "svg native size: " << m_currentSvg->size();

    // HACK if i ever did know one..
    m_currentSvg->resize(1024, 1024);

    // FIXME: size() constantly returns 48x48. looking at svg.cpp:225, calling into
    // ksharedsvgrenderer. problem lies in QSvgRenderer? regression?
    // or am i misusing API here? m_currentSvg->size()
    // SEE ABOVE at m_currentSvg->resize() for a hack
    m_svgPreviewImage->resize(m_currentSvg->size());
    m_svgPreviewImage->setPixmap(m_currentSvg->pixmap());

}

void SvgViewer::modelSelectionChanged(const QModelIndex& current, const QModelIndex& previous)
{
    modelIndexChanged(current);
}

#include "svgviewer.moc"
