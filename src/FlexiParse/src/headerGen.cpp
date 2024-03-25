#include "headerGen.hpp"

namespace headerGen {
int run(llvm::json::Array* JSONRoot, std::string filename) {
    llvm::outs() << "header called\n";
    std::ofstream file(filename);
    file << "test" << std::endl;
    return 1;
}
}
