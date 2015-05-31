#ifndef KDEV_CLANG_UTILS_H
#define KDEV_CLANG_UTILS_H

#include <memory>
#include <unordered_map>

#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Tooling/Tooling.h>

std::unique_ptr<clang::tooling::CompilationDatabase> makeCompilationDatabaseFromCMake(
        std::string buildPath,
        std::string &errorMessage
);

clang::tooling::ClangTool makeClangTool(
        const clang::tooling::CompilationDatabase &database,
        const std::vector<std::string>& sources
);

void makeClangToolCacheAware(
        clang::tooling::ClangTool &clangTool,
        const std::unordered_map<std::string, std::string> &cache
);

#endif //KDEV_CLANG_UTILS_H
