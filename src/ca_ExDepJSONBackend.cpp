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
#include "ca_Abilities.hpp"
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
void ExternalDependencyJSONBackend::onStartOfTranslationUnit() {
#ifdef DEBUG
  llvm::outs() << "In onStartOfTranslationUnit\n";
#endif
  // Clear the MacroDeduplicationSet
  MacroDeduplication.clear();

  // Clear the SigToAbility
  SigToAbility.clear();
}

void ExternalDependencyJSONBackend::run(
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

void ExternalDependencyJSONBackend::onEndOfTranslationUnit() {
  // Create a file descriptor outstream
  llvm::raw_fd_ostream fileOS(filePath, EC, llvm::sys::fs::OF_None);
  if (EC) {
    llvm::errs() << "Error opening file " << filePath << ": " << EC.message()
                 << "\n";
  } else {
    llvm::json::OStream J(fileOS);
    J.array([&] {
      // Traverse the ExternalDepToSignature
      for (auto &it : SigToAbility) {
        if (it.second != nullptr) {
          JSONRoot.push_back(it.second->buildJSON(AST_SM));
          // delete it.second;
          // TODO & WARNING!: May leak the memory

          // Output JSON to file
          J.value(it.second->buildJSON(AST_SM));
          fileOS << '\n';
        }
      }
    });
  }

  // Use Formatv to print the JSONRoot
  // llvm::outs() << "[\n";
  // for (auto &it : JSONRoot) {
  //   llvm::outs() << llvm::formatv("{0:2}", it);
  //   if (it != JSONRoot.back()) {
  //     llvm::outs() << ",";
  //   }
  //   llvm::outs() << "\n";
  // }
  // llvm::outs() << "]\n";
}

void ExternalDependencyJSONBackend::handleExternalTypeFuncD(
    const clang::FunctionDecl *FD, clang::SourceManager &SM,
    const clang::LangOptions &LO, bool internalMode) {
  std::string TypeSigString = "test";
  // llvm::raw_string_ostream StructDeclStream(StructDecl);
  if (SM.isInMainFile(FD->getLocation()) || internalMode) {
    ca::ExternalTypeAbility *RetTypeAbi = nullptr, *ParamTypeAbi = nullptr;
    RetTypeAbi = ca_utils::getExternalType(FD->getReturnType(), SM);
    if (RetTypeAbi != nullptr) {
      // llvm::outs() << "[Debug1]: ";
      TypeSigString = RetTypeAbi->getSignature();

      auto mapIterator = SigToAbility.find(TypeSigString);

      if (mapIterator != SigToAbility.end()) {
        mapIterator->second->addCallLoc(FD->getLocation());
        delete RetTypeAbi;
      } else {
        RetTypeAbi->popCallLoc();
        RetTypeAbi->addCallLoc(FD->getLocation());
        SigToAbility[TypeSigString] = static_cast<ca::Ability *>(RetTypeAbi);
      }
    }
    // Clear StuctDecl preventing duplications
    TypeSigString.clear();

    // Traverse the FuncDecl's ParamVarDecls
    for (const auto &PVD : FD->parameters()) {
      // llvm::outs() << "--------------------------------\n";
      // llvm::outs() << "ParameterDecl: " << PVD->getNameAsString() << "("
      //              << ca_utils::getLocationString(SM, PVD->getLocation())
      //              << ")\n";
      ParamTypeAbi = ca_utils::getExternalType(PVD->getType(), SM);
      if (ParamTypeAbi != nullptr) {
        TypeSigString = ParamTypeAbi->getSignature();

        auto mapIterator = SigToAbility.find(TypeSigString);

        if (mapIterator != SigToAbility.end()) {
          mapIterator->second->addCallLoc(PVD->getLocation());
          delete ParamTypeAbi;
        } else {
          ParamTypeAbi->popCallLoc();
          ParamTypeAbi->addCallLoc(PVD->getLocation());
          SigToAbility[TypeSigString] = static_cast<ca::Ability *>(ParamTypeAbi);
        }
      }
      // Clear StuctDecl preventing duplications due to looping
      TypeSigString.clear();
    }
  }
}

void ExternalDependencyJSONBackend::handleExternalTypeFD(
    const clang::RecordDecl *RD, clang::SourceManager &SM,
    int &isInFunctionOldValue) {
  std::string TypeSigString = "test";
  if (!RD->getName().empty() && SM.isInMainFile(RD->getLocation())) {
    for (const auto &FD : RD->fields()) {
      auto TypeAbi = ca_utils::getExternalType(FD->getType(), SM);
      if (TypeAbi != nullptr) {
        TypeSigString = TypeAbi->getSignature();
        auto mapIterator = SigToAbility.find(TypeSigString);

        if (mapIterator != SigToAbility.end()) {
          mapIterator->second->addCallLoc(FD->getLocation());
          delete TypeAbi;
        } else {
          TypeAbi->popCallLoc();
          TypeAbi->addCallLoc(FD->getLocation());
          SigToAbility[TypeSigString] = static_cast<ca::Ability *>(TypeAbi);
        }
      }
      // Clear StuctDecl preventing duplications due to looping
      TypeSigString.clear();
    }
  }
}

void ExternalDependencyJSONBackend::handleExternalTypeVD(
    const clang::VarDecl *VD, clang::SourceManager &SM,
    const clang::LangOptions &LO, int &isInFunctionOldValue) {
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
    // assert("Run into the deprecated handle for variable decl!");
    std::string TypeSigString = "test";
    if (SM.isInMainFile(VD->getLocation())) {
      // llvm::outs() << "--------------------------------\n";
      // llvm::outs() << "VariableDecl: " << VD->getNameAsString() << "("
      //              << ca_utils::getLocationString(SM, VD->getLocation())
      //              << ")\n";
      auto TypeAbi = ca_utils::getExternalType(VD->getType(), SM);
      if (TypeAbi != nullptr) {
        TypeSigString = TypeAbi->getSignature();
        auto mapIterator = SigToAbility.find(TypeSigString);

        if (mapIterator != SigToAbility.end()) {
          mapIterator->second->addCallLoc(VD->getLocation());
          delete TypeAbi;
        } else {
          TypeAbi->popCallLoc();
          TypeAbi->addCallLoc(VD->getLocation());
          SigToAbility[TypeSigString] = static_cast<ca::Ability *>(TypeAbi);
        }
      }
    }
}

void ExternalDependencyJSONBackend::handleExternalMacroInt(
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
    clang::SourceLocation MacroLocation;

    if (tmpStack.size() == 1) {
      MacroLocation = IntL->getBeginLoc();
    } else {
      MacroLocation = tmpStack[tmpStack.size() - 2];
    }
    auto MacroName = ca_utils::getMacroName(SM, MacroLocation);
    auto MacroDedupName =
        MacroName + "@" +
        ca_utils::getLocationString(SM, tmpStack[tmpStack.size() - 1]);
    std::string MacroIdentifier = ca_utils::getMacroIdentifier(MacroName);
    std::string ExpansionTree = "No Expansion Tree Found.";

    if (ca_utils::isMacroInteger(MacroName)) {
      if (MacroDeduplication.find(MacroDedupName) == MacroDeduplication.end()) {
        MacroDeduplication.insert(MacroDedupName);
        auto mapIterator = SigToAbility.find(MacroName);
        if (mapIterator != SigToAbility.end()) {
          mapIterator->second->addCallLoc(tmpStack[tmpStack.size() - 1]);
        } else {
          if (MacroNameToExpansion.find(MacroIdentifier) !=
              MacroNameToExpansion.end()) {
            ExpansionTree = MacroNameToExpansion[MacroIdentifier];
          }
          auto AbilityPTR = new ca::ExternalMacroAbility(
              tmpStack[tmpStack.size() - 1], MacroName, MacroLocation, false,
              ExpansionTree);
          SigToAbility[MacroName] = static_cast<ca::Ability *>(AbilityPTR);
        }
      }
    }
  }
}

void ExternalDependencyJSONBackend::handleExternalImplicitCE(
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
      // auto isExternal = ca_utils::getExternalStructType(
      //     ICE->getType(), llvm::outs(), SM, ExtraInfo);
      if (isExternal != nullptr) {
      }
    }
  }
#endif
  assert("Run into the deprecated handle for implicit cast!");
}

void ExternalDependencyJSONBackend::handleExternalCall(
    const clang::CallExpr *CE, clang::SourceManager &SM) {
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
        auto mapIterator = SigToAbility.find(FunctionDeclString);
        if (mapIterator != SigToAbility.end()) {
          mapIterator->second->addCallLoc(CE->getBeginLoc());
        } else {
          auto AbilityPTR =
              new ca::ExternalCallAbility(Loc, FunctionDeclString, FD);
          SigToAbility[FunctionDeclString] =
              static_cast<ca::Ability *>(AbilityPTR);
        }
        // llvm::outs() << "=============================\n";
        // llvm::outs() << "External Call parameters:\n";
        // llvm::outs() << "FunctionDecl: " << FunctionDeclString << "\n";

        handleExternalTypeFuncD(FD, SM, FD->getASTContext().getLangOpts(),
                                true);

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
        auto MacroDedupName =
            MacroName + "@" +
            ca_utils::getLocationString(SM, tmpStack[tmpStack.size() - 1]);
        std::string ExpansionTree = "No Expansion Tree Found.";
        std::string MacroIdentifier = ca_utils::getMacroIdentifier(MacroName);

        if (MacroDeduplication.find(MacroDedupName) ==
            MacroDeduplication.end()) {
          MacroDeduplication.insert(MacroDedupName);

          auto mapIterator = SigToAbility.find(MacroName);
          if (mapIterator != SigToAbility.end()) {
            mapIterator->second->addCallLoc(tmpStack[tmpStack.size() - 1]);
          } else {
            if (MacroNameToExpansion.find(MacroIdentifier) !=
                MacroNameToExpansion.end()) {
              ExpansionTree = MacroNameToExpansion[MacroIdentifier];
            }
            auto AbilityPTR = new ca::ExternalMacroAbility(
                tmpStack[tmpStack.size() - 1], MacroName, MacroLocation, true,
                ExpansionTree);
            SigToAbility[MacroName] = static_cast<ca::Ability *>(AbilityPTR);
          }
        }
      }
    }
  }
}
}  // namespace ca

// #endif  // !_CA_AST_HELPERS_HPP