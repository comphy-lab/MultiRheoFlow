/**
 * @file dropAtomisation.c
 * @brief This file contains the simulation code for the drop atomisation. 
 * @author Ayush Dixit & Vatsal Sanjay
 * @version 5.0
 * @date Oct 20, 2024
*/

// #include "axi.h"
#include "grid/octree.h"
// #include "grid/quadtree.h"
#include "navier-stokes/centered.h"

#define VANILLA 0 // vanilla cannot do 3D
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

#define tsnap (0.1) // 0.001 only for some cases. 
// Error tolerancs
#define fErr (1e-2)                                 // error tolerance in f1 VOF
#define KErr (1e-4)                                 // error tolerance in VoF curvature calculated using heigh function method (see adapt event)
#define VelErr (1e-2)                               // error tolerances in velocity -- Use 1e-2 for low Oh and 1e-3 to 5e-3 for high Oh/moderate to high J
#define AErr (1e-3)                             // error tolerances in conformation inside the liquid

#define R2(x,y,z)  (sq(x-3.) + sq(y) + sq(z))

// boundary conditions
// inflow: left
u.n[left]  = dirichlet(1.);
// p[left] = dirichlet(0);

// outflow: right
u.n[right] = neumann(0.);
p[right] = dirichlet(0);

int MAXlevel;
// We -> Weber number
// Oh -> Solvent Ohnesorge number
// Oha -> air Ohnesorge number
// De -> Deborah number
// Ec -> Elasto-capillary number

double Oh, Oha, De, We, RhoInOut, Ec, tmax;
char nameOut[80], dumpFile[80];

int  main(int argc, char const *argv[]) {
  // dtmax = 1e-5; //  BEWARE of this for stability issues. 

  L0 = 20;
  init_grid (1 << 6);

  origin (0, -L0/2
  #if dimension == 3
  , -L0/2
  #endif
  );

  // Values taken from the terminal
  MAXlevel = 7;
  RhoInOut = 830.;

  // elastic parts
  De = 0.0;
  Ec = 0.0;

  // Newtonian parts
  We = 15000; // based on the density of the gas
  Oh = 3e-3; // based on the density of the liquid
  Oha = 0.018*Oh; // based on the density of the liquid
  tmax = 200;

  // Create a folder named intermediate where all the simulation snapshots are stored.
  char comm[80];
  sprintf (comm, "mkdir -p intermediate");
  system(comm);
  // Name of the restart file. See writingFiles event.
  sprintf (dumpFile, "restart");


  rho1 = RhoInOut, rho2 = 1e0;

  // as both densities are based on the density of the liquid, we must multiply the Ohnesorge number by the square root of the density ratio
  mu1 = sqrt(RhoInOut)*Oh/sqrt(We), mu2 = sqrt(RhoInOut)*Oha/sqrt(We);

  // elastic parts
  // in G1, we need to mutiply by the density ratio (again, because Ec is based on the density of the liquid but in the code it is based on the density of the gas)
  G1 = Ec/We, G2 = 0.0;
  // Here, lambda is essentially the Weissenberg number, so there is no density in the expression. 
  lambda1 = De*sqrt(We), lambda2 = 0.0;
  
  // surface tension -- the Weber number is based on the density of the gas! So, all good. 
  f.sigma = 1.0/We;

  run();
}

event init (t = 0) {
  if (!restore (file = dumpFile)){
    refine(R2(x,y,z) < 1.1 && R2(x,y,z) > 0.9 && level < MAXlevel);
    fraction (f, 1. - R2(x,y,z));
  }
}

/**
## Adaptive Mesh Refinement
*/
event adapt(i++){
  scalar KAPPA[];
  curvature(f, KAPPA);

  adapt_wavelet ((scalar *){f, KAPPA, u.x, u.y
  #if dimension == 3
  ,u.z
  #endif
  },(double[]){fErr, KErr, VelErr, VelErr,
  #if dimension == 3
  VelErr
  #endif
  },MAXlevel, 4);
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
    fprintf(ferr, "Level %d, Oh %2.1e, We %2.1e, Oha %2.1e, De %2.1e, Ec %2.1e\n", MAXlevel, Oh, We, Oha, De, Ec);
}

/**
## Log writing
*/
event logWriting (i++) {

  double ke = 0.;
  foreach (reduction(+:ke)){
    ke += (0.5*rho(f[])*(sq(u.x[]) + sq(u.y[])
   #if dimension == 3
    + sq(u.z[])
   #endif
   ))*pow(Delta, dimension);
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
      fprintf(ferr, "Level %d, Oh %2.1e, We %2.1e, Oha %2.1e, De %2.1e, Ec %2.1e\n", MAXlevel, Oh, We, Oha, De, Ec);
      fprintf(ferr, "i dt t ke\n");
      fprintf(fp, "Level %d, Oh %2.1e, We %2.1e, Oha %2.1e, De %2.1e, Ec %2.1e\n", MAXlevel, Oh, We, Oha, De, Ec);
      fprintf(fp, "i dt t ke rM\n");
    }

    fprintf(fp, "%d %g %g %g\n", i, dt, t, ke);
    fprintf(ferr, "%d %g %g %g\n", i, dt, t, ke);

    fflush(fp);
    fclose(fp);
  }

  assert(ke > -1e-10);

  if (i > 1e1 && pid() == 0) {
    if (ke > 1e6 || ke < 1e-6) {
      const char* message = (ke > 1e6) ? 
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