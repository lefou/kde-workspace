/*
  Copyright (c) 2000,2001 Matthias Elter <elter@kde.org>

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

#include <qpainter.h>
#include <qwhatsthis.h>

#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kpixmap.h>
#include <kpixmapeffect.h>

#include "global.h"
#include "aboutwidget.h"
#include "aboutwidget.moc"
#include "modules.h"
#include "moduletreeview.h"

const char * title_text = I18N_NOOP("Configure your desktop environment.");

const char * intro_text = I18N_NOOP("Welcome to the \"KDE Control Center\", "
                                "a central place to configure your "
                                "desktop environment. "
                                "Select an item from the index on the left "
                                "to load a configuration module.");

const char * use_text = I18N_NOOP("Click on the \"<b>Help</b>\" tab on the left to view help "
                        "for the active "
                        "control module. Use the \"<b>Search</b>\" tab if you are unsure "
                        "where to look for "
                        "a particular configuration option.");

const char * version_text = I18N_NOOP("KDE version:");
const char * user_text = I18N_NOOP("User:");
const char * host_text = I18N_NOOP("Hostname:");
const char * system_text = I18N_NOOP("System:");
const char * release_text = I18N_NOOP("Release:");
const char * machine_text = I18N_NOOP("Machine:");

AboutWidget::AboutWidget(QWidget *parent , const char *name, QListViewItem* category)
   : QWidget(parent, name),
      _moduleList(false),
      _category(category)
{
    if (_category)
    {
      _moduleList = true;
    }
    
    setMinimumSize(400, 400);

    // load images
    _part1 = QPixmap(locate("data", "kcontrol/pics/part1.png"));
    _part2 = QPixmap(locate("data", "kcontrol/pics/part2.png"));
    _part3 = QPixmap(locate("data", "kcontrol/pics/part3.png"));

    // sanity check
    if(_part1.isNull() || _part2.isNull() || _part3.isNull()) {
        kdError() << "AboutWidget::paintEvent: Image loading error!" << endl;
        setBackgroundColor(QColor(49,121,172));
    }
    else
        setBackgroundMode(NoBackground); // no flicker

    // set qwhatsthis help
    QWhatsThis::add(this, i18n(intro_text));
}

void AboutWidget::paintEvent(QPaintEvent* e)
{
    QPainter p (this);

    if(_buffer.isNull())
        p.fillRect(0, 0, width(), height(), QBrush(QColor(49,121,172)));
    else
        p.drawPixmap(QPoint(e->rect().x(), e->rect().y()), _buffer, e->rect());
}

void AboutWidget::resizeEvent(QResizeEvent*)
{
   if(_part1.isNull() || _part2.isNull() || _part3.isNull())
        return;

    _buffer.resize(width(), height());

    QPainter p(&_buffer);

    // draw part1
    p.drawPixmap(0, 0, _part1);

    int xoffset = _part1.width();
    int yoffset = _part1.height();

    // draw part2 tiled
    int xpos = xoffset;
    if(width() > xpos)
        p.drawTiledPixmap(xpos, 0, width() - xpos, _part2.height(), _part2);

    // draw title text
    p.setPen(white);
    p.drawText(150, 84, width() - 150, 108 - 84, AlignLeft | AlignVCenter, i18n(title_text));

    // draw intro text
    p.setPen(black);
    p.drawText(28, 128, width() - 28, 184 - 128, AlignLeft | AlignVCenter | WordBreak, i18n(intro_text));

    // fill background
    p.fillRect(0, yoffset, width(), height() - yoffset, QBrush(QColor(49,121,172)));

    // draw part3
    if (height() <= 184) return;

    int yoffset3 = height() - _part3.height();
    int xoffset3 = width() - _part3.width();
    if (xoffset3 < 0) xoffset3 = 0;
    if (height() < 184 + _part3.height()) yoffset3 = 184;

    p.drawPixmap(xoffset3, yoffset3, _part3);

    // draw textbox
    if (height() <= 184 + 50) return;

    xoffset = 25;
    yoffset = 184 + 50;
    int bheight = height() - 184 - 50 - 40;
    int bwidth = width() - _part3.width() + 60;

    if (bheight < 0) bheight = 0;
    if (bwidth < 0) bheight = 0;
    if (bheight > 400) bheight = 400;
    if (bwidth > 500) bwidth = 500;

    KPixmap box(QSize(bwidth, bheight));

    QPainter pb;
    pb.begin(&box);
    pb.fillRect(0, 0, bwidth, bheight, QBrush(QColor(49,121,172)));
    pb.drawPixmap(xoffset3 - xoffset, yoffset3 - yoffset, _part3);
    pb.end();

    box = KPixmapEffect::fade(box, 0.75, white);

    p.drawPixmap(xoffset, yoffset, box);

    p.setViewport(xoffset, yoffset, bwidth, bheight);
    p.setWindow(0, 0, bwidth, bheight);
    p.setClipRect(xoffset, yoffset, bwidth, bheight);

    // draw info text
    xoffset = 10;
    yoffset = 30;

    int fheight = fontMetrics().height();
    int xadd = 120;

    QFont f1 = font();
    QFont f2 = f1;
    f2.setBold(true);

    
    if (!_moduleList)
    {
      // kde version
      p.setFont(f1);
      p.drawText(xoffset, yoffset, i18n(version_text));
      p.setFont(f2);
      p.drawText(xoffset + xadd, yoffset, KCGlobal::kdeVersion());
      yoffset += fheight + 5;
      if(yoffset > bheight) return;

      // user name
      p.setFont(f1);
      p.drawText(xoffset, yoffset, i18n(user_text));
      p.setFont(f2);
      p.drawText(xoffset + xadd, yoffset, KCGlobal::userName());
      yoffset += fheight + 5;
      if(yoffset > bheight) return;

      // host name
      p.setFont(f1);
      p.drawText(xoffset, yoffset, i18n(host_text));
      p.setFont(f2);
      p.drawText(xoffset + xadd, yoffset, KCGlobal::hostName());
      yoffset += fheight + 5;
      if(yoffset > bheight) return;

      // system
      p.setFont(f1);
      p.drawText(xoffset, yoffset, i18n(system_text));
      p.setFont(f2);
      p.drawText(xoffset + xadd, yoffset, KCGlobal::systemName());
      yoffset += fheight + 5;
      if(yoffset > bheight) return;

      // release
      p.setFont(f1);
      p.drawText(xoffset, yoffset, i18n(release_text));
      p.setFont(f2);
      p.drawText(xoffset + xadd, yoffset, KCGlobal::systemRelease());
      yoffset += fheight + 5;
      if(yoffset > bheight) return;

      // machine
      p.setFont(f1);
      p.drawText(xoffset, yoffset, i18n(machine_text));
      p.setFont(f2);
      p.drawText(xoffset + xadd, yoffset, KCGlobal::systemMachine());
      if(yoffset > bheight) return;

      yoffset += 10;

      if(width() < 450 || height() < 450) return;

      // draw use text
      bheight = bheight - yoffset - 10;
      bwidth = bwidth - xoffset - 10;

      p.setFont(f1);

      QString ut = i18n(use_text);
      // do not break message freeze
      ut.replace(QRegExp("<b>"), "");
      ut.replace(QRegExp("</b>"), "");

      p.drawText(xoffset, yoffset, bwidth, bheight, AlignLeft | AlignVCenter | WordBreak, ut);
    }
    else
    {
      QFont headingFont = f2;
      headingFont.setPointSize(headingFont.pointSize()+5);
      headingFont.setUnderline(true);

      p.setFont(headingFont);
      p.drawText(xoffset, yoffset, i18n(static_cast<ModuleTreeItem*>(_category)->caption().latin1()));
      yoffset += fheight + 10;

      // traverse the list
      QListViewItem* pEntry = _category->firstChild();
      while (pEntry != NULL)
        {
          QString szName;
          QString szComment;
          if (static_cast<ModuleTreeItem*>(pEntry)->module())
            {
              szName = static_cast<ModuleTreeItem*>(pEntry)->module()->name();
              szComment = static_cast<ModuleTreeItem*>(pEntry)->module()->comment();
              p.setFont(f2);
              p.drawText(xoffset, yoffset, i18n(szName.latin1()));
              p.setFont(f1);
              p.drawText(xoffset + xadd, yoffset, i18n(szComment.latin1()));
            }
          else
            {
              szName = static_cast<ModuleTreeItem*>(pEntry)->caption();
              p.setFont(f2);
              p.drawText(xoffset, yoffset, i18n(szName.latin1()));
            }

          yoffset += fheight + 5;
          if(yoffset > bheight) return;
          
          pEntry = pEntry->nextSibling();
        }
      }
}
