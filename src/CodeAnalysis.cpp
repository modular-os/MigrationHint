#include <clang-c/Index.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdio>
#include <fstream>
#include <iostream>
// Clang Findfile
// C-API
// Include After?
// Libtooling?

// 回调函数，用于遍历代码树
CXChildVisitResult visitor(CXCursor cursor, CXCursor parent,
                           CXClientData client_data) {
  // // 提取头文件引用节点
  // if (cursor.kind == CXCursor_InclusionDirective) {
  //   CXString included_file = clang_getInclusionDirective(cursor);
  //   const char* file_name = clang_getCString(included_file);
  //   std::cout << "Included File: " << file_name << std::endl;
  //   clang_disposeString(included_file);
  // }

  // // 继续遍历子节点
  // clang_visitChildren(cursor, visitor, client_data);

  // return CXChildVisit_Continue;
}

int main(int argc, char** argv) {
  /*
   * Usage:
   ** CodeAnalysis [path to compile_commands.json] [path to source file]
   */
  if (argc != 3) {
    std::printf(
        "Usage: CodeAnalysis [path to compile_commands.json] [path to source "
        "file]\n");
    return 1;
  }

  // Parse compile_commands.json, whose format begins with json array.
  auto llvm_json_input = llvm::MemoryBuffer::getFileOrSTDIN(argv[1]);
  auto compile_commands_json =
      llvm::json::parse(llvm_json_input.get()->getBuffer())->getAsArray();

  int cnt = 0;
  // Traverse compile_commands.json
  if (compile_commands_json) {
    for (auto& compile_command : *compile_commands_json) {
      if (auto P = compile_command.getAsObject()) {
        auto file_path =
            compile_command.getAsObject()->getString("file").getValue().str();
        auto args_array = compile_command.getAsObject()
                              ->getString("command")
                              .getValue()
                              .str();
        std::cout << "dafdasd" << ++cnt << std::endl;
        // if(file_path)
        std::cout << file_path << std::endl;
        std::cout << args_array << std::endl;
      }
      std::cout << ++cnt << std::endl;
    }
  }
  // compile_commands_json->getAsArray();

  // 遍历编译命令信息
  // for (const auto& command : compile_commands.getAsArray()) {
  //   auto file_path = command.getAsObject()->getString("file").getValue();
  //   auto args_array = command.getAsObject()->getArray("command");

  //   std::vector<const char*> args;
  //   for (const auto& arg : args_array) {
  //     const std::string& arg_str = arg.getValue();
  //     args.push_back(arg_str.c_str());
  //   }

  //   CXIndex index = clang_createIndex(0, 0);
  //   CXTranslationUnit translation_unit = clang_parseTranslationUnit(
  //       index, file_path.c_str(), args.data(), args.size(), nullptr, 0,
  //       CXTranslationUnit_None);

  //   if (translation_unit) {
  //     CXCursor root_cursor =
  //     clang_getTranslationUnitCursor(translation_unit);
  //     clang_visitChildren(root_cursor, visitor, nullptr);
  //     clang_disposeTranslationUnit(translation_unit);
  //   }

  //   clang_disposeIndex(index);
  // }

  return 0;
}