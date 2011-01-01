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

#include "qmps/trace_browser.h"

#include <cctype>
#include <boost/bind.hpp>

#include <QtEndian>
#include <QGridLayout>
#include <QSplitter>
#include <QHBoxLayout>
#include <QListView>
#include <QLabel>
#include <QComboBox>
#include <QPalette>
#include <QStackedWidget>

#include "base/lang.h"
#include "umps/arch.h"
#include "qmps/application.h"
#include "qmps/debug_session.h"
#include "qmps/hex_view.h"
#include "qmps/trace_browser_priv.h"

TraceBrowser::TraceBrowser(QWidget* parent)
    : QWidget(parent),
      dbgSession(Appl()->getDebugSession())
{
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    delegateFactory.push_back(ViewDelegateType("Hex Dump (Big-endian)",
                                               boost::bind(&TraceBrowser::createHexView,
                                                           _1, _2, false)));
#endif
    delegateFactory.push_back(ViewDelegateType("Hex Dump",
                                               boost::bind(&TraceBrowser::createHexView,
                                                           _1, _2, true)));
    delegateFactory.push_back(ViewDelegateType("ASCII",
                                               boost::bind(&AsciiView::Create, _1, _2)));

    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setColumnStretch(0, 1);
    layout->setVerticalSpacing(4);

    QLabel* heading = new QLabel("<b>Traced Regions</b>");
    heading->setIndent(2);
    layout->addWidget(heading, 0, 0);

    layout->addWidget(new QLabel("Display:"), 0, 1);
    delegateTypeCombo = new QComboBox;
    foreach (const ViewDelegateType& dt, delegateFactory)
        delegateTypeCombo->addItem(dt.name);
    layout->addWidget(delegateTypeCombo, 0, 2);
    connect(delegateTypeCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onDelegateTypeChanged(int)));
    delegateTypeCombo->setEnabled(false);

    tplView = new QListView;

    splitter = new QSplitter;
    splitter->addWidget(tplView);

    layout->addWidget(splitter, 1, 0, 1, 3);

    placeholder = new QLabel(
        QString("<i>No trace region selected "
                "(use <b>Debug %1 Add Traced Region</b> to insert one)</i>").arg(QChar(0x2192)));
    placeholder->setWordWrap(true);
    placeholder->setFrameStyle(QFrame::StyledPanel);
    placeholder->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    viewStack = new QStackedWidget;
    viewStack->addWidget(placeholder);

    splitter->addWidget(viewStack);

    connect(dbgSession, SIGNAL(MachineStarted()), this, SLOT(onMachineStarted()));
    connect(dbgSession, SIGNAL(MachineAboutToBeHalted()), this, SLOT(onMachineAboutToBeHalted()));
    connect(dbgSession, SIGNAL(DebugIterationCompleted()), this, SLOT(refreshView()));
}

TraceBrowser::~TraceBrowser() {}

void TraceBrowser::AddTracepoint(Word start, Word end)
{
    assert(tplModel);
    tplModel->Add(AddressRange(MachineConfig::MAX_ASID, start, end), AM_WRITE);
}

void TraceBrowser::onMachineStarted()
{
    tplModel.reset(new TracepointListModel(dbgSession->getTracepoints()));
    tplView->setModel(tplModel.get());

    connect(tplView->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this,
            SLOT(onSelectionChanged(const QItemSelection&)));

    connect(tplModel.get(), SIGNAL(rowsInserted(const QModelIndex&, int, int)),
            this, SLOT(onTracepointAdded()));

    if (tplModel->rowCount() > 0)
        tplView->setCurrentIndex(tplModel->index(0, 0));
}

void TraceBrowser::onMachineAboutToBeHalted()
{
    for (ViewDelegateMap::iterator it = viewMap.begin(); it != viewMap.end(); ++it)
        delete it->second.widget;
    viewMap.clear();
}

void TraceBrowser::onSelectionChanged(const QItemSelection& selected)
{
    QModelIndexList indexes = selected.indexes();
    if (!indexes.isEmpty()) {
        Stoppoint* sp = dbgSession->getTracepoints()->Get(indexes[0].row());
        ViewDelegateMap::iterator it = viewMap.find(sp->getId());
        if (it == viewMap.end()) {
            const AddressRange& r = sp->getRange();
            ViewDelegateInfo info;
            info.type = kDefaultViewDelegate;
            info.widget = delegateFactory[kDefaultViewDelegate].ctor(r.getStart(), r.getEnd());
            viewMap[sp->getId()] = info;
            viewStack->setCurrentIndex(viewStack->addWidget(info.widget));
            delegateTypeCombo->setCurrentIndex(kDefaultViewDelegate);
        } else {
            viewStack->setCurrentWidget(it->second.widget);
            delegateTypeCombo->setCurrentIndex(it->second.type);
        }
        delegateTypeCombo->setEnabled(true);
    } else {
        delegateTypeCombo->setEnabled(false);
    }
}

void TraceBrowser::refreshView()
{
    QModelIndexList indexes = tplView->selectionModel()->selectedIndexes();
    if (indexes.empty())
        return;

    int row = indexes.first().row();
    if (tplModel->IsDirty(row)) {
        tplModel->ClearDirty(row);
        MemoryViewDelegate* delegate = dynamic_cast<MemoryViewDelegate*>(viewStack->currentWidget());
        if (delegate)
            delegate->Refresh();
    }
}

void TraceBrowser::onDelegateTypeChanged(int index)
{
    Stoppoint* sp = selectedTracepoint();
    assert(sp != NULL);

    const AddressRange& r = sp->getRange();
    delete viewMap[sp->getId()].widget;
    viewMap[sp->getId()].widget = delegateFactory[index].ctor(r.getStart(), r.getEnd());
    viewMap[sp->getId()].type = index;

    viewStack->setCurrentIndex(viewStack->addWidget(viewMap[sp->getId()].widget));
}

Stoppoint* TraceBrowser::selectedTracepoint() const
{
    QModelIndexList indexes = tplView->selectionModel()->selectedIndexes();
    if (indexes.empty())
        return NULL;
    return dbgSession->getTracepoints()->Get(indexes[0].row());
}

void TraceBrowser::onTracepointAdded()
{
    if (tplView->selectionModel()->selectedIndexes().empty())
        tplView->setCurrentIndex(tplModel->index(0, 0));
}

QWidget* TraceBrowser::createHexView(Word start, Word end, bool nativeOrder)
{
    HexView* hexView = new HexView(start, end);
    hexView->setReversedByteOrder(!nativeOrder);
    return hexView;
}


TracepointListModel::TracepointListModel(StoppointSet* spSet, QObject* parent)
    : BaseStoppointListModel(spSet, parent),
      dirtySet(spSet->Size(), false)
{
    //onHitConnection = spSet->SignalHit.connect(sigc::mem_fun(*this, &TracepointListModel::onHit));
    //
    RegisterSigc(spSet->SignalHit.connect(sigc::mem_fun(*this, &TracepointListModel::onHit)));
}

QVariant TracepointListModel::headerData(int section,
                                         Qt::Orientation orientation,
                                         int role) const
{
    return QVariant();
}

QVariant TracepointListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.column() > 0 || index.row() >= stoppoints->Size())
        return QVariant();

    if (role == Qt::DisplayRole) {
        return getAddressRange(index.row());
    } else if (role == Qt::FontRole) {
        QFont font;
        font.setBold(dirtySet[index.row()]);
        return font;
    } else {
        return QVariant();
    }
}

void TracepointListModel::ClearDirty(int row)
{
    dirtySet[row] = false;
    QModelIndex idx = index(row, 0);
    Q_EMIT dataChanged(idx, idx);
}

void TracepointListModel::StoppointAdded()
{
    dirtySet.push_back(false);
}

void TracepointListModel::StoppointRemoved(int index)
{
    dirtySet.erase(dirtySet.begin() + index);
}

void TracepointListModel::onHit(size_t spIndex,
                                const Stoppoint* stoppoint,
                                Word addr,
                                const Processor* cpu)
{
    UNUSED_ARG(stoppoint);
    UNUSED_ARG(addr);
    UNUSED_ARG(cpu);
    dirtySet[spIndex] = true;
    QModelIndex idx = index(spIndex, 0);
    Q_EMIT dataChanged(idx, idx);
}


AsciiView::AsciiView(Word start, Word end)
    : QPlainTextEdit(),
      start(start),
      end(end)
{
    setFont(Appl()->getMonospaceFont());
    setWordWrapMode(QTextOption::WrapAnywhere);
    setReadOnly(true);

    Refresh();
}

AsciiView* AsciiView::Create(Word start, Word end)
{
    return new AsciiView(start, end);
}

void AsciiView::Refresh()
{
    Machine* machine = debugSession->getMachine();

    QString buffer;
    buffer.reserve((((end - start) >> 2) + 1) * WS);

    Word val;
    char bytes[WS];
    for (Word addr = start; addr <= end; addr += WS) {
        machine->ReadMemory(addr, &val);
        qToLittleEndian(val, (unsigned char *) bytes);
        for (unsigned int i = 0; i < WS; i++) {
            if (isprint(bytes[i]))
                buffer += bytes[i];
            else
                buffer += QChar(kUnicodeReplacementChar);
        }
    }

    setPlainText(buffer);
}
