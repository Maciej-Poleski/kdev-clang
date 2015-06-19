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

#ifndef KDEV_CLANG_UTILS_H
#define KDEV_CLANG_UTILS_H

// C++ std
#include <memory>
#include <unordered_map>

// LLVM
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/Core/Replacement.h>

// KDevelop
#include <language/codegen/documentchangeset.h>

// refactoring
#include "DocumentCache.h"

namespace cpp
{
// Use std::make_unique instead of this in C++14
template<typename T, typename... Args>
inline std::unique_ptr<T> make_unique(Args &&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
};

std::unique_ptr<clang::tooling::CompilationDatabase> makeCompilationDatabaseFromCMake(
        std::string buildPath, std::string &errorMessage);

clang::tooling::ClangTool makeClangTool(const clang::tooling::CompilationDatabase &database,
                                        const std::vector<std::string> &sources
);

llvm::ErrorOr<KDevelop::DocumentChangeSet> toDocumentChangeSet(
        const clang::tooling::Replacements &replacements,
        const DocumentCache &cache,
        const clang::FileManager &fileManager
);

#endif //KDEV_CLANG_UTILS_H
