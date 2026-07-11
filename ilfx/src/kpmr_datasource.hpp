#ifndef ILFX_KPMR_DATASOURCE_HPP
#define ILFX_KPMR_DATASOURCE_HPP

#include "allheaders.hpp"
#include "KPMRDataSource.hxx"
#include "chaicli.hpp"
#include "chaiscript_diagnostics.hpp"
#include "chaiscript/extras/math.hpp"
#include "absl/log/log.h"
#include "Weight.hxx"
#include <unordered_set>

typedef std::vector<int> int_vector;
typedef std::map<std::string, int_vector> map_type;
typedef std::pair<const std::string, int_vector> map_pair;

namespace kpmr::datasource
{
    class Evaluator
    {
    private:
        /* data */
        std::shared_ptr<ConsolidatedAssessmentType> datasources;
        std::shared_ptr<CompanyWeights> companyWeights;

        // Store computed input values for companies
        // this will be populated when we are evaluating the FSI rules
        std::unordered_map<std::string, double> computedInputs;

        // Cache consolidated values by group/item id to avoid re-evaluation
        std::unordered_map<std::string, int> consolidatedCache;
        std::unordered_set<std::string> visiting;

        // Helper function to set up ChaiScript evaluator with all necessary bindings
        void setupChaiScriptEvaluator(chaiscript::ChaiScript& chai);

    public:
        Evaluator(const std::string &configPath, const std::string &weightPath);
        ~Evaluator();

        std::unordered_map<std::string, double> getComputedInputs() const
        {
            return computedInputs;
        };

        std::shared_ptr<ConsolidatedAssessmentType> getDataSources() const
        {
            return datasources;
        };

        OperationStatus evaluate();

        // Recursively evaluate a risk group: process children and apply consolidation rule
        int evaluateGroup(RiskGroupType &group);

        // Recursively evaluate a risk item: process children and apply consolidation rule
        int evaluateItem(RiskItemType &item);

        // Return a map of company names to their corresponding vector of values for a given code
        chaiscript::Boxed_Value companyValuesByCode(const std::string &code);

        chaiscript::Boxed_Value companyValuesByCodeAndCompanyName(const std::string &code, const std::string &companyName);

        // Update the computed input for a given company based on provided values
        bool updateCompanyInputByValues(const std::string &code, const std::string &companyName, double value);

        // weight lookup
        // this function looks up the weight for a given company from the CompanyWeights data
        double findWeightByCompany(const std::string &company);

        // weight lookup
        // this function looks up the weight for a given company type from the CompanyWeights data
        double findWeightByCompanyType(const std::string &companyType);

        // weight lookup
        // this function looks up the bank and financing weight for a given company from the CompanyWeights data
        double findBankAndFinancingWeightByCompany(const std::string &company);
    };

    Evaluator::Evaluator(const std::string &configPath, const std::string &weightPath)
    {
        LOG(INFO) << "Loading Inherent Data Source from: " << configPath;
        datasources = data(configPath);
        companyWeights = CompanyWeights_(weightPath);
        computedInputs = std::unordered_map<std::string, double>{};
    }

    Evaluator::~Evaluator()
    {
    }

    inline void Evaluator::setupChaiScriptEvaluator(chaiscript::ChaiScript& chai)
    {
        auto mathlib = chaiscript::extras::math::bootstrap();
        chai.add(mathlib);
        
        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->companyValuesByCode(code);
                     }),
                 "companyValuesByCode");

        chai.add(chaiscript::fun(
                     [this](const std::string &code, const std::string &companyName, double values)
                     {
                         return this->updateCompanyInputByValues(code, companyName, values);
                     }),
                 "updateCompanyInputByValues");

        chai.add(chaiscript::fun(
                     [this](const std::string &code, const std::string &companyName)
                     {
                         return this->companyValuesByCodeAndCompanyName(code, companyName);
                     }),
                 "companyValuesByCodeAndCompanyName");
        
        chai.add(chaiscript::fun(
                     [this](const std::string &company)
                     {
                         return this->findWeightByCompany(company);
                     }),
                 "findWeightByCompany");

        chai.add(chaiscript::fun(
                     [this](const std::string &companyType)
                     {
                         return this->findWeightByCompanyType(companyType);
                     }),
                 "findWeightByCompanyType");

        chai.add(chaiscript::fun(
                     [this](const std::string &company)
                     {
                         return this->findBankAndFinancingWeightByCompany(company);
                     }),
                 "findBankAndFinancingWeightByCompany");
    }

    inline OperationStatus Evaluator::evaluate()
    {
        // Clear caches for fresh evaluation
        consolidatedCache.clear();
        visiting.clear();

        // Recursively evaluate all top-level risk groups
        for (auto &group : datasources->list())
        {
            (void)evaluateGroup(group);
        }

        return SuccessOperationStatus;
    }

    inline int Evaluator::evaluateGroup(RiskGroupType &group)
    {
        std::string id = group.id();

        // Check cache
        // if (consolidatedCache.find(id) != consolidatedCache.end())
        // {
        //     return consolidatedCache[id];
        // }

        // Cycle detection
        if (visiting.find(id) != visiting.end())
        {
            return static_cast<int>(group.value());
        }
        visiting.insert(id);

        // Recursively evaluate children items
        std::vector<int> childrenValues;
        if (group.children().present())
        {
            for (auto &item : group.children().get().item())
            {
                childrenValues.push_back(evaluateItem(item));
            }
        }

        visiting.erase(id);
        int result = static_cast<int>(group.value());
        //consolidatedCache[id] = result;
        return result;
    }

    inline int Evaluator::evaluateItem(RiskItemType &item)
    {
        std::string id = item.id();

        // Check cache
        // if (consolidatedCache.find(id) != consolidatedCache.end())
        // {
        //     return consolidatedCache[id];
        // }

        // Cycle detection
        if (visiting.find(id) != visiting.end())
        {
            return static_cast<int>(item.value());
        }
        visiting.insert(id);

        // Recursively evaluate children items
        std::vector<int> childrenValues;
        if (item.children().present())
        {
            for (auto &childItem : item.children().get().item())
            {
                childrenValues.push_back(evaluateItem(childItem));
            }
        }

        // Note: RiskItemType doesn't have consolidationRule in the schema
        // If it's added later, uncomment this section:
        // if (item.consolidationRule().present()) { ... }

        if (item.consolidationRule().present())
        {
            std::string consolidationRule = item.consolidationRule().get();
            if (!consolidationRule.empty())
            {
                chaiscript::ChaiScript chai;
                setupChaiScriptEvaluator(chai);

                chai.add(chaiscript::var(childrenValues), "childrenValues");

                LOG(INFO) << "Evaluating Consolidation Rule for item " << item.code() << ": " << consolidationRule;
                chaiscript::Boxed_Value v = ilfx::chaiscript_diagnostics::evaluateBoxed(
                    chai,
                    consolidationRule,
                    "consolidation",
                    "kpmr item id=" + item.id() + ", code=" + item.code(),
                    {
                        {"id", item.id()},
                        {"code", item.code()},
                        {"childrenValues", ilfx::chaiscript_diagnostics::value(childrenValues)},
                    });
                int consolidatedValue = chai.boxed_cast<double>(v);
                item.consolidate(consolidatedValue);
                LOG(INFO) << "Consolidated value for item " << item.id() << ": " << consolidatedValue;
            }
        }

        visiting.erase(id);
        int result = static_cast<int>(item.value());
        //consolidatedCache[id] = result;
        return result;
    }

    // Helper function to recursively collect company values from an item and its children
    inline void collectCompanyValuesFromItem(const RiskItemType &item, const std::string &code, std::map<std::string, std::vector<double>> &result)
    {
        if (item.code() == code)
        {
            if (item.detail().present())
            {
                for (const auto &row : item.detail().get().detail())
                {
                    LOG(INFO) << "Found row for company " << row.companyName() << " with value " << row.rating();
                        result[row.companyName()].push_back(row.rating() * 1.0);
                    }
            }
        }

        // Recursively check children
        if (item.children().present())
        {
            for (const auto &childItem : item.children().get().item())
            {
                collectCompanyValuesFromItem(childItem, code, result);
            }
        }
    }

    // Helper function to recursively collect company values from a group and its children
    inline void collectCompanyValuesFromGroup(const RiskGroupType &group, const std::string &code, std::map<std::string, std::vector<double>> &result)
    {
        if (group.code() == code)
        {
            if (group.detail().present())
            {
                for (const auto &row : group.detail().get().detail())
                {
                    LOG(INFO) << "Found row for company " << row.companyName() << " with value " << row.rating();
                    result[row.companyName()].push_back(row.rating() * 1.0);
                }
            }
        }

        // Recursively check children items
        if (group.children().present())
        {
            for (const auto &item : group.children().get().item())
            {
                collectCompanyValuesFromItem(item, code, result);
            }
        }
    }

    // Return a map of company names to their corresponding vector of values for a given code
    inline chaiscript::Boxed_Value Evaluator::companyValuesByCode(const std::string &code)
    {
        std::map<std::string, std::vector<double>> raw;

        // Recursively traverse all groups and their children
        for (const auto &group : datasources->list())
        {
            collectCompanyValuesFromGroup(group, code, raw);
        }

        // Convert to chaiscript format
        std::map<std::string, chaiscript::Boxed_Value> converted;
        for (auto &[company, vec] : raw)
        {
            std::vector<chaiscript::Boxed_Value> boxed_vec;
            boxed_vec.reserve(vec.size());

            for (double v : vec)
                boxed_vec.push_back(chaiscript::var(v));

            converted[company] = chaiscript::var(boxed_vec);
        }

        return chaiscript::var(converted);
    }

    // Helper function to recursively search for company value from an item and its children
    inline chaiscript::Boxed_Value findCompanyValueFromItem(const RiskItemType &item, const std::string &code, const std::string &companyName, bool &found)
    {
        if (item.code() == code)
        {
            if (item.detail().present())
            {
                for (const auto &row : item.detail().get().detail())
                {
                    if (row.companyName() == companyName)
                    {
                        found = true;
                        return chaiscript::var(row.value());
                    }
                }
            }
        }

        // Recursively check children
        if (item.children().present())
        {
            for (const auto &childItem : item.children().get().item())
            {
                auto result = findCompanyValueFromItem(childItem, code, companyName, found);
                if (found)
                {
                    return result;
                }
            }
        }

        return chaiscript::Boxed_Value();
    }

    // Helper function to recursively search for company value from a group and its children
    inline chaiscript::Boxed_Value findCompanyValueFromGroup(const RiskGroupType &group, const std::string &code, const std::string &companyName, bool &found)
    {
        if (group.code() == code)
        {
            if (group.detail().present())
            {
                for (const auto &row : group.detail().get().detail())
                {
                    if (row.companyName() == companyName)
                    {
                        found = true;
                        return chaiscript::var(row.value());
                    }
                }
            }
        }

        // Recursively check children items
        if (group.children().present())
        {
            for (const auto &item : group.children().get().item())
            {
                auto result = findCompanyValueFromItem(item, code, companyName, found);
                if (found)
                {
                    return result;
                }
            }
        }

        return chaiscript::Boxed_Value();
    }

    inline chaiscript::Boxed_Value Evaluator::companyValuesByCodeAndCompanyName(const std::string &code, const std::string &companyName)
    {
        bool found = false;

        // Recursively traverse all groups and their children
        for (const auto &group : datasources->list())
        {
            auto result = findCompanyValueFromGroup(group, code, companyName, found);
            if (found)
            {
                return result;
            }
        }

        return chaiscript::Boxed_Value();
    }

    inline bool Evaluator::updateCompanyInputByValues(const std::string &code, const std::string &companyName, double value)
    {
        LOG(INFO) << "Updating computed input for company " << companyName << " with values for code " << code;

        this->computedInputs[companyName] = value;

        return true;
    }

     // this function looks up the weight for a given company from the CompanyWeights data
    // it iterates through the CompanyWeights data to find the matching company and returns its weight
    // if the company is not found or does not have a weight, it returns a default weight of 1.0
    inline double Evaluator::findWeightByCompany(const std::string &company)
    {
        for (const auto &category : companyWeights->Category())
        {
            for (const auto &comp : category.Company())
            {
                for (const auto &name : comp.CompanyName())
                {
                    if (name == company)
                    {
                        if (comp.BobotPerPUJK().present()) {
                            return comp.BobotPerPUJK().get(); 
                        }
                    }
                }
            }
        }

        return 0.0;
    }

    // findWeightByCompanyType accepts company name
    // and return the weight of per bobotBidangUsaha
    inline double Evaluator::findWeightByCompanyType(const std::string &companyType)
    {
        for (const auto &category : companyWeights->Category())
        {
            for (const auto &comp : category.Company()){
                for (const auto &name : comp.CompanyName())
                {
                    if (name == companyType)
                    {
                        if (comp.BobotBidangUsaha().present()) {
                            return comp.BobotBidangUsaha().get(); 
                        }
                    }
                }
            }
        }

        return 0.0;
    }

    inline double Evaluator::findBankAndFinancingWeightByCompany(const std::string &company)
    {
        for (const auto &category : companyWeights->Category())
        {
            for (const auto &comp : category.Company())
            {
               for (const auto &name : comp.CompanyName())
               {
                    if (name == company)
                    {
                         if (comp.BobotBankPembiayaan().present()) {
                             return comp.BobotBankPembiayaan().get(); 
                         }
                    }
               }
            }
        }

        return 0.0;
    }

} // namespace kpmr::datasource

#endif // ILFX_KPMR_DATASOURCE_HPP
