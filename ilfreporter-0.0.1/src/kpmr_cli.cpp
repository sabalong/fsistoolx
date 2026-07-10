#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"

#include <xercesc/util/PlatformUtils.hpp>
#include <xqilla/xqilla-dom3.hpp>

#include "KPMRRiskProfile.hxx"
#include "riskprofile.hpp"
#include "otel_cli.hpp"

ABSL_FLAG(std::string, input_profile, "", "Path to KPMR risk profile XML file");
ABSL_FLAG(std::string, input_conf, "", "Path to aggregation config XML file");
ABSL_FLAG(std::string, output_report, "", "Path to output generated report");

int main(int argc, char** argv)
{
    absl::SetProgramUsageMessage("kpmr_cli --input_profile=<profile.xml> [--input_conf=<conf.xml>] [--output_report=<output.xml>]");
    absl::ParseCommandLine(argc, argv);

    const std::string profile_path = absl::GetFlag(FLAGS_input_profile);
    const std::string conf_path = absl::GetFlag(FLAGS_input_conf);
    const std::string output_report = absl::GetFlag(FLAGS_output_report);

    auto otel = ilfx::otel::runtimeFromEnvironment("ilfreporterkpmr");
    ilfx::otel::RootSpan root(otel, "ilfreporterkpmr");

    if (profile_path.empty() && conf_path.empty())
    {
        std::cerr << "--input_profile and --input_conf are required." << std::endl;
        return root.finish(1);
    }

    try
    {
        ilfx::otel::ScopedSpan initialize_span(otel, "ilfreporterkpmr.initialize");
        xercesc::XMLPlatformUtils::Initialize();
        XQillaPlatformUtils::initialize();
    }
    catch (const xercesc::XMLException& e)
    {
        char* msg = xercesc::XMLString::transcode(e.getMessage());
        std::cerr << "Failed to initialize XML platform: " << (msg ? msg : "<null>") << std::endl;
        xercesc::XMLString::release(&msg);
        root.markError("XML platform initialization failed");
        return root.finish(1);
    }

    int status = 0;
    try
    {
        ilfx::otel::ScopedSpan generate_span(otel, "ilfreporterkpmr.generate");
        auto tree_unique = kpmr::riskprofile::kpmr_risk_profile_tree_(profile_path, xml_schema::flags::keep_dom);
        std::shared_ptr<kpmr::riskprofile::kpmr_risk_profile_tree> tree(tree_unique.release());

         auto conf_unique = kpmr::conf::riskprofile::RiskProfileConf_(conf_path);
        std::shared_ptr<kpmr::conf::riskprofile::RiskProfileConf> conf(conf_unique.release());
        
        riskprofile::KPMRReportGenerator gen(tree, conf);

        auto report_status = gen.generateKPMRReport();
        if (!report_status.ok())
        {
                std::cerr << "Error generating KPMR report: " << report_status.message() << std::endl;
                generate_span.markError(std::string(report_status.message()));
                status = 1;
         }else{
                std::cout << "KPMR report generated successfully." << std::endl;
            }
        
        
       auto write_status = gen.writeKPMRRiskProfileConf(output_report);
       if (!write_status.ok()){
            std::cerr << "Error writing KPMR report: " << write_status.message() << std::endl;
            generate_span.markError(std::string(write_status.message()));
            status = 1;
        } else {
            std::cout << "KPMR report written successfully to: " << output_report << std::endl;
        }
    }
    
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        root.markError(e.what());
        status = 1;
    }

    try
    {
        xercesc::XMLPlatformUtils::Terminate();
        XQillaPlatformUtils::terminate();
    }
    catch (const xercesc::XMLException& e)
    {
        char* msg = xercesc::XMLString::transcode(e.getMessage());
        std::cerr << "Error during XML cleanup: " << (msg ? msg : "<null>") << std::endl;
        xercesc::XMLString::release(&msg);
    }

    return root.finish(status);
}
