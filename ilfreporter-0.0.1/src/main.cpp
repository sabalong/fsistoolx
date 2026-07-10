#include <iostream>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "riskprofile.hpp"
#include "otel_cli.hpp"
#include "InherentRiskProfile.hxx"
#include <xercesc/util/PlatformUtils.hpp>
#include <xqilla/xqilla-dom3.hpp>

ABSL_FLAG(std::string, input_profile, "", "Path to risk profile XML file");
ABSL_FLAG(std::string, input_conf, "", "Path to aggregation config XML file");
ABSL_FLAG(std::string, output, "", "Optional output file path");
ABSL_FLAG(std::string, xpath, "/RiskProfileTree/RiskProfileNode", "XPath to query within the risk profile");

int main(int argc, char** argv)
{
    absl::SetProgramUsageMessage("ilfreporter --input_profile=<profile.xml> --input_conf=<config.xml> [--output=<report.out>]");
    absl::ParseCommandLine(argc, argv);

    const std::string profile_path = absl::GetFlag(FLAGS_input_profile);
    const std::string conf_path = absl::GetFlag(FLAGS_input_conf);
    const std::string output_path = absl::GetFlag(FLAGS_output);

    auto otel = ilfx::otel::runtimeFromEnvironment("ilfreporter");
    ilfx::otel::RootSpan root(otel, "ilfreporter");

    if (profile_path.empty() || conf_path.empty())
    {
        std::cerr << "Both --input_profile and --input_conf are required." << std::endl;
        return root.finish(1);
    }

    try
    {
        ilfx::otel::ScopedSpan execute_span(otel, "ilfreporter.generate");
        xercesc::XMLPlatformUtils::Initialize();
        XQillaPlatformUtils::initialize();

        // Parse the risk profile XML into the generated tree model.
        auto tree_unique = RiskProfileTree_(profile_path, xml_schema::flags::keep_dom);
        std::shared_ptr<RiskProfileTree> tree(tree_unique.release());

        auto conf_unique = inherent::conf::InherentRiskProfileConf_(conf_path);
        std::shared_ptr<inherent::conf::InherentRiskProfileConf> conf(conf_unique.release());

        riskprofile::InherentReportGenerator generator(tree, conf);
        absl::Status st = generator.generateReport();
        if (!st.ok()) {
            std::cerr << "Report generation failed: " << st.message() << std::endl;
            execute_span.markError(std::string(st.message()));
            return root.finish(1);
        }

        if (!output_path.empty()) {
            st = generator.writeInherentRiskProfileConf(output_path);
            if (!st.ok()) {
                std::cerr << "Failed to write output config: " << st.message() << std::endl;
                execute_span.markError(std::string(st.message()));
                return root.finish(1);
            }
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Failed to query profile: " << ex.what() << std::endl;
        root.markError(ex.what());
        return root.finish(1);
    }

    return root.finish(0);
}
