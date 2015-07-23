/*
    This file is part of KDevelop

    Copyright 2015 Maciej Poleski <d82ks8djf82msd83hf8sc02lqb5gh5@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

// Qt
#include <QAction>
#include <QMenu>

// KF5
#include <KF5/KI18n/klocalizedstring.h>

// KDevelop
#include <interfaces/contextmenuextension.h>
#include <language/interfaces/editorcontext.h>

#include "contextmenumutator.h"
#include "refactoringmanager.h"
#include "kdevrefactorings.h"
#include "debug.h"

using namespace KDevelop;

ContextMenuMutator::ContextMenuMutator(ContextMenuExtension &extension, EditorContext *context,
                                       RefactoringManager *parent)
    : QObject(parent)
      , m_placeholder(new QAction(i18n("preparing list..."), this))
{
    extension.addAction(ContextMenuExtension::RefactorGroup, m_placeholder);
}

RefactoringManager *ContextMenuMutator::parent()
{
    return static_cast<RefactoringManager *>(QObject::parent());
}

void ContextMenuMutator::endFillingContextMenu(const QVector<Refactoring *> &refactorings)
{
//    for (QWidget *w : m_placeholder->associatedWidgets()) {
//        refactorDebug() << "widget:";
//        for (QObject *o = w; o; o = o->parent()) {
//            refactorDebug() << o->metaObject()->className() << " " << o->objectName();
//        }
//    }
    QMenu *menu = dynamic_cast<QMenu *>(m_placeholder->associatedWidgets()[0]);
    // FIXME: menu may already be closed - handle that
    Q_ASSERT(menu); // TODO: improve that (distinguish options)
    auto refactoringContext = parent()->parent()->refactoringContext();
    for (auto refactorAction : refactorings) {
        QAction *action = new QAction(refactorAction->name(), menu);
        refactorAction->setParent(action);  // delete as necessary
        connect(action, &QAction::triggered, [this, refactoringContext, refactorAction]()
        {
            // TODO: don't use refactorThis
            auto changes = Refactorings::refactorThis(refactoringContext, refactorAction,
                                                      {}, {});
            // FIXME:
            // use background thread
            // ... provided with ability to show GUI
            // show busy indicator

            changes.applyAllChanges();
        });

        menu->addAction(action);
    }
    // TODO: heuristic to detect submenu
    deleteLater();
}