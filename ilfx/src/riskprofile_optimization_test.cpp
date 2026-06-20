#include "riskprofile.hpp"

#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace {

bool expectEqual(const std::string& name, int actual, int expected) {
    if (actual != expected) {
        std::cerr << name << " expected " << expected << " but got " << actual << "\n";
        return false;
    }
    return true;
}

bool expectNear(const std::string& name, double actual, double expected) {
    if (std::fabs(actual - expected) > 0.000001) {
        std::cerr << name << " expected " << expected << " but got " << actual << "\n";
        return false;
    }
    return true;
}

} // namespace

int main() {
    riskprofile::Evaluator evaluator(nullptr, nullptr, nullptr, nullptr);
    bool ok = true;

    const std::string threshold =
        "1: x < 10\n"
        "2: 10 <= x < 20\n"
        "3: 20 <= x < 30\n"
        "4: 30 <= x < 40\n"
        "5: x >= 40";

    ok &= expectEqual("rating low", evaluator.ratingByThreshold(threshold, 5), 1);
    ok &= expectEqual("rating chained lower bound", evaluator.ratingByThreshold(threshold, 10), 2);
    ok &= expectEqual("rating chained middle", evaluator.ratingByThreshold(threshold, 25), 3);
    ok &= expectEqual("rating high", evaluator.ratingByThreshold(threshold, 45), 5);
    ok &= expectEqual("rating cache repeat", evaluator.ratingByThreshold(threshold, 15), 2);
    ok &= expectEqual("rating no match", evaluator.ratingByThreshold("1: x > 100", 50), -1);

    const std::string multiVarThreshold =
        "1: a > b and x <= 3\n"
        "2: a <= b or x > 3";

    std::map<std::string, chaiscript::Boxed_Value> firstVars;
    firstVars["a"] = chaiscript::var(5.0);
    firstVars["b"] = chaiscript::var(2.0);
    firstVars["x"] = chaiscript::var(3.0);
    ok &= expectEqual("multi variable first", evaluator.ratingByThresholdVars(multiVarThreshold, firstVars), 1);

    std::map<std::string, chaiscript::Boxed_Value> secondVars;
    secondVars["a"] = chaiscript::var(1.0);
    secondVars["b"] = chaiscript::var(2.0);
    secondVars["x"] = chaiscript::var(3.0);
    ok &= expectEqual("multi variable second", evaluator.ratingByThresholdVars(multiVarThreshold, secondVars), 2);

    ok &= expectNear("score comma map", evaluator.ratingToScore("1:92, 2:76, 3:60", 2), 76.0);
    ok &= expectNear("score newline map", evaluator.ratingToScore("1:92\n2:76\n3:60", 3), 60.0);
    ok &= expectNear("score no match", evaluator.ratingToScore("1:92, 2:76", 5), 0.0);

    return ok ? 0 : 1;
}
