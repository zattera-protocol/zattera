# Contributing to Zattera

Thank you for your interest in contributing to Zattera! This document provides guidelines for contributing to the project.

## Code of Conduct

By participating in this project, you agree to maintain a respectful and collaborative environment. We value:
- Constructive feedback and discussion
- Professional communication
- Focus on technical merit
- Respect for all contributors

## How to Contribute

### Reporting Bugs

Before submitting a bug report:
1. **Search existing issues** to avoid duplicates
2. **Verify the bug** on the latest version
3. **Collect relevant information** (logs, system info, steps to reproduce)

When submitting a bug report, include:
- Clear, descriptive title
- Detailed description of the issue
- Steps to reproduce the behavior
- Expected vs actual behavior
- Environment details (OS, node version, build configuration)
- Relevant logs or screenshots
- Blockchain state information (block height, hardfork version)

### Suggesting Enhancements

For protocol-level changes, use the **ZEP (Zattera Enhancement Proposal)** process:
- Review existing [ZEPs](https://github.com/zattera-protocol/ZEPs)
- Follow the [ZEP template](https://github.com/zattera-protocol/ZEPs/blob/main/zep-template.md)
- Discuss in GitHub Issues before drafting a formal ZEP
- Submit ZEP as a pull request to the ZEPs repository

For implementation-level improvements:
- Open a GitHub Issue with the `enhancement` label
- Provide clear rationale and use cases
- Discuss potential implementation approaches
- Consider backward compatibility implications

### Pull Requests

We welcome pull requests! Follow these guidelines:

#### Before Submitting
1. **Check for existing work** - search open PRs and issues
2. **Discuss major changes** - open an issue first for significant modifications
3. **Review the codebase** - understand the architecture and coding patterns
4. **Read the documentation** - review relevant docs in `docs/` directory

#### PR Guidelines
1. **One feature per PR** - keep changes focused and atomic
2. **Write tests** - all new features and bug fixes must include tests
3. **Follow coding standards** - match existing code style
4. **Update documentation** - modify relevant docs when changing behavior
5. **Write clear commit messages** - describe what and why, not how

#### Commit Message Format
```
Short summary (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Explain the problem this commit solves and why you chose
this particular solution.

References: #issue-number
```

#### Testing Requirements
- All tests must pass: `./tests/chain_test && ./tests/plugin_test`
- Add unit tests for new operations or evaluators
- Add integration tests for API changes
- Test on a private testnet when possible
- Verify no memory leaks with Valgrind (for major changes)

#### Code Review Process
1. Automated checks must pass (build, tests, linting)
2. At least one core developer approval required
3. Address all review feedback or provide rationale
4. Maintain a clean commit history (squash if requested)
5. Rebase on latest `develop` before merge

## Development Workflow

### Branch Strategy
- `main` - stable release branch
- `develop` - active development branch
- `feature/*` - feature branches
- `bugfix/*` - bug fix branches
- `hotfix/*` - critical production fixes

### Getting Started
```bash
# Fork and clone
git clone https://github.com/YOUR_USERNAME/zattera.git
cd zattera
git remote add upstream https://github.com/zattera-protocol/zattera.git

# Create feature branch from develop
git checkout develop
git checkout -b feature/my-feature

# Build and test
git submodule update --init --recursive
mkdir build && cd build
cmake -DBUILD_ZATTERA_TESTNET=ON -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc) chain_test plugin_test
./tests/chain_test
```

### Making Changes
```bash
# Make your changes and test
# ... edit files ...
./tests/chain_test

# Commit with clear messages
git add .
git commit -m "Add feature X"

# Keep up to date with upstream
git fetch upstream
git rebase upstream/develop

# Push and create PR
git push origin feature/my-feature
```

## Coding Standards

### C++ Guidelines
- Follow C++17 standard
- Use smart pointers (avoid raw `new`/`delete`)
- Prefer `const` and `constexpr` when possible
- Use RAII for resource management
- Follow the existing naming conventions:
  - `snake_case` for functions and variables
  - `PascalCase` for classes and structs
  - `SCREAMING_SNAKE_CASE` for macros and constants

### Blockchain-Specific Patterns
- **Operations**: Stateless, no database access, implement `validate()`
- **Evaluators**: Stateful, all business logic, database modifications
- **Serialization**: Use `FC_REFLECT` for all serializable types
- **Database objects**: Copy-on-write, immutable during transaction
- **Thread safety**: Use read/write locks for database access
- **Hardforks**: Never break backward compatibility without hardfork

### Testing Standards
- Test file location: `tests/chain/` (chain), `tests/plugin/` (plugin)
- Use Boost.Test framework
- Name tests clearly: `<module>_<feature>_test`
- Test edge cases and error conditions
- Mock external dependencies when needed

## Documentation

Update documentation when:
- Adding new features or operations
- Changing API behavior
- Modifying configuration options
- Changing build requirements
- Adding new dependencies

Documentation locations:
- User guides: `docs/getting-started/`
- Operations guides: `docs/operations/`
- Development guides: `docs/development/`
- API documentation: inline code comments + `docs/api-reference.md`

## Issue Tracker Guidelines

### What Belongs in Issues
✅ Bug reports with reproduction steps
✅ Implementation discussion for active development
✅ Technical design questions
✅ Performance optimization proposals
✅ Security vulnerability reports (or email privately)

### What Doesn't Belong in Issues
❌ General questions (use community channels)
❌ Feature requests without implementation plan
❌ Lengthy debates on minor features
❌ Support requests for running nodes
❌ Business or marketing discussions

## Getting Help

- **Development questions**: Open a GitHub Discussion
- **Bug reports**: GitHub Issues
- **Security issues**: Email security@zattera.club (or open issue for non-critical)
- **Build problems**: Check [Building Guide](docs/getting-started/building.md)
- **Protocol questions**: Review [Zattera Whitepaper](https://zattera.club/docs/zattera-whitepaper.pdf)

## Recognition

Contributors who submit accepted pull requests will be:
- Listed in release notes
- Credited in commit history
- Recognized in the community

Significant contributions may result in invitation to the core development team.

## License

By contributing to Zattera, you agree that your contributions will be licensed under the MIT License. See [LICENSE.md](LICENSE.md) for details.

---

**Thank you for contributing to Zattera!** Every contribution, no matter how small, helps make the blockchain better for everyone.
