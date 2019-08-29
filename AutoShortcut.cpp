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

#include "AutoShortcut.h"
#include "NamedFunction.h"
#include <QEvent>
using namespace Gui;


AutoShortcut::AutoShortcut( const QKeySequence & key, QWidget * parent, QObject* receiver, const char * member ):
	 QShortcut(key, parent, 0, 0, Qt::WidgetWithChildrenShortcut )
{
    d_f = new UiFunction( QString(), this, receiver, member );
}

AutoShortcut::~AutoShortcut()
{
    delete d_f;
}

void AutoShortcut::onActivated()
{
    d_f->prepare();
    if( d_f->isEnabled() )
        d_f->execute();
}

bool AutoShortcut::event(QEvent *e)
{
    // QShortcut wird vom System über diesen Event benachtrichtigt und ruft dann
    // triggered auf.
    if( e->type() == QEvent::Shortcut )
    {
        onActivated();
        return true;
    }else
        return QShortcut::event( e );
}

AutoShortcut::AutoShortcut( QWidget * parent, const char * member, const QKeySequence & key )
	:QShortcut(key, parent,0,0, Qt::WidgetWithChildrenShortcut )
{
	d_f = new NamedFunction( QString(), member, this );
}

AutoShortcut::AutoShortcut(const QKeySequence &key, QWidget *parent, const char *member)
	:QShortcut(key, parent,0,0, Qt::WidgetWithChildrenShortcut )
{
	d_f = new NamedFunction( QString(), member, this );
}
