#include <clang-c/Index.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdio>
#include <fstream>
#include <iostream>

// 回调函数，用于遍历代码树
CXChildVisitResult visitor(CXCursor cursor, CXCursor parent,
                           CXClientData client_data) {
  // 提取头文件引用节点
  if (cursor.kind == CXCursor_InclusionDirective) {
    CXString included_file = clang_getInclusionDirective(cursor);
    const char* file_name = clang_getCString(included_file);
    std::cout << "Included File: " << file_name << std::endl;
    clang_disposeString(included_file);
  }

  // 继续遍历子节点
  clang_visitChildren(cursor, visitor, client_data);

  return CXChildVisit_Continue;
}

int main(int argc, char** argv) {
  /*
   * Usage:
   ** CodeAnalyser [path to compile_commands.json] [path to source file]
   */
	if (argc != 3) {
		std::printf("Usage: CodeAnalyser [path to compile_commands.json] [path to source file]\n");
		return 1;
	}

  // 解析 compile_commands.json 文件
  auto llvmin = llvm::MemoryBuffer::getFileOrSTDIN(argv[1]);
  auto compile_commands = llvm::json::parse(llvmin.get()->getBuffer());

	// TODO: Fix the Grammar bugs

  // 遍历编译命令信息
  for (const auto& command : compile_commands.getAsArray()) {
    auto file_path = command.getAsObject()->getString("file").getValue();
    auto args_array = command.getAsObject()->getArray("command");

    std::vector<const char*> args;
    for (const auto& arg : args_array) {
      const std::string& arg_str = arg.getValue();
      args.push_back(arg_str.c_str());
    }

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit translation_unit = clang_parseTranslationUnit(
        index, file_path.c_str(), args.data(), args.size(), nullptr, 0,
        CXTranslationUnit_None);

    if (translation_unit) {
      CXCursor root_cursor = clang_getTranslationUnitCursor(translation_unit);
      clang_visitChildren(root_cursor, visitor, nullptr);
      clang_disposeTranslationUnit(translation_unit);
    }

    clang_disposeIndex(index);
  }

  return 0;
}