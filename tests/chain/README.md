# Core Blockchain Tests

This directory contains the core blockchain unit tests that compile into the `chain_test` executable.

## Test Categories

- **basic/** - Basic functionality tests (parsing, size checks, etc.)
- **block/** - Block validation and blockchain tests
- **operations/** - Operation unit tests organized by domain
- **serialization/** - Serialization and deserialization tests
- **database/** - Database and state management tests (undo/redo)
- **bmic/** - BMIC (Blockchain Managed Interest Credit) tests
- **live/** - Tests on live chain data and hardfork testing

## Running Tests

```bash
# Run all chain tests
./tests/chain_test

# Run specific test suite
./tests/chain_test --run_test=operation_tests
./tests/chain_test --run_test=block_tests
./tests/chain_test --run_test=basic_tests

# Run specific test case
./tests/chain_test -t operation_tests/account_create_validate
```

## Adding New Tests

1. Create your test file in the appropriate subdirectory
2. Use naming convention: `<domain>_test.cpp`
3. Include the database fixture: `#include "../fixtures/database_fixture.hpp"`
4. Add your file to `tests/CMakeLists.txt` in the `CHAIN_TEST_SOURCES` section

See [tests/README.md](../README.md) for more details.
