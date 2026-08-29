/* **************************************************************************
/@
/@ Name: matrix.h    Package: inc
/@
/@ Version: 1.00    Date: 22-JAN-87    Author: A.Urban   Company: SIGNUM GmbH
/@
/@ Language: C (cc)    Computer: QU68000 (Unix-System)
/@
/@ Short-title: Define element access for open matrices
/@
/@ ==========================================================================
/@
/@ Description
/@ -----------
/@ Possible in FORTRAN: SUBROUTINE EXAMPLE (ARRAY, NROW)
/@                      INTEGER ARRAY (NROW, *)
/@                        :
/@                      K = ARRAY (ROW, COL)
/@                      ARRAY (ROW, COL) = K
/@
/@
/@ Not possible in C:   example (array, ncol)
/@                      int array [] [ncol];
/@                        :
/@                      k = array [row] [col];
/@                      array [row] [col] = k;
/@
/@      because of syntax error "Int Constant required" in array declaration
/@                                   ========
/@
/@
/@ Help:                #define matrix(array,  ncol,  row,  col) \
/@                                  (*(array + ncol * row + col))
/@
/@                   or #include "/ssr/inc/matrix.h"
/@
/@                      example (array, ncol)
/@                      int array [];
/@                        :
/@                      k = matrix (array, ncol, row, col);
/@                      matrix (array, ncol, row, col) = k;
/@
/@ Array must be declared of the correct type!
/@
/@
/@ NOTE: FORTRAN arrays are organized in columns (fast row index, left),
/@ ====        C arrays are organized in rows    (fast col index, right).
/@
/@
/@ Modifications
/@ -------------
/@ V 1.00 : First edition released.
/@
/@ *************************************************************** @ 1985 **/

#ifndef MATRIX_H
#define MATRIX_H

#define matrix(array,  ncol,  row,  col)        \
	    (*(array + ncol * row + col))

#endif //  MATRIX_H

/***************************************************************************/
