#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
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
#include <map>
#include <string>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"

// Basic Infrastructure
std::vector<std::unique_ptr<clang::ASTUnit>> ASTs;

void printFuncDecl(const clang::FunctionDecl *FD,
                   const clang::SourceManager &SM) {
  auto Loc = FD->getLocation();
  // Get the spelling location for Loc
  auto SLoc = SM.getSpellingLoc(Loc);
  std::string FilePath = SM.getFilename(SLoc).str();
  unsigned LineNumber = SM.getSpellingLineNumber(SLoc);
  unsigned ColumnNumber = SM.getSpellingColumnNumber(SLoc);

  if (FilePath == "") {
    // Couldn't get the spelling location, try to get the presumed location
#if DEBUG
    llvm::outs << "Couldn't get the spelling location, try to get the presumed "
                  "location\n";
#endif
    auto PLoc = SM.getPresumedLoc(Loc);
    assert(PLoc.isValid() &&
           "Caller's Presumed location in the source file is invalid\n");
    FilePath = PLoc.getFilename();
    assert(FilePath != "" &&
           "Caller's location in the source file is invalid.");
    LineNumber = PLoc.getLine();
    ColumnNumber = PLoc.getColumn();
  }

  llvm::outs() << "`" << FD->getReturnType().getAsString() << " "
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
  llvm::outs() << ")`\n";
  llvm::outs() << "   - Location: `" << FilePath << ":" << LineNumber << ":"
               << ColumnNumber << "`\n";
#ifdef DEBUG
  // Print function with parameters to string FuncDeclStr;
  std::string FuncDeclStrBuffer, FuncDeclStr;
  llvm::raw_string_ostream FuncDeclStream(FuncDeclStrBuffer);
  clang::LangOptions LangOpts;
  clang::PrintingPolicy PrintPolicy(LangOpts);
  PrintPolicy.TerseOutput = true;  // 设置为只打印函数签名
  FD->print(FuncDeclStream, PrintPolicy);
  FuncDeclStr = FuncDeclStream.str();
  llvm::outs() << "---Found external function call: " << FuncDeclStr << "\n";
  FD->getBody()->printPretty(llvm::outs(), nullptr,
                             clang::PrintingPolicy(clang::LangOptions()));
#endif
}

void printCaller(const clang::CallExpr *CE, const clang::SourceManager &SM) {
  auto CallerLoc = CE->getBeginLoc();
  auto PLoc = SM.getPresumedLoc(CallerLoc);
  assert(PLoc.isValid() && "Caller's location in the source file is invalid\n");
  std::string FilePath = PLoc.getFilename();
  unsigned LineNumber = PLoc.getLine();
  unsigned ColumnNumber = PLoc.getColumn();

  llvm::outs() << "   - Caller Location: `" << FilePath << ":" << LineNumber
               << ":" << ColumnNumber << "`\n";
  // Judging whether the caller is expanded from predefined macros.
  if (SM.isMacroBodyExpansion(CallerLoc)) {
    auto ExpansionLoc = SM.getImmediateMacroCallerLoc(CallerLoc);

    ExpansionLoc = SM.getTopMacroCallerLoc(ExpansionLoc);
    PLoc = SM.getPresumedLoc(ExpansionLoc);
    FilePath = PLoc.getFilename();
    LineNumber = PLoc.getLine();
    ColumnNumber = PLoc.getColumn();
    assert(FilePath != "" &&
           "(Normal) Macro's original location defined in the headfiles is "
           "invalid.");
  } else if (SM.isMacroArgExpansion(CallerLoc)) {
#ifdef DEBUG
    llvm::outs() << "Is in macro arg expansion\n";
#endif
    auto ExpansionLoc = SM.getImmediateExpansionRange(CallerLoc).getBegin();
    FilePath = SM.getFilename(SM.getImmediateSpellingLoc(ExpansionLoc)).str();
    assert(FilePath != "" &&
           "(function-like) Macro's original location defined in the headfiles "
           "is invalid.");
    LineNumber = SM.getSpellingLineNumber(ExpansionLoc);
    ColumnNumber = SM.getSpellingColumnNumber(ExpansionLoc);
  } else {
    return;
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

  llvm::outs() << "   - Expanded from Macro, Macro's definition: `" << FilePath
               << ":" << LineNumber << ":" << ColumnNumber << "`\n\n";
}

class ExternalCallMatcher
    : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  void onStartOfTranslationUnit() override {
#ifdef DEBUG
    llvm::outs() << "In onStartOfTranslationUnit\n";
#endif
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

          if (FilenameToCallExprs.find(FilePath) == FilenameToCallExprs.end()) {
            FilenameToCallExprs[FilePath] =
                std::vector<const clang::CallExpr *>();
          }

        

          FilenameToCallExprs[FilePath].push_back(CE);
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
      llvm::outs() << "## Headfile: " << it.first << "\n";
      llvm::outs() << "- External Function Count: " << it.second.size()
                   << "\n\n";
      int file_cnt = 0;
      for (auto &it2 : it.second) {
        auto FD = it2->getDirectCallee();
        llvm::outs() << ++file_cnt << ". ";
        printFuncDecl(FD, SM);
        printCaller(it2, SM);
        llvm::outs() << "\n";
        ++cnt;
      }
      llvm::outs() << "---\n\n";
    }

    llvm::outs() << "# Summary\n"
                 << "- External Function Call Count: " << cnt << "\n";
  }

 private:
  std::map<std::string, std::vector<const clang::CallExpr *>>
      FilenameToCallExprs;
};

class ExternalStructMatcher
    : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  void onStartOfTranslationUnit() override {
#ifdef DEBUG
    llvm::outs() << "In onStartOfTranslationUnit\n";
#endif
    llvm::outs() << "# External Struct Report\n\n";
    structCnt = 0;
    externalStructCnt = 0;
  }

  virtual void run(
      const clang::ast_matchers::MatchFinder::MatchResult &Result) override {
    auto &SM = *Result.SourceManager;
    if (auto RD =
            Result.Nodes.getNodeAs<clang::RecordDecl>("externalFieldDecl")) {
#ifdef DEBUG
      llvm::outs() << "ExternalStructMatcher\n";
      llvm::outs() << FD->getQualifiedNameAsString() << "\n";
      llvm::outs() << FD->getType().getAsString() << " "
                   << FD->getNameAsString() << "\n";
      auto RD = FD->getParent();
      llvm::outs() << "\t" << RD->getQualifiedNameAsString() << "\n";
#endif
      // Dealing with the relaionship between RecordDecl and fieldDecl
      // output the basic information of the RecordDecl

      if (!RD->getName().empty() && SM.isInMainFile(RD->getLocation())) {
        // Output the basic info for specific RecordDecl
        llvm::outs() << "## " << ++structCnt << ". "
                     << RD->getQualifiedNameAsString() << "\n";

        // Output the basic location info for the fieldDecl
        auto Loc = RD->getLocation();
        // Get the spelling location for Loc
        auto SLoc = SM.getSpellingLoc(Loc);
        std::string FilePath = SM.getFilename(SLoc).str();
        unsigned LineNumber = SM.getSpellingLineNumber(SLoc);
        unsigned ColumnNumber = SM.getSpellingColumnNumber(SLoc);
        if (FilePath == "") {
          // Couldn't get the spelling location, try to get the presumed
          // location
#if DEBUG
          llvm::outs << "Couldn't get the spelling location, try
              to get the presumed location\n ";
#endif
              auto PLoc = SM.getPresumedLoc(Loc);
          assert(PLoc.isValid() &&
                 "Caller's Presumed location in the source file is invalid\n ");
          FilePath = PLoc.getFilename();
          assert(FilePath != "" &&
                 "Caller's location in the source file is invalid.");
          LineNumber = PLoc.getLine();
          ColumnNumber = PLoc.getColumn();
        }

        llvm::outs() << "- Location: `" << FilePath << ": " << LineNumber << ":"
                     << ColumnNumber << "`\n";

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

          // FDType: De-pointer the type and find the original type
          auto FDType = FD->getType();

          bool isPointer = 0;
          if (FDType->isPointerType()) {
            FDType = FDType->getPointeeType();
            isPointer = 1;
          }
          if (FDType->isStructureOrClassType()) {
            auto RT = FDType->getAs<clang::RecordType>();
            auto RTD = RT->getDecl();

            auto Range = RTD->getSourceRange();
            bool InCurrentFile = SM.isWrittenInMainFile(Range.getBegin()) &&
                                 SM.isWrittenInMainFile(Range.getEnd());
            if (!InCurrentFile) {
              llvm::outs() << "   - Member: `" << FD->getType().getAsString()
                           << " " << FD->getNameAsString() << "`\n"
                           << "   - Type: `" << RTD->getQualifiedNameAsString()
                           << "`\n";

              SLoc = SM.getSpellingLoc(RTD->getLocation());
              FilePath = SM.getFilename(SLoc).str();
              LineNumber = SM.getSpellingLineNumber(SLoc);
              ColumnNumber = SM.getSpellingColumnNumber(SLoc);
              if (FilePath == "") {
                // Couldn't get the spelling location, try to get the presumed
                // location
#if DEBUG
                llvm::outs << "Couldn't get the spelling location, try
                    to get the presumed location\n ";
#endif

                    auto PLoc = SM.getPresumedLoc(RTD->getLocation());
                assert(PLoc.isValid() &&
                       "Caller's Presumed location in the source file is "
                       "invalid\n ");
                FilePath = PLoc.getFilename();
                assert(FilePath != "" &&
                       "Caller's location in the source file is invalid.");
                LineNumber = PLoc.getLine();
                ColumnNumber = PLoc.getColumn();
              }

              llvm::outs() << "      - Location: "
                           << "`" << FilePath << ":" << LineNumber << ":"
                           << ColumnNumber << "`\n";

              llvm::outs() << "      - Is Pointer: ";
              if (isPointer) {
                llvm::outs() << "`Yes`\n";
              } else {
                llvm::outs() << "`No`\n";
              }

              if (!RTD->field_empty()) {
                llvm::outs() << "      - Full Definition: \n"
                             << "      ```c\n";
                RTD->print(llvm::outs().indent(6),
                           clang::PrintingPolicy(clang::LangOptions()), 3);
                llvm::outs() << "\n      ```\n";
              } else {
                // TODO: Fix missing zpool, why?
                llvm::outs() << "       - Full Definition: \n"
                             << "**Empty Field!**\n";
              }

              ++externalStructCnt;
            }
          }
        }
        llvm::outs() << "\n\n---\n\n\n";
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
    llvm::outs() << "# Summary\n"
                 << "- Struct Count: " << structCnt << "\n"
                 << "- External Struct Count: " << externalStructCnt << "\n\n";
  }

 private:
  int structCnt;
  int externalStructCnt;
};

// Global Infrastructure
// Apply a custom category to all command-line options so that they are
// the only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("Code-Analysis");

// Setting AST Matchers for call expr
using namespace clang::ast_matchers;
StatementMatcher ExternalCallMatcherPattern =
    callExpr(callee(functionDecl())).bind("externalCall");

// Bind Matcher to ExterenelFieldDecl
// DeclarationMatcher ExternalStructMatcherPattern =
//     fieldDecl().bind("externalFieldDecl");

DeclarationMatcher ExternalStructMatcherPattern =
    recordDecl().bind("externalFieldDecl");

// DeclarationMatcher ExternalStructMatcherPattern =
//     fieldDecl(hasType(recordDecl())).bind("externalFieldDecl");

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

  // Prepare the basic infrastructure
  Tool.buildASTs(ASTs);

  ExternalCallMatcher exCallMatcher;
  ExternalStructMatcher exStructMatcher;
  clang::ast_matchers::MatchFinder Finder;
  Finder.addMatcher(ExternalStructMatcherPattern, &exStructMatcher);
  Finder.addMatcher(ExternalCallMatcherPattern, &exCallMatcher);
  int status =
      Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());

  return status;
}