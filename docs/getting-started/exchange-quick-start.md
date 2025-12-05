# Exchange Quick Start Guide

This guide helps cryptocurrency exchanges integrate Zattera quickly and efficiently.

## Overview

Zattera is a Delegated Proof of Stake (DPoS) blockchain with high transaction throughput. Running a node for exchange integration requires adequate resources and proper configuration.

## System Requirements

### Minimum Requirements

- **CPU**: 4+ cores with decent single-core performance
- **RAM**: 16GB minimum (32GB recommended)
- **Storage**: 120GB+ fast SSD storage
- **Network**: Stable internet connection with adequate bandwidth
- **OS**: Ubuntu 22.04 LTS (recommended) or compatible Linux distribution

### Why Fast Storage?

Zattera handles a high volume of transactions per second. Fast SSD storage ensures:
- Efficient blockchain synchronization
- Quick account history queries
- Responsive API performance
- Reduced latency for deposit/withdrawal detection

## Integration Methods

We recommend **Docker** for running Zattera nodes. Docker ensures:
- Consistent build and runtime environment
- Easy deployment and updates
- Isolation from host system
- Simplified configuration management

Alternative: You can build from source and run natively (see [Building Guide](building.md)).

## Quick Start with Docker

### Step 1: Install Docker

#### Ubuntu 22.04
```bash
sudo apt-get update
sudo apt-get install -y docker.io docker-compose
sudo systemctl enable --now docker
sudo usermod -aG docker $USER  # Log out and back in after this
```

#### Other Linux Distributions
```bash
curl -fsSL https://get.docker.com -o get-docker.sh
sh get-docker.sh
sudo usermod -aG docker $USER
```

### Step 2: Get Docker Image

Choose one of the following options:

#### Option A: Use Pre-built Image (Recommended)

Fastest and easiest method for production use:

```bash
docker pull zatterahub/zattera:stable
```

#### Option B: Build from Source

For custom modifications or code verification:

```bash
# Install Git
sudo apt-get install -y git  # Ubuntu/Debian

# Clone repository
git clone https://github.com/zattera-protocol/zattera.git
cd zattera
git checkout stable

# Build image
docker build -t zatterahub/zattera:stable .
```

**Build time:** 30 minutes to 2 hours depending on hardware.

### Step 3: Prepare Data Directories

Create directories for persistent blockchain and wallet data:

```bash
mkdir -p ~/zattera-data/blockchain
mkdir -p ~/zattera-data/wallet
chmod -R 755 ~/zattera-data
```

### Step 4: Configure for Exchange Use

Create a configuration file optimized for exchange operations.

**If you built from source (Option B):**
```bash
cp configs/fullnode.config.ini ~/zattera-data/config.ini
```

**If you used pre-built image (Option A):**
```bash
# Download example configuration
curl -o ~/zattera-data/config.ini \
  https://raw.githubusercontent.com/zattera-protocol/zattera/stable/configs/fullnode.config.ini
```

Edit `~/zattera-data/config.ini` and configure these key settings:

```ini
# === Plugins (Required for Exchange) ===
plugin = webserver p2p json_rpc witness
plugin = database_api account_by_key_api network_broadcast_api
plugin = block_api account_history_api

# === Track Specific Accounts ===
# Add your exchange's deposit accounts here
# Format: account-history-track-account-range = ["account1","account1"]
# Or track all accounts (high memory usage):
# account-history-track-account-range = []

# Example: Track specific exchange accounts
account-history-track-account-range = ["exchange-deposit","exchange-deposit"]
account-history-track-account-range = ["exchange-hot-wallet","exchange-hot-wallet"]

# === Whitelist Operations (Recommended) ===
# Only track financially relevant operations to save memory
account-history-whitelist-ops = transfer_operation transfer_to_vesting_operation withdraw_vesting_operation interest_operation transfer_to_savings_operation transfer_from_savings_operation cancel_transfer_from_savings_operation escrow_transfer_operation escrow_approve_operation escrow_dispute_operation escrow_release_operation fill_convert_request_operation fill_order_operation claim_reward_balance_operation

# === Shared Memory Configuration ===
shared-file-dir = blockchain
shared-file-size = 80G  # Adjust based on available RAM

# === Network Configuration ===
# P2P endpoint
p2p-endpoint = 0.0.0.0:2001

# RPC endpoints
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090

# Seed nodes (will auto-discover more peers)
# See docs/seednodes.txt for current seed node list
```

### Step 5: Run the Node

```bash
docker run -d \
    --name zattera-exchange \
    --restart unless-stopped \
    -p 2001:2001 \
    -p 8090:8090 \
    -v ~/zattera-data/blockchain:/var/lib/zatterad/blockchain \
    -v ~/zattera-data/config.ini:/etc/zatterad/config.ini \
    zatterahub/zattera:stable
```

### Step 6: Monitor Synchronization

```bash
# Follow the logs
docker logs -f zatterad-exchange

# Check sync status
docker exec zatterad-exchange \
    curl -s http://localhost:8090 -d '{"jsonrpc":"2.0","method":"database_api.get_dynamic_global_properties","params":{},"id":1}' | jq
```

**Initial Sync Time**: 6-48 hours depending on hardware (faster with SSD).

The node is synchronized when `head_block_number` matches the current blockchain height.

## Using the CLI Wallet

The CLI wallet connects to your running node for account management and transaction signing.

### Start CLI Wallet

```bash
docker exec -it zatterad-exchange \
    /usr/local/bin/cli_wallet -s ws://localhost:8090 -w /var/zattera-wallet/wallet.json
```

### Basic Wallet Commands

```
# Set a password for the wallet
>>> set_password YOUR_SECURE_PASSWORD

# Unlock the wallet
>>> unlock YOUR_SECURE_PASSWORD

# Import your exchange's private keys
>>> import_key ACCOUNT_NAME PRIVATE_POSTING_KEY
>>> import_key ACCOUNT_NAME PRIVATE_ACTIVE_KEY

# Get account info
>>> get_account ACCOUNT_NAME

# Check balance
>>> list_my_accounts

# Transfer ZTR
>>> transfer FROM_ACCOUNT TO_ACCOUNT "100.000 ZTR" "memo" true

# Get help
>>> help
>>> gethelp transfer
```

## Exchange Integration Workflow

### 1. Generate Deposit Addresses

Zattera uses account names (not addresses). For each user:

1. **Option A**: Create unique memo identifiers
   - Single exchange account: `exchange-deposit`
   - Each user gets unique memo: `user123`, `user456`, etc.
   - Deposits include memo in transfer

2. **Option B**: Create sub-accounts
   - Each user gets unique account: `exchange.user123`, `exchange.user456`
   - Requires account creation fee per user

**Recommended**: Option A (memo-based) is more cost-effective.

### 2. Monitor Deposits

Use the `account_history_api` to track incoming transfers:

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "account_history_api.get_account_history",
  "params": {
    "account": "exchange-deposit",
    "start": -1,
    "limit": 100
  },
  "id": 1
}' | jq
```

Filter for `transfer_operation` and check:
- `to` field matches your deposit account
- `amount` field for deposit amount
- `memo` field for user identifier
- Confirm transaction is irreversible (wait for block confirmation)

### 3. Process Withdrawals

When users withdraw ZTR:

```bash
# Using cli_wallet
>>> transfer exchange-hot-wallet user-account "50.000 ZTR" "Withdrawal ID: 12345" true
```

Or via JSON-RPC:

```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "network_broadcast_api.broadcast_transaction",
  "params": {
    "trx": {
      "operations": [[
        "transfer",
        {
          "from": "exchange-hot-wallet",
          "to": "user-account",
          "amount": "50.000 ZTR",
          "memo": "Withdrawal ID: 12345"
        }
      ]],
      "signatures": []
    }
  },
  "id": 1
}'
```

**Important**: Sign transactions with your active key before broadcasting.

### 4. Confirmations

Zattera uses DPoS with 21 witnesses producing blocks every 3 seconds.

**Recommended confirmations**:
- **Small amounts**: 1 confirmation (3 seconds)
- **Medium amounts**: 15 confirmations (~45 seconds)
- **Large amounts**: 30+ confirmations (~90 seconds)

Blocks become irreversible after 2/3+ witness consensus.

## Account History Options

### Track Specific Accounts (Recommended for Exchanges)

Most efficient for exchanges - only track your own accounts:

```ini
account-history-track-account-range = ["exchange-deposit","exchange-deposit"]
account-history-track-account-range = ["exchange-hot-wallet","exchange-hot-wallet"]
account-history-track-account-range = ["exchange-cold-wallet","exchange-cold-wallet"]
```

### Track All Accounts (High Memory)

For full account history API access:

```ini
# Empty array = track all accounts
account-history-track-account-range = []
```

This requires significantly more RAM (~32GB+).

## Security Best Practices

### 1. Key Management

- **Never** store private keys in plain text
- Use separate keys for different security levels:
  - **Posting key**: Social operations (not needed for exchange)
  - **Active key**: Transfers and financial operations
  - **Owner key**: Account recovery (store offline in cold storage)
- Keep active keys in encrypted wallet only
- Store owner keys offline in multiple secure locations

### 2. Hot/Cold Wallet Strategy

- **Hot wallet**: Minimal balance for daily withdrawals (online)
- **Cold wallet**: Majority of funds (offline, air-gapped)
- Regularly sweep hot wallet excess to cold storage
- Use multi-signature if possible

### 3. Network Security

- Run node behind firewall
- Only expose necessary ports (8090 for RPC)
- Use HTTPS reverse proxy for RPC (see [Reverse Proxy Guide](../operations/reverse-proxy-guide.md))
- Implement rate limiting and DDoS protection
- Use VPN or SSH tunnels for administrative access

### 4. Monitoring

- Monitor node synchronization status
- Alert on node downtime or sync issues
- Track unusual transaction patterns
- Log all API access for audit trail

## API Reference

### Essential APIs for Exchanges

| API | Purpose |
|-----|---------|
| `database_api` | Query blockchain state, accounts, balances |
| `account_history_api` | Track account transaction history |
| `network_broadcast_api` | Submit signed transactions |
| `block_api` | Query block data and confirmations |
| `account_by_key_api` | Find accounts by public key |

### Common API Calls

#### Get Account Balance
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "database_api.find_accounts",
  "params": {"accounts": ["exchange-deposit"]},
  "id": 1
}'
```

#### Get Transaction History
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "account_history_api.get_account_history",
  "params": {
    "account": "exchange-deposit",
    "start": -1,
    "limit": 100
  },
  "id": 1
}'
```

#### Get Block
```bash
curl -s http://localhost:8090 -d '{
  "jsonrpc": "2.0",
  "method": "block_api.get_block",
  "params": {"block_num": 12345},
  "id": 1
}'
```

## Troubleshooting

### Node Won't Sync

1. Check internet connection and firewall
2. Verify seed nodes are accessible (see `docs/seednodes.txt`)
3. Check disk space (need 120GB+ free)
4. Review logs: `docker logs zatterad-exchange`

### Out of Memory Errors

1. Reduce `shared-file-size` in config
2. Enable operation whitelisting (track only financial ops)
3. Track specific accounts instead of all accounts
4. Increase system RAM or swap space

### Slow API Responses

1. Ensure using SSD storage
2. Reduce tracked account scope
3. Use operation whitelisting
4. Add more RAM for caching
5. Consider using RocksDB plugin for account history

### Transaction Not Confirming

1. Check transaction was properly signed
2. Verify account has sufficient balance
3. Check network connectivity
4. Review error messages in logs
5. Ensure node is fully synchronized

## Advanced Configuration

### Using RocksDB for Account History

For very high transaction volumes, use RocksDB plugin instead of in-memory account history:

```ini
plugin = account_history_rocksdb account_history_rocksdb_api

# RocksDB path
account-history-rocksdb-path = "blockchain/account-history-rocksdb-storage"
```

Benefits:
- Lower memory usage
- Better performance for high-volume exchanges
- Persistent storage of account history

### Running Behind Nginx Reverse Proxy

See [Reverse Proxy Guide](../operations/reverse-proxy-guide.md) for:
- HTTPS/SSL configuration
- Load balancing
- Rate limiting
- DDoS protection

### Backup and Disaster Recovery

**What to Backup**:
1. Wallet file (`wallet.json`) - **Critical**
2. Private keys (encrypted) - **Critical**
3. Configuration file (`config.ini`)
4. Blockchain data (optional - can re-sync from network)

**Backup Strategy**:
```bash
# Backup wallet and keys (encrypted)
tar -czf zattera-backup-$(date +%Y%m%d).tar.gz \
    ~/zattera-data/wallet/ \
    ~/zattera-data/config.ini

# Store backup in multiple secure locations
# - Encrypted cloud storage
# - Offline storage media
# - Separate physical location
```

## Getting Help

- **Documentation**: [docs/](../)
- **GitHub Issues**: [github.com/zattera-protocol/zattera/issues](https://github.com/zattera-protocol/zattera/issues)
- **Technical Questions**: Use GitHub Discussions

## Additional Resources

- [Quick Start Guide](quick-start.md) - Get started quickly with Docker
- [Building Guide](building.md) - Build from source on different platforms
- [CLI Wallet Guide](cli-wallet-guide.md) - Complete wallet usage guide
- [Node Modes Guide](../operations/node-modes-guide.md) - Different node types and configurations
- [Reverse Proxy Guide](../operations/reverse-proxy-guide.md) - Production deployment with NGINX
- [Configuration Examples](../../configs/README.md) - Ready-to-use configuration files

## Token Information

- **Token Symbol**: ZTR
- **Precision**: 3 decimal places (0.001 ZTR minimum)
- **Format**: `"100.000 ZTR"` (always 3 decimals, space before symbol)
- **Consensus**: Delegated Proof of Stake (21 witnesses)
- **Block Time**: 3 seconds
- **Inflation**: 10% APR narrowing to 1% APR over 20 years

## Support

For exchange integration support:
1. Review this guide thoroughly
2. Check existing GitHub issues
3. Open new issue with detailed information:
   - Node logs
   - Configuration file (remove sensitive data)
   - Specific error messages
   - Steps to reproduce
