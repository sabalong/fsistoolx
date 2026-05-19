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

    if (expr.empty() && file.empty()) {
        LOG(ERROR) << "Either --expr or --file must be specified";
        std::cerr << "Usage: " << argv[0] << " --expr='x >= 0 and x < 10' --x=5" << std::endl;
        std::cerr << "   or: " << argv[0] << " --expr='x >= 0 and y < 10' --variables=x=5,y=7" << std::endl;
        std::cerr << "   or: " << argv[0] << " --file=expression.txt --x=5" << std::endl;
        return 1;
    }

    std::string expression;
    if (!file.empty()) {
        std::ifstream inputFile(file);
        if (!inputFile.is_open()) {
            LOG(ERROR) << "Failed to open file: " << file;
            return 1;
        }
        std::getline(inputFile, expression);
        inputFile.close();
    } else {
        expression = expr;
    }

    LOG(INFO) << "Evaluating expression: " << expression;
    LOG(INFO) << "With x = " << x;

    try {
        std::unordered_map<std::string, double> variables = variablesFlag.empty()
            ? std::unordered_map<std::string, double>{{"x", x}}
            : parseVariables(variablesFlag);
        bool result = evalExprWithVariables(variables, expression);
        
        std::cout << "Expression: " << expression << std::endl;
        for (const auto& variable : variables) {
            std::cout << variable.first << " = " << variable.second << std::endl;
        }
        std::cout << "Result: " << (result ? "true" : "false") << std::endl;
        
        LOG(INFO) << "Successfully evaluated expression";
        return result ? 0 : 1;
        
    } catch (const std::exception& e) {
        LOG(ERROR) << "Error evaluating expression: " << e.what();
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
