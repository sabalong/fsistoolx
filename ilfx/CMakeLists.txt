cmake_minimum_required(VERSION 3.16)

project(ilfx VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

option(
    ILFX_ENABLE_OTEL
    "Enable OpenTelemetry OTLP HTTP tracing for ilfx CLI executables"
    ON
)

if(ILFX_ENABLE_OTEL)
    find_package(opentelemetry-cpp CONFIG REQUIRED)

    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/../telemetry"
        "${CMAKE_BINARY_DIR}/fsistoolx-telemetry"
    )
endif()

# ---------------------------------------------------------------------------
# Xerces-C
# ---------------------------------------------------------------------------

find_path(
    XERCES_INCLUDE_DIR
    NAMES xercesc/util/PlatformUtils.hpp
    PATHS
        /opt/homebrew/include
        /usr/local/include
        /usr/include
)

find_library(
    XERCES_LIBRARIES
    NAMES xerces-c
    PATHS
        /opt/homebrew/lib
        /usr/local/lib
        /usr/lib
)

if(NOT XERCES_INCLUDE_DIR OR NOT XERCES_LIBRARIES)
    message(FATAL_ERROR "Xerces-C not found")
endif()

# ---------------------------------------------------------------------------
# ANTLR4 Runtime
# ---------------------------------------------------------------------------

find_path(
    ANTLR4_INCLUDE_DIR
    NAMES antlr4-runtime.h
    PATHS
        /opt/homebrew/include/antlr4-runtime
        /usr/local/include/antlr4-runtime
        /usr/include/antlr4-runtime
)

find_library(
    ANTLR4_LIBRARIES
    NAMES antlr4-runtime
    PATHS
        /opt/homebrew/lib
        /usr/local/lib
        /usr/lib
)

if(NOT ANTLR4_INCLUDE_DIR OR NOT ANTLR4_LIBRARIES)
    message(FATAL_ERROR "ANTLR4 C++ runtime not found")
endif()

# ---------------------------------------------------------------------------
# GNU Scientific Library
# ---------------------------------------------------------------------------

find_package(GSL REQUIRED)

if(TARGET GSL::gsl)
    set(ILFX_GSL_LIBRARIES GSL::gsl)
else()
    set(ILFX_GSL_LIBRARIES ${GSL_LIBRARIES})
endif()

# ---------------------------------------------------------------------------
# Tests and third-party dependencies
# ---------------------------------------------------------------------------

enable_testing()

find_package(GTest REQUIRED)
include(GoogleTest)

if(NOT TARGET absl::base)
    add_subdirectory(thirdparty/abseil-cpp)
endif()

# ---------------------------------------------------------------------------
# XSD bindings
# ---------------------------------------------------------------------------

file(
    GLOB XSD_SCHEMAS
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/xsd/*.xsd"
)

add_custom_target(
    generate_bindings
    COMMAND
        xsd cxx-tree
        --generate-serialization
        --output-dir "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp"
        ${XSD_SCHEMAS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Generating C++ bindings from XSD schemas"
    VERBATIM
)

# ---------------------------------------------------------------------------
# Generated source files
# ---------------------------------------------------------------------------

set(
    INHERENT_SOURCES
    gen/cpp/InherentDataSource.cxx
    gen/cpp/Weight.cxx
    gen/cpp/InherentRiskProfile.cxx
)

set(
    KPMR_SOURCES
    gen/cpp/KPMRDataSource.cxx
    gen/cpp/Weight.cxx
    gen/cpp/KPMRRiskProfile.cxx
)

set(
    THRESHOLD_SOURCES
    gen/cpp/ThresholdGrammar/antlr4/ThresholdLexer.cpp
    gen/cpp/ThresholdGrammar/antlr4/ThresholdParser.cpp
    gen/cpp/ThresholdGrammar/antlr4/ThresholdBaseVisitor.cpp
    gen/cpp/ThresholdGrammar/antlr4/ThresholdVisitor.cpp
)

# ---------------------------------------------------------------------------
# Inherent library
# ---------------------------------------------------------------------------

add_library(ilfx_lib ${INHERENT_SOURCES})

target_include_directories(
    ilfx_lib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
        "${XERCES_INCLUDE_DIR}"
)

target_link_directories(
    ilfx_lib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../gofunct/build-macos"
        "${CMAKE_CURRENT_SOURCE_DIR}/../gofunct/gen"
        /usr/local/lib
)

target_link_libraries(
    ilfx_lib
    PUBLIC
        "${XERCES_LIBRARIES}"
        gofunct
)

# ---------------------------------------------------------------------------
# KPMR library
# ---------------------------------------------------------------------------

add_library(kpmr_lib ${KPMR_SOURCES})

target_include_directories(
    kpmr_lib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
        "${XERCES_INCLUDE_DIR}"
)

target_link_libraries(
    kpmr_lib
    PUBLIC
        "${XERCES_LIBRARIES}"
)

# ---------------------------------------------------------------------------
# Inherent datasource test
# ---------------------------------------------------------------------------

add_executable(
    inherent_datasource_test
    src/inherent_datasource_test.cpp
)

target_include_directories(
    inherent_datasource_test
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
)

target_link_libraries(
    inherent_datasource_test
    PRIVATE
        ilfx_lib
        "${XERCES_LIBRARIES}"
        GTest::gtest
        GTest::gtest_main
        absl::base
        absl::log
        ${ILFX_GSL_LIBRARIES}
)

if(NOT MSVC)
    target_link_libraries(
        inherent_datasource_test
        PRIVATE
            pthread
    )
endif()

if(MSVC)
    target_compile_options(
        inherent_datasource_test
        PRIVATE
            /W4
    )
else()
    target_compile_options(
        inherent_datasource_test
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
endif()

gtest_discover_tests(inherent_datasource_test)

# ---------------------------------------------------------------------------
# Chai CLI test
# ---------------------------------------------------------------------------

add_executable(
    chaicli_test
    src/chaicli_test.cpp
)

target_include_directories(
    chaicli_test
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
        "${XERCES_INCLUDE_DIR}"
)

target_link_libraries(
    chaicli_test
    PRIVATE
        ilfx_lib
        "${XERCES_LIBRARIES}"
        GTest::gtest
        GTest::gtest_main
        ${ILFX_GSL_LIBRARIES}
)

if(NOT MSVC)
    target_link_libraries(
        chaicli_test
        PRIVATE
            pthread
    )
endif()

add_test(
    NAME ChaiCLITests
    COMMAND chaicli_test
)

# ---------------------------------------------------------------------------
# ILFX CLI
# ---------------------------------------------------------------------------

add_executable(
    ilfx_cli
    src/ilfx_cli.cpp
)

target_include_directories(
    ilfx_cli
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
        "${XERCES_INCLUDE_DIR}"
)

target_link_libraries(
    ilfx_cli
    PRIVATE
        ilfx_lib
        "${XERCES_LIBRARIES}"
        absl::flags
        absl::flags_parse
        absl::log
        absl::log_initialize
        absl::log_internal_message
        ${ILFX_GSL_LIBRARIES}
)

if(NOT MSVC)
    target_link_libraries(
        ilfx_cli
        PRIVATE
            pthread
    )
endif()

# ---------------------------------------------------------------------------
# ChaiScript CLI
# ---------------------------------------------------------------------------

add_executable(
    chaiscript_cli
    src/chaiscript_cli.cpp
)

target_include_directories(
    chaiscript_cli
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
)

target_link_libraries(
    chaiscript_cli
    PRIVATE
        absl::flags
        absl::flags_parse
        absl::log
        absl::log_initialize
        ${ILFX_GSL_LIBRARIES}
)

if(NOT MSVC)
    target_link_libraries(
        chaiscript_cli
        PRIVATE
            pthread
    )
endif()

# ---------------------------------------------------------------------------
# KPMR CLI
# ---------------------------------------------------------------------------

add_executable(
    kpmr_cli
    src/kpmr_cli.cpp
)

target_include_directories(
    kpmr_cli
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
        "${XERCES_INCLUDE_DIR}"
)

target_link_libraries(
    kpmr_cli
    PRIVATE
        kpmr_lib
        "${XERCES_LIBRARIES}"
        absl::flags
        absl::flags_parse
        absl::log
        absl::log_initialize
        absl::log_internal_message
)

if(NOT MSVC)
    target_link_libraries(
        kpmr_cli
        PRIVATE
            pthread
    )
endif()

# ---------------------------------------------------------------------------
# Risk Profile CLI
# ---------------------------------------------------------------------------

add_executable(
    riskprofile_cli
    src/riskprofile_cli.cpp
    src/exprtkevaluator.cpp
    ${THRESHOLD_SOURCES}
)

target_include_directories(
    riskprofile_cli
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp/ThresholdGrammar/antlr4"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
        "${XERCES_INCLUDE_DIR}"
        "${ANTLR4_INCLUDE_DIR}"
)

target_link_libraries(
    riskprofile_cli
    PRIVATE
        ilfx_lib
        kpmr_lib
        "${XERCES_LIBRARIES}"
        "${ANTLR4_LIBRARIES}"
        absl::flags
        absl::flags_parse
        absl::log
        absl::log_initialize
        absl::log_internal_message
)

if(NOT MSVC)
    target_link_libraries(
        riskprofile_cli
        PRIVATE
            pthread
    )
endif()

# ---------------------------------------------------------------------------
# Risk Profile KPMR CLI
# ---------------------------------------------------------------------------

add_executable(
    riskprofilekpmr_cli
    src/riskprofilekpmr_cli.cpp
    src/exprtkevaluator.cpp
    ${THRESHOLD_SOURCES}
)

target_include_directories(
    riskprofilekpmr_cli
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp/ThresholdGrammar/antlr4"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
        "${XERCES_INCLUDE_DIR}"
        "${ANTLR4_INCLUDE_DIR}"
)

target_link_libraries(
    riskprofilekpmr_cli
    PRIVATE
        ilfx_lib
        kpmr_lib
        "${XERCES_LIBRARIES}"
        "${ANTLR4_LIBRARIES}"
        absl::flags
        absl::flags_parse
        absl::log
        absl::log_initialize
        absl::log_internal_message
)

if(NOT MSVC)
    target_link_libraries(
        riskprofilekpmr_cli
        PRIVATE
            pthread
    )
endif()

# ---------------------------------------------------------------------------
# Threshold CLI
# ---------------------------------------------------------------------------

add_executable(
    threshold_cli
    src/threshold_cli.cpp
    ${THRESHOLD_SOURCES}
)

target_include_directories(
    threshold_cli
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp/ThresholdGrammar/antlr4"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
        "${ANTLR4_INCLUDE_DIR}"
)

target_link_libraries(
    threshold_cli
    PRIVATE
        "${ANTLR4_LIBRARIES}"
        absl::flags
        absl::flags_parse
        absl::log
        absl::log_initialize
        absl::log_internal_message
)

if(NOT MSVC)
    target_link_libraries(
        threshold_cli
        PRIVATE
            pthread
    )
endif()

# ---------------------------------------------------------------------------
# ExprTk CLI
# ---------------------------------------------------------------------------

add_executable(
    exprtk_cli
    src/exprtk_cli.cpp
    src/exprtkevaluator.cpp
)

target_include_directories(
    exprtk_cli
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
)

target_link_libraries(
    exprtk_cli
    PRIVATE
        absl::flags
        absl::flags_parse
        absl::log
        absl::log_initialize
        absl::log_internal_message
)

if(NOT MSVC)
    target_link_libraries(
        exprtk_cli
        PRIVATE
            pthread
    )
endif()

# ---------------------------------------------------------------------------
# Score Rating Parser CLI
# ---------------------------------------------------------------------------

add_executable(
    scoreratingparser_cli
    src/scoreratingparser_cli.cpp
    src/scoreratingparser.cpp
)

target_include_directories(
    scoreratingparser_cli
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(
    scoreratingparser_cli
    PRIVATE
        absl::flags
        absl::flags_parse
        absl::log
        absl::log_initialize
        absl::log_internal_message
)

if(NOT MSVC)
    target_link_libraries(
        scoreratingparser_cli
        PRIVATE
            pthread
    )
endif()

set_target_properties(
    scoreratingparser_cli
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)

# ---------------------------------------------------------------------------
# Rating Threshold CLI
# ---------------------------------------------------------------------------

add_executable(
    rating_threshold_cli
    src/rating_threshold_cli.cpp
    src/exprtkevaluator.cpp
    ${THRESHOLD_SOURCES}
)

target_include_directories(
    rating_threshold_cli
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp/ThresholdGrammar/antlr4"
        "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty"
        "${ANTLR4_INCLUDE_DIR}"
)

target_link_libraries(
    rating_threshold_cli
    PRIVATE
        "${ANTLR4_LIBRARIES}"
        absl::flags
        absl::flags_parse
        absl::log
        absl::log_initialize
        absl::log_internal_message
)

if(NOT MSVC)
    target_link_libraries(
        rating_threshold_cli
        PRIVATE
            pthread
    )
endif()

# ---------------------------------------------------------------------------
# OpenTelemetry integration
# ---------------------------------------------------------------------------

set(
    ILFX_CLI_TARGETS
    ilfx_cli
    chaiscript_cli
    kpmr_cli
    riskprofile_cli
    riskprofilekpmr_cli
    threshold_cli
    exprtk_cli
    scoreratingparser_cli
    rating_threshold_cli
)

if(ILFX_ENABLE_OTEL)
    foreach(_ilfx_cli_target IN LISTS ILFX_CLI_TARGETS)
        fsistoolx_enable_otel("${_ilfx_cli_target}")
    endforeach()
endif()

# ---------------------------------------------------------------------------
# Build information
# ---------------------------------------------------------------------------

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Output Directory: ${CMAKE_BINARY_DIR}")