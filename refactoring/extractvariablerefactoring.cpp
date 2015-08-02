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

// C++
#include <queue>

// Qt
#include <QInputDialog>

// KF5
#include <KLocalizedString>

// Clang
#include <clang/AST/Expr.h>
#include <clang/Tooling/Core/Replacement.h>

#include "extractvariablerefactoring.h"
#include "utils.h"

using namespace std;
using namespace clang;
using clang::tooling::Replacement;
using clang::tooling::Replacements;

ExtractVariableRefactoring::ExtractVariableRefactoring(const clang::Expr *expr,
                                                       clang::ASTContext *astContext,
                                                       clang::SourceManager *sourceManager)
    : Refactoring(nullptr)
{
    const Stmt *nodeInCpndStmt = nullptr;
    using DynTypedNode = ast_type_traits::DynTypedNode;
    // Find closest CompoundStmt (BFS, getParents() as edges)
    queue<tuple<DynTypedNode, DynTypedNode >> queue; // {parent, node}
    for (auto p : astContext->getParents(*expr)) {
        queue.emplace(p, DynTypedNode::create(*expr));
    }
    while (!queue.empty()) {
        auto n = queue.front();
        queue.pop();
        if (get<0>(n).get<CompoundStmt>()) {
            nodeInCpndStmt = get<1>(n).get<Stmt>();
            Q_ASSERT(nodeInCpndStmt);
            decltype(queue)().swap(queue);  // break
        } else {
            for (auto p : astContext->getParents(get<0>(n))) {
                queue.emplace(p, get<0>(n));
            }
        }
    }
    if (nodeInCpndStmt) {
        // Usable
        {
            Replacement pattern(*sourceManager, expr, "");
            m_filenameExpression = pattern.getFilePath();
            m_offsetExpression = pattern.getOffset();
            m_lengthExpression = pattern.getLength();
            m_expression = codeFromASTNode(expr, *sourceManager, astContext->getLangOpts());
        }
        {
            Replacement pattern(*sourceManager, nodeInCpndStmt->getLocStart(), 0, "");
            m_filenameVariablePlacement = pattern.getFilePath();
            m_offsetVariablePlacement = pattern.getOffset();
            m_variableType = expr->getType().getAsString(); // qualification may be necessary
        }
    }
}

llvm::ErrorOr<clang::tooling::Replacements> ExtractVariableRefactoring::invoke(
    RefactoringContext *ctx)
{
    // This is local refactoring, all context dependent operations done in RefactoringManager
    Q_UNUSED(ctx);

    QString varName = QInputDialog::getText(nullptr, i18n("Variable name"),
                                            i18n("Type name of new variable"));
    if (varName.isEmpty()) {
        return cancelledResult();
    }
    string name = varName.toStdString();
    return Refactorings::ExtractVariable::run(m_filenameExpression, m_expression,
                                              m_filenameVariablePlacement,
                                              m_variableType, name, m_offsetExpression,
                                              m_lengthExpression, m_offsetVariablePlacement);
}

namespace Refactorings
{
namespace ExtractVariable
{
clang::tooling::Replacements run(const std::string &filenameExpression,
                                 const std::string &expression,
                                 const std::string &filenameVariablePlacement,
                                 const std::string &variableType, const std::string &variableName,
                                 const unsigned offsetExpression,
                                 const unsigned lengthExpression,
                                 const unsigned offsetVariablePlacement)
{
    Replacements result;
    result.insert(
        Replacement(filenameExpression, offsetExpression, lengthExpression, variableName));
    string varDef = variableType + " " + variableName + " = " + expression + ";\n";
    result.insert(Replacement(filenameVariablePlacement, offsetVariablePlacement, 0, varDef));
    return result;
}
}
}

QString ExtractVariableRefactoring::name() const
{
    return i18n("Extract variable");
}
