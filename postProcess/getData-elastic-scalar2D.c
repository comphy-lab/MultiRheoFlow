/**
# Simulation Snapshot Data Extractor for Multiphase Fluid Dynamics

This program extracts fluid dynamic properties from simulation snapshots,
specifically designed for analyzing deformable soft matter systems including
liquid drops, sheets, and bubbles. The code computes various tensor fields
relevant to non-Newtonian fluid dynamics and viscoelastic material behavior.

## Physical Quantities Computed

### Deformation Rate Tensor Analysis
- Computes the symmetric deformation rate tensor components D_ij
- Calculates the second invariant of the deformation rate tensor
- Applies logarithmic scaling for visualization of wide dynamic ranges

### Conformation Tensor Properties
- Extracts viscoelastic polymer conformation tensor components
- Computes trace deviations from equilibrium state
- Logarithmic scaling for polymer stretching visualization

### Velocity Field Analysis
- Calculates velocity magnitude from vector components
- Provides interpolation capabilities for field extraction

## Author Information
- **Author:** Vatsal Sanjay
- **Email:** vatsalsanjay@gmail.com
- **Affiliation:** Physics of Fluids Group
- **Institution:** University of Twente
*/

#include "utils.h"
#include "output.h"

/**
## Global Variables and Field Declarations

### Scalar Fields
- `f[]`: Volume fraction field for multiphase identification
- `conform_qq[]`: Out-of-plane conformation tensor component (q,q)
- `D2c[]`: Logarithm of second invariant of deformation rate tensor
- `vel[]`: Velocity magnitude field
- `trA[]`: Logarithm of trace deviation of conformation tensor

### Vector Fields
- `u[]`: Velocity field components (u.x, u.y)

### Conformation Tensor Components
- `A11[]`: In-plane conformation tensor component (1,1)
- `A12[]`: In-plane conformation tensor component (1,2)
- `A22[]`: In-plane conformation tensor component (2,2)

### Simulation Parameters
- `filename[80]`: Input snapshot filename
- `nx, ny`: Grid resolution in x and y directions
- `len`: Number of scalar fields in output list
- `xmin, ymin, xmax, ymax`: Domain boundaries
- `Deltax, Deltay`: Grid spacing in x and y directions
*/
scalar f[];
vector u[];
scalar A11[], A12[], A22[];
scalar conform_qq[];
char filename[80];
int nx, ny, len;
double xmin, ymin, xmax, ymax, Deltax, Deltay;
scalar D2c[], vel[], trA[];
scalar * list = NULL;

/**
## Main Function

### Purpose
Extracts and processes fluid dynamic fields from simulation snapshots,
computing deformation rates, velocity magnitudes, and polymer conformation
properties for visualization and analysis.

### Arguments
- `a`: Number of command line arguments
- `arguments[]`: Command line argument array containing:
  - `arguments[1]`: Input snapshot filename
  - `arguments[2]`: Minimum x-coordinate (xmin)
  - `arguments[3]`: Minimum y-coordinate (ymin)
  - `arguments[4]`: Maximum x-coordinate (xmax)
  - `arguments[5]`: Maximum y-coordinate (ymax)
  - `arguments[6]`: Number of grid points in y-direction (ny)

### Physical Implementation Details

#### Deformation Rate Tensor Calculation
The deformation rate tensor components are computed using finite differences:
- `D11 = ∂u_y/∂y`: Extensional rate in y-direction
- `D22 = u_y/y`: Azimuthal extensional rate (cylindrical coordinates)
- `D33 = ∂u_x/∂x`: Extensional rate in x-direction
- `D13 = 0.5(∂u_y/∂x + ∂u_x/∂y)`: Shear rate component

#### Second Invariant Computation
The second invariant of the deformation rate tensor:
```
D2 = D11² + D22² + D33² + 2×D13²
```

#### Logarithmic Scaling
For visualization across multiple decades:
```
log_scale = log₁₀(value) if value > 0, else -10
```

### Return Value
- `0` on successful execution
- Non-zero on error (implicit from system)

*/
int main(int a, char const *arguments[])
{
  // Parse command line arguments
  sprintf (filename, "%s", arguments[1]);
  xmin = atof(arguments[2]); ymin = atof(arguments[3]);
  xmax = atof(arguments[4]); ymax = atof(arguments[5]);
  ny = atoi(arguments[6]);

  // Build output field list
  list = list_add (list, D2c);
  list = list_add (list, vel);
  list = list_add (list, trA);

  // Load simulation snapshot
  restore (file = filename);

  /**
  ### Computational Loop for Field Processing

  This loop processes each grid cell to compute:
  - Deformation rate tensor components using central differences
  - Second invariant of deformation rate tensor
  - Velocity magnitude
  - Polymer conformation trace deviation
  */
  foreach() {
    // Compute deformation rate tensor components
    double D11 = (u.y[0,1] - u.y[0,-1])/(2*Delta);
    double D22 = (u.y[]/y);
    double D33 = (u.x[1,0] - u.x[-1,0])/(2*Delta);
    double D13 = 0.5*( (u.y[1,0] - u.y[-1,0] + u.x[0,1] - u.x[0,-1])/(2*Delta) );

    // Calculate second invariant
    double D2 = (sq(D11)+sq(D22)+sq(D33)+2.0*sq(D13));
    D2c[] = f[]*D2;  // Mask with volume fraction

    // Apply logarithmic scaling for deformation rate
    if (D2c[] > 0.){
      D2c[] = log(D2c[])/log(10);
    } else {
      D2c[] = -10;
    }

    // Compute velocity magnitude
    vel[] = sqrt(sq(u.x[])+sq(u.y[]));

    // Compute polymer conformation trace deviation
    trA[] = (A11[] + A22[] + conform_qq[])/3.0 - 1.0;
    if (trA[] > 0.){
      trA[] = log(trA[])/log(10);
    } else {
      trA[] = -10;
    }
  }

  /**
  ### Grid Setup and Data Extraction

  Creates a uniform grid for data extraction and interpolation.
  The grid spacing is determined by the specified y-resolution,
  maintaining aspect ratio for the x-direction.
  */
  FILE * fp = ferr;
  Deltay = (double)((ymax-ymin)/(ny));
  nx = (int)((xmax - xmin)/Deltay);
  Deltax = (double)((xmax-xmin)/(nx));
  len = list_len(list);

  // Allocate memory for extracted field data
  double ** field = (double **) matrix_new (nx, ny+1, len*sizeof(double));

  /**
  ### Field Interpolation Loop

  Interpolates field values at regular grid points using the
  simulation's interpolation routines. This provides smooth
  field representations suitable for visualization.
  */
  for (int i = 0; i < nx; i++) {
    double x = Deltax*(i+1./2) + xmin;
    for (int j = 0; j < ny; j++) {
      double y = Deltay*(j+1./2) + ymin;
      int k = 0;
      for (scalar s in list){
        field[i][len*j + k++] = interpolate (s, x, y);
      }
    }
  }

  /**
  ### Data Output

  Writes the extracted field data in ASCII format:
  - First two columns: x and y coordinates
  - Subsequent columns: Field values in order added to list
  */
  for (int i = 0; i < nx; i++) {
    double x = Deltax*(i+1./2) + xmin;
    for (int j = 0; j < ny; j++) {
      double y = Deltay*(j+1./2) + ymin;
      fprintf (fp, "%g %g", x, y);
      int k = 0;
      for (scalar s in list){
        fprintf (fp, " %g", field[i][len*j + k++]);
      }
      fputc ('\n', fp);
    }
  }

  // Cleanup
  fflush (fp);
  fclose (fp);
  matrix_free (field);
}
