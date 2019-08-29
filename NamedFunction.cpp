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

#include "NamedFunction.h"
#include <QApplication>
#include <cassert>
#include <ctype.h>
#include <QtDebug>
using namespace Gui;

NamedFunction::NamedFunction(const QString& title, const char* slot, QObject *parent)
	: UiFunction(title, parent), d_slot( slot ),d_target(0)
{
	if( !d_slot.isEmpty() && ::isdigit( d_slot[0] ) )
		d_slot = d_slot.mid( 1 );
	if( !d_slot.isEmpty() && !d_slot.endsWith( "()" ) )
		d_slot += "()";
}

bool NamedFunction::prepareImp( QObject* cur )
{
    // Rufe auf um Verfügbarkeit abzufragen
    if( callFunction( cur ) )
	{
        // Funktion wurde gefunden und erfolgreich aufgerufen
        d_target = cur;
		return true;
    }else
        return false;
}

void NamedFunction::prepare()
{
	// Wird von aboutToShow() etc. aufgerufen. Sucht entlang der VisualHierarchy nach
	// UiFunction
	d_preparing = true;
    d_target = 0;
	setEnabled(false);
	QObject* prev = 0;
	QWidget* cur = QApplication::focusWidget();
//#define Gui2_NamedFunction_SearchAllWayUp
#ifdef Gui2_NamedFunction_SearchAllWayUp
	if( cur == 0 )
		cur = QApplication::activeWindow();
	while( cur )
#else
	if( cur != 0 )
#endif
	{
		// Gehe zuerst durch alle Children, die wir noch nicht besucht haben
		// Widgets können Controller als Children haben.
		for( int i = 0; i < cur->children().size(); i++ )
		{
			if( cur->children()[i] != prev && prepareImp( cur->children()[i] ) )
				return;
		}
		if( prepareImp( cur ) )
			return;
		prev = cur;
		cur = cur->parentWidget();
	}
}

void NamedFunction::execute()
{
    // ENABLED_IF lässt in jedem Fall nur durch, wenn Condition erfüllt.
    // Hier muss aber zuvor prepare() aufgerufen worden sein, ansonsten target nicht bekannt ist.
    if( d_target )
	{
		d_preparing = false;
        // Rufe auf um auszuführen
		callFunction( d_target );
    }else
        qWarning() << "NamedFunction::execute(): no target found for slot" << d_slot;
}

bool NamedFunction::callFunction( QObject* o )
{
    Q_ASSERT( o != 0 );
    const int slot = o->metaObject()->indexOfSlot( d_slot );
    if( slot == -1 )
        return false;
    s_sender = this;
#ifdef __UsingQtPrivate__
    const int sig = metaObject()->indexOfSignal( "handle()" );
    assert( sig != -1 );
    // Signatur in Qt4.4:
    //    QMetaCallEvent(int id, const QObject *sender, int signalId,
    //    int nargs = 0, int *types = 0, void **args = 0, QSemaphore *semaphore = 0);
	QMetaCallEvent e( slot, this, sig ); // In qobject private, aber mit Q_CORE_EXPORT deklariert
    QApplication::sendEvent( o, &e );
#else
    // NOTE: Da in der Praxis niemand eine NamedFunction mit Parametern verwendet,
    // sollte dieser Code funktionieren; ansonsten müsste man im Private-Bereich von Qt hantieren.

    // Schneide "(xyz)" ab; wenn es Klammern hat, funktioniert invokeMethod nicht.
    const int pos = d_slot.indexOf( QChar('(') );
    const bool res = QMetaObject::invokeMethod( o, d_slot.left( pos ), Qt::DirectConnection );
    if( !res )
        qWarning() << "NamedFunction::callFunction failed:" << d_slot;
#endif
    s_sender = 0;
    return true;
}
