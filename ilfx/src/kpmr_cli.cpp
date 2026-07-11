#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/log.h"
#include "absl/log/initialize.h"
#include "absl/strings/str_format.h"
#include "kpmr_datasource.hpp"
#include "otel_cli.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>

ABSL_FLAG(std::string, data_path, "", "Path to the KPMR XML data file");
ABSL_FLAG(std::string, weight_path, "", "Path to the Weight XML file");
ABSL_FLAG(std::string, output_path, "", "Path to the output XML file (default: input_output.xml)");

int main(int argc, char* argv[]) {
    // Set program usage message
    absl::SetProgramUsageMessage("Usage: kpmr_cli [options]\n"
                                 "Process KPMR (Consolidated Assessment) XML data with recursive evaluation");

    // Parse command-line flags
    std::vector<char*> remaining_args = absl::ParseCommandLine(argc, argv);

    // Get the data path
    std::string data_path = absl::GetFlag(FLAGS_data_path);
    std::string weight_path = absl::GetFlag(FLAGS_weight_path);
    auto otel = ilfx::otel::runtimeFromFlags("kpmr_cli");
    ilfx::otel::RootSpan root(otel, "kpmr_cli");
    root.setAttribute("data_path", data_path);
    root.setAttribute("weight_path", weight_path);

    // Validate that data_path is provided
    {
        ilfx::otel::ScopedSpan validate_span(otel, "kpmr_cli.validate");
        if (data_path.empty()) {
            const std::string message = "Error: --data_path is required";
            validate_span.markError(message);
            std::cerr << message << "\n";
            std::cerr << "Usage: kpmr_cli --data_path=<path> --weight_path=<path>\n";
            return root.finish(1);
        }

        // Validate that weight_path is provided
        if (weight_path.empty()) {
            const std::string message = "Error: --weight_path is required";
            validate_span.markError(message);
            std::cerr << message << "\n";
            std::cerr << "Usage: kpmr_cli --data_path=<path> --weight_path=<path>\n";
            return root.finish(1);
        }

        // Validate that the files exist
        if (!std::filesystem::exists(data_path)) {
            const std::string message = "Error: File not found: " + data_path;
            validate_span.markError(message);
            std::cerr << message << "\n";
            return root.finish(1);
        }

        if (!std::filesystem::exists(weight_path)) {
            const std::string message = "Error: File not found: " + weight_path;
            validate_span.markError(message);
            std::cerr << message << "\n";
            return root.finish(1);
        }
    }

    // Check if the data file has a .xml extension
    if (std::filesystem::path(data_path).extension() != ".xml") {
        std::cerr << "Warning: Data file does not have .xml extension\n";
    }

    try {
        LOG(INFO) << "Loading KPMR data from: " << data_path;
        LOG(INFO) << "Loading weights from: " << weight_path;
        
        // Create an Evaluator instance with the provided XML paths
        std::unique_ptr<kpmr::datasource::Evaluator> evaluator;
        {
            ilfx::otel::ScopedSpan load_span(otel, "kpmr_cli.load_data");
            load_span.setAttribute("data_path", data_path);
            load_span.setAttribute("weight_path", weight_path);
            evaluator = std::make_unique<kpmr::datasource::Evaluator>(data_path, weight_path);
        }
        
        LOG(INFO) << "Data loaded successfully";
        
        // Run evaluation
        OperationStatus status;
        {
            ilfx::otel::ScopedSpan evaluate_span(otel, "kpmr_cli.evaluate");
            try {
                status = evaluator->evaluate();
            } catch (const ilfx::chaiscript_diagnostics::EvaluationError& e) {
                ilfx::chaiscript_diagnostics::record(evaluate_span, e);
                throw;
            }
            if (status != SuccessOperationStatus) {
                evaluate_span.markError("Evaluation failed");
            }
        }
        
        if (status == SuccessOperationStatus) {
            LOG(INFO) << "Evaluation completed successfully";
            std::cout << "Successfully processed: " << data_path << "\n";

            LOG(INFO) << "Computed Inputs:";

            auto computedInputs = evaluator->getComputedInputs();
            for (const auto& [company, value] : computedInputs) {
                LOG(INFO) << "  " << company << ": " << value;
            }

            LOG(INFO) << "Writing data sources to file:";

            auto datasources = evaluator->getDataSources();

            // Get output path from flag or generate default
            std::string output_path = absl::GetFlag(FLAGS_output_path);
            if (output_path.empty()) {
                // Prepare default output file path (add _output suffix before extension)
                std::filesystem::path input_path(data_path);
                output_path = input_path.parent_path().string() + "/" + 
                             input_path.stem().string() + "_output" + 
                             input_path.extension().string();
            }
            root.setAttribute("output_path", output_path);

            LOG(INFO) << "Writing results to: " << output_path;

            xml_schema::namespace_infomap map;
            map[""].name = "http://example.com/kpmr";
            map[""].schema = "./xsd/KPMRDataSource.xsd";

            // Serialize the data to XML file
            ilfx::otel::ScopedSpan serialize_span(otel, "kpmr_cli.serialize_output");
            serialize_span.setAttribute("output_path", output_path);
            std::ofstream ofs(output_path);
            if (!ofs.is_open()) {
                const std::string message = "Failed to open output file: " + output_path;
                serialize_span.markError(message);
                LOG(ERROR) << "Failed to open output file: " << output_path;
                std::cerr << "Error: Failed to open output file: " << output_path << "\n";
                return root.finish(1);
            }

            data(ofs, *datasources, map);
            ofs.close();

            std::cout << "Results written to: " << output_path << "\n";

            return root.finish(0);
        } else {
            LOG(ERROR) << "Evaluation failed";
            std::cerr << "Evaluation failed\n";
            return root.finish(1);
        }
    } catch (const ilfx::chaiscript_diagnostics::EvaluationError& e) {
        ilfx::chaiscript_diagnostics::record(root, e);
        std::cerr << e.what() << "\n";
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
