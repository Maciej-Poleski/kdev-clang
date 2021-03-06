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

#ifndef KDEV_CLANG_QACTIONWATCHER_H
#define KDEV_CLANG_QACTIONWATCHER_H

#include <QObject>

class QAction;

/**
 * Watches widgets associated to given @c QAction and deletes it when last widget is destroyed.
 *
 * NOTE: If @C QAction is removed from last widget (no destruction), it will be leaked.
 */
class ActionWatcher : public QObject
{
    Q_OBJECT;
    Q_DISABLE_COPY(ActionWatcher);
public:
    ActionWatcher(QAction *action);

private:
    QAction *parent();
    void decreaseCount();
    void registerAssociatedWidgets();

private:
    int m_count;
};


#endif //KDEV_CLANG_QACTIONWATCHER_H
