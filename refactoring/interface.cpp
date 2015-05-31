#include "interface.h"

#include "utils.h"

//////////////////////// Compilation Database

// Maybe skip this opaque pointer? (but would leak details...)
struct CompilationDatabase_t
{
    std::unique_ptr<clang::tooling::CompilationDatabase> database;
};

CompilationDatabase getCompilationDatabase(
        const std::string &buildPath,
        ProjectKind kind,
        std::string &errorMessage)
{
    if (kind != ProjectKind::CMAKE) {
        errorMessage = "Only CMake projects are supported for now";
        return nullptr;
    }
    auto result = makeCompilationDatabaseFromCMake(buildPath, errorMessage);
    if (result == nullptr) {
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
    std::unordered_map<std::string, std::string> cache;
    clang::tooling::ClangTool clangTool;
};

// We need fixed memory locations
auto makeCache(
        std::unordered_map<std::string, std::string> &&cache
)
{
    return std::move(cache);    // Rewrite may be necessary
}

RefactoringsContext getRefactoringsContext(
        CompilationDatabase db,
        const std::vector<std::string> &sources,
        std::unordered_map<std::string, std::string> &&cache
)
{
    auto result = new RefactoringsContext_t{
            std::move(db->database),
            makeCache(std::move(cache)),
            makeClangTool(*db->database, sources)
    };
    makeClangToolCacheAware(result->clangTool, result->cache);
    releaseCompilationDatabase(db);
    return result;
}
