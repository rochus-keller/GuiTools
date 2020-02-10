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

#include "CodeEditor.h"
#include <GuiTools/AutoMenu.h>
#include <QPainter>
#include <QtDebug>
#include <QFile>
#include <QScrollBar>
#include <QBuffer>
#include <QApplication>
#include <QClipboard>
#include <QInputDialog>
#include <QPrinter>
#include <QFileDialog>
#include <QFileInfo>
#include <QPrintDialog>
#include <QSettings>
#include <QFontDialog>
#include <QShortcut>
#include <QTextBlock>
#include <QMessageBox>

// adaptiert aus AdaViewer::AdaEditor

static const int s_charPerTab = 4; // default
static const int s_typingLatencyMs = 200; // default
static const int s_cursorLatencyMs = 500;

static inline int calcPosFromIndent( const QTextBlock& b, int indent, int charPerTab )
{
    const QString text = b.text();
    int spaces = 0;
    for( int i = 0; i < text.size(); i++ )
    {
        if( text[i] == QChar('\t') )
            spaces += charPerTab;
        else if( text[i] == QChar( ' ' ) )
            spaces++;
        else
            return b.position() + i; // ohne das aktuelle Zeichen, das ja kein Space ist
        if( ( spaces / charPerTab ) >= indent )
            return b.position() + i + 1; // inkl. dem aktuellen Zeichen, mit dem die Bedingung erfllt ist.
    }
    return b.position();
}

static inline int calcIndentsOfLine( const QTextBlock& b, int charPerTab, int* off = 0,
                                     bool* onlyWhitespace = 0, bool* rmWhitespace = 0 )
{
    if( onlyWhitespace )
        *onlyWhitespace = false;
    if( off )
        *off = 0;

    const QString text = b.text();

    int spaces = 0;
    int i;
    for( i = 0; i < text.size(); i++ )
    {
        if( text[i] == QChar('\t') )
        {
            spaces += charPerTab;
            if( off )
                *off += 1;
        }else if( text[i] == QChar( ' ' ) )
        {
            spaces++;
            if( off )
                *off += 1;
        }else
            break;
    }
    if( onlyWhitespace )
        *onlyWhitespace = ( i >= text.size() );

    if( rmWhitespace != 0 )
    {
        const int assigPos = text.indexOf("::=");
        const int commentPos = text.indexOf("//");

        if( commentPos == *off )
        {
            *rmWhitespace = true;
            return 0; // Ziehe Zeilen nur mit Comment nach Links
        }

        if( assigPos != -1 && ( commentPos == -1 || assigPos < commentPos ) )
        {
            *rmWhitespace = true;
            return 0; // Ziehe Zeilen mit einer Production nach links
        }
    }

    return spaces / charPerTab;
}

class _HandleArea : public QWidget
{
public:
    _HandleArea(CodeEditor *editor) : QWidget(editor), d_start(-1)
    {
        d_codeEditor = editor;
    }

    QSize sizeHint() const
    {
        return QSize(d_codeEditor->handleAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event)
    {
        // Weil die meisten Funktionen protected sind, delegieren wir die Arbeit an den Editor.
        d_codeEditor->paintHandleArea(event);
    }
    void mousePressEvent(QMouseEvent * event)
    {
        if( event->buttons() != Qt::LeftButton )
            return;
        if( event->modifiers() == Qt::ShiftModifier )
        {
            QTextCursor cur = d_codeEditor->textCursor();
            const int selStart = d_codeEditor->document()->findBlock( cur.selectionStart() ).blockNumber();
            const int selEnd = d_codeEditor->document()->findBlock( cur.selectionEnd() ).blockNumber();
            Q_ASSERT( selStart <= selEnd ); // bei position und anchor nicht erfllt
            const int clicked = d_codeEditor->lineAt( event->pos() );
            if( clicked <= selEnd )
            {
                if( cur.selectionStart() == cur.position() )
                    d_codeEditor->selectLines( selEnd, clicked );
                else
                    d_codeEditor->selectLines( selStart, clicked );
            }else
                d_codeEditor->selectLines( selStart, clicked );
        }else
        {
            // Beginne Drag
            d_start = d_codeEditor->lineAt( event->pos() );
            d_codeEditor->selectLines( d_start, d_start );
        }
    }
    void mouseMoveEvent(QMouseEvent * event)
    {
        if( d_start != -1 )
        {
            const int cur = d_codeEditor->lineAt( event->pos() );
            d_codeEditor->selectLines( d_start, cur );
        }
    }
    void mouseReleaseEvent(QMouseEvent * event )
    {
        if( d_start != -1 )
        {
            const int cur = d_codeEditor->lineAt( event->pos() );
            d_codeEditor->selectLines( d_start, cur );
            d_start = -1;
        }
    }
    void mouseDoubleClickEvent(QMouseEvent * event)
    {
        if( event->buttons() != Qt::LeftButton )
            return;
        const int line = d_codeEditor->lineAt( event->pos() );
        d_codeEditor->numberAreaDoubleClicked( line );
    }

private:
    CodeEditor* d_codeEditor;
    int d_start;
};

CodeEditor::CodeEditor(QWidget *parent) :
	QPlainTextEdit(parent), d_showNumbers(true),
    d_undoAvail(false),d_redoAvail(false),d_copyAvail(false),d_curPos(-1),
    d_pushBackLock(false), d_noEditLock(false), d_linkLineNr(0), d_linkColNr(0),d_paintIndents(true)
{
    d_charPerTab = s_charPerTab;
    d_typingLatencyMs = s_typingLatencyMs;
    setFont( defaultFont() );
    setLineWrapMode( QPlainTextEdit::NoWrap );
    setTabStopWidth( 30 );
    setTabChangesFocus(false);
    setMouseTracking(true);

    d_numberArea = new _HandleArea(this);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth()));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));
    connect(this, SIGNAL(undoAvailable(bool)), this, SLOT(onUndoAvail(bool)) );
    connect(this, SIGNAL(redoAvailable(bool)), this, SLOT(onRedoAvail(bool)) );
    connect( this, SIGNAL(copyAvailable(bool)), this, SLOT(onCopyAvail(bool)) );
    connect( this, SIGNAL( cursorPositionChanged() ), this, SLOT(  onUpdateCursor() ) );

    updateLineNumberAreaWidth();

    //d_hl = new EbnfHighlighter( document() );
    //updateTabWidth();

	QSettings set;
	const bool showLineNumbers = set.value( "CodeEditor/ShowLineNumbers" ).toBool();
	setShowNumbers( showLineNumbers );
	setFont( set.value( "CodeEditor/Font", QVariant::fromValue( font() ) ).value<QFont>() );

    connect( this, SIGNAL(textChanged()), this, SLOT(onTextChanged()) );
    d_typingLatency.setSingleShot(true);
    connect(&d_typingLatency, SIGNAL(timeout()), this, SLOT(onUpdateModel()));

    d_cursorLatency.setSingleShot(true);
    connect(&d_cursorLatency, SIGNAL(timeout()), this, SLOT(onUpdateLocation()));
}

QFont CodeEditor::defaultFont()
{
	QFont f;
#ifdef Q_WS_X11
	f.setFamily("Courier");
#else
	f.setStyleHint( QFont::TypeWriter );
#endif
	f.setPointSize(9);
	return f;
}

int CodeEditor::handleAreaWidth()
{
    if( !d_showNumbers )
        return 10; // RISK

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 5 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space;
}

int CodeEditor::lineAt(const QPoint & p) const
{
    const int y =  p.y() - contentOffset().y();
    return cursorForPosition( QPoint( contentsRect().left(), y ) ).blockNumber();
}

void CodeEditor::getCursorPosition(int *line, int *col)
{
    // Qt-Koordinaten
    QTextCursor cur = textCursor();
    if( line )
        *line = cur.block().blockNumber();
    if( col )
        *col = cur.positionInBlock();
}

void CodeEditor::setCursorPosition(int line, int col, bool center )
{
    // Qt-Koordinaten
    if( line >= 0 && line < document()->blockCount() )
    {
        QTextBlock block = document()->findBlockByNumber(line);
        QTextCursor cur = textCursor();
        cur.setPosition( block.position() + col );
        setTextCursor( cur );
        if( center )
            centerCursor();
        else
            ensureCursorVisible();
        updateExtraSelections();
        onUpdateLocation();
    }
}

void CodeEditor::clearNonTerms()
{
    d_nonTerms.clear();
    updateExtraSelections();
}

QString CodeEditor::textLine(int i) const
{
    if( i < document()->blockCount() )
        return document()->findBlockByNumber(i).text();
    else
		return QString();
}

int CodeEditor::lineCount() const
{
    return document()->blockCount();
}

void CodeEditor::ensureLineVisible(int line)
{
    if( line >= 0 && line < document()->blockCount() )
    {
        QTextBlock b = document()->findBlockByNumber(line);
        const int p = b.position();
        QTextCursor cur = textCursor();
        cur.setPosition( p );
        setTextCursor( cur );
        ensureCursorVisible();
    }
}

void CodeEditor::setPositionMarker(int line)
{
    d_curPos = line;
    d_numberArea->update();
    ensureLineVisible( line );
}

void CodeEditor::setSelection(int lineFrom, int indexFrom, int lineTo, int indexTo)
{
    if( lineFrom < document()->blockCount() && lineTo < document()->blockCount() )
    {
        QTextCursor cur = textCursor();
        cur.setPosition( document()->findBlockByNumber(lineFrom).position() + indexFrom );
        cur.setPosition( document()->findBlockByNumber(lineTo).position() + indexTo, QTextCursor::KeepAnchor );
        setTextCursor( cur );
        ensureCursorVisible();
    }
}

void CodeEditor::selectLines(int lineFrom, int lineTo)
{
    if( lineFrom < document()->blockCount() && lineTo < document()->blockCount() )
    {
        QTextCursor cur = textCursor();
        QTextBlock from = document()->findBlockByNumber(lineFrom);
        QTextBlock to = document()->findBlockByNumber(lineTo);
        if( lineFrom < lineTo )
        {
            cur.setPosition( from.position() );
            cur.setPosition( to.position() + to.length() - 1, QTextCursor::KeepAnchor );
        }else
        {
            // Auch wenn sie gleich sind, wird von rechts nach links selektiert, damit kein HoriScroll bei berlnge
            cur.setPosition( from.position() + from.length() - 1 );
            cur.setPosition( to.position(), QTextCursor::KeepAnchor );
        }
        setTextCursor( cur );
    }
}

bool CodeEditor::hasSelection() const
{
    return textCursor().hasSelection();
}

QString CodeEditor::selectedText() const
{
    return textCursor().selectedText();
}

bool CodeEditor::isModified() const
{
    return ( document() && document()->isModified() );
}

void CodeEditor::updateLineNumberAreaWidth()
{
    setViewportMargins( handleAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        d_numberArea->scroll(0, dy);
    else
        d_numberArea->update(0, rect.y(), d_numberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
		updateLineNumberAreaWidth();
}

void CodeEditor::onUpdateCursor()
{
    d_cursorLatency.start(s_cursorLatencyMs);
}

void CodeEditor::onTextChanged()
{
    if( d_noEditLock )
        return;
    d_typingLatency.start(d_typingLatencyMs);
}

void CodeEditor::onUpdateModel()
{
    //qDebug() << "updating model";
}

void CodeEditor::onUpdateLocation()
{
    int line, col;
    getCursorPosition( &line, &col );
    pushLocation(Location(line,col));
    emit sigUpdateLocation(line,col);
}

void CodeEditor::handleEditUndo()
{
	ENABLED_IF( isUndoAvailable() );
	undo();
}

void CodeEditor::handleEditRedo()
{
	ENABLED_IF( isRedoAvailable() );
	redo();
}

void CodeEditor::handleEditCut()
{
	ENABLED_IF( !isReadOnly() && isCopyAvailable() );
	cut();
}

void CodeEditor::handleEditCopy()
{
	ENABLED_IF( isCopyAvailable() );
	copy();
}

void CodeEditor::handleEditPaste()
{
	QClipboard* cb = QApplication::clipboard();
	ENABLED_IF( !isReadOnly() && !cb->text().isNull() );
	paste();
}

void CodeEditor::handleEditSelectAll()
{
	ENABLED_IF( true );
	selectAll();
}

void CodeEditor::handleFind()
{
	ENABLED_IF( true );
    bool ok	= false;
    const QString sel = textCursor().selectedText();
	QString res = QInputDialog::getText( this, tr("Find Text"),
        tr("Enter a string to look for:"), QLineEdit::Normal, sel, &ok );
	if( !ok )
		return;
    d_find = res;
    find( sel.isEmpty() );
}

void CodeEditor::handleFindAgain()
{
	ENABLED_IF( !d_find.isEmpty() );
	find( false );
}

void CodeEditor::handleReplace()
{
	// TODO
}

void CodeEditor::handleGoto()
{
	ENABLED_IF( true );
	int line, col;
	getCursorPosition( &line, &col );
    bool ok	= false;
    line = QInputDialog::getInt( this, tr("Goto Line"),
		tr("Please	enter a valid line number:"),
		line + 1, 1, 999999, 1,	&ok );
	if( !ok )
		return;
	setCursorPosition( line - 1, col );
	//ensureLineVisible( line - 1 );
}

void CodeEditor::handleIndent()
{
	ENABLED_IF( !isReadOnly() );

	indent();
}

void CodeEditor::handleUnindent()
{
	ENABLED_IF( !isReadOnly() );

	unindent();
}

void CodeEditor::handleSetIndent()
{
	ENABLED_IF( !isReadOnly() );

	bool ok;
    const int level = QInputDialog::getInt( this, tr("Set Indentation Level"),
		tr("Enter the indentation level (0..20):"), 0, 0, 20, 1, &ok );
	if( ok )
        setIndentation( level );
}

void CodeEditor::handleFixIndent()
{
    ENABLED_IF( !isReadOnly() );

    QTextCursor cur = textCursor();

    cur.beginEditBlock();
    fixIndent();
    cur.endEditBlock();
}

void CodeEditor::find(bool fromTop)
{
	int line, col;
	getCursorPosition( &line, &col );
	if( fromTop )
	{
		line = 0;
		col = 0;
	}else
		col++;
	const int count = lineCount();
	int j = -1;
    const int start = qMax(line,0);
    bool turnedAround = false;
    for( int i = start; i < count; i++ )
	{
        j = textLine( i ).indexOf( d_find, col,Qt::CaseInsensitive );
		if( j != -1 )
		{
			line = i;
			col = j;
			break;
		}else if( i < count )
			col = 0;
        if( j == -1 && start != 0 && !turnedAround && i == count - 1 )
        {
            turnedAround = true;
            i = -1;
        }
    }
	if( j != -1 )
	{
		setCursorPosition( line, col + d_find.size() );
		ensureLineVisible( line );
		setSelection( line, col, line, col + d_find.size() );
	}

}

void CodeEditor::fixIndent()
{
    QTextCursor cur = textCursor();

    const int selStart = cur.selectionStart();
    const int selEnd = cur.selectionEnd();
    Q_ASSERT( selStart <= selEnd );

    QTextBlock b = document()->findBlock( selStart );
    do
    {
        int startOfNws = 0;
        bool onlyWs;
        bool rmWs;
        int indent = calcIndentsOfLine(b,d_charPerTab,&startOfNws,&onlyWs,&rmWs);
        if( startOfNws != 0 )
        {
            cur.setPosition( b.position() );
            cur.setPosition( b.position() + startOfNws, QTextCursor::KeepAnchor );
            if( onlyWs )
                cur.deleteChar();
            else
            {
                if( !rmWs )
                    indent = indent == 0 && startOfNws != 0 ? 1 : indent;
                cur.insertText( QString( indent, QChar('\t') ) );
            }
        }
        b = b.next();
    }while( b.isValid() && b.position() < selEnd );
}

void CodeEditor::updateExtraSelections()
{
    ESL sum;

    QTextEdit::ExtraSelection line;
    line.format.setBackground(QColor(Qt::yellow).lighter(170));
    line.format.setProperty(QTextFormat::FullWidthSelection, true);
    line.cursor = textCursor();
    line.cursor.clearSelection();
    sum << line;

    sum << d_nonTerms;

    // TODO override

    sum << d_link;

    setExtraSelections(sum);
}

void CodeEditor::handlePrint()
{
	ENABLED_IF( true );

	QPrinter p;
	p.setPageMargins( 15, 10, 10, 10, QPrinter::Millimeter );

	QPrintDialog dialog( &p, this );
	if( dialog.exec() )
	{
		print( &p );
	}
}

void CodeEditor::handleExportPdf()
{
	ENABLED_IF( true );

	QString fileName = QFileDialog::getSaveFileName(this, tr("Export PDF"), QString(), tr("*.pdf") );
	if (fileName.isEmpty())
		return;

	QFileInfo info( fileName );

	if( info.suffix().toUpper() != "PDF" )
		fileName += ".pdf";
	info.setFile( fileName );

	QPrinter p;
	p.setPageMargins( 15, 10, 10, 10, QPrinter::Millimeter );
	p.setOutputFormat(QPrinter::PdfFormat);
	p.setOutputFileName(fileName);

	print( &p );
}

void CodeEditor::handleShowLinenumbers()
{
	CHECKED_IF( true, showNumbers() );

	const bool showLineNumbers = !showNumbers();
	QSettings set;
	set.setValue( "CodeEditor/ShowLineNumbers", showLineNumbers );
	setShowNumbers( showLineNumbers );
}

void CodeEditor::handleSetFont()
{
	ENABLED_IF( true );

	bool ok;
	QFont res = QFontDialog::getFont( &ok, font(), this );
	if( !ok )
		return;
	QSettings set;
	set.setValue( "CodeEditor/Font", QVariant::fromValue(res) );
	set.sync();
	setFont( res );
}

void CodeEditor::handleGoBack()
{
    ENABLED_IF( d_backHisto.size() > 1 );

    d_pushBackLock = true;
    d_forwardHisto.push_back( d_backHisto.last() );
    d_backHisto.pop_back();
    setCursorPosition( d_backHisto.last().d_line, d_backHisto.last().d_col, true );
    d_pushBackLock = false;
}

void CodeEditor::handleGoForward()
{
    ENABLED_IF( !d_forwardHisto.isEmpty() );

    Location cur = d_forwardHisto.last();
    d_forwardHisto.pop_back();
    setCursorPosition( cur.d_line, cur.d_col, true );
}

void CodeEditor::pushLocation(const CodeEditor::Location& loc)
{
    if( d_pushBackLock )
        return;
    // qDebug() << "push" << loc.d_line << loc.d_col;
    if( !d_backHisto.isEmpty() && d_backHisto.last() == loc )
        return; // o ist bereits oberstes Element auf dem Stack.
    d_backHisto.removeAll( loc );
    d_backHisto.push_back( loc );
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    d_numberArea->setGeometry(QRect(cr.left(), cr.top(), handleAreaWidth(), cr.height()));
}

void CodeEditor::paintEvent(QPaintEvent *e)
{
    QPlainTextEdit::paintEvent( e );
    if( d_paintIndents )
        paintIndents( e );
}

static inline int _firstNwsPos( const QTextBlock& b )
{
    const QString str = b.text();
    for( int i = 0; i < str.size(); i++ )
    {
        if( !str[i].isSpace() )
            return b.position() + i;
    }
    return b.position() + b.length(); // Es ist nur WS vorhanden
}

bool CodeEditor::viewportEvent( QEvent * event )
{
    if( event->type() == QEvent::FontChange )
    {
        updateTabWidth();
        updateLineNumberAreaWidth();
    }
    return QPlainTextEdit::viewportEvent(event);
}

void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    // NOTE: Qt macht aus SHIFT+TAB automatisch BackTab und versendet das!
    if( e->key() == Qt::Key_Tab )
    {
        if( e->modifiers() & Qt::ControlModifier )
        {
            // Wir brauchen CTRL+TAB fr Umschalten der Scripts
            e->ignore();
            return;
        }
        if( !isReadOnly() && ( hasSelection() || textCursor().position() <= _firstNwsPos( textCursor().block() ) ) )
        {
            if( e->modifiers() == Qt::NoModifier )
            {
                indent();
                e->accept();
                return;
            }
        }
	}else if( e->key() == Qt::Key_Backtab )
    {
        if( e->modifiers() & Qt::ControlModifier )
        {
            // Wir brauchen CTRL+TAB fr Umschalten der Scripts
            e->ignore();
            return;
        }
        if( !isReadOnly() && ( hasSelection() || textCursor().position() <= _firstNwsPos( textCursor().block() ) ) )
        {
            unindent();
            e->accept();
            return;
        }
    }else if( !isReadOnly() && ( e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return ) )
    {
        e->accept();
        if( e->modifiers() != Qt::NoModifier )
            return; // Verschlucke Return mit Ctrl etc.
        textCursor().beginEditBlock();
        QPlainTextEdit::keyPressEvent( e ); // Lasse Qt den neuen Block einfgen
        QTextBlock prev = textCursor().block().previous();
        if( prev.isValid() )
        {
            const int ws = _firstNwsPos( prev );
            textCursor().insertText( prev.text().left( ws - prev.position() ) );
        }
        textCursor().endEditBlock();
        return;
    }
    QPlainTextEdit::keyPressEvent( e );
}

void CodeEditor::mousePressEvent(QMouseEvent* e)
{
    if( !d_link.isEmpty() )
    {
        QTextCursor cur = cursorForPosition(e->pos());
        pushLocation( Location( cur.blockNumber(), cur.positionInBlock() ) );

        QApplication::restoreOverrideCursor();
        d_link.clear();
        setCursorPosition( d_linkLineNr, d_linkColNr, true );
    }else if( QApplication::keyboardModifiers() == Qt::ControlModifier )
    {
        /* to override
        QTextCursor cur = cursorForPosition(e->pos());
        const Ast::Symbol* sym = d_syn->findSymbolBySourcePos(cur.blockNumber() + 1,cur.positionInBlock() + 1);
        if( sym )
        {
            const Ast::Definition* d = d_syn->getDef( sym->d_tok.d_val );
            if( d )
            {
                pushLocation( Location( cur.blockNumber(), cur.positionInBlock() ) );
                setCursorPosition( d->d_tok.d_lineNr - 1, 0, true );
            }
       }
       */
    }else
        QPlainTextEdit::mousePressEvent(e);
}

void CodeEditor::mouseMoveEvent(QMouseEvent* e)
{
    QPlainTextEdit::mouseMoveEvent(e);
    /* to override
    if( QApplication::keyboardModifiers() == Qt::ControlModifier && d_syn.constData() )
    {
        QTextCursor cur = cursorForPosition(e->pos());
        const Ast::Symbol* sym = d_syn->findSymbolBySourcePos(cur.blockNumber() + 1, cur.positionInBlock() + 1);
        const bool alreadyArrow = !d_link.isEmpty();
        d_link.clear();
        if( sym )
        {
            //qDebug() << "Sym" << sym->d_tok.d_type << sym->d_tok.d_val;
            const int off = cur.positionInBlock() + 1 - sym->d_tok.d_colNr;
            cur.setPosition(cur.position() - off);
            cur.setPosition( cur.position() + sym->d_tok.d_len, QTextCursor::KeepAnchor );
            const Ast::Definition* d = d_syn->getDef( sym->d_tok.d_val );

            if( d )
            {
                QTextEdit::ExtraSelection sel;
                sel.cursor = cur;
                sel.format.setFontUnderline(true);
                d_link << sel;
                d_linkLineNr = d->d_tok.d_lineNr - 1;
                if( !alreadyArrow )
                    QApplication::setOverrideCursor(Qt::ArrowCursor);
            }
        }
        if( alreadyArrow && d_link.isEmpty() )
            QApplication::restoreOverrideCursor();
        updateExtraSelections();
    }else */
    if( !d_link.isEmpty() )
    {
        QApplication::restoreOverrideCursor();
        d_link.clear();
        updateExtraSelections();
    }
}

void CodeEditor::paintIndents(QPaintEvent *)
{
    QPainter p( viewport() );

    QPointF offset(contentOffset());
    QTextBlock block = firstVisibleBlock();
    const QRect viewportRect = viewport()->rect();
    // erst ab Qt5 const int margin = document()->documentMargin();
    const int margin = 4; // RISK: empirisch ermittelt; unklar, wo der herkommt (blockBoundingRect offensichtlich nicht)

    while( block.isValid() )
    {
        const QRectF r = blockBoundingRect(block).translated(offset);
        p.setPen( Qt::lightGray );
        const int indents = calcIndentsOfLine( block, d_charPerTab );
        for( int i = 0; i <= indents; i++ )
        {
            const int x0 = r.x() + ( i - 1 ) * tabStopWidth() + margin + 1; // + 1 damit Cursor nicht verdeckt wird
            p.drawLine( x0, r.top(), x0, r.bottom() - 1 );
        }
        offset.ry() += r.height();
        if (offset.y() > viewportRect.height())
            break;
        block = block.next();
    }
}

void CodeEditor::updateTabWidth()
{
    setTabStopWidth( fontMetrics().width( QLatin1Char('0') ) * d_charPerTab );
}

void CodeEditor::setCharPerTab(quint8 cpt)
{
    d_charPerTab = cpt;
    updateTabWidth();
}

void CodeEditor::setPaintIndents(bool on)
{
     d_paintIndents = on;
     update();
}

void CodeEditor::highlightCurrentLine()
{
    updateExtraSelections();
}

void CodeEditor::paintHandleArea(QPaintEvent *event)
{
    QPainter painter(d_numberArea);
    painter.fillRect(event->rect(), QColor(224,224,224) );

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    const int w = fontMetrics().width('w') + 2;
    const int h = fontMetrics().height();
    while (block.isValid() && top <= event->rect().bottom())
    {
        painter.setPen(Qt::black);
        if( d_breakPoints.contains( blockNumber ) )
        {
            const QRect r = QRect( 0, top, d_numberArea->width(), h );
            painter.fillRect( r, Qt::darkRed );
            painter.setPen(Qt::white);
        }
        if( d_showNumbers && block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.drawText(0, top, d_numberArea->width() - 2, h, Qt::AlignRight, number);
        }
        if( blockNumber == d_curPos )
        {
            const QRect r = QRect( d_numberArea->width() - w, top, w, h );
            painter.setBrush(Qt::yellow);
            painter.setPen(Qt::black);
            painter.drawPolygon( QPolygon() << r.topLeft() << r.bottomLeft() << r.adjusted(0,0,0,-h/2).bottomRight() );
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void CodeEditor::indent()
{
    QTextCursor cur = textCursor();

    const int selStart = cur.selectionStart();
    const int selEnd = cur.selectionEnd();
    Q_ASSERT( selStart <= selEnd );

    QTextBlock b = document()->findBlock( selStart );
    cur.beginEditBlock();
    do
    {
        cur.setPosition( b.position() );
        cur.insertText( QChar('\t') );
        b = b.next();
    }while( b.isValid() && b.position() < selEnd );
    cur.endEditBlock();
}

void CodeEditor::unindent()
{
    QTextCursor cur = textCursor();

    const int selStart = cur.selectionStart();
    const int selEnd = cur.selectionEnd();
    Q_ASSERT( selStart <= selEnd );

    QTextBlock b = document()->findBlock( selStart );
    cur.beginEditBlock();
    do
    {
        const int indent = calcIndentsOfLine(b, d_charPerTab);
        if( indent > 0 )
        {
            cur.setPosition( b.position() );
            cur.setPosition( calcPosFromIndent( b, d_charPerTab, indent ), QTextCursor::KeepAnchor );
            cur.insertText( QString( indent - 1, QChar('\t') ) );
        }
        b = b.next();
    }while( b.isValid() && b.position() < selEnd );
    cur.endEditBlock();
}

void CodeEditor::setIndentation(int i)
{
    QTextCursor cur = textCursor();

    const int selStart = cur.selectionStart();
    const int selEnd = cur.selectionEnd();
    Q_ASSERT( selStart <= selEnd );

    QTextBlock b = document()->findBlock( selStart );
    cur.beginEditBlock();
    do
    {
        const int indent = calcIndentsOfLine(b,d_charPerTab);
        cur.setPosition( b.position() );
        cur.setPosition( calcPosFromIndent( b, d_charPerTab, indent ), QTextCursor::KeepAnchor );
        cur.insertText( QString( i, QChar('\t') ) );
        b = b.next();
    }while( b.isValid() && b.position() < selEnd );
    cur.endEditBlock();
}

void CodeEditor::setShowNumbers(bool on)
{
    d_showNumbers = on;
    updateLineNumberAreaWidth();
    viewport()->update();
}

void CodeEditor::newFile()
{
    d_noEditLock = true;
    setPlainText(QString());
    d_noEditLock = false;
    d_path.clear();
    d_backHisto.clear();
    d_forwardHisto.clear();
    document()->setModified( false );
    emit modificationChanged(false);
}

bool CodeEditor::loadFromFile(const QString &path)
{
    QFile file(path);
    if( !file.open(QIODevice::ReadOnly ) )
        return false;
    d_noEditLock = true;
    setPlainText( QString::fromUtf8( file.readAll() ) );
    d_noEditLock = false;
    d_path = path;
    d_backHisto.clear();
    d_forwardHisto.clear();
    document()->setModified( false );
    emit modificationChanged(false);
    return true;
}

bool CodeEditor::saveToFile(const QString& path, bool report)
{
    QFile file(path);
    if( !file.open(QIODevice::WriteOnly ) )
    {
        if( report )
        {
            QMessageBox::critical(this,tr("Save File"), tr("Cannot save file to '%1'. %2").
                                  arg(path).arg(file.errorString()));
        }
        return false;
    }
    file.write( toPlainText().toUtf8() );
    document()->setModified( false );
    d_path = path;
    return true;
}

void CodeEditor::addBreakPoint(quint32 l)
{
    d_breakPoints.insert( l );
    d_numberArea->update();
}

void CodeEditor::removeBreakPoint(quint32 l)
{
    d_breakPoints.remove( l );
    d_numberArea->update();
}

bool CodeEditor::toggleBreakPoint(quint32* out)
{
    int line = -1;
    getCursorPosition(&line);
    if( line == -1 )
        line = 0;
    if( d_breakPoints.contains(line) )
    {
        d_breakPoints.remove(line);
        d_numberArea->update();
        if(out)
            *out = line;
        return false;
    }else if( line >= 0 )
    {
        d_breakPoints.insert( line );
        d_numberArea->update();
        if(out)
            *out = line;
        return true;
    }
}

void CodeEditor::clearBreakPoints()
{
    d_breakPoints.clear();
    d_numberArea->update();
}

void CodeEditor::installDefaultPopup()
{
    Gui::AutoMenu* pop = new Gui::AutoMenu( this, true );
    pop->addCommand( "Undo", this, SLOT(handleEditUndo()), tr("CTRL+Z"), true );
    pop->addCommand( "Redo", this, SLOT(handleEditRedo()), tr("CTRL+Y"), true );
	pop->addSeparator();
    pop->addCommand( "Cut", this, SLOT(handleEditCut()), tr("CTRL+X"), true );
	pop->addCommand( "Copy", this, SLOT(handleEditCopy()), tr("CTRL+C"), true );
    pop->addCommand( "Paste", this, SLOT(handleEditPaste()), tr("CTRL+V"), true );
	pop->addCommand( "Select all", this, SLOT(handleEditSelectAll()), tr("CTRL+A"), true  );
	//pop->addCommand( "Select Matching Brace", this, SLOT(handleSelectBrace()) );
	pop->addSeparator();
    pop->addCommand( "Find...", this, SLOT(handleFind()), tr("CTRL+F") );
    pop->addCommand( "Find again", this, SLOT(handleFindAgain()), tr("F3") );
    pop->addCommand( "Replace...", this, SLOT(handleReplace()), tr("CTRL+R"), true );
    pop->addSeparator();
    pop->addCommand( "&Goto...", this, SLOT(handleGoto()), tr("CTRL+G"), true );
    pop->addCommand( "Go Back", this, SLOT(handleGoBack()), tr("ALT+Left"), true );
    pop->addCommand( "Go Forward", this, SLOT(handleGoForward()), tr("ALT+Right"), true );
    pop->addSeparator();
    pop->addCommand( "Indent", this, SLOT(handleIndent()) );
    pop->addCommand( "Unindent", this, SLOT(handleUnindent()) );
    pop->addCommand( "Fix Indents", this, SLOT(handleFixIndent()) );
    pop->addCommand( "Set Indentation Level...", this, SLOT(handleSetIndent()) );
	pop->addSeparator();
	pop->addCommand( "Print...", this, SLOT(handlePrint()), tr("CTRL+P"), true );
	pop->addCommand( "Export PDF...", this, SLOT(handleExportPdf()), tr("CTRL+SHIFT+P"), true );
	pop->addSeparator();
	pop->addCommand( "Set &Font...", this, SLOT(handleSetFont()) );
    pop->addCommand( "Show &Linenumbers", this, SLOT(handleShowLinenumbers()) );
}
