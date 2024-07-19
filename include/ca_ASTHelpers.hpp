// Encode with UTF-8
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
#include <llvm/Support/JSON.h>
#include <llvm/Support/raw_ostream.h>

// #include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ca_ASTHelpers.hpp"
#include "ca_Abilities.hpp"
#include "ca_PreprocessorHelpers.hpp"
#include "ca_utils.hpp"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/MacroArgs.h"
namespace ca {
int ModuleAnalysisHelper(std::string sourceFiles);

void generateReport(std::string SourceFilePath);

class ReportMatcher : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  //   void onStartOfTranslationUnit() override;

  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) override;

  //   void onEndOfTranslationUnit() override;
};

/**********************************************************************
 * 2. Matcher Callbacks
 **********************************************************************/
class ExternalCallMatcher
    : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  enum WorkMode { Print, Collect };
  explicit inline ExternalCallMatcher(
      clang::SourceManager &SM, WorkMode _mode = Print,
      std::map<std::string, std::map<std::string, int>>
          *_ModuleFunctionCallCnt = nullptr)
      : ModuleFunctionCallCnt(_ModuleFunctionCallCnt), AST_SM(SM), mode(_mode) {
    if (mode == Collect && ModuleFunctionCallCnt == nullptr) {
      assert(false &&
             "Error! You should specify ModuleFunctionCallCnt to store results "
             "while collecting.\n");
    }
  }

  void onStartOfTranslationUnit() override;

  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) override;

  void onEndOfTranslationUnit() override;

 private:
  std::map<std::string,
           std::map<std::string, std::vector<const clang::CallExpr *>>>
      FilenameToCallExprs;
  std::map<std::string, std::map<std::string, int>> *ModuleFunctionCallCnt;
  clang::SourceManager &AST_SM;
  WorkMode mode;
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

  void handleExternalMacroInt(const clang::IntegerLiteral *IntL,
                              clang::SourceManager &SM,
                              const clang::LangOptions &LO,
                              int &isInFunctionOldValue);

  void handleExternalImplicitCE(const clang::ImplicitCastExpr *ICE,
                                clang::SourceManager &SM);

  void handleExternalCall(const clang::CallExpr *CE, clang::SourceManager &SM);

  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) override;

  void onEndOfTranslationUnit() override;

 private:
  /*Macro deduplication*/
  std::set<std::string> MacroDeduplication;
  /*Field control flags*/
  int isInFunction;

  /*Statistics*/
  int externalStructCnt;
  int externalVarDeclCnt;
  int externalParamVarDeclCnt;
  int externalImplicitExprCnt;
  int externalFunctionCallCnt;
};

class MigrateCodeGenerator
    : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  enum ExternalType { Function, Struct, MacroInt };
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

  void handleExternalMacroInt(const clang::IntegerLiteral *IntL,
                              clang::SourceManager &SM,
                              const clang::LangOptions &LO,
                              int &isInFunctionOldValue);

  void handleExternalImplicitCE(const clang::ImplicitCastExpr *ICE,
                                clang::SourceManager &SM);

  void handleExternalCall(const clang::CallExpr *CE, clang::SourceManager &SM);

  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) override;

  void onEndOfTranslationUnit() override;

 private:
  /*Macro deduplication*/
  std::set<std::string> MacroDeduplication;
  /*Map-Map-Set: ExternalType->Header(string)->`signature`*/
  std::map<ExternalType, std::map<std::string, std::set<std::string>>>
      ExternalDepToSignature;
};

class ExternalDependencyJSONBackend
    : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  ExternalDependencyJSONBackend(
      clang::SourceManager &SM, std::string outputPath,
      std::map<std::string, std::string> &_MacroToExpansion)
      : AST_SM(SM),
        filePath(outputPath),
        MacroNameToExpansion(_MacroToExpansion) {}

  void onStartOfTranslationUnit() override;

  void handleExternalTypeFuncD(const clang::FunctionDecl *FD,
                               clang::SourceManager &SM,
                               const clang::LangOptions &LO,
                               bool internalMode = false);

  void handleExternalTypeFD(const clang::RecordDecl *RD,
                            clang::SourceManager &SM,
                            int &isInFunctionOldValue);

  void handleExternalTypeVD(const clang::VarDecl *VD, clang::SourceManager &SM,
                            const clang::LangOptions &LO,
                            int &isInFunctionOldValue);

  void handleExternalMacroInt(const clang::IntegerLiteral *IntL,
                              clang::SourceManager &SM,
                              const clang::LangOptions &LO,
                              int &isInFunctionOldValue);

  void handleExternalImplicitCE(const clang::ImplicitCastExpr *ICE,
                                clang::SourceManager &SM);

  void handleExternalCall(const clang::CallExpr *CE, clang::SourceManager &SM);

  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) override;

  void onEndOfTranslationUnit() override;

 private:
  /*Macro deduplication*/
  std::set<std::string> MacroDeduplication;
  /*Signature to Ability*/
  /*JSON Root*/
  std::map<std::string, ca::Ability *> SigToAbility;
  llvm::json::Array JSONRoot;
  clang::SourceManager &AST_SM;
  // Save json file
  std::string filePath;
  std::error_code EC;
  /*Macro Name to Expansion*/
  std::map<std::string, std::string> &MacroNameToExpansion;
};

/* Utilities for Ph.D. Zeng*/
void ZengAnalysisHelper(std::vector<std::string> &files);

class ZengExpressionMatcher
    : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) override;

 private:
  class ZengExpressionVisitor
      : public clang::RecursiveASTVisitor<ZengExpressionVisitor> {
   public:
    ZengExpressionVisitor(clang::ASTContext &context, const clang::Expr *rootExpr)
        : context(context), rootExpr(rootExpr), hasMatch(false) {}

    bool VisitExpr(clang::Expr *expr);
    bool hasMatchedExpression() const;
    bool hasLinkMapMember(const clang::Expr *Operand);

   private:
    clang::ASTContext &context;
    const clang::Expr *rootExpr;
    bool hasMatch;
  };
};

}  // namespace ca

#endif  // !_CA_AST_HELPERS_HPP
