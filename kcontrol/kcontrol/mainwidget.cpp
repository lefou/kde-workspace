/* This is the default widget for kcc
   Author: Markus Wuebben
	   <markus.wuebben@kde.org>
   Date:   September '97         */


#include <kiconloader.h>

#include "mainwidget.moc"
#include "mainwidget.h"


mainWidget::mainWidget(QWidget *parent , const char *name)
  : QWidget(parent, name)
{
  KIconLoader iconLoader;

  QLabel *heading = new QLabel(klocale->translate("KDE Control Center"),this);
  QFont font("times",18,QFont::Bold);
  pmap = iconLoader.loadIcon("kdekcc.xpm");
  heading->setFont(font);
  heading->setGeometry(120,10,200,40);

  uname(&info);
}


void mainWidget::paintEvent(QPaintEvent *)
{
  QString str;
  char buf[512];
  QPainter p(this);
  
  QFont normalFont("times",12,QFont::Normal);
  QFont boldFont("times",12,QFont::Bold);

  
  p.drawPixmap(10,250,pmap);
  p.setFont(boldFont);
  p.drawText(20,70,"System:");

  p.setFont(boldFont);
  str= "User: ";
  p.drawText(60,90,str);
  str.sprintf("%s",cuserid(NULL));
  p.setFont(normalFont);
  p.drawText(150,90,str);

  str = "Hostname: ";
  p.setFont(boldFont);
  p.drawText(60,110,str);
  gethostname(buf,511);
  str.sprintf("%s",buf);
  p.setFont(normalFont);
  p.drawText(150,110,str);

  str = "System: ";
  p.setFont(boldFont);
  p.drawText(60,130,str);
  str.sprintf("%s",info.sysname);
  p.setFont(normalFont);
  p.drawText(150,130,str);
   
  str = "Release: ";
  p.setFont(boldFont);
  p.drawText(60,150,str);
  str.sprintf("%s",info.release);
  p.setFont(normalFont);
  p.drawText(150,150,str);


  str = "Version: ";
  p.setFont(boldFont);
  p.drawText(60,170,str);
  str.sprintf("%s",info.version);
  p.setFont(normalFont);
  p.drawText(150,170,str);

  str = "Machine: ";
  p.setFont(boldFont);
  p.drawText(60,190,str);
  str.sprintf("%s",info.machine);
  p.setFont(normalFont);
  p.drawText(150,190,str);

  p.end();

}
