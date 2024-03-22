#include "llvm/Support/JSON.h"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/MemoryBuffer.h>

#include <iostream>
#include <fstream>

using namespace llvm;
using namespace std;

static cl::opt<string> jsonFile("i", cl::desc("Select input JSON file"), cl::value_desc("Filename"));
static cl::opt<string> outputFile("o", cl::Positional, cl::desc("Name of output file"), cl::value_desc("Filename"));

int main(int argc, const char **argv) {
  cl::ParseCommandLineOptions(argc, argv);
  outs() << "test" <<  jsonFile;

  // Read from JSON to buffer
  auto BufferOrErr = llvm::MemoryBuffer::getFile(jsonFile);
  llvm::StringRef Buffer = (*BufferOrErr)->getBuffer();
  llvm::Expected<llvm::json::Value> Parsed = llvm::json::parse(Buffer);

  if (!Parsed) {
    // 解析失败，处理错误
    auto Err = Parsed.takeError();

    // 输出错误信息
    llvm::errs() << "Failed to parse JSON: ";
    llvm::handleAllErrors(std::move(Err), [&](const llvm::ErrorInfoBase &EIB) {
      llvm::errs() << EIB.message() << "\n";
    
    });
    return 1;
  
  }

  // Convert value type into array type
  llvm::json::Array* JSONRoot = Parsed.get().getAsArray();
  outs() << *JSONRoot->front().getAsString();
  outs() <<"passed";
}

