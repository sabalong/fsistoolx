//
// Created by kenny on 21/12/25.
//

#include <iostream>
#include <unordered_map>

#include "exprtkevaluator.h"
#include "exprtk/exprtk.hpp"

bool evalExprWithX(double x, const std::string& expr) {
    return evalExprWithVariables({{"x", x}}, expr);
}

bool evalExprWithVariables(const std::unordered_map<std::string, double>& variables, const std::string& expr) {
    typedef double T;

    exprtk::symbol_table<T> symbol_table;
    std::unordered_map<std::string, T> boundVariables = variables;
    for (auto& variable : boundVariables) {
        symbol_table.add_variable(variable.first, variable.second);
    }
    symbol_table.add_constants();

    exprtk::expression<T> expression;
    expression.register_symbol_table(symbol_table);

    exprtk::parser<T> parser;
    if (!parser.compile(expr, expression))
    {
        // You can log parser.error() here if needed
        std::cerr << "Error compiling expression: " << expr << " error: " << parser.error() << std::endl;

        return false;
    }

    return expression.value();
}
