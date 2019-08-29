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

#ifndef __Gui2_NamedFunction__
#define __Gui2_NamedFunction__

#include <GuiTools/UiFunction.h>

namespace Gui
{
    // Diese Klasse erlaubt es, einen Slot dynamisch zu suchen, bzw. den Receiver nicht explizit anzugeben.
	class NamedFunction : public UiFunction
	{
	public:
		NamedFunction( const QString& title, const char* slot, QObject* parent );
	protected:
		bool callFunction( QObject* );
		bool prepareImp( QObject* cur );
		// Overrides
		void prepare(); 
		void execute(); 
        bool hasTarget() const { return (d_target != 0); }
	private:
		QByteArray d_slot;
		QObject* d_target;
	};
}

#endif
