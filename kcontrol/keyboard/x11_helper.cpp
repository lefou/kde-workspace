/*
 *  Copyright (C) 2010 Andriy Rysin (rysin@kde.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "x11_helper.h"

#include <QtCore/QDebug>
#include <QtGui/QX11Info>
#include <kapplication.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

#ifdef HAVE_XINPUT
#include <X11/extensions/XInput.h>
#endif


// more information about the limit https://bugs.freedesktop.org/show_bug.cgi?id=19501
int X11Helper::MAX_GROUP_COUNT = 4;
const char* X11Helper::LEFT_VARIANT_STR = "(";
const char* X11Helper::RIGHT_VARIANT_STR = ")";

bool X11Helper::xkbSupported(int* xkbOpcode)
{
    // Verify the Xlib has matching XKB extension.

    int major = XkbMajorVersion;
    int minor = XkbMinorVersion;

    if (!XkbLibraryVersion(&major, &minor))
    {
        qWarning() << "Xlib XKB extension " << major << '.' << minor <<
            " != " << XkbMajorVersion << '.' << XkbMinorVersion;
        return false;
    }

    // Verify the X server has matching XKB extension.

    int opcode_rtrn;
    int error_rtrn;
    int xkb_opcode;
    if( ! XkbQueryExtension(QX11Info::display(), &opcode_rtrn, &xkb_opcode, &error_rtrn, &major, &minor)) {
        qWarning() << "X server XKB extension " << major << '.' << minor <<
            " != " << XkbMajorVersion << '.' << XkbMinorVersion;
        return false;
    }

    if( xkbOpcode != NULL ) {
    	*xkbOpcode = xkb_opcode;
    }

    return true;
}

void X11Helper::switchToNextLayout()
{
	int size = getLayoutsList().size();	//TODO: could optimize a bit as we don't need the layouts - just count
	int group = X11Helper::getGroup() + 1;

	if( group >= size ) {
		group = 0;
	}
	X11Helper::setGroup(group);
}

bool X11Helper::setLayout(const QString& layout)
{
	QStringList currentLayouts = getLayoutsList();
	int idx = currentLayouts.indexOf(layout);
	if( idx == -1 || idx >= X11Helper::MAX_GROUP_COUNT ) {
		qWarning() << "Layout" << layout << "is not found in current layout list" << currentLayouts;
		return false;
	}

	return X11Helper::setGroup((unsigned int)idx);
}

bool X11Helper::setDefaultLayout() {
	return X11Helper::setGroup(0);
}

bool X11Helper::isDefaultLayout() {
	return X11Helper::getGroup() == 0;
}

QString X11Helper::getCurrentLayout()
{
	QStringList currentLayouts = getLayoutsList();
	unsigned int group = X11Helper::getGroup();
	if( group < (unsigned int)currentLayouts.size() )
		return currentLayouts[group];

	qWarning() << "Current group number" << group << "is outside of current layout list" << currentLayouts;
	return "";
}

QStringList X11Helper::getLayoutsList()
{
	XkbConfig xkbConfig;
	QStringList layouts;
	if( X11Helper::getGroupNames(QX11Info::display(), &xkbConfig, LAYOUTS_ONLY) ) {
		for(int i=0; i<xkbConfig.layouts.size(); i++) {
			QString layout(xkbConfig.layouts[i]);
			if( i<xkbConfig.variants.size() && ! xkbConfig.variants[i].isEmpty() ) {
				layout += X11Helper::LEFT_VARIANT_STR + xkbConfig.variants[i] + X11Helper::RIGHT_VARIANT_STR;
			}
			layouts << layout;
		}
	}
	else {
		qWarning() << "Failed to get layout groups from X server";
	}
	return layouts;
}

bool X11Helper::setGroup(unsigned int group)
{
	return XkbLockGroup(QX11Info::display(), XkbUseCoreKbd, group);
}

unsigned int X11Helper::getGroup()
{
	XkbStateRec xkbState;
	XkbGetState( QX11Info::display(), XkbUseCoreKbd, &xkbState );
	return xkbState.group;
}

bool X11Helper::getGroupNames(Display* display, XkbConfig* xkbConfig, FetchType fetchType)
{
	static const char* OPTIONS_SEPARATOR = ",";

	Atom real_prop_type;
	int fmt;
	unsigned long nitems, extra_bytes;
	char *prop_data = NULL;
	Status ret;

	qDebug() << "Fetching layout groups from X server";

	Atom rules_atom = XInternAtom(display, _XKB_RF_NAMES_PROP_ATOM, False);

	/* no such atom! */
	if (rules_atom == None) {       /* property cannot exist */
		qWarning() << "Failed to fetch layouts from server:" << "could not find the atom" << _XKB_RF_NAMES_PROP_ATOM;
		return false;
	}

	ret = XGetWindowProperty(display,
			DefaultRootWindow(display),
			rules_atom, 0L, _XKB_RF_NAMES_PROP_MAXLEN,
			False, XA_STRING, &real_prop_type, &fmt,
			&nitems, &extra_bytes,
			(unsigned char **) (void *) &prop_data);

	/* property not found! */
	if (ret != Success) {
		qWarning() << "Failed to fetch layouts from server:" << "Could not get the property";
		return false;
	}

	/* has to be array of strings */
	if ((extra_bytes > 0) || (real_prop_type != XA_STRING) || (fmt != 8)) {
		if (prop_data)
			XFree(prop_data);
		qWarning() << "Failed to fetch layouts from server:" << "Wrong property format";
		return false;
	}

//	qDebug() << "prop_data:" << nitems << prop_data;
	QStringList names;
	for(char* p=prop_data; p-prop_data < (long)nitems && p != NULL; p += strlen(p)+1) {
		names.append( p );
//		qDebug() << " " << p;
	}

	if( names.count() < 4 ) { //{ rules, model, layouts, variants, options }
		XFree(prop_data);
		return false;
	}

	QStringList layouts = names[2].split(OPTIONS_SEPARATOR);
	QStringList variants = names[3].split(OPTIONS_SEPARATOR);

	for(int ii=0; ii<layouts.count(); ii++) {
		xkbConfig->layouts << (layouts[ii] != NULL ? layouts[ii] : "");
		xkbConfig->variants << (ii < variants.count() && variants[ii] != NULL ? variants[ii] : "");
	}
	qDebug() << "\tlayouts:" << xkbConfig->layouts;
	qDebug() << "\tvariants:" << xkbConfig->variants;

	if( fetchType == ALL ) {
		xkbConfig->keyboardModel = (names[1] != NULL ? names[1] : "");

		if( names.count() >= 5 ) {
			QString options = (names[4] != NULL ? names[4] : "");
			xkbConfig->options = options.split(OPTIONS_SEPARATOR);
			qDebug() << "\toptions:" << options;
		}
	}

	XFree(prop_data);
	return true;
}

XEventNotifier::XEventNotifier(EventType eventType_, QWidget* parent):
		QWidget(parent),
		xkbOpcode(-1),
		xinputEventType(-1),
		eventType(eventType_)
{
	if( KApplication::kApplication() == NULL ) {
		qWarning() << "Layout Widget won't work properly without KApplication instance";
	}
}

void XEventNotifier::start()
{
	if( KApplication::kApplication() != NULL && X11Helper::xkbSupported(&xkbOpcode) ) {
		if( eventType != XKB ) {
			XEventNotifier::registerForNewDeviceEvent(QX11Info::display());
		}
		if( eventType != XINPUT ) {
			XEventNotifier::registerForXkbEvents(QX11Info::display());
		}

		// start the event loop
		KApplication::kApplication()->installX11EventFilter(this);
	}
}

void XEventNotifier::stop()
{
	if( KApplication::kApplication() != NULL ) {
		//TODO: unregister
		if( eventType != XKB ) {
			//    XEventNotifier::unregisterForNewDeviceEvent(QX11Info::display());
		}
		if( eventType != XINPUT ) {
			//    XEventNotifier::unregisterForXkbEvents(QX11Info::display());
		}

		// stop the event loop
		KApplication::kApplication()->removeX11EventFilter(this);
	}
}

bool XEventNotifier::isXkbEvent(XEvent* event)
{
	return event->type == xkbOpcode;
}

bool XEventNotifier::x11Event(XEvent * event)
{
	//    qApp->x11ProcessEvent ( event );
	if( isXkbEvent(event) ) {
		if( XEventNotifier::isGroupSwitchEvent(event) ) {
			emit(layoutChanged());
		}
		else if( XEventNotifier::isLayoutSwitchEvent(event) ) {
			emit(layoutMapChanged());
		}
	}
	else if( XEventNotifier::isNewDeviceEvent(event) ) {
		emit(newDevice());
	}
	return QWidget::x11Event(event);
}

bool XEventNotifier::isGroupSwitchEvent(XEvent* event)
{
    XkbEvent *xkbEvent = (XkbEvent*) event;
#define GROUP_CHANGE_MASK \
    ( XkbGroupStateMask | XkbGroupBaseMask | XkbGroupLatchMask | XkbGroupLockMask )

    return xkbEvent->any.xkb_type == XkbStateNotify && xkbEvent->state.changed & GROUP_CHANGE_MASK;
}

bool XEventNotifier::isLayoutSwitchEvent(XEvent* event)
{
    XkbEvent *xkbEvent = (XkbEvent*) event;

    return //( (xkbEvent->any.xkb_type == XkbMapNotify) && (xkbEvent->map.changed & XkbKeySymsMask) ) ||
/*    	  || ( (xkbEvent->any.xkb_type == XkbNamesNotify) && (xkbEvent->names.changed & XkbGroupNamesMask) || )*/
    	   (xkbEvent->any.xkb_type == XkbNewKeyboardNotify);
}

int XEventNotifier::registerForXkbEvents(Display* display)
{
    int eventMask = XkbNewKeyboardNotifyMask | XkbStateNotifyMask;
    if( ! XkbSelectEvents(display, XkbUseCoreKbd, eventMask, eventMask) ) {
    	qWarning() << "Couldn't select desired XKB events";
    	return false;
    }
    return true;
}

