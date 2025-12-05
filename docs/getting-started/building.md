# Building Guide

This guide covers building Zattera from source on different platforms.

## Prerequisites

### Common Requirements

All platforms require:
- Git (for cloning the repository)
- CMake 3.x or higher
- C++14 compatible compiler (GCC 4.8+, Clang 3.3+, or newer)
- Python 3 with Jinja2
- Boost libraries
- OpenSSL
- Additional compression libraries (bzip2, snappy, zlib)

## Building with Docker

We ship a Dockerfile that builds both common node type binaries.

```bash
git clone https://github.com/zattera-protocol/zattera
cd zattera
docker build -t zatterahub/zattera .
```

## Building on Ubuntu 22.04

### Install Dependencies

Install required packages:

```bash
# Core build tools and libraries
sudo apt-get install -y \
    autoconf \
    automake \
    cmake \
    g++ \
    git \
    libbz2-dev \
    libsnappy-dev \
    libssl-dev \
    libtool \
    make \
    pkg-config \
    python3 \
    python3-jinja2
```

Install Boost libraries:

```bash
sudo apt-get install -y \
    libboost-chrono-dev \
    libboost-context-dev \
    libboost-coroutine-dev \
    libboost-date-time-dev \
    libboost-filesystem-dev \
    libboost-iostreams-dev \
    libboost-locale-dev \
    libboost-program-options-dev \
    libboost-serialization-dev \
    libboost-signals-dev \
    libboost-system-dev \
    libboost-test-dev \
    libboost-thread-dev
```

Optional packages for better development experience:

```bash
sudo apt-get install -y \
    doxygen \
    libncurses5-dev \
    libreadline-dev \
    perl
```

### Clone and Build

```bash
git clone https://github.com/zattera-protocol/zattera
cd zattera
git checkout stable
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc) zatterad
make -j$(nproc) cli_wallet
```

Optional: Install binaries system-wide:

```bash
sudo make install  # Installs to /usr/local by default
```

## Building on macOS

### Install Xcode

Install Xcode and its command line tools from https://guide.macports.org/#installing.xcode

On macOS 10.11 (El Capitan) and newer, you'll be prompted to install developer tools when running a developer command in the terminal.

Accept the Xcode license:

```bash
sudo xcodebuild -license accept
```

### Install Homebrew

Install Homebrew from http://brew.sh/

Initialize Homebrew:

```bash
brew doctor
brew update
```

### Install Dependencies

Install required packages:

```bash
brew install \
    autoconf \
    automake \
    cmake \
    git \
    boost \
    bzip2 \
    libtool \
    openssl \
    snappy \
    zlib \
    python3

pip3 install --user jinja2
```

Note: The codebase has been updated to work with modern Boost versions (tested with Boost 1.89.0).

Optional packages:

```bash
# For better performance with TCMalloc in LevelDB
brew install google-perftools

# For cli_wallet with better readline support
brew install --force readline
brew link --force readline
```

### Clone and Build

```bash
git clone https://github.com/zattera-protocol/zattera.git
cd zattera
git checkout stable
git submodule update --init --recursive

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBOOST_ROOT=$(brew --prefix boost) \
      -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) \
      -DZLIB_ROOT=$(brew --prefix zlib) \
      -DBZIP2_INCLUDE_DIR=$(brew --prefix bzip2)/include \
      -DBZIP2_LIBRARIES=$(brew --prefix bzip2)/lib/libbz2.a \
      ..
make -j$(sysctl -n hw.logicalcpu)
```

### Build Specific Targets

You can build specific targets instead of everything:

```bash
# Build only zatterad
make -j$(sysctl -n hw.logicalcpu) zatterad

# Build only cli_wallet
make -j$(sysctl -n hw.logicalcpu) cli_wallet

# Build test suite
make -j$(sysctl -n hw.logicalcpu) chain_test
```

## Building on Other Platforms

### Windows

Windows build instructions do not yet exist.

### Compiler Support

- **Officially supported**: GCC and Clang (used by core developers)
- **Community supported**: MinGW, Intel C++, and Microsoft Visual C++
  - These may work but are not regularly tested by developers
  - Pull requests fixing warnings/errors from these compilers are welcome

## CMake Build Options

Configure the build by passing options to CMake:

### CMAKE_BUILD_TYPE=[Release/Debug]

Specifies the build type:
- `Release`: Optimized build without debug symbols (recommended for production)
- `Debug`: Unoptimized build with debug symbols (for development and debugging)

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### LOW_MEMORY_NODE=[OFF/ON]

Build a consensus-only low memory node:
- Excludes data and fields not needed for consensus from the object database
- Recommended for witnesses and seed nodes

```bash
cmake -DLOW_MEMORY_NODE=ON ..
```

### CLEAR_VOTES=[ON/OFF]

Clear old votes from memory that are no longer required for consensus.

```bash
cmake -DCLEAR_VOTES=ON ..
```

### BUILD_ZATTERA_TESTNET=[OFF/ON]

Build for use in testnet. Also required for building unit tests.

```bash
cmake -DBUILD_ZATTERA_TESTNET=ON ..
```

### SKIP_BY_TX_ID=[OFF/ON]

Skip transaction-by-id indexing:
- Saves around 65% of CPU time during reindexing
- Disables the account history plugin's ability to query transactions by id
- Highly recommended if you don't need transaction-by-id lookups

```bash
cmake -DSKIP_BY_TX_ID=ON ..
```

## Next Steps

After building:
- See [quick-start.md](quick-start.md) for running your first node
- See [cli-wallet-guide.md](cli-wallet-guide.md) for wallet usage
- See [node-modes-guide.md](../operations/node-modes-guide.md) for different node configurations

## Additional Resources

- [Quick Start Guide](quick-start.md) - Get started quickly with Docker
- [Exchange Quick Start Guide](exchange-quick-start.md) - Detailed guide for cryptocurrency exchanges
- [CLI Wallet Guide](cli-wallet-guide.md) - Complete wallet usage guide
- [Node Modes Guide](../operations/node-modes-guide.md) - Different node types and configurations
- [Testing Guide](../development/testing.md) - Running tests and code coverage
- [Genesis Launch Guide](../operations/genesis-launch.md) - Launch your own Zattera network
