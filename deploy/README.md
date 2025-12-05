# Deployment Scripts

This directory contains deployment scripts and configuration files for running Zattera nodes in various environments.

## Quick Reference

### Bootstrap Scripts (Direct Execution)

- **[zatterad-entrypoint.sh](zatterad-entrypoint.sh)** - Docker container entrypoint script
- **[zatterad-paas-bootstrap.sh](zatterad-paas-bootstrap.sh)** - PaaS environment (AWS Elastic Beanstalk) bootstrap
- **[zatterad-test-bootstrap.sh](zatterad-test-bootstrap.sh)** - Private test network initialization
- **[zatterad-healthcheck.sh](zatterad-healthcheck.sh)** - Node health check script

### Service Management

- **[runit/](runit/)** - Runit process supervisor scripts
- **[nginx/](nginx/)** - NGINX reverse proxy configurations

## Docker Deployment

### zatterad-entrypoint.sh

Docker container entrypoint script that configures and starts zatterad based on environment variables.

**Environment Variables:**
- `ZATTERAD_NODE_MODE=<mode>` - Node mode: `fullnode`, `broadcast`, `ahnode`, `witness`, or `testnet` (sets config file)
- `USE_HIGH_MEMORY=1` - Enable high-memory mode with in-memory account history (uses zatterad-high binary)
- `USE_NGINX_PROXY=1` - Enable NGINX reverse proxy with `/health` endpoint
- `USE_PUBLIC_SHARED_MEMORY=1` - Download public shared memory snapshot on startup
- `USE_PUBLIC_BLOCKLOG=1` - Download public block log on startup
- `DEPLOY_TO_PAAS=1` - Enable PaaS deployment mode (uses zatterad-paas-bootstrap.sh)
- `IS_TEST_NODE=1` - Enable test node mode (uses zatterad-test-bootstrap.sh)
- `HOME=<path>` - Data directory path (default: `/var/lib/zatterad`)
- `ZATTERAD_SEED_NODES=<nodes>` - Whitespace-delimited list of P2P seed nodes
- `ZATTERAD_WITNESS_NAME=<name>` - Witness account name (for witness mode)
- `ZATTERAD_PRIVATE_KEY=<key>` - Witness private key (for witness mode)
- `ZATTERAD_TRACK_ACCOUNT=<account>` - Account to track in account history
- `ZATTERAD_EXTRA_OPTS=<options>` - Additional command line options
- `DISABLE_SHARED_MEMORY_SCALING=1` - Disable shared memory scaling (omit for default scaling)

**Example:**
```bash
# Run as full node with NGINX proxy
docker run -e ZATTERAD_NODE_MODE=fullnode -e USE_NGINX_PROXY=1 zatterahub/zattera:latest

# Run high-memory node
docker run -e USE_HIGH_MEMORY=1 -e ZATTERAD_NODE_MODE=fullnode zatterahub/zattera:latest

# Run testnet node
docker run -e ZATTERAD_NODE_MODE=testnet zatterahub/zattera:latest

# Run test node (private testnet)
docker run -e IS_TEST_NODE=1 zatterahub/zattera:latest
```

## PaaS Deployment

### zatterad-paas-bootstrap.sh

Bootstrap script for PaaS environments (AWS Elastic Beanstalk).

**Features:**
- Download blockchain snapshots from S3
- Configure and launch zatterad for cloud environments
- Automatic snapshot management and uploads

**Environment Variables:**
- `IS_SNAPSHOT_NODE=1` - Enable snapshot upload mode
- `S3_BUCKET=<bucket-name>` - S3 bucket for snapshots
- `ZATTERAD_SEED_NODES=<nodes>` - Whitespace-delimited list of seed nodes

**Example:**
```bash
IS_SNAPSHOT_NODE=1 S3_BUCKET=my-zattera-snapshots ./zatterad-paas-bootstrap.sh
```

## Test Network

### zatterad-test-bootstrap.sh

Initialize a private test network with witness accounts and genesis block.

**Features:**
- Initialize fastgen node for transaction generation
- Configure 21 witness accounts
- Setup test environment with custom genesis

**Example:**
```bash
./zatterad-test-bootstrap.sh
```

## Health Monitoring

### zatterad-healthcheck.sh

Node health check script for monitoring blockchain sync status.

**Features:**
- Verify blockchain synchronization status
- Measure time difference from latest block
- Return HTTP 200/503 response codes

**Usage:**
```bash
./zatterad-healthcheck.sh
# Returns: exit 0 (healthy) or exit 1 (unhealthy)
```

## Service Management

### Runit Scripts

See [runit/README.md](runit/README.md) for process supervisor configuration.

### NGINX Configuration

See [nginx/README.md](nginx/README.md) for reverse proxy setup.

## Additional Resources

- **[Configuration Examples](../configs/README.md)** - Node configuration examples
- **[Node Modes Guide](../docs/operations/node-modes-guide.md)** - Detailed guide to different node modes
- **[Quick Start Guide](../docs/getting-started/quick-start.md)** - Docker quick start
