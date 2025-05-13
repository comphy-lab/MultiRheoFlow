/**
 * @file pinchOff.c
 * @brief This file contains the simulation code for the pinch-off of a viscoelastic liquid jet. 
 * @author Vatsal Sanjay
 * @version 0.2
 * @date Oct 18, 2024
*/

#include "axi.h"
// #include "grid/octree.h"
// #include "grid/quadtree.h"
#include "navier-stokes/centered.h"

#define VANILLA 1
#if VANILLA
#include "../src-local/log-conform-viscoelastic.h"
#define logFile "logAxi-vanilla.dat"
#else
#if AXI
#include "../src-local/log-conform-viscoelastic-scalar-2D.h"
#define logFile "logAxi-scalar.dat"
#else
#include "../src-local/log-conform-viscoelastic-scalar-3D.h"
#define logFile "log3D-scalar.dat"
#endif
#endif

#define FILTERED // Smear density and viscosity jumps
#include "../src-local/two-phaseVE.h"

#include "navier-stokes/conserving.h"
#include "tension.h"

#define tsnap (1e-2)

// Error tolerancs
#define fErr (1e-3)                                 // error tolerance in f1 VOF
#define KErr (1e-6)                                 // error tolerance in VoF curvature calculated using heigh function method (see adapt event)
#define VelErr (1e-2)                               // error tolerances in velocity -- Use 1e-2 for low Oh and 1e-3 to 5e-3 for high Oh/moderate to high J

#define epsilon (0.5)
#define R2(x,y,z,e) (sqrt(sq(y) + sq(z)) + (e*sin(x/4.)))

// boundary conditions
u.n[top] = neumann(0.0);
p[top] = dirichlet(0.0);

int MAXlevel;
// Oh -> Solvent Ohnesorge number
// Oha -> air Ohnesorge number
// De -> Deborah number
// Ec -> Elasto-capillary number
// for now there is no viscoelasticity

double Oh, Oha, De, Ec, tmax;
char nameOut[80], dumpFile[80];

int main(int argc, char const *argv[]) {

  L0 = 2*pi;
  
  // Values taken from the terminal
  MAXlevel = 6;
  tmax = 10;
  Oh = 1e-2;
  Oha = 1e-2 * Oh;
  De = 1.0; // 1e-1;
  Ec = 1.0; // 1e-2;

  init_grid (1 << 4);

  // Create a folder named intermediate where all the simulation snapshots are stored.
  char comm[80];
  sprintf (comm, "mkdir -p intermediate");
  system(comm);
  // Name of the restart file. See writingFiles event.
  sprintf (dumpFile, "restart");


  rho1 = 1., rho2 = 1e-3;
  mu1 = Oh, mu2 = Oha;
  lambda1 = De, lambda2 = 0.;
  G1 = Ec, G2 = 0.;
  f.sigma = 1.0;

  run();

}

event init (t = 0) {
  if (!restore (file = dumpFile)){
    refine(R2(x,y,z,epsilon) < (1+epsilon) && R2(x,y,z,epsilon) > (1-epsilon) && level < MAXlevel);
   fraction (f, (1-R2(x,y,z,epsilon)));
  }
}

/**
## Adaptive Mesh Refinement
*/
event adapt(i++){
  scalar KAPPA[];
  curvature(f, KAPPA);
  adapt_wavelet ((scalar *){f, u.x, u.y, KAPPA},
      (double[]){fErr, VelErr, VelErr, KErr},
      MAXlevel, 4);
}

/**
## Dumping snapshots
*/
event writingFiles (t = 0; t += tsnap; t <= tmax) {
  dump (file = dumpFile);
  sprintf (nameOut, "intermediate/snapshot-%5.4f", t);
  dump(file=nameOut);
}

/**
## Ending Simulation
*/
event end (t = end) {
  if (pid() == 0)
    fprintf(ferr, "Level %d, Oh %2.1e\n", MAXlevel, Oh);
}

/**
## Log writing
*/
event logWriting (i++) {

  double ke = 0.;
  foreach (reduction(+:ke)){
    ke += (2*pi*y)*(0.5*rho(f[])*(sq(u.x[]) + sq(u.y[])+ sq(u.z[])))*sq(Delta);
  }

  static FILE * fp;
  if (pid() == 0) {
    const char* mode = (i == 0) ? "w" : "a";
    fp = fopen(logFile, mode);
    if (fp == NULL) {
      fprintf(ferr, "Error opening log file\n");
      return 1;
    }

    scalar pos[];
    position (f, pos, {0,1,0});
    double ymin = statsf(pos).min;

    if (i == 0) {
      fprintf(ferr, "Level %d, Oh %2.1e, Oha %2.1e, De %2.1e, Ec %2.1e\n", MAXlevel, Oh, Oha, De, Ec);
      fprintf(ferr, "i dt t ke ymin\n");
      fprintf(fp, "Level %d, Oh %2.1e, Oha %2.1e, De %2.1e, Ec %2.1e\n", MAXlevel, Oh, Oha, De, Ec);
      fprintf(fp, "i dt t ke ymin\n");
    }

    fprintf(fp, "%d %g %g %g %g\n", i, dt, t, ke, ymin);
    fprintf(ferr, "%d %g %g %g %g\n", i, dt, t, ke, ymin);

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
