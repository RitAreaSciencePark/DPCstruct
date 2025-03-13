set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/util)

# Add executable for plddt_json_parser
add_executable(plddt_json_parser
    plddt_json_parser.cc
)

set_target_properties(plddt_json_parser PROPERTIES
    OUTPUT_NAME plddt_json_parser
)

add_custom_target(copy_util_script ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/util/download_plddts.sh ${CMAKE_BINARY_DIR}/util/
)