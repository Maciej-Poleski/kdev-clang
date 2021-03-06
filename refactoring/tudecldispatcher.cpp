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

// Clang
#include <clang/AST/Decl.h>

#include "tudecldispatcher.h"
#include "declarationcomparator.h"
#include "utils.h"

TUDeclDispatcher::TUDeclDispatcher(const DeclarationComparator *declComparator)
    : m_declComparator(declComparator)
{
}

bool TUDeclDispatcher::equivalent(const clang::Decl *decl) const
{
    if (!decl) {
        return false;
    }
    decl = decl->getCanonicalDecl();
    auto location = lexicalLocation(decl);
    auto i = m_cache.find(location);
    if (i != m_cache.end()) {
        return i->second;
    } else {
        bool result = m_declComparator->equivalentTo(decl);
        m_cache[location] = result;
        return result;
    }
}

bool TUDeclDispatcher::equivalentImpl(const clang::DeclContext *declContext) const
{
    if (!declContext) {
        return false;
    }
    auto asDecl = llvm::dyn_cast<clang::Decl>(declContext);
    Q_ASSERT(asDecl != nullptr);
    // every (instance) of DeclContext should be a Decl
    // http://clang.llvm.org/doxygen/classclang_1_1DeclContext.html#details
    return equivalent(asDecl);
}
