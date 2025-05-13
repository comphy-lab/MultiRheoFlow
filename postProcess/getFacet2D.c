/**
# Facet Extraction Tool for Multiphase Flow Simulations

This program extracts and outputs facets from a Basilisk simulation file,
primarily used for interface reconstruction in multiphase flow simulations.

In multiphase flow simulations, facets represent the reconstructed interface
between different phases (e.g., liquid-gas interfaces). These facets are
computed using Volume-of-Fluid (VOF) methods where the interface is
approximated by piecewise linear segments within each cell.

## Overview

The facet extraction process involves:
- Restoring a simulation state from a saved file
- Extracting interface facets from the volume fraction field
- Outputting facet data for visualization or post-processing

## Physical Context

In VOF methods, each computational cell contains a volume fraction `f` that
represents the proportion of the cell occupied by one phase (typically the
liquid phase). Facets are linear approximations of the interface within
cells where `0 < f < 1`, computed using interface reconstruction algorithms
such as PLIC (Piecewise Linear Interface Calculation).

## Usage

```bash
./getFacet2D <input_file>
```

Where `input_file` is a Basilisk simulation output file containing volume
fraction data.

## Author Information

- **Author**: Vatsal Sanjay
- **Email**: vatsalsanjay@gmail.com
- **Affiliation**: Physics of Fluids Group
- **Date**: 2025-05-13

## Dependencies

This program requires the following Basilisk modules:
- `utils.h`: Core utilities and data structures
- `output.h`: Output functions including facet output routines
- `fractions.h`: Volume fraction handling and interface reconstruction
*/

#include "utils.h"
#include "output.h"
#include "fractions.h"

/**
## Global Variables

### Volume Fraction Field
*/
scalar f[];    /**< Volume fraction field representing the liquid phase */

/**
### Filename Buffer
*/
char filename[80];    /**< Buffer to store input filename from command line */

/**
## Main Function

### Purpose
Extracts facets from a Basilisk simulation file and outputs them to stderr.
This is the primary entry point for the facet extraction process.

### Implementation Details
The function performs the following operations:
1. Parses command line arguments to get the input filename
2. Restores the simulation state from the specified file
3. Extracts and outputs facets from the volume fraction field
4. Ensures output is flushed and closed properly

### Parameters
- `a`: Number of command line arguments
- `arguments`: Array of command line argument strings
  - `arguments[0]`: Program name (unused)
  - `arguments[1]`: Input simulation file path

### Return Value
Returns `EXIT_SUCCESS` (0) on successful completion, or `EXIT_FAILURE` if
an error occurs during file operations.

### Side Effects
- Reads and modifies program state by restoring simulation data
- Outputs facet data to `stderr`
- May produce error messages if file operations fail

### Error Handling
Basic error handling is provided through Basilisk's internal mechanisms.
The program will exit with an error code if the input file cannot be read.

### Notes
- Output is directed to `stderr` (`ferr`) rather than `stdout` to separate
  data output from potential error messages
- The output format depends on the `output_facets()` function implementation
- Facet output typically includes coordinates, normals, and other interface
  properties for visualization tools
*/
int main(int a, char const *arguments[]) {
  // Parse command line argument for input filename
  sprintf(filename, "%s", arguments[1]);

  // Restore simulation state from file
  restore(file = filename);

  // Set output destination to stderr
  FILE * fp = ferr;

  // Extract and output facets from volume fraction field
  output_facets(f, fp);

  // Ensure all output is written
  fflush(fp);

  // Close file pointer (stderr alias)
  fclose(fp);

  return 0;
}
