# Integration Tests

This directory contains integration tests that validate the Zattera blockchain across multiple components and scenarios.

## Test Categories

- **smoke/** - Node upgrade regression tests and blockchain replay comparison tests
- **api/** - API response validation tests comparing two zatterad instances

## Smoke Tests

Smoke tests validate that node upgrades and blockchain replays work correctly.

```bash
cd integration/smoke
./smoketest.sh
```

## API Tests

Python-based tests that compare API responses between two zatterad instances to ensure backward compatibility.

```bash
cd integration/api
python3 api_test.py
```

See individual subdirectories for detailed documentation.
