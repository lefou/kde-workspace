/*
    KTop, the KDE Task Manager and System Monitor
   
	Copyright (c) 1999 - 2001 Chris Schlaeger <cs@kde.org>
    
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

#include <qdragobject.h>
#include <qdom.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qpopupmenu.h>
#include <qspinbox.h>
#include <qwhatsthis.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>

#include "WorkSheet.h"
#include "SensorManager.h"
#include "FancyPlotter.h"
#include "MultiMeter.h"
#include "DancingBars.h"
#include "ProcessController.h"
#include "WorkSheetSettings.h"
#include "WorkSheet.moc"

WorkSheet::WorkSheet(QWidget* parent) :
	QWidget(parent)
{
	lm = 0;
	rows = columns = 0;
	displays = 0;
	modified = FALSE;
	fileName = "";

	setAcceptDrops(TRUE);
}

WorkSheet::WorkSheet(QWidget* parent, uint r, uint c, uint i) :
	QWidget(parent)
{
	lm = 0;
	displays = 0;
	modified = FALSE;
	fileName = "";
	createGrid(r, c);
	updateInterval = i;

	setAcceptDrops(TRUE);
}

WorkSheet::~WorkSheet()
{
	for (uint i = 0; i < rows; ++i)
		delete [] displays[i];

	delete [] displays;
}

bool
WorkSheet::hasBeenModified() const
{
	for (uint i = 0; i < rows; ++i)
		for (uint j = 0; j < columns; ++j)
			if (((SensorDisplay*)displays[i][j])->hasBeenModified())
				return (TRUE);

	return (modified);
}

bool
WorkSheet::load(const QString& fN)
{
	modified = FALSE;

	QFile file(fileName = fN);
	if (!file.open(IO_ReadOnly))
	{
		KMessageBox::sorry(this, i18n("Can't open the file %1")
						   .arg(fileName));
		return (FALSE);
	}

	// Parse the XML file.
	QDomDocument doc;
	// Read in file and check for a valid XML header.
	if (!doc.setContent(&file))
	{
		KMessageBox::sorry(
			this,
			i18n("The file %1 does not contain valid XML").arg(fileName));
		return (FALSE);
	}
	// Check for proper document type.
	if (doc.doctype().name() != "KSysGuardWorkSheet")
	{
		KMessageBox::sorry(
			this,
			i18n("The file %1 does not contain a valid work sheet\n"
				 "definition, which must have a document type\n"
				 "'KSysGuardWorkSheet'").arg(fileName));
		return (FALSE);
	}
	// Check for proper size.
	QDomElement element = doc.documentElement();
	updateInterval = element.attribute("interval").toUInt();
	if (updateInterval < 2 || updateInterval > 300)
		updateInterval = 2;
	bool rowsOk;
	uint r = element.attribute("rows").toUInt(&rowsOk);
	bool columnsOk;
	uint c = element.attribute("columns").toUInt(&columnsOk);
	if (!(rowsOk && columnsOk))
	{
		KMessageBox::sorry(
			this, i18n("The file %1 has an invalid work sheet size.")
			.arg(fileName));
		return (FALSE);
	}

	createGrid(r, c);

	uint i;

	/* Load lists of hosts that are needed for the work sheet and try
	 * to establish a connection. */
	QDomNodeList dnList = element.elementsByTagName("host");
	for (i = 0; i < dnList.count(); ++i)
	{
		QDomElement element = dnList.item(i).toElement();
		SensorMgr->engage(element.attribute("name"),
						  element.attribute("shell"),
						  element.attribute("command"));
	}

	// Load the displays and place them into the work sheet.
	dnList = element.elementsByTagName("display");
	for (i = 0; i < dnList.count(); ++i)
	{
		QDomElement element = dnList.item(i).toElement();
		uint row = element.attribute("row").toUInt();
		uint column = element.attribute("column").toUInt();
		if (row >= rows || column >= columns)
		{
			kdDebug () << "Row or Column out of range (" << row << ", "
					   << column << ")" << endl;
			return (FALSE);
		}

		QString classType = element.attribute("class");
		SensorDisplay* newDisplay;
		if (classType == "FancyPlotter")
			newDisplay = new FancyPlotter(this);
		else if (classType == "MultiMeter")
			newDisplay = new MultiMeter(this);
		else if (classType == "DancingBars")
			newDisplay = new DancingBars(this);
		else if (classType == "ProcessController")
			newDisplay = new ProcessController(this);
		else
		{
			kdDebug () << "Unkown class " <<  classType << endl;
			return (FALSE);
		}
		CHECK_PTR(newDisplay);
		newDisplay->setUpdateInterval(updateInterval);

		// load display specific settings
		if (!newDisplay->load(element))
			return (FALSE);

		replaceDisplay(row, column, newDisplay);
	}

	modified = FALSE;

	return (TRUE);
}

bool
WorkSheet::save(const QString& fN)
{
	// Parse the XML file.
	QDomDocument doc("KSysGuardWorkSheet");
	doc.appendChild(doc.createProcessingInstruction(
		"xml", "version=\"1.0\" encoding=\"UTF-8\""));

	// save work sheet information
	QDomElement ws = doc.createElement("WorkSheet");
	doc.appendChild(ws);
	ws.setAttribute("interval", updateInterval);
	ws.setAttribute("rows", rows);
	ws.setAttribute("columns", columns);

	QStringList hosts;
	collectHosts(hosts);

	// save host information (name, shell, etc.)
	QStringList::Iterator it;
	for (it = hosts.begin(); it != hosts.end(); ++it)
	{
		QString shell, command;

		if (SensorMgr->getHostInfo(*it, shell, command))
		{
			QDomElement host = doc.createElement("host");
			ws.appendChild(host);
			host.setAttribute("name", *it);
			host.setAttribute("shell", shell);
			host.setAttribute("command", command);
		}
	}
	
	for (uint i = 0; i < rows; ++i)
		for (uint j = 0; j < columns; ++j)
			if (!displays[i][j]->isA("QGroupBox"))
			{
				SensorDisplay* displayP = (SensorDisplay*) displays[i][j];
				QDomElement display = doc.createElement("display");
				ws.appendChild(display);
				display.setAttribute("row", i);
				display.setAttribute("column", j);
				display.setAttribute("class", displayP->className());

				displayP->save(doc, display);
			}	

	QFile file(fileName = fN);
	if (!file.open(IO_WriteOnly))
	{
		KMessageBox::sorry(this, i18n("Can't save file %1")
						   .arg(fileName));
		return (FALSE);
	}
	QTextStream s(&file);
	s.setEncoding(QTextStream::UnicodeUTF8);
	s << doc;
	file.close();

	modified = FALSE;
	return (TRUE);
}

SensorDisplay*
WorkSheet::addDisplay(const QString& hostName, const QString& sensorName,
					  const QString& sensorType, const QString& sensorDescr,
					  uint r, uint c)
{
	if (!SensorMgr->engageHost(hostName))
	{
		QString msg = i18n("Impossible to connect to \'%1\'!").arg(hostName);
		KMessageBox::error(this, msg);
		return (0);
	}

	/* If the by 'r' and 'c' specified display is a QGroupBox dummy
	 * display we replace the widget. Otherwise we just try to add
	 * the new sensor to an existing display. */
	if (displays[r][c]->isA("QGroupBox"))
	{
		SensorDisplay* newDisplay = 0;
		/* If the sensor type is supported by more than one display
		 * type we popup a menu so the user can select what display is
		 * wanted. */
		if (sensorType == "integer")
		{
			QPopupMenu pm;
			pm.insertItem(i18n("Select a display type"), 0);
			pm.setItemEnabled(0, FALSE);
			pm.insertSeparator();
			pm.insertItem(i18n("&Signal Plotter"), 1);
			pm.insertItem(i18n("&Multimeter"), 2);
			pm.insertItem(i18n("&BarGraph"), 3);
			switch (pm.exec(QCursor::pos()))
			{
			case 1:
				newDisplay = new FancyPlotter(this, "FancyPlotter",
											  sensorDescr);
				break;
			case 2:
				newDisplay = new MultiMeter(this, "MultiMeter", sensorDescr);
				break;
			case 3:
				newDisplay = new DancingBars(this, "DancingBars", sensorDescr);
				break;
			default:
				return (0);
			}
		}
		else if (sensorType == "float")
		{
			QPopupMenu pm;
			pm.insertItem(i18n("Select a display type"), 0);
			pm.setItemEnabled(0, FALSE);
			pm.insertSeparator();
			pm.insertItem(i18n("&Signal Plotter"), 1);
			pm.insertItem(i18n("&Multimeter"), 2);
			switch (pm.exec(QCursor::pos()))
			{
			case 1:
				newDisplay = new FancyPlotter(this, "FancyPlotter",
											  sensorDescr);
				break;
			case 2:
				newDisplay = new MultiMeter(this, "MultiMeter", sensorDescr);
				break;
			}
		}
		else if (sensorType == "table")
			newDisplay = new ProcessController(this);
		else
		{
			kdDebug() << "Unkown sensor type: " <<  sensorType << endl;
			return (0);
		}
		replaceDisplay(r, c, newDisplay);
	}

	((SensorDisplay*) displays[r][c])->
		addSensor(hostName, sensorName, sensorDescr);
	((SensorDisplay*) displays[r][c])->setUpdateInterval(updateInterval);

	modified = TRUE;
	return ((SensorDisplay*) displays[r][c]);
}

void
WorkSheet::settings()
{
	WorkSheetSettings* wss = new WorkSheetSettings(this, "WorkSheetSettings",
												   true);
	CHECK_PTR(wss);
	/* The sheet name should be changed with the "Save as..." function,
	 * so we don't have to display the display frame. */
	wss->titleFrame->hide();
	wss->resize(wss->sizeHint());

	wss->rows->setValue(rows);	
	wss->columns->setValue(columns);
	wss->interval->setValue(updateInterval);

	if (wss->exec())
	{
		updateInterval = wss->interval->text().toUInt();
		resizeGrid(wss->rows->text().toUInt(),
				   wss->columns->text().toUInt());
	}
	delete wss;
}

void
WorkSheet::showPopupMenu(SensorDisplay* display)
{
	display->settings();
}

void
WorkSheet::removeDisplay(SensorDisplay* display)
{
	for (uint i = 0; i < rows; ++i)
		for (uint j = 0; j < columns; ++j)
			if (displays[i][j] == display)
			{
				delete display;
				insertDummyDisplay(i, j);
				modified = TRUE;
			}
}

void
WorkSheet::dragEnterEvent(QDragEnterEvent* ev)
{
    ev->accept(QTextDrag::canDecode(ev));
}

void
WorkSheet::dropEvent(QDropEvent* ev)
{
	QString dObj;

	if (QTextDrag::decode(ev, dObj))
	{
		// The host name, sensor name and type are seperated by a ' '.
		QString hostName = dObj.left(dObj.find(' '));
		dObj.remove(0, dObj.find(' ') + 1);
		QString sensorName = dObj.left(dObj.find(' '));
		dObj.remove(0, dObj.find(' ') + 1);
		QString sensorType = dObj.left(dObj.find(' '));
		dObj.remove(0, dObj.find(' ') + 1);
		QString sensorDescr = dObj;

		if (hostName.isEmpty() || sensorName.isEmpty() ||
			sensorType.isEmpty())
		{
			return;
		}

		/* Find the sensor display that is supposed to get the drop
		 * event and replace or add sensor. */
		for (uint i = 0; i < rows; ++i)
			for (uint j = 0; j < columns; ++j)
				if (displays[i][j]->geometry().contains(ev->pos()))
				{
					addDisplay(hostName, sensorName, sensorType,
							   sensorDescr, i, j);
					// Notify parent about possibly new minimum size.
					((QWidget*) parent()->parent())->setMinimumSize(
						((QWidget*) parent()->parent())->sizeHint());
					lm->invalidate();
					return;
				}
	}
}

void
WorkSheet::customEvent(QCustomEvent* ev)
{
	if (ev->type() == QEvent::User)
	{
		// SensorDisplays send out this event if they want to be removed.
		removeDisplay((SensorDisplay*) ev->data());
	}
}

void
WorkSheet::insertDummyDisplay(uint r, uint c)
{
	QGroupBox* dummy = new QGroupBox(this, "dummy frame");
	dummy->setTitle(i18n("Drop sensor here"));
	dummy->setMinimumSize(24, 16);
	QWhatsThis::add(dummy, i18n(
		"This is an empty space in a work sheet. Drag a sensor from "
		"the Sensor Browser and drop it here. A sensor display will "
		"appear that allows you to monitor the values of the sensor "
		"over time."));
	displays[r][c] = dummy;
	lm->addWidget(dummy, r, c);
	displays[r][c]->show();

	// Notify parent about possibly new minimum size.
	((QWidget*) parent()->parent())->setMinimumSize(
		((QWidget*) parent()->parent())->sizeHint());
	lm->invalidate();
}

void
WorkSheet::replaceDisplay(uint r, uint c, SensorDisplay* newDisplay)
{
	// remove the old display at this location
	delete displays[r][c];

	// insert new display
	lm->addWidget(newDisplay, r, c);
	newDisplay->show();
	displays[r][c] = newDisplay;
	connect(newDisplay, SIGNAL(showPopupMenu(SensorDisplay*)),
			this, SLOT(showPopupMenu(SensorDisplay*)));

	// Notify parent about possibly new minimum size.
	((QWidget*) parent()->parent())->setMinimumSize(
		((QWidget*) parent()->parent())->sizeHint());
	lm->invalidate();
}

void
WorkSheet::collectHosts(QValueList<QString>& list)
{
	for (uint r = 0; r < rows; ++r)
		for (uint c = 0; c < columns; ++c)
			if (!displays[r][c]->isA("QGroupBox"))
				((SensorDisplay*) displays[r][c])->collectHosts(list);
}

void
WorkSheet::createGrid(uint r, uint c)
{
	if (lm)
		delete lm;

	rows = r;
	columns = c;

	// create grid layout with specified dimentions
	lm = new QGridLayout(this, r, c, 5);
	CHECK_PTR(lm);

	// and fill it with dummy displays
	displays = new QWidget**[rows];
	CHECK_PTR(displays);
	for (r = 0; r < rows; ++r)
	{
		displays[r] = new QWidget*[columns];
		CHECK_PTR(displays[r]);
		for (c = 0; c < columns; ++c)
			insertDummyDisplay(r, c);
	}
	/* set stretch factors for rows and columns */
	for (r = 0; r < rows; ++r)
		lm->setRowStretch(r, 1);
	for (c = 0; c < columns; ++c)
		lm->setColStretch(c, 1);

	lm->activate();
}

void
WorkSheet::resizeGrid(uint newRows, uint newColumns)
{
	uint r, c;
	/* Create new array for display pointers */
	QWidget*** newDisplays = new QWidget**[newRows];
	CHECK_PTR(newDisplays);
	for (r = 0; r < newRows; ++r)
	{
		newDisplays[r] = new QWidget*[newColumns];
		CHECK_PTR(displays[r]);
		for (c = 0; c < newColumns; ++c)
			if (c < columns && r < rows)
				newDisplays[r][c] = displays[r][c];
	}

	/* remove obsolete displays */
	for (r = 0; r < rows; ++r)
	{
		for (c = 0; c < columns; ++c)
			if (r >= newRows || c >= newColumns)
				delete displays[r][c];
		delete displays[r];
	}
	delete displays;

	/* now we make the new display the regular one */
	displays = newDisplays;

	/* create new displays */
	for (r = 0; r < newRows; ++r)
		for (c = 0; c < newColumns; ++c)
			if (r >= rows || c >= columns)
				insertDummyDisplay(r, c);

	/* set stretch factors for new rows and columns (if any) */
	for (r = rows; r < newRows; ++r)
		lm->setRowStretch(r, 1);
	for (c = columns; c < newColumns; ++c)
		lm->setColStretch(c, 1);
	/* Obviously Qt does not shrink the size of the QGridLayout
	 * automatically.  So we simply force the rows and columns that
	 * are no longer used to have a strech factor of 0 and hence be
	 * invisible. */
	for (r = newRows; r < rows; ++r)
		lm->setRowStretch(r, 0);
	for (c = newColumns; c < columns; ++c)
		lm->setColStretch(c, 0);

	rows = newRows;
	columns = newColumns;

	lm->activate();
}
