# Smoke Test Suite Documentation

## Overview

The smoke test suite performs integration testing by comparing a tested version of `zatterad` against a reference version. Tests replay blockchain data and verify that both instances produce identical API responses.

## Architecture

The smoke test framework consists of:

- **`smoketest.sh`** - Main orchestrator that runs all test groups
- **Test groups** - Individual test suites in subdirectories (e.g., `account_history/`, `votes/`)
- **Scripts** - Helper scripts in `scripts/` directory:
  - `run_replay.sh` - Replays blockchain up to specified block
  - `open_node.sh` - Starts zatterad node and verifies it's listening
- **Docker** - Containerized test environment via `Dockerfile` and `docker_build_and_run.sh`

## Running Smoke Tests

### Prerequisites

You need two zatterad instances and their respective blockchain data directories:
- **Tested instance** - The version being tested
- **Reference instance** - The known-good reference version

### Directory Structure

Setup a test workspace with the following structure:

```
~/zattera-test/
├── tested/
│   ├── build/programs/zatterad/zatterad    # Tested binary
│   └── data/
│       └── blockchain/block_log             # Blockchain data
└── reference/
    ├── build/programs/zatterad/zatterad    # Reference binary
    └── data/
        └── blockchain/block_log             # Blockchain data
```

### Native Execution

From the `tests/integration/smoke/` directory:

```bash
./smoketest.sh \
  ~/zattera-test/tested/build/programs/zatterad/zatterad \
  ~/zattera-test/reference/build/programs/zatterad/zatterad \
  ~/zattera-test/tested/data \
  ~/zattera-test/reference/data \
  5000000 \
  12
```

**Parameters:**
1. `tested_zatterad_path` - Path to tested zatterad binary
2. `reference_zatterad_path` - Path to reference zatterad binary
3. `tested_data_dir` - Directory containing tested blockchain data
4. `reference_data_dir` - Directory containing reference blockchain data
5. `number_of_blocks_to_replay` - Block height to replay up to
6. `number_of_jobs` (optional) - Parallel jobs to run (defaults to `nproc`)
7. `--dont-copy-config` (optional) - Skip copying config.ini files

**Important:** Always use absolute paths, not relative paths.

### Docker Execution

Build and run via Docker:

```bash
sudo ./docker_build_and_run.sh \
  ~/zattera-test/tested/build/programs/zatterad/zatterad \
  ~/zattera-test/reference/build/programs/zatterad/zatterad \
  ~/zattera-test/tested/data \
  ~/zattera-test/reference/data \
  ~/zattera-test/logs \
  5000000 \
  12
```

**Parameters:**
1. `tested_zatterad_path` - Path to tested zatterad binary
2. `reference_zatterad_path` - Path to reference zatterad binary
3. `tested_data_dir` - Directory containing tested blockchain data
4. `reference_data_dir` - Directory containing reference blockchain data
5. `logs_dir` - Directory where test logs will be saved
6. `stop_replay_at_block` - Block height to replay up to
7. `jobs` (optional) - Parallel jobs to run (defaults to `nproc`)
8. `--dont-copy-config` (optional) - Skip copying config.ini files

## Test Execution Flow

For each test group, the smoke test framework:

1. **Replays blockchain** - Both tested and reference instances replay blocks up to the specified limit
2. **Starts nodes** - Launches both instances with API endpoints:
   - Tested instance: `0.0.0.0:8090`
   - Reference instance: `0.0.0.0:8091`
3. **Runs API tests** - Python scripts send identical API requests to both nodes
4. **Compares responses** - Verifies that responses are identical
5. **Logs differences** - Any mismatches are logged to the test group's `logs/` directory

## Test Groups

Current test groups in the suite:

- **`account_history/`** - Tests account history API endpoints
- **`ah_limit_account/`** - Tests account history with account limits
- **`ah_limit_account_operation/`** - Tests account history with operation limits
- **`ah_limit_operations/`** - Tests account history operation limits
- **`votes/`** - Tests vote listing APIs

Each test group contains:
- `test_group.sh` - Test execution script
- `config.ini` or `config_ref.ini`/`config_test.ini` - Configuration for zatterad instances
- Python test scripts (referenced from `../../api_tests/`)

## Output and Logs

### Success
When tests pass, you'll see:
```
test group account_history PASSED
TOTAL test groups: 5
PASSED test groups: 5
SKIPPED test groups: 0
FAILED test groups: 0
```

### Failure
When tests fail, JSON response files are generated in the test group's subdirectory showing:
- The API request that was sent
- The response from the tested instance
- The response from the reference instance
- The difference between them

## Adding a New Test Group

To add a new test group:

1. **Create subdirectory** in `tests/integration/smoke/`:
   ```bash
   mkdir my_new_test
   ```

2. **Create `test_group.sh`** with this structure:
   ```bash
   #!/bin/bash

   # Parameter handling
   JOBS=$1
   TEST_ZATTERAD_PATH=$2
   REF_ZATTERAD_PATH=$3
   TEST_DATA_DIR=$4
   REF_DATA_DIR=$5
   BLOCK_LIMIT=$6
   COPY_CONFIG=$7

   # Import shared scripts
   SCRIPT_DIR=../scripts

   # Run replay
   $SCRIPT_DIR/run_replay.sh $TEST_ZATTERAD_PATH $REF_ZATTERAD_PATH \
     $TEST_DATA_DIR $REF_DATA_DIR $BLOCK_LIMIT $COPY_CONFIG

   # Start nodes on ports 8090 and 8091
   # Run your Python test scripts
   # Handle cleanup
   ```

3. **Create config.ini** (or `config_ref.ini` and `config_test.ini`) with required plugin configuration

4. **Write Python test scripts** in `tests/api_tests/` that compare API responses

5. **Test your test group** by running smoketest.sh

## Configuration Files

### Single Configuration (`config.ini`)
Used when both instances need the same configuration. The script copies it to both data directories.

### Separate Configurations
- `config_ref.ini` - Configuration for reference instance
- `config_test.ini` - Configuration for tested instance

Use separate configs when instances need different plugin configurations or settings.

### Skip Config Copy
Pass `--dont-copy-config` to skip copying config files. Useful when configs are already in place in the data directories.

## Troubleshooting

### Port Already in Use
If tests fail with port binding errors, another zatterad instance may be running:
```bash
# Check for running instances
netstat -tlpn | grep zatterad
# Kill if found
pkill zatterad
```

### Blockchain Data Missing
Ensure both data directories contain `blockchain/block_log`:
```bash
ls -la ~/zattera-test/tested/data/blockchain/block_log
ls -la ~/zattera-test/reference/data/blockchain/block_log
```

### Docker Permission Issues
Docker commands may require sudo:
```bash
sudo ./docker_build_and_run.sh ...
```

### Test Hangs
Check that both instances are running:
```bash
ps aux | grep zatterad
```

Verify they're listening on the correct ports:
```bash
netstat -tlpn | grep -E '8090|8091'
```

## Python Test Script Requirements

Python test scripts should:
- Accept parameters: `JOBS`, `tested_node_url`, `reference_node_url`, `logs_dir`
- Send identical API requests to both nodes
- Compare responses and log differences
- Return exit code 0 on success, non-zero on failure

Example invocation from `test_group.sh`:
```bash
python3 test_ah_get_account_history.py \
  $JOBS \
  http://0.0.0.0:8090 \
  http://0.0.0.0:8091 \
  $PWD/logs
```

## Notes

- Always use absolute paths for binaries and data directories
- Tested parameters always come before reference parameters
- The framework automatically skips test groups without `test_group.sh`
- Tests run sequentially; a failure stops execution of remaining groups
- Use Docker for isolated, reproducible test environments
- Native execution is faster but may conflict with system zatterad instances
