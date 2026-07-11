#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <unistd.h>
#include <utility>

#ifdef ILFX_ENABLE_OTEL
#include "opentelemetry/context/context.h"
#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/context/propagation/text_map_propagator.h"
#include "opentelemetry/context/runtime_context.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_http_exporter_options.h"
#include "opentelemetry/nostd/shared_ptr.h"
#include "opentelemetry/nostd/string_view.h"
#include "opentelemetry/sdk/common/global_log_handler.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/sdk/trace/batch_span_processor_factory.h"
#include "opentelemetry/sdk/trace/batch_span_processor_options.h"
#include "opentelemetry/sdk/trace/provider.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/context.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/scope.h"
#include "opentelemetry/trace/span.h"
#include "opentelemetry/trace/span_metadata.h"
#include "opentelemetry/trace/span_startoptions.h"
#include "opentelemetry/trace/tracer.h"
#endif

namespace ilfx::otel {

#ifdef ILFX_ENABLE_OTEL
namespace context = opentelemetry::context;
namespace propagation = opentelemetry::context::propagation;
namespace otlp = opentelemetry::exporter::otlp;
namespace resource = opentelemetry::sdk::resource;
namespace trace = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace internal_log = opentelemetry::sdk::common::internal_log;

class CliTraceCarrier : public propagation::TextMapCarrier {
public:
    explicit CliTraceCarrier(std::map<std::string, std::string> headers)
        : headers_(std::move(headers))
    {
    }

    opentelemetry::nostd::string_view Get(
        opentelemetry::nostd::string_view key) const noexcept override
    {
        auto it = headers_.find(std::string(key));
        if (it != headers_.end()) {
            return it->second;
        }
        return "";
    }

    void Set(opentelemetry::nostd::string_view key,
             opentelemetry::nostd::string_view value) noexcept override
    {
        headers_[std::string(key)] = std::string(value);
    }

private:
    std::map<std::string, std::string> headers_;
};

class Runtime {
public:
    Runtime(const std::string& service_name,
            const std::string& traceparent,
            const std::string& tracestate)
        : service_name_(service_name),
          traceparent_present_(!traceparent.empty()),
          tracestate_present_(!tracestate.empty())
    {
        const char* sdk_disabled = std::getenv("OTEL_SDK_DISABLED");
        std::string disabled = sdk_disabled ? sdk_disabled : "";
        std::transform(disabled.begin(), disabled.end(), disabled.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        const bool endpoint_configured =
            std::getenv("OTEL_EXPORTER_OTLP_ENDPOINT") != nullptr ||
            std::getenv("OTEL_EXPORTER_OTLP_TRACES_ENDPOINT") != nullptr;
        if (disabled == "true" || !endpoint_configured) {
            return;
        }

        // Export failures must not alter a CLI's stderr contract. Elastic/OTLP
        // availability is observability state, not a business-command error.
        internal_log::GlobalLogHandler::SetLogHandler(
            opentelemetry::nostd::shared_ptr<internal_log::LogHandler>(
                new internal_log::NoopLogHandler()));

        // OtlpHttpExporterOptions reads the standard OTEL_EXPORTER_OTLP_*
        // environment variables. Elastic APM is configured as the OTLP
        // destination by the deployment, not in the executable.
        otlp::OtlpHttpExporterOptions opts;
        opts.console_debug = false;
        if (!std::getenv("OTEL_EXPORTER_OTLP_TIMEOUT") &&
            !std::getenv("OTEL_EXPORTER_OTLP_TRACES_TIMEOUT")) {
            opts.timeout = std::chrono::seconds(2);
        }

        auto exporter = otlp::OtlpHttpExporterFactory::Create(opts);
        trace_sdk::BatchSpanProcessorOptions processor_options{};
        processor_options.schedule_delay_millis = std::chrono::milliseconds(100);
        processor_options.max_export_batch_size = 64;
        auto processor = trace_sdk::BatchSpanProcessorFactory::Create(
            std::move(exporter), processor_options);

        resource::ResourceAttributes attributes = {
            {"service.name", service_name_},
            {"service.version", "1.0.0"}
        };
        auto service_resource = resource::Resource::Create(attributes);

        provider_ = trace_sdk::TracerProviderFactory::Create(std::move(processor), service_resource);
        std::shared_ptr<trace::TracerProvider> api_provider = provider_;
        trace_sdk::Provider::SetTracerProvider(api_provider);

        propagation::GlobalTextMapPropagator::SetGlobalPropagator(
            opentelemetry::nostd::shared_ptr<propagation::TextMapPropagator>(
                new trace::propagation::HttpTraceContext()));

        tracer_ = trace::Provider::GetTracerProvider()->GetTracer(service_name_);
        enabled_ = true;

        extractParent(traceparent, tracestate);
    }

    ~Runtime()
    {
        shutdown();
    }

    bool enabled() const
    {
        return enabled_;
    }

    const std::string& serviceName() const
    {
        return service_name_;
    }

    bool traceparentPresent() const
    {
        return traceparent_present_;
    }

    bool tracestatePresent() const
    {
        return tracestate_present_;
    }

    bool hasInvalidParent() const
    {
        return invalid_parent_;
    }

    opentelemetry::nostd::shared_ptr<trace::Span> startSpan(const std::string& name)
    {
        if (!enabled_ || !tracer_) {
            return opentelemetry::nostd::shared_ptr<trace::Span>();
        }
        return tracer_->StartSpan(name);
    }

    opentelemetry::nostd::shared_ptr<trace::Span> startRootSpan(const std::string& name)
    {
        if (!enabled_ || !tracer_) {
            return opentelemetry::nostd::shared_ptr<trace::Span>();
        }

        trace::StartSpanOptions options;
        if (parent_context_.IsValid()) {
            options.parent = parent_context_;
        }

        return tracer_->StartSpan(name, options);
    }

    std::unique_ptr<trace::Scope> activate(opentelemetry::nostd::shared_ptr<trace::Span>& span)
    {
        if (!enabled_ || !span) {
            return nullptr;
        }
        return std::make_unique<trace::Scope>(trace::Tracer::WithActiveSpan(span));
    }

    void shutdown()
    {
        if (provider_) {
            provider_->ForceFlush(std::chrono::seconds(2));
            provider_->Shutdown(std::chrono::seconds(2));
            provider_.reset();
        }

        if (enabled_) {
            std::shared_ptr<trace::TracerProvider> none;
            trace_sdk::Provider::SetTracerProvider(none);
            enabled_ = false;
        }
    }

private:
    void extractParent(const std::string& traceparent, const std::string& tracestate)
    {
        if (traceparent.empty()) {
            return;
        }

        std::map<std::string, std::string> headers;
        headers["traceparent"] = traceparent;
        if (!tracestate.empty()) {
            headers["tracestate"] = tracestate;
        }

        const CliTraceCarrier carrier(headers);
        auto propagator = propagation::GlobalTextMapPropagator::GetGlobalPropagator();
        auto current_context = context::RuntimeContext::GetCurrent();
        auto extracted_context = propagator->Extract(carrier, current_context);
        parent_context_ = trace::GetSpan(extracted_context)->GetContext();
        invalid_parent_ = !parent_context_.IsValid();
    }

    std::string service_name_;
    bool enabled_ = false;
    bool invalid_parent_ = false;
    bool traceparent_present_ = false;
    bool tracestate_present_ = false;
    trace::SpanContext parent_context_ = trace::SpanContext::GetInvalid();
    opentelemetry::nostd::shared_ptr<trace::Tracer> tracer_;
    std::shared_ptr<trace_sdk::TracerProvider> provider_;
};

inline void markError(trace::Span* span, const std::string& message)
{
    if (!span) {
        return;
    }

    span->SetStatus(trace::StatusCode::kError, message);
    span->SetAttribute("error", true);
    span->SetAttribute("error.message", message);
    span->AddEvent("error");
}

class ScopedSpan {
public:
    ScopedSpan(Runtime& otel, const std::string& name)
        : span_(otel.startSpan(name)),
          scope_(otel.activate(span_))
    {
    }

    ~ScopedSpan()
    {
        if (span_) {
            span_->End();
        }
    }

    trace::Span* get()
    {
        return span_.get();
    }

    template <typename T>
    void setAttribute(const std::string& key, const T& value)
    {
        if (span_) {
            span_->SetAttribute(key, value);
        }
    }

    void markError(const std::string& message)
    {
        ilfx::otel::markError(span_.get(), message);
    }

private:
    opentelemetry::nostd::shared_ptr<trace::Span> span_;
    std::unique_ptr<trace::Scope> scope_;
};

class RootSpan {
public:
    RootSpan(Runtime& otel, const std::string& name)
        : otel_(otel),
          span_(otel.startRootSpan(name)),
          scope_(otel.activate(span_))
    {
        setAttribute("process.executable.name", otel_.serviceName());
        setAttribute("process.pid", static_cast<std::int64_t>(::getpid()));
        setAttribute("traceparent.present", otel_.traceparentPresent());
        setAttribute("tracestate.present", otel_.tracestatePresent());
        if (otel_.hasInvalidParent()) {
            setAttribute("traceparent.valid", false);
            if (span_) {
                span_->AddEvent("invalid traceparent");
            }
        }
    }

    ~RootSpan()
    {
        if (!finished_) {
            finish(0);
        }
    }

    template <typename T>
    void setAttribute(const std::string& key, const T& value)
    {
        if (span_) {
            span_->SetAttribute(key, value);
        }
    }

    void markError(const std::string& message)
    {
        ilfx::otel::markError(span_.get(), message);
    }

    int finish(int code)
    {
        if (finished_) {
            return code;
        }

        if (span_) {
            span_->SetAttribute("process.exit.code", code);
            if (code != 0) {
                span_->SetStatus(trace::StatusCode::kError, "command failed");
                span_->SetAttribute("error.type", "_OTHER");
            }
            scope_.reset();
            span_->End();
        }
        otel_.shutdown();
        finished_ = true;
        return code;
    }

private:
    Runtime& otel_;
    opentelemetry::nostd::shared_ptr<trace::Span> span_;
    std::unique_ptr<trace::Scope> scope_;
    bool finished_ = false;
};

#else
class Runtime {
public:
    Runtime(const std::string&, const std::string&, const std::string&) {}
    bool enabled() const { return false; }
    const std::string& serviceName() const { return empty_; }
    bool traceparentPresent() const { return false; }
    bool tracestatePresent() const { return false; }
    bool hasInvalidParent() const { return false; }
    void shutdown() {}

private:
    std::string empty_;
};

class ScopedSpan {
public:
    ScopedSpan(Runtime&, const std::string&) {}
    void* get() { return nullptr; }

    template <typename T>
    void setAttribute(const std::string&, const T&) {}

    void markError(const std::string&) {}
};

class RootSpan {
public:
    RootSpan(Runtime&, const std::string&) {}

    template <typename T>
    void setAttribute(const std::string&, const T&) {}

    void markError(const std::string&) {}

    int finish(int code)
    {
        return code;
    }
};
#endif

inline std::string environmentValue(const char* name)
{
    const char* value = std::getenv(name);
    return value ? value : "";
}

inline Runtime runtimeFromEnvironment(const std::string& service_name)
{
    return Runtime(service_name,
                   environmentValue("TRACEPARENT"),
                   environmentValue("TRACESTATE"));
}

// Kept as a source-compatible name for the existing CLI entrypoints. Context
// and exporter configuration now come exclusively from environment carriers.
inline Runtime runtimeFromFlags(const std::string& service_name)
{
    return runtimeFromEnvironment(service_name);
}

} // namespace ilfx::otel
