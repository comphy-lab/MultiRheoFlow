# RheoMultiFlow

_An extensible framework for multiphase flows with complex rheology, built on Basilisk C_

## Overview

RheoMultiFlow is an open-source multiphase multirheology extension of the [ElastoFlow](https://github.com/comphy-lab/Viscoelastic3D/releases/tag/v2.5.1) solver. Here, we provide a catalogue of several models implemnented in [Basilisk C](http://basilisk.fr). Building upon Basilisk's powerful adaptive mesh capabilities, this framework enables high-fidelity simulations of non-Newtonian fluids and viscoelastic materials with free surfaces, interfaces, and capillary effects.


y the [Computational Multiphase Physics (CoMPhy) Lab](https://comphy-lab.org/) at the University of Twente, RheoMultiFlow provides a modular platform where different rheological constitutive equations can be seamlessly integrated with multiphase flow scenarios.

## Key Features

- **Diverse Rheology Models**: Simulates a wide spectrum of material behaviors:
    - Newtonian fluids (done ✅)
    - Viscoelastic fluids (Oldroyd-B - done ✅, Giesekus - good to have 🍀, FENE-P - urgent 📌)
    - Yield-stress materials (Bingham - done ✅, Herschel-Bulkley - done ✅)
    - Elastoviscoplastic materials (Saramito model - good to have 🍀)

- **Multiphase Capabilities**: Handles interfacial dynamics with: 
    - Two-phase and multiphase flow support
    - Accurate surface tension and capillary effects
    - Interface tracking via Volume-of-Fluid method
    - Large Density and viscosity contrasts

- **High-Performance Computing**: Leverages Basilisk's advantages:
    - Adaptive mesh refinement for computational efficiency
    - Parallelization for large-scale simulations
    - 2D, Axisymmetric, and 3D simulation capabilities

- **Log-Conformation Method**: Ensures stability for highly elastic flows using advanced numerical techniques 

## Test Cases

RheoMultiFlow is particularly well-suited for simulating:


## Installation

```bash
# Prerequisites: Basilisk installation (http://basilisk.fr)
git clone https://github.com/comphy-lab/RheoMultiFlow.git
cd RheoMultiFlow
make
```

## Contributing

Contributions to RheoMultiFlow are welcome! Please see our [Contributing](Contributing.md) guide for details on how to submit code, report bugs, or request features.

## License

This project is licensed under the GNU GPLv3 License - see the [LICENSE](LICENSE) file for details, consistent with Basilisk's licensing.

## Acknowledgments

RheoMultiFlow builds upon ElastoFlow and the Basilisk C framework.
