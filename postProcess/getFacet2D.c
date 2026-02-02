/**
# Facet Extraction (2D)

Restores a Basilisk snapshot and outputs VOF interface facets for
visualization or post-processing.

## Usage

```bash
./getFacet2D <input_file>
```

## Dependencies

- `utils.h`
- `output.h`
- `fractions.h`

## Author

Vatsal Sanjay (vatsal.sanjay@comphy-lab.org)
CoMPhy Lab
Date: 2025-05-13
*/

#include "utils.h"
#include "output.h"
#include "fractions.h"

/**
## Globals

- `f[]`: Volume fraction field (liquid phase)
- `filename[80]`: Input snapshot filename
*/
scalar f[];
char filename[80];

/**
### main()

Restores a snapshot and writes interface facets to `stderr`.

#### Args

- `a`: Number of CLI arguments.
- `arguments[1]`: Input snapshot filename.

#### Returns

- `0` on success.

#### Notes

- Output uses `output_facets()` and is written to `ferr`.
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
