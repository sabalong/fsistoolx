#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include "scoreratingparser.hpp"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/log.h"
#include "absl/log/initialize.h"
#include "otel_cli.hpp"

ABSL_FLAG(std::string, input, "", "Rating to score mapping (e.g., '1:Low, 2:Medium, 3:High')");
ABSL_FLAG(std::string, file, "", "File containing rating to score mapping");
ABSL_FLAG(std::string, lookup, "", "Rating key to lookup in the map");

int main(int argc, char* argv[]) {
    absl::ParseCommandLine(argc, argv);
    absl::InitializeLog();

    std::string input = absl::GetFlag(FLAGS_input);
    std::string file = absl::GetFlag(FLAGS_file);
    std::string lookup = absl::GetFlag(FLAGS_lookup);
    auto otel = ilfx::otel::runtimeFromFlags("scoreratingparser_cli");
    ilfx::otel::RootSpan root(otel, "scoreratingparser_cli");
    root.setAttribute("input.present", !input.empty());
    root.setAttribute("file", file);
    root.setAttribute("lookup", lookup);

    std::string mappingStr;
    {
        ilfx::otel::ScopedSpan read_span(otel, "scoreratingparser_cli.read_input");
        if (input.empty() && file.empty()) {
            const std::string message = "Either --input or --file must be specified";
            read_span.markError(message);
            LOG(ERROR) << message;
            std::cerr << "Usage: " << argv[0] << " --input='1:Low, 2:Medium, 3:High'" << std::endl;
            std::cerr << "   or: " << argv[0] << " --file=rating_map.txt" << std::endl;
            std::cerr << "\nOptional: --lookup=2 (to lookup a specific rating)" << std::endl;
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
            std::getline(inputFile, mappingStr);
            inputFile.close();
        } else {
            mappingStr = input;
        }
        read_span.setAttribute("mapping.length", static_cast<int>(mappingStr.length()));
    }

    LOG(INFO) << "Parsing rating to score mapping: " << mappingStr;

    try {
        std::unordered_map<std::string, std::string> ratingMap;
        {
            ilfx::otel::ScopedSpan parse_span(otel, "scoreratingparser_cli.parse_rating_mapping");
            ratingMap = ratingToScore(mappingStr);
            parse_span.setAttribute("mapping.count", static_cast<int>(ratingMap.size()));
        }
        
        if (ratingMap.empty()) {
            root.markError("No mappings found");
            LOG(WARNING) << "No mappings found";
            std::cout << "No mappings found in input" << std::endl;
            return root.finish(1);
        }

        std::cout << "Rating to Score Mapping:" << std::endl;
        std::cout << "========================" << std::endl;
        
        for (const auto& [rating, score] : ratingMap) {
            std::cout << "Rating " << rating << " -> Score: " << score << std::endl;
        }
        
        if (!lookup.empty()) {
            std::cout << "\nLookup Result:" << std::endl;
            std::cout << "==============" << std::endl;
            
            auto it = ratingMap.find(lookup);
            if (it != ratingMap.end()) {
                std::cout << "Rating " << lookup << " has score: " << it->second << std::endl;
            } else {
                root.markError("Lookup rating not found");
                std::cout << "Rating " << lookup << " not found in mapping" << std::endl;
                return root.finish(1);
            }
        }
        
        LOG(INFO) << "Successfully parsed rating mapping with " << ratingMap.size() << " entries";
        return root.finish(0);
        
    } catch (const std::exception& e) {
        root.markError("Exception occurred: " + std::string(e.what()));
        root.setAttribute("exception.type", "std::exception");
        root.setAttribute("exception.message", std::string(e.what()));
        LOG(ERROR) << "Error parsing mapping: " << e.what();
        std::cerr << "Error: " << e.what() << std::endl;
        return root.finish(1);
    }
}
