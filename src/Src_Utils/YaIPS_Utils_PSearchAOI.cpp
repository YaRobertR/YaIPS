/****************************************************************************

  YaIPS_Utils_PSearchAOI.cpp

  Common pattern search support.

 28.08.2025 RR: First edition of this file.

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

//x/#define USE_VGEOEST 1  // define this to use vgeoest() in place of vuparcal() to calulate the transforamtion system

/* ======================================================================= */

static int minTeachQual =  3;       // Minimum correlation teach quality
static int minProcQual  = 30;       // Minimum inspection quality

/* ---------------------- defines ---------------------------------------- */

#define TR1_MAX_SUBWIN 3                // maximum supported subwindows

#define PYR_SHRINK_SWITCH 2689600    /* 40 x 40 x 41 x 41 (window 40x40, Tol 20) */
#define PYR_SHRINK_TOL          2    /* 2 pixels tolerance for shrink levels < maxShrink */
#define PYR_MIN_OBJSIZE        10    /* minimum x-/y-Object size due to shrink */
#define PYR_SHRINK_MAX_LEVEL    5    /* maximal nr of shrinks (with 1:1) */

#define MIN_DEV     1            /* min square deviation for vgeoest() */
#define MAX_RUNS    10           /* max runs for vgeoest() */

#define INTERPOL1              (char *)"0"
#define INTERPOL3_R            (char *)"3r"

#define CORR_MINTOL     1
#define CORR_TEACHTOL   3            // used if tol <

#define FSV_RED   0
#define FSV_GREEN 1
#define FSV_BLUE  2

// syserror codes

#define MY_SYSERR_BASE     (11 * 100)

#define SE_IMDEF          (MY_SYSERR_BASE+0)      // failed to define image
#define SE_ILLTYPE        (MY_SYSERR_BASE+1)      // illegal window type %d
#define SE_IMDEL          (MY_SYSERR_BASE+2)      // error %d deleting image
#define SE_VCRE           (MY_SYSERR_BASE+3)      // failed to create vector
#define SE_VALLOC         (MY_SYSERR_BASE+4)      // error %d allocating vector
#define SE_VREM           (MY_SYSERR_BASE+5)      // error %d removing vector
#define SE_VCOPY          (MY_SYSERR_BASE+6)      // error %d in vcopy()
#define SE_IMCRE          (MY_SYSERR_BASE+7)      // failed to create image
#define SE_IMREM          (MY_SYSERR_BASE+8)      // error %d removing image
#define SE_IMOVE          (MY_SYSERR_BASE+9)      // error %d moving image
#define SE_CRCDF          (MY_SYSERR_BASE+10)     // error %d in correlation
#define SE_ISHRINK        (MY_SYSERR_BASE+11)     // error %d shrinking image
#define SE_TXYQUAL        (MY_SYSERR_BASE+12)     // error %d in cdf2_txyqual()
#define SE_IFILT3         (MY_SYSERR_BASE+13)     // error %d in ifilt3()
#define SE_ILLSUBWIN      (MY_SYSERR_BASE+14)     // illegal subwindow number %d
#define SE_ILLNPOINT      (MY_SYSERR_BASE+15)     // illegal number of points
#define SE_VGEOEST        (MY_SYSERR_BASE+16)     // error %d in vgeoest()
#define SE_ILLWINPARSIZ   (MY_SYSERR_BASE+17)     // illegal window parameter structure size %d
#define SE_ILLWINRESSIZ   (MY_SYSERR_BASE+18)     // illegal window result structure size %d
#define SE_ILLSWINPARSIZ  (MY_SYSERR_BASE+19)     // illegal subWindow parameter structure size %d
#define SE_MEMALLOC       (MY_SYSERR_BASE+20)     // memory allocation failed
#define SE_PYRERR         (MY_SYSERR_BASE+21)     // internal error %d in pyramid calculation
#define SE_NO_WORK_DATA   (MY_SYSERR_BASE+22)     // No work data allocated
#define SE_WIN_BORDER     (MY_SYSERR_BASE+23)     // Error window border
#define SE_ERR_UNIT       (MY_SYSERR_BASE+24)     // Error illegal unit

/* ---------------------- inspection error codes ------------------------- */

#define PPCWINTR1_INSPERR_OK          0       // window OK
#define PPCWINTR1_INSPERR_TOLERANCE   1       // Over max position deviation. Must be value 1.
#define PPCWINTR1_INSPERR_POS         2       // position error
#define PPCWINTR1_INSPERR_SEARCRANGE  3       // position tolerance error
#define PPCWINTR1_INSPERR_WINBORDER   4       // touches image border
#define PPCWINTR1_INSPERR_ILLTRAFO    5       // unable to compute trafo system (illegal window positions)
#define PPCWINTR1_INSPERR_TEACH_ERR   6       // error on teaching the window
#define PPCWINTR1_INSPERR_OTHER       7       // Any other inspection error

// check errors

#define CHECK_ERR_IMBORDER    10   // window touches image border
#define CHECK_ERR_BPAT        11   // bad pattern
#define CHECK_ERR_BPATCONTR   12   // bad pattern contrast
#define CHECK_ERR_PATNUNIQ    13   // pattern not unique
#define CHECK_ERR_PATNUSAB    14   // pattern not usable
#define CHECK_ERR_WINSIZE     15   // window too big

/* ---------------------- structures ------------------------------------- */

typedef struct {
  int xo, yo, xxo, yyo;     // unshrinked object position and size, rounded to multiple of shrink
  int shrinkX, shrinkY;     // shrink factors for this pyramid level
  Timages *iAllocObject;    // allocated object on heap (with maximum size due to round to multiple of shrink)
  Timages *iObject;         // object on heap (without wr_correction)
} T_pyramidLevel_TR1;

typedef struct {
  double PosToleranceX;     // x-Tolerance in units
  double PosToleranceY;     // y-Tolerance in units
  int32 xTol;               // x-Tolerance in pixels
  int32 yTol;               // y-Tolerance in pixels
  int uniX;                 // if TRUE: unidirectional correlation in X
  int uniY;                 // if TRUE: unidirectional correlation in Y
  int maxShrinkXY;          // 0: no correlation needed, else maximum shrink level for X or Y direction
  int maxShrinkX;           // 0: no correlation needed, else maximum shrink level for X direction
  int maxShrinkY;           // 0: no correlation needed, else maximum shrink level for Y direction
  int nLevels;              // number of shrink levels (including unshrinked level)
} T_pyramidDesc_TR1;

typedef struct {
  T_pyramidDesc_TR1 pyrDesc;  // pyramid description
  T_pyramidLevel_TR1 *pPyr;   // the pyramid itself
  T_WinDesc lastWinDesc;      // position and size in last inspection run
  double lastPosToleranceX;   // last x-Tolerance in units
  double lastPosToleranceY;   // last y-Tolerance in units
  int32 lastUniX;             // uniX in last inspection run
  int32 lastUniY;             // uniY in last inspection run
} Tsip_SubWinTR1;

typedef struct {
  Tvector *vin;
  Tvector *vref;
  Tvector *vtmp;
  Tvector *v1t;
  Tvector *vcorrel;
  Tvector *vmat;         // local transform system
  int lastNSubWin;
  int lastActive;        // last active flag

  Tsip_SubWinTR1 *pSubWin; // point to TR1_MAX_SUBWIN allocated data members
} Tsip_winTR1;


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

// Other

#define INSPUTIL_ERR_IMOVE    -300
#define INSPUTIL_ERR_ISHRINK  -301

/************************************************************************************
* inspUtil_imoveImage
*
* Move a color component to an int32 image
*
*/

static int inspUtil_imoveImage( Fl_RGB_Image *pSrc, int AoiX, int AoiY, int AoiXX, int AoiYY,
                                Timages *dstim, int colorSel)
{
  register int xx, yy, x, y, d, ld;
  uchar *ps, *pSline;
  int32 *pd;

  // Get data from source image

  d  = pSrc->d();
  xx = pSrc->data_w();
  yy = pSrc->data_h();
  ld = pSrc->ld() ? pSrc->ld() : pSrc->data_w() * pSrc->d();
  ps = (uchar *)pSrc->data()[ 0];

  if( AoiX < 0 || AoiY < 0 ||    // Check AOI
      AoiX + AoiXX > xx ||
      AoiY + AoiYY > yy) {

    return( -1);
  }

  if ( uticheck( dstim, DV_HOST, TY_INT32) ) {   // Check destination type

    return(-2);
  }

  if( pSrc->d() >= 3) {          // is a RGB image

    if( colorSel < 0 ||          // Check colorSel range
       colorSel > 2) {

      return( -3);
    }

  } else if( pSrc->d() >= 1) {   // is a BW image

    colorSel = 0;                 // use first byte

  } else {

    return( INSPUTIL_ERR_IMOVE);
  }

  pSline = ps + AoiY * ld + AoiX * d;

  for( y = 0; y < AoiYY; y++) {

    ps = pSline;

    pd = (int32 *)pixadt ( 0, y, dstim, int32);

    for( x = 0; x < AoiXX; x++ ) {

      *pd++ = ps[ colorSel];
      ps += d;
    }

    pSline += ld;
  }

  return( 0);    // Return OK
}

/************************************************************************************
* ishrink2_32to32_ex
*
* shrink factor 2 image 32->32 bit, to shrink in X/Y is argument
*
*/

static int ishrink2_32to32_ex( Timages *srcim, Timages *dstim, int shrinkInX, int shrinkInY)
{
  register int32 xx, yy, x, y;
  register int32 *ps0, *ps1;
  register int32 *pd;
  register int32 xms;

  if ( uticheck (srcim, DV_HOST, TY_INT32) ) return(-1);
  if ( uticheck (dstim, DV_HOST, TY_INT32) ) return(-2);

  if( shrinkInX && shrinkInY) {                 // shrink both directions

    xx = (int32) getxx(srcim) >> 1;
    yy = (int32) getyy(srcim) >> 1;
    if (getxx(dstim) < xx) xx = getxx(dstim);
    if (getyy(dstim) < yy) yy = getyy(dstim);

    xms = getxm(srcim);

    for (y = 0; y < yy; y++) {
      ps0 = (int32 *) pixadt(0, y << 1, srcim, int32);
      ps1 = ps0 + xms;
      pd  = (int32 *) pixadt(0, y, dstim, int32);
      for (x = 0; x < xx; x++) {
        *pd++ = (ps0[0] + ps0[1] + ps1[0] + ps1[1] + 2) >> 2;
        ps0 += 2;
        ps1 += 2;
      }
    }

    return(0);                                  // done
  }

  if( shrinkInX) {                              // shrink only X directions

    xx = (int32) getxx(srcim) >> 1;
    yy = (int32) getyy(srcim);
    if (getxx(dstim) < xx) xx = getxx(dstim);
    if (getyy(dstim) < yy) yy = getyy(dstim);

    xms = getxm(srcim);

    for (y = 0; y < yy; y++) {
      ps0 = (int32 *) pixadt(0, y, srcim, int32);
      pd  = (int32 *) pixadt(0, y, dstim, int32);
      for (x = 0; x < xx; x++) {
        *pd++ = (ps0[0] + ps0[1] + 1) >> 1;
        ps0 += 2;
      }
    }

    return(0);                                  // done
  }

  if( shrinkInY) {                              // shrink only Y directions

    xx = (int32) getxx(srcim);
    yy = (int32) getyy(srcim) >> 1;
    if (getxx(dstim) < xx) xx = getxx(dstim);
    if (getyy(dstim) < yy) yy = getyy(dstim);

    xms = getxm(srcim);

    for (y = 0; y < yy; y++) {
      ps0 = (int32 *) pixadt(0, y << 1, srcim, int32);
      ps1 = ps0 + xms;
      pd  = (int32 *) pixadt(0, y, dstim, int32);
      for (x = 0; x < xx; x++) {
        *pd++ = (ps0[0] + ps1[0] + 1) >> 1;
        ps0 += 1;
        ps1 += 1;
      }
    }

    return(0);                                  // done
  }

  // Shrink in no direction
  // This should not be, but for shandi's shake make a copy

  xx = (int32) getxx(srcim);
  yy = (int32) getyy(srcim);
  if (getxx(dstim) < xx) xx = getxx(dstim);
  if (getyy(dstim) < yy) yy = getyy(dstim);

  xms = getxm(srcim);

  for (y = 0; y < yy; y++) {
    ps0 = (int32 *) pixadt(0, y, srcim, int32);
    pd  = (int32 *) pixadt(0, y, dstim, int32);
    for (x = 0; x < xx; x++) {
      *pd++ = *ps0++;
    }
  }

  return(0);
}

static int inspUtil_ishrinkImageXY( Fl_RGB_Image *pSrc, int AoiX, int AoiY, int AoiXX, int AoiYY,
                             Timages *dst, int colorSel, int shrinkX, int shrinkY) /*
==================================================== */
{
  register int xx, yy, x, y, d, ld, xi, yi;
  int ierr, sum;
  double factor;
  uchar *ps, *psi, *psi2, *pSline;
  int32 *pd;

  ierr = 0;

  // Get data from source image

  d  = pSrc->d();
  xx = pSrc->data_w();
  yy = pSrc->data_h();
  ld = pSrc->ld() ? pSrc->ld() : pSrc->data_w() * pSrc->d();
  ps = (uchar *)pSrc->data()[ 0];

  if( AoiX < 0 || AoiY < 0 ||    // Check AOI
      AoiX + AoiXX > xx ||
      AoiY + AoiYY > yy) {

    return( -1);
  }

  if ( uticheck( dst, DV_HOST, TY_INT32) ) {   // Check destination type

    return(-2);
  }

  if( pSrc->d() >= 3) {          // is a RGB image

    if( colorSel < 0 ||          // Check colorSel range
       colorSel > 2) {

      return( -3);
    }

  } else if( pSrc->d() >= 1) {   // is a BW image

    colorSel = 0;                 // use first byte

  } else {

    return( INSPUTIL_ERR_IMOVE);
  }

  xx = xx / shrinkX;
  yy = yy / shrinkY;
  if( getxx( dst) < xx) xx = getxx(dst);
  if( getyy( dst) < yy) yy = getyy(dst);

  factor = 1.0 / ((double)(shrinkX * shrinkY));  // norm

  pSline = ps + AoiY * ld + AoiX * d;

  for (y = 0; y < yy; y++) {

    ps = pSline;

    pd = (int32 *) pixadt( 0, y, dst, int32);

    for (x = 0; x < xx; x++) {

      sum = 0;
      psi = ps;

      for( yi = 0; yi < shrinkY; yi++) {

        psi2 = psi;

        for( xi = 0; xi < shrinkX; xi++) {
          sum += psi2[ colorSel];
          psi2 += d;
        }
        psi += ld;
      }

      sum = dto32(factor * (double)sum);
      if (sum > 0xff) sum = 0xff;
      *pd++ = sum;
      ps += shrinkX * d;
    }

    pSline += shrinkY * ld;
  }

  if( ierr) {
    ierr = INSPUTIL_ERR_ISHRINK;
    goto exitPoint;
  }

  ierr = 0;

exitPoint:

  return(ierr);

} /* inspUtil_ishrinkImageXY() */

/************************************************************************************
* ishrink2_32to32_ex
*
* shrink factor 2 image 32->32 bit, to shrink in X/Y is argument
*
*/

static int imove_32to32_wr( Timages *srcim, Timages *dstim, lfloat wr_factor) /* move image 32->32 bit
===========================================================================     make mult. correction with wr */
{
  register int32 xx, yy, x, y;
  register int32 *ps;
  register int32 *pd;
  register int32 help;

  xx = (int32) getxx(srcim);
  yy = (int32) getyy(srcim);

  if ( uticheck (srcim, DV_HOST, TY_INT32) )  return(-1);
  if ( uticheck (dstim, DV_HOST, TY_INT32) )  return(-2);
  if (xx > getxx(dstim) || yy > getyy(dstim)) return (-3);

  for ( y = 0L; y < yy; y++ ) {
    ps = (int32 *) pixadt ( 0, y, srcim, int32 );
    pd = (int32 *) pixadt ( 0, y, dstim, int32 );
    for ( x = 0L; x < xx; x++ ) {
      help = dto32( wr_factor * (lfloat)*ps++);
      if (help > 0xff) help = 0xff;
      *pd++ = help;
    }
  }

  return(0);

} /* int imove_32to32_wr() */

/************************************************************************************
* inspUtil_imDefineTolEx2ClipPixelCenter
*
* Define child on parent with tolerance, transform centerpoint,
* clip tolerance area to imParent,
* take pixel center to be as near as possible to correct position
*
*/

static int inspUtil_imDefineTolEx2ClipPixelCenter(
                              Fl_RGB_Image *imParent,          // Define AOI on this
                             int *pAoiX, int *pAoiY, int *pAoiXX, int *pAoiYY,  // Out AOI
                             Tvector *vmat,
                             T_WinDesc *pWinDesc,
                             int16 unitX, int16 unitY,   /* tolerance is rounded to multiples of unit */
                             int16 *xTolP, int16 *xTolN, /* modified in case of clipping */
                             int16 *yTolP, int16 *yTolN, /* modified in case of clipping */
                             int16 *xci, int16 *yci, double *xf, double *yf,
                             int16 *x, int16 *y)
{
  sfloat *pmat;
  double xc, yc;                                   /* center point */
  int16 xt, yt, xx, yy;

  pmat = (sfloat *)vgetpm(vmat);

  xc = (double)(pWinDesc->XPos + (pWinDesc->XSize + 1) / 2);
  yc = (double)(pWinDesc->YPos + (pWinDesc->YSize + 1) / 2);

  // NOTE: take pixel center (dto16) to be as near as possible to correct position
  *xf   = pmat[0] * xc + pmat[1] * yc + pmat[4] / 16.0;
  *xci  = dto16(*xf);
  *yf   = pmat[2] * xc + pmat[3] * yc + pmat[5] / 16.0;
  *yci  = dto16(*yf);

  *x = *xci - (pWinDesc->XSize + 1) / 2;                      /* corner point */
  *y = *yci - (pWinDesc->YSize + 1) / 2;

  /* check that image without tolerance is completely inside parent */
  if( *x < 0 || (*x + pWinDesc->XSize) > imParent->w() ||
      *y < 0 || (*y + pWinDesc->YSize) > imParent->h()) {

    return( -SE_WIN_BORDER);
  }

  if (unitX <= 0 || unitY <= 0) {

    return( -SE_ERR_UNIT);
  }

  *xTolP = (*xTolP / unitX) * unitX;  // be sure, make multiple of unit
  *xTolN = (*xTolN / unitX) * unitX;
  *yTolP = (*yTolP / unitY) * unitY;
  *yTolN = (*yTolN / unitY) * unitY;

  /* now clip tolerance range to imParent */
  xt = *x - *xTolN;
  if (xt < 0) {
    *xTolN = (*x / unitX) * unitX;
    xt = *x - *xTolN;
  }
  xx = pWinDesc->XSize + *xTolN + *xTolP;
  if (xt + xx > imParent->w()) {
    *xTolP = ((imParent->w() - xt - pWinDesc->XSize - *xTolN) / unitX) * unitX;
    xx = pWinDesc->XSize + *xTolN + *xTolP;
  }

  yt = *y - *yTolN;
  if (yt < 0) {
    *yTolN = (*y / unitY) * unitY;
    yt = *y - *yTolN;
  }
  yy = pWinDesc->YSize + *yTolN + *yTolP;
  if (yt + yy > imParent->h()) {
    *yTolP = ((imParent->h() - yt - pWinDesc->YSize - *yTolN) / unitY) * unitY;
    yy = pWinDesc->YSize + *yTolN + *yTolP;
  }

#ifdef use_again
  int ierr;

  if (ierr = inspUtil_RangeCheckAndDefine(im, imParent, xt, yt, xx, yy)) {
    return(ierr);
  }
#else
  /* if touching the image border, return with special error */
  if (xt < 0 || (xt + xx) > imParent->w() ||
      yt < 0 || (yt + yy) > imParent->h()) {

    return( -SE_WIN_BORDER);
  }

  *pAoiX = xt;
  *pAoiY = yt;
  *pAoiXX = xx;
  *pAoiYY = yy;
#endif

  return(0);

} /* int inspUtil_imDefineTolEx2ClipPixelCenter() */

/************************************************************************************
* gcrcdf32
*
*/

#define ABSTAB 1              // define this to use a table for making absolut value

#ifdef ABSTAB
static int *pabsDiff8Tab0 = (int *)NULL;
static int absDiff8Tab[511];
#endif

static int gcrcdf32( Timages *objim, Timages *scnim, Timages *corim, Tvector *bestcorvec)
{
  register int32 sum2, hlp, ox;
  register int32 *pScene, *pObj;
  int32 oy;
  int32 xmo, xms, xmc;
  int32 xxo, yyo, xxc, yyc;
  int32 bestsum, bestx, besty;
  int32 cy, cx;
  int32 *pSceneLine, *pSceneLineScene, *pScene2;
  sfloat *pCorrLine, *pCorr;
  int32 *pObjLine;
  Tffuncval *bestval;

  /* PRINTF0("FSV: gcrcdf32()\n"); */
  /*-------------- check input parameters of correlation function ----*/
  if (utvcheck(bestcorvec, DV_HOST, TY_FUNCVAL)) {
    return(-1);
  }

  if (ve_alloc(bestcorvec, 3L, sizeof(Tffuncval), TY_FUNCVAL)) {
    return(-2);
  }

  if (uticheck(objim, DV_HOST, TY_INT32)) return(-3);
  if (uticheck(scnim, DV_HOST, TY_INT32)) return(-3);
  if (uticheck(corim, DV_HOST, TY_SFLOAT)) return(-3);

  if ((getxx(objim) * (int32)getyy(objim)) > 0x007fffff) {
    errstring =  (char *)"object image too large";
    return(-20);
  }

  if (getxx(objim) > getxx(scnim) || getyy(objim) > getyy(scnim)) {
    errstring = (char *)"object larger than scene";
    return(-22);
  }

  bestval = (Tffuncval *)vgetpm(bestcorvec);

  /*----------------------- INITIALIZE -------------------------------*/

#ifdef ABSTAB
  if (pabsDiff8Tab0 == (int *)NULL) {
    pabsDiff8Tab0 = absDiff8Tab;
    for (ox = 255; ox >= 0; ox--) {
      *pabsDiff8Tab0++ = ox;
    }
    for (ox = 1; ox <= 255; ox++) {
      *pabsDiff8Tab0++ = ox;
    }
    pabsDiff8Tab0 = &absDiff8Tab[255];
  }
#endif

  bestx = besty = 0;
  bestsum = MAXINT32;

  /* TRICK: the row offsets are multiplied with 4 and the increment of pointers
     is done with a (int8 *) cast, so the processor doesn't shift the offset
     before each pointer increment */
  xmo = (int32)getxm(objim);
  xms = (int32)getxm(scnim);
  xxo = (int32)getxx(objim);
  yyo = (int32)getyy(objim);

  /*------- now the parameters related to correlation image ----------*/

  xxc = (int32)getxx(scnim) - xxo + 1;
  yyc = (int32)getyy(scnim) - yyo + 1;

  /* PRINTF2("FSV: xxc %d yyc %d\n", xxc, yyc); */
  /* PRINTF2("FSV: xxo %d yyo %d\n", xxo, yyo); */

  xmc = (int32)getxm(corim);
  if ((int32)getxx(corim) < xxc) return(-20);
  if ((int32)getyy(corim) < yyc) return(-20);

  /* PRINTF3("FSV: xmo %d xmc %d xms %d\n", xmo, xmc, xms); */

  /* ----------------- Zero result ------------------------------------- */

  pCorrLine = pixadt(0, 0, corim, sfloat);
  for(cy = 0; cy < yyc; cy++) {
    pCorr = pCorrLine;
    for (cx = 0; cx < xxc; cx++) {
      *((int *)pCorr) = 0;
      pCorr++;
    }
    pCorrLine += xmc;
  }

#ifdef ABSTAB
  /* ----------------- clip scene/obj ----------------------------------- */

  for(cy = 0; cy < getyy(scnim); cy++) {
    pScene = (int32 *)pixadt(0, cy, scnim, int32);
    for (cx = 0; cx < getxx(scnim); cx++) {
      *pScene++ &= 0xff;
    }
  }

  for(cy = 0; cy < getyy(objim); cy++) {
    pObj = (int32 *)pixadt(0, cy, objim, int32);
    for (cx = 0; cx < getxx(objim); cx++) {
      *pObj++ &= 0xff;
    }
  }
#endif

  /* ----------------- STANDARD CORRELATION ----------------------------- */

  pCorrLine = pixadt(0, 0, corim, sfloat);
  pSceneLine = (int32*) pixadt(0, 0, scnim, int32);

  for (cy = 0; cy < yyc; cy++) {

    if (iabort())
      break; /* abort by CNTRL C */

    pSceneLineScene = pSceneLine;
    pObjLine = (int32*) pixadt(0, 0, objim, int32);

    for (oy = 0; oy < yyo; oy++) {

      pScene2 = pSceneLineScene;
      pCorr = pCorrLine;

      for (cx = 0; cx < xxc; cx++) {

        sum2 = 0;
        pScene = pScene2;
        pObj = pObjLine;

#ifdef use_again
          for(ox = 0; ox < xxo; ox++) {
            hlp = *pObj++ - *pScene++;
            if (hlp >= 0) sum2 += hlp;
            else          sum2 -= hlp;
          }
#else
#ifndef ABSTAB
        ox = xxo;
        while (ox >= 8) {
          hlp = pObj[0] - pScene[0];
          sum2 += hlp >= 0 ? hlp : -hlp;
          hlp = pObj[1] - pScene[1];
          sum2 += hlp >= 0 ? hlp : -hlp;
          hlp = pObj[2] - pScene[2];
          sum2 += hlp >= 0 ? hlp : -hlp;
          hlp = pObj[3] - pScene[3];
          sum2 += hlp >= 0 ? hlp : -hlp;
          hlp = pObj[4] - pScene[4];
          sum2 += hlp >= 0 ? hlp : -hlp;
          hlp = pObj[5] - pScene[5];
          sum2 += hlp >= 0 ? hlp : -hlp;
          hlp = pObj[6] - pScene[6];
          sum2 += hlp >= 0 ? hlp : -hlp;
          hlp = pObj[7] - pScene[7];
          sum2 += hlp >= 0 ? hlp : -hlp;
          ox -= 8;
          pObj += 8;
          pScene += 8;
        }
        // have 0 .. 7
        if (ox >= 4) {
          hlp = pObj[0] - pScene[0];
          sum2 += hlp >= 0 ? hlp : -hlp;
          hlp = pObj[1] - pScene[1];
          sum2 += hlp >= 0 ? hlp : -hlp;
          hlp = pObj[2] - pScene[2];
          sum2 += hlp >= 0 ? hlp : -hlp;
          hlp = pObj[3] - pScene[3];
          sum2 += hlp >= 0 ? hlp : -hlp;
          ox -= 4;
          pObj += 4;
          pScene += 4;
        }
        if (ox >= 2) {
          hlp = pObj[0] - pScene[0];
          sum2 += hlp >= 0 ? hlp : -hlp;
          hlp = pObj[1] - pScene[1];
          sum2 += hlp >= 0 ? hlp : -hlp;
          ox -= 2;
          pObj += 2;
          pScene += 2;
        }
        if (ox >= 1) {
          hlp = *pObj - *pScene;
          sum2 += hlp >= 0 ? hlp : -hlp;
        }
#else
          ox = xxo;
          while (ox >= 8) {
            sum2 += pabsDiff8Tab0[pObj[ 0] - pScene[ 0]];
            sum2 += pabsDiff8Tab0[pObj[ 1] - pScene[ 1]];
            sum2 += pabsDiff8Tab0[pObj[ 2] - pScene[ 2]];
            sum2 += pabsDiff8Tab0[pObj[ 3] - pScene[ 3]];
            sum2 += pabsDiff8Tab0[pObj[ 4] - pScene[ 4]];
            sum2 += pabsDiff8Tab0[pObj[ 5] - pScene[ 5]];
            sum2 += pabsDiff8Tab0[pObj[ 6] - pScene[ 6]];
            sum2 += pabsDiff8Tab0[pObj[ 7] - pScene[ 7]];
            ox -= 8;
            pObj += 8;
            pScene += 8;
          }
          // have 0 .. 7
          if (ox >= 4) {
            sum2 += pabsDiff8Tab0[pObj[ 0] - pScene[ 0]];
            sum2 += pabsDiff8Tab0[pObj[ 1] - pScene[ 1]];
            sum2 += pabsDiff8Tab0[pObj[ 2] - pScene[ 2]];
            sum2 += pabsDiff8Tab0[pObj[ 3] - pScene[ 3]];
            ox -= 4;
            pObj += 4;
            pScene += 4;
          }
          if (ox >= 2) {
            sum2 += pabsDiff8Tab0[pObj[ 0] - pScene[ 0]];
            sum2 += pabsDiff8Tab0[pObj[ 1] - pScene[ 1]];
            ox -= 2;
            pObj += 2;
            pScene += 2;
          }
          if (ox >= 1) {
            sum2 += pabsDiff8Tab0[pObj[ 0] - pScene[ 0]];
          }
#endif
#endif

        *((int*) pCorr) += sum2;
        pCorr++;
        pScene2++;
      }

      pObjLine += xmo;
      pSceneLineScene += xms;
    }
    pCorrLine += xmc;
    pSceneLine += xms;
  }

  /* ----------------- get best result ------------------------------------- */

  pCorrLine = pixadt(0, 0, corim, sfloat);
  for(cy = 0; cy < yyc; cy++) {
    pCorr = pCorrLine;

    for (cx = 0; cx < xxc; cx++) {
      hlp = *((int *)pCorr);
      if (hlp < bestsum) {
        bestx = cx;
        besty = cy;
        bestsum = hlp;
      }
      *pCorr = (sfloat)hlp;   // final covert to float
      pCorr++;
    }
    pCorrLine += xmc;
  }

  bestval->fxpos    = (int16)bestx;
  bestval->fypos    = (int16)besty;
  bestval->ffuncval = (sfloat)bestsum;

  vputnm( bestcorvec,(int32)2L);
  PRINTF3("FSV: x %d y %d val %f\n", bestval->fxpos, bestval->fypos,
           bestval->ffuncval);

  return(0);
}

/************************************************************************************
* Pyramid processing functions
*
*/

static int calculatePyramid( YaIPS_PSearchAOI_t *pSubWin, T_pyramidDesc_TR1 *pPyrDesc)
{
  double nCalcs;              // # of calculations in correlation loop, double to prevent int32 overflow

  pPyrDesc->PosToleranceX = (double)pSubWin->PosToleranceX;
  if (pSubWin->Flags & PROJPAR_WINFLAG_XNACTIVE) {
    pPyrDesc->PosToleranceX = 0.0;
  }
  pPyrDesc->PosToleranceY = (double)pSubWin->PosToleranceY;
  if (pSubWin->Flags & PROJPAR_WINFLAG_YNACTIVE) {
    pPyrDesc->PosToleranceY = 0.0;
  }

#ifdef use_again
  pPyrDesc->xTol = dto16(pPyrDesc->PosToleranceX * (double)ProjTrafo[ refNr][A11]);  // * ScaleX
  pPyrDesc->yTol = dto16(pPyrDesc->PosToleranceY * (double)ProjTrafo[ refNr][A22]);  // * ScaleY
#else
  pPyrDesc->xTol = dto16(pPyrDesc->PosToleranceX / YaIPS_Calib_UPP_X);  // * ScaleX
  pPyrDesc->yTol = dto16(pPyrDesc->PosToleranceY / YaIPS_Calib_UPP_Y);  // * ScaleY
#endif

  if (pPyrDesc->PosToleranceX <= 0.0) {
    pPyrDesc->uniY = TRUE;
  } else {
    pPyrDesc->uniY = FALSE;
  }

  if (pPyrDesc->PosToleranceY <= 0.0) {
    pPyrDesc->uniX = TRUE;
  } else {
    pPyrDesc->uniX = FALSE;
  }

#ifdef use_again
  if( pPyrDesc->PosToleranceX * (double)ProjTrafo[ refNr][A11] <= 0.0 &&
      pPyrDesc->PosToleranceY * (double)ProjTrafo[ refNr][A22] <= 0.0) {
#else
    if( pPyrDesc->PosToleranceX / YaIPS_Calib_UPP_X <= 0.0 &&
        pPyrDesc->PosToleranceY / YaIPS_Calib_UPP_Y <= 0.0) {
#endif

    pPyrDesc->maxShrinkXY = 0;
    pPyrDesc->maxShrinkX  = 0;
    pPyrDesc->maxShrinkY  = 0;
    pPyrDesc->nLevels     = 0;
    pPyrDesc->uniX        = TRUE;
    pPyrDesc->uniY        = TRUE;
    return(0);               // no correlation needed
  }

  if (pPyrDesc->uniY == FALSE && pPyrDesc->xTol < 1) pPyrDesc->xTol = 1;
  if (pPyrDesc->uniX == FALSE && pPyrDesc->yTol < 1) pPyrDesc->yTol = 1;

  // calculate shrink factor

  pPyrDesc->nLevels     = 1;  // init for unschrinked
  pPyrDesc->maxShrinkXY = 1;  // no shrink
  pPyrDesc->maxShrinkX  = 1;
  pPyrDesc->maxShrinkY  = 1;

  for( ; ; ) {
    int XSize2, YSize2, xTol2, yTol2, AnyShrink;

    // get window, tolerance sizes depented from shrink factors

    XSize2 = ((pSubWin->AOI.XSize + pPyrDesc->maxShrinkX / 2) / pPyrDesc->maxShrinkX);               // size of window XX depented from shrinkX factor
    YSize2 = ((pSubWin->AOI.YSize + pPyrDesc->maxShrinkY / 2) / pPyrDesc->maxShrinkY);               // size of window YY depented from shrinkY factor

    xTol2  = ((pPyrDesc->xTol + pPyrDesc->maxShrinkX / 2) / pPyrDesc->maxShrinkX);      // size of window X search tolerance from shrinkX factor
    if( xTol2 < 1) {  // security test, not lower than this
      xTol2 = 1;
    }

    yTol2  = ((pPyrDesc->yTol + pPyrDesc->maxShrinkY / 2) / pPyrDesc->maxShrinkY);      // size of window Y search tolerance from shrinkY factor
    if( yTol2 < 1) {  // security test, not lower than this
      yTol2 = 1;
    }

    nCalcs = (double)XSize2 * (double)YSize2 * (double)(2 * xTol2 + 1) * (double)(2 * yTol2 + 1);

#ifdef use_again
    if ( pPyrDesc->nLevels <= 1 && Trace.traceDebug <= TRACE_SEVERE_ERROR) {

      PRINTL2( TRACE_SEVERE_ERROR, "calcPyramid TR1: Shrinkfactor X %d Y %d\n",
        pPyrDesc->maxShrinkX, pPyrDesc->maxShrinkY);

      PRINTL5( TRACE_SEVERE_ERROR, "   size %d * %d  Tol %d/%d = %.0lf\n",
        XSize2, YSize2, 2 * xTol2 + 1, 2 * yTol2 + 1, nCalcs);
    } else {

      PRINTL2( TRACE_DEBUG, "calcPyramid TR1: Shrinkfactor X %d Y %d\n",
        pPyrDesc->maxShrinkX, pPyrDesc->maxShrinkY);

      PRINTL5( TRACE_DEBUG, "   size %d * %d  Tol %d/%d = %.0lf\n",
        XSize2, YSize2, 2 * xTol2 + 1, 2 * yTol2 + 1, nCalcs);
    }
#endif

    if (nCalcs < (double)PYR_SHRINK_SWITCH) {             // estimated calculation time is in range ?

      PRINTL0( TRACE_DEBUG, "   nCalcs below PYR_SHRINK_SWITCH, DONE\n");
      break;                                              // done
    }

    if( pPyrDesc->nLevels > PYR_SHRINK_MAX_LEVEL) {   // clip to this

      PRINTL1( TRACE_DEBUG, "   nLevels %d at maximum, DONE\n", pPyrDesc->nLevels);
      break;                                              // done
    }

    // ...

    AnyShrink = FALSE;                                    // preset no shrink of X or Y

    if( XSize2 >= PYR_MIN_OBJSIZE * 2) {                  // space left to shrink in X

      PRINTL2( TRACE_DEBUG, "   xx %d >= MIN_OBJSIZE %d --> shrink X\n", XSize2, PYR_MIN_OBJSIZE * 2);

      AnyShrink = TRUE;                                   // any shrink done
      pPyrDesc->maxShrinkX *= 2;                          // higher shrinkX factor
    }

    if( YSize2 >= PYR_MIN_OBJSIZE * 2) {                  // space left to shrink in Y

      PRINTL2( TRACE_DEBUG, "   xx %d >= MIN_OBJSIZE %d --> shrink Y\n", YSize2, PYR_MIN_OBJSIZE * 2);

      AnyShrink = TRUE;                                   // any shrink done
      pPyrDesc->maxShrinkY *= 2;                          // higher shrinkY factor
    }

    if( ! AnyShrink) {                                    // can't shrink any of the sides

      PRINTL1( TRACE_DEBUG, "   NO shrink, DONE\n", pPyrDesc->nLevels);
      break;                                              // done
    }

    // ...

    pPyrDesc->nLevels += 1;                               // have one level more

    if( pPyrDesc->maxShrinkX > pPyrDesc->maxShrinkY) {    // which one is higher

      pPyrDesc->maxShrinkXY = pPyrDesc->maxShrinkX;       // this is the maximum
    } else {

      pPyrDesc->maxShrinkXY = pPyrDesc->maxShrinkY;       // this is the maximum
    }
  }

  PRINTL1( TRACE_SEVERE_ERROR, "calcPyramid TR1: nLevels %d\n", pPyrDesc->nLevels);
  PRINTL2( TRACE_SEVERE_ERROR, "calcPyramid TR1: max. shrink %d/%d\n", pPyrDesc->maxShrinkX, pPyrDesc->maxShrinkY);

  return(0);

} /* static int calculatePyramid() */

static int initPyramid( YaIPS_PSearchAOI_t *pSubWin, Fl_RGB_Image *piref, T_pyramidDesc_TR1 *pPyrDesc, T_pyramidLevel_TR1 *pPyr)
{
  int ierr;
  int shrinkX, shrinkY, level;
  T_pyramidLevel_TR1 *pActPyr, *pLastPyr;
#ifdef use_again
  Timages *irTR1 = IMNULL;        // object on reference for correlation
#endif
  int xxMax, yyMax;

  if (pPyrDesc->maxShrinkXY <= 0 || pPyrDesc->nLevels <= 0) {
    return(0);                    // nothing to do
  }

  // determine object position and size in all shrink levels
  xxMax = -1;
  yyMax = -1;

  shrinkX = 1;
  shrinkY = 1;

  for ( level = 0; level < pPyrDesc->nLevels; level++) {

    pActPyr = &pPyr[level];

    // shrink factors per direction

    if( level > 0) {                             // higher than first level

      // higher shrink factors after first level

      if( shrinkX < pPyrDesc->maxShrinkX) {      // below limit for this direction

        shrinkX <<= 1;                           // can shrink more
      }

      if( shrinkY < pPyrDesc->maxShrinkY) {      // below limit for this direction

        shrinkY <<= 1;                           // can shrink more
      }
    }

    pActPyr->shrinkX = shrinkX;                  // save shrink factors for this pyramid level
    pActPyr->shrinkY = shrinkY;

    PRINTL3( TRACE_DEBUG, "initPyramid: Shrinkfactor %d: X %d Y %d\n",
                  level, pActPyr->shrinkX, pActPyr->shrinkY);

    // ...

    pActPyr->xo  = pSubWin->AOI.XPos;
    pActPyr->yo  = pSubWin->AOI.YPos;
    pActPyr->xxo = pSubWin->AOI.XSize;
    pActPyr->yyo = pSubWin->AOI.YSize;

    pActPyr->xxo = ((pActPyr->xxo + shrinkX / 2) / shrinkX) * shrinkX; // round to multiple of shrinkX
    if (pActPyr->xo + pActPyr->xxo > piref->w() /*getxx(piref)*/) {
      pActPyr->xxo -= shrinkX;
      if (pActPyr->xxo <= 0) {
        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_PYRERR, 1, "internal error %d in pyramid calculation");
        ierr = -SE_PYRERR;
        goto syserrorExit;
      }
    }
    pActPyr->yyo = ((pActPyr->yyo + shrinkY / 2) / shrinkY) * shrinkY; // round to multiple of shrinkY
    if (pActPyr->yo + pActPyr->yyo > piref->h() /*getyy(piref)*/) {
      pActPyr->yyo -= shrinkY;    // 15.11.2011 RR: Bugfix Systemerror 'image not inside memory', this line was missing
      if (pActPyr->yyo <= 0) {
        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_PYRERR, 2, "internal error %d in pyramid calculation");
        ierr = -SE_PYRERR;
        goto syserrorExit;
      }
    }

    if (xxMax == -1 || pActPyr->xxo > xxMax) {
      xxMax = pActPyr->xxo;
    }
    if (yyMax == -1 || pActPyr->yyo > yyMax) {
      yyMax = pActPyr->yyo;
    }
  }

  // --------------- extract first pyramid level (original unshrinked) -----------------

  level = 0;
  pActPyr = &pPyr[level];

#ifdef use_again
  if ((irTR1 = im_udefine( piref, pSubWin->AOI.XPos, pSubWin->AOI.YPos, xxMax, yyMax)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMDEF, 0, "failed to define image");
    ierr = -SE_IMDEF;
    goto syserrorExit;
  }

  if ((pActPyr->iAllocObject = im_ucreate(TY_INT32, DV_HOST, xxMax, yyMax, 0, 0, 32, 0)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
    ierr = -SE_IMCRE;
    goto syserrorExit;
  }
  if ((pActPyr->iObject = im_udefine(pActPyr->iAllocObject, 0, 0, pActPyr->xxo, pActPyr->yyo)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMDEF, 0, "failed to define image");
    ierr = -SE_IMDEF;
    goto syserrorExit;
  }

  if (ierr = inspUtil_imoveImage( irTR1, pActPyr->iAllocObject, CONVERSION_STD, pSubWin->colorUsed)) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMOVE, ierr, "error %d moving image");
    ierr = -SE_IMOVE;
    goto syserrorExit;
  }

  im_delete(irTR1);
  irTR1 = IMNULL;
#else

  if ((pActPyr->iAllocObject = im_ucreateMem( NULL, TY_INT32, DV_HOST, xxMax, yyMax, NULL)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
    ierr = -SE_IMCRE;
    goto syserrorExit;
  }
  if ((pActPyr->iObject = im_udefineMem( NULL, pActPyr->iAllocObject, 0, 0, pActPyr->xxo, pActPyr->yyo)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMDEF, 0, "failed to define image");
    ierr = -SE_IMDEF;
    goto syserrorExit;
  }

  if( (ierr = inspUtil_imoveImage( piref, pSubWin->AOI.XPos, pSubWin->AOI.YPos, xxMax, yyMax,
                                  pActPyr->iAllocObject, pSubWin->colorUsed)) != 0) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMOVE, ierr, "error %d moving image");
    ierr = -SE_IMOVE;
    goto syserrorExit;
  }
#endif

  // --------------- and calculate the pyramid -----------------
  for( level = 1; level < pPyrDesc->nLevels; level++) {

    pLastPyr = pActPyr;      // 1 level larger than actual
    pActPyr = &pPyr[level];

    shrinkX = pActPyr->shrinkX;                  // get shrink factor for this pyramid level
    shrinkY = pActPyr->shrinkY;

    // --------------- create object on heap ----------------------

#ifdef use_again
    if ((pActPyr->iAllocObject = im_ucreate(TY_INT32, DV_HOST, xxMax / shrinkX,
                                            yyMax / shrinkY, 0, 0, 32, 0)) == IMNULL) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
      ierr = -SE_IMCRE;
      goto syserrorExit;
    }
    if ((pActPyr->iObject = im_udefine(pActPyr->iAllocObject, 0, 0, pActPyr->xxo / shrinkX, pActPyr->yyo / shrinkY)) == IMNULL) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMDEF, 0, "failed to define image");
      ierr = -SE_IMDEF;
      goto syserrorExit;
    }

    // shrink one pyramid level
    if (ierr = ishrink2_32to32_ex(pLastPyr->iAllocObject, pActPyr->iAllocObject, pActPyr->shrinkX != pLastPyr->shrinkX, pActPyr->shrinkY != pLastPyr->shrinkY)) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_ISHRINK, ierr, "failed %d shrinking image");
      ierr = -SE_ISHRINK;
      goto syserrorExit;
    }
#else

    if ((pActPyr->iAllocObject = im_ucreateMem( NULL, TY_INT32, DV_HOST, xxMax / shrinkX, yyMax / shrinkY, NULL)) == IMNULL) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
      ierr = -SE_IMCRE;
      goto syserrorExit;
    }
    if ((pActPyr->iObject = im_udefineMem( NULL, pActPyr->iAllocObject, 0, 0, pActPyr->xxo / shrinkX, pActPyr->yyo / shrinkY)) == IMNULL) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMDEF, 0, "failed to define image");
      ierr = -SE_IMDEF;
      goto syserrorExit;
    }

    // shrink one pyramid level
    if( (ierr = ishrink2_32to32_ex( pLastPyr->iAllocObject, pActPyr->iAllocObject,
                                    pActPyr->shrinkX != pLastPyr->shrinkX, pActPyr->shrinkY != pLastPyr->shrinkY)) != 0) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_ISHRINK, ierr, "failed %d shrinking image");
      ierr = -SE_ISHRINK;
      goto syserrorExit;
    }
 #endif
  }

  ierr = 0;

syserrorExit:

#ifdef use_again
  if (irTR1  != IMNULL)    im_delete(irTR1);
#endif

  return(ierr);

} /* static int initPyramid() */

static int endPyramid( T_pyramidDesc_TR1 *pPyrDesc, T_pyramidLevel_TR1 *pPyr)
{
  int level;
  T_pyramidLevel_TR1 *pActPyr;

  // free allocated ressources in all shrink levels
  for (level = pPyrDesc->nLevels - 1; level >= 0; level--) {
    pActPyr = &pPyr[level];

    // --------------- remove object on heap ----------------------
    if (pActPyr->iAllocObject != IMNULL) {
      im_remove(pActPyr->iAllocObject);
      pActPyr->iAllocObject = IMNULL;
    }
  }

  return(0);

} /* static int endPyramid() */

static int doInspInit( Fl_RGB_Image *piref, YaIPS_PSearchXY_t *pWin, Tsip_winTR1 *pSip) /*
=================================================================================== */
{
  int level, ierr, nAOIs;
  int subWinNr;
  YaIPS_PSearchAOI_t *pSubWin;

  if ((pSip->vin = ve_ucreate(DV_HOST)) == VENULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VCRE, 0, "failed to create vector");
    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  if( (ierr = ve_alloc(pSip->vin, TR1_MAX_SUBWIN * 2, sizeof(int32), TY_INT32)) != 0) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VALLOC, ierr, "error %d allocating vector");
    ierr = -SE_VALLOC;
    goto syserrorExit;
  }
  if ((pSip->vref = ve_ucreate(DV_HOST)) == VENULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VCRE, 0, "failed to create vector");
    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  if( (ierr = ve_alloc(pSip->vref, TR1_MAX_SUBWIN * 2, sizeof(int32), TY_INT32)) != 0) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VALLOC, ierr, "error %d allocating vector");
    ierr = -SE_VALLOC;
    goto syserrorExit;
  }
  /* ------------------- vgeoest temporary vectors */
  if ((pSip->vtmp = ve_ucreate(DV_HOST)) == VENULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VCRE, 0, "failed to create vector");
    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  if( (ierr = ve_alloc(pSip->vtmp, 6, sizeof(lfloat), TY_LFLOAT)) != 0) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VALLOC, ierr, "error %d allocating vector");
    ierr = -SE_VALLOC;
    goto syserrorExit;
  }
  if ((pSip->v1t = ve_ucreate(DV_HOST)) == VENULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VCRE, 0, "failed to create vector");
    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  if( (ierr = ve_alloc(pSip->v1t, TR1_MAX_SUBWIN * 2, sizeof(lfloat), TY_LFLOAT)) != 0) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VALLOC, ierr, "error %d allocating vector");
    ierr = -SE_VALLOC;
    goto syserrorExit;
  }
  if( (pSip->vcorrel = ve_ucreate(DV_HOST)) == VENULL) {
    //x/sysstate_setsyserr(SYSERR_MODULE_IP, SEIP_VCRE, 0, NULL);      // failed to create vector
    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  if( (ierr = ve_alloc( pSip->vcorrel, 3, sizeof(Tffuncval), TY_FUNCVAL)) != 0) {
    //x/sysstate_setsyserr(SYSERR_MODULE_IP, SEIP_VALLOC, ierr, NULL);   // error %d allocating vector
    ierr = -SE_VALLOC;
    goto syserrorExit;
  }
  if ((pSip->vmat = ve_ucreate(DV_HOST)) == VENULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VCRE, 0, "failed to create vector");
    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  if( (ierr = ve_alloc(pSip->vmat, 6, (int16)sizeof(sfloat), TY_SFLOAT)) != 0) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VALLOC, ierr, "error %d allocating vector");
    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  // Allocate data for subwindows

  pSip->pSubWin = (Tsip_SubWinTR1 *)malloc( TR1_MAX_SUBWIN * sizeof( Tsip_SubWinTR1));
  if (pSip->pSubWin == NULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_MEMALLOC, 0, "memory allocation failed");
    ierr = -SE_MEMALLOC;
    goto syserrorExit;
  }

  memset( pSip->pSubWin, 0, TR1_MAX_SUBWIN * sizeof( Tsip_SubWinTR1));  // set all to zero

  // loop over all subwindows

  nAOIs = pWin->MeasureMode == YAIPS_PSEARCH_MODE_1XY ? 1 : 3;

  for (subWinNr = 0; subWinNr < nAOIs && subWinNr < TR1_MAX_SUBWIN; subWinNr++) {

    pSubWin = &pWin->AOI[subWinNr];

    // calculate the pyramid
    if( (ierr = calculatePyramid( pSubWin, &pSip->pSubWin[ subWinNr].pyrDesc)) != 0) {
      // sets syserror itself
      goto syserrorExit;
    }

    // allocate the descriptors for all pyramid levels
    if (pSip->pSubWin[ subWinNr].pyrDesc.maxShrinkXY > 0 && pSip->pSubWin[ subWinNr].pyrDesc.nLevels > 0) {
      pSip->pSubWin[ subWinNr].pPyr = (T_pyramidLevel_TR1 *)malloc( pSip->pSubWin[ subWinNr].pyrDesc.nLevels * sizeof(T_pyramidLevel_TR1));
      if (pSip->pSubWin[ subWinNr].pPyr == (T_pyramidLevel_TR1 *)NULL) {
        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_MEMALLOC, 0, "memory allocation failed");
        ierr = -SE_MEMALLOC;
        goto syserrorExit;
      }
      for (level = 0; level < pSip->pSubWin[ subWinNr].pyrDesc.nLevels; level++) {
        pSip->pSubWin[ subWinNr].pPyr[level].iAllocObject = IMNULL;                         // preset
      }
    }

    // initialize the pyramid
    if( (ierr = initPyramid( pSubWin, piref, &pSip->pSubWin[ subWinNr].pyrDesc, pSip->pSubWin[ subWinNr].pPyr)) != 0) {
      // sets syserror itself
      goto syserrorExit;
    }
  }

  ierr = 0;

syserrorExit:

  return(ierr);

} /* static int doInspInit() */

static int ppcWinTR1_InspInit( Fl_RGB_Image *piref, YaIPS_PSearchXY_t *pWin) /*
======================================================================================== */
{
  int ierr, subWinNr, nAOIs;
  YaIPS_PSearchAOI_t *pSubWin;
  Tsip_winTR1 *pSip;

  PRINTL1(TRACE_RESULT, "ppcWinTR1_InspInit %d\n", winNr);

  // Check work data allocated

  pSip = (Tsip_winTR1 *)pWin->pWorkData;

  if( pSip != NULL) {      // Already allocated

    return( 0);            // Return OK
  }

  pSip = (Tsip_winTR1 *)malloc( sizeof( Tsip_winTR1));    // Allocated memory

  if( pSip == NULL) {      // Check allocated

    ierr = -SE_MEMALLOC;
    goto syserrorExit;
  }

  memset( pSip, 0, sizeof( Tsip_winTR1));    // Zero memory

  pWin->pWorkData = pSip;                    // Remember pointer to allocated memory

  // preset with VENULL/IMNULL for ppcWinTR1_InspEnd() call after error or NOT active window

  pSip->vin     = VENULL;
  pSip->vref    = VENULL;
  pSip->vtmp    = VENULL;
  pSip->v1t     = VENULL;
  pSip->vcorrel = VENULL;
  pSip->vmat    = VENULL;

  pSip->lastActive = true;

  if( (ierr = doInspInit( piref, pWin, pSip)) != 0) {
    // sets syserror itself
    goto syserrorExit;
  }

  // remember number of subwindows, window positions and sizes

  nAOIs = pWin->MeasureMode == YAIPS_PSEARCH_MODE_1XY ? 1 : 3;

  pSip->lastNSubWin = nAOIs;
  for (subWinNr = 0; subWinNr < nAOIs; subWinNr++) {

    pSubWin = &pWin->AOI[subWinNr];

    pSip->pSubWin[ subWinNr].lastWinDesc.XPos  = pSubWin->AOI.XPos;
    pSip->pSubWin[ subWinNr].lastWinDesc.YPos  = pSubWin->AOI.YPos;
    pSip->pSubWin[ subWinNr].lastWinDesc.XSize = pSubWin->AOI.XSize;
    pSip->pSubWin[ subWinNr].lastWinDesc.YSize = pSubWin->AOI.YSize;

    pSip->pSubWin[ subWinNr].lastPosToleranceX = pSubWin->PosToleranceX;
    pSip->pSubWin[ subWinNr].lastPosToleranceY = pSubWin->PosToleranceY;

    pSip->pSubWin[ subWinNr].lastUniX = pSubWin->Flags & PROJPAR_WINFLAG_YNACTIVE;
    pSip->pSubWin[ subWinNr].lastUniY = pSubWin->Flags & PROJPAR_WINFLAG_XNACTIVE;
  }

  ierr = 0;

syserrorExit:

  return(ierr);

} /* int ppcWinTR1_InspInit() */

static int doInspEnd( Tsip_winTR1 *pSip, YaIPS_PSearchXY_t *pWin) /*
================================================================= */
{
  int ierr, subWinNr, nAOIs;

  // free the descriptors for all pyramid levels

  nAOIs = pWin->MeasureMode == YAIPS_PSEARCH_MODE_1XY ? 1 : 3;

  if (pSip->pSubWin != NULL) {

    for (subWinNr = 0; subWinNr < nAOIs && subWinNr < TR1_MAX_SUBWIN; subWinNr++) {

      if (pSip->pSubWin[ subWinNr].pPyr != (T_pyramidLevel_TR1 *)NULL) {
        // end the pyramid
        if( (ierr = endPyramid(&pSip->pSubWin[ subWinNr].pyrDesc, pSip->pSubWin[ subWinNr].pPyr)) != 0) {
          // sets syserror itself
          goto syserrorExit;
        }

        // free the descriptors for all pyramid levels
        if (pSip->pSubWin[ subWinNr].pyrDesc.maxShrinkXY > 0 && pSip->pSubWin[ subWinNr].pyrDesc.nLevels > 0) {
          free((anypnt)pSip->pSubWin[ subWinNr].pPyr);
          pSip->pSubWin[ subWinNr].pPyr = (T_pyramidLevel_TR1 *)NULL;
        }
      }
    }

    free( (anypnt)pSip->pSubWin);
    pSip->pSubWin = NULL;
  }

  // ...

  if (pSip->vin != VENULL) {
    if( (ierr = ve_remove(pSip->vin)) != 0) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VREM, ierr, "error %d removing vector");
      ierr = -SE_VREM;
      goto syserrorExit;
    }
    pSip->vin = VENULL;
  }
  if (pSip->vref != VENULL) {
    if( (ierr = ve_remove(pSip->vref)) != 0) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VREM, ierr, "error %d removing vector");
      ierr = -SE_VREM;
      goto syserrorExit;
    }
    pSip->vref = VENULL;
  }
  if (pSip->vtmp != VENULL) {
    if( (ierr = ve_remove(pSip->vtmp)) != 0) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VREM, ierr, "error %d removing vector");
      ierr = -SE_VREM;
      goto syserrorExit;
    }
    pSip->vtmp = VENULL;
  }
  if (pSip->v1t != VENULL) {
    if( (ierr = ve_remove(pSip->v1t)) != 0) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VREM, ierr, "error %d removing vector");
      ierr = -SE_VREM;
      goto syserrorExit;
    }
    pSip->v1t = VENULL;
  }
  if (pSip->vcorrel != VENULL) {
    if( (ierr = ve_remove(pSip->vcorrel)) != 0) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VREM, ierr, "error %d removing vector");
      ierr = -SE_VREM;
      goto syserrorExit;
    }
    pSip->vcorrel = VENULL;
  }
  if (pSip->vmat != VENULL) {
    if( (ierr = ve_remove(pSip->vmat)) != 0) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VREM, ierr, "error %d removing vector");
      ierr = -SE_VREM;
      goto syserrorExit;
    }
    pSip->vmat = VENULL;
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

int YaIPS_PSearchAOI_WorkDataFree( YaIPS_PSearchXY_t *pWin) /*
=========================================================== */
{
  int ierr;
  Tsip_winTR1  *pSip;              // pointer to sip structure

  PRINTL1(TRACE_RESULT, "ppcWinTR1_InspEnd %d\n", winNr);

  // Check work data allocated

  pSip = (Tsip_winTR1 *)pWin->pWorkData;
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

} /* int ppcWinTR1_InspEnd() */

/************************************************************************************
* ppcWinTR1_DoWinCheck
*
*/

static int ppcWinTR1_DoWinCheck( Fl_RGB_Image *iref, YaIPS_PSearchAOI_t *pSubWin,
                                 int doCheck, int *RetNorm, int *RetQual, int *RetColorUsed) /*
=================================================================================== */
{
  int ierr;
  int xTol, yTol, xx, yy, x, y, i, nColors;
  int xo, yo, xxo, yyo;
  int xs, ys, xxs, yys;
  int xxc, yyc;
  int shrinkX, shrinkY, minTolX, minTolY;
  //x/Timages *sceneRef  = IMNULL;
  Timages *scene     = IMNULL;
  //x/Timages *objectRef  = IMNULL;
  Timages *objectFilt = IMNULL;
  Timages *object     = IMNULL;
  Timages *correl    = IMNULL;
  Tvector *vcorrel = VENULL;
  Tffuncval *pfvcor;
  int16 qual[3], cdfQualIerr[3];
  int bestqual, bestColor;
  T_pyramidDesc_TR1 pyrDesc;

  xx = iref->w();  //x/ getxx(iref);
  yy = iref->h();  //x/ getyy(iref);

  // preset results
  *RetNorm = -1;
  *RetQual = 0;
  *RetColorUsed = 0;

  if( (ierr = calculatePyramid( pSubWin, &pyrDesc)) != 0) {
    // sets syserror itself
    goto syserrorExit;
  }

  if (pyrDesc.maxShrinkXY <= 0) return(0);   // no check needed

  xTol = pyrDesc.xTol;
  yTol = pyrDesc.yTol;

  // check window border
  if ((pSubWin->AOI.XPos - xTol < 0) ||
      (pSubWin->AOI.XPos + pSubWin->AOI.XSize + xTol > xx)) {
    ierr = CHECK_ERR_IMBORDER;
    goto exitPoint;
  }
  if ((pSubWin->AOI.YPos - yTol < 0) ||
      (pSubWin->AOI.YPos + pSubWin->AOI.YSize + yTol > yy)) {
    ierr = CHECK_ERR_IMBORDER;
    goto exitPoint;
  }

  // check correlation pattern in highest shrink level

  xo = pSubWin->AOI.XPos;
  yo = pSubWin->AOI.YPos;
  xxo = pSubWin->AOI.XSize;
  yyo = pSubWin->AOI.YSize;

  shrinkX = pyrDesc.maxShrinkX;
  shrinkY = pyrDesc.maxShrinkY;

  PRINTL2(TRACE_DEBUG, "shrinkfactor %d/%d\n", shrinkX, shrinkY);

  xxo = ((xxo + shrinkX / 2) / shrinkX) * shrinkX; // round to multiple of shrinkX
  if (xo + xxo > xx) {
    if (xxo > shrinkX) {
      xxo -= shrinkX;
    } else {
      xo = xx - xxo;
    }
  }
  yyo = ((yyo + shrinkY / 2) / shrinkY) * shrinkY; // round to multiple of shrinkY
  if (yo + yyo > yy) {
    if (yyo > shrinkY) {
      yyo -= shrinkY;
    } else {
      yo = yy - yyo;
    }
  }

  xTol <<= 1;  // take double tolerance
  yTol <<= 1;

  if (pyrDesc.uniY == FALSE) {
    xTol = ((xTol + shrinkX / 2) / shrinkX) * shrinkX; // round to multiple of shrinkX
    if (xTol < shrinkX) xTol = shrinkX;
  }
  if (pyrDesc.uniX == FALSE) {
    yTol = ((yTol + shrinkY / 2) / shrinkY) * shrinkY; // round to multiple of shrinkY
    if (yTol < shrinkY) yTol = shrinkY;
  }

  // if selected tolerance is too small, we take the minimum required
  // tolerance for the teach algorithm.
  minTolX = CORR_TEACHTOL * shrinkX;  // we need in case of shrink this minimum tolerance too
  minTolY = CORR_TEACHTOL * shrinkY;  // we need in case of shrink this minimum tolerance too
  if (pyrDesc.uniY == FALSE && xTol < minTolX) xTol = minTolX;
  if (pyrDesc.uniX == FALSE && yTol < minTolY) yTol = minTolY;

  PRINTL2(TRACE_DEBUG, "XTol %d YTol %d\n", xTol, yTol);

  xs = xo - xTol;
  ys = yo - yTol;
  xxs = xxo + (xTol << 1);
  yys = yyo + (yTol << 1);

  // we teach with the double x_tol/y_tol. but we allow a minimum distance
  // from image border of x_tol/y_tol
  if ((xs < 0) && (xs >= (-xTol/2))) {
    x = ((xs - (shrinkX - 1)) / shrinkX) * shrinkX;
    xxs += x;
    xs  -= x;
  }
  if ((xs + xxs > xx) && (xs + xxs <= xx + xTol/2)) {
    xxs = xxs - (xxs + xs - xx);
    xxs = (xxs / shrinkX) * shrinkX; // round down
  }
  if ((ys < 0) && (ys >= (-yTol/2))) {
    y = ((ys - (shrinkY - 1)) / shrinkY) * shrinkY;
    yys += y;
    ys  -= y;
  }
  if ((ys + yys > yy) && (ys + yys <= yy + yTol/2)) {
    yys = yys - (yys + ys - yy);
    yys = (yys / shrinkY) * shrinkY; // round down
  }

  // check for minimum tolerance
  if (pyrDesc.uniY == FALSE && xxs < xxo + (minTolX << 1)) {
    ierr = CHECK_ERR_IMBORDER;
    goto exitPoint;
  }
  if (pyrDesc.uniX == FALSE && yys < yyo + (minTolY << 1)) {
    ierr = CHECK_ERR_IMBORDER;
    goto exitPoint;
  }

  // be sure
  if (xs < 0 || (xs + xxs) > xx || ys < 0 || (ys + yys) > yy) {
    ierr = CHECK_ERR_IMBORDER;
    goto exitPoint;
  }

  // be sure
  if (xo < shrinkX || (xo + xxo) > (xx - 2 * shrinkX) ||
      yo < shrinkY || (yo + yyo) > (yy - 2 * shrinkY)) {
    ierr = CHECK_ERR_IMBORDER;
    goto exitPoint;
  }

#ifdef use_again
  // -------------------- scene ------------------------
  if ((sceneRef  = im_udefine( iref, xs, ys, xxs, yys)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMDEF, 0, "failed to define image");
    ierr = -SE_IMDEF;
    goto syserrorExit;
  }
  if ((scene = im_ucreate(TY_INT32, DV_HOST, xxs / shrinkX,
                          yys / shrinkY, 0, 0, 32, 0)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
    ierr = -SE_IMCRE;
    goto syserrorExit;
  }
  // -------------------- object ------------------------
  if ((objectRef = im_udefine(iref, xo - shrinkX, yo - shrinkY,
                              xxo + 2 * shrinkX, yyo + 2 * shrinkY)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMDEF, 0, "failed to define image");
    ierr = -SE_IMDEF;
    goto syserrorExit;
  }
  if ((objectFilt = im_ucreate(TY_INT32, DV_HOST, (xxo + 2 * shrinkX) / shrinkX,
                              (yyo + 2 * shrinkY) / shrinkY, 0, 0, 32, 0)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
    ierr = -SE_IMCRE;
    goto syserrorExit;
  }
  if ((object = im_udefine(objectFilt, 1, 1, xxo / shrinkX, yyo / shrinkY)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMDEF, 0, "failed to define image");
    ierr = -SE_IMDEF;
    goto syserrorExit;
  }
  xxc = ((xxs - xxo) / shrinkX) + 1;
  yyc = ((yys - yyo) / shrinkY) + 1;
  if ((correl = im_ucreate(TY_SFLOAT, DV_HOST, xxc, yyc,
                           0, 0, 32, 0)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
    ierr = -SE_IMCRE;
    goto syserrorExit;
  }


  nColors = (getyp(iref) == TY_INT32) ? 3 : 1;

#else
  // -------------------- scene ------------------------
  if ((scene = im_ucreateMem( NULL, TY_INT32, DV_HOST, xxs / shrinkX, yys / shrinkY, NULL)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
    ierr = -SE_IMCRE;
    goto syserrorExit;
  }
  // -------------------- object ------------------------
  if ((objectFilt = im_ucreateMem( NULL, TY_INT32, DV_HOST, (xxo + 2 * shrinkX) / shrinkX,
                                   (yyo + 2 * shrinkY) / shrinkY, NULL)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
    ierr = -SE_IMCRE;
    goto syserrorExit;
  }
  if ((object = im_udefineMem( NULL, objectFilt, 1, 1, xxo / shrinkX, yyo / shrinkY)) == IMNULL) {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMDEF, 0, "failed to define image");
    ierr = -SE_IMDEF;
    goto syserrorExit;
  }
  xxc = ((xxs - xxo) / shrinkX) + 1;
  yyc = ((yys - yyo) / shrinkY) + 1;
  if ((correl = im_ucreateMem( NULL, TY_SFLOAT, DV_HOST, xxc, yyc, NULL)) == IMNULL) {
    //x/sysstate_setsyserr( MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
    ierr = -SE_IMCRE;
    goto syserrorExit;
  }

  /* ------------------- correlation vectors */

  if( (vcorrel = ve_ucreate(DV_HOST)) == VENULL) {
    //x/sysstate_setsyserr(SYSERR_MODULE_IP, SEIP_VCRE, 0, NULL);      // failed to create vector
    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  if( (ierr = ve_alloc( vcorrel, 3, sizeof(Tffuncval), TY_FUNCVAL)) != 0) {
    //x/sysstate_setsyserr(SYSERR_MODULE_IP, SEIP_VALLOC, ierr, NULL);   // error %d allocating vector
    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  nColors = iref->d() >= 3 ? 3 : 1;

#endif

  for (i = 0; i < nColors; i++) {

    if (shrinkX > 1 || shrinkY > 1) {
      if( (ierr = inspUtil_ishrinkImageXY( iref, xs, ys, xxs, yys,
                                          scene, i, shrinkX, shrinkY)) != 0) {
        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_ISHRINK, ierr, "failed %d shrinking image");
        ierr = -SE_ISHRINK;
        goto syserrorExit;
      }
      if( (ierr = inspUtil_ishrinkImageXY( iref, xo - shrinkX, yo - shrinkY,
                                          xxo + 2 * shrinkX, yyo + 2 * shrinkY,
                                          objectFilt, i, shrinkX, shrinkY)) != 0) {
        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_ISHRINK, ierr, "failed %d shrinking image");
        ierr = -SE_ISHRINK;
        goto syserrorExit;
      }
    } else {
      if( (ierr = inspUtil_imoveImage( iref, xs, ys, xxs, yys,
                                      scene, i)) != 0) {
        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMOVE, ierr, "failed %d moving image");
        ierr = -SE_IMOVE;
        goto syserrorExit;
      }
      if( (ierr = inspUtil_imoveImage( iref, xo - shrinkX, yo - shrinkY,
                                      xxo + 2 * shrinkX, yyo + 2 * shrinkY,
                                      objectFilt, i)) != 0) {
        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMOVE, ierr, "failed %d moving image");
        ierr = -SE_IMOVE;
        goto syserrorExit;
      }
    }

    /* simulate subpixel shift with filter.
       We can make an inplace calculation, because we use only coeff 4, 5, 7, 8 */
    if( (ierr = ifilt3( objectFilt, objectFilt, -3, 0,
                        0, 0, 0, 0, 5, 1, 0, 1, 1)) != 0) {
      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IFILT3, ierr, "error %d in ifilt3()");
      ierr = -SE_IFILT3;
      goto syserrorExit;
    }

    // -----------------------------------------------------------------------
    // do the correlation, no interpolation
    // -----------------------------------------------------------------------

    if( (ierr = gcrcdf32( object, scene, correl, vcorrel)) != 0) {
      if (ierr == -10) {
        /* correlation extr. on image border */
        cdfQualIerr[i] = -50;
        qual[i] = 0;
        continue;
      } else {
        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_CRCDF, ierr, "error %d in correlation");
        ierr = -SE_CRCDF;
        goto syserrorExit;
      }
    }

    // setup weights for quality check
    //            1lin 1sqrt 1sq  2lin 2sqrt 2sq  3lin 3sqrt 3sq   sum    wcomp12
    // default is 1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0,  300.0, 0.0
    cdf2_testSetup(1.0, 0.0,  0.0, 1.0, 0.0,  0.0, 1.0, 0.0,  0.0,  300.0, 0.0);

    /* compute teach quality */
    pfvcor = (Tffuncval *)vgetpm( vcorrel);
    if (pyrDesc.uniX == TRUE) {
      cdfQualIerr[i] = cdf2_txqual(correl, pfvcor->fxpos, pfvcor->fypos,
                                   (int32)getxx(object) * getyy(object),
                                   minTeachQual, &qual[i], doCheck);
    } else if (pyrDesc.uniY == TRUE) {
      cdfQualIerr[i] = cdf2_tyqual(correl, pfvcor->fxpos, pfvcor->fypos,
                                   (int32)getxx(object) * getyy(object),
                                   minTeachQual, &qual[i], doCheck);
    } else {
      cdfQualIerr[i] = cdf2_txyqual(correl, pfvcor->fxpos, pfvcor->fypos,
                                    (int32)getxx(object) * getyy(object),
                                    minTeachQual, &qual[i], doCheck);
    }

  } /* end of for (i = 0; i < nColors; i++) { */

  bestColor = -1;
  bestqual = -9999;
  for (i = 0; i < nColors; i++) {
    PRINTL3(TRACE_DEBUG, "%s: norm %d, qual %d\n",
            i == FSV_RED ? "red" : (i == FSV_GREEN ? "green" : "blue"), cdfQualIerr[i], qual[i]);
    if (cdfQualIerr[i] < 0) continue;
    if (bestqual == -9999 || qual[i] > bestqual) {
      bestColor = i;
      bestqual = qual[i];
    }
  }

  if (bestqual == -9999) {
    // no good quality found
    bestColor = 0;
    bestqual = qual[0];
    ierr = cdfQualIerr[0];
  } else {
    ierr = cdfQualIerr[bestColor];
  }

  *RetNorm = ierr;
  *RetQual = bestqual;
  if (nColors == 1) {
    bestColor = FSV_GREEN;
  }
  *RetColorUsed = bestColor;
  PRINTL3(TRACE_DEBUG, "%s: norm %d, qual %d\n",
          bestColor == FSV_RED ? "red" : (bestColor == FSV_GREEN ? "green" : "blue"), ierr, bestqual);

  if (ierr >= 0) {
    ierr = 0;                              /* set ierr to 0 for good window */
  } else if (ierr == -50) {
    ierr = CHECK_ERR_BPAT;
    goto exitPoint;
  } else if (ierr == -51) {
    ierr = CHECK_ERR_BPATCONTR;
    goto exitPoint;
  } else if (ierr == -52) {
    ierr = CHECK_ERR_PATNUNIQ;
    goto exitPoint;
  } else if (ierr == -53) {
    ierr = CHECK_ERR_PATNUSAB;
    goto exitPoint;
  } else {
    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_TXYQUAL, ierr, "error %d in cdf2_txyqual()");
    ierr = -SE_TXYQUAL;
    goto syserrorExit;
  }

exitPoint:
  ;

syserrorExit:

  //x/if (sceneRef   != IMNULL)  im_delete(sceneRef);
  if (scene      != IMNULL)  im_remove(scene);
  //x/if (objectRef  != IMNULL)  im_delete(objectRef);
  if (object     != IMNULL)  im_remove(object);
  if (objectFilt != IMNULL)  im_remove(objectFilt);  // created
  if (correl     != IMNULL)  im_remove(correl);
  if (vcorrel    != VENULL)  ve_remove( vcorrel);

  return(ierr);

} /* static int ppcWinTR1_DoWinCheck() */

/************************************************************************************
* YaIPS_CheckCode2Text
*
*/

static void YaIPS_CheckCode2Text( int CheckError,
                                  char *pCheckText,
                                  int SizeOfCheckText)
{

  switch( CheckError) {

  case 0:   // This should be handled outside this function

    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_CheckErr0=OK"), SizeOfCheckText - 1);
    break;
  case CHECK_ERR_IMBORDER:
    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_CheckErr1=Touch border!"), SizeOfCheckText - 1);  // AOI touches image border!
    break;
  case CHECK_ERR_BPAT:
    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_CheckErr2=Bad pattern!"), SizeOfCheckText - 1);
    break;
  case CHECK_ERR_BPATCONTR:
    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_CheckErr3=Bad contrast!"), SizeOfCheckText - 1);   // Bad pattern contrast
    break;
  case CHECK_ERR_PATNUNIQ:
    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_CheckErr4=Not unique!"), SizeOfCheckText - 1);  // Pattern not unique
    break;
  case CHECK_ERR_PATNUSAB:
    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_CheckErr5=Not usable!"), SizeOfCheckText - 1);  // Pattern not usable
    break;
  case CHECK_ERR_WINSIZE:
    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_CheckErr6=AOI too big!"), SizeOfCheckText - 1);
    break;
  default:
    sprintf( pCheckText, LANGDEF_ERROR_CODE, CheckError);
    break;
  }
}

/************************************************************************************
* YaIPS_PSearchAOI_Teach
*
* Teach all AOIs
*
*/

int YaIPS_PSearchAOI_Teach( Fl_RGB_Image *piref, YaIPS_PSearchXY_t *pPSearchXY) /*
=============================================================================== */
{
  int ierr = 0, i, nAOIs, exitStatus;
  int CorrNorm, CorrQual, colorUsed;
  YaIPS_PSearchAOI_t *pPSearchAOI;

  PRINTL0(TRACE_RESULT, "ppcWinTR1_Teach\n");

  exitStatus = 0;
  pPSearchXY->CheckError = 0;
  pPSearchXY->CheckText[ 0] = '\0';

  nAOIs = pPSearchXY->MeasureMode == YAIPS_PSEARCH_MODE_1XY ? 1 : 3;

  // walk through all subwindows

  pPSearchAOI = pPSearchXY->AOI;

  for( i = 0; i < nAOIs && i < TR1_MAX_SUBWIN; i++, pPSearchAOI++) {

    // set doCheck to 1 anycase because we need the colorUsed
    ierr = ppcWinTR1_DoWinCheck( piref, pPSearchAOI, 1, &CorrNorm, &CorrQual, &colorUsed);

    pPSearchAOI->CheckError = ierr;
    YaIPS_CheckCode2Text( ierr, pPSearchAOI->CheckText, sizeof( pPSearchAOI->CheckText));

    if (ierr != 0) {
      if (!exitStatus) {

        exitStatus = ierr;

        pPSearchXY->CheckError = exitStatus;

        if( nAOIs <= 1) {

          strcpy( pPSearchXY->CheckText, pPSearchAOI->CheckText);

        } else {

          sprintf( pPSearchXY->CheckText, "%d: %s", i + 1, pPSearchAOI->CheckText);
        }
      }
      pPSearchAOI->CorrNorm = -1;
      pPSearchAOI->colorUsed = 0;
      pPSearchAOI->CheckCorrQual = 0;
    } else {
      pPSearchAOI->CorrNorm = CorrNorm;
      pPSearchAOI->colorUsed = colorUsed;
      pPSearchAOI->CheckCorrQual = CorrQual;

      PRINTL2(TRACE_DEBUG, "subwin %d: colorUsed %s\n", i,
              pMySubWinPar->colorUsed == FSV_RED ? "red" : (pMySubWinPar->colorUsed == FSV_GREEN ? "green" : "blue"));
    }
  }

  if( pPSearchXY->CheckText[ 0] == '\0') {   // No check text until now

    YaIPS_CheckCode2Text( pPSearchXY->CheckError, pPSearchXY->CheckText, sizeof( pPSearchXY->CheckText));
  }

  return( exitStatus);

} /* int ppcWinTR1_Teach() */

/************************************************************************************
* YaIPS_InspectCode2Text
*
*/

static void YaIPS_InspectCode2Text( int CheckError,
                                    char *pCheckText,
                                    int SizeOfCheckText)
{

  switch( CheckError) {

  case PPCWINTR1_INSPERR_OK:   // This should be handled outside this function

    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_InspErr0="), SizeOfCheckText - 1);
    break;
  case PPCWINTR1_INSPERR_POS:
    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_InspErr1=Pos. Error!"), SizeOfCheckText - 1);  // position error
    break;
  case PPCWINTR1_INSPERR_SEARCRANGE:
    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_InspErr2=Search range!"), SizeOfCheckText - 1);  // position tolerance/search range error
    break;
  case PPCWINTR1_INSPERR_WINBORDER:
    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_InspErr3=Touch border!"), SizeOfCheckText - 1);   // touches image border
    break;
  case PPCWINTR1_INSPERR_ILLTRAFO:
    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_InspErr4=Positions!"), SizeOfCheckText - 1);  // unable to compute trafo system (illegal window positions)
    break;
  case PPCWINTR1_INSPERR_TEACH_ERR:
    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_InspErr5=No teach!"), SizeOfCheckText - 1);  // error on teaching the window
    break;
  case PPCWINTR1_INSPERR_OTHER:
    strncpy( pCheckText, LangStringLookup( "&Utils_PSearch_InspErr6=Error!"), SizeOfCheckText - 1); // Any other inspection error
    break;
  default:
    sprintf( pCheckText, LANGDEF_ERROR_CODE, CheckError);
    break;
  }
}

/************************************************************************************
* YaIPS_PSearchAOI_Teach
*
* Inspect
*
*/

int YaIPS_PSearchAOI_Inspect( Fl_RGB_Image *pImgIn,            // Input image
                              Fl_RGB_Image *piref,             // Reference image
                              YaIPS_PSearchXY_t *pWin)         // Search parameter
{
  int i, ierr;
#ifdef use_again
  Timages *iin = IMNULL;          // image on input for correlation
#else
  int AoiX, AoiY, AoiXX, AoiYY;
#endif
  Tsip_winTR1  *pSip;             // pointer to sip structure
  sfloat *pmat;                   // pointer to local transform system
  int32 dx, dy, dxIter, dyIter;
  int16 xTol, yTol, xci, yci, xNew, yNew;
  double xd, yd, xaDelta, yaDelta;
  Timages *correl = IMNULL;       // correlation result
  Timages *scene  = IMNULL;       // scene
  Timages *object = IMNULL;       // object
  int shrinkX, shrinkY, level;
  T_WinDesc tmpWinDesc;
  //x/char buf[100], buf2[100];
  int CorrQuality;
  //x/char *pU1, *pU2;
  sfloat xPrec, yPrec;
  YaIPS_PSearchAOI_t *pSubWin;
  double dxU, dyU;
  register int32 *pin, *pref;
  int numValid;
  int subWinPosSizeChanged, nAOIs;
  int16 xTolN, xTolP, yTolN, yTolP;

  // -----------------------------------------------------------------------
  // initialization
  // -----------------------------------------------------------------------
  numValid = 0;

  PRINTL2(TRACE_RESULT, "********** ppcWinTR1_InspDo %d, Ref %d\n", winNr, pRef->refNr);

  // Setup up temporary work data

  ierr = ppcWinTR1_InspInit( piref, pWin);         // Ensure initialized

  pSip = (Tsip_winTR1 *)pWin->pWorkData;

  if( ierr != 0 ||            // Check problem with work data
      pSip == NULL) {

    ierr = -SE_NO_WORK_DATA;
    errstring = (char *)"No work data allocated";

    return( ierr);
  }

  nAOIs = pWin->MeasureMode == YAIPS_PSEARCH_MODE_1XY ? 1 : 3;

  pWin->CheckError = 0;                          // init no error
  pWin->CheckText[ 0] = '\0';

  pWin->DeltaX = 0.0;                  // Preset sane results
  pWin->DeltaY = 0.0;
  pWin->DeltaA = 0.0;

  pWin->T_Matrix[ A11] = 1.0;
  pWin->T_Matrix[ A12] = 0.0;
  pWin->T_Matrix[ A21] = 0.0;
  pWin->T_Matrix[ A22] = 1.0;
  pWin->T_Matrix[ B1]  = 0.0;
  pWin->T_Matrix[ B2]  = 0.0;

  // -----------------------------------------------------------------------
  // check for changed number of subwindows, position and size (onTheFly)
  // -----------------------------------------------------------------------
  subWinPosSizeChanged = FALSE;

  for (i = 0; i < nAOIs; i++) {

    pSubWin = &pWin->AOI[ i];

    if ( (pSip->pSubWin[ i].lastWinDesc.XPos  != pSubWin->AOI.XPos) ||
         (pSip->pSubWin[ i].lastWinDesc.YPos  != pSubWin->AOI.YPos) ||
         (pSip->pSubWin[ i].lastWinDesc.XSize != pSubWin->AOI.XSize) ||
         (pSip->pSubWin[ i].lastWinDesc.YSize != pSubWin->AOI.YSize) ||
         (pSip->pSubWin[ i].lastPosToleranceX != pSubWin->PosToleranceX) ||
         (pSip->pSubWin[ i].lastPosToleranceY != pSubWin->PosToleranceY) ||
         (pSip->pSubWin[ i].lastUniX != (pSubWin->Flags & PROJPAR_WINFLAG_YNACTIVE)) ||
         (pSip->pSubWin[ i].lastUniY != (pSubWin->Flags & PROJPAR_WINFLAG_XNACTIVE))) {
      subWinPosSizeChanged = TRUE;
      break;
    }
  }
  if (subWinPosSizeChanged == TRUE ||
      /*(pSip->lastActive != (pWin->Flags & PROJPAR_WINFLAG_ACTIVE)) || */
      nAOIs != pSip->lastNSubWin) {

    // re-Initialize inspection
    if( (ierr = doInspEnd(pSip, pWin)) != 0) {
      // sets syserror itself
      goto syserrorExit;
    }
    if( (ierr = doInspInit( piref, pWin, pSip)) != 0) {
      // sets syserror itself
      goto syserrorExit;
    }

    // remember number of subwindows, window positions and sizes
    pSip->lastActive = true;
    pSip->lastNSubWin = nAOIs;

    for (i = 0; i < nAOIs; i++) {

      pSubWin = &pWin->AOI[ i];

      pSip->pSubWin[ i].lastWinDesc.XPos  = pSubWin->AOI.XPos;
      pSip->pSubWin[ i].lastWinDesc.YPos  = pSubWin->AOI.YPos;
      pSip->pSubWin[ i].lastWinDesc.XSize = pSubWin->AOI.XSize;
      pSip->pSubWin[ i].lastWinDesc.YSize = pSubWin->AOI.YSize;

      pSip->pSubWin[ i].lastPosToleranceX = pSubWin->PosToleranceX;
      pSip->pSubWin[ i].lastPosToleranceY = pSubWin->PosToleranceY;

      pSip->pSubWin[ i].lastUniX = pSubWin->Flags & PROJPAR_WINFLAG_YNACTIVE;
      pSip->pSubWin[ i].lastUniY = pSubWin->Flags & PROJPAR_WINFLAG_XNACTIVE;
    }
  }

  // -----------------------------------------------------------------------
  // and do it
  // -----------------------------------------------------------------------
  // reset point vectors
  vputnm( pSip->vin, 0);
  vputnm( pSip->vref, 0);

  // walk through all subwindows

  for (i = 0; i < nAOIs; i++) {

    pSubWin = &pWin->AOI[i];

    pSubWin->InspError = PPCWINTR1_INSPERR_OTHER;  // Preset error
  }

  for (i = 0; i < nAOIs; i++) {

    double xd_first, yd_first, xRef_first, yRef_first;
    int xsDraw, ysDraw, xxsDraw = 0, yysDraw = 0;

    pSubWin = &pWin->AOI[i];

    pSubWin->InspError = 0;                 // Preset no error
    pSubWin->InspText[ 0] = '\0';

    CorrQuality = -1;
    pSubWin->InspQual = CorrQuality;

    // -----------------------------------------------------------------------
    // make a local transform system
    // -----------------------------------------------------------------------

#ifdef use_again
    if (ierr = vcopy(inspData.vmat[pRef->refNr], pSip->vmat, 0, -1, 0)) {
      sysstate_setsyserr(MY_SYSERR_MODULE, SE_VCOPY, ierr, "error %d in vcopy()");
      ierr = -SE_VCOPY;
      goto syserrorExit;
    }
#else
    // Set an 1:1 trafo systen
    pmat = (sfloat *)vgetpm(pSip->vmat);

    pmat[ A11] = 1.0;
    pmat[ A12] = 0.0;
    pmat[ A21] = 0.0;
    pmat[ A22] = 1.0;
    pmat[  B1] = 0.0;
    pmat[  B2] = 0.0;
#endif

    // -----------------------------------------------------------------------
    // correlation
    // -----------------------------------------------------------------------

    if (pSip->pSubWin[ i].pyrDesc.maxShrinkXY <= 0) {

      dx = 0;                  // no correlation
      dy = 0;

    } else {

      // ...

      dx = 0;                  // iterative sum, so initialize with 0
      dy = 0;

      // ...

      PRINTL2(TRACE_DEBUG, "max. shrinkfactor %d/%d\n", pSip->pSubWin[ i].pyrDesc.maxShrinkX, pSip->pSubWin[ i].pyrDesc.maxShrinkY);
      PRINTL4(TRACE_DEBUG, "XTol %d -> %d, YTol %d -> %d\n",
        dto32(pSip->pSubWin[ i].pyrDesc.PosToleranceX * (double)ProjTrafo[ pRef->refNr][A11]), pSip->pSubWin[ i].pyrDesc.xTol,
        dto32(pSip->pSubWin[ i].pyrDesc.PosToleranceY * (double)ProjTrafo[ pRef->refNr][A22]), pSip->pSubWin[ i].pyrDesc.yTol);

      for( level = pSip->pSubWin[ i].pyrDesc.nLevels - 1; level >= 0; level--) {

        T_pyramidLevel_TR1 *pActPyr;

        pActPyr = &pSip->pSubWin[ i].pPyr[level];

        shrinkX = pActPyr->shrinkX;                  // get shrink factor for this pyramid level
        shrinkY = pActPyr->shrinkY;

        PRINTL3(TRACE_DEBUG, "========== shrink %d/%d (level %d) ============\n", shrinkX, shrinkY, level);

        xTol = pSip->pSubWin[ i].pyrDesc.xTol;
        yTol = pSip->pSubWin[ i].pyrDesc.yTol;

        if (pSip->pSubWin[ i].pyrDesc.uniY == FALSE) {
          xTol = ((xTol + shrinkX / 2) / shrinkX) * shrinkX; // round to multiple of shrinkX
          xTol += shrinkX;                                   // one pixel more due to INTERPOL3_R
          if (xTol < shrinkX) xTol = shrinkX;
        }
        if (pSip->pSubWin[ i].pyrDesc.uniX == FALSE) {
          yTol = ((yTol + shrinkY / 2) / shrinkY) * shrinkY; // round to multiple of shrinkY
          yTol += shrinkY;                                   // one pixel more due to INTERPOL3_R
          if (yTol < shrinkY) yTol = shrinkY;
        }

        // make windesc for scene
        tmpWinDesc.XPos  = pActPyr->xo;
        tmpWinDesc.YPos  = pActPyr->yo;
        tmpWinDesc.XSize = pActPyr->xxo;
        tmpWinDesc.YSize = pActPyr->yyo;

        PRINTL4(TRACE_DEBUG, "object %d / %d, %d x %d --->\n",
          pSubWin->AOI.XPos, pSubWin->AOI.YPos,
          pSubWin->AOI.XSize, pSubWin->AOI.YSize);
        PRINTL4(TRACE_DEBUG, "       %d / %d, %d x %d\n", pActPyr->xo, pActPyr->yo, pActPyr->xxo, pActPyr->yyo);



        if (level >= pSip->pSubWin[ i].pyrDesc.nLevels - 1) {

          // first run, search in complete tolerance
          xTolN = xTolP = xTol;
          yTolN = yTolP = yTol;
        } else {
          // use only tolerance of max. PYR_SHRINK_TOL pixels in each direction
          if (xTol >= PYR_SHRINK_TOL * shrinkX) {
            xTolN = xTolP = PYR_SHRINK_TOL * shrinkX;
          } else {
            xTolN = xTolP = xTol;         // is minimum 1 * shrinkX
          }
          if (yTol >= PYR_SHRINK_TOL * shrinkY) {
            yTolN = yTolP = PYR_SHRINK_TOL * shrinkY;
          } else {
            yTolN = yTolP = yTol;         // is minimum 1 * shrinkY
          }
        }
        // clip tolerance if needed. Round integer position to PixelCenter to be as near as possible to correct
        // position because we have only 2 pixels tolerance in iteration runs
        if( (ierr = inspUtil_imDefineTolEx2ClipPixelCenter(
                          pImgIn,
                          &AoiX, &AoiY, &AoiXX, &AoiYY,
                          pSip->vmat, &tmpWinDesc, shrinkX, shrinkY,
                          &xTolP, &xTolN, &yTolP, &yTolN,
                          &xci, &yci, &xd, &yd, &xNew, &yNew)) != 0) {
            if (ierr == -SE_WIN_BORDER) {
              // window touches image border
              pSubWin->InspError = PPCWINTR1_INSPERR_WINBORDER;
              goto nextWindow;
            } else {
              //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMDEF, 0, "failed to define image");
              ierr = -SE_IMDEF;
              goto syserrorExit;
            }
        }

        if (level >= pSip->pSubWin[ i].pyrDesc.nLevels - 1) {
          // first run, remember for later drawings and calculations

          xd_first = xd;   // center position of new
          yd_first = yd;

          xRef_first = tmpWinDesc.XPos + (tmpWinDesc.XSize + 1) / 2;  // center position of ref
          yRef_first = tmpWinDesc.YPos + (tmpWinDesc.YSize + 1) / 2;

          // take the position of window - tolerance, because iin-size is in multiples of maxShrink
          xsDraw = xNew - pSip->pSubWin[ i].pyrDesc.xTol;
          ysDraw = yNew - pSip->pSubWin[ i].pyrDesc.yTol;

          // 10.08.2011 RR: Bugfix, got unsmetric search range drawings in inspection
          //                for unidirectional search range.
          //                The size of scene is without the '+ 1'!
          //                See also the code for inspUtil_imDefineTolEx2ClipPixelCenter().
          xxsDraw = pSubWin->AOI.XSize + 2 * pSip->pSubWin[ i].pyrDesc.xTol;
          yysDraw = pSubWin->AOI.YSize + 2 * pSip->pSubWin[ i].pyrDesc.yTol;

          // clip to image border
          if (xsDraw < 0) {
            xxsDraw += xsDraw;
            xsDraw = 0;
          }
          if (ysDraw < 0) {
            yysDraw += ysDraw;
            ysDraw = 0;
          }
          if (xsDraw + xxsDraw > pImgIn->w()) {
            xxsDraw = pImgIn->w() - xsDraw;
          }
          if (ysDraw + yysDraw > pImgIn->h()) {
            yysDraw = pImgIn->h() - ysDraw;
          }
        }

        // -----------------------------------------------------------------------
        // check quality of correlation
        // -----------------------------------------------------------------------

        if( pSubWin->CorrNorm <= 0) {              // test for error on teaching the window

          // give a better feedback to the user as 'position calculation error'

          /* CorrNorm not calculated => error on teaching the window */
          pSubWin->InspError = PPCWINTR1_INSPERR_TEACH_ERR;
          goto nextWindow;
        }

        // ...

        xTolP /= shrinkX;
        xTolN /= shrinkX;
        yTolP /= shrinkY;
        yTolN /= shrinkY;

        if( (correl = im_ucreateMem( NULL, TY_SFLOAT, DV_HOST,
                                     xTolP + xTolN + 1, yTolP + yTolN + 1, NULL)) == IMNULL) {
            //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
            ierr = -SE_IMCRE;
            goto syserrorExit;
        }

        // --------------- scene on heap ----------------------

        if( (scene = im_ucreateMem( NULL, TY_INT32, DV_HOST, AoiXX / shrinkX,
                                    AoiYY / shrinkY, NULL)) == IMNULL) {
            //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
            ierr = -SE_IMCRE;
            goto syserrorExit;
        }
        if (shrinkX > 1 || shrinkY > 1) {
          if( (ierr = inspUtil_ishrinkImageXY( pImgIn, AoiX, AoiY, AoiXX, AoiYY,
                                               scene, pSubWin->colorUsed, shrinkX, shrinkY)) != 0) {
            //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_ISHRINK, ierr, "failed %d shrinking image");
            ierr = -SE_ISHRINK;
            goto syserrorExit;
          }
        } else {
          if( (ierr = inspUtil_imoveImage( pImgIn, AoiX, AoiY, AoiXX, AoiYY,
                                          scene, pSubWin->colorUsed)) != 0) {
            //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMOVE, ierr, "failed %d moving image");
            ierr = -SE_IMOVE;
            goto syserrorExit;
          }
        }

        // --------------- object on heap ----------------------

        if ((object = im_ucreateMem( NULL, TY_INT32, DV_HOST, getxx(pActPyr->iObject),
                                     getyy(pActPyr->iObject), NULL)) == IMNULL) {
            //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMCRE, 0, "failed to create image");
            ierr = -SE_IMCRE;
            goto syserrorExit;
        }

        // white reference correction out of precalculated pyramid
        if( (ierr = imove_32to32_wr(pActPyr->iObject, object, 1.0 /* / pRef->refData.wr_inFactor[pMySubWinPar->colorUsed]*/)) != 0) {
          //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_IMOVE, ierr, "error %d moving image");
          ierr = -SE_IMOVE;
          goto syserrorExit;
        }

        // -----------------------------------------------------------------------
        // do the correlation
        // -----------------------------------------------------------------------

        PRINTL2(TRACE_DEBUG, "used object %3d x %3d\n", getxx( object), getyy( object));
        PRINTL2(TRACE_DEBUG, "used scene  %3d x %3d\n", getxx( scene), getyy( scene));
        PRINTL2(TRACE_DEBUG, "used correl %3d x %3d\n", getxx( correl), getyy( correl));
        PRINTL4(TRACE_DEBUG, " sceneoff 1:1 %3d/%3d shr. %3d/%3d\n", AoiX, AoiY, AoiX / shrinkX, AoiY / shrinkY);

        if( (ierr = gcrcdf32( object, scene, correl, pSip->vcorrel)) != 0) {

          // gcrcdf32 is called with INTERPOL1 => it cannot give back ierr == -10 (from internally called extposv)
          //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_CRCDF, ierr, "error %d in correlation");
          ierr = -SE_CRCDF;
          goto syserrorExit;
        }

        // ...

        if ( level < pSip->pSubWin[ i].pyrDesc.nLevels - 1) {
          // check that we do not have a plateau inside correl (without 1 pixel border, used for interpolation)
          sfloat *pCorr, valCorr;
          int x, y, xx, yy, havePlateau;

          xx = getxx(correl);
          yy = getyy(correl);
          havePlateau = TRUE;

          // 03.07.2012 RR: Bugfix for unidirectinal search ranges

          if( yy == 1) {
            /* case: unidirectional interpolation in x */

            pCorr = (sfloat *)pixadt(1, 0, correl, sfloat);
            valCorr = *pCorr;                               // preset
            for (x = 1; x < xx - 1; x++) {
              if (*pCorr++ != valCorr) {
                havePlateau = FALSE;
                break;
              }
            }
          } else if ( xx == 1) {
            /* case: unidirectional interpolation in y */

            pCorr = (sfloat *)pixadt(0, 1, correl, sfloat);
            valCorr = *pCorr;                               // preset
            for (y = 1; y < yy - 1; y++) {
              pCorr = (sfloat *)pixadt(0, y, correl, sfloat);
              if (*pCorr++ != valCorr) {
                havePlateau = FALSE;
                break;
              }
            }
          } else {
            /* case: bidirectional interpolation */

            pCorr = (sfloat *)pixadt(1, 1, correl, sfloat);
            valCorr = *pCorr;                               // preset
            for (y = 1; y < yy - 1; y++) {
              pCorr = (sfloat *)pixadt(1, y, correl, sfloat);
              for (x = 1; x < xx - 1; x++) {
                if (*pCorr++ != valCorr) {
                  havePlateau = FALSE;
                  break;
                }
              }
              if (havePlateau == FALSE) {
                break;
              }
            }
          }

          if (havePlateau == TRUE) {
            PRINTL0(TRACE_DEBUG, "all points of same value: no extremum => ABORT\n");
            goto abortIter;
          }
        }

        if ((ierr = extposv(correl, pSip->vcorrel, INTERPOL3_R, &xPrec, &yPrec)) < 0 ) {
          if (ierr == -10) {
            /* correlation extr. on image border => failed to determine position */
            if (level >= pSip->pSubWin[ i].pyrDesc.nLevels - 1) {
              // first run
              pSubWin->InspError = PPCWINTR1_INSPERR_POS;
              goto nextWindow;
            } else {
              PRINTL0(TRACE_DEBUG, "correlation extr. on image border => ABORT\n");
              goto abortIter;
            }
          } else {
            //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_CRCDF, ierr, "error %d in correlation");
            ierr = -SE_CRCDF;
            goto syserrorExit;
          }
        } else if (ierr > 0) {
          // ierr == 1 all points of same value: no extremum
          if (level >= pSip->pSubWin[ i].pyrDesc.nLevels - 1) {
            // first run
            pSubWin->InspError = PPCWINTR1_INSPERR_POS;
            goto nextWindow;
          } else {
            PRINTL0(TRACE_DEBUG, "all points of same value: no extremum => ABORT\n");
            goto abortIter;
          }
        }

        // ...

        if (level >= pSip->pSubWin[ i].pyrDesc.nLevels - 1) {

          // first run
          // -----------------------------------------------------------------------
          // check quality of correlation
          // -----------------------------------------------------------------------
          if ((ierr = cdf2_pqual(correl, 0, pSubWin->CorrNorm,
            (int32)getxx(object) * getyy(object))) < 0) {
              /* bad correlation pattern => failed to determine position */
              pSubWin->InspError = PPCWINTR1_INSPERR_POS;
              goto nextWindow;
          }

          CorrQuality = ierr;     // correlation quality
          pSubWin->InspQual = CorrQuality;

          if( CorrQuality < minProcQual) { // test correlation quality
            /* bad correlation pattern => failed to determine position */
            pSubWin->InspError = PPCWINTR1_INSPERR_POS;
            goto nextWindow;
          }
        }

        // trafosys is computed with 6 bit precision
        dxIter = dto32((xPrec * 64.0 - (double)(xTolN << 6)) * (double)shrinkX);
        dyIter = dto32((yPrec * 64.0 - (double)(yTolN << 6)) * (double)shrinkY);

        PRINTL2(TRACE_DEBUG, "uncorrected dxIter %lf, dyIter %lf\n", (double)dxIter / 64.0, (double)dyIter / 64.0);

        // we pretransformad only in hole pixels, so subtract the primary
        // subpixel value
        if (xTol != 0) dxIter -= (int32)(64. * xd) - (xci << 6);
        if (yTol != 0) dyIter -= (int32)(64. * yd) - (yci << 6);

        PRINTL2(TRACE_DEBUG, "subpixel pretransform correction: %.2lf %.2lf\n",
          (double)((int32)(64. * xd) - (xci << 6)) / 64.0,
          (double)((int32)(64. * yd) - (yci << 6)) / 64.0);

        // add additional translation
        pmat = (sfloat *)vgetpm(pSip->vmat);
        /* transform system was calculated with points with 6 bit precision,
        internally it is hold with 4 bit precision for b1, b2 */
        pmat[B1] += (float)dxIter / (float)4.0;
        pmat[B2] += (float)dyIter / (float)4.0;

        PRINTL2(TRACE_DEBUG, "dxIter %lf, dyIter %lf\n", (double)dxIter / 64.0, (double)dyIter / 64.0);
        dx += dxIter;
        dy += dyIter;
        PRINTL2(TRACE_DEBUG, "dx %lf, dy %lf\n", (double)dx / 64.0, (double)dy / 64.0);

        // remove temporary images
        im_remove(object);
        object = IMNULL;
        im_remove(scene);
        scene = IMNULL;
        im_remove(correl);
        correl = IMNULL;

      } // end of iteration loop

abortIter:

      // be sure
      if (pSip->pSubWin[ i].pyrDesc.uniY == TRUE) {
        dx = 0;
      }
      if (pSip->pSubWin[ i].pyrDesc.uniX == TRUE) {
        dy = 0;
      }

      // due to rounding we can search a little bit more than the tolerance
      // so check tolerance
      dxU = ((double)dx / 64.0) * YaIPS_Calib_UPP_X;  // / (double)ProjTrafo[ pRef->refNr][A11]; // to units
      dyU = ((double)dy / 64.0) * YaIPS_Calib_UPP_Y;  // / (double)ProjTrafo[ pRef->refNr][A22]; // to units

      if (dxU >= 0.0) {
        xaDelta = dxU;
      } else {
        xaDelta = -dxU;
      }
      if (dyU >= 0.0) {
        yaDelta = dyU;
      } else {
        yaDelta = -dyU;
      }
      // check tolerance/search rane
      if (xaDelta > (double)pSubWin->PosToleranceX ||
          yaDelta > (double)pSubWin->PosToleranceY) {

        pSubWin->InspError = PPCWINTR1_INSPERR_SEARCRANGE;
        goto nextWindow;
      } else {
        // Nothing to do here
      }
    }

    // -----------------------------------------------------------------------
    // no error
    // -----------------------------------------------------------------------

    pin  = (int32 *)vgetpm(pSip->vin);
    pref = (int32 *)vgetpm(pSip->vref);

    pin  += 2 * numValid;
    pref += 2 * numValid;

    /* precision 1/64 pixel */

    // 17.02.2012 RR: Working with positions saved at first run gives exact results

    if (xxsDraw > 0 && yysDraw > 0) {  // have valid corner position of first pyramid level

      *pref++ = dto32( xRef_first * 64.0);
      *pref   = dto32( yRef_first * 64.0);

      *pin++ = dto32( xd_first * 64.0) + dx;
      *pin   = dto32( yd_first * 64.0) + dy;
    } else {

      *pref++ = ((int32)pSubWin->AOI.XPos << 6) +
                ((int32)pSubWin->AOI.XSize << 5);
      *pref   = ((int32)pSubWin->AOI.YPos << 6) +
                ((int32)pSubWin->AOI.YSize << 5);

      *pin++ = pref[-1];
      *pin   = pref[0];
    }

    numValid += 1;

    pSubWin->LastDX = dx / 64.0;        // Last position deviation
    pSubWin->LastDY = dy / 64.0;

    vputnm(pSip->vin,  (int32)(2 * numValid));
    vputnm(pSip->vref, (int32)(2 * numValid));

nextWindow:

    // delete for next subwindow
    if (object != IMNULL) {
      im_remove(object);
      object = IMNULL;
    }
    if (scene  != IMNULL) {
      im_remove(scene);
      scene = IMNULL;
    }
    if (correl != IMNULL) {
      im_remove(correl);
      correl = IMNULL;
    }

  }

  // -----------------------------------------------------------------------
  // exitPoint
  // -----------------------------------------------------------------------

//x/exitPoint:

  // Collect errors

  pWin->CheckError = 0;
  pWin->CheckText[ 0] = '\0';

  for (i = 0; i < nAOIs; i++) {

    pSubWin = &pWin->AOI[i];

    YaIPS_InspectCode2Text( pSubWin->InspError, pSubWin->InspText, sizeof( pSubWin->InspText));

    // Catch first error
    if( pSubWin->InspError != 0 &&       // This has an error
        pWin->CheckError == 0) {         // and no main error

      pWin->CheckError = pSubWin->InspError;

      sprintf( pWin->CheckText, "%d: %s", i + 1, pSubWin->InspText);
    }
  }

  if( pWin->CheckError == 0) {
    //x/double dphi, cos_a;

    if( nAOIs >= 3)  {                 // Have three ore more AOIs

      if (vgetnm(pSip->vin) < 6 || vgetnm(pSip->vref) < 6) {
        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_ILLNPOINT, 0, "illegal number of points");
        ierr = -SE_ILLNPOINT;
        goto syserrorExit;
      }

      if (vgetnm(pSip->vin) > 6 && vgetnm(pSip->vref) > 6) {
        ierr = vgeoest(pSip->vin, pSip->vref, pSip->vmat,
                       MIN_DEV, MAX_RUNS, 0, pSip->v1t, pSip->vtmp);
      } else {
        ierr = vgeoest(pSip->vin, pSip->vref, pSip->vmat,
                       0, 0, 0, pSip->v1t,pSip->vtmp);
      }

      if (ierr == -14 || ierr == -15 || ierr == -16 || ierr == -17) {
        /* illegal points for calculation of transform system */
        pWin->CheckError = PPCWINTR1_INSPERR_ILLTRAFO;
        goto badTransformSystem;
      }

      if (ierr) {
        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VGEOEST, ierr, "error %d in vgeoest()");
        ierr = -SE_VGEOEST;
        goto syserrorExit;
      }

      // plausicheck for meaningful values
      pmat = (sfloat *)vgetpm(pSip->vmat);

      if ((pmat[0] < 0.1 && pmat[0] > -0.1) || pmat[0] > 10.0 || pmat[0] < -10 ||
          pmat[1] > 10.0 || pmat[1] < -10 ||
          pmat[2] > 10.0 || pmat[2] < -10 ||
          (pmat[3] < 0.1 && pmat[3] > -0.1) || pmat[3] > 10.0 || pmat[3] < -10 ||
          pmat[4] / 64.0 >  10 * pImgIn->w() ||
          pmat[4] / 64.0 < -10 * pImgIn->w() ||
          pmat[5] / 64.0 >  10 * pImgIn->h() ||
          pmat[5] / 64.0 < -10 * pImgIn->h()) {
        /* illegal points for calculation of transform system */
        pWin->CheckError = PPCWINTR1_INSPERR_ILLTRAFO;
        goto badTransformSystem;
      }

      /* enter transform system in results */
      pmat = (sfloat *)vgetpm( pSip->vmat);

      /* transform system was calculated with points with 6 bit precision,
         internally it is hold with 4 bit precision for b1, b2 */

      pWin->DeltaX = pmat[4] / 64.0;
      pWin->DeltaY = pmat[5] / 64.0;

#ifdef use_again
      dphi = asin((double)pmat[1]);
#else
      /* Transform system:
          /        \     /              \     /        \     /      \
         | A11  A12 |   | Sx cos  Sy sin |   | cos  sin |   | Sx  0  |
         |          | = |                | = |          | * |        |
         | A21  A22 |   |-Sx sin  Sy cos |   |-sin  cos |   | 0   Sy |
          \        /     \              /     \        /     \      /

         assuming no shearing we get phi out of A12/A22
      */
      if( (double)pmat[1] >= -0.0001 && (double)pmat[1] <= 0.0001 &&
          (double)pmat[3] >= -0.0001 && (double)pmat[3] <= 0.0001) {
        /* illegal points for calculation of transform system */
        pWin->CheckError = PPCWINTR1_INSPERR_ILLTRAFO;
        goto badTransformSystem;
      }

      pWin->DeltaA = atan2((double)pmat[1], (double)pmat[3]) * 180.0 / M_PI;
#endif
      pWin->T_Matrix[ A11] = pmat[ A11];
      pWin->T_Matrix[ A12] = pmat[ A12];
      pWin->T_Matrix[ A21] = pmat[ A21];
      pWin->T_Matrix[ A22] = pmat[ A22];
      pWin->T_Matrix[ B1]  = pWin->DeltaX;
      pWin->T_Matrix[ B2]  = pWin->DeltaY;

      // Rotation center for the matrix is the left
      // upper corner (coordinate 0/0) of the image.
      // This give unusable /DeltaY values.

      // Estimate an object center and transform this gives
      // much better results.

      double P1X, P1Y, P2X, P2Y, PTX, PTY;

      pSubWin = pWin->AOI;

      // Use min/max center of the three AOIs

      P1X = (double)pSubWin[ 0].AOI.XPos + (double)pSubWin[ 0].AOI.XSize * 0.5;
      P1Y = (double)pSubWin[ 0].AOI.YPos + (double)pSubWin[ 0].AOI.YSize * 0.5;

      P2X = P1X;
      P2Y = P1Y;

      PTX = (double)pSubWin[ 1].AOI.XPos + (double)pSubWin[ 1].AOI.XSize * 0.5;
      PTY = (double)pSubWin[ 1].AOI.YPos + (double)pSubWin[ 1].AOI.YSize * 0.5;

      if( PTX < P1X) P1X = PTX;
      if( PTY < P1Y) P1Y = PTY;
      if( PTX > P2X) P2X = PTX;
      if( PTY > P2Y) P2Y = PTY;

      PTX = (double)pSubWin[ 2].AOI.XPos + (double)pSubWin[ 2].AOI.XSize * 0.5;
      PTY = (double)pSubWin[ 2].AOI.YPos + (double)pSubWin[ 2].AOI.YSize * 0.5;

      if( PTX < P1X) P1X = PTX;
      if( PTY < P1Y) P1Y = PTY;
      if( PTX > P2X) P2X = PTX;
      if( PTY > P2Y) P2Y = PTY;

      P1X = (P1X + P2X) * 0.5;    // Average min/max
      P1Y = (P1Y + P2Y) * 0.5;

      P2X = MATRIX_TRANSFORM_X( pWin->T_Matrix, P1X, P1Y);
      P2Y = MATRIX_TRANSFORM_Y( pWin->T_Matrix, P1X, P1Y);

      // Delta after transformation is the offset

      pWin->DeltaX = P2X - P1X;
      pWin->DeltaY = P2Y - P1Y;

    } else {  // Have one or too points (only translation)

      // Must be YAIPS_PSEARCH_MODE_1XY with one AOI

      pWin->DeltaX = pWin->AOI[ 0].LastDX;  // Use delta from first AOI
      pWin->DeltaY = pWin->AOI[ 0].LastDY;

      pWin->T_Matrix[ A11] = 1.0;
      pWin->T_Matrix[ A12] = 0.0;
      pWin->T_Matrix[ A21] = 0.0;
      pWin->T_Matrix[ A22] = 1.0;
      pWin->T_Matrix[ B1]  = pWin->DeltaX;
      pWin->T_Matrix[ B2]  = pWin->DeltaY;
    }

  }

badTransformSystem:

  if( pWin->CheckError == 0) {            // Have a position

    // Check tolerance

    float DeltaX, DeltaY;
    int ErrX, ErrY;

    DeltaX = pWin->DeltaX * YaIPS_Calib_UPP_X;    // Deviation in units
    if( DeltaX < 0) DeltaX = - DeltaX;            // Make absolute value
    ErrX = DeltaX > pWin->MaxDevX;                // Check over threshold

    DeltaY = pWin->DeltaY * YaIPS_Calib_UPP_X;    // Deviation in units
    if( DeltaY < 0) DeltaY = - DeltaY;            // Make absolute value
    ErrY = DeltaY > pWin->MaxDevY;                // Check over threshold

    if( ErrX || ErrY) {                           // Position deviation to high

      pWin->CheckError = PPCWINTR1_INSPERR_TOLERANCE;     // Set tolerance violation
    }

    if( nAOIs <= 2) {   // Only position deltas

      sprintf( pWin->CheckText, LangStringLookup( "%+.2f%s/%+.2f%s %s"),
                                pWin->DeltaX * YaIPS_Calib_UPP_X, ErrX ? "!" : "",
                                pWin->DeltaY * YaIPS_Calib_UPP_Y, ErrY ? "!" : "",
                                pYaIPS_Calib_Unit2String());

    } else {            // Position deltas + angle

      sprintf( pWin->CheckText, LangStringLookup( "%+.2f%s/%+.2f%s %s, %+.2f °"),
                                pWin->DeltaX * YaIPS_Calib_UPP_X, ErrX ? "!" : "",
                                pWin->DeltaY * YaIPS_Calib_UPP_Y, ErrY ? "!" : "",
                                pYaIPS_Calib_Unit2String(), pWin->DeltaA);
    }
  }

  // Ensure CheckText is set

  if( pWin->CheckError != 0 &&                 // Have an error on exit
      pWin->CheckText[ 0] == '\0') {           // but no text set until now

    YaIPS_InspectCode2Text( pWin->CheckError, pWin->CheckText, sizeof( pWin->CheckText));
  }

  ierr = pWin->CheckError;

syserrorExit:

  if (object != IMNULL)    im_remove(object);
  if (scene  != IMNULL)    im_remove(scene);
  if (correl != IMNULL)    im_remove(correl);

  return( ierr);

} /* int ppcWinTR1_InspDo() */

/************************************************************************************
* YaIPS_PSearchAOI_Draw
*
* Draw a position search AOI
*
* NOTE: Make a fl_push_clip() call before calling this function
*
*/

void YaIPS_PSearchAOI_Draw( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                            YaIPS_PSearchAOI_t *pPSearchAOI, // Pointer to PSearchAOI data
                            int Teach_mode,               // 0 = inspection mode, 1 = teach mode
                            int IsSelected,               // True if window is selected
                            int iAOI)                     // What AOI, is zero based
{
  int LineWidth;
  int x1, y1, OffX, OffY, DrawText, x1Temp, y1Temp, LenMarker;
  int AOIx1, AOIy1, AOIx2, AOIy2, AOIxx, AOIyy, CentX, CentY;
  int mdx, mdy, mw, mh, xTol, yTol;
  static char MyLineDashes[] = { 4, 4, 0};
  Fl_Color DrawColor;
  char TempString[ 256];

  // Preparations

  MyLineDashes[ 0] = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 4.0 + 0.5);
  if( MyLineDashes[ 0] < 4) {
    MyLineDashes[ 0] = 4;
  }
  MyLineDashes[ 1] =  MyLineDashes[ 0];

  x1 = pYaIPS_ImageDisp->BigImage_sx;
  y1 = pYaIPS_ImageDisp->BigImage_sy;

  LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

  // Points relative to image

  OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
  OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

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
  }

  // Draw AOIs

  if( Teach_mode) {                // Teach mode

    if( IsSelected) {     // Teach mode and mouse is over this AOI

      DrawColor = FL_RED;
    } else {

      DrawColor = FL_GREEN - 2;
    }

    if( pPSearchAOI->CheckError != 0) {

      if( ! IsSelected) {

        DrawColor = FL_MAGENTA;
      }

      sprintf( TempString, "%d-%s", iAOI + 1, pPSearchAOI->CheckText);

    } else {

      sprintf( TempString, "%d-Q%d", iAOI + 1, pPSearchAOI->CheckCorrQual);
    }

  } else {                                // Inspection mode

    if( pPSearchAOI->InspError != 0) {

      sprintf( TempString, "%d-%s", iAOI + 1, pPSearchAOI->InspText);

      DrawColor = FL_RED;

    } else {

      sprintf( TempString, "%d-Q%d", iAOI + 1, pPSearchAOI->InspQual);

      DrawColor = FL_GREEN - 2;
    }
  }

  fl_line_style( 0, LineWidth);   // Set line width
  fl_color( DrawColor);           // Color

  fl_line_style( 0, LineWidth);   // Set line width

  AOIx1 = (int)( (pPSearchAOI->AOI.XPos - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
  AOIy1 = (int)( (pPSearchAOI->AOI.YPos - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

  AOIxx = (int)( pPSearchAOI->AOI.XSize * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
  AOIyy = (int)( pPSearchAOI->AOI.YSize * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

  AOIx2 = AOIx1 + AOIxx - 1;
  AOIy2 = AOIy1 + AOIyy - 1;

  // Draw rectangle
  fl_rect( AOIx1, AOIy1, AOIxx, AOIyy);

  // Draw tolerance rectangle

  xTol = dto32( (pPSearchAOI->PosToleranceX / YaIPS_Calib_UPP_X) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);  // * ScaleX
  yTol = dto32( (pPSearchAOI->PosToleranceY / YaIPS_Calib_UPP_Y) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);  // * ScaleY

  fl_line_style( FL_DOT, 0, MyLineDashes);

  fl_rect( AOIx1 - xTol, AOIy1 - yTol, AOIxx + xTol + xTol, AOIyy + yTol + yTol);

  fl_line_style( 0);   // Reset to default

  // Draw center marker

  CentX = (AOIx1 + AOIx2) / 2;
  CentY = (AOIy1 + AOIy2) / 2;

  LenMarker = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 5.0 + 0.5);

  if( AOIyy > 10) {  /* mark inside */
    fl_line( CentX, AOIy1 + 1, CentX, AOIy1 + LenMarker);
    fl_line( CentX, AOIy2 - 1, CentX, AOIy2 - LenMarker);
    if( (AOIx2 - AOIx1) & 1) {
      CentX += 1;
      fl_line( CentX, AOIy1 + 1, CentX, AOIy1 + LenMarker);
      fl_line( CentX, AOIy2 - 1, CentX, AOIy2 - LenMarker);
    }
  } else {             /* mark outside */

    fl_line( CentX, AOIy1 - 1, CentX, AOIy1 - LenMarker);
    fl_line( CentX, AOIy2 + 1, CentX, AOIy2 + LenMarker);
    if( (AOIx2 - AOIx1) & 1) {
      CentX += 1;
      fl_line( CentX, AOIy1 - 1, CentX, AOIy1 - LenMarker);
      fl_line( CentX, AOIy2 + 1, CentX, AOIy2 + LenMarker);
    }
  }

  if( AOIxx > 10) {  /* mark inside */
    fl_line( AOIx1 + 1, CentY, AOIx1 + LenMarker, CentY);
    fl_line( AOIx2 - 1, CentY, AOIx2 - LenMarker, CentY);
    if( (AOIy2 - AOIy1) & 1) {
      CentY += 1;
      fl_line( AOIx1 + 1, CentY, AOIx1 + LenMarker, CentY);
      fl_line( AOIx2 - 1, CentY, AOIx2 - LenMarker, CentY);
    }
  } else {             /* mark outside */

    fl_line( AOIx1 - 1, CentY, AOIx1 - LenMarker, CentY);
    fl_line( AOIx2 + 1, CentY, AOIx2 + LenMarker, CentY);
    if( (AOIy2 - AOIy1) & 1) {
      CentY += 1;
      fl_line( AOIx1 - 1, CentY, AOIx1 - LenMarker, CentY);
      fl_line( AOIx2 + 1, CentY, AOIx2 + LenMarker, CentY);
    }
  }

  // Draw text

  if( 1) {   // Draw text

    x1Temp = AOIx1;
    y1Temp = AOIy1;

    fl_text_extents( TempString, mdx, mdy, mw, mh);

    if( y1Temp - y1 + mdy < 4) {    // To near to upper border
      y1Temp += mh + 5;             // Show below upper frame
      x1Temp += 4;
    } else {                       // Fits above upper frame
      y1Temp -= 4;
    }
    fl_draw( TempString, x1Temp, y1Temp);
  }

  // Finish up

  fl_line_style( 0);   // Reset to default
}

/****************************** End Of File ******************************/
