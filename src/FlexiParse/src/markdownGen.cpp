#include "markdownGen.hpp"

/*
The markdownGen is where the JSONs will be translated into a markdown-format analysis file,
excavates the details of the externalities of a source file.
*/

namespace markdownGen {
void categoryGen(std::string category, std::ofstream& file) {
    category = "`" + category + "`";
    file << "- Category: " << category << "\n";
}
    
void signatureGen(std::string signature, std::ofstream& file) {
    signature = "`" + signature + "`";
    file << "### External Macro Integer: " << signature << "\n";
}

void definedLocationGen(std::string filePath, long line, long column, bool fromCallLocs, std::ofstream& file) {
    if(fromCallLocs)
        file << "This one is from callLocs\n";
    else
        file << "- Defined Location: " << filePath << " :" << line << "\n";
}

void isMacroFunctionGen(bool isMacroFunction, std::ofstream& file) {
    file << isMacroFunction << "\n";
}

void expansionTreeGen(std::string expansionTree, std::ofstream& file) {
    file << expansionTree << "\n";
}

void functionNameGen(std::string functionName, std::ofstream& file) {
    functionName = "`" + functionName + "`";
    file << "## Function: " << functionName << "\n";
}

void returnTypeGen(std::string returnType, std::ofstream& file) {
    returnType = "`" + returnType + "`";
    file <<"- Return Type: " << returnType << "\n";
}

void isPointerGen(bool isPointer, std::ofstream& file) {
    file << "- Is Pointer: " << (isPointer ? "Yes" : "no") << "\n";
}

void fullDefinitionGen(std::string fullDefinition, std::ofstream& file) {
    fullDefinition = "```c\n" + fullDefinition + "\n```\n"; 
    file << "- Full Definition: " << fullDefinition << "\n";
}

void typeNameGen(std::string typeName, std::ofstream& file) {
    typeName = "`" + typeName + "`"; 
    file << "- Full Definition: " << typeName << "\n";
}

}
 