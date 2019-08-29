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

#include "UiFunction.h"
#include <QEvent>
using namespace Gui;
 
UiFunction* UiFunction::s_sender = 0;
bool UiFunction::d_preparing = false;

UiFunction::UiFunction(QObject *parent)
	: QAction(parent)
{
    connect( this, SIGNAL( triggered( bool ) ), this, SLOT( execute() ) );
    setEnabled(true);
	setMenuRole(QAction::NoRole); // damit Qt on OS X nicht wild Menüs umplatziert und umbenennt
}

UiFunction::UiFunction(const QString& text, QObject *parent)
	: QAction(text,parent)
{
    connect( this, SIGNAL( triggered( bool ) ), this, SLOT( execute() ) );
    setEnabled(true);
	setMenuRole(QAction::NoRole); // damit Qt on OS X nicht wild Menüs umplatziert und umbenennt
}

UiFunction::UiFunction(const QString& text, QObject *parent, QObject *receiver, const char* member)
	: QAction(text,parent)
{
    connect( this, SIGNAL( triggered( bool ) ), this, SLOT( execute() ) );
    connect( this, SIGNAL( handle() ), receiver, member );
    setEnabled(true);
	setMenuRole(QAction::NoRole); // damit Qt on OS X nicht wild Menüs umplatziert und umbenennt
}

UiFunction::~UiFunction()
{

}

void UiFunction::prepare()
{
	d_preparing = true;
	s_sender = this;
    setEnabled( false );
	emit handle();
	s_sender = 0;
}

void UiFunction::execute()
{
    // ENABLED_IF lässt in jedem Fall nur durch, wenn Condition erfüllt, auch wenn vorher kein prepare() aufgerufen
	d_preparing = false;
	s_sender = this;
	emit handle();
	s_sender = 0;
}

// QEvent::Shortcut muss hier nicht überschrieben werden, da die Default-Version lediglich triggered(bool) aufruft.
//bool UiFunction::event(QEvent *e)
//{
//	if( e->type() == QEvent::Shortcut )
//	{
//		prepare();
//		if( isEnabled() )
//			QAction::event( e );
//		return true;
//	}else
//		return QAction::event( e );
//}


