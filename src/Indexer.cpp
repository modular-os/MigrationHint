// Indexer.cpp: a tool part of the code analysis project to index the kernel
// source code, and make a guess of modules according to the information.
// Simplified from clangd within the LLVM Project.
//
//===--- IndexerMain.cpp -----------------------------------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// clangd-indexer is a tool to gather index data (symbols, xrefs) from source.
//
//===----------------------------------------------------------------------===//

#include <mutex>

#include <clang/Tooling/ArgumentsAdjusters.h>
#include <clang/Tooling/Execution.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Signals.h>

static llvm::cl::opt<IndexFileFormat>
    Format("format", llvm::cl::desc("Format of the index to be written"),
           llvm::cl::values(clEnumValN(IndexFileFormat::YAML, "yaml",
                                       "human-readable YAML format"),
                            clEnumValN(IndexFileFormat::RIFF, "binary",
                                       "binary RIFF format")),
           llvm::cl::init(IndexFileFormat::RIFF));

static llvm::cl::list<std::string> QueryDriverGlobs{
    "query-driver",
    llvm::cl::desc(
        "Comma separated list of globs for white-listing gcc-compatible "
        "drivers that are safe to execute. Drivers matching any of these globs "
        "will be used to extract system includes. e.g. "
        "/usr/bin/**/clang-*,/path/to/repo/**/g++-*"),
    llvm::cl::CommaSeparated,
};

class IndexActionFactory : public tooling::FrontendActionFactory {
public:
  IndexActionFactory(IndexFileIn &Result) : Result(Result) {}

  std::unique_ptr<FrontendAction> create() override {
    return std::make_unique<IndexAction>();
  }

  bool runInvocation(std::shared_ptr<CompilerInvocation> Invocation,
                     FileManager *Files,
                     std::shared_ptr<PCHContainerOperations> PCHContainerOps,
                     DiagnosticConsumer *DiagConsumer) override {
    disableUnsupportedOptions(*Invocation);
    return tooling::FrontendActionFactory::runInvocation(
        std::move(Invocation), Files, std::move(PCHContainerOps), DiagConsumer);
  }

  // Awkward: we write the result in the destructor, because the executor
  // takes ownership so it's the easiest way to get our data back out.
  ~IndexActionFactory() {
    Result.Symbols = std::move(Symbols).build();
    Result.Refs = std::move(Refs).build();
    Result.Relations = std::move(Relations).build();
  }

private:
  IndexFileIn &Result;
  std::mutex FilesMu;
  llvm::StringSet<> Files;
  std::mutex SymbolsMu;
  SymbolSlab::Builder Symbols;
  std::mutex RefsMu;
  RefSlab::Builder Refs;
  std::mutex RelsMu;
  RelationSlab::Builder Relations;
};

int main(int argc, const char **argv) {
  llvm::sys::PrintStackTraceOnErrorSignal(argv[0]);

  const char *Overview = R"(
  Creates an index of symbol and other information in a whole project.
  Example usage for a project using CMake compile commands:
  $ clangd-indexer --executor=all-TUs compile_commands.json > clangd.dex
  )";

  auto Executor = clang::tooling::createExecutorFromCommandLineArgs(
      argc, argv, llvm::cl::getGeneralCategory(), Overview);

  if (!Executor) {
    llvm::errs() << llvm::toString(Executor.takeError()) << "\n";
    return 1;
  }

  // Collect symbols found in each translation unit, merging as we go.
  IndexFileIn Data;
  auto Mangler = std::make_shared<CommandMangler>(
      CommandMangler::detect());
  Mangler->SystemIncludeExtractor = getSystemIncludeExtractor(
      static_cast<llvm::ArrayRef<std::string>>(
          QueryDriverGlobs));
  auto Err = Executor->get()->execute(
      std::make_unique<IndexActionFactory>(Data),
      clang::tooling::ArgumentsAdjuster(
          [Mangler = std::move(Mangler)](const std::vector<std::string> &Args,
                                         llvm::StringRef File) {
            clang::tooling::CompileCommand Cmd;
            Cmd.CommandLine = Args;
            Mangler->operator()(Cmd, File);
            return Cmd.CommandLine;
          }));
  if (Err) {
    elog("{0}", std::move(Err));
  }

  // Emit collected data.
  IndexFileOut Out(Data);
  Out.Format = Format;
  llvm::outs() << Out;
  return 0;
}
