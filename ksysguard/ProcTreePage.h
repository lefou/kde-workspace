/*
    KTop, a taskmanager and cpu load monitor
   
    Copyright (C) 1997 Bernd Johannes Wuebben
                       wuebben@math.cornell.edu

    Copyright (C) 1998 Nicolas Leclercq
                       nicknet@planete.net
    
	Copyright (c) 1999 Chris Schlaeger
	                   cs@kde.org
    
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

// $Id$

#ifndef _ProcTreePage_H_
#define _ProcTreePage_H_

#include <qstring.h>
#include <qwidget.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlayout.h>

#include <kapp.h>

#include "ProcessTree.h"

extern KApplication* Kapp;

/**
 * This class is responsible for creating the process tree tab in the tab
 * dialog. It creates the buttons and the combo box. For the tree view it
 * uses a KTopProcTree object.
 */
class ProcTreePage : public QWidget
{
	Q_OBJECT

public:
	ProcTreePage(QWidget *parent = 0, const char *name = 0);
	~ProcTreePage()
	{
		delete bRefresh;
		delete bRoot;
		delete bKill;
		delete cbSort;
		delete box;
		delete pTree;

		delete gm;
	}

	void resizeEvent(QResizeEvent* ev);

	/**
	 * This function returns the process id of the currently seletected
	 * process. If no process is selected -1 is returned.
	 */
	int selectionPid(void)
	{
		return (pTree->selectedProcess());
	}

	void clearSelection(void)
	{
		pTree->clearSelection();
	}

	void saveSettings(void)
	{
		QString t;

		/* save sort criterium (process tree) */
		Kapp->getConfig()->writeEntry(QString(cfgkey_pTreeSort),
									  t.setNum(sortby), TRUE);
	}

public slots:

	void update()
	{
		pTree->update();
	}

	void handleSortChange(int idx);

	void killTask();

signals:
	void killProcess(int);

private:
	/**
	 * This function gets an int value from the config file. The value is
	 * specified with the tag string. If no value is found the 'val'
	 * cal-by-ref parameter is not changed.
	 */
	bool loadSetting(int& val, const char* tag)
	{
		QString tmpStr = Kapp->getConfig()->readEntry(tag);
		bool res;
		int tmpInt = tmpStr.toInt(&res);
		if (res)
		{
			val = tmpInt;
			return (true);
		}
		return (false);
	}

	QPushButton* bRefresh;
	QPushButton* bRoot;
	QPushButton* bKill; 
	QComboBox* cbSort;
	QGroupBox* box;
	ProcessTree* pTree;

	QVBoxLayout* gm;
	QHBoxLayout* gm1;

	int sortby;
	char cfgkey_pTreeSort[12];
} ;

#endif
