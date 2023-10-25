#pragma once

#ifndef _CA_AST_HELPERS_HPP
#define _CA_AST_HELPERS_HPP

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
// #include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
// #include <clang/Lex/PPCallbacks.h>
// #include <clang/Lex/Preprocessor.h>
// #include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
// #include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "ca_ASTHelpers.hpp"
#include "ca_PreprocessorHelpers.hpp"
#include "ca_utils.hpp"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/MacroArgs.h"
namespace ca {
int ModuleAnalysisHelper(std::string sourceFiles);
/**********************************************************************
 * 2. Matcher Callbacks
 **********************************************************************/
class ExternalCallMatcher
    : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  explicit inline ExternalCallMatcher(clang::SourceManager &SM) : AST_SM(SM) {}

  void onStartOfTranslationUnit() override;

  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) override;

  void onEndOfTranslationUnit() override;

 private:
  std::map<std::string,
           std::map<std::string, std::vector<const clang::CallExpr *>>>
      FilenameToCallExprs;
  clang::SourceManager &AST_SM;
};

class ExternalDependencyMatcher
    : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  void onStartOfTranslationUnit() override;

  void handleExternalTypeFuncD(const clang::FunctionDecl *FD,
                               clang::SourceManager &SM,
                               const clang::LangOptions &LO);

  void handleExternalTypeFD(const clang::RecordDecl *RD,
                            clang::SourceManager &SM,
                            int &isInFunctionOldValue);

  void handleExternalTypeVD(const clang::VarDecl *VD, clang::SourceManager &SM,
                            const clang::LangOptions &LO,
                            int &isInFunctionOldValue);

  void handleExternalImplicitCE(const clang::ImplicitCastExpr *ICE,
                                clang::SourceManager &SM);

  void handleExternalCall(const clang::CallExpr *CE, clang::SourceManager &SM);

  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) override;

  void onEndOfTranslationUnit() override;

 private:
  /*Field control flags*/
  int isInFunction;

  /*Statistics*/
  int externalStructCnt;
  int externalVarDeclCnt;
  int externalParamVarDeclCnt;
  int externalImplicitExprCnt;
  int externalFunctionCallCnt;
};

}  // namespace ca

#endif  // !_CA_AST_HELPERS_HPP