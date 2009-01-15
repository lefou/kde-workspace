/***************************************************************************
 *   Copyright (C) 2009 by Ben Cooksley <ben@eclipse.endoftheinternet.org> *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA          *
 ***************************************************************************/

#include "solid-actions.h"
#include "action-item.h"

#include <KGenericFactory>
#include <KStandardDirs>
#include <KDebug>
#include <KAboutData>
#include <KDialog>
#include <KDesktopFile>
#include <KUrl>
#include <kdesktopfileactions.h>
#include <KIO/NetAccess>

#include <QComboBox>
#include <QPushButton>

#include <Solid/DeviceInterface>
#include <Solid/Predicate>

K_PLUGIN_FACTORY(SolidActionsFactory, registerPlugin<SolidActions>();)
K_EXPORT_PLUGIN(SolidActionsFactory("kcmsolidactions", "kcm_solid_actions"))

SolidActions::SolidActions(QWidget* parent, const QVariantList&)
        : KCModule(SolidActionsFactory::componentData(), parent)
{
    KAboutData* about = new KAboutData("Device Actions", 0, ki18n("Solid Device Actions Editor"), "1.0",
                                       ki18n("Solid Device Actions Control Panel Module"),
                                       KAboutData::License_GPL,
                                       ki18n("(c) 2009 Solid Device Actions team"));
    about->addAuthor(ki18n("Ben Cooksley"), ki18n("Maintainer"), "ben@eclipse.endoftheinternet.org");
    setAboutData(about);
    setButtons(KCModule::NoAdditionalButton);

    // Prepare main display dialog
    mainUi = new Ui_SolidActionsConfig();
    mainUi->setupUi(this);
    mainUi->LwActions->setSelectionMode(QAbstractItemView::SingleSelection); // Only a single item should be selectable

    connect(mainUi->PbEditAction, SIGNAL(clicked()), this, SLOT(editAction()));
    connect(mainUi->PbDeleteAction, SIGNAL(clicked()), this, SLOT(deleteAction()));
    connect(mainUi->LwActions, SIGNAL(itemSelectionChanged()), this, SLOT(enableEditDelete()));

    // Prepare + connect up with Edit dialog
    editUi = new SolidActionsEdit(this);
    connect(editUi, SIGNAL(okClicked()), this, SLOT(acceptActionChanges()));

    // Prepare + connect up add action dialog
    addDialog = new KDialog(this);
    addWidget = new QWidget(addDialog);
    addDialog->setMainWidget(addWidget);
    addUi = new Ui_SolidActionAdd();
    addUi->setupUi(addWidget);
    addDialog->setInitialSize(QSize(300, 100)); // Set a sensible default size
    connect(addDialog, SIGNAL(okClicked()), this, SLOT(addAction()));
    connect(mainUi->PbAddAction, SIGNAL(clicked()), addDialog, SLOT(show()));

}


SolidActions::~SolidActions()
{
    clearActions();
    delete mainUi;
    delete editUi;
}

void SolidActions::load()
{
    fillActionsList();
}

void SolidActions::defaults()
{
}

void SolidActions::save()
{
}

void SolidActions::addAction()
{
    QListWidgetItem * newAction = 0;

    QString enteredName = addUi->LeActionName->text();
    // Lets get a desktop file
    QString internalName = enteredName; // copy the name the user entered -> we will be making mods
    internalName.replace(" ", "-", Qt::CaseSensitive); // replace spaces with dashes
    QString filePath = KStandardDirs::locateLocal("data", 0); // Get the location on disk for "data"
    filePath = filePath + "solid/actions/" + internalName + ".desktop"; // Create a file path for new action
    KDesktopFile newDesktop(filePath); // Create the desktop file
    // Fill in an inital template
    newDesktop.actionGroup("open").writeEntry("Icon", "unknown"); // Create a action -> fill XDG required items
    newDesktop.actionGroup("open").writeEntry("Exec", ""); // ditto
    newDesktop.actionGroup("open").writeEntry("Name", enteredName); // ditto
    newDesktop.desktopGroup().writeEntry("Actions", "open;"); // Make sure the new action can be found
    newDesktop.desktopGroup().writeEntry("Type", "Service"); // The desktop file contains a service
    newDesktop.desktopGroup().writeEntry("X-KDE-Solid-Action-Custom", "true"); // This is user created
    // Write the file contents
    newDesktop.sync();
    // Prepare to open the editDialog
    fillActionsList();
    foreach(ActionItem * newItem, actionsDb.values()) { // Lets find our new action
        if (newItem->desktopMasterPath == filePath) {
            newAction = actionsDb.key(newItem); // Grab it
            break;
        }
    }
    mainUi->LwActions->setCurrentItem(newAction); // Set it as currently active
    editAction(); // Open the edit dialog
}

void SolidActions::editAction()
{
    ActionItem * selectedItem = selectedAction();
    // Set all the text appropriately
    editUi->ui.IbActionIcon->setIcon(selectedItem->icon());
    editUi->ui.LeActionFriendlyName->setText(selectedItem->name());
    editUi->ui.LeActionCommand->setPath(selectedItem->exec());
    editUi->fillPredicateTree(selectedItem->readKey(ActionItem::GroupDesktop, "X-KDE-Solid-Predicate", ""));
    editUi->setCaption(i18n("Editing action %1", selectedItem->name())); // Set a friendly i18n caption
    // Display us!
    editUi->show();
}

void SolidActions::deleteAction()
{
    ActionItem * action = selectedAction();
    if (action->isUserSupplied()) { // Is the action user supplied?
        KIO::NetAccess::del(KUrl(action->desktopMasterPath), this); // Remove the main desktop file then
    }
    KIO::NetAccess::del(KUrl(action->desktopWritePath), this); // Remove the modified desktop file now
    fillActionsList(); // Update the list of actions
}

QListWidgetItem * SolidActions::selectedWidget()
{
    foreach(QListWidgetItem* candidate, actionsDb.keys()) { // Lets find the selected item
        if (candidate->isSelected()) { // Is it selected?
            return candidate; // We have found the selected item
        }
    }
    return 0; // Return null -> couldn't be found ( which can't happen normally )
}

ActionItem * SolidActions::selectedAction()
{
    QListWidgetItem *action = selectedWidget();
    ActionItem * actionItem = actionsDb.value(action); // Retrieve the current action using the current item
    return actionItem;
}

void SolidActions::fillActionsList()
{
    QStringList allPossibleActions;

    clearActions();
    // Prepare to search for possible actions -> we only want solid types
    allPossibleActions = KGlobal::dirs()->findAllResources("data", "solid/actions/");
    // Get service objects for those actions and add them to the display
    foreach(const QString &desktop, allPossibleActions) {
        KUrl desktopFile(desktop); // create a desktop item for the solid action file
        QString filePath = KStandardDirs::locate("data", "solid/actions/" + desktopFile.fileName()); // Get its on disk location
        QList<KServiceAction> services = KDesktopFileActions::userDefinedServices(filePath, true); // Get the list of contained services
        foreach(const KServiceAction &deviceAction, services) { // We want every single action
            ActionItem * actionItem = new ActionItem(filePath, deviceAction.name(), this); // Create an action
            QListWidgetItem * actionListItem = new QListWidgetItem(KIcon(actionItem->icon()), actionItem->name(), 0, QListWidgetItem::Type);
            mainUi->LwActions->insertItem(mainUi->LwActions->count(), actionListItem); // Add it to the list
            actionsDb.insert(actionListItem, actionItem);  // add action to action db
        }
    }
    toggleEditDelete(false); // Switch off the ability to edit / delete -> no-one is selected
}

void SolidActions::acceptActionChanges()
{
    ActionItem *selectedItem = selectedAction();
    // apply the changes
    if (editUi->ui.IbActionIcon->icon() != selectedItem->icon()) { // Has the icon changed?
        selectedItem->setIcon(editUi->ui.IbActionIcon->icon()); // Write the change
    }
    if (editUi->ui.LeActionFriendlyName->text() != selectedItem->name()) { // Has the friendly name changed?
        selectedItem->setName(editUi->ui.LeActionFriendlyName->text()); // Write the change
    }
    if (editUi->ui.LeActionCommand->text() != selectedItem->exec()) { // Has the command changed?
        selectedItem->setExec(editUi->ui.LeActionCommand->text()); // Write the change
    }
    QString newPredicate = editUi->predicate(); // retrieve the predicate
    if (newPredicate != selectedItem->readKey(ActionItem::GroupDesktop, "X-KDE-Solid-Predicate", "")) { // Has it changed?
        selectedItem->setKey(ActionItem::GroupDesktop, "X-KDE-Solid-Predicate", newPredicate); // Write the change
    }
    // Re-read the actions list to ensure changes are reflected
    fillActionsList();
}

void SolidActions::toggleEditDelete(bool toggle)
{
    mainUi->PbEditAction->setEnabled(toggle); // Change them to the new state
    mainUi->PbDeleteAction->setEnabled(toggle); // Ditto

    if (mainUi->LwActions->selectedItems().count() == 0) { // Is an action selected?
        mainUi->PbDeleteAction->setText(i18n("No Action Selected")); // Set a friendly disabled text
        return;
    }

    KUrl writeDesktopFile(selectedAction()->desktopWritePath); // Get the write desktop file
    if (selectedAction()->isUserSupplied()) { // Is the action user supplied?
        mainUi->PbDeleteAction->setText(i18n("Delete Action")); // We can directly delete it then
    } else if (KIO::NetAccess::exists(writeDesktopFile, true, this)) { // Does the write file exist?
        mainUi->PbDeleteAction->setText(i18n("Revert Modifications")); // Otherwise we can only revert
    } else {
        mainUi->PbDeleteAction->setText(i18n("Cannot be deleted")); // We cannot do anything then
        mainUi->PbDeleteAction->setEnabled(false); // So disable the ability to delete
    }
}

void SolidActions::enableEditDelete()
{
    toggleEditDelete(true);
}

void SolidActions::clearActions()
{
    foreach(ActionItem * deleteAction, actionsDb.values()) { // destroy all action items
        delete deleteAction;
    }
    mainUi->LwActions->clear(); // Clear the list of actions
    actionsDb.clear(); // Empty out the db
}

#include "solid-actions.moc"
