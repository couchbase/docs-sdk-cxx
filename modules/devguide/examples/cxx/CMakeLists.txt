cmake_minimum_required(VERSION 3.17)
include(cmake/CPM.cmake)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(docs_cxx)

macro(define_example name)
    add_executable(${name} src/${name}.cxx)
    target_include_directories(${name} PUBLIC ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/private)
    target_link_libraries(
            ${name}
            ${CMAKE_THREAD_LIBS_INIT}
            fmt::fmt
            taocpp::json
            couchbase_cxx_client)
endmacro()

cpmaddpackage(
        NAME
        fmt
        GIT_TAG
        11.2.0
        VERSION
        11.2.0
        GITHUB_REPOSITORY
        "fmtlib/fmt"
        EXCLUDE_FROM_ALL ON
        OPTIONS
        "FMT_INSTALL OFF"
        "FMT_UNICODE OFF"
        "FMT_DOC OFF"
        "BUILD_SHARED_LIBS OFF"
        "CMAKE_C_VISIBILITY_PRESET hidden"
        "CMAKE_CXX_VISIBILITY_PRESET hidden"
        "CMAKE_POSITION_INDEPENDENT_CODE ON"
)

cpmaddpackage(
        NAME
        couchbase_cxx_client
        GIT_TAG
        main
        VERSION
        1.2.0
        GITHUB_REPOSITORY
        "couchbase/couchbase-cxx-client"
        OPTIONS
        "COUCHBASE_CXX_CLIENT_INSTALL ON"
        "COUCHBASE_CXX_CLIENT_BUILD_SHARED ON"
        "COUCHBASE_CXX_CLIENT_BUILD_STATIC OFF")

file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cxx")

define_example(minimal)
define_example(kv_operations)
define_example(subdocument)
define_example(queries)
define_example(client_settings)
define_example(transactions)
define_example(data_model)
