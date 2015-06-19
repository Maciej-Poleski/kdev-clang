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

#ifndef KDEV_CLANG_CACHE_H
#define KDEV_CLANG_CACHE_H

// C++ std
#include <string>
#include <memory>
#include <unordered_map>

// LLVM
#include <llvm/ADT/StringMap.h>
#include <clang/Tooling/Tooling.h>

/**
 * Implementation of documents cache for use with libTooling
 */
class DocumentCache
{
public:
    // TODO: consider handling IDocumentController directly (maybe using inferior QObject inside
    // this class to connect to Qt signals). Remember about threads!!!
    DocumentCache(std::unordered_map<std::string, std::string> data);

    bool containsFile(llvm::StringRef name) const;
    const std::string& getFileContent(llvm::StringRef name) const;

    void updateFileContent(std::string &&name, std::string content);
    void updateFileContent(const std::string &name, std::string content);
    void removeFile(const std::string &name);

    void makeClangToolCacheAware(clang::tooling::ClangTool &clangTool);

private:
    llvm::StringMap<std::unique_ptr<std::pair<std::string, std::string>>> m_data;
};


#endif //KDEV_CLANG_CACHE_H
