# Genesis Launch Guide

How to launch a new Zattera blockchain from genesis (block 0).

## Overview

This guide covers the complete process of launching a new Zattera blockchain, including:
- Configuring genesis parameters
- Creating the initial witness
- Starting block production
- Bootstrapping the network
- Adding additional witnesses

**Use Cases**:
- Private testnet deployment
- Development environment
- Blockchain forks
- Custom Zattera-based chains

## Prerequisites

### Build Requirements

Build `zatterad` for your target network. For detailed build instructions including dependency installation and platform-specific requirements, see [Building Guide](../getting-started/building.md).

#### For Testnet (Development/Testing)

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_ZATTERA_TESTNET=ON \
      ..
make -j$(nproc) zatterad
```

#### For Mainnet (Production/Custom Chain)

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_ZATTERA_TESTNET=OFF \
      ..
make -j$(nproc) zatterad
```

**Note**: Both testnet and mainnet builds can launch genesis chains. The key difference is in genesis parameters:
- **Testnet**: auto-generated keys, "TST" prefix, initial witness must be named "genesis"
- **Mainnet**: fixed public key, "ZTR" prefix

For complete build instructions including system requirements, dependency installation, and troubleshooting, refer to [Building Guide](../getting-started/building.md).

### System Requirements

- **RAM**: 4GB minimum
- **Disk**: 10GB available for blockchain data
- **Network**: P2P port accessible (default: 2001)

## Genesis Configuration

### Chain Parameters

Genesis parameters are configured at build time in `src/core/protocol/include/zattera/protocol/config.hpp`.

#### Testnet Configuration

When building with `BUILD_ZATTERA_TESTNET=ON`:

- **Chain ID**: "testnet"
- **Address Prefix**: "TST"
- **Genesis Time**: 2016-01-01 00:00:00 UTC
- **Initial Supply**: 250,000,000 ZTR
- **Initial Account**: "genesis"
- **Private Key**: Auto-generated from "init_key" seed
  - WIF: `5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n`
  - Public Key: `TST6LLegbAgLAy28EHrffBVuANFWcFgmqRMW13wBmTExqFE9SCkg4`

#### Mainnet Configuration

For mainnet builds (`BUILD_ZATTERA_TESTNET=OFF`, default):

- **Chain ID**: "" (empty)
- **Address Prefix**: "ZTR"
- **Genesis Time**: 2016-03-24 16:00:00 UTC
- **Initial Supply**: 0 ZTR
- **Initial Public Key**: `ZTR8GC13uCZbP44HzMLV6zPZGwVQ8Nt4Kji8PapsPiNq1BK153XTX`

**Important for Custom Chains**:
- Modify `ZATTERA_CHAIN_ID_NAME` to make it unique (prevents replay attacks)
- Set `ZATTERA_INIT_SUPPLY` if you want pre-mined tokens
- Generate your own keys for the initial witness

## Genesis Initialization Process

When `zatterad` starts with an empty database, it automatically creates the genesis state with:

- **System Accounts**: `null` (burn), `temp` (temporary operations)
- **Initial Account**: `genesis` receives all initial supply (250M ZTR for testnet)
- **Initial Witness**: `genesis` configured for block production
- **Global Properties**: Initial blockchain state and parameters
- **Hardfork State**: Genesis hardfork timestamp
- **Reward Funds**: Post and curation reward pools

## Launching Your Genesis Chain

### Step 1: Prepare Configuration

Create `config.ini` for your genesis node:

```ini
# Logging
log-console-appender = {"appender":"stderr","stream":"std_error"}
log-logger = {"name":"default","level":"info","appender":"stderr"}

# Required plugins
plugin = chain p2p webserver witness

# API plugins (optional, for development)
plugin = database_api witness_api

# Network settings
p2p-endpoint = 0.0.0.0:2001
webserver-http-endpoint = 0.0.0.0:8090
webserver-ws-endpoint = 0.0.0.0:8090

# Witness configuration
enable-stale-production = true
required-participation = 0

# Name of witness controlled by this node (e.g. genesis)
witness = "genesis"

# WIF PRIVATE KEY to be used by witnesses
private-key = 5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n
```

**Note**: For testnet, the genesis private key is always `5JNHfZYKGaomSFvd4NUdQ9qMcEAC43kujbfjueTHpVapX1Kzq2n`

### Step 2: Launch Genesis Node

```bash
# Create data directory
mkdir -p witness_node_data_dir

# Copy config
cp config.ini witness_node_data_dir/

# Start zatterad (will create genesis on first run)
./programs/zatterad/zatterad \
    --data-dir=witness_node_data_dir \
    2>&1 | tee genesis.log
```

### Step 3: Verify Genesis

Check the logs for genesis initialization:

```
info  database: Open database in /path/to/witness_node_data_dir/blockchain
info  database: Creating genesis state
info  database: Genesis state created successfully
info  witness: Witness genesis starting block production
info  witness: Generated block #1 with timestamp 2016-01-01T00:00:03
```

### Step 4: Query Genesis State

Using `cli_wallet` or API calls:

```bash
# Connect to node
./programs/cli_wallet/cli_wallet -s ws://127.0.0.1:8090

# Check genesis account
>>> get_account genesis

# Check witness
>>> get_witness genesis

# Check blockchain info
>>> info
```

Expected output:

```json
{
  "head_block_num": 1,
  "head_block_id": "0000000109b3667c257e7171da61984da0d3279f",
  "time": "2016-01-01T00:00:03",
  "current_witness": "genesis",
  "total_supply": "250000000.000 ZTR",
  "current_supply": "250000000.000 ZTR"
}
```

## Adding Witnesses

### Create New Witness Account

```bash
# In cli_wallet
>>> create_account genesis "witness1" "" true

# Fund the account
>>> transfer genesis witness1 "1000.000 ZTR" "Initial funding" true

# Generate witness keys
>>> suggest_brain_key

# Update witness
>>> update_witness "witness1" \
    "https://witness1.example.com" \
    "ZTR..." \  # signing key from suggest_brain_key
    {"account_creation_fee": "0.100 ZTR", "maximum_block_size": 65536} \
    true
```

### Vote for Witness

```bash
# Vote with genesis account
>>> vote_for_witness genesis witness1 true true
```

### Add to Config

Add witness to `config.ini`:

```ini
witness = "genesis"
witness = "witness1"

# Add private key for witness1
private-key = 5K...  # WIF from suggest_brain_key
```

### Restart Node

```bash
# Restart zatterad to pick up new witness
pkill zatterad
./programs/zatterad/zatterad --data-dir=witness_node_data_dir
```

## Multi-Node Network

### Bootstrap Additional Nodes

#### Node 2 Configuration

```ini
# config.ini for node 2
p2p-endpoint = 0.0.0.0:2001
p2p-seed-node = 127.0.0.1:2001  # Point to genesis node

# Sync-only node (no witness)
enable-stale-production = false

# API access
webserver-http-endpoint = 0.0.0.0:8091
webserver-ws-endpoint = 0.0.0.0:8091

plugin = chain p2p webserver
plugin = database_api
```

#### Launch Node 2

```bash
mkdir -p node2_data
cp config_node2.ini node2_data/config.ini
./programs/zatterad/zatterad --data-dir=node2_data
```

Node 2 will sync from genesis node and replay all blocks.

### Network Topology

```
Genesis Node (genesis)
    ├── Port 2001: P2P
    ├── Port 8090: API
    └── Witness: genesis

Peer Node 2
    ├── Port 2001: P2P (connects to genesis)
    ├── Port 8091: API
    └── Sync only

Witness Node 3
    ├── Port 2001: P2P (connects to genesis)
    ├── Port 8092: API
    └── Witness: witness1
```

## Genesis Parameters Reference

### Time Constants

| Parameter | Testnet | Mainnet | Description |
|-----------|---------|---------|-------------|
| `ZATTERA_GENESIS_TIME` | 2016-01-01 00:00:00 | 2016-03-24 16:00:00 | Genesis block timestamp |
| `ZATTERA_BLOCK_INTERVAL` | 3 seconds | 3 seconds | Time between blocks |
| `ZATTERA_BLOCKS_PER_DAY` | 28,800 | 28,800 | Blocks produced daily |

### Supply & Distribution

| Parameter | Testnet | Mainnet | Description |
|-----------|---------|---------|-------------|
| `ZATTERA_INIT_SUPPLY` | 250,000,000 ZTR | 0 ZTR | Initial token supply |

### Witness Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| `ZATTERA_MAX_WITNESSES` | 21 | Maximum active witnesses |
| `ZATTERA_MAX_VOTED_WITNESSES` | 20 | Elected witnesses |
| `ZATTERA_MAX_RUNNER_WITNESSES` | 1 | Backup witnesses |
| `ZATTERA_HARDFORK_REQUIRED_WITNESSES` | 17 | Witnesses needed for hardfork |

## Troubleshooting

### Genesis Already Exists

**Error**: `Database already initialized`

**Solution**: Delete existing blockchain data:

```bash
rm -rf witness_node_data_dir/blockchain
./programs/zatterad/zatterad --data-dir=witness_node_data_dir
```

### No Blocks Produced

**Error**: Node starts but doesn't produce blocks

**Check**:
1. `witness = "genesis"` in config
2. `private-key` matches genesis's key
3. `enable-stale-production = true` is set
4. System time is correct

**Debug**:
```bash
# Check witness plugin loaded
grep "witness_plugin" genesis.log

# Check witness scheduled
grep "starting block production" genesis.log
```

### Chain ID Mismatch

**Error**: `Remote chain ID does not match`

**Cause**: Trying to connect nodes with different `ZATTERA_CHAIN_ID_NAME`

**Solution**: Ensure all nodes are built with same `config.hpp` settings

### Port Already in Use

**Error**: `Address already in use`

**Solution**:
```bash
# Find process using port
lsof -i :2001

# Kill old zatterad instance
pkill zatterad

# Or use different port
p2p-endpoint = 0.0.0.0:2002
```

## Docker Deployment

### Docker Build for Genesis

Build a custom Docker image with genesis configuration:

```bash
# Build from repository root
docker build -t zattera:latest .

# Or build with testnet flag
docker build --build-arg BUILD_STEP=2 -t zattera:testnet .
```

### Configuration File Location

When running `zatterad` in Docker, the configuration file should be located at:

**Default location**: `/var/lib/zatterad/config.ini`

This is the directory specified by the `HOME` environment variable (default: `/var/lib/zatterad`).

#### Providing config.ini to Docker Containers

Copy your `config.ini` into the volume before starting the container:

```bash
# Create volume
docker volume create zattera-data

# Copy config.ini into volume
docker run --rm \
  -v zattera-data:/var/lib/zatterad \
  -v $(pwd)/config.ini:/tmp/config.ini:ro \
  alpine \
  cp /tmp/config.ini /var/lib/zatterad/config.ini

# Start container
docker run -d \
  --name zattera-genesis \
  -v zattera-data:/var/lib/zatterad \
  -p 2001:2001 \
  -p 8090:8090 \
  zattera:testnet
```

#### Verifying Configuration

Check if config.ini is properly loaded:

```bash
# View config in running container
docker exec zattera-genesis cat /var/lib/zatterad/config.ini

# Check which config file is being used
docker exec zattera-genesis ps aux | grep zatterad
```

### Storage Configuration

Docker containers require proper storage allocation for blockchain data.

#### Storage Requirements

| Node Type | Shared Memory | Block Log | Total Minimum | Recommended |
|-----------|---------------|-----------|---------------|-------------|
| **Genesis Node** | 8GB | 1GB | 10GB | 20GB |
| **Full Node (API)** | 56GB | 27GB+ | 100GB | 150GB |
| **Witness Node** | 16GB | 27GB+ | 50GB | 80GB |
| **Seed Node (Low Memory)** | 4GB | 27GB+ | 35GB | 50GB |

#### Docker Volume Setup

```bash
# Create named volume for persistent storage
docker volume create zattera-data

# Inspect volume
docker volume inspect zattera-data

# Volume location: /var/lib/docker/volumes/zattera-data/_data
```

#### Storage Options

**Option 1: Named Volume (Recommended)**

```bash
docker run -d \
  --name zattera-genesis \
  -v zattera-data:/var/lib/zatterad \
  -p 2001:2001 \
  -p 8090:8090 \
  zattera:testnet
```

**Option 2: Bind Mount**

```bash
# Create host directory
mkdir -p /mnt/zattera-data

docker run -d \
  --name zattera-genesis \
  -v /mnt/zattera-data:/var/lib/zatterad \
  -p 2001:2001 \
  -p 8090:8090 \
  zattera:testnet
```

**Option 3: tmpfs for Shared Memory (High Performance)**

```bash
docker run -d \
  --name zattera-genesis \
  -v zattera-data:/var/lib/zatterad \
  --tmpfs /dev/shm:rw,size=16g \
  --shm-size=16g \
  -p 2001:2001 \
  -p 8090:8090 \
  zattera:testnet
```

### Docker Environment Variables

Configure storage and node behavior via environment variables:

```bash
docker run -d \
  --name zattera-genesis \
  -e HOME=/var/lib/zatterad \
  -e ZATTERAD_SHARED_FILE_SIZE=8G \
  -e ZATTERAD_SHARED_FILE_DIR=/dev/shm \
  -v zattera-data:/var/lib/zatterad \
  --shm-size=16g \
  -p 2001:2001 \
  -p 8090:8090 \
  zattera:testnet
```

#### Key Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `HOME` | `/var/lib/zatterad` | Data directory path |
| `ZATTERAD_SHARED_FILE_SIZE` | `54G` | Shared memory file size |
| `ZATTERAD_SHARED_FILE_DIR` | `/var/lib/zatterad/blockchain` | Shared memory location |
| `ZATTERAD_SEED_NODES` | (empty) | P2P seed nodes (whitespace-delimited) |

### Docker Compose Setup

Create `docker-compose.yml` for genesis network:

```yaml
version: '3.8'

services:
  genesis-node:
    image: zattera:testnet
    container_name: zattera-genesis
    ports:
      - "2001:2001"
      - "8090:8090"
    volumes:
      - genesis-data:/var/lib/zatterad
      - ./config.ini:/var/lib/zatterad/config.ini:ro
    environment:
      - HOME=/var/lib/zatterad
      - ZATTERAD_SHARED_FILE_SIZE=8G
      - ZATTERAD_SHARED_FILE_DIR=/dev/shm
    shm_size: 16gb
    restart: unless-stopped
    networks:
      - zattera-network

  witness-node:
    image: zattera:testnet
    container_name: zattera-witness1
    ports:
      - "2002:2001"
      - "8091:8090"
    volumes:
      - witness1-data:/var/lib/zatterad
      - ./witness1-config.ini:/var/lib/zatterad/config.ini:ro
    environment:
      - HOME=/var/lib/zatterad
      - ZATTERAD_SHARED_FILE_SIZE=8G
      - ZATTERAD_SEED_NODES=genesis-node:2001
    shm_size: 16gb
    restart: unless-stopped
    depends_on:
      - genesis-node
    networks:
      - zattera-network

  api-node:
    image: zattera:testnet
    container_name: zattera-api
    ports:
      - "2003:2001"
      - "8092:8090"
    volumes:
      - api-data:/var/lib/zatterad
      - ./api-config.ini:/var/lib/zatterad/config.ini:ro
    environment:
      - HOME=/var/lib/zatterad
      - ZATTERAD_SHARED_FILE_SIZE=32G
      - ZATTERAD_SEED_NODES=genesis-node:2001
    shm_size: 48gb
    restart: unless-stopped
    depends_on:
      - genesis-node
    networks:
      - zattera-network

volumes:
  genesis-data:
    driver: local
  witness1-data:
    driver: local
  api-data:
    driver: local

networks:
  zattera-network:
    driver: bridge
```

Launch the network:

```bash
docker-compose up -d

# View logs
docker-compose logs -f genesis-node

# Scale witnesses
docker-compose up -d --scale witness-node=3
```

### Storage Monitoring

Monitor storage usage in containers:

```bash
# Check container disk usage
docker exec zattera-genesis df -h /var/lib/zatterad

# Check shared memory usage
docker exec zattera-genesis df -h /dev/shm

# Monitor in real-time
watch -n 5 'docker exec zattera-genesis du -sh /var/lib/zatterad/*'
```

### Storage Optimization for Genesis

For new genesis chains, optimize storage allocation:

```bash
# Minimal genesis node (development)
docker run -d \
  --name zattera-genesis-dev \
  -v genesis-dev-data:/var/lib/zatterad \
  -e ZATTERAD_SHARED_FILE_SIZE=2G \
  --shm-size=4g \
  -p 2001:2001 -p 8090:8090 \
  zattera:testnet

# Production genesis node
docker run -d \
  --name zattera-genesis-prod \
  -v genesis-prod-data:/var/lib/zatterad \
  -e ZATTERAD_SHARED_FILE_SIZE=16G \
  --shm-size=24g \
  --restart=always \
  -p 2001:2001 -p 8090:8090 \
  zattera:latest
```

### Backup and Recovery

#### Backup Genesis State

```bash
# Stop container
docker stop zattera-genesis

# Backup entire data volume
docker run --rm \
  -v zattera-data:/source:ro \
  -v $(pwd):/backup \
  alpine \
  tar czf /backup/genesis-backup-$(date +%Y%m%d).tar.gz -C /source .

# Restart container
docker start zattera-genesis
```

#### Restore from Backup

```bash
# Create new volume
docker volume create zattera-genesis-restore

# Restore data
docker run --rm \
  -v zattera-genesis-restore:/target \
  -v $(pwd):/backup \
  alpine \
  tar xzf /backup/genesis-backup-20250101.tar.gz -C /target

# Start container with restored volume
docker run -d \
  --name zattera-genesis-restored \
  -v zattera-genesis-restore:/var/lib/zatterad \
  -p 2001:2001 -p 8090:8090 \
  zattera:testnet
```

### Storage Performance Tuning

For optimal performance in production:

```bash
docker run -d \
  --name zattera-genesis-optimized \
  -v zattera-data:/var/lib/zatterad \
  -e ZATTERAD_SHARED_FILE_DIR=/dev/shm \
  --tmpfs /dev/shm:rw,size=24g,mode=1777 \
  --shm-size=24g \
  --memory=32g \
  --cpus=4 \
  --restart=always \
  -p 2001:2001 -p 8090:8090 \
  zattera:testnet
```

## Advanced Topics

### Custom Genesis Supply

To change the initial token supply, modify `ZATTERA_INIT_SUPPLY` in `src/core/protocol/include/zattera/protocol/config.hpp` and rebuild.

Example: For 1 billion initial supply instead of 250 million, change the value accordingly and run `make -j$(nproc) zatterad`.

### Multiple Genesis Witnesses

To create multiple initial witnesses, increase `ZATTERA_NUM_GENESIS_WITNESSES` in `config.hpp`. This will create accounts: `genesis`, `genesis1`, `genesis2`, etc.

### Custom Genesis Time

Genesis time can be customized by setting `ZATTERA_GENESIS_TIME` in `config.hpp`. Use a Unix timestamp (seconds since epoch).

**Important**: Genesis time affects all time-based protocol features including block timestamps, cashout windows, and vesting periods.

## Production Considerations

### Security

1. **Never reuse mainnet keys**: Generate unique keys for your chain
2. **Secure private keys**: Store witness keys in encrypted storage
3. **Network isolation**: Use firewalls for witness nodes
4. **Backup strategy**: Regularly backup `blockchain/` directory

### Performance

1. **SSD storage**: Use SSD for `blockchain/` directory
2. **Memory**: Allocate sufficient RAM for shared memory file
3. **CPU**: Ensure consistent block production (every 3 seconds)

### Monitoring

Monitor genesis node:

```bash
# Block production rate
tail -f genesis.log | grep "Generated block"

# Network connectivity
tail -f genesis.log | grep "p2p"

# Witness participation
curl -s http://127.0.0.1:8090 | jq '.participation'
```

## Additional Resources

- [Quick Start Guide](../getting-started/quick-start.md) - Get started quickly with Docker
- [Building Guide](../getting-started/building.md) - Build instructions for all platforms
- [CLI Wallet Guide](../getting-started/cli-wallet-guide.md) - Complete wallet usage guide
- [Node Modes Guide](node-modes-guide.md) - Different node types and configurations
- [Reverse Proxy Guide](reverse-proxy-guide.md) - Production deployment with NGINX
- [Testing Guide](../development/testing.md) - Running tests and code coverage
- [Configuration Examples](../../configs/README.md) - Ready-to-use configuration files

## License

See [LICENSE.md](../LICENSE.md)
