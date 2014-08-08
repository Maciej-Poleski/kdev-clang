/*
 * This file is part of KDevelop
 *
 * Copyright 2014 Milian Wolff <mail@milianw.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "clangparsingenvironment.h"

using namespace KDevelop;

int ClangParsingEnvironment::type() const
{
    return CppParsingEnvironment;
}

void ClangParsingEnvironment::addIncludes(const Path::List& includes)
{
    m_includes += includes;
}

Path::List ClangParsingEnvironment::includes() const
{
    return m_includes;
}

void ClangParsingEnvironment::addDefines(const QHash<QString, QString>& defines)
{
    m_defines.unite(defines);
}

QHash<QString, QString> ClangParsingEnvironment::defines() const
{
    return m_defines;
}

void ClangParsingEnvironment::setPchInclude(const Path& path)
{
    m_pchInclude = path;
}

Path ClangParsingEnvironment::pchInclude() const
{
    return m_pchInclude;
}

void ClangParsingEnvironment::setProjectKnown(bool known)
{
    m_projectKnown = known;
}

bool ClangParsingEnvironment::projectKnown() const
{
    return m_projectKnown;
}

uint ClangParsingEnvironment::hash() const
{
    KDevHash hash;
    hash << m_defines.size();
    for (auto it = m_defines.constBegin(); it != m_defines.constEnd(); ++it) {
        hash << qHash(it.key()) << qHash(it.value());
    }
    hash << m_includes.size();
    for (const auto& include : m_includes) {
        hash << qHash(include);
    }
    hash << qHash(m_pchInclude);
    return hash;
}

bool ClangParsingEnvironment::operator==(const ClangParsingEnvironment& other) const
{
    return m_defines == other.m_defines
        && m_includes == other.m_includes
        && m_pchInclude == other.m_pchInclude
        && m_projectKnown == other.m_projectKnown;
}