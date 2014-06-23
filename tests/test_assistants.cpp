/* This file is part of KDevelop
     Copyright 2012 Olivier de Gaalon <olivier.jg@gmail.com>
               2014 David Stevens <dgedstevens@gmail.com>

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License version 2 as published by the Free Software Foundation.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
     Library General Public License for more details.

     You should have received a copy of the GNU Library General Public License
     along with this library; see the file COPYING.LIB. If not, write to
     the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
     Boston, MA 02110-1301, USA.
*/

#include "test_assistants.h"

#include <language/assistant/staticassistant.h>
#include <language/assistant/staticassistantsmanager.h>

#include <QtTest/QtTest>
#include <KTempDir>
#include <qtest_kde.h>

#include <tests/autotestshell.h>
#include <tests/testcore.h>
#include <interfaces/foregroundlock.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/ilanguagecontroller.h>
#include <interfaces/iplugincontroller.h>
#include <interfaces/isourceformattercontroller.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <language/backgroundparser/backgroundparser.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainutils.h>
#include <language/codegen/coderepresentation.h>
#include <shell/documentcontroller.h>
#include <interfaces/ilanguage.h>

using namespace KDevelop;
using namespace KTextEditor;

QTEST_KDEMAIN(TestAssistants, GUI)

ForegroundLock *globalTestLock = 0;
StaticAssistantsManager *staticAssistantsManager() { return Core::self()->languageController()->staticAssistantsManager(); }

void TestAssistants::initTestCase()
{
    QVERIFY(qputenv("KDEV_DISABLE_PLUGINS", "kdevcppsupport"));
    AutoTestShell::init();
    TestCore::initialize();
    DUChain::self()->disablePersistentStorage();
    Core::self()->languageController()->backgroundParser()->setDelay(0);
    Core::self()->sourceFormatterController()->disableSourceFormatting(true);
    CodeRepresentation::setDiskChangesForbidden(true);

    globalTestLock = new ForegroundLock;
}

void TestAssistants::cleanupTestCase()
{
    Core::self()->cleanup();
    delete globalTestLock;
    globalTestLock = 0;
}

static QString createFile(const QString& fileContents, QString extension, int id)
{
    static KTempDir dirA;
    QFile file(dirA.name() + QString::number(id) + extension);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(fileContents.toUtf8());
    file.close();
    return file.fileName();
}

class Testbed
{
public:
    enum TestDoc
    {
        HeaderDoc,
        CppDoc
    };

    Testbed(const QString& headerContents, const QString& cppContents)
    {
        static int i = 0;
        int id = i;
        ++i;
        m_headerDocument.url = createFile(headerContents,".h",id);
        m_headerDocument.textDoc = openDocument(m_headerDocument.url);

        m_cppDocument.url = createFile(QString("#include \"%1\"\n").arg(m_headerDocument.url) + cppContents,".cpp",id);
        m_cppDocument.textDoc = openDocument(m_cppDocument.url);
    }
    ~Testbed()
    {
        Core::self()->documentController()->documentForUrl(m_cppDocument.url)->close(KDevelop::IDocument::Discard);
        Core::self()->documentController()->documentForUrl(m_headerDocument.url)->close(KDevelop::IDocument::Discard);

        staticAssistantsManager()->hideAssistant();
    }

    void changeDocument(TestDoc which, Range where, const QString& what, bool waitForUpdate = false)
    {
        TestDocument document;
        if (which == CppDoc)
        {
            document = m_cppDocument;
            where = Range(where.start().line() + 1, where.start().column(),
                                        where.end().line() + 1, where.end().column()); //The include adds a line
        }
        else {
            document = m_headerDocument;
        }
        document.textDoc->activeView()->setSelection(where);
        document.textDoc->activeView()->removeSelectionText();
        document.textDoc->activeView()->setCursorPosition(where.start());
        document.textDoc->activeView()->insertText(what);
        QCoreApplication::processEvents();
        if (waitForUpdate) {
            DUChain::self()->waitForUpdate(IndexedString(document.url), KDevelop::TopDUContext::AllDeclarationsAndContexts);
        }
    }

    QString documentText(TestDoc which)
    {
        if (which == CppDoc)
        {
            //The CPP document text shouldn't include the autogenerated include line
            QString text = m_cppDocument.textDoc->text();
            return text.mid(text.indexOf("\n") + 1);
        }
        else
            return m_headerDocument.textDoc->text();
    }
private:
    struct TestDocument {
        QString url;
        Document *textDoc;
    };

    Document* openDocument(const QString& url)
    {
        Core::self()->documentController()->openDocument(url);
        DUChain::self()->waitForUpdate(IndexedString(url), KDevelop::TopDUContext::AllDeclarationsAndContexts);
        return Core::self()->documentController()->documentForUrl(url)->textDocument();
    }

    TestDocument m_headerDocument;
    TestDocument m_cppDocument;
};


/**
 * A StateChange describes an insertion/deletion/replacement and the expected result
**/
struct StateChange
{
    StateChange(){};
    StateChange(Testbed::TestDoc document, const Range& range, const QString& newText, const QString& result)
        : document(document)
        , range(range)
        , newText(newText)
        , result(result)
    {
    }
    Testbed::TestDoc document;
    Range range;
    QString newText;
    QString result;
};

Q_DECLARE_METATYPE(StateChange)
Q_DECLARE_METATYPE(QList<StateChange>)

const QString SHOULD_ASSIST = "SHOULD_ASSIST"; //An assistant will be visible
const QString NO_ASSIST = "NO_ASSIST";               //No assistant visible

void TestAssistants::testSignatureAssistant_data()
{
    QTest::addColumn<QString>("headerContents");
    QTest::addColumn<QString>("cppContents");
    QTest::addColumn<QList<StateChange> >("stateChanges");
    QTest::addColumn<QString>("finalHeaderContents");
    QTest::addColumn<QString>("finalCppContents");

    QTest::newRow("Change Argument Type")
      << "class Foo {\nint bar(int a, char* b, int c = 10); \n};"
      << "int Foo::bar(int a, char* b, int c)\n{ a = c; b = new char; return a + *b; }"
      << (QList<StateChange>() << StateChange(Testbed::HeaderDoc, Range(1,8,1,11), "char", SHOULD_ASSIST))
      << "class Foo {\nint bar(char a, char* b, int c = 10); \n};"
      << "int Foo::bar(char a, char* b, int c)\n{ a = c; b = new char; return a + *b; }";

    QTest::newRow("Change Default Parameter")
        << "class Foo {\nint bar(int a, char* b, int c = 10); \n};"
        << "int Foo::bar(int a, char* b, int c)\n{ a = c; b = new char; return a + *b; }"
        << (QList<StateChange>() << StateChange(Testbed::HeaderDoc, Range(1,29,1,34), "", NO_ASSIST))
        << "class Foo {\nint bar(int a, char* b, int c); \n};"
        << "int Foo::bar(int a, char* b, int c)\n{ a = c; b = new char; return a + *b; }";

    QTest::newRow("Change Function Type")
        << "class Foo {\nint bar(int a, char* b, int c = 10); \n};"
        << "int Foo::bar(int a, char* b, int c)\n{ a = c; b = new char; return a + *b; }"
        << (QList<StateChange>() << StateChange(Testbed::CppDoc, Range(0,0,0,3), "char", SHOULD_ASSIST))
        << "class Foo {\nchar bar(int a, char* b, int c = 10); \n};"
        << "char Foo::bar(int a, char* b, int c)\n{ a = c; b = new char; return a + *b; }";

    QTest::newRow("Swap Args Definition Side")
        << "class Foo {\nint bar(int a, char* b, int c = 10); \n};"
        << "int Foo::bar(int a, char* b, int c)\n{ a = c; b = new char; return a + *b; }"
        << (QList<StateChange>() << StateChange(Testbed::CppDoc, Range(0,13,0,28), "char* b, int a,", SHOULD_ASSIST))
        << "class Foo {\nint bar(char* b, int a, int c = 10); \n};"
        << "int Foo::bar(char* b, int a, int c)\n{ a = c; b = new char; return a + *b; }";

    // see https://bugs.kde.org/show_bug.cgi?id=299393
    // actually related to the whitespaces in the header...
    QTest::newRow("Change Function Constness")
        << "class Foo {\nvoid bar(const Foo&) const;\n};"
        << "void Foo::bar(const Foo&) const\n{}"
        << (QList<StateChange>() << StateChange(Testbed::CppDoc, Range(0,25,0,31), "", SHOULD_ASSIST))
        << "class Foo {\nvoid bar(const Foo&);\n};"
        << "void Foo::bar(const Foo&)\n{}";
}

void TestAssistants::testSignatureAssistant()
{
    QFETCH(QString, headerContents);
    QFETCH(QString, cppContents);
    Testbed testbed(headerContents, cppContents);

    QFETCH(QList<StateChange>, stateChanges);
    foreach (StateChange stateChange, stateChanges)
    {
        testbed.changeDocument(stateChange.document, stateChange.range, stateChange.newText, true);

        if (stateChange.result == SHOULD_ASSIST) {
            QVERIFY(staticAssistantsManager()->activeAssistant());
            QVERIFY(staticAssistantsManager()->activeAssistant()->actions().size());
        } else {
            QVERIFY(!staticAssistantsManager()->activeAssistant() || !staticAssistantsManager()->activeAssistant()->actions().size());
        }
    }
    if (staticAssistantsManager()->activeAssistant() && staticAssistantsManager()->activeAssistant()->actions().size())
        staticAssistantsManager()->activeAssistant()->actions().first()->execute();

    QFETCH(QString, finalHeaderContents);
    QFETCH(QString, finalCppContents);
    QCOMPARE(testbed.documentText(Testbed::HeaderDoc), finalHeaderContents);
    QCOMPARE(testbed.documentText(Testbed::CppDoc), finalCppContents);
}

#include "test_assistants.moc"