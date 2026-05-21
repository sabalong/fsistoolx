#pragma once
#include <antlr4-runtime.h>
#include "ThresholdBaseVisitor.h"
#include <sstream>
#include <string>
#include <any>

class EvalVisitor : public ThresholdBaseVisitor {
public:
    std::string rating;
    std::string evalExpr;

    antlrcpp::Any visitRuleDecl(ThresholdParser::RuleDeclContext *ctx) override {
        rating = ctx->rating()->INT()->getText();
        evalExpr = std::any_cast<std::string>(visit(ctx->expr()));
        return evalExpr;
    }

    antlrcpp::Any visitExpr(ThresholdParser::ExprContext *ctx) override {
        return visit(ctx->orExpr());
    }

    antlrcpp::Any visitOrExpr(ThresholdParser::OrExprContext *ctx) override {
        std::ostringstream oss;
        auto andExprs = ctx->andExpr();

        for (size_t i = 0; i < andExprs.size(); ++i) {
            if (i > 0) {
                oss << " or ";
            }
            oss << "(" << std::any_cast<std::string>(visit(andExprs[i])) << ")";
        }

        return oss.str();
    }

    antlrcpp::Any visitAndExpr(ThresholdParser::AndExprContext *ctx) override {
        std::ostringstream oss;
        auto comparisons = ctx->comparison();

        for (size_t i = 0; i < comparisons.size(); ++i) {
            if (i > 0) {
                oss << " and ";
            }
            oss << "(" << std::any_cast<std::string>(visit(comparisons[i])) << ")";
        }

        return oss.str();
    }

    antlrcpp::Any visitComparison(ThresholdParser::ComparisonContext *ctx) override {
        if (ctx->expr()) {
            return std::string("(") + std::any_cast<std::string>(visit(ctx->expr())) + ")";
        }

        auto values = ctx->value();
        auto ops = ctx->op();

        std::ostringstream oss;
        for (size_t i = 0; i < ops.size(); ++i) {
            if (i > 0) {
                oss << " and ";
            }
            oss << "(" << values[i]->getText() << " " << ops[i]->getText()
                << " " << values[i + 1]->getText() << ")";
        }
        return oss.str();
    }

    antlrcpp::Any visitValue(ThresholdParser::ValueContext *ctx) override {

        return ctx->getText();
    }

};
