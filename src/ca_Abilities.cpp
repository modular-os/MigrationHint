#include "ca_Abilities.hpp"

#include <clang/Basic/SourceManager.h>
#include <llvm/Support/JSON.h>

#include <string>

#include "clang/AST/Type.h"

namespace ca {

/* Classes */
llvm::json::Object ExternalCallAbility::buildJSON(
    const clang::SourceManager &SM) {
  llvm::json::Object AbilityJSON;
  llvm::json::Array CallLocsJSON;
  AbilityJSON["Category"] = "Function";

  for (auto &Loc : CallLocs) {
    CallLocsJSON.push_back(getLocationValue(SM, Loc));
  }

  AbilityJSON["Signature"] = signature;
  AbilityJSON["FunctionName"] = CurrFunc->getNameAsString();
  AbilityJSON["ReturnType"] = CurrFunc->getReturnType().getAsString();
  AbilityJSON["DefinedLoc"] = getLocationValue(SM, CurrFunc->getLocation());
  AbilityJSON.try_emplace("CallLocs", CallLocsJSON);
  return AbilityJSON;
}

llvm::json::Object ExternalTypeAbility::buildJSON(
    const clang::SourceManager &SM) {
  llvm::json::Object AbilityJSON;
  llvm::json::Array CallLocsJSON;
  AbilityJSON["Category"] = "Type";

  for (auto &Loc : CallLocs) {
    CallLocsJSON.push_back(getLocationValue(SM, Loc));
  }

  if (CurrType->isPointerType()) {
    CurrType = CurrType->getPointeeType();
    AbilityJSON["IsPointer"] = true;
  } else {
    AbilityJSON["IsPointer"] = false;
  }

  if (CurrType->isStructureOrClassType()) {
    const auto RT = CurrType->getAs<clang::RecordType>();
    const auto RTD = RT->getDecl();

    AbilityJSON["DefinedLoc"] = getLocationValue(SM, RTD->getLocation());

    if (!RTD->field_empty()) {
      std::string FieldStr;
      llvm::raw_string_ostream FieldStrSteam(FieldStr);
      RTD->print(FieldStrSteam);
      FieldStrSteam.flush();
      AbilityJSON["FullDefinition"] = FieldStr;
    } else {
      AbilityJSON["FullDefinition"] = "Empty!";
    }
  } else {
    // TODO: Add support for other types
    // Merge the jl's code
  }

  AbilityJSON["Signature"] = signature;
  AbilityJSON["TypeName"] = CurrType.getAsString();
  AbilityJSON.try_emplace("CallLocs", CallLocsJSON);
  return AbilityJSON;
}

llvm::json::Object ExternalMacroAbility::buildJSON(
    const clang::SourceManager &SM) {
  llvm::json::Object AbilityJSON;
  llvm::json::Array CallLocsJSON;
  AbilityJSON["Category"] = "Macro";

  for (auto &Loc : CallLocs) {
    CallLocsJSON.push_back(getLocationValue(SM, Loc));
  }

  AbilityJSON["Signature"] = signature;
  AbilityJSON["MacroLoc"] = getLocationValue(SM, MacroLoc);
  AbilityJSON["IsMacroFunction"] = isMacroFunction;
  AbilityJSON.try_emplace("CallLocs", CallLocsJSON);
  return AbilityJSON;
}

/* Utility functions */
llvm::json::Object getLocationValue(const clang::SourceManager &SM,
                                   clang::SourceLocation Loc) {
  // Get the spelling location for Loc
  auto SLoc = SM.getSpellingLoc(Loc);
  std::string FilePath = SM.getFilename(SLoc).str();
  unsigned LineNumber = SM.getSpellingLineNumber(SLoc);
  unsigned ColumnNumber = SM.getSpellingColumnNumber(SLoc);

  if (FilePath == "") {
    // Couldn't get the spelling location, try to get the presumed location
    auto PLoc = SM.getPresumedLoc(Loc);
    assert(PLoc.isValid() &&
           "[getLocationValue]: Presumed location in the source file is "
           "invalid\n");
    FilePath = PLoc.getFilename();
    assert(
        FilePath != "" &&
        "[getLocationValue]: Location string in the source file is invalid.");
    LineNumber = PLoc.getLine();
    ColumnNumber = PLoc.getColumn();
  }

  llvm::json::Object LocJSON;
  LocJSON["File"] = FilePath;
  LocJSON["Line"] = LineNumber;
  LocJSON["Column"] = ColumnNumber;

  return LocJSON;
}

}  // namespace ca