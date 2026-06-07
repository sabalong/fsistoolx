#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include "exprtkevaluator.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/log.h"
#include "absl/log/initialize.h"
#include "otel_cli.hpp"

ABSL_FLAG(std::string, expr, "", "Expression to evaluate (e.g., 'x >= 0 and x < 10')");
ABSL_FLAG(double, x, 0.0, "Value of variable x");
ABSL_FLAG(std::string, variables, "", "Comma-separated variables for expressions (e.g., x=5,y=10). If omitted, --x is used.");
ABSL_FLAG(std::string, file, "", "File containing the expression");

std::string trim(const std::string& input) {
    size_t start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = input.find_last_not_of(" \t\r\n");
    return input.substr(start, end - start + 1);
}

std::unordered_map<std::string, double> parseVariables(const std::string& variables) {
    std::unordered_map<std::string, double> parsed;
    std::istringstream stream(variables);
    std::string assignment;

    while (std::getline(stream, assignment, ',')) {
        assignment = trim(assignment);
        if (assignment.empty()) {
            continue;
        }

        size_t equalsPos = assignment.find('=');
        if (equalsPos == std::string::npos) {
            throw std::invalid_argument("Invalid variable assignment: " + assignment);
        }

        std::string name = trim(assignment.substr(0, equalsPos));
        std::string value = trim(assignment.substr(equalsPos + 1));
        if (name.empty() || value.empty()) {
            throw std::invalid_argument("Invalid variable assignment: " + assignment);
        }

        parsed[name] = std::stod(value);
    }

    return parsed;
}

int main(int argc, char* argv[]) {
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();

    std::string expr = absl::GetFlag(FLAGS_expr);
    std::string file = absl::GetFlag(FLAGS_file);
    double x = absl::GetFlag(FLAGS_x);
    std::string variablesFlag = absl::GetFlag(FLAGS_variables);
    auto otel = ilfx::otel::runtimeFromFlags("exprtk_cli");
    ilfx::otel::RootSpan root(otel, "exprtk_cli.run");
    root.setAttribute("expr.present", !expr.empty());
    root.setAttribute("file", file);
    root.setAttribute("x", x);
    root.setAttribute("variables.present", !variablesFlag.empty());

    std::string expression;
    {
        ilfx::otel::ScopedSpan read_span(otel, "exprtk_cli.read_input");
        if (expr.empty() && file.empty()) {
            const std::string message = "Either --expr or --file must be specified";
            read_span.markError(message);
            LOG(ERROR) << message;
            std::cerr << "Usage: " << argv[0] << " --expr='x >= 0 and x < 10' --x=5" << std::endl;
            std::cerr << "   or: " << argv[0] << " --expr='x >= 0 and y < 10' --variables=x=5,y=7" << std::endl;
            std::cerr << "   or: " << argv[0] << " --file=expression.txt --x=5" << std::endl;
            return root.finish(1);
        }

        if (!file.empty()) {
            std::ifstream inputFile(file);
            if (!inputFile.is_open()) {
                const std::string message = "Failed to open file: " + file;
                read_span.markError(message);
                LOG(ERROR) << "Failed to open file: " << file;
                return root.finish(1);
            }
            std::getline(inputFile, expression);
            inputFile.close();
        } else {
            expression = expr;
        }
        read_span.setAttribute("expression.length", static_cast<int>(expression.length()));
    }

    LOG(INFO) << "Evaluating expression: " << expression;
    LOG(INFO) << "With x = " << x;

    try {
        std::unordered_map<std::string, double> variables;
        bool result;
        {
            ilfx::otel::ScopedSpan evaluate_span(otel, "exprtk_cli.evaluate_expression");
            variables = variablesFlag.empty()
                ? std::unordered_map<std::string, double>{{"x", x}}
                : parseVariables(variablesFlag);
            evaluate_span.setAttribute("variables.count", static_cast<int>(variables.size()));
            result = evalExprWithVariables(variables, expression);
            evaluate_span.setAttribute("result", result);
        }
        
        std::cout << "Expression: " << expression << std::endl;
        for (const auto& variable : variables) {
            std::cout << variable.first << " = " << variable.second << std::endl;
        }
        std::cout << "Result: " << (result ? "true" : "false") << std::endl;
        
        LOG(INFO) << "Successfully evaluated expression";
        return root.finish(result ? 0 : 1);
        
    } catch (const std::exception& e) {
        root.markError("Exception occurred: " + std::string(e.what()));
        root.setAttribute("exception.type", "std::exception");
        root.setAttribute("exception.message", std::string(e.what()));
        LOG(ERROR) << "Error evaluating expression: " << e.what();
        std::cerr << "Error: " << e.what() << std::endl;
        return root.finish(1);
    }
}
