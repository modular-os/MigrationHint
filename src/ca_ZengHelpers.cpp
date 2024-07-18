#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/Support/raw_ostream.h>

#include <string>
#include <vector>

#include "ca_ASTHelpers.hpp"
#include "ca_utils.hpp"

void ca::ZengAnalysisHelper(std::vector<std::string> &files) {
  clang::ast_matchers::MatchFinder Finder;
  using namespace clang::ast_matchers;
  std::string ErrMsg;
  auto CompileDatabase =
      clang::tooling::CompilationDatabase::autoDetectFromSource(files.front(),
                                                                ErrMsg);
    llvm::outs() << "testing\n";
  clang::tooling::ClangTool Tool(*CompileDatabase, files);
  ca::ZengExpressionMatcher Matcher;
  Finder.addMatcher(binaryOperator().bind("binaryOp"), &Matcher);

  Tool.run(clang::tooling::newFrontendActionFactory(&Finder).get());
}

void ca::ZengExpressionMatcher::run(
    const clang::ast_matchers::MatchFinder::MatchResult &Result) {
  if (const auto *BinaryOp =
          Result.Nodes.getNodeAs<clang::BinaryOperator>("binaryOp")) {
    std::string expression =
        getExpressionString(BinaryOp, *(Result.SourceManager));
    if (true || hasLinkMapMember(BinaryOp)) {
      llvm::outs() << "Found expression: " << expression << "\n"
                   << "\t at "
                   << ca_utils::getLocationString(*(Result.SourceManager),
                                                  BinaryOp->getExprLoc())
                   << "\n";
    }
  }
}

std::string ca::ZengExpressionMatcher::getExpressionString(
    const clang::BinaryOperator *BinaryOp,
    const clang::SourceManager &SourceManager) {
  std::string expression;
  const clang::Expr *Operand = BinaryOp->getLHS();
  expression +=
      Operand->getSourceRange().getBegin().printToString(SourceManager);

  while (true) {
    expression += " " + getOperatorString(BinaryOp->getOpcode()) + " ";
    Operand = BinaryOp->getRHS();
    expression +=
        Operand->getSourceRange().getBegin().printToString(SourceManager);

    if (const auto *NextBinaryOp = dyn_cast<clang::BinaryOperator>(Operand)) {
      BinaryOp = NextBinaryOp;
    } else {
      break;
    }
  }

  return expression;
}

std::string ca::ZengExpressionMatcher::getOperatorString(
    clang::BinaryOperatorKind OpCode) {
  switch (OpCode) {
    case clang::BO_Add:
      return "+";
    case clang::BO_Sub:
      return "-";
    // case clang::BO_Mul:
    //   return "*";
    // case clang::BO_Div:
    //   return "/";
    default:
      return "";
  }
}

bool ca::ZengExpressionMatcher::hasLinkMapMember(
    const clang::BinaryOperator *BinaryOp) {
  const clang::Expr *Operand = BinaryOp->getLHS();
  if (const auto *MemberExpr = dyn_cast<clang::MemberExpr>(Operand)) {
    if (MemberExpr->getMemberDecl()
            ->getType()
            ->getAs<clang::RecordType>()
            ->getDecl()
            ->getName() == "link_map") {
      return true;
    }
  }

  Operand = BinaryOp->getRHS();
  if (const auto *MemberExpr = dyn_cast<clang::MemberExpr>(Operand)) {
    if (MemberExpr->getMemberDecl()
            ->getType()
            ->getAs<clang::RecordType>()
            ->getDecl()
            ->getName() == "link_map") {
      return true;
    }
  }

  return false;
}