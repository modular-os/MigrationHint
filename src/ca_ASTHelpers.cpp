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
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
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
void ExternalCallMatcher::onStartOfTranslationUnit() {
#ifdef DEBUG
  llvm::outs() << "In onStartOfTranslationUnit\n";
#endif
  // Clean the map-map-vec
  for (auto &it : FilenameToCallExprs) {
    it.second.clear();
  }
  FilenameToCallExprs.clear();
}

void ExternalCallMatcher::run(
    const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  auto &SM = *Result.SourceManager;
  if (auto CE = Result.Nodes.getNodeAs<clang::CallExpr>("externalCall")) {
    if (auto FD = CE->getDirectCallee()) {
      // output the basic information of the function declaration
      if (!SM.isInMainFile(FD->getLocation()) &&
          SM.isInMainFile(CE->getBeginLoc())) {
        auto CallerLoc = CE->getBeginLoc();
        if (SM.isMacroBodyExpansion(CallerLoc) ||
            SM.isMacroArgExpansion(CallerLoc)) {
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
            MacroLocation = tmpStack[tmpStack.size() - 2];
          }
#ifdef CHN
          auto MacroName =
              ca_utils::getMacroName(SM, tmpStack[tmpStack.size() - 1]) +
              "(宏)`" + ca_utils::getLocationString(SM, MacroLocation) + "`";
#else
          auto MacroName =
              ca_utils::getMacroName(SM, tmpStack[tmpStack.size() - 1]) +
              "(Macro)`" + ca_utils::getLocationString(SM, MacroLocation) + "`";
#endif
          // auto Loc = FD->getLocation();
          // Get the spelling location for Loc
          auto SLoc = SM.getSpellingLoc(MacroLocation);
          std::string FilePath = SM.getFilename(SLoc).str();

          if (FilePath == "") {
            // Couldn't get the spelling location, try to get the presumed
            // location
#if DEBUG
            llvm::outs << "Couldn't get the spelling location, try to get the "
                          "presumed "
                          "location\n";
#endif
            auto PLoc = SM.getPresumedLoc(MacroLocation);
            assert(
                PLoc.isValid() &&
                "Caller's Presumed location in the source file is invalid\n");
            FilePath = PLoc.getFilename();
            assert(FilePath != "" &&
                   "Caller's location in the source file is invalid.");
          }

          // auto FuncDeclStr = ca_utils::getFuncDeclString(FD);

          if (FilenameToCallExprs.find(FilePath) == FilenameToCallExprs.end()) {
            FilenameToCallExprs[FilePath] =
                std::map<std::string, std::vector<const clang::CallExpr *>>();
          }
          if (FilenameToCallExprs[FilePath].find(MacroName) ==
              FilenameToCallExprs[FilePath].end()) {
            FilenameToCallExprs[FilePath][MacroName] =
                std::vector<const clang::CallExpr *>();
          }
          FilenameToCallExprs[FilePath][MacroName].push_back(CE);
        } else {
          auto Loc = FD->getLocation();
          // Get the spelling location for Loc
          auto SLoc = SM.getSpellingLoc(Loc);
          std::string FilePath = SM.getFilename(SLoc).str();

          if (FilePath == "") {
            // Couldn't get the spelling location, try to get the presumed
            // location
#if DEBUG
            llvm::outs << "Couldn't get the spelling location, try to get the "
                          "presumed "
                          "location\n";
#endif
            auto PLoc = SM.getPresumedLoc(Loc);
            assert(
                PLoc.isValid() &&
                "Caller's Presumed location in the source file is invalid\n");
            FilePath = PLoc.getFilename();
            assert(FilePath != "" &&
                   "Caller's location in the source file is invalid.");
          }

          auto FuncDeclStr = ca_utils::getFuncDeclString(FD);

          if (FilenameToCallExprs.find(FilePath) == FilenameToCallExprs.end()) {
            FilenameToCallExprs[FilePath] =
                std::map<std::string, std::vector<const clang::CallExpr *>>();
          }
          if (FilenameToCallExprs[FilePath].find(FuncDeclStr) ==
              FilenameToCallExprs[FilePath].end()) {
            FilenameToCallExprs[FilePath][FuncDeclStr] =
                std::vector<const clang::CallExpr *>();
          }
          FilenameToCallExprs[FilePath][FuncDeclStr].push_back(CE);
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
  } else {
#ifdef DEBUG
    llvm::outs() << "No call or fieldDecl expression found\n";
#endif
  }
}

void ExternalCallMatcher::onEndOfTranslationUnit() {
#ifdef DEBUG
  llvm::outs() << "In onEndOfTranslationUnit\n";
#endif
  auto &SM = AST_SM;
  if (mode == Print) {
#ifdef CHN
    llvm::outs() << "# 外部函数调用报告\n\n";
#else
    llvm::outs() << "# External Function Call Report\n\n";
#endif

    // Traverse the FilenameToCallExprs
    int cnt = 0;
    for (auto &it : FilenameToCallExprs) {
#ifdef CHN
      llvm::outs() << "## 头文件: " << it.first << "\n";
      llvm::outs() << "- 外部函数调用次数: " << it.second.size() << "\n\n";
#else
      llvm::outs() << "## Header File: " << it.first << "\n";
      llvm::outs() << "- External Function Count: " << it.second.size()
                   << "\n\n";
#endif
      int file_cnt = 0;
      for (auto &it2 : it.second) {
        auto FD = it2.first;
        llvm::outs() << ++file_cnt << ". ";
        int caller_cnt = 0;

        for (auto &it3 : it2.second) {
          if (!caller_cnt) {
            auto CallerLoc = it3->getBeginLoc();
            if (SM.isMacroBodyExpansion(CallerLoc) ||
                SM.isMacroArgExpansion(CallerLoc)) {
              const auto &MacroNameWithLoc = it2.first;
              int pos = 0;
              pos = MacroNameWithLoc.find('`');
              llvm::outs() << "`" << MacroNameWithLoc.substr(0, pos) << "`\n";
#ifdef CHN
              llvm::outs() << "   - 位置: "
                           << MacroNameWithLoc.substr(
                                  pos, MacroNameWithLoc.size() - pos)
                           << "\n";
#else
              llvm::outs() << "   - Location: "
                           << MacroNameWithLoc.substr(
                                  pos, MacroNameWithLoc.size() - pos)
                           << "\n";
#endif
            } else {
              auto FD = it3->getDirectCallee();
              ca_utils::printFuncDecl(FD, SM);
            }
#ifdef CHN
            llvm::outs() << "   - 调用次数: **" << it2.second.size()
                         << "**, 详情:\n";
#else
            llvm::outs() << "   - Caller Counts: **" << it2.second.size()
                         << "**, details:\n";
#endif
          }
          llvm::outs() << "      " << ++caller_cnt << ". ";
          ca_utils::printCaller(it3, SM);
        }
        llvm::outs() << "\n";
        ++cnt;
      }

      llvm::outs() << "---\n\n";
    }

#ifdef CHN
    llvm::outs() << "# 总结\n"
                 << "- 外部函数调用次数: " << cnt << "\n";
#else
    llvm::outs() << "# Summary\n"
                 << "- External Function Call Count: " << cnt << "\n";
#endif
  } else if (mode == Collect) {
    // llvm::outs() << "Hi" << "\n";
    for (auto &it : FilenameToCallExprs) {
      auto HeaderName = it.first;
      if ((*ModuleFunctionCallCnt).find(HeaderName) ==
          (*ModuleFunctionCallCnt).end()) {
        (*ModuleFunctionCallCnt)[HeaderName] = std::map<std::string, int>();
      }
      for (auto &it2 : it.second) {
        auto FD = it2.first;
        std::string FuncNameWithLoc = "";
        int callerCnt = 0;
        for (auto &it3 : it2.second) {
          if (!callerCnt) {
            auto CallerLoc = it3->getBeginLoc();
            if (SM.isMacroBodyExpansion(CallerLoc) ||
                SM.isMacroArgExpansion(CallerLoc)) {
              FuncNameWithLoc = it2.first;
            } else {
              auto FD = it3->getDirectCallee();
              FuncNameWithLoc +=
                  ca_utils::getFuncDeclString(FD) + "`" +
                  ca_utils::getLocationString(SM, FD->getLocation()) + "`";
            }
          }
          ++callerCnt;
        }
        if ((*ModuleFunctionCallCnt)[HeaderName].find(FuncNameWithLoc) ==
            (*ModuleFunctionCallCnt)[HeaderName].end()) {
          (*ModuleFunctionCallCnt)[HeaderName][FuncNameWithLoc] = 0;
        }
        (*ModuleFunctionCallCnt)[HeaderName][FuncNameWithLoc] +=
            1 * 100000 + callerCnt;
        /*
        // Former Metrics
        callerCnt * 100000 + 1;
        */
      }
    }
  } else {
    assert(false && "Unkown work type for ExternalDependencyMatcher.\n");
  }
}

void ExternalDependencyMatcher::onStartOfTranslationUnit() {
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
  externalStructCnt = 0;
  externalVarDeclCnt = 0;
  externalParamVarDeclCnt = 0;
  externalImplicitExprCnt = 0;
  externalFunctionCallCnt = 0;
  isInFunction = 0;
}

void ExternalDependencyMatcher::handleExternalTypeFuncD(
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
                   << ca_utils::getMacroName(SM, tmpStack[tmpStack.size() - 1])
                   << "`\n"
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
    isInFunction = 1;
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
      if (isExternalType) {
        ++externalParamVarDeclCnt;
      }
    }
  }
}

void ExternalDependencyMatcher::handleExternalTypeFD(
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
      if (isInFunction) {
#ifdef DEBUG
        llvm::outs() << "-Field changes here. StructDecl 1->0.\n";
#endif
        isInFunctionOldValue = isInFunction;
        isInFunction = 0;
#ifdef CHN
#else
        BasicInfo = "\n\n---\n\n\n## Global: \n" + BasicInfo;
#endif
      }
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
      bool IsExternalType = ca_utils::getExternalStructType(
          FD->getType(), llvm::outs(), SM, ExtraInfo);
      if (IsExternalType) {
        ++externalStructCnt;
      } else {
// Recover the field control flag if the Decl is not external(so it
// is passed).
#ifdef DEBUG
        llvm::outs() << "Recovering... " << isInFunctionOldValue << "\n";
#endif
        isInFunction = isInFunctionOldValue;
      }
    }
    llvm::outs() << "\n\n---\n\n\n";
  }
}

void ExternalDependencyMatcher::handleExternalTypeVD(
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
      if (isInFunction) {
#ifdef DEBUG
        llvm::outs() << "--Field changes here, VarDecl 1->0.\n";
#endif
        isInFunctionOldValue = isInFunction;
        isInFunction = 0;
#ifdef CHN
        ExtraInfo = "\n\n---\n\n\n## 全局: \n" + ExtraInfo;
#else
        ExtraInfo = "\n\n---\n\n\n## Global: \n" + ExtraInfo;
#endif
      }
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
    if (isExternalType) {
      ++externalVarDeclCnt;
    } else {
#ifdef DEBUG
      llvm::outs() << "Recovering... " << isInFunctionOldValue << "\n";
#endif
      // Recover the field control flag if the Decl is not external(so it
      // is passed)
      isInFunction = isInFunctionOldValue;
    }
  }
}

void ExternalDependencyMatcher::handleExternalImplicitCE(
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
      int isExternal = ca_utils::getExternalStructType(
          ICE->getType(), llvm::outs(), SM, ExtraInfo);
      if (isExternal) {
        ++externalImplicitExprCnt;
      }
    }
  }
}

void ExternalDependencyMatcher::handleExternalCall(const clang::CallExpr *CE,
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
        auto MacroName =
            ca_utils::getMacroName(SM, tmpStack[tmpStack.size() - 1]);
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
      ++externalFunctionCallCnt;
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

void ExternalDependencyMatcher::run(
    const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  auto &SM = *Result.SourceManager;
  auto &LO = Result.Context->getLangOpts();
  int isInFunctionOldValue = isInFunction;
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
  } else if (auto ILS = Result.Nodes.getNodeAs<clang::IntegerLiteral>(
                 "integerLiteral")) {
    auto Loc = ILS->getBeginLoc();
    if (SM.isInMainFile(Loc)) {
      if (SM.isMacroBodyExpansion(Loc)) {
        auto IntLoc = ca_utils::getLocationString(SM, Loc);
        llvm::outs() << IntLoc << "\n";
        ILS->getExprStmt()->dump(llvm::outs(), *Result.Context);
      }
    }
    // Do nothing
  } else if (auto RS =
                 Result.Nodes.getNodeAs<clang::ReturnStmt>("returnStmt")) {
    auto Loc = RS->getReturnLoc();
    if (SM.isInMainFile(Loc)) {
      // auto IntLoc = ca_utils::getLocationString(SM, Loc);
      llvm::outs() << ca_utils::getLocationString(SM, Loc) << "\n";
      if (RS->getRetValue() != nullptr) {
        // print the return expr type like IntegerLiteral
        auto RetValue = RS->getRetValue();
        RetValue->getExprStmt()->dump(llvm::outs(), *Result.Context);
      }
    }
    // Do nothing
  } else {
#ifdef DEBUG
    llvm::outs() << "No call or fieldDecl expression found\n";
#endif
  }
}

void ExternalDependencyMatcher::onEndOfTranslationUnit() {
#ifdef DEBUG
  llvm::outs() << "In onEndOfTranslationUnit\n";
#endif

#ifdef CHN
  llvm::outs() << "\n\n---\n\n\n# 总结\n"
               << "- 外部结构体定义: " << externalStructCnt << "\n"
               << "- 外部变量定义: " << externalVarDeclCnt << "\n"
               << "- 外部函数参数定义: " << externalParamVarDeclCnt << "\n"
               << "- 外部隐式转换: " << externalImplicitExprCnt << "\n"
               << "- 外部函数调用: " << externalFunctionCallCnt << "\n\n";
#else
  llvm::outs() << "\n\n---\n\n\n# Summary\n"
               << "- External Struct Count: " << externalStructCnt << "\n"
               << "- External VarDecl Count: " << externalVarDeclCnt << "\n"
               << "- External ParamVarDecl Count: " << externalParamVarDeclCnt
               << "\n"
               << "- External ImplicitExpr Count: " << externalImplicitExprCnt
               << "\n"
               << "- External Function Call Count: " << externalFunctionCallCnt
               << "\n\n";
#endif
}

int ModuleAnalysisHelper(std::string sourceFiles) {
  /*
    0. Handle the source file paths
  */
  std::vector<std::string> SourcePaths;
  int BasicFunctionThreshold = 0;
  int pos = 0, previousPos = 0;
  while ((pos = sourceFiles.find(",", pos)) != std::string::npos) {
    auto source = sourceFiles.substr(previousPos, pos - previousPos);
    SourcePaths.push_back(source);
    ++pos;
    previousPos = pos;
  }
  auto source =
      sourceFiles.substr(previousPos, sourceFiles.size() - previousPos);
  SourcePaths.push_back(source);
  BasicFunctionThreshold = std::ceil(SourcePaths.size() * 0.8);

  /*
    1. Basic infrastructures
  */
  std::string ErrMsg;
  std::vector<std::unique_ptr<clang::ASTUnit>> ASTs;
  std::map<std::string, std::map<std::string, int>> CollectResults;
  int returnStatus = 1;
  using namespace clang::ast_matchers;
  StatementMatcher ExternalCallMatcherPattern = callExpr().bind("externalCall");
  /*
  TODO: Weird bugs to include path of ast matcher
      StatementMatcher ExternalCallMatcherPattern =
      callExpr(callee(FunctionDecl())).bind("externalCall");
  */

  /*
    2. Begin handling modules
  */
  for (auto &SourcePath : SourcePaths) {
#ifdef DEBUG
    llvm::outs() << source << "\n";
#endif
    auto CompileDatabase =
        clang::tooling::CompilationDatabase::autoDetectFromSource(SourcePath,
                                                                  ErrMsg);
    clang::tooling::ClangTool Tool(*CompileDatabase, SourcePath);
    ASTs.clear();
    Tool.buildASTs(ASTs);
    if (ASTs.size() == 0) {
      llvm::outs() << "Error! No AST found for " << SourcePath << "\n";
      exit(1);
    }
    ExternalCallMatcher exCallMatcher(ASTs[0]->getSourceManager(),
                                      ca::ExternalCallMatcher::Collect,
                                      &CollectResults);
    clang::ast_matchers::MatchFinder Finder;
    Finder.addMatcher(ExternalCallMatcherPattern, &exCallMatcher);
    returnStatus *=
        Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());
  }

  /*
    3. Output the results
  */

#ifdef CHN
  llvm::outs() << "# 外部模块调用报告\n\n";
#else
  llvm::outs() << "# External Module Call Report\n\n";
#endif

  // Traverse the FilenameToCallExprs
  int cnt = 0;
  for (auto &it : CollectResults) {
    int file_cnt = 0;
    // for (auto &it2 : it.second) {
    //   auto FD = it2.first;
    //   llvm::outs() << ++file_cnt << ". "
    //                << "`" << it2.first << "`: `" << it2.second << "`\n";
    //   ++cnt;
    // }
    std::vector<std::pair<std::string, int>> sortedFiles(it.second.begin(),
                                                         it.second.end());
    std::sort(sortedFiles.begin(), sortedFiles.end(),
              [](const std::pair<std::string, int> &a,
                 const std::pair<std::string, int> &b) {
                return a.second > b.second;
              });
    for (auto &it2 : sortedFiles) {
      if ((it2.second / 100000) < BasicFunctionThreshold) {
        continue;
      }
      const auto &MacroNameWithLoc = it2.first;
      int pos = 0;
      pos = MacroNameWithLoc.find('`');
      if (file_cnt == 0) {
#ifdef CHN
        llvm::outs() << "## 头文件: " << it.first << "\n";
#else
        llvm::outs() << "## Header File: " << it.first << "\n";
#endif
      }
#ifdef CHN
      llvm::outs() << ++file_cnt << ". "
                   << "`" << MacroNameWithLoc.substr(0, pos) << "`\n"
                   << "   - 位置: "
                   << MacroNameWithLoc.substr(pos,
                                              MacroNameWithLoc.size() - pos)
                   << "\n"
                   << "   - 调用次数: `" << it2.second / 100000 << "`\n"
                   << "   - 调用频率: `" << it2.second % 100000 << "`\n"
                   << "   - 调用函数: `"
                   << "<Filled-By-AI>"
                   << "`\n\n";
#else
      llvm::outs() << ++file_cnt << ". "
                   << "`" << MacroNameWithLoc.substr(0, pos) << "`\n"
                   << "   - Location: "
                   << MacroNameWithLoc.substr(pos,
                                              MacroNameWithLoc.size() - pos)
                   << "\n"
                   << "   - Times: `" << it2.second / 100000 << "`\n"
                   << "   - Frequency: `" << it2.second % 100000 << "`\n"
                   << "   - Description: `"
                   << "<Filled-By-AI>"
                   << "`\n\n";
#endif
    }
    if (file_cnt) {
      llvm::outs() << "---\n\n";
    }
  }

  return returnStatus;
}

void generateReport(std::string SourceFilePath) {
  clang::ast_matchers::MatchFinder Finder;
  using namespace clang::ast_matchers;
  std::string ErrMsg;
  auto CompileDatabase =
      clang::tooling::CompilationDatabase::autoDetectFromSource(SourceFilePath,
                                                                ErrMsg);
  clang::tooling::ClangTool Tool(*CompileDatabase, SourceFilePath);
  DeclarationMatcher ReportMatcherPattern =
      anyOf(recordDecl().bind("recDecl"), varDecl().bind("varDecl"),
            functionDecl().bind("funcDecl"));
  ca::ReportMatcher Matcher;
  Finder.addMatcher(ReportMatcherPattern, &Matcher);

  llvm::outs() << "#### 细节分析\n"
               << "\n---\n\n"
               << "> **" << ca_utils::getCoreFileNameFromPath(SourceFilePath)
               << "**\n"
               << "\n---\n\n";

  Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());
}

void ReportMatcher::run(
    const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  auto &SM = *Result.SourceManager;
  auto &LO = Result.Context->getLangOpts();
  if (auto VD = Result.Nodes.getNodeAs<clang::VarDecl>("varDecl")) {
    auto Loc = VD->getLocation();
    if (SM.isInMainFile(Loc) && !SM.isMacroBodyExpansion(Loc) &&
        !SM.isMacroArgExpansion(Loc)) {
      if (VD->getParentFunctionOrMethod() == nullptr) {
        llvm::outs() << "\n```c\n"
                     << VD->getType().getAsString() << " "
                     << VD->getNameAsString() << "\n```\n";
        llvm::outs() << "- **功能描述**\n  <filled-by-ai>\n\n";
      }
    }
  } else if (auto FD =
                 Result.Nodes.getNodeAs<clang::FunctionDecl>("funcDecl")) {
    auto Loc = FD->getLocation();
    if (SM.isInMainFile(Loc)) {
      if (FD->getParentFunctionOrMethod() == nullptr) {
        if (!SM.isMacroBodyExpansion(Loc) && !SM.isMacroArgExpansion(Loc)) {
          if (!FD->hasBody() ||
              (FD->hasBody() && FD->doesThisDeclarationHaveABody())) {
            llvm::outs() << "\n```c\n"
                         << ca_utils::getFuncDeclString(FD) << "\n```\n";
            llvm::outs()
                << "- **功能描述**\n  <filled-by-ai>\n\n"
                << "- **核心逻辑**\n  <filled-by-ai>\n\n"
                << "- **外部能力使用**\n  <filled-by-CodeAnalysis>\n\n";
          }
        } else {
          // Handle the macro expansion
          // llvm::outs() << "\n```MACRO\n"
          //              << ca_utils::getFuncDeclString(FD) << "\n```\n";

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
            MacroLocation = tmpStack[tmpStack.size() - 2];
          }
#ifdef CHN
          auto MacroName =
              ca_utils::getMacroName(SM, tmpStack[tmpStack.size() - 1]) +
              "(宏)";
#else
          auto MacroName =
              ca_utils::getMacroName(SM, tmpStack[tmpStack.size() - 1]) +
              "(Macro)`" + ca_utils::getLocationString(SM, MacroLocation) + "`";
#endif
          llvm::outs() << "\n```c\n" << MacroName << "\n```\n";
          llvm::outs() << "- **功能描述**\n  <filled-by-ai>\n\n"
                       << "- **核心逻辑**\n  <filled-by-ai>\n\n"
                       << "- **外部能力使用**\n  <filled-by-CodeAnalysis>\n\n";
        }
      }
    }
  } else if (auto RD = Result.Nodes.getNodeAs<clang::RecordDecl>("recDecl")) {
    auto Loc = RD->getLocation();
    if (SM.isInMainFile(Loc) && !SM.isMacroBodyExpansion(Loc) &&
        !SM.isMacroArgExpansion(Loc)) {
      if (RD->getParentFunctionOrMethod() == nullptr) {
        llvm::outs() << "\n```c\n";
        RD->print(llvm::outs(), clang::PrintingPolicy(clang::LangOptions()));
        llvm::outs() << "\n```\n";
        llvm::outs() << "- **功能描述**\n  <filled-by-ai>\n\n";
      }
    }
  }
}
}  // namespace ca

// #endif  // !_CA_AST_HELPERS_HPP