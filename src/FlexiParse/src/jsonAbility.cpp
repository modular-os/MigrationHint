#include "jsonAbility.hpp"

void ca_frontend::HeaderFunc::output() {
  llvm::outs() << "/* Path: " << defLoc.file << ":" << defLoc.line << " */ \n"
               << signature << ";\n";
}

void ca_frontend::HeaderMacro::output() {
  llvm::outs() << "/* Path: " << defLoc.file << ":" << defLoc.line << "\n"
               << signature << "Expansion Tree: \n"
               << expansionTree << " */ \n";
}

void ca_frontend::HeaderType::output() {
  llvm::outs() << "/* Path: " << defLoc.file << ":" << defLoc.line << ", "
               << "IsPointer: " << isPointer << " */ \n"
               << signature << ";\n";
}