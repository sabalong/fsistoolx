cmake_minimum_required(VERSION 3.16)
project(xsltcli LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(XML2 REQUIRED IMPORTED_TARGET libxml-2.0)
pkg_check_modules(XSLT REQUIRED IMPORTED_TARGET libxslt)
find_package(opentelemetry-cpp CONFIG REQUIRED)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../telemetry"
                 "${CMAKE_BINARY_DIR}/fsistoolx-telemetry")

add_executable(xsltcli main.c)
target_link_libraries(xsltcli PRIVATE PkgConfig::XML2 PkgConfig::XSLT)
set_target_properties(xsltcli PROPERTIES
    LINKER_LANGUAGE CXX
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
fsistoolx_enable_otel(xsltcli KEYWORD)
