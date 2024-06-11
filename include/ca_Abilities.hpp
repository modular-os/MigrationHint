#ifndef _CA_ABILITIES_HPP
#define _CA_ABILITIES_HPP

#include <clang/AST/Decl.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/Support/JSON.h>

#include <string>
#include <vector>

namespace ca {

/* Classes */
class Ability {
 protected:
  // Vector of source locations of the call
  std::vector<clang::SourceLocation> CallLocs;
  // Signature of the specific function, type, or macro
  std::string signature;

 public:
  Ability(clang::SourceLocation InitLoc, std::string _signature)
      : signature(_signature) {
    CallLocs.push_back(InitLoc);
  }

  void addCallLoc(clang::SourceLocation Loc) { CallLocs.push_back(Loc); }

  void popCallLoc() {
    if (CallLocs.size() > 0) {
      CallLocs.pop_back();
    }
  }

  std::string getSignature() const { return signature; }

  virtual llvm::json::Object buildJSON(const clang::SourceManager &SM) = 0;
};

class ExternalCallAbility : public Ability {
 private:
  const clang::FunctionDecl *CurrFunc;

 public:
  ExternalCallAbility(clang::SourceLocation InitLoc, std::string _signature,
                      const clang::FunctionDecl *FD)
      : Ability(InitLoc, _signature), CurrFunc(FD) {}

  llvm::json::Object buildJSON(const clang::SourceManager &SM) override;
};

enum class ExternalTypeKind {
  STRUCT,
  TYPEDEF,
  ENUM,
  UNKNOWN
  // UNION,
  // POINTER,
  // ARRAY,
  // FUNCTION,
};

class ExternalTypeAbility : public Ability {
 private:
  clang::QualType CurrType;
  ExternalTypeKind TypeKind;

 public:
  ExternalTypeAbility(clang::SourceLocation InitLoc, std::string _signature,
                      clang::QualType QT, ExternalTypeKind _TypeKind)
      : Ability(InitLoc, _signature), CurrType(QT), TypeKind(_TypeKind) {}

  llvm::json::Object buildJSON(const clang::SourceManager &SM) override;
};

class ExternalMacroAbility : public Ability {
 private:
  clang::SourceLocation MacroLoc;
  /*
    Divide into two types: function-like and object-like.
    function-like: #define FOO(x) (x+1).
    object-like: #define FOO 1.
  */
  bool isMacroFunction;
  std::string MacroExpansionTree;

 public:
  ExternalMacroAbility(clang::SourceLocation InitLoc, std::string _signature,
                       clang::SourceLocation _MacroLoc, bool _isMacroFunction,
                       std::string _MacroExpansionTree)
      : Ability(InitLoc, _signature),
        MacroLoc(_MacroLoc),
        isMacroFunction(_isMacroFunction),
        MacroExpansionTree(_MacroExpansionTree) {}

  llvm::json::Object buildJSON(const clang::SourceManager &SM) override;
};

/* utility functions */

llvm::json::Object getLocationValue(const clang::SourceManager &SM,
                                    clang::SourceLocation Loc);

}  // namespace ca

#endif  // !_CA_ABILITIES_HPP