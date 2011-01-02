/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * uMPS - A general purpose computer system simulator
 *
 * Copyright (C) 2010 Tomislav Jonjic
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "qmps/hex_view.h"
#include <QtDebug>

#include <QTextBlock>
#include <QPainter>

#include "umps/types.h"
#include "umps/arch.h"
#include "umps/machine.h"

#include "qmps/application.h"
#include "qmps/debug_session.h"
#include "qmps/hex_view_priv.h"
#include "qmps/ui_utils.h"

HexView::HexView(Word start, Word end, QWidget* parent)
    : QPlainTextEdit(parent),
      start(start),
      end(end),
      margin(new HexViewMargin(this))
{
    QFont font = Appl()->getMonospaceFont();
    setFont(font);

    setViewportMargins(margin->sizeHint().width(), 0, 0, 0);
    connect(this, SIGNAL(updateRequest(QRect, int)), this, SLOT(updateMargin(QRect, int)));

    setCursorWidth(fontMetrics().width(QLatin1Char('o')));
    setLineWrapMode(NoWrap);
    setOverwriteMode(true);
    setTabChangesFocus(true);
    setContextMenuPolicy(Qt::NoContextMenu);
    setUndoRedoEnabled(false);
    viewport()->setCursor(Qt::ArrowCursor);

    Refresh();
    QTextCursor cursor = textCursor();
    cursor.setPosition(4);
    setTextCursor(cursor);

    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightWord()));
    highlightWord();
}

void HexView::setReversedByteOrder(bool setting)
{
    if (revByteOrder != setting) {
        revByteOrder = setting;
        Refresh();
    }
}

void HexView::Refresh()
{
    QString buf;

    bool first = true;

    Machine* m = Appl()->getDebugSession()->getMachine();

    for (Word addr = start; addr <= end; addr += WS) {
        unsigned int wi = (addr - start) >> 2;
        if (!first && (wi % kWordsPerRow == 0))
            buf += '\n';
        first = false;

        Word value;
        m->ReadMemory(addr, &value);

        for (unsigned int bi = 0; bi < WS; bi++) {
            if (bi > 0 || wi % kWordsPerRow != 0)
                buf += ' ';
            unsigned int byteVal;
            if (revByteOrder)
                byteVal = ((unsigned char *) &value)[WS - bi - 1];
            else
                byteVal = ((unsigned char *) &value)[bi];
            buf += QString("%1").arg(byteVal, 2, 16, QLatin1Char('0'));
        }
    }

    setPlainText(buf);
}

void HexView::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    margin->setGeometry(QRect(cr.left(), cr.top(), margin->sizeHint().width(), cr.height()));
}

bool HexView::canInsertFromMimeData(const QMimeData* source) const
{
    UNUSED_ARG(source);
    return false;
}

void HexView::insertFromMimeData(const QMimeData* source)
{
    UNUSED_ARG(source);
}

void HexView::keyPressEvent(QKeyEvent* event)
{
    unsigned int colType = currentNibble();

    // Ugly defensive hack, yes.
    if (colType == COL_SPACING) {
        moveCursor(QTextCursor::Start);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Left:
        if (colType == COL_LO_NIBBLE)
            moveCursor(QTextCursor::Left);
        else if (currentByte() != 0)
            moveCursor(QTextCursor::Left, 1 + kHorizontalSpacing);
        break;

    case Qt::Key_Right:
        if (colType == COL_HI_NIBBLE)
            moveCursor(QTextCursor::Right);
        else if (currentByte() != kWordsPerRow * WS - 1)
            moveCursor(QTextCursor::Right, 1 + kHorizontalSpacing);
        break;

    case Qt::Key_Up:
        moveCursor(QTextCursor::Up);
        break;

    case Qt::Key_Down:
        moveCursor(QTextCursor::Down);
        break;

    default:
        break;
    }
}

// FIXME: implement this
void HexView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;

    QTextCursor cursor = cursorForPosition(event->pos());
    qDebug() << "HexView::mousePressEvent()" << cursor.position();
}

void HexView::updateMargin(const QRect& rect, int dy)
{
    if (dy != 0)
        margin->scroll(0, dy);
    else
        margin->update(0, rect.y(), margin->width(), rect.height());
}

void HexView::highlightWord()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;

    QTextCursor cursor = textCursor();
    cursor.clearSelection();

    unsigned int bi = currentByte();
    unsigned int ni = currentNibble();

    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor,
                        (bi % WS) * N_COLS_PER_BYTE + ni);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
                        kCharsPerWord - 1);

    selection.cursor = cursor;
    selection.format.setBackground(palette().highlight());
    selection.format.setForeground(palette().highlightedText());

    extraSelections.append(selection);
    setExtraSelections(extraSelections);
}

inline unsigned int HexView::currentWord() const
{
    return textCursor().position() / kCharsPerWord;
}

inline unsigned int HexView::currentByte() const
{
    return (textCursor().position() / N_COLS_PER_BYTE) % (kWordsPerRow * WS);
}

inline unsigned int HexView::currentNibble() const
{
    return textCursor().position() % N_COLS_PER_BYTE;
}

void HexView::moveCursor(QTextCursor::MoveOperation operation, int n)
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(operation, QTextCursor::MoveAnchor, n);
    setTextCursor(cursor);
}

void HexView::paintMargin(QPaintEvent* event)
{
    QPainter painter(margin);
    const QRect& er = event->rect();

    painter.fillRect(er, palette().window().color());

    painter.setPen(palette().shadow().color());
    painter.drawLine(er.topRight(), er.bottomRight());

    painter.setPen(palette().windowText().color());

    QTextBlock block = firstVisibleBlock();
    if (!block.isValid())
        return;

    int y0 = (int) blockBoundingGeometry(block).translated(contentOffset()).y();
    int y1 = y0 + (int) blockBoundingRect(block).height();

    Word addr = start + block.blockNumber() * kWordsPerRow * WS;

    while (y0 <= er.bottom()) {
        if (er.top() <= y1) {
            painter.drawText(HexViewMargin::kLeftPadding, y0,
                             margin->width(), margin->fontMetrics().height(),
                             Qt::AlignLeft, FormatAddress(addr));
        }
        block = block.next();
        if (!block.isValid())
            break;
        y0 = y1;
        y1 += blockBoundingRect(block).height();
        addr += WS * kWordsPerRow;
    }
}

HexViewMargin::HexViewMargin(HexView* view)
    : QWidget(view),
      hexView(view)
{}

QSize HexViewMargin::sizeHint() const
{
    return QSize(kLeftPadding + fontMetrics().width("0xdead.beef") + kRightPadding, 0);
}

void HexViewMargin::paintEvent(QPaintEvent* event)
{
    hexView->paintMargin(event);
}

void HexViewMargin::wheelEvent(QWheelEvent* event)
{
    hexView->wheelEvent(event);
}
