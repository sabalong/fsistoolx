cmake_minimum_required(VERSION 3.16)

if(TARGET fsistoolx_otel_cli)
    return()
endif()

if(TARGET opentelemetry-cpp::otlp_http_exporter)
    set(
        FSISTOOLX_OTEL_HTTP_TARGET
        opentelemetry-cpp::otlp_http_exporter
    )
elseif(TARGET opentelemetry_exporter_otlp_http)
    set(
        FSISTOOLX_OTEL_HTTP_TARGET
        opentelemetry_exporter_otlp_http
    )
else()
    message(
        FATAL_ERROR
        "OpenTelemetry OTLP HTTP exporter target was not found. "
        "Install or rebuild opentelemetry-cpp with WITH_OTLP_HTTP=ON."
    )
endif()

if(TARGET opentelemetry-cpp::trace)
    set(
        FSISTOOLX_OTEL_TRACE_TARGET
        opentelemetry-cpp::trace
    )
elseif(TARGET opentelemetry_trace)
    set(
        FSISTOOLX_OTEL_TRACE_TARGET
        opentelemetry_trace
    )
else()
    message(
        FATAL_ERROR
        "OpenTelemetry trace target was not found."
    )
endif()

if(TARGET opentelemetry-cpp::resources)
    set(
        FSISTOOLX_OTEL_RESOURCE_TARGET
        opentelemetry-cpp::resources
    )
elseif(TARGET opentelemetry_resources)
    set(
        FSISTOOLX_OTEL_RESOURCE_TARGET
        opentelemetry_resources
    )
else()
    message(
        FATAL_ERROR
        "OpenTelemetry resources target was not found."
    )
endif()

add_library(
    fsistoolx_otel_cli
    STATIC
    otel_cli_c.cpp
)

target_compile_features(
    fsistoolx_otel_cli
    PUBLIC
        cxx_std_17
)

target_compile_definitions(
    fsistoolx_otel_cli
    PUBLIC
        ILFX_ENABLE_OTEL
)

target_include_directories(
    fsistoolx_otel_cli
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../ilfx/src"
)

target_link_libraries(
    fsistoolx_otel_cli
    PUBLIC
        ${FSISTOOLX_OTEL_HTTP_TARGET}
        ${FSISTOOLX_OTEL_TRACE_TARGET}
        ${FSISTOOLX_OTEL_RESOURCE_TARGET}
)

function(fsistoolx_enable_otel target_name)
    if(NOT TARGET "${target_name}")
        message(
            FATAL_ERROR
            "Cannot enable OpenTelemetry for unknown target: ${target_name}"
        )
    endif()

    target_compile_definitions(
        "${target_name}"
        PRIVATE
            ILFX_ENABLE_OTEL
    )

    target_link_libraries(
        "${target_name}"
        PRIVATE
            fsistoolx_otel_cli
    )
endfunction()