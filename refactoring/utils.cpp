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

// Qt
#include <QString>
#include <QFileInfo>

// Clang
#include <clang/Lex/Lexer.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Refactoring.h>

#include "redeclarationchain.h"
#include "declarationsymbol.h"
#include "debug.h"
#include "usrcomparator.h"

using namespace clang;
using namespace clang::tooling;
using namespace KDevelop;

using llvm::StringRef;
using llvm::ErrorOr;

std::unique_ptr<CompilationDatabase> makeCompilationDatabaseFromCMake(const std::string &buildPath,
                                                                      QString &errorMessage)
{
    std::string msg;
    auto result = CompilationDatabase::loadFromDirectory(buildPath, msg);
    errorMessage = QString::fromStdString(msg);
    return result;
}

std::unique_ptr<RefactoringTool> makeRefactoringTool(const CompilationDatabase &database,
                                                     const std::vector<std::string> &sources)
{
    auto result = cpp::make_unique<RefactoringTool>(database, sources);
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
static ErrorOr<std::string> readFileContent(StringRef name, DocumentCache *cache,
                                            FileManager &fileManager)
{
    if (cache->fileIsOpened(name)) {
        // It would be great if we could get rid of this use of cache
        return cache->contentOfOpenedFile(name);
    } else {
        auto r = fileManager.getBufferForFile(name);
        if (!r) {
            return r.getError();
        }
        return r.get()->getBuffer().str();
    }
}

static ErrorOr<DocumentChangePointer> toDocumentChange(const Replacement &replacement,
                                                       DocumentCache *cache,
                                                       FileManager &fileManager)
{
    // (Clang) FileManager is unaware of cache (from ClangTool) (cache is applied just before run)
    // SourceManager enumerates columns counting from 1 (probably also lines)

    ErrorOr<std::string> fileContent = readFileContent(replacement.getFilePath(), cache,
                                                       fileManager);

    if (!fileContent) {
        return fileContent.getError();
    }

    // workaround deduplicate issues in DocumentChangeSet
    QString filePath = QFileInfo(
        QString::fromLocal8Bit(replacement.getFilePath().data(), replacement.getFilePath().size()))
        .canonicalFilePath();

    auto result = DocumentChangePointer(new DocumentChange(
        IndexedString(filePath),
        toRange(
            fileContent.get(),
            replacement.getOffset(),
            replacement.getLength()
        ),
        QString(), // we don't have this data
        QString::fromStdString(replacement.getReplacementText())
        // NOTE: above conversion assumes UTF-8 encoding
    ));
    result->m_ignoreOldText = true;
    return result;
}

ErrorOr<DocumentChangeSet> toDocumentChangeSet(const Replacements &replacements,
                                               DocumentCache *cache,
                                               FileManager &fileManager)
{
    // NOTE: DocumentChangeSet can handle file renaming, libTooling will not do that, but it may be
    // reasonable in some cases (renaming of a class, ...). This feature may be used outside to
    // further polish result.
    DocumentChangeSet result;
    std::error_code lastError;
    QList<DocumentChangePointer> changes;
    for (const auto &r : replacements) {
        ErrorOr<DocumentChangePointer> documentChange = toDocumentChange(r, cache, fileManager);
        if (!documentChange) {
            lastError = documentChange.getError();
            refactorWarning() << "Unable to translate replacement: " << r.toString();
        } else {
            bool duplicate = false;
            // this slow O(n^2) algorithm can be (quite) easily improved to almost O(n) (hash)
            for (auto existingChange : changes) {
                if (existingChange->m_newText == documentChange.get()->m_newText &&
                    existingChange->m_range == documentChange.get()->m_range &&
                    existingChange->m_document == documentChange.get()->m_document) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                result.addChange(documentChange.get());
                changes.push_back(documentChange.get());
                refactorDebug() << "Translated replacement: " << documentChange.get()->m_document <<
                                documentChange.get()->m_range.start().line() <<
                                documentChange.get()->m_range.start().column() <<
                                documentChange.get()->m_range.end().line() <<
                                documentChange.get()->m_range.end().column() <<
                                documentChange.get()->m_newText;
            }
        }
    }
    if (!changes.empty()) {
        return result;
    } else if (lastError) {
        return lastError;
    } else {
        // it's ok, we just don't have replacements
        return result;
    }
}

/**
 * Translate to definite eol (last character in line)
 */
static char toChar(EndOfLine eol)
{
    switch (eol) {
    case EndOfLine::LF:
    case EndOfLine::CRLF:
        return '\n';
    case EndOfLine::CR:
        return '\r';
    }
    Q_ASSERT(false);
    return '\0';
}

ErrorOr<unsigned> toOffset(const std::string &fileName, const KTextEditor::Cursor &position,
                           ClangTool &clangTool,
                           DocumentCache *documentCache)
{
    auto fileContent = readFileContent(fileName, documentCache, clangTool.getFiles());
    if (!fileContent) {
        return fileContent.getError();
    }
    int currentLine = 0;
    unsigned currentOffset = 0;
    const char eol = toChar(endOfLine(fileContent.get()));
    auto i = fileContent.get().begin();
    while (currentLine < position.line()) {
        Q_ASSERT(i != fileContent.get().end());
        auto c = *i++;
        currentOffset++;
        if (c == eol) {
            currentLine++;
        }
    }
    int currentColumn = 0;
    while (currentColumn < position.column()) {
        Q_ASSERT(i != fileContent.get().end());
        i++;
        currentColumn++;
        currentOffset++;
    }
    return currentOffset;
}

bool isInRange(const std::string &fileName, unsigned offset, SourceLocation start,
               SourceLocation end, const SourceManager &sourceManager)
{
    auto startD = sourceManager.getDecomposedLoc(start);
    auto endD = sourceManager.getDecomposedLoc(end);
    auto fileEntry = sourceManager.getFileEntryForID(startD.first);
    if (fileEntry == nullptr) {
        return false;
    }
    if ((fileEntry->getName() != fileName) &&
        (!llvm::sys::fs::equivalent(fileEntry->getName(), fileName))) {
        return false;
    }
    return startD.second <= offset && offset <= endD.second;
}

bool isInRange(const std::string &fileName, unsigned offset, SourceRange range,
               const SourceManager &sourceManager)
{
    return isInRange(fileName, offset, range.getBegin(), range.getEnd(), sourceManager);
}

SourceRange tokenRangeToCharRange(SourceRange range, const SourceManager &sourceManager,
                                  const LangOptions &langOptions)
{
    SourceRange result(range.getBegin(), range.getEnd().getLocWithOffset(
        Lexer::MeasureTokenLength(range.getEnd(), sourceManager, langOptions)));
    return result;
}

SourceRange tokenRangeToCharRange(SourceRange range,
                                  const CompilerInstance &CI)
{
    return tokenRangeToCharRange(range, CI.getSourceManager(), CI.getLangOpts());
}

bool isLocationEqual(const std::string &fileName, unsigned offset, clang::SourceLocation location,
                     const clang::SourceManager &sourceManager)
{
    auto locationD = sourceManager.getDecomposedLoc(location);
    auto fileEntry = sourceManager.getFileEntryForID(locationD.first);
    if (fileEntry == nullptr) {
        return false;
    }
    if (!llvm::sys::fs::equivalent(fileEntry->getName(), fileName)) {
        return false;
    }
    return locationD.second == offset;
}

bool operator==(const LexicalLocation &lhs, const LexicalLocation &rhs)
{
    return (lhs.offset == rhs.offset) && (lhs.fileName == rhs.fileName);
}

LexicalLocation lexicalLocation(const Decl *decl)
{
    const auto &srcMgr = decl->getASTContext().getSourceManager();
    auto location = srcMgr.getDecomposedLoc(decl->getLocation());
    auto fileEntry = srcMgr.getFileEntryForID(location.first);
    if (fileEntry == nullptr) {
        return {"", location.second};
    }
    else {
        return {fileEntry->getName(), location.second};
    }
}

// It is terribly inconvenient. Clang AST can be used to reason about declarations _within_one_TU_
// but its not difficult task to make set of translation units with code referring to the same
// entity (object, function, ...) but without _any_ connection in Clang AST. Compiler works on TUs,
// we are working here on the whole project. That's difference. Clang can provide us little help
// here
std::unique_ptr<DeclarationComparator> declarationComparator(const Decl *decl)
{
    // First try to use Clang USR
    auto usrComparator = UsrComparator::create(decl);
    if (usrComparator) {
        return std::move(usrComparator);
    } else {
        refactorDebug() << "clang::index::generateUSRForDecl refused to mangle:";
        std::string msg;
        llvm::raw_string_ostream ostream(msg);
        decl->dump(ostream);
        refactorDebug() << ostream.str();
    }
    // fallback
    if (const NamedDecl *namedDecl = llvm::dyn_cast<NamedDecl>(decl)) {
        auto linkage = namedDecl->getLinkageInternal();
        if (linkage == ExternalLinkage) {
            return cpp::make_unique<DeclarationSymbol>(namedDecl);
        }
    }
    return cpp::make_unique<RedeclarationChain>(decl);
}

int getTokenRangeSize(const SourceRange &range, const SourceManager &sourceManager,
                      const LangOptions &langOpts)
{
    SourceLocation spellingBegin = sourceManager.getSpellingLoc(range.getBegin());
    SourceLocation spellingEnd = sourceManager.getSpellingLoc(range.getEnd());
    std::pair<FileID, unsigned> start = sourceManager.getDecomposedLoc(spellingBegin);
    std::pair<FileID, unsigned> end = sourceManager.getDecomposedLoc(spellingEnd);
    if (start.first != end.first) {
        return -1;
    }
    end.second += Lexer::MeasureTokenLength(spellingEnd, sourceManager, langOpts);
    return end.second - start.second;
}

std::string textFromTokenRange(clang::SourceRange range, const clang::SourceManager &sourceManager,
                               const clang::LangOptions &langOpts)
{
    int size = getTokenRangeSize(range, sourceManager, langOpts);
    const char *data = sourceManager.getCharacterData(range.getBegin());
    if (data == nullptr) {
        refactorWarning() << "data is nullptr";
    } else if (size == -1) {
        refactorWarning() << "range does not fit in one file";
    } else {
        Q_ASSERT(size >= 0);
        return std::string(data, static_cast<std::size_t>(size));
    }
    return "$FAILED$";  // Always invalid as source code
}

void dumpTokenRange(clang::SourceRange range, const SourceManager &sourceManager,
                    const LangOptions &langOpts)
{
    refactorDebug() << textFromTokenRange(range, sourceManager, langOpts);
}

std::string suggestGetterName(const std::string &fieldName)
{
    Q_ASSERT(!fieldName.empty());
    auto i = fieldName.find('_');
    if (i != fieldName.npos && i + 1 < fieldName.size()) {
        return fieldName.substr(i + 1);
    } else if (i != fieldName.npos && fieldName.size() > 1) {
        return fieldName.substr(0, i);
    } else {
        return std::string("get") + std::toupper(fieldName[0], std::locale()) + fieldName.substr(1);
    }
}

std::string suggestSetterName(const std::string &fieldName)
{
    Q_ASSERT(!fieldName.empty());
    auto i = fieldName.find('_');
    std::string result;
    if (i != fieldName.npos && i + 1 < fieldName.size()) {
        result = fieldName.substr(i + 1);
    } else if (i != fieldName.npos && fieldName.size() > 1) {
        result = fieldName.substr(0, i);
    } else {
        result = fieldName;
    }
    result[0] = std::toupper(result[0], std::locale());
    return "set" + result;
}

std::string functionName(const std::string &functionDeclaration, const std::string &fallbackName)
{
    auto ast = buildASTFromCode(functionDeclaration);
    if (!ast) {
        refactorWarning() << "Unable to parse function:\n" << functionDeclaration;
        refactorWarning() << "Using fallback" << fallbackName;
        return fallbackName;
    }
    TranslationUnitDecl *tuDecl = ast->getASTContext().getTranslationUnitDecl();
    FunctionDecl *functionDecl = nullptr;
    for (Decl *decl : tuDecl->decls()) {
        if (FunctionDecl *fdecl = dyn_cast<FunctionDecl>(decl)) {
            if (fdecl->isThisDeclarationADefinition()) {
                functionDecl = fdecl;
                break;
            }
        }
    }
    if (!functionDecl) {
        refactorWarning() << "Didn't find function definition here:\n" << functionDeclaration;
        refactorWarning() << "Using fallback" << fallbackName;
        return fallbackName;
    }
    return functionDecl->getName();
}

std::string toString(clang::QualType type, const clang::LangOptions &langOpts)
{
    PrintingPolicy policy(langOpts);
    policy.SuppressUnwrittenScope = true;
    return type.getAsString(policy);
}
