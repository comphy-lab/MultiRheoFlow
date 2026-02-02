/**
# Snapshot Data Extractor (Elastic Scalar 2D)

Extracts scalar diagnostics from Basilisk snapshots for
viscoelastic/multiphase cases. Computes deformation-rate invariants,
velocity magnitude, and conformation-trace deviations.

## Diagnostics
- Deformation-rate invariant `D2c` (log10-scaled)
- Velocity magnitude `vel`
- Conformation trace deviation `trA` (log10-scaled)

## Author
Vatsal Sanjay (vatsal.sanjay@comphy-lab.org)
CoMPhy Lab
*/

#include "utils.h"
#include "output.h"

/**
## Globals

- `f[]`: Volume fraction field
- `u[]`: Velocity components
- `A11[]`, `A12[]`, `A22[]`: In-plane conformation components
- `conform_qq[]`: Out-of-plane conformation component
- `D2c[]`: Log10 of second invariant of deformation rate tensor
- `vel[]`: Velocity magnitude
- `trA[]`: Log10 of conformation-trace deviation
- `filename[80]`: Snapshot filename
- `nx`, `ny`: Output grid resolution
- `xmin`, `ymin`, `xmax`, `ymax`: Domain bounds
- `Deltax`, `Deltay`: Output grid spacing
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
### main()

Extracts fields, computes diagnostics, and writes interpolated samples.

#### Args

- `a`: Number of command line arguments.
- `arguments[1]`: Snapshot filename.
- `arguments[2]`: `xmin`.
- `arguments[3]`: `ymin`.
- `arguments[4]`: `xmax`.
- `arguments[5]`: `ymax`.
- `arguments[6]`: `ny` (points in y).

#### Details

Deformation-rate components (finite differences):
- `D11 = du_y/dy`
- `D22 = u_y/y`
- `D33 = du_x/dx`
- `D13 = 0.5 * (du_y/dx + du_x/dy)`

Second invariant:
```
$$ D2 = D11^2 + D22^2 + D33^2 + 2 * D13^2 $$
```

Log scaling:
```
log10(value) if value > 0, else -10
```

#### Returns

- `0` on success.
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
  ### Field Processing Loop

  Computes deformation-rate invariants, velocity magnitude, and
  conformation-trace diagnostics at each cell.
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
  ### Grid Setup

  Creates a uniform sampling grid. The x-resolution follows the
  aspect ratio implied by `ny`.
  */
  FILE * fp = ferr;
  Deltay = (double)((ymax-ymin)/(ny));
  nx = (int)((xmax - xmin)/Deltay);
  Deltax = (double)((xmax-xmin)/(nx));
  len = list_len(list);

  // Allocate memory for extracted field data
  double ** field = (double **) matrix_new (nx, ny+1, len*sizeof(double));

  /**
  ### Field Interpolation

  Interpolates fields on the uniform sampling grid for output.
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

  Writes ASCII rows with `x y` followed by fields in `list` order.
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
