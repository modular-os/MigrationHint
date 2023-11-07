// Encode with UTF-8
#pragma once

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
  explicit inline MacroPPCallbacks(const clang::CompilerInstance &compiler)
      : compiler(compiler), MacroCounts(0), HeaderCounts(0) {
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

 protected:
  void ExecuteAction() override;
};
}  // namespace ca
#endif  // !_CA_PREPROCESSOR_HELPERS_HPP