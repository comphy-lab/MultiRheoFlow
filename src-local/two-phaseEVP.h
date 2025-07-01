/**
 * Modification by Vatsal Sanjay, Arivazhagan G. Balasubramanian (Ari) 
 * Version 2.0, Jun 30, 2025

# Changelog
- Jun 30, 2025: added support for EVP simulations.

# Brief history
- v1.0 is the vanilla Basilisk code for two-phase flows: http://basilisk.fr/src/two-phase.h + http://basilisk.fr/src/two-phase-generic.h
- v2.0 is the modification for elasto-viscoplastic fluids using the log-conformation method.

# Two-phase interfacial flows
This is a modified version of [two-phase.h](http://basilisk.fr/src/two-phase.h). It contains the implementation of
Viscoplastic Fluid (Bingham Fluid).<br/>
This file helps setup simulations for flows of two fluids separated by
an interface (i.e. immiscible fluids). It is typically used in
combination with a [Navier--Stokes solver](navier-stokes/centered.h).

The interface between the fluids is tracked with a Volume-Of-Fluid
method. The volume fraction in fluid 1 is $f=1$ and $f=0$ in fluid
2. The densities and dynamic viscosities for fluid 1 and 2 are *rho1*,
*mu1*, *rho2*, *mu2*, respectively. */

#include "vof.h"

scalar f[], * interfaces = {f};

double rho1 = 1., mu1 = 0., rho2 = 1., mu2 = 0.;
double G1 = 0., G2 = 0.; // elastic moduli
double lambda1 = 0., lambda2 = 0.; // relaxation times
double tau01 = 0., tau02 = 0.; // yield-stress
double TOLelastic = 1e-2; // tolerance for elastic modulus #TOFIX: this must always be a very small number.

/**
Auxilliary fields are necessary to define the (variable) specific
volume $\alpha=1/\rho$ as well as the cell-centered density. */

face vector alphav[];
scalar rhov[];
scalar Gpd[];
scalar lambdapd[];
scalar tau0d[];

event defaults (i = 0) {
  alpha = alphav;
  rho = rhov;
  Gp = Gpd;
  lambda = lambdapd;
  tau0 = tau0d;
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
    tau0d[] = 0.;
    
    if (clamp(sf[], 0., 1.) > TOLelastic){
      Gpd[] += G1*clamp(sf[], 0., 1.);
      lambdapd[] += lambda1*clamp(sf[], 0., 1.);
      tau0d[] += tau01*clamp(sf[], 0., 1.);
    }
    if (clamp((1-sf[]), 0., 1.) > TOLelastic){
      Gpd[] += G2*clamp((1-sf[]), 0., 1.);
      lambdapd[] += lambda2*clamp((1-sf[]), 0., 1.);
      tau0d[] += tau02*clamp((1-sf[]), 0., 1.);
    }
  }

#if TREE
  sf.prolongation = fraction_refine;
  sf.dirty = true; // boundary conditions need to be updated
#endif
}
