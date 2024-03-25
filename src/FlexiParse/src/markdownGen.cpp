#include "markdownGen.hpp"

namespace markdownGen {
void signatureGen(std::string signature, std::ofstream& file) {
    file << "### External Macro Integer:" << signature << "\n";
}

void definedLocationGen(std::string filePath, std::optional<int64_t> line, std::optional<int64_t> column, std::ofstream& file) {
    file << "- Defined Location: " << filePath << " :" <<  "\n";
}




}
