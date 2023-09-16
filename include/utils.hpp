#pragma once

#ifndef _CA_UTILS_HPP
#define _CA_UTILS_HPP

#include <clang/AST/AST.h>
// #include <clang/AST/ASTContext.h>
// #include <clang/AST/Expr.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
// #include <clang/Frontend/ASTUnit.h>
// #include <clang/Frontend/CompilerInstance.h>
// #include <clang/Frontend/FrontendActions.h>
// #include <clang/Tooling/CommonOptionsParser.h>
// #include <clang/Tooling/Tooling.h>
// #include <llvm/Support/CommandLine.h>
// #include <llvm/Support/raw_ostream.h>

// #include <iostream>
// #include <map>
#include <string>

std::string getLocationString(const clang::SourceManager &SM,
                              const clang::SourceLocation &Loc);

std::string getFuncDeclString(const clang::FunctionDecl *FD);

void printFuncDecl(const clang::FunctionDecl *FD,
                   const clang::SourceManager &SM);

void printCaller(const clang::CallExpr *CE, const clang::SourceManager &SM);

#endif  // !_CA_UTILS_HPP