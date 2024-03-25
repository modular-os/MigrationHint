#include <llvm/Support/raw_ostream.h>
#include "llvm/Support/JSON.h"
#include <fstream>

namespace markdownGen {
void signatureGen(std::string signature, std::ofstream& file);
void definedLocationGen(std::string filePath, std::optional<int64_t> line, std::optional<int64_t> column, std::ofstream& file);
}
