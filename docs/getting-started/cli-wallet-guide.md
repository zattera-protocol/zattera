# CLI Wallet User Guide

Complete guide to using the Zattera command-line wallet (`cli_wallet`).

## Overview

The `cli_wallet` is an interactive command-line interface for managing accounts, keys, and transactions on the Zattera blockchain. It connects to a running `zatterad` node via WebSocket and provides a secure environment for blockchain operations.

**Features:**
- Account management (create, update)
- Key management and wallet encryption
- Token transfers and conversions
- Content operations (post, vote, comment)
- Witness operations
- Multi-signature transaction support
- Self-documented with built-in help system

## Prerequisites

### Node Requirements

The `zatterad` node you connect to must have the following plugins enabled. The wallet uses **direct modular API calls**.

**Required Plugins:**
- `webserver` - WebSocket/HTTP server
- `database_api` - Core blockchain state queries
- `account_by_key_api` - Public key to account mapping
- `network_broadcast_api` - Transaction broadcasting

**Recommended Plugins** (for full functionality):
- `block_api` - Block and header queries
- `account_history_api` - Account transaction history
- `tags_api` - Content and discussion queries
- `follow_api` - Social graph and feeds
- `reputation_api` - Reputation scores
- `market_history_api` - Market data and trading history
- `witness_api` - Witness and bandwidth data

**Minimum Configuration** (basic wallet operations):
```ini
# Core plugins
plugin = webserver
plugin = database_api
plugin = account_by_key_api
plugin = network_broadcast_api

# Optional but recommended
plugin = block_api
plugin = account_history_api

# WebSocket endpoint
webserver-ws-endpoint = 0.0.0.0:8090
```

**Full Node Configuration** (all wallet features):
```ini
# Core plugins
plugin = webserver
plugin = database_api
plugin = account_by_key_api
plugin = network_broadcast_api

# Optional plugins
plugin = block_api
plugin = account_history_api
plugin = tags_api
plugin = follow_api
plugin = reputation_api
plugin = market_history_api
plugin = witness_api

# WebSocket endpoint
webserver-ws-endpoint = 0.0.0.0:8090
```

### Building cli_wallet

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) cli_wallet
```

Binary location: `./programs/cli_wallet/cli_wallet`

## Getting Started

### 1. Connect to Node

```bash
# Local node
./cli_wallet -s ws://127.0.0.1:8090

# Remote node
./cli_wallet -s ws://node.example.com:8090

# Specify wallet file
./cli_wallet -s ws://127.0.0.1:8090 -w my_wallet.json
```

### 2. First Launch

On first launch, you'll see:

```
new >>>
```

The `new` prompt indicates the wallet is not yet created.

### 3. Set Wallet Password

Create a password to encrypt your wallet:

```
new >>> set_password my_secure_password
set_password my_secure_password
null
locked >>>
```

The prompt changes to `locked` - wallet is encrypted but locked.

### 4. Unlock Wallet

```
locked >>> unlock my_secure_password
unlock my_secure_password
null
unlocked >>>
```

Now the wallet is unlocked and ready to use!

## Basic Commands

### Getting Help

```bash
# List all commands
>>> help

# Get help for specific command
>>> gethelp transfer
>>> gethelp create_account
```

### Wallet Information

```bash
# Check if wallet is locked
>>> is_locked

# Check if wallet is new
>>> is_new

# Show wallet info
>>> info

# List imported keys (public keys only)
>>> list_keys

# List accounts in wallet
>>> list_my_accounts
```

## Key Management

### Import Private Keys

```bash
# Import private key (WIF format)
>>> import_key 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n
true

# Verify key imported
>>> list_keys
true

# Verify key imported
>>> list_keys
```

### Generate New Keys

```bash
# Generate new key pair (brain key method)
>>> suggest_brain_key

# Output:
{
  "brain_priv_key": "WORD1 WORD2 WORD3 ... WORD16",
  "wif_priv_key": "5J...",
  "pub_key": "STM..."
}
```

**Important**: Save the `brain_priv_key` and `wif_priv_key` securely!

### Derive Keys from Brain Key

```bash
# Get private key from brain key
>>> get_private_key_from_password account_name owner "BRAIN KEY WORDS HERE"
```

## Account Operations

### Check Account Balance

```bash
# Get account info
>>> get_account alice

# Output shows:
# - balance (liquid ZTR)
# - sbd_balance (liquid ZBD)
# - vesting_shares (Zattera Power)
```

### Create Account

On a testnet or genesis chain where you control genesis:

```bash
# Import genesis key first
>>> import_key 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n

# Create new account (free on testnet)
>>> create_account genesis alice "" true

# Arguments:
# - creator: genesis
# - new_account_name: alice
# - json_meta: "" (empty)
# - broadcast: true (send to blockchain)
```

### Create Account with Keys

```bash
# Generate keys first
>>> suggest_brain_key
# Save the output!

# Create account with specific keys
>>> create_account_with_keys genesis alice "" "OWNER_KEY" "ACTIVE_KEY" "POSTING_KEY" "MEMO_KEY" true
```

### Update Account Keys

```bash
# Update owner key
>>> update_account alice "" "NEW_OWNER_KEY" "" "" true

# Update active key
>>> update_account alice "" "" "NEW_ACTIVE_KEY" "" true

# Update posting key
>>> update_account alice "" "" "" "NEW_POSTING_KEY" true
```

## Token Operations

### Transfer ZTR

```bash
# Basic transfer
>>> transfer alice bob "10.000 ZTR" "Payment for services" true

# Arguments:
# - from: alice
# - to: bob
# - amount: "10.000 ZTR" (must include 3 decimals)
# - memo: "Payment for services"
# - broadcast: true
```

### Transfer to Savings

```bash
# Move to savings (3-day withdrawal)
>>> transfer_to_savings alice alice "100.000 ZTR" "Saving for later" true

# Transfer from savings (starts 3-day waiting period)
>>> transfer_from_savings alice 1 "50.000 ZTR" "alice" "Withdrawal" true

# Cancel pending savings withdrawal
>>> cancel_transfer_from_savings alice 1 true
```

### Power Up (ZTR to Zattera Power)

```bash
# Convert ZTR to Zattera Power (vesting)
>>> transfer_to_vesting alice alice "100.000 ZTR" true

# Power up to another account
>>> transfer_to_vesting alice bob "50.000 ZTR" true
```

### Power Down (Start Vesting Withdrawal)

```bash
# Start 13-week power down
>>> withdraw_vesting alice "1000.000000 VESTS" true

# Stop power down
>>> withdraw_vesting alice "0.000000 VESTS" true
```

### Delegate Zattera Power

```bash
# Delegate to another account
>>> delegate_vesting_shares alice bob "10000.000000 VESTS" true

# Remove delegation (set to 0)
>>> delegate_vesting_shares alice bob "0.000000 VESTS" true
```

### Convert ZBD to ZTR

```bash
# Convert ZBD to ZTR (3.5 day conversion)
>>> convert_dollar alice "10.000 ZBD" true
```

## Content Operations

### Post Content

```bash
# Create post
>>> post_comment alice "test-post" "" "parentpermlink" "My First Post" "This is the body of my post" "{}" true

# Arguments:
# - author: alice
# - permlink: test-post (URL slug)
# - parent_author: "" (empty for top-level post)
# - parent_permlink: parentpermlink (category/tag)
# - title: "My First Post"
# - body: "This is the body of my post"
# - json_metadata: "{}"
# - broadcast: true
```

### Comment on Post

```bash
# Reply to post
>>> post_comment alice "my-reply" bob "test-post" "" "Great post!" "{}" true

# Arguments:
# - author: alice
# - permlink: my-reply
# - parent_author: bob
# - parent_permlink: test-post
# - title: "" (empty for comments)
# - body: "Great post!"
```

### Vote on Content

```bash
# Upvote (100%)
>>> vote alice bob "test-post" 100 true

# Partial upvote (50%)
>>> vote alice bob "test-post" 50 true

# Downvote (flag)
>>> vote alice bob "test-post" -100 true

# Remove vote
>>> vote alice bob "test-post" 0 true

# Weight: -100 to 100 (in percentage points)
```

### Delete Comment

```bash
# Delete your own comment/post
>>> delete_comment alice "test-post" true
```

## Witness Operations

### Update Witness

```bash
# Generate signing key
>>> suggest_brain_key
# Save the signing key!

# Update witness properties
>>> update_witness alice "https://witness.example.com" "STM_SIGNING_PUBLIC_KEY" {"account_creation_fee":"0.100 ZTR","maximum_block_size":65536,"sbd_interest_rate":0} true
```

### Vote for Witness

```bash
# Vote for witness
>>> vote_for_witness alice bob true true

# Unvote witness
>>> vote_for_witness alice bob false true
```

### Set Witness Voting Proxy

```bash
# Set proxy (delegate witness votes)
>>> set_voting_proxy alice bob true

# Remove proxy
>>> set_voting_proxy alice "" true
```

## Advanced Operations

### Escrow Transactions

```bash
# Create escrow
>>> escrow_transfer alice bob 123 eve "10.000 ZTR" "0.000 ZBD" "0.100 ZTR" "2025-12-31T00:00:00" "2026-01-31T00:00:00" "{}" true

# Approve escrow (agent)
>>> escrow_approve alice bob eve 123 true true

# Release escrow (agent)
>>> escrow_release alice bob eve 123 alice "10.000 ZTR" "0.000 ZBD" true

# Dispute escrow
>>> escrow_dispute alice bob eve 123 true
```

### Multi-Signature Transactions

```bash
# Create account with multi-sig
>>> update_account alice "" "STM_KEY1" "" "" true
>>> update_account_auth_account alice active bob 1 true
>>> update_account_auth_threshold alice active 2 true

# Now alice requires 2 signatures (owner key + bob's key)
```

## Transaction Building

### Build Custom Transaction

```bash
# Begin building transaction
>>> begin_builder_transaction

# Add operation
>>> add_operation_to_builder_transaction 0 ["transfer",{"from":"alice","to":"bob","amount":"1.000 ZTR","memo":"test"}]

# Preview transaction
>>> preview_builder_transaction 0

# Sign transaction
>>> sign_builder_transaction 0 true

# Broadcast or save for later
```

## Query Operations

### Blockchain Information

```bash
# Get blockchain info
>>> info

# Get dynamic global properties
>>> get_dynamic_global_properties

# Get specific block
>>> get_block 1000

# Get account
>>> get_account alice

# Get witness
>>> get_witness bob
```

### Account History

```bash
# Get account history (last 100 operations)
>>> get_account_history alice -1 100

# Get specific operation types
>>> get_account_history alice -1 100
```

## Configuration

### Wallet File Location

Default: `wallet.json` in current directory

Specify custom location:
```bash
./cli_wallet -s ws://127.0.0.1:8090 -w /path/to/my_wallet.json
```

### Chain ID

For custom chains, specify chain ID:
```bash
./cli_wallet -s ws://127.0.0.1:8090 --chain-id=MY_CHAIN_ID
```

Get chain ID from node:
```bash
curl -s http://127.0.0.1:8090 -d '{"jsonrpc":"2.0","method":"database_api.get_config","params":{},"id":1}' | jq -r '.result.ZATTERA_CHAIN_ID'
```

## Security Best Practices

### 1. Wallet Password

- Use strong, unique password
- Never share wallet password
- Store password securely (password manager)

### 2. Private Keys

- Never expose private keys
- Keep backup of keys in secure location
- Use different keys for different authority levels (owner/active/posting)

### 3. Wallet File

- Backup `wallet.json` regularly
- Encrypt backups
- Store in secure location

### 4. Network Security

- Use HTTPS/WSS for remote connections
- Verify node authenticity
- Use trusted nodes only

### 5. Key Hierarchy

**Owner Key**: Master key, use only for:
- Account recovery
- Changing other keys
- Keep offline when possible

**Active Key**: For financial operations:
- Transfers
- Power up/down
- Conversions

**Posting Key**: For social operations:
- Posting
- Voting
- Comments
- Safest to use regularly

**Memo Key**: For encrypted memos

## Common Use Cases

### Genesis Node Setup

```bash
# Connect to genesis node
./cli_wallet -s ws://127.0.0.1:8090

# Set password
>>> set_password mypassword
>>> unlock mypassword

# Import genesis key (testnet)
>>> import_key 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n

# Create accounts
>>> create_account genesis alice "" true
>>> create_account genesis bob "" true

# Fund accounts
>>> transfer genesis alice "1000.000 ZTR" "Initial funding" true
>>> transfer genesis bob "1000.000 ZTR" "Initial funding" true
```

### Witness Setup

```bash
# Generate witness signing key
>>> suggest_brain_key
# Save the keys!

# Import active key
>>> import_key YOUR_ACTIVE_PRIVATE_KEY

# Update witness
>>> update_witness myaccount "https://witness.example.com" "STM_SIGNING_PUBLIC_KEY" {"account_creation_fee":"0.100 ZTR","maximum_block_size":65536} true

# Add to zatterad config.ini:
# witness = "myaccount"
# private-key = WIF_SIGNING_PRIVATE_KEY
```

### Voting Bot

```bash
# Import posting key
>>> import_key POSTING_PRIVATE_KEY

# Vote on content
>>> vote mybot author "permlink" 100 true
```

## Command Reference

### Wallet Management
- `set_password` - Set wallet password
- `unlock` - Unlock wallet
- `lock` - Lock wallet
- `import_key` - Import private key
- `suggest_brain_key` - Generate new key pair
- `list_keys` - List imported public keys
- `list_my_accounts` - List wallet accounts

### Account Operations
- `create_account` - Create new account
- `create_account_with_keys` - Create account with specific keys
- `update_account` - Update account keys/metadata
- `get_account` - Get account information

### Transfers
- `transfer` - Transfer ZTR/ZBD
- `transfer_to_vesting` - Power up
- `withdraw_vesting` - Power down
- `transfer_to_savings` - Move to savings
- `transfer_from_savings` - Withdraw from savings
- `delegate_vesting_shares` - Delegate Zattera Power

### Content
- `post_comment` - Post or comment
- `vote` - Vote on content
- `delete_comment` - Delete comment/post

### Witness
- `update_witness` - Update witness properties
- `vote_for_witness` - Vote for witness
- `set_voting_proxy` - Set witness voting proxy

### Query
- `info` - Wallet info
- `get_block` - Get block by number
- `get_account_history` - Get account history
- `get_dynamic_global_properties` - Get blockchain state

### Advanced
- `begin_builder_transaction` - Start building custom transaction
- `add_operation_to_builder_transaction` - Add operation
- `sign_builder_transaction` - Sign and broadcast

## Troubleshooting

### Common Errors

#### "Could not connect to zatterad"
**Problem**: Can't connect to node

**Solutions**:
- Verify node is running: `curl http://127.0.0.1:8090`
- Check WebSocket endpoint: `webserver-ws-endpoint` in config.ini
- Try different port or address

#### "Missing Active Authority"
**Problem**: Transaction requires active key but not imported

**Solution**:
```bash
>>> import_key YOUR_ACTIVE_PRIVATE_KEY
```

#### "Wallet is locked"
**Problem**: Need to unlock wallet

**Solution**:
```bash
>>> unlock YOUR_PASSWORD
```

#### "Assert Exception: account_by_key_api plugin not enabled"
**Problem**: Node missing required plugin

**Solution**: Add to node's config.ini:
```ini
plugin = account_by_key_api
```

#### "Invalid transaction signature"
**Problem**: Wrong private key or chain ID

**Solutions**:
- Verify correct private key imported
- Check chain ID matches node
- Ensure wallet is unlocked

### Connection Issues

**Problem:** `Server has disconnected us` or connection errors

**Solution:**
1. Verify `zatterad` is running and WebSocket endpoint is accessible:
   ```bash
   curl -s --data '{"jsonrpc":"2.0","method":"database_api.get_config","params":{},"id":1}' http://127.0.0.1:8090
   ```

2. Check node logs for plugin initialization
3. Ensure firewall allows WebSocket connections

### Plugin Not Available Errors

**Problem:** RPC errors mentioning missing plugins or APIs

**Solution:**
1. Check which plugins are loaded in node:
   ```bash
   # In zatterad logs, look for "Registered plugin:" messages
   grep "Registered plugin" witness_node_data_dir/logs/*.log
   ```

2. Add missing plugins to `config.ini`:
   ```ini
   plugin = database_api
   plugin = account_history_api
   plugin = block_api
   # ... add other required plugins
   ```

3. Restart `zatterad` node

### Command Not Working

**Problem:** Certain wallet commands return errors or empty results

**Possible Causes:**
- **get_account_votes()** - Currently returns empty vector (known limitation)
- **broadcast_transaction_synchronous()** - Returns dummy result (known limitation)
- Missing optional plugins on connected node

**Solution:**
- For account votes, use alternative APIs via custom queries
- For synchronous broadcast, use regular `broadcast_transaction()` and poll for confirmation
- Enable recommended plugins for full wallet functionality

### Performance Issues

**Problem:** Slow command responses

**Solution:**
1. Ensure node has sufficient resources (RAM, CPU, SSD)
2. Use local node instead of remote for better latency
3. Enable only required plugins to reduce node overhead
4. Check node sync status with `get_dynamic_global_properties`

## Additional Resources

- [Quick Start Guide](quick-start.md) - Get started quickly with Docker
- [Building Guide](building.md) - Build from source on different platforms
- [Exchange Quick Start Guide](exchange-quick-start.md) - Detailed guide for cryptocurrency exchanges
- [Node Modes Guide](../operations/node-modes-guide.md) - Different node types and configurations
- [Genesis Launch Guide](../operations/genesis-launch.md) - Setting up genesis node
- [Configuration Examples](../../configs/README.md) - Ready-to-use configuration files

## License

See [LICENSE.md](../../LICENSE.md)
