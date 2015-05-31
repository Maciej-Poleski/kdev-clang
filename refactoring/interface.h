// This header file should be independent of any Clang libraries both
// compile time and run time.
// All of refactorings features will be accessible from here

#ifndef KDEV_CLANG_INTERFACE_H
#define KDEV_CLANG_INTERFACE_H

#include <vector>
#include <string>
#include <unordered_map>

struct CompilationDatabase_t;
struct RefactoringsContext_t;

// All std::strings appearing here may be replaced to something different
// if necessary (QString, QUrl, ...)
// All functions provided here get C linkage. These (few) functions as designed
// to provide interoperability with KDevelop based on stable, predictable ABI.
// It can bu used later to dlopen implementation, or as "engine" in standalone
// executable.
// Note: it doesn't mean use of POD data types here
// Note: some types are not fully known now. typedefs will change soon
extern "C" {
// Qt plugin system may be better solution

typedef void* _unspecified; // Will specify later

// CompilationDatabase will be used internally, but needs initialization
typedef CompilationDatabase_t* CompilationDatabase;

// All data which can be required for any operation of this library.
// It is CompilationDatabase, ClangTool (with virtual files), ...
typedef RefactoringsContext_t* RefactoringsContext;

// Short information about one particular kind of refactoring.
typedef _unspecified RefactoringKind;

enum class ProjectKind
{
    CMAKE,      // CMake project are already nice
    NON_CMAKE,  // Non CMake are nice after use of Bear
};  // Where "nice" means prepared compile_commands.json file

/**
 * Prepares and returns compilation database for given project build path.
 * Note: result of this operation is deprecated by any change in project
 * structure (adding new file, removing, renaming, changing flags, ...).
 * In such a case new object must be created using this function.
 *
 * @param buildPath build path for given project (ex. CMAKE_BINARY_DIR)
 * @param kind CMAKE if underlying project is a configured CMake project
 *             with CMAKE_EXPORT_COMPILE_COMMANDS enabled, NON_CMAKE otherwise
 * @param errorMessage will be set to error message if any occurred
 * @return Compilation database for given project
 */
CompilationDatabase getCompilationDatabase(
        const std::string &buildPath,
        ProjectKind kind,
        std::string &errorMessage
);

/**
 * Frees resources used by given compilation database.
 * NOTE: You mustn't release database given to @c getRefactoringsContext
 * as that function takes ownership of compilation database.
 *
 * @param db compilation database to be released
 */
void releaseCompilationDatabase(
        CompilationDatabase  db
);

// TODO: mergeCompilationDatabases - handle refactorings between projects

/**
 * Prepare and return main context for refactorings in KDevelop.
 *
 * @param db Up-to-date compilation database
 * @param sources All sources which can be subject of refactorings
 * @param cache Mapping from file name to file content of cached files
 * @return Context for future use with refactorings
 */
RefactoringsContext getRefactoringsContext(
        CompilationDatabase db,
        const std::vector<std::string> &sources,
        std::unordered_map<std::string, std::string> &&cache
);

// TODO: method to update cache of RefactoringsContext

/**
 * Returns all applicable refactorings "here"
 *
 * @param rc Initialized refactorings context
 * @param sourceFile queried source file name (full path) (may be changed to QUrl, ...)
 * @param location offset from beginning of the file to location we are querying
 *
 * @return List of all applicable refactorings "here"
 */
std::vector<RefactoringKind> getAllApplicableRefactorings(
        RefactoringsContext rc,
        std::string sourceFile,
        unsigned location
);

/**
 * Returns short human readable "name" of a refactoring action.
 * Can be used to populate context-menu...
 *
 * @param refactoring A refactoring kind we are asking for description
 * @return Short human readable description
 */
std::string describeRefactoringKind(
        RefactoringKind refactoring
);

// TODO: refactorThis(RefactoringsContext, RefactoringKind, Location, ...)
// TODO: refactor(RefactoringsContext, RefactoringKind, What, ...)
// ^^^^ will return Replacements or immediate translation to DocumentChangeSet
// THIS IS THE NEXT TASK
};

#endif //KDEV_CLANG_INTERFACE_H
