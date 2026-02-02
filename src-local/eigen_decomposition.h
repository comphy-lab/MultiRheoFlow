/**
# Symmetric 3x3 Eigen-Decomposition

Householder tridiagonalization and QL iteration for symmetric
3x3 matrices.
*/
#define SQUARE(x) ((x)*(x))

/**
### tridiagonalize_symmetric_3x3()

Reduces a symmetric 3x3 matrix to tridiagonal form using the
Householder method.

#### Parameters

- `matrix`: Input symmetric matrix.
- `eigenvectors`: Orthogonal Householder vectors.
- `diagonal`: Diagonal entries of the tridiagonal matrix.
- `subdiagonal`: Subdiagonal entries of the tridiagonal matrix.
*/
static void tridiagonalize_symmetric_3x3(double matrix[3][3], double eigenvectors[3][3], 
                                         double diagonal[3], double subdiagonal[2])
{
    const int size = 3;
    double householder_vector[size], temp_vector[size];
    double omega, scale, sigma, tau;
    
    // Initialize eigenvectors to the identity matrix
    for (int i = 0; i < size; i++) {
        eigenvectors[i][i] = 1.0;
        for (int j = 0; j < i; j++)
            eigenvectors[i][j] = eigenvectors[j][i] = 0.0;
    }

    // Compute the first Householder reflection
    scale = SQUARE(matrix[0][1]) + SQUARE(matrix[0][2]);
    sigma = (matrix[0][1] > 0) ? -sqrt(scale) : sqrt(scale);
    subdiagonal[0] = sigma;
    tau = sigma * matrix[0][1];
    householder_vector[1] = matrix[0][1] - sigma;
    householder_vector[2] = matrix[0][2];
    
    omega = scale - tau;
    if (omega > 0.0) {
        omega = 1.0 / omega;
        sigma = 0.0;
        for (int i = 1; i < size; i++) {
            tau = matrix[1][i] * householder_vector[1] + matrix[i][2] * householder_vector[2];
            temp_vector[i] = omega * tau;
            sigma += householder_vector[i] * tau;
        }
        sigma *= 0.5 * SQUARE(omega);

        for (int i = 1; i < size; i++)
            temp_vector[i] -= sigma * householder_vector[i];
        
        diagonal[0] = matrix[0][0];
        diagonal[1] = matrix[1][1] - 2.0 * temp_vector[1] * householder_vector[1];
        diagonal[2] = matrix[2][2] - 2.0 * temp_vector[2] * householder_vector[2];
        
        for (int j = 1; j < size; j++) {
            tau = omega * householder_vector[j];
            for (int i = 1; i < size; i++)
                eigenvectors[i][j] -= tau * householder_vector[i];
        }

        subdiagonal[1] = matrix[1][2] - temp_vector[1] * householder_vector[2] - householder_vector[1] * temp_vector[2];
    }
    else {
        for (int i = 0; i < size; i++)
            diagonal[i] = matrix[i][i];
        subdiagonal[1] = matrix[1][2];
    }
}

/**
### compute_eigensystem_symmetric_3x3()

Computes eigenvalues and eigenvectors of a symmetric 3x3 matrix using
the QL algorithm with implicit shifts.

#### Parameters

- `matrix`: Input symmetric matrix.
- `eigenvectors`: Eigenvector matrix (column-wise).
- `eigenvalues`: Output eigenvalues.

#### Returns

- `0` on success.
- `-1` if the iteration fails to converge.
*/
static int compute_eigensystem_symmetric_3x3(double matrix[3][3], double eigenvectors[3][3], double eigenvalues[3])
{
    const int size = 3;
    const int max_iterations = 30;
    double subdiagonal[3];
    double g, r, p, f, b, s, c, t;
    int iteration_count;
    int m;

    // Check for diagonal matrix with unit entries
    if (SQUARE(matrix[0][1]) < 1e-15 && SQUARE(matrix[0][2]) < 1e-15 && SQUARE(matrix[1][2]) < 1e-15) {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                eigenvectors[i][j] = (i == j) ? 1.0 : 0.0;
            }
            eigenvalues[i] = matrix[i][i];
        }
        return 0;
    }

    tridiagonalize_symmetric_3x3(matrix, eigenvectors, eigenvalues, subdiagonal);
    
    for (int l = 0; l < size - 1; l++) {
        iteration_count = 0;
        while (1) {
            for (m = l; m <= size - 2; m++) {
                g = fabs(eigenvalues[m]) + fabs(eigenvalues[m+1]);
                if (fabs(subdiagonal[m]) + g == g)
                    break;
            }
            if (m == l)
                break;
            
            if (iteration_count++ >= max_iterations)
                return -1;

            g = (eigenvalues[l+1] - eigenvalues[l]) / (2.0 * subdiagonal[l]);
            r = sqrt(SQUARE(g) + 1.0);
            g = eigenvalues[m] - eigenvalues[l] + subdiagonal[l] / (g + (g > 0 ? fabs(r) : -fabs(r)));

            s = c = 1.0;
            p = 0.0;
            for (int i = m - 1; i >= l; i--) {
                f = s * subdiagonal[i];
                b = c * subdiagonal[i];
                if (fabs(f) > fabs(g)) {
                    c = g / f;
                    r = sqrt(SQUARE(c) + 1.0);
                    subdiagonal[i+1] = f * r;
                    c *= (s = 1.0 / r);
                }
                else {
                    s = f / g;
                    r = sqrt(SQUARE(s) + 1.0);
                    subdiagonal[i+1] = g * r;
                    s *= (c = 1.0 / r);
                }
                
                g = eigenvalues[i+1] - p;
                r = (eigenvalues[i] - g) * s + 2.0 * c * b;
                p = s * r;
                eigenvalues[i+1] = g + p;
                g = c * r - b;

                for (int k = 0; k < size; k++) {
                    t = eigenvectors[k][i+1];
                    eigenvectors[k][i+1] = s * eigenvectors[k][i] + c * t;
                    eigenvectors[k][i]   = c * eigenvectors[k][i] - s * t;
                }
            }
            eigenvalues[l] -= p;
            subdiagonal[l] = g;
            subdiagonal[m] = 0.0;
        }
    }

    return 0;
}
