#include "llvm/Support/JSON.h"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/MemoryBuffer.h>

#include <iostream>
#include <filesystem>

#include "parser.hpp"


static llvm::cl::opt<std::string> jsonFile("i", llvm::cl::desc("Select input JSON file"), llvm::cl::value_desc("Filename"));
static llvm::cl::opt<std::string> outputFile("o", llvm::cl::Positional, llvm::cl::desc("Name of output file"), llvm::cl::value_desc("Filename"));
static llvm::cl::opt<bool> markdown("m", llvm::cl::desc("Generate markdown file"));
static llvm::cl::opt<bool> header("h", llvm::cl::desc("Generate header file"));


int main(int argc, const char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv);

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
  llvm::outs() << *JSONRoot->front().getAsString();

  // Default name of output file
  std::string outputName = outputFile.empty() ? jsonFile : outputFile;
  // 创建一个 filesystem::path 对象
  std::filesystem::path filepath(outputName);

  // 使用 stem() 方法获取不带扩展名的文件名
  outputName = filepath.stem().string();

  if(!markdown && !header) {
    llvm::outs() << "Sepcify an output file type!\n";
    return 0;  
  }
  // if(markdown) {
  //   markdownGen::run(JSONRoot, outputName + ".md");
  //   llvm::outs() << "Name of output file: " << outputName << ".md\n";
  // } 
  // if(header) {
  //   markdownGen::run(JSONRoot, outputName + ".hpp");
  //   llvm::outs() << "Name of output file: " << outputName << ".hpp\n";
  // } 
  parser::outputInfo info = {
    .filename = outputName,
    .is_markdown = markdown,
    .is_header = header,
  };
  
  parser::run(JSONRoot, &info);
  

}

