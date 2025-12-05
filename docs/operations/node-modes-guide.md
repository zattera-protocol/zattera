# Zattera Node Modes Guide

Complete reference for different types of Zattera nodes and their configurations.

## Overview

Zattera nodes can be configured for different purposes by enabling specific plugins and adjusting resource allocations. This guide describes the main node types, their use cases, and how to configure them.

**Key Principle**: A node's type is determined by its **plugin configuration**, not the binary itself. The same `zatterad` binary can be configured as any node type.

## Node Type Comparison

| Node Type | Memory | Use Case | APIs | Block Production |
|-----------|--------|----------|------|------------------|
| **Witness Node** | 54GB | Block production | Minimal | Yes |
| **Seed Node** | 4GB | P2P relay | None | No |
| **Full Node** | 260GB+ | Public API | All | No |
| **Account History Node** | 70GB+ | Account queries | Account history | No |
| **Broadcast Node** | 24GB | Transaction submission | Network broadcast | No |
| **Testnet Node** | 30GB | Development/testing | Full | Yes (genesis) |

## Witness Node

**Purpose**: Produce blocks and validate transactions for the Zattera blockchain.

### Characteristics

- **Primary function**: Block production and validation
- **Memory**: 54GB shared memory
- **Plugins**: Minimal (witness, basic APIs)
- **Network**: P2P for block propagation
- **Reliability**: Mission-critical, requires 24/7 uptime

### Configuration

**Config file**: [configs/witness.config.ini](../../configs/witness.config.ini)

```ini
# Logging
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"info","appender":"stderr"}

# Plugins - minimal set for consensus
plugin = chain p2p webserver witness
plugin = database_api witness_api

# Storage
shared-file-size = 54G
shared-file-dir = blockchain
flush-state-interval = 0

# Network
p2p-endpoint = 0.0.0.0:2001
p2p-seed-node = seed.zattera.com:2001

# Witness configuration
enable-stale-production = false
required-participation = 33
witness = "your-witness-account"
private-key = 5K...  # Your witness signing key
```

### Requirements

- **Hardware**:
  - CPU: 4+ cores
  - RAM: 64GB+ (54GB shared memory + system overhead)
  - Storage: 100GB+ SSD
  - Network: Stable, low-latency connection

- **Software**:
  - Build: Standard build (`BUILD_ZATTERA_TESTNET=OFF`)
  - OS: Linux (Ubuntu 20.04/22.04 recommended)

### Use Cases

- **Block production**: Sign and produce blocks when scheduled
- **Transaction validation**: Validate and broadcast transactions
- **Consensus participation**: Vote on blockchain parameters

### Best Practices

1. **Redundancy**: Run backup witness on separate infrastructure
2. **Monitoring**: Alert on missed blocks, high latency
3. **Security**: Firewall all ports except P2P (2001)
4. **Key management**: Store signing keys securely (hardware wallet/HSM)
5. **Price feed**: Regularly update SBD price feed (if top 20 witness)

### Docker Deployment

```bash
docker run -d \
  --name zattera-witness \
  -v witness-data:/var/lib/zatterad \
  -v $(pwd)/witness.config.ini:/var/lib/zatterad/config.ini:ro \
  -p 2001:2001 \
  --restart=unless-stopped \
  zatterahub/zattera:latest
```

## Seed Node

**Purpose**: Provide P2P connectivity and block relay without storing full state.

### Characteristics

- **Primary function**: P2P network relay
- **Memory**: 4GB shared memory (LOW_MEMORY_NODE build)
- **Plugins**: P2P only, no APIs
- **Build**: Requires `LOW_MEMORY_NODE=ON`
- **Minimal resource usage**

### Configuration

```ini
# Logging (minimal)
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"warn","appender":"stderr"}

# Plugins - P2P only
plugin = chain p2p

# Storage - minimal
shared-file-size = 4G

# Network
p2p-endpoint = 0.0.0.0:2001
p2p-max-connections = 200
p2p-seed-node = seed1.example.com:2001
p2p-seed-node = seed2.example.com:2001
```

### Build Requirements

Must be built with `LOW_MEMORY_NODE=ON`:

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DLOW_MEMORY_NODE=ON \
      ..
make -j$(nproc) zatterad
```

### Use Cases

- **P2P bootstrapping**: Help new nodes discover peers
- **Block relay**: Propagate blocks across network
- **Geographic distribution**: Provide low-latency peers in different regions

### Best Practices

1. **High bandwidth**: Ensure sufficient upload capacity
2. **Many connections**: Set `p2p-max-connections` high (200+)
3. **Global distribution**: Deploy across multiple regions
4. **Minimal logging**: Reduce disk I/O

## Full Node (Full API Node)

**Purpose**: Provide complete API access for applications, exchanges, and explorers.

### Characteristics

- **Primary function**: Serve all blockchain data via APIs
- **Memory**: 260GB+ shared memory (continuously growing)
- **Plugins**: All state-tracking and API plugins
- **Network**: API endpoints (WebSocket/HTTP)
- **High resource requirements**

### Configuration

**Config file**: [configs/fullnode.config.ini](../../configs/fullnode.config.ini)

```ini
# Logging
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"debug","appender":"stderr"}

# All state-tracking plugins
plugin = webserver p2p json_rpc witness
plugin = account_by_key tags follow market_history

# All API plugins
plugin = database_api account_by_key_api network_broadcast_api
plugin = tags_api follow_api market_history_api witness_api
plugin = block_api

# Storage - large
shared-file-size = 260G

# Network
p2p-endpoint = 0.0.0.0:2001
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090
webserver-thread-pool-size = 256

# Optimizations
follow-max-feed-size = 500
market-history-bucket-size = [15,60,300,3600,86400]
market-history-buckets-per-size = 5760
```

### Requirements

- **Hardware**:
  - CPU: 8+ cores
  - RAM: 300GB+ (260GB shared memory + OS + buffers)
  - Storage: 400GB+ SSD
  - Network: High bandwidth for API traffic

- **Software**:
  - Build: Standard build
  - Reverse proxy: NGINX/HAProxy recommended for production

### Enabled APIs

- `database_api` - Core blockchain data
- `account_by_key_api` - Account lookups by public key
- `network_broadcast_api` - Transaction broadcasting
- `tags_api` - Content tags and trending
- `follow_api` - Follow relationships and feeds
- `market_history_api` - Internal market OHLCV data
- `witness_api` - Witness information
- `block_api` - Block data queries
- `account_history_api` - Account transaction history

### Use Cases

- **Application backends**: Power social media dapps
- **Blockchain explorers**: Display full blockchain data
- **Analytics platforms**: Query historical data
- **Content platforms**: Serve posts, comments, votes

### Best Practices

1. **Use reverse proxy**: NGINX for rate limiting, caching, TLS
2. **Monitor resource usage**: Watch RAM, disk, network
3. **Regular backups**: Backup shared memory and block log
4. **Rate limiting**: Prevent API abuse
5. **Load balancing**: Multiple nodes behind load balancer for high traffic

### Docker Deployment

```bash
docker run -d \
  --name zattera-fullnode \
  -e USE_HIGH_MEMORY=1 \
  -e ZATTERAD_NODE_MODE=fullnode \
  -v fullnode-data:/var/lib/zatterad \
  -p 8090:8090 \
  -p 2001:2001 \
  --shm-size=280g \
  --restart=unless-stopped \
  zatterahub/zattera:latest
```

## Account History Node

**Purpose**: Specialized node for querying account transaction history.

### Characteristics

- **Primary function**: Account transaction history queries
- **Memory**: 70GB+ shared memory
- **Plugins**: `account_history_rocksdb` (disk-based, more efficient)
- **Storage**: Higher disk usage, lower memory usage vs full node

### Configuration

**Config file**: [configs/ahnode.config.ini](../../configs/ahnode.config.ini)

```ini
# Logging
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"debug","appender":"stderr"}

# Plugins - account history focused
plugin = webserver p2p json_rpc account_history_rocksdb
plugin = database_api account_history_api

# Storage
shared-file-size = 70G

# Network
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090
webserver-thread-pool-size = 32

# Account history configuration
# Track all accounts (default)
# account-history-track-account-range = ["a", "z"]

# Or track specific accounts
# account-history-track-account-range = ["alice", "alice"]
# account-history-track-account-range = ["bob", "bob"]
```

### Plugin Comparison

**Traditional account_history** (memory-based):
- Stores all data in shared memory
- Faster queries
- Higher memory usage (~200GB+)

**account_history_rocksdb** (disk-based):
- Stores data in RocksDB on disk
- Slightly slower queries
- Much lower memory usage (~70GB)
- Recommended for most use cases

### Selective Tracking

Track only specific operations to reduce storage:

```ini
# Whitelist - only track these operations
account-history-whitelist-ops = transfer_operation
account-history-whitelist-ops = transfer_to_vesting_operation
account-history-whitelist-ops = withdraw_vesting_operation
account-history-whitelist-ops = claim_reward_balance_operation

# Blacklist - ignore these operations
account-history-blacklist-ops = vote_operation
account-history-blacklist-ops = custom_json_operation
```

### Use Cases

- **Exchange integration**: Track deposit/withdrawal history
- **Wallet backends**: Display user transaction history
- **Account analytics**: Analyze account activity
- **Compliance/auditing**: Track financial transactions

### Best Practices

1. **Use RocksDB plugin**: Lower memory, similar performance
2. **Selective tracking**: Whitelist only needed operations for exchanges
3. **SSD storage**: RocksDB benefits from fast I/O
4. **Regular compaction**: RocksDB requires periodic compaction

## Broadcast Node

**Purpose**: Lightweight node dedicated to transaction broadcasting.

### Characteristics

- **Primary function**: Accept and broadcast transactions
- **Memory**: 24GB shared memory (minimal)
- **Plugins**: `network_broadcast_api` only
- **Lightweight**: Lowest resource requirements

### Configuration

**Config file**: [configs/broadcast.config.ini](../../configs/broadcast.config.ini)

```ini
# Logging
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"info","appender":"stderr"}

# Minimal plugins
plugin = webserver p2p json_rpc
plugin = database_api network_broadcast_api

# Storage - minimal
shared-file-size = 24G

# Network
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090
```

### Use Cases

- **High-volume transaction submission**: Exchanges, bots
- **Cost-effective broadcasting**: Minimal infrastructure
- **Transaction relay**: Forward transactions to network

### Best Practices

1. **Load balancing**: Multiple broadcast nodes for redundancy
2. **Monitoring**: Track transaction success rates
3. **Rate limiting**: Prevent spam/abuse

## Testnet Node

**Purpose**: Development and testing on private or public testnets.

### Characteristics

- **Primary function**: Development environment
- **Memory**: 30GB shared memory
- **Plugins**: Full plugin set for testing
- **Build**: Requires `BUILD_ZATTERA_TESTNET=ON`

### Configuration

**Config file**: [configs/test.config.ini](../../configs/test.config.ini)

```ini
# Logging - verbose for debugging
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"debug","appender":"stderr"}

# Full plugin set
plugin = webserver p2p json_rpc witness account_by_key tags follow market_history account_history
plugin = database_api account_by_key_api network_broadcast_api tags_api follow_api market_history_api witness_api account_history_api

# Storage
shared-file-size = 30G

# Network
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090

# Testnet-specific settings
enable-stale-production = true
required-participation = 0

# Genesis witness
witness = "genesis"
private-key = 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n
```

### Build Requirements

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_ZATTERA_TESTNET=ON \
      ..
make -j$(nproc) zatterad
```

### Use Cases

- **Application development**: Test apps without real ZATTERA
- **Protocol testing**: Test hardforks and upgrades
- **Plugin development**: Test custom plugins
- **Integration testing**: CI/CD pipelines

### Best Practices

1. **Separate infrastructure**: Don't mix testnet and mainnet
2. **Genesis configuration**: Customize genesis for specific tests
3. **Reset regularly**: Fresh testnet for clean tests
4. **Mock data**: Populate with realistic test data

## Specialized Node Configurations

### Exchange Node

Optimized for cryptocurrency exchange operations.

```ini
# Plugins - focused on transfers and account history
plugin = webserver p2p json_rpc account_history_rocksdb
plugin = database_api account_history_api network_broadcast_api

# Track only exchange accounts
account-history-track-account-range = ["exchange-deposit", "exchange-deposit"]
account-history-track-account-range = ["exchange-hot", "exchange-hot"]
account-history-track-account-range = ["exchange-cold", "exchange-cold"]

# Whitelist only financial operations
account-history-whitelist-ops = transfer_operation
account-history-whitelist-ops = transfer_to_vesting_operation
account-history-whitelist-ops = withdraw_vesting_operation
account-history-whitelist-ops = transfer_to_savings_operation
account-history-whitelist-ops = transfer_from_savings_operation
account-history-whitelist-ops = fill_vesting_withdraw_operation

# Storage
shared-file-size = 70G
```

See [docs/exchange-quick-start.md](../exchange-quick-start.md) for detailed exchange integration.

### Content Platform Node

Optimized for social media applications.

```ini
# Plugins - focused on content and social features
plugin = webserver p2p json_rpc
plugin = tags follow account_by_key
plugin = database_api tags_api follow_api account_by_key_api network_broadcast_api

# Follow plugin settings
follow-max-feed-size = 500
follow-start-feeds = 0  # Track feeds from genesis

# Storage
shared-file-size = 200G
```

### Analytics Node

Optimized for blockchain data analysis.

```ini
# All plugins for complete data
plugin = webserver p2p json_rpc
plugin = account_history_rocksdb tags follow market_history
plugin = database_api account_history_api tags_api follow_api market_history_api block_api

# Market history - full resolution
market-history-bucket-size = [15,60,300,3600,86400]
market-history-buckets-per-size = 10000  # More history

# Account history - all operations
# (no whitelist/blacklist)

# Storage
shared-file-size = 150G
```

## Node Selection Guide

### Choose Your Node Type

**Question**: What is your primary use case?

1. **I want to produce blocks as a witness**
   → **Witness Node** (54GB RAM, high reliability)

2. **I want to provide public API access**
   → **Full Node** (300GB+ RAM, reverse proxy recommended)

3. **I run an exchange and need deposit/withdrawal tracking**
   → **Account History Node** with selective tracking (70GB RAM)

4. **I need to relay blocks on the P2P network**
   → **Seed Node** (4GB RAM, special build required)

5. **I need to broadcast many transactions**
   → **Broadcast Node** (24GB RAM, lightweight)

6. **I'm developing/testing applications**
   → **Testnet Node** (30GB RAM, special build)


### Resource Planning

| Node Type | CPU | RAM | Storage | Network |
|-----------|-----|-----|---------|---------|
| Witness | 4+ cores | 64GB | 100GB SSD | Stable, low-latency |
| Seed | 2+ cores | 8GB | 50GB | High bandwidth |
| Full Node | 8+ cores | 300GB | 400GB SSD | High bandwidth |
| Account History | 4+ cores | 80GB | 300GB SSD | Medium bandwidth |
| Broadcast | 2+ cores | 32GB | 100GB | Medium bandwidth |
| Testnet | 4+ cores | 40GB | 100GB | Low bandwidth |

### Scaling Strategies

**Horizontal Scaling** (recommended):
- Run multiple specialized nodes
- Use load balancer to distribute traffic
- Example: 2x Full Node + 1x Account History Node + 1x Broadcast Node

**Vertical Scaling**:
- Single full-featured node
- Requires high-end hardware (32+ cores, 512GB+ RAM)
- More expensive, single point of failure

## Migration Between Node Types

### Changing Node Configuration

Changing plugins requires different approaches based on plugin type:

**API-only plugins** (no replay needed):
- `database_api`, `witness_api`, `network_broadcast_api`, etc.
- Just add/remove from config and restart

**State-tracking plugins** (replay required):
- `account_history`, `tags`, `follow`, `market_history`
- Requires full blockchain replay

### Replay Process

```bash
# Stop node
sudo systemctl stop zatterad

# Update config.ini with new plugins
vim witness_node_data_dir/config.ini

# Replay blockchain (can take hours/days)
./programs/zatterad/zatterad --replay-blockchain --data-dir=witness_node_data_dir

# Or delete state and resync from P2P
rm -rf witness_node_data_dir/blockchain
./programs/zatterad/zatterad --data-dir=witness_node_data_dir
```

### Downgrade Example: Full Node → Witness Node

```bash
# 1. Backup current state (optional)
cp -r witness_node_data_dir/blockchain /backup/

# 2. Update config.ini - remove unnecessary plugins
# Before:
# plugin = tags follow market_history account_history
# After:
# (remove above line)

# 3. Restart (no replay needed if removing plugins)
sudo systemctl restart zatterad
```

### Upgrade Example: Witness Node → Full Node

```bash
# 1. Stop node
sudo systemctl stop zatterad

# 2. Update config.ini - add plugins
# plugin = tags follow market_history account_history
# plugin = tags_api follow_api market_history_api account_history_api

# 3. Increase shared-file-size
# shared-file-size = 260G

# 4. Replay blockchain (REQUIRED for state-tracking plugins)
./programs/zatterad/zatterad --replay-blockchain --data-dir=witness_node_data_dir
```

## Monitoring and Maintenance

### Key Metrics by Node Mode

**All Nodes**:
- Block height (should match network)
- P2P connections (5-30 active)
- Memory usage
- Disk I/O

**Witness Node**:
- Missed blocks (should be 0)
- Block production time (within slot)
- Network latency to peers

**Full Node / API Nodes**:
- API request rate
- API response time
- WebSocket connections
- Query queue depth

### Health Checks

```bash
# Check sync status
curl -s -X POST http://localhost:8090 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","id":1,"method":"database_api.get_dynamic_global_properties"}' \
  | jq -r '.result | "\(.head_block_number) \(.time)"'

# Check plugin status
curl -s http://localhost:8090 | jq

# Monitor logs
tail -f witness_node_data_dir/logs/zatterad.log

# Check resource usage
docker stats zattera-node
# or
htop
```

### Backup Strategies

**Witness Node**:
- Backup signing keys (critical)
- Backup config.ini
- Optional: backup shared memory file for faster recovery

**Full Node / API Nodes**:
- Regular snapshots of shared memory file
- Backup RocksDB data (if using account_history_rocksdb)
- Backup config.ini

**All Nodes**:
- Block log is reproducible (can resync from network)
- Shared memory file can be regenerated via replay

## Additional Resources

- [Quick Start Guide](../getting-started/quick-start.md) - Get started quickly with Docker
- [Building Guide](../getting-started/building.md) - Build instructions for all platforms
- [Exchange Quick Start Guide](../getting-started/exchange-quick-start.md) - Exchange integration guide
- [Genesis Launch Guide](genesis-launch.md) - Launch your own Zattera network
- [Reverse Proxy Guide](reverse-proxy-guide.md) - Production deployment with NGINX
- [Configuration Examples](../../configs/README.md) - Ready-to-use configuration files

## License

See [LICENSE.md](../LICENSE.md)
