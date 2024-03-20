#include "llvm/Support/JSON.h"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using namespace std;

static cl::opt<string> jsonFile("i", cl::desc("Select input JSON file"), cl::value_desc("Filename"));
static cl::opt<string> outputFile("o", cl::Positional, cl::desc("Name of output file"), cl::value_desc("Filename"));

int main(int argc, const char **argv) {
  cl::ParseCommandLineOptions(argc, argv);
  llvm::outs() << "test" <<  jsonFile;

}

