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

#include "Controller.h"
#include <QKeyEvent>
using namespace Gui;


Controller::Controller(QWidget* view ):
	QObject( view )
{
	Q_ASSERT( view != 0 );
	view->installEventFilter( this );
}

void Controller::observe(QObject * o)
{
	o->installEventFilter( this );
}

void Controller::unobserve(QObject *o)
{
	o->removeEventFilter( this );
}

bool Controller::eventFilter( QObject * watched, QEvent * event )
{
	QWidget* v = view();
	if( watched != v )
		return false;

	bool done = false;

    // ignore mouse events when disabled
    if ( v && !v->isEnabled()) 
	{
        switch(event->type()) 
		{
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        // case QEvent::ContextMenu:
        case QEvent::Wheel:
            return false;
        default:
            break;
        }
    }
    switch (event->type()) 
	{
    case QEvent::MouseMove:
        done = mouseMoveEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonPress:
        done = mousePressEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonRelease:
        done = mouseReleaseEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonDblClick:
        done = mouseDoubleClickEvent((QMouseEvent*)event);
        break;
    case QEvent::Wheel:
        done = wheelEvent((QWheelEvent*)event);
        break;
    case QEvent::KeyPress: {
        QKeyEvent *k = (QKeyEvent *)event;
        bool res = false;
        if (!(k->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
            if (k->key() == Qt::Key_Backtab
                || (k->key() == Qt::Key_Tab && (k->modifiers() & Qt::ShiftModifier)))
                res = focusNextPrevChild(false);
            else if (k->key() == Qt::Key_Tab)
                res = focusNextPrevChild(true);
            if (res)
                break;
        }
        done = keyPressEvent(k);
		}
        break;

    case QEvent::KeyRelease:
        done = keyReleaseEvent((QKeyEvent*)event);
        // fall through
    case QEvent::ShortcutOverride:
        break;

    case QEvent::InputMethod:
        done = inputMethodEvent((QInputMethodEvent *) event);
        break;

    case QEvent::FocusIn:
        done = focusInEvent((QFocusEvent*)event);
        break;

    case QEvent::FocusOut:
        done = focusOutEvent((QFocusEvent*)event);
        break;

    case QEvent::Enter:
        done = enterEvent(event);
        break;

    case QEvent::Leave:
        done = leaveEvent(event);
        break;

    case QEvent::Show:
        done = showEvent((QShowEvent*) event);
        break;

    case QEvent::Hide:
        done = hideEvent((QHideEvent*) event);
        break;

    case QEvent::Paint:
        // At this point the event has to be delivered, regardless
        // whether the widget isVisible() or not because it
        // already went through the filters
        done = paintEvent((QPaintEvent*)event);
        break;

    case QEvent::Move:
        done = moveEvent((QMoveEvent*)event);
        break;

    case QEvent::Resize:
        done = resizeEvent((QResizeEvent*)event);
        break;

    case QEvent::Close:
        done = closeEvent((QCloseEvent *)event);
        break;

		/* RK Das überlassen wir weiterhin dem QWidget, da wir ja Gui::Menu haben
    case QEvent::ContextMenu:
        switch (data->context_menu_policy) {
        case Qt::DefaultContextMenu:
            contextMenuEvent(static_cast<QContextMenuEvent *>(event));
            break;
        case Qt::CustomContextMenu:
            emit customContextMenuRequested(static_cast<QContextMenuEvent *>(event)->pos());
            break;
        case Qt::ActionsContextMenu:
            if (d->actions.count()) {
                QMenu::exec(d->actions, static_cast<QContextMenuEvent *>(event)->globalPos());
                break;
            }
            // fall through
        default:
            event->ignore();
            break;
        }
        break;
		*/

    case QEvent::Drop:
        done = dropEvent((QDropEvent*) event);
        break;

    case QEvent::DragEnter:
        done = dragEnterEvent((QDragEnterEvent*) event);
        break;

    case QEvent::DragMove:
        done = dragMoveEvent((QDragMoveEvent*) event);
        break;

    case QEvent::DragLeave:
        done = dragLeaveEvent((QDragLeaveEvent*) event);
        break;

    case QEvent::ToolBarChange:
    case QEvent::ActivationChange:
    case QEvent::EnabledChange:
    case QEvent::FontChange:
    case QEvent::StyleChange:
    case QEvent::PaletteChange:
    case QEvent::WindowTitleChange:
    case QEvent::IconTextChange:
    case QEvent::ModifiedChange:
    case QEvent::MouseTrackingChange:
    case QEvent::ParentChange:
    case QEvent::WindowStateChange:
    case QEvent::LanguageChange:
        done = changeEvent( event);
        break;

	case QEvent::Timer:
		done = tickEvent( (QTimerEvent *)event);
		break;
    default:
        return QObject::eventFilter(watched, event);
    }
    return done;
}
