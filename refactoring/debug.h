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

#ifndef KDEV_CLANG_DEBUG_H
#define KDEV_CLANG_DEBUG_H

#include <QtCore/QLoggingCategory>

#include <llvm/ADT/StringRef.h>

Q_DECLARE_LOGGING_CATEGORY(KDEV_CLANG_REFACTORING)
#define refactorDebug() qCDebug(KDEV_CLANG_REFACTORING)
#define refactorWarning() qCWarning(KDEV_CLANG_REFACTORING)
#define refactorCritical() qCCritical(KDEV_CLANG_REFACTORING)

QDebug operator<<(QDebug dbg, const std::string &string);
QDebug operator<<(QDebug dbg, llvm::StringRef string);

#endif //KDEV_CLANG_DEBUG_H
