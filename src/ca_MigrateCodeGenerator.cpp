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
}

void MigrateCodeGenerator::handleExternalTypeFuncD(
    const clang::FunctionDecl *FD, clang::SourceManager &SM,
    const clang::LangOptions &LO) {
  if (SM.isInMainFile(FD->getLocation())) {
#ifdef DEBUG
    llvm::outs() << "FunctionDecl("
                 << ca_utils::getLocationString(SM, FD->getLocation())
                 << "): ^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
#endif

    auto Loc = FD->getLocation();
    if (SM.isMacroBodyExpansion(Loc) || SM.isMacroArgExpansion(Loc)) {
      // Judging whether the caller is expanded from predefined macros.
      auto CallerLoc = Loc;
      // !!! Handle macro operations
      // Judging whether the caller is expanded from predefined macros.
      std::vector<clang::SourceLocation> tmpStack;
      while (true) {
        if (SM.isMacroBodyExpansion(CallerLoc)) {
          auto ExpansionLoc = SM.getImmediateMacroCallerLoc(CallerLoc);
          CallerLoc = ExpansionLoc;
        } else if (SM.isMacroArgExpansion(CallerLoc)) {
#ifdef DEBUG
          llvm::outs() << "Is in macro arg expansion\n";
#endif
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
        MacroLocation = tmpStack[tmpStack.size() - 1];
      }

#ifdef CHN
      llvm::outs() << "\n\n---\n\n\n## 函数(宏): `"
                   << ca_utils::getMacroName(SM, MacroLocation) << "`\n"
                   << "- 函数位置: `"
                   << ca_utils::getLocationString(SM, MacroLocation) << "`\n";
#else
      llvm::outs() << "\n\n---\n\n\n## Function(Macro): `"
                   << ca_utils::getFuncDeclString(FD) << "`\n"
                   << "- Function Location: `"
                   << ca_utils::getLocationString(SM, FD->getLocation())
                   << "`\n";
#endif
#ifdef CHN
      llvm::outs() << "- 该函数是宏展开的一部分。\n";
#else
      llvm::outs() << "- Expanded from macros.\n";
#endif

    } else {
#ifdef CHN
      llvm::outs() << "\n\n---\n\n\n## 函数: `"
                   << ca_utils::getFuncDeclString(FD) << "`\n"
                   << "- 函数位置: `"
                   << ca_utils::getLocationString(SM, FD->getLocation())
                   << "`\n";
#else
      llvm::outs() << "\n\n---\n\n\n## Function: `"
                   << ca_utils::getFuncDeclString(FD) << "`\n"
                   << "- Function Location: `"
                   << ca_utils::getLocationString(SM, FD->getLocation())
                   << "`\n";
#endif
    }
#ifdef DEBUG
    llvm::outs() << "Field changes here. FuncDecl 0->1.\n";
#endif

// Deal with the external return type
#ifdef CHN
    llvm::outs() << "- 返回类型: `" + FD->getReturnType().getAsString() + "`\n";
#else
    llvm::outs() << "- Return Type: `" + FD->getReturnType().getAsString() +
                        "`\n";
#endif

    ca_utils::getExternalStructType(FD->getReturnType(), llvm::outs(), SM, "");
    // Traverse the FuncDecl's ParamVarDecls
    for (const auto &PVD : FD->parameters()) {
#ifdef DEBUG
      llvm::outs() << "ParamVarDecl("
                   << ca_utils::getLocationString(SM, PVD->getLocation())
                   << "): ^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
#endif
#ifdef CHN
      std::string ExtraInfo = "\n   `" + PVD->getType().getAsString() + " " +
                              PVD->getNameAsString() + "`\n";
      ExtraInfo += "   - 类型: `外部类型（函数参数）`\n";
      ExtraInfo += "   - 定义路径: `" +
                   ca_utils::getLocationString(SM, PVD->getLocation()) + "`\n";
      ExtraInfo += "   - 简介：`<Filled-By-AI>`\n";
      ExtraInfo += "   - 外部类型细节：\n";
#else
      std::string ExtraInfo =
          "\n### ParamVarDecl: `" + PVD->getNameAsString() + "`\n";
      ExtraInfo += "   - Location: `" +
                   ca_utils::getLocationString(SM, PVD->getLocation()) + "`\n";
      ExtraInfo += "   - Type: `" + PVD->getType().getAsString() + "`\n";
#endif
      if (const auto ParentFuncDeclContext = PVD->getParentFunctionOrMethod()) {
        // Notice: Method is only used in C++
        if (const auto ParentFD =
                dyn_cast<clang::FunctionDecl>(ParentFuncDeclContext)) {
#ifdef CHN
#else
          ExtraInfo += "   - Function: `" +
                       ca_utils::getFuncDeclString(ParentFD) + "`\n";
#endif
        }
      } else if (const auto TU = PVD->getTranslationUnitDecl()) {
#ifdef CHN
#else
        ExtraInfo += "   - Parent: Global variable, no parent function.\n";
#endif
      }

      // Output the init expr for the VarDecl
      if (PVD->hasInit()) {
        auto InitText =
            clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(
                                            PVD->getInit()->getSourceRange()),
                                        SM, LO);
        if (InitText.str() != "" && InitText.str() != "NULL") {
#ifdef CHN
#else
          ExtraInfo += "   - ParamVarDecl Has Init: \n   ```c\n" +
                       InitText.str() + "\n   ```\n";
#endif

        } else {
#ifdef CHN
#else
          ExtraInfo += "   - ParamVarDecl Has Init, but no text found.\n";
#endif
        }
      }
      auto isExternalType = ca_utils::getExternalStructType(
          PVD->getType(), llvm::outs(), SM, ExtraInfo);
      if (isExternalType != nullptr) {
      }
    }
  }
}

void MigrateCodeGenerator::handleExternalTypeFD(
    const clang::RecordDecl *RD, clang::SourceManager &SM,
    int &isInFunctionOldValue) {
#ifdef DEBUG
  llvm::outs() << "ExternalStructMatcher\n";
  llvm::outs() << FD->getQualifiedNameAsString() << "\n";
  llvm::outs() << FD->getType().getAsString() << " " << FD->getNameAsString()
               << "\n";
  auto RD = FD->getParent();
  llvm::outs() << "\t" << RD->getQualifiedNameAsString() << "\n";
#endif
  // Dealing with the relationships between RecordDecl and fieldDecl
  // output the basic information of the RecordDecl

  if (!RD->getName().empty() && SM.isInMainFile(RD->getLocation())) {
    // Output the basic info for specific RecordDecl
    std::string BasicInfo = "";
#ifdef CHN
#else
    BasicInfo = "\n### StructDecl: `" + RD->getQualifiedNameAsString() + "`\n";
#endif

// Output the basic location info for the fieldDecl
#ifdef CHN
#else
    BasicInfo += "- Location: `" +
                 ca_utils::getLocationString(SM, RD->getLocation()) + "`\n";
#endif

    if (const auto ParentFuncDeclContext = RD->getParentFunctionOrMethod()) {
      // Notice: Method is only used in C++
      if (const auto ParentFD =
              dyn_cast<clang::FunctionDecl>(ParentFuncDeclContext)) {
#ifdef CHN
#else
        BasicInfo +=
            "- Parent: `" + ca_utils::getFuncDeclString(ParentFD) + "`\n";
#endif
      }
    } else if (const auto TU = RD->getTranslationUnitDecl()) {
#ifdef CHN
#else
      BasicInfo += "- Parent: Global variable, no parent function.\n";
#endif

    }
    llvm::outs() << BasicInfo;

// Output the full definition for the fieldDecl
#ifdef CHN
#else
    llvm::outs() << "- Full Definition: \n"
                 << "```c\n";
    RD->print(llvm::outs(), clang::PrintingPolicy(clang::LangOptions()));
    llvm::outs() << "\n```\n";
#endif

// Traverse its fieldDecl and find external struct member
#ifdef CHN
#else
    llvm::outs() << "- External Struct Members: \n";
#endif
    for (const auto &FD : RD->fields()) {
#ifdef DEBUG
      llvm::outs() << "\t" << FD->getType().getAsString() << " "
                   << FD->getNameAsString() << " "
                   << FDType->isStructureOrClassType() << "\n";
#endif
      std::string ExtraInfo = "";
#ifdef CHN
      ExtraInfo += "\n   `" + FD->getType().getAsString() + " " +
                   FD->getNameAsString() + "`\n";
      ExtraInfo += "   - 类型: `外部类型（结构体声明的外部成员）`\n";
      ExtraInfo += "   - 定义路径: `" +
                   ca_utils::getLocationString(SM, FD->getLocation()) + "`\n";
      ExtraInfo += "   - 父结构体声明: `" + RD->getQualifiedNameAsString() +
                   "(" + ca_utils::getLocationString(SM, RD->getLocation()) +
                   ")" + "`\n";
      ExtraInfo += "   - 简介：`<Filled-By-AI>`\n";
      ExtraInfo += "   - 外部类型细节：\n";
#else
      ExtraInfo += "   - Member: `" + FD->getType().getAsString() + " " +
                   FD->getNameAsString() + "`\n";
#endif
      auto IsExternalType = ca_utils::getExternalStructType(
          FD->getType(), llvm::outs(), SM, ExtraInfo);
      if (IsExternalType != nullptr) {
      } else {
// Recover the field control flag if the Decl is not external(so it
// is passed).
#ifdef DEBUG
        llvm::outs() << "Recovering... " << isInFunctionOldValue << "\n";
#endif
      }
    }
    llvm::outs() << "\n\n---\n\n\n";
  }
}

void MigrateCodeGenerator::handleExternalTypeVD(
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
      if (SM.isInMainFile(VD->getLocation())) {
#ifdef DEBUG
    llvm::outs() << "VarDecl("
                 << ca_utils::getLocationString(SM, VD->getLocation())

                 << "): ^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
#endif
#ifdef CHN
    std::string ExtraInfo = "\n   `" + VD->getType().getAsString() + " " +
                            VD->getNameAsString() + "`\n";
    ExtraInfo += "   - 类型: `外部类型（变量声明）`\n";
    ExtraInfo += "   - 定义路径: `" +
                 ca_utils::getLocationString(SM, VD->getLocation()) + "`\n";
    ExtraInfo += "   - 简介：`<Filled-By-AI>`\n";
    ExtraInfo += "   - 外部类型细节：\n";

#else
    std::string ExtraInfo = "\n### VarDecl: `" + VD->getNameAsString() + "`\n";
    ExtraInfo += "   - Location: `" +
                 ca_utils::getLocationString(SM, VD->getLocation()) + "`\n";
    ExtraInfo += "   - Type: `" + VD->getType().getAsString() + "`\n";
#endif
    if (const auto ParentFuncDeclContext = VD->getParentFunctionOrMethod()) {
      // Notice: Method is only used in C++
      if (const auto ParentFD =
              dyn_cast<clang::FunctionDecl>(ParentFuncDeclContext)) {
#ifdef CHN
#else
        ExtraInfo +=
            "   - Parent: `" + ca_utils::getFuncDeclString(ParentFD) + "`\n";
#endif
      }
    } else if (const auto TU = VD->getTranslationUnitDecl()) {
#ifdef CHN
#else
      ExtraInfo += "   - Parent: Global variable, no parent function.\n";
#endif
    }

    // Output the init expr for the VarDecl
    if (VD->hasInit()) {
      auto InitText =
          clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(
                                          VD->getInit()->getSourceRange()),
                                      SM, LO);
      if (InitText.str() != "" && InitText.str() != "NULL") {
#ifdef CHN
#else
        ExtraInfo += "   - VarDecl Has Init: \n   ```c\n" + InitText.str() +
                     "\n   ```\n";
#endif

      } else {
#ifdef CHN
#else
        ExtraInfo += "   - VarDecl Has Init, but no text found.\n";
#endif
      }
    }

    auto isExternalType = ca_utils::getExternalStructType(
        VD->getType(), llvm::outs(), SM, ExtraInfo);
    if (isExternalType != nullptr) {
    } else {
#ifdef DEBUG
      llvm::outs() << "Recovering... " << isInFunctionOldValue << "\n";
#endif
      // Recover the field control flag if the Decl is not external(so it
      // is passed)
    }
  }
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
    auto MacroDedupName =
        MacroName + "@" +
        ca_utils::getLocationString(SM, tmpStack[tmpStack.size() - 1]);

    // Output the basic info for specific RecordDecl
    std::string BasicInfo = "";
#ifdef CHN
#else
    BasicInfo = "\n### External Macro Integer: `" + MacroName + "`\n";
#endif

// Output the basic location info for the fieldDecl
#ifdef CHN
#else
    BasicInfo +=
        "- Call Location: `" +
        ca_utils::getLocationString(SM, tmpStack[tmpStack.size() - 1]) + "`\n";
    BasicInfo += "- Defined Location: `" +
                 ca_utils::getLocationString(SM, MacroLocation) + "`\n";
#endif

    // TODO: find the parents for integerLiteral
    if (ca_utils::isMacroInteger(MacroName) &&
        MacroDeduplication.find(MacroDedupName) == MacroDeduplication.end()) {
      llvm::outs() << BasicInfo;
      llvm::outs() << "\n\n---\n\n\n";
      MacroDeduplication.insert(MacroDedupName);
    }
  }
}

void MigrateCodeGenerator::handleExternalImplicitCE(
    const clang::ImplicitCastExpr *ICE, clang::SourceManager &SM) {
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
}

void MigrateCodeGenerator::handleExternalCall(const clang::CallExpr *CE,
                                                   clang::SourceManager &SM) {
  // TODO: Fix Macro bugs.
  if (auto FD = CE->getDirectCallee()) {
    // output the basic information of the function declaration
    if (!SM.isInMainFile(FD->getLocation()) &&
        SM.isInMainFile(CE->getBeginLoc())) {
      auto Loc = CE->getBeginLoc();
      ;
      if (!SM.isMacroBodyExpansion(Loc) && !SM.isMacroArgExpansion(Loc)) {
#ifdef CHN
        llvm::outs() << "\n\n   `" << ca_utils::getFuncDeclString(FD) << "`\n";
#else
        llvm::outs() << "\n### External Function Call: ";
        ca_utils::printFuncDecl(FD, SM);
#endif
        if (FD->isInlineSpecified()) {
#ifdef CHN
#else
          llvm::outs() << "   - Function `" << FD->getNameAsString()
                       << "` is declared as inline.\n";
#endif
        }
#ifdef CHN
        llvm::outs() << "   - 类型: `函数`\n"
                     << "   - 定义路径: `" +
                            ca_utils::getLocationString(SM, FD->getLocation()) +
                            "`\n";
#else
        llvm::outs() << "   - Call Location: ";
        ca_utils::printCaller(CE, SM);
#endif
#ifdef CHN
        llvm::outs() << "   - 简介：`<Filled-By-AI>`\n";
#endif

      } else {
        // !!! Handle macro operations
#ifdef CHN
        llvm::outs() << "\n\n   ";
#else
        llvm::outs() << "\n### External Function Call(Macro): ";
#endif
        // ca_utils::printFuncDecl(FD, SM);
        auto CallerLoc = CE->getBeginLoc();
        // Judging whether the caller is expanded from predefined macros.
        std::vector<clang::SourceLocation> tmpStack;
        while (true) {
          if (SM.isMacroBodyExpansion(CallerLoc)) {
            auto ExpansionLoc = SM.getImmediateMacroCallerLoc(CallerLoc);
            CallerLoc = ExpansionLoc;
          } else if (SM.isMacroArgExpansion(CallerLoc)) {
#ifdef DEBUG
            llvm::outs() << "Is in macro arg expansion\n";
#endif
            auto ExpansionLoc =
                SM.getImmediateExpansionRange(CallerLoc).getBegin();
            CallerLoc = ExpansionLoc;
          } else {
            break;
          }
          tmpStack.push_back(CallerLoc);
        }
        assert(tmpStack.size() != 0 && "Macro expansion stack is empty.\n");
        if (tmpStack.size() == 0) {
          llvm::outs() << "(Error!)\n### External Function Call: ";
          ca_utils::printFuncDecl(FD, SM);
          llvm::outs() << "   - Call Location: ";
          ca_utils::printCaller(CE, SM);
          return;
        }
        clang::SourceLocation MacroLocation;
        if (tmpStack.size() == 1) {
          MacroLocation = FD->getLocation();
        } else {
          MacroLocation = tmpStack[tmpStack.size() - 2];
        }
        auto MacroName = ca_utils::getMacroName(SM, MacroLocation);
        llvm::outs() << "`" << MacroName << "`\n";

        if (FD->isInlineSpecified()) {
#ifdef CHN
#else
          llvm::outs() << "   - Function `" << MacroName
                       << "` is declared as inline.\n";
#endif
        }

#ifdef CHN
        llvm::outs() << "   - 类型: `宏`\n"
                     << "   - 定义路径: " +
                            ca_utils::getLocationString(SM, MacroLocation) +
                            "`\n";
#else
        llvm::outs() << "   - Call Location: ";
        ca_utils::printCaller(CE, SM);
#endif
#ifdef CHN
        llvm::outs() << "   - 简介：`<Filled-By-AI>`\n";
#endif
      }
    }
#ifdef DEBUG
    /// Determine whether this declaration came from an AST file (such as
    /// a precompiled header or module) rather than having been parsed.
    llvm::outs() << "----------------------------------------\n";
    if (!FD->isFromASTFile()) {
      llvm::outs() << "Found external function call: "
                   << FD->getQualifiedNameAsString() << "\n";
    }

    if (FD->isExternC()) {
      llvm::outs() << "Found external C function call: "
                   << FD->getQualifiedNameAsString() << "\n";
    }
#endif
  } else {
#ifdef DEBUG
    llvm::outs() << "No function declaration found for call\n";
#endif
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

}
}  // namespace ca

// #endif  // !_CA_AST_HELPERS_HPP