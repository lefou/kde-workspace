    /*

    Copyright (C) 2000 Stefan Westerfeld
                       stefan@space.twc.de

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

    Permission is also granted to link this program with the Qt
    library, treating Qt like a library that normally accompanies the
    operating system kernel, whether or not that is in fact the case.

    */

#ifndef KARTSCONFIG_H
#define KARTSCONFIG_H

#include <kapp.h>

#include <qlabel.h>
#include <qcheckbox.h>

#include <kcmodule.h>

#include <qbuttongroup.h>
#include <qradiobutton.h>


class KArtsModule : public KCModule
{
  Q_OBJECT

public:

  KArtsModule(QWidget *parent=0, const char *name=0);

  void saveParams( void );

  void load();
  void save();
  void defaults();

  QString quickHelp() const;

private slots:

  void slotChanged();
 

private:

  void updateWidgets(void);
  void GetSettings( void );

  QCheckBox *startServer, *startRealtime, *networkTransparent, *x11Comm,
  			*fullDuplex;
  QButtonGroup *responseGroup;
  QRadioButton *responseButton[3];

  KConfig *config;
};


#endif

