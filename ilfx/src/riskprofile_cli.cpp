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

ABSL_FLAG(std::string, inherent_datasource, "", "Path to the Inherent DataSource XML file");
ABSL_FLAG(std::string, kpmr_datasource, "", "Path to the KPMR DataSource XML file");
ABSL_FLAG(std::string, inherent_riskprofile, "", "Path to the Inherent Risk Profile XML file");
ABSL_FLAG(std::string, kpmr_riskprofile, "", "Path to the KPMR Risk Profile XML file");
ABSL_FLAG(std::string, output_path, "", "Path to the output file (optional)");
ABSL_FLAG(std::string, output_inherent_riskprofile, "", "Path to write the evaluated Inherent Risk Profile XML (optional)");
ABSL_FLAG(std::string, output_kpmr_riskprofile, "", "Path to write the evaluated KPMR Risk Profile XML (optional)");

int main(int argc, char *argv[])
{
    // Set program usage message
    absl::SetProgramUsageMessage(
        "Usage: riskprofile_cli [options]\n"
        "Evaluate risk profiles using inherent and KPMR data sources\n\n"
        "Required flags:\n"
        "  --inherent_datasource=<path>  Path to Inherent DataSource XML\n"
        "  --kpmr_datasource=<path>      Path to KPMR DataSource XML\n"
        "  --inherent_riskprofile=<path> Path to Inherent Risk Profile XML\n"
        "  --kpmr_riskprofile=<path>     Path to KPMR Risk Profile XML\n\n"
        "Optional flags:\n"
        "  --output_path=<path>                    Path to output file (default: riskprofile_output.txt)\n"
        "  --output_inherent_riskprofile=<path>    Path to write evaluated Inherent Risk Profile XML");

    // Parse command-line flags
    std::vector<char *> remaining_args = absl::ParseCommandLine(argc, argv);

    // Get flag values
    std::string inherent_datasource = absl::GetFlag(FLAGS_inherent_datasource);
    std::string kpmr_datasource = absl::GetFlag(FLAGS_kpmr_datasource);
    std::string inherent_riskprofile = absl::GetFlag(FLAGS_inherent_riskprofile);
    std::string kpmr_riskprofile = absl::GetFlag(FLAGS_kpmr_riskprofile);
    std::string output_path = absl::GetFlag(FLAGS_output_path);
    std::string output_inherent_riskprofile = absl::GetFlag(FLAGS_output_inherent_riskprofile);
    std::string output_kpmr_riskprofile = absl::GetFlag(FLAGS_output_kpmr_riskprofile);
    auto otel = ilfx::otel::runtimeFromFlags("riskprofile_cli");
    ilfx::otel::RootSpan root(otel, "riskprofile_cli");
    root.setAttribute("inherent_datasource", inherent_datasource);
    root.setAttribute("kpmr_datasource", kpmr_datasource);
    root.setAttribute("inherent_riskprofile", inherent_riskprofile);
    root.setAttribute("kpmr_riskprofile", kpmr_riskprofile);
    root.setAttribute("output_path", output_path);
    root.setAttribute("output_inherent_riskprofile", output_inherent_riskprofile);
    root.setAttribute("output_kpmr_riskprofile", output_kpmr_riskprofile);

    // Validate required flags
    {
        ilfx::otel::ScopedSpan validate_span(otel, "riskprofile_cli.validate");
        bool has_error = false;
        if (inherent_datasource.empty())
        {
            const std::string message = "Error: --inherent_datasource is required";
            validate_span.markError(message);
            std::cerr << message << "\n";
            has_error = true;
        }

        if (inherent_riskprofile.empty())
        {
            const std::string message = "Error: --inherent_riskprofile is required";
            validate_span.markError(message);
            std::cerr << message << "\n";
            has_error = true;
        }

        if (has_error)
        {
            std::cerr << "\nUsage: riskprofile_cli --inherent_datasource=<path> --kpmr_datasource=<path> "
                      << "--inherent_riskprofile=<path> --kpmr_riskprofile=<path> [--output_path=<path>]\n";
            return root.finish(1);
        }

        // Validate that files exist
        if (!std::filesystem::exists(inherent_datasource))
        {
            const std::string message = "Error: Inherent DataSource file not found: " + inherent_datasource;
            validate_span.markError(message);
            std::cerr << message << "\n";
            return root.finish(1);
        }

        if (!std::filesystem::exists(inherent_riskprofile))
        {
            const std::string message = "Error: Inherent Risk Profile file not found: " + inherent_riskprofile;
            validate_span.markError(message);
            std::cerr << message << "\n";
            return root.finish(1);
        }
    }

    try
    {
        LOG(INFO) << "Loading Inherent DataSource from: " << inherent_datasource;

        // Parse XML files with correct namespaces
        std::shared_ptr<inherent::datasource::DataType> inherentDataSources;
        {
            ilfx::otel::ScopedSpan load_span(otel, "riskprofile_cli.load_data");
            load_span.setAttribute("inherent_datasource", inherent_datasource);
            load_span.setAttribute("inherent_riskprofile", inherent_riskprofile);
            inherentDataSources = std::move(inherent::datasource::data(inherent_datasource));

            LOG(INFO) << "Loading Inherent Risk Profile from: " << inherent_riskprofile;
        }
        std::shared_ptr<RiskProfileTree> inherentRiskProfile;
        {
            ilfx::otel::ScopedSpan load_span(otel, "riskprofile_cli.load_riskprofile");
            load_span.setAttribute("inherent_riskprofile", inherent_riskprofile);
            inherentRiskProfile = std::move(RiskProfileTree_(inherent_riskprofile));
        }

        LOG(INFO) << "All data loaded successfully";

        // Create evaluator with loaded data
        riskprofile::Evaluator evaluator(
            inherentDataSources,
            nullptr,
            inherentRiskProfile,
            nullptr);

        LOG(INFO) << "Starting risk profile evaluation...";

        // Run evaluation
        // OperationStatus status = evaluator.evaluate();

        LOG(INFO) << "Risk profile evaluation completed successfully";
        std::cout << "Successfully evaluated risk profiles\n";
        std::cout << "  Inherent DataSource: " << inherent_datasource << "\n";

        std::cout << "  Inherent Risk Profile: " << inherent_riskprofile << "\n";

        // Evaluate individual profiles
        LOG(INFO) << "Evaluating Inherent Risk Profile...";
        OperationStatus inherent_status;
        {
            ilfx::otel::ScopedSpan evaluate_span(otel, "riskprofile_cli.evaluate_inherent_riskprofile");
            inherent_status = evaluator.evaluateInherentRiskProfile();
            if (inherent_status != SuccessOperationStatus) {
                evaluate_span.markError("Inherent Risk Profile evaluation failed");
            }
        }
        if (inherent_status == SuccessOperationStatus)
        {
            LOG(INFO) << "Inherent Risk Profile evaluation completed";
        }
        else
        {
            LOG(WARNING) << "Inherent Risk Profile evaluation failed";
        }

        try
        {
            ilfx::otel::ScopedSpan serialize_span(otel, "riskprofile_cli.serialize_output");
            serialize_span.setAttribute("output_inherent_riskprofile", output_inherent_riskprofile);
            LOG(INFO) << "Writing evaluated Inherent Risk Profile XML to: " << output_inherent_riskprofile;

            // Create namespace map for serialization
       
            xml_schema::namespace_infomap map;
            map[""].schema = "./xsd/InherentRiskProfile.xsd";

            // Serialize to file
            std::ofstream xml_ofs(output_inherent_riskprofile);
            RiskProfileTree_(xml_ofs, *inherentRiskProfile, map);
            xml_ofs.close();

            LOG(INFO) << "Evaluated Inherent Risk Profile XML written successfully";
            std::cout << "Evaluated Inherent Risk Profile XML written to: " << output_inherent_riskprofile << "\n";
        }
        catch (const xml_schema::exception &e)
        {
            root.markError("Failed to write Inherent Risk Profile XML: " + std::string(e.what()));
            LOG(ERROR) << "Failed to write Inherent Risk Profile XML: " << e.what();
            std::cerr << "Error writing XML: " << e.what() << "\n";
            // Don't fail the whole program, just log the error
        }

        return root.finish(0);
    }
    catch (const std::exception &e)
    {
        root.markError("Exception occurred: " + std::string(e.what()));
        root.setAttribute("exception.type", "std::exception");
        root.setAttribute("exception.message", std::string(e.what()));
        LOG(ERROR) << "Error: " << e.what();
        std::cerr << "Error: " << e.what() << "\n";
        return root.finish(1);
    }
}
