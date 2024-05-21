// Encode with UTF-8
#include "ca_utils.hpp"

#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/Support/raw_ostream.h>

#include <cctype>
#include <ostream>
#include <string>

namespace ca_utils {

std::string getCoreFileNameFromPath(const std::string &path) {
  std::string coreFileName;
  // Find the last slash
  size_t lastSlashPos = path.find_last_of('/');
  if (lastSlashPos == std::string::npos) {
    coreFileName = path;
  } else {
    coreFileName = path.substr(lastSlashPos + 1);
  }

  // Find the last dot
  size_t lastDotPos = coreFileName.find_last_of('.');
  assert(lastDotPos != std::string::npos &&
         "Error! Incorrect source file format.\n");
  coreFileName = coreFileName.substr(0, lastDotPos);
#ifdef DEBUG
  llvm::outs() << "Original path is: " << path
               << "; Core File path is: " << coreFileName << "\n";
#endif

  return coreFileName;
}

std::string getLocationString(const clang::SourceManager &SM,
                              const clang::SourceLocation &Loc) {
  // Get the spelling location for Loc
  auto SLoc = SM.getSpellingLoc(Loc);
  std::string FilePath = SM.getFilename(SLoc).str();
  unsigned LineNumber = SM.getSpellingLineNumber(SLoc);
#ifdef DEPRECATED
  unsigned ColumnNumber = SM.getSpellingColumnNumber(SLoc);
#endif

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
  }
  return FilePath + ":" + std::to_string(LineNumber);
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
#ifdef CHN
  llvm::outs() << "   - 函数位置: `" << getLocationString(SM, FD->getLocation())
               << "`\n";
#else
  llvm::outs() << "   - Location: `" << getLocationString(SM, FD->getLocation())
               << "`\n";
#endif
  // FD->print(llvm::outs(), clang::PrintingPolicy(clang::LangOptions()));
#ifdef DEBUG
  // Print function with parameters to string FuncDeclStr;
  std::string FuncDeclStrBuffer, FuncDeclStr;
  llvm::raw_string_ostream FuncDeclStream(FuncDeclStrBuffer);
  clang::LangOptions LangOpts;
  clang::PrintingPolicy PrintPolicy(LangOpts);
  PrintPolicy.TerseOutput = true; // 设置为只打印函数签名
  FD->print(FuncDeclStream, PrintPolicy);
  FuncDeclStr = FuncDeclStream.str();
  llvm::outs() << "---Found external function call: " << FuncDeclStr << "\n";
  FD->getBody()->printPretty(llvm::outs(), nullptr,
                             clang::PrintingPolicy(clang::LangOptions()));
#endif
}

std::string getMacroName(const clang::SourceManager &SM,
                         clang::SourceLocation Loc) {
  // TODO: Change to new way of searching for macro name[Everywhere]
  // 获取源码文本
  const char *bufIdx = SM.getCharacterData(Loc);
  const char *lastSpace = bufIdx;
  bool isMacro = false;
  while (*bufIdx != '\0') {
    if (*bufIdx == '#' && *(bufIdx + 1) == 'd' && *(bufIdx + 2) == 'e' &&
        *(bufIdx + 3) == 'f' && *(bufIdx + 4) == 'i' && *(bufIdx + 5) == 'n' &&
        *(bufIdx + 6) == 'e') {
      isMacro = true;
      break;
    }
    if (*bufIdx == ' ' || *bufIdx == '\t') {
      lastSpace = bufIdx;
    }
    --bufIdx;
  }

  if (!isMacro) {
    return "No Macro";
  }

  std::string MacroName;
  int bracesNum = 0;
  ++lastSpace;
  while (*lastSpace != '\0') {
    if (*lastSpace == ' ' || *lastSpace == '\n' || *lastSpace == '\t' ||
        *lastSpace == '\r') {
      if (bracesNum == 0) {
        break;
      }
    } else if (*lastSpace == '(') {
      ++bracesNum;
    } else if (*lastSpace == ')') {
      --bracesNum;
    }

    MacroName += *lastSpace;
    ++lastSpace;
  }
  return MacroName;
}

clang::SourceLocation getMacroLoc(const clang::SourceManager &SM,
                                  const clang::SourceLocation OriginalLoc) {
  auto CurrentLoc = OriginalLoc;

  std::vector<clang::SourceLocation> tmpStack;
  tmpStack.push_back(CurrentLoc);
  while (true) {
    if (SM.isMacroBodyExpansion(CurrentLoc)) {
      CurrentLoc = SM.getImmediateMacroCallerLoc(CurrentLoc);
    } else if (SM.isMacroArgExpansion(CurrentLoc)) {
      CurrentLoc = SM.getImmediateExpansionRange(CurrentLoc).getBegin();
    } else {
      break;
    }
    tmpStack.push_back(CurrentLoc);
  }
  assert(tmpStack.size() > 1 && "Macro expansion stack is empty.\n");

  return tmpStack[tmpStack.size() - 2];
}

void printCaller(const clang::CallExpr *CE, const clang::SourceManager &SM) {
  // Special Handle for Caller's Location, since the spelling location is
  // incorrect for caller, so we can only use presumed location.
  auto CallerLoc = CE->getBeginLoc();
  auto PLoc = SM.getPresumedLoc(CallerLoc);
  assert(PLoc.isValid() && "Caller's location in the source file is invalid\n");
  std::string FilePath = PLoc.getFilename();
  unsigned LineNumber = PLoc.getLine();

  llvm::outs() << "`" << FilePath << ":" << LineNumber << "`\n";

#ifdef DEPRECATED
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
    // if(SM.isInMainFile(CallerLoc)){
    //   break;
    // }
    // Output the expanded location
    llvm::outs() << "         - Expanded from Macro, Macro's definition: `"
                 << getLocationString(SM, CallerLoc) << "\n";
    //   << ":"
    //  << getMacroName(SM, CallerLoc) << "`\n";
  }
#endif
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

clang::RecordDecl *getExternalStructType(clang::QualType Type,
                                         llvm::raw_ostream &output,
                                         clang::SourceManager &SM,
                                         const std::string &ExtraInfo,
                                         const int OutputIndent) {
#ifdef DEBUG
  auto varType = Type;
#endif
  bool isPointer = false;
  // Type: De-pointer the type and find the original type
  if (Type->isPointerType()) {
    Type = Type->getPointeeType();
    isPointer = true;
  }

  if (Type->isStructureOrClassType()) {
    const auto RT = Type->getAs<clang::RecordType>();
    const auto RTD = RT->getDecl();

    const auto Range = RTD->getSourceRange();
    const bool InCurrentFile = SM.isWrittenInMainFile(Range.getBegin()) &&
                               SM.isWrittenInMainFile(Range.getEnd());

    if (!InCurrentFile) {
#ifdef DEBUG
      output << "   - Member: `" << varType.getAsString() << " " << ExtraInfo
             << "`\n";
#endif
      if (OutputIndent == -1) {
        return RTD;
      }
#ifdef CHN
      output << ExtraInfo << "      - 外部类型名称: `"
             << RTD->getQualifiedNameAsString() << "`\n";

      output << "         - 位置: `"
             << ca_utils::getLocationString(SM, RTD->getLocation()) << "`\n";

      output << "         - 是否为指针: ";
      if (isPointer) {
        output << "`是`\n";
      } else {
        output << "`否`\n";
      }
#else
      output << ExtraInfo << "   - External Type Detailed Info: `"
             << RTD->getQualifiedNameAsString() << "`\n";

      output << "      - Location: `"
             << ca_utils::getLocationString(SM, RTD->getLocation()) << "`\n";

      output << "      - Is Pointer: ";
      if (isPointer) {
        output << "`Yes`\n";
      } else {
        output << "`No`\n";
      }

      if (!RTD->field_empty()) {
        output << "      - Full Definition: \n"
               << "      ```c\n";
        RTD->print(output.indent(6),
                   clang::PrintingPolicy(clang::LangOptions()), 3);
        output << "\n      ```\n";
      } else {
        output << "       - Full Definition: "
               << "**Empty Field!**\n";
      }
#endif
      return RTD;
    }
  } else {
    // return nullptr;
    // TODO：Add support for other types
    // search for typedef
    if (Type->isTypedefNameType()) {
      const clang::TypedefType *TT = Type->getAs<clang::TypedefType>();
      const clang::TypedefNameDecl *TTD = TT->getDecl();
      int depth = 0;
      while (Type->isTypedefNameType()) { // Constantly resolving Typedefs
        if (depth == 5)                   // Limit nesting depth to 5
          break;
        // output << "is Typedef\n";
        TT = Type->getAs<clang::TypedefType>();
        TTD = TT->getDecl();
        Type = TTD->getTypeSourceInfo()->getTypeLoc().getType();
        // if (Type->isBuiltinType())
        //   output << "is builtin type\n";
        // output << Type.getAsString() << "\n"
        //        << getLocationString(SM, TTD->getLocation()) << "\n";
        depth++;
      }

      output << ExtraInfo << "      - ddd外部类型名称: `" << Type.getAsString()
             << "`\n";
      output << "         - 位置: `"
             << getLocationString(SM, TTD->getLocation()) << "`\n";
      output << "         - 是否为指针: ";
      if (isPointer) {
        output << "`是`\n";
      } else {
        output << "`否`\n";
      }
    }
  }
  return nullptr;
}

std::string getMacroDeclString(const clang::MacroDefinition &MD,
                               const clang::SourceManager &SM,
                               const clang::LangOptions &LO) {
  clang::SourceLocation MacroBeginLoc = MD.getMacroInfo()->getDefinitionLoc();
  clang::SourceLocation MacroEndLoc = MD.getMacroInfo()->getDefinitionEndLoc();
// By default, the method "getSourceText" will miss the final token of
// sourcefile. Hence we should enlarge the char-range manually.
#ifdef DEPRECATED
  // Not just miss a character, but a token.
  clang::SourceLocation MacroEndLocPlusOne = MacroEndLoc.getLocWithOffset(1);
#endif
  clang::Token NextToken;
  clang::Lexer::getRawToken(MacroEndLoc, NextToken, SM, LO);
  // clang::SourceLocation MacroEndLocPlusOne = NextToken.getEndLoc();

  clang::CharSourceRange MacroRange =
      clang::CharSourceRange::getCharRange(MacroBeginLoc, MacroEndLoc);

  // Use Lexer to the raw infomation.
  clang::Token MacroToken;
  clang::Lexer::getRawToken(MacroBeginLoc, MacroToken, SM, LO);
  std::string FullMacroText =
      clang::Lexer::getSourceText(MacroRange, SM, LO).str();
  return FullMacroText;
}

bool isMacroInteger(const std::string &MacroText) {
  // TODO: Merge with getMacroName, cost too much time.
  if (MacroText.empty()) {
    return false;
  } else if (MacroText.size() == 1) {
    return true;
  }

  if (MacroText[0] == '_' && MacroText[1] == '_') {
    return false;
  }

  if (MacroText.find("(") != std::string::npos) {
    return false;
  }
  return true;
}

std::string getMacroIdentifier(const std::string &MacroText) {
  // e.g. A -> A, A( -> A, A(1 -> A, A(1) -> A
  // e.g. __A -> __A, __A( -> __A, __A(1 -> __A, __A(1) -> __A
  // Traverse the macro text
  std::string Identifier;
  for (auto &it : MacroText) {
    if (std::isalnum(it) || it == '_') {
    Identifier += it;
    } else {
      break;
    }
  }
  return Identifier;
}

}  // namespace ca_utils