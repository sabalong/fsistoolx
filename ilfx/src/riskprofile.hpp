#ifndef ILFX_RISKPROFILE_HPP
#define ILFX_RISKPROFILE_HPP

#include "allheaders.hpp"
#include "InherentDataSource.hxx"
#include "KPMRDataSource.hxx"
#include "InherentRiskProfile.hxx"
#include "KPMRRiskProfile.hxx"
#include <chaiscript/chaiscript.hpp>
#include <chaiscript/extras/math.hpp>
#include <antlr4-runtime.h>
#include "ThresholdLexer.h"
#include "ThresholdParser.h"
#include "EvalVisitor.hpp"
#include "scoreratingparser.hpp"
#include <map>
#include <sstream>
#include <unordered_map>

// Forward declaration
bool evalExprWithX(double x, const std::string& expr);
bool evalExprWithVariables(const std::unordered_map<std::string, double>& variables, const std::string& expr);

namespace riskprofile {
    inline double boxedValueToThresholdDouble(const std::string& name, const chaiscript::Boxed_Value& value) {
        if (value.is_undef()) {
            throw std::runtime_error("Threshold variable '" + name + "' is undefined");
        }

        try {
            return chaiscript::boxed_cast<double>(value);
        } catch (const chaiscript::exception::bad_boxed_cast&) {
        }

        try {
            return static_cast<double>(chaiscript::boxed_cast<int>(value));
        } catch (const chaiscript::exception::bad_boxed_cast&) {
        }

        try {
            return static_cast<double>(chaiscript::boxed_cast<long>(value));
        } catch (const chaiscript::exception::bad_boxed_cast&) {
        }

        try {
            return static_cast<double>(chaiscript::boxed_cast<long long>(value));
        } catch (const chaiscript::exception::bad_boxed_cast&) {
        }

        try {
            return static_cast<double>(chaiscript::boxed_cast<float>(value));
        } catch (const chaiscript::exception::bad_boxed_cast&) {
        }

        throw std::runtime_error("Threshold variable '" + name + "' must be numeric");
    }

    inline std::unordered_map<std::string, double> thresholdVariablesFromChaiMap(
        const std::map<std::string, chaiscript::Boxed_Value>& boxedVariables) {
        std::unordered_map<std::string, double> variables;

        for (const auto& entry : boxedVariables) {
            variables.emplace(entry.first, boxedValueToThresholdDouble(entry.first, entry.second));
        }

        return variables;
    }

    class Evaluator
    {
    private:
        std::shared_ptr<inherent::datasource::DataType> inherentDataSources;
        std::shared_ptr<kpmr::datasource::ConsolidatedAssessmentType> datasources;
        std::shared_ptr<RiskProfileTree> inherentRiskProfile;
        std::shared_ptr<kpmr::riskprofile::kpmr_risk_profile_tree> kpmrRiskProfile;
        
        // Helper function to set up ChaiScript evaluator with all necessary bindings
        void setupChaiScriptEvaluator(chaiscript::ChaiScript& chai);

    public:
        Evaluator(std::shared_ptr<inherent::datasource::DataType> inherentDataSources,
                  std::shared_ptr<kpmr::datasource::ConsolidatedAssessmentType> kpmrDataSources, std::shared_ptr<RiskProfileTree> inherentRiskProfile, std::shared_ptr<kpmr::riskprofile::kpmr_risk_profile_tree> kpmrRiskProfile)
            : inherentDataSources(inherentDataSources), datasources(kpmrDataSources), inherentRiskProfile(inherentRiskProfile), kpmrRiskProfile(kpmrRiskProfile) {};
        ~Evaluator() = default;

        OperationStatus evaluate();
        OperationStatus evaluateInherentRiskProfile();
        OperationStatus evaluateKPMRRiskProfile();

        double findConsolidateByCode(const std::string& code) {
         for (auto item : inherentDataSources->list().item()) {
                if (item.code() == code) {
                    return item.consolidate();
                }
         }

         return 0.0;
        };

        double computedScoreByCode(const std::string& code) {
            // Helper function to recursively search through nodes
            std::function<double(const RiskProfileNodeType&)> searchNode = 
                [&](const RiskProfileNodeType& node) -> double {
                    if (node.Profile_ID() == code) {
                        if (node.computed_score().present()) {
                            return std::strtod(node.computed_score().get().c_str(), nullptr);
                        }
                        return 0.0;
                    }
                    
                    // Recursively search in children
                    for (const auto& child : node.RiskProfileNode()) {
                        double result = searchNode(child);
                        if (result != 0.0 || child.Profile_ID() == code) {
                            return result;
                        }
                    }
                    
                    return 0.0;
                };
            
            for (const auto& item : inherentRiskProfile->RiskProfileNode()) {
                double result = searchNode(item);
                if (result != 0.0 || item.Profile_ID() == code) {
                    return result;
                }
            }
            
            return 0.0;
        };

        double computedValueByCode(const std::string& code) {
            // Helper function to recursively search through nodes
            std::function<double(const RiskProfileNodeType&)> searchNode = 
                [&](const RiskProfileNodeType& node) -> double {
                    if (node.Profile_ID() == code) {
                        if (node.computed_value().present()) {
                            return std::strtod(node.computed_value().get().c_str(), nullptr);
                        }
                        return 0.0;
                    }
                    
                    // Recursively search in children
                    for (const auto& child : node.RiskProfileNode()) {
                        double result = searchNode(child);
                        if (result != 0.0 || child.Profile_ID() == code) {
                            return result;
                        }
                    }
                    
                    return 0.0;
                };
            
            for (const auto& item : inherentRiskProfile->RiskProfileNode()) {
                double result = searchNode(item);
                if (result != 0.0 || item.Profile_ID() == code) {
                    return result;
                }
            }
            
            return 0.0;
        };

        double weightByCode(const std::string& code) {
            // Helper function to recursively search through nodes
            std::function<double(const RiskProfileNodeType&)> searchNode = 
                [&](const RiskProfileNodeType& node) -> double {
                    if (node.Profile_ID() == code) {
                        if (node.weight().present()) {
                            return node.weight().get();
                        }
                        return 1.0;
                    }
                    
                    // Recursively search in children
                    for (const auto& child : node.RiskProfileNode()) {
                        double result = searchNode(child);
                        if (result != 0.0 || child.Profile_ID() == code) {
                            return result;
                        }
                    }
                    
                    return 1.0;
                };
            
            for (const auto& item : inherentRiskProfile->RiskProfileNode()) {
                double result = searchNode(item);
                if (result != 0.0 || item.Profile_ID() == code) {
                    return result;
                }
            }
            
            return 0.0;
        };

        double weightByCodeKPMR(const std::string& code) {
            // Helper function to recursively search through KPMR nodes
            std::function<double(const kpmr::riskprofile::NodeType&)> searchNode = 
                [&](const kpmr::riskprofile::NodeType& node) -> double {
                    if (node.profile_id() == code) {
                        if (node.weight().present()) {
                            return node.weight().get();
                        }
                        return 1.0;
                    }
                    
                    // Recursively search in children if present
                    if (node.children().present()) {
                        for (const auto& child : node.children()->node()) {
                            double result = searchNode(child);
                            if (result != 0.0 || child.profile_id() == code) {
                                return result;
                            }
                        }
                    }
                    
                    return 1.0;
                };
            
            for (const auto& item : kpmrRiskProfile->node()) {
                double result = searchNode(item);
                if (result != 0.0 || item.profile_id() == code) {
                    return result;
                }
            }
            
            return 0.0;
        };

        double computedWeightedScoreByCode(const std::string& code) {
            // Helper function to recursively search through nodes
            std::function<double(const RiskProfileNodeType&)> searchNode = 
                [&](const RiskProfileNodeType& node) -> double {
                    if (node.Profile_ID() == code) {
                        std::string scoreRule = node.score_rule().present() ? node.score_rule().get() : "";
                        if (scoreRule.empty()) {
                            return 0.0;
                        }

                        LOG(INFO) << "Evaluating score for code: " << code << " using rule: " << scoreRule;

                        chaiscript::ChaiScript chai;
                        setupChaiScriptEvaluator(chai);

                        LOG(INFO) << "hooho";
                        auto score = chai.eval<double>(scoreRule);

                        if (node.weight().present()) {
                            score *= node.weight().get();
                        }

                        LOG(INFO) << "hooh2o";

                        return score;
                    }
                    
                    // Recursively search in children
                    for (const auto& child : node.RiskProfileNode()) {
                        double result = searchNode(child);
                        if (result != 0.0 || child.Profile_ID() == code) {
                            return result;
                        }
                    }
                    
                    return 0.0;
                };
            
            for (const auto& item : inherentRiskProfile->RiskProfileNode()) {
                double result = searchNode(item);
                if (result != 0.0 || item.Profile_ID() == code) {
                    return result;
                }
            }
            
            return 0.0;
        };

        double computedWeightedScoreByCodeKPMR(const std::string& code) {
            // Helper function to recursively search through KPMR nodes
            std::function<double(const kpmr::riskprofile::NodeType&)> searchNode =
                [&](const kpmr::riskprofile::NodeType& node) -> double {
                    if (node.profile_id() == code) {
                        std::string scoreRule = node.score_rule().present() ? node.score_rule().get() : "";
                        if (scoreRule.empty()) {
                            return 0.0;
                        }

                        LOG(INFO) << "Evaluating KPMR score for code: " << code << " using rule: " << scoreRule;

                        chaiscript::ChaiScript chai;
                        setupChaiScriptEvaluator(chai);

                        auto score = chai.eval<double>(scoreRule);

                        if (node.weight().present()) {
                            score *= node.weight().get();
                        }

                        return score;
                    }

                    // Recursively search in children if present
                    if (node.children().present()) {
                        for (const auto& child : node.children()->node()) {
                            double result = searchNode(child);
                            if (result != 0.0 || child.profile_id() == code) {
                                return result;
                            }
                        }
                    }

                    return 0.0;
                };

            for (const auto& item : kpmrRiskProfile->node()) {
                double result = searchNode(item);
                if (result != 0.0 || item.profile_id() == code) {
                    return result;
                }
            }

            return 0.0;
        };

        int computedRatingByCode(const std::string& code) {
            // Helper function to recursively search through nodes
            std::function<int(const RiskProfileNodeType&)> searchNode = 
                [&](const RiskProfileNodeType& node) -> int {
                    if (node.Profile_ID() == code) {
                        if (node.computed_rating().present()) {
                            return std::stoi(node.computed_rating().get());
                        }
                        return 0;
                    }
                    
                    // Recursively search in children
                    for (const auto& child : node.RiskProfileNode()) {
                        int result = searchNode(child);
                        if (result != 0 || child.Profile_ID() == code) {
                            return result;
                        }
                    }
                    
                    return 0;
                };
            
            for (const auto& item : inherentRiskProfile->RiskProfileNode()) {
                int result = searchNode(item);
                if (result != 0 || item.Profile_ID() == code) {
                    return result;
                }
            }
            
            return 0;
        };

        double findConsolidateKPMRByCode(const std::string& code) {
            // Check if datasources is valid
            if (!datasources) {
                return 0.0;
            }
            
            // Helper function to recursively search through RiskItemType children
            std::function<double(const kpmr::datasource::RiskItemType&)> searchInItem = 
                [&](const kpmr::datasource::RiskItemType& item) -> double {
                    // Check if current item matches the code
                    if (item.code() == code) {
                        if (item.consolidate().present()) {
                            return static_cast<double>(item.consolidate().get());
                        }
                        return 0.0;
                    }
                    
                    // Recursively search in children if present
                    if (item.children().present()) {
                        for (const auto& child : item.children()->item()) {
                            double result = searchInItem(child);
                            if (result != 0.0 || child.code() == code) {
                                return result;
                            }
                        }
                    }
                    
                    return 0.0;
                };
            
            // Iterate through the top-level list
            for (const auto& riskGroup : datasources->list()) {
                // Check if the risk group itself matches
                if (riskGroup.code() == code) {
                    return static_cast<double>(riskGroup.value());
                }
                
                // Search in children if present
                if (riskGroup.children().present()) {
                    for (const auto& item : riskGroup.children()->item()) {
                        double result = searchInItem(item);
                        if (result != 0.0 || item.code() == code) {
                            return result;
                        }
                    }
                }
            }
            
            return 0.0;
        };
        
        std::string thresholdByCode(const std::string& code) {
            // Helper function to recursively search through nodes
            std::function<std::string(const RiskProfileNodeType&)> searchNode = 
                [&](const RiskProfileNodeType& node) -> std::string {
                    if (node.Profile_ID() == code) {
                        if (node.threshold().present()) {
                            return node.threshold().get();
                        }
                        return "";
                    }
                    
                    // Recursively search in children
                    for (const auto& child : node.RiskProfileNode()) {
                        std::string result = searchNode(child);
                        if (!result.empty() || child.Profile_ID() == code) {
                            return result;
                        }
                    }
                    
                    return "";
                };
            
            for (const auto& item : inherentRiskProfile->RiskProfileNode()) {
                std::string result = searchNode(item);
                if (!result.empty() || item.Profile_ID() == code) {
                    return result;
                }
            }
            
            return "";
        };
        
        std::string thresholdByCodeKPMR(const std::string& code) {
            // Helper function to recursively search through KPMR nodes
            std::function<std::string(const kpmr::riskprofile::NodeType&)> searchNode = 
                [&](const kpmr::riskprofile::NodeType& node) -> std::string {
                    if (node.profile_id() == code) {
                        return node.threshold();;
                    }
                    
                    // Recursively search in children if present
                    if (node.children().present()) {
                        for (const auto& child : node.children()->node()) {
                            std::string result = searchNode(child);
                            if (!result.empty() || child.profile_id() == code) {
                                return result;
                            }
                        }
                    }
                    
                    return "";
                };
            
            for (const auto& item : kpmrRiskProfile->node()) {
                std::string result = searchNode(item);
                if (!result.empty() || item.profile_id() == code) {
                    return result;
                }
            }
            
            return "";
        };
        
        std::string scoreFormulaByCode(const std::string& code) {
            // Helper function to recursively search through nodes
            std::function<std::string(const RiskProfileNodeType&)> searchNode = 
                [&](const RiskProfileNodeType& node) -> std::string {
                    if (node.Profile_ID() == code) {
                        if (node.rating_to_score().present()) {
                            return node.rating_to_score().get();
                        }
                        return "";
                    }
                    
                    // Recursively search in children
                    for (const auto& child : node.RiskProfileNode()) {
                        std::string result = searchNode(child);
                        if (!result.empty() || child.Profile_ID() == code) {
                            return result;
                        }
                    }
                    
                    return "";
                };
            
            for (const auto& item : inherentRiskProfile->RiskProfileNode()) {
                std::string result = searchNode(item);
                if (!result.empty() || item.Profile_ID() == code) {
                    return result;
                }
            }
            
            return "";
        };
        
        std::string scoreFormulaByCodeKPMR(const std::string& code) {
            // Helper function to recursively search through KPMR nodes
            std::function<std::string(const kpmr::riskprofile::NodeType&)> searchNode = 
                [&](const kpmr::riskprofile::NodeType& node) -> std::string {
                    if (node.profile_id() == code) {
                       
                        return node.score_formula().get();
                    }
                    
                    // Recursively search in children if present
                    if (node.children().present()) {
                        for (const auto& child : node.children()->node()) {
                            std::string result = searchNode(child);
                            if (!result.empty() || child.profile_id() == code) {
                                return result;
                            }
                        }
                    }
                    
                    return "";
                };
            
            for (const auto& item : kpmrRiskProfile->node()) {
                std::string result = searchNode(item);
                if (!result.empty() || item.profile_id() == code) {
                    return result;
                }
            }
            
            return "";
        };
        
        int ratingByThreshold(const std::string& threshold, double value) {
            return ratingByThreshold(threshold, std::unordered_map<std::string, double>{{"x", value}});
        };

        int ratingByThresholdVars(const std::string& threshold, const std::map<std::string, chaiscript::Boxed_Value>& boxedVariables) {
            return ratingByThreshold(threshold, thresholdVariablesFromChaiMap(boxedVariables));
        };

        int ratingByThreshold(const std::string& threshold, const std::unordered_map<std::string, double>& variables) {
            std::cout << "Evaluating rating using threshold: " << threshold << std::endl;
            // Split threshold by \n
            std::istringstream stream(threshold);
            std::string line;
            
            while (std::getline(stream, line)) {
                // Trim leading and trailing whitespace
                line.erase(0, line.find_first_not_of(" \t\r\n"));
                line.erase(line.find_last_not_of(" \t\r\n") + 1);
                
                if (line.empty()) continue;
                
                try {
                    // Remove extra spaces around colon
                    size_t colonPos = line.find(':');
                    if (colonPos == std::string::npos) continue;
                    
                    std::string ratingStr = line.substr(0, colonPos);
                    std::string exprStr = line.substr(colonPos + 1);
                    
                    // Trim the expression
                    exprStr.erase(0, exprStr.find_first_not_of(" \t"));
                    exprStr.erase(exprStr.find_last_not_of(" \t") + 1);
                    
                    std::cout << "  Parsing rating: '" << ratingStr << "' with expr: '" << exprStr << "'" << std::endl;
                    
                    // Parse with ANTLR
                    antlr4::ANTLRInputStream inputStream(exprStr);
                    ThresholdLexer lexer(&inputStream);
                    antlr4::CommonTokenStream tokens(&lexer);
                    ThresholdParser parser(&tokens);
                    
                    // Parse as expression directly
                    ThresholdParser::ExprContext* tree = parser.expr();
                    
                    // Extract expression from parse tree
                    EvalVisitor visitor;
                    std::string compiledExpr = std::any_cast<std::string>(visitor.visitExpr(tree));
                    
                    std::cout << "  Compiled expression: '" << compiledExpr << "'" << std::endl;
                    
                    // Evaluate the expression with the given variables
                    if (evalExprWithVariables(variables, compiledExpr)) {
                        // Return the rating if the expression matches
                        int rating = std::stoi(ratingStr);
                        std::cout << "  Rating matched: " << rating << std::endl;
                        return rating;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing threshold line: '" << line << "' - " << e.what() << std::endl;
                    continue;
                }
            }
            
            // No matching rule found
            return -1;
        };

        double ratingToScore(const std::string& ratingToScoreStr, int rating) {
            std::cout << "Mapping rating to score using string: " << ratingToScoreStr << " for rating: " << rating << std::endl;
            
            // Parse the rating to score mapping (format: "1:92\n2:76\n3:60\n4:44\n5:28")
            std::istringstream stream(ratingToScoreStr);
            std::string line;
            
            while (std::getline(stream, line)) {
                if (line.empty()) continue;
                
                // Split by ':'
                size_t colonPos = line.find(':');
                if (colonPos == std::string::npos) continue;
                
                try {
                    int lineRating = std::stoi(line.substr(0, colonPos));
                    double score = std::stod(line.substr(colonPos + 1));
                    
                    if (lineRating == rating) {
                        std::cout << "Found matching rating " << rating << " -> score " << score << std::endl;
                        return score;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing rating-to-score line: " << line << " - " << e.what() << std::endl;
                    continue;
                }
            }
            
            // No matching rating found
            std::cout << "No matching rating found for: " << rating << std::endl;
            return 0.0;
        };
        
    private:
        void processInherentRiskNode(RiskProfileNodeType& node, int depth = 0);
        void processKPMRRiskNode(kpmr::riskprofile::NodeType& node, int depth = 0);
    };

    inline void Evaluator::setupChaiScriptEvaluator(chaiscript::ChaiScript& chai)
    {
        // Add math library
        auto mathlib = chaiscript::extras::math::bootstrap();
        chai.add(mathlib);
        
        // Add inherent datasource helper functions
        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->findConsolidateByCode(code);
                     }),
                 "findConsolidateByCode");

        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->computedScoreByCode(code);
                     }),
                 "computedScoreByCode");

        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->computedValueByCode(code);
                     }),
                 "computedValueByCode");

        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->weightByCode(code);
                     }),
                 "weightByCode");

        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->computedWeightedScoreByCode(code);
                     }),
                 "computedWeightedScoreByCode");

        // Add KPMR datasource helper functions
        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->findConsolidateKPMRByCode(code);
                     }),
                 "findConsolidateKPMRByCode");

        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->weightByCodeKPMR(code);
                     }),
                 "weightByCodeKPMR");

        // Add threshold lookup function
        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->thresholdByCode(code);
                     }),
                 "thresholdByCode");

        // Add KPMR threshold lookup function
        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->thresholdByCodeKPMR(code);
                     }),
                 "thresholdByCodeKPMR");

        // Add score formula lookup function
        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->scoreFormulaByCode(code);
                     }),
                 "scoreFormulaByCode");

        // Add KPMR score formula lookup function
        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->scoreFormulaByCodeKPMR(code);
                     }),
                 "scoreFormulaByCodeKPMR");

        // Add rating and scoring functions
        chai.add(chaiscript::fun(
                     [this](const std::string &threshold, double value)
                     {
                         return this->ratingByThreshold(threshold, value);
                     }),
                 "ratingByThreshold");

        chai.add(chaiscript::fun(
                     [this](const std::string &threshold, const std::map<std::string, chaiscript::Boxed_Value> &variables)
                     {
                         return this->ratingByThresholdVars(threshold, variables);
                     }),
                 "ratingByThresholdVars");

        chai.add(chaiscript::fun(
                     [this](const std::string &ratingToScoreStr, int rating)
                     {
                         return this->ratingToScore(ratingToScoreStr, rating);
                     }),
                 "scoreByRating");

        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->computedRatingByCode(code);
                     }),
                 "computedRatingByCode");
        
        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->computedWeightedScoreByCodeKPMR(code);
                     }),
                 "computedWeightedScoreByCodeKPMR");
    }

    inline OperationStatus Evaluator::evaluate()
    {
        // Dummy implementation
        std::cout << "Evaluating risk profile..." << std::endl;

            evaluateInherentRiskProfile();
           // evaluateKPMRRiskProfile();
        return SuccessOperationStatus;
    }
    
    inline void Evaluator::processInherentRiskNode(RiskProfileNodeType& node, int depth)
    {
        // Recursively process children first
        for (auto& childNode : node.RiskProfileNode()) {
            processInherentRiskNode(childNode, depth + 1);
        }
        
        std::string indent(depth * 2, ' ');
        std::cout << indent << "Processing Inherent Risk Profile Node: " << node.Profile_ID() << std::endl;
        
        // Only print risiko_name if it's present (it's optional)
        if (node.assessment_factor().present()) {
            std::cout << indent << "  Assessment Name: " << node.assessment_factor().get() << std::endl;
        }

        // calculate the rules first
        if (node.value_rule().present() && !node.value_rule().get().empty()) {
            std::cout << indent << "  Value Rule: " << node.value_rule().get() << std::endl;

            // Create fresh ChaiScript instance for this rule
            chaiscript::ChaiScript chai;
            setupChaiScriptEvaluator(chai);
            
            if (node.threshold().present()) {
                chai.add(chaiscript::var(std::string(node.threshold().get())), "threshold");
            }
            if (node.score_formula().present()) {
                chai.add(chaiscript::var(std::string(node.score_formula().get())), "rating_to_score");
            }
            
            auto result = 0.0;

            try {
                result = chai.eval<double>(node.value_rule().get());

            }catch (const std::exception& e) {
                std::cerr << "Error evaluating value rule for node " << node.Profile_ID() << ": " << e.what() << std::endl;
                result = 0.0; // Default to 0 on error
            }

            std::cout << indent << "    Evaluated Value Rule Result: " << result << std::endl;

            node.computed_value(std::to_string(result));
        }

        if (node.rating_rule().present() && !node.rating_rule().get().empty()) {
            std::cout << indent << "  Rating Rule: " << node.rating_rule().get() << std::endl;

            // Create fresh ChaiScript instance for this rule
            chaiscript::ChaiScript chai;
            setupChaiScriptEvaluator(chai);
            
            if (node.threshold().present()) {
                chai.add(chaiscript::var(std::string(node.threshold().get())), "threshold");
            }
            if (node.score_formula().present()) {
                chai.add(chaiscript::var(std::string(node.score_formula().get())), "rating_to_score");
            }
            if (node.computed_value().present()) {
                chai.add(chaiscript::var(std::strtod(node.computed_value().get().c_str(), nullptr)), "value");
            }

            auto result = -1;
            try {
                result = chai.eval<int>(node.rating_rule().get());
            } catch (const std::exception& e) {
                std::cerr << "Error evaluating rating rule for node " << node.Profile_ID() << ": " << e.what() << std::endl;
                result = -1; // Default to -1 on error
            }

            std::cout << indent << "    Evaluated Rating Rule Result: " << result << std::endl;

            node.computed_rating(std::to_string(result));
        }

        if (node.score_rule().present() && !node.score_rule().get().empty()) {
            std::cout << indent << "  Score Rule: " << node.score_rule().get() << std::endl;

            // Create fresh ChaiScript instance for this rule
            chaiscript::ChaiScript chai;
            setupChaiScriptEvaluator(chai);
            
            if (node.threshold().present()) {
                chai.add(chaiscript::var(std::string(node.threshold().get())), "threshold");
            }
            if (node.score_formula().present()) {
                chai.add(chaiscript::var(std::string(node.score_formula().get())), "rating_to_score");
            }
            if (node.computed_value().present()) {
                chai.add(chaiscript::var(std::strtod(node.computed_value().get().c_str(), nullptr)), "value");
            }
            if (node.computed_rating().present()) {
                chai.add(chaiscript::var(std::stoi(node.computed_rating().get())), "rating");
            }

            auto result = 0.0;
            try {
                result = chai.eval<double>(node.score_rule().get());
            } catch (const std::exception& e) {
                std::cerr << "Error evaluating score rule for node " << node.Profile_ID() << ": " << e.what() << std::endl;
                result = 0.0; // Default to 0 on error
            }

            std::cout << indent << "    Evaluated Score Rule Result: " << result << std::endl;

            node.computed_score(std::to_string(result));

            double weight = 1.0;

            if (node.weight().present()) {
                weight = node.weight().get();
            }

            double weightedScore = result * weight;
            std::cout << indent << "    Weighted Score: " << weightedScore << std::endl;

            node.computed_weighted_score(std::to_string(weightedScore));
        }

        
        
        // Add evaluation logic here
    }
    
    inline OperationStatus Evaluator::evaluateInherentRiskProfile()
    {
        for (auto &node : inherentRiskProfile->RiskProfileNode()) {
            processInherentRiskNode(node, 0);
        }
        return SuccessOperationStatus;
    }
    
    inline void Evaluator::processKPMRRiskNode(kpmr::riskprofile::NodeType& node, int depth)
    {
        std::string indent(depth * 2, ' ');
        std::cout << indent << "Processing KPMR Risk Profile Node: " << node.profile_id() << std::endl;
        std::cout << indent << "  Risk Name: " << node.risiko_name() << std::endl;
        std::cout << indent << "  Children present: " << (node.children().present() ? "YES" : "NO") << std::endl;
        
        // Recursively process children first
        if (node.children().present()) {
            std::cout << indent << "  Entering children processing..." << std::endl;
            std::cout << indent << "  Number of children: " << node.children()->node().size() << std::endl;
            
            for (auto& childNode : node.children()->node()) {
                processKPMRRiskNode(childNode, depth + 1);

                if (childNode.rating_rule().present() && !childNode.rating_rule().get().empty()) {
                    std::string indentChild((depth + 1) * 2, ' ');
                    std::cout << indentChild << "  Child Rating Rule: " << childNode.rating_rule().get() << std::endl;

                    // Create fresh ChaiScript instance for this rule
                    chaiscript::ChaiScript chai;
                    setupChaiScriptEvaluator(chai);

                    auto computed_rating = -1;
                    try {
                        computed_rating = chai.eval<int>(childNode.rating_rule().get());
                    } catch (const std::exception& e) {
                        std::cerr << "Error evaluating rating rule for child node " << childNode.profile_id() << ": " << e.what() << std::endl;
                        computed_rating = -1; // Default to -1 on error
                    }

                    std::cout << indentChild << "Computed Child Rating: " << computed_rating << std::endl;

                    childNode.computed_rating(std::to_string(computed_rating));
                }

                if (childNode.score_rule().present() && !childNode.score_rule().get().empty()) {
                    std::string indentChild((depth + 1) * 2, ' ');
                    std::cout << indentChild << "  Child Score Rule: " << childNode.score_rule().get() << std::endl;

                    // Create fresh ChaiScript instance for this rule
                    chaiscript::ChaiScript chai;
                    setupChaiScriptEvaluator(chai);
                    
                    if (childNode.computed_rating().present()) {
                        chai.add(chaiscript::var(std::stoi(childNode.computed_rating().get())), "rating");
                    }

                    auto computed_score = 0.0;
                    try {
                        computed_score = chai.eval<double>(childNode.score_rule().get());
                    } catch (const std::exception& e) {
                        std::cerr << "Error evaluating score rule for child node " << childNode.profile_id() << ": " << e.what() << std::endl;
                        computed_score = 0.0; // Default to 0 on error
                    }

                    std::cout << indentChild << "Computed Child Score: " << computed_score << std::endl;

                    childNode.computed_score(std::to_string(computed_score));


                    double weight = 1.0;

                    if (childNode.weight().present()) {
                        weight = childNode.weight().get();
                    }

                    double weightedScore = computed_score * weight;
                    std::cout << indentChild << "    Child Weighted Score: " << weightedScore << std::endl;

                    childNode.computed_weighted_score(std::to_string(weightedScore));

                }
            }
        } else {
            std::cout << indent << "  No children element found in this node" << std::endl;
        }
        
        // Add evaluation logic here
    }

    inline OperationStatus Evaluator::evaluateKPMRRiskProfile()
    {
        for (auto &node : kpmrRiskProfile->node()) {
            std::cout << "traversing kpmr node" << std::endl;
            std::cout << "node profile id: " << node.profile_id() << std::endl;

            processKPMRRiskNode(node, 0);
        }
        return SuccessOperationStatus;
    }
}

#endif // ILFX_RISKPROFILE_HPP
