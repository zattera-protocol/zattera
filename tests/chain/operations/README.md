# Operation Tests

This directory contains unit tests for blockchain operations, organized by domain.

## Test Files

- **account_test.cpp** - Account lifecycle and recovery (create/update, witness vote/proxy, post rate limit, clear null account, account recovery/change recovery, account bandwidth, claim account, create claimed account)
- **comment_test.cpp** - Comment operations (comment, delete, vote, options, payout, nested comments, freeze, beneficiaries validation/apply)
- **vesting_test.cpp** - Vesting flows (transfer to vesting, withdraw vesting, withdraw routes, delegate vesting shares, issue 971 regression)
- **transfer_test.cpp** - Transfers, savings, decline voting rights, and escrow (transfer, savings in/out, escrow transfer/approve/dispute/release)
- **witness_test.cpp** - Witness operations (update, set properties, feed publish, feed publish mean)
- **market_test.cpp** - Market operations (convert, limit orders, ZBD interest/stability/price feed, convert delay)
- **reward_test.cpp** - Reward operations (claim reward balance, reward funds, recent claims decay)
- **custom_test.cpp** - Custom/custom_json/custom_binary authorities
- **pow_test.cpp** - POW validation/authority/apply stubs (kept for suite parity)

## Test Suite

All tests in this directory use the `operation_tests` test suite for backward compatibility.

## Running Tests

```bash
# Run all operation tests
./tests/chain_test --run_test=operation_tests

# Run specific test case
./tests/chain_test --run_test=operation_tests/account_create_validate
./tests/chain_test --run_test=operation_tests/vesting_withdrawals
```

## Adding New Tests

When adding new operation tests:

1. Determine the appropriate domain file (account, comment, vesting, etc.)
2. If a new domain is needed, create a new `<domain>_test.cpp` file
3. Use the `operation_tests` test suite for consistency
4. Add the file to `CHAIN_TEST_SOURCES` in tests/CMakeLists.txt
