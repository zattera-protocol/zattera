add_definitions( -DFC_TEST_ROOT_DIR="${CMAKE_SOURCE_DIR}" )

add_executable( fc_test
   main.cpp
   compress/compress_test.cpp
   crypto/aes_test.cpp
   crypto/base_n_test.cpp
   crypto/bigint_test.cpp
   crypto/ecc_blind_test.cpp
   crypto/blowfish_test.cpp
   crypto/hmac_test.cpp
   crypto/rand_test.cpp
   crypto/sha_test.cpp
   network/ntp_test.cpp
   network/http/websocket_test.cpp
   thread/task_cancel_test.cpp
   thread/thread_test.cpp
   bloom_test.cpp
   real128_test.cpp
   saturation_test.cpp
   utf8_test.cpp
)
target_link_libraries( fc_test fc )

# Manual testing tools
add_subdirectory( tools )
