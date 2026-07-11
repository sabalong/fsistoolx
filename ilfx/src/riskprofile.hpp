#ifndef ILFX_RISKPROFILE_HPP
#define ILFX_RISKPROFILE_HPP

#include "allheaders.hpp"
#include "InherentDataSource.hxx"
#include "KPMRDataSource.hxx"
#include "InherentRiskProfile.hxx"
#include "KPMRRiskProfile.hxx"
#include <chaiscript/chaiscript.hpp>
#include <chaiscript/extras/math.hpp>
#include "chaiscript_diagnostics.hpp"
#include <antlr4-runtime.h>
#include "ThresholdLexer.h"
#include "ThresholdParser.h"
#include "EvalVisitor.hpp"
#include "scoreratingparser.hpp"
#include <map>
#include <sstream>
#include <unordered_map>
#include <iomanip>
#include <chrono>
#include <unordered_set>

// Forward declaration
bool evalExprWithX(double x, const std::string& expr);
bool evalExprWithVariables(const std::unordered_map<std::string, double>& variables, const std::string& expr);

// Helper function to format double with 10 decimal places
inline std::string formatDouble(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(10) << value;
    return oss.str();
}

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
    public:
        struct Telemetry {
            std::uint64_t rule_compile_count = 0;
            std::uint64_t rule_executions = 0;
            std::uint64_t helper_cache_hits = 0;
            std::uint64_t helper_cache_misses = 0;
            std::uint64_t recursive_evaluations = 0;
            std::uint64_t maximum_recursion_depth = 0;
            std::uint64_t cycles = 0;
            std::uint64_t evaluation_duration_ms = 0;
        };

    private:
        std::shared_ptr<inherent::datasource::DataType> inherentDataSources;
        std::shared_ptr<kpmr::datasource::ConsolidatedAssessmentType> datasources;
        std::shared_ptr<RiskProfileTree> inherentRiskProfile;
        std::shared_ptr<kpmr::riskprofile::kpmr_risk_profile_tree> kpmrRiskProfile;
        std::unique_ptr<chaiscript::ChaiScript> chai_;
        std::unordered_map<std::string, RiskProfileNodeType*> inherent_nodes_;
        std::unordered_map<std::string, kpmr::riskprofile::NodeType*> kpmr_nodes_;
        std::unordered_map<std::string, double> inherent_consolidates_;
        std::unordered_map<std::string, double> kpmr_consolidates_;
        std::unordered_map<std::string, std::string> compiled_rules_;
        std::unordered_map<std::string, double> double_cache_;
        std::unordered_map<std::string, int> int_cache_;
        std::unordered_map<std::string, std::unordered_map<int, double>> score_maps_;
        std::unordered_map<std::string, std::vector<std::pair<int, std::string>>> threshold_rules_;
        std::unordered_set<std::string> computing_;
        std::vector<std::string> dependency_stack_;
        Telemetry telemetry_;
        
        // Helper function to set up ChaiScript evaluator with all necessary bindings
        void setupChaiScriptEvaluator(chaiscript::ChaiScript& chai);
        void buildIndexes();
        std::string compileRule(const std::string& profile, const std::string& kind,
                                const std::string& script);
        template <typename Result>
        Result runRule(const std::string& profile, const std::string& kind,
                       const std::string& script, const std::string& threshold = "",
                       const std::string& ratingToScore = "", double value = 0.0,
                       int rating = 0);
        template <typename T, typename F>
        T memoized(const std::string& key, std::unordered_map<std::string, T>& cache, F&& compute);

    public:
        Evaluator(std::shared_ptr<inherent::datasource::DataType> inherentDataSources,
                  std::shared_ptr<kpmr::datasource::ConsolidatedAssessmentType> kpmrDataSources, std::shared_ptr<RiskProfileTree> inherentRiskProfile, std::shared_ptr<kpmr::riskprofile::kpmr_risk_profile_tree> kpmrRiskProfile)
            : inherentDataSources(inherentDataSources), datasources(kpmrDataSources), inherentRiskProfile(inherentRiskProfile), kpmrRiskProfile(kpmrRiskProfile),
              chai_(std::make_unique<chaiscript::ChaiScript>()) {
                setupChaiScriptEvaluator(*chai_);
                buildIndexes();
              };
        ~Evaluator() = default;

        OperationStatus evaluate();
        OperationStatus evaluateInherentRiskProfile();
        OperationStatus evaluateKPMRRiskProfile();
        const Telemetry& telemetry() const { return telemetry_; }

        double findConsolidateByCode(const std::string& code) {
            const auto it = inherent_consolidates_.find(code);
            if (it != inherent_consolidates_.end()) { ++telemetry_.helper_cache_hits; return it->second; }
            ++telemetry_.helper_cache_misses;
            return 0.0;
        };

        double computedScoreByCode(const std::string& code) {
            const auto indexed = inherent_nodes_.find(code);
            if (indexed != inherent_nodes_.end()) {
                auto* node = indexed->second;
                if (node->computed_score().present()) { ++telemetry_.helper_cache_hits; return std::strtod(node->computed_score().get().c_str(), nullptr); }
                return memoized<double>("inherent:score:" + code, double_cache_, [&, node] {
                    if (!node->score_rule().present() || node->score_rule().get().empty()) return 0.0;
                    const double result = runRule<double>(code, "score", node->score_rule().get(),
                        node->threshold().present() ? node->threshold().get() : "",
                        node->score_formula().present() ? node->score_formula().get() : "",
                        computedValueByCode(code), computedRatingByCode(code));
                    node->computed_score(std::to_string(result));
                    return result;
                });
            }
            ++telemetry_.helper_cache_misses;
            return 0.0;
#if 0
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
#endif
        };

        double computedValueByCode(const std::string& code) {
            const auto indexed = inherent_nodes_.find(code);
            if (indexed != inherent_nodes_.end()) {
                auto* node = indexed->second;
                if (node->computed_value().present()) { ++telemetry_.helper_cache_hits; return std::strtod(node->computed_value().get().c_str(), nullptr); }
                return memoized<double>("inherent:value:" + code, double_cache_, [&, node] {
                    if (!node->value_rule().present() || node->value_rule().get().empty()) return 0.0;
                    const double result = runRule<double>(code, "value", node->value_rule().get(),
                        node->threshold().present() ? node->threshold().get() : "",
                        node->score_formula().present() ? node->score_formula().get() : "");
                    node->computed_value(formatDouble(result));
                    return result;
                });
            }
            ++telemetry_.helper_cache_misses;
            return 0.0;
#if 0
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
#endif
        };

        double weightByCode(const std::string& code) {
            const auto indexed = inherent_nodes_.find(code);
            if (indexed != inherent_nodes_.end()) {
                ++telemetry_.helper_cache_hits;
                return indexed->second->weight().present() ? static_cast<double>(indexed->second->weight().get()) : 1.0;
            }
            ++telemetry_.helper_cache_misses;
            return 0.0;
#if 0
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
#endif
        };

        double weightByCodeKPMR(const std::string& code) {
            const auto indexed = kpmr_nodes_.find(code);
            if (indexed != kpmr_nodes_.end()) {
                ++telemetry_.helper_cache_hits;
                return indexed->second->weight().present() ? indexed->second->weight().get() : 1.0;
            }
            ++telemetry_.helper_cache_misses;
            return 0.0;
#if 0
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
#endif
        };

        double computedWeightedScoreByCode(const std::string& code) {
            const auto indexed = inherent_nodes_.find(code);
            if (indexed == inherent_nodes_.end()) { ++telemetry_.helper_cache_misses; return 0.0; }
            auto* node = indexed->second;
            if (node->computed_weighted_score().present()) {
                ++telemetry_.helper_cache_hits;
                return std::strtod(node->computed_weighted_score().get().c_str(), nullptr);
            }
            return memoized<double>("inherent:weighted:" + code, double_cache_, [&, node] {
                const double weighted = computedScoreByCode(code) *
                    (node->weight().present() ? static_cast<double>(node->weight().get()) : 1.0);
                node->computed_weighted_score(std::to_string(weighted));
                return weighted;
            });
#if 0
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
                        auto score = ilfx::chaiscript_diagnostics::evaluate<double>(
                            chai,
                            scoreRule,
                            "score",
                            "inherent profile code=" + code,
                            {{"code", code}});

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
#endif
        };

        double computedWeightedScoreByCodeKPMR(const std::string& code) {
            const auto indexed = kpmr_nodes_.find(code);
            if (indexed == kpmr_nodes_.end()) { ++telemetry_.helper_cache_misses; return 0.0; }
            auto* node = indexed->second;
            if (node->computed_weighted_score().present()) {
                ++telemetry_.helper_cache_hits;
                return std::strtod(node->computed_weighted_score().get().c_str(), nullptr);
            }
            return memoized<double>("kpmr:weighted:" + code, double_cache_, [&, node] {
                if (!node->score_rule().present() || node->score_rule().get().empty()) return 0.0;
                const double score = runRule<double>(code, "kpmr_score", node->score_rule().get(),
                    node->threshold(), node->score_formula().present() ? node->score_formula().get() : "", 0.0,
                    node->computed_rating().present() ? std::stoi(node->computed_rating().get()) : 0);
                return score * (node->weight().present() ? node->weight().get() : 1.0);
            });
#if 0
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

                        auto score = ilfx::chaiscript_diagnostics::evaluate<double>(
                            chai,
                            scoreRule,
                            "score",
                            "kpmr profile code=" + code,
                            {{"code", code}});

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
#endif
        };

        int computedRatingByCode(const std::string& code) {
            const auto indexed = inherent_nodes_.find(code);
            if (indexed != inherent_nodes_.end()) {
                auto* node = indexed->second;
                if (node->computed_rating().present()) { ++telemetry_.helper_cache_hits; return std::stoi(node->computed_rating().get()); }
                return memoized<int>("inherent:rating:" + code, int_cache_, [&, node] {
                    if (!node->rating_rule().present() || node->rating_rule().get().empty()) return 0;
                    const int result = runRule<int>(code, "rating", node->rating_rule().get(),
                        node->threshold().present() ? node->threshold().get() : "",
                        node->score_formula().present() ? node->score_formula().get() : "", computedValueByCode(code));
                    node->computed_rating(std::to_string(result));
                    return result;
                });
            }
            ++telemetry_.helper_cache_misses;
            return 0;
#if 0
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
#endif
        };

        double findConsolidateKPMRByCode(const std::string& code) {
            const auto it = kpmr_consolidates_.find(code);
            if (it != kpmr_consolidates_.end()) { ++telemetry_.helper_cache_hits; return it->second; }
            ++telemetry_.helper_cache_misses;
            return 0.0;
#if 0
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
#endif
        };
        
        std::string thresholdByCode(const std::string& code) {
            const auto it = inherent_nodes_.find(code);
            if (it != inherent_nodes_.end()) { ++telemetry_.helper_cache_hits; return it->second->threshold().present() ? it->second->threshold().get() : ""; }
            ++telemetry_.helper_cache_misses;
            return "";
#if 0
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
#endif
        };
        
        std::string thresholdByCodeKPMR(const std::string& code) {
            const auto it = kpmr_nodes_.find(code);
            if (it != kpmr_nodes_.end()) { ++telemetry_.helper_cache_hits; return it->second->threshold(); }
            ++telemetry_.helper_cache_misses;
            return "";
#if 0
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
#endif
        };
        
        std::string scoreFormulaByCode(const std::string& code) {
            const auto it = inherent_nodes_.find(code);
            if (it != inherent_nodes_.end()) { ++telemetry_.helper_cache_hits; return it->second->rating_to_score().present() ? it->second->rating_to_score().get() : ""; }
            ++telemetry_.helper_cache_misses;
            return "";
#if 0
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
#endif
        };
        
        std::string scoreFormulaByCodeKPMR(const std::string& code) {
            const auto it = kpmr_nodes_.find(code);
            if (it != kpmr_nodes_.end()) { ++telemetry_.helper_cache_hits; return it->second->score_formula().present() ? it->second->score_formula().get() : ""; }
            ++telemetry_.helper_cache_misses;
            return "";
#if 0
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
#endif
        };
        
        int ratingByThreshold(const std::string& threshold, double value) {
            return ratingByThreshold(threshold, std::unordered_map<std::string, double>{{"x", value}});
        };

        int ratingByThresholdVars(const std::string& threshold, const std::map<std::string, chaiscript::Boxed_Value>& boxedVariables) {
            return ratingByThreshold(threshold, thresholdVariablesFromChaiMap(boxedVariables));
        };

        int ratingByThreshold(const std::string& threshold, const std::unordered_map<std::string, double>& variables) {
            auto cached = threshold_rules_.find(threshold);
            if (cached == threshold_rules_.end()) {
                std::vector<std::pair<int, std::string>> parsed;
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
                    
                    parsed.emplace_back(std::stoi(ratingStr), std::move(compiledExpr));
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing threshold line: '" << line << "' - " << e.what() << std::endl;
                    continue;
                }
                }
                cached = threshold_rules_.emplace(threshold, std::move(parsed)).first;
            }
            for (const auto& rule : cached->second) {
                if (evalExprWithVariables(variables, rule.second)) return rule.first;
            }
            return -1;
        };

        double ratingToScore(const std::string& ratingToScoreStr, int rating) {
            auto cached = score_maps_.find(ratingToScoreStr);
            if (cached == score_maps_.end()) {
                std::unordered_map<int, double> parsed;
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
                    
                    parsed[lineRating] = score;
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing rating-to-score line: " << line << " - " << e.what() << std::endl;
                    continue;
                }
                }
                cached = score_maps_.emplace(ratingToScoreStr, std::move(parsed)).first;
            }
            const auto found = cached->second.find(rating);
            return found == cached->second.end() ? 0.0 : found->second;
        };
        
    private:
        void processInherentRiskNode(RiskProfileNodeType& node, int depth = 0);
        void processKPMRRiskNode(kpmr::riskprofile::NodeType& node, int depth = 0);
    };

    inline void Evaluator::buildIndexes()
    {
        if (inherentDataSources) {
            for (const auto& item : inherentDataSources->list().item())
                inherent_consolidates_[item.code()] = item.consolidate();
        }
        std::function<void(RiskProfileNodeType&)> addInherent = [&](RiskProfileNodeType& node) {
            inherent_nodes_[node.Profile_ID()] = &node;
            for (auto& child : node.RiskProfileNode()) addInherent(child);
        };
        if (inherentRiskProfile)
            for (auto& node : inherentRiskProfile->RiskProfileNode()) addInherent(node);

        std::function<void(kpmr::riskprofile::NodeType&)> addKpmr = [&](kpmr::riskprofile::NodeType& node) {
            kpmr_nodes_[node.profile_id()] = &node;
            if (node.children().present())
                for (auto& child : node.children()->node()) addKpmr(child);
        };
        if (kpmrRiskProfile)
            for (auto& node : kpmrRiskProfile->node()) addKpmr(node);

        std::function<void(const kpmr::datasource::RiskItemType&)> addItem =
            [&](const kpmr::datasource::RiskItemType& item) {
                kpmr_consolidates_[item.code()] = item.consolidate().present()
                    ? static_cast<double>(item.consolidate().get()) : 0.0;
                if (item.children().present())
                    for (const auto& child : item.children()->item()) addItem(child);
            };
        if (datasources) {
            for (const auto& group : datasources->list()) {
                kpmr_consolidates_[group.code()] = static_cast<double>(group.value());
                if (group.children().present())
                    for (const auto& item : group.children()->item()) addItem(item);
            }
        }
    }

    inline std::string Evaluator::compileRule(const std::string& profile, const std::string& kind,
                                               const std::string& script)
    {
        const std::string key = profile + "\x1f" + kind + "\x1f" + script;
        const auto found = compiled_rules_.find(key);
        if (found != compiled_rules_.end()) return found->second;
        const std::string name = "__ilfx_rule_" + std::to_string(compiled_rules_.size());
        const std::string wrapped = "def " + name + "(threshold, rating_to_score, value, rating) {\n" + script + "\n}";
        try {
            chai_->eval(wrapped);
        } catch (const chaiscript::exception::eval_error& error) {
            throw ilfx::chaiscript_diagnostics::EvaluationError(kind, "profile=" + profile, script,
                {{"profile_id", profile}}, "chaiscript::exception::eval_error", error.what(), error.pretty_print());
        }
        ++telemetry_.rule_compile_count;
        compiled_rules_.emplace(key, name);
        return name;
    }

    template <typename Result>
    inline Result Evaluator::runRule(const std::string& profile, const std::string& kind,
                                     const std::string& script, const std::string& threshold,
                                     const std::string& ratingToScore, double value, int rating)
    {
        const std::string name = compileRule(profile, kind, script);
        ++telemetry_.rule_executions;
        try {
            auto function = chai_->eval<std::function<Result(const std::string&, const std::string&, double, int)>>(name);
            return function(threshold, ratingToScore, value, rating);
        } catch (const chaiscript::exception::eval_error& error) {
            throw ilfx::chaiscript_diagnostics::EvaluationError(kind, "profile=" + profile, script,
                {{"profile_id", profile}, {"threshold", threshold}, {"rating_to_score", ratingToScore},
                 {"value", ilfx::chaiscript_diagnostics::value(value)}, {"rating", std::to_string(rating)}},
                "chaiscript::exception::eval_error", error.what(), error.pretty_print());
        } catch (const std::exception& error) {
            throw ilfx::chaiscript_diagnostics::EvaluationError(kind, "profile=" + profile, script,
                {{"profile_id", profile}}, typeid(error).name(), error.what(), "");
        }
    }

    template <typename T, typename F>
    inline T Evaluator::memoized(const std::string& key, std::unordered_map<std::string, T>& cache, F&& compute)
    {
        const auto found = cache.find(key);
        if (found != cache.end()) { ++telemetry_.helper_cache_hits; return found->second; }
        ++telemetry_.helper_cache_misses;
        if (computing_.count(key)) {
            ++telemetry_.cycles;
            std::ostringstream chain;
            for (const auto& entry : dependency_stack_) chain << entry << " -> ";
            chain << key;
            throw std::runtime_error("dependency cycle: " + chain.str());
        }
        computing_.insert(key);
        dependency_stack_.push_back(key);
        ++telemetry_.recursive_evaluations;
        telemetry_.maximum_recursion_depth = std::max<std::uint64_t>(telemetry_.maximum_recursion_depth, dependency_stack_.size());
        try {
            T result = compute();
            cache.emplace(key, result);
            dependency_stack_.pop_back();
            computing_.erase(key);
            return result;
        } catch (...) {
            dependency_stack_.pop_back();
            computing_.erase(key);
            throw;
        }
    }

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

            auto result = runRule<double>(node.Profile_ID(), "value", node.value_rule().get(),
                node.threshold().present() ? node.threshold().get() : "",
                node.score_formula().present() ? node.score_formula().get() : "");

            std::cout << indent << "    Evaluated Value Rule Result: " << result << std::endl;
            
            node.computed_value(formatDouble(result));
        }

        if (node.rating_rule().present() && !node.rating_rule().get().empty()) {
            std::cout << indent << "  Rating Rule: " << node.rating_rule().get() << std::endl;

            auto result = runRule<int>(node.Profile_ID(), "rating", node.rating_rule().get(),
                node.threshold().present() ? node.threshold().get() : "",
                node.score_formula().present() ? node.score_formula().get() : "",
                node.computed_value().present() ? std::strtod(node.computed_value().get().c_str(), nullptr) : 0.0);

            std::cout << indent << "    Evaluated Rating Rule Result: " << result << std::endl;

            node.computed_rating(std::to_string(result));
        }

        if (node.score_rule().present() && !node.score_rule().get().empty()) {
            std::cout << indent << "  Score Rule: " << node.score_rule().get() << std::endl;

            auto result = runRule<double>(node.Profile_ID(), "score", node.score_rule().get(),
                node.threshold().present() ? node.threshold().get() : "",
                node.score_formula().present() ? node.score_formula().get() : "",
                node.computed_value().present() ? std::strtod(node.computed_value().get().c_str(), nullptr) : 0.0,
                node.computed_rating().present() ? std::stoi(node.computed_rating().get()) : 0);

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
        const auto started = std::chrono::steady_clock::now();
        for (auto &node : inherentRiskProfile->RiskProfileNode()) {
            processInherentRiskNode(node, 0);
        }
        telemetry_.evaluation_duration_ms = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count());
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

                    auto computed_rating = runRule<int>(childNode.profile_id(), "kpmr_rating",
                        childNode.rating_rule().get(), childNode.threshold(),
                        childNode.score_formula().present() ? childNode.score_formula().get() : "");

                    std::cout << indentChild << "Computed Child Rating: " << computed_rating << std::endl;

                    childNode.computed_rating(std::to_string(computed_rating));
                }

                if (childNode.score_rule().present() && !childNode.score_rule().get().empty()) {
                    std::string indentChild((depth + 1) * 2, ' ');
                    std::cout << indentChild << "  Child Score Rule: " << childNode.score_rule().get() << std::endl;

                    auto computed_score = runRule<double>(childNode.profile_id(), "kpmr_score",
                        childNode.score_rule().get(), childNode.threshold(),
                        childNode.score_formula().present() ? childNode.score_formula().get() : "", 0.0,
                        childNode.computed_rating().present() ? std::stoi(childNode.computed_rating().get()) : 0);

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
        const auto started = std::chrono::steady_clock::now();
        for (auto &node : kpmrRiskProfile->node()) {
            std::cout << "traversing kpmr node" << std::endl;
            std::cout << "node profile id: " << node.profile_id() << std::endl;

            processKPMRRiskNode(node, 0);
        }
        telemetry_.evaluation_duration_ms = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count());
        return SuccessOperationStatus;
    }
}

#endif // ILFX_RISKPROFILE_HPP
