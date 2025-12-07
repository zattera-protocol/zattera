# Zattera Test Suite

## Directory Structure

```
tests/
├── chain/              # Core blockchain tests → chain_test executable
│   ├── basic/          # Basic functionality tests
│   ├── block/          # Block validation tests
│   ├── operations/     # Operation tests (account, comment, vesting, etc.)
│   ├── serialization/  # Serialization tests
│   ├── database/       # Database and state management tests
│   ├── bmic/           # BMIC tests
│   └── live/           # Live chain tests (hardforks, mainnet data)
│
├── plugin/             # Plugin tests → plugin_test executable
│   ├── json_rpc/       # JSON-RPC plugin tests
│   └── market_history/ # Market history plugin tests
│
├── fixtures/           # Test fixtures (database_fixture, etc.)
├── helpers/            # Test helper utilities
│   ├── bmic/           # BMIC test helpers
│   └── undo/           # Undo test utilities
│
├── integration/        # Integration tests
│   ├── smoke/          # Node upgrade regression tests
│   └── api/            # API response validation tests
│
└── scripts/            # Test automation scripts
```

## Running Tests Locally

### Build Tests

```bash
cd build
cmake -DBUILD_ZATTERA_TEST_MODE=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) chain_test plugin_test
```

**Note:** Building tests requires `-DBUILD_ZATTERA_TEST_MODE=ON` flag. Without this flag, tests will fail to build.

### Run All Tests

```bash
# Run all chain tests
./tests/chain_test

# Run all plugin tests
./tests/plugin_test
```

### Run Specific Test Suites

```bash
# Run specific test suite
./tests/chain_test -t operation_tests
./tests/chain_test -t block_tests
./tests/chain_test -t basic_tests

# Run specific test case
./tests/chain_test -t operation_tests/account_create_validate
./tests/chain_test -t basic_tests/parse_size_test

# Verbose output
./tests/chain_test --log_level=all
./tests/chain_test --show_progress

# List all tests
./tests/chain_test --list_content
```

### Available Test Suites

**chain_test executable:**
- `basic_tests` - Basic functionality tests
- `block_tests` - Block validation tests
- `operation_tests` - Operation unit tests (account, comment, transfer, vesting, witness, custom, market, reward)
- `serialization_tests` - Serialization tests
- `database_tests` - Database and undo/redo tests
- `bmic_tests` - BMIC tests
- `live_tests` - Live chain tests (hardforks, mainnet data)

**plugin_test executable:**
- `json_rpc_tests` - JSON-RPC plugin tests
- `market_history_tests` - Market history plugin tests

## Running Integration Tests

### Smoke Tests

Smoke tests validate node upgrade and regression scenarios by comparing a test node against a reference node.

#### What is a Reference Node?

The smoke test compares **two zatterad nodes** to perform regression testing:

1. **Test Node** - The new version of zatterad you want to test
2. **Reference Node** - A verified/stable version of zatterad used as comparison baseline (e.g., previous stable release)

This ensures that:
- New code changes don't break existing behavior (regression prevention)
- API responses remain consistent between versions (API compatibility)
- Node upgrades are safe (upgrade validation)

#### Preparing Reference Node

To prepare a reference node for comparison:

```bash
# 1. Clone repository to separate directory
git clone https://github.com/zattera-protocol/zattera.git ~/zattera-ref
cd ~/zattera-ref

# 2. Check out stable/verified version (tag, branch, or commit)
git checkout v0.23.0  # Or any verified commit/tag

# 3. Build reference binary
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) zatterad
```

Now you have a reference binary at `~/zattera-ref/build/programs/zatterad/zatterad`.

#### Running Smoke Tests

From the `tests/integration/smoke/` directory:

```bash
# Run smoke test
./smoketest.sh <test_zatterad_path> <ref_zatterad_path> <test_blockchain_dir> <ref_blockchain_dir> <block_limit> <jobs>

# Example:
./smoketest.sh ~/zattera/build/programs/zatterad/zatterad \
               ~/zattera-ref/build/programs/zatterad/zatterad \
               ~/test_blockchain \
               ~/ref_blockchain \
               3000000 \
               12
```

**Parameters:**
- `test_zatterad_path` - Path to the zatterad binary being tested (new version)
- `ref_zatterad_path` - Path to the reference zatterad binary (stable version)
- `test_blockchain_dir` - Working directory for test node
- `ref_blockchain_dir` - Working directory for reference node
- `block_limit` - Maximum block number to process
- `jobs` - Number of parallel jobs

The test will run both nodes, send identical API requests to each, and compare their responses. Any differences are logged for investigation.

See [integration/smoke/README.md](integration/smoke/README.md) for more details.

## Running Tests with Docker

### Build Docker Image

From the root of the repository:

```bash
docker build -t zatterahub/zattera:latest -f Dockerfile .
```

### Run Tests in Docker

```bash
# Run chain tests
docker run --rm zatterahub/zattera:latest /usr/local/zatterad-test/bin/tests/chain_test

# Run plugin tests
docker run --rm zatterahub/zattera:latest /usr/local/zatterad-test/bin/tests/plugin_test

# Run specific test suite
docker run --rm zatterahub/zattera:latest /usr/local/zatterad-test/bin/tests/chain_test -t operation_tests
```

### Interactive Docker Troubleshooting

```bash
# Run interactive shell in Docker container
docker run -ti --rm zatterahub/zattera:latest /bin/bash

# Inside container, you can manually run tests:
cd /usr/local/zatterad-test/bin
./tests/chain_test
./tests/plugin_test
```

## Test Development

### Adding New Tests

1. **For chain tests:** Add test file to appropriate subdirectory under `tests/chain/`
2. **For plugin tests:** Add test file to appropriate subdirectory under `tests/plugin/`
3. **Update CMakeLists.txt:** Add new test source file to `CHAIN_TEST_SOURCES` or `PLUGIN_TEST_SOURCES`
4. **Use fixtures:** Include test fixtures from `tests/fixtures/` for common setup
5. **Write tests using Boost.Test framework**

Example test structure:

```cpp
#include <boost/test/unit_test.hpp>
#include "fixtures/database_fixture.hpp"

BOOST_FIXTURE_TEST_SUITE(my_test_suite, clean_database_fixture)

BOOST_AUTO_TEST_CASE(my_test_case)
{
    // Test implementation
}

BOOST_AUTO_TEST_SUITE_END()
```

### Test Utilities

- **database_fixture:** Provides clean database environment for each test
- **bmic helpers:** Utilities for BMIC testing
- **undo helpers:** Utilities for testing undo/redo functionality

See test source files in `tests/chain/` and `tests/plugin/` for examples.

## Code Coverage

To generate code coverage reports:

```bash
cd build
cmake -DBUILD_ZATTERA_TEST_MODE=ON -DENABLE_COVERAGE_TESTING=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Capture baseline
lcov --capture --initial --directory . --output-file base.info --no-external

# Run tests
./tests/chain_test
./tests/plugin_test

# Capture test coverage
lcov --capture --directory . --output-file test.info --no-external

# Combine coverage data
lcov --add-tracefile base.info --add-tracefile test.info --output-file total.info

# Remove test files from coverage
lcov -o interesting.info -r total.info 'tests/*'

# Generate HTML report
mkdir -p lcov
genhtml interesting.info --output-directory lcov --prefix `pwd`

# Open lcov/index.html in browser
```

## Continuous Integration

Tests are automatically run in CI on every commit and pull request. Make sure all tests pass before submitting pull requests.

## Troubleshooting

### Common Issues

1. **Tests fail to build:** Make sure you used `-DBUILD_ZATTERA_TEST_MODE=ON` when running cmake
2. **Tests timeout:** Increase timeout or run on faster hardware
3. **Memory issues:** Tests require significant RAM; ensure at least 8GB available
4. **Parallel test failures:** Some tests may have race conditions; try running sequentially

### Getting Help

- Check test output for detailed error messages
- Review test source code in `tests/chain/` or `tests/plugin/`
- Run tests with `--log_level=all` for verbose output
- Check CI logs for comparison with local results
