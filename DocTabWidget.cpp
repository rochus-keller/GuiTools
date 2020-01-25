/*
* Copyright 2010-2018 Rochus Keller <mailto:me@rochus-keller.info>
*
* This file is part of the CrossLine application.
*
* The following is the license that applies to this copy of the
* application. For a license to use the application under conditions
* other than those described here, please email to me@rochus-keller.info.
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

#include "DocTabWidget.h"
#include <QToolButton>
#include <QTabBar>
#include <GuiTools/UiFunction.h>
#include <QMessageBox>
#include "DocSelector.h"

DocTabWidget::DocTabWidget( QWidget* p, bool hideSingleTab ):QTabWidget(p),
	d_backLock(false),d_observed(false), d_hideSingleTab( hideSingleTab )
{
	setUsesScrollButtons( true );
	setElideMode( Qt::ElideNone );
	connect( this, SIGNAL( currentChanged ( int  ) ), this, SLOT( onTabChanged( int ) ) );
	d_closer = new QToolButton( this );
	d_closer->setEnabled(true);
	d_closer->setIcon( QIcon( ":/MasterPlan/images/close.png" ) );
	d_closer->setAutoRaise( true );
	d_closer->setEnabled( false );
	connect( d_closer, SIGNAL( clicked() ), this, SLOT( onCloseDoc() ) );
	setCornerWidget( d_closer, Qt::TopLeftCorner );
	if( d_hideSingleTab )
		tabBar()->hide(); // wir beginnen zuerst ohne Tab	
}

void DocTabWidget::setCloserIcon( const QString& path )
{
	d_closer->setIcon( QIcon( path ) );
}

int DocTabWidget::addFixed( QWidget* w, const QString& title )
{
    d_views.append( QVariant() );
	addTab( w, title );
	const bool old = d_backLock;
	d_backLock = true;
	setCurrentIndex( d_views.size() - 1 );
	d_order.append( w );
	d_backLock = old;
	return d_views.size() - 1;
}

int DocTabWidget::showDoc(const QVariant& doc )
{
	const int pos = findDoc( doc );
	if( pos != -1 )
		setCurrentIndex( pos );
	return pos;
}

int DocTabWidget::findDoc(const QVariant& doc )
{
	for( int i = 0; i < d_views.size(); i++ )
	{
        if( d_views[i] == doc )
		{
			return i;
		}
	}
	return -1;
}

int DocTabWidget::showWidget( QWidget* w )
{
	for( int i = 0; i < count(); i++ )
	{
		if( widget(i) == w )
		{
			setCurrentIndex( i );
			return i;
		}
	}
	return -1;
}

int DocTabWidget::addDoc(QWidget* w, const QVariant& doc, const QString& title )
{
	d_views.append( doc );
    addTab( w, title );
	const bool old = d_backLock;
	d_backLock = true;
	setCurrentIndex( d_views.size() - 1 );
	d_order.append( w );
	d_backLock = old;
	return d_views.size() - 1;
}

void DocTabWidget::onCloseDoc()
{
	const int i = currentIndex();
    ENABLED_IF( i != -1 && !d_views[i].isNull() );

    if( isUnsaved(i) )
    {
        switch( QMessageBox::critical( this, tr("Closing tab"),
                                       tr("The tab has unsaved data; do you want to save it?"),
                               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes ) )
        {
        case QMessageBox::Yes:
            if( !save(i) )
            {
                QMessageBox::critical( this, tr("Closing tab"), tr("Error while saving the tab") );
                return;
            }
            break;
        case QMessageBox::No:
            break;
        default:
            return;
        }
    }
	closeTab( i );
}

void DocTabWidget::onTabChanged( int i )
{
	if( i < 0 ) // bei remove des letzen Tab kommt -1
	{
		d_order.clear();
	}else
	{
		const int pos = d_order.indexOf( widget(i) );
		if( pos != -1 )
		{
			QWidget* w = d_order[pos];
			d_order.removeAt( pos );
			d_order.append( w );
		}
	}
	updateState();
}

void DocTabWidget::updateState()
{
    d_closer->setEnabled( currentIndex() != -1 && !d_views[ currentIndex() ].isNull() );
	if( d_hideSingleTab)
        tabBar()->setVisible( count() > 1 );
}

bool DocTabWidget::checkSavedAll(bool butCur)
{
    const int except = butCur ? currentIndex() : -1;
    QList<int> toSave;
    for( int i = 0; i < d_views.size(); i++ )
    {
        if( d_views[i].isNull() )
            continue;
        else if( i != except && isUnsaved(i) )
            toSave << i;
    }
    if( !toSave.isEmpty() )
    {
        switch( QMessageBox::critical( this, tr("Closing tabs"),
                                       tr("Some tabs have unsaved data; do you want to save them?"),
                               QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes ) )
        {
        case QMessageBox::Yes:
            foreach( int i, toSave )
                if( !save(i) )
                    QMessageBox::critical( this, tr("Closing tabs"), tr("Error while saving the tabs") );
                    return false;
            break;
        case QMessageBox::No:
            break;
        default:
            return false;
        }
    }
    return true;
}

bool DocTabWidget::isUnsaved(int)
{
    return false;
}

bool DocTabWidget::save(int)
{
    return true;
}

void DocTabWidget::closeTab( int i )
{
	if( i == -1 || i >= d_views.size() )
		return;
    if( d_views[i].isNull() )
		return;
	emit closing( i );
	QWidget* w = widget(i);
	Q_ASSERT( w != 0 );
	for( int j = 0; j < d_order.size(); j++ )
	{
		if( d_order[j] == w ) 
		{
			d_order.removeAt( j );
			break;
		}
	}
    d_views.removeAt( i );
    removeTab( i );
    w->deleteLater();
}

void DocTabWidget::onDocSelect()
{
	ENABLED_IF( count() > 1 );

	for( int i = 0; i < d_order.size(); i++ )
		d_order[i]->setObjectName( tabText( indexOf( d_order[i] ) ) ); // ev. teuer
	DocSelector* ds = new DocSelector( d_order );
    connect( ds, SIGNAL( sigSelected( QWidget* ) ), this, SLOT( onSelectDoc( QWidget* ) ) );
}

void DocTabWidget::onCloseAll()
{
    ENABLED_IF(!d_views.isEmpty());

    if( !checkSavedAll(false) )
        return;
    int i = 0;
    while( i < d_views.size() )
    {
        if( d_views[i].isNull() )
            i++;
        else
            closeTab(i);
    }
}

void DocTabWidget::onCloseAllButThis()
{
    const int cur = currentIndex();
    ENABLED_IF( cur != -1 && !d_views[cur].isNull() );

    if( !checkSavedAll(true) )
        return;
    QWidget* w = widget(cur);
    int i = 0;
    while( i < d_views.size() )
    {
        if( d_views[i].isNull() || widget(i) == w )
            i++;
        else
            closeTab(i);
    }
}

void DocTabWidget::onSelectDoc( QWidget* w )
{
	for( int i = 0; i < count(); i++ )
		if( widget(i) == w )
		{
			setCurrentIndex( i );
			return;
		}
}

QVariant DocTabWidget::getCurrentDoc() const
{
	if( currentIndex() == -1 )
        return QVariant();
	else
        return d_views[ currentIndex() ];
}

QVariant DocTabWidget::getDoc(int i) const
{
    if( i < d_views.size() )
        return d_views[ i ];
    else
        return QVariant();
}

QWidget* DocTabWidget::getCurrentTab() const
{
	if( currentIndex() == -1 )
		return 0;
	else
		return widget( currentIndex() );
}
