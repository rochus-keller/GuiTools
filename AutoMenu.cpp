/*
 * Copyright 2000-2015 Rochus Keller <mailto:rkeller@nmr.ch>
 *
 * This file is part of the CARA (Computer Aided Resonance Assignment,
 * see <http://cara.nmr.ch/>) NMR Application Framework (NAF) library.
 *
 * The following is the license that applies to this copy of the
 * library. For a license to use the library under conditions
 * other than those described here, please email to rkeller@nmr.ch.
 *
 * GNU General Public License Usage
 * This file may be used under the terms of the GNU General Public
 * License (GPL) versions 2.0 or 3.0 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in
 * the packaging of this file. Please review the following information
 * to ensure GNU General Public Licensing requirements will be met:
 * http://www.fsf.org/licensing/licenses/info/GPLv2.html and
 * http://www.gnu.org/copyleft/gpl.html.
 */

#include "AutoMenu.h"
#include "NamedFunction.h"
#include <QMainWindow>
#include <QMenuBar>
#include <QtDebug>
#include <QShortcut>
#include <QEvent>
using namespace Gui;

// Wir verwenden nicht direkt UiFunction selber als Shortcut, da diese mit setEnabled(false) gleich
// auch den Shortcut deaktiviert und wir somit bei Betätigung des Shortcut nicht mehr prüfen können,
// ob der Befehl verfügbar ist, sobald das Item einmal als deaktiviert erkannt wurde.
class AutoMenuShortcut : public QShortcut
{
public:
    // Eigentlich ist QShortcut nicht darauf angewiesen, dass parent ein QWidget ist.
    // parentWidget wird im Code nur einmal verwendet und nur um auf Null zu testen.
    AutoMenuShortcut( QWidget* w, UiFunction* item, const QKeySequence& key ):
        QShortcut( w ),d_item(item)
    {
        connect( item, SIGNAL(destroyed()), this, SLOT(deleteLater() ) );
        setContext( Qt::WidgetWithChildrenShortcut  );
        setKey( key );
    }
    bool event(QEvent *e)
    {
        // QShortcut wird vom System über diesen Event benachtrichtigt und ruft dann
        // triggered auf.
        if( e->type() == QEvent::Shortcut )
        {
            d_item->prepare();
            if( d_item->isEnabled() )
                d_item->execute();
            return true;
        }else
            return QShortcut::event( e );
    }
    UiFunction* d_item;
};

AutoMenu::AutoMenu(QWidget *parent, bool context)
    : QMenu(parent), d_noPopup(false)
{
    connect( this, SIGNAL( aboutToShow() ), this, SLOT( onShow() ) );
    if( AutoMenu* p = dynamic_cast<AutoMenu*>( parent ) )
        d_noPopup = p->d_noPopup;
    if( context )
    {
		registerPopup();
        d_noPopup = false;
    }
}

AutoMenu::AutoMenu(const QString& title, QWidget *parent, bool addMenuBar)
    : QMenu(title, parent), d_noPopup(false)
{
    connect( this, SIGNAL( aboutToShow() ), this, SLOT( onShow() ) );
    if( AutoMenu* p = dynamic_cast<AutoMenu*>( parent ) )
        d_noPopup = p->d_noPopup;
	if( addMenuBar )
	{
		if( QMainWindow* w = dynamic_cast<QMainWindow*>( parent ) )
        {
			w->menuBar()->addMenu( this );
            d_noPopup = true;
        }
    }
}

void AutoMenu::onShow()
{
	// Gehe durch alle Actions und löse einen Update-Cycle aus
	QList<QAction*> l = actions();
	foreach( QAction* a, l )
	{
		if( UiFunction* f = dynamic_cast<UiFunction*>( a ) )
			f->prepare();
	}
}

void AutoMenu::onContextRequest( const QPoint & pos )
{
    QWidget* w = dynamic_cast<QWidget*>( sender() );
    Q_ASSERT( w != 0 );
    // The position pos is the position of the request in widget coordinates.
    QPoint p = w->mapToGlobal( pos );
	exec( p );
}

void AutoMenu::registerPopup()
{
	Q_ASSERT( parentWidget() != 0 );

	parentWidget()->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( parentWidget(), SIGNAL( customContextMenuRequested ( const QPoint & ) ),
		this, SLOT( onContextRequest( const QPoint &) ) );
}

void AutoMenu::connectPopup(QWidget * w)
{
	Q_ASSERT( w != 0 );
	w->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( w, SIGNAL( customContextMenuRequested ( const QPoint & ) ),
		this, SLOT( onContextRequest( const QPoint &) ) );
}

static QWidget* _climbToNonMenu( QObject* o )
{
    if( dynamic_cast<QMenu*>( o ) || dynamic_cast<QMenuBar*>( o ) )
        return _climbToNonMenu( o->parent() );
    else if( QWidget* w = dynamic_cast<QWidget*>( o ) )
        return w;
    else if( o == 0 )
        return 0;
    else
        return _climbToNonMenu( o->parent() );
}

static void _setShortCut( UiFunction* f, const QKeySequence & s, bool createShortcut )
{
    f->setShortcut( s ); // Damit Shortcut in jedem Fall im Menü angezeigt wird.
    if( !s.isEmpty() && createShortcut )
    {
        f->setShortcutContext( Qt::WidgetShortcut  );   // damit keine Mehrdeutigkeit mit AutoMenuShortcut

        QWidget* parent = _climbToNonMenu( f->parent() );
        if( parent )
            new AutoMenuShortcut( parent, f, s );
        else
            qWarning() << "AutoMenu::addCommand cannot create shortcut" << f->text() << s.toString();
    }
}

QAction* AutoMenu::addAutoCommand( const QString& text, const char* member,
                                   const QKeySequence & s, bool addAutoShortcut )
{
	Q_ASSERT( parent() != 0 );
    UiFunction* a = new NamedFunction( text, member, this );
	addAction( a );
    _setShortCut( a, s, addAutoShortcut || d_noPopup );
    // nur bei Popups soll es möglich sein, dass kein AutoMenuShortcut erzeugt wird.
	return a;
}

QAction* AutoMenu::addCommand( const QString& text, QObject* receiver, const char* member,
                               const QKeySequence & s, bool addAutoShortcut )
{
    UiFunction* a = new UiFunction( text, this, receiver, member );
	addAction( a );
    _setShortCut( a, s, addAutoShortcut || d_noPopup );
    return a;
}



