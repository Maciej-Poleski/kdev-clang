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

#include "Cache.h"

#include <memory>

Cache::Cache(std::unordered_map<std::string, std::string> &&data)
{
    for (auto &p : data)
    {
        updateFileContent(p.first, std::move(p.second));
    }
}

bool Cache::containsFile(llvm::StringRef name) const
{
    // TODO: Can make hash table with string references as keys?
    return _data.find(name) != _data.end(); // name is copied only for query
}

const std::string &Cache::getFileContent(llvm::StringRef name) const
{
    return _data.find(name)->second->second;
}


void Cache::updateFileContent(
        std::string &&name,
        std::string &&content
)
{
    auto t = std::make_unique<std::pair<std::string, std::string>>(
            std::move(name),
            std::move(content)
    );
    llvm::StringRef key = t->first;
    _data[key] = std::move(t);
}

void Cache::updateFileContent(
        const std::string &name,
        std::string &&content
)
{
    auto t = std::make_unique<std::pair<std::string, std::string>>(
            std::cref(name),
            std::move(content)
    );
    llvm::StringRef key = t->first;
    _data[key] = std::move(t);
}

void Cache::removeFile(const std::string &name)
{
    _data.erase(name);
}

void Cache::makeClangToolCacheAware(clang::tooling::ClangTool &clangTool)
{
    for (auto &p : _data)
    {
        auto &d = *p.second;
        clangTool.mapVirtualFile(d.first, d.second);
    }
}
