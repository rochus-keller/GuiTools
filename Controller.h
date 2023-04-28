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
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License (LGPL) as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * You should have received a copy of the LGPL along with this library.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GUI2_CONTROLLER
#define _GUI2_CONTROLLER

#include <QWidget>

namespace Gui
{
	/// Diese Klasse dient dazu, das Event-Handling aus einem Widget auszulagern bzw.
	/// an einen unabhängigen Controller zu delegieren, so dass Widget nicht direkt 
	/// vererbt werden muss. Damit kann ein gewöhnliches Widget als View/Controller-Host
	/// verwendet und die ganze Arbeit an ein oder mehrere Contoller delegiert werden.
	class Controller : public QObject
	{
	public:
		Controller(QWidget* view );

		//* Override von QObject
		bool eventFilter( QObject * watched, QEvent * event );
		//-

		QWidget* view() const { return static_cast<QWidget*>( parent() ); }
	protected:
		void unobserve( QObject* o );
		void observe( QObject* );

		//* Calldowns
		virtual bool mouseMoveEvent(QMouseEvent*) { return false; }
        virtual bool mousePressEvent(QMouseEvent*) { return false; }
        virtual bool mouseReleaseEvent(QMouseEvent*) { return false; }
        virtual bool mouseDoubleClickEvent(QMouseEvent*) { return false; }
        virtual bool wheelEvent(QWheelEvent*) { return false; }
        virtual bool focusNextPrevChild(bool ) { return false; }
        virtual bool keyPressEvent(QKeyEvent*) { return false; }
        virtual bool keyReleaseEvent(QKeyEvent*) { return false; }
        virtual bool inputMethodEvent(QInputMethodEvent*) { return false; }
        virtual bool focusInEvent(QFocusEvent*) { return false; }
        virtual bool focusOutEvent(QFocusEvent*) { return false; }
        virtual bool enterEvent(QEvent*) { return false; }
        virtual bool leaveEvent(QEvent*) { return false; }
        virtual bool paintEvent(QPaintEvent*) { return false; }
        virtual bool moveEvent(QMoveEvent*) { return false; }
        virtual bool resizeEvent(QResizeEvent*) { return false; }
        virtual bool closeEvent(QCloseEvent *) { return false; }
        virtual bool dropEvent(QDropEvent*) { return false; }
        virtual bool dragEnterEvent(QDragEnterEvent*) { return false; }
        virtual bool dragMoveEvent(QDragMoveEvent*) { return false; }
        virtual bool dragLeaveEvent(QDragLeaveEvent*) { return false; }
		virtual bool showEvent(QShowEvent *) { return false; }
		virtual bool hideEvent(QHideEvent *) { return false; }
        virtual bool changeEvent(QEvent *) { return false; }
        virtual bool tickEvent(QTimerEvent *) { return false; }
	};
}

#endif // _GUI2_CONTROLLER
