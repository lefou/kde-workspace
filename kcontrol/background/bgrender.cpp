/* vi: ts=8 sts=4 sw=4
 *
 * This file is part of the KDE project, module kdesktop.
 * Copyright (C) 1999 Geert Jansen <g.t.jansen@stud.tue.nl>
 *
 * You can Freely distribute this program under the GNU Library General
 * Public License. See the file "COPYING.LIB" for the exact licensing terms.
 */

#include <config.h>

#include <time.h>
#include <stdlib.h>


#include <qtimer.h>
#include <qpainter.h>
#include <qimage.h>
#include <QDesktopWidget>
#include <QPaintEngine>
//Added by qt3to4:
#include <QPixmap>

#include <kapplication.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kimageeffect.h>
#include <kprocess.h>
#include <kpixmapio.h>
#include <ktempfile.h>
#include <kcursor.h>
#include <kmimetype.h>

#ifdef HAVE_LIBAGG
#include <ksvgiconengine.h>
#endif

#include "bgrender.h"

#include <X11/Xlib.h>

#include <config.h>
#include <QX11Info>
#include <QDesktopWidget>

/**** KBackgroundRenderer ****/


KBackgroundRenderer::KBackgroundRenderer(int desk, KConfig *config)
    : KBackgroundSettings(desk, config)
{
    m_State = 0;
    m_isBusyCursor = false;
    m_enableBusyCursor = false;
    m_pDirs = KGlobal::dirs();
    m_rSize = m_Size = QApplication::desktop()->size();
    m_pBackground = 0L; m_pImage = 0L; m_pPixmap = 0L;
    m_pProc = 0L;
    m_Tempfile = 0L;
    m_bPreview = false;

    m_pTimer = new QTimer(this);
    connect(m_pTimer, SIGNAL(timeout()), SLOT(render()));
}


KBackgroundRenderer::~KBackgroundRenderer()
{
    cleanup();
    delete m_Tempfile;
    m_Tempfile = 0;
}

/*
 * Re-configure because the desktop has been resized.
 */
void KBackgroundRenderer::desktopResized()
{
    m_State = 0;
    m_rSize = QApplication::desktop()->size();
    if( !m_bPreview )
        m_Size = m_rSize;
}


void KBackgroundRenderer::tile(QImage *dest, QRect rect, QImage *src)
{
    rect &= dest->rect();

    int x, y;
    int h = rect.height(), w = rect.width();
    int offx = rect.x(), offy = rect.y();
    int sw = src->width(), sh = src->height();

    for (y=offy; y<offy+h; y++)
	for (x=offx; x<offx+w; x++)
	    dest->setPixel(x, y, src->pixel(x%sw, y%sh));
}


/*
 * Build a command line to run the program.
 */

QString KBackgroundRenderer::buildCommand()
{
    QString num;
    int pos = 0;

    QString cmd;
    if (m_bPreview)
        cmd = previewCommand();
    else
        cmd = command();

    if (cmd.isEmpty())
	return QString();

    while ((pos = cmd.find('%', pos)) != -1) {

        if (pos == (int) (cmd.length() - 1))
            break;

        switch (cmd.at(pos+1).latin1()) {
        case 'f':
            createTempFile();
            cmd.replace(pos, 2, KShellProcess::quote(m_Tempfile->name()));
            pos += m_Tempfile->name().length() - 2;
            break;

        case 'x':
            num.setNum(m_Size.width());
            cmd.replace(pos, 2, num);
            pos += num.length() - 2;
            break;

        case 'y':
            num.setNum(m_Size.height());
            cmd.replace(pos, 2, num);
            pos += num.length() - 2;
            break;

        case '%':
            cmd.replace(pos, 2, "%");
            pos--;
            break;
        }

    }
    return cmd;
}


/*
 * Create a background tile. If the background mode is `Program',
 * this is asynchronous.
 */
int KBackgroundRenderer::doBackground(bool quit)
{
    if (m_State & BackgroundDone)
        return Done;
    int bgmode = backgroundMode();

    if (!enabled())
      bgmode= Flat;

    if (quit) {
	if (bgmode == Program && m_pProc)
	    m_pProc->kill();
        return Done;
    }

    int retval = Done;
    QString file;

    static unsigned int tileWidth = 0;
    static unsigned int tileHeight = 0;
    if( tileWidth == 0 )
        {
        int tile_val = QPixmap::defaultDepth() >= 24 ? 1 : 2;
    // some dithering may be needed even with bpb==15/16, so don't use tileWidth==1
    // for them
    // with tileWidth>2, repainting the desktop causes nasty effect (XFree86 4.1.0 )
        if( XQueryBestTile( QX11Info::display(), QX11Info::appRootWindow(), tile_val, tile_val,
            &tileWidth, &tileHeight ) != Success )
            tileWidth = tileHeight = tile_val; // some defaults
    }
    switch (bgmode) {

    case Flat:
        // this can be tiled correctly without problems
	m_pBackground->create( tileWidth, tileHeight, 32);
        m_pBackground->fill(colorA().rgb());
        break;

    case Pattern:
    {
        if (pattern().isEmpty())
            break;
        file = m_pDirs->findResource("dtop_pattern", pattern());
        if (file.isEmpty())
            break;

	m_pBackground->load(file);
	if (m_pBackground->isNull())
	    break;
	int w = m_pBackground->width();
	int h = m_pBackground->height();
	if ((w > m_Size.width()) || (h > m_Size.height())) {
	    w = QMIN(w, m_Size.width());
	    h = QMIN(h, m_Size.height());
	    *m_pBackground = m_pBackground->copy(0, 0, w, h);
	}
	KImageEffect::flatten(*m_pBackground, colorA(), colorB(), 0);
	break;
    }
    case Program:
        if (m_State & BackgroundStarted)
            break;
        m_State |= BackgroundStarted;
        createTempFile();

	file = buildCommand();
	if (file.isEmpty())
	    break;

        delete m_pProc;
        m_pProc = new KShellProcess;
        *m_pProc << file;
        connect(m_pProc, SIGNAL(processExited(KProcess *)),
                SLOT(slotBackgroundDone(KProcess *)));
        m_pProc->start(KShellProcess::NotifyOnExit);
        retval = Wait;
        break;

    case HorizontalGradient:
    {
	QSize size = m_Size;
        // on <16bpp displays the gradient sucks when tiled because of dithering
        if( optimize())
	    size.setHeight( tileHeight );
	*m_pBackground = KImageEffect::gradient(size, colorA(), colorB(),
		KImageEffect::HorizontalGradient, 0);
        break;
    }
    case VerticalGradient:
    {
	QSize size = m_Size;
        // on <16bpp displays the gradient sucks when tiled because of dithering
        if( optimize())
	    size.setWidth( tileWidth );
        *m_pBackground = KImageEffect::gradient(size, colorA(), colorB(),
		KImageEffect::VerticalGradient, 0);
        break;
    }
    case PyramidGradient:
        *m_pBackground = KImageEffect::gradient(m_Size, colorA(), colorB(),
		KImageEffect::PyramidGradient, 0);
        break;

    case PipeCrossGradient:
        *m_pBackground = KImageEffect::gradient(m_Size, colorA(), colorB(),
		KImageEffect::PipeCrossGradient, 0);
        break;

    case EllipticGradient:
        *m_pBackground = KImageEffect::gradient(m_Size, colorA(), colorB(),
		KImageEffect::EllipticGradient, 0);
        break;
    }

    if (retval == Done)
        m_State |= BackgroundDone;

    return retval;
}


int KBackgroundRenderer::doWallpaper(bool quit)
{
    if (m_State & WallpaperDone)
        return Done;

    if (quit)
        // currently no asynch. wallpapers
        return Done;

    int wpmode= enabled()?wallpaperMode():NoWallpaper;

    QImage wp;
    if (wpmode != NoWallpaper) {
wp_load:
	if (currentWallpaper().isEmpty()) {
	    wpmode = NoWallpaper;
	    goto wp_out;
	}
	QString file = m_pDirs->findResource("wallpaper", currentWallpaper());
	if (file.isEmpty()) {
	    wpmode = NoWallpaper;
	    goto wp_out;
	}

        // _Don't_ use KMimeType, as it relies on ksycoca which we really
        // don't want in krootimage (kdm context).
        //if ( KMimeType::findByPath( file )->is( "image/svg+xml" ) ) {
        if (file.endsWith(".svg") || file.endsWith(".svgz")) {
#ifdef HAVE_LIBAGG
	    // Special stuff for SVG icons
	    KSVGIconEngine* svgEngine = new KSVGIconEngine();

	    //FIXME
	    //ksvgiconloader doesn't seem to let us find out the
	    //ratio of width to height so for the most part we just
	    //assume it's a square
	    int svgWidth;
	    int svgHeight;
	    switch (wpmode)
	    {
	        case Centred:
	        case CentredAutoFit:
		    svgHeight = (int)(m_Size.height() * 0.8);
		    svgWidth = svgHeight;
	            break;
	        case Tiled:
	        case CenterTiled:
		    svgHeight = (int)(m_Size.height() * 0.5);
		    svgWidth = svgHeight;
	            break;
	        case Scaled:
		    svgHeight = m_Size.height();
		    svgWidth = m_Size.width();
	            break;
	        case CentredMaxpect:
		case ScaleAndCrop:
	        case TiledMaxpect:
		    svgHeight = m_Size.height();
		    svgWidth = svgHeight;
	            break;
	        case NoWallpaper:
	        default:
	            kdWarning() << k_funcinfo << "unknown diagram type" << endl;
		    svgHeight = m_Size.height();
		    svgWidth = svgHeight;
		    break;
	    }
	    //FIXME hack due to strangeness with
	    //background control modules
	    if ( svgHeight < 200 ) {
		svgHeight *= 6;
	        svgWidth *= 6;
	    }

	    if (svgEngine->load(svgWidth, svgHeight, file )) {
		wp = ( *svgEngine->image() );
	    } else {
		kdWarning() << "failed to load SVG file " << file << endl;
	    }

	    delete svgEngine;
#else //not libagg
	    kdWarning() << k_funcinfo
			<< "tried to load SVG file but libagg not installed" << endl;
#endif
	} else {
	    wp.load(file);
	}
	if (wp.isNull()) {
            if (discardCurrentWallpaper())
               goto wp_load;
	    wpmode = NoWallpaper;
	    goto wp_out;
	}
	wp = wp.convertDepth(32, Qt::DiffuseAlphaDither);

	// If we're previewing, scale the wallpaper down to make the preview
	// look more like the real desktop.
	if (m_bPreview) {
	    int xs = wp.width() * m_Size.width() / m_rSize.width();
	    int ys = wp.height() * m_Size.height() / m_rSize.height();
	    if ((xs < 1) || (ys < 1))
	    {
	       xs = ys = 1;
	    }
	    wp = wp.smoothScale(xs, ys);
	}
    }
wp_out:

    if (m_pBackground->isNull()) {
	m_pBackground->create(8, 8, 32);
	m_pBackground->fill(colorA().rgb());
    }

    int retval = Done;

    int w = m_Size.width();	// desktop width/height
    int h = m_Size.height();

    int ww = wp.width();	// wallpaper width/height
    int wh = wp.height();

    QRect d;	// to be filled destination rectangle; may exceed desktop!

    switch (wpmode)
    {
	case NoWallpaper:
	    break;
	case Centred:
	    d.setRect((w - ww) / 2, (h - wh) / 2, ww, wh);
	    break;
	case Tiled:
	    d.setRect(0, 0, w, h);
	    break;
	case CenterTiled:
	    d.setCoords(-ww + ((w - ww) / 2) % ww, -wh + ((h - wh) / 2) % wh, w-1, h-1);
	    break;
	case Scaled:
	    wp = wp.smoothScale(ww = w, wh = h);
	    d.setRect(0, 0, w, h);
	    break;
        case CentredAutoFit:
            if( ww <= w && wh <= h ) {
    	        d.setRect((w - ww) / 2, (h - wh) / 2, ww, wh); // like Centred
	        break;
            }
            // fall through
	case CentredMaxpect:
            {
              double sx = (double) w / ww;
              double sy = (double) h / wh;
              if (sx > sy) {
                  ww = (int)(sy * ww);
                  wh = h;
              } else {
                  wh = (int)(sx * wh);
                  ww = w;
              }
              wp = wp.smoothScale(ww, wh);
	      d.setRect((w - ww) / 2, (h - wh) / 2, ww, wh);
	      break;
            }
	case TiledMaxpect:
            {
              double sx = (double) w / ww;
              double sy = (double) h / wh;
              if (sx > sy) {
                  ww = (int)(sy * ww);
                  wh = h;
              } else {
                  wh = (int)(sx * wh);
                  ww = w;
              }
              wp = wp.smoothScale(ww, wh);
	      d.setRect(0, 0, w, h);
	      break;
            }
	 case ScaleAndCrop:
            {
              double sx = (double) w / ww;
              double sy = (double) h / wh;
              if (sx > sy) {
	      	  //Case 1: x needs bigger scaling. Lets increase x and leave part of y offscreen
                  ww = w;
		  wh=(int)(sx * wh);
              } else {
	          //Case 2: y needs bigger scaling. Lets increase y and leave part of x offscreen
                  wh = h;
                  ww = (int)(sy*ww);
              }
              wp = wp.smoothScale(ww, wh);
	      d.setRect((w - ww) / 2, (h - wh) / 2,w, h);
	      break;
            }
    }

    wallpaperBlend( d, wp, ww, wh );

    if (retval == Done)
        m_State |= WallpaperDone;

    return retval;
}

void KBackgroundRenderer::wallpaperBlend( const QRect& d, QImage& wp, int ww, int wh )
{
    if( !enabled() || (blendMode() == NoBlending && 
        ( QApplication::desktop()->paintEngine()->hasFeature(QPaintEngine::Antialiasing) || !wp.hasAlphaBuffer()))) {
        fastWallpaperBlend( d, wp, ww, wh );
    }
    else {
        fullWallpaperBlend( d, wp, ww, wh );
    }
}

// works only for NoBlending and no alpha in wallpaper
// but is much faster than QImage fidling
void KBackgroundRenderer::fastWallpaperBlend( const QRect& d, QImage& wp, int ww, int wh )
{
    *m_pImage = QImage();
    // copy background to m_pPixmap
    if( !enabled() || (wallpaperMode() == NoWallpaper && optimize())) {
        // if there's no wallpaper, no need to tile the pixmap to the size of desktop, as X does
        // that automatically and using a smaller pixmap should save some memory
        m_pPixmap->convertFromImage( *m_pBackground );
        return;
    }
    else if( wallpaperMode() == Tiled && !wp.hasAlphaBuffer() && optimize() && !m_bPreview ) {
    // tiles will be tiled by X automatically
        if( useShm()) {
            KPixmapIO io;
            *m_pPixmap = io.convertToPixmap( wp );
        }
        else
            m_pPixmap->convertFromImage( wp );
        return;
    }
    else if (m_pBackground->size() == m_Size)
        m_pPixmap->convertFromImage( *m_pBackground );
    else {
        *m_pPixmap = QPixmap( m_Size );
        QPainter p( m_pPixmap );
        QPixmap pm;
        pm.convertFromImage( *m_pBackground );
        p.drawTiledPixmap( 0, 0, m_Size.width(), m_Size.height(), pm );
    }

    // paint/alpha-blend wallpaper to destination rectangle of m_pPixmap
    if (d.isValid()) {
        QPixmap wp_pixmap;
        if( useShm() && !wp.hasAlphaBuffer()) {
            KPixmapIO io;
            wp_pixmap = io.convertToPixmap( wp );
        }
        else
            wp_pixmap.convertFromImage( wp );
        for (int y = d.top(); y < d.bottom(); y += wh) {
	    for (int x = d.left(); x < d.right(); x += ww) {
		bitBlt( m_pPixmap, x, y, &wp_pixmap, 0, 0, ww, wh );
	    }
	}
    }
}


void KBackgroundRenderer::fullWallpaperBlend( const QRect& d, QImage& wp, int ww, int wh )
{
    *m_pPixmap = QPixmap();
    int w = m_Size.width();	// desktop width/height
    int h = m_Size.height();
    // copy background to m_pImage
    if (m_pBackground->size() == m_Size) {
	*m_pImage = m_pBackground->copy();

	if (m_pImage->depth() < 32)
	    *m_pImage = m_pImage->convertDepth(32, Qt::DiffuseAlphaDither);

    } else {
	m_pImage->create(w, h, 32);
	tile(m_pImage, QRect(0, 0, w, h), m_pBackground);
    }

    // blend wallpaper to destination rectangle of m_pImage
    if (d.isValid())
    {
        int blendFactor = 100;
        if (blendMode() == FlatBlending)
            blendFactor = (blendBalance()+200)/4;
        for (int y = d.top(); y < d.bottom(); y += wh) {
	    for (int x = d.left(); x < d.right(); x += ww) {
		blend(m_pImage, QRect(x, y, ww, wh), &wp,
			QPoint(-QMIN(x, 0), -QMIN(y, 0)), blendFactor);
	    }
	}
    }


    // blend whole desktop
    if ( wallpaperMode() != NoWallpaper) {
      int bal = blendBalance();

      switch( blendMode() ) {
      case HorizontalBlending:
	KImageEffect::blend( *m_pImage, *m_pBackground,
			     KImageEffect::HorizontalGradient,
			     bal, 100 );
	break;

      case VerticalBlending:
	KImageEffect::blend( *m_pImage, *m_pBackground,
			     KImageEffect::VerticalGradient,
			     100, bal );
	break;

      case PyramidBlending:
	KImageEffect::blend( *m_pImage, *m_pBackground,
			     KImageEffect::PyramidGradient,
			     bal, bal );
	break;

      case PipeCrossBlending:
	KImageEffect::blend( *m_pImage, *m_pBackground,
			     KImageEffect::PipeCrossGradient,
			     bal, bal );
	break;

      case EllipticBlending:
	KImageEffect::blend( *m_pImage, *m_pBackground,
			     KImageEffect::EllipticGradient,
			     bal, bal );
	break;

      case IntensityBlending:
	KImageEffect::modulate( *m_pImage, *m_pBackground, reverseBlending(),
		    KImageEffect::Intensity, bal, KImageEffect::All );
	break;

      case SaturateBlending:
	KImageEffect::modulate( *m_pImage, *m_pBackground, reverseBlending(),
		    KImageEffect::Saturation, bal, KImageEffect::Gray );
	break;

      case ContrastBlending:
	KImageEffect::modulate( *m_pImage, *m_pBackground, reverseBlending(),
		    KImageEffect::Contrast, bal, KImageEffect::All );
	break;

      case HueShiftBlending:
	KImageEffect::modulate( *m_pImage, *m_pBackground, reverseBlending(),
		    KImageEffect::HueShift, bal, KImageEffect::Gray );
	break;

      case FlatBlending:
        // Already handled
	break;
      }
    }
}

/* Alpha blend an area from <src> with offset <soffs> to rectangle <dr> of <dst>
 * Default offset is QPoint(0, 0).
 * blendfactor = [0, 100%]
 */
void KBackgroundRenderer::blend(QImage *dst, QRect dr, QImage *src, QPoint soffs, int blendFactor)
{
    int x, y, a;
    dr &= dst->rect();

    for (y = 0; y < dr.height(); y++) {
	if (dst->scanLine(dr.y() + y) && src->scanLine(soffs.y() + y)) {
	    QRgb *b, *d;
	    for (x = 0; x < dr.width(); x++) {
		b = reinterpret_cast<QRgb*>(dst->scanLine(dr.y() + y)
			+ (dr.x() + x) * sizeof(QRgb));
                d = reinterpret_cast<QRgb*>(src->scanLine(soffs.y() + y)
			+ (soffs.x() + x) * sizeof(QRgb));
                a = (qAlpha(*d) * blendFactor) / 100;
                *b = qRgb(qRed(*b) - (((qRed(*b) - qRed(*d)) * a) >> 8),
                          qGreen(*b) - (((qGreen(*b) - qGreen(*d)) * a) >> 8),
                          qBlue(*b) - (((qBlue(*b) - qBlue(*d)) * a) >> 8));
            }
        }
    }
}



void KBackgroundRenderer::slotBackgroundDone(KProcess *process)
{
    Q_ASSERT(process == m_pProc);
    m_State |= BackgroundDone;

    if (m_pProc->normalExit() && !m_pProc->exitStatus()) {
        m_pBackground->load(m_Tempfile->name());
        m_State |= BackgroundDone;
    }

    m_Tempfile->unlink();
    delete m_Tempfile; m_Tempfile = 0;
    m_pTimer->start(0, true);
    setBusyCursor(false);
}



/*
 * Starts the rendering process.
 */
void KBackgroundRenderer::start(bool enableBusyCursor)
{
    m_enableBusyCursor = enableBusyCursor;
    setBusyCursor(true);
    if (m_pBackground == 0L)
	m_pBackground = new QImage();
    if (m_pImage == 0L)
	m_pImage = new QImage();
    if (m_pPixmap == 0L)
	m_pPixmap = new QPixmap();

    m_State = Rendering;
    m_pTimer->start(0, true);
}


/*
 * This slot is connected to a timer event. It is called repeatedly until
 * the rendering is done.
 */
void KBackgroundRenderer::render()
{
    setBusyCursor(true);
    if (!(m_State & Rendering))
        return;

    int ret;

    if (!(m_State & BackgroundDone)) {
        ret = doBackground();
        if (ret != Wait)
	    m_pTimer->start(0, true);
	return;
    }

    // No async wallpaper
    doWallpaper();

    done();
    setBusyCursor(false);
}


/*
 * Rendering is finished.
 */
void KBackgroundRenderer::done()
{
    setBusyCursor(false);
    m_State |= AllDone;
    emit imageDone(desk());
    if(backgroundMode() == Program && m_pProc &&
       m_pProc->normalExit() && m_pProc->exitStatus()) {
         emit programFailure(desk(), m_pProc->exitStatus());
     } else if(backgroundMode() == Program && m_pProc &&
       !m_pProc->normalExit()) {
         emit programFailure(desk(), -1);
     } else if(backgroundMode() == Program) {
         emit programSuccess(desk());
     }

}

/*
 * This function toggles a busy cursor on and off, for use in rendering.
 * It is useful because of the ASYNC nature of the rendering - it is hard
 * to make sure we don't set the busy cursor twice, but only restore
 * once.
 */
void KBackgroundRenderer::setBusyCursor(bool isBusy) {
   if(m_isBusyCursor == isBusy)
      return;
   if (isBusy && !m_enableBusyCursor)
      return;
   m_isBusyCursor = isBusy;
   if(isBusy)
      QApplication::setOverrideCursor( KCursor::workingCursor() );
   else
      QApplication::restoreOverrideCursor();
}

/*
 * Stop the rendering.
 */
void KBackgroundRenderer::stop()
{
    if (!(m_State & Rendering))
	return;

    doBackground(true);
    doWallpaper(true);
    m_State = 0;
}


/*
 * Cleanup after rendering.
 */
void KBackgroundRenderer::cleanup()
{
    setBusyCursor(false);
    delete m_pBackground; m_pBackground = 0L;
    delete m_pImage; m_pImage = 0L;
    delete m_pPixmap; m_pPixmap = 0L;
    delete m_pProc; m_pProc = 0L;
    m_State = 0;
}


void KBackgroundRenderer::setPreview(const QSize &size)
{
    if (size.isNull())
        m_bPreview = false;
    else {
        m_bPreview = true;
        m_Size = size;
    }
}


QPixmap *KBackgroundRenderer::pixmap()
{
    if (m_State & AllDone) {
        if( m_pPixmap->isNull())
            m_pPixmap->convertFromImage( *m_pImage );
        return m_pPixmap;
    }
    return 0L;
}

QImage *KBackgroundRenderer::image()
{
    if (m_State & AllDone) {
        if( m_pImage->isNull()) {
            if( m_pPixmap->size() != m_Size ) {
                QPainter p( m_pPixmap );
                QPixmap pm( m_Size ); // the pixmap may be smaller, so tile it
                pm.convertFromImage( *m_pBackground );
                p.drawTiledPixmap( 0, 0, m_Size.width(), m_Size.height(), pm );
                *m_pImage = pm.convertToImage();
            }
            else
                *m_pImage = m_pPixmap->convertToImage();
        }
        return m_pImage;
    }
    return 0L;
}


void KBackgroundRenderer::load(int desk, bool reparseConfig)
{
    if (m_State & Rendering)
        stop();

    cleanup();
    m_bPreview = false;
    m_Size = m_rSize;

    KBackgroundSettings::load(desk, reparseConfig);
}

void KBackgroundRenderer::createTempFile()
{
   if( !m_Tempfile )
     m_Tempfile = new KTempFile();
}


#include "bgrender.moc"
