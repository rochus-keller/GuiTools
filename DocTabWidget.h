#ifndef __Oln_DocTabWidget__
#define __Oln_DocTabWidget__

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

#include <QTabWidget>
#include <QVariant>

class QToolButton;

// adaptiert aus CrossLine

class DocTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    DocTabWidget( QWidget*, bool hideSingleTab = true );
    int addFixed( QWidget*, const QString& title );
    int findDoc(const QVariant& doc ); // Index oder -1
    int showDoc( const QVariant& doc ); // Index oder -1
    int addDoc( QWidget*, const QVariant& doc, const QString& title = QString() );
    int showWidget( QWidget* );
    QVariant getCurrentDoc() const;
    QVariant getDoc( int i ) const;
    QWidget* getCurrentTab() const;
    QTabBar* getBar() const { return tabBar(); }
    void closeTab( int i );
    void setCloserIcon( const QString& );
    // overrides
    QSize minimumSizeHint () const { return QSize( 30, 30 ); } // RISK
signals:
    void closing(int);
public slots:
    void onCloseDoc(); // Menübefehl
    void onDocSelect(); // Menübefehl
    void onCloseAll();
    void onCloseAllButThis();
protected slots:
    void onTabChanged( int );
    void onSelectDoc( QWidget* );
protected:
    void updateState();
    bool checkSavedAll(bool butCur );
    virtual bool isUnsaved(int);
    virtual bool save(int);
private:
    QList<QVariant> d_views; // isNull..fixed
    QList<QWidget*> d_order;
    QToolButton* d_closer;
    bool d_backLock;
    bool d_observed;
    bool d_hideSingleTab;
};

#endif
