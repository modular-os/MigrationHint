// Encode with UTF-8
// #pragma once

// #ifndef _CA_AST_HELPERS_HPP
// #define _CA_AST_HELPERS_HPP

// #include <clang/AST/AST.h>
// #include <clang/AST/ASTContext.h>
// #include <clang/AST/Decl.h>
// #include <clang/AST/Expr.h>
// #include <clang/AST/OperationKinds.h>
// #include <clang/Basic/SourceLocation.h>
// #include <clang/Basic/SourceManager.h>
// #include <clang/Frontend/ASTUnit.h>
// #include <clang/Frontend/CompilerInstance.h>
// #include <clang/Frontend/FrontendAction.h>
// #include <clang/Frontend/FrontendActions.h>
// #include <clang/Lex/PPCallbacks.h>
// #include <clang/Lex/Preprocessor.h>
// #include <clang/Tooling/CommonOptionsParser.h>
// #include <clang/Tooling/Tooling.h>
// #include <llvm/Support/raw_ostream.h>
// #include <llvm/Support/CommandLine.h>
// #include <llvm/Support/raw_ostream.h>

// #include <iostream>
// #include <map>
// #include <string>
// #include <vector>

// #include "clang/AST/RecursiveASTVisitor.h"
// #include "clang/ASTMatchers/ASTMatchFinder.h"
#include "ca_utils.hpp"
#include "clang/ASTMatchers/ASTMatchers.h"
// #include "clang/Lex/MacroArgs.h"
// #include "ca_utils.hpp"
// #include "ca_PreprocessorHelpers.hpp"
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/Support/raw_ostream.h>

#include <cmath>
#include <string>

#include "ca_ASTHelpers.hpp"
namespace ca {
/**********************************************************************
 * 2. Matcher Callbacks
 **********************************************************************/
void MigrateCodeGenerator::onStartOfTranslationUnit() {
#ifdef DEBUG
  llvm::outs() << "In onStartOfTranslationUnit\n";
#endif
#ifdef CHN
  llvm::outs() << "# 外部依赖报告\n\n";
  llvm::outs() << "## 全局: \n";
#else
  llvm::outs() << "# External Dependencies Report\n\n";
  llvm::outs() << "## Global: \n";
#endif
  // Clear the MacroDeduplicationSet
  MacroDeduplication.clear();

  // Clear the ExternalDepToSignature
  for (auto &it : ExternalDepToSignature) {
    it.second.clear();
  }
  ExternalDepToSignature.clear();

  // Initialize the ExternalDepToSignature
  ExternalDepToSignature[Function] = {};
  ExternalDepToSignature[Struct] = {};
  ExternalDepToSignature[MacroInt] = {};
}

void MigrateCodeGenerator::handleExternalTypeFuncD(
    const clang::FunctionDecl *FD, clang::SourceManager &SM,
    const clang::LangOptions &LO) {
  std::string StructDecl;
  llvm::raw_string_ostream StructDeclStream(StructDecl);
  if (SM.isInMainFile(FD->getLocation())) {
    clang::RecordDecl *RetRTD = nullptr, *ParamRTD = nullptr;
    RetRTD = ca_utils::getExternalStructType(FD->getReturnType(), llvm::outs(),
                                             SM, "", -1);
    if (RetRTD != nullptr) {
      // llvm::outs() << "[Debug1]: ";
      RetRTD->print(StructDeclStream);
      StructDeclStream.flush();

      auto LocString_ = ca_utils::getLocationString(SM, RetRTD->getLocation());
      std::size_t colonPos = LocString_.find(':');
      auto LocString = LocString_.substr(0, colonPos);

      // Insert into the ExternalDepToSignature
      if (ExternalDepToSignature[Struct].find(LocString) ==
          ExternalDepToSignature[Struct].end()) {
        ExternalDepToSignature[Struct][LocString] = {};
        ExternalDepToSignature[Struct][LocString].insert(StructDecl);
      }
    }

    // Traverse the FuncDecl's ParamVarDecls
    for (const auto &PVD : FD->parameters()) {
      ParamRTD = ca_utils::getExternalStructType(PVD->getType(), llvm::outs(),
                                                 SM, "", -1);
      if (ParamRTD != nullptr) {
        // llvm::outs() << "[Debug2]: ";
        ParamRTD->print(StructDeclStream);
        StructDeclStream.flush();

        auto LocString_ =
            ca_utils::getLocationString(SM, ParamRTD->getLocation());
        std::size_t colonPos = LocString_.find(':');
        auto LocString = LocString_.substr(0, colonPos);

        // Insert into the ExternalDepToSignature
        if (ExternalDepToSignature[Struct].find(LocString) ==
            ExternalDepToSignature[Struct].end()) {
          ExternalDepToSignature[Struct][LocString] = {};
          ExternalDepToSignature[Struct][LocString].insert(StructDecl);
        }
      }
    }
  }
}

void MigrateCodeGenerator::handleExternalTypeFD(const clang::RecordDecl *RD,
                                                clang::SourceManager &SM,
                                                int &isInFunctionOldValue) {
  std::string StructDecl;
  llvm::raw_string_ostream StructDeclStream(StructDecl);
  if (!RD->getName().empty() && SM.isInMainFile(RD->getLocation())) {
    for (const auto &FD : RD->fields()) {
      auto StructRTD =
          ca_utils::getExternalStructType(FD->getType(), llvm::outs(), SM, "");
      if (StructRTD != nullptr) {
        // llvm::outs() << "[Debug2]: ";
        StructRTD->print(StructDeclStream);
        StructDeclStream.flush();

        auto LocString_ =
            ca_utils::getLocationString(SM, StructRTD->getLocation());
        std::size_t colonPos = LocString_.find(':');
        auto LocString = LocString_.substr(0, colonPos);

        // Insert into the ExternalDepToSignature
        if (ExternalDepToSignature[Struct].find(LocString) ==
            ExternalDepToSignature[Struct].end()) {
          ExternalDepToSignature[Struct][LocString] = {};
          ExternalDepToSignature[Struct][LocString].insert(StructDecl);
        }
      }
    }
  }
}

void MigrateCodeGenerator::handleExternalTypeVD(const clang::VarDecl *VD,
                                                clang::SourceManager &SM,
                                                const clang::LangOptions &LO,
                                                int &isInFunctionOldValue) {
#ifdef DEPRECATED
  /*
  Deprecated, now we print the info of ParamVarDecl in FuncDecl/
  But we can still analysis the info of ParamVarDecl in the following
  part.
  */
  if (auto PVD = dyn_cast<clang::ParmVarDecl>(VD)) {
    if (SM.isInMainFile(PVD->getLocation())) {
    }

  } else
#endif
    assert("Run into the deprecated handle for variable decl!");
}

void MigrateCodeGenerator::handleExternalMacroInt(
    const clang::IntegerLiteral *IntL, clang::SourceManager &SM,
    const clang::LangOptions &LO, int &isInFunctionOldValue) {
  auto Loc = IntL->getBeginLoc();
  if (SM.isInMainFile(Loc) && SM.isMacroBodyExpansion(Loc)) {
    // !!! Handle macro operations
    // Judging whether the caller is expanded from predefined macros.

    std::vector<clang::SourceLocation> tmpStack;
    while (true) {
      if (SM.isMacroBodyExpansion(Loc)) {
        auto ExpansionLoc = SM.getImmediateMacroCallerLoc(Loc);
        Loc = ExpansionLoc;
      } else if (SM.isMacroArgExpansion(Loc)) {
        auto ExpansionLoc = SM.getImmediateExpansionRange(Loc).getBegin();
        Loc = ExpansionLoc;
      } else {
        break;
      }
      tmpStack.push_back(Loc);
    }
    assert(tmpStack.size() != 0 && "Macro expansion stack is empty.\n");
    // TODO: Begin testing
    clang::SourceLocation MacroLocation;

    if (tmpStack.size() == 1) {
      MacroLocation = IntL->getBeginLoc();
    } else {
      MacroLocation = tmpStack[tmpStack.size() - 2];
    }
    auto MacroName = ca_utils::getMacroName(SM, MacroLocation);
    // auto MacroDedupName =
        // MacroName + "@" +
        // ca_utils::getLocationString(SM, tmpStack[tmpStack.size() - 1]);

    auto LocString_ = ca_utils::getLocationString(SM, MacroLocation);
    std::size_t colonPos = LocString_.find(':');
    auto LocString = LocString_.substr(0, colonPos);

    if (ExternalDepToSignature[MacroInt].find(LocString) ==
        ExternalDepToSignature[MacroInt].end()) {
      ExternalDepToSignature[MacroInt][LocString] = {};
      ExternalDepToSignature[MacroInt][LocString].insert(MacroName);
    }
  }
}

void MigrateCodeGenerator::handleExternalImplicitCE(
    const clang::ImplicitCastExpr *ICE, clang::SourceManager &SM) {
#ifdef DEPRECATED
  if (SM.isInMainFile(ICE->getBeginLoc()) &&
      SM.isInMainFile(ICE->getEndLoc())) {
    if (ICE->getCastKind() == clang::CK_LValueToRValue ||
        ICE->getCastKind() == clang::CK_IntegralToPointer) {
      // Only handle clang::CK_LValueToRValue IntegralToPointer
#ifdef DEBUG
      llvm::outs() << "ImplicitCastExpr("
                   << ca_utils::getLocationString(SM, ICE->getBeginLoc())
                   << "): " << ICE->getType().getAsString()
                   << "^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
#endif
      std::string ExtraInfo = "";
#ifdef CHN

      ExtraInfo +=
          "\n   `Implicit Cast(" + std::string(ICE->getCastKindName()) + ")`\n";
      ExtraInfo += "   - 类型: `外部类型（隐式转换）`\n";
      ExtraInfo += "   - 定义路径: `" +
                   ca_utils::getLocationString(SM, ICE->getBeginLoc()) + "`\n";
      ExtraInfo += "   - 简介：`<Filled-By-AI>`\n";
      ExtraInfo += "   - 外部类型细节：\n";
#else
      ExtraInfo += "\n### ImplicitCastExpr\n";
      ExtraInfo += "   - Location: " +
                   ca_utils::getLocationString(SM, ICE->getBeginLoc()) + "\n";
      ExtraInfo +=
          "   - CastKind: " + std::string(ICE->getCastKindName()) + "\n";
#endif
      auto isExternal = ca_utils::getExternalStructType(
          ICE->getType(), llvm::outs(), SM, ExtraInfo);
      if (isExternal != nullptr) {
      }
    }
  }
#endif
  assert("Run into the deprecated handle for implicit cast!");
}

void MigrateCodeGenerator::handleExternalCall(const clang::CallExpr *CE,
                                              clang::SourceManager &SM) {
  // TODO: Fix Macro bugs.
  std::string FunctionDeclString = "";
  std::string LocString_ = "", LocString = "";
  if (auto FD = CE->getDirectCallee()) {
    // output the basic information of the function declaration
    if (!SM.isInMainFile(FD->getLocation()) &&
        SM.isInMainFile(CE->getBeginLoc())) {
      auto Loc = CE->getBeginLoc();
      std::string LocString_ = "", LocString = "";
      if (!SM.isMacroBodyExpansion(Loc) && !SM.isMacroArgExpansion(Loc)) {
        FunctionDeclString = ca_utils::getFuncDeclString(FD);
        LocString_ = ca_utils::getLocationString(SM, FD->getLocation());
        std::size_t colonPos = LocString_.find(':');
        LocString = LocString_.substr(0, colonPos);
        //

      } else {
        // !!! Handle macro operations
        auto CallerLoc = CE->getBeginLoc();
        // Judging whether the caller is expanded from predefined macros.
        std::vector<clang::SourceLocation> tmpStack;
        while (true) {
          if (SM.isMacroBodyExpansion(CallerLoc)) {
            auto ExpansionLoc = SM.getImmediateMacroCallerLoc(CallerLoc);
            CallerLoc = ExpansionLoc;
          } else if (SM.isMacroArgExpansion(CallerLoc)) {
            auto ExpansionLoc =
                SM.getImmediateExpansionRange(CallerLoc).getBegin();
            CallerLoc = ExpansionLoc;
          } else {
            break;
          }
          tmpStack.push_back(CallerLoc);
        }
        assert(tmpStack.size() != 0 && "Macro expansion stack is empty.\n");
        clang::SourceLocation MacroLocation;
        if (tmpStack.size() == 1) {
          MacroLocation = FD->getLocation();
        } else {
          MacroLocation = tmpStack[tmpStack.size() - 2];
        }
        auto MacroName = ca_utils::getMacroName(SM, MacroLocation);

        FunctionDeclString = MacroName;
        LocString_ = ca_utils::getLocationString(SM, MacroLocation);
        std::size_t colonPos = LocString_.find(':');
        LocString = LocString_.substr(0, colonPos);
      }
    }
    if (ExternalDepToSignature[Function].find(LocString) ==
        ExternalDepToSignature[Function].end()) {
      ExternalDepToSignature[Function][LocString] = {};
      ExternalDepToSignature[Function][LocString].insert(
          FunctionDeclString);
    }
  }
}

void MigrateCodeGenerator::run(
    const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  auto &SM = *Result.SourceManager;
  auto &LO = Result.Context->getLangOpts();
  int isInFunctionOldValue = 0;
  if (auto FD =
          Result.Nodes.getNodeAs<clang::FunctionDecl>("externalTypeFuncD")) {
    handleExternalTypeFuncD(FD, SM, LO);
  } else if (auto RD =
                 Result.Nodes.getNodeAs<clang::RecordDecl>("externalTypeFD")) {
    handleExternalTypeFD(RD, SM, isInFunctionOldValue);
  } else if (auto VD =
                 Result.Nodes.getNodeAs<clang::VarDecl>("externalTypeVD")) {
    if (VD->getKind() != clang::Decl::ParmVar) {
      handleExternalTypeVD(VD, SM, LO, isInFunctionOldValue);
    }
  } else if (auto ICE = Result.Nodes.getNodeAs<clang::ImplicitCastExpr>(
                 "externalImplicitCE")) {
    handleExternalImplicitCE(ICE, SM);
  } else if (auto CE =
                 Result.Nodes.getNodeAs<clang::CallExpr>("externalCall")) {
    handleExternalCall(CE, SM);
  } else if (auto IntL = Result.Nodes.getNodeAs<clang::IntegerLiteral>(
                 "integerLiteral")) {
    handleExternalMacroInt(IntL, SM, LO, isInFunctionOldValue);
  } else if (auto TU = Result.Nodes.getNodeAs<clang::TranslationUnitDecl>(
                 "translationUnit")) {
    TU->dump(llvm::outs());
  } else {
#ifdef DEBUG
    llvm::outs() << "No call or fieldDecl expression found\n";
#endif
  }
}

void MigrateCodeGenerator::onEndOfTranslationUnit() {
  // Traverse the ExternalDepToSignature
   

}
}  // namespace ca

// #endif  // !_CA_AST_HELPERS_HPP