cmake_minimum_required(VERSION 3.10)
project(ilfx VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Xerces
find_path(XERCES_INCLUDE_DIR
        NAMES xercesc/util/PlatformUtils.hpp
        PATHS /opt/homebrew/include
        /usr/local/include
        /usr/include
)

find_library(XERCES_LIBRARIES
        NAMES xerces-c
        PATHS /opt/homebrew/lib
        /usr/local/lib
        /usr/lib
)


if(NOT XERCES_INCLUDE_DIR OR NOT XERCES_LIBRARIES)
    message(FATAL_ERROR "Xerces-C not found")
endif()

# ANTLR4 Runtime
find_path(ANTLR4_INCLUDE_DIR
        NAMES antlr4-runtime.h
        PATHS /opt/homebrew/include/antlr4-runtime
        /usr/local/include/antlr4-runtime
        /usr/include/antlr4-runtime
)

find_library(ANTLR4_LIBRARIES
        NAMES antlr4-runtime
        PATHS /opt/homebrew/lib
        /usr/local/lib
        /usr/lib
)

if(NOT ANTLR4_INCLUDE_DIR OR NOT ANTLR4_LIBRARIES)
    message(FATAL_ERROR "ANTLR4 C++ runtime not found")
endif()

# GSL (GNU Scientific Library)
find_package(GSL REQUIRED)
if(NOT GSL_FOUND)
    message(STATUS "GSL not found via find_package, attempting manual search...")
    find_path(GSL_INCLUDE_DIR
            NAMES gsl/gsl_vector.h
            PATHS /opt/gsl/include
            /usr/local/include
            /usr/include
    )
    find_library(GSL_LIBRARY
            NAMES gsl
            PATHS /opt/gsl/lib
            /usr/local/lib
            /usr/lib
    )
    find_library(GSLCBLAS_LIBRARY
            NAMES gslcblas
            PATHS /opt/gsl/lib
            /usr/local/lib
            /usr/lib
    )
    if(NOT GSL_INCLUDE_DIR OR NOT GSL_LIBRARY)
        message(WARNING "GSL not found, some features may not compile")
    endif()
endif()

if(TARGET GSL::gsl)
    set(ILFX_GSL_LIBRARIES GSL::gsl)
else()
    set(ILFX_GSL_LIBRARIES ${GSL_LIBRARIES})
endif()

# Find all XSD schema files
file(GLOB XSD_SCHEMAS "${CMAKE_CURRENT_SOURCE_DIR}/xsd/*.xsd")

# Custom target to generate C++ bindings from XSD schemas
add_custom_target(generate_bindings
    COMMAND xsd cxx-tree --generate-serialization 
            --output-dir ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
            ${XSD_SCHEMAS}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating C++ bindings from XSD schemas"
    VERBATIM
)

# Add source files
set(INHERENT_SOURCES
    gen/cpp/InherentDataSource.cxx
    gen/cpp/Weight.cxx
    gen/cpp/InherentRiskProfile.cxx
)

set(KPMR_SOURCES
    gen/cpp/KPMRDataSource.cxx
    gen/cpp/Weight.cxx
    gen/cpp/KPMRRiskProfile.cxx
)

# Create main library for Inherent datasource
add_library(ilfx_lib ${INHERENT_SOURCES})
target_include_directories(ilfx_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${XERCES_INCLUDE_DIR}
)
target_link_directories(ilfx_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../gofunct/build-macos
    ${CMAKE_CURRENT_SOURCE_DIR}/../gofunct/gen
    /usr/local/lib
)
target_link_libraries(ilfx_lib PUBLIC
    ${XERCES_LIBRARIES}
    gofunct
)

# Create library for KPMR datasource
add_library(kpmr_lib ${KPMR_SOURCES})
target_include_directories(kpmr_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${XERCES_INCLUDE_DIR}
)
target_link_libraries(kpmr_lib PUBLIC
    ${XERCES_LIBRARIES}
)

# Find required packages
find_package(GTest REQUIRED)

add_subdirectory(thirdparty/abseil-cpp)

# Add test executable
add_executable(inherent_datasource_test
    src/inherent_datasource_test.cpp
)

add_executable(chaicli_test src/chaicli_test.cpp)
target_include_directories(chaicli_test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${XERCES_INCLUDE_DIR}
)
target_link_libraries(chaicli_test
    ilfx_lib
    ${XERCES_LIBRARIES}
    GTest::gtest
    GTest::gtest_main
    pthread
    ${ILFX_GSL_LIBRARIES}
)
add_test(NAME ChaiCLITests COMMAND chaicli_test)

# Add CLI executable
add_executable(ilfx_cli src/ilfx_cli.cpp)
target_include_directories(ilfx_cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${XERCES_INCLUDE_DIR}
)
target_link_libraries(ilfx_cli
    ilfx_lib
    ${XERCES_LIBRARIES}
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    absl::log_internal_message
    pthread
    ${ILFX_GSL_LIBRARIES}
)

# Add ChaiScript CLI executable
add_executable(chaiscript_cli src/chaiscript_cli.cpp)
target_include_directories(chaiscript_cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)
target_link_libraries(chaiscript_cli
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    pthread
    ${ILFX_GSL_LIBRARIES}
)

# Add KPMR CLI executable
add_executable(kpmr_cli src/kpmr_cli.cpp)
target_include_directories(kpmr_cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${XERCES_INCLUDE_DIR}
)
target_link_libraries(kpmr_cli
    kpmr_lib
    ${XERCES_LIBRARIES}
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    absl::log_internal_message
    pthread
)

# Define Threshold sources (used by multiple targets)
set(THRESHOLD_SOURCES
    gen/cpp/ThresholdGrammar/antlr4/ThresholdLexer.cpp
    gen/cpp/ThresholdGrammar/antlr4/ThresholdParser.cpp
    gen/cpp/ThresholdGrammar/antlr4/ThresholdBaseVisitor.cpp
    gen/cpp/ThresholdGrammar/antlr4/ThresholdVisitor.cpp
)

# Add Risk Profile CLI executable
add_executable(riskprofile_cli src/riskprofile_cli.cpp src/exprtkevaluator.cpp ${THRESHOLD_SOURCES})
target_include_directories(riskprofile_cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp/ThresholdGrammar/antlr4
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${XERCES_INCLUDE_DIR}
    ${ANTLR4_INCLUDE_DIR}
)
target_link_libraries(riskprofile_cli
    ilfx_lib
    kpmr_lib
    ${XERCES_LIBRARIES}
    ${ANTLR4_LIBRARIES}
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    absl::log_internal_message
    pthread
)

add_executable(riskprofilekpmr_cli src/riskprofilekpmr_cli.cpp src/exprtkevaluator.cpp ${THRESHOLD_SOURCES})
target_include_directories(riskprofilekpmr_cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp/ThresholdGrammar/antlr4
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${XERCES_INCLUDE_DIR}
    ${ANTLR4_INCLUDE_DIR}
)
target_link_libraries(riskprofilekpmr_cli
    ilfx_lib
    kpmr_lib
    ${XERCES_LIBRARIES}
    ${ANTLR4_LIBRARIES}
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    absl::log_internal_message
    pthread
)

# Add Threshold CLI executable
add_executable(threshold_cli src/threshold_cli.cpp ${THRESHOLD_SOURCES})
target_include_directories(threshold_cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp/ThresholdGrammar/antlr4
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${ANTLR4_INCLUDE_DIR}
)
target_link_libraries(threshold_cli
    ${ANTLR4_LIBRARIES}
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    absl::log_internal_message
    pthread
)

# Add ExprTk CLI executable
add_executable(exprtk_cli src/exprtk_cli.cpp src/exprtkevaluator.cpp)
target_include_directories(exprtk_cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)
target_link_libraries(exprtk_cli
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    absl::log_internal_message
    pthread
)

# Add Score Rating Parser CLI executable
add_executable(scoreratingparser_cli src/scoreratingparser_cli.cpp src/scoreratingparser.cpp)
target_include_directories(scoreratingparser_cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(scoreratingparser_cli
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    absl::log_internal_message
    pthread
)
# scoreratingparser uses C++20 ranges
set_target_properties(scoreratingparser_cli PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Add Rating Threshold CLI executable
add_executable(rating_threshold_cli src/rating_threshold_cli.cpp src/exprtkevaluator.cpp ${THRESHOLD_SOURCES})
target_include_directories(rating_threshold_cli PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp/ThresholdGrammar/antlr4
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${ANTLR4_INCLUDE_DIR}
)
target_link_libraries(rating_threshold_cli
    ${ANTLR4_LIBRARIES}
    absl::flags
    absl::flags_parse
    absl::log
    absl::log_initialize
    absl::log_internal_message
    pthread
)

target_include_directories(inherent_datasource_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gen/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)

target_link_libraries(inherent_datasource_test
    ilfx_lib
    ${XERCES_LIBRARIES}
    GTest::gtest
    GTest::gtest_main
    pthread
    absl::base
    absl::log
    ${ILFX_GSL_LIBRARIES}
)

# Enable testing
enable_testing()
add_test(NAME InherentDataSourceTests COMMAND inherent_datasource_test)

# Optional: Add all test discovery
gtest_discover_tests(inherent_datasource_test)

# Compiler flags
if(MSVC)
    target_compile_options(inherent_datasource_test PRIVATE /W4)
else()
    target_compile_options(inherent_datasource_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Print build info
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Output Directory: ${CMAKE_BINARY_DIR}")
