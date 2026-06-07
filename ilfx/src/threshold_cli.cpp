#include <iostream>
#include <fstream>
#include <string>
#include <antlr4-runtime.h>
#include "ThresholdLexer.h"
#include "ThresholdParser.h"
#include "EvalVisitor.hpp"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/log.h"
#include "absl/log/initialize.h"
#include "otel_cli.hpp"

ABSL_FLAG(std::string, input, "", "Input threshold expression (e.g., '3: 0 <= x < 10')");
ABSL_FLAG(std::string, file, "", "Input file containing threshold expression");

int main(int argc, char* argv[]) {
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();

    std::string input = absl::GetFlag(FLAGS_input);
    std::string file = absl::GetFlag(FLAGS_file);
    auto otel = ilfx::otel::runtimeFromFlags("threshold_cli");
    ilfx::otel::RootSpan root(otel, "threshold_cli.run");
    root.setAttribute("input.present", !input.empty());
    root.setAttribute("file", file);

    std::string expression;
    {
        ilfx::otel::ScopedSpan read_span(otel, "threshold_cli.read_input");
        if (input.empty() && file.empty()) {
            const std::string message = "Either --input or --file must be specified";
            read_span.markError(message);
            LOG(ERROR) << message;
            std::cerr << "Usage: " << argv[0] << " --input='3: 0 <= x < 10'" << std::endl;
            std::cerr << "   or: " << argv[0] << " --file=threshold.txt" << std::endl;
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
            expression = input;
        }
        read_span.setAttribute("expression.length", static_cast<int>(expression.length()));
    }

    LOG(INFO) << "Parsing threshold expression: " << expression;

    try {
        ilfx::otel::ScopedSpan parse_span(otel, "threshold_cli.parse_threshold");
        // Create input stream from string
        antlr4::ANTLRInputStream inputStream(expression);
        
        // Create lexer
        ThresholdLexer lexer(&inputStream);
        
        // Create token stream
        antlr4::CommonTokenStream tokens(&lexer);
        
        // Create parser
        ThresholdParser parser(&tokens);
        
        // Parse the input
        ThresholdParser::RuleFileContext* tree = parser.ruleFile();
        
        // Check for parse errors
        if (parser.getNumberOfSyntaxErrors() > 0) {
            parse_span.markError("Failed to parse expression");
            LOG(ERROR) << "Failed to parse expression: " << parser.getNumberOfSyntaxErrors() << " syntax error(s)";
            return root.finish(1);
        }
        
        // Create visitor and visit the tree
        EvalVisitor visitor;
        visitor.visit(tree);
        
        // Output results
        std::cout << "Rating: " << visitor.rating << std::endl;
        std::cout << "Normalized Expression: " << visitor.evalExpr << std::endl;
        
        LOG(INFO) << "Successfully parsed threshold expression";
        return root.finish(0);
        
    } catch (const std::exception& e) {
        root.markError("Exception occurred: " + std::string(e.what()));
        root.setAttribute("exception.type", "std::exception");
        root.setAttribute("exception.message", std::string(e.what()));
        LOG(ERROR) << "Error parsing expression: " << e.what();
        std::cerr << "Error: " << e.what() << std::endl;
        return root.finish(1);
    }
}
