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
#include "absl/log/log.h"
#include "exprtk/exprtk.hpp"
#include <map>
#include <sstream>
#include <unordered_map>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <memory>
#include <set>
#include <vector>

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
    private:
        std::shared_ptr<inherent::datasource::DataType> inherentDataSources;
        std::shared_ptr<kpmr::datasource::ConsolidatedAssessmentType> datasources;
        std::shared_ptr<RiskProfileTree> inherentRiskProfile;
        std::shared_ptr<kpmr::riskprofile::kpmr_risk_profile_tree> kpmrRiskProfile;

        struct ThresholdCompiledRule {
            int rating = -1;
            std::string expressionText;
            std::unordered_map<std::string, double> boundVariables;
            exprtk::symbol_table<double> symbolTable;
            exprtk::expression<double> expression;
            bool compiled = false;

            ThresholdCompiledRule(int ruleRating,
                                  const std::string& compiledExpression,
                                  const std::vector<std::string>& variableNames)
                : rating(ruleRating), expressionText(compiledExpression) {
                for (const auto& name : variableNames) {
                    boundVariables.emplace(name, 0.0);
                }
                for (auto& variable : boundVariables) {
                    symbolTable.add_variable(variable.first, variable.second);
                }
                symbolTable.add_constants();
                expression.register_symbol_table(symbolTable);

                exprtk::parser<double> parser;
                compiled = parser.compile(expressionText, expression);
                if (!compiled) {
                    DLOG(INFO) << "Error compiling threshold expression: " << expressionText
                               << " error: " << parser.error();
                }
            }

            bool evaluate(const std::unordered_map<std::string, double>& variables) {
                for (auto& variable : boundVariables) {
                    auto it = variables.find(variable.first);
                    variable.second = (it != variables.end()) ? it->second : 0.0;
                }
                return compiled && expression.value();
            }
        };

        struct ThresholdProgram {
            std::vector<std::unique_ptr<ThresholdCompiledRule>> rules;
        };

        mutable std::unordered_map<std::string, RiskProfileNodeType*> inherentProfileByCode;
        mutable std::unordered_map<std::string, kpmr::riskprofile::NodeType*> kpmrProfileByCode;
        mutable std::unordered_map<std::string, inherent::datasource::ItemType*> inherentDataSourceByCode;
        mutable std::unordered_map<std::string, kpmr::datasource::RiskGroupType*> kpmrGroupByCode;
        mutable std::unordered_map<std::string, kpmr::datasource::RiskItemType*> kpmrItemByCode;
        mutable std::unordered_map<std::string, double> inherentConsolidateCache;
        mutable std::unordered_map<std::string, double> kpmrConsolidateCache;
        mutable std::unordered_map<std::string, std::unique_ptr<ThresholdProgram>> thresholdProgramCache;
        mutable std::unordered_map<std::string, std::unordered_map<int, double>> ratingToScoreCache;
        mutable bool indexesBuilt = false;
        
        // Helper function to set up ChaiScript evaluator with all necessary bindings
        void setupChaiScriptEvaluator(chaiscript::ChaiScript& chai);

        static std::string trim(std::string value);
        static std::string thresholdProgramCacheKey(
            const std::string& threshold,
            const std::unordered_map<std::string, double>& variables);
        void resetEvaluationCaches();
        void ensureIndexes();
        void indexInherentNode(RiskProfileNodeType& node);
        void indexKPMRNode(kpmr::riskprofile::NodeType& node);
        void indexKPMRItem(kpmr::datasource::RiskItemType& item);
        ThresholdProgram& thresholdProgramFor(
            const std::string& threshold,
            const std::unordered_map<std::string, double>& variables);
        const std::unordered_map<int, double>& ratingToScoreMap(const std::string& ratingToScoreStr);

    public:
        Evaluator(std::shared_ptr<inherent::datasource::DataType> inherentDataSources,
                  std::shared_ptr<kpmr::datasource::ConsolidatedAssessmentType> kpmrDataSources, std::shared_ptr<RiskProfileTree> inherentRiskProfile, std::shared_ptr<kpmr::riskprofile::kpmr_risk_profile_tree> kpmrRiskProfile)
            : inherentDataSources(inherentDataSources), datasources(kpmrDataSources), inherentRiskProfile(inherentRiskProfile), kpmrRiskProfile(kpmrRiskProfile) {};
        ~Evaluator() = default;

        OperationStatus evaluate();
        OperationStatus evaluateInherentRiskProfile();
        OperationStatus evaluateKPMRRiskProfile();

        double findConsolidateByCode(const std::string& code) {
            ensureIndexes();
            auto cacheIt = inherentConsolidateCache.find(code);
            if (cacheIt != inherentConsolidateCache.end()) {
                return cacheIt->second;
            }

            auto itemIt = inherentDataSourceByCode.find(code);
            if (itemIt == inherentDataSourceByCode.end()) {
                return 0.0;
            }

            double value = itemIt->second->consolidate();
            inherentConsolidateCache[code] = value;
            return value;
        };

        double computedScoreByCode(const std::string& code) {
            ensureIndexes();
            auto it = inherentProfileByCode.find(code);
            if (it != inherentProfileByCode.end() && it->second->computed_score().present()) {
                return std::strtod(it->second->computed_score().get().c_str(), nullptr);
            }
            return 0.0;
        };

        double computedValueByCode(const std::string& code) {
            ensureIndexes();
            auto it = inherentProfileByCode.find(code);
            if (it != inherentProfileByCode.end() && it->second->computed_value().present()) {
                return std::strtod(it->second->computed_value().get().c_str(), nullptr);
            }
            return 0.0;
        };

        double weightByCode(const std::string& code) {
            ensureIndexes();
            auto it = inherentProfileByCode.find(code);
            if (it == inherentProfileByCode.end()) {
                return 0.0;
            }
            return it->second->weight().present() ? static_cast<double>(it->second->weight().get()) : 1.0;
        };

        double weightByCodeKPMR(const std::string& code) {
            ensureIndexes();
            auto it = kpmrProfileByCode.find(code);
            if (it == kpmrProfileByCode.end()) {
                return 0.0;
            }
            return it->second->weight().present() ? static_cast<double>(it->second->weight().get()) : 1.0;
        };

        double computedWeightedScoreByCode(const std::string& code) {
            ensureIndexes();
            auto it = inherentProfileByCode.find(code);
            if (it == inherentProfileByCode.end()) {
                return 0.0;
            }

            RiskProfileNodeType& node = *it->second;
            if (node.computed_weighted_score().present()) {
                return std::strtod(node.computed_weighted_score().get().c_str(), nullptr);
            }
            if (node.computed_score().present()) {
                return std::strtod(node.computed_score().get().c_str(), nullptr) * weightByCode(code);
            }
            return 0.0;
        };

        double computedWeightedScoreByCodeKPMR(const std::string& code) {
            ensureIndexes();
            auto it = kpmrProfileByCode.find(code);
            if (it == kpmrProfileByCode.end()) {
                return 0.0;
            }

            auto& node = *it->second;
            if (node.computed_weighted_score().present()) {
                return std::strtod(node.computed_weighted_score().get().c_str(), nullptr);
            }
            if (node.computed_score().present()) {
                return std::strtod(node.computed_score().get().c_str(), nullptr) * weightByCodeKPMR(code);
            }
            return 0.0;
        };

        int computedRatingByCode(const std::string& code) {
            ensureIndexes();
            auto it = inherentProfileByCode.find(code);
            if (it != inherentProfileByCode.end() && it->second->computed_rating().present()) {
                return std::stoi(it->second->computed_rating().get());
            }
            return 0;
        };

        double findConsolidateKPMRByCode(const std::string& code) {
            ensureIndexes();
            auto cacheIt = kpmrConsolidateCache.find(code);
            if (cacheIt != kpmrConsolidateCache.end()) {
                return cacheIt->second;
            }

            auto itemIt = kpmrItemByCode.find(code);
            if (itemIt != kpmrItemByCode.end()) {
                double value = itemIt->second->consolidate().present()
                    ? static_cast<double>(itemIt->second->consolidate().get())
                    : 0.0;
                kpmrConsolidateCache[code] = value;
                return value;
            }

            auto groupIt = kpmrGroupByCode.find(code);
            if (groupIt != kpmrGroupByCode.end()) {
                double value = static_cast<double>(groupIt->second->value());
                kpmrConsolidateCache[code] = value;
                return value;
            }

            return 0.0;
        };
        
        std::string thresholdByCode(const std::string& code) {
            ensureIndexes();
            auto it = inherentProfileByCode.find(code);
            if (it != inherentProfileByCode.end() && it->second->threshold().present()) {
                return it->second->threshold().get();
            }
            return "";
        };
        
        std::string thresholdByCodeKPMR(const std::string& code) {
            ensureIndexes();
            auto it = kpmrProfileByCode.find(code);
            if (it != kpmrProfileByCode.end()) {
                return it->second->threshold();
            }
            return "";
        };
        
        std::string scoreFormulaByCode(const std::string& code) {
            ensureIndexes();
            auto it = inherentProfileByCode.find(code);
            if (it != inherentProfileByCode.end() && it->second->rating_to_score().present()) {
                return it->second->rating_to_score().get();
            }
            return "";
        };
        
        std::string scoreFormulaByCodeKPMR(const std::string& code) {
            ensureIndexes();
            auto it = kpmrProfileByCode.find(code);
            if (it != kpmrProfileByCode.end() && it->second->score_formula().present()) {
                return it->second->score_formula().get();
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
            ThresholdProgram& program = thresholdProgramFor(threshold, variables);
            for (const auto& rule : program.rules) {
                if (rule->evaluate(variables)) {
                    DLOG(INFO) << "Threshold rating matched: " << rule->rating;
                    return rule->rating;
                }
            }

            return -1;
        };

        double ratingToScore(const std::string& ratingToScoreStr, int rating) {
            const auto& scoreMap = ratingToScoreMap(ratingToScoreStr);
            auto it = scoreMap.find(rating);
            if (it != scoreMap.end()) {
                return it->second;
            }
            DLOG(INFO) << "No matching score found for rating: " << rating;
            return 0.0;
        };
        
    private:
        void processInherentRiskNode(RiskProfileNodeType& node, int depth = 0);
        void processKPMRRiskNode(kpmr::riskprofile::NodeType& node, int depth = 0);
    };

    inline std::string Evaluator::trim(std::string value)
    {
        auto notSpace = [](unsigned char c) { return !std::isspace(c); };
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
        value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
        return value;
    }

    inline std::string Evaluator::thresholdProgramCacheKey(
        const std::string& threshold,
        const std::unordered_map<std::string, double>& variables)
    {
        std::vector<std::string> names;
        names.reserve(variables.size());
        for (const auto& variable : variables) {
            names.push_back(variable.first);
        }
        std::sort(names.begin(), names.end());

        std::ostringstream key;
        key << threshold << '\0';
        for (const auto& name : names) {
            key << name << '\0';
        }
        return key.str();
    }

    inline void Evaluator::resetEvaluationCaches()
    {
        inherentProfileByCode.clear();
        kpmrProfileByCode.clear();
        inherentDataSourceByCode.clear();
        kpmrGroupByCode.clear();
        kpmrItemByCode.clear();
        inherentConsolidateCache.clear();
        kpmrConsolidateCache.clear();
        thresholdProgramCache.clear();
        ratingToScoreCache.clear();
        indexesBuilt = false;
    }

    inline void Evaluator::ensureIndexes()
    {
        if (indexesBuilt) {
            return;
        }

        inherentProfileByCode.clear();
        kpmrProfileByCode.clear();
        inherentDataSourceByCode.clear();
        kpmrGroupByCode.clear();
        kpmrItemByCode.clear();

        if (inherentRiskProfile) {
            for (auto& node : inherentRiskProfile->RiskProfileNode()) {
                indexInherentNode(node);
            }
        }

        if (kpmrRiskProfile) {
            for (auto& node : kpmrRiskProfile->node()) {
                indexKPMRNode(node);
            }
        }

        if (inherentDataSources) {
            for (auto& item : inherentDataSources->list().item()) {
                inherentDataSourceByCode[item.code()] = &item;
            }
        }

        if (datasources) {
            for (auto& group : datasources->list()) {
                kpmrGroupByCode[group.code()] = &group;
                if (group.children().present()) {
                    for (auto& item : group.children()->item()) {
                        indexKPMRItem(item);
                    }
                }
            }
        }

        indexesBuilt = true;
    }

    inline void Evaluator::indexInherentNode(RiskProfileNodeType& node)
    {
        inherentProfileByCode[node.Profile_ID()] = &node;
        for (auto& child : node.RiskProfileNode()) {
            indexInherentNode(child);
        }
    }

    inline void Evaluator::indexKPMRNode(kpmr::riskprofile::NodeType& node)
    {
        kpmrProfileByCode[node.profile_id()] = &node;
        if (node.children().present()) {
            for (auto& child : node.children()->node()) {
                indexKPMRNode(child);
            }
        }
    }

    inline void Evaluator::indexKPMRItem(kpmr::datasource::RiskItemType& item)
    {
        kpmrItemByCode[item.code()] = &item;
        if (item.children().present()) {
            for (auto& child : item.children()->item()) {
                indexKPMRItem(child);
            }
        }
    }

    inline Evaluator::ThresholdProgram& Evaluator::thresholdProgramFor(
        const std::string& threshold,
        const std::unordered_map<std::string, double>& variables)
    {
        const std::string key = thresholdProgramCacheKey(threshold, variables);
        auto cached = thresholdProgramCache.find(key);
        if (cached != thresholdProgramCache.end()) {
            return *cached->second;
        }

        std::vector<std::string> variableNames;
        variableNames.reserve(variables.size());
        for (const auto& variable : variables) {
            variableNames.push_back(variable.first);
        }
        std::sort(variableNames.begin(), variableNames.end());

        auto program = std::make_unique<ThresholdProgram>();
        std::istringstream stream(threshold);
        std::string line;

        while (std::getline(stream, line)) {
            line = trim(line);
            if (line.empty()) {
                continue;
            }

            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) {
                continue;
            }

            try {
                const int rating = std::stoi(trim(line.substr(0, colonPos)));
                const std::string exprStr = trim(line.substr(colonPos + 1));
                if (exprStr.empty()) {
                    continue;
                }

                antlr4::ANTLRInputStream inputStream(exprStr);
                ThresholdLexer lexer(&inputStream);
                antlr4::CommonTokenStream tokens(&lexer);
                ThresholdParser parser(&tokens);
                ThresholdParser::ExprContext* tree = parser.expr();

                EvalVisitor visitor;
                std::string compiledExpr = std::any_cast<std::string>(visitor.visitExpr(tree));
                auto rule = std::make_unique<ThresholdCompiledRule>(rating, compiledExpr, variableNames);
                if (rule->compiled) {
                    program->rules.push_back(std::move(rule));
                }
            } catch (const std::exception& e) {
                DLOG(INFO) << "Error parsing threshold line: '" << line << "' - " << e.what();
                continue;
            }
        }

        auto inserted = thresholdProgramCache.emplace(key, std::move(program));
        return *inserted.first->second;
    }

    inline const std::unordered_map<int, double>& Evaluator::ratingToScoreMap(
        const std::string& ratingToScoreStr)
    {
        auto cached = ratingToScoreCache.find(ratingToScoreStr);
        if (cached != ratingToScoreCache.end()) {
            return cached->second;
        }

        std::string normalized = ratingToScoreStr;
        std::replace(normalized.begin(), normalized.end(), ',', '\n');

        std::unordered_map<int, double> scoreMap;
        std::istringstream stream(normalized);
        std::string line;
        while (std::getline(stream, line)) {
            line = trim(line);
            if (line.empty()) {
                continue;
            }

            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) {
                continue;
            }

            try {
                int lineRating = std::stoi(trim(line.substr(0, colonPos)));
                double score = std::stod(trim(line.substr(colonPos + 1)));
                scoreMap[lineRating] = score;
            } catch (const std::exception& e) {
                DLOG(INFO) << "Error parsing rating-to-score line: " << line << " - " << e.what();
                continue;
            }
        }

        auto inserted = ratingToScoreCache.emplace(ratingToScoreStr, std::move(scoreMap));
        return inserted.first->second;
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
        DLOG(INFO) << "Evaluating risk profile...";
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
        DLOG(INFO) << indent << "Processing Inherent Risk Profile Node: " << node.Profile_ID();
        
        // Only print risiko_name if it's present (it's optional)
        if (node.assessment_factor().present()) {
            DLOG(INFO) << indent << "  Assessment Name: " << node.assessment_factor().get();
        }

        chaiscript::ChaiScript chai;
        setupChaiScriptEvaluator(chai);

        if (node.threshold().present()) {
            chai.add(chaiscript::var(std::string(node.threshold().get())), "threshold");
        }
        if (node.score_formula().present()) {
            chai.add(chaiscript::var(std::string(node.score_formula().get())), "rating_to_score");
        }

        bool valueBound = false;
        bool ratingBound = false;

        // calculate the rules first
        if (node.value_rule().present() && !node.value_rule().get().empty()) {
            DLOG(INFO) << indent << "  Value Rule: " << node.value_rule().get();
            
            auto result = 0.0;

            try {
                result = chai.eval<double>(node.value_rule().get());

            }catch (const std::exception& e) {
                std::cerr << "Error evaluating value rule for node " << node.Profile_ID() << ": " << e.what() << std::endl;
                result = 0.0; // Default to 0 on error
            }

            DLOG(INFO) << indent << "    Evaluated Value Rule Result: " << result;
            
            node.computed_value(formatDouble(result));
        }

        if (node.rating_rule().present() && !node.rating_rule().get().empty()) {
            DLOG(INFO) << indent << "  Rating Rule: " << node.rating_rule().get();

            if (!valueBound && node.computed_value().present()) {
                chai.add(chaiscript::var(std::strtod(node.computed_value().get().c_str(), nullptr)), "value");
                valueBound = true;
            }

            auto result = -1;
            try {
                result = chai.eval<int>(node.rating_rule().get());
            } catch (const std::exception& e) {
                std::cerr << "Error evaluating rating rule for node " << node.Profile_ID() << ": " << e.what() << std::endl;
                result = -1; // Default to -1 on error
            }

            DLOG(INFO) << indent << "    Evaluated Rating Rule Result: " << result;

            node.computed_rating(std::to_string(result));
        }

        if (node.score_rule().present() && !node.score_rule().get().empty()) {
            DLOG(INFO) << indent << "  Score Rule: " << node.score_rule().get();

            if (!valueBound && node.computed_value().present()) {
                chai.add(chaiscript::var(std::strtod(node.computed_value().get().c_str(), nullptr)), "value");
                valueBound = true;
            }
            if (!ratingBound && node.computed_rating().present()) {
                chai.add(chaiscript::var(std::stoi(node.computed_rating().get())), "rating");
                ratingBound = true;
            }

            auto result = 0.0;
            try {
                result = chai.eval<double>(node.score_rule().get());
            } catch (const std::exception& e) {
                std::cerr << "Error evaluating score rule for node " << node.Profile_ID() << ": " << e.what() << std::endl;
                result = 0.0; // Default to 0 on error
            }

            DLOG(INFO) << indent << "    Evaluated Score Rule Result: " << result;

            node.computed_score(std::to_string(result));

            double weight = 1.0;

            if (node.weight().present()) {
                weight = node.weight().get();
            }

            double weightedScore = result * weight;
            DLOG(INFO) << indent << "    Weighted Score: " << weightedScore;

            node.computed_weighted_score(std::to_string(weightedScore));
        }

        
        
        // Add evaluation logic here
    }
    
    inline OperationStatus Evaluator::evaluateInherentRiskProfile()
    {
        resetEvaluationCaches();
        ensureIndexes();

        for (auto &node : inherentRiskProfile->RiskProfileNode()) {
            processInherentRiskNode(node, 0);
        }
        return SuccessOperationStatus;
    }
    
    inline void Evaluator::processKPMRRiskNode(kpmr::riskprofile::NodeType& node, int depth)
    {
        std::string indent(depth * 2, ' ');
        DLOG(INFO) << indent << "Processing KPMR Risk Profile Node: " << node.profile_id();
        DLOG(INFO) << indent << "  Risk Name: " << node.risiko_name();
        DLOG(INFO) << indent << "  Children present: " << (node.children().present() ? "YES" : "NO");
        
        // Recursively process children first
        if (node.children().present()) {
            DLOG(INFO) << indent << "  Entering children processing...";
            DLOG(INFO) << indent << "  Number of children: " << node.children()->node().size();
            
            for (auto& childNode : node.children()->node()) {
                processKPMRRiskNode(childNode, depth + 1);

                chaiscript::ChaiScript chai;
                setupChaiScriptEvaluator(chai);
                bool ratingBound = false;

                if (childNode.rating_rule().present() && !childNode.rating_rule().get().empty()) {
                    std::string indentChild((depth + 1) * 2, ' ');
                    DLOG(INFO) << indentChild << "  Child Rating Rule: " << childNode.rating_rule().get();

                    auto computed_rating = -1;
                    try {
                        computed_rating = chai.eval<int>(childNode.rating_rule().get());
                    } catch (const std::exception& e) {
                        std::cerr << "Error evaluating rating rule for child node " << childNode.profile_id() << ": " << e.what() << std::endl;
                        computed_rating = -1; // Default to -1 on error
                    }

                    DLOG(INFO) << indentChild << "Computed Child Rating: " << computed_rating;

                    childNode.computed_rating(std::to_string(computed_rating));
                }

                if (childNode.score_rule().present() && !childNode.score_rule().get().empty()) {
                    std::string indentChild((depth + 1) * 2, ' ');
                    DLOG(INFO) << indentChild << "  Child Score Rule: " << childNode.score_rule().get();

                    if (!ratingBound && childNode.computed_rating().present()) {
                        chai.add(chaiscript::var(std::stoi(childNode.computed_rating().get())), "rating");
                        ratingBound = true;
                    }

                    auto computed_score = 0.0;
                    try {
                        computed_score = chai.eval<double>(childNode.score_rule().get());
                    } catch (const std::exception& e) {
                        std::cerr << "Error evaluating score rule for child node " << childNode.profile_id() << ": " << e.what() << std::endl;
                        computed_score = 0.0; // Default to 0 on error
                    }

                    DLOG(INFO) << indentChild << "Computed Child Score: " << computed_score;

                    childNode.computed_score(std::to_string(computed_score));


                    double weight = 1.0;

                    if (childNode.weight().present()) {
                        weight = childNode.weight().get();
                    }

                    double weightedScore = computed_score * weight;
                    DLOG(INFO) << indentChild << "    Child Weighted Score: " << weightedScore;

                    childNode.computed_weighted_score(std::to_string(weightedScore));

                }
            }
        } else {
            DLOG(INFO) << indent << "  No children element found in this node";
        }
        
        // Add evaluation logic here
    }

    inline OperationStatus Evaluator::evaluateKPMRRiskProfile()
    {
        resetEvaluationCaches();
        ensureIndexes();

        for (auto &node : kpmrRiskProfile->node()) {
            DLOG(INFO) << "traversing kpmr node";
            DLOG(INFO) << "node profile id: " << node.profile_id();

            processKPMRRiskNode(node, 0);
        }
        return SuccessOperationStatus;
    }
}

#endif // ILFX_RISKPROFILE_HPP
