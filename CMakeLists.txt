cmake_minimum_required(VERSION 3.11)
project(DPCstruct)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
)

# Set the output directories 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)  
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_TESTS ${CMAKE_BINARY_DIR}/tests)  
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)   


# Catch2 ---------------------------------------------------------------


find_package(Catch2 3 QUIET)

if (NOT Catch2_FOUND)
    message(STATUS "Catch2 not found, fetching it using FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.0  # Replace with the desired Catch2 version
    )
    FetchContent_MakeAvailable(Catch2)
endif()

include(Catch)


# Libraries ---------------------------------------------------------------

add_library(lib_primarycluster STATIC
    src/primarycluster_core.cc
    src/primarycluster_proc.cc
    src/common/distance.cc
    src/common/sort.cc

)
set_target_properties(lib_primarycluster PROPERTIES
    OUTPUT_NAME primarycluster
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

add_library(lib_secondarycluster STATIC
    # src/secondarycluster.cc
    src/secondarycluster/distance_module.cc
    src/secondarycluster/distance_proc.cc
    src/secondarycluster/classify_module.cc
    src/secondarycluster/classify_proc.cc
    src/common/distance.cc
)
set_target_properties(lib_secondarycluster PROPERTIES
    OUTPUT_NAME secondarycluster
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# memorymapped
add_library(memorymapped STATIC
    lib/memorymapped/MemoryMapped.cc
)
set_target_properties(memorymapped PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# openmp
find_package(OpenMP REQUIRED)

# Pipeline ---------------------------------------------------------------

# Prefilter
add_executable(prefilters
    src/prefilters.cc
    src/fileparser/AlnsFileParser.cc
)

set_target_properties(prefilters PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

target_link_libraries(prefilters PRIVATE memorymapped)
if(OpenMP_CXX_FOUND)
    target_link_libraries(prefilters PRIVATE OpenMP::OpenMP_CXX)
    target_compile_options(prefilters PRIVATE ${OpenMP_CXX_FLAGS})
endif()

# Primary cluster
add_executable(primarycluster
    src/primarycluster.cc
    src/fileparser/AlnsFileParser.cc
)

set_target_properties(primarycluster PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

target_link_libraries(primarycluster PRIVATE lib_primarycluster)
target_link_libraries(primarycluster PRIVATE memorymapped)
if(OpenMP_CXX_FOUND)
    target_link_libraries(primarycluster PRIVATE OpenMP::OpenMP_CXX)
    target_compile_options(primarycluster PRIVATE ${OpenMP_CXX_FLAGS})
endif()

# Secondary cluster
add_executable(secondarycluster
    src/secondarycluster.cc
    src/fileparser/PCsFileParser.cc
)

set_target_properties(secondarycluster PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

target_link_libraries(secondarycluster PRIVATE lib_secondarycluster)
if(OpenMP_CXX_FOUND)
    target_link_libraries(secondarycluster PRIVATE OpenMP::OpenMP_CXX)
    target_compile_options(secondarycluster PRIVATE ${OpenMP_CXX_FLAGS})
endif()

# Traceback
add_executable(traceback
    src/traceback.cc
    src/fileparser/PCsFileParser.cc
    src/common/sort.cc
)

# Postfilter
add_executable(postfilters
    src/postfilters.cc
)

# Tests ---------------------------------------------------------------

# Enable testing
enable_testing()

# Tests
add_subdirectory(tests)


