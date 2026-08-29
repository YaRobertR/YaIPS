/* **************************************************************************
/@
/@ Short-title: Linear equations system solver
/@
/@ ==========================================================================
/@
/@ Function names / Parameters for C call
/@ --------------------------------------
/@ int linequ (sys, n, vec, m)
/@ double *sys, *vec;                   (sys [n][n], vec [n][m])
/@ int n, m;
/@
/@ ==========================================================================
/@
/@ Description
/@ -----------
/@ Linear equation systems are solved by Gauss elimination followed by back
/@ substitution with linewise pivotisation.
/@
/@ The system matrix "sys [n rows] [n cols]" will be destroyed afterwards.
/@ The vector matrix "vec [n rows] [m cols]" must contain the m right side
/@ vectors; after the computation it contains the m solution vectors.
/@
/@ It is of big advantage to use the multi vector feature (m>1), if you have
/@ the same system matrix for different right side vectors.
/@
/@ Also, you can compute the inverse matrix by filling the unity matrix
/@ into the vector matrix (m=n).
/@
/@ If the determinant of the system matrix is 0, no single solution is
/@ possible and -2 is returned (matrices are destroyed).
/@ If n<2 or n>40 or m<1 or m>n, -1 is returned with matrices intact.
/@ At correct computation, 0 is returned.
/@
/@ Example 1:   #define N 10
/@  (simple)     :
/@              main ()
/@              {
/@              double sys [N][N], vec [N];
/@              register int i;
/@               :
/@              i = linequ (sys, N, vec, 1);
/@               :
/@
/@
/@ Example 2:   #define N 10
/@  (inversion)  :
/@              main ()
/@              {
/@              double sys [N][N], vec [N][N];
/@              register int i, k;
/@               :
/@              for (i=0; i<N; i++)
/@              for (k=0; k<N; k++) if (i == k) vec [i][k] = 1;
/@                                  else        vec [i][k] = 0;
/@               :
/@              i = linequ (sys, N, vec, N);
/@               :
/@
/@
/@ Modifications
/@ -------------
/@ V 1.00 : First edition released.
/@
/@ *************************************************************** @ 1985 **/


#include "matrix.h"             /* element access for open matrices */

/*-------------------------------------------------------------------------*/

#define NPV     40              /* length of pivot vector */
#define Z       (1e-30)         /* singularity threshold  */

/*=========================================================================*/

int linequ( double *sys,
            int n,
            double *vec,
            int m)
{
  int i, k, l, is, ip;
  double zp, r;
  int pv [NPV];

  if (n < 2 || n > NPV || m < 1 || m > n)
    return (-1); /* parameter error */

  for (i = 0; i < n; i++)
    pv[i] = i; /* initialize pivot vector */

  for (is = 0; is < n; is++) { /* Gauss elimination */

    ip = 0;    // Keep compiler silent (no warning about use of uninitialized variable)

    for (zp = 0, i = is; i < n; i++) { /* search pivot element */

      r = matrix(sys, n, pv[i], is);
      if (r < 0)
        r = -r;
      if (r > zp) {
        zp = r;
        ip = i;
      }
    }

    if (zp < Z)
      return (-2); /* singular system matrix */

    i = pv[is]; /* exchange pivot index */
    pv[is] = pv[ip];
    pv[ip] = i;

    r = 1 / matrix(sys, n, pv[is], is);

    for (k = is; k < n; k++)
      matrix (sys, n, pv[is], k) =
      matrix (sys, n, pv[is], k) * r;
    for (l = 0; l < m; l++)
      matrix (vec, m, pv[is], l) =
      matrix (vec, m, pv[is], l) * r;

    for (i = is + 1; i < n; i++) {
      r = matrix(sys, n, pv[i], is);

      for (k = is + 1; k < n; k++)
        matrix (sys, n, pv[i], k) =
        matrix (sys, n, pv[i], k) - r * matrix(sys, n, pv[is], k);
      for (l = 0; l < m; l++)
        matrix (vec, m, pv[i], l) =
        matrix (vec, m, pv[i], l) - r * matrix(vec, m, pv[is], l);
    }
  }

  for (i = n - 2; i >= 0; i--) { /* back substitution */

    for (l = 0; l < m; l++) { /* for each right side */

      r = matrix(vec, m, pv[i], l);
      for (k = i + 1; k < n; k++)
        r = r - matrix (sys, n, pv[i], k) * matrix(vec, m, pv[k], l);
      matrix (vec, m, pv[i], l) = r;
    }
  }

  for (i = 0; i < n; i++)
    for (l = 0; l < m; l++)
      matrix (sys, n, i, l) = matrix(vec, m, pv[i], l);
  for (i = 0; i < n; i++)
    for (l = 0; l < m; l++)
      matrix (vec, m, i, l) = matrix(sys, n, i, l);
  return (0);
}

/***************************************************************************/
