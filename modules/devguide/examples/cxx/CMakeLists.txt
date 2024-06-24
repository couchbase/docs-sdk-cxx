cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(docs_cxx)

macro(define_example name)
    add_executable(${name} src/${name}.cxx)
    target_include_directories(${name} PUBLIC ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/private)
    target_link_libraries(
            ${name}
            ${CMAKE_THREAD_LIBS_INIT}
            taocpp::json
            couchbase_cxx_client)
    if(COUCHBASE_CXX_CLIENT_STATIC_BORINGSSL AND WIN32)
        # Ignore the `LNK4099: PDB ['crypto.pdb'|'ssl.pdb'] was not found` warnings, as we don't (atm) keep track fo the
        # *.PDB from the BoringSSL build
        set_target_properties(${name} PROPERTIES LINK_FLAGS "/ignore:4099")
    endif()
endmacro()

set(COUCHBASE_CXX_CLIENT_BUILD_DOCS FALSE)
set(COUCHBASE_CXX_CLIENT_BUILD_EXAMPLES FALSE)
set(COUCHBASE_CXX_CLIENT_BUILD_TESTS FALSE)
set(COUCHBASE_CXX_CLIENT_STATIC_BORINGSSL TRUE)
set(COUCHBASE_CXX_CLIENT_EMBED_MOZILLA_CA_BUNDLE FALSE)
add_subdirectory("${PROJECT_SOURCE_DIR}/couchbase-cxx-client")

file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cxx")

define_example(minimal)
define_example(subdocument)
