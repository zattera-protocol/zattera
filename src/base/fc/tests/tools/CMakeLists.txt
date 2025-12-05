# FC Manual Testing Tools
# These are manual testing and development utilities, not part of automated test suite

add_executable( ecc_interop_test ecc_interop_test_tool.cpp )
target_link_libraries( ecc_interop_test fc )

add_executable( log_test log_test_tool.cpp )
target_link_libraries( log_test fc )

add_executable( saturation_exhaustive_test saturation_exhaustive_test_tool.cpp )
target_link_libraries( saturation_exhaustive_test fc )

add_executable( sha_test sha_test_tool.cpp )
target_link_libraries( sha_test fc )

add_executable( websocket_api_test websocket_api_test_tool.cpp )
target_link_libraries( websocket_api_test fc )

add_executable( file_mutex_test file_mutex_test_tool.cpp )
target_link_libraries( file_mutex_test fc )

add_executable( thread_sleep_test thread_sleep_test_tool.cpp )
target_link_libraries( thread_sleep_test fc )

add_executable( rate_limiting_test rate_limiting_test_tool.cpp )
target_link_libraries( rate_limiting_test fc )
