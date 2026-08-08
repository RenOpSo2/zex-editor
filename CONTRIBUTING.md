# Contributing to Noxe-Editor

Thank you for your interest in contributing to Noxe-Editor!

## Getting Started

1. Fork the repository
2. Clone your fork: `git clone <your-fork-url>`
3. Create a branch: `git checkout -b feature/your-feature-name`
4. Make your changes
5. Build and test: `make && make test`
6. Commit your changes
7. Push to your fork: `git push origin feature/your-feature-name`
8. Open a pull request

## Development

### Building

```bash
make           # Build the project
make run       # Build and run
make test      # Build and run tests
make clean     # Remove build artifacts
make format    # Format source code
```

### Code Style

- Follow the existing code style
- Use `make format` to format your code
- Write clear, readable code with meaningful variable names

### Testing

Run tests before submitting:

```bash
make test
```

## Guidelines

- Keep changes focused and minimal
- Write descriptive commit messages
- Update documentation if needed
- Test your changes thoroughly

## Reporting Issues

Please check existing issues before creating new ones. Include:
- Steps to reproduce
- Expected behavior
- Actual behavior
- Environment details

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
