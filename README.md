# MultiRheoFlow

_An extensible framework for multiphase flows with complex rheology, built on Basilisk C_

## Overview

RheoMultiFlow is an open-source multiphase multirheology extension of the [ElastoFlow](https://github.com/comphy-lab/Viscoelastic3D/releases/tag/v2.5.1) solver. Here, we provide a catalogue of several models implemnented in [Basilisk C](http://basilisk.fr). Building upon Basilisk's powerful adaptive mesh capabilities, this framework enables high-fidelity simulations of non-Newtonian fluids and viscoelastic materials with free surfaces, interfaces, and capillary effects.


Developed at the [Computational Multiphase Physics (CoMPhy) Lab](https://comphy-lab.org/) at the University of Twente, RheoMultiFlow provides a modular platform where different rheological constitutive equations can be seamlessly integrated with multiphase flow scenarios.

## Key Features

- **Diverse Rheology Models**: Simulates a wide spectrum of material behaviors:
    - Newtonian fluids (done ✅)
    - Viscoelastic fluids (Oldroyd-B - done ✅, Giesekus - good to have 🍀, FENE-P - urgent 📌)
    - Yield-stress materials (Bingham - done ✅, Herschel-Bulkley - done ✅)
    - Elastoviscoplastic materials (Saramito model - good to have 🍀)

- **Multiphase Capabilities**: Handles interfacial dynamics with: 
    - Two-phase and multiphase flow support
    - Accurate surface tension and capillary effects
    - Interface capturing via Volume-of-Fluid method
    - Arbitary Density and viscosity contrasts

- **High-Performance Computing**: Leverages Basilisk's advantages:
    - Adaptive mesh refinement for computational efficiency
    - Parallelization for large-scale simulations
    - 2D, Axisymmetric, and 3D simulation capabilities

- **Log-Conformation Method**: Ensures stability for highly elastic flows using advanced numerical techniques 

## Test Cases

RheoMultiFlow is particularly well-suited for simulating:


## Running the code

### Install Basilisk. 

- Follow the instructions [here](http://basilisk.fr/src/INSTALL) to install Basilisk. 
- For MacOS and Linux (we have tested some but not all the different Linux distros), you can use the `reset_install_requirements.sh` script to install Basilisk. 
- For Windows, we recommend using the Windows Subsystem for Linux (WSL). However, people have reported issues with this. If you find issues, please let us know. 

```bash
# Prerequisites: Basilisk installation (http://basilisk.fr)
git clone https://github.com/comphy-lab/RheoMultiFlow.git
cd RheoMultiFlow
bash reset_install_requirements.sh --hard # use --hard to force the installation from scratch.
```

### Compile the code

#### Using makefile and visualization on the fly

```bash
cd simulationCases/
CFLAGS=-DDISPLAY=-1 make caseToRun.tst
```

#### Using makefile but no visualization on the fly

```bash
cd simulationCases/
make caseToRun.tst
```

#### Using bash script

- Use the `run_case.sh` script to run a case. 

```bash
bash run_case.sh caseToRun # no display.
```

- Compile and run from cli

```bash
qcc -O2 -Wall -disable-dimensions caseToRun.c -o caseToRun -lm
./caseToRun
```

- Compile and run with MPI (MACOS)

> Note: you should have OpenMPI installed. 

```bash
CC99='mpicc -std=c99 -D_GNU_SOURCE=1' qcc -Wall -O2 -D_MPI=1 -disable-dimensions caseToRun.c -o caseToRun -lm
mpirun -np $NUM_PROCESSORS_TO_USE ./caseToRun
```



## Contributing

Contributions to RheoMultiFlow are welcome! Please see our [Contributing](Contributing.md) guide for details on how to submit code, report bugs, or request features.

## License

This project is licensed under the GNU GPLv3 License - see the [LICENSE](LICENSE) file for details, consistent with Basilisk's licensing.

## Acknowledgments

RheoMultiFlow builds upon ElastoFlow and the Basilisk C framework.
