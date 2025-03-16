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

#include "BottomStretcher.h"
#include <QStyleOption>
#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
using namespace Gui;

SplitBar::SplitBar( QWidget* p, bool hori ):
	QWidget(p),d_hovering(false),d_pos(-1),d_rb( 0 ), d_hori( hori )
{
	setCursor( (d_hori)?Qt::SplitHCursor:Qt::SplitVCursor );
	if( d_hori )
		setFixedWidth( getGripSize() );
	else
		setFixedHeight( getGripSize() );
}

SplitBar::~SplitBar()
{
	if( d_rb )
		delete d_rb;
}

int SplitBar::getGripSize() const
{
    QStyleOption opt(0);
    opt.state = QStyle::State_None;
	int hw = parentWidget()->style()->pixelMetric(QStyle::PM_SplitterWidth, 0, this);
    QSize s = parentWidget()->style()->sizeFromContents(QStyle::CT_Splitter, &opt, QSize(hw, hw))
		.expandedTo(QApplication::globalStrut());
	if( d_hori )
		return s.width();
	else
		return s.height();
}

void SplitBar::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOption opt(0);
    opt.rect = rect(); 
    opt.palette = palette();
    if( d_hovering )
        opt.state |= QStyle::State_MouseOver;
    if( isEnabled() )
        opt.state |= QStyle::State_Enabled;
	if( d_hori )
		opt.state |= QStyle::State_Horizontal;
    style()->drawControl(QStyle::CE_Splitter, &opt, &p);
}

void SplitBar::enterEvent ( QEvent * )
{
	d_hovering = true;
	update();
}

void SplitBar::leaveEvent( QEvent * )
{
	d_hovering = false;
	update();
}

void SplitBar::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
	{
		if( d_hori )
			d_pos = e->pos().x();
		else
			d_pos = e->pos().y();
	}
}

void SplitBar::mouseReleaseEvent(QMouseEvent *e)
{
    if( e->button() == Qt::LeftButton ) 
	{
		int pos = 0;
		if( d_hori )
			pos = mapFromGlobal(e->globalPos()).x() - d_pos;
		else
			pos = mapFromGlobal(e->globalPos()).y() - d_pos;
		if( d_rb )
		{
			d_rb->hide();
			delete d_rb;
		}
		d_rb = 0;
		handleMoved( pos );
    }
}

void SplitBar::mouseMoveEvent(QMouseEvent *e)
{
    if( !(e->buttons() & Qt::LeftButton))
        return;
    int pos = 0;
	if( d_hori )
		pos = mapFromGlobal(e->globalPos()).x() - d_pos;
	else
		pos = mapFromGlobal(e->globalPos()).y() - d_pos;
	setRubberBand(pos);
}

void SplitBar::setRubberBand( int pos )
{
	if( d_rb == 0 )
		d_rb = new QRubberBand( QRubberBand::Line );
    QRect r = contentsRect();
	const int rBord = (d_hori)?width():height(); 
	if( d_hori )
		r = QRect( mapToGlobal( QPoint(pos + rBord / 2 - rBord, r.y()) ), 
			QSize( rBord, r.height() ));
	else
		r = QRect( mapToGlobal( QPoint(r.x(), pos + rBord / 2 - rBord) ), 
			QSize(r.width(), rBord));
    d_rb->setGeometry(r);
    if( !d_rb->isVisible() )
        d_rb->show();
}

BottomStretcher::BottomStretcher( QWidget* p ):
	QWidget(p),d_body(0)
{
	d_handle = new SplitBar( this );
    setFixedHeight( 200 );
	connect( d_handle, SIGNAL( handleMoved(int) ), this, SLOT( onHandleMoved(int) ) );
}

void BottomStretcher::setBody( QWidget* b )
{
	if( b == d_body )
		return;
	if( d_body && d_body->parent() == this )
		delete d_body;
	d_body = b;
}

void BottomStretcher::resizeEvent ( QResizeEvent * e )
{
    Q_UNUSED(e);
	QSize s = size();
	const int hw = d_handle->getGripSize();
	s.setHeight( s.height() - hw );
	if( d_body )
		d_body->setGeometry( 0, 0, s.width(), s.height() );
	d_handle->setGeometry( 0, s.height(), s.width(), hw );
}

void BottomStretcher::onHandleMoved(int pos)
{
	pos = qMax( 100, pos + height() - d_handle->height() );
	setFixedHeight( pos );
	heightChanged( pos );
    updateGeometry();
}

TopStretcher::TopStretcher(QWidget *p):
    QWidget(p),d_body(0)
{
	d_handle = new SplitBar( this );
    d_handle->setBackgroundRole( QPalette::Window );
    d_handle->setAutoFillBackground( true );
    setFixedHeight( 200 );
    connect( d_handle, SIGNAL( handleMoved(int) ), this, SLOT( onHandleMoved(int) ) );
}

void TopStretcher::setBody(QWidget *b)
{
    if( b == d_body )
		return;
	if( d_body && d_body->parent() == this )
		delete d_body;
    d_body = b;
    updateGeometry();
}

void TopStretcher::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    QSize bodySize = size();
	const int hw = d_handle->getGripSize();
	bodySize.setHeight( bodySize.height() - hw );
	if( d_body )
		d_body->setGeometry( 0, hw, bodySize.width(), bodySize.height() );
	d_handle->setGeometry( 0, 0, bodySize.width(), hw );
}

void TopStretcher::onHandleMoved(int pos)
{
    int newHeight = qMax( d_handle->height(), height() - pos + d_handle->height() );
	setFixedHeight( newHeight );
	heightChanged( newHeight );
    updateGeometry();
}
