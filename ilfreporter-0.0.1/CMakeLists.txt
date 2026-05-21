cmake_minimum_required(VERSION 3.20)
project(ilfreporter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(thirdparty/abseil-cpp)

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

find_path(XQILLA_INCLUDE_DIR xqilla/xqilla-dom3.hpp
    PATHS
        /opt/homebrew/include
        /usr/local/include
)

find_library(XQILLA_LIBRARY NAMES xqilla
    PATHS
        /opt/homebrew/lib/xqilla
        /opt/homebrew/lib
        /usr/local/lib
)

if(NOT XQILLA_INCLUDE_DIR)
    message(FATAL_ERROR "xqilla headers not found; set XQILLA_INCLUDE_DIR")
endif()
if(NOT XQILLA_LIBRARY)
    message(FATAL_ERROR "xqilla library not found; set XQILLA_LIBRARY")
endif()

get_filename_component(XQILLA_LIBRARY_DIR ${XQILLA_LIBRARY} DIRECTORY)

if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    # Ensure Homebrew library locations are part of the runtime search path.
    list(APPEND CMAKE_BUILD_RPATH
        "/opt/homebrew/lib"
        "/usr/local/lib"
        ${XQILLA_LIBRARY_DIR}
    )
    set(CMAKE_INSTALL_RPATH "${CMAKE_BUILD_RPATH}")
endif()

add_library(riskprofile
    src/riskprofile.cpp
    src/kpmrriskprofile.cpp
    src/exprtkevaluator.cpp
    gen/cpp/InherentRiskProfile.cxx
    gen/cpp/InherentRiskProfileConf.cxx
    gen/cpp/KPMRRiskProfile.cxx
    gen/cpp/KPMRRiskProfileConf.cxx
    gen/cpp/ThresholdGrammar/antlr4/ThresholdLexer.cpp
    gen/cpp/ThresholdGrammar/antlr4/ThresholdParser.cpp
    gen/cpp/ThresholdGrammar/antlr4/ThresholdBaseVisitor.cpp
    gen/cpp/ThresholdGrammar/antlr4/ThresholdVisitor.cpp
)

target_include_directories(riskprofile
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/gen/cpp
        ${PROJECT_SOURCE_DIR}/gen/cpp/ThresholdGrammar/antlr4
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
        ${XQILLA_INCLUDE_DIR}
        ${ANTLR4_INCLUDE_DIR}
        ${XERCES_INCLUDE_DIR}
)

target_link_libraries(riskprofile
    PUBLIC
        absl::status
        absl::strings
        ${XQILLA_LIBRARY}
        ${ANTLR4_LIBRARIES}
        ${XERCES_LIBRARIES}
)

add_executable(ilfreporter src/main.cpp)

target_link_libraries(ilfreporter
    PRIVATE
        riskprofile
        absl::flags
        absl::flags_parse
)

if(APPLE)
    set_target_properties(ilfreporter PROPERTIES
        BUILD_RPATH "${CMAKE_BUILD_RPATH}"
    )
endif()

add_executable(xpath_cli src/xpath_cli.cpp)

target_include_directories(xpath_cli
    PRIVATE
        ${XQILLA_INCLUDE_DIR}
)

target_link_libraries(xpath_cli
    PRIVATE
        riskprofile
        absl::flags
        absl::flags_parse
)

if(APPLE)
    set_target_properties(xpath_cli PROPERTIES
        BUILD_RPATH "${CMAKE_BUILD_RPATH}"
    )
endif()

add_executable(kpmr_cli src/kpmr_cli.cpp)

target_include_directories(kpmr_cli
    PRIVATE
        ${XQILLA_INCLUDE_DIR}
)

target_link_libraries(kpmr_cli
    PRIVATE
        riskprofile
        absl::flags
        absl::flags_parse
)

if(APPLE)
    set_target_properties(kpmr_cli PROPERTIES
        BUILD_RPATH "${CMAKE_BUILD_RPATH}"
    )
endif()
