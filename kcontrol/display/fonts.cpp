//
// KDE Display fonts setup tab
//
// Copyright (c)  Mark Donohoe 1997
//                lars Knoll 1999

/* $Id$ */

#include <kconfig.h>
#include <ksimpleconfig.h>
#include <kglobal.h>
#include <klocale.h>
#include <kipc.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qlabel.h>
#include <kstddirs.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "fonts.h"

#include "fonts.moc"


FontUseItem::FontUseItem( const QString& n, QFont default_fnt, bool f )
	: selected(0)
{
	_text = n;
	_default = _font = default_fnt;
	fixed = f;
}

QString FontUseItem::fontString( QFont rFont )
{
	QString aValue;
	aValue = rFont.rawName();
	return aValue;
}

void FontUseItem::setRC( const QString& group, const QString& key, const QString&rc )
{
	_rcgroup = group;
	_rckey = key;
	if ( !rc.isNull() ) _rcfile = rc;
}

void FontUseItem::setDefault()
{
	_font = _default;
}

void FontUseItem::readFont()
{
	KConfigBase *config=NULL;

	if ( _rcfile.isEmpty() ) {
	  config  = kapp->config();
	} else {
	  config = new KSimpleConfig( locate("config", _rcfile), true );
	}

	config->setGroup( _rcgroup );
	
	QFont tmpFnt( _font );
	_font = config->readFontEntry( _rckey, &tmpFnt );
}

void FontUseItem::writeFont()
{
	KConfigBase *config;
	if ( _rcfile.isEmpty() ) {
 	    config = kapp->config();
	    config->setGroup( _rcgroup );
	    config->writeEntry( _rckey, _font, true, true );
	} else {
	  config = new KSimpleConfig( locate("config", _rcfile) );
	  config->setGroup( _rcgroup );
	  config->writeEntry( _rckey, _font );
	  config->sync();
	  delete config;
	}
}

//------------------------------------------------------------------

KFonts::KFonts(QWidget *parent, Mode m)
	: KDisplayModule(parent, m)
{
	int i;
	changed = false;

	// if we are just initialising we don't need to create setup widget
	if (mode() == Init)
		return;
	
	setName( i18n("Fonts").ascii() );

	readSettings();
	
	QBoxLayout *topLayout = new QVBoxLayout( this, 10 );
	
	topLayout->addSpacing( 15 );
	
	QBoxLayout *pushLayout = new QHBoxLayout( 5 );
	
	topLayout->addLayout( pushLayout );
	
	FontUseItem *item = new FontUseItem( i18n("General font"),
				QFont( "helvetica", 12 ) );
	item->setRC( "General", "font" );
	fontUseList.append( item );
	
	item = new FontUseItem( i18n("Fixed font"),
				QFont( "fixed", 12 ), true );
	item->setRC( "General", "fixedFont" );
	fontUseList.append( item );
	
	item = new FontUseItem( i18n("Window title font"),
				QFont( "helvetica", 12, QFont::Bold ) );
	item->setRC( "WM", "titleFont" );
	fontUseList.append( item );
				
	item = new FontUseItem( i18n("Panel button font"),
				QFont( "helvetica", 12 )  );
	item->setRC( "kpanel", "DesktopButtonFont", "kpanelrc" );
	fontUseList.append( item );
	
	item = new FontUseItem( i18n("Panel clock font"),
				QFont( "helvetica", 12, QFont::Normal) );
	item->setRC( "kpanel", "DateFont", "kpanelrc" );
	fontUseList.append( item );
	
	for ( i = 0; i < (int) fontUseList.count(); i++ )
		fontUseList.at( i )->readFont();
	
	lbFonts = new QListBox( this );
	for ( i=0; i < (int) fontUseList.count(); i++ )
  	     lbFonts->insertItem( fontUseList.at( i )->text() );
	lbFonts->adjustSize();
	lbFonts->setMinimumSize(lbFonts->size());
	
	connect( lbFonts, SIGNAL( highlighted( int ) ),
		 SLOT( slotPreviewFont( int ) ) );
			
	pushLayout->addWidget(lbFonts, 2);
	
	fntChooser = new KFontChooser( this );
	
	connect( fntChooser, SIGNAL( fontSelected(const QFont &) ), this,
		SLOT( slotSetFont(const QFont &) ) );
	
	pushLayout->addWidget(fntChooser, 5);
	
	lbFonts->setCurrentItem( 0 );

	topLayout->activate();

	changed = false;
}


KFonts::~KFonts()
{
}


void KFonts::readSettings( int )
{		
    useRM = kapp->config()->readBoolEntry( "useResourceManager", true );
}

void KFonts::setDefaults()
{
	int ci = lbFonts->currentItem();
	for ( int i = 0; i < (int) fontUseList.count(); i++ )
		fontUseList.at( i )->setDefault();
	fontUseList.at( ci );
	slotPreviewFont( ci );
	changed = true;
}

void KFonts::defaultSettings()
{
	setDefaults();
}

void KFonts::writeSettings()
{
	if ( !changed )
		return;
		
	for ( int i = 0; i < (int) fontUseList.count(); i++ )
		fontUseList.at( i )->writeFont();	
	
	fontUseList.at( lbFonts->currentItem() );

}

void KFonts::apply()
{
	if ( !changed )
		return;
	
	KIPC::sendMessageAll("KDEChangeGeneral");
	
	if ( useRM )
	    runResourceManager = TRUE;
}

void KFonts::slotSetFont(const QFont &fnt)
{
	fontUseList.current()->setFont( fnt );
	changed = true;
}

void KFonts::slotPreviewFont( int index )
{
	fntChooser->setFont( fontUseList.at( index )->font(),
			fontUseList.at( index )->spacing() );
}

void KFonts::applySettings()
{
    if (changed)
    {
        writeSettings();
        apply();
        changed = false;
    }
}

