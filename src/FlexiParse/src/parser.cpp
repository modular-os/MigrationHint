#include "parser.hpp"
#include "markdownGen.hpp"
#include "headerGen.hpp"

/*
The parser is where the JSON being parsed, resulting in three types of outcome:
    1. The macros
    2. The functions
    3. The types
After the parsing ends, these output will be translated into desired outcome,
which can be a markdown file, or a header file, or both.
*/ 

namespace parser {
// Parse the macros of the JSON tree
static void macroExpander(llvm::json::Object* node, outputInfo* info, bool fromCallLocs) {
    if(info->isMarkdown)
        markdownGen::categoryGen("Macro", info->markdownfile);

    if(node->get("Signature")) {
        std::string signature = node->getString("Signature")->str();
        if(info->isMarkdown) 
            markdownGen::signatureGen(signature, info->markdownfile);
        if(info->isHeader) 
            headerGen::signatureGen(signature, info->headerfile);
    } 

    if(node->get("MacroLoc")) {
        llvm::json::Object* macroLoc = node->getObject("MacroLoc");
        std::string filePath = macroLoc->getString("File")->str();
        long lineNumber = macroLoc->getInteger("Line").value();
        long columnNumber = macroLoc->getInteger("Column").value();
        if(info->isMarkdown) 
            markdownGen::definedLocationGen(filePath, lineNumber, columnNumber, fromCallLocs, info->markdownfile);
        if(info->isHeader) 
            headerGen::definedLocationGen(filePath, lineNumber, columnNumber, fromCallLocs, info->headerfile);
    } 

    if(node->get("IsMacroFunction")) {
        bool isMacroFunction = bool(node->getBoolean("IsMacroFunction"));
        if(info->isMarkdown)
            markdownGen::isMacroFunctionGen(isMacroFunction, info->markdownfile);
    } 

    if(node->get("CallLocs")) {
        llvm::json::Value* callLocs = node->get("CallLocs");
        llvm::json::Array* callLocsJSON = callLocs->getAsArray();
        
        for(auto &i: *callLocsJSON) {
            llvm::outs() << "Expanding callLocs ";
            llvm::json::Object* callLoc = i.getAsObject();
            std::string filePath = callLoc->getString("File")->str();
            long lineNumber = callLoc->getInteger("Line").value();
            long columnNumber = callLoc->getInteger("Column").value();
            if(info->isMarkdown) 
                markdownGen::definedLocationGen(filePath, lineNumber, columnNumber, true, info->markdownfile);
            if(info->isHeader) 
                headerGen::definedLocationGen(filePath, lineNumber, columnNumber, true, info->headerfile);
        }
    } 

    if(node->get("ExpansionTree")) {
        std::string expansionTree = node->getString("ExpansionTree")->str();
        if(info->isMarkdown)
            markdownGen::expansionTreeGen(expansionTree, info->markdownfile);
        if(info->isHeader)
            headerGen::expansionTreeGen(expansionTree, info->headerfile);
    }
}

// Parse the functions of the JSON tree,
static void functionExpander(llvm::json::Object* node, outputInfo* info, bool fromCallLocs) {
    if(info->isMarkdown)
        markdownGen::categoryGen("Function", info->markdownfile);

    if(node->get("Signature")) {
        std::string signature = node->getString("Signature")->str();
        if(info->isMarkdown) 
            markdownGen::signatureGen(signature, info->markdownfile);
        if(info->isHeader) 
            headerGen::signatureGen(signature, info->headerfile);
    }

    if(node->get("FunctionName")) {
        std::string functionName = node->getString("FunctionName")->str();
        if(info->isMarkdown)
            markdownGen::functionNameGen(functionName, info->markdownfile);
        if(info->isHeader)
            headerGen::functionNameGen(functionName, info->headerfile);
    }

    if(node->get("ReturnType")) {
        std::string returnType = node->getString("ReturnType")->str();
        if(info->isMarkdown)
            markdownGen::returnTypeGen(returnType, info->markdownfile);
        if(info->isHeader)
            headerGen::returnTypeGen(returnType, info->headerfile);
    }

    if(node->get("DefinedLoc")) {
        llvm::json::Object* macroLoc = node->getObject("DefinedLoc");
        std::string filePath = macroLoc->getString("File")->str();
        long lineNumber = macroLoc->getInteger("Line").value();
        long columnNumber = macroLoc->getInteger("Column").value();
        if(info->isMarkdown) 
            markdownGen::definedLocationGen(filePath, lineNumber, columnNumber, fromCallLocs, info->markdownfile);
        if(info->isHeader) 
            headerGen::definedLocationGen(filePath, lineNumber, columnNumber, fromCallLocs, info->headerfile);
    }

    if(node->get("CallLocs")) {
        llvm::json::Value* callLocs = node->get("CallLocs");
        llvm::json::Array* callLocsJSON = callLocs->getAsArray();
        
        for(auto &i: *callLocsJSON) {
            llvm::outs() << "Expanding callLocs ";
            llvm::json::Object* callLoc = i.getAsObject();
            std::string filePath = callLoc->getString("File")->str();
            long lineNumber = callLoc->getInteger("Line").value();
            long columnNumber = callLoc->getInteger("Column").value();
            if(info->isMarkdown) 
                markdownGen::definedLocationGen(filePath, lineNumber, columnNumber, true, info->markdownfile);
            if(info->isHeader) 
                headerGen::definedLocationGen(filePath, lineNumber, columnNumber, true, info->headerfile);
        }

    }
}

static void typeExpander(llvm::json::Object* node, outputInfo* info, bool fromCallLocs) {
    if(info->isMarkdown)
        markdownGen::categoryGen("Type", info->markdownfile);
    
    if(node->get("Signature")) {
        std::string signature = node->getString("Signature")->str();
        if(info->isMarkdown)
            markdownGen::signatureGen(signature, info->markdownfile);
        if(info->isHeader)
            headerGen::signatureGen(signature, info->headerfile);
    }

    
    if(node->get("IsPointer")) {
        bool isPointer = bool(node->getBoolean("isPointer"));
        if(info->isMarkdown)
            markdownGen::isPointerGen(isPointer, info->markdownfile);
    }

    if(node->get("DefinedLoc")) {
        llvm::json::Object* macroLoc = node->getObject("DefinedLoc");
        std::string filePath = macroLoc->getString("File")->str();
        long lineNumber = macroLoc->getInteger("Line").value();
        long columnNumber = macroLoc->getInteger("Column").value();
        if(info->isMarkdown) 
            markdownGen::definedLocationGen(filePath, lineNumber, columnNumber, fromCallLocs, info->markdownfile);
    }

    if(node->get("FullDefinition")) {
        std::string fullDefinition = node->getString("FullDefinition")->str();
        if(info->isMarkdown)
            markdownGen::fullDefinitionGen(fullDefinition, info->markdownfile);
        if(info->isHeader)
            headerGen::fullDefinitionGen(fullDefinition, info->headerfile);
    }

    if(node->get("TypeName")) {
        std::string typeName = node->getString("TypeName")->str();
        if(info->isMarkdown)
            markdownGen::typeNameGen(typeName, info->markdownfile);
    }

    if(node->get("CallLocs")) {
        llvm::json::Value* callLocs = node->get("CallLocs");
        llvm::json::Array* callLocsJSON = callLocs->getAsArray();
        
        for(auto &i: *callLocsJSON) {
            llvm::outs() << "Expanding callLocs ";
            llvm::json::Object* callLoc = i.getAsObject();
            std::string filePath = callLoc->getString("File")->str();
            long lineNumber = callLoc->getInteger("Line").value();
            long columnNumber = callLoc->getInteger("Column").value();
            if(info->isMarkdown) 
                markdownGen::definedLocationGen(filePath, lineNumber, columnNumber, true, info->markdownfile);
            if(info->isHeader) 
                headerGen::definedLocationGen(filePath, lineNumber, columnNumber, true, info->headerfile);
        }
    }
}

// Read each element in the JSONRoot, and expand them depends on their type
static void readJSON(llvm::json::Array* JSONRoot, outputInfo* info) {
    for(auto &i: *JSONRoot) {
        // Each element in JSONRoot is an object
        llvm::json::Object* object = i.getAsObject();
        std::string type = object->getString("Category")->data();

        // Identify the type of each element in JSONRoot
        if(type == "Macro") {
            macroExpander(object, info, false);
        } else if(type == "Function") {
            functionExpander(object, info, false);
        } else if(type == "Type") {
            typeExpander(object, info, false);
        } else {
            llvm::errs()<< "Undefined type, quitting...\n";
            return;
        }
    }
}

int run(llvm::json::Array* JSONRoot, outputInfo* info) {
    // Create file
    if(info->isMarkdown) {
        std::ofstream file(info->filename + ".md");
        info->markdownfile = std::move(file);
    }
    if(info->isHeader) {
        std::ofstream file(info->filename + ".hpp");
        info->headerfile = std::move(file);
    }

    // Parse JSONRoot
    readJSON(JSONRoot, info);

    return 1;
}
}