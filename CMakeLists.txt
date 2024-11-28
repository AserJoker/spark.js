cmake_minimum_required(VERSION 3.12)
project(spark)
set(CMAKE_CXX_STANDARD 20)
include_directories(${PROJECT_SOURCE_DIR}/include)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/common COMMON_SOURCES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/compiler COMPILER_SOURCES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/vm VM_SOURCES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/engine/base ENGINE_BASE_SOURCES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/engine/entity ENGINE_ENTITY_SOURCES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/engine/runtime ENGINE_RUNTIME_SOURCES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/engine/lib ENGINE_LIB_SOURCES)
find_package(fmt CONFIG REQUIRED)
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCE_DIR}/src/main.cc
    ${COMMON_SOURCES}
    ${COMPILER_SOURCES}
    ${ENGINE_BASE_SOURCES}
    ${ENGINE_ENTITY_SOURCES}
    ${ENGINE_RUNTIME_SOURCES}
    ${ENGINE_LIB_SOURCES}
    ${VM_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt-header-only)
