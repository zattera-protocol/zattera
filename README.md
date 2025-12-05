# Zattera Mainnet

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)

Zattera Mainnet is a Delegated Proof of Stake (DPoS) blockchain implementing a "Proof of Brain" social consensus algorithm for fair content rewards and token distribution.

## Key Features

- **Currency Symbol**: ZTR
- **Consensus**: Delegated Proof of Stake with 21 witnesses
- **Block Time**: 3 seconds
- **Inflation**: 10% APR narrowing to 1% APR over 20 years
- **Reward Distribution**: 75% content creators, 15% stakers, 10% witnesses

## Quick Start

Run a full API node with Docker:

```bash
docker run -d \
    --name zattera-node \
    -e ZATTERAD_NODE_MODE=fullnode \
    -p 2001:2001 -p 8090:8090 \
    zatterahub/zattera:latest

docker logs -f zattera-node
```

**System Requirements**: 16GB+ RAM, 110GB+ disk space, SSD recommended

For more deployment options, see [Quick Start Guide](docs/getting-started/quick-start.md).

## Documentation

### Getting Started
- [Quick Start Guide](docs/getting-started/quick-start.md) - Docker deployment and node setup
- [Building from Source](docs/getting-started/building.md) - Compile for Linux/macOS
- [CLI Wallet Guide](docs/getting-started/cli-wallet-guide.md) - Command-line wallet usage
- [Exchange Integration](docs/getting-started/exchange-quick-start.md) - Guide for exchanges

### Operations
- [Node Modes Guide](docs/operations/node-modes-guide.md) - Witness, full node, seed node configurations
- [Genesis Launch](docs/operations/genesis-launch.md) - Launch procedures for new networks
- [Reverse Proxy Setup](docs/operations/reverse-proxy-guide.md) - NGINX/HAProxy configuration

### Development
- [Testing Guide](docs/development/testing.md) - Run and write tests
- [Plugin Development](docs/development/plugin.md) - Create custom plugins

### Resources
- [Zattera Whitepaper](https://zattera.club/docs/zattera-whitepaper.pdf) - Technical design and economics
- [ZEPs (Zattera Enhancement Proposals)](https://github.com/zattera-protocol/ZEPs) - Protocol improvement proposals

## Repository Structure

```
zattera/
├── src/
│   ├── core/           # Protocol, chain, and chainbase implementation
│   ├── base/           # Foundational libraries (fc, appbase, networking)
│   ├── plugins/        # Blockchain plugins and APIs
│   └── wallet/         # Wallet library
├── programs/           # Executables (zatterad, cli_wallet, utilities)
├── tests/              # Test suite
├── configs/            # Node configuration examples
├── deploy/             # Deployment scripts and Docker configurations
└── docs/               # Documentation
```

## Development

### Building from Source

```bash
git clone https://github.com/zattera-protocol/zattera.git
cd zattera
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) zatterad cli_wallet
```

See [Building Guide](docs/getting-started/building.md) for detailed instructions and platform-specific requirements.

### Running Tests

```bash
# Build with testnet support
cmake -DBUILD_ZATTERA_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) chain_test plugin_test

# Run tests
./tests/chain_test
./tests/plugin_test
```

See [Testing Guide](docs/development/testing.md) for comprehensive testing documentation.

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for:
- Code of conduct
- Development workflow
- Pull request guidelines
- Coding standards

## License

This software is provided under the MIT License. See [LICENSE.md](LICENSE.md) for full terms.

```
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
```

## Acknowledgments

Zattera is built upon the [Steem blockchain](https://github.com/steemit/steem) codebase. We acknowledge and appreciate the foundational work of the Steem development community.
