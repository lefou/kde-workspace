//
// KDE Display color scheme setup module
//
// Copyright (c)  Mark Donohoe 1997
//
// Converted to a kcc module by Matthias Hoelzer 1997
// Ported to Qt-2.0 by Matthias Ettrich 1999
//

#include <qgroupbox.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qfiledialog.h>
#include <qslider.h>
#include <qradiobutton.h>
#include <qdrawutil.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qcursor.h>
#include <qbitmap.h>
#include <qstringlist.h>

#include "colorscm.h"

#include <kapp.h>
#include <kconfig.h>
#include <kcharsets.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>
#include <kcursor.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <kipc.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include "kcolordlg.h"

#include "widgetcanvas.h"

#include "colorscm.moc"

#define SUPPLIED_SCHEMES 5

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


KColorScheme::KColorScheme(QWidget *parent, Mode m)
	: KDisplayModule(parent, m)
{	
	changed = false;
	nSysSchemes = 2;
	useRM = true;

	// if we are just initialising we don't need to create setup widget
	if (mode() == Init) {
	    return;
	}
	
	//("KColorScheme::KColorScheme");
	
	setName( i18n("Colors").ascii() );
	
	cs = new WidgetCanvas( this );
	cs->setCursor( KCursor::handCursor() );
	
	// LAYOUT
	
	QGridLayout *topLayout = new QGridLayout( this, 2, 2, 10 );
	
	topLayout->setRowStretch(0,0);
	topLayout->setRowStretch(1,1);
	
	topLayout->setColStretch(0,1);
	topLayout->setColStretch(1,1);
	
	//cs->drawSampleWidgets();
	cs->setFixedHeight( 150 );
	connect( cs, SIGNAL( widgetSelected( int ) ),
		 SLOT( slotWidgetColor( int ) ) );
	connect( cs, SIGNAL( colorDropped( int, const QColor&)),
		 SLOT( slotColorForWidget( int, const QColor&)));
	topLayout->addMultiCellWidget( cs, 0, 0, 0, 1 );

	QGroupBox *group = new QGroupBox( i18n("Color Scheme"), this );
	
	topLayout->addWidget( group, 1, 0 );
	
	QBoxLayout *groupLayout = new QVBoxLayout( group, 10 );
	groupLayout->addSpacing(10);

	sFileList = new QStrList();
	sList = new QListBox( group );
	
	sList->clear();
	sFileList->clear();
	sList->insertItem( i18n("Current scheme"), 0 );
	sFileList->append( "Not a  kcsrc file" );
	sList->insertItem( i18n("KDE default"), 1 );
	sFileList->append( "Not a kcsrc file" );
	readSchemeNames();
	sList->setMinimumHeight(sList->sizeHint().height()/2);
	sList->setCurrentItem( 0 );
	connect( sList, SIGNAL( highlighted( int ) ),
			SLOT( slotPreviewScheme( int ) ) );
					
	groupLayout->addWidget( sList, 10 );
	
	QBoxLayout *pushLayout = new QHBoxLayout;
	groupLayout->addLayout( pushLayout );
	
	addBt = new QPushButton(  i18n("&Add ..."), group );
	connect( addBt, SIGNAL( clicked() ), SLOT( slotAdd() ) );
	
	pushLayout->addWidget( addBt, 10 );
	
	removeBt = new QPushButton(  i18n("&Remove"), group );
	removeBt->setEnabled(FALSE);
	connect( removeBt, SIGNAL( clicked() ), SLOT( slotRemove() ) );
	
	pushLayout->addWidget( removeBt, 10 );
	
	saveBt = new QPushButton(  i18n("&Save changes"), group );
	saveBt->setEnabled(FALSE);
	connect( saveBt, SIGNAL( clicked() ), SLOT( slotSave() ) );
	
	groupLayout->addWidget( saveBt, 10 );
	groupLayout->activate();
	
	QBoxLayout *stackLayout = new QVBoxLayout;
	
	topLayout->addLayout( stackLayout, 1, 1 );

	group = new QGroupBox(  i18n("Widget color"), this );
	
	stackLayout->addWidget( group, 15 );

	groupLayout = new QVBoxLayout( group, 10 );
	groupLayout->addSpacing(10);

	wcCombo = new QComboBox( false, group );
	wcCombo->insertItem(  i18n("Inactive title bar") );
	wcCombo->insertItem(  i18n("Inactive title text") );
	wcCombo->insertItem(  i18n("Inactive title blend") );
	wcCombo->insertItem(  i18n("Active title bar") );
	wcCombo->insertItem(  i18n("Active title text") );
	wcCombo->insertItem(  i18n("Active title blend") );
	wcCombo->insertItem(  i18n("Background") );
	wcCombo->insertItem(  i18n("Text") );
	wcCombo->insertItem(  i18n("Select background") );
	wcCombo->insertItem(  i18n("Select text") );
	wcCombo->insertItem(  i18n("Window background") );
	wcCombo->insertItem(  i18n("Window text") );
	wcCombo->insertItem(  i18n("Button background") );
	wcCombo->insertItem(  i18n("Button text") );
	wcCombo->adjustSize();
	connect( wcCombo, SIGNAL( activated(int) ),
			SLOT( slotWidgetColor(int)  )  );
	

	groupLayout->addWidget( wcCombo );
	
	colorButton = new KColorButton( group );

	colorButton->setColor( cs->iaTitle );
	colorPushColor = cs->iaTitle;
	connect( colorButton, SIGNAL( changed(const QColor &) ),
		SLOT( slotSelectColor(const QColor &) ) );
		
	groupLayout->addWidget( colorButton );
	groupLayout->addStretch(10);
	groupLayout->activate();

	group = new QGroupBox(  i18n("Contrast"), this );
	
	stackLayout->addWidget( group, 10 );

	QVBoxLayout *groupLayout2 = new QVBoxLayout(group, 10);
	groupLayout2->addSpacing(10);
	
	groupLayout = new QHBoxLayout;
	groupLayout2->addLayout(groupLayout);

	sb = new QSlider( QSlider::Horizontal,group,"Slider" );
	sb->setRange( 0, 10 );
	sb->setValue(cs->contrast);
	sb->setFocusPolicy( QWidget::StrongFocus );
	connect( sb, SIGNAL( valueChanged( int ) ),
				SLOT( sliderValueChanged( int ) ) );
	
	QLabel *label = new QLabel( sb, i18n("&Low"), group );
	
	groupLayout->addWidget( label );
	groupLayout->addWidget( sb, 10 );
	
	label = new QLabel( group );
	label->setText(  i18n("High"));
	
	groupLayout->addWidget( label );
	groupLayout2->activate();
	
	slotPreviewScheme( 0 );
	
	topLayout->activate();
	
	loadSettings();

	changed = false;
}

void KColorScheme::loadSettings()
{
    KConfigGroupSaver saver(kapp->config(), "X11");
    useRM = kapp->config()->readBoolEntry( "useResourceManager", true );
}

void KColorScheme::resizeEvent( QResizeEvent * )
{
	cs->drawSampleWidgets();
}

void KColorScheme::sliderValueChanged( int val )
{
	cs->contrast = val;
	cs->drawSampleWidgets();
	changed = true;
}

void KColorScheme::slotSave( )
{
	KSimpleConfig *config =
	  new KSimpleConfig( sFileList->at( sList->currentItem() ) );
				
	config->setGroup( "Color Scheme" );
	config->writeEntry( "background", cs->back );
	config->writeEntry( "selectBackground", cs->select );
	config->writeEntry( "foreground", cs->txt );
	config->writeEntry( "activeForeground", cs->aTxt );
	config->writeEntry( "inactiveBackground", cs->iaTitle );
	config->writeEntry( "inactiveBlend", cs->iaBlend );
	config->writeEntry( "activeBackground", cs->aTitle );
	config->writeEntry( "activeBlend", cs->aBlend );
	config->writeEntry( "inactiveForeground", cs->iaTxt );
	config->writeEntry( "windowForeground", cs->windowTxt );
	config->writeEntry( "windowBackground", cs->window );
	config->writeEntry( "selectForeground", cs->selectTxt );
	config->writeEntry( "contrast", cs->contrast );
	config->writeEntry( "buttonForeground", cs->buttonTxt );
	config->writeEntry( "buttonBackground", cs->button );
	
	config->sync();
	
	saveBt->setEnabled( FALSE );
}

void KColorScheme::slotRemove()
{
	QString kcsPath = locateLocal("data", "kdisplay/color-schemes");
	
	QDir d( kcsPath );
	if (!d.exists()) // what can we do?
	  return;

	d.setFilter( QDir::Files );
	d.setSorting( QDir::Name );
	d.setNameFilter("*.kcsrc");
	
	uint ind = sList->currentItem();

	if ( !d.remove( sFileList->at( ind ) ) ) {
		KMessageBox::error( 0, 
		      i18n("This color scheme could not be removed.\n"
			   "Perhaps you do not have permission to alter the file\n"
               "system where the color scheme is stored." ));
		return;
	}
	
	sList->removeItem( ind );
	sFileList->remove( ind );
	
}

void KColorScheme::slotAdd()
{
	SaveScm *ss = new SaveScm( 0,  "save scheme" );
	
	bool nameValid;
	QString sName;
	QString sFile;
	
	do {
	
	nameValid = TRUE;
	
	if ( ss->exec() ) {
		sName = ss->nameLine->text();
		if ( sName.stripWhiteSpace().isEmpty() )
			return;
			
		sName = sName.simplifyWhiteSpace();
		sFile = sName;
		
		int ind = 0;
		while ( ind < (int) sFile.length() ) {
			
			// parse the string for first white space
			
			ind = sFile.find(" ");
			if (ind == -1) {
				ind = sFile.length();
				break;
			}
		
			// remove from string
		
			sFile.remove( ind, 1);
			
			// Make the next letter upper case
			
			QString s = sFile.mid( ind, 1 );
			s = s.upper();
			sFile.replace( ind, 1, s.data() );
			
		}
		
		for ( int i = 0; i < (int) sList->count(); i++ ) {
			if ( sName == sList->text( i ) ) {
				nameValid = FALSE;
				KMessageBox::error( 0, 
					i18n( "Please choose a unique name for the new color\n"\
                          "scheme. The one you entered already appears\n"\
                          "in the color scheme list." ));
			}
		}
	} else return;
	
	} while ( nameValid == FALSE );
	
	disconnect( sList, SIGNAL( highlighted( int ) ), this,
			SLOT( slotPreviewScheme( int ) ) );
	
	sList->insertItem( sName );
	sList->setFocus();
	sList->setCurrentItem( sList->count()-1 );
	
	QString kcsPath = locateLocal("data", "kdisplay");

	QDir d( kcsPath );
	if ( !d.exists() )
		if ( !d.mkdir( kcsPath ) ) {
			warning("KColorScheme: Could not make directory to store user info.");
			return;
		}
		
	kcsPath += "/color-schemes/";
	
	d.setPath( kcsPath );
	if ( !d.exists() )
		if ( !d.mkdir( kcsPath ) ) {
			warning("KColorScheme: Could not make directory to store user info.");
			return;
		}
	
	sFile.prepend( kcsPath );
	sFile += ".kcsrc";
	sFileList->append( sFile.data() );
	
	KSimpleConfig *config =
			new KSimpleConfig( sFile );
			
	config->setGroup( "Color Scheme" );
	config->writeEntry( "name", sName );
	delete config;
	
	slotSave();
	
	connect( sList, SIGNAL( highlighted( int ) ), this,
			SLOT( slotPreviewScheme( int ) ) );
			
	slotPreviewScheme( sList->currentItem() );
}

void KColorScheme::slotSelectColor( const QColor &col )
{
	colorPushColor = col;
	
	int selection;
	selection = wcCombo->currentItem()+1;
	switch(selection) {
		case 1:		cs->iaTitle = colorPushColor;
					break;
		case 2:		cs->iaTxt = colorPushColor;
					break;
		case 3:		cs->iaBlend = colorPushColor;
					break;
		case 4:		cs->aTitle = colorPushColor;
					break;
		case 5:		cs->aTxt = colorPushColor;
					break;
		case 6:		cs->aBlend = colorPushColor;
					break;
		case 7:		cs->back = colorPushColor;
					break;
		case 8:		cs->txt = colorPushColor;
					break;
		case 9:		cs->select = colorPushColor;
					break;
		case 10:	cs->selectTxt = colorPushColor;
					break;
		case 11:	cs->window = colorPushColor;
					break;
		case 12:	cs->windowTxt = colorPushColor;
					break;
		case 13:	cs->button = colorPushColor;
					break;
		case 14:	cs->buttonTxt = colorPushColor;
					break;
	}
	
	cs->drawSampleWidgets();
	
	if ( removeBt->isEnabled() )
		saveBt->setEnabled( TRUE );
	else
		saveBt->setEnabled( FALSE );
		

	changed=true;
}

void KColorScheme::slotWidgetColor( int indx )
{
	int selection;
	QColor col;
	
	if ( wcCombo->currentItem() != indx )
		wcCombo->setCurrentItem( indx );

	selection = indx + 1;
	switch(selection) {
		case 1:		col = cs->iaTitle;
					break;
		case 2:		col = cs->iaTxt;
					break;
		case 3: 	col = cs->iaBlend;
					break;
		case 4:		col = cs->aTitle;
					break;
		case 5:		col = cs->aTxt;
					break;	
		case 6: 	col = cs->aBlend;
					break;
		case 7:		col = cs->back;
					break;
		case 8:		col = cs->txt;
					break;
		case 9:		col = cs->select;
					break;
		case 10:	col = cs->selectTxt;
					break;
		case 11:	col = cs->window;
					break;
		case 12:	col = cs->windowTxt;
					break;
		case 13:	col = cs->button;
					break;
		case 14:	col = cs->buttonTxt;
					break;
	}

	colorButton->setColor( col );
	colorPushColor = col;	
    changed = true;
}

void KColorScheme::slotColorForWidget( int indx, const QColor& col)
{
  slotWidgetColor( indx);
  slotSelectColor( col);
}

void KColorScheme::writeNamedColor( KConfigBase *config,
				    const char *key, const char *name)
{
  QColor tmp;
  tmp.setNamedColor( name) ;
  config->writeEntry( key, tmp );
}

KColorScheme::~KColorScheme()
{
}

void KColorScheme::readScheme( int index )
{
  KConfigBase* config;

  if( index == 1 ) {
    cs->back = lightGray;
    cs->txt = black;
    cs->select = darkBlue;
    cs->selectTxt = white;
    cs->window = white;
    cs->windowTxt = black;
    cs->iaTitle = darkGray;
    cs->iaTxt = lightGray;
    cs->iaBlend = lightGray;
    cs->aTitle = darkBlue;
    cs->aTxt = white;
    cs->aBlend = black;
    cs->button = cs->back;
    cs->buttonTxt = cs->txt;
    cs->contrast = 7;

    return;
  } if ( index == 0 ) {
    config  = kapp->config();
  } else {
    config =
      new KSimpleConfig( sFileList->at( index ), true );
  }

  if ( index == 0 )
    config->setGroup( "General" );
  else
    config->setGroup( "Color Scheme" );

  cs->txt =
    config->readColorEntry( "foreground", &black );

  cs->back =
    config->readColorEntry( "background", &lightGray );

  cs->select =
    config->readColorEntry( "selectBackground", &darkBlue);

  cs->selectTxt =
    config->readColorEntry( "selectForeground", &white );

  cs->window =
    config->readColorEntry( "windowBackground", &white );

  cs->windowTxt =
    config->readColorEntry( "windowForeground", &black );

  cs->button =
    config->readColorEntry( "buttonBackground", &lightGray );

  cs->buttonTxt =
    config->readColorEntry( "buttonForeground", &black );

  if ( index == 0 ) config->setGroup( "WM" );

  cs->iaTitle =
    config->readColorEntry( "inactiveBackground", &darkGray);

  cs->iaTxt =
    config->readColorEntry( "inactiveForeground", &lightGray );

  cs->iaBlend =
    config->readColorEntry( "inactiveBlend", &lightGray );

  cs->aTitle =
    config->readColorEntry( "activeBackground", &darkBlue );

  cs->aTxt =
    config->readColorEntry( "activeForeground", &white );

  cs->aBlend =
    config->readColorEntry( "activeBlend", &black );

  if ( index == 0 ) config->setGroup( "KDE" );

  cs->contrast =
    config->readNumEntry( "contrast", 7 );
}

void KColorScheme::readSchemeNames( )
{
  QStringList list = KGlobal::dirs()->findAllResources("data", "kdisplay/color-schemes/*.kcsrc", false, true);
  QStringList localList;
  QString local_prefix = locateLocal("data", "test");
  local_prefix = local_prefix.left(local_prefix.findRev('/'));
  for(QStringList::Iterator it = list.begin(); it != list.end(); it++) {
    QString temp_s = (*it).left( (*it).findRev('/') );
    if (temp_s.find(local_prefix) != -1) {
      if (localList.find(temp_s) == localList.end()) {
        localList.append(*it);
        it = list.remove(it);  // returns next item
        it--;  // reset back one to compensate
      }
    }
  }

  nSysSchemes = 2;
  QDir d;
  for(QStringList::Iterator it = list.begin(); it != list.end(); it++) {
    // Always a current and a default scheme
    QString str;
    KSimpleConfig *config =
      new KSimpleConfig( *it, true );
    config->setGroup( "Color Scheme" );
    str = config->readEntry( "name" );

    sList->insertItem( str );
    sFileList->append( (*it).ascii() );

    nSysSchemes++;
    delete config;
  }

  // Now repeat for local files
  for(QStringList::Iterator it = localList.begin(); it != localList.end(); it++) {
    QString str;
    KSimpleConfig *config =
      new KSimpleConfig( (*it), true );
    config->setGroup( "Color Scheme" );
    str = config->readEntry( "name" );

    sList->insertItem( str );
    sFileList->append( (*it).ascii() );

    delete config;
  }
}

void KColorScheme::setDefaults()
{
	slotPreviewScheme( 1 );
}

void KColorScheme::defaultSettings()
{
	setDefaults();
}

void KColorScheme::writeSettings()
{
  if ( !changed )
    return;

  KConfig* sys = kapp->config();

  sys->setGroup( "General" );
  sys->writeEntry("background", cs->back, true, true);
  sys->writeEntry("selectBackground", cs->select, true, true);
  sys->writeEntry("foreground", cs->txt, true, true);
  sys->writeEntry("windowForeground", cs->windowTxt, true, true);
  sys->writeEntry("windowBackground", cs->window, true, true);
  sys->writeEntry("selectForeground", cs->selectTxt, true, true);
  sys->writeEntry("buttonBackground", cs->button, true, true);
  sys->writeEntry("buttonForeground", cs->buttonTxt, true, true);

  sys->setGroup( "WM" );
  sys->writeEntry("activeForeground", cs->aTxt, true, true);
  sys->writeEntry("inactiveBackground", cs->iaTitle, true, true);
  sys->writeEntry("inactiveBlend", cs->iaBlend, true, true);
  sys->writeEntry("activeBackground", cs->aTitle, true, true);
  sys->writeEntry("activeBlend", cs->aBlend, true, true);
  sys->writeEntry("inactiveForeground", cs->iaTxt, true, true);

  sys->setGroup( "KDE" );
  sys->writeEntry("contrast", cs->contrast, true, true);
  sys->sync();
  
  if ( useRM )
      runResourceManager = TRUE;
	
}

void KColorScheme::apply()
{
  KIPC::sendMessageAll("KDEChangePalette");
}

void KColorScheme::slotPreviewScheme( int indx )
{
  readScheme( indx );

  // Set various appropriate for the scheme

  cs->drawSampleWidgets();
  sb->setValue( cs->contrast );
  slotWidgetColor( 0 );
  if ( indx < nSysSchemes ) {
    removeBt->setEnabled( FALSE );
    saveBt->setEnabled( FALSE );
  } else
    removeBt->setEnabled( TRUE );
  changed = true;
}

void KColorScheme::applySettings()
{
    if (changed)
    {
        debug("KColorScheme::applySettings");
        writeSettings();
        apply();
        changed = false;
    }
}



/* this function should dissappear: colorscm should work directly on a Qt palette, since
   this will give us much more cusomization with qt-2.0.
   */
QPalette KColorScheme::createPalette()
{

  KConfigBase* config;
  config  = kapp->config();
  config->setGroup( "General" );
  QColor button =
    config->readColorEntry( "buttonBackground", &lightGray );

  QColor buttonText =
    config->readColorEntry( "buttonForeground", &black );

  QColor background =
    config->readColorEntry( "background", &lightGray );

  QColor highlight =
    config->readColorEntry( "selectBackground", &darkBlue);

  QColor highlightedText =
    config->readColorEntry( "selectForeground", &white );

  QColor base =
    config->readColorEntry( "windowBackground", &white );

  QColor foreground =
    config->readColorEntry( "windowForeground", &black );


  int contrast =
    config->readNumEntry( "contrast", 7 );

  int highlightVal, lowlightVal;

  highlightVal=100+(2*contrast+4)*16/10;
  lowlightVal=100+(2*contrast+4)*10;


  QColorGroup disabledgrp( foreground, background,
			   background.light(150),
			   background.dark(),
			   background.dark(120),
			   background.dark(120), base );

  QColorGroup colgrp( foreground, background,
		      background.light(150),
		      background.dark(),
		      background.dark(120),
		      foreground, base );

  colgrp.setColor( QColorGroup::Highlight, highlight);
  colgrp.setColor( QColorGroup::HighlightedText, highlightedText);
  colgrp.setColor( QColorGroup::Button, button);
  colgrp.setColor( QColorGroup::ButtonText, buttonText);
  return QPalette( colgrp, disabledgrp, colgrp);
}

