# MultiRheoFlow

_An extensible framework for multiphase flows with complex rheology, built on
Basilisk C_

## Overview

MultiRheoFlow is an open-source multiphase, multirheology extension of the
[ElastoFlow](https://github.com/comphy-lab/Viscoelastic3D/releases/tag/v2.5.1)
solver. It provides a catalogue of models implemented in
[Basilisk C](https://basilisk.fr), leveraging adaptive mesh refinement for
high-fidelity simulations of non-Newtonian fluids and viscoelastic materials
with free surfaces, interfaces, and capillary effects.

Developed at the
[Computational Multiphase Physics (CoMPhy) Lab](https://comphy-lab.org/) at the
University of Twente, MultiRheoFlow provides a modular platform where
different rheological constitutive equations can be integrated with multiphase
flow scenarios.

## Key Features

- **Diverse Rheology Models**: Simulates a wide spectrum of material
  behaviors:
  - Newtonian fluids (done ✅)
  - Viscoelastic fluids (Oldroyd-B - done ✅, Giesekus - good to have 🍀,
    FENE-P - urgent 📌)
  - Yield-stress materials (Bingham - done ✅, Herschel-Bulkley - done ✅)
  - Elastoviscoplastic materials (Saramito model - good to have 🍀)
- **Multiphase Capabilities**: Handles interfacial dynamics with:
  - Two-phase and multiphase flow support
  - Accurate surface tension and capillary effects
  - Interface capturing via the volume-of-fluid method
  - Arbitrary density and viscosity contrasts
- **High-Performance Computing**: Leverages Basilisk's advantages:
  - Adaptive mesh refinement for computational efficiency
  - Parallelization for large-scale simulations
  - 2D, axisymmetric, and 3D simulation capabilities
- **Log-Conformation Method**: Ensures stability for highly elastic flows
  using advanced numerical techniques

## Test Cases

- `simulationCases/dropImpact.c`
- `simulationCases/dropAtomisation.c`
- `simulationCases/pinchOff.c`
- `simulationCases/testEigenDecomposition.c`
- Parameter files such as `simulationCases/Bo0.0010.dat`

## Repository Structure

- `src-local/`: Project-specific Basilisk headers and helpers.
- `simulationCases/`: Simulation entry points and case utilities.
- `postProcess/`: Post-processing and visualization tools.
- `.github/`: Documentation and CI tooling (generated docs go to
  `.github/docs/`).
- Root scripts such as `reset_install_requirements.sh`.

## Running the Code

### Install Basilisk

- Follow the instructions [here](http://basilisk.fr/src/INSTALL).
- For macOS and Linux, you can use `reset_install_requirements.sh`.
- For Windows, we recommend using WSL; please report any issues you find.

```bash
# Prerequisites: Basilisk installation (http://basilisk.fr)
git clone https://github.com/comphy-lab/RheoMultiFlow.git
cd RheoMultiFlow
curl -sL https://raw.githubusercontent.com/comphy-lab/basilisk-C/main/reset_install_basilisk-ref-locked.sh | bash -s -- --ref=v2026-01-29 --hard
```

Update `v2026-01-29` with the latest version.

### Compile a Case with Make

```bash
cd simulationCases
make dropImpact.tst
```

To disable on-the-fly visualization:

```bash
cd simulationCases
CFLAGS=-DDISPLAY=-1 make dropImpact.tst
```

### Run via Helper Scripts

```bash
cd simulationCases
bash runCases.sh dropImpact
bash cleanup.sh dropImpact
```

### Compile and Run from CLI

```bash
qcc -O2 -Wall -disable-dimensions -I$PWD/src-local \
  simulationCases/dropImpact.c -o dropImpact -lm
./dropImpact
```

### Compile and Run with MPI (macOS)

> Note: you should have OpenMPI installed.

```bash
CC99='mpicc -std=c99 -D_GNU_SOURCE=1' qcc -Wall -O2 -D_MPI=1 \
  -disable-dimensions -I$PWD/src-local simulationCases/dropImpact.c \
  -o dropImpact -lm
mpirun -np $NUM_PROCESSORS_TO_USE ./dropImpact
```

## Post-Processing

- `postProcess/VideoAxi.py`
- `postProcess/getData-elastic-scalar2D.c`
- `postProcess/getFacet2D.c`

## Documentation

Generate the documentation locally:

```bash
.github/scripts/build.sh
```

Preview locally:

```bash
.github/scripts/deploy.sh
```

## Contributing

Contributions to MultiRheoFlow are welcome! Please see our
[Contributing](Contributing.md) guide for details on how to submit code,
report bugs, or request features.

## License

This project is licensed under the GNU GPLv3 License. See the
[LICENSE](LICENSE) file for details, consistent with Basilisk's licensing.

## Acknowledgments

MultiRheoFlow builds upon ElastoFlow and the Basilisk C framework.
