# Testing Guide

This document provides a comprehensive guide to testing in the Zattera blockchain codebase, including unit tests, fixtures, test utilities, and code coverage analysis.

## Overview

The Zattera test suite uses the [Boost.Test](https://www.boost.org/doc/libs/1_74_0/libs/test/doc/html/index.html) framework and is organized into two main test executables:

- **`chain_test`** - Core blockchain unit tests
- **`plugin_test`** - Plugin functionality tests

## Building Tests

### Prerequisites

Tests require the testnet build configuration:

```bash
cmake -DBUILD_ZATTERA_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) chain_test
make -j$(nproc) plugin_test
```

**Important**: The `BUILD_ZATTERA_TESTNET=ON` flag is **required** for building tests. Without it, the test executables will fail to build.

### Build Targets

**File**: [tests/CMakeLists.txt](../../tests/CMakeLists.txt)

- `chain_test` - Built from all `.cpp` files in `tests/chain/`
- `plugin_test` - Built from all `.cpp` files in `tests/plugin/`
- `database_fixture` - Shared test fixture library

## Running Tests

### Run All Tests

```bash
# Run all blockchain tests
./tests/chain_test

# Run all plugin tests
./tests/plugin_test
```

### Run Specific Test Suite

```bash
# Run a specific test suite
./tests/chain_test --run_test=basic_tests
./tests/chain_test --run_test=block_tests
./tests/chain_test --run_test=operation_tests
./tests/chain_test --run_test=operation_time_tests
./tests/chain_test --run_test=serialization_tests
```

### Run Single Test Case

```bash
# Run a specific test case within a suite
./tests/chain_test --run_test=basic_tests/parse_size_test
./tests/chain_test --run_test=basic_tests/valid_name_test
./tests/chain_test --run_test=operation_tests/account_create_apply
```

### Test Output Options

```bash
# Show detailed progress
./tests/chain_test --show_progress

# Set log level (all, test_suite, message, warning, error, cpp_exception, system_error, fatal_error, nothing)
./tests/chain_test --log_level=all
./tests/chain_test --log_level=message

# List all test cases without running them
./tests/chain_test --list_content

# Run tests with custom arguments
./tests/chain_test --record-assert-trip  # Record assertion trips for debugging
./tests/chain_test --show-test-names     # Print test names as they run
```

## Test Suites

### chain_test Suites

**File locations**: [tests/chain/](../../tests/chain/)

| Test Suite | File | Description |
|------------|------|-------------|
| `basic_tests` | [tests/chain/basic/](../../tests/chain/basic/) | Basic functionality tests (name validation, parsing, merkle trees) |
| `block_tests` | [tests/chain/block/](../../tests/chain/block/) | Blockchain and block validation tests |
| `live_tests` | [tests/chain/live/](../../tests/chain/live/) | Tests on live chain data (hardfork testing) |
| `operation_tests` | [tests/chain/operations/](../../tests/chain/operations/) | Unit tests for Zattera operations |
| `serialization_tests` | [tests/chain/serialization/](../../tests/chain/serialization/) | Serialization and deserialization tests |
| `database_tests` | [tests/chain/database/](../../tests/chain/database/) | Database undo/redo functionality tests |
| `bmic_tests` | [tests/chain/bmic/](../../tests/chain/bmic/) | BMIC (Block Metadata Index Cache) tests |

### plugin_test Suites

**File locations**: [tests/plugin/](../../tests/plugin/)

- [JSON-RPC Plugin Tests](../../tests/plugin/json_rpc/)
- [Market History Plugin Tests](../../tests/plugin/market_history/)

## Test Fixtures

Test fixtures provide a controlled database environment for tests. All fixtures are defined in [tests/fixtures/database_fixture.hpp](../../tests/fixtures/database_fixture.hpp).

### clean_database_fixture

The most commonly used fixture. Creates a fresh blockchain database for each test.

**File**: [tests/fixtures/database_fixture.cpp](../../tests/fixtures/database_fixture.cpp)

```cpp
BOOST_FIXTURE_TEST_SUITE( operation_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( account_create_apply )
{
    // Test code here - fresh database for this test
}

BOOST_AUTO_TEST_SUITE_END()
```

**Features**:
- Fresh database instance per test
- Genesis block with `genesis` account
- Hardfork set to latest version
- Initial witnesses created and funded
- Database validated after setup
- Automatic cleanup after test

**Setup process**:
1. Registers required plugins (account_history, debug_node, witness)
2. Initializes appbase application
3. Opens clean database
4. Generates genesis block
5. Sets hardfork to current version
6. Vests 10,000 TTR to genesis
7. Creates and funds additional witnesses

### live_database_fixture

Used for testing against an existing blockchain snapshot.

**File**: [tests/fixtures/database_fixture.cpp](../../tests/fixtures/database_fixture.cpp)

```cpp
BOOST_FIXTURE_TEST_SUITE( live_tests, live_database_fixture )

BOOST_AUTO_TEST_CASE( hardfork_test )
{
    // Test against real blockchain data
}

BOOST_AUTO_TEST_SUITE_END()
```

**Requirements**:
- Expects blockchain data in `./test_blockchain` directory
- Used primarily for hardfork testing on historical data

### json_rpc_database_fixture

Used for JSON-RPC API testing.

**File**: [tests/fixtures/database_fixture.hpp](../../tests/fixtures/database_fixture.hpp)

**Features**:
- `make_request()` - Make JSON-RPC request and validate response
- `make_array_request()` - Make batch JSON-RPC requests
- `make_positive_request()` - Make request expecting success

## Test Utilities and Macros

### Actor Macros

Create test accounts quickly:

```cpp
// Create a single actor
ACTOR(alice)  // Creates account "alice" with auto-generated keys
ACTOR(bob)

// Create multiple actors at once
ACTORS((alice)(bob)(carol))  // Creates all three accounts

// Get existing actor
GET_ACTOR(alice)  // Get reference to existing "alice" account
```

**Defined in**: [tests/fixtures/database_fixture.hpp](../../tests/fixtures/database_fixture.hpp)

**What `ACTOR(name)` does**:
1. Generates a private key from the name
2. Generates a posting key
3. Derives the public key
4. Creates the account with those keys
5. Stores account ID in `name_id` variable

### Asset Macro

```cpp
// Create assets from string
asset amount = ASSET("1.000 TTR");
asset sbd_amount = ASSET("10.000 TBD");
```

**Defined in**: [tests/fixtures/database_fixture.hpp](../../tests/fixtures/database_fixture.hpp)

### Operation Validation Macros

Test operation validation without affecting database state:

```cpp
account_create_operation op;
op.creator = "alice";
op.new_account_name = "bob";

// Test that a field value passes validation
REQUIRE_OP_VALIDATION_SUCCESS( op, fee, ASSET("0.100 TTR") );

// Test that a field value fails validation
REQUIRE_OP_VALIDATION_FAILURE( op, new_account_name, "ab" );  // Too short
REQUIRE_OP_VALIDATION_FAILURE( op, new_account_name, "" );    // Empty
```

**Defined in**: [tests/fixtures/database_fixture.hpp](../../tests/fixtures/database_fixture.hpp)

### Exception Testing Macros

```cpp
// Require that expression throws specific exception
ZATTERA_REQUIRE_THROW( db->push_transaction(tx, 0), fc::exception );

// Require throw with custom value
REQUIRE_THROW_WITH_VALUE( op, fee, ASSET("99999999.999 TTR") );
```

### Transaction Macros

```cpp
transfer_operation op;
op.from = "alice";
op.to = "bob";
op.amount = ASSET("1.000 TTR");

// Push operation to blockchain
PUSH_OP(op, alice_private_key);

// Push operation twice (testing idempotency)
PUSH_OP_TWICE(op, alice_private_key);

// Expect operation to fail
FAIL_WITH_OP(op, alice_private_key, fc::assert_exception);
```

**Defined in**: [tests/fixtures/database_fixture.hpp](../../tests/fixtures/database_fixture.hpp)

## Database Fixture Helper Functions

The `database_fixture` class provides many helper functions for test setup.

**File**: [tests/fixtures/database_fixture.hpp](../../tests/fixtures/database_fixture.hpp)

### Account Management

```cpp
// Create account with specific parameters
const account_object& alice = account_create(
    "alice",                          // name
    "genesis",                      // creator
    init_account_priv_key,           // creator key
    ASSET("0.100 TTR").amount,     // fee
    alice_public_key,                // owner/active key
    alice_post_key.get_public_key(), // posting key
    "{}"                             // json_metadata
);

// Create account with default parameters
const account_object& bob = account_create("bob", bob_public_key);
```

### Funding

```cpp
// Fund account with default amount (500000 satoshis)
fund("alice");

// Fund with specific amount
fund("alice", 1000000);

// Fund with asset
fund("alice", ASSET("100.000 TTR"));
```

### Vesting

```cpp
// Vest ZTR to get VESTS
vest("alice", 10000);
vest("alice", ASSET("100.000 TTR"));
```

### Transfer and Convert

```cpp
// Transfer between accounts
transfer("alice", "bob", ASSET("10.000 TTR"));

// Convert ZTR to ZBD
convert("alice", ASSET("100.000 TTR"));
```

### Witness Operations

```cpp
// Create witness
const witness_object& wit = witness_create(
    "alice",                    // owner
    alice_private_key,          // owner key
    "https://alice.com",        // url
    alice_public_key,           // signing key
    ASSET("0.000 TTR").amount // fee
);

// Set price feed
set_price_feed( price( ASSET("1.000 TBD"), ASSET("1.000 TTR") ) );

// Set witness properties
flat_map<string, vector<char>> props;
set_witness_props(props);
```

### Block Generation

```cpp
// Generate a single block
generate_block();

// Generate specific number of blocks
generate_blocks(100);  // Generate 100 blocks

// Generate blocks until timestamp
generate_blocks(db->head_block_time() + fc::seconds(60));
```

### Database Validation

```cpp
// Validate database invariants
validate_database();  // Called automatically by ACTORS() macro
```

## Writing Tests

### Basic Test Structure

```cpp
BOOST_FIXTURE_TEST_SUITE( operation_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( transfer_operation_test )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: transfer_operation" );

        // Setup
        ACTORS((alice)(bob))
        fund("alice", ASSET("100.000 TTR"));

        // Execute
        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = ASSET("10.000 TTR");

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.set_expiration(db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db->get_chain_id());
        db->push_transaction(tx, 0);

        // Verify
        BOOST_REQUIRE(get_balance("alice") == ASSET("90.000 TTR"));
        BOOST_REQUIRE(get_balance("bob") == ASSET("10.000 TTR"));

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

### Test Best Practices

1. **Always wrap tests in try-catch**: Use `FC_LOG_AND_RETHROW()` to get detailed error information
2. **Use descriptive test names**: Name should clearly indicate what is being tested
3. **Add test messages**: Use `BOOST_TEST_MESSAGE()` to document test steps
4. **Validate database after changes**: Call `validate_database()` to ensure consistency
5. **Test both success and failure cases**: Verify operations fail when they should
6. **Use macros for common patterns**: Leverage `ACTORS()`, `PUSH_OP()`, etc.
7. **Keep tests focused**: Each test should verify one specific behavior
8. **Clean up properly**: Fixtures handle cleanup, but be aware of state

### Example: Testing Operation Validation

```cpp
BOOST_AUTO_TEST_CASE( account_create_validate )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_create validation" );

        account_create_operation op;
        op.creator = "alice";
        op.fee = ASSET("0.100 TTR");

        // Test valid names
        REQUIRE_OP_VALIDATION_SUCCESS(op, new_account_name, "validname");
        REQUIRE_OP_VALIDATION_SUCCESS(op, new_account_name, "valid-name");
        REQUIRE_OP_VALIDATION_SUCCESS(op, new_account_name, "valid.name");

        // Test invalid names
        REQUIRE_OP_VALIDATION_FAILURE(op, new_account_name, "a");      // Too short
        REQUIRE_OP_VALIDATION_FAILURE(op, new_account_name, "ab");     // Too short
        REQUIRE_OP_VALIDATION_FAILURE(op, new_account_name, "INVALID"); // Uppercase
        REQUIRE_OP_VALIDATION_FAILURE(op, new_account_name, "-invalid"); // Starts with dash
    }
    FC_LOG_AND_RETHROW()
}
```

### Example: Testing Operation Application

```cpp
BOOST_AUTO_TEST_CASE( account_create_apply )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: account_create application" );

        set_price_feed(price(ASSET("1.000 TBD"), ASSET("1.000 TTR")));

        ACTOR(alice)
        fund("alice", ASSET("10.000 TTR"));

        private_key_type bob_key = generate_private_key("bob");

        account_create_operation op;
        op.creator = "alice";
        op.new_account_name = "bob";
        op.fee = ASSET("0.100 TTR");
        op.owner = authority(1, bob_key.get_public_key(), 1);
        op.active = authority(1, bob_key.get_public_key(), 1);
        op.posting = authority(1, bob_key.get_public_key(), 1);
        op.memo_key = bob_key.get_public_key();
        op.json_metadata = "{}";

        PUSH_OP(op, alice_private_key);

        const account_object& bob = db->get_account("bob");
        BOOST_REQUIRE(bob.name == "bob");
        BOOST_REQUIRE(bob.memo_key == bob_key.get_public_key());
        BOOST_REQUIRE(bob.created == db->head_block_time());

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}
```

### Example: Testing Authorities

```cpp
BOOST_AUTO_TEST_CASE( transfer_authorities )
{
    try
    {
        BOOST_TEST_MESSAGE( "Testing: transfer authorities" );

        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = ASSET("1.000 TTR");

        flat_set<account_name_type> auths;
        flat_set<account_name_type> expected;

        // Owner authority not required
        op.get_required_owner_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        // Active authority required
        expected.insert("alice");
        auths.clear();
        op.get_required_active_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        // Posting authority not required
        expected.clear();
        auths.clear();
        op.get_required_posting_authorities(auths);
        BOOST_REQUIRE(auths == expected);
    }
    FC_LOG_AND_RETHROW()
}
```

## Code Coverage Testing

Code coverage analysis shows which parts of the code are executed during tests.

### Install lcov

```bash
# macOS
brew install lcov

# Ubuntu/Debian
sudo apt-get install lcov
```

### Generate Coverage Report

```bash
# 1. Configure build with coverage enabled
cmake -DBUILD_ZATTERA_TESTNET=ON \
      -DENABLE_COVERAGE_TESTING=ON \
      -DCMAKE_BUILD_TYPE=Debug \
      ..

# 2. Build tests
make -j$(nproc)

# 3. Capture baseline coverage
lcov --capture --initial --directory . --output-file base.info --no-external

# 4. Run tests
./tests/chain_test

# 5. Capture test coverage
lcov --capture --directory . --output-file test.info --no-external

# 6. Combine baseline and test coverage
lcov --add-tracefile base.info --add-tracefile test.info --output-file total.info

# 7. Remove test code from coverage report
lcov -o interesting.info -r total.info 'tests/*'

# 8. Generate HTML report
mkdir -p lcov
genhtml interesting.info --output-directory lcov --prefix $(pwd)

# 9. Open report in browser
open lcov/index.html  # macOS
xdg-open lcov/index.html  # Linux
```

### Coverage Report Details

The HTML report shows:
- **Line coverage**: Percentage of lines executed
- **Function coverage**: Percentage of functions called
- **Branch coverage**: Percentage of conditional branches taken
- Color-coded source files (green = covered, red = not covered)

### Interpreting Coverage

- **High coverage (>80%)**: Good test coverage
- **Medium coverage (50-80%)**: Some gaps in testing
- **Low coverage (<50%)**: Significant testing needed

**Note**: 100% coverage doesn't guarantee bug-free code, but low coverage indicates untested code paths.

## Running Specific Test Scenarios

### Test with Different Skip Flags

```cpp
// Skip all validation checks
db->push_transaction(tx, ~0);

// Skip specific checks
db->push_transaction(tx, database::skip_transaction_signatures);
db->push_transaction(tx, database::skip_authority_check);
db->push_transaction(tx, database::skip_transaction_dupe_check);
```

### Test Time-Based Operations

```cpp
// Set up vesting withdrawal
withdraw_vesting_operation op;
op.account = "alice";
op.vesting_shares = ASSET("1000.000000 VESTS");
PUSH_OP(op, alice_private_key);

// Generate blocks to advance time
generate_blocks(db->head_block_time() + fc::days(7));

// Verify withdrawal executed
BOOST_REQUIRE(get_balance("alice").amount > 0);
```

### Test Hardfork Behavior

```cpp
// Test behavior before hardfork
db->set_hardfork(18);
// ... test code ...

// Test behavior after hardfork
db->set_hardfork(19);
// ... test code ...
```

## Plugin Tests

Plugin tests verify plugin functionality and APIs.

### Example: Market History Plugin Test

```cpp
BOOST_FIXTURE_TEST_SUITE( market_history_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( mh_test )
{
    try
    {
        // Setup market trades
        ACTORS((alice)(bob))
        fund("alice", ASSET("1000.000 TTR"));
        convert("alice", ASSET("100.000 TTR"));

        generate_blocks(db->head_block_time() + fc::hours(1));

        // Query market history API
        // Verify historical data recorded correctly
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
```

## Debugging Tests

### Enable Debug Output

```bash
# Run with verbose logging
./tests/chain_test --log_level=all --run_test=operation_tests/account_create_apply

# Record assertion trips
./tests/chain_test --record-assert-trip

# Show test names as they run
./tests/chain_test --show-test-names
```

### Debug Test in GDB

```bash
# Run test in debugger
gdb --args ./tests/chain_test --run_test=operation_tests/account_create_apply

# Inside GDB
(gdb) break database_fixture.cpp:100  # Set breakpoint
(gdb) run                              # Run test
(gdb) bt                               # Show backtrace when stopped
```

### Common Test Failures

1. **Database validation failures**: Usually indicate state inconsistency
2. **Assertion failures**: Check operation validation and authorization
3. **Insufficient funds**: Ensure accounts are properly funded before operations
4. **Authority failures**: Verify correct keys are used for signing
5. **Hardfork issues**: Check if operation is available at current hardfork

## Continuous Integration

Tests should be run:
- Before committing changes
- As part of pull request validation
- On main branch after merges

Recommended CI workflow:
```bash
mkdir build && cd build
cmake -DBUILD_ZATTERA_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) chain_test plugin_test
./tests/chain_test
./tests/plugin_test
```

## Directory Structure

```
tests/
├── CMakeLists.txt                 # Test build configuration
├── fixtures/                      # Test fixtures and utilities
│   ├── database_fixture.hpp       # Fixture class definitions
│   └── database_fixture.cpp       # Fixture implementations
├── chain/                         # Blockchain unit tests → chain_test
│   ├── main.cpp                   # Test runner entry point
│   ├── basic/                     # Basic functionality tests
│   ├── block/                     # Block validation tests
│   ├── operations/                # Operation unit tests
│   ├── serialization/             # Serialization tests
│   ├── database/                  # Database and undo/redo tests
│   ├── bmic/                      # BMIC tests
│   └── live/                      # Live blockchain tests
├── plugin/                        # Plugin-specific tests → plugin_test
│   ├── json_rpc/                  # JSON-RPC plugin tests
│   └── market_history/            # Market history plugin tests
├── helpers/                       # Test helper utilities
│   ├── bmic/                      # BMIC test helpers
│   └── undo/                      # Undo test utilities
├── integration/                   # Integration tests
│   ├── smoke/                     # Node upgrade regression tests
│   └── api/                       # API response validation tests
└── scripts/                       # Test automation scripts
```

## Additional Resources

- [Plugin Development Guide](plugin.md) - Plugin development and API design
- [Building Guide](../getting-started/building.md) - Build instructions for all platforms
- [Quick Start Guide](../getting-started/quick-start.md) - Get started quickly with Docker
- [Node Modes Guide](../operations/node-modes-guide.md) - Different node types and configurations
- [Boost.Test Documentation](https://www.boost.org/doc/libs/1_74_0/libs/test/doc/html/index.html) - Boost.Test framework reference

## Summary

- Use `clean_database_fixture` for most tests
- Leverage helper macros (`ACTORS`, `PUSH_OP`, etc.) for cleaner test code
- Test both validation and application of operations
- Always validate database after state changes
- Run code coverage to identify untested code
- Use descriptive test names and messages
- Wrap tests in try-catch with `FC_LOG_AND_RETHROW()`
