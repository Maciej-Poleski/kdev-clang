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

#include "utils.h"

#include <llvm/ADT/StringRef.h>

#include "util/clangdebug.h"

using namespace clang::tooling;

std::unique_ptr<CompilationDatabase> makeCompilationDatabaseFromCMake(
        std::string buildPath,
        std::string &errorMessage
)
{
    return CompilationDatabase::loadFromDirectory(buildPath, errorMessage);
}

ClangTool makeClangTool(
        const CompilationDatabase &database,
        const std::vector<std::string> &sources
)
{
    auto result = ClangTool(database, sources);
    return result;
}

enum class EndOfLine
{
    LF,
    CRLF,
    CR,
};

/// Detect end of line marker in @p text
static EndOfLine endOfLine(llvm::StringRef text)
{
    bool seenCr = false;
    for (auto c : text)
    {
        switch (c)
        {
        case '\r':
            seenCr = true;
            break;
        case '\n':
            if (seenCr)
            {
                return EndOfLine::CRLF;
            }
            else
            {
                return EndOfLine::LF;
            }
        default:
            if (seenCr)
            {
                return EndOfLine::CR;
            }
        }
    }
    // if file has only one line or ends with CR
    return seenCr ? EndOfLine::CR : EndOfLine::LF;
}

// Cached entries need manual handling because these are outside of FileManager
static KTextEditor::Range toRange(
        llvm::StringRef text,
        unsigned offset,
        unsigned length
)
{
    unsigned lastLine = 0;
    unsigned lastColumn = 0;
    const EndOfLine eolMarker = endOfLine(text);
    // LF always ends line
    auto move = [&text, &lastLine, &lastColumn, eolMarker](
            unsigned start,
            unsigned length
    ) {
        for (unsigned i = start; i < start + length && i < text.size(); ++i)
        {
            switch (text[i])
            {
            case '\r':
                if (eolMarker == EndOfLine::CR)
                {
                    lastLine++;
                    lastColumn = 0;
                }
                else
                {
                    lastColumn++;
                }
                break;
            case '\n':
                lastLine++;
                lastColumn = 0;
                break;
            default:
                lastColumn++;
            }
        }
    };
    move(0, offset);
    const KTextEditor::Cursor start(lastLine, lastColumn);
    move(offset, length);
    const KTextEditor::Cursor end(lastLine, lastColumn);
    return KTextEditor::Range(std::move(start), std::move(end));
}

/// Decides if file should be taken from cache or file system and takes it
static llvm::StringRef getFileContent(
        llvm::StringRef name,
        const Cache &cache,
        clang::FileManager &fileManager
)
{
    if (cache.containsFile(name))
    {
        return cache.getFileContent(name);
    }
    else
    {
        auto r = fileManager.getBufferForFile(name);
        if (!r)
        {
            //throw std::runtime_error("Unable to read from file " + name.str());
            clangDebug()<<"Unable to read from file " << name.str().c_str();
            return llvm::StringRef();
            // are exceptions really so scary?
        }
        return r.get()->getBuffer();
    }
}

static KDevelop::DocumentChange toDocumentChange(
        const Replacement &replacement,
        const Cache &cache,
        clang::FileManager &fileManager
)
{
    // mapping z cache odbywa się przed samym run - filemanager o nim nie wie
    // SourceManager::getColumnNumber chyba numeruje od 1
    // linie chyba też
    // czy on w ogóle robi refaktoring na zamapowanych plikach?

    // IndexedString constructor has this limitation
    Q_ASSERT(replacement.getFilePath().size() <=
             std::numeric_limits<unsigned short>::max());

    auto result = KDevelop::DocumentChange(
            KDevelop::IndexedString(
                    replacement.getFilePath().data(),
                    static_cast<unsigned short>(
                            replacement.getFilePath().size()
                    )
            ),
            toRange(
                    getFileContent(
                            replacement.getFilePath(),
                            cache,
                            fileManager
                    ),
                    replacement.getOffset(),
                    replacement.getLength()
            ),
            QString(), // we don't have this data
            QString::fromStdString(replacement.getReplacementText())
            // NOTE: above conversion assumes UTF-8 encoding
    );
    result.m_ignoreOldText = true;
    return result;
}

KDevelop::DocumentChangeSet toDocumentChangeSet(
        const clang::tooling::Replacements &replacements,
        const Cache &cache,
        clang::FileManager &fileManager
)
{
    //NOTE: Can handle file renaming
    KDevelop::DocumentChangeSet result;
    for (const auto &r : replacements)
    {
        result.addChange(toDocumentChange(r, cache, fileManager));
    }
    return result;
}