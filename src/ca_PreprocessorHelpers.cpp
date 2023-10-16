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
// #include <llvm/Support/CommandLine.h>
// #include <llvm/Support/raw_ostream.h>

// #include <iostream>
// #include <map>
// #include <string>
#include <vector>

// #include "clang/AST/RecursiveASTVisitor.h"
// #include "clang/ASTMatchers/ASTMatchFinder.h"
// #include "clang/ASTMatchers/ASTMatchers.h"
#include "ca_utils.hpp"
// #include "clang/Lex/MacroArgs.h"
#include "ca_PreprocessorHelpers.hpp"
// #include "ca_ASTHelpers.hpp"
namespace ca {
/**********************************************************************
 * 1. Preprocessor Callbacks
 **********************************************************************/
int MacroPPCallbacks::getMacroExpansionStackDepth(
    std::string MacroName, std::string MacroString,
    const clang::MacroArgs *Args, const clang::Preprocessor &PP) {
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
      llvm::outs() << "Argument " << I << ": " << PP.getSpelling(*Arg) << "\n";
#endif
      CurrMacro.push_back(PP.getSpelling(*Arg));
    }
  }

  if (MacroExpansionStack.size() == 0) {
    MacroExpansionStack.push_back(CurrMacro);
    return 0;
  }
  int flag = 0;
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

void MacroPPCallbacks::InclusionDirective(
    clang::SourceLocation HashLoc, const clang::Token &IncludeTok,
    clang::StringRef FileName, bool IsAngled,
    clang::CharSourceRange FilenameRange, clang::OptionalFileEntryRef File,
    clang::StringRef SearchPath, clang::StringRef RelativePath,
    const clang::Module *Imported, clang::SrcMgr::CharacteristicKind FileType) {
  auto &SM = compiler.getSourceManager();
  if (SM.isInMainFile(HashLoc)) {
    if (HeaderCounts == 0) {
      llvm::outs() << "# Header File: \n";
    }
    llvm::outs() << ++HeaderCounts << ". `"
                 << ca_utils::getLocationString(SM, HashLoc) << "`: `"
                 << SearchPath.str() + "/" + RelativePath.str() << "`\n";
  }
}

void MacroPPCallbacks::MacroExpands(const clang::Token &MacroNameTok,
                                    const clang::MacroDefinition &MD,
                                    clang::SourceRange Range,
                                    const clang::MacroArgs *Args) {
  auto &SM = compiler.getSourceManager();
  auto &PP = compiler.getPreprocessor();
  const clang::LangOptions &LO = PP.getLangOpts();
  if (SM.isInMainFile(Range.getBegin())) {
    auto MacroDefinition = ca_utils::getMacroDeclString(MD, SM, LO);
    int MacroDepth = getMacroExpansionStackDepth(PP.getSpelling(MacroNameTok),
                                                 MacroDefinition, Args, PP);
    if (MacroCounts == 0) {
      llvm::outs() << "# Macro Expansion Analysis: \n";
    }
    if (MacroDepth == 0) {
      llvm::outs() << "```\n\n## Macro " << ++MacroCounts << ": \n```\n";
    }
    llvm::outs().indent(MacroDepth * 3)
        << PP.getSpelling(MacroNameTok) << ", "
        << ca_utils::getLocationString(SM, Range.getBegin()) << "\n";
  }
}

void MacroPPOnlyAction::ExecuteAction() {
  getCompilerInstance().getPreprocessor().addPPCallbacks(
      std::make_unique<MacroPPCallbacks>(getCompilerInstance()));

  clang::PreprocessOnlyAction::ExecuteAction();
}

}  // namespace ca