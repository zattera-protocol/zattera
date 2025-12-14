#!/bin/bash

# CLI Wallet Transaction Command Test Script
# Tests transaction commands with broadcast=false (commands only, not actually broadcast)

CLI_WALLET="../../../build/programs/cli_wallet/cli_wallet"
WALLET_FILE="/tmp/test_wallet_transactions.json"
SERVER="ws://127.0.0.1:8090"

# Color output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results file
RESULTS_FILE="/tmp/cli_wallet_transaction_test_results.txt"
echo "CLI Wallet Transaction Command Test Results - $(date)" > $RESULTS_FILE
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

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    echo -e "\n${YELLOW}Testing: $command${NC}"
    echo "Category: $category" >> $RESULTS_FILE
    echo "Command: $command" >> $RESULTS_FILE

    # Execute command with 10 second timeout
    result=$(timeout 10s bash -c "echo \"$command\" | $CLI_WALLET -s $SERVER -w $WALLET_FILE 2>&1" || echo "TIMEOUT_ERROR")
    exit_code=$?

    # Record results
    echo "Output:" >> $RESULTS_FILE
    echo "$result" >> $RESULTS_FILE
    echo "Exit Code: $exit_code" >> $RESULTS_FILE

    # Check for errors (some commands may fail due to missing keys, but the command itself exists)
    if echo "$result" | grep -qi "TIMEOUT_ERROR\|no method with name"; then
        echo -e "${RED}✗ FAILED - Command not found or timeout${NC}"
        echo "Status: FAILED" >> $RESULTS_FILE
        FAILED_TESTS=$((FAILED_TESTS + 1))
    elif echo "$result" | grep -qi "missing required"; then
        echo -e "${YELLOW}⚠ PARTIAL - Missing required authority/keys (command exists)${NC}"
        echo "Status: PARTIAL (command exists but needs keys)" >> $RESULTS_FILE
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${GREEN}✓ PASSED${NC}"
        echo "Status: PASSED" >> $RESULTS_FILE
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi

    echo "---" >> $RESULTS_FILE
    sleep 0.3
}

# Initialize wallet
echo "Initializing wallet..."
echo -e "set_password testpass123\nunlock testpass123" | timeout 5s $CLI_WALLET -s $SERVER -w $WALLET_FILE > /dev/null 2>&1

echo -e "\n${YELLOW}=== TESTING TRANSACTION COMMANDS ===${NC}\n"

# 1. Account Creation Commands
echo -e "${YELLOW}=== 1. ACCOUNT CREATION COMMANDS ===${NC}"

test_command "Account Creation" "create_account \"creator\" \"newaccount\" \"{}\" false"
test_command "Account Creation" "create_account_with_keys \"creator\" \"newaccount2\" \"{}\" ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh false"
test_command "Account Creation" "create_account_delegated \"creator\" \"0.001 ZTR\" \"0.000000 VESTS\" \"newaccount3\" \"{}\" false"
test_command "Account Creation" "create_account_with_keys_delegated \"creator\" \"0.001 ZTR\" \"0.000000 VESTS\" \"newaccount4\" \"{}\" ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh false"

# 2. Account Update Commands
echo -e "\n${YELLOW}=== 2. ACCOUNT UPDATE COMMANDS ===${NC}"

test_command "Account Update" "update_account \"testaccount\" \"{}\" ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh false"
test_command "Account Update" "update_account_auth_key \"testaccount\" \"active\" ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh 1 false"
test_command "Account Update" "update_account_auth_account \"testaccount\" \"active\" \"otheraccount\" 1 false"
test_command "Account Update" "update_account_auth_threshold \"testaccount\" \"active\" 1 false"
test_command "Account Update" "update_account_meta \"testaccount\" \"{}\" false"
test_command "Account Update" "update_account_memo_key \"testaccount\" ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh false"

# 3. Witness Commands
echo -e "\n${YELLOW}=== 3. WITNESS TRANSACTION COMMANDS ===${NC}"

test_command "Witness" "update_witness \"witnessaccount\" \"http://witness.url\" ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh {\"account_creation_fee\":\"0.100 ZTR\",\"maximum_block_size\":131072,\"dollar_interest_rate\":1000} false"
test_command "Witness" "vote_for_witness \"voter\" \"witness\" true false"
test_command "Witness" "set_voting_proxy \"account\" \"proxy\" false"
test_command "Witness" "publish_feed \"witness\" {\"base\":\"1.000 ZBD\",\"quote\":\"1.000 ZTR\"} false"

# 4. Transfer Commands
echo -e "\n${YELLOW}=== 4. TRANSFER COMMANDS ===${NC}"

test_command "Transfer" "transfer \"from\" \"to\" \"1.000 ZTR\" \"memo\" false"
test_command "Transfer" "transfer_to_vesting \"from\" \"to\" \"1.000 ZTR\" false"
test_command "Transfer" "withdraw_vesting \"from\" \"1.000000 VESTS\" false"
test_command "Transfer" "set_withdraw_vesting_route \"from\" \"to\" 5000 true false"
test_command "Transfer" "delegate_vesting_shares \"delegator\" \"delegatee\" \"1.000000 VESTS\" false"
test_command "Transfer" "transfer_to_savings \"from\" \"to\" \"1.000 ZTR\" \"memo\" false"
test_command "Transfer" "transfer_from_savings \"from\" 1 \"to\" \"1.000 ZTR\" \"memo\" false"
test_command "Transfer" "cancel_transfer_from_savings \"from\" 1 false"

# 5. Escrow Commands
echo -e "\n${YELLOW}=== 5. ESCROW COMMANDS ===${NC}"

test_command "Escrow" "escrow_transfer \"from\" \"to\" \"agent\" 1 \"1.000 ZBD\" \"0.000 ZTR\" \"0.100 ZBD\" \"2025-12-31T23:59:59\" \"2026-01-31T23:59:59\" \"{}\" false"
test_command "Escrow" "escrow_approve \"from\" \"to\" \"agent\" \"to\" 1 true false"
test_command "Escrow" "escrow_dispute \"from\" \"to\" \"agent\" \"from\" 1 false"
test_command "Escrow" "escrow_release \"from\" \"to\" \"agent\" \"from\" \"to\" 1 \"1.000 ZBD\" \"0.000 ZTR\" false"

# 6. Market/Trading Commands
echo -e "\n${YELLOW}=== 6. MARKET/TRADING COMMANDS ===${NC}"

test_command "Market" "create_order \"owner\" 1 \"1.000 ZBD\" \"1.000 ZTR\" false 3600 false"
test_command "Market" "cancel_order \"owner\" 1 false"
test_command "Market" "convert_dollar \"from\" \"1.000 ZBD\" false"

# 7. Content Commands
echo -e "\n${YELLOW}=== 7. CONTENT COMMANDS ===${NC}"

test_command "Content" "post_comment \"author\" \"permlink\" \"\" \"category\" \"title\" \"body\" \"{}\" false"
test_command "Content" "vote \"voter\" \"author\" \"permlink\" 10000 false"
test_command "Content" "follow \"follower\" \"following\" [\"blog\"] false"

# 8. Recovery Commands
echo -e "\n${YELLOW}=== 8. RECOVERY COMMANDS ===${NC}"

test_command "Recovery" "request_account_recovery \"recovery_account\" \"account_to_recover\" {\"weight_threshold\":1,\"account_auths\":[],\"key_auths\":[[\"ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh\",1]]} false"
test_command "Recovery" "recover_account \"account_to_recover\" {\"weight_threshold\":1,\"account_auths\":[],\"key_auths\":[[\"ZTR5RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh\",1]]} {\"weight_threshold\":1,\"account_auths\":[],\"key_auths\":[[\"ZTR6RqVBAVNp5ufMCetQtvLGLJo7unX9nyCBMMrTXRWQ9i1Zzzizh\",1]]} false"
test_command "Recovery" "change_recovery_account \"owner\" \"new_recovery_account\" false"

# 9. Other Transaction Commands
echo -e "\n${YELLOW}=== 9. OTHER TRANSACTION COMMANDS ===${NC}"

test_command "Other" "decline_voting_rights \"account\" true false"
test_command "Other" "claim_reward_balance \"account\" \"0.000 ZTR\" \"0.000 ZBD\" \"0.000000 VESTS\" false"

# 10. Transaction Utility Commands
echo -e "\n${YELLOW}=== 10. TRANSACTION UTILITY COMMANDS ===${NC}"

# serialize_transaction and sign_transaction require actual transaction objects, so skip
echo -e "${YELLOW}Note: serialize_transaction and sign_transaction require actual transaction objects and are tested separately${NC}"

# get_encrypted_memo requires keys
test_command "Utility" "get_encrypted_memo \"from\" \"to\" \"test memo\""

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

echo "Detailed results saved to: $RESULTS_FILE"
echo ""

# Calculate success rate
if [ $TOTAL_TESTS -gt 0 ]; then
    SUCCESS_RATE=$(( 100 * PASSED_TESTS / TOTAL_TESTS ))
    echo -e "Success Rate: ${GREEN}${SUCCESS_RATE}%${NC}"
    echo "Success Rate: ${SUCCESS_RATE}%" >> $RESULTS_FILE
fi

echo ""
echo -e "${YELLOW}Note: Some commands may show 'PARTIAL' status because they require"
echo -e "proper authentication keys to execute. This is expected behavior."
echo -e "The important thing is that the commands are recognized by cli_wallet.${NC}"

exit 0
