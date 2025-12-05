# CI Scripts

Continuous Integration scripts for automated builds and tests.

## Overview

Shell scripts for automated Docker builds and tests. These scripts can be used with GitHub Actions or other CI/CD platforms.

## Scripts

| Script | Purpose |
|--------|---------|
| **[build-release.sh](build-release.sh)** | Build and push Docker image |
| **[run-tests.sh](run-tests.sh)** | Run tests and generate coverage reports |

## build-release.sh

Build and push Docker image to Docker Hub.

### Usage

```bash
export BRANCH_NAME=main
export DOCKER_USER=<username>
export DOCKER_TOKEN=<access-token>
./build-release.sh
```

### Environment Variables

| Variable | Description |
|----------|-------------|
| `BRANCH_NAME` | Git branch name (determines Docker tag) |
| `DOCKER_USER` | Docker Hub username |
| `DOCKER_TOKEN` | Docker Hub Personal Access Token (PAT) |

### Behavior

- **Branch `stable`**: Tags as `zatterahub/zattera:latest`
- **Other branches**: Tags as `zatterahub/zattera:$BRANCH_NAME`
- Builds with `BUILD_STEP=2` argument
- Pushes to Docker Hub
- Cleans up exited containers

### Example

```bash
# Build from stable branch
export BRANCH_NAME=stable
./build-release.sh
# → zatterahub/zattera:latest

# Build from feature branch
export BRANCH_NAME=feature-xyz
./build-release.sh
# → zatterahub/zattera:feature-xyz
```

## run-tests.sh

Run tests in Docker and extract code coverage reports.

### Usage

```bash
export WORKSPACE=/path/to/workspace
./run-tests.sh
```

### Environment Variables

| Variable | Description |
|----------|-------------|
| `WORKSPACE` | Working directory for build artifacts |

### Behavior

- Builds test image with `BUILD_STEP=1`
- Runs tests in container
- Extracts Cobertura coverage reports to `$WORKSPACE/cobertura/`
- Cleans up exited containers

### Output

Coverage reports copied to:
```
$WORKSPACE/cobertura/
```

## Docker Build Steps

### BUILD_STEP=1 (Tests)

```dockerfile
# In Dockerfile
ARG BUILD_STEP
RUN if [ "$BUILD_STEP" = "1" ]; then \
    make test && \
    generate_coverage_reports; \
    fi
```

### BUILD_STEP=2 (Release)

```dockerfile
ARG BUILD_STEP
RUN if [ "$BUILD_STEP" = "2" ]; then \
    make release_build; \
    fi
```

## GitHub Actions Example

```yaml
name: CI

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Set workspace
        run: echo "WORKSPACE=$GITHUB_WORKSPACE" >> $GITHUB_ENV

      - name: Run tests
        run: bash scripts/ci/build-tests.sh

      - name: Upload coverage
        uses: actions/upload-artifact@v4
        with:
          name: coverage
          path: cobertura/

  build:
    runs-on: ubuntu-latest
    if: github.ref == 'refs/heads/main'
    steps:
      - uses: actions/checkout@v4

      - name: Build and push Docker image
        run: bash scripts/ci/build-release.sh
        env:
          BRANCH_NAME: ${{ github.ref_name }}
          DOCKER_USER: ${{ secrets.DOCKER_USERNAME }}
          DOCKER_PASS: ${{ secrets.DOCKER_PASSWORD }}
```

## Error Handling

All scripts use `set -e` to exit immediately on error:

```bash
#!/bin/bash
set -e  # Exit on any error
```

## Docker Cleanup

Scripts automatically clean up exited containers:

```bash
docker rm -v $(docker ps -a -q -f status=exited) || true
```

The `|| true` ensures cleanup failures don't fail the build.

## Security Notes

### Credentials Management

- Never commit credentials to repository
- Use CI platform secrets (GitHub Secrets, etc.)
- Pass via environment variables

### Docker Hub Authentication

```bash
# Login is automated in build-release.sh
echo "$DOCKER_TOKEN" | docker login --username=$DOCKER_USER --password-stdin
```

## Troubleshooting

### Build Failures

```bash
# Check Docker build logs
docker logs <container_id>

# Manual build test
docker build --build-arg BUILD_STEP=2 -t test .
```

### Coverage Reports Not Generated

```bash
# Verify WORKSPACE is set
echo $WORKSPACE

# Check container volume mount
docker run -v $WORKSPACE:/var/workspace zatterahub/zattera:tests ls -la /var/cobertura
```

### Docker Permission Issues

```bash
# Add user to docker group (if needed on local machine)
sudo usermod -aG docker $USER

# Log out and log back in for group changes to take effect
```

## Local Testing

### Test Build Script

```bash
export BRANCH_NAME=test
export DOCKER_USER=testuser
export DOCKER_TOKEN=testtoken
./build-release.sh
```

### Test Coverage Generation

```bash
export WORKSPACE=/tmp/test-workspace
mkdir -p $WORKSPACE
./run-tests.sh
ls -la $WORKSPACE/cobertura/
```

## Additional Resources

- [Dockerfile](../../Dockerfile) - Multi-stage build configuration
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Docker Documentation](https://docs.docker.com/)

## License

See [LICENSE](../../LICENSE.md)
