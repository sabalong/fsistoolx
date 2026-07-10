#ifndef FSISTOOLX_OTEL_CLI_C_H
#define FSISTOOLX_OTEL_CLI_C_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fsis_otel_runtime fsis_otel_runtime;
typedef struct fsis_otel_span fsis_otel_span;

/*
 * Start a CLI trace. The runtime reads TRACEPARENT/TRACESTATE and standard
 * OTEL_EXPORTER_OTLP_* variables from the process environment. All functions
 * are safe no-ops when tracing is disabled or not configured.
 */
fsis_otel_runtime *fsis_otel_start(const char *service_name);
fsis_otel_span *fsis_otel_start_span(fsis_otel_runtime *runtime, const char *span_name);
void fsis_otel_span_error(fsis_otel_span *span, const char *message);
void fsis_otel_end_span(fsis_otel_span *span);
void fsis_otel_root_error(fsis_otel_runtime *runtime, const char *message);
int fsis_otel_finish(fsis_otel_runtime *runtime, int exit_code);

#define FSIS_OTEL_RETURN(runtime, code) return fsis_otel_finish((runtime), (code))

#ifdef __cplusplus
}
#endif

#endif
