// Encode with UTF-8
#pragma once

#include <clang/Tooling/Tooling.h>
#include <string>
#ifndef _CA_PREPROCESSOR_HELPERS_HPP
#define _CA_PREPROCESSOR_HELPERS_HPP

// #include <clang/AST/AST.h>
// #include <clang/AST/ASTContext.h>
// #include <clang/AST/Decl.h>
// #include <clang/AST/Expr.h>
// #include <clang/AST/OperationKinds.h>
#include <clang/Basic/SourceLocation.h>
// #include <clang/Basic/SourceManager.h>
// #include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
// #include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
// #include <clang/Tooling/CommonOptionsParser.h>
// #include <clang/Tooling/Tooling.h>
// #include <llvm/Support/CommandLine.h>
// #include <llvm/Support/raw_ostream.h>

// #include <iostream>
// #include <map>
// #include <string>
// #include <vector>

#include "clang/AST/RecursiveASTVisitor.h"
// #include "clang/ASTMatchers/ASTMatchFinder.h"
// #include "clang/ASTMatchers/ASTMatchers.h"
// #include "ca_utils.hpp"
#include "clang/Lex/MacroArgs.h"
// #include "ca_PreprocessorHelpers.hpp"
// #include "ca_ASTHelpers.hpp"
namespace ca {
/**********************************************************************
 * 1. Preprocessor Callbacks
 **********************************************************************/
class MacroPPCallbacks : public clang::PPCallbacks {
 public:
  explicit inline MacroPPCallbacks(
      std::map<std::string, std::string> &_NameToExpansion,
      const clang::CompilerInstance &compiler)
      : NameToExpansion(_NameToExpansion),
        CurrentMacroName(""),
        CurrentMacroExpansion(""),
        compiler(compiler),
        MacroCounts(0),
        HeaderCounts(0) {
    name = compiler.getSourceManager()
               .getFileEntryForID(compiler.getSourceManager().getMainFileID())
               ->getName();
  }

 private:
  int getMacroExpansionStackDepth(std::string MacroName,
                                  std::string MacroString,
                                  const clang::MacroArgs *Args,
                                  const clang::Preprocessor &PP);

  bool matchMacroFromExpansion(const std::string &MacroName,
                               const std::string &MacroExpansion);

 public:
  // Provide key-value pairs of macro name and its expansion
  std::map<std::string, std::string> &NameToExpansion;
  std::string CurrentMacroName;
  std::string CurrentMacroExpansion;

  void InclusionDirective(clang::SourceLocation HashLoc,
                          const clang::Token &IncludeTok,
                          clang::StringRef FileName, bool IsAngled,
                          clang::CharSourceRange FilenameRange,
                          clang::OptionalFileEntryRef File,
                          clang::StringRef SearchPath,
                          clang::StringRef RelativePath,
                          const clang::Module *Imported,
                          clang::SrcMgr::CharacteristicKind FileType) override;

  void MacroExpands(const clang::Token &MacroNameTok,
                    const clang::MacroDefinition &MD, clang::SourceRange Range,
                    const clang::MacroArgs *Args) override;

 private:
  const clang::CompilerInstance &compiler;
  std::string name;

  std::vector<std::vector<std::string>> MacroExpansionStack;
  int MacroCounts;
  int HeaderCounts;
};
class MacroPPOnlyAction : public clang::PreprocessOnlyAction {
#ifdef DEPRECATED
  // The following code is deprecated temporarily. May be useful in the
  // future.
  virtual bool BeginSourceFileAction(CompilerInstance &CI) { return true; }
  virtual void EndSourceFileAction() {}
#endif
 public:
  std::map<std::string, std::string> &NameToExpansion;
  //  Constructor
  MacroPPOnlyAction(std::map<std::string, std::string> &_NameToExpansion)
      : NameToExpansion(_NameToExpansion) {
    NameToExpansion.clear();
  }

 protected:
  void ExecuteAction() override;
};

// MyFrontendActionFactory
template <typename T>
std::unique_ptr<clang::tooling::FrontendActionFactory>
newFrontendActionFactory(std::map<std::string, std::string> &_NameToExpansion) {
  class TZSimpleFrontendActionFactory : public clang::tooling::FrontendActionFactory {
   public:
    explicit TZSimpleFrontendActionFactory(std::map<std::string, std::string> &NameToExpansion)
        : NameToExpansion(NameToExpansion) {}

    std::unique_ptr<clang::FrontendAction> create() override {
      return std::make_unique<T>(NameToExpansion);
    }

   private:
    std::map<std::string, std::string> &NameToExpansion;
  };

  return std::unique_ptr<clang::tooling::FrontendActionFactory>(
      new TZSimpleFrontendActionFactory(_NameToExpansion));
}
}  // namespace ca
#endif  // !_CA_PREPROCESSOR_HELPERS_HPP