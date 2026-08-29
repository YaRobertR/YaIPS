/* **************************************************************************
/@
/@ Short-title: computation of geometric transform system out of 3
/@              unidirectional passpoints
/@
/@ ==========================================================================
/@
/@ INDEX
/@
/@ # Mvuparcal    # vuparcal    # computation of geometric transform system
/@
/@ USER DESCRIPTION
/@.. vuparcal refvec testvec dstvec cxty
/@
/@.     computation of the geometric transform system x1 = A * x2 + b
/@.     using 3 unidirectional passpoint pairs
/@
/@      'refvec' contains the coordinates of 3 unidirectional passpoints 
/@      (2 X-position and 1 Y-position or 1 X-position and 2 Y-position)
/@      (reference) in the following form:
/@      xX0 yX0 xY0 yY0 xX1 yX1 xY1 yY1 
/@      with one of the 4 pairs set to -1/-1 (to define 3 points out of 4).
/@      'testvec' contains the x- and the y-coordinate of the test passpoints
/@      in the following form:
/@      xX0' yY0' xX1' yY1' with one coordinate set to -1. 
/@
/@      The transform system 
/@
/@      /  \   /       \   /  \   /  \     a11 a12  rotation matrix
/@      |xt|   |a11 a12|   |x |   |b1|     a21 a22
/@      |  | = |       | * |  | + |  |            T
/@      |yt|   |a21 a22|   |y |   |b2|     [b1 b2]  translation vector
/@      \  /   \       /   \  /   \  /
/@
/@      is computed out of the 3 equations
/@
/@      for 2 X- and 1 Y-passpoint:
/@
/@      xX0' = a11 * xX0 + a12 * yX0 + b1 
/@      yY'  = a21 * xY  + a22 * yY  + b2
/@      xX1' = a11 * xX1 + a12 * yX1 + b1
/@
/@      for 2 Y- and 1 X-passpoint:
/@
/@      xX'  = a11 * xX  + a12 * yX  + b1
/@      yY0' = a21 * xY0 + a22 * yY0 + b2
/@      yY1' = a21 * xY1 + a22 * yY1 + b2
/@
/@      and returned in 'dstvec'.
/@
/@      The following symmetries are defined:
/@      a11 = a22 (no scaling, Sx = Sy = 1)
/@      a21 = -cxty*cxty*a12 (no perspective)
/@
/@      with:
/@      a11 = cos (rotation angle)
/@      a12 = 1/cxty * sin(rotation angle)
/@      a21 = -cxty * sin(rotation angle)
/@      sin * sin + cos * cos = 1
/@
/@      'dstvec':
/@               index      value
/@                 0         a11
/@                 1         a12
/@                 2         a21
/@                 3         a22
/@                 4         b1
/@                 5         b2
/@
/@
/@      PARAMETERS
/@      refvec          (int32)  vector passpoint coordinates reference system
/@      testvec         (int32)  vector passpoint coordinates test system
/@      dstvec          (sfloat) vector with result
/@      cxty            (sfloat) x to y pixel relation of camera
/@
/@      RESTRICTIONS
/@
/@      All coordinates out of refvec and testvec have to be < 16384,
/@      no overflow check is done!!
/@
/@      SEE ALSO
/@
/@ FUNCTION DESCRIPTION
/@
/@     #include <math.h>
/@     #include "portab.h"
/@     #include "sip.h"
/@     #include "sipve.h"
/@     #include "tstpr.h"
/@
/@     int vuparcal(refvec,testvec,dstvec,cxty)
/@     Tvector *refvec;         vector ( int32 )
/@     Tvector *testvec;        vector ( int32 )
/@     Tvector *dstvec;         resultvector (sfloat )
/@     sfloat cxty;
/@
/@     Algorithm description:
/@
/@     (1) sin * sin + cos * cos = 1;
/@     (2) a = b * sin + c * cos + y;
/@     (3) d = e * sin + f * cos + y;
/@
/@     out of the equations (2) and (3) we get:
/@                          
/@     sin = (f - c)/(ce - fb) * y + (cd - fa)/(ce - fb) = A/N * y + B/N;
/@     cos = (b - e)/(ce - fb) * y + (ea - bd)/(ce - fb) = C/N * y + D/N;
/@
/@     inserted in equation (1) we get:
/@
/@     R y^2 + S y + T = 0 with the solution
/@
/@     y = (-S +/- sqrt(S^2 - 4RT))/2R) 
/@
/@     with: R = (A/N)^2 + (C/N)^2
/@           S = 2(A/N B/N + C/N D/N)
/@           T = (B/N)^2 + (D/N)^2 - 1
/@
/@     from the two solutions the one with cos > 0 (rotation -90...90)
/@     is the correct one. 
/@
/@ RETURN VALUES
/@
/@      All routines return 0 after successful execution.
/@      In case of error a negative value is returned.
/@
/@
/@ MODIFICATIONS
/@ V 1.00 : First edition released.
/@ V 1.01 : BUGFUX, for same strange point values, the sqrt argument
/@          was negative. --> Got a math error sqrt(): domain error
/@          Return error -16 (illegal passpoints, can't calculate
/@          transform system) in this case.
/@          ==> Aus 32 Bit Umgebung nachgezogen.
/@         
/@ *************************************************************************/

#include <windows.h>
#include <math.h>

#include "YaIPS_IPS_Interface.h" // 15.05.2025 RR: Need this for IPS defines
#include "YaIPS_LanguageStrings.h"  // Language string definitions

/*======================IP ROUTINE ========================================*/

int vuparcal( Tvector *refvec,   /* passpoints ref system              */
              Tvector *testvec,  /* passpoints test system             */
              Tvector *dstvec,   /* resultvector                       */
              float cxty)        /* x to y pixel relation of camera    */
{
  int ierr;

  register int32 *pr, *pt;
  register sfloat *pdst;                   /* pointer on dstvec           */
  register lfloat b1, b2;
  register lfloat sina, cosa;
  register lfloat A, B, C, D, N, R, S, T;
  register lfloat SqrtArgument;

  /*----------------------------------------------------------------------*/
  /* check input parameters                                               */
  /*----------------------------------------------------------------------*/
  if( (ierr = utvcheck( refvec, DV_HOST, TY_INT32)) != 0)  return(ierr);
  if( (ierr = utvcheck( testvec, DV_HOST, TY_INT32)) != 0) return(ierr);
  if( (ierr = utvcheck( dstvec, DV_HOST, TY_SFLOAT)) != 0) return(ierr);

  if (vgetnm(refvec) < 8) {
    errstring = (char *)"refvec too short";
    return(-10);
  }
  if (vgetnm(testvec) < 4) {
    errstring = (char *)"testvec too short";
    return(-11);
  }

  if (cxty < 0.1) {
    errstring = (char *)"cxty too small";
    return(-15);
  }

  pr = (int32 *)vgetpm(refvec);  
  pt = (int32 *)vgetpm(testvec);  
  if( (ierr = ve_alloc(dstvec,6L,(int16)sizeof(sfloat),TY_SFLOAT)) != 0) return(ierr);
  pdst = (sfloat *)vgetpm(dstvec);  

  if ((pr[0] != -1 && pr[1] != -1) && (pr[4] != -1 && pr[5] != -1) &&
       pt[0] != -1 && pt[2] != -1) {

    /* 2 * X-passpoint, 1 * Y-passpoint */
    N = (lfloat)pr[0] * (lfloat)pr[5]/cxty - 
             (lfloat)pr[4] * (lfloat)pr[1]/cxty;
    if (N < 1.0e-10 && N > -1.0e-10) {
      errstring = (char *)"illegal passpoints, can't calculate transform system";
      return(-16);
    }

    A = (lfloat)pr[4] - (lfloat)pr[0];
    B = (lfloat)pr[0] * (lfloat)pt[2] - (lfloat)pr[4] * (lfloat)pt[0];
    C = ((lfloat)pr[1] - (lfloat)pr[5])/cxty;
    D = ((lfloat)pr[5] * (lfloat)pt[0] -
        (lfloat)pt[2] * (lfloat)pr[1])/cxty;

    R = A * A + C * C;
    if (R < 1.0e-10 && R > -1.0e-10) {
      errstring = (char *)"illegal passpoints, can't calculate transform system";
      return(-16);
    }
    S = 2. * (A * B + C * D);
    T = B * B + D * D - N * N;

    // 16.08.2016 RR: Test for negative (or too small) argument for sqrt()
    SqrtArgument = S * S - 4. * R * T;
    if( SqrtArgument < 1.0e-10) {            // Is below reasonable value
      errstring = (char *)"illegal passpoints, can't calculate transform system";
      return(-16);
    }

    b1 = ((-S) + sqrt( SqrtArgument))/(2. * R);
    cosa = (C * b1 + D)/N;
    if (cosa < 0.) {
      b1 = ((-S) - sqrt( SqrtArgument))/(2. * R);
      cosa = (C * b1 + D)/N;
    }
    sina = (A * b1 + B)/N;
    if (pr[2] != -1 && pr[3] != -1 && pt[1] != -1) {
      b2 = (lfloat)pt[1] + cxty * (lfloat)pr[2] * sina - cosa * (lfloat)pr[3]; 
    } else if (pr[6] != -1 && pr[7] != -1 && pt[3] != -1) {
      b2 = (lfloat)pt[3] + cxty * (lfloat)pr[6] * sina - cosa * (lfloat)pr[7]; 
    } else {
      errstring = (char *)"not enough passpoints != -1";
      return(-13);
    }

  } else if ((pr[2] != -1 && pr[3] != -1) && (pr[6] != -1 && pr[7] != -1) &&
              pt[1] != -1 && pt[3] != -1) {

    /* 2 * Y-passpoint, 1 * X-passpoint */
    N = (lfloat)pr[7] * (lfloat)pr[2] * cxty -
             (lfloat)pr[3] * (lfloat)pr[6] * cxty;
    if (N < 1.0e-10 && N > -1.0e-10) {
      errstring = (char *)"illegal passpoints, can't calculate transform system";
      return(-16);
    }
    A = (lfloat)pr[7] - (lfloat)pr[3];
    B = (lfloat)pr[3] * (lfloat)pt[3] - (lfloat)pr[7] * (lfloat)pt[1];
    C = ((lfloat)pr[6] - (lfloat)pr[2]) * cxty;
    D = ((lfloat)pr[2] * (lfloat)pt[3] -
        (lfloat)pt[1] * (lfloat)pr[6]) * cxty;

    R = A * A + C * C;
    if (R < 1.0e-10 && R > -1.0e-10) {
      errstring = (char *)"illegal passpoints, can't calculate transform system";
      return(-16);
    }
    S = 2. * (A * B + C * D);
    T = B * B + D * D - N * N;

    // 16.08.2016 RR: Test for negative (or too small) argument for sqrt()
    SqrtArgument = S * S - 4. * R * T;
    if( SqrtArgument < 1.0e-10) {            // Is below reasonable value
      errstring = (char *)"illegal passpoints, can't calculate transform system";
      return(-16);
    }

    b2 = ((-S) + sqrt( SqrtArgument))/(2. * R);
    cosa = (C * b2 + D)/N;
    if (cosa < 0.) {
      b2 = ((-S) - sqrt( SqrtArgument))/(2. * R);
      cosa = (C * b2 + D)/N;
    }
    sina = (A * b2 + B)/N;

    if (pr[0] != -1 && pr[1] != -1 && pt[0] != -1) {
      b1 = (lfloat)pt[0] - ((lfloat)pr[1] * sina)/cxty - cosa * (lfloat)pr[0]; 
    } else if (pr[4] != -1 && pr[5] != -1 && pt[2] != -1) {
      b1 = (lfloat)pt[2] - ((lfloat)pr[5] * sina)/cxty - cosa * (lfloat)pr[4]; 
    } else {
      errstring = (char *)"not enough passpoints != -1";
      return(-14);
    }
  } else {
    errstring = (char *)"not enough passpoints != -1";
    return(-12);
  }

  *pdst++ = (sfloat)cosa;                  /* a11 */
  *pdst++ = (sfloat)sina/cxty;             /* a12 */
  *pdst++ = (- cxty) * (sfloat)sina;       /* a21 */
  *pdst++ = (sfloat)cosa;                  /* a22 */
  *pdst++ = (sfloat)b1;                    /* b1  */
  *pdst   = (sfloat)b2;                    /* b2  */
  
  vputnm(dstvec,6);
  return(0);

} /* int vuparcal() */

/************************************* E O F ********************************/
