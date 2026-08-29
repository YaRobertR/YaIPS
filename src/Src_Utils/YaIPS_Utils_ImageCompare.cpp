/****************************************************************************

  YaIPS_Utils_ImageCompare.cpp

  Image compare.

 16.09.2025 RR: First edition of this file.

*****************************************************************************
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <sysinfoapi.h>

#include <iostream>
//x/using namespace std;
#include "YaIPS.h"

/* ======================================================================= */

static int MaxNumCompareErrors = 100000;   // Compare window, max number of error pixels, range 1000 .. 999999


/* ======================================================================= */

// syserror codes

#define MY_SYSERR_BASE     (12 * 100)

#define SE_NO_WORK_DATA   (MY_SYSERR_BASE+0)     // No work data allocated
#define SE_VCRE           (MY_SYSERR_BASE+1)     // failed to create vector
#define SE_VALLOC         (MY_SYSERR_BASE+2)     // error %d allocating vector
#define SE_MEMALLOC       (MY_SYSERR_BASE+3)     // memory allocation failed
#define SE_RGB_2_IMGD     (MY_SYSERR_BASE+4)     // Error YaIPS_RGB_to_ImgD()
#define SE_VREM           (MY_SYSERR_BASE+5)     // error %d removing vector
#define SE_COMPAREP1      (MY_SYSERR_BASE+6)     // error %d in compareTPass1_bw()
#define SE_COMPARELAB     (MY_SYSERR_BASE+7)     // error %d in compareTrlt_labmea()
#define SE_COMPARESTAT    (MY_SYSERR_BASE+8)     // error %d in compareTStatistics()
#define SE_COMPAREDRAW    (MY_SYSERR_BASE+9)     // error %d in compareTDraw()

/* ---------------------- inspection error codes ------------------------- */

#define PPCWINC0_INSPERR_OK        0       // window OK
#define PPCWINC0_INSPERR_WINBORDER 1       // touches image border
#define PPCWINC0_INSPERR_BAREA     2       // blob area tolerance error
#define PPCWINC0_INSPERR_LBLOBS    3       // large blobs error
#define PPCWINC0_INSPERR_TOOMANY   4       // too many errors

/* ---------------------- structures ------------------------------------- */

typedef struct {
  Tvector *vRLC;              // compare RLC
  Tvector *vRowIdxTab;        // compare row start index table into vRLC
  Tvector *vCompareObj;       // compare error objects
  Tvector *vLabWork;          // work vector for labler

  int lastActive;             // last active flag

  Fl_YaIPS_AOI_t lastWinDesc; // position and size in last inspection run
} Tsip_winC0;

/************************************************************************************
* At some other necessary things
*
*/

// processing window flags

//x/#define PROJPAR_WINFLAG_FIXED           0x00000001   // fixed window, not movable
//x/#define PROJPAR_WINFLAG_ACTIVE          0x00000002   // active window
//x/#define PROJPAR_WINFLAG_ISDELETED       0x00000004   // window is deleted
//x/#define PROJPAR_WINFLAG_NDEL            0x00000008   // undeletable window
//x/#define PROJPAR_WINFLAG_FIXED2MAX       0x00000010   // fixed window, not movable, fixed to maximum (ref) size
#define PROJPAR_WINFLAG_XNACTIVE        0x00000020   // x-processing NOT active
#define PROJPAR_WINFLAG_YNACTIVE        0x00000040   // y-processing NOT active

// Some debugging stuff

#define TSTPR_LEVEL_SEVERE_ERROR  1
#define TSTPR_LEVEL_ERROR         2
#define TSTPR_LEVEL_WARNING       4
#define TSTPR_LEVEL_INFO          8
#define TSTPR_LEVEL_DEBUG        10

#define PRINTL0(l,p0)
#define PRINTL1(l,p0,p1)
#define PRINTL2(l,p0,p1,p2)
#define PRINTL3(l,p0,p1,p2,p3)
#define PRINTL4(l,p0,p1,p2,p3,p4)
#define PRINTL5(l,p0,p1,p2,p3,p4,p5)

/************************************************************************************
* Compare defines
*
*/

#define FSV_RED   0
#define FSV_GREEN 1
#define FSV_BLUE  2

/* -------------------------------- RLC definitions ---------------------- */

typedef struct {
  int32 rlxa;
  int32 rlxe;
  int32 rlab;
  int32 rlid;
  int32 numLightErrPix;
  int32 numDarkErrPix;
  int8 meanRefVal[3];
  int8 meanContrDev[3];
  int16 alignment_dummy;
  int32 rly;              /* row */
} T_compareT_RLC;

/* ------------------------------- RLC labler definitions ---------------- */

/* working/free list of rlt_labmea */
typedef struct {
  int32 xmin;
  int32 xmax;
  int32 ymin;
  int32 ymax;
  int32 area;
  int32 sumRefVal[3];
  int32 sumContrDev[3];
  int32 numLightErrPix;
  int32 numDarkErrPix;
  int32 next; /* index of next element */
  int32 prev; /* index of previous element, only used in working list */
  int32 label;
} Tobj;

/* destination list of rlt_labmea */
typedef struct {
  int32 xmin;
  int32 xmax;
  int32 ymin;
  int32 ymax;
  int32 area;
  int32 label;
  int32 relContrDev[3];
  int32 numLightErrPix;
  int32 numDarkErrPix;
} T_compareT_Obj;

#define RLCONNECTIVITY_4 0
#define RLCONNECTIVITY_8 1

/* flags */
#define RLVWRAPAROUND        0x0001
#define RLENTERLABELS        0x0002
#define RLDONTSTORE          0x0004
#define RLSEPARATELIGHTDARK  0x0008
#define RLENDLESS_TOP        0x0010
#define RLENDLESS_BOTTOM     0x0020
#define RLENDLESS            0x0030
#define RLENDLESS_LEFT       0x0040
#define RLENDLESS_RIGHT      0x0080
#define RLENDLESS_ALL        0x00f0

/* error codes */
#define RLTLM_OBJECT_LIMIT  -99    /* labler reached object limit */

/************************************************************************************
* Compare Pass 1
*
*/

static int compareTPass1( YaIPS_RGB_ImgD_t *pInputAOI,
                          YaIPS_RGB_ImgD_t *pRefAOI,
                          Tvector *vRLC, Tvector *vRowIdxTab, int GWToleranceLo, int GWToleranceHi,
                          int *numLightErrPix, int *numDarkErrPix, int *numErrPix,
                          int maxNumErrPix)
{
  uchar  *pi, *pi2, *pr, *pr2, *pis, *prs;
  int    help, minmax;
  int    x, y, i, len;
  int    xxd, yyd;                            /* actual sizes */
  int xmr, xmi;
  T_compareT_RLC *pRLC;
  int32          *pRowIdx;
  int32            numRLC, numDarkErr, numLightErr;
  int32    sumDarkErr, sumLightErr;
  int32    sumRefVal[3], sumContrDev[3];
  int32    firstRefVal[3], firstContrDev[3];
  int32    lastRefVal[3], lastContrDev[3];
  int iColor, nColors, RefHasNoAlpha;
  int IOff00, IOff01, IOff02, IOff10, IOff11, IOff12, IOff20, IOff21, IOff22;
  int ROff00, ROff01, ROff02, ROff10, ROff11, ROff12, ROff20, ROff21, ROff22;


  if( (pInputAOI->d >= 3) != (pRefAOI->d >= 3)) {           // BW or color must be equal

    return( -1);
  }

  if( pInputAOI->xx != pRefAOI->xx ||                   // Must have same size
      pInputAOI->yy != pRefAOI->yy) {

    return( -2);
  }

  if (utvcheck(vRLC,       DV_HOST, TY_INT8))   return(-11);
  if (utvcheck(vRowIdxTab, DV_HOST, TY_INT32))  return(-12);

  pRLC     = (T_compareT_RLC *)vgetpm(vRLC);
  pRowIdx  = (int32 *)vgetpm(vRowIdxTab);

  pis = RGB_pixad( 1, 1, pInputAOI);
  prs = RGB_pixad( 1, 1, pRefAOI);

  xxd = pRefAOI->xx - 1L;
  yyd = pRefAOI->yy - 1L;
  xmr = pRefAOI->ld;
  xmi = pInputAOI->ld;

  IOff10 = - pInputAOI->d;
  IOff11 = 0;
  IOff12 = pInputAOI->d;
  IOff00 = IOff10 - xmi;
  IOff01 = IOff11 - xmi;
  IOff02 = IOff12 - xmi;
  IOff20 = IOff10 + xmi;
  IOff21 = IOff11 + xmi;
  IOff22 = IOff12 + xmi;

  ROff10 = - pRefAOI->d;
  ROff11 = 0;
  ROff12 = pRefAOI->d;
  ROff00 = ROff10 - xmr;
  ROff01 = ROff11 - xmr;
  ROff02 = ROff12 - xmr;
  ROff20 = ROff10 + xmr;
  ROff21 = ROff11 + xmr;
  ROff22 = ROff12 + xmr;

  nColors       = pRefAOI->d >= 3 ? 3 : 1;
  RefHasNoAlpha = pRefAOI->d != 2 && pRefAOI->d != 4;

  sumDarkErr = 0;
  sumLightErr = 0;

  numRLC = 0;
  *pRowIdx = numRLC;           /* we begin with y = 1 */
  pRowIdx++;

  for( y = 1; y < yyd; y++, pRowIdx++) {

    *pRowIdx = -1;       /* initialize no RLC for this row */
    numDarkErr = 0;
    numLightErr = 0;

    pr = prs;
    prs += xmr;         /* next row */

    pi = pis;
    pis += xmi;         /* next row */

    for( x = 1L; x < xxd; x++, pr += pRefAOI->d, pi += pInputAOI->d)  {

      if( RefHasNoAlpha ||                               // No alpha mask
          (pr[ nColors] & YAIPS_REFMASK_CONTOUR) != 0) { // Contour bit is set

        /* Area */

        for( iColor = 0; iColor < nColors; iColor++) {

          help = (int)pi[ iColor];

          if( help < (int)pr[ iColor] - GWToleranceLo) {        /* in darker as ref - tolerance */
            numDarkErr++;
            goto CompareError;
          } else if (help > (int)pr[ iColor] + GWToleranceHi) { /* in lighter than ref + tolerance*/
            numLightErr++;
            goto CompareError;
          }
        }  // End for( iColor ...

      } else {

        /* Contour */

        pr2 = pr;
        pi2 = pi;

        for( iColor = 0; iColor < nColors; iColor++, pr2++, pi2++) {

          help = (int)*pi2;

          if( help < (int)*pr2 - GWToleranceLo) {        /* in darker as ref - tolerance */

            /* get max of input neighborhood */
            minmax = help;    // Is IOff11
            help = pi2[ IOff10]; if( help > minmax) minmax = help;
            help = pi2[ IOff12]; if( help > minmax) minmax = help;
            help = pi2[ IOff00]; if( help > minmax) minmax = help;
            help = pi2[ IOff01]; if( help > minmax) minmax = help;
            help = pi2[ IOff02]; if( help > minmax) minmax = help;
            help = pi2[ IOff20]; if( help > minmax) minmax = help;
            help = pi2[ IOff21]; if( help > minmax) minmax = help;
            help = pi2[ IOff22]; if( help > minmax) minmax = help;
            if (minmax < (int)*pr2 - GWToleranceLo) {
              numDarkErr++;
              goto CompareError;
            }

            /* get min of ref neighborhood */
            minmax = (int)*pr2;    // Is ROff11
            help = pr2[ ROff10]; if( help < minmax) minmax = help;
            help = pr2[ ROff12]; if( help < minmax) minmax = help;
            help = pr2[ ROff00]; if( help < minmax) minmax = help;
            help = pr2[ ROff01]; if( help < minmax) minmax = help;
            help = pr2[ ROff02]; if( help < minmax) minmax = help;
            help = pr2[ ROff20]; if( help < minmax) minmax = help;
            help = pr2[ ROff21]; if( help < minmax) minmax = help;
            help = pr2[ ROff22]; if( help < minmax) minmax = help;
            if( (int)*pi2 < minmax  - GWToleranceLo) {
              numDarkErr++;
              goto CompareError;
            }

          } else if (help > (int)*pr2 + GWToleranceHi) { /* in lighter than ref + tolerance*/

            /* get min of input neighborhood */
            minmax = help;    // Is IOff11
            help = pi2[ IOff10]; if( help < minmax) minmax = help;
            help = pi2[ IOff12]; if( help < minmax) minmax = help;
            help = pi2[ IOff00]; if( help < minmax) minmax = help;
            help = pi2[ IOff01]; if( help < minmax) minmax = help;
            help = pi2[ IOff02]; if( help < minmax) minmax = help;
            help = pi2[ IOff20]; if( help < minmax) minmax = help;
            help = pi2[ IOff21]; if( help < minmax) minmax = help;
            help = pi2[ IOff22]; if( help < minmax) minmax = help;
            if (minmax > (int)*pr2 + GWToleranceHi) {
              numLightErr++;
              goto CompareError;
            }

            /* get max of ref neighborhood */
            minmax = (int)*pr2;    // Is ROff11
            help = pr2[ ROff10]; if( help > minmax) minmax = help;
            help = pr2[ ROff12]; if( help > minmax) minmax = help;
            help = pr2[ ROff00]; if( help > minmax) minmax = help;
            help = pr2[ ROff01]; if( help > minmax) minmax = help;
            help = pr2[ ROff02]; if( help > minmax) minmax = help;
            help = pr2[ ROff20]; if( help > minmax) minmax = help;
            help = pr2[ ROff21]; if( help > minmax) minmax = help;
            help = pr2[ ROff22]; if( help > minmax) minmax = help;
            if( (int)*pi2 > minmax + GWToleranceLo) {
              numLightErr++;
              goto CompareError;
            }
          }
        } // end for( iColor, ...

      } /* end of switch (pr->ol & maskPixClass) */

      // If we come to here, there is no compare error

      continue;   // Continue for( x ...

      /* ---------------------- error case ------------------------- */

CompareError:

      if (*numErrPix > maxNumErrPix) {      /* too many errors abort */

        PRINTL1(TRACE_DEBUG,
                "compareTPass1_rgb: too many errors (%d RLCs)\n", numRLC);

        if (*pRowIdx == -1) {           /* no RLC written for this row */

          *pRowIdx = numRLC;

        } else {                        /* end last RLC */

          sumDarkErr  += pRLC->numDarkErrPix;
          sumLightErr += pRLC->numLightErrPix;

          len = pRLC->rlxe - pRLC->rlxa + 1;
          if (len < 3) {
            for (i = 0; i < 3; i++) {
              pRLC->meanRefVal[i]   = (sumRefVal[i]   / len) & 0xff;
              pRLC->meanContrDev[i] = (sumContrDev[i] / len) & 0xff;
            }
          } else {
            len -= 2;
            for (i = 0; i < 3; i++) {
              sumRefVal[i]   = sumRefVal[i]   - firstRefVal[i]   -
                               lastRefVal[i];
              sumContrDev[i] = sumContrDev[i] - firstContrDev[i] -
                               lastContrDev[i];
              pRLC->meanRefVal[i]   = (sumRefVal[i]   / len) & 0xff;
              pRLC->meanContrDev[i] = (sumContrDev[i] / len) & 0xff;
            }
          }

          pRLC++;
          numRLC++;
        }

        pRowIdx++; /* for y-row pRowIdx was written surely */
        y++;

        goto abort_loop;
      }

      *numErrPix = *numErrPix + 1;

      if (*pRowIdx == -1) {  /* start of a new Row */

        *pRowIdx = numRLC;

        pRLC->rly                = y;
        pRLC->rlxa               = x;
        pRLC->rlxe               = x;
        pRLC->numLightErrPix     = numLightErr;
        pRLC->numDarkErrPix      = numDarkErr;
        pRLC->rlab               = 0;            /* write dummy label */
        pRLC->rlid               = 0;            /* write dummy ID */
        pRLC->rlxa               = x;

        firstRefVal[FSV_RED]     = btoi(pr[ 0]);
        firstContrDev[FSV_RED]   = abs(btoi(pr[ 0]) - btoi(pi[ 0]));
        sumRefVal[FSV_RED]       = firstRefVal[FSV_RED];
        sumContrDev[FSV_RED]     = firstContrDev[FSV_RED];

        if( nColors == 3) {

          firstRefVal[FSV_GREEN]   = btoi(pr[ 1]);
          firstRefVal[FSV_BLUE]    = btoi(pr[ 2]);
          firstContrDev[FSV_GREEN] = abs(btoi(pr[ 1]) - btoi(pi[ 1]));
          firstContrDev[FSV_BLUE]  = abs(btoi(pr[ 2]) - btoi(pi[ 2]));

          sumRefVal[FSV_GREEN]     = firstRefVal[FSV_GREEN];
          sumContrDev[FSV_GREEN]   = firstContrDev[FSV_GREEN];
          sumRefVal[FSV_RED]       = firstRefVal[FSV_RED];
          sumContrDev[FSV_BLUE]    = firstContrDev[FSV_BLUE];
        }

      } else {

        if (x > (pRLC->rlxe + 1)) {  /* end last RLC, start new RLC */

          /* end RLC */

          /* we do not know about the actual error type */
          numDarkErr  -= pRLC->numDarkErrPix;
          numLightErr -= pRLC->numLightErrPix;
          sumDarkErr  += pRLC->numDarkErrPix;
          sumLightErr += pRLC->numLightErrPix;

          len = pRLC->rlxe - pRLC->rlxa + 1;
          if (len < 3) {
            for (i = 0; i < 3; i++) {
              pRLC->meanRefVal[i]   = (sumRefVal[i]   / len) & 0xff;
              pRLC->meanContrDev[i] = (sumContrDev[i] / len) & 0xff;
            }
          } else {
            len -= 2;
            for (i = 0; i < 3; i++) {
              sumRefVal[i]   = sumRefVal[i]   - firstRefVal[i]   -
                               lastRefVal[i];
              sumContrDev[i] = sumContrDev[i] - firstContrDev[i] -
                               lastContrDev[i];
              pRLC->meanRefVal[i]   = (sumRefVal[i]   / len) & 0xff;
              pRLC->meanContrDev[i] = (sumContrDev[i] / len) & 0xff;
            }
          }

          pRLC++;
          numRLC++;

          /* start the new RLC */

          pRLC->rly              = y;
          pRLC->rlxa             = x;
          pRLC->rlxe             = x;
          pRLC->numLightErrPix   = numLightErr;
          pRLC->numDarkErrPix    = numDarkErr;
          pRLC->rlab             = 0;          /* write dummy label */
          pRLC->rlid             = 0;          /* write dummy ID */

          firstRefVal[FSV_RED]     = btoi(pr[ 0]);
          firstContrDev[FSV_RED]   = abs(btoi(pr[ 0]) - btoi(pi[ 0]));
          sumRefVal[FSV_RED]       = firstRefVal[FSV_RED];
          sumContrDev[FSV_RED]     = firstContrDev[FSV_RED];

          if( nColors == 3) {

            firstRefVal[FSV_GREEN]   = btoi(pr[ 1]);
            firstRefVal[FSV_BLUE]    = btoi(pr[ 2]);
            firstContrDev[FSV_GREEN] = abs(btoi(pr[ 1]) - btoi(pi[ 1]));
            firstContrDev[FSV_BLUE]  = abs(btoi(pr[ 2]) - btoi(pi[ 2]));

            sumRefVal[FSV_GREEN]     = firstRefVal[FSV_GREEN];
            sumContrDev[FSV_GREEN]   = firstContrDev[FSV_GREEN];
            sumRefVal[FSV_RED]       = firstRefVal[FSV_RED];
            sumContrDev[FSV_BLUE]    = firstContrDev[FSV_BLUE];
          }

        } else {             /* update running RLC */

          pRLC->numLightErrPix    = numLightErr;
          pRLC->numDarkErrPix     = numDarkErr;
          pRLC->rlxe              = x;

          lastRefVal[FSV_RED]     = btoi(pr[ 0]);
          lastContrDev[FSV_RED]   = abs(btoi(pr[ 0]) - btoi(pi[ 0]));
          sumRefVal[FSV_RED]      += lastRefVal[FSV_RED];
          sumContrDev[FSV_RED]    += lastContrDev[FSV_RED];

          if( nColors == 3) {

            lastRefVal[FSV_GREEN]   = btoi(pr[ 1]);
            lastRefVal[FSV_BLUE]    = btoi(pr[ 2]);
            lastContrDev[FSV_GREEN] = abs(btoi(pr[ 1]) - btoi(pi[ 1]));
            lastContrDev[FSV_BLUE]  = abs(btoi(pr[ 2]) - btoi(pi[ 2]));

            sumRefVal[FSV_GREEN]    += lastRefVal[FSV_GREEN];
            sumContrDev[FSV_GREEN]  += lastContrDev[FSV_GREEN];
            sumRefVal[FSV_BLUE]     += lastRefVal[FSV_BLUE];
            sumContrDev[FSV_BLUE]   += lastContrDev[FSV_BLUE];
          }
        }
      }
    } /* end of x-loop */

    if (*pRowIdx == -1) {           /* no RLC written for this row */

      *pRowIdx = numRLC;

    } else {                        /* end last RLC */

      sumDarkErr  += pRLC->numDarkErrPix;
      sumLightErr += pRLC->numLightErrPix;

      len = pRLC->rlxe - pRLC->rlxa + 1;
      if (len < 3) {
        for (i = 0; i < 3; i++) {
          pRLC->meanRefVal[i]   = (sumRefVal[i]   / len) & 0xff;
          pRLC->meanContrDev[i] = (sumContrDev[i] / len) & 0xff;
        }
      } else {
        len -= 2;
        for (i = 0; i < 3; i++) {
          sumRefVal[i]   = sumRefVal[i]   - firstRefVal[i]   -
                           lastRefVal[i];
          sumContrDev[i] = sumContrDev[i] - firstContrDev[i] -
                           lastContrDev[i];
          pRLC->meanRefVal[i]   = (sumRefVal[i]   / len) & 0xff;
          pRLC->meanContrDev[i] = (sumContrDev[i] / len) & 0xff;
        }
      }

      pRLC++;
      numRLC++;
    }

  } /* end of y-loop */

abort_loop:

  *numLightErrPix = sumLightErr;
  *numDarkErrPix  = sumDarkErr;

  /* fill up RowIdx vector up to the end */
  for ( ; y < pRefAOI->yy; y++) {
    *pRowIdx = numRLC;            /* we end with y = yy - 1 */
    pRowIdx++;
  }

  /* store one dummy code for last row end recognition */
  pRLC->rly                     = MAXINT32;
  pRLC->rlxa                    = 0;
  pRLC->rlxe                    = 0;
  pRLC->numLightErrPix          = 0;
  pRLC->numDarkErrPix           = 0;
  pRLC->rlab                    = 0;
  pRLC->rlid                    = 0;
  pRLC->meanRefVal[FSV_RED]     = 0;
  pRLC->meanRefVal[FSV_GREEN]   = 0;
  pRLC->meanRefVal[FSV_BLUE]    = 0;
  pRLC->meanContrDev[FSV_RED]   = 0;
  pRLC->meanContrDev[FSV_GREEN] = 0;
  pRLC->meanContrDev[FSV_BLUE]  = 0;
  pRLC++;
  numRLC++;

  vputnm(vRLC, (int32)(numRLC * sizeof(T_compareT_RLC)));
  vputnm( vRowIdxTab, pRefAOI->yy);

  return(0);

}

/* **************************************************************************
 *
 *  CompareT labler
 *
 ************************************************************************** */

#define NO_ELEMENT    -1
#define NOLABEL 0x0000    /* Sign Bit !! negative if set ! */

typedef struct {
  int32 first;
  int32 firstempty;
  int32 max_objects;
  int32 num_objects;
  int32 num_underlimit;
  Tobj *pwork; /* work pointer (Object/empty list) */
  int32 label;
  int32 *pRowIdx;
  int32 xx;
  int32 yy;
} Trlt_labmea_desc;

static Trlt_labmea_desc rltlmd = {
  NO_ELEMENT,
  NO_ELEMENT,
  0,
  0,
  0,
  (Tobj *)0,
  0,
  (int32 *)0,
  0,
  0
};

static void init_work_list() /* init work list
============================ */
{
  register int32 i;
  register Tobj *p;

  /* don't use element 0 (conflicts with NOLABEL) */
  rltlmd.firstempty = 1;
  rltlmd.first = NO_ELEMENT;
  p = rltlmd.pwork + 1;

  /* init empty list */
  for (i = 1; i < rltlmd.max_objects; i++) {
    (p++)->next = (i + 1);
  }

  /* init last element of empty list */
  p->next = NO_ELEMENT;

  return;
}

static void meas_init(int32 act, int32 xa, int32 xe, int32 ypos, int32 label,
                      int32 nLErr, int32 nDErr, int8 refVal[], int8 contrDev[]) /* init measurement
============================================================================= */
{
  register Tobj *pw;
  register int32 len;

  pw = rltlmd.pwork + act;

  len = xe - xa + 1;

  pw->xmin = xa;
  pw->xmax = xe;
  pw->ymin = ypos;
  pw->ymax = ypos;
  pw->area = len;
  pw->label = label;
  pw->numLightErrPix = nLErr;
  pw->numDarkErrPix  = nDErr;

  if (len >= 3) len -= 2;
  pw->sumRefVal[FSV_RED]     = btoi(refVal[FSV_RED])     * len;
  pw->sumRefVal[FSV_GREEN]   = btoi(refVal[FSV_GREEN])   * len;
  pw->sumRefVal[FSV_BLUE]    = btoi(refVal[FSV_BLUE])    * len;
  pw->sumContrDev[FSV_RED]   = btoi(contrDev[FSV_RED])   * len;
  pw->sumContrDev[FSV_GREEN] = btoi(contrDev[FSV_GREEN]) * len;
  pw->sumContrDev[FSV_BLUE]  = btoi(contrDev[FSV_BLUE])  * len;

  return;
}

static void meas_update(int32 act, int32 xa, int32 xe, int32 ypos,
                        int32 nLErr, int32 nDErr, int8 refVal[], int8 contrDev[]) /* update measurement
================================================================================ */
{
  register Tobj *pw;
  register int32 len;

  pw = rltlmd.pwork + act;

  len = xe - xa + 1;

  /* PRINTF1("meas_update %d\n", act); */
  if (pw->xmin > xa) pw->xmin = xa;
  if (pw->xmax < xe) pw->xmax = xe;
  pw->ymax = ypos;
  pw->area += len;
  pw->numLightErrPix += (int32)nLErr;
  pw->numDarkErrPix  += (int32)nDErr;

  if (len >= 3) len -= 2;
  pw->sumRefVal[FSV_RED]     += btoi(refVal[FSV_RED])     * len;
  pw->sumRefVal[FSV_GREEN]   += btoi(refVal[FSV_GREEN])   * len;
  pw->sumRefVal[FSV_BLUE]    += btoi(refVal[FSV_BLUE])    * len;
  pw->sumContrDev[FSV_RED]   += btoi(contrDev[FSV_RED])   * len;
  pw->sumContrDev[FSV_GREEN] += btoi(contrDev[FSV_GREEN]) * len;
  pw->sumContrDev[FSV_BLUE]  += btoi(contrDev[FSV_BLUE])  * len;

  return;
}

//#define DEBUG_TRACE_MEAS_REUNION 1    // define this to have debug prints in meas_reunion()

static void meas_reunion(int32 inew, int32 iold, int flags, int32 yy) /* object reunion
===================================================================== */
{
  register Tobj *pw, *pwo;
#ifdef DEBUG_TRACE_MEAS_REUNION
  int WrapAroundReunion = FALSE;
#endif

  pw = rltlmd.pwork + inew;
  pwo = rltlmd.pwork + iold;

  /* put old together with new -> new */

  if (pw->xmin > pwo->xmin) pw->xmin = pwo->xmin;
  if (pw->xmax < pwo->xmax) pw->xmax = pwo->xmax;
  if (flags & RLVWRAPAROUND) {
    if ((pw->ymin == 0 && pw->ymax == yy -1) ||       // one of both coves complete Y-Range
        (pwo->ymin == 0 && pwo->ymax == yy -1)) {

#ifdef DEBUG_TRACE_MEAS_REUNION
      WrapAroundReunion = TRUE;    // for printout later
      PRINTL2(TRACE_INFO, "reunion: new %d .. %d\n", pw->ymin, pw->ymax);
      PRINTL2(TRACE_INFO, " YY-all  old %d .. %d\n", pwo->ymin, pwo->ymax);
#endif

      pw->ymin = 0;
      pw->ymax = yy - 1;

#ifdef DEBUG_TRACE_MEAS_REUNION
      PRINTL2(TRACE_INFO, "         YY-all new %d .. %d\n", pw->ymin, pw->ymax);
#endif
    } else if (pw->ymin == 0) {   /* new = pure top row object */

#ifdef DEBUG_TRACE_MEAS_REUNION
      WrapAroundReunion = TRUE;    // for printout later
      PRINTL3(TRACE_INFO, "reunion: new %d .. %d, l = %d\n", pw->ymin, pw->ymax, pw->ymax - pw->ymin + 1);
      PRINTL3(TRACE_INFO, "  New 0  old %d .. %d, l = %d\n", pwo->ymin, pwo->ymax, pwo->ymax - pwo->ymin + 1);
#endif

      pw->ymin += yy;
      pw->ymax += yy;

#ifdef DEBUG_TRACE_MEAS_REUNION
      PRINTL3(TRACE_INFO, "         + yy to new %d .. %d, l = %d\n", pw->ymin, pw->ymax, pw->ymax - pw->ymin + 1);
#endif

      if (pw->ymin > pwo->ymin) pw->ymin = pwo->ymin;
      if (pw->ymax < pwo->ymax) pw->ymax = pwo->ymax;
    } else {

      // std case
      if (pw->ymin > pwo->ymin) pw->ymin = pwo->ymin;
      if (pw->ymax < pwo->ymax) pw->ymax = pwo->ymax;
    }
  } else {
    // no RLVWRAPAROUND
    if (pw->ymin > pwo->ymin) pw->ymin = pwo->ymin;
    if (pw->ymax < pwo->ymax) pw->ymax = pwo->ymax;
  }

#ifdef DEBUG_TRACE_MEAS_REUNION
  if( WrapAroundReunion) {
    PRINTL3(TRACE_INFO, "         updated new %d .. %d, l = %d\n", pw->ymin, pw->ymax, pw->ymax - pw->ymin + 1);
  }
#endif

  pw->area += pwo->area;

  pw->numLightErrPix += pwo->numLightErrPix;
  pw->numDarkErrPix  += pwo->numDarkErrPix;

  pw->sumRefVal[FSV_RED]   += pwo->sumRefVal[FSV_RED];
  pw->sumRefVal[FSV_GREEN] += pwo->sumRefVal[FSV_GREEN];
  pw->sumRefVal[FSV_BLUE]  += pwo->sumRefVal[FSV_BLUE];

  pw->sumContrDev[FSV_RED]   += pwo->sumContrDev[FSV_RED];
  pw->sumContrDev[FSV_GREEN] += pwo->sumContrDev[FSV_GREEN];
  pw->sumContrDev[FSV_BLUE]  += pwo->sumContrDev[FSV_BLUE];

  return;
}

static void rlc_reunion(int32 newlab, int32 newid, int32 oldlab, int32 y, Tvector *rlc) /*
======================================================================================= */
{
  register T_compareT_RLC *p;

  if (y < 0 || y >= rltlmd.yy) {
    /* PRINTF1("WARNING: rlc_reunion(): y = %d\n", y); */
    return;
  }

  p = (T_compareT_RLC *)vgetpm(rlc) + rltlmd.pRowIdx[y];

  while((int32)p->rly == y) {
    if (p->rlab == oldlab) {
      p->rlid = newid;
      p->rlab = newlab;
    }
    p++;
  }

  return;
}

static void rlc_reunionAll(int32 newlab, int32 newid, int32 old, int32 oldlab, int32 ymax, Tvector *rlc) /*
======================================================================================================== */
{
  register Tobj *pwo;
  register T_compareT_RLC *p;
  register int32 y;

  if (ymax < 0 || ymax >= rltlmd.yy) {
#ifdef _PPC_
    PRINTF1("WARNING: rlc_reunion(): ymax = %d\n", ymax);
#else
    PRINTL1(TRACE_DEBUG, "WARNING: rlc_reunion(): ymax = %d\n", ymax);
#endif
    return;
  }

  if (old < 0) {
    y = 0; /* start with image begin */
  } else {

    pwo = rltlmd.pwork + old;
    y = pwo->ymin;
  }

  /* reunion starting from old->ymin */
  for (; y <= ymax; y++) {

    p = (T_compareT_RLC *)vgetpm(rlc) + rltlmd.pRowIdx[y];

    while((int32)p->rly == y) {
      if (p->rlab == oldlab) {
        p->rlid = newid;
        p->rlab = newlab;
      }
      p++;
    }

  }

  return;
}

static int32 object_w_append() /* append new element out
===============================   of empty list
                                  the object is always appended at the begin
                                  if empty list is empty, clean up with worked
                                  off objects */
{
  int32 act;
  register Tobj *pw;

  if (rltlmd.firstempty == NO_ELEMENT) goto error; /* empty free list */

  act = (int32)rltlmd.firstempty;
  pw = rltlmd.pwork + act;

  rltlmd.firstempty = pw->next;
  pw->next = rltlmd.first;
  pw->prev = NO_ELEMENT;
  if (rltlmd.first != NO_ELEMENT) (rltlmd.pwork + rltlmd.first)->prev = act;
  rltlmd.first = act;

  /* PRINTF1("object_w_append(): new %d\n", act); */
  return(act);

error:
  errstring = (char *)"object buffer overflow";
  return(-1);
}

static void object_w_delete(int32 act) /* delete object out of work list
====================================== */
{
  int32 next, prev;

  next = (rltlmd.pwork + act)->next;
  prev = (rltlmd.pwork + act)->prev;

  if (prev == NO_ELEMENT) rltlmd.first = next;
  else (rltlmd.pwork + prev)->next = next;
  if (next != NO_ELEMENT) (rltlmd.pwork + next)->prev = prev;

  (rltlmd.pwork + act)->next = rltlmd.firstempty;
  rltlmd.firstempty = act;

  return;
}

static int object_store(int32 act, Tvector *obj, int32 minarea, int flags, int32 xx, int32 yy) /* store obj in dst list
==============================================================================================    and free work object */
{
  int ierr;
  register T_compareT_Obj *pobj;
  register Tobj *pw;

  pw = rltlmd.pwork + act;

  /* PRINTF2("FSV: object_store, area %d (min %d)\n", pw->area, minarea); */
  /* check if area is big enough */
  if (pw->area < minarea) {
    if (pw->area <= 0) goto end; /* shouldn't happen */
    if ( ((flags & RLENDLESS_TOP) &&                /* vertical top endless acquisition */
          pw->ymin > 0) ||                          /* ... and object doesn't touch upper border */
         ((flags & RLENDLESS_BOTTOM) &&             /* vertical bottom endless acquisition */
          pw->ymax < (yy - 1)) ||                   /* ... and object doesn't touch lower border */
         ((flags & RLENDLESS_LEFT) &&               /* horizontal left endless acquisition */
          pw->xmin > 0) ||                          /* ... and object doesn't touch left border */
         ((flags & RLENDLESS_RIGHT) &&              /* horizontal right endless acquisition */
          pw->xmax < (xx - 1)) ||                   /* ... and object doesn't touch lower border */
          ((flags & RLENDLESS_ALL) == 0) ) {        /* or no endless acquisition at all */
      (rltlmd.num_underlimit)++;
      goto end;
    }
  }

  if (flags & RLDONTSTORE) {
    (rltlmd.num_objects)++;
    goto end;
  }

  /* check if obj has enough space */
  if ((int32)vgetln(obj) < (int32)((rltlmd.num_objects + 1) * sizeof(T_compareT_Obj))) {
#ifdef _PPC_
    PRINTL3(TSTPR_LEVEL_ERROR,
            "FSV ERROR: object limit reached (%d < %d (%d Obj))\n",
            vgetln(obj), (rltlmd.num_objects + 1) * sizeof(T_compareT_Obj),
            rltlmd.num_objects + 1);
#else
    PRINTL3(TRACE_ERROR,
            "FSV ERROR: object limit reached (%d < %d (%d Obj))\n",
            vgetln(obj), (rltlmd.num_objects + 1) * sizeof(T_compareT_Obj),
            rltlmd.num_objects + 1);
#endif
    errstring = (char *)"object limit reached";
    ierr = RLTLM_OBJECT_LIMIT;
    goto error;
  }
  pobj = (T_compareT_Obj *)vgetpm(obj);
  pobj += rltlmd.num_objects;
#ifdef _PPC_
  PRINTL5(TSTPR_LEVEL_DEBUG,
          "object store %d, 0x%08lx (%d >= %d), act %d\n",
          rltlmd.num_objects, pobj,
          (int32)vgetln(obj),
          (rltlmd.num_objects + 1) * sizeof(T_compareT_Obj), act);
#else
  PRINTL5(TRACE_DEBUG,
          "object store %d, 0x%08lx (%d >= %d), act %d\n",
          rltlmd.num_objects, pobj,
          (int32)vgetln(obj),
          (rltlmd.num_objects + 1) * sizeof(T_compareT_Obj), act);
#endif

  pobj->xmin = pw->xmin;
  pobj->xmax = pw->xmax;
  pobj->ymin = pw->ymin;
  pobj->ymax = pw->ymax;
  pobj->area = pw->area;
  pobj->label = pw->label;
  pobj->numLightErrPix = pw->numLightErrPix;
  pobj->numDarkErrPix  = pw->numDarkErrPix;

  /* prevent division by 0 */
  if (pw->sumRefVal[FSV_RED]   == 0) pw->sumRefVal[FSV_RED] = 1;
  if (pw->sumRefVal[FSV_GREEN] == 0) pw->sumRefVal[FSV_GREEN] = 1;
  if (pw->sumRefVal[FSV_BLUE]  == 0) pw->sumRefVal[FSV_BLUE] = 1;

  /* max object size is 23 bit worst case for contrastDev, so calculate
     in double to prevent overflow due to mult 100 */
  pobj->relContrDev[FSV_RED] =
    dto32(((double)pw->sumContrDev[FSV_RED] /
           (double)pw->sumRefVal[FSV_RED]) * 100.0);
  pobj->relContrDev[FSV_GREEN] =
    dto32(((double)pw->sumContrDev[FSV_GREEN] /
           (double)pw->sumRefVal[FSV_GREEN]) * 100.0);
  pobj->relContrDev[FSV_BLUE] =
    dto32(((double)pw->sumContrDev[FSV_BLUE] /
           (double)pw->sumRefVal[FSV_BLUE]) * 100.0);

  /* PRINTF5("  %d %d %d %d %d\n", pobj->xmin, pobj->xmax,
          pobj->ymin, pobj->ymax, pobj->area); */

  (rltlmd.num_objects)++;
  vputnm(obj, rltlmd.num_objects * sizeof(T_compareT_Obj));

end:
  ierr = 0;
error:
  object_w_delete(act);
  /* PRINTF1("FSV: object_store, total # obj: %d\n", rltlmd.num_objects); */

  return(ierr);
}

static int object_store_all(Tvector *obj, int32 minarea, int flags, int32 xx, int32 yy) /* store all work objects in
=======================================================================================    dst list */
{
  int ierr;
  int32 act, next;

  act = rltlmd.first;

  while(act != NO_ELEMENT) {
    next = (rltlmd.pwork + act)->next;

    ierr = object_store(act, obj, minarea, flags, xx, yy);
    if( ierr != 0) {
      return(ierr);
    }
    act = next;
  }

  return(0);
}

static int object_cleanup(Tvector *obj, int32 minarea, int32 y, int flags, int32 xx, int32 yy) /* store all worked off
==============================================================================================    objects in dst list */
{
  int ierr;
  int32 act, next;

  act = rltlmd.first;

  while(act != NO_ELEMENT) {
    next = (rltlmd.pwork + act)->next;
    /* PRINTF2("FSV: object_cleanup checking %d, next %d\n", act, next); */
    if (y > (rltlmd.pwork + act)->ymax) { /* if wasn't updated in this row */
      if (flags & RLVWRAPAROUND) {
        if ((rltlmd.pwork + act)->ymin > 0) {
          /* leave the top row touching objects in the work list */
          ierr = object_store(act, obj, minarea, flags, xx, yy);
          if( ierr != 0) {
            return(ierr);
          }
        }
      } else {
        ierr = object_store(act, obj, minarea, flags, xx, yy);
        if( ierr != 0) {
          return(ierr);
        }
      }
    }
    act = next;
  }

  return(0);
}

static int get_first_line(Tvector *rlc) /* get first line into work list
========================================*/
{
  int32 act;
  register T_compareT_RLC *p;

  p = (T_compareT_RLC *)vgetpm(rlc) + rltlmd.pRowIdx[0];

  /* enter line in work list */
  while(p->rly == 0) {
    act = object_w_append();
    if (act < 0) return(act);
    (rltlmd.label)++;
    p->rlab = rltlmd.label;        /* label */
    p->rlid = act;                 /* list index */
    meas_init(act, p->rlxa, p->rlxe, 0, p->rlab,
                   p->numLightErrPix, p->numDarkErrPix,
                   p->meanRefVal, p->meanContrDev);
    p++;
  }

  return(0);

}

static int object_lab(Tvector *rlc, Tvector *obj, int32 minarea, int overlap, int flags) /* label rlc image
======================================================================================== */
{
  int ierr;
  register int32 y, xx, yy;
  int32 act;
  T_compareT_RLC *p, *pl;

  yy = rltlmd.yy;
  xx = rltlmd.xx;

  for (y = 1; y < yy; y++) {

    if (iabort()) return(0);        /* break or timeout */

    pl = (T_compareT_RLC *)vgetpm(rlc) + rltlmd.pRowIdx[y - 1];
    p  = (T_compareT_RLC *)vgetpm(rlc) + rltlmd.pRowIdx[y];

    /* now we try to label actual row with last row */
    while(1) {

      if (p->rly != y) {                           /* actual line at end */

        ierr = object_cleanup(obj, minarea, y, flags, xx, yy);
        if( ierr != 0) {
          return(ierr);
        }
        break;                               /* this row is done */

      } else if ((pl->rly != y - 1) ||             /* last line at end */
                 (p->rlxe + overlap < pl->rlxa)) { /* actual before last line */

        if (p->rlab == NOLABEL) { /* open new working object */
          act = object_w_append();
          if (act < 0) return(act);
          (rltlmd.label)++;
          p->rlab = rltlmd.label;        /* label */
          p->rlid = act;                 /* list index */
          meas_init(act, p->rlxa, p->rlxe, y, p->rlab,
                    p->numLightErrPix, p->numDarkErrPix,
                    p->meanRefVal, p->meanContrDev);
        }
        p++;                      /* update actual pointer */

      } else if (pl->rlxe + overlap < p->rlxa) { /* last line before actual */

        pl++;                     /* update last line pointer */

      } else {                    /* last and actual touch */

        /* the actual and last field touch, so label the actual field.
           For RLSEPARATELIGHTDARK only if both the same error type */
        if (((flags & RLSEPARATELIGHTDARK) == 0) ||
            ((flags & RLSEPARATELIGHTDARK) &&
             ((p->numDarkErrPix != 0 && pl->numDarkErrPix != 0) ||
              (p->numLightErrPix != 0 && pl->numLightErrPix != 0)))) {

          if (p->rlab == NOLABEL) {

            p->rlid = pl->rlid;
            p->rlab = pl->rlab;
            meas_update((int32)p->rlid, p->rlxa, p->rlxe, y,
                        p->numLightErrPix, p->numDarkErrPix,
                        p->meanRefVal, p->meanContrDev);

          } else if (p->rlab != pl->rlab) {

            /* we have to do a label reunion */
            /* PRINTF2("FSV reunion, new %d old %d\n",
            (int32)p->rlid, (int32)pl->rlid); */
            meas_reunion((int32)p->rlid,           /* new */
                         (int32)pl->rlid, 0, yy);  /* old */
            object_w_delete((int32)pl->rlid);

            if (flags & RLENTERLABELS) {

              rlc_reunionAll(p->rlab, p->rlid, pl->rlid, pl->rlab, y, rlc);

            } else {

              rlc_reunion(p->rlab, p->rlid, pl->rlab, y, rlc);

              if (flags & RLVWRAPAROUND) {
                /* for wrap around update the labels in the top row too */
                rlc_reunion(p->rlab, p->rlid, pl->rlab, 0, rlc);
              }
              /* do the reunion in the last line at last, because pl->rlab is
              overwritten */
              rlc_reunion(p->rlab, p->rlid, pl->rlab, y-1, rlc);

            }

          } /* else { p->rlab == pl->rlab,circular!, they are just unified */

        } else {

          /* the actual and last field touch, but they are NOT the same error type */

          if (p->rlab == NOLABEL) { /* open new working object */
            act = object_w_append();
            if (act < 0) return(act);
            (rltlmd.label)++;
            p->rlab = rltlmd.label;        /* label */
            p->rlid = act;                 /* list index */
            meas_init(act, p->rlxa, p->rlxe, y, p->rlab,
                      p->numLightErrPix, p->numDarkErrPix,
                      p->meanRefVal, p->meanContrDev);
          }
        }

        /* update pointer of leftside field */
        if (p->rlxe > pl->rlxe) pl++;
        else                    p++;

      }
    }
  }

  /* for wrap around try to do reunions between top and bottom row */
  if (flags & RLVWRAPAROUND) {

    pl = (T_compareT_RLC *)vgetpm(rlc) + rltlmd.pRowIdx[yy - 1];
    p  = (T_compareT_RLC *)vgetpm(rlc) + rltlmd.pRowIdx[0];

    /* now we try to label last top with bottom row */
    while(1) {

      if ((p->rly != 0) || (pl->rly != yy - 1)) {     /* one line at end */

        break;                                        /* done */

      } else if ((p->rlxe + overlap < pl->rlxa)) { /* actual before last line */

        p++;                      /* update actual pointer */

      } else if (pl->rlxe + overlap < p->rlxa) { /* last line before actual */

        pl++;                     /* update last line pointer */

      } else {                    /* last and actual touch */

        /* the actual and last field touch, so label the actual field.
           For RLSEPARATELIGHTDARK only if both the same error type */
        if (((flags & RLSEPARATELIGHTDARK) == 0) ||
            ((flags & RLSEPARATELIGHTDARK) &&
             ((p->numDarkErrPix != 0 && pl->numDarkErrPix != 0) ||
              (p->numLightErrPix != 0 && pl->numLightErrPix != 0)))) {

          if (p->rlab != pl->rlab) {

            /* we have to do a label reunion */
            meas_reunion((int32)p->rlid,               /* new */
              (int32)pl->rlid, flags, yy);  /* old */
            object_w_delete((int32)pl->rlid);
            if (flags & RLENTERLABELS) {
#ifdef use_again
              rlc_reunionAll(p->rlab, p->rlid, -1, pl->rlid, yy - 1, rlc);
#else
              rlc_reunionAll(p->rlab, p->rlid, -1, pl->rlab, yy - 1, rlc);
#endif
            } else {
              rlc_reunion(p->rlab, p->rlid, pl->rlab, 0, rlc);
              /* do the reunion in the last line at last, because pl->rlab is
              overwritten */
              rlc_reunion(p->rlab, p->rlid, pl->rlab, yy - 1, rlc);
            }

          } /* else { p->rlab == pl->rlab, circular!, they are just unified */
        }

        /* update pointer of leftside field */
        if (p->rlxe > pl->rlxe) pl++;
        else                    p++;

      }
    }
  }

  return(0);

}

static int compareTrlt_labmeaEx(Tvector *src_rlc, Tvector *src_rowIdxTab, Tvector *dst_obj, Tvector *work_obj,
                         int32 minarea, int rloverlap, int rlflags, int32 xx) /* label & meas
============================================================================== */
{
  int ierr;

  if(utvcheck(src_rlc, DV_HOST, TY_INT8)) {
    errstring = (char *)"bad rlc vector";
    return(-1);
  }
  if(utvcheck(src_rowIdxTab, DV_HOST, TY_INT32)) {
    errstring = (char *)"bad rlc row index table";
    return(-1);
  }
  if ((rlflags & RLDONTSTORE) == 0) {
    if(utvcheck(dst_obj, DV_HOST, TY_INT8)) {
      errstring = (char *)"bad obj vector";
      return(-2);
    }
  }
  if(utvcheck(work_obj, DV_HOST, TY_INT8)) {
    errstring = (char *)"bad work vector";
    return(-3);
  }
  if (vgetln(work_obj) < (int)sizeof(Tobj)) {
    errstring = (char *)"work vector too small";
    return(-4);
  }

  rltlmd.max_objects = vgetln(work_obj) / sizeof(Tobj) - 1; /* elem. 0 unused */
  rltlmd.pwork = (Tobj *)vgetpm(work_obj);
  rltlmd.pRowIdx = (int32 *)vgetpm(src_rowIdxTab);
  rltlmd.xx = xx;
  rltlmd.yy = vgetnm(src_rowIdxTab);

  if ((rlflags & RLDONTSTORE) == 0) {
    /* at least space for one object */
    if ((int32)vgetln(dst_obj) < (int)sizeof(T_compareT_Obj)) {
      errstring = (char *)"object limit reached";
      return(RLTLM_OBJECT_LIMIT);
    }

    vputnm(dst_obj, 0L);  /* empty dst list */
  }
  rltlmd.num_objects = 0;
  rltlmd.num_underlimit = 0;
  rltlmd.label = 0;

  /* make hole work list to a free list */
  init_work_list();
  /* PRINTF0("FSV: rlt_labmea: init_work_list() done\n"); */

  /* take first RLC line, initialize work objects */
  ierr = get_first_line(src_rlc);
  if( ierr != 0) {
    goto error;
  }
  /* PRINTF0("FSV: rlt_labmea: get_first_line() done\n"); */

  /* now analyze rest of RLC image */
  ierr = object_lab(src_rlc, dst_obj, minarea, rloverlap, rlflags);
  if( ierr != 0) {
    goto error;
  }
  /* PRINTF1("FSV: rlt_labmea: object_lab() done with %d\n", ierr); */
//x/end:

  /* store all work objects in destination object list */
  ierr = object_store_all(dst_obj, minarea, rlflags, rltlmd.xx, rltlmd.yy);
  if( ierr != 0) {
    goto error;
  }
  /* PRINTF1("FSV: rlt_labmea: object_store_all() done with %d\n", ierr); */

  ierr = rltlmd.num_objects;

error:

#ifdef _PPC_
  PRINTF1("under limit: %d\n", rltlmd.num_underlimit);
#else
  PRINTL1(TRACE_DEBUG, "under limit: %d\n", rltlmd.num_underlimit);
#endif
  return(ierr);
}

static int compareTrlt_labmea(Tvector *src_rlc, Tvector *src_rowIdxTab, Tvector *dst_obj, Tvector *work_obj,
                       int32 minarea, int rloverlap, int rlflags) /* label & meas
============================================================================== */
{

  if (rlflags & (RLENDLESS_LEFT | RLENDLESS_RIGHT)) {

    //x/printf2Console("compareTrlt_labmea(): no horizontal endless acquisition supported\n");
  }
  rlflags &= ~(RLENDLESS_LEFT | RLENDLESS_RIGHT); /* no horizontal endless acquisition supported */

  return( compareTrlt_labmeaEx(src_rlc, src_rowIdxTab, dst_obj, work_obj,
                              minarea, rloverlap, rlflags, 0));
}

static int drawObject( YaIPS_RGB_ImgD_t *pOut, Tvector *vRLC, Tvector *vRowIdxTab,
                       T_compareT_Obj *pObj, int32 imxa, int32 imya, Fl_Color Color) /*
=============================================================================== */
{
  int x, y;
  uchar r, g, b;
  T_compareT_RLC *p;
  int32 *pRowIdx;
  uchar *d8;

  // Security test

  if( pOut->d < 3) {                                                  // Expect a color image

    return( -10);
  }

  if( pObj->xmin + imxa < 0 || pObj->xmax + imxa >= pOut->xx ||       // Object must fit into the image
      pObj->ymin + imya < 0 || pObj->ymax + imya >= pOut->yy) {

    return( -11);
  }


  // Prepare color

  Fl::get_color( Color, r, g, b);   // Background color

  //...

  pRowIdx = (int32 *)vgetpm(vRowIdxTab);

  for( y = pObj->ymin; y <= pObj->ymax; y++) {

    p = (T_compareT_RLC *)vgetpm(vRLC) + pRowIdx[y];

    while( p->rly == y) {

      if (p->rlxa > pObj->xmax) break;  /* passed the object */

      if (p->rlab == pObj->label) {     /* found RLC of object */

        d8 = RGB_pixad( p->rlxa + imxa, y + imya, pOut);

        for( x = p->rlxa; x <= p->rlxe; x++) {

#ifdef use_again
          // Set color
          d8[ 0] = r;
          d8[ 1] = g;
          d8[ 2] = b;
#else
          // Average color with pixel value
          d8[ 0] = (d8[ 0] + r + r + r) >> 2;
          d8[ 1] = (d8[ 1] + g + g + g) >> 2;
          d8[ 2] = (d8[ 2] + b + b + b) >> 2;
#endif

          d8 += pOut->d;
        }
      }

      p++;

    }
  }

  return(0);

} /* static int drawObject() */

/* **************************************************************************
 *
 *  compareTDrawEx
 *  draw Objects
 *
 * dispMode 1:
 *  col0: all light errors
 *  col1: all dark errors
 *  col2: not used
 *
 * dispMode other than 1:
 *  col0: large blob
 *  col1: blob
 *  col2: small blob
 ************************************************************************** */
static int compareTDrawEx( YaIPS_RGB_ImgD_t *pOut, Tvector *vObj, Tvector *vRLC, Tvector *vRowIdxTab,
                           int32 thresAB, int32 thresAL, int32 imxa, int32 imya, int dispMode,
                           Fl_Color col0, Fl_Color col1, Fl_Color col2) /*
==================================================================================== */
{
  register int i, numObj;
  T_compareT_Obj *p;

  if (dispMode <= 0) return(0);

  if( pOut->d < 3) {                  // Expect a color image

    return( -1);
  }

  numObj = vgetnm(vObj) / sizeof(T_compareT_Obj);
  p = (T_compareT_Obj *)vgetpm(vObj);

  for (i = 0; i < numObj; i++, p++) {

    if (p->area <= 0) continue;

    switch(dispMode) {
      case 1:                       /* dark/bright display */
        if (p->numLightErrPix > p->numDarkErrPix) {
          drawObject( pOut, vRLC, vRowIdxTab, p, imxa, imya, col0);

        } else {
          drawObject( pOut, vRLC, vRowIdxTab, p, imxa, imya, col1);
        }
        break;
      default:
        if (p->area >= thresAL) {             /* large blob */
          drawObject( pOut, vRLC, vRowIdxTab, p, imxa, imya, col0);
        } else if (p->area >= thresAB) {            /* blob */
          drawObject( pOut, vRLC, vRowIdxTab, p, imxa, imya, col1);
        } else if (p->area > 0) {             /* small blob */
          drawObject( pOut, vRLC, vRowIdxTab, p, imxa, imya, col2);
        }
    }
  }

  return(0);

} /* int compareTDrawEx() */

/* **************************************************************************
 *
 *  Compare Statistics extended Version2
 *
 *  count the objects and the sum/max of area related to each class
 ************************************************************************** */

static int compareTStatisticsEx2( Tvector *vObj, int32 thresAB, int32 thresAL, int32 *numS, int32 *numB, int32 *numL,
                                  int32 *sumAreaS, int32 *sumAreaB, int32 *sumAreaL,
                                  int32 *maxAreaS, int32 *maxAreaB, int32 *maxAreaL) /*
========================================================================================================== */
{
  int i, numObj;
  T_compareT_Obj *p;

  numObj = vgetnm(vObj) / sizeof(T_compareT_Obj);
  p = (T_compareT_Obj *)vgetpm(vObj);

  *numS = 0;
  *numB = 0;
  *numL = 0;
  *sumAreaS = 0;
  *sumAreaB = 0;
  *sumAreaL = 0;
  *maxAreaS = 0;
  *maxAreaB = 0;
  *maxAreaL = 0;

  for (i = 0; i < numObj; i++, p++) {

    if (p->area >= thresAL) {             /* large blob */
      *numL = *numL + 1;
      *sumAreaL = *sumAreaL + p->area;
      if (p->area > *maxAreaL) *maxAreaL = p->area;
    } else if (p->area >= thresAB) {            /* blob */
      *numB = *numB + 1;
      *sumAreaB = *sumAreaB + p->area;
      if (p->area > *maxAreaB) *maxAreaB = p->area;
    } else if (p->area > 0) {             /* small blob */
      *numS = *numS + 1;
      *sumAreaS = *sumAreaS + p->area;
      if (p->area > *maxAreaS) *maxAreaS = p->area;
    }
  }

  return(0);

} /* int compareTStatisticsEx2() */

/***************************************************************************
* YaIPS_ImageCompare_CopyImage
*
* Copy image color or black/white to a color image.
* Alpha is copied.
*
* ppDst        Out: Pointer to pointer to RGB image
* pSrc         In: Pointer to image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_ImageCompare_CopyImage( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to image
                                  Fl_RGB_Image *pSrc,   // In: Pointer to RGB image
                                  int FlagDarken)       // If set, darken output image
{
  int ierr, x, y, SrcD, DstD, SrcHasAlpha;
  int xmin, ymin;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8, t;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  SrcHasAlpha = iSrc.d == 2 || iSrc.d == 4;

  // Ensure that pPDst image has the same size and 3 byte per pixel
  ierr = YaIPS_RGB_ImageSetSize( ppDst, iSrc.xx, iSrc.yy, 3 + SrcHasAlpha);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data

  xmin = iSrc.xx;
  ymin = iSrc.yy;
  SrcD = iSrc.d;

  DstD = iDst.d;

  for( y = 0; y < ymin; y++) {

    s8 = RGB_pixad( 0, y, &iSrc);
    d8 = RGB_pixad( 0, y, &iDst);

    if( SrcD >= 3) {                  // Source is a color image

      if( FlagDarken) {

        for( x = 0; x < xmin; x++) {

          d8[ 0] = (s8[ 0] >> 1) + (s8[ 0] >> 2);   // 3/4 of pixel value
          d8[ 1] = (s8[ 1] >> 1) + (s8[ 1] >> 2);   // 3/4 of pixel value
          d8[ 2] = (s8[ 2] >> 1) + (s8[ 2] >> 2);   // 3/4 of pixel value

          if( SrcHasAlpha) {     // Copy alpha

            d8[ 3] = s8[ 3];
          }

          s8 += SrcD;
          d8 += DstD;
        }

      } else {

        for( x = 0; x < xmin; x++) {

          d8[ 0] = s8[ 0];
          d8[ 1] = s8[ 1];
          d8[ 2] = s8[ 2];

          if( SrcHasAlpha) {     // Copy alpha

            d8[ 3] = s8[ 3];
          }

          s8 += SrcD;
          d8 += DstD;
        }
      }

    } else {                          // Source is a black white image

      if( FlagDarken) {

        for( x = 0; x < xmin; x++) {

          t = (s8[ 0] >> 1) + (s8[ 0] >> 2);   // 3/4 of pixel value

          d8[ 0] = t;
          d8[ 1] = t;
          d8[ 2] = t;

          if( SrcHasAlpha) {     // Copy alpha

            d8[ 3] = s8[ 1];
          }

          s8 += SrcD;
          d8 += DstD;
        }

      } else {

        for( x = 0; x < xmin; x++) {

          t = s8[ 0];

          d8[ 0] = t;
          d8[ 1] = t;
          d8[ 2] = t;

          if( SrcHasAlpha) {     // Copy alpha

            d8[ 3] = s8[ 1];
          }

          s8 += SrcD;
          d8 += DstD;
        }
      }
    }
  }

  return( 0);                                 // Return OK
}

/************************************************************************************
* Inspection init/end things
*
*/

static int doInspInit( YaIPS_CompareAOI_t *pWin, Tsip_winC0 *pSip) /*
======================================================================================= */
{
  int ierr;

  // compare vectors

  if ((pSip->vRLC = ve_ucreate(DV_HOST)) == VENULL) {
    //x/sysstate_setsyserr(SYSERR_MODULE_IP, SEIP_VCRE, 0, NULL);      // failed to create vector
    ierr = -SE_VCRE;
    goto syserrorExit;
  }

  ierr = ve_alloc(pSip->vRLC, (MaxNumCompareErrors + 2) * sizeof(T_compareT_RLC), sizeof(int8), TY_INT8);
  if( ierr != 0) {
    //x/sysstate_setsyserr(SYSERR_MODULE_IP, SEIP_VALLOC, ierr, NULL);   // error %d allocating vector
    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  if ((pSip->vRowIdxTab = ve_ucreate(DV_HOST)) == VENULL) {
    //x/sysstate_setsyserr(SYSERR_MODULE_IP, SEIP_VCRE, 0, NULL);      // failed to create vector
    ierr = -SE_VCRE;
    goto syserrorExit;
  }

  ierr = ve_alloc(pSip->vRowIdxTab, pWin->AOI.YSize, sizeof(int32), TY_INT32);
  if( ierr != 0) {
    //x/sysstate_setsyserr(SYSERR_MODULE_IP, SEIP_VALLOC, ierr, NULL);   // error %d allocating vector
    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  if ((pSip->vCompareObj = ve_ucreate(DV_HOST)) == VENULL) {
    //x/sysstate_setsyserr(SYSERR_MODULE_IP, SEIP_VCRE, 0, NULL);      // failed to create vector
    ierr = -SE_VCRE;
    goto syserrorExit;
  }

  ierr = ve_alloc(pSip->vCompareObj, (MaxNumCompareErrors + 1) * sizeof(T_compareT_Obj), sizeof(int8), TY_INT8);
  if( ierr != 0) {
    //x/sysstate_setsyserr(SYSERR_MODULE_IP, SEIP_VALLOC, ierr, NULL);   // error %d allocating vector
    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  if ((pSip->vLabWork = ve_ucreate(DV_HOST)) == VENULL) {
    //x/sysstate_setsyserr(SYSERR_MODULE_IP, SEIP_VCRE, 0, NULL);      // failed to create vector
    ierr = -SE_VCRE;
    goto syserrorExit;
  }

  ierr = ve_alloc(pSip->vLabWork, (pWin->AOI.XSize + 2) * sizeof(Tobj), sizeof(int8), TY_INT8);
  if( ierr != 0) {
    //x/sysstate_setsyserr(SYSERR_MODULE_IP, SEIP_VALLOC, ierr, NULL);   // error %d allocating vector
    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  ierr = 0;

syserrorExit:

  return(ierr);

} /* static int doInspInit() */

static int ppcWin_InspInit( Fl_RGB_Image *piref, YaIPS_CompareAOI_t *pWin) /*
========================================================================== */
{
  int ierr;
  Fl_YaIPS_AOI_t *pAoi;
  Tsip_winC0 *pSip;

  PRINTL1(TRACE_RESULT, "ppcWin_InspInit %d\n", winNr);

  // Check work data allocated

  pSip = (Tsip_winC0 *)pWin->pWorkData;

  if( pSip != NULL) {      // Already allocated

    return( 0);            // Return OK
  }

  pSip = (Tsip_winC0 *)malloc( sizeof( Tsip_winC0));    // Allocated memory

  if( pSip == NULL) {      // Check allocated

    ierr = -SE_MEMALLOC;
    goto syserrorExit;
  }

  memset( pSip, 0, sizeof( Tsip_winC0));    // Zero memory

  pWin->pWorkData = pSip;                    // Remember pointer to allocated memory

  pAoi = &pWin->AOI;

  // preset with VENULL/IMNULL for ppcWin_InspEnd() call after error or NOT active window

  pSip->vRLC        = VENULL;
  pSip->vRowIdxTab  = VENULL;
  pSip->vCompareObj = VENULL;
  pSip->vLabWork    = VENULL;

  pSip->lastActive = true;

  if( (ierr = doInspInit( pWin, pSip)) != 0) {
    // sets syserror itself
    goto syserrorExit;
  }

  // remember positions and sizes

  pSip->lastActive = true;

  pSip->lastWinDesc.XPos  = pAoi->XPos;
  pSip->lastWinDesc.YPos  = pAoi->YPos;
  pSip->lastWinDesc.XSize = pAoi->XSize;
  pSip->lastWinDesc.YSize = pAoi->YSize;

  ierr = 0;

syserrorExit:

  return(ierr);

} /* int ppcWin_InspInit() */

static int doInspEnd( Tsip_winC0 *pSip, YaIPS_CompareAOI_t *pWin) /*
================================================================= */
{
  int ierr;

  if (pSip->vRLC != VENULL) {
    if( (ierr = ve_remove(pSip->vRLC)) != 0) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VREM, ierr, "error %d removing vector");
      ierr = -SE_VREM;
      goto syserrorExit;
    }
    pSip->vRLC = VENULL;
  }
  if (pSip->vRowIdxTab != VENULL) {
    if( (ierr = ve_remove(pSip->vRowIdxTab)) != 0) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VREM, ierr, "error %d removing vector");
      ierr = -SE_VREM;
      goto syserrorExit;
    }
    pSip->vRowIdxTab = VENULL;
  }
  if (pSip->vCompareObj != VENULL) {
    if( (ierr = ve_remove(pSip->vCompareObj)) != 0) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VREM, ierr, "error %d removing vector");
      ierr = -SE_VREM;
      goto syserrorExit;
    }
    pSip->vCompareObj = VENULL;
  }
  if (pSip->vLabWork != VENULL) {
    if( (ierr = ve_remove(pSip->vLabWork)) != 0) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VREM, ierr, "error %d removing vector");
      ierr = -SE_VREM;
      goto syserrorExit;
    }
    pSip->vLabWork = VENULL;
  }

  ierr = 0;

syserrorExit:

  return(ierr);

} /* int doInspEnd() */

/************************************************************************************
* YaIPS_PSearchAOI_WorkDataFree
*
* Free allocated work data at close of window.
*
*/

int YaIPS_ImageCompare_WorkDataFree( YaIPS_CompareAOI_t *pWin) /*
============================================================== */
{
  int ierr;
  Tsip_winC0  *pSip;              // pointer to sip structure

  PRINTL1(TRACE_RESULT, "ppcWin_InspEnd %d\n", winNr);

  // Check work data allocated

  pSip = (Tsip_winC0 *)pWin->pWorkData;
  if( pSip == NULL) {             // Was not allocated

    ierr = 0;                     // Done, nothing to do
    goto syserrorExit;
  }

  if( (ierr = doInspEnd(pSip, pWin)) != 0) {
    // sets syserror itself
    goto syserrorExit;
  }

  ierr = 0;

syserrorExit:

  if( pSip != NULL) {             // Was allocated

    free( pSip);                  // Free data

    pWin->pWorkData = NULL;       // No work data
  }

  return( ierr);

} /* int ppcWin_InspEnd() */

/************************************************************************************
* YaIPS_InspectCode2Text
*
*/

static void YaIPS_InspectCode2Text( int CheckError,
                                    char *pCheckText,
                                    int SizeOfCheckText)
{

  switch( CheckError) {

  case PPCWINC0_INSPERR_OK:   // This should be handled outside this function

    strncpy( pCheckText, LangStringLookup( "&Utils_ImgCompare_InspErr0="), SizeOfCheckText - 1);
    break;
  case PPCWINC0_INSPERR_WINBORDER:
    strncpy( pCheckText, LangStringLookup( "&Utils_ImgCompare_InspErr1=Touch border!"), SizeOfCheckText - 1);  // position error
    break;
  case PPCWINC0_INSPERR_BAREA:
    strncpy( pCheckText, LangStringLookup( "&Utils_ImgCompare_InspErr2=Sum small blobs!"), SizeOfCheckText - 1);  // position tolerance/search range error
    break;
  case PPCWINC0_INSPERR_LBLOBS:
    strncpy( pCheckText, LangStringLookup( "&Utils_ImgCompare_InspErr3=Large blobs!"), SizeOfCheckText - 1);   // touches image border
    break;
  case PPCWINC0_INSPERR_TOOMANY:
    strncpy( pCheckText, LangStringLookup( "&Utils_ImgCompare_InspErr4=Too many errors!"), SizeOfCheckText - 1);  // unable to compute trafo system (illegal window positions)
    break;
  default:
    sprintf( pCheckText, LANGDEF_ERROR_CODE, CheckError);
    break;
  }
}

/************************************************************************************
* YaIPS_ImageCompare_Inspect
*
* Inspect
*
*/

int YaIPS_ImageCompare_Inspect( Fl_RGB_Image **ppOut,     // Output image
                                Fl_RGB_Image *pSrc,       // Input image
                                Fl_RGB_Image *pRef,       // Reference image
                                YaIPS_CompareAOI_t *pWin, // Point to image compare data
                                int Teach_mode)           // 0 = inspection mode, 1 = teach mode
{
  int ierr;
  Fl_RGB_Image *pOut;
  YaIPS_RGB_ImgD_t iSrc, iRef, iOut, iSrcAOI, iRefAOI, iOutAOI;
  Tsip_winC0  *pSip;             // pointer to sip structure
  Fl_YaIPS_AOI_t *pAoi;
  int numErrPix;
  int32 minBlobArea, minLargeBlobArea;
  double minBlobAreaD, minLargeBlobAreaD;
  int32 numS, numB, numL;
  int32 sumAreaS, sumAreaB, sumAreaL;
  int32 maxAreaS, maxAreaB, maxAreaL;

  // -----------------------------------------------------------------------
  // initialization
  // -----------------------------------------------------------------------

  PRINTL2(TRACE_RESULT, "********** ppcWin_InspDo %d, Ref %d\n", winNr, pRef->refNr);

  pWin->InspError = 0;                         // Preset no error
  pWin->InspText[ 0] = '\0';

  // Check teach mode

  if( Teach_mode) {             // Teach mode is not supported

    ierr = 0;                   // Set no error
    goto ErrorExit;
  }

  // Setup up temporary work data

  ierr = ppcWin_InspInit( pSrc, pWin);         // Ensure initialized

  pSip = (Tsip_winC0 *)pWin->pWorkData;

  if( ierr != 0 ||            // Check problem with work data
      pSip == NULL) {

    ierr = -SE_NO_WORK_DATA;
    errstring = (char *)"No work data allocated";

    goto ErrorExit;
  }

  // Check scene image
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error

    ierr = -SE_RGB_2_IMGD;

    goto ErrorExit;
  }

  // Check reference image
  ierr = YaIPS_RGB_to_ImgD( pRef, &iRef);
  if( ierr != 0)  {                           // Check for error

    ierr = -SE_RGB_2_IMGD;

    goto ErrorExit;
  }

  // Copy input image to output image. Output image is always a
  // color image with alpha.
  // The color values are reduced to 3/4 of original value.
  // ==> This makes colored error blobs better visible.

  ierr = YaIPS_ImageCompare_CopyImage( ppOut, pSrc, pWin->FlagDarken);

  pOut = *ppOut;                              // Get pointer to destination image

  // Check output image
  ierr = YaIPS_RGB_to_ImgD( pOut, &iOut);
  if( ierr != 0)  {                          // Check for error

    ierr = -SE_RGB_2_IMGD;

    goto ErrorExit;
  }

  // -----------------------------------------------------------------------
  // check for changed position and size (onTheFly)
  // -----------------------------------------------------------------------

  pAoi = &pWin->AOI;

  if ( (pSip->lastWinDesc.XPos  != pAoi->XPos) ||
       (pSip->lastWinDesc.YPos  != pAoi->YPos) ||
       (pSip->lastWinDesc.XSize != pAoi->XSize) ||
       (pSip->lastWinDesc.YSize != pAoi->YSize)) {

    // re-Initialize inspection
    if( (ierr = doInspEnd( pSip, pWin)) != 0) {
      // sets syserror itself
      goto syserrorExit;
    }
    if( (ierr = doInspInit( pWin, pSip)) != 0) {
      // sets syserror itself
      goto syserrorExit;
    }

    // remember window positions and sizes

    pSip->lastActive = true;

    pSip->lastWinDesc.XPos  = pAoi->XPos;
    pSip->lastWinDesc.YPos  = pAoi->YPos;
    pSip->lastWinDesc.XSize = pAoi->XSize;
    pSip->lastWinDesc.YSize = pAoi->YSize;
  }

  // -----------------------------------------------------------------------
  // and do it
  // -----------------------------------------------------------------------

  // Setup AOIs

  ierr = YaIPS_ImgD_AOI( &iSrcAOI, &iSrc, pAoi);
  if( ierr != 0) {    // Check for error

    // window touches image border

    ierr = PPCWINC0_INSPERR_WINBORDER;
    goto ErrorExit;
  }

  ierr = YaIPS_ImgD_AOI( &iRefAOI, &iRef, pAoi);
  if( ierr != 0) {    // Check for error

    // window touches image border

    ierr = PPCWINC0_INSPERR_WINBORDER;
    goto ErrorExit;
  }

  ierr = YaIPS_ImgD_AOI( &iOutAOI, &iOut, pAoi);
  if( ierr != 0) {    // Check for error

    // window touches image border

    ierr = PPCWINC0_INSPERR_WINBORDER;
    goto ErrorExit;
  }

  // -----------------------------------------------------------------------
  // do the compare PASS 1
  // -----------------------------------------------------------------------

  vputnm( pSip->vRLC, 0);  /* reset runlength codes */

  numErrPix = 0;

  pWin->numLightErrPix = 0;
  pWin->numDarkErrPix = 0;
  pWin->numBlobs = 0;
  pWin->numLargeBlobs = 0;
  pWin->sumBErrArea = 0;
  pWin->sumLBErrArea = 0;
  pWin->maxBErrArea = 0;
  pWin->maxLBErrArea = 0;
  pWin->maxSErrArea = 0;

  // Pass 1 ...

  ierr = compareTPass1( &iSrcAOI, &iRefAOI,
                        pSip->vRLC, pSip->vRowIdxTab, pWin->GWToleranceLo, pWin->GWToleranceHi,
                        &pWin->numLightErrPix, &pWin->numDarkErrPix, &numErrPix,
                        MaxNumCompareErrors);

  if (ierr) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_COMPAREP1, 0, "error %d in compareTPass1_bw()");
    ierr = -SE_COMPAREP1;
    goto syserrorExit;
  }

  PRINTL2(TRACE_DEBUG, "PASS1: %d RLC's, %d index entries\n",
           vgetnm(pSip->vRLC) / sizeof(T_compareT_RLC),
           vgetnm(pSip->vRowIdxTab));
  PRINTL2(TRACE_RESULT, "dark: %d, bright: %d Pixels\n",
          pWin->numDarkErrPix, pWin->numLightErrPix);

  // -----------------------------------------------------------------------
  // error object labelling PASS 1
  // -----------------------------------------------------------------------

  // to be sure we do not find errors < minArea we have to round up the pixel area to the next integer value
  minBlobAreaD     = (double)pWin->MinBlobArea / YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y; // to pixel
  if (minBlobAreaD == (double)((int)minBlobAreaD)) {
    minBlobArea = (int)minBlobAreaD;
  } else {
    minBlobArea = (int)(minBlobAreaD + 1.0);
  }
  minLargeBlobAreaD = (double)pWin->MinLBlobArea / YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y; // to pixel
  if (minLargeBlobAreaD == (double)((int)minLargeBlobAreaD)) {
    minLargeBlobArea = (int)minLargeBlobAreaD;
  } else {
    minLargeBlobArea = (int)(minLargeBlobAreaD + 1.0);
  }

  if( true) {

    // label all objects with area 0...
    ierr = compareTrlt_labmea( pSip->vRLC, pSip->vRowIdxTab,
                              pSip->vCompareObj, pSip->vLabWork,
                              0, RLCONNECTIVITY_8, RLENTERLABELS);
  } else {
    // label all objects with area MinArea...
    ierr = compareTrlt_labmea(pSip->vRLC, pSip->vRowIdxTab,
                              pSip->vCompareObj, pSip->vLabWork,
                              minBlobArea, RLCONNECTIVITY_8, 0);
  }

  if (ierr < 0) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_COMPARELAB, 0, "error %d in compareTrlt_labmea()");
    ierr = -SE_COMPARELAB;
    goto syserrorExit;
  }

  PRINTL1(TRACE_DEBUG, "label: %d Objects\n",
           vgetnm(pSip->vCompareObj) / sizeof(T_compareT_Obj));

  // -----------------------------------------------------------------------
  // here possible PASS 3
  // -----------------------------------------------------------------------

  // -----------------------------------------------------------------------
  // compare Statistics
  // -----------------------------------------------------------------------

  ierr = compareTStatisticsEx2( pSip->vCompareObj,
                                minBlobArea, minLargeBlobArea,
                                &numS, &numB, &numL,
                                &sumAreaS, &sumAreaB, &sumAreaL,
                                &maxAreaS, &maxAreaB, &maxAreaL);
  if( ierr != 0) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_COMPARESTAT, 0, "error %d in compareTStatistics()");
    ierr = -SE_COMPARESTAT;
    goto syserrorExit;
  }

  PRINTL4(TRACE_RESULT, "small: %d (area %.2lf), large: %d (area %.2lf)\n",
          numS, (double)sumAreaS / (ProjTrafo[ pRef->refNr][A11] * ProjTrafo[ pRef->refNr][A22]),
          numL, (double)sumAreaL / (ProjTrafo[ pRef->refNr][A11] * ProjTrafo[ pRef->refNr][A22]));

  pWin->numBlobs = numB;
  pWin->numLargeBlobs = numL;
  pWin->sumBErrArea = (float)((double)sumAreaB * YaIPS_Calib_UPP_X * YaIPS_Calib_UPP_Y); // to units
  pWin->sumLBErrArea = (float)((double)sumAreaL * YaIPS_Calib_UPP_X * YaIPS_Calib_UPP_Y); // to units
  pWin->maxBErrArea = (float)((double)maxAreaB * YaIPS_Calib_UPP_X * YaIPS_Calib_UPP_Y); // to units
  pWin->maxLBErrArea = (float)((double)maxAreaL * YaIPS_Calib_UPP_X * YaIPS_Calib_UPP_Y); // to units
  pWin->maxSErrArea = (float)((double)maxAreaS * YaIPS_Calib_UPP_X * YaIPS_Calib_UPP_Y); // to units

  // No error until now

  ierr = 0;                 // OK
  pWin->InspError = ierr;

  // -----------------------------------------------------------------------
  // check tolerances
  // -----------------------------------------------------------------------

  if( ierr == 0 && numErrPix > MaxNumCompareErrors) {

    // too many errors
    ierr = PPCWINC0_INSPERR_TOOMANY;
  }

  if( ierr == 0 && pWin->numLargeBlobs > 0) {

    // large blobs error
    ierr =  PPCWINC0_INSPERR_LBLOBS;
  }

  if( ierr == 0 && pWin->sumBErrArea > pWin->SumBlobArea) {

    // blob area tolerance error
    ierr = PPCWINC0_INSPERR_BAREA;
  }

  sprintf( pWin->InspText, LangStringLookup( "&Utils_ImgCompare_Res1=%d large blobs%s, %.2f%s sum small"),
                                              pWin->numLargeBlobs, pWin->numLargeBlobs > 0 || ierr == PPCWINC0_INSPERR_TOOMANY ? "!" : "",
                                              pWin->sumBErrArea, pWin->sumBErrArea > pWin->SumBlobArea || ierr == PPCWINC0_INSPERR_TOOMANY ? "!" : "");

  pWin->InspError = ierr;     // Set inspection error

  // -----------------------------------------------------------------------
  // compare display
  // -----------------------------------------------------------------------

  ierr = compareTDrawEx( &iOut,
                         pSip->vCompareObj, pSip->vRLC, pSip->vRowIdxTab,
                         minBlobArea, minLargeBlobArea, pAoi->XPos, pAoi->YPos,
                         2,  // large blob: col0, blob: col1, small blob: col2
                         FL_RED, FL_GREEN, FL_BLUE); //x/ DISPLAY_RED, DISPLAY_BLUE, DISPLAY_GREEN);

  if( ierr != 0) {

    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_COMPAREDRAW, 0, "error %d in compareTDraw()");
    ierr = -SE_COMPAREDRAW;
    goto syserrorExit;
  }

  // -----------------------------------------------------------------------
  // exitPoint
  // -----------------------------------------------------------------------

ErrorExit:

  if( ierr != 0 && pWin->InspError == 0) {  // Error entry

    pWin->InspError = ierr;
  }

  // Ensure CheckText is set

  if( pWin->InspError != 0 &&                 // Have an error on exit
      pWin->InspText[ 0] == '\0') {           // but no text set until now

    YaIPS_InspectCode2Text( pWin->InspError, pWin->InspText, sizeof( pWin->InspText));
  }

  ierr = pWin->InspError;

syserrorExit:

  if( ierr != 0 && pWin->InspError == 0) {  // System error entry

    pWin->InspError = ierr;
  }

  return( ierr);

} /* int YaIPS_ImageCompare_Inspect() */

/************************************************************************************
* YaIPS_ImageCompare_Inspect
*
* Inspect
*
*/

int YaIPS_ImageCompare_DrawArea( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                                 YaIPS_CompareAOI_t *pWin,                // Point to image compare data
                                 int Teach_mode)
{
  //x/int LineWidth;
  int x1, y1, x, y, OffX, OffY, DrawText;
  Fl_Color DrawColor;
  Tsip_winC0 *pSip;
  int i, numObj;
  T_compareT_Obj *p;
  int mdx, mdy, mw, mh, CalcExtents;
  char TempString[ 256];

  // Check teach mode

  if( Teach_mode) {             // Teach mode is not supported

    return( 0);                 // Return OK
  }

  if( pWin->InspError < 0) {    // There was an inspection error

    return( 0);                 // Return OK
  }

  // Check for valid output image displayed

  if( pYaIPS_ImageDisp->pImage_Img == NULL) {

    return( -1);
  }

  // Preparations

  x1 = pYaIPS_ImageDisp->BigImage_sx;
  y1 = pYaIPS_ImageDisp->BigImage_sy;

  // Points relative to image

  OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
  OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

  //x/LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

  // Prepare font size

  DrawText = false;                                            // Preset, do not draw text

  if( pYaIPS_ImageDisp->PixelImageToScreen >= 0.33) {          // Screen resolution is NOT to tiny

    int TempFontSize;

    DrawText = true;                                           // Draw crcdf text

    TempFontSize = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 16.0 + 0.5);  // * 12.0

    if( TempFontSize < 10) {
      TempFontSize = 10;
    }

    fl_font( FL_HELVETICA, TempFontSize);

  } else {

    // No text drawing

    return( 0);                 // Return OK
  }

  // Security test

  // Check work data allocated

  pSip = (Tsip_winC0 *)pWin->pWorkData;

  if( pSip == NULL) {      // NO work data allocated

    return( 0);            // Return OK
  }

  if( pSip->vCompareObj == NULL) {  // No obecject vetor allocated

    return( 0);            // Return OK
  }


  // Draw ...

  int32 minBlobArea, minLargeBlobArea;
  double minBlobAreaD, minLargeBlobAreaD;
  int32 imxa, imya;

  // to be sure we do not find errors < minArea we have to round up the pixel area to the next integer value
  minBlobAreaD     = (double)pWin->MinBlobArea / YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y; // to pixel
  if (minBlobAreaD == (double)((int)minBlobAreaD)) {
    minBlobArea = (int)minBlobAreaD;
  } else {
    minBlobArea = (int)(minBlobAreaD + 1.0);
  }
  minLargeBlobAreaD = (double)pWin->MinLBlobArea / YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y; // to pixel
  if (minLargeBlobAreaD == (double)((int)minLargeBlobAreaD)) {
    minLargeBlobArea = (int)minLargeBlobAreaD;
  } else {
    minLargeBlobArea = (int)(minLargeBlobAreaD + 1.0);
  }

  imxa = pWin->AOI.XPos;
  imya = pWin->AOI.YPos;

  numObj = vgetnm( pSip->vCompareObj) / sizeof(T_compareT_Obj);
  p = (T_compareT_Obj *)vgetpm( pSip->vCompareObj);

  CalcExtents = false;

  for (i = 0; i < numObj; i++, p++) {

    if (p->area <= 0) continue;

    if (p->area >= minLargeBlobArea) {        /* large blob */

      // OK

      DrawColor = FL_RED;

    } else if (p->area >= minBlobArea) {            /* blob */

      // OK
      DrawColor = FL_GREEN;

    } else if (p->area > 0) {             /* small blob */

      // Skip drawing small blobs
      continue;
    }

    sprintf( TempString, "%.2lf", (double)p->area * YaIPS_Calib_UPP_X * YaIPS_Calib_UPP_Y);

    if(! CalcExtents) {        // Need to call extents only the first time

      fl_text_extents( TempString, mdx, mdy, mw, mh);

      CalcExtents = true;
    }

    x = imxa + p->xmin;
    y = imya + p->ymin;

    x = (int)( (x - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    y = (int)( (y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

    if( y + mdy < 4) {         // To near to upper border

      y += mh + 5;             // Show below upper frame
      x += 4;

    } else {                   // Fits above upper frame

      y -= 4;
    }

    fl_color( DrawColor);           // Color

    fl_draw( TempString, x1 + x, y1 + y);
  }

  return( 0);            // Return OK

} /* int YaIPS_ImageCompare_DrawArea() */

/****************************** End Of File ******************************/
