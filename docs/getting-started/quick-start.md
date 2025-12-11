# Quick Start Guide

## Prerequisites

### Install Docker

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

#### macOS / Windows
Download and install [Docker Desktop](https://www.docker.com/products/docker-desktop/)

### System Requirements

- **RAM**: 4GB minimum (16GB+ recommended for full node)
- **Storage**: 30GB minimum (120GB+ recommended for full node)
- **SSD**: Recommended for better performance

## Running Zattera with Docker

The easiest way to get started with Zattera is using Docker.

First, pull the latest stable image:

```bash
docker pull zatterahub/zattera:stable
```

### Low Memory Node

Suitable for seed nodes, witness nodes, and exchanges:

```bash
docker run -d \
    --name zattera-minimal \
    -p 2001:2001 -p 8090:8090 \
    --restart unless-stopped \
    zatterahub/zattera:stable
```

### Full API Node

For a full API node with all features enabled:

```bash
docker run -d \
    --name zattera-fullnode \
    -e ZATTERAD_NODE_MODE=fullnode \
    -e USE_HIGH_MEMORY=1 \
    -p 2001:2001 -p 8090:8090 \
    --restart unless-stopped \
    zatterahub/zattera:stable
```

### Testnet Node

For connecting to the Zattera testnet:

```bash
docker run -d \
    --name zattera-testnet \
    -e ZATTERAD_NODE_MODE=testnet \
    -p 2001:2001 -p 8090:8090 \
    --restart unless-stopped \
    zatterahub/zattera:stable
```

## Configure for Your Use Case

### Full API Node

You need to use `ZATTERAD_NODE_MODE=fullnode` and `USE_HIGH_MEMORY=1` as stated above. You can use `configs/fullnode.config.ini` as a base for your `config.ini` file.

### Exchange Nodes

For exchanges, use a low memory node with account tracking enabled.

Make sure that your `config.ini` contains:
```
enable-plugin = account_history
public-api = database_api login_api
track-account-range = ["yourexchangeid", "yourexchangeid"]
```

**Note:** Do not add other APIs or plugins unless you know what you are doing.

You can run this configuration with Docker using the following command:

```bash
docker run -d \
    --name zattera-exchange \
    -e TRACK_ACCOUNT="yourexchangeid" \
    -p 2001:2001 -p 8090:8090 \
    --restart unless-stopped \
    zatterahub/zattera:stable
```

**For detailed exchange integration guide, see [Exchange Quick Start Guide](exchange-quick-start.md).**

## Resource Requirements

Ensure you have sufficient resources available. Check the `shared-file-size =` parameter in your `config.ini` to reflect your needs. Set it to at least 25% more than the current size.

**Note:** These values are expected to grow significantly over time. Blockchain data currently requires over **16GB** of storage space.

### Full Node

Shared memory file for a full node requires over **65GB**.

### Exchange Node

Shared memory file for an exchange node requires over **16GB** (for tracking a single account's history).

### Seed Node

Shared memory file for a seed node requires over **5.5GB**.

### Other Configurations

Shared memory file size varies depending on your specific configuration, typically ranging between seed node and full node requirements.

## Additional Resources

- [Building Guide](building.md) - Build from source on different platforms
- [Exchange Quick Start Guide](exchange-quick-start.md) - Detailed guide for cryptocurrency exchanges
- [CLI Wallet Guide](cli-wallet-guide.md) - Complete wallet usage guide
- [Node Modes Guide](../operations/node-modes-guide.md) - Different node types and configurations
- [Genesis Launch Guide](../operations/genesis-launch.md) - Launch your own Zattera network
- [Reverse Proxy Guide](../operations/reverse-proxy-guide.md) - Production deployment with NGINX
