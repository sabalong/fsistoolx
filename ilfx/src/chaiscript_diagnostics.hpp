#pragma once

#include "otel_cli.hpp"

#include <chaiscript/chaiscript.hpp>

#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

namespace ilfx::chaiscript_diagnostics {

using Context = std::map<std::string, std::string>;

inline std::string value(const std::string& input) { return input; }
inline std::string value(const char* input) { return input ? input : ""; }
inline std::string value(bool input) { return input ? "true" : "false"; }

template <typename T>
inline std::string value(const T& input)
{
    std::ostringstream stream;
    stream << std::setprecision(17) << input;
    return stream.str();
}

template <typename T>
inline std::string value(const std::vector<T>& input)
{
    std::ostringstream stream;
    stream << '[';
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (i != 0) {
            stream << ", ";
        }
        stream << value(input[i]);
    }
    stream << ']';
    return stream.str();
}

inline std::string formatContext(const Context& context)
{
    std::ostringstream stream;
    for (const auto& [name, context_value] : context) {
        stream << "  " << name << " = " << context_value << '\n';
    }
    return stream.str();
}

inline std::string formatDiagnostic(
    const std::string& rule_kind,
    const std::string& entity,
    const std::string& script,
    const Context& context,
    const std::string& exception_type,
    const std::string& exception_message,
    const std::string& formatted_error)
{
    std::ostringstream stream;
    stream << "ChaiScript evaluation failed\n"
           << "rule_kind: " << rule_kind << '\n'
           << "entity: " << entity << '\n'
           << "exception.type: " << exception_type << '\n'
           << "exception.message: " << exception_message << '\n'
           << "script.begin\n"
           << script << '\n'
           << "script.end\n"
           << "context.begin\n"
           << formatContext(context)
           << "context.end";
    if (!formatted_error.empty()) {
        stream << "\nchaiscript.error.begin\n"
               << formatted_error << '\n'
               << "chaiscript.error.end";
    }
    return stream.str();
}

class EvaluationError : public std::runtime_error {
public:
    EvaluationError(std::string rule_kind,
                    std::string entity,
                    std::string script,
                    Context context,
                    std::string exception_type,
                    std::string exception_message,
                    std::string formatted_error)
        : std::runtime_error(formatDiagnostic(rule_kind,
                                              entity,
                                              script,
                                              context,
                                              exception_type,
                                              exception_message,
                                              formatted_error)),
          rule_kind_(std::move(rule_kind)),
          entity_(std::move(entity)),
          script_(std::move(script)),
          context_(std::move(context)),
          exception_type_(std::move(exception_type)),
          exception_message_(std::move(exception_message)),
          formatted_error_(std::move(formatted_error))
    {
    }

    const std::string& ruleKind() const { return rule_kind_; }
    const std::string& entity() const { return entity_; }
    const std::string& script() const { return script_; }
    const Context& context() const { return context_; }
    const std::string& exceptionType() const { return exception_type_; }
    const std::string& exceptionMessage() const { return exception_message_; }
    const std::string& formattedError() const { return formatted_error_; }

private:
    std::string rule_kind_;
    std::string entity_;
    std::string script_;
    Context context_;
    std::string exception_type_;
    std::string exception_message_;
    std::string formatted_error_;
};

template <typename Result>
inline Result evaluate(chaiscript::ChaiScript& chai,
                       const std::string& script,
                       const std::string& rule_kind,
                       const std::string& entity,
                       Context context = {})
{
    try {
        return chai.eval<Result>(script);
    } catch (const chaiscript::exception::eval_error& error) {
        throw EvaluationError(rule_kind,
                              entity,
                              script,
                              std::move(context),
                              "chaiscript::exception::eval_error",
                              error.what(),
                              error.pretty_print());
    } catch (const std::exception& error) {
        throw EvaluationError(rule_kind,
                              entity,
                              script,
                              std::move(context),
                              typeid(error).name(),
                              error.what(),
                              "");
    }
}

inline chaiscript::Boxed_Value evaluateBoxed(chaiscript::ChaiScript& chai,
                                             const std::string& script,
                                             const std::string& rule_kind,
                                             const std::string& entity,
                                             Context context = {})
{
    try {
        return chai.eval(script);
    } catch (const chaiscript::exception::eval_error& error) {
        throw EvaluationError(rule_kind,
                              entity,
                              script,
                              std::move(context),
                              "chaiscript::exception::eval_error",
                              error.what(),
                              error.pretty_print());
    } catch (const std::exception& error) {
        throw EvaluationError(rule_kind,
                              entity,
                              script,
                              std::move(context),
                              typeid(error).name(),
                              error.what(),
                              "");
    }
}

template <typename Span>
inline void record(Span& span, const EvaluationError& error)
{
    span.markError(error.what());
    span.setAttribute("exception.type", error.exceptionType());
    span.setAttribute("exception.message", error.exceptionMessage());
    span.setAttribute("chaiscript.rule.kind", error.ruleKind());
    span.setAttribute("chaiscript.entity", error.entity());
    span.setAttribute("chaiscript.script", error.script());
    span.setAttribute("chaiscript.context", formatContext(error.context()));
    span.setAttribute("chaiscript.formatted_error", error.formattedError());
}

} // namespace ilfx::chaiscript_diagnostics
