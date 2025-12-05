# Python Debug Node

## Overview

The Python Debug Node is a wrapper class that automates the creation and maintenance of a running Zattera Debug Node. The Debug Node is a plugin for Zattera that allows realtime local modification of the chain state in a way that mimics real world behaviors without corrupting a locally saved blockchain or propagating changes to the live chain.

More information about the debug node can be found in [debug_node_plugin.md](debug-node-plugin.md).

## Why Should I Use This?

The Debug Node Plugin is powerful but requires knowledge of node configuration and RPC/WebSocket interfaces. This Python module simplifies the process by providing:

- Programmatic node launching and lifecycle management
- Simple WebSocket-based JSON-RPC client (`zatteranoderpc`)
- Clean context manager (`with` statement) interface for automatic cleanup

## How Do I Use This?

### Installation

Navigate to `tools/python_debug_node` and run:

```bash
python3 setup.py install
```

### Usage

To use the debug node in your scripts:

```python
from zatteradebugnode import ZatteraDebugNode
```

The module provides two main components:
- **`ZatteraDebugNode`** - Wrapper class for launching and managing a debug node instance
- **`ZatteraNodeRPC`** - WebSocket-based JSON-RPC client for communicating with zatterad nodes (imported from `zatteranoderpc`)

## Example Scripts

### debug_hardforks.py

[tests/debug_hardforks.py](tests/debug_hardforks.py) - This script demonstrates hardfork testing using the debug node. It:
- Starts a debug node
- Replays blocks from an existing chain
- Schedules a hardfork using `debug_set_hardfork()`
- Generates blocks after the hardfork
- Analyzes results via the JSON-RPC interface (e.g., generates a histogram of block producers)

The purpose is to verify that hardforks execute correctly without crashing the chain.

### debugnode.py

[zatteradebugnode/debugnode.py](zatteradebugnode/debugnode.py#L280) - The main module file includes a standalone example (run with `python -m zatteradebugnode.debugnode`). This simple script:
- Parses command-line arguments for zatterad path and data directory
- Creates and starts a debug node
- Replays the blockchain
- Waits indefinitely for user interaction via RPC calls or CLI wallet
- Cleanly shuts down on SIGINT (Ctrl+C)

### Basic Usage Pattern

The important part of these scripts is:

```python
debug_node = ZatteraDebugNode(str(zatterad_path), str(data_dir))
with debug_node:
   # Do stuff with blockchain
```

The module is set up to use the `with` construct. The object construction simply checks for the correct directories and sets internal state. `with debug_node:` connects the node and establishes the internal RPC connection. The script can then do whatever it wants to. When the `with` block ends, the node automatically shuts down and cleans up.

The node uses a system standard temp directory through the standard Python `TemporaryDirectory` as the working data directory for the running node. The only work your script needs to do is specify the zatterad binary location and a populated data directory. For most configurations this will be `programs/zatterad/zatterad` and `witness_node_data_dir` respectively, from the git root directory for Zattera.

## API Reference

### ZatteraDebugNode Class

#### Constructor

```python
ZatteraDebugNode(
    zatterad_path: str,
    data_dir: str,
    zatterad_args: str = "",
    plugins: List[str] = [],
    apis: List[str] = [],
    zatterad_stdout: Optional[IO] = None,
    zatterad_stderr: Optional[IO] = None
)
```

**Parameters:**
- `zatterad_path` - Path to the zatterad binary
- `data_dir` - Path to an existing data directory (used as source for blockchain data)
- `zatterad_args` - Additional command-line arguments to pass to zatterad
- `plugins` - Additional plugins to enable (beyond `witness` and `debug_node`)
- `apis` - Additional APIs to expose (beyond `database_api`, `login_api`, and `debug_node_api`)
- `zatterad_stdout` - Stream for zatterad stdout (defaults to `/dev/null`)
- `zatterad_stderr` - Stream for zatterad stderr (defaults to `/dev/null`)

#### Debug Node API Methods

**`debug_generate_blocks(count: int) -> int`**

Generates `count` new blocks on the chain. Pending transactions will be applied; otherwise blocks will be empty. Uses the debug key `5JHNbFNDg834SFj8CMArV6YW7td4zrPzXveqTfaShmYVuYNeK69` (from `get_dev_key zattera debug`). **Never use this key on the live chain.**

Returns the number of blocks actually generated.

**`debug_generate_blocks_until(timestamp: int, generate_sparsely: bool = True) -> Tuple[str, int]`**

Generates blocks until the head block time reaches the specified POSIX timestamp.

- `timestamp` - Target head block time (POSIX timestamp)
- `generate_sparsely` - If `True`, skips intermediate blocks and only updates the head block time. Useful for triggering time-based events (payouts, bandwidth updates) without generating every block. If `False`, generates all blocks sequentially.

Returns a tuple of (new_head_block_time, blocks_generated).

**`debug_set_hardfork(hardfork_id: int) -> None`**

Schedules a hardfork to execute on the next block. Call `debug_generate_blocks(1)` to trigger it. All hardforks with ID ≤ `hardfork_id` will be scheduled.

- `hardfork_id` - Hardfork ID (starts at 1; 0 is genesis). Maximum value is defined by `ZATTERA_NUM_HARDFORKS` in [hardfork.d/0-preamble.hf](../../src/core/protocol/hardfork.d/0-preamble.hf)

**`debug_has_hardfork(hardfork_id: int) -> bool`**

Checks if a hardfork has been applied.

**`debug_get_witness_schedule() -> dict`**

Returns the current witness schedule.

**`debug_get_hardfork_property_object() -> dict`**

Returns the hardfork property object with current hardfork state.

### ZatteraNodeRPC Class

A simple WebSocket-based JSON-RPC client for communicating with zatterad nodes.

#### Constructor

```python
ZatteraNodeRPC(url: str, username: str = "", password: str = "")
```

**Parameters:**
- `url` - WebSocket URL (e.g., `"ws://127.0.0.1:8095"`)
- `username` - Optional username for authentication
- `password` - Optional password for authentication

#### Methods

**`call(request: Dict[str, Any]) -> Any`**

Executes a JSON-RPC request.

**Request format:**
```python
{
    "jsonrpc": "2.0",
    "method": "call",
    "params": [api_id, method_name, args],
    "id": 1
}
```

**API IDs:**
- `0` - `database_api`
- `1` - `login_api`
- `2` - `debug_node_api`
- `3+` - Additional APIs in the order specified in `ZatteraDebugNode` constructor

Returns the `result` field from the JSON-RPC response.

**Example:**
```python
# Get dynamic global properties
request = {
    "jsonrpc": "2.0",
    "method": "call",
    "params": [0, "get_dynamic_global_properties", []],
    "id": 1
}
result = rpc.call(request)
```

**`close() -> None`**

Closes the WebSocket connection. Automatically called when using context manager (`with` statement).

## Implementation Notes

### Current Architecture

The module is self-contained with the following components:

- **`zatteradebugnode.ZatteraDebugNode`** - Main wrapper for debug node management
- **`zatteranoderpc.ZatteraNodeRPC`** - Custom WebSocket JSON-RPC client
- Uses `websocket-client` library for WebSocket connectivity

### Configuration

The debug node automatically configures itself with:
- **P2P endpoint**: `127.0.0.1:2001` (localhost only to prevent remote connections)
- **RPC endpoint**: `127.0.0.1:8095` (localhost only for security)
- **Enabled plugins**: `witness`, `debug_node`, plus any specified in constructor
- **Public APIs**: `database_api`, `login_api`, `debug_node_api`, plus any specified in constructor

### Data Directory Handling

The `ZatteraDebugNode` creates a temporary working directory using Python's `TemporaryDirectory`:
1. Copies all directories from the source data directory
2. Copies `db_version` file if present
3. Generates a custom `config.ini`
4. Automatically cleans up on exit

The source data directory remains unchanged and can be reused.

## Future Enhancements

Areas for potential improvement:

- **Higher-level API wrappers:** Provide Python methods for common database_api and condenser_api calls
- **Code generation:** Auto-generate API wrappers from C++ source or JSON schema
- **Testing framework:** Integrate with pytest or unittest for structured integration testing
- **Chain state snapshots:** Support for saving/loading chain states for reproducible testing
- **Multi-node support:** Ability to run multiple debug nodes for P2P testing scenarios