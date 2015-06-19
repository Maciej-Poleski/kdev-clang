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
using namespace KDevelop;

using llvm::StringRef;
using llvm::ErrorOr;

std::unique_ptr<CompilationDatabase> makeCompilationDatabaseFromCMake(std::string buildPath,
                                                                      std::string &errorMessage)
{
    return CompilationDatabase::loadFromDirectory(buildPath, errorMessage);
}

ClangTool makeClangTool(const CompilationDatabase &database,
                        const std::vector<std::string> &sources)
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
static EndOfLine endOfLine(StringRef text)
{
    bool seenCr = false;
    for (auto c : text) {
        switch (c) {
        case '\r':
            seenCr = true;
            break;
        case '\n':
            if (seenCr) {
                return EndOfLine::CRLF;
            } else {
                return EndOfLine::LF;
            }
        default:
            if (seenCr) {
                return EndOfLine::CR;
            }
        }
    }
    // if file has only one line or ends with CR
    return seenCr ? EndOfLine::CR : EndOfLine::LF;
}

// Cached entries need manual handling because these are outside of FileManager
static KTextEditor::Range toRange(StringRef text, unsigned offset, unsigned length)
{
    Q_ASSERT(offset + length < text.size());
    unsigned lastLine = 0;
    unsigned lastColumn = 0;
    // last* is our cursor
    const EndOfLine eolMarker = endOfLine(text);
    // LF always ends line
    // Shift cursor in text assuming cursor is in @p offset and eating @p length chars
    auto shift = [&text, &lastLine, &lastColumn, eolMarker]
            (unsigned start, unsigned length)
    {
        for (unsigned i = start; i < start + length && i < text.size(); ++i) {
            switch (text[i]) {
            case '\r':
                if (eolMarker == EndOfLine::CR) {
                    lastLine++;
                    lastColumn = 0;
                }
                else {
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
    shift(0, offset);
    const KTextEditor::Cursor start(lastLine, lastColumn);
    shift(offset, length);
    const KTextEditor::Cursor end(lastLine, lastColumn);
    return KTextEditor::Range(std::move(start), std::move(end));
}

/// Decides if file should be taken from cache or file system and takes it
static ErrorOr<StringRef> readFileContent(StringRef name, const DocumentCache &cache,
                                          clang::FileManager &fileManager)
{
    if (cache.containsFile(name)) {
        return StringRef(cache.getFileContent(name));
    } else {
        auto r = fileManager.getBufferForFile(name);
        if (!r) {
            return r.getError();
        }
        return r.get()->getBuffer();
    }
}

static ErrorOr<DocumentChange> toDocumentChange(const Replacement &replacement,
                                                const DocumentCache &cache,
                                                clang::FileManager &fileManager)
{
    // (Clang) FileManager is unaware of cache (from ClangTool) (cache is applied just before run)
    // SourceManager enumerates columns counting from 1 (probably also lines)
    // Is ClangTool doing refactoring on mapped files? Certainly Replacements cannot be applied
    // on cache (not a problem - Refactorings are translated to DocumentChangeSet HERE)

    // IndexedString constructor has this limitation
    Q_ASSERT(replacement.getFilePath().size() <=
             std::numeric_limits<unsigned short>::max());

    ErrorOr<StringRef> fileContent = readFileContent(replacement.getFilePath(), cache, fileManager);
    if (!fileContent) {
        return fileContent.getError();
    }
    auto result = DocumentChange(
            IndexedString(
                    replacement.getFilePath().data(),
                    static_cast<unsigned short>(
                            replacement.getFilePath().size()
                    )
            ),
            toRange(
                    fileContent.get(),
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

ErrorOr<DocumentChangeSet> toDocumentChangeSet(const clang::tooling::Replacements &replacements,
                                               const DocumentCache &cache,
                                               clang::FileManager &fileManager)
{
    // NOTE: DocumentChangeSet can handle file renaming, libTooling will not do that, but it may be
    // reasonable in some cases (renaming of a class, ...). This feature may be used outside to
    // further polish result.
    DocumentChangeSet result;
    for (const auto &r : replacements) {
        ErrorOr<DocumentChange> documentChange = toDocumentChange(r, cache, fileManager);
        if (!documentChange) {
            return documentChange.getError();
        }
        result.addChange(documentChange.get());
    }
    return result;
}