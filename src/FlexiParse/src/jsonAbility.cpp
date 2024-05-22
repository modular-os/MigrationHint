#include "jsonAbility.hpp"

void ca_frontend::HeaderFunc::output() {
  llvm::outs() << "\\*" << defLoc.file << ":" << defLoc.line << "*\\ \n"
               << signature << ";\n";
}

void ca_frontend::HeaderMacro::output() {
  llvm::outs() << "\\*" << defLoc.file << ":" << defLoc.line << signature
               << "Expansion Tree: \n"
               << expansionTree << "*\\ \n";
}

void ca_frontend::HeaderType::output() {
  llvm::outs() << "\\*" << defLoc.file << ":" << defLoc.line << ", "
               << "IsPointer: " << isPointer << "*\\ \n"
               << signature << ";\n";
}