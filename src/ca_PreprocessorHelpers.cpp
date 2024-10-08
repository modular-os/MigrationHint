// Encode with UTF-8
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
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>

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
bool MacroPPCallbacks::matchMacroFromExpansion(
    const std::string &MacroName, const std::string &MacroExpansion) {
  // std::string A = "example";
  // std::string B = "This is an example (string)";

  size_t pos = 0;
  while ((pos = MacroExpansion.find(MacroName, pos)) != std::string::npos) {
#ifdef DEPRECATED
    // Check that there is either a space or the beginning of the matched string
    // before the target position
    if (pos > 0 && MacroExpansion[pos - 1] != ' ' &&
        MacroExpansion[pos - 1] != MacroName[0]) {
      pos += MacroName.length();
      continue;
    }
#endif

    // Check that there is either a space, a left parenthesis, or the end of the
    // matched string after the target position
    if (pos + MacroName.length() < MacroExpansion.length() &&
        MacroExpansion[pos + MacroName.length()] != ' ' &&
        MacroExpansion[pos + MacroName.length()] != '\t' &&
        MacroExpansion[pos + MacroName.length()] != '(' &&
        MacroExpansion[pos + MacroName.length()] != ')' &&
        MacroExpansion[pos + MacroName.length()] != ',' &&
        MacroExpansion[pos + MacroName.length()] != ';' &&
        MacroExpansion[pos + MacroName.length()] != '|' &&
        MacroExpansion[pos + MacroName.length()] !=
            MacroName[MacroName.length() - 1]) {
      pos += MacroName.length();
      continue;
    }

    return true;
  }
  return false;
}

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
    int cnt = 0;
    for (auto MacroPart : MacroExpansionStack.back()) {
      ++cnt;
      if (cnt == 1) {
        if (MacroPart == MacroName) {
#ifdef DEBUG
          llvm::outs() << "---" << cnt << ": " << MacroPart << "\n";
#endif
          // If the currentMacro is identical to the macros in stack, return.
          return MacroExpansionStack.size() - 1;
        }
      } else {  // cnt > 1, which is macro expansion and args.
        if ((cnt == 2 && matchMacroFromExpansion(MacroName, MacroPart)) ||
            (cnt > 2 && MacroPart == MacroName)) {
          flag = 1;
#ifdef DEBUG
          llvm::outs() << cnt << ": " << MacroPart << "\n";
#endif
          break;
        }
      }
#ifdef DEPRECATED
      if (MacroPart.find(MacroName) != std::string::npos) {
        if (MacroPart == MacroExpansionStack.back().front() &&
            MacroPart == MacroName) {
          // If the currentMacro is identical to the macros in stack, return.
          llvm::outs() << "---" << MacroPart << "\n";
          return MacroExpansionStack.size() - 1;
        }
        flag = 1;
        llvm::outs() << MacroPart << "\n";
        break;
      }
#endif
    }
    if (flag) {
      break;
    } else {
#ifdef DEBUG
      llvm::outs() << MacroExpansionStack.back().front() << ", ";
#endif
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
#ifdef DEPRECATED
    if (HeaderCounts == 0) {
#ifdef CHN
      llvm::outs() << "# 头文件分析: \n";
#else
      llvm::outs() << "# Header File: \n";
#endif
    }
    llvm::outs() << ++HeaderCounts << ". `"
                 << ca_utils::getLocationString(SM, HashLoc) << "`: `"
                 << SearchPath.str() + "/" + RelativePath.str() << "`\n";
#endif
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
#ifdef DEPRECATED
#ifdef CHN
      llvm::outs() << "# 宏展开分析: \n";
#else
      llvm::outs() << "# Macro Expansion Analysis: \n";
#endif  // CHN
#endif  // DEPRECATED
    }
    if (MacroDepth == 0) {
#ifdef DEPRECATED
#ifdef CHN
      llvm::outs() << "```\n\n## 宏 " << ++MacroCounts << ": \n```\n";
#else
      llvm::outs() << "```\n\n## Macro " << ++MacroCounts << ": \n```\n";
#endif  // CHN
#endif  // DEPRECATED
      if (CurrentMacroName != "") {
        NameToExpansion[CurrentMacroName] = CurrentMacroExpansion;
      }
      CurrentMacroName = PP.getSpelling(MacroNameTok);
      CurrentMacroExpansion = "";
    }
#ifdef DEPRECATED
    llvm::outs().indent(MacroDepth * 3)
        << PP.getSpelling(MacroNameTok) << ", "
        << ca_utils::getLocationString(SM, Range.getBegin()) << "\n";
#endif

    // Save the correspond contents into CurrentMacroExpansion
    CurrentMacroExpansion += std::string(MacroDepth * 3, ' ');
    CurrentMacroExpansion += PP.getSpelling(MacroNameTok);
    if (MacroDepth != 0) {
      CurrentMacroExpansion += ", ";
      CurrentMacroExpansion +=
          ca_utils::getLocationString(SM, Range.getBegin()) + "\n";
    } else {
      CurrentMacroExpansion += "\n";
    }

#ifdef DEBUG
    llvm::outs() << "---\n" << MacroDefinition << "===\n";
#endif
  }
}

void MacroPPOnlyAction::ExecuteAction() {
  getCompilerInstance().getPreprocessor().addPPCallbacks(
      std::make_unique<MacroPPCallbacks>(NameToExpansion,
                                         getCompilerInstance()));

  clang::PreprocessOnlyAction::ExecuteAction();
}
}  // namespace ca