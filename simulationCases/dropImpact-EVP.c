/**
# Drop Impact (EVP)

Simulates elasto-viscoplastic drop impact on a solid surface using the
Basilisk two-phase solver and log-conformation rheology.

## Author
Vatsal Sanjay (vatsal.sanjay@comphy-lab.org)
CoMPhy Lab
Date: Feb 09, 2026
*/

#include "axi.h"
// #include "grid/octree.h"
// #include "grid/quadtree.h"
#include "navier-stokes/centered.h"

#if AXI
#include "log-conform-elastoviscoplastic-scalar-2D.h"
#define logFile "logAxi-EVP-scalar.dat"
#else
#include "log-conform-elastoviscoplastic-scalar-3D.h"
#define logFile "log3D-EVP-scalar.dat"
#endif

#define FILTERED // Smear density and viscosity jumps
#include "two-phaseEVP.h"

#include "navier-stokes/conserving.h"
#include "tension.h"
#include "case-params.h"

/**
## Output Cadence
*/
#define tsnap (1e-2)

/**
## Adaptivity Tolerances

- `fErr`: VOF error for `f`
- `KErr`: Curvature error (height-function)
- `VelErr`: Velocity error
*/
#define fErr (1e-3)
#define KErr (1e-6)
#define VelErr (1e-2)

#define xDist (5e-2)
#define R2(x,y,z)  (sq(x-1.-xDist) + sq(y) + sq(z))

/**
## Boundary Conditions

Left boundary uses free-slip for faster testing.
*/
f[left] = dirichlet(0.0);

int MAXlevel;
/**
## Dimensionless Groups

- `We`: Drop Weber number
- `Ohs`: Solvent Ohnesorge number
- `Oha`: Air Ohnesorge number
- `De`: Deborah number
- `Ec`: Elasto-capillary number
*/

double We, Ohs, Oha, De, Ec, tmax, Ldomain;
char nameOut[80], dumpFile[80];

static int loadInputParams (const char * params_file) {
  paramEntry params[] = {
    {"MAXlevel", &MAXlevel, PARAM_KIND_INT, 0, 0},
    {"Ldomain", &Ldomain, PARAM_KIND_DOUBLE, 0, 0},
    {"L0", &Ldomain, PARAM_KIND_DOUBLE, 0, 0},
    {"tmax", &tmax, PARAM_KIND_DOUBLE, 0, 0},
    {"We", &We, PARAM_KIND_DOUBLE, 0, 0},
    {"Ohs", &Ohs, PARAM_KIND_DOUBLE, 0, 0},
    {"Oha", &Oha, PARAM_KIND_DOUBLE, 0, 0},
    {"De", &De, PARAM_KIND_DOUBLE, 0, 0},
    {"Ec", &Ec, PARAM_KIND_DOUBLE, 0, 0}
  };
  return parseCaseParams(params_file, params,
                         sizeof(params)/sizeof(params[0]));
}

/**
### main()

Sets domain parameters, material properties, and launches the run.
*/
int main(int argc, char const *argv[]) {

  dtmax = 1e-5;

  // Values taken from the terminal
  MAXlevel = 8;
  Ldomain = 4.0;
  tmax = 4.0;
  We = 5.0;
  Ohs = 1e-2;
  Oha = 1e-2 * Ohs;
  De = 1.0;
  Ec = 1.0;

  if (argc > 2) {
    fprintf(ferr, "Usage: %s [params_file]\n", argv[0]);
    return 1;
  }
  if (argc == 2 && !loadInputParams(argv[1]))
    return 1;

  L0 = Ldomain;

  init_grid (1 << 4);

  // Create a folder named intermediate where all the simulation
  // snapshots are stored.
  char comm[80];
  sprintf (comm, "mkdir -p intermediate");
  system(comm);
  // Name of the restart file. See writingFiles event.
  sprintf (dumpFile, "restart");

  rho1 = 1., rho2 = 1e-3;
  mu1 = Ohs/sqrt(We), mu2 = Oha/sqrt(We);
  G1 = Ec/We, G2 = 0.0;
  lambda1 = De*sqrt(We), lambda2 = 0.0;
  tau01 = 0.0, tau02 = 0.0;

  f.sigma = 1.0/We;

  run();

}

event init (t = 0) {
  if (!restore (file = dumpFile)){
   refine(R2(x,y,z) < (1.1) && R2(x,y,z) > (0.9) && level < MAXlevel);
   fraction (f, (1-R2(x,y,z)));
   foreach(){
    u.x[] = -f[]*1.0;
    p[] = 2*f[];
   }
  }
}

/**
## Adaptive Mesh Refinement

Refines based on interface, curvature, and velocity errors.
*/
event adapt(i++){
  scalar KAPPA[], trA[];
  curvature(f, KAPPA);
#if dimension == 3
  foreach(){
    trA[] = A11[]+A22[]+A33[];
  }
  adapt_wavelet ((scalar *){f, u.x, u.y, u.z, KAPPA},
      (double[]){fErr, VelErr, VelErr, VelErr, KErr},
      MAXlevel, 4);
#else
  adapt_wavelet ((scalar *){f, u.x, u.y, KAPPA},
      (double[]){fErr, VelErr, VelErr, KErr},
      MAXlevel, 4);
#endif

}

/**
## Dumping Snapshots

Writes restart and time-stamped snapshot dumps.
*/
event writingFiles (t = 0; t += tsnap; t <= tmax) {
  p.nodump = false;
  dump (file = dumpFile);
  sprintf (nameOut, "intermediate/snapshot-%5.4f", t);
  dump(file=nameOut);
}

/**
## Ending Simulation

Prints summary parameters at completion.
*/
event end (t = end) {
  if (pid() == 0)
    fprintf(ferr,
            "Level %d, Ohs %2.1e, We %2.1e, Oha %2.1e, "
            "De %2.1e, Ec %2.1e\n",
            MAXlevel, Ohs, We, Oha, De, Ec);
}

/**
## Log Writing

Tracks kinetic energy and aborts on blow-up or decay.
*/
event logWriting (i++) {

  double ke = 0.;
  foreach (reduction(+:ke)){
    ke += (2*pi*y)*(0.5*rho(f[])*(sq(u.x[]) + sq(u.y[])))*sq(Delta);
  }

  static FILE * fp;
  if (pid() == 0) {
    const char* mode = (i == 0) ? "w" : "a";
    fp = fopen(logFile, mode);
    if (fp == NULL) {
      fprintf(ferr, "Error opening log file\n");
      return 1;
    }

    if (i == 0) {
      fprintf(ferr,
              "Level %d, Ohs %2.1e, We %2.1e, Oha %2.1e, "
              "De %2.1e, Ec %2.1e\n",
              MAXlevel, Ohs, We, Oha, De, Ec);
      fprintf(ferr, "i dt t ke\n");
      fprintf(fp,
              "Level %d, Ohs %2.1e, We %2.1e, Oha %2.1e, "
              "De %2.1e, Ec %2.1e\n",
              MAXlevel, Ohs, We, Oha, De, Ec);
      fprintf(fp, "i dt t ke rM\n");
    }

    fprintf(fp, "%d %g %g %g\n", i, dt, t, ke);
    fprintf(ferr, "%d %g %g %g\n", i, dt, t, ke);

    fflush(fp);
    fclose(fp);
  }

  assert(ke > -1e-10);

  if (i > 1e1 && pid() == 0) {
    if (ke > 1e2 || ke < 1e-8) {
      const char* message = (ke > 1e2) ?
        "The kinetic energy blew up. Stopping simulation\n" :
        "kinetic energy too small now! Stopping!\n";

      fprintf(ferr, "%s", message);

      fp = fopen("log", "a");
      fprintf(fp, "%s", message);
      fflush(fp);
      fclose(fp);

      dump(file=dumpFile);
      return 1;
    }
  }

}
