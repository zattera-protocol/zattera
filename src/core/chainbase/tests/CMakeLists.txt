add_definitions( -DFC_TEST_ROOT_DIR="${CMAKE_SOURCE_DIR}" )

add_executable( chainbase_test main.cpp database_test.cpp )
target_link_libraries( chainbase_test chainbase )
