#include <llvm/Support/CommandLine.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <filesystem>
#include <map>
#include <vector>

#include "jsonAbility.hpp"
#include "llvm/Support/JSON.h"

std::map<ca_frontend::abilityCategory, std::vector<ca_frontend::JsonAbility *>>
    abilities;

static llvm::cl::opt<std::string> jsonFile(
    "i", llvm::cl::desc("Select input JSON file"),
    llvm::cl::value_desc("Filename"));
static llvm::cl::opt<std::string> outputFile(
    "o", llvm::cl::Positional, llvm::cl::desc("Name of output file"),
    llvm::cl::value_desc("Filename"));
static llvm::cl::opt<bool> markdownMode(
    "markdown", llvm::cl::desc("Generate markdown file"));
static llvm::cl::opt<bool> headerMode("header",
                                      llvm::cl::desc("Generate header file"));

int main(int argc, const char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv);

  // Read from JSON to buffer
  // llvm::outs() << "Reading JSON file: " << jsonFile << "\n";
  if (!markdownMode && !headerMode) {
    llvm::errs() << "Sepcify an output file type!\n";
    return 0;
  }

  // Default name of output file
  std::string outputName = outputFile.empty() ? jsonFile : outputFile;
  // 创建一个 filesystem::path 对象
  std::filesystem::path filepath(outputName);

  // 使用 stem() 方法获取不带扩展名的文件名
  outputName = filepath.stem().string();

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
  llvm::json::Array *JSONRoot = Parsed.get().getAsArray();

  if (headerMode) {
    // Traverse through the array
    for (auto &Element : *JSONRoot) {
      llvm::json::Object *obj = Element.getAsObject();
      std::string category = obj->getString("Category")->str();
      if (category == "Function") {
        ca_frontend::FuncJsonAbility *func = new ca_frontend::HeaderFunc(obj);
        abilities[ca_frontend::abilityCategory::Function].push_back(func);
      } else if (category == "Macro") {
        ca_frontend::MacroJsonAbility *macro =
            new ca_frontend::HeaderMacro(obj);
        abilities[ca_frontend::abilityCategory::Macro].push_back(macro);
      } else if (category == "Type") {
        ca_frontend::TypeJsonAbility *type = new ca_frontend::HeaderType(obj);
        abilities[ca_frontend::abilityCategory::Type].push_back(type);
      }
    }

    // Output & Release the memory
    for (auto &i : abilities) {
      // State the type of the ability
      switch (i.first) {
        case ca_frontend::abilityCategory::Function:
          llvm::outs() << "/* External Function Abilities: */ \n\n";
          break;
        case ca_frontend::abilityCategory::Macro:
          llvm::outs() << "/* External Macro Abilities: */ \n\n";
          break;
        case ca_frontend::abilityCategory::Type:
          llvm::outs() << "/* External Type Abilities: */ \n\n";
          break;
        default:
          break;
      }

      for (auto &j : i.second) {
        if (j != nullptr) {
          j->output();
          delete j;
        }
        llvm::outs() << "\n";
      }
    }
  }
  return 0;
}
