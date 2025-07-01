/** 
# Log-Conformation Method for 3D Elasto-viscoplastic Fluids

## Overview
- **Title**: log-conform-elastoviscoplastic-3D.h
- **Version**: 2.7
- **Description**: Implementation of the log-conformation method for elasto-viscoplastic fluids in 3D

### Key Features
1. Conformation tensor A exists across the domain and relaxes according to λ
2. Stress acts according to elastic modulus G
3. 3D implementation extending log-conform-elastoviscoplastic.h
4. Eigenvalue clamping for numerical stability

### Author Information
- **Name**: Vatsal Sanjay, Arivazhagan G. Balasubramanian (Ari)
- **Email**: vatsalsanjay@gmail.com
- **Institution**: Physics of Fluids
- **Last Updated**: Jun 30, 2025

### Dependencies
- bcg.h: Bell-Collela-Glaz scheme for advection
- eigen_decomposition.h: For 3D eigenvalue computation
- navier-stokes/centered.h: For base flow solver

### References
- Fattal & Kupferman (2004, 2005): Original log-conformation method
- Comminal et al. (2015): Constitutive model functions
- Hao & Pan (2007): Split scheme implementation

## Version History

### v1.0 (Oct 19, 2024)
- Initial 3D implementation
- Scalar implementation approach

### v1.1 (Oct 20, 2024)
- Added negative eigenvalue detection
- Added error reporting system

### v2.0 (Oct 29, 2024)
- Major matrix algebra corrections for 3D
- Optimized tensor calculations
- Improved code structure and documentation

### v2.1 (Oct 29, 2024)
- Added initialization functions for tensor structures

### v2.2 (Nov 3, 2024)
- Refactored tensor operations
- Improved code maintainability
- Enhanced tensor manipulation consistency

### v2.3 (Nov 14, 2024)
- Added infinite Deborah number support

### v2.5 (Nov 23, 2024)
- Documentation improvements
- Added mathematical explanations

### v2.6 (Mar 16, 2025)
- Implemented eigenvalue clamping system
- Added minimum eigenvalue threshold (EIGENVALUE_MIN = 1e-8)
- Improved numerical stability handling
- Added diagnostic capabilities
- Fixed 3D velocity gradient calculation

### v2.7 (Jun 30, 2025)
- Added the plasticity based relaxation module (Saramito (2007) model)

## Implementation Notes
1. The code extends the standard Basilisk log-conformation implementation
2. Uses G-λ formulation for better physical interpretation
3. Fixes surface tension coupling with polymeric stress
4. Includes both 2D and 3D implementations
5. Uses atomic operations for thread-safe diagnostics

## Future Work
### Axisymmetric Compatibility
- Currently not implemented
- Use log-conform-elastoviscoplastic-scalar-2D.h for axi cases
- Or use log-conform-elastoviscoplastic.h for better efficiency

### Metric Terms Improvements
- [ ] Enforce tensor compatibility using foreach_dimension
- [ ] Complete metric terms (cm, fm) implementation
*/

#if AXI
#error "axi compatibility is not there. To keep the code easy to read, we will not implement axi compatibility just yet."
#endif

/**
# The log-conformation method for some elasto-viscoplastic constitutive models

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
the polymer chains. $\mathbf{A}$ is governed by the equation
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
$\lambda$ and the elastic modulus $G_p$. The solvent viscosity
$\mu_s$ is defined in the [Navier-Stokes
solver](navier-stokes/centered.h). 

Gp and lambda are defined in [two-phaseVE.h](two-phaseVE.h).
*/

/**
## The log conformation approach

The numerical resolution of viscoelastic and EVP fluid problems often faces the
[High-Weissenberg Number
Problem](http://www.ma.huji.ac.il/~razk/iWeb/My_Site/Research_files/Visco1.pdf). 
This is a numerical instability appearing when strongly elastic flows
create regions of high stress and fine features. This instability
poses practical limits to the values of the relaxation time of the
viscoelastic and EVP fluids, $\lambda$.  [Fattal \& Kupferman (2004,
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
\frac{G_p}{\lambda} \mathbf{f}_s (\mathbf{A}).
$$

We will use the Bell--Collela--Glaz scheme to advect the log-conformation 
tensor $\Psi$. */

/*
TODO: 
- Perhaps, instead of the Bell--Collela--Glaz scheme, we can use the conservative form of the advection equation and transport the log-conformation tensor with the VoF color function, similar to [http://basilisk.fr/src/navier-stokes/conserving.h](http://basilisk.fr/src/navier-stokes/conserving.h)
*/

#define EIGENVALUE_MIN 1e-8

#ifdef DEBUG_EIGENVALUES
static int eigenvalue_corrections = 0;
#endif

#include "bcg.h"

(const) scalar Gp = unity; // elastic modulus
(const) scalar lambda = unity; // relaxation time
(const) scalar tau0 = unity; // yield-stress

/*
conformation tensor */
// diagonal elements
scalar A11[], A22[], A33[];
// off-diagonal elements
scalar A12[], A13[], A23[];
/*
stress tensor */
// diagonal elements
scalar T11[], T22[], T33[];
// off-diagonal elements
scalar T12[], T13[], T23[];

event defaults (i = 0) {
  if (is_constant (a.x))
    a = new face vector;

  /*
  initialize A and T
  */
  for (scalar s in {A11, A22, A33}) {
    foreach () {
      s[] = 1.;
    }
  }
  for (scalar s in {T11, T12, T13, T22, T23, T33, A12, A13, A23}) {
    foreach(){
      s[] = 0.;
    }
  }

  for (scalar s in {A11, A22, A33, T11, T22, T33, A12, A13, A23, T12, T13, T23}) {
    if (s.boundary[left] != periodic_bc) {
      s[left] = neumann(0);
	    s[right] = neumann(0);
    }
    if (s.boundary[top] != periodic_bc) {
      s[top] = neumann(0);
	    s[bottom] = neumann(0);
    }
#if dimension == 3
    if (s.boundary[front] != periodic_bc) {
      s[front] = neumann(0);
	    s[back] = neumann(0);
    }
#endif
  }
}

/**
## Useful functions in 2D

The first step is to implement a routine to calculate the eigenvalues
and eigenvectors of the conformation tensor $\mathbf{A}$.

These structs ressemble Basilisk vectors and tensors but are just
arrays not related to the grid. */

#if dimension == 2
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
#endif

/*
Now this is the 3D implementation.
*/
#if dimension == 3

#include "eigen_decomposition.h"

typedef struct { double x, y, z; } pseudo_v3d;
typedef struct { pseudo_v3d x, y, z; } pseudo_t3d;

// Function to initialize pseudo_v3d
static inline void init_pseudo_v3d(pseudo_v3d *v, double value) {
    v->x = value;
    v->y = value;
    v->z = value;
}

// Function to initialize pseudo_t3d
static inline void init_pseudo_t3d(pseudo_t3d *t, double value) {
    init_pseudo_v3d(&t->x, value);
    init_pseudo_v3d(&t->y, value);
    init_pseudo_v3d(&t->z, value);
}

static void diagonalization_3D (pseudo_v3d * Lambda, pseudo_t3d * R, pseudo_t3d * A)
{
  // Check if the matrix is already diagonal
  if (sq(A->x.y) + sq(A->x.z) + sq(A->y.z) < 1e-15) {
    R->x.x = R->y.y = R->z.z = 1.;
    R->y.x = R->x.y = R->z.x = R->x.z = R->z.y = R->y.z = 0.;
    Lambda->x = A->x.x; Lambda->y = A->y.y; Lambda->z = A->z.z;
    return;
  }

  // Compute eigenvalues using the eigen_decomposition function
  double matrix[3][3] = {
    {A->x.x, A->x.y, A->x.z},
    {A->y.x, A->y.y, A->y.z},
    {A->z.x, A->z.y, A->z.z}
  };
  double eigenvectors[3][3];
  double eigenvalues[3];
  
  compute_eigensystem_symmetric_3x3(matrix, eigenvectors, eigenvalues);

  // Store eigenvalues and eigenvectors
  Lambda->x = eigenvalues[0];
  Lambda->y = eigenvalues[1];
  Lambda->z = eigenvalues[2];

  R->x.x = eigenvectors[0][0]; R->x.y = eigenvectors[0][1]; R->x.z = eigenvectors[0][2];
  R->y.x = eigenvectors[1][0]; R->y.y = eigenvectors[1][1]; R->y.z = eigenvectors[1][2];
  R->z.x = eigenvectors[2][0]; R->z.y = eigenvectors[2][1]; R->z.z = eigenvectors[2][2];

}
#endif

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

#if dimension == 2
event tracer_advection(i++)
{
  scalar Psi11 = A11;
  scalar Psi12 = A12;
  scalar Psi22 = A22;

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

    pseudo_t A = {{A11[], A12[]}, {A12[], A22[]}};

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
    Check for negative eigenvalues and clamp them to EIGENVALUE_MIN.
    This prevents numerical instabilities while maintaining physical meaning.
    */
    if (Lambda.x <= 0. || Lambda.y <= 0.) {
      fprintf(ferr, "WARNING: Negative eigenvalue detected at (%g,%g): [%g,%g]\n", 
              x, y, Lambda.x, Lambda.y);
      
      #ifdef DEBUG_EIGENVALUES
      atomic_increment(&eigenvalue_corrections);
      #endif
      
      Lambda.x = max(Lambda.x, EIGENVALUE_MIN);
      Lambda.y = max(Lambda.y, EIGENVALUE_MIN);
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
  }

  /**
  ### Advection of $\Psi$
 
  We proceed with step (b), the advection of the log of the
  conformation tensor $\Psi$. */

  advection ({Psi11, Psi12, Psi22}, uf, dt);

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

    /**
    We perform now step (c) by integrating 
    $\mathbf{A}_t = -\mathbf{f}_r (\mathbf{A})/\lambda$ to obtain
    $\mathbf{A}^{n+1}$. This step is analytic,
    $$
    \int_{t^n}^{t^{n+1}}\frac{d \mathbf{A}}{\mathbf{I}- \mathbf{A}} = 
    \frac{\Delta t}{\lambda}
    $$
    */

    double tauD = sqrt(0.25*(T11[] - T22[])*(T11[] - T22[]) + T12[]*T12[]);
    double yieldFactor = max(0., (tauD - tau0[])/(tauD + 1e-6)); // $\mathcal{F}$
    double intFactor = (lambda[] != 0. ? (lambda[] == 1e30 ? 1: exp(-dt*yieldFactor/lambda[])): 0.);
    
    A.x.y *= intFactor;
    foreach_dimension()
      A.x.x = (1. - intFactor) + A.x.x*intFactor;

    /**
      Then the Conformation tensor $\mathcal{A}_p^{n+1}$ is restored from
      $\mathbf{A}^{n+1}$.  */
    
    A12[] = A.x.y;
    T12[] = Gp[]*A.x.y;
    A11[] = A.x.x;
    T11[] = Gp[]*(A.x.x - 1.);
    A22[] = A.y.y;
    T22[] = Gp[]*(A.y.y - 1.);
  }
}

#elif dimension == 3

event tracer_advection(i++)
{
  /**
  ### Computation of $\Psi = \log \mathbf{A}$ and upper convective term */

  // start by declaring the scalar variables that will store the components of $\Psi$
  scalar Psi11 = A11, Psi12 = A12, Psi13 = A13,
         Psi22 = A22, Psi23 = A23, Psi33 = A33;

  foreach() {
    pseudo_t3d A, R;
    init_pseudo_t3d(&R, 0.0);
    pseudo_v3d Lambda;
    init_pseudo_v3d(&Lambda, 0.0);

    A.x.x = A11[]; A.x.y = A12[]; A.x.z = A13[];
    A.y.x = A12[]; A.y.y = A22[]; A.y.z = A23[];
    A.z.x = A13[]; A.z.y = A23[]; A.z.z = A33[];

    // Diagonalize the conformation tensor A to obtain the eigenvalues Lambda and eigenvectors R
    diagonalization_3D (&Lambda, &R, &A);

    /*
    Check for negative eigenvalues and clamp them to EIGENVALUE_MIN.
    This prevents numerical instabilities while maintaining physical meaning.
    */
    if (Lambda.x <= 0. || Lambda.y <= 0. || Lambda.z <= 0.) {
      fprintf(ferr, "WARNING: Negative eigenvalue detected at (%g,%g,%g): [%g,%g,%g]\n", 
              x, y, z, Lambda.x, Lambda.y, Lambda.z);
      
      #ifdef DEBUG_EIGENVALUES
      atomic_increment(&eigenvalue_corrections);
      #endif
      
      Lambda.x = max(Lambda.x, EIGENVALUE_MIN);
      Lambda.y = max(Lambda.y, EIGENVALUE_MIN);
      Lambda.z = max(Lambda.z, EIGENVALUE_MIN);
    }
    
    // Compute Psi = log(A) = R * log(Lambda) * R^T
    Psi11[] = sq(R.x.x)*log(Lambda.x) + sq(R.x.y)*log(Lambda.y) + sq(R.x.z)*log(Lambda.z);
    Psi22[] = sq(R.y.x)*log(Lambda.x) + sq(R.y.y)*log(Lambda.y) + sq(R.y.z)*log(Lambda.z);
    Psi33[] = sq(R.z.x)*log(Lambda.x) + sq(R.z.y)*log(Lambda.y) + sq(R.z.z)*log(Lambda.z);

    Psi12[] = R.x.x*R.y.x*log(Lambda.x) + R.x.y*R.y.y*log(Lambda.y) + R.x.z*R.y.z*log(Lambda.z);
    Psi13[] = R.x.x*R.z.x*log(Lambda.x) + R.x.y*R.z.y*log(Lambda.y) + R.x.z*R.z.z*log(Lambda.z);
    Psi23[] = R.y.x*R.z.x*log(Lambda.x) + R.y.y*R.z.y*log(Lambda.y) + R.y.z*R.z.z*log(Lambda.z);

    // Compute B and Omega tensors (3D version)
    pseudo_t3d B, M, Omega;
    init_pseudo_t3d(&B, 0.0);
    init_pseudo_t3d(&M, 0.0);
    init_pseudo_t3d(&Omega, 0.0);

    // Check if any pair of eigenvalues are numerically equal (within a small tolerance)
    if (fabs(Lambda.x - Lambda.y) <= 1e-20 || fabs(Lambda.y - Lambda.z) <= 1e-20 || fabs(Lambda.z - Lambda.x) <= 1e-20) {
      // In case of equal eigenvalues, the calculations for B and Omega simplify significantly
      // B is grad U and Omega is zero.

      // Compute off-diagonal elements of B using central differences
      // These represent the symmetric part of the velocity gradient tensor
      B.x.y = (u.y[1,0,0] - u.y[-1,0,0] + u.x[0,1,0] - u.x[0,-1,0])/(4.*Delta);  // (dv/dx + du/dy)/2
      B.x.z = (u.z[1,0,0] - u.z[-1,0,0] + u.x[0,0,1] - u.x[0,0,-1])/(4.*Delta);  // (dw/dx + du/dz)/2
      B.y.z = (u.z[0,1,0] - u.z[0,-1,0] + u.y[0,0,1] - u.y[0,0,-1])/(4.*Delta);  // (dw/dy + dv/dz)/2

      // Compute diagonal elements of B
      // These represent the normal strain rates
      B.x.x = (u.x[1,0,0] - u.x[-1,0,0])/(2.*Delta);  // du/dx
      B.y.y = (u.y[0,1,0] - u.y[0,-1,0])/(2.*Delta);  // dv/dy
      B.z.z = (u.z[0,0,1] - u.z[0,0,-1])/(2.*Delta);  // dw/dz

      // Set all components of Omega to zero
      // This is because Omega represents the antisymmetric part of the velocity gradient tensor,
      // which vanishes when eigenvalues are equal
      Omega.x.y = Omega.x.z = Omega.y.z = Omega.y.x = Omega.z.x = Omega.z.y = 0.;

    } else {
      
      /*
      ### Compute the velocity gradient tensor components using central differences
      - These represent the spatial derivatives of each velocity component
      - These gradients form the velocity gradient tensor (nablaU):
      [ dudx  dudy  dudz ]
      [ dvdx  dvdy  dvdz ]
      [ dwdx  dwdy  dwdz ]
      */

      // Derivatives of u (x-component of velocity)
      double dudx = (u.x[1,0,0] - u.x[-1,0,0])/(2.0*Delta);  // du/dx
      double dudy = (u.x[0,1,0] - u.x[0,-1,0])/(2.0*Delta);  // du/dy
      double dudz = (u.x[0,0,1] - u.x[0,0,-1])/(2.0*Delta);  // du/dz

      // Derivatives of v (y-component of velocity)
      double dvdx = (u.y[1,0,0] - u.y[-1,0,0])/(2.0*Delta);  // dv/dx
      double dvdy = (u.y[0,1,0] - u.y[0,-1,0])/(2.0*Delta);  // dv/dy
      double dvdz = (u.y[0,0,1] - u.y[0,0,-1])/(2.0*Delta);  // dv/dz

      // Derivatives of w (z-component of velocity)
      double dwdx = (u.z[1,0,0] - u.z[-1,0,0])/(2.0*Delta);  // dw/dx
      double dwdy = (u.z[0,1,0] - u.z[0,-1,0])/(2.0*Delta);  // dw/dy
      double dwdz = (u.z[0,0,1] - u.z[0,0,-1])/(2.0*Delta);  // dw/dz

      /*
      Calculate the M tensor through matrix multiplication: M = R * (nablaU)^T R^T. This represents the velocity gradient tensor transformed to the eigenvector basis of the conformation tensor.
      
      * Steps:
      1. Compute intermediate products (R * nablaU^T):
         - Store row-wise products in Rx_gradU_*, Ry_gradU_*, Rz_gradU_*
         - Each variable represents one row of the intermediate matrix
      2. Multiply by R^T to obtain the final M tensor:
         - M.i.j represents the (i,j) component of the transformed velocity gradient
         - This transformation expresses the velocity gradient in the eigenvector basis
         - The resulting M tensor is used to compute Omega (Ω) and B tensors, such that 
      */

      // First, compute intermediate products of R and (nablaU)^T
      double Rx_gradU_x = R.x.x*dudx + R.x.y*dvdx + R.x.z*dwdx;
      double Rx_gradU_y = R.x.x*dudy + R.x.y*dvdy + R.x.z*dwdy;
      double Rx_gradU_z = R.x.x*dudz + R.x.y*dvdz + R.x.z*dwdz;

      double Ry_gradU_x = R.y.x*dudx + R.y.y*dvdx + R.y.z*dwdx;
      double Ry_gradU_y = R.y.x*dudy + R.y.y*dvdy + R.y.z*dwdy;
      double Ry_gradU_z = R.y.x*dudz + R.y.y*dvdz + R.y.z*dwdz;

      double Rz_gradU_x = R.z.x*dudx + R.z.y*dvdx + R.z.z*dwdx;
      double Rz_gradU_y = R.z.x*dudy + R.z.y*dvdy + R.z.z*dwdy;
      double Rz_gradU_z = R.z.x*dudz + R.z.y*dvdz + R.z.z*dwdz;

      // Now compute M components by multiplying the intermediate products with R^T
      M.x.x = R.x.x*Rx_gradU_x + R.x.y*Rx_gradU_y + R.x.z*Rx_gradU_z;
      M.x.y = R.x.x*Ry_gradU_x + R.x.y*Ry_gradU_y + R.x.z*Ry_gradU_z;
      M.x.z = R.x.x*Rz_gradU_x + R.x.y*Rz_gradU_y + R.x.z*Rz_gradU_z;

      M.y.x = R.y.x*Rx_gradU_x + R.y.y*Rx_gradU_y + R.y.z*Rx_gradU_z;
      M.y.y = R.y.x*Ry_gradU_x + R.y.y*Ry_gradU_y + R.y.z*Ry_gradU_z;
      M.y.z = R.y.x*Rz_gradU_x + R.y.y*Rz_gradU_y + R.y.z*Rz_gradU_z;

      M.z.x = R.z.x*Rx_gradU_x + R.z.y*Rx_gradU_y + R.z.z*Rx_gradU_z;
      M.z.y = R.z.x*Ry_gradU_x + R.z.y*Ry_gradU_y + R.z.z*Ry_gradU_z;
      M.z.z = R.z.x*Rz_gradU_x + R.z.y*Rz_gradU_y + R.z.z*Rz_gradU_z;

      // Compute the off-diagonal elements of the Omega tensor in the eigenvector basis
      double omega_xy = (Lambda.y*M.x.y + Lambda.x*M.y.x)/(Lambda.y - Lambda.x);
      double omega_xz = (Lambda.z*M.x.z + Lambda.x*M.z.x)/(Lambda.z - Lambda.x);
      double omega_yz = (Lambda.z*M.y.z + Lambda.y*M.z.y)/(Lambda.z - Lambda.y);

      // Calculate intermediate rotation combinations for each direction
      // x-direction rotation combinations
      double rot_x_xy_yz = (R.x.x*omega_xy - R.x.z*omega_yz);  // xy rotation minus yz rotation, x components
      double rot_x_xy_xz = (R.x.y*omega_xy + R.x.z*omega_xz);  // xy rotation plus xz rotation, x components
      double rot_x_xz_yz = (R.x.x*omega_xz + R.x.y*omega_yz);  // xz rotation plus yz rotation, x components

      // y-direction rotation combinations
      double rot_y_xy_yz = (R.y.x*omega_xy - R.y.z*omega_yz);  // xy rotation minus yz rotation, y components
      double rot_y_xy_xz = (R.y.y*omega_xy + R.y.z*omega_xz);  // xy rotation plus xz rotation, y components
      double rot_y_xz_yz = (R.y.x*omega_xz + R.y.y*omega_yz);  // xz rotation plus yz rotation, y components

      // z-direction rotation combinations
      double rot_z_xy_yz = (R.z.x*omega_xy - R.z.z*omega_yz);  // xy rotation minus yz rotation, z components
      double rot_z_xy_xz = (R.z.y*omega_xy + R.z.z*omega_xz);  // xy rotation plus xz rotation, z components
      double rot_z_xz_yz = (R.z.x*omega_xz + R.z.y*omega_yz);  // xz rotation plus yz rotation, z components

      /* Calculate the components of the Omega tensor in the physical coordinate system
       * 
       * The Omega tensor represents the rotational part of the velocity gradient tensor
       * and is computed through the following steps:
       * 
       * 1. We already have:
       *    - R: eigenvector matrix of the conformation tensor
       *    - rot_*_*_*: pre-computed rotation combinations for each direction
       * 
       * 2. Mathematical background:
       *    Omega = R * Omega_eigen * R^T
       *    where Omega_eigen is the rotation tensor in eigenvector space
       * 
       * 3. The components are calculated using the rotation combinations:
       *    - rot_i_jk_lm represents combined rotations in the i-direction
       *    - Each component Omega_ij is a linear combination of these rotations
       */

      // Compute x-row components of Omega
      Omega.x.x = R.x.y*rot_x_xy_yz  // xy-yz rotation contribution
                - R.x.x*rot_x_xy_xz   // xy-xz rotation contribution
                + R.x.z*rot_x_xz_yz;  // xz-yz rotation contribution
      
      Omega.x.y = R.y.y*rot_x_xy_yz  // xy-yz rotation mapped to y-direction
                - R.y.x*rot_x_xy_xz   // xy-xz rotation mapped to y-direction
                + R.y.z*rot_x_xz_yz;  // xz-yz rotation mapped to y-direction
      
      Omega.x.z = R.z.y*rot_x_xy_yz  // xy-yz rotation mapped to z-direction
                - R.z.x*rot_x_xy_xz   // xy-xz rotation mapped to z-direction
                + R.z.z*rot_x_xz_yz;  // xz-yz rotation mapped to z-direction

      // Compute y-row components using similar pattern
      Omega.y.x = R.x.y*rot_y_xy_yz - R.x.x*rot_y_xy_xz + R.x.z*rot_y_xz_yz;
      Omega.y.y = R.y.y*rot_y_xy_yz - R.y.x*rot_y_xy_xz + R.y.z*rot_y_xz_yz;
      Omega.y.z = R.z.y*rot_y_xy_yz - R.z.x*rot_y_xy_xz + R.z.z*rot_y_xz_yz;

      // Compute z-row components using similar pattern
      Omega.z.x = R.x.y*rot_z_xy_yz - R.x.x*rot_z_xy_xz + R.x.z*rot_z_xz_yz;
      Omega.z.y = R.y.y*rot_z_xy_yz - R.y.x*rot_z_xy_xz + R.y.z*rot_z_xz_yz;
      Omega.z.z = R.z.y*rot_z_xy_yz - R.z.x*rot_z_xy_xz + R.z.z*rot_z_xz_yz;

      /* Note: The resulting Omega tensor is skew-symmetric, meaning:
       * Omega_ij = -Omega_ji
       * This property is automatically satisfied by the construction above
       * and is essential for preserving the physical meaning of rotation
       */

      // Extract diagonal components of M (velocity gradient tensor in eigenvector basis)
      double M_diag_x = M.x.x, M_diag_y = M.y.y, M_diag_z = M.z.z;

      /*
      Compute B tensor: B = R * diag(M) * R^T
      - This transforms the diagonal velocity gradient tensor back to the original coordinate system
      - B is symmetric, so we only need to compute the upper triangle
      */

      // Compute diagonal elements of B
      B.x.x = M_diag_x*sq(R.x.x) + M_diag_y*sq(R.x.y) + M_diag_z*sq(R.x.z);
      B.y.y = M_diag_x*sq(R.y.x) + M_diag_y*sq(R.y.y) + M_diag_z*sq(R.y.z);
      B.z.z = M_diag_x*sq(R.z.x) + M_diag_y*sq(R.z.y) + M_diag_z*sq(R.z.z);

      // Compute off-diagonal elements of B (upper triangle)
      B.x.y = M_diag_x*R.x.x*R.y.x + M_diag_y*R.x.y*R.y.y + M_diag_z*R.x.z*R.y.z;
      B.x.z = M_diag_x*R.x.x*R.z.x + M_diag_y*R.x.y*R.z.y + M_diag_z*R.x.z*R.z.z;
      B.y.z = M_diag_x*R.y.x*R.z.x + M_diag_y*R.y.y*R.z.y + M_diag_z*R.y.z*R.z.z;

      // Fill in lower triangle using symmetry of B
      B.y.x = B.x.y;
      B.z.x = B.x.z;
      B.z.y = B.y.z;
    }

    /**
    We now advance $\Psi$ in time, adding the upper convective
    contribution. 
    This step 1: \partial_t \Psi = 2 \mathbf{B} + (\Omega \cdot \Psi -\Psi \cdot \Omega)
    */

    // save old values of Psi components
    double old_Psi11 = Psi11[];
    double old_Psi22 = Psi22[];
    double old_Psi33 = Psi33[];
    double old_Psi12 = Psi12[];
    double old_Psi13 = Psi13[];
    double old_Psi23 = Psi23[];

    // Psi11
    Psi11[] += dt * (2.0 * B.x.x + Omega.x.y * old_Psi12 - Omega.y.x * old_Psi12 + Omega.x.z * old_Psi13 - Omega.z.x * old_Psi13);
    // Psi22
    Psi22[] += dt * (2.0 * B.y.y - Omega.x.y * old_Psi12 + Omega.y.x * old_Psi12 + Omega.y.z * old_Psi23 - Omega.z.y * old_Psi23);
    // Psi33
    Psi33[] += dt * (2.0 * B.z.z - Omega.x.z * old_Psi13 + Omega.z.x * old_Psi13 - Omega.y.z * old_Psi23 + Omega.z.y * old_Psi23);
    // Psi12
    Psi12[] += dt * (2.0 * B.x.y + Omega.x.x * old_Psi12 - Omega.x.y * old_Psi11 + Omega.x.y * old_Psi22 - Omega.y.y * old_Psi12 + Omega.x.z * old_Psi23 - Omega.z.y * old_Psi13);
    // Psi13
    Psi13[] += dt * (2.0 * B.x.z + Omega.x.x*old_Psi13 - Omega.x.z * old_Psi11 + Omega.x.y*old_Psi23 - Omega.y.z*old_Psi12 + Omega.x.z * old_Psi33 - Omega.z.z * old_Psi13);
    // Psi23
    Psi23[] += dt * (2.0 * B.y.z + Omega.y.x * old_Psi13 - Omega.x.z * old_Psi12 + Omega.y.y * old_Psi23 - Omega.y.z * old_Psi22 + Omega.y.z * old_Psi33 - Omega.z.z * old_Psi23);

  }

  // Advection of Psi, which is the log-conformation tensor
  advection ({Psi11, Psi12, Psi13, Psi22, Psi23, Psi33}, uf, dt);

  /**
  ### Convert back to A and T

  We now convert the log-conformation tensor Psi back to the conformation tensor A
  and compute the stress tensor T. This process involves diagonalization,
  exponentiation of eigenvalues, and application of the relaxation factor.
  */

  foreach() {
    pseudo_t3d A, R;
    init_pseudo_t3d(&R, 0.0);
    pseudo_v3d Lambda;
    init_pseudo_v3d(&Lambda, 0.0);

    // Reconstruct the log-conformation tensor from its components
    A.x.x = Psi11[]; A.x.y = Psi12[]; A.x.z = Psi13[];
    A.y.x = Psi12[]; A.y.y = Psi22[]; A.y.z = Psi23[];
    A.z.x = Psi13[]; A.z.y = Psi23[]; A.z.z = Psi33[];

    // Diagonalize A to obtain eigenvalues and eigenvectors
    diagonalization_3D(&Lambda, &R, &A);
    
    // Exponentiate eigenvalues
    Lambda.x = exp(Lambda.x);
    Lambda.y = exp(Lambda.y);
    Lambda.z = exp(Lambda.z);

    // Reconstruct A using A = R * diag(Lambda) * R^T
    A.x.x = Lambda.x * sq(R.x.x) + Lambda.y * sq(R.x.y) + Lambda.z * sq(R.x.z);
    A.x.y = Lambda.x * R.x.x * R.y.x + Lambda.y * R.x.y * R.y.y + Lambda.z * R.x.z * R.y.z;
    A.y.x = A.x.y;
    A.x.z = Lambda.x * R.x.x * R.z.x + Lambda.y * R.x.y * R.z.y + Lambda.z * R.x.z * R.z.z;
    A.z.x = A.x.z;
    A.y.y = Lambda.x * sq(R.y.x) + Lambda.y * sq(R.y.y) + Lambda.z * sq(R.y.z);
    A.y.z = Lambda.x * R.y.x * R.z.x + Lambda.y * R.y.y * R.z.y + Lambda.z * R.y.z * R.z.z;
    A.z.y = A.y.z;
    A.z.z = Lambda.x * sq(R.z.x) + Lambda.y * sq(R.z.y) + Lambda.z * sq(R.z.z);

    // Apply relaxation using the relaxation time lambda

    double tauD = sqrt((1./6.)*((T11[] - T22[])*(T11[] - T22[])+(T22[] - T33[])*(T22[] - T33[])+(T33[] - T11[])*(T33[] - T11[])) + 3.*(T12[]*T12[] + T23[]*T23[] + T13[]*T13[]));
    double yieldFactor = max(0., (tauD - tau0[])/(tauD + 1e-6)); // $\mathcal{F}$
    double intFactor = (lambda[] != 0. ? (lambda[] == 1e30 ? 1: exp(-dt*yieldFactor/lambda[])): 0.);

    A.x.y *= intFactor;
    A.y.x = A.x.y;
    A.x.z *= intFactor;
    A.z.x = A.x.z;
    A.y.z *= intFactor;
    A.z.y = A.y.z;
    foreach_dimension()
      A.x.x = 1. + (A.x.x - 1.)*intFactor;

    /*
    Get Aij from A. These commands might look repetitive. But, I do this so that in the future, generalization to tensor only form is easier.
    */

    // diagonal terms:
    A11[] = A.x.x;
    A22[] = A.y.y;
    A33[] = A.z.z;
    // off-diagonal terms:
    A12[] = A.x.y;
    A13[] = A.x.z;
    A23[] = A.y.z;

    // Compute the stress tensor T using the polymer modulus Gp
    T11[] = Gp[]*(A.x.x - 1.);
    T22[] = Gp[]*(A.y.y - 1.);
    T33[] = Gp[]*(A.z.z - 1.);
    T12[] = Gp[]*A.x.y;
    T13[] = Gp[]*A.x.z;
    T23[] = Gp[]*A.y.z;
  }
}
#endif

/**
## Divergence of the extra stress tensor

The extra stress tensor $\mathbf{T}$ is defined at cell centers,
while the corresponding force (acceleration) is defined at cell faces. 
For each component of the momentum equation, we need to compute the 
divergence of the corresponding row of the stress tensor.

For example, for the x-component in 3D:

$$
(\nabla \cdot \mathbf{T})_x = \partial_x T_{xx} + \partial_y T_{xy} + \partial_z T_{xz}
$$

The normal stress gradient (e.g. $\partial_x T_{xx}$) is computed directly 
from cell-centered values. The shear stress gradients (e.g. $\partial_y T_{xy}$) 
are computed using vertex-averaged values to avoid checkerboard instabilities.
*/

event acceleration (i++)
{
  face vector av = a;

#if dimension == 2
  // 2D implementation
  foreach_face(x) {
    if (fm.x[] > 1e-20) {
      // y-gradient of T12 (shear stress)
      double shearX = (T12[0,1]*cm[0,1] + T12[-1,1]*cm[-1,1] - 
                       T12[0,-1]*cm[0,-1] - T12[-1,-1]*cm[-1,-1])/4.;
      // x-gradient of T11 (normal stress)
      double gradX_T11 = cm[]*T11[] - cm[-1]*T11[-1];
      
      av.x[] += (shearX + gradX_T11)*alpha.x[]/(sq(fm.x[])*Delta);
    }
  }
  
  foreach_face(y) {
    if (fm.y[] > 1e-20) {
      // x-gradient of T12 (shear stress)
      double shearY = (T12[1,0]*cm[1,0] + T12[1,-1]*cm[1,-1] - 
                       T12[-1,0]*cm[-1,0] - T12[-1,-1]*cm[-1,-1])/4.;
      // y-gradient of T22 (normal stress)
      double gradY_T22 = cm[]*T22[] - cm[0,-1]*T22[0,-1];
      
      av.y[] += (shearY + gradY_T22)*alpha.y[]/(sq(fm.y[])*Delta);
    }
  }

#elif dimension == 3
  // 3D implementation
  foreach_face(x) {
    if (fm.x[] > 1e-20) {
      // y-gradient of T12
      double shearY = (T12[0,1,0]*cm[0,1,0] + T12[-1,1,0]*cm[-1,1,0] - 
                       T12[0,-1,0]*cm[0,-1,0] - T12[-1,-1,0]*cm[-1,-1,0])/4.;
      // z-gradient of T13
      double shearZ = (T13[0,0,1]*cm[0,0,1] + T13[-1,0,1]*cm[-1,0,1] - 
                       T13[0,0,-1]*cm[0,0,-1] - T13[-1,0,-1]*cm[-1,0,-1])/4.;
      // x-gradient of T11
      double gradX_T11 = cm[]*T11[] - cm[-1,0,0]*T11[-1,0,0];
      
      av.x[] += (shearY + shearZ + gradX_T11)*alpha.x[]/(sq(fm.x[])*Delta);
    }
  }

  foreach_face(y) {
    if (fm.y[] > 1e-20) {
      // x-gradient of T12
      double shearX = (T12[1,0,0]*cm[1,0,0] + T12[1,-1,0]*cm[1,-1,0] - 
                       T12[-1,0,0]*cm[-1,0,0] - T12[-1,-1,0]*cm[-1,-1,0])/4.;
      // z-gradient of T23
      double shearZ = (T23[0,0,1]*cm[0,0,1] + T23[0,-1,1]*cm[0,-1,1] - 
                       T23[0,0,-1]*cm[0,0,-1] - T23[0,-1,-1]*cm[0,-1,-1])/4.;
      // y-gradient of T22
      double gradY_T22 = cm[]*T22[] - cm[0,-1,0]*T22[0,-1,0];
      
      av.y[] += (shearX + shearZ + gradY_T22)*alpha.y[]/(sq(fm.y[])*Delta);
    }
  }

  foreach_face(z) {
    if (fm.z[] > 1e-20) {
      // x-gradient of T13
      double shearX = (T13[1,0,0]*cm[1,0,0] + T13[1,0,-1]*cm[1,0,-1] - 
                       T13[-1,0,0]*cm[-1,0,0] - T13[-1,0,-1]*cm[-1,0,-1])/4.;
      // y-gradient of T23
      double shearY = (T23[0,1,0]*cm[0,1,0] + T23[0,1,-1]*cm[0,1,-1] - 
                       T23[0,-1,0]*cm[0,-1,0] - T23[0,-1,-1]*cm[0,-1,-1])/4.;
      // z-gradient of T33
      double gradZ_T33 = cm[]*T33[] - cm[0,0,-1]*T33[0,0,-1];
      
      av.z[] += (shearX + shearY + gradZ_T33)*alpha.z[]/(sq(fm.z[])*Delta);
    }
  }
#endif
}

