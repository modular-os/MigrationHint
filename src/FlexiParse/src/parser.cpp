#include "parser.hpp"
#include "markdownGen.hpp"
#include "headerGen.hpp"

namespace parser {
static void macroExpander(llvm::json::Object* node, outputInfo* info) {
    if(node->get("Signature")) {
        std::string signature = node->getString("Signature")->str();
        if(info->is_markdown) {
            markdownGen::signatureGen(signature, info->markdownfile);
        }
    } 

    if(node->get("MacroLoc")) {
        llvm::json::Object* macroLoc = node->getObject("MacroLoc");
        std::string filePath = macroLoc->getString("File")->str();
        std::optional<int64_t> lineNumber = macroLoc->getInteger("Line");
        std::optional<int64_t> columnNumber = macroLoc->getInteger("Column");
        if(info->is_markdown) {
            markdownGen::definedLocationGen(filePath, lineNumber, columnNumber, info->markdownfile);
        }
    } 

    if(node->get("IsMacroFunction")) {
        
    } 

    if(node->get("CallLocs")) {
        llvm::json::Value* callLocs = node->get("CallLocs");
        llvm::json::Array* callLocsJSON = callLocs->getAsArray();
        llvm::outs() << "Expanding callLocs ";
        for(auto &i: *callLocsJSON) {
            macroExpander(i.getAsObject(), info);
        }
    } 

    if(node->get("ExpansionTree")) {
        std::string expansionTree = node->getString("ExpansionTree")->str();
        llvm::outs() << expansionTree << "\n";
    }
}

static void functionExpander(llvm::json::Object* node, outputInfo* info) {
    if(node->get("Signature")) {

    }

    if(node->get("FunctionName")) {
        
    }

    if(node->get("ReturnType")) {
        
    }

    if(node->get("DefinedLoc")) {
        
    }

    if(node->get("CallLocs")) {
        
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
            parser::macroExpander(object, info);
        } else if(type == "Function") {
            parser::functionExpander(object, info);
        } else if(type == "Type") {
            // stypeGen(object);
        } else {
            llvm::errs()<< "Undefined type, quitting...\n";
            return;
        }
    }
}

int run(llvm::json::Array* JSONRoot, outputInfo* info) {
    // Create file
    if(info->is_markdown) {
        std::ofstream file(info->filename + ".md");
        info->markdownfile = std::move(file);
    }
    if(info->is_header) {
        std::ofstream file(info->filename + ".hpp");
        info->headerfile = std::move(file);
    }

    // Parse JSONRoot
    readJSON(JSONRoot, info);

    return 1;
}
}