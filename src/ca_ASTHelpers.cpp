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
#include "clang/ASTMatchers/ASTMatchers.h"
// #include "clang/Lex/MacroArgs.h"
// #include "ca_utils.hpp"
// #include "ca_PreprocessorHelpers.hpp"
#include <clang/Tooling/CompilationDatabase.h>

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
          assert(PLoc.isValid() &&
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

  llvm::outs() << "# External Function Call Report\n\n";

  // Traverse the FilenameToCallExprs
  int cnt = 0;
  for (auto &it : FilenameToCallExprs) {
    llvm::outs() << "## Header File: " << it.first << "\n";
    llvm::outs() << "- External Function Count: " << it.second.size() << "\n\n";
    int file_cnt = 0;
    for (auto &it2 : it.second) {
      auto FD = it2.first;
      llvm::outs() << ++file_cnt << ". ";
      int caller_cnt = 0;

      for (auto &it3 : it2.second) {
        if (!caller_cnt) {
          auto FD = it3->getDirectCallee();
          ca_utils::printFuncDecl(FD, SM);
          llvm::outs() << "   - Caller Counts: **" << it2.second.size()
                       << "**, details:\n";
        }
        llvm::outs() << "      " << ++caller_cnt << ". ";
        ca_utils::printCaller(it3, SM);
      }
      llvm::outs() << "\n";
      ++cnt;
    }

    llvm::outs() << "---\n\n";
  }

  llvm::outs() << "# Summary\n"
               << "- External Function Call Count: " << cnt << "\n";
}

void ExternalDependencyMatcher::onStartOfTranslationUnit() {
#ifdef DEBUG
  llvm::outs() << "In onStartOfTranslationUnit\n";
#endif
  llvm::outs() << "# External Dependencies Report\n\n";
  llvm::outs() << "## Global: \n";
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

    llvm::outs() << "\n\n---\n\n\n## Function: `"
                 << ca_utils::getFuncDeclString(FD) << "`\n"
                 << "- Function Location: `"
                 << ca_utils::getLocationString(SM, FD->getLocation()) << "`\n";
    isInFunction = 1;
#ifdef DEBUG
    llvm::outs() << "Field changes here. FuncDecl 0->1.\n";
#endif

    // Deal with the external return type
    llvm::outs() << "- Return Type: `" + FD->getReturnType().getAsString() +
                        "`\n";
    ca_utils::getExternalStructType(FD->getReturnType(), llvm::outs(), SM, "");

    // Traverse the FuncDecl's ParamVarDecls
    for (const auto &PVD : FD->parameters()) {
#ifdef DEBUG
      llvm::outs() << "ParamVarDecl("
                   << ca_utils::getLocationString(SM, PVD->getLocation())
                   << "): ^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
#endif
      std::string ExtraInfo =
          "\n### ParamVarDecl: `" + PVD->getNameAsString() + "`\n";
      ExtraInfo += "   - Location: `" +
                   ca_utils::getLocationString(SM, PVD->getLocation()) + "`\n";
      ExtraInfo += "   - Type: `" + PVD->getType().getAsString() + "`\n";
      if (const auto ParentFuncDeclContext = PVD->getParentFunctionOrMethod()) {
        // Notice: Method is only used in C++
        if (const auto ParentFD =
                dyn_cast<clang::FunctionDecl>(ParentFuncDeclContext)) {
          ExtraInfo += "   - Function: `" +
                       ca_utils::getFuncDeclString(ParentFD) + "`\n";
        }
      } else if (const auto TU = PVD->getTranslationUnitDecl()) {
        ExtraInfo += "   - Parent: Global variable, no parent function.\n";
      }

      // Output the init expr for the VarDecl
      if (PVD->hasInit()) {
        auto InitText =
            clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(
                                            PVD->getInit()->getSourceRange()),
                                        SM, LO);
        if (InitText.str() != "" && InitText.str() != "NULL") {
          ExtraInfo += "   - ParamVarDecl Has Init: \n   ```c\n" +
                       InitText.str() + "\n   ```\n";

        } else {
          ExtraInfo += "   - ParamVarDecl Has Init, but no text found.\n";
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

    BasicInfo = "\n### StructDecl: `" + RD->getQualifiedNameAsString() + "`\n";

    // Output the basic location info for the fieldDecl
    BasicInfo += "- Location: `" +
                 ca_utils::getLocationString(SM, RD->getLocation()) + "`\n";

    if (const auto ParentFuncDeclContext = RD->getParentFunctionOrMethod()) {
      // Notice: Method is only used in C++
      if (const auto ParentFD =
              dyn_cast<clang::FunctionDecl>(ParentFuncDeclContext)) {
        BasicInfo +=
            "- Parent: `" + ca_utils::getFuncDeclString(ParentFD) + "`\n";
      }
    } else if (const auto TU = RD->getTranslationUnitDecl()) {
      BasicInfo += "- Parent: Global variable, no parent function.\n";
      if (isInFunction) {
#ifdef DEBUG
        llvm::outs() << "-Field changes here. StructDecl 1->0.\n";
#endif
        isInFunctionOldValue = isInFunction;
        isInFunction = 0;
        BasicInfo = "\n\n---\n\n\n## Global: \n" + BasicInfo;
      }
    }
    llvm::outs() << BasicInfo;

    // Output the full definition for the fieldDecl
    llvm::outs() << "- Full Definition: \n"
                 << "```c\n";
    RD->print(llvm::outs(), clang::PrintingPolicy(clang::LangOptions()));
    llvm::outs() << "\n```\n";

    // Traverse its fieldDecl and find external struct member
    llvm::outs() << "- External Struct Members: \n";
    for (const auto &FD : RD->fields()) {
#ifdef DEBUG
      llvm::outs() << "\t" << FD->getType().getAsString() << " "
                   << FD->getNameAsString() << " "
                   << FDType->isStructureOrClassType() << "\n";
#endif
      std::string ExtraInfo = "";
      ExtraInfo += "   - Member: `" + FD->getType().getAsString() + " " +
                   FD->getNameAsString() + "`\n";
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
    std::string ExtraInfo = "\n### VarDecl: `" + VD->getNameAsString() + "`\n";
    ExtraInfo += "   - Location: `" +
                 ca_utils::getLocationString(SM, VD->getLocation()) + "`\n";
    ExtraInfo += "   - Type: `" + VD->getType().getAsString() + "`\n";
    if (const auto ParentFuncDeclContext = VD->getParentFunctionOrMethod()) {
      // Notice: Method is only used in C++
      if (const auto ParentFD =
              dyn_cast<clang::FunctionDecl>(ParentFuncDeclContext)) {
        ExtraInfo +=
            "   - Parent: `" + ca_utils::getFuncDeclString(ParentFD) + "`\n";
      }
    } else if (const auto TU = VD->getTranslationUnitDecl()) {
      ExtraInfo += "   - Parent: Global variable, no parent function.\n";
      if (isInFunction) {
#ifdef DEBUG
        llvm::outs() << "--Field changes here, VarDecl 1->0.\n";
#endif
        isInFunctionOldValue = isInFunction;
        isInFunction = 0;
        ExtraInfo = "\n\n---\n\n\n## Global: \n" + ExtraInfo;
      }
    }

    // Output the init expr for the VarDecl
    if (VD->hasInit()) {
      auto InitText =
          clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(
                                          VD->getInit()->getSourceRange()),
                                      SM, LO);
      if (InitText.str() != "" && InitText.str() != "NULL") {
        ExtraInfo += "   - VarDecl Has Init: \n   ```c\n" + InitText.str() +
                     "\n   ```\n";

      } else {
        ExtraInfo += "   - VarDecl Has Init, but no text found.\n";
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
      ExtraInfo += "\n### ImplicitCastExpr\n";
      ExtraInfo += "   - Location: " +
                   ca_utils::getLocationString(SM, ICE->getBeginLoc()) + "\n";
      ExtraInfo +=
          "   - CastKind: " + std::string(ICE->getCastKindName()) + "\n";
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
  if (auto FD = CE->getDirectCallee()) {
    // output the basic information of the function declaration
    if (!SM.isInMainFile(FD->getLocation()) &&
        SM.isInMainFile(CE->getBeginLoc())) {
      llvm::outs() << "\n### External Function Call: ";
      ca_utils::printFuncDecl(FD, SM);
      llvm::outs() << "   - Call Location: ";
      ca_utils::printCaller(CE, SM);
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
    handleExternalTypeVD(VD, SM, LO, isInFunctionOldValue);
  } else if (auto ICE = Result.Nodes.getNodeAs<clang::ImplicitCastExpr>(
                 "externalImplicitCE")) {
    handleExternalImplicitCE(ICE, SM);
  } else if (auto CE =
                 Result.Nodes.getNodeAs<clang::CallExpr>("externalCall")) {
    handleExternalCall(CE, SM);
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
  llvm::outs() << "\n\n---\n\n\n# Summary\n"
               << "- External Struct Count: " << externalStructCnt << "\n"
               << "- External VarDecl Count: " << externalVarDeclCnt << "\n"
               << "- External ParamVarDecl Count: " << externalParamVarDeclCnt
               << "\n"
               << "- External ImplicitExpr Count: " << externalImplicitExprCnt
               << "\n"
               << "- External Function Call Count: " << externalFunctionCallCnt
               << "\n\n";
}

int ModuleAnalysisHelper(std::string sourceFiles) {
  /*
    0. Handle the source file paths
  */
  std::vector<std::string> SourcePaths;
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

  /*
    1. Basic infrastructures
  */
  std::string ErrMsg;
  std::vector<std::unique_ptr<clang::ASTUnit>> ASTs;
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
    ExternalCallMatcher exCallMatcher(ASTs[0]->getSourceManager());
    clang::ast_matchers::MatchFinder Finder;
    Finder.addMatcher(ExternalCallMatcherPattern, &exCallMatcher);
    returnStatus *=
        Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());
  }

  return returnStatus;
}

}  // namespace ca

// #endif  // !_CA_AST_HELPERS_HPP