#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "chaicli.hpp"
#include <chaiscript/chaiscript.hpp>

namespace ChaiClient::testing
{
    class ChaiCLITest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            cli = std::make_unique<ChaiCLI>();
        }

        void TearDown() override
        {
            cli.reset();
        }

        std::unique_ptr<ChaiCLI> cli;
    };

    // Test: Constructor initialization
    TEST_F(ChaiCLITest, ConstructorInitialization)
    {
        EXPECT_NE(cli, nullptr);
    }

    // Test: getChai returns valid ChaiScript instance
    TEST_F(ChaiCLITest, GetChaiReturnsValidInstance)
    {
        auto chai = cli->getChai();
        EXPECT_NE(chai, nullptr);
    }

    // Test: Evaluate simple arithmetic expression
    TEST_F(ChaiCLITest, EvaluateSimpleArithmetic)
    {
        auto result = cli->evaluate("2 + 3");
        int value = chaiscript::boxed_cast<int>(result.value);
        EXPECT_EQ(value, 5);
    }

    // Test: Evaluate floating point arithmetic
    TEST_F(ChaiCLITest, EvaluateFloatingPointArithmetic)
    {
        auto result = cli->evaluate("10.5 + 5.25");
        double value = chaiscript::boxed_cast<double>(result.value);
        EXPECT_DOUBLE_EQ(value, 15.75);
    }

    // Test: Evaluate division operation
    TEST_F(ChaiCLITest, EvaluateDivision)
    {
        auto result = cli->evaluate("20 / 4");
        int value = chaiscript::boxed_cast<int>(result.value);
        EXPECT_EQ(value, 5);
    }

    // Test: Evaluate multiplication operation
    TEST_F(ChaiCLITest, EvaluateMultiplication)
    {
        auto result = cli->evaluate("7 * 6");
        int value = chaiscript::boxed_cast<int>(result.value);
        EXPECT_EQ(value, 42);
    }

    // Test: Evaluate string concatenation
    TEST_F(ChaiCLITest, EvaluateStringConcatenation)
    {
        auto result = cli->evaluate("\"Hello\" + \" \" + \"World\"");
        std::string value = chaiscript::boxed_cast<std::string>(result.value);
        EXPECT_EQ(value, "Hello World");
    }

    // Test: Evaluate boolean expression
    TEST_F(ChaiCLITest, EvaluateBooleanExpression)
    {
        auto result = cli->evaluate("5 > 3");
        bool value = chaiscript::boxed_cast<bool>(result.value);
        EXPECT_TRUE(value);
    }

    // Test: Evaluate comparison operations
    TEST_F(ChaiCLITest, EvaluateComparison)
    {
        auto less = cli->evaluate("3 < 5");
        auto equal = cli->evaluate("5 == 5");
        auto greater_equal = cli->evaluate("10 >= 5");

        bool less_val = chaiscript::boxed_cast<bool>(less.value);
        bool equal_val = chaiscript::boxed_cast<bool>(equal.value);
        bool greater_equal_val = chaiscript::boxed_cast<bool>(greater_equal.value);

        EXPECT_TRUE(less_val);
        EXPECT_TRUE(equal_val);
        EXPECT_TRUE(greater_equal_val);
    }

    // Test: Evaluate complex arithmetic expression
    TEST_F(ChaiCLITest, EvaluateComplexExpression)
    {
        auto result = cli->evaluate("(10 + 5) * 2 - 3");
        int value = chaiscript::boxed_cast<int>(result.value);
        EXPECT_EQ(value, 27);
    }

    // Test: Evaluate variable assignment
    TEST_F(ChaiCLITest, EvaluateVariableAssignment)
    {
        cli->evaluate("var x = 42");
        auto result = cli->evaluate("x");
        int value = chaiscript::boxed_cast<int>(result.value);
        EXPECT_EQ(value, 42);
    }

    // Test: Evaluate function definition and call
    TEST_F(ChaiCLITest, EvaluateFunctionCall)
    {
        cli->evaluate("def add(a, b) { return a + b; }");
        auto result = cli->evaluate("add(10, 20)");
        int value = chaiscript::boxed_cast<int>(result.value);
        EXPECT_EQ(value, 30);
    }

    // Test: Evaluate lambda function
    TEST_F(ChaiCLITest, EvaluateLambda)
    {
        cli->evaluate("var double_val = fun(x) { return x * 2; }");
        auto result = cli->evaluate("double_val(21)");
        int value = chaiscript::boxed_cast<int>(result.value);
        EXPECT_EQ(value, 42);
    }

    // Test: Evaluate list operations
    TEST_F(ChaiCLITest, EvaluateListOperations)
    {
        cli->evaluate("var nums = [1, 2, 3, 4, 5]");
        auto first = cli->evaluate("nums[0]");
        auto last = cli->evaluate("nums[4]");

        int first_val = chaiscript::boxed_cast<int>(first.value);
        int last_val = chaiscript::boxed_cast<int>(last.value);

        EXPECT_EQ(first_val, 1);
        EXPECT_EQ(last_val, 5);
    }

    // Test: Evaluate conditional logic
    TEST_F(ChaiCLITest, EvaluateConditionalLogic)
    {
        cli->evaluate(
            "def classify(value) { "
            "  if (value > 100) { return \"HIGH\"; } "
            "  else if (value > 50) { return \"MEDIUM\"; } "
            "  else { return \"LOW\"; } "
            "}"
        );

        auto high = cli->evaluate("classify(150)");
        auto medium = cli->evaluate("classify(75)");
        auto low = cli->evaluate("classify(25)");

        std::string high_val = chaiscript::boxed_cast<std::string>(high.value);
        std::string medium_val = chaiscript::boxed_cast<std::string>(medium.value);
        std::string low_val = chaiscript::boxed_cast<std::string>(low.value);

        EXPECT_EQ(high_val, "HIGH");
        EXPECT_EQ(medium_val, "MEDIUM");
        EXPECT_EQ(low_val, "LOW");
    }

    // Test: Evaluate loop with accumulation
    TEST_F(ChaiCLITest, EvaluateLoopAccumulation)
    {
        cli->evaluate(
            "var total = 0; "
            "for(var i = 1; i <= 10; ++i) { total = total + i; } "
        );
        auto result = cli->evaluate("total");
        int value = chaiscript::boxed_cast<int>(result.value);
        EXPECT_EQ(value, 55);
    }

    // Test: Evaluate mathematical function
    TEST_F(ChaiCLITest, EvaluateMathematicalFunction)
    {
        cli->evaluate("def square(x) { return x * x; }");
        auto result = cli->evaluate("square(8)");
        int value = chaiscript::boxed_cast<int>(result.value);
        EXPECT_EQ(value, 64);
    }

    // Test: Evaluate ratio calculation (financial use case)
    TEST_F(ChaiCLITest, EvaluateRatioCalculation)
    {
        cli->evaluate(
            "def calculate_ratio(numerator, denominator) { "
            "  if (denominator == 0) { return 0.0; } "
            "  return numerator / denominator; "
            "}"
        );
        auto result = cli->evaluate("calculate_ratio(100.0, 250.0)");
        double value = chaiscript::boxed_cast<double>(result.value);
        EXPECT_DOUBLE_EQ(value, 0.4);
    }

    // Test: Evaluate weighted sum
    TEST_F(ChaiCLITest, EvaluateWeightedSum)
    {
        cli->evaluate(
            "def weighted_sum(values, weights) { "
            "  var sum = 0.0; "
            "  var len = 0; "
            "  for(var i = 0; i < values.size(); ++i) { "
            "    sum = sum + (values[i] * weights[i]); "
            "    len = len + 1; "
            "  } "
            "  return sum; "
            "}"
        );
        
        cli->evaluate("var vals = [10.0, 20.0, 30.0]");
        cli->evaluate("var wts = [0.2, 0.3, 0.5]");
        auto result = cli->evaluate("weighted_sum(vals, wts)");
        
        double value = chaiscript::boxed_cast<double>(result.value);
        EXPECT_DOUBLE_EQ(value, 23.0);
    }

    // Test: Evaluate risk assessment formula
    TEST_F(ChaiCLITest, EvaluateRiskAssessment)
    {
        cli->evaluate(
            "def assess_risk_level(value, low_threshold, high_threshold) { "
            "  if (value >= high_threshold) { return \"CRITICAL\"; } "
            "  else if (value >= low_threshold) { return \"WARNING\"; } "
            "  else { return \"OK\"; } "
            "}"
        );

        auto critical = cli->evaluate("assess_risk_level(95, 50, 80)");
        auto warning = cli->evaluate("assess_risk_level(65, 50, 80)");
        auto ok = cli->evaluate("assess_risk_level(25, 50, 80)");

        std::string critical_val = chaiscript::boxed_cast<std::string>(critical.value);
        std::string warning_val = chaiscript::boxed_cast<std::string>(warning.value);
        std::string ok_val = chaiscript::boxed_cast<std::string>(ok.value);

        EXPECT_EQ(critical_val, "CRITICAL");
        EXPECT_EQ(warning_val, "WARNING");
        EXPECT_EQ(ok_val, "OK");
    }

    // Test: Evaluate multi-statement code block
    TEST_F(ChaiCLITest, EvaluateMultipleStatements)
    {
        cli->evaluate("var a = 10; var b = 20; var c = a + b");
        auto result = cli->evaluate("c");
        int value = chaiscript::boxed_cast<int>(result.value);
        EXPECT_EQ(value, 30);
    }

    // Test: Exception handling for invalid code
    TEST_F(ChaiCLITest, EvaluateInvalidCodeThrows)
    {
        EXPECT_THROW(
        {
            cli->evaluate("var x = undefined_variable");
        },
        ilfx::chaiscript_diagnostics::EvaluationError);
    }

    TEST_F(ChaiCLITest, EvaluationErrorContainsCompleteExecutionContext)
    {
        const std::string script = "var x = undefined_variable";
        try {
            cli->evaluate(
                script,
                "fsi",
                "datasource code=R001, company=Example Bank",
                {
                    {"code", "R001"},
                    {"companyName", "Example Bank"},
                    {"value", "42.5"},
                });
            FAIL() << "expected ChaiScript evaluation to fail";
        } catch (const ilfx::chaiscript_diagnostics::EvaluationError& error) {
            EXPECT_EQ(error.script(), script);
            EXPECT_EQ(error.ruleKind(), "fsi");
            EXPECT_EQ(error.entity(), "datasource code=R001, company=Example Bank");
            EXPECT_EQ(error.context().at("value"), "42.5");

            const std::string diagnostic = error.what();
            EXPECT_NE(diagnostic.find(script), std::string::npos);
            EXPECT_NE(diagnostic.find("rule_kind: fsi"), std::string::npos);
            EXPECT_NE(diagnostic.find("companyName = Example Bank"), std::string::npos);
            EXPECT_FALSE(error.formattedError().empty());
        }
    }

    // Test: Multiple evaluate calls maintain state
    TEST_F(ChaiCLITest, StatePreservedAcrossEvaluations)
    {
        cli->evaluate("var counter = 0");
        cli->evaluate("counter = counter + 5");
        cli->evaluate("counter = counter + 10");
        
        auto result = cli->evaluate("counter");
        int value = chaiscript::boxed_cast<int>(result.value);
        EXPECT_EQ(value, 15);
    }

    TEST(ChaiScriptSharedRuntimeTest, RuleFunctionsIsolateLocalsAndAllowNestedHelperEvaluation)
    {
        chaiscript::ChaiScript chai;

        chai.add(chaiscript::fun([&chai](const std::string& function_name) {
            auto function = chai.eval<std::function<double()>>(function_name);
            return function();
        }), "evaluateRuleByName");

        chai.eval("def child_rule() { var result = 4.0; return result; }");
        chai.eval(
            "def parent_rule() { "
            "  var result = evaluateRuleByName(\"child_rule\"); "
            "  return result * 2.0; "
            "}");

        auto child = chai.eval<std::function<double()>>("child_rule");
        auto parent = chai.eval<std::function<double()>>("parent_rule");

        EXPECT_DOUBLE_EQ(child(), 4.0);
        EXPECT_DOUBLE_EQ(parent(), 8.0);
        EXPECT_DOUBLE_EQ(parent(), 8.0);
    }

    // Test: Destructor cleanup
    TEST_F(ChaiCLITest, DestructorCleanup)
    {
        EXPECT_NO_THROW(
        {
            auto temp_cli = std::make_unique<ChaiCLI>();
            temp_cli->evaluate("var x = 42");
            temp_cli.reset();
        });
    }

} // namespace ChaiClient::testing

// Main test runner
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
