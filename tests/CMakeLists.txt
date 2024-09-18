# Set the output directory for test executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/bin)

# Test 1: test_main
add_executable(test_main test_main.cc)
target_link_libraries(test_main PRIVATE Catch2::Catch2WithMain)
# target_link_libraries(test_main PRIVATE
#     ${Catch2_SOURCE_DIR}/lib/libCatch2Main.a
#     ${Catch2_SOURCE_DIR}/lib/libCatch2.a
# )

# Test 2: test_density
add_executable(test_density test_density.cc)
target_link_libraries(test_density PRIVATE Catch2::Catch2WithMain lib_primarycluster)
# target_link_libraries(test_density PRIVATE
#     ${Catch2_SOURCE_DIR}/lib/libCatch2Main.a
#     ${Catch2_SOURCE_DIR}/lib/libCatch2.a
#     lib_primarycluster
# )

# Test 3: test_delta
add_executable(test_delta test_delta.cc)
target_link_libraries(test_delta PRIVATE Catch2::Catch2WithMain lib_primarycluster)
# target_link_libraries(test_delta PRIVATE
#     ${Catch2_SOURCE_DIR}/lib/libCatch2Main.a
#     ${Catch2_SOURCE_DIR}/lib/libCatch2.a
#     lib_primarycluster
# )

# Test 4: test_peaks
add_executable(test_peaks test_peaks.cc)
target_link_libraries(test_peaks PRIVATE Catch2::Catch2WithMain lib_primarycluster)
# target_link_libraries(test_peaks PRIVATE
#     ${Catch2_SOURCE_DIR}/lib/libCatch2Main.a
#     ${Catch2_SOURCE_DIR}/lib/libCatch2.a
#     lib_primarycluster
# )


# Set output directory for all test executables and object files
set_target_properties(test_main test_density test_delta test_peaks
    PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/bin   # Test executables
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/lib   # For shared libraries, if any
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/lib   # For static libraries or object files
)

# Discover tests with Catch2
catch_discover_tests(test_main)
catch_discover_tests(test_density)
catch_discover_tests(test_delta)
catch_discover_tests(test_peaks)
