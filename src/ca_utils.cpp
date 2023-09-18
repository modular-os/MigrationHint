#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <llvm/Support/raw_ostream.h>

#include <string>

#include "utils.hpp"

namespace ca_utils {

std::string getLocationString(const clang::SourceManager &SM,
                              const clang::SourceLocation &Loc) {
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
           "[getLocationString]: Presumed location in the source file is "
           "invalid\n");
    FilePath = PLoc.getFilename();
    assert(
        FilePath != "" &&
        "[getLocationString]: Location string in the source file is invalid.");
    LineNumber = PLoc.getLine();
    ColumnNumber = PLoc.getColumn();
  }
  return FilePath + ":" + std::to_string(LineNumber) + ":" +
         std::to_string(ColumnNumber);
}

std::string getFuncDeclString(const clang::FunctionDecl *FD) {
  // Return the function declaration string
  std::string FuncDeclStr;
  FuncDeclStr =
      FD->getReturnType().getAsString() + " " + FD->getNameAsString() + "(";
  if (int paramNum = FD->getNumParams()) {
    for (auto &it : FD->parameters()) {
      FuncDeclStr += it->getType().getAsString();
      if (it->getNameAsString() != "") {
        FuncDeclStr += " " + it->getNameAsString();
      }
      if (--paramNum) {
        FuncDeclStr += ", ";
      }
    }
  }
  FuncDeclStr += ")";
  return FuncDeclStr;
}

void printFuncDecl(const clang::FunctionDecl *FD,
                   const clang::SourceManager &SM) {
  llvm::outs() << "`" << getFuncDeclString(FD) << "`\n";
  llvm::outs() << "   - Location: `" << getLocationString(SM, FD->getLocation())
               << "`\n";
  // FD->print(llvm::outs(), clang::PrintingPolicy(clang::LangOptions()));
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
  // Special Handle for Caller's Location, since the spelling location is
  // incorrect for caller, so we can only use presumed location.
  auto CallerLoc = CE->getBeginLoc();
  auto PLoc = SM.getPresumedLoc(CallerLoc);
  assert(PLoc.isValid() && "Caller's location in the source file is invalid\n");
  std::string FilePath = PLoc.getFilename();
  unsigned LineNumber = PLoc.getLine();
  unsigned ColumnNumber = PLoc.getColumn();

  llvm::outs() << "`" << FilePath << ":" << LineNumber << ":" << ColumnNumber
               << "`\n";

  // Judging whether the caller is expanded from predefined macros.
  while (true) {
    if (SM.isMacroBodyExpansion(CallerLoc)) {
      auto ExpansionLoc = SM.getImmediateMacroCallerLoc(CallerLoc);
      CallerLoc = ExpansionLoc;
    } else if (SM.isMacroArgExpansion(CallerLoc)) {
#ifdef DEBUG
      llvm::outs() << "Is in macro arg expansion\n";
#endif
      auto ExpansionLoc = SM.getImmediateExpansionRange(CallerLoc).getBegin();
      CallerLoc = ExpansionLoc;
    } else {
      break;
    }
    // Output the expanded location
    llvm::outs() << "         - Expanded from Macro, Macro's definition: `"
                 << getLocationString(SM, CallerLoc) << "`\n";
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
}
}  // namespace ca_utils