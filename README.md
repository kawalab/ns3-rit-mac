# ns-3 RIT MAC Implementation and Analysis Scripts

## Overview

This repository provides the implementation and analysis scripts used in our study of a
**Receiver-Initiated (RIT) MAC protocol compliant with IEEE 802.15.4e-2012 and IEEE802.15.4-2020**, developed on top of the
**ns-3 network simulator**.

It accompanies and supports the ns-3 implementation used in: Murata et al., "An ns-3 Evaluation Framework for Receiver-Initiated MAC Protocols with Configurable Enhancement Modules across Various Network Scenarios", Sensors, 2026. https://doi.org/10.3390/s26010164

The repository contains only the **newly implemented or modified components** required to support
RIT-based MAC operation, extracted from the original ns-3 source tree, as well as
Python scripts used for simulation result analysis and figure generation.

The ns-3 core framework and standard modules are **not included**.

## Purpose of This Repository

The primary purposes of this repository are:

- To ensure **reproducibility** of the simulation results presented in the corresponding paper
- To clearly document the **implementation details and design choices** of an RIT-based MAC in ns-3
- To provide a **reference implementation and evaluation framework** for future research on
  receiver-initiated MAC protocols

This code is intended for **research and evaluation purposes**, not for direct deployment in
production systems.

## Directory Structure
```
.
├── analysis
│ ├── single_run_analysis.ipynb
│ ├── multi_run_analysis.ipynb
│ └── ...
│
├── ns3-rit-mac
│ │
│ ├── lr-wpan/
│ └── rit-wpan/
│
└── README.md
```

### analysis

This directory contains **Python and Jupyter Notebook–based analysis scripts** used to process
ns-3 simulation outputs, including:

- Log aggregation and statistical processing
- Computation of performance metrics (e.g., PDR, End-to-End Latency, Wakeup Ratio)
- Generation of figures used in the paper

All figures reported in the paper were generated using these scripts.

### ns3-rit-mac

This directory includes only the parts that were **newly implemented or modified** for this study:

- MAC-layer extensions enabling RIT-based operation (`rit-wpan`)
- Extensions and adjustments to the existing `lr-wpan` module in ns-3
- Logging extensions for detailed MAC-layer behavior analysis
  (raw logs are generated under a `logs/` directory inside the ns-3 working directory)

The contents are intended to be **directly copied into the `src` directory** of an ns-3
installation.

## Usage

### 1. Set up ns-3

First, set up an ns-3 build environment following the official ns-3 documentation.

- Supported ns-3 version: `ns-3.46`

### 2. Install the RIT MAC Implementation

You can install the RIT MAC implementation in one of the following ways.

#### Option A: Copy the source directories

Copy the contents of `ns3-rit-mac/` into the `src` directory of your ns-3 installation.

```
ns-3/
└── src/
├── lr-wpan/
└── rit-wpan/
```

#### Option B: Apply the patch file (simplest)

Using `ns3.46-rit-mac.patch`, it can be applied directly to a clean ns-3.46 source tree.

```bash
cd ns-3.46
git apply /path/to/ns3.46-rit-mac.patch
```

This will automatically add and modify the required source files under src/.

### 3. Install Simulation Scenarios

Copy the scenario files provided in the `example` directory into the `scratch` directory of ns-3.

```
ns-3/
└── scratch/
    └── rit-xxx-scenario.cc
```

### 4. Build ns-3

From the root directory of ns-3, run:

```bash
./ns3 configure -d release
./ns3 build
```

### 5. Place the Analysis Directory

Place the analysis directory in parallel with the ns-3 directory.

```
workspace/
├── ns-3/
└── analysis/
```

### 6. Run Analysis

Use the following notebooks in the analysis directory:

- single_run_analysis.ipynb
For analysis of a single scenario and a single simulation run

- multi_run_analysis.ipynb
For statistical analysis across multiple scenarios and random seeds

Simulation scenarios and parameters can be configured directly within each notebook.

## Notes

This implementation is designed for research and simulation purposes

Robustness and performance optimizations are intentionally limited

Compatibility with future versions of ns-3 is not guaranteed

For detailed protocol behavior and design rationale, please refer to the corresponding paper

## License

This repository is licensed under the **GNU General Public License v2.0 (GPL-2.0-only)**.

The implementation provided here is derived from and extends the ns-3 network simulator,
which is distributed under the GPL-2.0 license.
To remain consistent with the licensing terms of ns-3, all newly implemented and modified
source code in this repository is released under the same license.
