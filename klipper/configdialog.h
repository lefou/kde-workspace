/* -------------------------------------------------------------

   configdialog.h (part of Klipper - Cut & paste history for KDE)

   $Id$

   (C) 2000 by Carsten Pfeiffer <pfeiffer@kde.org>

   Licensed under the Artistic License

 ------------------------------------------------------------- */

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <qcheckbox.h>
#include <qevent.h>
#include <qgroupbox.h>
#include <qheader.h>
#include <qvgroupbox.h>

#include <kdialogbase.h>
#include <keditlistbox.h>
#include <kkeydialog.h>
#include <klistview.h>
#include <knuminput.h>

#include "urlgrabber.h"

class KGlobalAccel;
class KListView;

class GeneralWidget : public QVGroupBox
{
    Q_OBJECT

    friend class ConfigDialog;

public:
    GeneralWidget( QWidget *parent, const char *name );
    ~GeneralWidget();

private:
    QCheckBox *cbMousePos, *cbSaveContents, *cbReplayAIH;
    KIntNumInput *popupTimeout, *maxItems;

};

class ActionWidget : public QVGroupBox
{
    Q_OBJECT

    friend class ConfigDialog;

public:
    ActionWidget( const ActionList *list, QWidget *parent, const char *name );
    ~ActionWidget();

    /**
     * Creates a list of actions from the listView and returns a pointer to
     * the list.
     * Make sure to free that pointer when you don't need it anymore.
     */
    ActionList * actionList();

private slots:
    void slotAddAction();
    void slotDeleteAction();
    void slotRightPressed( QListViewItem *, const QPoint&, int col );
    void slotItemChanged( QListViewItem *, const QPoint& , int );

private:
    KListView *listView;
    KEditListBox *editListBox;
};

class KeysWidget : public QVGroupBox
{
    Q_OBJECT

    friend class ConfigDialog;

public:
    KeysWidget( KKeyEntryMap *keyMap, QWidget *parent, const char *name );
    ~KeysWidget();

private:
    KKeyChooser *keyChooser;
};


class ConfigDialog : public KDialogBase
{
    Q_OBJECT

public:
    ConfigDialog( const ActionList *list, KKeyEntryMap *keyMap );
    ~ConfigDialog();

    ActionList * actionList() const { return actionWidget->actionList(); }

    bool keepContents()    const {
	return generalWidget->cbSaveContents->isChecked();
    }
    bool popupAtMousePos() const {
	return generalWidget->cbMousePos->isChecked();
    }

    bool replayActionInHistory() const {
	return generalWidget->cbReplayAIH->isChecked();
    }
    int popupTimeout() const {
	return generalWidget->popupTimeout->value();
    }
    int maxItems() const {
	return generalWidget->maxItems->value();
    }
    QStringList noActionsFor() const {
	return actionWidget->editListBox->items();
    }

    void setKeepContents( bool enable ) {
	generalWidget->cbSaveContents->setChecked( enable );
    }
    void setPopupAtMousePos( bool enable ) {
	generalWidget->cbMousePos->setChecked( enable );
    }
    void setReplayActionInHistory( bool enable ) {
	generalWidget->cbReplayAIH->setChecked( enable );
    }
    void setPopupTimeout( int timeout ) {
	generalWidget->popupTimeout->setValue( timeout );
    }
    void setMaxItems( int items ) {
	generalWidget->maxItems->setValue( items );
    }
    void setNoActionsFor( const QStringList& items ) {
	actionWidget->editListBox->insertStringList( items );
    }

    virtual void show();

private:
    GeneralWidget *generalWidget;
    ActionWidget *actionWidget;
    KeysWidget *keysWidget;

};

class ListView : public KListView
{
public:
    ListView( QWidget *parent, const char *name )
	: KListView( parent, name ) {}
    // QListView has a weird idea of a sizeHint...
    virtual QSize sizeHint () const {
	int w = minimumSizeHint().width();
	int h = header()->height();
	h += viewport()->sizeHint().height();
	h += horizontalScrollBar()->height();
	
	QListViewItem *item = firstChild();
	while ( item ) {
	    h += item->totalHeight();
	    item = item->nextSibling();
	}

	qDebug("*** w, h: %i, %i", w, h);	
	return QSize( w, h );
    }

};

#endif // CONFIGDIALOG_H
