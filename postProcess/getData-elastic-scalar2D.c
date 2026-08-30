/**
# Snapshot Data Extractor (Elastic Scalar 2D)

Extracts scalar diagnostics from Basilisk snapshots for
viscoelastic/multiphase cases. Computes deformation-rate invariants,
velocity magnitude, and conformation-trace deviations.

Accepts both layout families used by this repository:

- Scalar 2D/axi: `A11`, `A12`, `A22`, and hoop `AThTh`
- Vanilla tensor 2D/axi: `conform_p.x.x`, `conform_p.y.x`,
  `conform_p.y.y`, and hoop `conform_qq`

## Usage

```
./getData-elastic-scalar2D dump [xmin ymin xmax ymax ny]
```

With only `dump`, the restored origin and box size are used and `ny`
defaults to 128. `VideoAxi.py` always passes the five bound arguments.

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
- `D2c[]`: Log10 of second invariant of deformation rate tensor
- `vel[]`: Velocity magnitude
- `trA[]`: Log10 of conformation-trace deviation
- `filename[512]`: Snapshot filename
- `nx`, `ny`: Output grid resolution
- `xmin`, `ymin`, `xmax`, `ymax`: Domain bounds
- `Deltax`, `Deltay`: Output grid spacing
*/
scalar f[];
vector u[];
char filename[512];
int nx, ny, len;
double xmin, ymin, xmax, ymax, Deltax, Deltay;
scalar D2c[], vel[], trA[];
scalar * list = NULL;

/**
### read_double(), read_positive_int()

Strict token conversion for the optional bound arguments.
`ny` is capped below `INT_MAX` so `ny + 1` is a valid matrix height.
*/
static int read_double (const char * s, double * v)
{
  char * end;
  *v = strtod (s, &end);
  return end != s && *end == '\0' && isfinite (*v);
}

static int read_positive_int (const char * s, int * v)
{
  char * end;
  long n = strtol (s, &end, 10);
  if (end == s || *end != '\0' || n <= 0 || n >= INT_MAX)
    return 0;
  *v = (int) n;
  return 1;
}

/**
### main()

Extracts fields, computes diagnostics, and writes interpolated samples
to `stderr` (the stream `VideoAxi.py` reads).

#### Args

- `arguments[1]`: Snapshot filename.
- `arguments[2]`: `xmin` (optional with the rest of the bound set).
- `arguments[3]`: `ymin`.
- `arguments[4]`: `xmax`.
- `arguments[5]`: `ymax`.
- `arguments[6]`: `ny` (points in y).

#### Details

`restore(..., list = all)` creates dump-only fields so vanilla tensor
dumps are visible to `lookup_field`. Conformation components are **not**
pre-declared: an empty `A11[]` would otherwise mask a dump that stores
`conform_p` instead.

Deformation-rate components (finite differences):
- `D11 = du_y/dy`
- `D22 = u_y/y` (`D11` on the axis, by regularity)
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

- `0` on success, `1` on usage, restore, or layout errors.
*/
int main (int argc, char const *arguments[])
{
  if (argc != 2 && argc != 7) {
    fprintf (fout,
	     "usage: %s dump [xmin ymin xmax ymax ny]\n",
	     arguments[0]);
    return 1;
  }

  int npath = snprintf (filename, sizeof(filename), "%s", arguments[1]);
  if (npath < 0 || npath >= (int) sizeof(filename)) {
    fprintf (fout, "%s: snapshot path too long\n", arguments[0]);
    return 1;
  }

  list = list_add (list, D2c);
  list = list_add (list, vel);
  list = list_add (list, trA);

  if (!restore (file = filename, list = all)) {
    fprintf (fout, "%s: could not restore '%s'\n", arguments[0], filename);
    return 1;
  }

  if (argc == 7) {
    if (!read_double (arguments[2], &xmin) ||
	!read_double (arguments[3], &ymin) ||
	!read_double (arguments[4], &xmax) ||
	!read_double (arguments[5], &ymax) ||
	!read_positive_int (arguments[6], &ny)) {
      fprintf (fout, "%s: invalid xmin ymin xmax ymax ny\n", arguments[0]);
      return 1;
    }
  }
  else {
    xmin = X0; ymin = Y0;
    xmax = X0 + L0; ymax = Y0 + L0;
    ny = 128;
  }

  if (ny <= 0 || xmax <= xmin || ymax <= ymin) {
    fprintf (fout, "%s: invalid output grid bounds or ny\n", arguments[0]);
    return 1;
  }

  scalar A11 = lookup_field ("A11");
  scalar A22 = lookup_field ("A22");
  scalar Aqq = lookup_field ("AThTh");
  if (A11.i < 0 || A22.i < 0) {
    A11 = lookup_field ("conform_p.x.x");
    A22 = lookup_field ("conform_p.y.y");
    Aqq = lookup_field ("conform_qq");
  }

  if (A11.i < 0 || A22.i < 0) {
    fprintf (fout,
	     "%s: dump '%s' has neither A11/A22 nor conform_p\n",
	     arguments[0], filename);
    return 1;
  }

  /**
  ### Field Processing Loop

  Computes deformation-rate invariants, velocity magnitude, and
  conformation-trace diagnostics at each cell.
  */
  foreach() {
    double D11 = (u.y[0,1] - u.y[0,-1])/(2*Delta);
    /* On the axis, u_r/r -> du_r/dr by regularity. */
    double D22 = (fabs(y) > 1e-30) ? u.y[]/y : D11;
    double D33 = (u.x[1,0] - u.x[-1,0])/(2*Delta);
    double D13 = 0.5*((u.y[1,0] - u.y[-1,0] + u.x[0,1] - u.x[0,-1])/(2*Delta));

    double D2 = (sq(D11) + sq(D22) + sq(D33) + 2.0*sq(D13));
    D2c[] = f[]*D2;

    if (D2c[] > 0.)
      D2c[] = log(D2c[])/log(10);
    else
      D2c[] = -10;

    vel[] = sqrt (sq(u.x[]) + sq(u.y[]));

    double tr;
    if (Aqq.i >= 0)
      tr = (A11[] + A22[] + Aqq[])/3.0 - 1.0;
    else
      tr = (A11[] + A22[])/2.0 - 1.0;
    if (tr > 0.)
      trA[] = log(tr)/log(10);
    else
      trA[] = -10;
  }

  /**
  ### Grid Setup

  Creates a uniform sampling grid. The x-resolution follows the
  aspect ratio implied by `ny`.
  */
  FILE * fp = ferr;
  Deltay = (ymax - ymin)/ny;
  double nx_d = (xmax - xmin)/Deltay;
  if (!(Deltay > 0.) || !isfinite(nx_d) || nx_d > INT_MAX) {
    fprintf (fout, "%s: invalid output grid bounds or ny\n", arguments[0]);
    return 1;
  }
  nx = (int) nx_d;
  if (nx < 1)
    nx = 1;
  Deltax = (xmax - xmin)/nx;
  len = list_len (list);

  double ** field = (double **) matrix_new (nx, ny + 1, len*sizeof(double));

  /**
  ### Field Interpolation

  Interpolates fields on the uniform sampling grid for output.
  */
  for (int i = 0; i < nx; i++) {
    double x = Deltax*(i + 1./2) + xmin;
    for (int j = 0; j < ny; j++) {
      double y = Deltay*(j + 1./2) + ymin;
      int k = 0;
      for (scalar s in list)
	field[i][len*j + k++] = interpolate (s, x, y);
    }
  }

  /**
  ### Data Output

  Writes ASCII rows with `x y` followed by fields in `list` order.
  */
  for (int i = 0; i < nx; i++) {
    double x = Deltax*(i + 1./2) + xmin;
    for (int j = 0; j < ny; j++) {
      double y = Deltay*(j + 1./2) + ymin;
      fprintf (fp, "%g %g", x, y);
      int k = 0;
      for (scalar s in list)
	fprintf (fp, " %g", field[i][len*j + k++]);
      fputc ('\n', fp);
    }
  }

  fflush (fp);
  fclose (fp);
  matrix_free (field);
  return 0;
}
