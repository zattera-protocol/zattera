#!/bin/bash

# CLI Wallet Command Test Script
# Tests all wallet commands comprehensively

CLI_WALLET="../../../build/programs/cli_wallet/cli_wallet"
WALLET_FILE="/tmp/test_wallet.json"
SERVER="ws://127.0.0.1:8090"

# Color output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results file
RESULTS_FILE="/tmp/cli_wallet_test_results.txt"
echo "CLI Wallet Command Test Results - $(date)" > $RESULTS_FILE
echo "========================================" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Remove existing wallet file
rm -f $WALLET_FILE

# Test function
test_command() {
    local category=$1
    local command=$2
    local expected_pattern=$3

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    echo -e "\n${YELLOW}Testing: $command${NC}"
    echo "Category: $category" >> $RESULTS_FILE
    echo "Command: $command" >> $RESULTS_FILE

    # Execute command with 5 second timeout
    result=$(timeout 5s bash -c "echo \"$command\" | $CLI_WALLET -s $SERVER -w $WALLET_FILE 2>&1" || echo "TIMEOUT_ERROR")
    exit_code=$?

    # Record results
    echo "Output:" >> $RESULTS_FILE
    echo "$result" >> $RESULTS_FILE
    echo "Exit Code: $exit_code" >> $RESULTS_FILE

    # Check for errors
    if echo "$result" | grep -qi "error\|exception\|failed\|TIMEOUT_ERROR"; then
        echo -e "${RED}✗ FAILED${NC}"
        echo "Status: FAILED" >> $RESULTS_FILE
        FAILED_TESTS=$((FAILED_TESTS + 1))
        echo "$result"
    else
        echo -e "${GREEN}✓ PASSED${NC}"
        echo "Status: PASSED" >> $RESULTS_FILE
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi

    echo "---" >> $RESULTS_FILE

    # Avoid sending requests too quickly
    sleep 0.5
}

# 1. Wallet Management Commands
echo -e "\n${YELLOW}=== 1. WALLET MANAGEMENT COMMANDS ===${NC}"

test_command "Wallet Management" "help"
test_command "Wallet Management" "about"
test_command "Wallet Management" "is_new"
test_command "Wallet Management" "set_password testpassword123"
test_command "Wallet Management" "is_locked"
test_command "Wallet Management" "unlock testpassword123"
test_command "Wallet Management" "is_locked"
test_command "Wallet Management" "get_wallet_filename"
test_command "Wallet Management" "save_wallet_file"
test_command "Wallet Management" "lock"
test_command "Wallet Management" "unlock testpassword123"

# 2. Key Management Commands
echo -e "\n${YELLOW}=== 2. KEY MANAGEMENT COMMANDS ===${NC}"

test_command "Key Management" "suggest_brain_key"
test_command "Key Management" "normalize_brain_key \"EXAMPLE brain KEY with   SPACES\""
test_command "Key Management" "list_keys"
# import_key requires an actual key, tested later

# 3. Basic Query Commands
echo -e "\n${YELLOW}=== 3. BASIC QUERY COMMANDS ===${NC}"

test_command "Query" "info"
test_command "Query" "get_dynamic_global_properties"
test_command "Query" "get_feed_history"
test_command "Query" "get_active_witnesses"
test_command "Query" "get_block 1"
test_command "Query" "get_block 100"
test_command "Query" "get_ops_in_block 1 true"
test_command "Query" "get_ops_in_block 1 false"

# 4. Account Query Commands
echo -e "\n${YELLOW}=== 4. ACCOUNT QUERY COMMANDS ===${NC}"

test_command "Account Query" "list_my_accounts"
test_command "Account Query" "list_accounts \"\" 10"
test_command "Account Query" "list_accounts \"a\" 5"
test_command "Account Query" "get_account \"genesis\""

# Test with actual account if available
test_command "Account Query" "get_account_history \"genesis\" -1 10"
test_command "Account Query" "get_owner_history \"genesis\""

# 5. Witness Commands
echo -e "\n${YELLOW}=== 5. WITNESS COMMANDS ===${NC}"

test_command "Witness" "list_witnesses \"\" 10"
test_command "Witness" "get_witness \"genesis\""

# 6. Market Commands
echo -e "\n${YELLOW}=== 6. MARKET COMMANDS ===${NC}"

test_command "Market" "get_order_book 10"
test_command "Market" "get_order_book 100"
test_command "Market" "get_open_orders \"genesis\""
test_command "Market" "get_conversion_requests \"genesis\""

# 7. Withdraw Routes
echo -e "\n${YELLOW}=== 7. WITHDRAW ROUTES ===${NC}"

test_command "Vesting" "get_withdraw_routes \"genesis\" \"all\""
test_command "Vesting" "get_withdraw_routes \"genesis\" \"incoming\""
test_command "Vesting" "get_withdraw_routes \"genesis\" \"outgoing\""

# 8. Utility Commands
echo -e "\n${YELLOW}=== 8. UTILITY COMMANDS ===${NC}"

test_command "Utility" "get_prototype_operation \"transfer_operation\""
test_command "Utility" "get_prototype_operation \"vote_operation\""
test_command "Utility" "get_prototype_operation \"comment_operation\""
test_command "Utility" "set_transaction_expiration 30"

# 9. Help command tests (gethelp)
echo -e "\n${YELLOW}=== 9. HELP COMMANDS ===${NC}"

test_command "Help" "gethelp info"
test_command "Help" "gethelp transfer"
test_command "Help" "gethelp vote"
test_command "Help" "gethelp get_account"

# 10. More prototype operations tests
echo -e "\n${YELLOW}=== 10. PROTOTYPE OPERATIONS ===${NC}"

OPERATIONS=(
    "account_create_operation"
    "account_update_operation"
    "witness_update_operation"
    "delete_comment_operation"
    "escrow_transfer_operation"
    "escrow_approve_operation"
    "escrow_dispute_operation"
    "escrow_release_operation"
    "transfer_to_vesting_operation"
    "withdraw_vesting_operation"
    "set_withdraw_vesting_route_operation"
    "limit_order_create_operation"
    "limit_order_cancel_operation"
    "feed_publish_operation"
    "convert_operation"
    "account_witness_vote_operation"
    "account_witness_proxy_operation"
    "custom_operation"
    "custom_json_operation"
    "pow_operation"
    "request_account_recovery_operation"
    "recover_account_operation"
    "change_recovery_account_operation"
    "transfer_to_savings_operation"
    "transfer_from_savings_operation"
    "cancel_transfer_from_savings_operation"
    "decline_voting_rights_operation"
    "claim_reward_balance_operation"
    "delegate_vesting_shares_operation"
)

for op in "${OPERATIONS[@]}"; do
    test_command "Prototype Operation" "get_prototype_operation \"$op\""
done

# 11. Private Key tests (after generating brain key)
echo -e "\n${YELLOW}=== 11. PRIVATE KEY TESTS ===${NC}"

# Generate key using suggest_brain_key
BRAIN_KEY_OUTPUT=$(echo "suggest_brain_key" | timeout 5s $CLI_WALLET -s $SERVER -w $WALLET_FILE 2>&1)
echo "Brain Key Output: $BRAIN_KEY_OUTPUT" >> $RESULTS_FILE

# Try to extract WIF key
if echo "$BRAIN_KEY_OUTPUT" | grep -q "wif_priv_key"; then
    WIF_KEY=$(echo "$BRAIN_KEY_OUTPUT" | grep -oP '"wif_priv_key":\s*"\K[^"]+' | head -1)
    if [ ! -z "$WIF_KEY" ]; then
        test_command "Key Management" "import_key \"$WIF_KEY\""
        test_command "Key Management" "list_keys"
    fi
fi

# get_private_key_from_password tests
test_command "Key Management" "get_private_key_from_password \"testaccount\" \"active\" \"testpassword\""
test_command "Key Management" "get_private_key_from_password \"testaccount\" \"owner\" \"testpassword\""
test_command "Key Management" "get_private_key_from_password \"testaccount\" \"posting\" \"testpassword\""
test_command "Key Management" "get_private_key_from_password \"testaccount\" \"memo\" \"testpassword\""

# 12. Memo encryption/decryption tests
echo -e "\n${YELLOW}=== 12. MEMO ENCRYPTION TESTS ===${NC}"

test_command "Memo" "decrypt_memo \"test memo\""

# 13. Transaction ID tests
echo -e "\n${YELLOW}=== 13. TRANSACTION TESTS ===${NC}"

# Test with non-existent transaction ID
test_command "Transaction" "get_transaction \"0000000000000000000000000000000000000000\""

# Final results output
echo -e "\n${YELLOW}=== TEST SUMMARY ===${NC}"
echo ""
echo -e "Total Tests:  ${YELLOW}$TOTAL_TESTS${NC}"
echo -e "Passed:       ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed:       ${RED}$FAILED_TESTS${NC}"
echo ""

# Record to results file
echo "" >> $RESULTS_FILE
echo "========================================" >> $RESULTS_FILE
echo "TEST SUMMARY" >> $RESULTS_FILE
echo "========================================" >> $RESULTS_FILE
echo "Total Tests: $TOTAL_TESTS" >> $RESULTS_FILE
echo "Passed: $PASSED_TESTS" >> $RESULTS_FILE
echo "Failed: $FAILED_TESTS" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE
echo "Detailed results saved to: $RESULTS_FILE" >> $RESULTS_FILE

echo "Detailed results saved to: $RESULTS_FILE"
echo ""

# Calculate success rate
if [ $TOTAL_TESTS -gt 0 ]; then
    SUCCESS_RATE=$(( 100 * PASSED_TESTS / TOTAL_TESTS ))
    echo -e "Success Rate: ${GREEN}${SUCCESS_RATE}%${NC}"
    echo "Success Rate: ${SUCCESS_RATE}%" >> $RESULTS_FILE
fi

exit 0
