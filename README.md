## fsistoolx

Utility tool for Financial Integrated Service

## CLI tracing

The CLIs use the OpenTelemetry C++ SDK for traces and export with OTLP/HTTP.
Elastic APM remains the backend and the parent application tracer. `fsis-api`
passes its active Elastic span to each child process through the W3C
`TRACEPARENT` and `TRACESTATE` environment-variable carrier.

Configure the Elastic OTLP endpoint and authentication with the standard SDK
variables supplied by the deployment, for example:

```bash
OTEL_EXPORTER_OTLP_TRACES_ENDPOINT=https://elastic-otlp.example/v1/traces
OTEL_EXPORTER_OTLP_HEADERS='Authorization=Bearer ...'
```

Tracing is a no-op when `OTEL_SDK_DISABLED=true` or when neither
`OTEL_EXPORTER_OTLP_ENDPOINT` nor `OTEL_EXPORTER_OTLP_TRACES_ENDPOINT` is set.
Exporter failures do not change CLI exit codes or write SDK diagnostics into
the CLI output streams. Command arguments are not attached to spans because
they can contain sensitive data.

The implementation follows the official
[OpenTelemetry C++ instrumentation](https://opentelemetry.io/docs/languages/cpp/instrumentation/),
[OTLP exporter](https://opentelemetry.io/docs/languages/cpp/exporters/), and
[environment carrier](https://opentelemetry.io/docs/specs/otel/context/env-carriers/)
guidance.

When OpenTelemetry C++ is installed outside the default CMake search paths,
pass its install prefix explicitly to every CLI project:

```bash
cmake -S ilfx -B ilfx/build-otel \
  -DCMAKE_PREFIX_PATH="$HOME/.local/opentelemetry-cpp"
```

The prefix must contain the installed OpenTelemetry CMake package (normally
under `lib/cmake/opentelemetry-cpp`), rather than only the SDK source tree.

## Bundle 

Produce bundle by 

```
docker buildx build \
	-f Dockerfile \
	--target deps-bundle \
	--output type=tar,dest=deps-bundle-stage.tar \
	.

tar -xf deps-bundle-stage.tar deps-bundle.tar.gz
```

## macOS Build

Build the same project components locally on macOS with Homebrew dependencies:

```bash
# First run only, installs the expected Homebrew packages.
scripts/build_macos.sh --install-deps

# Later runs.
scripts/build_macos.sh
```

The script builds:

- `gofunct/build-macos/libgofunct.a`
- `ilfreporter-0.0.1/build-macos/`
- `ilf/build-macos/bin/`
- `ilfx/build-macos/bin/`
- `xsltcli/build-macos/bin/xsltcli`

If dependencies are already installed, skip `--install-deps`. The script expects Homebrew-provided `libxml2`, `libxslt`, `glib`, `gsl`, `xerces-c`, `xqilla`, and `antlr4-cpp-runtime`. It also needs TinyCC/libtcc; on macOS this may need to be installed manually because a Homebrew TinyCC formula is not always available.
