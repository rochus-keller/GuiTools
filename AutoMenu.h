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

#ifndef AUTOMENU_H
#define AUTOMENU_H

#include <QMenu>
#include <GuiTools/UiFunction.h>

namespace Gui
{
	class AutoMenu : public QMenu
	{
		Q_OBJECT
	public:
		AutoMenu(QWidget *parent, bool context = false);
        AutoMenu(const QString& title, QWidget *parent = 0, bool addMenuBar = true);

		void connectPopup( QWidget* );
		void registerPopup();

        // Command geht direkt; Receiver ist explizit angegeben
		QAction* addCommand( const QString& text, QObject* receiver, const char* member,
                             const QKeySequence & = 0, bool addAutoShortcut = false );
        // Receiver wird automatisch gesucht
		QAction* addAutoCommand( const QString& text, const char* member,
                                 const QKeySequence & = 0, bool addAutoShortcut = false );
	protected slots:
		void onShow();
		void onContextRequest( const QPoint &);
    private:
        bool d_noPopup;
	};
}

#endif // AUTOMENU_H
