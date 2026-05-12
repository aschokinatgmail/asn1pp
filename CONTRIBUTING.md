# Contributing to asn1pp

Thank you for your interest in contributing to asn1pp.

asn1pp is a zero-dependency, header-only ASN.1 codec library written in modern C++20.
This document describes the expectations for contributors.

## Code of Conduct

We are committed to fostering a welcoming and respectful community. All contributors
are expected to follow our Code of Conduct, which is based on the
[Contributor Covenant Code of Conduct](https://www.contributor-covenant.org/version/2/1/code_of_conduct/).
Please read it before participating.

## Development Setup

### Requirements

- CMake 3.20 or later
- A C++20-compliant compiler (GCC 11+, Clang 14+, MSVC 2022+)
- Git

### Building

Clone the repository and build using CMake:

```bash
git clone https://github.com/aschokinatgmail/asn1pp.git
cd asn1pp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

To build with a specific compiler:

```bash
cmake .. -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Running Tests

After building, run the test suite:

```bash
cd build
ctest --output-on-failure
```

Or run individual test executables:

```bash
./test/smoke_test
./test/tlv_test
./test/buffer_view_test
```

## Coding Style

asn1pp enforces strict compiler flags to maintain high code quality.

### Compiler Flags

- `-Wall -Wextra -Wpedantic -Werror` on GCC and Clang
- `/W4 /WX` on MSVC

All code must compile without warnings at the highest warning level.

### C++ Standard

- C++20 is required
- No exceptions
- No RTTI (typeid, dynamic_cast)
- No heap allocation in hot paths (embedded profile)

### Formatting

- Use 4 spaces for indentation
- Prefer `snake_case` for variables and functions
- Prefer `PascalCase` for types and enums
- Use `kebab-case` or `snake_case` for file names
- Keep lines under 120 characters when practical
- Always use braces for if/while/for statements, even single-line bodies

### Header Organization

- Headers in `include/asn1pp/`
- Implementation in `src/` (compiled into static library)
- Tests in `test/`
- Use include guards, not `#pragma once`

## Branching and Commits

### Branch Naming

- `main` — stable release branch
- `feature/*` — new features
- `fix/*` — bug fixes
- `refactor/*` — refactoring without behavior change

### Commit Messages

asn1pp follows the [Conventional Commits](https://www.conventionalcommits.org/) specification.

Format:

```
<type>(<scope>): <description>

[optional body]
```

### Type Prefixes

| Type | Use For |
|------|---------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `style` | Formatting, whitespace |
| `refactor` | Code restructuring |
| `test` | Adding or updating tests |
| `perf` | Performance improvement |
| `build` | Build system changes |
| `ci` | CI/CD changes |

### Examples

```
feat(ber): add SIMD batch encode for BER/TLV
fix(tlv): correct length field calculation for values > 65535
docs(readme): update benchmark numbers
test(buffer): add tests for zero-copy buffer_view
```

## Pull Request Process

### Before Submitting

1. Ensure all tests pass locally (`ctest --output-on-failure`)
2. Verify your changes compile without warnings
3. Confirm the code follows the coding style
4. Keep commits focused and atomic
5. Write clear commit messages following conventional commits

### PR Description

Every pull request should include:

- A brief description of what changed and why
- Reference any related issues (e.g., "Fixes #42")
- Note any breaking changes
- List any new dependencies added

### Review Process

- PRs require at least one review approval before merging
- Address all reviewer feedback before merging
- Do not force-push to branches with open PRs

## Testing Guidelines

### Test Coverage

- New features must include tests
- Bug fixes must include a regression test
- aim for meaningful coverage, not checkbox coverage

### Test Organization

Tests live in `test/` and are built with GoogleTest.

```cpp
#include <gtest/gtest.h>

TEST(MyFeature, encodes_valid_input) {
    // test body
}
```

### Running Specific Tests

```bash
# Run all tests
ctest

# Run a specific test executable
./build/test/tlv_test --gtest_filter="MyFeature.*"
```

## License

By contributing to asn1pp, you agree that your contributions will be licensed
under the MIT License. See the `LICENSE` file for details.

## Getting Help

If you have questions or need guidance:

- Open an issue on GitHub for bugs or feature requests
- Use the repository discussions for questions
- Follow the existing code patterns for consistency
