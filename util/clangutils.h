/*
 * Copyright 2014 Kevin Funk <kfunk@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef CLANGUTILS_H
#define CLANGUTILS_H

#include <project/path.h>

namespace ClangUtils
{
    /**
     * Returns the default include directories for Clang
     *
     * Something along:
     * /usr/bin/../lib/clang/3.4/include
     * ...
     * /usr/include
     *
     * @note This function internally runs "clang++" to retrieve the include directories.
     * The result of the first run is cached, hence consecutive calls to this function are very cheap.
     */
    KDevelop::Path::List defaultIncludeDirectories();
};

#endif // CLANGUTILS_H