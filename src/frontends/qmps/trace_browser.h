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

#ifndef QMPS_TRACE_BROWSER_H
#define QMPS_TRACE_BROWSER_H

#include <boost/function.hpp>

#include <QWidget>
#include <QPointer>

#include "base/lang.h"
#include "umps/types.h"
#include "qmps/stoppoint_list_model.h"
#include "qmps/hex_view.h"

class QAction;
class QListView;
class DebugSession;
class QItemSelection;
class QComboBox;
class QSplitter;
class QStackedWidget;
class TracepointListModel;

class TraceBrowser : public QWidget {
    Q_OBJECT

public:
    TraceBrowser(QAction* insertAction, QAction* removeAction, QWidget* parent = 0);
    ~TraceBrowser();

    bool AddTracepoint(Word start, Word end);

private Q_SLOTS:
    void onMachineStarted();
    void onMachineHalted();

    void onTracepointAdded();
    void removeTracepoint();
    void onSelectionChanged(const QItemSelection&);
    void onDelegateTypeChanged(int index);
    void refreshView();

private:
    static const int kDefaultViewDelegate = 0;

    struct ViewDelegateInfo {
        int type;
        QPointer<QWidget> widget;
    };

    typedef boost::function<QWidget* (Word, Word)> DelegateFactoryFunc;

    struct ViewDelegateType {
        ViewDelegateType(const char* name, DelegateFactoryFunc func)
            : name(name), ctor(func) {}
        const char* name;
        DelegateFactoryFunc ctor;
    };

    Stoppoint* selectedTracepoint() const;

    static QWidget* createHexView(Word start, Word end, bool nativeOrder);

    DebugSession* const dbgSession;

    scoped_ptr<TracepointListModel> tplModel;

    QComboBox* delegateTypeCombo;
    QSplitter* splitter;
    QListView* tplView;
    QStackedWidget* viewStack;

    typedef std::map<unsigned int, ViewDelegateInfo> ViewDelegateMap;
    ViewDelegateMap viewMap;

    std::vector<ViewDelegateType> delegateFactory;
};

#endif // QMPS_TRACE_BROWSER_H
