#include <llvm/Support/raw_ostream.h>
#include "llvm/Support/JSON.h"
#include <fstream>

namespace markdownGen {
void categoryGen(std::string category, std::ofstream& file);
void signatureGen(std::string signature, std::ofstream& file);
void definedLocationGen(std::string filePath, long line, long column, bool fromCallLocs, std::ofstream& file);
void isMacroFunctionGen(bool isMacroFunction, std::ofstream& file);
void expansionTreeGen(std::string expansionTree, std::ofstream& file);
void functionNameGen(std::string functionName, std::ofstream& file); 
void returnTypeGen(std::string returnType, std::ofstream& file);
void isPointerGen(bool isPointer, std::ofstream& file);
void fullDefinitionGen(std::string fullDefinition, std::ofstream& file);
void typeNameGen(std::string typeName, std::ofstream& file);
}
