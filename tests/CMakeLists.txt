# Subdirectories for test support libraries
add_subdirectory(fixtures)
add_subdirectory(helpers)

# Gperftools (optional)
find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling tests with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

# ============================================================
# Core Blockchain Tests (chain_test executable)
# ============================================================
set( CHAIN_TEST_SOURCES
    # Main entry point
    chain/main.cpp

    # Basic functionality tests
    chain/basic/basic_test.cpp

    # Block validation tests
    chain/block/block_test.cpp

    # Operation tests
    chain/operations/account_test.cpp
    chain/operations/comment_test.cpp
    chain/operations/custom_test.cpp
    chain/operations/market_test.cpp
    chain/operations/reward_test.cpp
    chain/operations/transfer_test.cpp
    chain/operations/vesting_test.cpp
    chain/operations/witness_test.cpp

    # Serialization tests
    chain/serialization/serialization_test.cpp

    # Database and state management tests
    chain/database/undo_test.cpp

    # BMIC tests
    chain/bmic/bmic_test.cpp

    # Live chain tests (hardforks, mainnet data)
    chain/live/live_test.cpp
)

add_executable( chain_test ${CHAIN_TEST_SOURCES} )
target_link_libraries( chain_test
    fixtures              # Renamed from db_fixture
    bmic_helpers          # BMIC test helpers
    chainbase
    zattera_chain
    zattera_protocol
    account_history_plugin
    market_history_plugin
    witness_plugin
    debug_node_plugin
    fc
    ${PLATFORM_SPECIFIC_LIBS}
)

# ============================================================
# Plugin Tests (plugin_test executable)
# ============================================================
set( PLUGIN_TEST_SOURCES
    plugin/main.cpp
    plugin/json_rpc/json_rpc_test.cpp
    plugin/market_history/market_history_test.cpp
)

add_executable( plugin_test ${PLUGIN_TEST_SOURCES} )
target_link_libraries( plugin_test
    fixtures              # Renamed from db_fixture
    zattera_chain
    zattera_protocol
    account_history_plugin
    market_history_plugin
    witness_plugin
    debug_node_plugin
    fc
    ${PLATFORM_SPECIFIC_LIBS}
)

# ============================================================
# MSVC-specific compilation flags
# ============================================================
if(MSVC)
    # Large object files for serialization tests
    set_source_files_properties(
        chain/serialization/serialization_test.cpp
        PROPERTIES COMPILE_FLAGS "/bigobj"
    )
endif(MSVC)
