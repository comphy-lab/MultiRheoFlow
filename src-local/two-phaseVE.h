/**
# Two-Phase Viscoelastic Solver

Modified from Basilisk `two-phase.h` and `two-phase-generic.h` to
support viscoelastic flows with log-conformation rheology.

## Change Log

- 2024-10-17: Add support for VE simulations.

## Two-Phase Interfacial Flows

The interface is tracked with VOF. The volume fraction is `f = 1` in
fluid 1 and `f = 0` in fluid 2. Densities and viscosities are `rho1`,
`mu1`, `rho2`, `mu2`.
*/

#include "vof.h"

scalar f[], * interfaces = {f};

double rho1 = 1., mu1 = 0., rho2 = 1., mu2 = 0.;
double G1 = 0., G2 = 0.; // elastic moduli
double lambda1 = 0., lambda2 = 0.; // relaxation times
double TOLelastic = 1e-2; // tolerance for elastic modulus #TOFIX: this must always be a very small number.

/**
Auxiliary fields define the specific volume $\alpha = 1/\rho$ and the
cell-centered density.
*/

face vector alphav[];
scalar rhov[];
scalar Gpd[];
scalar lambdapd[];

event defaults (i = 0) {
  alpha = alphav;
  rho = rhov;
  Gp = Gpd;
  lambda = lambdapd;

  /**
  If the viscosity is non-zero, we need to allocate the face-centered
  viscosity field. */

  mu = new face vector;
}

/**
The density and viscosity are defined using arithmetic averages by
default. The user can overload these definitions to use other types of
averages (i.e. harmonic). */

#ifndef rho
# define rho(f) (clamp(f,0.,1.)*(rho1 - rho2) + rho2)
#endif
#ifndef mu
// for Arithmetic mean, use this
# define mu(f)  (clamp(f,0.,1.)*(mu1 - mu2) + mu2)
#endif

/**
We have the option of using some "smearing" of the density/viscosity
jump. */

#ifdef FILTERED
scalar sf[];
#else
# define sf f
#endif

event tracer_advection (i++) {

  /**
  When using smearing of the density jump, we initialise *sf* with the
  vertex-average of *f*. */

#ifndef sf
#if dimension <= 2
  foreach()
    sf[] = (4.*f[] +
	    2.*(f[0,1] + f[0,-1] + f[1,0] + f[-1,0]) +
	    f[-1,-1] + f[1,-1] + f[1,1] + f[-1,1])/16.;
#else // dimension == 3
  foreach()
    sf[] = (8.*f[] +
	    4.*(f[-1] + f[1] + f[0,1] + f[0,-1] + f[0,0,1] + f[0,0,-1]) +
	    2.*(f[-1,1] + f[-1,0,1] + f[-1,0,-1] + f[-1,-1] +
		f[0,1,1] + f[0,1,-1] + f[0,-1,1] + f[0,-1,-1] +
		f[1,1] + f[1,0,1] + f[1,-1] + f[1,0,-1]) +
	    f[1,-1,1] + f[-1,1,1] + f[-1,1,-1] + f[1,1,1] +
	    f[1,1,-1] + f[-1,-1,-1] + f[1,-1,-1] + f[-1,-1,1])/64.;
#endif
#endif

#if TREE
  sf.prolongation = refine_bilinear;
  sf.dirty = true; // boundary conditions need to be updated
#endif
}

event properties (i++) {
  
  foreach_face() {
    double ff = (sf[] + sf[-1])/2.;
    alphav.x[] = fm.x[]/rho(ff);
    face vector muv = mu;
    muv.x[] = fm.x[]*mu(ff);
  }

  foreach(){
    rhov[] = cm[]*rho(sf[]);

    Gpd[] = 0.;
    lambdapd[] = 0.;

    if (clamp(sf[], 0., 1.) > TOLelastic){
      Gpd[] += G1*clamp(sf[], 0., 1.);
      lambdapd[] += lambda1*clamp(sf[], 0., 1.);
    }
    if (clamp((1-sf[]), 0., 1.) > TOLelastic){
      Gpd[] += G2*clamp((1-sf[]), 0., 1.);
      lambdapd[] += lambda2*clamp((1-sf[]), 0., 1.);
    }
  }

#if TREE
  sf.prolongation = fraction_refine;
  sf.dirty = true; // boundary conditions need to be updated
#endif
}
