#include "headerGen.hpp"

namespace headerGen {
void categoryGen(std::string category, std::ofstream& file) {
}
    
void signatureGen(std::string signature, std::ofstream& file) {
    signature = signature + ";";
    file <<  signature << "\n";
}

void definedLocationGen(std::string filePath, long line, long column, bool fromCallLocs, std::ofstream& file) {
    // if(fromCallLocs)
    //     file << "- Call Location: " << filePath << ":" << line << "\n";
    // else
    //     file << "- Defined Location: " << filePath << " :" << line << "\n";
}

void isMacroFunctionGen(bool isMacroFunction, std::ofstream& file) {

}

void expansionTreeGen(std::string expansionTree, std::ofstream& file) {
    // file << "expansionTreeGen" << expansionTree << "\n";
}

void functionNameGen(std::string functionName, std::ofstream& file) {
    // functionName = functionName + ";";
    // file << functionName << "\n";
}

void returnTypeGen(std::string returnType, std::ofstream& file) {
    // returnType = "`" + returnType + "`";
    // file <<"- Return Type: " << returnType << "\n";
}

void isPointerGen(bool isPointer, std::ofstream& file) {
}

void fullDefinitionGen(std::string fullDefinition, std::ofstream& file) {
    // fullDefinition = fullDefinition + ";"; 
    // file << fullDefinition << "\n";
}

void typeNameGen(std::string typeName, std::ofstream& file) {
    // typeName = "`" + typeName + "`"; 
}
}
