#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>

#include <iostream>
#include <string>
// using namespace clang;
// using namespace clang::tooling;
// using namespace llvm;

class HeadFileDependencyAction : public clang::ASTFrontendAction {
public:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance
    &CI, llvm::StringRef file) override {
        return
        std::make_unique<HeadFileDependencyAction>(CI.getSourceManager());
    }
};

class HeadFileDependencyConsumer : public clang::ASTConsumer {
public:
    explicit HeadFileDependencyConsumer(clang::SourceManager &SM) : SM(SM) {}

    virtual void HandleTranslationUnit(clang::ASTContext &Context) override {
        for (const auto &F : Context.getTranslationUnitDecl()->decls()) {
            if (const auto *ID = dyn_cast<clang::InclusionDirective>(F)) {
                auto FileName = ID->getFileName();
                llvm::outs() << FileName.str() << "\n";
            }
        }
    }

private:
    clang::SourceManager &SM;
};

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("Code-Analysis");

int main(int argc, const char **argv) {
  //   /*
  //    * Usage:
  //    ** CodeAnalysis [path to compile_commands.json] [path to source file]
  //    */
  //   if (argc != 3) {
  //     std::printf(
  //         "Usage: CodeAnalysis [path to compile_commands.json] [path to
  //         source " "file]\n");
  //     return 1;
  //   }
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
  // return
  // Tool.run(newFrontendActionFactory<HeadFileDependencyAction>().get());
}