/*
  toplevel.h - the mainview of the KDE control center

  written 1997 by Matthias Hoelzer
  
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


#ifndef _TOPLEVEL_H_
#define _TOPLEVEL_H_

#include <qapp.h>
#include <qmenubar.h>
#include <kapp.h>
#include <ktopwidget.h>
#include <ktoolbar.h>
#include <ktreelist.h>
#include <kstatusbar.h>
#include <kpanner.h>

#include "mainwidget.h"
#include "configlist.h"


class TopLevel : public KTopLevelWidget
{
  Q_OBJECT
  
public:

  TopLevel(ConfigList *cl);

  QPopupMenu *file, *helpMenu, *options;
  int        helpModuleID, helpID, swallowID;

protected:

  void setupMenuBar();
  void setupStatusBar();

  virtual void resizeEvent(QResizeEvent *event);

private:

  KMenuBar   *menubar;
  KToolBar   *toolbar;
  KTreeList  *treelist;
  KStatusBar *statusbar;
  KPanner    *panner;
  ConfigList *configList;
  mainWidget *mwidget;

  const int ID_GENERAL;

  KModuleListEntry *current;

  KModuleListEntry *getListEntry(int item);

  void updateMenu();

public slots:

  void about();
 
  void pannerChanged();
  void help();
  void swallowChanged();
    
  void item_selected(int item);
  void item_highlighted(int item);
};

#endif


