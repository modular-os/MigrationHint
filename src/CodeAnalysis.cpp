// Encode with UTF-8
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
#include <filesystem>

#include "ca_ASTHelpers.hpp"
#include "ca_PreprocessorHelpers.hpp"
#include "ca_utils.hpp"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/MacroArgs.h"

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
DeclarationMatcher BasicExternalFuncDeclMatcherPattern =
    functionDecl().bind("externalTypeFuncD");

StatementMatcher ExternalCallMatcherPattern =
    callExpr(callee(functionDecl())).bind("externalCall");

// Bind Matcher to ExternalFieldDecl
// Notice: Since ParamVarDecl is the subclass of VarDecl,
//        so they share the same Matcher pattern.
DeclarationMatcher ExternalStructMatcherPattern = anyOf(
    recordDecl().bind("externalTypeFD"), varDecl().bind("externalTypeVD"));

StatementMatcher ExternalMacroIntegersMatcherPattern =
    integerLiteral().bind("integerLiteral");

StatementMatcher ReturnMatcherPattern = returnStmt().bind("returnStmt");

StatementMatcher ExternalEnumMatcherPattern = declRefExpr().bind("declRefExpr");

StatementMatcher ExternalMacroStringMatcherPattern =
    stringLiteral().bind("stringLiteral");

// TranslationUnitDecl is the root of AST, for outputting the whole ast tree.
DeclarationMatcher TranslationUnitDecl =
    translationUnitDecl().bind("translationUnit");

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
 * 1. Command Line Options
 **********************************************************************/
llvm::cl::opt<std::string> OptionSourceFilePath(
    "s",
    llvm::cl::desc("Path to the source file which is expected to be analyzed."),
    llvm::cl::value_desc("path-to-sourcefile"));
llvm::cl::opt<bool> OptionEnableFunctionAnalysis(
    "enable-function-analysis",
    llvm::cl::desc("Enable external function analysis to source file"),
    llvm::cl::init(false));
llvm::cl::opt<bool> OptionEnableFunctionAnalysisByHeaders(
    "enable-function-analysis-by-headers",
    llvm::cl::desc("Enable external function analysis to source file, all the "
                   "function declarations are grouped by header files."),
    llvm::cl::init(false));
llvm::cl::opt<bool> OptionEnableStructAnalysis(
    "enable-struct-analysis",
    llvm::cl::desc("Enable external struct type analysis to source file"),
    llvm::cl::init(false));
llvm::cl::opt<bool> OptionEnablePPAnalysis(
    "enable-pp-analysis",
    llvm::cl::desc("Enable preprocess analysis to source file, show details of "
                   "all the header files and macros."),
    llvm::cl::init(false));
llvm::cl::opt<bool> OptionEnableModuleAnalysis(
    "enable-module-analysis",
    llvm::cl::desc("Compute the external function set for a sequence of source "
                   "files. Note: The source file path should be specified "
                   "using the -s option, and distinct source files should be "
                   "provided as comma-separated intervals."),
    llvm::cl::init(false));

llvm::cl::opt<bool> OptionGenerateReport(
    "generate-report", llvm::cl::desc("Generate a report for huawei."),
    llvm::cl::init(false));
llvm::cl::opt<bool> OptionGenerateMigrateCode(
    "enable-migrate-code-gen",
    llvm::cl::desc("Generate the code for migration."), llvm::cl::init(false));
llvm::cl::opt<bool> OptionGenerateJSON(
    "enable-json-gen", llvm::cl::desc("Generate the json info."),
    llvm::cl::init(false));
llvm::cl::opt<std::string> OptionJSONoutput("o",
                                            llvm::cl::desc("Output of json."),
                                            llvm::cl::value_desc("Filename"));

llvm::cl::extrahelp MoreHelp(R"(
Notice: 1. The compile_commands.json file should be in the same directory as the source file or in the parent directory of the source file.
        2. The Compilation Database should be named as compile_commands.json.
        3. You can input only one source file as you wish.

Developed by Zhe Tang<tangzh6101@gmail.com> for modular-OS project.
Version 1.0.0
)");

/**********************************************************************
 * 2. Main Function
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
    llvm::outs() << "Usage: ./CodeAnalysis --help for detailed usage.\n";
    return 1;
  }
  // Basic infrastructures
  std::vector<std::string> SourceFilePaths;
  int status = 1;
  std::string ErrMsg;

  // Begin parsing options.
  llvm::cl::ParseCommandLineOptions(argc, argv);

  if (!OptionSourceFilePath.empty()) {
    // llvm::outs() << "Source File Path: " << OptionSourceFilePath << "\n";
    SourceFilePaths.push_back(OptionSourceFilePath);
    if (OptionEnableModuleAnalysis) {
      ca::ModuleAnalysisHelper(OptionSourceFilePath);
      return 0;
    }
  } else {
    llvm::outs()
        << "Error! Missing critical option: No source file path found! You can "
           "use option -S to specify the source file path.\n";
    exit(1);
  }

  if (OptionGenerateReport) {
    ca::generateReport(OptionSourceFilePath);
    return 0;
  }
#ifdef DEPRECATED
  // Deprecated, now we use llvm::cl::opt to parse the options.
  llvm::Expected<clang::tooling::CommonOptionsParser> OptionsParser =
      clang::tooling::CommonOptionsParser::create(argc, argv, MyToolCategory,
                                                  llvm::cl::OneOrMore);
  // Database can also be imported manually with JSONCompilationDatabase
  clang::tooling::ClangTool Tool(OptionsParser->getCompilations(),
                                 OptionsParser->getSourcePathList());
#endif

  // Database can also be imported manually with JSONCompilationDatabase
  auto CompileDatabase =
      clang::tooling::CompilationDatabase::autoDetectFromSource(
          OptionSourceFilePath, ErrMsg);
  clang::tooling::ClangTool Tool(*CompileDatabase, SourceFilePaths);

#ifdef DEBUG
  // Validate the compile commands and source file lists.
  auto compileCommands =
      OptionsParser->getCompilations().getAllCompileCommands();
  auto fileLists = OptionsParser->getSourcePathList();

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
  if (OptionEnablePPAnalysis) {
    std::map<std::string, std::string> NameToExpansion;

    // ca::MacroPPOnlyAction MacroActions;
    status *= Tool.run(
        ca::newFrontendActionFactory<ca::MacroPPOnlyAction>(NameToExpansion)
            .get());
  }

  clang::ast_matchers::MatchFinder Finder;

#ifdef DEBUG
  // Temporary debug code
  ca::ExternalDependencyMatcher exDependencyMatcher;
  Finder.addMatcher(ExternalMacroStringMatcherPattern, &exDependencyMatcher);
  Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());
  return 0;
#endif

  if (OptionEnableFunctionAnalysis || OptionEnableStructAnalysis ||
      OptionEnableFunctionAnalysisByHeaders) {
    ca::ExternalCallMatcher exCallMatcher(ASTs[0]->getSourceManager(),
                                          ca::ExternalCallMatcher::Print);
    ca::ExternalDependencyMatcher exDependencyMatcher;

    if (OptionEnableFunctionAnalysis || OptionEnableStructAnalysis) {
      Finder.addMatcher(BasicExternalFuncDeclMatcherPattern,
                        &exDependencyMatcher);
    }
    if (OptionEnableStructAnalysis) {
      Finder.addMatcher(ExternalStructMatcherPattern, &exDependencyMatcher);
      Finder.addMatcher(ExternalMacroIntegersMatcherPattern,
                        &exDependencyMatcher);
      Finder.addMatcher(ExternalEnumMatcherPattern, &exDependencyMatcher);

#ifdef DEPRECATED
      Finder.addMatcher(ExternalExprsMatcherPatter, &exDependencyMatcher);
#endif
    }
    if (OptionEnableFunctionAnalysis) {
      Finder.addMatcher(ExternalCallMatcherPattern, &exDependencyMatcher);
    }
    if (OptionEnableFunctionAnalysisByHeaders) {
      Finder.addMatcher(ExternalCallMatcherPattern, &exCallMatcher);
    }
    status *= Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());
  }

  if (OptionGenerateMigrateCode) {
    ca::MigrateCodeGenerator migrateCodeGenerator;

    Finder.addMatcher(BasicExternalFuncDeclMatcherPattern,
                      &migrateCodeGenerator);
    Finder.addMatcher(ExternalStructMatcherPattern, &migrateCodeGenerator);
    Finder.addMatcher(ExternalMacroIntegersMatcherPattern,
                      &migrateCodeGenerator);
    Finder.addMatcher(ExternalCallMatcherPattern, &migrateCodeGenerator);

    status *= Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());
  }

  if (OptionGenerateJSON) {
    std::map<std::string, std::string> NameToExpansion;
    status *= Tool.run(
        ca::newFrontendActionFactory<ca::MacroPPOnlyAction>(NameToExpansion)
            .get());

    // Translate relative path into absolute path
    std::string outputPath = !OptionJSONoutput.empty() ? OptionJSONoutput : std::string("output.json");
    auto base_path = std::filesystem::current_path();
    if(outputPath[0] != '/')
      outputPath = (base_path / outputPath).string();
  
    ca::ExternalDependencyJSONBackend jsonBackend(ASTs[0]->getSourceManager(), outputPath, NameToExpansion);

    Finder.addMatcher(BasicExternalFuncDeclMatcherPattern, &jsonBackend);
    Finder.addMatcher(ExternalStructMatcherPattern, &jsonBackend);
    Finder.addMatcher(ExternalMacroIntegersMatcherPattern, &jsonBackend);
    Finder.addMatcher(ExternalCallMatcherPattern, &jsonBackend);

    status *= Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());
  }

  return status;
}