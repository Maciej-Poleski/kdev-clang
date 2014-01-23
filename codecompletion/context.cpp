/*
 * This file is part of KDevelop
 * Copyright 2014 Milian Wolff <mail@milianw.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "context.h"

#include <language/duchain/duchainlock.h>
#include <language/duchain/ducontext.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/declaration.h>
#include <language/interfaces/iastcontainer.h>
#include <language/codecompletion/codecompletionitem.h>
#include <language/codecompletion/codecompletionmodel.h>
#include <language/codecompletion/normaldeclarationcompletionitem.h>

#include "../duchain/parsesession.h"
#include "../duchain/clangtypes.h"

#include "../debug.h"

#include <memory>
#include <KTextEditor/Document>

using namespace KDevelop;

namespace {

class SimpleItem : public CompletionTreeItem
{
public:
    SimpleItem(const QString& typed, const QString& result, const QString& text,
               CodeCompletionModel::CompletionProperties properties);
    virtual ~SimpleItem() = default;

    virtual QVariant data(const QModelIndex& index, int role, const CodeCompletionModel* model) const;

    virtual void execute(KTextEditor::Document* document, const KTextEditor::Range& word);

    virtual CodeCompletionModel::CompletionProperties completionProperties() const;

private:
    QString m_typed;
    QString m_result;
    QString m_text;
    CodeCompletionModel::CompletionProperties m_properties;
};

SimpleItem::SimpleItem(const QString& typed, const QString& result, const QString& text,
                       CodeCompletionModel::CompletionProperties properties)
    : m_typed(typed)
    , m_result(result)
    , m_text(text)
    , m_properties(properties)
{

}

QVariant SimpleItem::data(const QModelIndex& index, int role, const CodeCompletionModel* /*model*/) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }
    switch (index.column()) {
        case CodeCompletionModel::Name:
            return m_typed;
        case CodeCompletionModel::Prefix:
            return m_result;
    }
    return {};
}

void SimpleItem::execute(KTextEditor::Document* document, const KTextEditor::Range& word)
{
    document->replaceText(word, m_text);
}

CodeCompletionModel::CompletionProperties SimpleItem::completionProperties() const
{
    return m_properties;
}

QByteArray concatenate(const QStringList& contents)
{
    QByteArray ret;
    int sizeGuess = 0;
    foreach(const QString& line, contents) {
        sizeGuess += line.size() + 1;
    }
    ret.reserve(sizeGuess);
    foreach(const QString& line, contents) {
        ret += line.toUtf8() + '\n';
    }
    return ret;
}

}

ClangCodeCompletionContext::ClangCodeCompletionContext(const ParseSession* const session,
                                                       const SimpleCursor& position,
                                                       const QStringList& contents
                                                      )
    : CodeCompletionContext({}, QString(), {}, 0)
    , m_results(nullptr, clang_disposeCodeCompleteResults)
{
    ClangString file(clang_getFileName(session->file()));

    CXUnsavedFile unsaved;
    const QByteArray fileContents = concatenate(contents);
    unsaved.Contents = fileContents.constData();
    unsaved.Length = fileContents.size();
    unsaved.Filename = file;

    m_results.reset( clang_codeCompleteAt(session->unit(), file,
                        position.line + 1, position.column + 1,
                        &unsaved, 1,
                        clang_defaultCodeCompleteOptions()) );
}

ClangCodeCompletionContext::~ClangCodeCompletionContext()
{
}

QList<CompletionTreeItemPointer> ClangCodeCompletionContext::completionItems(const TopDUContext* const top,
                                                                             const CursorInRevision& position)
{
    QList<CompletionTreeItemPointer> items;
    items.reserve(m_results->NumResults);

    QSet<QualifiedIdentifier> handled;

    for (uint i = 0; i < m_results->NumResults; ++i) {
        auto result = m_results->Results[i];
        const uint chunks = clang_getNumCompletionChunks(result.CompletionString);

        QString typed;
        QString resultType;
        QString text;
        for (uint j = 0; j < chunks; ++j) {
            const auto kind = clang_getCompletionChunkKind(result.CompletionString, j);
            const QString string = ClangString(clang_getCompletionChunkText(result.CompletionString, j)).toString();
            switch (kind) {
                case CXCompletionChunk_TypedText:
                    typed = string;
                    break;
                case CXCompletionChunk_ResultType:
                    resultType = string;
                    continue;
                case CXCompletionChunk_CurrentParameter:
                case CXCompletionChunk_Informative:
                case CXCompletionChunk_Optional:
                    continue;
                case CXCompletionChunk_Placeholder:
                    text += "/*" + string + "*/";
                    continue;
                default:
                    break;
            }
            text += string;
        }
        if (result.CursorKind != CXCursor_MacroDefinition && result.CursorKind != CXCursor_NotImplemented) {
            const Identifier id(typed);
            QualifiedIdentifier qid;
            ClangString parent(clang_getCompletionParent(result.CompletionString, nullptr));
            if (parent.c_str() != nullptr) {
                qid = QualifiedIdentifier(parent.toString());
            }
            qid.push(id);

            if (handled.contains(qid)) {
                // TODO: support overload items - for now, just skip them
                continue;
            }
            handled.insert(qid);
            Declaration* found = 0;
            foreach(Declaration* dec, top->findDeclarations(qid, position)) {
                found = dec;
                break;
            }
            if (found) {
                items.append(CompletionTreeItemPointer(new NormalDeclarationCompletionItem(DeclarationPointer(found))));
                continue;
            } else {
                debug() << "Could not find declaration for" << qid;
            }
        }

        // TODO: grouping of macros and built-in stuff
        items.append(CompletionTreeItemPointer(new SimpleItem(typed, resultType, text, CodeCompletionModel::GlobalScope)));
    }
    return items;
}