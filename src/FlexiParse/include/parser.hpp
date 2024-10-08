#include <llvm/Support/raw_ostream.h>
#include "llvm/Support/JSON.h"
#include <fstream>

namespace parser {
// A struct used to describe the desired output
struct outputInfo {
    std::string filename;
    bool isMarkdown, isHeader;
    std::ofstream markdownfile;
    std::ofstream headerfile;
};

int run(llvm::json::Array* JSONRoot, outputInfo* info); 
}
