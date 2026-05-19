//
// Created by kenny on 21/12/25.
//

#ifndef FX_EXPRTKEVALUATOR_H
#define FX_EXPRTKEVALUATOR_H

#include "exprtk/exprtk.hpp"
#include <string>
#include <unordered_map>

bool evalExprWithX(double x, const std::string& expr);
bool evalExprWithVariables(const std::unordered_map<std::string, double>& variables, const std::string& expr);

#endif //FX_EXPRTKEVALUATOR_H
