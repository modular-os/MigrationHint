#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <string>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
// using namespace clang;
// using namespace clang::tooling;
// using namespace llvm;

// class HeadFileDependencyAction : public clang::ASTFrontendAction {
// public:
//     virtual std::unique_ptr<clang::ASTConsumer>
//     CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef file)
//     override {
//         return
//         std::make_unique<HeadFileDependencyAction>(CI.getSourceManager());
//     }
// };

// class HeadFileDependencyConsumer : public clang::ASTConsumer {
// public:
//     explicit HeadFileDependencyConsumer(clang::SourceManager &SM) : SM(SM) {}

//     virtual void HandleTranslationUnit(clang::ASTContext &Context) override {
//         for (const auto &F : Context.getTranslationUnitDecl()->decls()) {
//             if (const auto *ID = dyn_cast<clang::InclusionDirective>(F)) {
//                 auto FileName = ID->getFileName();
//                 llvm::outs() << FileName.str() << "\n";
//             }
//         }
//     }

// private:
//     clang::SourceManager &SM;
// };

// Basic Infrastructure
std::vector<std::unique_ptr<clang::ASTUnit>> ASTs;

void printFuncDecl(const clang::FunctionDecl *FD,
                   const clang::SourceManager &SM) {
  auto Loc = FD->getLocation();
  std::string FilePath = SM.getFilename(Loc).str();
  unsigned LineNumber = SM.getSpellingLineNumber(Loc);
  unsigned ColumnNumber = SM.getSpellingColumnNumber(Loc);

  llvm::outs() << FilePath << ":" << LineNumber << ":" << ColumnNumber << " ";

  llvm::outs() << FD->getReturnType().getAsString() << " "
               << FD->getNameAsString() << "(";
  if (int paramNum = FD->getNumParams()) {
    for (auto &it : FD->parameters()) {
      llvm::outs() << it->getType().getAsString();
      if (it->getNameAsString() != "") {
        llvm::outs() << " " << it->getNameAsString();
      }
      if (--paramNum) {
        llvm::outs() << ", ";
      }
    }
  }
  llvm::outs() << ")\n";
}

void printCaller(const clang::CallExpr *CE, const clang::SourceManager &SM) {
  auto CallerLoc = CE->getRParenLoc();
  std::string FilePath = SM.getFilename(CallerLoc).str();
  unsigned LineNumber = SM.getSpellingLineNumber(CallerLoc);
  unsigned ColumnNumber = SM.getSpellingColumnNumber(CallerLoc);
  if (SM.isMacroBodyExpansion(CallerLoc)) {
    auto ExpansionLoc = SM.getImmediateMacroCallerLoc(CallerLoc);
    // FilePath = SM.getFilename(ExpansionLoc).str();
    {
      // TODO: Find the printK Reference Location
      FilePath = SM.getFilename(SM.getImmediateSpellingLoc(ExpansionLoc)).str();
    }
    LineNumber = SM.getSpellingLineNumber(ExpansionLoc);
    ColumnNumber = SM.getSpellingColumnNumber(ExpansionLoc);
    // if (FilePath == "") {
    //   llvm::outs() << "Wrong macro parsing ways.\n";
    // }
  } else if (SM.isMacroArgExpansion(CallerLoc)) {
    llvm::outs() << "Is in macro arg expansion\n";
    auto ExpansionLoc = SM.getImmediateExpansionRange(CE->getBeginLoc());
    FilePath = SM.getFilename(ExpansionLoc.getBegin()).str();
    {
      // FilePath =
      // SM.getFilename(SM.getImmediateSpellingLoc(ExpansionLoc)).str();
    }
    LineNumber = SM.getSpellingLineNumber(ExpansionLoc.getBegin());
    ColumnNumber = SM.getSpellingColumnNumber(ExpansionLoc.getBegin());
  }
#ifdef DEBUG
  if (SM.isInExternCSystemHeader(CallerLoc)) {
    llvm::outs() << "Is in system header\n";
  }
  if (SM.isWrittenInBuiltinFile(CallerLoc)) {
    llvm::outs() << "Is in builtin file\n";
  }
  if (SM.isInSystemHeader(CallerLoc)) {
    llvm::outs() << "Is in system header\n";
  }
  if (SM.isMacroArgExpansion(CallerLoc) or SM.isMacroBodyExpansion(CallerLoc) or
      SM.isInSystemMacro(CallerLoc)) {
    llvm::outs() << SM.isMacroBodyExpansion(CallerLoc) << " "
                 << SM.isMacroArgExpansion(CallerLoc) << " "
                 << SM.isInSystemMacro(CallerLoc) << " "
                 << "Is in macro arg expansion\n";
  }
#endif

  if (FilePath == "") {
    llvm::outs() << "Caller file path not found\n";
  } else {
    llvm::outs() << FilePath << ":" << LineNumber << ":" << ColumnNumber
                 << "\n";
  }
}

class ExternalCallMatcher
    : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) {
    auto &SM = *Result.SourceManager;
    llvm::outs() << "========================================\n";
    if (auto CE = Result.Nodes.getNodeAs<clang::CallExpr>("externalCall")) {
      if (auto FD = CE->getDirectCallee()) {
        // output the basic information of the function declaration
        if (!SM.isInMainFile(FD->getLocation())) {
          // auto CalleeLoc = FD->getLocation();
          printCaller(CE, SM);
          printFuncDecl(FD, SM);
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
      llvm::outs() << "No call expression found\n";
    }
  }
};

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("Code-Analysis");

// Setting AST Matchers for call expr
using namespace clang::ast_matchers;
StatementMatcher CallMatcher =
    callExpr(callee(functionDecl())).bind("externalCall");

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
        << "Notice: 1. The compile_commands.json file should be in the same "
           "directory as the source file or in the parent directory of the "
           "source file.\n"
        << "        2. The compile_commands.json file should be named as "
           "compile_commands.json.\n"
        << "        3. You can input any number of source file as you wish.\n";
    return 1;
  }
  llvm::Expected<clang::tooling::CommonOptionsParser> OptionsParser =
      clang::tooling::CommonOptionsParser::create(argc, argv, MyToolCategory,
                                                  llvm::cl::OneOrMore);
  // Database can also be imported manually with JSONCompilationDatabase
  clang::tooling::ClangTool Tool(OptionsParser->getCompilations(),
                                 OptionsParser->getSourcePathList());

#ifdef DEBUG
  // Validate the compile commands and source file lists.
  auto compileCommands =
      OptionsParser->getCompilations().getAllCompileCommands();

  // traverse compile_commands
  llvm::outs() << "[Debug] Validating compile database: "
               << "\n";
  for (auto &it : compileCommands) {
    // Output the filename and directory
    llvm::outs() << "[Debug] Filename: " << it.Filename << "\n";
    llvm::outs() << "[Debug] Directory: " << it.Directory << "\n";
    // llvm::outs() << it.CommandLine << "\n";
    // Output the command line content
    llvm::outs() << "[Debug] Commandline: "
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
  auto fileLists = OptionsParser->getSourcePathList();
  for (auto &it : fileLists) {
    llvm::outs() << it << "\n";
  }
  llvm::outs() << "[Debug] End of source file lists"
               << "\n";
#endif

  ExternalCallMatcher Matcher;
  clang::ast_matchers::MatchFinder Finder;
  Finder.addMatcher(CallMatcher, &Matcher);
  return Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());
  // Tool.run(newFrontendActionFactory<HeadFileDependencyAction>().get());
}