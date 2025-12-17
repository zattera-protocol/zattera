# CLI Wallet Test Scripts

This directory contains comprehensive test scripts for the `cli_wallet` command-line interface.

## Test Scripts

### test_commands.sh

Tests all wallet query and informational commands. This script validates that all commands are recognized by `cli_wallet` and return expected responses.

**Test Categories:**
1. Wallet Management Commands (11 tests)
   - Password management, locking/unlocking, wallet file operations
2. Key Management Commands (4 tests)
   - Brain key generation, key normalization, key listing, key import
3. Basic Query Commands (8 tests)
   - Chain info, global properties, feed history, witnesses, blocks, operations
4. Account Query Commands (6 tests)
   - Account listing, account details, account history
5. Witness Commands (2 tests)
   - Witness listing and details
6. Market Commands (4 tests)
   - Order book, open orders, conversion requests
7. Withdraw Routes (3 tests)
   - Vesting withdraw route queries
8. Utility Commands (4 tests)
   - Prototype operations, transaction expiration settings
9. Help Commands (4 tests)
   - Command-specific help documentation
10. Prototype Operations (28 tests)
    - All operation type definitions
11. Private Key Tests (5 tests)
    - Key import, key derivation from passwords
12. Memo Encryption Tests (1 test)
    - Memo decryption
13. Transaction Tests (1 test)
    - Transaction ID queries

**Total: 80 tests**

### test_transactions.sh

Tests all wallet transaction commands with `broadcast=false` to validate command syntax and parameter handling without actually broadcasting to the network.

**Test Categories:**
1. Account Creation Commands (4 tests)
   - create_account, create_account_with_keys, delegated account creation
2. Account Update Commands (6 tests)
   - update_account, authority updates, metadata updates, memo key updates
3. Witness Transaction Commands (4 tests)
   - update_witness, vote_for_witness, set_voting_proxy, publish_feed
4. Transfer Commands (8 tests)
   - transfer, transfer_to_vesting, withdraw_vesting, vesting delegation, savings operations
5. Escrow Commands (4 tests)
   - escrow_transfer, escrow_approve, escrow_dispute, escrow_release
6. Market/Trading Commands (3 tests)
   - create_order, cancel_order, convert_dollar
7. Content Commands (3 tests)
   - post_comment, vote, follow
8. Recovery Commands (3 tests)
   - request_account_recovery, recover_account, change_recovery_account
9. Other Transaction Commands (2 tests)
   - decline_voting_rights, claim_reward_balance
10. Transaction Utility Commands (1 test)
    - get_encrypted_memo

**Total: 38 tests**

## Prerequisites

Before running the tests, ensure you have:

1. **Built cli_wallet**
   ```bash
   cd /path/to/zattera
   mkdir -p build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make -j$(nproc) cli_wallet
   ```

2. **Running zatterad node**

   The node must be running with websocket endpoint enabled and the following plugins:
   - `account_by_key_api`
   - `account_history_api`
   - `database_api`
   - `block_api`
   - `network_broadcast_api`
   - `tags_api`
   - `follow_api`
   - `reputation_api`
   - `market_history_api`
   - `witness_api`

   Example zatterad configuration:
   ```ini
   webserver-ws-endpoint = 127.0.0.1:8090
   plugin = account_by_key account_by_key_api
   plugin = account_history account_history_api
   plugin = database_api
   plugin = block_api
   plugin = network_broadcast_api
   plugin = tags tags_api
   plugin = follow follow_api
   plugin = reputation reputation_api
   plugin = market_history market_history_api
   plugin = witness witness_api
   ```

## Running the Tests

### Run all command tests
```bash
cd /path/to/zattera
./programs/cli_wallet/tests/test_commands.sh
```

### Run all transaction tests
```bash
cd /path/to/zattera
./programs/cli_wallet/tests/test_transactions.sh
```

## Test Results

Both scripts generate detailed test results:

- **Console output**: Color-coded pass/fail status for each test
  - Green (✓) = Passed
  - Red (✗) = Failed
  - Yellow (⚠) = Partial (command exists but needs authentication)

- **Result files**: Detailed logs written to `/tmp/`
  - `test_commands.sh` → `/tmp/cli_wallet_test_results.txt`
  - `test_transactions.sh` → `/tmp/cli_wallet_transaction_test_results.txt`

- **Summary**: Total tests, passed, failed, and success rate

## Expected Behavior

### test_commands.sh
Most tests should pass completely. Some tests may fail if:
- zatterad is not running or not accessible at `ws://127.0.0.1:8090`
- Required API plugins are not enabled on the node
- Network connectivity issues

### test_transactions.sh
Tests are expected to show "PARTIAL" status when:
- Command is recognized but requires proper authentication keys
- This is normal and indicates the command exists and is properly registered

Tests will fail only if:
- Command is not recognized by cli_wallet (`no method with name` error)
- Command times out (takes longer than 10 seconds)

## Customization

You can customize the test scripts by editing the following variables at the top of each file:

```bash
CLI_WALLET="./build/programs/cli_wallet/cli_wallet"  # Path to cli_wallet executable
WALLET_FILE="/tmp/test_wallet.json"                   # Temporary wallet file path
SERVER="ws://127.0.0.1:8090"                          # zatterad websocket endpoint
```

## Troubleshooting

### Connection refused
- Ensure zatterad is running
- Check that websocket endpoint is configured in zatterad config
- Verify the SERVER variable matches your zatterad websocket endpoint

### Command timeouts
- Increase timeout values in the scripts (default: 5s for commands, 10s for transactions)
- Check zatterad performance and resource availability

### Missing plugins
- Verify all required API plugins are enabled in zatterad configuration
- Restart zatterad after modifying plugin configuration

## Contributing

When adding new wallet commands:
1. Add corresponding test cases to the appropriate script
2. Update the test category counts in this README
3. Ensure tests validate both command syntax and error handling
