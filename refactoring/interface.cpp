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

#include "interface.h"

#include "utils.h"
#include "Cache.h"

//////////////////////// Compilation Database

// Maybe skip this opaque pointer? (but would leak details...)
struct CompilationDatabase_t
{
    std::unique_ptr<clang::tooling::CompilationDatabase> database;
};

CompilationDatabase getCompilationDatabase(
        const std::string &buildPath,
        ProjectKind kind,
        std::string &errorMessage
)
{
    if (kind != ProjectKind::CMAKE)
    {
        errorMessage = "Only CMake projects are supported for now";
        return nullptr;
    }
    auto result = makeCompilationDatabaseFromCMake(buildPath, errorMessage);
    if (result == nullptr)
    {
        return nullptr;
    }
    return new CompilationDatabase_t{std::move(result)};
};

void releaseCompilationDatabase(
        CompilationDatabase db
)
{
    delete db;
}

///////////////// Refactorings Context

struct RefactoringsContext_t
{
    std::unique_ptr<clang::tooling::CompilationDatabase> database;
    Cache cache;
    // NOTE: exists CodeRepresentation (KDevelop side)
    clang::tooling::ClangTool clangTool;
};

RefactoringsContext getRefactoringsContext(
        CompilationDatabase db,
        const std::vector<std::string> &sources,
        std::unordered_map<std::string, std::string> &&cache
)
{
    auto result = new RefactoringsContext_t{
            std::move(db->database),
            Cache(std::move(cache)),
            makeClangTool(*db->database, sources)
    };
    result->cache.makeClangToolCacheAware(result->clangTool);
    releaseCompilationDatabase(db);
    return result;
}

void updateCache(
        RefactoringsContext rc,
        std::string &&fileName,
        std::string &&fileContent
)
{
    rc->cache.updateFileContent(std::move(fileName), std::move(fileContent));
}

void removeFromCache(
        RefactoringsContext rc,
        const std::string &fileName
)
{
    rc->cache.removeFile(fileName);
    auto &fm = rc->clangTool.getFiles();
    auto file = fm.getFile(fileName, false, false);
    if (file)   // if file is not cached - done,
    {
        //  otherwise ...
        fm.invalidateCache(file);
    }
}