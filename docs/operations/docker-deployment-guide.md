# Docker Deployment Guide

Complete guide for building, deploying, and managing Zattera nodes with Docker.

## Overview

This guide covers Docker-specific operations for Zattera, including multi-platform builds, troubleshooting, and deployment best practices.

## Building Docker Images

### Single Platform Build

Basic build for your current platform:

```bash
docker build -t zatterahub/zattera:latest .
```

### Multi-Platform Build

Build for both AMD64 and ARM64 architectures simultaneously.

#### Setup Buildx

First-time setup for multi-platform builds:

```bash
# Create a new buildx builder instance
docker buildx create --name multiarch --driver docker-container --use

# Bootstrap the builder
docker buildx inspect --bootstrap

# Verify builder is ready
docker buildx ls
```

#### Build and Push

```bash
# Build for multiple platforms and push to registry
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t zatterahub/zattera:latest \
  -t zatterahub/zattera:stable \
  --push \
  .
```

#### Build with Version Tags

```bash
# Release build with semantic versioning
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t zatterahub/zattera:latest \
  -t zatterahub/zattera:1.0.0 \
  -t zatterahub/zattera:stable \
  --push \
  .

# Development build
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  -t zatterahub/zattera:dev \
  -t zatterahub/zattera:nightly \
  --push \
  .
```

#### Local Testing (Single Platform)

Multi-platform builds must be pushed to a registry. For local testing, build a single platform:

```bash
# Build only for current platform and load locally
docker buildx build \
  --platform linux/arm64 \
  -t zatterahub/zattera:latest \
  --load \
  .

# Or use standard docker build
docker build -t zatterahub/zattera:latest .
```

## Deployment Configurations

### Production Deployment

#### Witness Node

```bash
docker run -d \
  --name zattera-witness \
  --restart=unless-stopped \
  -v witness-data:/var/lib/zatterad \
  -p 2001:2001 \
  --memory=64g \
  --cpus=4 \
  zatterahub/zattera:latest
```

#### Full API Node

```bash
docker run -d \
  --name zattera-fullnode \
  --restart=unless-stopped \
  -e USE_HIGH_MEMORY=1 \
  -v fullnode-data:/var/lib/zatterad \
  -p 8090:8090 \
  -p 2001:2001 \
  --shm-size=280g \
  --memory=300g \
  --cpus=8 \
  zatterahub/zattera:latest
```

#### Seed Node

```bash
docker run -d \
  --name zattera-seed \
  --restart=unless-stopped \
  -v seed-data:/var/lib/zatterad \
  -p 2001:2001 \
  --memory=8g \
  --cpus=2 \
  zatterahub/zattera:latest
```

### Docker Compose

Create `docker-compose.yml`:

```yaml
version: '3.8'

services:
  witness:
    image: zatterahub/zattera:latest
    container_name: zattera-witness
    restart: unless-stopped
    ports:
      - "2001:2001"
    volumes:
      - witness-data:/var/lib/zatterad
    deploy:
      resources:
        limits:
          cpus: '4'
          memory: 64G
    networks:
      - zattera-net

  fullnode:
    image: zatterahub/zattera:latest
    container_name: zattera-fullnode
    restart: unless-stopped
    environment:
      - USE_HIGH_MEMORY=1
    ports:
      - "8090:8090"
      - "2002:2001"
    volumes:
      - fullnode-data:/var/lib/zatterad
    shm_size: 280g
    deploy:
      resources:
        limits:
          cpus: '8'
          memory: 300G
    networks:
      - zattera-net

volumes:
  witness-data:
  fullnode-data:

networks:
  zattera-net:
    driver: bridge
```

Deploy with:

```bash
docker-compose up -d
```

### Environment Variables

Available Docker environment variables:

```bash
# High memory mode (full API node)
USE_HIGH_MEMORY=1

# Full web node with all APIs
USE_FULL_WEB_NODE=1

# Enable NGINX reverse proxy with /health endpoint
USE_NGINX_FRONTEND=1

# Multi-reader mode (experimental, 4 readers per core)
USE_MULTICORE_READONLY=1

# Data directory
HOME=/var/lib/zatterad

# PaaS mode for AWS Elastic Beanstalk
USE_PAAS=1

# S3 bucket for shared memory files
S3_BUCKET=my-zattera-bucket

# Generate and upload shared memory to S3
SYNC_TO_S3=1

# Override default seed nodes (whitespace-delimited)
ZATTERAD_SEED_NODES="seed1.zattera.com:2001 seed2.zattera.com:2001"
```

Example with environment variables:

```bash
docker run -d \
  --name zattera-fullnode \
  -e USE_HIGH_MEMORY=1 \
  -e USE_FULL_WEB_NODE=1 \
  -e ZATTERAD_SEED_NODES="seed1.zattera.com:2001 seed2.zattera.com:2001" \
  -v fullnode-data:/var/lib/zatterad \
  -p 8090:8090 \
  --shm-size=280g \
  zatterahub/zattera:latest
```

## Monitoring and Maintenance

### Health Checks

Add health check to Dockerfile:

```dockerfile
HEALTHCHECK --interval=30s --timeout=10s --start-period=5m --retries=3 \
  CMD curl -f http://localhost:8090/ || exit 1
```

Or in docker-compose.yml:

```yaml
services:
  fullnode:
    # ... other config ...
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8090/"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 5m
```

### Logging

```bash
# View logs
docker logs zattera-witness

# Follow logs
docker logs -f zattera-witness

# Last 100 lines
docker logs --tail 100 zattera-witness

# Logs since timestamp
docker logs --since 2024-01-01T00:00:00 zattera-witness
```

Configure logging driver in docker-compose.yml:

```yaml
services:
  witness:
    # ... other config ...
    logging:
      driver: "json-file"
      options:
        max-size: "100m"
        max-file: "3"
```

### Backup and Restore

#### Backup Volumes

```bash
# Backup witness data
docker run --rm \
  -v witness-data:/data \
  -v $(pwd):/backup \
  ubuntu tar czf /backup/witness-backup.tar.gz /data

# Backup with timestamp
docker run --rm \
  -v witness-data:/data \
  -v $(pwd):/backup \
  ubuntu tar czf /backup/witness-backup-$(date +%Y%m%d).tar.gz /data
```

#### Restore Volumes

```bash
# Restore witness data
docker run --rm \
  -v witness-data:/data \
  -v $(pwd):/backup \
  ubuntu tar xzf /backup/witness-backup.tar.gz -C /
```

#### Export Container State

```bash
# Commit container to image
docker commit zattera-witness zatterahub/zattera:backup-20240101

# Save image to file
docker save zatterahub/zattera:backup-20240101 | gzip > zattera-backup.tar.gz

# Load image from file
gunzip -c zattera-backup.tar.gz | docker load
```

## Best Practices

### Security

1. **Use official base images**: Start with official Ubuntu/Debian images
2. **Run as non-root**: Create dedicated user in Dockerfile
3. **Scan images**: Use `docker scan` or Trivy
4. **Minimize layers**: Combine RUN commands
5. **Don't include secrets**: Use secrets management or build args

```dockerfile
# Good: Run as non-root user
RUN useradd -m -u 1000 zattera
USER zattera

# Good: Minimize layers
RUN apt-get update && apt-get install -y \
    package1 package2 package3 \
    && rm -rf /var/lib/apt/lists/*
```

### Performance

1. **Multi-stage builds**: Reduce final image size
2. **BuildKit cache**: Use `--cache-from` and `--cache-to`
3. **Layer caching**: Order Dockerfile from least to most frequently changing
4. **Resource limits**: Set appropriate memory and CPU limits
5. **Shared memory**: Use `--shm-size` for Zattera nodes

### Maintenance

1. **Regular updates**: Keep base images and dependencies updated
2. **Automated builds**: Use CI/CD for consistent builds
3. **Tag strategy**: Use semantic versioning
4. **Cleanup schedule**: Automate Docker cleanup
5. **Monitor resources**: Track disk, memory, CPU usage

## Additional Resources

- [Node Modes Guide](node-modes-guide.md) - Different node types and configurations
- [Reverse Proxy Guide](reverse-proxy-guide.md) - Production deployment with NGINX
- [Quick Start Guide](../getting-started/quick-start.md) - Getting started with Docker
- [Building Guide](../getting-started/building.md) - Building from source
- [Docker Documentation](https://docs.docker.com/) - Official Docker docs
- [Docker Buildx Documentation](https://docs.docker.com/buildx/working-with-buildx/) - Multi-platform builds

## License

See [LICENSE.md](../../LICENSE.md)
