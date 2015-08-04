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
#include <unordered_map>

// Qt
#include <QInputDialog>

// KF5
#include <KLocalizedString>

// Clang
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include "extractfunctionrefactoring.h"
#include "utils.h"

using namespace std;
using namespace clang;
using namespace clang::tooling;

// place definition above definition
// place declaration above declaration
// inherit nested name spec

// replacement on replacement

namespace
{
// Collects all uses of declarations from given DeclContext
class UsesFromDeclContext : public RecursiveASTVisitor<UsesFromDeclContext>
{
public:
    UsesFromDeclContext(const DeclContext *context);

    bool VisitDeclRefExpr(const DeclRefExpr *declRef);

    const DeclContext *const context;
    unordered_map<const ValueDecl *, const DeclRefExpr *> usedDecls; // These will need to be passed explicitly
};

}

// NOTE: It may be desirable to adjust accessibility of generated member (if the generated is a member)
ExtractFunctionRefactoring::ExtractFunctionRefactoring(const clang::Expr *expr,
                                                       clang::ASTContext *astContext,
                                                       clang::SourceManager *sourceManager)
    : Refactoring(nullptr)
{
    const FunctionDecl *declContext = nullptr;
    using DynTypedNode = ast_type_traits::DynTypedNode;
    // Find closest DeclContext (BFS, getParents() as edges)
    queue<DynTypedNode> queue; // {parent, node}
    for (auto p : astContext->getParents(*expr)) {
        queue.push(p);
    }
    while (!queue.empty()) {
        auto front = queue.front();
        queue.pop();
        // DeclContext below doesn't work. why?
        if ((declContext = front.get<FunctionDecl>())) {
            decltype(queue)().swap(queue);  // break
        } else {
            for (auto p : astContext->getParents(front)) {
                queue.push(p);
            }
        }
    }
    if (declContext) {
        UsesFromDeclContext visitor(declContext);
        // matchers provide const nodes, visitors consume non-const
        visitor.TraverseStmt(const_cast<Expr *>(expr));
        string arguments = "(";
        string invocation = "(";
        for (auto entry : visitor.usedDecls) {
            arguments += entry.second->getType().getAsString() + " " + entry.first->getName().str();
            arguments += ", ";
            invocation += codeFromASTNode(entry.second, *sourceManager, astContext->getLangOpts());
            invocation += ", ";
        }
        if (arguments.length() > 1) {
            arguments = arguments.substr(0, arguments.length() - 2);
        }
        arguments += ")";
        if (invocation.length() > 1) {
            invocation = invocation.substr(0, invocation.length() - 2);
        }
        invocation += ")";
        {
            Replacement pattern(*sourceManager, expr, "");
            m_tasks.emplace_back(
                pattern.getFilePath(), pattern.getOffset(), pattern.getLength(),
                [invocation](const string &name)
                {
                    return name + invocation;
                });
        }
        string returnType = expr->getType().getAsString();
        for (const FunctionDecl *decl : declContext->redecls()) {
            Replacement pattern(*sourceManager, decl->getLocStart(), 0, "");
            std::function<string(const string &)> replacement;
            // FIXME: nested name specifier!!!!
            if (decl->isThisDeclarationADefinition()) {
                // emit full definition
                string exprString = codeFromASTNode(expr, *sourceManager,
                                                    astContext->getLangOpts());
                replacement = [returnType, exprString, arguments](const string &name)
                {
                    string result = returnType + " " + name + arguments + "\n";
                    result += "{\n";
                    result += "\treturn " + exprString + ";\n";
                    result += "}\n\n";
                    return result;
                };
            } else {
                // emit only forward declaration
                replacement = [returnType, arguments](const string &name)
                {
                    return returnType + " " + name + arguments + ";\n";
                };
            }
            m_tasks.emplace_back(pattern.getFilePath(), pattern.getOffset(), 0, replacement);
        }
    }
}

llvm::ErrorOr<clang::tooling::Replacements> ExtractFunctionRefactoring::invoke(
    RefactoringContext *ctx)
{
    // This is local refactoring, all context dependent operations done in RefactoringManager
    Q_UNUSED(ctx);
    if (m_tasks.empty()) {
        return cancelledResult(); // TODO: error
    }

    QString funName = QInputDialog::getText(nullptr, i18n("Function Name"),
                                            i18n("Type name of new function"));
    if (funName.isEmpty()) {
        return cancelledResult();
    }
    string name = funName.toStdString();
    Replacements result;
    for (const Task &task : m_tasks) {
        result.insert(Replacement(task.filename, task.offset, task.length, task.replacement(name)));
    }
    return result;
}

QString ExtractFunctionRefactoring::name() const
{
    return i18n("Extract function");
}

UsesFromDeclContext::UsesFromDeclContext(const DeclContext *context)
    : context(context)
{
}

bool UsesFromDeclContext::VisitDeclRefExpr(const DeclRefExpr *declRef)
{
    auto ctx = declRef->getDecl()->getDeclContext();
    while (ctx && ctx != context) {
        ctx = ctx->getParent();
    }
    if (ctx) {
        // infer type _now_ (not to have 'auto' type specifier in generated function declaration)
        usedDecls[declRef->getDecl()] = declRef;
    }
    return true;
}

ExtractFunctionRefactoring::Task::Task(const std::string &filename, unsigned offset,
                                       unsigned length,
                                       const std::function<std::string(
                                           const std::string &)> &replacement)
    : filename(filename)
    , offset(offset)
    , length(length)
    , replacement(replacement)
{
}
