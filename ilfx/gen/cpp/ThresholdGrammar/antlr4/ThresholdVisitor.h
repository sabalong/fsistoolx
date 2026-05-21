
// Generated from antlr4/Threshold.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "ThresholdParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by ThresholdParser.
 */
class  ThresholdVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by ThresholdParser.
   */
    virtual std::any visitRuleFile(ThresholdParser::RuleFileContext *context) = 0;

    virtual std::any visitRuleDecl(ThresholdParser::RuleDeclContext *context) = 0;

    virtual std::any visitRating(ThresholdParser::RatingContext *context) = 0;

    virtual std::any visitExpr(ThresholdParser::ExprContext *context) = 0;

    virtual std::any visitOrExpr(ThresholdParser::OrExprContext *context) = 0;

    virtual std::any visitAndExpr(ThresholdParser::AndExprContext *context) = 0;

    virtual std::any visitComparison(ThresholdParser::ComparisonContext *context) = 0;

    virtual std::any visitValue(ThresholdParser::ValueContext *context) = 0;

    virtual std::any visitOp(ThresholdParser::OpContext *context) = 0;


};

