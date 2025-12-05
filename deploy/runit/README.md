# Runit Service Scripts

This directory contains [runit](http://smarden.org/runit/) service scripts for process supervision and monitoring.

## Overview

Runit is a lightweight process supervisor that manages long-running services. These scripts are used in Docker containers and PaaS environments to ensure zatterad processes run reliably with automatic restart on failure.

## Scripts

### zatterad.run

Standard runit service script for zatterad execution.

**Features:**
- Direct zatterad process execution
- Managed by runit process supervisor
- Automatic restart on crash
- Signal handling for graceful shutdown

**Usage:**
```bash
# Typically managed by runit supervisor
sv start zatterad
sv stop zatterad
sv restart zatterad
sv status zatterad
```

**Installation:**
```bash
# Copy to runit service directory
cp zatterad.run /etc/service/zatterad/run
chmod +x /etc/service/zatterad/run
```

### zatterad-paas-monitor.run

Runit monitoring script for PaaS environments.

**Features:**
- Monitors zatterad process health every 10 seconds
- Collects core dumps on crash
- Triggers container restart on failure
- Logs monitoring events

**Environment Variables:**
- Standard zatterad environment variables
- Container orchestration variables (AWS ECS, etc.)

**Usage:**
```bash
# Runs as a runit service alongside zatterad
sv start zatterad-paas-monitor
```

### zatterad-snapshot-uploader.run

Runit script for blockchain snapshot management.

**Features:**
- Monitors blockchain synchronization status every 60 seconds
- Compresses and uploads snapshots when fully synced
- Handles crash recovery with core dump collection
- Supports S3 and compatible storage backends

**Environment Variables:**
- `IS_SNAPSHOT_NODE=1` - Enable snapshot upload mode
- `S3_BUCKET=<bucket-name>` - S3 bucket name for snapshot storage
- `HOME=<path>` - Data directory path

**Usage:**
```bash
# Runs as a runit service
sv start zatterad-snapshot-uploader
```

**Upload Process:**
1. Wait for blockchain to fully synchronize
2. Stop zatterad gracefully
3. Compress shared memory file
4. Upload to S3 bucket
5. Restart zatterad

## Runit Service Management

### Common Commands

```bash
# Start service
sv start <service-name>

# Stop service
sv stop <service-name>

# Restart service
sv restart <service-name>

# Check status
sv status <service-name>

# Send signal
sv kill <service-name>

# View logs
tail -f /var/log/<service-name>/current
```

### Service Installation

1. Create service directory:
```bash
mkdir -p /etc/service/<service-name>
```

2. Copy run script:
```bash
cp <script>.run /etc/service/<service-name>/run
chmod +x /etc/service/<service-name>/run
```

3. Create log directory (optional):
```bash
mkdir -p /etc/service/<service-name>/log
```

4. Runit automatically starts the service

## Monitoring and Logs

Runit services log to `/var/log/<service-name>/` by default when using `svlogd`.

**View logs:**
```bash
# Real-time logs
tail -f /var/log/zatterad/current

# Filter logs
cat /var/log/zatterad/current | grep ERROR
```

## Additional Resources

- [Runit Documentation](http://smarden.org/runit/)
- [Main Deploy README](../README.md)
- [Docker Quick Start](../../docs/getting-started/quick-start.md)
