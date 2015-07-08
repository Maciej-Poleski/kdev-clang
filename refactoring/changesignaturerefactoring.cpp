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
#include <clang/AST/DeclCXX.h>

// KF5
#include <KF5/KI18n/klocalizedstring.h>

#include "changesignaturerefactoring.h"
#include "refactoringcontext.h"
#include "documentcache.h"
#include "changesignaturedialog.h"
#include "changesignaturerefactoringinfopack.h"
#include "declarationcomparator.h"
#include "debug.h"

using namespace clang;

ChangeSignatureRefactoring::ChangeSignatureRefactoring(const FunctionDecl *functionDecl)
    : Refactoring(nullptr), m_infoPack(InfoPack::fromFunctionDecl(functionDecl))
{
    functionDecl->dump();
}

ChangeSignatureRefactoring::~ChangeSignatureRefactoring() = default;

llvm::ErrorOr<clang::tooling::Replacements> ChangeSignatureRefactoring::invoke(
    RefactoringContext *ctx)
{
    std::unique_ptr<ChangeSignatureDialog> dialog(
        new ChangeSignatureDialog(m_infoPack.get(), nullptr));
    if (dialog->exec() == 0) {
        return cancelledResult();
    }
    return cancelledResult();
}

QString ChangeSignatureRefactoring::name() const
{
    return i18n("Change signature");
}
