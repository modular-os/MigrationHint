#include <clang/AST/AST.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>

#include <string>

#include "utils.hpp"

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
  llvm::outs() << "Original path is: " << path
               << "; Core File path is: " << coreFileName << "\n";
#ifdef DEBUG
#endif

  return coreFileName;
}

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

bool getExternalStructType(clang::QualType Type, llvm::raw_ostream &output,
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
    }
    return !InCurrentFile;
  } else {
    return false;
  }

  return false;
}
}  // namespace ca_utils