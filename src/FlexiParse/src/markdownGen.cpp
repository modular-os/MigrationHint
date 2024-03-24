#include "markdownGen.hpp"
#include "llvm/Support/JSON.h"

namespace markdownGen {
int run(std::string filename) {
    llvm::outs() << "makrdown called\n";
    std::ofstream file(filename);
    file << "test" << std::endl;
    return 1;
}
}
