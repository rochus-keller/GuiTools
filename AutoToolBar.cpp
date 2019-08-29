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

#include "AutoToolBar.h"
#include <QApplication>
#include "NamedFunction.h"
using namespace Gui;

AutoToolBar::AutoToolBar(QWidget *parent)
	: QToolBar(parent)
{
   d_updater.start(QApplication::cursorFlashTime() / 2.0, this ); // RISK
}

AutoToolBar::~AutoToolBar()
{
	d_updater.stop();
}

void AutoToolBar::timerEvent(QTimerEvent *e)
{ 
    if( e->timerId() == d_updater.timerId() ) 
	{
		// Gehe durch alle Actions und löse einen Update-Cycle aus
		QList<QAction*> l = actions();
		foreach( QAction* a, l )
		{
			if( UiFunction* f = dynamic_cast<UiFunction*>( a ) )
				f->prepare();
		}
   }
}

QAction* AutoToolBar::addCommand( const QString& text, QObject* receiver, const char* member, const QKeySequence & s )
{
    QAction* a = new UiFunction( text, this, receiver, member );
	if( !s.isEmpty() )
		a->setShortcut( s );
	addAction( a );
    return a;
}

QAction *AutoToolBar::addCommand(const QIcon & i, const QString &text, QObject *receiver,
                                 const char *member, const QKeySequence & ks)
{
    QAction* a = addCommand( text, receiver, member, ks );
    a->setIcon( i );
    return a;
}

QAction *AutoToolBar::addAutoCommand(const QString &text, const char *member, const QKeySequence & s)
{
    Q_ASSERT( parentWidget() != 0 );
	QAction* a = new NamedFunction( text, member, parentWidget() );
	if( !s.isEmpty() )
		a->setShortcut( s );
	addAction( a );
    return a;
}

QAction *AutoToolBar::addAutoCommand(const QIcon & i, const QString &text, const char *member,
                                     const QKeySequence & ks)
{
    QAction* a = addAutoCommand( text, member, ks );
    a->setIcon( i );
    return a;
}

