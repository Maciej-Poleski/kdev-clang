#include "utils.h"

using namespace clang::tooling;

std::unique_ptr<CompilationDatabase> makeCompilationDatabaseFromCMake(
        std::string buildPath,
        std::string &errorMessage)
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

void makeClangToolCacheAware(
        clang::tooling::ClangTool &clangTool,
        const std::unordered_map<std::string, std::string> &cache
)
{
    for (auto &p : cache) {
        clangTool.mapVirtualFile(p.first, p.second);
    }
}
