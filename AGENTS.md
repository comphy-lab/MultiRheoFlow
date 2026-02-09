# MultiRheoFlow Development Guidelines

## Project Structure
- `basilisk/src/`: Core Basilisk CFD library (reference only, do not
  modify). If you keep a local Basilisk checkout, leave it untracked in
  `basilisk/`.
- `src-local/`: Project-specific Basilisk headers and helpers.
- `simulationCases/`: Main simulation entry points and case utilities.
- `postProcess/`: Post-processing and visualization tools.
- `.github/`: Documentation and CI tooling (generated docs go to
  `.github/docs/`).

## Build & Test Commands
- Compile single file: `qcc -O2 -Wall -disable-dimensions file.c -o executable -lm`
- Compile with custom headers: `qcc -O2 -Wall -disable-dimensions -I$PWD/src-local file.c -o executable -lm`
- Run a case with make: `cd simulationCases && make case_name.tst`
- Run a case with helper script: `cd simulationCases && bash runCases.sh case_name`
- Run a case with custom params: `cd simulationCases && bash runCases.sh case_name custom.params`
- Cleanup case output: `cd simulationCases && bash cleanup.sh case_name`
- Generate documentation (no deploy): `bash .github/scripts/build.sh`

## Documentation Generation
- Do not edit `.github/docs/` directly (generated output).
- Use `.github/scripts/build.sh` for local builds.
- See `.github/Website-generator-readme.md` for usage.
- Customize assets in `.github/assets/**` and scripts in
  `.github/scripts/**`.

## Code Style
- **Indentation**: 2 spaces (no tabs).
- **Line Length**: Maximum 80 characters per line.
- **Comments**: Use markdown in comments starting with `/**`.
- **Spacing**: Include spaces after commas and around operators.
- **File Organization**:
  - Place core functionality in `.h` headers.
  - Implement tests in `.c` files.
- **Naming Conventions**:
  - Use `snake_case` for variables and parameters.
  - Use `camelCase` for functions and methods.
- **Error Handling**: Return meaningful values and provide descriptive
  `stderr` messages.
