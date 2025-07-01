/** 
# Log-Conformation Method for 2D Elasto-viscoplastic Fluids

## Overview
- **Title**: log-conform-elastoviscoplastic-scalar-2D.h
- **Version**: 2.5
- **Description**: 2D and axisymmetric scalar implementation of elastoviscoplastic fluid dynamics using the log-conformation method

### Key Features
1. Conformation tensor A exists across domain and relaxes according to λ
2. Stress acts according to elastic modulus G
3. Supports both 2D and axisymmetric configurations
4. Scalar implementation approach for better performance
5. Compatible with [log-conform-elastoviscoplastic.h](log-conform-elastoviscoplastic.h)

### Author Information
- **Name**: Vatsal Sanjay, Arivazhagan G. Balasubramanian (Ari)
- **Email**: vatsalsanjay@gmail.com
- **Institution**: Physics of Fluids
- **Last Updated**: Jun 30, 2025

### Implementation Notes
- Based on http://basilisk.fr/src/log-conform.h with key improvements:
  - Uses G-λ formulation for better physical interpretation
  - Fixes surface tension coupling bug where [σ_p] = 0 & [σ_s] = γκ
  - Ensures [σ_s+σ_p] = γκ for correct interface behavior

## Version History

### v1.0 (Oct 18, 2024)
- Initial implementation with 2D+axi support
- Scalar-based implementation for efficiency

### v2.0 (Nov 3, 2024)
- Major documentation improvements
- Made code an axisymmetric mirror of [log-conform-elastoviscoplastic-scalar-3D.h](log-conform-elastoviscoplastic-scalar-3D.h)
- Added negative eigenvalue detection with location reporting
- Added initialization functions for pseudo_v and pseudo_t

### v2.1 (Nov 14, 2024)
- Added support for infinite Deborah number cases

### v2.5 (Nov 23, 2024)
- Enhanced documentation clarity and completeness

## Future Work

### Tensor Formulation
- Convert to consistent tensor formulation for:
  - Improved readability and maintainability
  - Better computational efficiency
  - Reduced potential for bugs
  - Prerequisites for axi compatibility in 3D version
- Related issues:
  - https://github.com/comphy-lab/Viscoelastic3D/issues/11
  - https://github.com/comphy-lab/Viscoelastic3D/issues/5

### Code Improvements
- [ ] Enforce tensor usage throughout codebase
- [ ] Implement foreach_dimension compatibility
*/

/**
# The log-conformation method for viscoelastic constitutive models

## Introduction

Elasto-viscoplastic fluids exhibit viscous, elastic and plastic behaviour when
subjected to deformation. Therefore these materials are governed by
the Navier--Stokes equations enriched with an extra stress
$Tij$
$$
\rho\left[\partial_t\mathbf{u}+\nabla\cdot(\mathbf{u}\otimes\mathbf{u})\right] = 
- \nabla p + \nabla\cdot(2\mu_s\mathbf{D}) + \nabla\cdot\mathbf{T}
+ \rho\mathbf{a}
$$
where $\mathbf{D}=[\nabla\mathbf{u} + (\nabla\mathbf{u})^T]/2$ is the
deformation tensor and $\mu_s$ is the solvent viscosity of the
EVP fluid.

The extra stress $\mathbf{T}$ includes memory effects due to the polymers and the 
plasticity effects associated with internal structural changes in the material
that constitutes to yielding the material and fluidizes it. Several constitutive 
rheological models are available in the literature where the extra stress 
$\mathbf{T}$ is typically a function $\mathbf{f_s}(\cdot)$ of the conformation 
tensor $\mathbf{A}$ such as
$$
\mathbf{T} = G_p \mathbf{f_s}(\mathbf{A})
$$
where $G_p$ is the elastic modulus and $\mathbf{f_s}(\cdot)$ is the relaxation function.

The conformation tensor $\mathbf{A}$ is related to the deformation of
the elasto-viscoplastic matrix. $\mathbf{A}$ is governed by the equation
$$
D_t \mathbf{A} - \mathbf{A} \cdot \nabla \mathbf{u} - \nabla
\mathbf{u}^{T} \cdot \mathbf{A} =
-\frac{\mathbf{f_r}(\mathbf{A})}{\lambda} 
$$
where $D_t$ denotes the material derivative and
$\mathbf{f_r}(\cdot)$ is the relaxation function. Here, $\lambda$ is the relaxation time.

In the case of Saramito (2007) elasto-viscoplastic model, where the
material before yielding is represented by Kelvin-Voigt viscoelastic solid and
long after yielding recovers the Oldroyd-B type viscoelastic fluid behaviour,
 $\mathbf{f}_s (\mathbf{A}) = \mathbf{A} -\mathbf{I}$,
 $\mathbf{f}_r (\mathbf{A}) = \mathcal{F}(\mathbf{A} -\mathbf{I})$,
and the above equations can be combined to avoid the use of
$\mathbf{A}$
$$
(\mathcal{F}/\lambda) \mathbf{T} + (D_t \mathbf{T} -
\mathbf{T} \cdot \nabla \mathbf{u} -
\nabla \mathbf{u}^{T} \cdot \mathbf{T})  = 2 G_p\lambda \mathbf{D}
$$

Here, $\mathcal{F} = max(0., \frac{\|\tau_d\|-\tau_y}{\|\tau_d\|}$.
[Comminal et al. (2015)](#comminal2015) gathered the functions
$\mathbf{f}_s (\mathbf{A})$ and $\mathbf{f}_r (\mathbf{A})$ for
different viscoelastic constitutive models. This work is an extension
of such formulation for elasto-viscoplastic materials.

## Parameters

The primary parameters are the relaxation time
$\lambda$, elastic modulus $G_p$ and yield stress $\tau_y$. The solvent viscosity
$\mu_s$ is defined in the [Navier-Stokes
solver](navier-stokes/centered.h). 

Gp, lambda and tau_y are defined in [two-phaseEVP.h](two-phaseEVP.h).
*/

/**
## The log conformation approach

The numerical resolution of viscoelastic and EVP fluid problems often faces the
[High-Weissenberg Number
Problem](http://www.ma.huji.ac.il/~razk/iWeb/My_Site/Research_files/Visco1.pdf). 
This is a numerical instability appearing when strongly elastic flows
create regions of high stress and fine features. This instability
poses practical limits to the values of the relaxation time of the
viscoelastic and EVP fluid, $\lambda$.  [Fattal \& Kupferman (2004,
2005)](#fattal2004) identified the exponential nature of the solution
as the origin of the instability. They proposed to use the logarithm
of the conformation tensor $\Psi = \log \, \mathbf{A}$ rather than the
extra stress tensor to circumvent the instability.

The constitutive equation for the log of the conformation tensor is
$$ 
D_t \Psi = (\Omega \cdot \Psi -\Psi \cdot \Omega) + 2 \mathbf{B} +
\frac{e^{-\Psi} \mathbf{f}_r (e^{\Psi})}{\lambda}
$$
where $\Omega$ and $\mathbf{B}$ are tensors that result from the
decomposition of the transpose of the tensor gradient of the
velocity
$$ 
(\nabla \mathbf{u})^T = \Omega + \mathbf{B} + N
\mathbf{A}^{-1} 
$$ 

The antisymmetric tensor $\Omega$ requires only the memory of a scalar
in 2D since,
$$ 
\Omega = \left( 
\begin{array}{cc}
0 & \Omega_{12} \\
-\Omega_{12} & 0
\end{array} 
\right)
$$

For 3D, $\Omega$ is a skew-symmetric tensor given by

$$
\Omega = \left( 
\begin{array}{ccc}
0 & \Omega_{12} & \Omega_{13} \\
-\Omega_{12} & 0 & \Omega_{23} \\
-\Omega_{13} & -\Omega_{23} & 0
\end{array} 
\right)
$$

The log-conformation tensor, $\Psi$, is related to the
polymeric stress tensor $\mathbf{T}$, by the strain function 
$\mathbf{f}_s (\mathbf{A})$
$$ 
\Psi = \log \, \mathbf{A} \quad \mathrm{and} \quad \mathbf{T} =
\frac{G_p}{\lambda} \mathbf{f}_s (\mathbf{A})
$$
where $Tr$ denotes the trace of the tensor and $L$ is an additional
property of the viscoelastic fluid.

We will use the Bell--Collela--Glaz scheme to advect the log-conformation 
tensor $\Psi$. */

/*
TODO: 
- Perhaps, instead of the Bell--Collela--Glaz scheme, we can use the conservative form of the advection equation and transport the log-conformation tensor with the VoF color function, similar to [http://basilisk.fr/src/navier-stokes/conserving.h](http://basilisk.fr/src/navier-stokes/conserving.h)
*/

#include "bcg.h"

(const) scalar Gp = unity; // elastic modulus
(const) scalar lambda = unity; // relaxation time
(const) scalar tau0 = unity; // yield-stress

scalar A11[], A12[], A22[]; // conformation tensor
scalar T11[], T12[], T22[]; // stress tensor
#if AXI
scalar AThTh[], T_ThTh[];
#endif

event defaults (i = 0) {
  if (is_constant (a.x))
    a = new face vector;

  /*
  initialize A and T
  */
  for (scalar s in {A11, A22}) {
    foreach () {
      s[] = 1.;
    }
  }
  for (scalar s in {T11, T12, T22, A12}) {
    foreach(){
      s[] = 0.;
    }
  }
#if AXI
  foreach(){
    T_ThTh[] = 0;
    AThTh[] = 1.;
  }
#endif

  for (scalar s in {T11, T12, T22}) {
    if (s.boundary[left] != periodic_bc) {
        s[left] = neumann(0);
	      s[right] = neumann(0);
      }
  }

  for (scalar s in {A11, A12, A22}) {
    if (s.boundary[left] != periodic_bc) {
        s[left] = neumann(0);
	      s[right] = neumann(0);
    }
  }

#if AXI
  T12[bottom] = dirichlet (0.);  
  A12[bottom] = dirichlet (0.);  
#endif
}

/**
## Useful functions in 2D

The first step is to implement a routine to calculate the eigenvalues
and eigenvectors of the conformation tensor $\mathbf{A}$.

These structs ressemble Basilisk vectors and tensors but are just
arrays not related to the grid. */

typedef struct { double x, y;}   pseudo_v;
typedef struct { pseudo_v x, y;} pseudo_t;

// Function to initialize pseudo_v
static inline void init_pseudo_v(pseudo_v *v, double value) {
    v->x = value;
    v->y = value;
}

// Function to initialize pseudo_t
static inline void init_pseudo_t(pseudo_t *t, double value) {
    init_pseudo_v(&t->x, value);
    init_pseudo_v(&t->y, value);
}

static void diagonalization_2D (pseudo_v * Lambda, pseudo_t * R, pseudo_t * A)
{
  /**
  The eigenvalues are saved in vector $\Lambda$ computed from the
  trace and the determinant of the symmetric conformation tensor
  $\mathbf{A}$. */

  if (sq(A->x.y) < 1e-15) {
    R->x.x = R->y.y = 1.;
    R->y.x = R->x.y = 0.;
    Lambda->x = A->x.x; Lambda->y = A->y.y;
    return;
  }

  double T = A->x.x + A->y.y; // Trace of the tensor
  double D = A->x.x*A->y.y - sq(A->x.y); // Determinant

  /**
  The eigenvectors, $\mathbf{v}_i$ are saved by columns in tensor
  $\mathbf{R} = (\mathbf{v}_1|\mathbf{v}_2)$. */

  R->x.x = R->x.y = A->x.y;
  R->y.x = R->y.y = -A->x.x;
  double s = 1.;
  for (int i = 0; i < dimension; i++) {
    double * ev = (double *) Lambda;
    ev[i] = T/2 + s*sqrt(sq(T)/4. - D);
    s *= -1;
    double * Rx = (double *) &R->x;
    double * Ry = (double *) &R->y;
    Ry[i] += ev[i];
    double mod = sqrt(sq(Rx[i]) + sq(Ry[i]));
    Rx[i] /= mod;
    Ry[i] /= mod;
  }
}

/**
The stress tensor depends on previous instants and has to be
integrated in time. In the log-conformation scheme the advection of
the stress tensor is circumvented, instead the conformation tensor,
$\mathbf{A}$ (or more precisely the related variable $\Psi$) is
advanced in time.

In what follows we will adopt a scheme similar to that of [Hao \& Pan
(2007)](#hao2007). We use a split scheme, solving successively

a) the upper convective term:
$$
\partial_t \Psi = 2 \mathbf{B} + (\Omega \cdot \Psi -\Psi \cdot \Omega)
$$
b) the advection term:
$$
\partial_t \Psi + \nabla \cdot (\Psi \mathbf{u}) = 0
$$
c) the model term (but set in terms of the conformation 
tensor $\mathbf{A}$). In an Oldroyd-B viscoelastic fluid, the model is
$$ 
\partial_t \mathbf{A} = -\frac{\mathbf{f}_r (\mathbf{A})}{\lambda}
$$
*/

event tracer_advection(i++)
{
  scalar Psi11 = A11;
  scalar Psi12 = A12;
  scalar Psi22 = A22;
#if AXI
  scalar Psiqq = AThTh;
#endif

  /**
  ### Computation of $\Psi = \log \mathbf{A}$ and upper convective term */

  foreach() {
    /**
      We assume that the stress tensor $\mathbf{\tau}_p$ depends on the
      conformation tensor $\mathbf{A}$ as follows
      $$
      \mathbf{\tau}_p = G_p (\mathbf{A}) =
      G_p (\mathbf{A} - I)
      $$
    */

    pseudo_t A;

    A.x.x = A11[]; A.y.y = A22[];
    A.x.y = A12[];

#if AXI
    double Aqq = AThTh[]; 
    Psiqq[] = log (Aqq); 
#endif

    /**
    The conformation tensor is diagonalized through the
    eigenvector tensor $\mathbf{R}$ and the eigenvalues diagonal
    tensor, $\Lambda$. */

    pseudo_v Lambda;
    init_pseudo_v(&Lambda, 0.0); 
    pseudo_t R;
    init_pseudo_t(&R, 0.0);
    diagonalization_2D (&Lambda, &R, &A);

    /*
    Check for negative eigenvalues -- this should never happen. If it does, print the location and value of the offending eigenvalue.
    Please report this bug by opening an issue on the GitHub repository. 
    */
    if (Lambda.x <= 0. || Lambda.y <= 0.) {
      fprintf(ferr, "Negative eigenvalue detected: Lambda.x = %g, Lambda.y = %g\n", Lambda.x, Lambda.y);
      fprintf(ferr, "x = %g, y = %g\n", x, y);
      exit(1);
    }
    
    /**
    $\Psi = \log \mathbf{A}$ is easily obtained after diagonalization, 
    $\Psi = R \cdot \log(\Lambda) \cdot R^T$. */
    
    Psi12[] = R.x.x*R.y.x*log(Lambda.x) + R.y.y*R.x.y*log(Lambda.y);
    Psi11[] = sq(R.x.x)*log(Lambda.x) + sq(R.x.y)*log(Lambda.y);
    Psi22[] = sq(R.y.y)*log(Lambda.y) + sq(R.y.x)*log(Lambda.x);

    /**
    We now compute the upper convective term $2 \mathbf{B} +
    (\Omega \cdot \Psi -\Psi \cdot \Omega)$.

    The diagonalization will be applied to the velocity gradient
    $(\nabla u)^T$ to obtain the antisymmetric tensor $\Omega$ and
    the traceless, symmetric tensor, $\mathbf{B}$. If the conformation
    tensor is $\mathbf{I}$, $\Omega = 0$ and $\mathbf{B}= \mathbf{D}$.  

    Otherwise, compute M = R * (nablaU)^T * R^T, where nablaU is the velocity gradient tensor. Then, 
    
    1. Calculate omega using the off-diagonal elements of M and eigenvalues:
       omega = (Lambda.y*M.x.y + Lambda.x*M.y.x)/(Lambda.y - Lambda.x)
       This represents the rotation rate in the eigenvector basis.
    
    2. Transform omega back to physical space to get OM:
       OM = (R.x.x*R.y.y - R.x.y*R.y.x)*omega
       This gives us the rotation tensor Omega in the original coordinate system.
    
    3. Compute B tensor components using M and R: B is related to M and R through:
       
       In 2D:
       $$
       B_{xx} = R_{xx}^2 M_{xx} + R_{xy}^2 M_{yy} \\
       B_{xy} = R_{xx}R_{yx} M_{xx} + R_{xy}R_{yy} M_{yy} \\
       B_{yx} = B_{xy} \\
       B_{yy} = -B_{xx}
       $$
       
       Where:
       - R is the eigenvector matrix of the conformation tensor
       - M is the velocity gradient tensor in the eigenvector basis
       - The construction ensures B is symmetric and traceless
    */

    pseudo_t B;
    init_pseudo_t(&B, 0.0);
    double OM = 0.;
    if (fabs(Lambda.x - Lambda.y) <= 1e-20) {
      B.x.y = (u.y[1,0] - u.y[-1,0] + u.x[0,1] - u.x[0,-1])/(4.*Delta); 
      foreach_dimension() 
        B.x.x = (u.x[1,0] - u.x[-1,0])/(2.*Delta);
    } else {
      pseudo_t M;
      init_pseudo_t(&M, 0.0);
      foreach_dimension() {
        M.x.x = (sq(R.x.x)*(u.x[1] - u.x[-1]) +
        sq(R.y.x)*(u.y[0,1] - u.y[0,-1]) +
        R.x.x*R.y.x*(u.x[0,1] - u.x[0,-1] + 
        u.y[1] - u.y[-1]))/(2.*Delta);
        
        M.x.y = (R.x.x*R.x.y*(u.x[1] - u.x[-1]) + 
        R.x.y*R.y.x*(u.y[1] - u.y[-1]) +
        R.x.x*R.y.y*(u.x[0,1] - u.x[0,-1]) +
        R.y.x*R.y.y*(u.y[0,1] - u.y[0,-1]))/(2.*Delta);
      }
      double omega = (Lambda.y*M.x.y + Lambda.x*M.y.x)/(Lambda.y - Lambda.x);
      OM = (R.x.x*R.y.y - R.x.y*R.y.x)*omega;

      B.x.y = M.x.x*R.x.x*R.y.x + M.y.y*R.y.y*R.x.y;
      foreach_dimension()
        B.x.x = M.x.x*sq(R.x.x)+M.y.y*sq(R.x.y);	
    }

    /**
    We now advance $\Psi$ in time, adding the upper convective
    contribution. */

    double s = -Psi12[];
    Psi12[] += dt * (2. * B.x.y + OM * (Psi22[] - Psi11[]));
    s *= -1;
    Psi11[] += dt * 2. * (B.x.x + s * OM);
    s *= -1;
    Psi22[] += dt * 2. * (B.y.y + s * OM);

    /**
    In the axisymmetric case, the governing equation for $\Psi_{\theta
    \theta}$ only involves that component, 
    $$ 
    \Psi_{\theta \theta}|_t - 2 L_{\theta \theta} = 
    \frac{\mathbf{f}_r(e^{-\Psi_{\theta \theta}})}{\lambda} 
    $$
    with $L_{\theta \theta} = u_y/y$. Therefore step (a) for
    $\Psi_{\theta \theta}$ is */

#if AXI
    Psiqq[] += dt*2.*u.y[]/max(y, 1e-20);
#endif

}

  /**
  ### Advection of $\Psi$
  
  We proceed with step (b), the advection of the log of the
  conformation tensor $\Psi$. */

#if AXI
  advection ({Psi11, Psi12, Psi22, Psiqq}, uf, dt);
#else
  advection ({Psi11, Psi12, Psi22}, uf, dt);
#endif

  /**
  ### Convert back to Aij */

  foreach() {
    /**
    It is time to undo the log-conformation, again by
    diagonalization, to recover the conformation tensor $\mathbf{A}$
    and to perform step (c).*/

    pseudo_t A = {{Psi11[], Psi12[]}, {Psi12[], Psi22[]}}, R;
    init_pseudo_t(&R, 0.0);
    pseudo_v Lambda;
    init_pseudo_v(&Lambda, 0.0);
    diagonalization_2D (&Lambda, &R, &A);
    Lambda.x = exp(Lambda.x), Lambda.y = exp(Lambda.y);
    
    A.x.y = R.x.x*R.y.x*Lambda.x + R.y.y*R.x.y*Lambda.y;
    foreach_dimension()
      A.x.x = sq(R.x.x)*Lambda.x + sq(R.x.y)*Lambda.y;
#if AXI
      double Aqq = exp(Psiqq[]);
#endif

    /**
    We perform now step (c) by integrating 
    $\mathbf{A}_t = -\mathbf{f}_r (\mathbf{A})/\lambda$ to obtain
    $\mathbf{A}^{n+1}$. This step is analytic,
    $$
    \int_{t^n}^{t^{n+1}}\frac{d \mathbf{A}}{\mathbf{I}- \mathbf{A}} = 
    \frac{\Delta t}{\lambda}
    $$
    */
#if AXI
     double tauD = sqrt((1./6.)*((T11[] - T22[])*(T11[] - T22[])+(T22[] - T_ThTh[])*(T22[] - T_ThTh[])+(T_ThTh[] - T11[])*(T_ThTh[] - T11[])) + T12[]*T12[]);
#else
    double tauD = sqrt(0.25*(T11[] - T22[])*(T11[] - T22[]) + T12[]*T12[]);
#endif
    double yieldFactor = max(0., (tauD - tau0[])/(tauD + 1e-6)); // $\mathcal{F}$
    double intFactor = (lambda[] != 0. ? (lambda[] == 1e30 ? 1: exp(-dt*yieldFactor/lambda[])): 0.);
     
#if AXI
      Aqq = (1. - intFactor) + intFactor*exp(Psiqq[]);
#endif

    A.x.y *= intFactor;
    foreach_dimension()
      A.x.x = (1. - intFactor) + A.x.x*intFactor;

    /**
      Then the Conformation tensor $\mathcal{A}_p^{n+1}$ is restored from
      $\mathbf{A}^{n+1}$.  */
    
    A12[] = A.x.y;
    T12[] = Gp[]*A.x.y;
#if AXI
      AThTh[] = Aqq;
      T_ThTh[] = Gp[]*(Aqq - 1.);
#endif

    A11[] = A.x.x;
    T11[] = Gp[]*(A.x.x - 1.);
    A22[] = A.y.y;
    T22[] = Gp[]*(A.y.y - 1.);
  }
}

/**
### Divergence of the extra stress tensor

The extra stress tensor $\mathbf{\tau}_p$ is defined at cell centers
while the corresponding force (acceleration) will be defined at cell
faces. Two terms contribute to each component of the momentum
equation. For example the $x$-component in Cartesian coordinates has
the following terms: $\partial_x \mathbf{\tau}_{p_{xx}} + \partial_y
\mathbf{\tau}_{p_{xy}}$. The first term is easy to compute since it can be
calculated directly from center values of cells sharing the face. The
other one is harder. It will be computed from vertex values. The
vertex values are obtained by averaging centered values.  Note that as
a result of the vertex averaging cells `[]` and `[-1,0]` are not
involved in the computation of shear. */

event acceleration (i++)
{
  face vector av = a;

  foreach_face(x){
    if (fm.x[] > 1e-20) {
      
      double shearX = (T12[0,1]*cm[0,1] + T12[-1,1]*cm[-1,1] - 
      T12[0,-1]*cm[0,-1] - T12[-1,-1]*cm[-1,-1])/4.;
      
      av.x[] += (shearX + cm[]*T11[] - cm[-1]*T11[-1])*
      alpha.x[]/(sq(fm.x[])*Delta);
    
    }
  }

  foreach_face(y){
    if (fm.y[] > 1e-20) {

      double shearY = (T12[1,0]*cm[1,0] + T12[1,-1]*cm[1,-1] - 
      T12[-1,0]*cm[-1,0] - T12[-1,-1]*cm[-1,-1])/4.;
      
      av.y[] += (shearY + cm[]*T22[] - cm[0,-1]*T22[0,-1])*
      alpha.y[]/(sq(fm.y[])*Delta);

    }
  }

#if AXI
  foreach_face(y)
    if (y > 1e-20)
      av.y[] -= (T_ThTh[] + T_ThTh[0,-1])*alpha.y[]/sq(y)/2.;
#endif
}
