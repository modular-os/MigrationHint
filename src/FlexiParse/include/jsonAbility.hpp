#include <vector>

#include "llvm/Support/JSON.h"

namespace ca_frontend {
enum class abilityCategory { Macro, Type, Function };

class CodeLocation {
 public:
  std::string file;
  unsigned line;
  unsigned column;
  CodeLocation(std::string _file = "", unsigned _line = 0, unsigned _column = 0)
      : file(_file), line(_line), column(_column) {}
  CodeLocation(llvm::json::Object *obj) {
    file = obj->getString("File")->str();
    line = obj->getInteger("Line").value();
    column = obj->getInteger("Column").value();
  }
};

class JsonAbility {
 protected:
  std::string signature;
  std::string category;
  CodeLocation defLoc;
  std::vector<CodeLocation> callLocs;

 public:
  JsonAbility(llvm::json::Object *obj) {
    signature = obj->getString("Signature")->str();
    category = obj->getString("Category")->str();
    if (category == "Macro") {
      defLoc = CodeLocation(obj->getObject("MacroLoc"));
    } else {
      defLoc = CodeLocation(obj->getObject("DefinedLoc"));
    }
    for (auto &loc : *obj->getArray("CallLocs")) {
      callLocs.push_back(CodeLocation(loc.getAsObject()));
    }
  }

  // Get the information with the desired format
  virtual void output() = 0;
};

class FuncJsonAbility : public JsonAbility {
 protected:
  std::string functionName;
  std::string returnType;

 public:
  FuncJsonAbility(llvm::json::Object *obj) : JsonAbility(obj) {
    functionName = obj->getString("FunctionName")->str();
    returnType = obj->getString("ReturnType")->str();
  }
};

class MacroJsonAbility : public JsonAbility {
 protected:
  std::string expansionTree;
  bool isMacroFunction;

 public:
  MacroJsonAbility(llvm::json::Object *obj) : JsonAbility(obj) {
    expansionTree = obj->getString("ExpansionTree")->str();
    isMacroFunction = obj->getBoolean("IsMacroFunction").value();
  }
};

class TypeJsonAbility : public JsonAbility {
 protected:
  std::string fullDefinition;
  std::string typeName;
  bool isPointer;

 public:
  TypeJsonAbility(llvm::json::Object *obj) : JsonAbility(obj) {
    fullDefinition = obj->getString("FullDefinition")->str();
    typeName = obj->getString("TypeName")->str();
    isPointer = obj->getBoolean("IsPointer").value();
  }
};

/* Format Factory 0: C Headers for kernel */
class HeaderFunc : public FuncJsonAbility {
 public:
  HeaderFunc(llvm::json::Object *obj) : FuncJsonAbility(obj) {}
  void output() override;
};

class HeaderMacro : public MacroJsonAbility {
 public:
  HeaderMacro(llvm::json::Object *obj) : MacroJsonAbility(obj) {}
  void output() override;
};

class HeaderType : public TypeJsonAbility {
 public:
  HeaderType(llvm::json::Object *obj) : TypeJsonAbility(obj) {}
  void output() override;
};

}  // namespace ca_frontend