#include "headerGen.hpp"

namespace headerGen {
int run(std::string filename) {
    llvm::outs() << "header called\n";
    std::ofstream file(filename);
    file << "test" << std::endl;
    return 1;
}
}
