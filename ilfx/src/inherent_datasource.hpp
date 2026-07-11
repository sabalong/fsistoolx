#ifndef ILFX_INHERENT_DATASOURCE_HPP
#define ILFX_INHERENT_DATASOURCE_HPP

#include "allheaders.hpp"
#include "InherentDataSource.hxx"
#include "chaicli.hpp"
#include "chaiscript_diagnostics.hpp"
#include "chaiscript/extras/math.hpp"
#include "absl/log/log.h"
#include "Weight.hxx"
#include "libgofunct.h"
#include <optional>

typedef std::vector<int> int_vector;
typedef std::map<std::string, int_vector> map_type;
typedef std::pair<const std::string, int_vector> map_pair;

namespace inherent::datasource
{
    class Evaluator
    {
    private:
        /* data */
        std::shared_ptr<DataType> datasources;
        std::shared_ptr<CompanyWeights> companyWeights;
        
        

        // Store computed input values for companies
        // this will be populated when we are evaluating the FSI rules
        std::unordered_map<std::string, double> computedInputs;

        // Helper function to set up ChaiScript evaluator with all necessary bindings
        void setupChaiScriptEvaluator(chaiscript::ChaiScript& chai);

    public:
        Evaluator(const std::string &configPath, const std::string &weightPath);
        ~Evaluator();

        std::unordered_map<std::string, double> getComputedInputs() const
        {
            return computedInputs;
        };

        std::shared_ptr<DataType> getDataSources() const
        {
            return datasources;
        };

        OperationStatus evaluate();

        OperationStatus riskProfileEvaluation();

        // Return a map of company names to their corresponding vector of values for a given code
        chaiscript::Boxed_Value companyValuesByCode(const std::string &code);

        // Return a vector of company names for a given code
        chaiscript::Boxed_Value companyNamesByCode(const std::string &code);

        chaiscript::Boxed_Value companyValuesByCodeAndCompanyName(const std::string &code, const std::string &companyName);
        chaiscript::Boxed_Value companyValuesByCodeAndCompanyNameV2(const std::string &code, const std::string &companyName);
        chaiscript::Boxed_Value companyValueAndEligibilityByCode(const std::string &code);

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

        std::string getCompanyTypeByCodeAndName(const std::string &code, const std::string &companyName);
        
        std::string mapCompanyType(const std::string &companyTypeCode);

        // Find item by code, evaluate its consolidation rule and return the consolidated value
        double evalConsolidationByCode(const std::string &code);
    };

    Evaluator::Evaluator(const std::string &configPath, const std::string &weightPath)
    {
        datasources = data(configPath);
        companyWeights = CompanyWeights_(weightPath);
        computedInputs = std::unordered_map<std::string, double>{};
    }

    Evaluator::~Evaluator()
    {
    }

    inline std::optional<double> safeBoxedCastDouble(const chaiscript::Boxed_Value &value)
    {
        if (value.is_undef())
        {
            return std::nullopt;
        }

        try
        {
            return chaiscript::boxed_cast<double>(value);
        }
        catch (const chaiscript::exception::bad_boxed_cast &)
        {
            return std::nullopt;
        }
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
                     [this](const std::string &code)
                     {
                         return this->companyNamesByCode(code);
                     }),
                 "companyNamesByCode");

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
                     [this](const std::string &code, const std::string &companyName)
                     {
                         return this->companyValuesByCodeAndCompanyNameV2(code, companyName);
                     }),
                 "companyValuesByCodeAndCompanyNameV2");

        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->companyValueAndEligibilityByCode(code);
                     }),
                 "companyValueAndEligibilityByCode");
        
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
        
        chai.add(chaiscript::fun(
                     [this](const std::string &code, const std::string &companyName)
                     {
                         return this->getCompanyTypeByCodeAndName(code, companyName);
                     }),
                 "getCompanyTypeByCodeAndName");

        chai.add(chaiscript::fun(
                     [this](const std::string &companyTypeCode)
                     {
                         return this->mapCompanyType(companyTypeCode);
                     }),
                 "mapCompanyType");

        chai.add(chaiscript::fun(
                     [this](const std::string &code)
                     {
                         return this->evalConsolidationByCode(code);
                     }),
                 "evalConsolidationByCode");

        // Register majority function
        chai.add(chaiscript::fun([](const std::vector<chaiscript::Boxed_Value>& values_boxed) {
            // Convert boxed values to doubles
            std::vector<double> values;
            
            for (const auto& val : values_boxed) {
                values.push_back(chaiscript::boxed_cast<double>(val));
            }
            
            if (values.empty()) {
                throw std::runtime_error("Majority: input vector cannot be empty");
            }
            
            return MajorityScoreF64(values.data(), values.size());
        }), "majority");

        // Register PearsonCorrelation function
        chai.add(chaiscript::fun([](const std::vector<chaiscript::Boxed_Value>& x_boxed, 
                                     const std::vector<chaiscript::Boxed_Value>& y_boxed) {
            // Convert boxed values to doubles
            std::vector<double> x;
            std::vector<double> y;
            
            for (const auto& val : x_boxed) {
                x.push_back(chaiscript::boxed_cast<double>(val));
            }
            for (const auto& val : y_boxed) {
                y.push_back(chaiscript::boxed_cast<double>(val));
            }
            
            if (x.size() != y.size()) {
                throw std::runtime_error("PearsonCorrelation: x and y must have the same length");
            }
            
            return PearsonCorrelationF64(x.data(), y.data(), x.size());
        }), "correlation");

        // Register minimum function
        chai.add(chaiscript::fun([](const std::vector<chaiscript::Boxed_Value>& values_boxed) {
            std::vector<double> values;
            
            for (const auto& val : values_boxed) {
                values.push_back(chaiscript::boxed_cast<double>(val));
            }
            
            if (values.empty()) {
                throw std::runtime_error("Minimum: input vector cannot be empty");
            }
            
            return MinimumScoreF64(values.data(), values.size());
        }), "minimum");
    }

    inline OperationStatus Evaluator::evaluate()
    {
        // register built in functions for chaiscript if needed

        // std::shared_ptr<chaiscript::ChaiScript> chai = cli->getChai();
        

        // first step, we need to evaluate all FSI rules and update the values accordingly
        for (auto &item : datasources->list().item())
        {


            std::string code = item.code();

            std::string fsiRule = "";

            if (item.fsiRule().present())
            {
                fsiRule = item.fsiRule().get();
            }

            std::string consolidationRule = "";

            if (item.consolidationRule().present())
            {
                consolidationRule = item.consolidationRule().get();
            }

            if (item.detail().present())
            {
                for (auto &row : item.detail()->row())
                {
                    
                    std::string companyName = row.companyName();
                    std::string companyType = "";
                    if (row.companyType().present())
                    {
                        companyType = row.companyType().get();
                    }

                    double value = row.value();
                    if (!fsiRule.empty())
                    {
                        DLOG(INFO) << "Evaluating FSI Rule for code " << code << ": " << fsiRule;

                        // TODO: this can we made more flexible by providing an opaque object
                        // that can be queried for data instead of injecting individual variables
                        chaiscript::ChaiScript chai;
                        setupChaiScriptEvaluator(chai);
                        
                        chai.add(chaiscript::var(code), "code");
                        chai.add(chaiscript::var(companyName), "companyName");
                        chai.add(chaiscript::var(companyType), "companyType");
                        chai.add(chaiscript::var(value), "value");

                        chaiscript::Boxed_Value v = ilfx::chaiscript_diagnostics::evaluateBoxed(
                            chai,
                            fsiRule,
                            "fsi",
                            "datasource code=" + code + ", company=" + companyName,
                            {
                                {"code", code},
                                {"companyName", companyName},
                                {"companyType", companyType},
                                {"value", ilfx::chaiscript_diagnostics::value(value)},
                            });
                        auto computedValue = safeBoxedCastDouble(v);
                        if (!computedValue.has_value())
                        {
                            DLOG(INFO) << "FSI rule returned undefined or non-numeric result for company " << companyName << " and code " << code;
                            row.value(0);
                            row.eligible(false);
                        }else {
                            row.value(*computedValue);
                             row.eligible(true);

                             DLOG(INFO) << "Computed value for company " << companyName << " and code " << code << ": " << *computedValue;
                        }

                        // Update the row's value with the computed value
                        
                    }
                }
            }

            // if (!consolidationRule.empty()){
            //     chaiscript::ChaiScript chai;
            //     setupChaiScriptEvaluator(chai);

            //     chaiscript::Boxed_Value v = chai.eval(consolidationRule);
            //     double consolidatedValue = chai.boxed_cast<double>(v);

            //     // Set the consolidated value somewhere
            //     // For this example, we'll just log it
            //     DLOG(INFO) << "Consolidated value for code " << item.code() << ": " << consolidatedValue;

            //     item.consolidate(consolidatedValue);
            // }   

        }

        // If we reach here, evaluation was successful
        // then we need to evaluate the consolidation rules
        for (auto &item : datasources->list().item())
        {

            std::string fsiRule = "";

            if (item.fsiRule().present())
            {
                // already evaluated in the previous loop
                fsiRule = item.fsiRule().get();    
            }

            std::string consolidationRule = "";

            if (item.consolidationRule().present())
            {
                consolidationRule = item.consolidationRule().get();
            }


            if (!consolidationRule.empty())
            {
                DLOG(INFO) << "Evaluating Consolidation Rule for code " << item.code() << ": " << consolidationRule;

                chaiscript::ChaiScript chai;
                setupChaiScriptEvaluator(chai);
                chaiscript::Boxed_Value v = ilfx::chaiscript_diagnostics::evaluateBoxed(
                    chai,
                    consolidationRule,
                    "consolidation",
                    "datasource code=" + item.code(),
                    {{"code", item.code()}});
                auto consolidatedValue = safeBoxedCastDouble(v);
                if (!consolidatedValue.has_value())
                {
                    DLOG(INFO) << "Consolidation rule returned undefined or non-numeric result for code " << item.code();
                    item.consolidate(0);

                    
                }else {
                    DLOG(INFO) << "Consolidated value for code " << item.code() << ": " << *consolidatedValue;

                   item.consolidate(*consolidatedValue);
                }

                // Set the consolidated value somewhere
                // For this example, we'll just log it
                
            }
            
        }

        return SuccessOperationStatus;
    }

    // Return a map of company names to their corresponding vector of values for a given code
    inline chaiscript::Boxed_Value Evaluator::companyValuesByCode(const std::string &code)
    {

        for (const auto &item : datasources->list().item())
        {
            if (item.code() == code)
            {
                std::map<std::string, std::vector<double>> raw;

                if (item.detail().present())
                {
                    for (const auto &row : item.detail()->row())
                    {
                        LOG(INFO) << "Found row for company " << row.companyName() << " with value " << row.value();
                        raw[row.companyName()] = {row.value()};
                        
                        
                    }
                }

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
        }

        return chaiscript::var(std::map<std::string, chaiscript::Boxed_Value>{});
    }

    // Return a vector of company names for a given code
    inline chaiscript::Boxed_Value Evaluator::companyNamesByCode(const std::string &code)
    {
        for (const auto &item : datasources->list().item())
        {
            if (item.code() == code)
            {
                std::vector<std::string> companyNames;

                if (item.detail().present())
                {
                    for (const auto &row : item.detail()->row())
                    {
                        companyNames.push_back(row.companyName());
                    }
                }

                std::vector<chaiscript::Boxed_Value> boxed_names;
                boxed_names.reserve(companyNames.size());

                for (const auto &name : companyNames)
                    boxed_names.push_back(chaiscript::var(name));

                return chaiscript::var(boxed_names);
            }
        }

        return chaiscript::var(std::vector<chaiscript::Boxed_Value>{});
    }

    inline chaiscript::Boxed_Value Evaluator::companyValuesByCodeAndCompanyName(const std::string &code, const std::string &companyName)
    {
        for (const auto &item : datasources->list().item())
        {
            if (item.code() == code)
            {
                if (item.detail().present())
                {
                    for (const auto &row : item.detail()->row())
                    {
                        if (row.companyName() == companyName)
                        {
                            return chaiscript::var(row.value());
                        }
                    }
                }
            }
        }

        return chaiscript::var(0.0);
    }

    inline chaiscript::Boxed_Value Evaluator::companyValuesByCodeAndCompanyNameV2(const std::string &code, const std::string &companyName)
    {
        for (const auto &item : datasources->list().item())
        {
            if (item.code() == code)
            {
                if (item.detail().present())
                {
                    for (const auto &row : item.detail()->row())
                    {
                        if (row.companyName() == companyName)
                        {
                            std::map<std::string, chaiscript::Boxed_Value> result;
                            result["value"] = chaiscript::var(row.value());
                            result["eligible"] = chaiscript::var(static_cast<bool>(row.eligible()));

                            return chaiscript::var(result);
                        }
                    }
                }
            }
        }

        std::map<std::string, chaiscript::Boxed_Value> result;
        result["value"] = chaiscript::var(0.0);
        result["eligible"] = chaiscript::var(false);

        return chaiscript::var(result);
    }

    inline chaiscript::Boxed_Value Evaluator::companyValueAndEligibilityByCode(const std::string &code)
    {
        for (const auto &item : datasources->list().item())
        {
            if (item.code() == code)
            {
                std::map<std::string, chaiscript::Boxed_Value> result;

                if (item.detail().present())
                {
                    for (const auto &row : item.detail()->row())
                    {
                        std::map<std::string, chaiscript::Boxed_Value> companyData;
                        companyData["value"] = chaiscript::var(row.value());
                        companyData["eligible"] = chaiscript::var(static_cast<bool>(row.eligible()));

                        result[row.companyName()] = chaiscript::var(companyData);
                    }
                }

                return chaiscript::var(result);
            }
        }

        return chaiscript::var(std::map<std::string, chaiscript::Boxed_Value>{});
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

    inline std::string Evaluator::getCompanyTypeByCodeAndName(const std::string &code, const std::string &companyName)
    {
        for (const auto &item : datasources->list().item())
        {
            if (item.code() == code)
            {
                if (item.detail().present())
                {
                    for (const auto &row : item.detail()->row())
                    {
                        if (row.companyName() == companyName)
                        {
                            if (row.companyType().present()) {
                                return row.companyType().get();
                            }
                        }
                    }
                }
            }
        }

        return "";
    }

    inline double Evaluator::evalConsolidationByCode(const std::string &code)
    {
        for (auto &item : datasources->list().item())
        {
            if (item.code() == code)
            {
                if (!item.consolidationRule().present())
                {
                    DLOG(INFO) << "No consolidation rule for code " << code;
                    return 0.0;
                }

                std::string consolidationRule = item.consolidationRule().get();

                DLOG(INFO) << "Evaluating Consolidation Rule for code " << code << ": " << consolidationRule;

                chaiscript::ChaiScript chai;
                setupChaiScriptEvaluator(chai);
                chaiscript::Boxed_Value v = ilfx::chaiscript_diagnostics::evaluateBoxed(
                    chai,
                    consolidationRule,
                    "consolidation",
                    "datasource code=" + code,
                    {{"code", code}});
                auto consolidatedValue = safeBoxedCastDouble(v);
                if (!consolidatedValue.has_value())
                {
                    DLOG(INFO) << "Consolidation rule returned undefined or non-numeric result for code " << code;
                    return 0.0;
                }

                DLOG(INFO) << "Consolidated value for code " << code << ": " << *consolidatedValue;

                //item.consolidate(consolidatedValue);

                return *consolidatedValue;
            }
        }

        DLOG(INFO) << "Item not found for code " << code;
        return 0.0;
    }

    inline std::string Evaluator::mapCompanyType(const std::string &companyTypeCode)
    {
        static const std::unordered_map<std::string, std::string> companyTypeMap = {
            {"BANK", "Bank"},
            {"FINANCING_COMPANY", "Perusahaan Pembiayaan"},
            {"REINSURANCE_COMPANY", "Perusahaan Asuransi & Perusahaan Reasuransi"},
            {"INSURANCE_COMPANY", "Perusahaan Asuransi & Perusahaan Reasuransi"},
            {"SECURITIES_COMPANY", "Perusahaan Efek"}
        };

        auto it = companyTypeMap.find(companyTypeCode);
        if (it != companyTypeMap.end()) {
            return it->second;
        }

        return companyTypeCode; // Return the original code if not found
    }

} // namespace inherent::datasource

#endif // ILFX_INHERENT_DATASOURCE_HPP
