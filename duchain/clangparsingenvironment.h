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

#ifndef CLANGPARSINGENVIRONMENT_H
#define CLANGPARSINGENVIRONMENT_H

#include <util/path.h>
#include <language/duchain/parsingenvironment.h>

#include "duchainexport.h"

class KDEVCLANGDUCHAIN_EXPORT ClangParsingEnvironment : public KDevelop::ParsingEnvironment
{
public:
    virtual ~ClangParsingEnvironment() = default;
    virtual int type() const override;

    void addIncludes(const KDevelop::Path::List& includes);
    KDevelop::Path::List includes() const;

    void addDefines(const QHash<QString, QString>& defines);
    QHash<QString, QString> defines() const;

    void setPchInclude(const KDevelop::Path& path);
    KDevelop::Path pchInclude() const;

    void setProjectKnown(bool known);
    bool projectKnown() const;

    /**
     * Hash all contents of this environment and return the result.
     *
     * This is useful for a quick comparison, and enough to store on-disk
     * to figure out if the environment changed or not.
     */
    uint hash() const;

    bool operator==(const ClangParsingEnvironment& other) const;
    bool operator!=(const ClangParsingEnvironment& other) const
    {
        return !(*this == other);
    }

private:
    KDevelop::Path::List m_includes;
    QHash<QString, QString> m_defines;
    KDevelop::Path m_pchInclude;
    bool m_projectKnown = false;
};

#endif // CLANGPARSINGENVIRONMENT_H