cmake_minimum_required(VERSION 3.19)
project(dpcstruct)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
)

# Set the output directories 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)  
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_TESTS ${CMAKE_BINARY_DIR}/tests)  
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)   

option(BUILD_TESTING "Build the tests" OFF)

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
add_executable(dpcstruct-prefilters
    src/prefilters.cc
    src/fileparser/AlnsFileParser.cc
)

set_target_properties(dpcstruct-prefilters PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

target_link_libraries(dpcstruct-prefilters PRIVATE memorymapped)
if(OpenMP_CXX_FOUND)
    target_link_libraries(dpcstruct-prefilters PRIVATE OpenMP::OpenMP_CXX)
    target_compile_options(dpcstruct-prefilters PRIVATE ${OpenMP_CXX_FLAGS})
endif()

# Primary cluster
add_executable(dpcstruct-primarycluster
    src/primarycluster.cc
    src/fileparser/AlnsFileParser.cc
)

set_target_properties(dpcstruct-primarycluster PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

target_link_libraries(dpcstruct-primarycluster PRIVATE lib_primarycluster)
target_link_libraries(dpcstruct-primarycluster PRIVATE memorymapped)
if(OpenMP_CXX_FOUND)
    target_link_libraries(dpcstruct-primarycluster PRIVATE OpenMP::OpenMP_CXX)
    target_compile_options(dpcstruct-primarycluster PRIVATE ${OpenMP_CXX_FLAGS})
endif()

# Secondary cluster
add_executable(dpcstruct-secondarycluster
    src/secondarycluster.cc
    src/fileparser/PCsFileParser.cc
)

set_target_properties(dpcstruct-secondarycluster PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

target_link_libraries(dpcstruct-secondarycluster PRIVATE lib_secondarycluster)
if(OpenMP_CXX_FOUND)
    target_link_libraries(dpcstruct-secondarycluster PRIVATE OpenMP::OpenMP_CXX)
    target_compile_options(dpcstruct-secondarycluster PRIVATE ${OpenMP_CXX_FLAGS})
endif()

# Traceback
add_executable(dpcstruct-traceback
    src/traceback.cc
    src/fileparser/PCsFileParser.cc
    src/common/sort.cc
)

# Postfilter
add_executable(dpcstruct-postfilters
    src/postfilters.cc
)

install(TARGETS dpcstruct-prefilters dpcstruct-primarycluster dpcstruct-secondarycluster dpcstruct-traceback dpcstruct-postfilters
        DESTINATION bin
)

# Copy the wrapper to the build/bin directory
add_custom_command(TARGET dpcstruct-traceback POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/scripts/dpcstruct.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/dpcstruct
)

# Install the dpcstruct wrapper script
install(PROGRAMS ${CMAKE_SOURCE_DIR}/scripts/dpcstruct.sh
    DESTINATION bin
    RENAME dpcstruct
)


# Tests ---------------------------------------------------------------

# add condition for building tests
if (BUILD_TESTING)
    find_package(Catch2 3 REQUIRED)
    include(Catch)
    enable_testing()
    add_subdirectory(tests)
endif()




