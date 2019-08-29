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

#ifndef UIFUNCTION_H
#define UIFUNCTION_H

#include <QAction>
#include <QUuid>

namespace Gui
{
	class UiFunction : public QAction
	{
		Q_OBJECT
	public:
		UiFunction(QObject *parent);
		UiFunction( const QString& text, QObject *parent);
        UiFunction( const QString& text, QObject *parent, QObject *receiver, const char* member);
		~UiFunction();

		bool isPreparing() const { return d_preparing; }
        virtual bool hasTarget() const { return true; }
		static UiFunction* me() { return s_sender; } // RISK: nicht thread-safe
	signals:
		void handle();
	public slots:
		virtual void prepare(); // Aktualisiert enabled und checked
		virtual void execute(); // Führt die Funktion aus
	protected:
		static UiFunction* s_sender;
		static bool d_preparing;
	};
	// Die folgenden Makros funktionieren sowohl bei Aufruf des Slots via UiFunction als auch direkt
	#define ENABLED_IF( cond ) \
    { \
        Gui::UiFunction* f = Gui::UiFunction::me(); \
        if( f && f->isPreparing() ) \
        { \
            f->setEnabled( cond ); \
            return; \
        } \
        if( !( cond ) ) return; \
        /* else pass through */ \
    }
    // 2014-05-04: folgende Funktion ruft neu gleich selber setCheckable auf, wodurch bei Menü unnötig
	// NOTE: if(enabled) damit checked nur dann geprueft wird; sonst muss ueberall der Code auch noch auf Nullpointer gesichert werden
	#define CHECKED_IF( enabled, checked ) \
    { \
        Gui::UiFunction* f = Gui::UiFunction::me(); \
        if( f && f->isPreparing() ) \
        { \
			f->setEnabled( enabled ); f->setCheckable(true); \
			if(enabled) { f->setChecked( checked ); } \
            return; \
        } \
        if( !( enabled ) ) return; \
        /* else pass through */ \
    }
}

#endif // UIFUNCTION_H
