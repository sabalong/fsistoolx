#include "otel_cli_c.h"

#include "otel_cli.hpp"

#include <new>
#include <string>

struct fsis_otel_runtime {
    explicit fsis_otel_runtime(const std::string& service_name)
        : runtime(ilfx::otel::runtimeFromEnvironment(service_name)),
          root(runtime, service_name)
    {
    }

    ilfx::otel::Runtime runtime;
    ilfx::otel::RootSpan root;
};

struct fsis_otel_span {
    fsis_otel_span(fsis_otel_runtime& runtime, const std::string& span_name)
        : span(runtime.runtime, span_name)
    {
    }

    ilfx::otel::ScopedSpan span;
};

extern "C" fsis_otel_runtime *fsis_otel_start(const char *service_name)
{
    try {
        const std::string service = service_name && *service_name
            ? service_name
            : "fsistoolx_cli";
        return new fsis_otel_runtime(service);
    } catch (...) {
        return nullptr;
    }
}

extern "C" fsis_otel_span *fsis_otel_start_span(
    fsis_otel_runtime *runtime, const char *span_name)
{
    if (!runtime || !span_name || !*span_name) {
        return nullptr;
    }
    try {
        return new fsis_otel_span(*runtime, span_name);
    } catch (...) {
        return nullptr;
    }
}

extern "C" void fsis_otel_span_error(fsis_otel_span *span, const char *message)
{
    if (span) {
        span->span.markError(message ? message : "operation failed");
    }
}

extern "C" void fsis_otel_end_span(fsis_otel_span *span)
{
    delete span;
}

extern "C" void fsis_otel_root_error(fsis_otel_runtime *runtime, const char *message)
{
    if (runtime) {
        runtime->root.markError(message ? message : "command failed");
    }
}

extern "C" int fsis_otel_finish(fsis_otel_runtime *runtime, int exit_code)
{
    if (!runtime) {
        return exit_code;
    }

    const int result = runtime->root.finish(exit_code);
    delete runtime;
    return result;
}
