#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/log.h"
#include "absl/log/initialize.h"
#include "absl/strings/str_format.h"
#include "riskprofile.hpp"
#include "otel_cli.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>


ABSL_FLAG(std::string, kpmr_datasource, "", "Path to the KPMR DataSource XML file");
ABSL_FLAG(std::string, kpmr_riskprofile, "", "Path to the KPMR Risk Profile XML file");
ABSL_FLAG(std::string, output_path, "", "Path to the output file (optional)");

int main(int argc, char* argv[]) {
    // Set program usage message
    absl::SetProgramUsageMessage(
        "Usage: riskprofile_cli [options]\n"
        "Evaluate risk profiles using KPMR data sources\n\n"
        "Required flags:\n"
        "  --kpmr_datasource=<path>      Path to KPMR DataSource XML\n"
        "  --kpmr_riskprofile=<path>     Path to KPMR Risk Profile XML\n\n"
        "Optional flags:\n"
        "  --output_path=<path>                    Path to output file (default: riskprofile_output.txt)\n"
    );

    // Parse command-line flags
    std::vector<char*> remaining_args = absl::ParseCommandLine(argc, argv);

    // Get flag values
    std::string kpmr_datasource = absl::GetFlag(FLAGS_kpmr_datasource);
    std::string kpmr_riskprofile = absl::GetFlag(FLAGS_kpmr_riskprofile);
    std::string output_path = absl::GetFlag(FLAGS_output_path);
    auto otel = ilfx::otel::runtimeFromFlags("riskprofilekpmr_cli");
    ilfx::otel::RootSpan root(otel, "riskprofilekpmr_cli");
    root.setAttribute("kpmr_datasource", kpmr_datasource);
    root.setAttribute("kpmr_riskprofile", kpmr_riskprofile);
    root.setAttribute("output_path", output_path);

    // Validate required flags
    {
        ilfx::otel::ScopedSpan validate_span(otel, "riskprofilekpmr_cli.validate");
        bool has_error = false;
        if (kpmr_datasource.empty()) {
            const std::string message = "Error: --kpmr_datasource is required";
            validate_span.markError(message);
            std::cerr << message << "\n";
            has_error = true;
        }
        if (kpmr_riskprofile.empty()) {
            const std::string message = "Error: --kpmr_riskprofile is required";
            validate_span.markError(message);
            std::cerr << message << "\n";
            has_error = true;
        }


        if (has_error) {
            std::cerr << "\nUsage: riskprofile_cli --kpmr_datasource=<path> --kpmr_riskprofile=<path> [--output_path=<path>]\n";
            return root.finish(1);
        }

        // Validate that files exist
   
        if (!std::filesystem::exists(kpmr_datasource)) {
            const std::string message = "Error: KPMR DataSource file not found: " + kpmr_datasource;
            validate_span.markError(message);
            std::cerr << message << "\n";
            return root.finish(1);
        }
   
        if (!std::filesystem::exists(kpmr_riskprofile)) {
            const std::string message = "Error: KPMR Risk Profile file not found: " + kpmr_riskprofile;
            validate_span.markError(message);
            std::cerr << message << "\n";
            return root.finish(1);
        }
    }

    try {
       
        // Parse XML files with correct namespaces
        LOG(INFO) << "Loading KPMR DataSource from: " << kpmr_datasource;
        std::shared_ptr<kpmr::datasource::ConsolidatedAssessmentType> kpmrDataSources;
        {
            ilfx::otel::ScopedSpan load_span(otel, "riskprofilekpmr_cli.load_data");
            load_span.setAttribute("kpmr_datasource", kpmr_datasource);
            kpmrDataSources = std::move(kpmr::datasource::data(kpmr_datasource));
        }
       
        LOG(INFO) << "Loading KPMR Risk Profile from: " << kpmr_riskprofile;
        std::shared_ptr<kpmr::riskprofile::kpmr_risk_profile_tree> kpmrRiskProfileTree;
        {
            ilfx::otel::ScopedSpan load_span(otel, "riskprofilekpmr_cli.load_riskprofile");
            load_span.setAttribute("kpmr_riskprofile", kpmr_riskprofile);
            kpmrRiskProfileTree = std::move(kpmr::riskprofile::kpmr_risk_profile_tree_(kpmr_riskprofile));
        }

        LOG(INFO) << "All data loaded successfully";
        LOG(INFO) << "Number of top-level nodes: " << kpmrRiskProfileTree->node().size();
        
        // Debug: check first node
        if (!kpmrRiskProfileTree->node().empty()) {
            const auto& firstNode = kpmrRiskProfileTree->node()[0];
            LOG(INFO) << "First node profile_id: " << firstNode.profile_id();
            LOG(INFO) << "First node has children: " << (firstNode.children().present() ? "YES" : "NO");
            if (firstNode.children().present()) {
                LOG(INFO) << "Number of children: " << firstNode.children()->node().size();
            }
        }

        // Create evaluator with loaded data
        riskprofile::Evaluator evaluator(
            nullptr,
            kpmrDataSources,
            nullptr,
            kpmrRiskProfileTree
        );

        LOG(INFO) << "Starting risk profile evaluation...";

        // Run evaluation
        OperationStatus status;
        {
            ilfx::otel::ScopedSpan evaluate_span(otel, "riskprofilekpmr_cli.evaluate_kpmr_riskprofile");
            try {
                status = evaluator.evaluateKPMRRiskProfile();
            } catch (const ilfx::chaiscript_diagnostics::EvaluationError& e) {
                ilfx::chaiscript_diagnostics::record(evaluate_span, e);
                throw;
            }
            if (status != SuccessOperationStatus) {
                evaluate_span.markError("Risk profile evaluation failed");
            }
        }

        if (status == SuccessOperationStatus) {
            LOG(INFO) << "Risk profile evaluation completed successfully";
            std::cout << "Successfully evaluated risk profiles\n";
           

            // Handle output
            if (output_path.empty()) {
                output_path = "riskprofile_output.txt";
            }
            root.setAttribute("output_path", output_path);

            LOG(INFO) << "Writing results to: " << output_path;

 

            // Write evaluated KPMR Risk Profile XML if output path is specified
           
                try {
                    ilfx::otel::ScopedSpan serialize_span(otel, "riskprofilekpmr_cli.serialize_output");
                    serialize_span.setAttribute("output_path", output_path);
                    LOG(INFO) << "Writing evaluated KPMR Risk Profile XML to: " << output_path;
                    
                    // Create namespace map for serialization
                    xml_schema::namespace_infomap map;
                    map[""].name = "http://example.com/kpmr";
                    map[""].schema = "./xsd/KPMRRiskProfile.xsd";
                    
                    // Serialize to file
                    std::ofstream xml_ofs(output_path);
                    kpmr::riskprofile::kpmr_risk_profile_tree_(xml_ofs, *kpmrRiskProfileTree, map);
                    xml_ofs.close();
                    
                    LOG(INFO) << "Evaluated KPMR Risk Profile XML written successfully";
                    std::cout << "Evaluated KPMR Risk Profile XML written to: " << output_path << "\n";
                } catch (const xml_schema::exception& e) {
                    root.markError("Failed to write KPMR Risk Profile XML: " + std::string(e.what()));
                    LOG(ERROR) << "Failed to write KPMR Risk Profile XML: " << e.what();
                    std::cerr << "Error writing XML: " << e.what() << "\n";
                    // Don't fail the whole program, just log the error
                }
            

            return root.finish(0);
        } else {
            LOG(ERROR) << "Risk profile evaluation failed";
            std::cerr << "Error: Risk profile evaluation failed\n";
            return root.finish(1);
        }
    } catch (const ilfx::chaiscript_diagnostics::EvaluationError& e) {
        ilfx::chaiscript_diagnostics::record(root, e);
        std::cerr << e.what() << "\n";
        return root.finish(1);
    } catch (const std::exception& e) {
        root.markError("Exception occurred: " + std::string(e.what()));
        root.setAttribute("exception.type", "std::exception");
        root.setAttribute("exception.message", std::string(e.what()));
        LOG(ERROR) << "Error: " << e.what();
        std::cerr << "Error: " << e.what() << "\n";
        return root.finish(1);
    }
}
