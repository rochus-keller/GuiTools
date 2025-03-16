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

#ifndef __BottomStretcher__
#define __BottomStretcher__

#include <QWidget>
#include <QRubberBand>

namespace Gui
{
	class SplitBar : public QWidget
	{
		Q_OBJECT
	public:
		SplitBar(QWidget*, bool hori = false);
		~SplitBar();
		int getGripSize() const;
	signals:
		void handleMoved( int );
	protected:
		void paintEvent(QPaintEvent *);
		void enterEvent ( QEvent * );
		void leaveEvent( QEvent * );
		void mouseMoveEvent(QMouseEvent *);
		void mousePressEvent(QMouseEvent *);
		void mouseReleaseEvent(QMouseEvent *);
		void setRubberBand( int );
		int d_pos;
		QRubberBand* d_rb;
		bool d_hovering;
		bool d_hori;
	};

	class BottomStretcher : public QWidget
	{
		Q_OBJECT
	public:
		BottomStretcher( QWidget* parent );
		void setBody( QWidget* b );
	signals:
		void heightChanged( int );
	protected:
		void resizeEvent ( QResizeEvent * event ) ;
	protected slots:
		void onHandleMoved(int);
	private:
		QWidget* d_body;
		SplitBar* d_handle;
	};

    class TopStretcher : public QWidget
	{
		Q_OBJECT
	public:
		TopStretcher( QWidget* parent );
		void setBody( QWidget* b );
	signals:
		void heightChanged( int );
	protected:
		void resizeEvent ( QResizeEvent * event ) ;
	protected slots:
		void onHandleMoved(int);
	private:
		QWidget* d_body;
		SplitBar* d_handle;
	};
}

#endif
