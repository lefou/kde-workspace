/*
    KTop, the KDE Task Manager and System Monitor
   
	Copyright (c) 1999, 2000 Chris Schlaeger <cs@kde.org>
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	KTop is currently maintained by Chris Schlaeger <cs@kde.org>. Please do
	not commit any changes without consulting me first. Thanks!

	$Id$
*/

#ifndef _FancyPlotter_h_
#define _FancyPlotter_h_

#include <qlabel.h>
#include <qsize.h>

#include "SensorDisplay.h"
#include "SignalPlotter.h"

class QGroupBox;

class FancyPlotter : public SensorDisplay
{
	Q_OBJECT

public:
	FancyPlotter(QWidget* parent = 0, const char* name = 0,
				 const char* title = 0, int min = 0,
				 int max = 100);
	~FancyPlotter();

	bool addSensor(const QString& hostName, const QString& sensorName,
				   const QString& title);

	void addSample(int s0, int s1 = 0, int s2 = 0, int s3 = 0, int s4 = 0)
	{
		plotter->addSample(s0, s1, s2, s3, s4);
	}

	void setLowPass(bool lp)
	{
		plotter->setLowPass(lp);
	}

	virtual QSize sizeHint(void);

	virtual void answerReceived(int id, const QString& s);

	bool load(QDomElement& el);
	bool save(QTextStream& s);

protected:
	virtual void resizeEvent(QResizeEvent*);

private:
	int beams;

	QGroupBox* meterFrame;

	SignalPlotter* plotter;
} ;

#endif
