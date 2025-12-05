# Plugin Tests

This directory contains tests for Zattera plugins that compile into the `plugin_test` executable.

## Test Categories

- **json_rpc/** - JSON-RPC plugin tests
- **market_history/** - Market history plugin tests

## Running Tests

```bash
# Run all plugin tests
./tests/plugin_test

# Run specific test suite
./tests/plugin_test --run_test=json_rpc_tests
./tests/plugin_test --run_test=market_history_tests
```

## Adding New Plugin Tests

1. Create a new subdirectory for your plugin
2. Create a test file with naming convention: `<plugin_name>_test.cpp`
3. Add your file to `PLUGIN_TEST_SOURCES` in tests/CMakeLists.txt
