#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/log.h"
#include "absl/log/initialize.h"
#include "chaicli.hpp"
#include "otel_cli.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

ABSL_FLAG(std::string, script, "", "Path to the ChaiScript file to execute");
ABSL_FLAG(bool, interactive, false, "Run in interactive mode (REPL)");

std::string readScriptFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void runInteractiveMode(ChaiClient::ChaiCLI& cli) {
    std::cout << "ChaiScript Interactive Mode (REPL)\n";
    std::cout << "Type 'exit' or 'quit' to exit\n";
    std::cout << "Type 'help' for help\n\n";
    
    std::string line;
    while (true) {
        std::cout << "chai> ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\n\r"));
        line.erase(line.find_last_not_of(" \t\n\r") + 1);
        
        if (line.empty()) {
            continue;
        }
        
        if (line == "exit" || line == "quit") {
            std::cout << "Goodbye!\n";
            break;
        }
        
        if (line == "help") {
            std::cout << "Available commands:\n";
            std::cout << "  exit, quit - Exit the REPL\n";
            std::cout << "  help       - Show this help message\n";
            std::cout << "  Any other input will be evaluated as ChaiScript code\n\n";
            continue;
        }
        
        try {
            ChaiClient::EvalResult result = cli.evaluate(line);
            
            if (result.status == SuccessOperationStatus) {
                // Try to print the result
                try {
                    auto chai = cli.getChai();
                    std::cout << chai->eval<std::string>("to_string(" + line + ")") << "\n";
                } catch (...) {
                    std::cout << "[Execution completed]\n";
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    // Initialize absl logging
    absl::InitializeLog();
    
    // Set program usage message
    absl::SetProgramUsageMessage(
        "ChaiScript CLI - Execute ChaiScript files or run interactive mode\n"
        "Usage: chaiscript_cli --script=<path> | --interactive\n"
        "Examples:\n"
        "  chaiscript_cli --script=myscript.chai\n"
        "  chaiscript_cli --interactive"
    );
    
    // Parse command-line flags
    absl::ParseCommandLine(argc, argv);
    
    std::string script_path = absl::GetFlag(FLAGS_script);
    bool interactive = absl::GetFlag(FLAGS_interactive);
    auto otel = ilfx::otel::runtimeFromFlags("chaiscript_cli");
    ilfx::otel::RootSpan root(otel, "chaiscript_cli");
    root.setAttribute("script_path", script_path);
    root.setAttribute("interactive", interactive);
    
    // Create ChaiCLI instance
    ChaiClient::ChaiCLI cli;
    
    try {
        if (interactive) {
            // Run interactive REPL mode
            LOG(INFO) << "Starting interactive mode";
            ilfx::otel::ScopedSpan interactive_span(otel, "chaiscript_cli.interactive_session");
            runInteractiveMode(cli);
            return root.finish(0);
        }
        
        {
            ilfx::otel::ScopedSpan validate_span(otel, "chaiscript_cli.validate");
            if (script_path.empty()) {
                const std::string message = "Error: Either --script=<path> or --interactive must be specified";
                validate_span.markError(message);
                std::cerr << message << "\n";
                std::cerr << "Use --help for more information\n";
                return root.finish(1);
            }
            
            // Validate that the file exists
            if (!std::filesystem::exists(script_path)) {
                const std::string message = "Error: Script file not found: " + script_path;
                validate_span.markError(message);
                std::cerr << message << "\n";
                return root.finish(1);
            }
        }
        
        LOG(INFO) << "Loading script from: " << script_path;
        
        // Read the script file
        std::string script_content;
        {
            ilfx::otel::ScopedSpan read_span(otel, "chaiscript_cli.read_script");
            read_span.setAttribute("script_path", script_path);
            script_content = readScriptFile(script_path);
            read_span.setAttribute("script.bytes", static_cast<int>(script_content.length()));
        }
        
        LOG(INFO) << "Executing script (" << script_content.length() << " bytes)";
        
        // Execute the script
        ChaiClient::EvalResult result;
        {
            ilfx::otel::ScopedSpan execute_span(otel, "chaiscript_cli.execute_script");
            result = cli.evaluate(script_content);
            if (result.status != SuccessOperationStatus) {
                execute_span.markError("Script execution failed");
            }
        }
        
        if (result.status == SuccessOperationStatus) {
            LOG(INFO) << "Script executed successfully";
            std::cout << "Script execution completed successfully\n";
            
            // Try to print the result if it's convertible
            try {
                auto chai = cli.getChai();
                auto str_result = chaiscript::boxed_cast<std::string>(result.value);
                std::cout << "Result: " << str_result << "\n";
            } catch (...) {
                // If the result can't be printed, that's okay
            }
            
            return root.finish(0);
        } else {
            LOG(ERROR) << "Script execution failed";
            std::cerr << "Script execution failed\n";
            return root.finish(1);
        }
        
    } catch (const chaiscript::exception::eval_error& e) {
        const std::string message = "ChaiScript evaluation error: " + std::string(e.what());
        root.markError(message);
        root.setAttribute("exception.type", "chaiscript::exception::eval_error");
        root.setAttribute("exception.message", std::string(e.what()));
        LOG(ERROR) << "ChaiScript evaluation error: " << e.what();
        std::cerr << "ChaiScript Error: " << e.what() << "\n";
        std::cerr << "  " << e.pretty_print() << "\n";
        return root.finish(1);
    } catch (const std::exception& e) {
        const std::string message = "Exception occurred: " + std::string(e.what());
        root.markError(message);
        root.setAttribute("exception.type", "std::exception");
        root.setAttribute("exception.message", std::string(e.what()));
        LOG(ERROR) << "Exception occurred: " << e.what();
        std::cerr << "Error: " << e.what() << "\n";
        return root.finish(1);
    }
}
