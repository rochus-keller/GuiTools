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

#ifndef _Gui2_AutoShortcut
#define _Gui2_AutoShortcut

#include <QShortcut>
#include <GuiTools/UiFunction.h>

namespace Gui
{
	class UiFunction;

	class AutoShortcut : public QShortcut
	{
		Q_OBJECT
	public:
        // Receiver wird automatisch gesucht
		AutoShortcut( QWidget * parent, const char * member, const QKeySequence & key = QKeySequence() );
		AutoShortcut( const QKeySequence & key, QWidget * parent, const char * member );

        // Receiver wird explizit angegeben
		AutoShortcut( const QKeySequence & key, QWidget * parent, 
			QObject* receiver, const char * member ); // UiFunction -> receiver

        ~AutoShortcut();
	protected slots:
		void onActivated();
    protected:
        bool event(QEvent *e);
	private:
		UiFunction* d_f;
	};
}

#endif // _Gui2_AutoShortcut
