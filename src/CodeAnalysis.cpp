#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/OperationKinds.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/MacroArgs.h"
#include "utils.hpp"

/**********************************************************************
 * 0. Global Infrastructure
 **********************************************************************/
// Basic Infrastructure
std::vector<std::unique_ptr<clang::ASTUnit>> ASTs;
// Apply a custom category to all command-line options so that they are
// the only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("Code-Analysis");

// Setting AST Matchers for call expr
using namespace clang::ast_matchers;
StatementMatcher ExternalCallMatcherPattern =
    callExpr(callee(functionDecl())).bind("externalCall");

// Bind Matcher to ExternalFieldDecl
// Notice: Since ParamVarDecl is the subclass of VarDecl,
//        so they share the same Matcher pattern.
DeclarationMatcher ExternalStructMatcherPattern =
    anyOf(recordDecl().bind("externalTypeFD"), varDecl().bind("externalTypeVD"),
          functionDecl().bind("externalTypeFuncD"));

// Add matchers to expressions
StatementMatcher ExternalExprsMatcherPatter =
    implicitCastExpr().bind("externalImplicitCE");

#ifdef DEPRECATED
// Matcher is not that useful, because it can only match the struct type
// But not the struct pointer
varDecl(hasType(recordDecl())).bind("externalTypeVD"),
varDecl(hasType(isAnyPointer())).bind("externalTypeVD"),
parmVarDecl(hasType(recordDecl())).bind("externalTypePVD"));
#endif

/**********************************************************************
 * 1. Preprocessor Callbacks
 **********************************************************************/
class MacroPPCallbacks : public clang::PPCallbacks {
 public:
  explicit inline MacroPPCallbacks(const clang::CompilerInstance &compiler)
      : compiler(compiler) {
    name = compiler.getSourceManager()
               .getFileEntryForID(compiler.getSourceManager().getMainFileID())
               ->getName();
  }

 private:
  int getMacroExpansionStackDepth(std::string MacroName,
                                  std::string MacroString,
                                  const clang::MacroArgs *Args,
                                  const clang::Preprocessor &PP) {
    std::vector<std::string> CurrMacro;
    CurrMacro.push_back(MacroName);
    CurrMacro.push_back(MacroString);
    if (Args) {
#ifdef DEBUG
      llvm::outs() << Args->getNumMacroArguments() << "\n";
#endif
      for (unsigned I = 0, E = Args->getNumMacroArguments(); I != E; ++I) {
#ifdef DEPRECATED
        // Output the expanded args for macro, may depend on the following
        // expansion of other macros.
        const auto &Arg =
            const_cast<clang::MacroArgs *>(Args)->getPreExpArgument(I, PP);
        // Traverse the Args
        llvm::outs() << "Argument " << I << ": ";
        for (auto &it : Arg) {
          llvm::outs() << PP.getSpelling(it) << " ";
        }
        llvm::outs() << "\n";
#endif
        const auto Arg = Args->getUnexpArgument(I);
#ifdef DEBUG
        llvm::outs() << "Argument " << I << ": " << PP.getSpelling(*Arg)
                     << "\n";
#endif
        CurrMacro.push_back(PP.getSpelling(*Arg));
      }
    }

    if (MacroExpansionStack.size() == 0) {
      MacroExpansionStack.push_back(CurrMacro);
      return 0;
    }
    int itr = MacroExpansionStack.size(), flag = 0;
    while (MacroExpansionStack.size()) {
      for (auto MacroPart : MacroExpansionStack.back()) {
        if (MacroPart.find(MacroName) != std::string::npos) {
          if (MacroPart == MacroExpansionStack.back().front() &&
              MacroPart == MacroName) {
            // If the currentMacro is identical to the macros in stack, return.
            return MacroExpansionStack.size() - 1;
          }
          flag = 1;
          break;
        }
      }
      if (flag) {
        break;
      } else {
        MacroExpansionStack.pop_back();
      }
    }
    MacroExpansionStack.push_back(CurrMacro);
    return MacroExpansionStack.size() - 1;
  }

 public:
  void InclusionDirective(clang::SourceLocation HashLoc,
                          const clang::Token &IncludeTok,
                          clang::StringRef FileName, bool IsAngled,
                          clang::CharSourceRange FilenameRange,
                          clang::OptionalFileEntryRef File,
                          clang::StringRef SearchPath,
                          clang::StringRef RelativePath,
                          const clang::Module *Imported,
                          clang::SrcMgr::CharacteristicKind FileType) override {
    auto &SM = compiler.getSourceManager();
    if (SM.isInMainFile(HashLoc)) {
      llvm::outs() << "InclusionDirective: "
                   << ca_utils::getLocationString(SM, HashLoc) << " "
                   << SearchPath.str() + "/" + RelativePath.str() << "\n";
    }
  }

  void MacroExpands(const clang::Token &MacroNameTok,
                    const clang::MacroDefinition &MD, clang::SourceRange Range,
                    const clang::MacroArgs *Args) override {
    auto &SM = compiler.getSourceManager();
    auto &PP = compiler.getPreprocessor();
    const clang::LangOptions &LO = PP.getLangOpts();
    if (SM.isInMainFile(Range.getBegin())) {
      auto MacroDefinition = ca_utils::getMacroDeclString(MD, SM, LO);
      int MacroDepth = getMacroExpansionStackDepth(PP.getSpelling(MacroNameTok),
                                                   MacroDefinition, Args, PP);
      if (MacroDepth == 0) {
        llvm::outs() << "\n[MPP]Macro: \n";
      }
      llvm::outs().indent(MacroDepth * 3)
          << PP.getSpelling(MacroNameTok) << ", "
          << ca_utils::getLocationString(SM, Range.getBegin()) << "\n";
    }
  }

 private:
  const clang::CompilerInstance &compiler;
  std::string name;

  std::vector<std::vector<std::string>> MacroExpansionStack;
};
class MacroPPOnlyAction : public clang::PreprocessOnlyAction {
#ifdef DEPRECATED
  // The following code is deprecated temporarily. May be useful in the
  // future.
  virtual bool BeginSourceFileAction(CompilerInstance &CI) { return true; }
  virtual void EndSourceFileAction() {}
#endif

 protected:
  void ExecuteAction() override {
    getCompilerInstance().getPreprocessor().addPPCallbacks(
        std::make_unique<MacroPPCallbacks>(getCompilerInstance()));

    clang::PreprocessOnlyAction::ExecuteAction();
  }
};

/**********************************************************************
 * 2. Matcher Callbacks
 **********************************************************************/
class ExternalCallMatcher
    : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  void onStartOfTranslationUnit() override {
#ifdef DEBUG
    llvm::outs() << "In onStartOfTranslationUnit\n";
#endif
    // Clean the map-map-vec
    for (auto &it : FilenameToCallExprs) {
      it.second.clear();
    }
    FilenameToCallExprs.clear();
  }

  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) override {
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
        llvm::outs() << "No function declaration found for call\n";
      }
    } else {
#ifdef DEBUG
      llvm::outs() << "No call or fieldDecl expression found\n";
#endif
    }
  }

  void onEndOfTranslationUnit() override {
#ifdef DEBUG
    llvm::outs() << "In onEndOfTranslationUnit\n";
#endif
    auto &SM = ASTs[0]->getSourceManager();

    llvm::outs() << "# External Function Call Report\n\n";

    // Traverse the FilenameToCallExprs
    int cnt = 0;
    for (auto &it : FilenameToCallExprs) {
      llvm::outs() << "## Header File: " << it.first << "\n";
      llvm::outs() << "- External Function Count: " << it.second.size()
                   << "\n\n";
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

 private:
  std::map<std::string,
           std::map<std::string, std::vector<const clang::CallExpr *>>>
      FilenameToCallExprs;
};

class ExternalStructMatcher
    : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  void onStartOfTranslationUnit() override {
#ifdef DEBUG
    llvm::outs() << "In onStartOfTranslationUnit\n";
#endif
    llvm::outs() << "# External Struct Type Report\n\n";
    llvm::outs() << "## Global Decls: \n";
    externalStructCnt = 0;
    externalVarDeclCnt = 0;
    externalParamVarDeclCnt = 0;
    externalImplicitExprCnt = 0;
    isInFunction = 0;
  }

  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) override {
    auto &SM = *Result.SourceManager;
    int isInFunctionOldValue = isInFunction;
    if (auto FD =
            Result.Nodes.getNodeAs<clang::FunctionDecl>("externalTypeFuncD")) {
      if (SM.isInMainFile(FD->getLocation())) {
#ifdef DEBUG
        llvm::outs() << "FunctionDecl("
                     << ca_utils::getLocationString(SM, FD->getLocation())
                     << "): ^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
#endif

        llvm::outs() << "\n\n---\n\n\n## Function: `"
                     << ca_utils::getFuncDeclString(FD) << "`\n"
                     << "- Function Location: `"
                     << ca_utils::getLocationString(SM, FD->getLocation())
                     << "`\n";
        isInFunction = 1;
#ifdef DEBUG
        llvm::outs() << "Field changes here. FuncDecl 0->1.\n";
#endif

        // Deal with the external return type
        llvm::outs() << "- Return Type: `" + FD->getReturnType().getAsString() +
                            "`\n";
        ca_utils::getExternalStructType(FD->getReturnType(), llvm::outs(), SM,
                                        "");

        // Traverse the FuncDecl's ParamVarDecls
        for (const auto &PVD : FD->parameters()) {
#ifdef DEBUG
          llvm::outs() << "ParamVarDecl("
                       << ca_utils::getLocationString(SM, PVD->getLocation())
                       << "): ^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
#endif
          std::string ExtraInfo =
              "### ParamVarDecl: `" + PVD->getNameAsString() + "`\n";
          ExtraInfo += "   - Location: `" +
                       ca_utils::getLocationString(SM, PVD->getLocation()) +
                       "`\n";
          ExtraInfo += "   - Type: `" + PVD->getType().getAsString() + "`\n";
          if (const auto ParentFuncDeclContext =
                  PVD->getParentFunctionOrMethod()) {
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
            auto InitText = clang::Lexer::getSourceText(
                clang::CharSourceRange::getTokenRange(
                    PVD->getInit()->getSourceRange()),
                SM, Result.Context->getLangOpts());
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
    } else if (auto RD = Result.Nodes.getNodeAs<clang::RecordDecl>(
                   "externalTypeFD")) {
#ifdef DEBUG
      llvm::outs() << "ExternalStructMatcher\n";
      llvm::outs() << FD->getQualifiedNameAsString() << "\n";
      llvm::outs() << FD->getType().getAsString() << " "
                   << FD->getNameAsString() << "\n";
      auto RD = FD->getParent();
      llvm::outs() << "\t" << RD->getQualifiedNameAsString() << "\n";
#endif
      // Dealing with the relationships between RecordDecl and fieldDecl
      // output the basic information of the RecordDecl

      if (!RD->getName().empty() && SM.isInMainFile(RD->getLocation())) {
        // Output the basic info for specific RecordDecl
        std::string BasicInfo = "";

        BasicInfo =
            "### StructDecl: `" + RD->getQualifiedNameAsString() + "`\n";

        // Output the basic location info for the fieldDecl
        BasicInfo += "- Location: `" +
                     ca_utils::getLocationString(SM, RD->getLocation()) + "`\n";

        if (const auto ParentFuncDeclContext =
                RD->getParentFunctionOrMethod()) {
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
    } else if (auto VD =
                   Result.Nodes.getNodeAs<clang::VarDecl>("externalTypeVD")) {
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
        std::string ExtraInfo =
            "### VarDecl: `" + VD->getNameAsString() + "`\n";
        ExtraInfo += "   - Location: `" +
                     ca_utils::getLocationString(SM, VD->getLocation()) + "`\n";
        ExtraInfo += "   - Type: `" + VD->getType().getAsString() + "`\n";
        if (const auto ParentFuncDeclContext =
                VD->getParentFunctionOrMethod()) {
          // Notice: Method is only used in C++
          if (const auto ParentFD =
                  dyn_cast<clang::FunctionDecl>(ParentFuncDeclContext)) {
            ExtraInfo += "   - Parent: `" +
                         ca_utils::getFuncDeclString(ParentFD) + "`\n";
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
                                          SM, Result.Context->getLangOpts());
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
    } else if (auto ICE = Result.Nodes.getNodeAs<clang::ImplicitCastExpr>(
                   "externalImplicitCE")) {
      if (SM.isInMainFile(ICE->getBeginLoc()) &&
          SM.isInMainFile(ICE->getEndLoc())) {
        if (ICE->getCastKind() == clang::CK_LValueToRValue ||
            ICE->getCastKind() == clang::CK_IntegralToPointer) {
#ifdef DEBUG
          llvm::outs() << "ImplicitCastExpr("
                       << ca_utils::getLocationString(SM, ICE->getBeginLoc())
                       << "): " << ICE->getType().getAsString()
                       << "^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
#endif
          std::string ExtraInfo = "";
          ExtraInfo += "### ImplicitCastExpr\n";
          ExtraInfo += "   - Location: " +
                       ca_utils::getLocationString(SM, ICE->getBeginLoc()) +
                       "\n";
          ExtraInfo += "   - CastKind: " + std::string(ICE->getCastKindName());
          int isExternal = ca_utils::getExternalStructType(
              ICE->getType(), llvm::outs(), SM, ExtraInfo);
          if (isExternal) {
            ++externalImplicitExprCnt;
          }
        }
        //  clang::CK_LValueToRValue IntegralToPointer
      }
    }

    else {
#ifdef DEBUG
      llvm::outs() << "No call or fieldDecl expression found\n";
#endif
    }
  }

  void onEndOfTranslationUnit() override {
#ifdef DEBUG
    llvm::outs() << "In onEndOfTranslationUnit\n";
#endif
    llvm::outs() << "\n\n---\n\n\n# Summary\n"
                 << "- External Struct Count: " << externalStructCnt << "\n"
                 << "- External VarDecl Count: " << externalVarDeclCnt << "\n"
                 << "- External ParamVarDecl Count: " << externalParamVarDeclCnt
                 << "\n"
                 << "- External ImplicitExpr Count: " << externalImplicitExprCnt
                 << "\n\n";
  }

 private:
  /*Field control flags*/
  int isInFunction;

  /*Statistics*/
  int externalStructCnt;
  int externalVarDeclCnt;
  int externalParamVarDeclCnt;
  int externalImplicitExprCnt;
};

/**********************************************************************
 * 3. Main Function
 **********************************************************************/
int main(int argc, const char **argv) {
  /*
   * Usage:
   ** ./CodeAnalysis [path to source file]
   */
  if (argc < 2) {
    // std::printf(
    //     "Usage: CodeAnalysis [path to compile_commands.json] [path to
    //     source " "file]\n");
    llvm::outs()
        << "Usage: ./CodeAnalysis [path to source file]\n"
        << "Example: CodeAnalysis ./test.cpp\n"
        << "Notice: 1. The compile_commands.json file should be in the "
           "same "
           "\n"
           "directory as the source file or in the parent directory of "
           "the \n"
           "source file.\n"
        << "        2. The compile_commands.json file should be named as "
           "compile_commands.json.\n"
        << "        3. You can input any number of source file as you "
           "wish.\n";
    return 1;
  }
  llvm::Expected<clang::tooling::CommonOptionsParser> OptionsParser =
      clang::tooling::CommonOptionsParser::create(argc, argv, MyToolCategory,
                                                  llvm::cl::OneOrMore);
  auto compileCommands =
      OptionsParser->getCompilations().getAllCompileCommands();
  auto fileLists = OptionsParser->getSourcePathList();

  // Database can also be imported manually with JSONCompilationDatabase
  clang::tooling::ClangTool Tool(OptionsParser->getCompilations(),
                                 OptionsParser->getSourcePathList());

#ifdef DEBUG
  // Validate the compile commands and source file lists.

  // traverse compile_commands
  llvm::outs() << "[Debug] Validating compile database: "
               << "\n";
  for (auto &it : compileCommands) {
    // Output the filename and directory
    llvm::outs() << "[Debug] Filename: " << it.Filename << "\n";
    llvm::outs() << "[Debug] Directory: " << it.Directory << "\n";
    // llvm::outs() << it.CommandLine << "\n";
    // Output the command line content
    llvm::outs() << "[Debug] Command Line: "
                 << "\n";
    for (auto &it2 : it.CommandLine) {
      llvm::outs() << it2 << "\n";
    }
    break;
  }
  llvm::outs() << "[Debug] End of compile database."
               << "\n";

  llvm::outs() << "[Debug] Validating source file lists: "
               << "\n";
  for (auto &it : fileLists) {
    llvm::outs() << it << "\n";
  }
  llvm::outs() << "[Debug] End of source file lists"
               << "\n";
#endif

  // Prepare the basic infrastructure
  Tool.buildASTs(ASTs);

  int Result = Tool.run(
      clang::tooling::newFrontendActionFactory<MacroPPOnlyAction>().get());

  /*
  // Comments while testing preprocessor callbacks.
  ExternalCallMatcher exCallMatcher;
  ExternalStructMatcher exStructMatcher;
  clang::ast_matchers::MatchFinder Finder;
  Finder.addMatcher(ExternalStructMatcherPattern, &exStructMatcher);
  Finder.addMatcher(ExternalExprsMatcherPatter, &exStructMatcher);
  Finder.addMatcher(ExternalCallMatcherPattern, &exCallMatcher);
  int status =
      Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());

  return status;
  */
  return 0;
}