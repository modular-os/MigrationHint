#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/LLVM.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/Support/raw_ostream.h>

#include <string>
#include <vector>

#include "ca_ASTHelpers.hpp"
#include "ca_utils.hpp"

bool ca::ZengExpressionMatcher::ZengExpressionVisitor::VisitExpr(
    clang::Expr *expr) {
  // Check if the current expression has a link_map member
  if (hasLinkMapMember(expr)) {
    hasMatch = true;
    return false;  // 停止遍历,因为已经找到一个满足条件的节点
  }

  // 递归访问子表达式
  for (const auto *child : expr->children()) {
    if (auto *childExpr = clang::dyn_cast<clang::Expr>(child)) {
      return TraverseStmt(static_cast<clang::Stmt *>((clang::Expr *)childExpr));
    }
  }

  return true;
}

bool ca::ZengExpressionMatcher::ZengExpressionVisitor::hasMatchedExpression()
    const {
  return hasMatch;
}

void ca::ZengAnalysisHelper(std::vector<std::string> &files) {
  // Traverse file in files
  for (int i = 0; i < files.size(); ++i) {
    llvm::outs() << "\n=============================\n";
    llvm::outs() << "[" << i + 1 << "/" << files.size() << "] " << "Analyzing "
                 << files[i] << "...\n";
    clang::ast_matchers::MatchFinder Finder;
    using namespace clang::ast_matchers;
    std::string ErrMsg;
    auto CompileDatabase =
        clang::tooling::CompilationDatabase::autoDetectFromSource(files[i],
                                                                  ErrMsg);
    clang::tooling::ClangTool Tool(*CompileDatabase, files[i]);

    /*
    To Be Continued
    auto adjustArgumentsZeng = clang::tooling::getInsertArgumentAdjuster(
        "-I/usr/include", clang::tooling::ArgumentInsertPosition::BEGIN);

    Tool.appendArgumentsAdjuster(adjustArgumentsZeng);
    llvm::outs() << "Initialization finished\n";
    */
    ca::ZengExpressionMatcher Matcher;
    Finder.addMatcher(binaryOperator().bind("binaryOp"), &Matcher);

    Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());
  }
}

void ca::ZengExpressionMatcher::run(
    const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  auto &SM = *(Result.SourceManager);
  if (const auto *BinaryOp =
          Result.Nodes.getNodeAs<clang::BinaryOperator>("binaryOp")) {
    /* TODO: In the main file may be wrong */
    if (SM.isInMainFile(BinaryOp->getExprLoc()) &&
        (BinaryOp->getOpcode() == clang::BO_Add ||
         BinaryOp->getOpcode() == clang::BO_Sub)) {
      auto locationString =
          ca_utils::getLocationString(SM, BinaryOp->getExprLoc());
      if (!locationString.empty() && locationString[0] == '.') {
        // Relative path, which is in the dependency file and not in the main
        // file.
        return;
      }
      auto binExpr = BinaryOp->getExprStmt();
      ZengExpressionVisitor visitor(*Result.Context, binExpr);
      visitor.TraverseStmt(static_cast<clang::Stmt *>((clang::Expr *)binExpr));
      if (visitor.hasMatchedExpression()) {
        clang::LangOptions LangOpts;
        clang::PrintingPolicy PrintPolicy(LangOpts);
        PrintPolicy.TerseOutput = true;

        llvm::outs() << "-----------------------------\n"
                     << "Location: " << locationString << "\nExpression: \n";
        binExpr->printPretty(llvm::outs(), nullptr, PrintPolicy);
        llvm::outs() << "\n";
      }
    }
  }
}

bool ca::ZengExpressionMatcher::ZengExpressionVisitor::hasLinkMapMember(
    const clang::Expr *Operand) {
  while (true) {
    if (auto memberExpr = clang::dyn_cast<clang::MemberExpr>(Operand)) {
      Operand = memberExpr->getBase();
    } else if (auto arraySubscriptExpr =
                   clang::dyn_cast<clang::ArraySubscriptExpr>(Operand)) {
      Operand = arraySubscriptExpr->getBase();
    } else if (auto ImpliExpr =
                   clang::dyn_cast<clang::ImplicitCastExpr>(Operand)) {
      Operand = ImpliExpr->getSubExpr();
    } else if (auto DeclRExpr = clang::dyn_cast<clang::DeclRefExpr>(Operand)) {
      break;
    } else if (auto ParExpr = clang::dyn_cast<clang::ParenExpr>(Operand)) {
      Operand = ParExpr->getSubExpr();
    } else {
      break;
    }

    auto currType = Operand->getType();
    if (currType->isPointerType()) {
      currType = currType->getPointeeType();
    }
    if (currType.getAsString() == "struct link_map") {
      if (auto TypeDecl = currType->getAsRecordDecl()) {
        if (ca_utils::getLocationString(context.getSourceManager(),
                                        TypeDecl->getBeginLoc()) ==
            "../include/link.h:95") {
          return true;
        }
      }
    }
  }
  return false;
}