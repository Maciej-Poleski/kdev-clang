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
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Tooling/Refactoring.h>

// KF5
#include <KF5/KI18n/klocalizedstring.h>

#include "encapsulatefieldrefactoring.h"
#include "encapsulatefielddialog.h"
#include "encapsulatefieldrefactoring_changepack.h"
#include "refactoringcontext.h"
#include "redeclarationchain.h"
#include "tudecldispatcher.h"
#include "utils.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace std;

namespace
{
class Translator : public MatchFinder::MatchCallback
{
    using ChangePack = EncapsulateFieldRefactoring::ChangePack;
public:
    Translator(Replacements &replacements, ChangePack *changePack,
               RedeclarationChain *declDispatcher);

    virtual void run(const MatchFinder::MatchResult &Result) override;

    void handleDeclRefExpr(const DeclRefExpr *declRefExpr, SourceManager *sourceManager,
                           const LangOptions &langOpts);
    void handleMemberExpr(const MemberExpr *memberExpr, SourceManager *sourceManager,
                          const LangOptions &langOpts);
    void handleAssignToDeclRefExpr(const BinaryOperator *assignmentOperator,
                                   SourceManager *sourceManager, const LangOptions &langOpts);
    void handleAssignToMemberExpr(const BinaryOperator *assignmentOperator,
                                  SourceManager *sourceManager, const LangOptions &langOpts);
    void handleAssignOperatorCall(const CXXOperatorCallExpr *operatorCallExpr,
                                  SourceManager *sourceManager, const LangOptions &langOpts);

private:
    Replacements &m_replacements;
    ChangePack *m_changePack;
    TUDeclDispatcher m_declDispatcher;
};
}

EncapsulateFieldRefactoring::EncapsulateFieldRefactoring(const DeclaratorDecl *decl)
    : Refactoring(nullptr)
      , m_changePack(ChangePack::fromDeclaratorDecl(decl))
      , m_declDispatcher(decl)
      , m_isStatic(!llvm::isa<FieldDecl>(decl))
{
    decl->dump();
    decl->getCanonicalDecl()->dump();
}

EncapsulateFieldRefactoring::~EncapsulateFieldRefactoring() = default;

llvm::ErrorOr<clang::tooling::Replacements> EncapsulateFieldRefactoring::invoke(
    RefactoringContext *ctx)
{
    unique_ptr<EncapsulateFieldDialog> dialog(new EncapsulateFieldDialog(m_changePack.get()));
    if (dialog->exec() == 0) {
        return cancelledResult();
    }

    auto pureDeclRefExpr = declRefExpr().bind("DeclRefExpr");   // static
    auto pureMemberExpr = memberExpr().bind("MemberExpr");      // instance

    auto assignDeclRefExpr = binaryOperator(hasOperatorName("="), hasLHS(declRefExpr()))
        .bind("AssignDeclRefExpr");
    auto assignMemberExpr = binaryOperator(hasOperatorName("="), hasLHS(memberExpr()))
        .bind("AssignMemberExpr");
    auto assignCXXOperatorCallExpr =
        operatorCallExpr(
            hasOverloadedOperatorName("="),
            argumentCountIs(2),
            hasArgument(0, anyOf(
                declRefExpr(),
                memberExpr()))).bind("AssignCXXOperatorCallExpr");

    auto nonAssignDeclRefExpr =
        declRefExpr(expr().bind("DeclRefExpr"),
                    unless(anyOf(
                        hasParent(
                            binaryOperator(hasOperatorName("="),
                                           hasLHS(equalsBoundNode("DeclRefExpr")))),
                        hasParent(
                            operatorCallExpr(hasOverloadedOperatorName("="),
                                             argumentCountIs(2),
                                             hasArgument(0, equalsBoundNode("DeclRefExpr")))))));
    auto nonAssignMemberExpr =
        memberExpr(expr().bind("MemberExpr"),
                   unless(anyOf(
                       hasParent(
                           binaryOperator(hasOperatorName("="),
                                          hasLHS(equalsBoundNode("MemberExpr")))),
                       hasParent(
                           operatorCallExpr(hasOverloadedOperatorName("="),
                                            argumentCountIs(2),
                                            hasArgument(0, equalsBoundNode("MemberExpr")))))));
    // Name clash above is by design
    // Note: It may be considered to support also CompoundAssignOperator
    // rewriting a+=b to setA(getA()+b), but such implementation would have to explicitly list
    // all operators

    // FIXME: handle also declaration

    auto &tool = ctx->cache->refactoringTool();
    MatchFinder finder;
    Translator callback(tool.getReplacements(), m_changePack.get(), &m_declDispatcher);
    // Choose appropriate set depending on static/instance and get/set
    if (m_changePack->createSetter()) {
        finder.addMatcher(nonAssignDeclRefExpr, &callback);
        finder.addMatcher(nonAssignMemberExpr, &callback);
        finder.addMatcher(assignDeclRefExpr, &callback);
        finder.addMatcher(assignMemberExpr, &callback);
        finder.addMatcher(assignCXXOperatorCallExpr, &callback);
    } else {
        finder.addMatcher(pureDeclRefExpr, &callback);
        finder.addMatcher(pureMemberExpr, &callback);
    }
    tool.run(newFrontendActionFactory(&finder).get());

    return tool.getReplacements();
}

QString EncapsulateFieldRefactoring::name() const
{
    return i18n("encapsulate");
}

Translator::Translator(Replacements &replacements, ChangePack *changePack,
                       RedeclarationChain *declDispatcher)
    : m_replacements(replacements)
      , m_changePack(changePack)
      , m_declDispatcher(declDispatcher)
{
}

void Translator::run(const MatchFinder::MatchResult &Result)
{
    // Name clash is used here
    auto declRef = Result.Nodes.getNodeAs<DeclRefExpr>("DeclRefExpr");
    if (declRef && m_declDispatcher.equivalent(declRef->getDecl())) {
        handleDeclRefExpr(declRef, Result.SourceManager, Result.Context->getLangOpts());
        return;
    }
    auto memberExpr = Result.Nodes.getNodeAs<MemberExpr>("MemberExpr");
    if (memberExpr && m_declDispatcher.equivalent(memberExpr->getMemberDecl())) {
        handleMemberExpr(memberExpr, Result.SourceManager, Result.Context->getLangOpts());
    }
    auto assignDeclRefExpr = Result.Nodes.getNodeAs<BinaryOperator>("AssignDeclRefExpr");
    if (assignDeclRefExpr && m_declDispatcher.equivalent(
        llvm::dyn_cast<DeclRefExpr>(assignDeclRefExpr->getLHS())->getDecl())) {
        handleAssignToDeclRefExpr(assignDeclRefExpr, Result.SourceManager,
                                  Result.Context->getLangOpts());
    }
    auto assignMemberExpr = Result.Nodes.getNodeAs<BinaryOperator>("AssignMemberExpr");
    if (assignMemberExpr && m_declDispatcher.equivalent(
        llvm::dyn_cast<MemberExpr>(assignMemberExpr->getLHS())->getMemberDecl())) {
        handleAssignToMemberExpr(assignMemberExpr, Result.SourceManager,
                                 Result.Context->getLangOpts());
    }
    auto assignOperatorCall = Result.Nodes.getNodeAs<CXXOperatorCallExpr>(
        "AssignCXXOperatorCallExpr");
    if (assignOperatorCall) {
        auto assignee = assignOperatorCall->getArg(0);
        const Decl *decl;
        if (llvm::isa<MemberExpr>(assignee)) {
            decl = llvm::dyn_cast<MemberExpr>(assignee)->getMemberDecl();
        } else {
            Q_ASSERT(llvm::isa<DeclRefExpr>(assignee));
            decl = llvm::dyn_cast<DeclRefExpr>(assignee)->getDecl();
        }
        if (m_declDispatcher.equivalent(decl)) {
            handleAssignOperatorCall(assignOperatorCall, Result.SourceManager,
                                     Result.Context->getLangOpts());
        }
    }
}

void Translator::handleDeclRefExpr(const DeclRefExpr *declRefExpr, SourceManager *sourceManager,
                                   const LangOptions &langOpts)
{
    m_replacements.insert(
        Replacement(*sourceManager, CharSourceRange::getTokenRange(declRefExpr->getLocation()),
                    m_changePack->getterName() + "()"));
    // TODO Clang 3.7: use @p langOpts above
}

void Translator::handleMemberExpr(const MemberExpr *memberExpr, SourceManager *sourceManager,
                                  const LangOptions &langOpts)
{
    m_replacements.insert(
        Replacement(*sourceManager, CharSourceRange::getTokenRange(memberExpr->getMemberLoc()),
                    m_changePack->getterName() + "()"));
    // TODO Clang 3.7: use @p langOpts above
}

void Translator::handleAssignToDeclRefExpr(const BinaryOperator *assignmentOperator,
                                           SourceManager *sourceManager,
                                           const LangOptions &langOpts)
{
    m_replacements.insert(
        Replacement(*sourceManager,
                    CharSourceRange::getTokenRange(
                        llvm::dyn_cast<DeclRefExpr>(assignmentOperator->getLHS())->getLocation(),
                        assignmentOperator->getLocEnd()),
                    m_changePack->setterName() + "(" +
                    codeFromASTNode(assignmentOperator->getRHS(), *sourceManager, langOpts) + ")"));
}

void Translator::handleAssignToMemberExpr(const BinaryOperator *assignmentOperator,
                                          SourceManager *sourceManager,
                                          const LangOptions &langOpts)
{
    m_replacements.insert(
        Replacement(*sourceManager,
                    CharSourceRange::getTokenRange(
                        llvm::dyn_cast<MemberExpr>(assignmentOperator->getLHS())->getMemberLoc(),
                        assignmentOperator->getLocEnd()),
                    m_changePack->setterName() + "(" +
                    codeFromASTNode(assignmentOperator->getRHS(), *sourceManager, langOpts) + ")"));
}

void Translator::handleAssignOperatorCall(const CXXOperatorCallExpr *operatorCallExpr,
                                          SourceManager *sourceManager,
                                          const LangOptions &langOpts)
{
    auto assignee = operatorCallExpr->getArg(0);
    SourceLocation location;
    if (llvm::isa<MemberExpr>(assignee)) {
        location = llvm::dyn_cast<MemberExpr>(assignee)->getMemberLoc();
    } else {
        Q_ASSERT(llvm::isa<DeclRefExpr>(assignee));
        location = llvm::dyn_cast<DeclRefExpr>(assignee)->getLocation();
    }
    m_replacements.insert(
        Replacement(*sourceManager,
                    CharSourceRange::getTokenRange(location, operatorCallExpr->getLocEnd()),
                    m_changePack->setterName() + "(" +
                    codeFromASTNode(operatorCallExpr->getArg(1), *sourceManager, langOpts) + ")"));
}

