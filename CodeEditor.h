#ifndef GENERICCODEEDITOR_H
#define GENERICCODEEDITOR_H

/*
* Copyright 2019 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the EbnfStudio application.
*
* The following is the license that applies to this copy of the
* application. For a license to use the application under conditions
* other than those described here, please email to me@rochus-keller.ch.
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

#include <QPlainTextEdit>
#include <QSet>
#include <QTimer>

// adaptiert aus AdaViewer::AdaEditor

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = 0);
    static QFont defaultFont();

    void newFile();
    void setText( const QString& );
    bool loadFromFile(const QString &path);
    bool loadFromFile(QIODevice*, const QString &path = QString());
    bool saveToFile(const QString& path, bool report = true);
    QString getPath() const { return d_path; }

    void paintHandleArea(QPaintEvent *event);
    int handleAreaWidth();
    int lineAt( const QPoint& ) const;

    QString textLine( int i ) const;
    int lineCount() const;
    void ensureLineVisible( int line );
    void setPositionMarker( int line ); // -1..unsichtbar
    void setSelection(int lineFrom,int indexFrom, int lineTo,int indexTo);
    void selectLines( int lineFrom, int lineTo );
    bool hasSelection() const;
    QString selectedText() const;

    void getCursorPosition(int *line,int *col = 0);
    void setCursorPosition(int line, int col, bool center = false);
    void clearNonTerms();
    virtual void updateExtraSelections();

    bool isUndoAvailable() const { return d_undoAvail; }
    bool isRedoAvailable() const { return d_redoAvail; }
    bool isCopyAvailable() const { return d_copyAvail; }

    bool isModified() const;

    void indent();
    void setIndentation(int);
    void unindent();

    void installDefaultPopup();

    void setShowNumbers( bool on );
    bool showNumbers() const { return d_showNumbers; }
    void updateTabWidth();
    void setCharPerTab( quint8 );
    void setPaintIndents(bool on);
    void setTypingLatency( int l ) { d_typingLatencyMs = l; }

    void addBreakPoint( quint32 );
    void removeBreakPoint( quint32 );
    bool toggleBreakPoint(quint32* out = 0); // current line
    void clearBreakPoints();
    const QSet<quint32>& getBreakPoints() const { return d_breakPoints; }
signals:
    void sigSyntaxUpdated();
    void sigUpdateLocation( int line, int col ); // cursor moved + latency

public slots:
    void handleEditUndo();
    void handleEditRedo();
    void handleEditCut();
    void handleEditCopy();
    void handleEditPaste();
    void handleEditSelectAll();
    void handleFind();
    void handleFindAgain();
    void handleReplace();
    void handleGoto();
    void handleIndent();
    void handleUnindent();
    void handleSetIndent();
    void handleFixIndent();
    void handlePrint();
    void handleExportPdf();
    void handleShowLinenumbers();
    void handleSetFont();
    void handleGoBack();
    void handleGoForward();
protected:
    friend class _HandleArea;
    struct Location
    {
        // Qt-Koordinaten
        int d_line;
        int d_col;
        bool operator==( const Location& rhs ) { return d_line == rhs.d_line && d_col == rhs.d_col; }
        Location(int l, int c ):d_line(l),d_col(c){}
    };
    void pushLocation( const Location& );
    void paintIndents( QPaintEvent *e );
    void find(bool fromTop);
    void fixIndent();

    // overrides
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *e);
    bool viewportEvent( QEvent * event );
    void keyPressEvent ( QKeyEvent * e );
    void mousePressEvent(QMouseEvent * e);
    void mouseMoveEvent(QMouseEvent * e);

    // To override
    virtual void numberAreaDoubleClicked( int ) {}
protected slots:
    void updateLineNumberAreaWidth();
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);
    void onUndoAvail(bool on) { d_undoAvail = on; }
    void onRedoAvail(bool on) { d_redoAvail = on; }
    void onCopyAvail(bool on) { d_copyAvail = on; }
    void onUpdateCursor();
    void onTextChanged();
    virtual void onUpdateModel();
    void onUpdateLocation();
protected:
    QWidget* d_numberArea;
    QSet<quint32> d_breakPoints;
    int d_curPos; // Zeiger für die aktuelle Ausführungsposition oder -1
    QString d_find;
    QString d_path;
    QTimer d_typingLatency;
    int d_typingLatencyMs;
    QTimer d_cursorLatency;
    typedef QList<QTextEdit::ExtraSelection> ESL;
    ESL d_link;
    ESL d_nonTerms;
    int d_linkLineNr,d_linkColNr;
    int d_charPerTab;
    QList<Location> d_backHisto; // d_backHisto.last() ist aktuell angezeigtes Objekt
    QList<Location> d_forwardHisto;
    bool d_pushBackLock;
    bool d_undoAvail;
    bool d_redoAvail;
    bool d_copyAvail;
    bool d_showNumbers;
    bool d_noEditLock;
    bool d_paintIndents;
};

#endif // GENERICCODEEDITOR_H
