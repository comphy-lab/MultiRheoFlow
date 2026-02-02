# Contributing to MultiRheoFlow

## Getting Started
1. Fork the repository and clone your fork
2. Create a feature branch: `git checkout -b feature/your-feature`
3. Follow the project structure and code style guidelines below
4. Installing Basilisk C:

```
curl -sL https://raw.githubusercontent.com/comphy-lab/basilisk-C/main/reset_install_basilisk-ref-locked.sh | bash -s -- --ref=v2026-01-13 --hard
```

Replace `v2026-01-13` with the latest version from [Basilisk C](https://github.com/comphy-lab/basilisk-C).

## Project Structure
- `basilisk/src/`: Core Basilisk CFD library (reference only, do not modify)
- `src-local/`: Project-specific Basilisk headers and helpers
- `simulationCases/`: Main simulation entry points and case utilities
- `postProcess/`: Post-processing and visualization tools
- `.github/`: Documentation and CI tooling

## Code Style Guidelines
- **Indentation**: 2 spaces (no tabs)
- **Line Length**: Maximum 80 characters per line
- **Naming**: `snake_case` for variables, `camelCase` for functions
- **Comments**: Use markdown in comments starting with `/**`
- **Spacing**: Include spaces after commas and around operators
- **Error Handling**: Return meaningful values with descriptive `stderr` messages

## Building & Testing
```bash
# Compile single file
qcc -O2 -Wall -disable-dimensions file.c -o executable -lm

# Compile with custom headers
qcc -O2 -Wall -disable-dimensions -I$PWD/src-local file.c -o executable -lm

# Run simulation cases
cd simulationCases && make case_name.tst
cd simulationCases && bash runCases.sh case_name

# Cleanup
cd simulationCases && bash cleanup.sh case_name
```

## Documentation
- Do not edit `.github/docs/` directly (auto-generated)
- Run `bash .github/scripts/build.sh` for local documentation builds
- Customize assets in `.github/assets/` and scripts in `.github/scripts/`

## Submitting Changes
1. Commit with clear, descriptive messages
2. Push to your fork and open a pull request
3. Ensure all tests pass and code follows style guidelines
4. Request review from maintainers
