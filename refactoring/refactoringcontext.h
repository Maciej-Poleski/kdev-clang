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

#ifndef KDEV_CLANG_REFACTORINGCONTEXT_H
#define KDEV_CLANG_REFACTORINGCONTEXT_H

// base class
#include <QObject>

// KF5
#include <KTextEditor/ktexteditor/cursor.h>

// LLVM
#include <llvm/Support/ErrorOr.h>

// Clang
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Refactoring.h>

namespace KDevelop
{
class IDocumentController;

class IProject;
};

class KDevRefactorings;

class DocumentCache;

// TODO: join with DocumentCache, handle CompilationDatabase here
class RefactoringContext : public QObject
{
    Q_OBJECT;
    Q_DISABLE_COPY(RefactoringContext);

    class Worker;

public:
    RefactoringContext(KDevRefactorings *parent);

    KDevRefactorings *parent();

    llvm::ErrorOr<unsigned> offset(const std::string &sourceFile,
                                   const KTextEditor::Cursor &position) const;

    void schedule(std::function<void(clang::tooling::RefactoringTool &)> task);
    void scheduleOnSingleFile(std::function<void(clang::tooling::RefactoringTool &)> task,
                              const std::string &filename);

private: // (slots)
    // Only one project for now
    void projectOpened(KDevelop::IProject *project);
    void projectConfigured(KDevelop::IProject *project);


public:
    std::unique_ptr<clang::tooling::CompilationDatabase> database;
    DocumentCache *cache;

private:
    Worker *m_worker;
};


#endif //KDEV_CLANG_REFACTORINGCONTEXT_H
