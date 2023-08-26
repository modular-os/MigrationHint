#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>

// #include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/JSONCompilationDatabase.h"
// Clang Findfile
// C-API
// Include After?
// Libtooling?

int main(int argc, char **argv) {
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
  std::string compile_commands_path = argv[1];
  std::string source_file_path = argv[2];
  std::cout << compile_commands_path << std::endl;
  std::cout << source_file_path << std::endl;

  // Parse compile_commands.json, whose format begins with json array.
  std::string dbErrorMessages;
  std::unique_ptr<clang::tooling::JSONCompilationDatabase> compilation_database(
      clang::tooling::JSONCompilationDatabase::loadFromFile(
          compile_commands_path, dbErrorMessages,
          clang::tooling::JSONCommandLineSyntax::AutoDetect));
  auto compile_commands = compilation_database->getAllCompileCommands();
  // traverse compile_commands
  for (auto &it : compile_commands) {
    // std::cout << it.CommandLine << std::endl;
    // Output the command line content
    for (auto &it2 : it.CommandLine) {
      std::cout << it2 << std::endl;
    }
    // Output the filename and directory
    std::cout << it.Filename << std::endl;
    std::cout << it.Directory << std::endl << std::endl;
  }

  return 0;
}