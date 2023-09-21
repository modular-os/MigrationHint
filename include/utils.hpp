#pragma once

#ifndef _CA_UTILS_HPP
#define _CA_UTILS_HPP

#include <clang/AST/AST.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>

#include <string>

namespace ca_utils {
std::string getCoreFileNameFromPath(const std::string &path);

std::string getLocationString(const clang::SourceManager &SM,
                              const clang::SourceLocation &Loc);

std::string getFuncDeclString(const clang::FunctionDecl *FD);

void printFuncDecl(const clang::FunctionDecl *FD,
                   const clang::SourceManager &SM);

void printCaller(const clang::CallExpr *CE, const clang::SourceManager &SM);

bool getExternalStructType(clang::QualType FDType, llvm::raw_ostream &output,
                           clang::SourceManager &SM);
}  // namespace ca_utils

#endif  // !_CA_UTILS_HPP