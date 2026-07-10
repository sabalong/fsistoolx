cmake_minimum_required(VERSION 3.16)
project(ilf_cli LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(XML2 REQUIRED IMPORTED_TARGET libxml-2.0)
pkg_check_modules(GSL REQUIRED IMPORTED_TARGET gsl)
find_library(TCC_LIBRARY NAMES tcc REQUIRED PATHS /usr/local/lib /opt/homebrew/lib)

find_package(opentelemetry-cpp CONFIG REQUIRED)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../telemetry"
                 "${CMAKE_BINARY_DIR}/fsistoolx-telemetry")

set(ILF_COMMON_SOURCES
    src/eval.c
    src/risk_profile_tree.c
    src/kpmr_source_tree.c
    src/parser.c
    src/glibh.c
    src/gslh.c
    src/tcch.c
    src/xmlh.c
)

add_library(ilf_common STATIC ${ILF_COMMON_SOURCES})
target_include_directories(ilf_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src /usr/local/include)
target_link_libraries(ilf_common
    PUBLIC PkgConfig::GLIB PkgConfig::XML2 PkgConfig::GSL ${TCC_LIBRARY} ${CMAKE_DL_LIBS} m)

function(add_traced_ilf_cli target source)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(${target} PRIVATE ilf_common)
    set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)
    fsistoolx_enable_otel(${target} KEYWORD)
endfunction()

add_traced_ilf_cli(eval_cli src/eval_cli.c)
add_traced_ilf_cli(build_tree_cli src/build_tree_cli.c)
add_traced_ilf_cli(kpmr_source_tree_cli src/kpmr_source_tree_cli.c)
add_traced_ilf_cli(src_inherent_cli src/src_inherent_cli.c)

add_executable(build_tree_kpmr_cli
    src/build_tree_kpmr_cli.c
    src/xmlh.c
    src/glibh.c
)
target_include_directories(build_tree_kpmr_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(build_tree_kpmr_cli PRIVATE PkgConfig::GLIB PkgConfig::XML2)
set_target_properties(build_tree_kpmr_cli PROPERTIES LINKER_LANGUAGE CXX)
fsistoolx_enable_otel(build_tree_kpmr_cli KEYWORD)

add_executable(eval_server src/eval_server.c)
target_include_directories(eval_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(eval_server PRIVATE ilf_common)
