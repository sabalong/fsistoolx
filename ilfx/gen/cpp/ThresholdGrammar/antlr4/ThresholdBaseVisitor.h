
// Generated from antlr4/Threshold.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "ThresholdVisitor.h"


/**
 * This class provides an empty implementation of ThresholdVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  ThresholdBaseVisitor : public ThresholdVisitor {
public:

  virtual std::any visitRuleFile(ThresholdParser::RuleFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRuleDecl(ThresholdParser::RuleDeclContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRating(ThresholdParser::RatingContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpr(ThresholdParser::ExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOrExpr(ThresholdParser::OrExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAndExpr(ThresholdParser::AndExprContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitComparison(ThresholdParser::ComparisonContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitValue(ThresholdParser::ValueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOp(ThresholdParser::OpContext *ctx) override {
    return visitChildren(ctx);
  }


};

