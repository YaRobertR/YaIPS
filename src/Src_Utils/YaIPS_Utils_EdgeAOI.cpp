/****************************************************************************

  YaIPS_Utils_EdgeAOI.cpp

  Common edge search support.

 20.03.2025 RR: First edition of this file.

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

// ...

#define LTOR 0
#define RTOL 1

//  DM minimum edge contrast (Modify) [Gray values]

#define EDGE_MIN_CONTRAST 10

// DM search range edges (Modify) [Pixel]

#define EDGE_DM_SEARCH_RANGE 8 // 5

// Size factor for nominal position marker

#define NOM_MARKER_FAK 1.5

// orientation

#define WIN_PAR_ORIENT_HORIZONTAL  0   // measure horizontal or X-Distance
#define WIN_PAR_ORIENT_VERTIKAL    1   // measure vertical or Y-Distance
#define WIN_PAR_ORIENT_PARAMETER   2   // measure orientation is given parameter

// syserror codes

#define MY_SYSERR_BASE     (10 * 100)

#define SE_IMDEF           (MY_SYSERR_BASE+0)  // failed to define image
#define SE_COLSUM          (MY_SYSERR_BASE+1)  // error %d in colsum()
#define SE_ROWSUM          (MY_SYSERR_BASE+2)  // error %d in rowsum()
#define SE_ILLTYPE         (MY_SYSERR_BASE+3)  // illegal window type %d
#define SE_ILLWINPARSIZ    (MY_SYSERR_BASE+4)  // illegal window parameter structure size %d
#define SE_ILLWINRESSIZ    (MY_SYSERR_BASE+5)  // illegal window result structure size %d
#define SE_VFILTN          (MY_SYSERR_BASE+6)  // error %d in vfiltn()
#define SE_VMINMAX         (MY_SYSERR_BASE+7)  // error %d in vminmax()
#define SE_VMMSORT2        (MY_SYSERR_BASE+8)  // error %d in vmmsort2()
#define SE_VCRE            (MY_SYSERR_BASE+9)  // failed to create vector
#define SE_VALLOC          (MY_SYSERR_BASE+10) // error %d allocating vector
#define SE_VREM            (MY_SYSERR_BASE+11) // error %d removing vector
#define SE_ILLNSUBW        (MY_SYSERR_BASE+12) // illegal number of subwindows %d
#define SE_IMCRE           (MY_SYSERR_BASE+13) // failed to create image
#define SE_I16TOI8         (MY_SYSERR_BASE+14) // error %d in i16toi8()
#define SE_IRGBTOIHS       (MY_SYSERR_BASE+15) // error %d in convertImage_rgb2ihs()
#define SE_IMDEFINE        (MY_SYSERR_BASE+16) // failed to define image
#define SE_LINESUM         (MY_SYSERR_BASE+17) // error %d in linesum()
#define SE_ILL_PARAMETER   (MY_SYSERR_BASE+18) // illegal window parameter
#define SE_VUPARCAL        (MY_SYSERR_BASE+19) // error %d in vuparcal()
#define SE_VGEOEST         (MY_SYSERR_BASE+20) // error %d in vgeoest()

/*
 =================
 pointToLineDistance

// Find the distance between a point (x0,y0) and a line specified by
// two points (x1, y1), (x2, y2)
 =================
 */
static double pointToLineDistance( int LineNr, double x0, double y0, double x1, double y1, double x2, double y2)
{
  double distF, TempSquare;

  TempSquare = (x2-x1) * (x2-x1) + (y2-y1) * (y2-y1);

  if( TempSquare > 0.0) {
    distF = ((x2-x1) * (y1-y0) - (x1-x0) * (y2-y1)) / sqrt( TempSquare);

    distF = - distF;   // negate here
  } else {
    distF = 0.0;
  }

#ifdef _DEBUG
  //printf2Console( "Dist %d: %7.2f\n", LineNr, distF);
#endif

  return( distF);
}

 /*
 =================
 LinePiceLength

  distance between points
 =================
 */
static double PointDistance( double x1, double y1, double x2, double y2)
{
  double distF, TempSquare;

  TempSquare = (x2-x1) * (x2-x1) + (y2-y1) * (y2-y1);

  distF = sqrt( TempSquare);

  return( distF);
}

/* ======================================================================= */

// Actions for free angle
// Bit 0 , 1: Which of the four lines will change (0 .. 3)
// Bit 2: changes XX if set, change YY if not set
// Bit 3: is inverser of bit 2, because we need minimum one of the bits set in the action return
// Bit 4: if set, change is negated
// Bit 5: if set, also change positon to other direction

#define ACTION_FA_MASK_NR     0x0003    // Bit Mask for Line Number
#define ACTION_FA_CHANGE_XX   0x0004
#define ACTION_FA_CHANGE_YY   0x0008

static int getWinMoreData( T_FreeAngleInfo *pFreeAngleInfo, Fl_YaIPS_AOI_t *pWinDesc,
                           double ytox, int clipXX, int clipYY,
                           double xA, double xB, double xC, double xD,
                           double yA, double yB, double yC, double yD) /*
======================================================================= */
{
  double xMin, xMax, yMin, yMax, d;
  int i, HighXIndex, LowXIndex;

  static int CursorShapeTable[ 8] = {
    FL_CURSOR_NESW, // WINACTION_BOTTOM | WINACTION_LEFT,
    FL_CURSOR_WE,   // WINACTION_LEFT,
    FL_CURSOR_NWSE, // WINACTION_LEFT | WINACTION_TOP,
    FL_CURSOR_NS,   // WINACTION_TOP,
    FL_CURSOR_NESW, // WINACTION_TOP | WINACTION_RIGHT,
    FL_CURSOR_WE,   // WINACTION_RIGHT,
    FL_CURSOR_NWSE, // WINACTION_RIGHT | WINACTION_BOTTOM,
    FL_CURSOR_NS    // WINACTION_BOTTOM
  };

  static int CursorActionTable[ 4] = {
    0 | ACTION_FA_CHANGE_XX,     // Line 0
    1 | ACTION_FA_CHANGE_YY,     // Line 1
    2 | ACTION_FA_CHANGE_XX,     // Line 2
    3 | ACTION_FA_CHANGE_YY      // Line 3
  };

  // move points to array

  pFreeAngleInfo->xP[ 0] = xA;
  pFreeAngleInfo->yP[ 0] = yA;

  pFreeAngleInfo->xP[ 1] = xB;
  pFreeAngleInfo->yP[ 1] = yB;

  pFreeAngleInfo->xP[ 2] = xD;
  pFreeAngleInfo->yP[ 2] = yD;

  pFreeAngleInfo->xP[ 3] = xC;
  pFreeAngleInfo->yP[ 3] = yC;

  // get minmas and maximas

  xMin = pFreeAngleInfo->xP[ 0];
  xMax = pFreeAngleInfo->xP[ 0];
  yMin = pFreeAngleInfo->yP[ 0];
  yMax = pFreeAngleInfo->yP[ 0];

  for( i = 1; i < 4; i++) {

    if( xMin > pFreeAngleInfo->xP[ i]) {
      xMin = pFreeAngleInfo->xP[ i];
    }

    if( xMax < pFreeAngleInfo->xP[ i]) {
      xMax = pFreeAngleInfo->xP[ i];
    }

    if( yMin > pFreeAngleInfo->yP[ i]) {
      yMin = pFreeAngleInfo->yP[ i];
    }

    if( yMax < pFreeAngleInfo->yP[ i]) {
      yMax = pFreeAngleInfo->yP[ i];
    }
  }

  // Surrounding rectangle is extended by one pixel to the outside

  pFreeAngleInfo->SurroundingRectangle.XPos = (int)floor( xMin) - 1;
  pFreeAngleInfo->SurroundingRectangle.YPos = (int)floor( yMin) - 1;

  pFreeAngleInfo->SurroundingRectangle.XSize = (int)ceil( xMax) - (int)floor( xMin) + 1 + 2;
  pFreeAngleInfo->SurroundingRectangle.YSize = (int)ceil( yMax) - (int)floor( yMin) + 1 + 2;

  // get position for text

  HighXIndex = 0;
  LowXIndex = 0;

  for( i = 1; i < 4; i++) {

    // HighXIndex

    d = pFreeAngleInfo->yP[ i] - pFreeAngleInfo->yP[ HighXIndex];
    if( d < 0) {
      d = - d;
    }

    if( d <= 1.5) {    // fast keine Shieflage

      // take more left

      if( pFreeAngleInfo->xP[ i] < pFreeAngleInfo->xP[ HighXIndex]) {

        HighXIndex = i;
      }
    } else {

      // take higher (more to top of screen)

      if( pFreeAngleInfo->yP[ i] < pFreeAngleInfo->yP[ HighXIndex]) {

        HighXIndex = i;
      }
    }

    // LowXIndex

    d = pFreeAngleInfo->yP[ i] - pFreeAngleInfo->yP[ LowXIndex];
    if( d < 0) {
      d = - d;
    }

    if( d <= 1.5) {    // fast keine Shieflage

      // take more left

      if( pFreeAngleInfo->xP[ i] < pFreeAngleInfo->xP[ LowXIndex]) {

        LowXIndex = i;
      }
    } else {

      // take lower (more to bottom of screen)

      if( pFreeAngleInfo->yP[ i] > pFreeAngleInfo->yP[ LowXIndex]) {

        LowXIndex = i;
      }
    }
  }

#ifdef use_again
  if( floor( pFreeAngleInfo->yP[ HighXIndex]) - gp_attr.charyy > 0) {

    pFreeAngleInfo->HeaderTextXPos = (int)floor( pFreeAngleInfo->xP[ HighXIndex]);
    pFreeAngleInfo->HeaderTextYPos = (int)floor( pFreeAngleInfo->yP[ HighXIndex]);
  } else {

    pFreeAngleInfo->HeaderTextXPos = (int)floor( pFreeAngleInfo->xP[ LowXIndex]);
    pFreeAngleInfo->HeaderTextYPos = (int)ceil( pFreeAngleInfo->yP[ LowXIndex]) + gp_attr.charyy + 1;
  }

  if( pFreeAngleInfo->HeaderTextXPos < 0) {
    pFreeAngleInfo->HeaderTextXPos = 0;
  }

#else

  pFreeAngleInfo->TextXPosHigh = (int)floor( pFreeAngleInfo->xP[ HighXIndex]);
  pFreeAngleInfo->TextYPosHigh = (int)floor( pFreeAngleInfo->yP[ HighXIndex]);

  pFreeAngleInfo->TextXPosLow = (int)floor( pFreeAngleInfo->xP[ LowXIndex]);
  pFreeAngleInfo->TextYPosLow = (int)ceil( pFreeAngleInfo->yP[ LowXIndex]);

  if( pFreeAngleInfo->TextXPosHigh < 0) {
    pFreeAngleInfo->TextXPosHigh = 0;
  }

#endif

  // cursor Shape things

  LowXIndex = 0;

  if( pFreeAngleInfo->OrientationAngle > 22.5 + 135.0) {

    LowXIndex = 4;

  } else if( pFreeAngleInfo->OrientationAngle > 22.5 + 90.0) {

    LowXIndex = 5;

  } else if( pFreeAngleInfo->OrientationAngle >= 22.5 + 45.0) {

    LowXIndex = 6;

  } else if( pFreeAngleInfo->OrientationAngle >= 22.5) {

    LowXIndex = 7;

  } else if( pFreeAngleInfo->OrientationAngle < -22.5 - 135.0) {

    LowXIndex = 4;

  } else if( pFreeAngleInfo->OrientationAngle < -22.5 - 90.0) {

    LowXIndex = 3;

  } else if( pFreeAngleInfo->OrientationAngle <= -22.5 - 45.0) {

    LowXIndex = 2;

  } else if( pFreeAngleInfo->OrientationAngle <= -22.5) {

    LowXIndex = 1;
  }

  for( i = 0; i < 4; i++) {

    pFreeAngleInfo->CursorPointShape[ i] = CursorShapeTable[ (LowXIndex + i * 2) % 8];
    pFreeAngleInfo->CursorLineShape[ i]  = CursorShapeTable[ (LowXIndex + i * 2 + 1) % 8];
  }

  // cursor Action things

  LowXIndex = 0;

  if( pFreeAngleInfo->OrientationAngle > 135.0) {

    LowXIndex = 2;

  } else if( pFreeAngleInfo->OrientationAngle >= 45.0) {

    LowXIndex = 3;

  } else if( pFreeAngleInfo->OrientationAngle < -135.0) {

    LowXIndex = 2;

  } else if( pFreeAngleInfo->OrientationAngle <= -45.0) {

    LowXIndex = 1;
  }

  for( i = 0; i < 4; i++) {

    CursorActionTable[ (LowXIndex + i) % 4] &= ~ACTION_FA_MASK_NR;
    CursorActionTable[ (LowXIndex + i) % 4] |= i;
  }

  for( i = 0; i < 4; i++) {
    pFreeAngleInfo->CursorPointAction[ i] = CursorActionTable[ (LowXIndex + i) % 4] | (CursorActionTable[ (LowXIndex + i + 3) % 4] << 8);
    pFreeAngleInfo->CursorLineAction[ i]  = CursorActionTable[ (LowXIndex + i) % 4];
  }

  return( 0);
} /* static int getWinMoreData() */

static int getWinCoordinates( T_FreeAngleInfo * pFreeAngleInfo, Fl_YaIPS_AOI_t *pWinDesc,
                              int OrientationAngle, double ytox, int mode,
                              int clipXX, int clipYY, double peakdev,
                              int *retnSample, int *retnQuer,
                              double *retx0, double *rety0, double *retx1, double *rety1,
                              double *retxA, double *retxB, double *retxC, double *retxD,
                              double *retyA, double *retyB, double *retyC, double *retyD,
                              double *retxAP, double *retyAP, double *retxBP, double *retyBP,
                              double *pdxScanDir, double *pdyScanDir, double *pdxSideDir, double *pdySideDir) /*
============================================================================================= */
// mode=0: calculate window border coordinates only
// mode=1: calculate peak position coordinates only
// mode=2: calculate both
{
  double x0, y0, x1, y1, xPeak, yPeak;
  double dxScanDir, dyScanDir, dxSideDir, dySideDir;
  int TempAngle, nSample, nQuer;
  double CenterX, CenterY, LengthScanDir, LengthSideDir;
  double xA, xB, xC, xD, yA, yB, yC, yD;

  dxScanDir =   cos((double)OrientationAngle * M_PI / 180.0) * ytox;
  dyScanDir = - sin((double)OrientationAngle * M_PI / 180.0);

  // get positive angle
  TempAngle = OrientationAngle;
  if( TempAngle < 0) {
    TempAngle = - TempAngle;
  }

  // Depending from dir get length in scan direction and length in the 90 degree direction
  if( TempAngle >= 45 && TempAngle <= 135) {

    nSample       = pWinDesc->YSize;
    nQuer         = pWinDesc->XSize;
    LengthScanDir = pWinDesc->YSize - 1.0;
    LengthSideDir = pWinDesc->XSize - 1.0;
  } else {

    nSample       = pWinDesc->XSize;
    nQuer         = pWinDesc->YSize;
    LengthScanDir = pWinDesc->XSize - 1.0;
    LengthSideDir = pWinDesc->YSize - 1.0;
  }

  // calculate window center point, compensate 1 pixel for border drawing

  CenterX = (double)pWinDesc->XPos + (double)pWinDesc->XSize * 0.5;
  CenterY = (double)pWinDesc->YPos + (double)pWinDesc->YSize * 0.5;

  // calculate window center line

  x0 = CenterX - dxScanDir * (LengthScanDir * 0.5);
  y0 = CenterY - dyScanDir * (LengthScanDir * 0.5);
  x1 = CenterX + dxScanDir * (LengthScanDir * 0.5);
  y1 = CenterY + dyScanDir * (LengthScanDir * 0.5);

  // get slope to the side

  dxSideDir =   cos((double)(OrientationAngle + 90) * M_PI / 180.0) * ytox;
  dySideDir = - sin((double)(OrientationAngle + 90) * M_PI / 180.0);

  xA = (double)x0 - dxSideDir * (LengthSideDir * 0.5);
  yA = (double)y0 - dySideDir * (LengthSideDir * 0.5);
  xB = (double)x0 + dxSideDir * (LengthSideDir * 0.5);
  yB = (double)y0 + dySideDir * (LengthSideDir * 0.5);
  xC = (double)x1 - dxSideDir * (LengthSideDir * 0.5);
  yC = (double)y1 - dySideDir * (LengthSideDir * 0.5);
  xD = (double)x1 + dxSideDir * (LengthSideDir * 0.5);
  yD = (double)y1 + dySideDir * (LengthSideDir * 0.5);

  if( mode != 1) {
    *retx0 = x0;
    *rety0 = y0;
    *retx1 = x1;
    *rety1 = y1;
    *retxA = xA;
    *retyA = yA;
    *retxB = xB;
    *retyB = yB;
    *retxC = xC;
    *retyC = yC;
    *retxD = xD;
    *retyD = yD;
  }

  // calculate line coordinates for peak position
  if( mode != 0) {
    xPeak = x0 + dxScanDir * (peakdev) * ytox;
    yPeak = y0 + dyScanDir * (peakdev);
    *retxAP = xPeak - dxSideDir * (LengthSideDir * 0.5);
    *retyAP = yPeak - dySideDir * (LengthSideDir * 0.5);
    *retxBP = xPeak + dxSideDir * (LengthSideDir * 0.5);
    *retyBP = yPeak + dySideDir * (LengthSideDir * 0.5);
  }

  // optional returns

  if( retnSample != (int *)NULL) {
    *retnSample = nSample;
  }

  if( retnQuer != (int *)NULL) {
    *retnQuer = nQuer;
  }

  if( pdxScanDir != (double *)NULL) {
    *pdxScanDir = dxScanDir;
  }

  if( pdyScanDir != (double *)NULL) {
    *pdyScanDir = dyScanDir;
  }

  if( pdxSideDir != (double *)NULL) {
    *pdxSideDir = dxSideDir;
  }

  if( pdySideDir != (double *)NULL) {
    *pdySideDir = dySideDir;
  }

  // collect same more info

  pFreeAngleInfo->OrientationAngle = OrientationAngle;

  // get more date

  getWinMoreData( pFreeAngleInfo, pWinDesc, ytox,
                  clipXX, clipYY,
                  xA, xB, xC, xD, yA, yB, yC, yD);

  return(0);
} /* static int getWinCoordinates() */

static int getPeakContrast(Tvector *vsrc,
                           Tvector *vsrcNoDiff, int minMax, int dir,
                           int indexPeak, int32 *retContrast,
                           int *retlPos, int *retrPos) /*
============================================================== */
{
  int32 *p32, *p32NoDiff;
  int32 peakval, actVal, lastVal, slope, maxSlope, lCon, rCon, isPosP, isPosA;
  int i, j;

  if( indexPeak < 0 || indexPeak >= vgetnm(vsrc)) {
    *retContrast = 0;
    return(-1);
  }
  if( indexPeak < 0 || indexPeak >= vgetnm(vsrcNoDiff)) {
    *retContrast = 0;
    return(-1);
  }
  //PRINTL3(TRACE_DEBUG, "getPeakContrast minMax %d, dir %d indexPeak %d\n", minMax, dir, indexPeak);

  p32NoDiff = (int32 *)vgetpm(vsrcNoDiff);
  p32       = (int32 *)vgetpm(vsrc);

  peakval = p32[indexPeak];
  if( peakval < 0) {
    isPosP = 0;
  } else {
    isPosP = 1;
  }

  // to the left
  actVal = peakval;
  maxSlope = MININT32;
  for (i = indexPeak - 1; i >= 0; i--) {
    lastVal = actVal;
    actVal = p32[i];
    if( minMax == 0) { // minimum
      slope = actVal - lastVal;
    } else {           // maximum
      slope = lastVal - actVal;
    }
    if( slope > maxSlope) maxSlope = slope;
    if( actVal < 0) {
      isPosA = 0;
    } else {
      isPosA = 1;
    }
    if( slope < maxSlope / 4  || (isPosA != isPosP)) break; // end of peak reached
  }
  if( i < 0) i = 0;
  lCon = p32NoDiff[indexPeak];
  if( (minMax == 0 && dir == LTOR) ||
      (minMax == 1 && dir == RTOL)) {
    for (j = indexPeak - 1; j > i; j--) {
      if( p32NoDiff[j] > lCon) lCon = p32NoDiff[j];
    }
  } else {
    for (j = indexPeak - 1; j > i; j--) {
      if( p32NoDiff[j] < lCon) lCon = p32NoDiff[j];
    }
  }
  //PRINTL3(TRACE_DEBUG, "getPeakContrast left %d...%d (con %d)\n", i, indexPeak, lCon);

  *retlPos = i;

  // to the right
  actVal = peakval;
  maxSlope = MININT32;
  for (i = indexPeak + 1; i < vgetnm(vsrc); i++) {
    lastVal = actVal;
    actVal = p32[i];
    if( minMax == 0) { // minimum
      slope = actVal - lastVal;
    } else {           // maximum
      slope = lastVal - actVal;
    }
    if( slope > maxSlope) maxSlope = slope;
    if( actVal < 0) {
      isPosA = 0;
    } else {
      isPosA = 1;
    }
    if( slope < maxSlope / 4  || (isPosA != isPosP)) break; // end of peak reached
  }
  if( i >= vgetnm(vsrc)) i = vgetnm(vsrc) - 1;
  rCon = p32NoDiff[indexPeak];
  if( (minMax == 0 && dir == LTOR) ||
    (minMax == 1 && dir == RTOL)) {
      for (j = indexPeak + 1; j < i; j++) {
        if( p32NoDiff[j] < rCon) rCon = p32NoDiff[j];
      }
  } else {
    for (j = indexPeak + 1; j < i; j++) {
      if( p32NoDiff[j] > rCon) rCon = p32NoDiff[j];
    }
  }
  //PRINTL3(TRACE_DEBUG, "getPeakContrast right %d...%d (con %d)\n", indexPeak, i, rCon);

  *retrPos = i;

  // return the real edge contrast
  if( (minMax == 0 && dir == LTOR) ||
    (minMax == 1 && dir == RTOL)) {
      *retContrast = lCon - rCon;
  } else {
    *retContrast = rCon - lCon;
  }
  //PRINTL1(TRACE_DEBUG, "getPeakContrast contrast %d\n", *retContrast);

  return(0);

} /* static int getPeakContrast() */

/************************************************************************************
* DistanceFromPixelPos
*
* Calculate distance from the edge positions.
*
* return     0 OK
*          < 0 Error
*/
static int YaIPS_EdgeDM_DistanceFromPixelPos( YaIPS_RGB_ImgD_t *pSrc,      // Input image
                                              YaIPS_EdgeDM_t *pEdgeDM,     // Point to edge DM data
                                              double *pRetDist)            // Return distance here
{
  double peakXpos[2], peakYpos[2];
  YaIPS_EdgeAOI_t *pEdgeAOI;
  double dist;
  int i;
  double xAP, yAP, xBP, yBP;
  double x0, y0, x1, y1, xA, xB, xC, xD, yA, yB, yC, yD; /* border coordinates */
  double dxScanDir, dyScanDir, dxSideDir, dySideDir;
  int    nSample, nQuer;
  T_FreeAngleInfo FreeAngleInfo;

  for( i = 0; i < 2; i++) {        // always have 2 subwindows so this loop is OK

    if( i == 0) {
      pEdgeAOI = &pEdgeDM->SW_1;
    } else {
      pEdgeAOI = &pEdgeDM->SW_2;
    }

    if( pEdgeAOI->refPeak < 0.0) {     // was not calculated
      return( -1);                     // return error
    }

    switch( pEdgeDM->orientation) {

    case WIN_PAR_ORIENT_HORIZONTAL:

      peakXpos[i] = pEdgeAOI->refPeak + pEdgeAOI->AOI.XPos;             // back to absolute position
      peakYpos[i] = (double)pEdgeAOI->AOI.YPos + (double)pEdgeAOI->AOI.YSize * 0.5;
#ifdef use_again      // 07.07.2025 RR: Lens correction is not supported
      inspUtil_LensCorrection( &peakXpos[i], &peakYpos[i],       // lens correction of this point
        getxx( iref), getyy( iref),
        inspData.PLensFac, inspData.cytox, inspData.camClass == 2)
#endif
      break;

    case WIN_PAR_ORIENT_VERTIKAL:

      peakXpos[i] = (double)pEdgeAOI->AOI.XPos + (double)pEdgeAOI->AOI.XSize * 0.5;
      peakYpos[i] = pEdgeAOI->refPeak + pEdgeAOI->AOI.YPos;
#ifdef use_again      // 07.07.2025 RR: Lens correction is not supported
      inspUtil_LensCorrection( &peakXpos[i], &peakYpos[i],       // lens correction of this point
        getxx( iref), getyy( iref),
        inspData.PLensFac, inspData.cytox, inspData.camClass == 2);
#endif
      break;

    default:

      // WIN_PAR_ORIENT_PARAMETER

      getWinCoordinates( &FreeAngleInfo, &pEdgeAOI->AOI, pEdgeDM->OrientationAngle, YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y, 0,
                         pSrc->xx, pSrc->yy, 0.0, &nSample, &nQuer,
                         &x0, &y0, &x1, &y1, &xA, &xB, &xC, &xD, &yA, &yB, &yC, &yD,
                         &xAP, &yAP, &xBP, &yBP,
                         &dxScanDir, &dyScanDir, &dxSideDir, &dySideDir);

      peakXpos[i] = x0 + pEdgeAOI->refPeak * dxScanDir;
      peakYpos[i] = y0 + pEdgeAOI->refPeak * dyScanDir;
#ifdef use_again      // 07.07.2025 RR: Lens correction is not supported
      inspUtil_LensCorrection( &peakXpos[i], &peakYpos[i],       // lens correction of this point
        getxx( iref), getyy( iref),
        inspData.PLensFac, inspData.cytox, inspData.camClass == 2);
#endif
      break;
    } // end switch
  }

  switch( pEdgeDM->orientation) {

  case WIN_PAR_ORIENT_HORIZONTAL:

    dist = peakXpos[0] - peakXpos[1];

    // measure absolute ?
    if( pEdgeDM->DifferenceMeas == 0) {
      if( dist < 0.0) dist = -dist;
    }

    // enter in results in units
#ifdef use_again
    dist /= (double)ProjTrafo[ RefNr][A11]; // to units
#else
    dist *= YaIPS_Calib_UPP_X; // to units
#endif

    break;

  case WIN_PAR_ORIENT_VERTIKAL:

    dist = peakYpos[0] - peakYpos[1];

    // measure absolute ?
    if( pEdgeDM->DifferenceMeas == 0) {
      if( dist < 0.0) dist = -dist;
    }

    // enter in results in units
#ifdef use_again
    dist /= (double)ProjTrafo[ RefNr][A22]; // to units
#else
    dist *= YaIPS_Calib_UPP_Y; // to units
#endif

    break;

  default:

    // WIN_PAR_ORIENT_PARAMETER

    // points to units

#ifdef use_again
    peakXpos[0] /= (double)ProjTrafo[ RefNr][A11]; // to units
    peakXpos[1] /= (double)ProjTrafo[ RefNr][A11]; // to units

    peakYpos[0] /= (double)ProjTrafo[ RefNr][A22]; // to units
    peakYpos[1] /= (double)ProjTrafo[ RefNr][A22]; // to units
#else
    peakXpos[0] *= YaIPS_Calib_UPP_X; // to units
    peakXpos[1] *= YaIPS_Calib_UPP_X; // to units

    peakYpos[0] *= YaIPS_Calib_UPP_Y; // to units
    peakYpos[1] *= YaIPS_Calib_UPP_Y; // to units
#endif

    // get slope to the side

    dxSideDir =   cos((double)(pEdgeDM->OrientationAngle + 90) * M_PI / 180.0);
    dySideDir = - sin((double)(pEdgeDM->OrientationAngle + 90) * M_PI / 180.0);

    // distance point 0 to line through point 1

    dist = pointToLineDistance( 0, peakXpos[0], peakYpos[0],
                                peakXpos[1], peakYpos[1],
                                peakXpos[1] + dxSideDir, peakYpos[1] + dySideDir);

    // measure absolute ?
    if( pEdgeDM->DifferenceMeas == 0) {
      if( dist < 0.0) dist = -dist;
    }

    break;
  } // end switch

  *pRetDist = dist;

  return( 0);
}

static int allocVectors( YaIPS_EdgeDM_t *pEdgeDM,
                         Tvector **vtmp32_1, Tvector **vtmp32_2,
                         Tvector **vtmp32_3, Tvector **vtmp16_1) /*
=============================================================== */
{
  int ierr, i, size;
  YaIPS_EdgeAOI_t *pSubWin;

  // walk through all subwindows
  for( i = 0; i < 2; i++) {

    if( i == 0) {
      pSubWin = &pEdgeDM->SW_1;
    } else {
      pSubWin = &pEdgeDM->SW_2;
    }

    if( i == 0) {
      size = pSubWin->AOI.XSize;
    }

    if( size < pSubWin->AOI.XSize) {
      size = pSubWin->AOI.XSize;
    }

    if( size < pSubWin->AOI.YSize) {
      size = pSubWin->AOI.YSize;
    }
  }

  if( (*vtmp32_1 = ve_ucreate(DV_HOST)) == VENULL) {

    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  ierr = ve_alloc(*vtmp32_1, size, sizeof(int32), TY_INT32);
  if( ierr != 0) {

    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  if( (*vtmp32_2 = ve_ucreate(DV_HOST)) == VENULL) {

    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  ierr = ve_alloc(*vtmp32_2, size, sizeof(int32), TY_INT32);
  if( ierr != 0) {

    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  if( (*vtmp32_3 = ve_ucreate(DV_HOST)) == VENULL) {

    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  ierr = ve_alloc(*vtmp32_3, size, sizeof(int32), TY_INT32);
  if( ierr != 0) {

    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  if( (*vtmp16_1 = ve_ucreate(DV_HOST)) == VENULL) {

    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  ierr = ve_alloc(*vtmp16_1, size / 2 + 1, sizeof(int16), TY_INT16);
  if( ierr != 0) {

    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  ierr = 0;

syserrorExit:

  return(ierr);

} /* static int allocVectors() */

static int allocVectors( YaIPS_EdgePM_t *pEdgePM,
                         Tvector **vtmp32_1, Tvector **vtmp32_2,
                         Tvector **vtmp32_3, Tvector **vtmp16_1) /*
=============================================================== */
{
  int ierr, i, size, nAOIs;
  YaIPS_EdgeAOI_t *pSubWin;

  // walk through all subwindows

  nAOIs = (pEdgePM->MeasureMode / 2) + 1;

  if( nAOIs < 1 || nAOIs > YAIPS_EDGEPM_MAX_AOIS) {  // Security test

    ierr = -SE_ILL_PARAMETER;
    goto syserrorExit;
  }

  for( i = 0; i < nAOIs; i++) {

    pSubWin = &pEdgePM->Edges[ i];

    if( i == 0) {
      size = pSubWin->AOI.XSize;
    }

    if( size < pSubWin->AOI.XSize) {
      size = pSubWin->AOI.XSize;
    }

    if( size < pSubWin->AOI.YSize) {
      size = pSubWin->AOI.YSize;
    }
  }

  if( (*vtmp32_1 = ve_ucreate(DV_HOST)) == VENULL) {

    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  ierr = ve_alloc(*vtmp32_1, size, sizeof(int32), TY_INT32);
  if( ierr != 0) {

    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  if( (*vtmp32_2 = ve_ucreate(DV_HOST)) == VENULL) {

    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  ierr = ve_alloc(*vtmp32_2, size, sizeof(int32), TY_INT32);
  if( ierr != 0) {

    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  if( (*vtmp32_3 = ve_ucreate(DV_HOST)) == VENULL) {

    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  ierr = ve_alloc(*vtmp32_3, size, sizeof(int32), TY_INT32);
  if( ierr != 0) {

    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  if( (*vtmp16_1 = ve_ucreate(DV_HOST)) == VENULL) {

    ierr = -SE_VCRE;
    goto syserrorExit;
  }
  ierr = ve_alloc(*vtmp16_1, size / 2 + 1, sizeof(int16), TY_INT16);
  if( ierr != 0) {

    ierr = -SE_VALLOC;
    goto syserrorExit;
  }

  ierr = 0;

syserrorExit:

  return(ierr);

} /* static int allocVectors() */

/************************************************************************************
* removeVectors
*/

static int removeVectors(Tvector **vtmp32_1, Tvector **vtmp32_2,
                         Tvector **vtmp32_3, Tvector **vtmp16_1) /*
=============================================================== */
{
  int ierr;

  if( *vtmp32_1 != VENULL) {
    ierr = ve_remove(*vtmp32_1);
    if( ierr != 0) {

      ierr = -SE_VREM;
      goto syserrorExit;
    }
  }
  if( *vtmp32_2 != VENULL) {
    ierr = ve_remove(*vtmp32_2);
    if( ierr != 0) {

      ierr = -SE_VREM;
      goto syserrorExit;
    }
  }
  if( *vtmp32_3 != VENULL) {
    ierr = ve_remove(*vtmp32_3);
    if( ierr != 0) {

      ierr = -SE_VREM;
      goto syserrorExit;
    }
  }
  if( *vtmp16_1 != VENULL) {
    ierr = ve_remove(*vtmp16_1);
    if( ierr != 0) {

      ierr = -SE_VREM;
      goto syserrorExit;
    }
  }

  ierr = 0;

syserrorExit:

  return(ierr);

} /* static int removeVectors() */

/************************************************************************************
* findPeak
*
* Get edges from a specific AOI
*
* return     0 OK
*          < 0 Error
*/

static int findPeak( Fl_YaIPS_AOI_t *pWinDesc, YaIPS_RGB_ImgD_t *im, YaIPS_RGB_ImgD_t *iref,
                     int orientation,            // 0: x-distance, 1: y-distance, 2: orientation is given parameter
                     int OrientationAngle,       // orientation angle for mode (180 .. - 180 Grad)
                     int ByteComponent,    // What byte to use from the pixel
                     Tvector *vtmp32_1, Tvector *vtmp32_2, Tvector *vtmp32_3, Tvector *vtmp16_1,
                     int dir, int minMax, int teachMode,
                     int minCon, int subPix, int dispMode, double ytox,
                     double *retIdx, int *retCon, int *retlConPos, int *retrConPos, int *pNomPos)
{
  int ierr = 0, numPeaks, iPeaks, idxPeak, OffsetPeak, nomPos;
  int16 filter[ 9], nFilter, *p16;
  int32 *p32, contrast;
  // variables for free angle
  double xAP, yAP, xBP, yBP;
  double x0, y0, x1, y1, xA, xB, xC, xD, yA, yB, yC, yD; /* border coordinates */
  double dxScanDir, dyScanDir, dxSideDir, dySideDir;
  int    nSample, nQuer;
  T_FreeAngleInfo FreeAngleInfo;
  int UseWiderFilter = false;          // 08.07.2025 RR: May be a parameter later

  // ...

  minCon = minCon * 4;               // << 2 due to lowpass amplification

  // do the row/colsum
  switch( orientation) {

  case WIN_PAR_ORIENT_HORIZONTAL:

    nSample = im->xx;
    nQuer   = im->yy;

    minCon = minCon * nQuer;   // due to colsum

    ierr = YaIPS_RGB_Colsum( im, vtmp32_2, ByteComponent, 0);         // sum mode
    if( ierr) {

      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_COLSUM, ierr, "error %d in colsum()");
      return(-SE_COLSUM);
    }
    break;

  case WIN_PAR_ORIENT_VERTIKAL:

    nSample = im->yy;
    nQuer   = im->xx;

    minCon = minCon * nQuer;   // due to rowsum

    ierr = YaIPS_RGB_Rowsum( im, vtmp32_2, ByteComponent, 0);         // sum mode
    if( ierr) {

      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_ROWSUM, ierr, "error %d in rowsum()");
      return(-SE_ROWSUM);
    }
    break;

  default:

    // WIN_PAR_ORIENT_PARAMETER

    getWinCoordinates( &FreeAngleInfo, pWinDesc, OrientationAngle, ytox, 0,
                       iref->xx, iref->yy, 0.0, &nSample, &nQuer,
                       &x0, &y0, &x1, &y1, &xA, &xB, &xC, &xD, &yA, &yB, &yC, &yD,
                       &xAP, &yAP, &xBP, &yBP,
                       &dxScanDir, &dyScanDir, &dxSideDir, &dySideDir);

    minCon = minCon * nQuer;   // due to linesum

    ierr = YaIPS_RGB_LinesumBiLin( im, vtmp32_2, ByteComponent,
                      x0 - FreeAngleInfo.SurroundingRectangle.XPos, y0 - FreeAngleInfo.SurroundingRectangle.YPos,
                      x1 - FreeAngleInfo.SurroundingRectangle.XPos, y1 - FreeAngleInfo.SurroundingRectangle.YPos,
                      nSample, nQuer, 0, ytox);
    if( ierr) {

      //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_LINESUM, ierr, "error %d in linesum()");
      return(-SE_LINESUM);
    }
    break;
  } // end switch

  nomPos = nSample / 2;          // Nominal position by 1/2 search length

  if( pNomPos != NULL) {         // Return nominal position

    *pNomPos = nomPos;
  }

  // Test worst case problems, mindest width must be biggest kernel size

  if( UseWiderFilter == false /*(pMyWinPar->WinDMOpt & WINDM_OPT_WIDER_DIFF_FILT) == 0*/) {   // use smaller filter
    nFilter = 3;
  } else {                                                        // use wider filter
    nFilter = 7;
  }

  if( vgetnm(vtmp32_2) < nFilter) {   // is too short
    // filtering not possible
    *retIdx = 0.0;
    *retCon = 0;
    goto exitPoint;
  }

  // ...

  nFilter = 3;
  filter[0] = 1; filter[1] = 2; filter[2] = 1;
  if( (ierr = vfiltn(vtmp32_2, vtmp32_3, 0, 0, nFilter, filter)) != 0) {

    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VFILTN, ierr, "error %d in vfiltn()");
    return(-SE_VFILTN);
  }

  if( dir == LTOR) {
    if( UseWiderFilter == false /*(pMyWinPar->WinDMOpt & WINDM_OPT_WIDER_DIFF_FILT) == 0*/ ) {   // use smaller filter
      nFilter = 3;
      filter[0] = -1; filter[1] = 0; filter[2] = 1;
    } else {                                                        // use wider filter
      nFilter = 7;
      filter[0] = -1; filter[1] = -2; filter[2] = -1; filter[3] = 0; filter[4] = 1; filter[5] = 2; filter[6] = 1;
    }
  } else {
    if( UseWiderFilter == false /*(pMyWinPar->WinDMOpt & WINDM_OPT_WIDER_DIFF_FILT) == 0*/) {   // use smaller filter
      nFilter = 3;
      filter[0] =  1; filter[1] = 0; filter[2] = -1;
    } else {                                                        // use wider filter
      nFilter = 7;
      filter[0] = 1; filter[1] = 2; filter[2] = 1; filter[3] = 0; filter[4] = -1; filter[5] = -2; filter[6] = -1;
    }
  }
  if( (ierr = vfiltn(vtmp32_2, vtmp32_1, 0, 0, nFilter, filter)) != 0) {

    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VFILTN, ierr, "error %d in vfiltn()");
    return(-SE_VFILTN);
  }

  if( teachMode != 0) {             // Teach mode

    // 03.12.2008 RR: nominal position is now handled in findPeak()
    //                limit search range to nominal position

    int NomIndexLow, NomIndexHigh;

    if( nomPos < 0) {            // security test
      nomPos = 0;
    }

    if( nomPos >= nSample) {     // security test
      nomPos = nSample - 1;
    }

    if( pNomPos != NULL) {         // Return nominal position

      *pNomPos = nomPos;
    }

    NomIndexLow = nomPos - (EDGE_DM_SEARCH_RANGE + 1);

    if( NomIndexLow < 0) {            // security test
      NomIndexLow = 0;
    }

    if( NomIndexLow >= nSample) {     // security test
      NomIndexLow = nSample - 1;
    }

    NomIndexHigh = nomPos + (EDGE_DM_SEARCH_RANGE+ 1);

    if( NomIndexHigh < 0) {            // security test
      NomIndexHigh = 0;
    }

    if( NomIndexHigh >= nSample) {     // security test
      NomIndexHigh = nSample - 1;
    }

    if( NomIndexHigh - NomIndexLow + 1 < 3) {  // test with OK
      // skip this subwindow
      *retIdx = 0.0;
      *retCon = 0;
      goto exitPoint;
    }

    // copy down

    if( NomIndexLow > 0) {

      p32 = (int32 *)vgetpm(vtmp32_1);
      memcpy( p32, p32 + NomIndexLow, sizeof( int32) * (NomIndexHigh - NomIndexLow + 1));

      p32 = (int32 *)vgetpm(vtmp32_3);
      memcpy( p32, p32 + NomIndexLow, sizeof( int32) * (NomIndexHigh - NomIndexLow + 1));
    }

    vputnm( vtmp32_1, NomIndexHigh - NomIndexLow + 1);
    vputnm( vtmp32_3, NomIndexHigh - NomIndexLow + 1);

    OffsetPeak = NomIndexLow;       // resulting peak is offset by this

#ifdef use_again
    numPeaks = 1;                   // search biggest peak
#else
    // 12.07.2025 RR: Support multiple beaks with same value

    numPeaks = (NomIndexHigh - NomIndexLow + 1 + 3) / 4;  // Round up 1/4
#endif

  } else {                          // Inspection mode, search all peaks

    numPeaks = nSample / 2;
    if( numPeaks < 1) numPeaks = 1; // be sure

    OffsetPeak = 0;                 // resulting peak is offset by this
  }

  // get minima/maxima
  if( (ierr = vminmax(vtmp32_1, vtmp16_1, minMax, numPeaks, 2)) != 0) {

    //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VMINMAX, ierr, "error %d in vminmax()");
    return(-SE_VMINMAX);
  }

  if( vgetnm(vtmp16_1) < 1) {
    *retIdx = 0.0;
    *retCon = 0;
    goto exitPoint;
  }

  if( teachMode != 0) {             // Teach mode

    // 12.07.2025 RR: Support multiple beaks with same value

    int nPeaks;
    int BestPeak, BestValue, BestDist, ThisValue, ThisDist;
    int32 *pValues;            // pointer to values
    int16 *pPeaks;             // pointer to peak positions

    nPeaks = vgetnm( vtmp16_1);

    if( nPeaks > 1) {              // Have more than one

      // Sort by height  values
      if( (ierr = vmmsort2(vtmp32_1, vtmp16_1, minMax, 0)) != 0) {

        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VMMSORT2, ierr, "error %d in vmmsort2()");
        return(-SE_VMMSORT2);
      }

      BestPeak  = -1;
      BestValue = 0;
      BestDist  = 0;

      pValues = (int32 *)vgetpm(vtmp32_1);        // pointer to values
      pPeaks  = (int16 *)vgetpm(vtmp16_1);        // pointer to peak positions

      for( iPeaks = 0; iPeaks < nPeaks; iPeaks++) {

        ThisValue = pValues[ pPeaks[ iPeaks]];     // Value of this peak
        ThisDist =  nomPos - OffsetPeak - pPeaks[ iPeaks];  // Distance to nominal position
        if( ThisDist < 0) ThisDist = - ThisDist;

        if( iPeaks > 0 &&                 // NOT the first peak
            ThisValue != BestValue) {     // next peak has lower value than first peak(s)
          break;                          // can break loop
        }

        if( BestPeak < 0 ||               // First peak
            ThisDist < BestDist) {        // or shorter distance to nominal value

          BestPeak  = iPeaks;
          BestValue = ThisValue;
          BestDist  = ThisDist;
        }
      }

      if( BestPeak > 0) {                // Was not the first peak


        pPeaks[ 0] = pPeaks[ BestPeak];  // Copy down to first place
      }

      vputnm( vtmp16_1, 1);              // Set number of peaks to 1
    }

  } else {                          // Inspection mode, search all peaks

    if( dir == LTOR) {
      if( (ierr = vmmsort2(vtmp32_1, vtmp16_1, minMax, 1)) != 0) {

        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VMMSORT2, ierr, "error %d in vmmsort2()");
        return(-SE_VMMSORT2);
      }
    } else {
      if( (ierr = vmmsort2(vtmp32_1, vtmp16_1, minMax, 2)) != 0) {

        //x/sysstate_setsyserr(MY_SYSERR_MODULE, SE_VMMSORT2, ierr, "error %d in vmmsort2()");
        return(-SE_VMMSORT2);
      }
    }

    if( vgetnm(vtmp16_1) < 1) {  // be sure
      *retIdx = 0.0;
      *retCon = 0;
      goto exitPoint;
    }
  }

  for( iPeaks = 0; iPeaks < vgetnm(vtmp16_1); iPeaks++) {

    p16 = (int16 *)vgetpm(vtmp16_1);
    idxPeak = p16[iPeaks];

    if( idxPeak <= 0 || idxPeak >= vgetnm(vtmp32_1) - 1) continue;

    // determine contrast of peak
    getPeakContrast( vtmp32_1, vtmp32_3, minMax, dir, idxPeak, &contrast, retlConPos, retrConPos);

    *retIdx = (double)(idxPeak + OffsetPeak);

    *retCon = dto32((double)contrast / (4.0 * (double)nQuer));

    if( contrast >= minCon) {
      if( subPix != 0) { // parabola interpolation
        if( (idxPeak > 0) && (idxPeak < (vgetnm(vtmp32_1) - 1))) {
          int32 val1, val2, val3, nenner;

          p32 = (int32 *)vgetpm(vtmp32_1);

          val1 = p32[idxPeak - 1];
          val2 = p32[idxPeak];
          val3 = p32[idxPeak + 1];

          nenner = 2 * (val1 - 2 * val2 + val3);
          if( nenner != 0) {
            *retIdx = (double)(idxPeak + OffsetPeak) + (double)(val1 - val3) / (double)nenner;
          }
        }
      }

      goto exitPoint;

    } else {

#ifdef use_again
      if( dispMode > 0 && Trace.traceDebug >= TRACE_SEVERE_ERROR) {

        display_slinestyle((int16)0xffff); // solid

        switch( pMyWinPar->orientation) {

        case WIN_PAR_ORIENT_HORIZONTAL:

#ifdef use_again // 04.12.2008 RR: relative AOI
          display_line(idisp, pWinDesc->XPos + dto32(*retIdx), pWinDesc->YPos,
                       pWinDesc->XPos + dto32(*retIdx), pWinDesc->YPos + getyy(idisp) - 1, SIPGR_BLUE);
#else
          display_line(idisp, dto32(*retIdx), 0, dto32(*retIdx), getyy(idisp) - 1, SIPGR_BLUE);
#endif
          break;

        case WIN_PAR_ORIENT_VERTIKAL:

#ifdef use_again // 04.12.2008 RR: relative AOI
          display_line( idisp, pWinDesc->XPos, pWinDesc->YPos + dto32(*retIdx),
                        pWinDesc->XPos + getxx(idisp) - 1, pWinDesc->YPos + dto32(*retIdx), SIPGR_BLUE);
#else
          display_line( idisp, 0, dto32(*retIdx), getxx(idisp) - 1, dto32(*retIdx), SIPGR_BLUE);
#endif
          break;

        default:

          // WIN_PAR_ORIENT_PARAMETER

          getWinCoordinates( &FreeAngleInfo, pWinDesc, pMyWinPar->OrientationAngle, ytox, 1,
                             getxx( iref), getyy( iref), *retIdx, &nSample, &nQuer,
                             &x0, &y0, &x1, &y1, &xA, &xB, &xC, &xD, &yA, &yB, &yC, &yD,
                             &xAP, &yAP, &xBP, &yBP,
                             &dxScanDir, &dyScanDir, &dxSideDir, &dySideDir);

#ifdef use_again // 04.12.2008 RR: relative AOI
          display_line( idisp, dto32(xAP), dto32(yAP), dto32(xBP), dto32(yBP), SIPGR_BLUE);
#else
          display_line( idisp,
                        dto32(xAP) - FreeAngleInfo.SurroundingRectangle.XPos,
                        dto32(yAP) - FreeAngleInfo.SurroundingRectangle.YPos,
                        dto32(xBP) - FreeAngleInfo.SurroundingRectangle.XPos,
                        dto32(yBP) - FreeAngleInfo.SurroundingRectangle.YPos,
                        SIPGR_BLUE);
#endif
          break;
        } // end switch
      }
#endif
    }
    // try next peak
  }

  *retIdx = 0.0;   // no peak found
  *retCon = 0;

exitPoint:

  return(ierr);
}

/************************************************************************************
* YaIPS_EdgeDM_Inspect_AOI
*
* Get edges from a specific AOI
*
* return     0 OK
*          < 0 Error
*/

static int YaIPS_EdgeDM_Inspect_AOI( YaIPS_RGB_ImgD_t *pSrc,      // Input image
                                     YaIPS_EdgeAOI_t *pEdgeAOI,   // Point to specific AOI
                                     int orientation,            // 0: x-distance, 1: y-distance, 2: orientation is given parameter
                                     int OrientationAngle,        // orientation angle for mode (180 .. - 180 Grad)
                                     int AOI_Nr,                  // AOI number, 0 = any, else 1 .. 3
                                     int Teach_mode,              // 0 = inspection mode, 1 = teach mode
                                     Tvector *vtmp32_1, Tvector *vtmp32_2,  // Working vectors
                                     Tvector *vtmp32_3, Tvector *vtmp16_1)
{
  int ierr;
  YaIPS_RGB_ImgD_t iAOI;
  float relPeak, DistNomPos1, DistNomPos2;
  int contrast, minmax, colorUsed;
  double ytox;
  int contrastTmp, contrast1, lConPos, rConPos, rNomPos;
  double peakTmp;
  //
  double xAP, yAP, xBP, yBP;
  double x0, y0, x1, y1, xA, xB, xC, xD, yA, yB, yC, yD; /* border coordinates */
  double dxScanDir, dyScanDir, dxSideDir, dySideDir;
  int    nSample, nQuer;
  T_FreeAngleInfo FreeAngleInfo;

  ytox = YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y;

  // Preset edge results with illegal

  relPeak   = -1.0;      // preset illegal
  contrast  = 0;
  minmax    = 0;
  colorUsed = 0;

  // Define image on AOI
  switch( orientation) {

  case WIN_PAR_ORIENT_HORIZONTAL:
  case WIN_PAR_ORIENT_VERTIKAL:

    ierr = YaIPS_ImgD_AOI( &iAOI, pSrc, &pEdgeAOI->AOI);

    break;
  default:

    // WIN_PAR_ORIENT_PARAMETER

    getWinCoordinates( &FreeAngleInfo, &pEdgeAOI->AOI, OrientationAngle, ytox, 0,
                       pSrc->xx, pSrc->yy, 0.0, &nSample, &nQuer,
                       &x0, &y0, &x1, &y1, &xA, &xB, &xC, &xD, &yA, &yB, &yC, &yD,
                       &xAP, &yAP, &xBP, &yBP,
                       &dxScanDir, &dyScanDir, &dxSideDir, &dySideDir);

    ierr = YaIPS_ImgD_AOI( &iAOI, pSrc,
                           FreeAngleInfo.SurroundingRectangle.XPos, FreeAngleInfo.SurroundingRectangle.YPos,
                           FreeAngleInfo.SurroundingRectangle.XSize, FreeAngleInfo.SurroundingRectangle.YSize);
    break;
  }

  if( ierr != 0) {                         // touching windows border

    // Check error code
    ierr = YAIPS_EDGEDM_ERR_IMBORDER + AOI_Nr;
    goto exitPoint;
  }

  // Get edges

  if( Teach_mode != 0) {             // Teach mode

    // Check color channel 0 always. Is Black/White channel or red channel.

    if( (ierr = findPeak( &pEdgeAOI->AOI, &iAOI, pSrc,
                          orientation, OrientationAngle, 0,
                          vtmp32_1, vtmp32_2, vtmp32_3, vtmp16_1,
                          pEdgeAOI->r2l_b2t, 0, Teach_mode,
                          EDGE_MIN_CONTRAST, 1, 0, ytox, &peakTmp, &contrastTmp, &lConPos, &rConPos, &rNomPos)) != 0) {

      // sets syserror itself
      goto syserrorExit;
    }

    contrast1 = contrastTmp;
    DistNomPos1 = rNomPos - peakTmp;
    if( DistNomPos1 < 0.0) DistNomPos1 = - DistNomPos1;

    if( contrastTmp > contrast) {

      contrast  = contrastTmp;
      minmax    = 0;
      colorUsed = 0;
      relPeak   = peakTmp;
    }

    if( (ierr = findPeak( &pEdgeAOI->AOI, &iAOI, pSrc,
                          orientation, OrientationAngle, 0,
                          vtmp32_1, vtmp32_2, vtmp32_3, vtmp16_1,
                          pEdgeAOI->r2l_b2t, 1, Teach_mode,
                          EDGE_MIN_CONTRAST, 1, 0, ytox, &peakTmp, &contrastTmp, &lConPos, &rConPos, &rNomPos)) != 0) {

      // sets syserror itself
      goto syserrorExit;
    }

    DistNomPos2 = rNomPos - peakTmp;
    if( DistNomPos2 < 0.0) DistNomPos2 = - DistNomPos2;

#ifdef use_again
    if( contrastTmp > contrast ||
        (contrast1 == contrastTmp && contrast1 == contrast && DistNomPos2 < DistNomPos1)) { // If same contrast take nearer min or max
#else
    if( (contrast == 0 && contrastTmp > contrast) ||
        (contrast1 != contrast && contrastTmp > contrast) ||
        (contrast1 == contrast && contrast1 >= contrastTmp / 2 && DistNomPos2 < DistNomPos1)) { // If same contrast take nearer min or max
#endif

      contrast  = contrastTmp;
      minmax    = 1;
      colorUsed = 0;
      relPeak   = peakTmp;
    }

    if( pSrc->d >= 3) {                 // Have a color image

      // Check green channel.

      if( (ierr = findPeak( &pEdgeAOI->AOI, &iAOI, pSrc,
                            orientation, OrientationAngle, 1,
                            vtmp32_1, vtmp32_2, vtmp32_3, vtmp16_1,
                            pEdgeAOI->r2l_b2t, 0, Teach_mode,
                            EDGE_MIN_CONTRAST, 1, 0, ytox, &peakTmp, &contrastTmp, &lConPos, &rConPos, &rNomPos)) != 0) {

        // sets syserror itself
        goto syserrorExit;
      }

      contrast1 = contrastTmp;
      DistNomPos1 = rNomPos - peakTmp;
      if( DistNomPos1 < 0.0) DistNomPos1 = - DistNomPos1;

      if( contrastTmp > contrast) {

        contrast  = contrastTmp;
        minmax    = 0;
        colorUsed = 1;
        relPeak   = peakTmp;
      }

      if( (ierr = findPeak( &pEdgeAOI->AOI, &iAOI, pSrc,
                            orientation, OrientationAngle, 1,
                            vtmp32_1, vtmp32_2, vtmp32_3, vtmp16_1,
                            pEdgeAOI->r2l_b2t, 1, Teach_mode,
                            EDGE_MIN_CONTRAST, 1, 0, ytox, &peakTmp, &contrastTmp, &lConPos, &rConPos, &rNomPos)) != 0) {

        // sets syserror itself
        goto syserrorExit;
      }

      DistNomPos2 = rNomPos - peakTmp;
      if( DistNomPos2 < 0.0) DistNomPos2 = - DistNomPos2;

#ifdef use_again
      if( contrastTmp > contrast ||
          (contrast1 == contrastTmp && contrast1 == contrast && DistNomPos2 < DistNomPos1)) { // If same contrast take nearer min or max
#else
      if( (contrast == 0 && contrastTmp > contrast) ||
          (contrast1 != contrast && contrastTmp > contrast) ||
          (contrast1 == contrast && contrast1 >= contrastTmp / 2 && DistNomPos2 < DistNomPos1)) { // If same contrast take nearer min or max
#endif

        contrast  = contrastTmp;
        minmax    = 1;
        colorUsed = 1;
        relPeak   = peakTmp;
      }

      // Check blue channel.

      if( (ierr = findPeak( &pEdgeAOI->AOI, &iAOI, pSrc,
                            orientation, OrientationAngle, 2,
                            vtmp32_1, vtmp32_2, vtmp32_3, vtmp16_1,
                            pEdgeAOI->r2l_b2t, 0, Teach_mode,
                            EDGE_MIN_CONTRAST, 1, 0, ytox, &peakTmp, &contrastTmp, &lConPos, &rConPos, &rNomPos)) != 0) {

        // sets syserror itself
        goto syserrorExit;
      }

      contrast1 = contrastTmp;
      DistNomPos1 = rNomPos - peakTmp;
      if( DistNomPos1 < 0.0) DistNomPos1 = - DistNomPos1;

      if( contrastTmp > contrast) {

        contrast  = contrastTmp;
        minmax    = 0;
        colorUsed = 2;
        relPeak   = peakTmp;
      }

      if( (ierr = findPeak( &pEdgeAOI->AOI, &iAOI, pSrc,
                            orientation, OrientationAngle, 2,
                            vtmp32_1, vtmp32_2, vtmp32_3, vtmp16_1,
                            pEdgeAOI->r2l_b2t, 1, Teach_mode,
                            EDGE_MIN_CONTRAST, 1, 0, ytox, &peakTmp, &contrastTmp, &lConPos, &rConPos, &rNomPos)) != 0) {

        // sets syserror itself
        goto syserrorExit;
      }

      DistNomPos2 = rNomPos - peakTmp;
      if( DistNomPos2 < 0.0) DistNomPos2 = - DistNomPos2;

#ifdef use_again
      if( contrastTmp > contrast ||
          (contrast1 == contrastTmp && contrast1 == contrast && DistNomPos2 < DistNomPos1)) { // If same contrast take nearer min or max
#else
      if( (contrast == 0 && contrastTmp > contrast) ||
          (contrast1 != contrast && contrastTmp > contrast) ||
          (contrast1 == contrast && contrast1 >= contrastTmp / 2 && DistNomPos2 < DistNomPos1)) { // If same contrast take nearer min or max
#endif

        contrast  = contrastTmp;
        minmax    = 1;
        colorUsed = 2;
        relPeak   = peakTmp;
      }
    }

  } else {                          // Inspection mode, search all peaks

    if( (ierr = findPeak( &pEdgeAOI->AOI, &iAOI, pSrc,
                          orientation, OrientationAngle, pEdgeAOI->colorUsed,
                          vtmp32_1, vtmp32_2, vtmp32_3, vtmp16_1,
                          pEdgeAOI->r2l_b2t, pEdgeAOI->minMax, Teach_mode,
                          (pEdgeAOI->minPeakConPerc * pEdgeAOI->refPeakCon + 50) / 100,
                          1, 0, ytox, &peakTmp, &contrastTmp, &lConPos, &rConPos, NULL)) != 0) {

      // sets syserror itself
      goto syserrorExit;
    }
    if( contrastTmp > contrast) {
      contrast  = contrastTmp;
      minmax    = pEdgeAOI->minMax;
      colorUsed = pEdgeAOI->colorUsed;
      relPeak   = peakTmp;
    }
  }

exitPoint:
syserrorExit:

  pEdgeAOI->LastPeak    = relPeak;      // Peak position
  pEdgeAOI->LastPeakCon = contrast;     // Peak contrast of last edge found

  if( Teach_mode != 0) {                // Teach mode

    // Latch setting
    pEdgeAOI->refPeak    = relPeak;
    pEdgeAOI->refPeakCon = contrast;
    pEdgeAOI->minMax     = minmax;
    pEdgeAOI->colorUsed  = colorUsed;
  }

  // Check error code
  if( ierr == 0 &&
      pEdgeAOI->LastPeak <= 0) {          // NO Peak

    // Check error code
    ierr = YAIPS_EDGEDM_ERR_ENF  + AOI_Nr;
  }

  return( ierr);
}

/************************************************************************************
* YaIPS_EdgeDM_Inspect
*
*/

static void YaIPS_CheckCode2Text( int CheckError,
                                  char *pCheckText,
                                  int SizeOfCheckText)
{

  switch( CheckError) {

  case 0:   // This should be handled outside this function

    strncpy( pCheckText, "Coded elsewhere !", SizeOfCheckText - 1);
    break;
  case YAIPS_EDGEDM_ERR_NO_TEACH:
    strncpy( pCheckText, LangStringLookup( "&Utils_EdgeDM_InspErr1=NOT teached!"), SizeOfCheckText - 1);
    break;
  case YAIPS_EDGEDM_ERR_IMBORDER:
    strncpy( pCheckText, LangStringLookup( "&Utils_EdgeDM_InspErr2=Any image border!"), SizeOfCheckText - 1);
    break;
  case YAIPS_EDGEDM_ERR_1_IMBORDER:
    strncpy( pCheckText, LangStringLookup( "&Utils_EdgeDM_InspErr3=.1 image border!"), SizeOfCheckText - 1);
    break;
  case YAIPS_EDGEDM_ERR_2_IMBORDER:
    strncpy( pCheckText, LangStringLookup( "&Utils_EdgeDM_InspErr4=.2 image border!"), SizeOfCheckText - 1);
    break;
  case YAIPS_EDGEDM_ERR_3_IMBORDER:
    strncpy( pCheckText, LangStringLookup( "&Utils_EdgeDM_InspErr5=.3 image border!"), SizeOfCheckText - 1);
    break;
  case YAIPS_EDGEDM_ERR_ENF:
    strncpy( pCheckText, LangStringLookup( "&Utils_EdgeDM_InspErr6=Any Edge!"), SizeOfCheckText - 1);
    break;
  case YAIPS_EDGEDM_ERR_1_ENF:
    strncpy( pCheckText, LangStringLookup( "&Utils_EdgeDM_InspErr7=.1 Edge!"), SizeOfCheckText - 1);
    break;
  case YAIPS_EDGEDM_ERR_2_ENF:
    strncpy( pCheckText, LangStringLookup( "&Utils_EdgeDM_InspErr8=.2 Edge!"), SizeOfCheckText - 1);
    break;
  case YAIPS_EDGEDM_ERR_3_ENF:
    strncpy( pCheckText, LangStringLookup( "&Utils_EdgeDM_InspErr9=.3 Edge!"), SizeOfCheckText - 1);
    break;
  case YAIPS_EDGEDM_ERR_BOTH_ENF:
    strncpy( pCheckText, LangStringLookup( "&Utils_EdgeDM_InspErr10=No edge found!"), SizeOfCheckText - 1);
    break;
  case YAIPS_EDGEDM_ERR_ILLTRAFO:
    strncpy( pCheckText, LangStringLookup( "&Utils_EdgeDM_InspErr11=AOI position relative to each other!"), SizeOfCheckText - 1);
    break;
  default:
    sprintf( pCheckText, LANGDEF_ERROR_CODE, CheckError);
    break;
  }
}

/************************************************************************************
* YaIPS_EdgeDM_Inspect
*
* Get edges for distance measurement
*
* return     0 OK
*          > 0 One of the YAIPS_EDGEDM_ERR_XXX codes
*          < 0 Error
*/

int YaIPS_EdgeDM_Inspect( Fl_RGB_Image *pSrc,          // Input image
                          YaIPS_EdgeDM_t *pEdgeDM,     // Point to edge DM data
                          int Teach_mode)              // 0 = inspection mode, 1 = teach mode
{
  int ierr1, ierr2;
  double dist;
  YaIPS_RGB_ImgD_t iSrc;
  Tvector *vtmp32_1 = VENULL, *vtmp32_2 = VENULL, *vtmp32_3 = VENULL, *vtmp16_1 = VENULL;

  // Check source first
  ierr1 = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr1 != 0)  {                           // Check for error

    return( ierr1);
  }

  if( (ierr1 = allocVectors( pEdgeDM, &vtmp32_1, &vtmp32_2, &vtmp32_3, &vtmp16_1)) != 0) {
    // sets syserror itself
    goto exitPoint;
  }

  // Get edge from first AOI
  ierr1 = YaIPS_EdgeDM_Inspect_AOI( &iSrc, &pEdgeDM->SW_1,
                                    pEdgeDM->orientation, pEdgeDM->OrientationAngle, 1,Teach_mode,
                                    vtmp32_1, vtmp32_2, vtmp32_3, vtmp16_1);

  // Get edge from second AOI
  ierr2 = YaIPS_EdgeDM_Inspect_AOI( &iSrc, &pEdgeDM->SW_2,
                                    pEdgeDM->orientation, pEdgeDM->OrientationAngle, 2, Teach_mode,
                                    vtmp32_1, vtmp32_2, vtmp32_3, vtmp16_1);

  // Check error code
  if( ierr1 == YAIPS_EDGEDM_ERR_1_ENF && ierr2 == YAIPS_EDGEDM_ERR_2_ENF) {

    ierr1 = YAIPS_EDGEDM_ERR_BOTH_ENF;
  }

  if( ierr1 == 0 && ierr2 != 0) {

    ierr1 = ierr2;
  }

  if( ierr1 != 0) {

    goto exitPoint;
  }

  // Calculate distance of edges
  // NOTE: Error checks are mode above.
  // YaIPS_EdgeDM_DistanceFromPixelPos() can't return an error. NOT error check needed.

  dist = 0.0;

  YaIPS_EdgeDM_DistanceFromPixelPos( &iSrc, pEdgeDM, &dist);

  pEdgeDM->refDist = (float)dist;       // Distance of edges

  if( Teach_mode != 0) {                // Teach mode

    if( pEdgeDM->nomOutOfRef != 0) {    // Nominal value out of measured value in reference

      pEdgeDM->nomVal = (float)dist;
    }
  }

  ierr1 = 0;    // OK

exitPoint:

  ierr2 = removeVectors(&vtmp32_1, &vtmp32_2, &vtmp32_3, &vtmp16_1);
  if( ierr1 == 0 && ierr2 != 0) {
    // sets syserror itself
    ierr1 = ierr2;
  }

  pEdgeDM->CheckError = ierr1;

  // Set text depending from  return code

  pEdgeDM->CheckText[  sizeof( pEdgeDM->CheckText) - 1] = '\0'; // Ensure proper end of string
  switch( ierr1) {

  case 0:   // Have a distance

    if( Teach_mode != 0) {             // Teach mode

      sprintf( pEdgeDM->CheckText, LangStringLookup( "&Utils_EdgeDM_InspDM1=Dist. %.2f %s"), pEdgeDM->refDist, pYaIPS_Calib_Unit2String());

    } else {                           // Inspection mode

      if ((pEdgeDM->refDist < pEdgeDM->nomVal - pEdgeDM->nTol) ||
          (pEdgeDM->refDist > pEdgeDM->nomVal + pEdgeDM->pTol)) {

        ierr1 = YAIPS_EDGEDM_ERR_INSP_TOL;     // Set tolerance violation
      }

      sprintf( pEdgeDM->CheckText, LangStringLookup( "&Utils_EdgeDM_InspDM2=Dist. %.2f %+.2f %s"), pEdgeDM->refDist, pEdgeDM->refDist - pEdgeDM->nomVal, pYaIPS_Calib_Unit2String());
    }
    break;
  default:
    YaIPS_CheckCode2Text( ierr1, pEdgeDM->CheckText, sizeof( pEdgeDM->CheckText));
    break;
  }

  return( ierr1);
}

/************************************************************************************
* YaIPS_EdgePM_Inspect
*
* Get edges for position measurement
*
* return     0 OK
*          > 0 One of the YAIPS_EDGEDM_ERR_XXX codes
*          < 0 Error
*/

int YaIPS_EdgePM_Inspect( Fl_RGB_Image *pSrc,          // Input image
                          YaIPS_EdgePM_t *pEdgePM,     // Point to edge PM data
                          int Teach_mode)              // 0 = inspection mode, 1 = teach mode
{
  int ierr, ierr2, i, nAOIs, orientation, nErrEnf;
  YaIPS_RGB_ImgD_t iSrc;
  Tvector *vtmp32_1 = VENULL, *vtmp32_2 = VENULL, *vtmp32_3 = VENULL, *vtmp16_1 = VENULL;
  YaIPS_EdgeAOI_t *pSubWin;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error

    return( ierr);
  }

  if( (ierr = allocVectors( pEdgePM, &vtmp32_1, &vtmp32_2, &vtmp32_3, &vtmp16_1)) != 0) {
    // sets syserror itself
    goto exitPoint;
  }

  nAOIs = (pEdgePM->MeasureMode / 2) + 1;

  if( nAOIs < 1 || nAOIs > YAIPS_EDGEPM_MAX_AOIS) {  // Security test

    ierr = -SE_ILL_PARAMETER;
    goto exitPoint;
  }

  ierr = 0;
  nErrEnf = 0;

  for( i = 0; i < nAOIs; i++) {

    pSubWin = &pEdgePM->Edges[ i];

    if( i == 0) {           // First AOI

      orientation = pEdgePM->MeasureMode == YAIPS_EDGEPM_MODE_Y || pEdgePM->MeasureMode == YAIPS_EDGEPM_MODE_YX ?
                       WIN_PAR_ORIENT_VERTIKAL : WIN_PAR_ORIENT_HORIZONTAL;

    } else if( i == 1) {   // Second AOI

      orientation = pEdgePM->MeasureMode == YAIPS_EDGEPM_MODE_XY || pEdgePM->MeasureMode == YAIPS_EDGEPM_MODE_XYY ?
                       WIN_PAR_ORIENT_VERTIKAL : WIN_PAR_ORIENT_HORIZONTAL;

    } else {               // Third AOI

      orientation =  WIN_PAR_ORIENT_VERTIKAL;
    }

    // Get edge from AOI
    ierr2 = YaIPS_EdgeDM_Inspect_AOI( &iSrc, pSubWin,
                                      orientation, 0, i + 1, Teach_mode,
                                      vtmp32_1, vtmp32_2, vtmp32_3, vtmp16_1);

    pSubWin->CheckError = ierr2;

    if( ierr2 == YAIPS_EDGEDM_ERR_IMBORDER) {

      ierr2 += i + 1;
    }

    if( ierr2 == YAIPS_EDGEDM_ERR_ENF) {

      nErrEnf += 1;

      ierr2 += i + 1;
    }

    if( ierr == 0 && ierr2 != 0) {

      ierr = ierr2;
    }
  }

  // Check error code
  if( nErrEnf == nAOIs) {                   // All AOIs have the edge not found

    ierr = YAIPS_EDGEDM_ERR_BOTH_ENF;
  }

  if( ierr != 0) {

    goto exitPoint;
  }

  // Calculate position deltas

  pEdgePM->DeltaX = 0.0;
  pEdgePM->DeltaY = 0.0;
  pEdgePM->DeltaA = 0.0;

  pEdgePM->T_Matrix[ A11] = 1.0;
  pEdgePM->T_Matrix[ A12] = 0.0;
  pEdgePM->T_Matrix[ A21] = 0.0;
  pEdgePM->T_Matrix[ A22] = 1.0;
  pEdgePM->T_Matrix[ B1]  = 0.0;
  pEdgePM->T_Matrix[ B2]  = 0.0;

  if( Teach_mode == 0) {                // Inspection mode

    int numCX, numCY;
    int32 *pin, *pref;

#ifdef USE_VGEOEST  // Use vgeoest() in place of vuparcal() to calulate the transforamtion system

    // Data needed for vgeoest() call

    int32 vrefdata[ IPS_VGEOEST_SRC_N];
    int32 vindata[ IPS_VGEOEST_SRC_N];
    float dstdata[ IPS_VGEOEST_DST_N];
    lfloat v1tdata[ IPS_VGEOEST_SRC_N];
    lfloat vtmpdata[ IPS_VGEOEST_DST_N];
    Tvector vref, vin, dstvec, v1t, vtmp;
#else
    // Data needed for vuparcal() call

    int32 refdata[ IPS_VUPARCAL_REF_N];
    int32 testdata[ IPS_VUPARCAL_TEST_N];
    float dstdata[ IPS_VUPARCAL_DST_N];
    Tvector refvec, testvec, dstvec;
#endif

    numCX = 0;
    numCY = 0;

#ifdef USE_VGEOEST  // Use vgeoest() in place of vuparcal() to calulate the transforamtion system

    ve_Mem2Vec( &vref, DV_HOST, (anypnt)vrefdata, IPS_VGEOEST_SRC_N, sizeof(int32), TY_INT32);
    ve_Mem2Vec( &vin, DV_HOST, (anypnt)vindata, IPS_VGEOEST_SRC_N, sizeof(int32), TY_INT32);
    ve_Mem2Vec( &dstvec, DV_HOST, (anypnt)dstdata, IPS_VGEOEST_DST_N, sizeof(float), TY_SFLOAT);
    ve_Mem2Vec( &v1t, DV_HOST, (anypnt)v1tdata, IPS_VGEOEST_SRC_N, sizeof(lfloat), TY_LFLOAT);
    ve_Mem2Vec( &vtmp, DV_HOST, (anypnt)vtmpdata, IPS_VGEOEST_DST_N, sizeof(lfloat), TY_LFLOAT);
#else
    ve_Mem2Vec( &refvec, DV_HOST, (anypnt)refdata, IPS_VUPARCAL_REF_N, sizeof(int32), TY_INT32);
    ve_Mem2Vec( &testvec, DV_HOST, (anypnt)testdata, IPS_VUPARCAL_TEST_N, sizeof(int32), TY_INT32);
    ve_Mem2Vec( &dstvec, DV_HOST, (anypnt)dstdata, IPS_VUPARCAL_DST_N, sizeof(float), TY_SFLOAT);
#endif

    /* initialize vectors for x/y_transform */
#ifndef USE_VGEOEST  // Use vgeoest() in place of vuparcal() to calulate the transforamtion system
    for (i = 0; i <  IPS_VUPARCAL_REF_N; i++) refdata[i] = -1;
    for (i = 0; i < IPS_VUPARCAL_TEST_N; i++) testdata[i] = -1;
#endif

    for( i = 0; i < nAOIs; i++) {

      pSubWin = &pEdgePM->Edges[ i];

      if( pSubWin->refPeak < 0.0) {     // Reference peak not calculated

        // Error, not teached

        ierr = YAIPS_EDGEDM_ERR_NO_TEACH;
        goto exitPoint;
      }

      if( pSubWin->LastPeak < 0.0) {    // Last peak not calculated above

        // inspection error, should not happen !

        ierr = YAIPS_EDGEDM_ERR_ENF;
        goto exitPoint;
      }

      if( i == 0) {           // First AOI

        orientation = pEdgePM->MeasureMode == YAIPS_EDGEPM_MODE_Y || pEdgePM->MeasureMode == YAIPS_EDGEPM_MODE_YX ?
                         WIN_PAR_ORIENT_VERTIKAL : WIN_PAR_ORIENT_HORIZONTAL;

      } else if( i == 1) {   // Second AOI

        orientation = pEdgePM->MeasureMode == YAIPS_EDGEPM_MODE_XY || pEdgePM->MeasureMode == YAIPS_EDGEPM_MODE_XYY ?
                         WIN_PAR_ORIENT_VERTIKAL : WIN_PAR_ORIENT_HORIZONTAL;
      } else {               // Third AOI

        orientation =  WIN_PAR_ORIENT_VERTIKAL;
      }

      if( orientation == WIN_PAR_ORIENT_HORIZONTAL) {    // x-direction

        if( pEdgePM->MeasureMode == YAIPS_EDGEPM_MODE_XXY) {

          pEdgePM->DeltaX = (pEdgePM->DeltaX + pSubWin->LastPeak - pSubWin->refPeak) * 0.5;   // Second horizontal AOI

        } else {

          pEdgePM->DeltaX = pSubWin->LastPeak - pSubWin->refPeak;
        }

        /* precision 1/64 pixel */

#ifdef USE_VGEOEST  // Use vgeoest() in place of vuparcal() to calulate the transforamtion system

        pin  = vindata  + (2 * i);
        pref = vrefdata + (2 * i);

        pref[ 0] = ((int32)pSubWin->AOI.XPos << 6) + ((int32)pSubWin->AOI.XSize << 5);
        pref[ 1] = ((int32)pSubWin->AOI.YPos << 6) + ((int32)pSubWin->AOI.YSize << 5);

        pin[ 0] = pref[ 0] + (pSubWin->LastPeak - pSubWin->refPeak) * (1 << 6);
        pin[ 1] = pref[ 1];
#else
        pin  = testdata + (2 * numCX);
        pref = refdata  + (4 * numCX);

        *pref++ = ((int32)pSubWin->AOI.XPos << 6) + ((int32)pSubWin->AOI.XSize << 5);
        *pref   = ((int32)pSubWin->AOI.YPos << 6) + ((int32)pSubWin->AOI.YSize << 5);

        *pin = pref[ -1] + (pSubWin->LastPeak - pSubWin->refPeak) * (1 << 6);
#endif

        numCX++;

      } else {                                          // y-direction

        if( pEdgePM->MeasureMode == YAIPS_EDGEPM_MODE_XYY) {

          pEdgePM->DeltaY = (pEdgePM->DeltaY + pSubWin->LastPeak - pSubWin->refPeak) * 0.5;   // Second vertical AOI

        } else {

          pEdgePM->DeltaY = pSubWin->LastPeak - pSubWin->refPeak;
        }

        /* precision 1/64 pixel */

#ifdef USE_VGEOEST  // Use vgeoest() in place of vuparcal() to calulate the transforamtion system

        pin  = vindata  + (2 * i);
        pref = vrefdata + (2 * i);

        pref[ 0] = ((int32)pSubWin->AOI.XPos << 6) + ((int32)pSubWin->AOI.XSize << 5);
        pref[ 1] = ((int32)pSubWin->AOI.YPos << 6) + ((int32)pSubWin->AOI.YSize << 5);

        pin[ 0] = pref[ 0];
        pin[ 1] = pref[ 1] + (pSubWin->LastPeak - pSubWin->refPeak) * (1 << 6);
#else
        pin  = testdata + (2 * numCY + 1);
        pref = refdata  + (4 * numCY + 2);

        *pref++ = ((int32)pSubWin->AOI.XPos << 6) + ((int32)pSubWin->AOI.XSize << 5);
        *pref   = ((int32)pSubWin->AOI.YPos << 6) + ((int32)pSubWin->AOI.YSize << 5);

        *pin = pref[ 0] + (pSubWin->LastPeak - pSubWin->refPeak) * (1 << 6);
#endif

        numCY++;
      }
    }

    if( nAOIs >= 3)  {                 // Have three AOIOs

#ifdef USE_VGEOEST  // Use vgeoest() in place of vuparcal() to calulate the transforamtion system

      ierr = vgeoest( &vin, &vref, &dstvec,0, 0, 0, &v1t, &vtmp);

      if( ierr == -14 || ierr == -15 || ierr == -16 || ierr == -17) {

        /* illegal points for calculation of transform system */

        ierr = YAIPS_EDGEDM_ERR_ILLTRAFO;

        goto exitPoint;
      }

      if( ierr) {

        if( errstring == NULL) {                  // No error until now

          sprintf( errbuffer, "error %d in vgeoest()", ierr);

          errstring = errbuffer;
        }

        ierr = -SE_VGEOEST;
        goto exitPoint;
      }
#else

      ierr = vuparcal( &refvec, &testvec, &dstvec,
                       YaIPS_Calib_UPP_Y / YaIPS_Calib_UPP_X);  // camera x/y relatio


      if( ierr == -12 || ierr == -13 || ierr == -14 || ierr == -16) {

        /* illegal points for calculation of transform system */

        ierr = YAIPS_EDGEDM_ERR_ILLTRAFO;

        goto exitPoint;
      }

      if( ierr) {

        if( errstring == NULL) {                  // No error until now

          sprintf( errbuffer, "error %d in vuparcal()", ierr);

          errstring = errbuffer;
        }

        ierr = -SE_VUPARCAL;
        goto exitPoint;
      }
#endif

      /* enter transform system in results */

      /* transform system was calculated with points with 6 bit precision*/

      pEdgePM->DeltaX = dstdata[4] / 64.0;
      pEdgePM->DeltaY = dstdata[5] / 64.0;

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
      if( (double)dstdata[1] >= -0.0001 && (double)dstdata[1] <= 0.0001 &&
          (double)dstdata[3] >= -0.0001 && (double)dstdata[3] <= 0.0001) {

        /* illegal points for calculation of transform system */
        ierr = YAIPS_EDGEDM_ERR_ILLTRAFO;

        goto exitPoint;
      }

      pEdgePM->DeltaA = atan2((double)dstdata[1], (double)dstdata[3]) * 180.0 / M_PI;
#endif
      pEdgePM->T_Matrix[ A11] = dstdata[ A11];
      pEdgePM->T_Matrix[ A12] = dstdata[ A12];
      pEdgePM->T_Matrix[ A21] = dstdata[ A21];
      pEdgePM->T_Matrix[ A22] = dstdata[ A22];
      pEdgePM->T_Matrix[ B1]  = pEdgePM->DeltaX;
      pEdgePM->T_Matrix[ B2]  = pEdgePM->DeltaY;

      // Rotation center for the matrix is the left
      // upper corner (coordinate 0/0) of the image.
      // This give unusable /DeltaY values.

      // Estimate an object center and transform this gives
      // much better results.

      double P1X, P1Y, P2X, P2Y;
      YaIPS_EdgeAOI_t *pEdgeAOI;

      pEdgeAOI = pEdgePM->Edges;

      if( pEdgePM->MeasureMode == YAIPS_EDGEPM_MODE_XYY) {

        // X is average of center of the 2 y AOIs
        P1X = ((double)pEdgeAOI[ 1].AOI.XPos + (double)pEdgeAOI[ 1].AOI.XSize * 0.5 + (double)pEdgeAOI[ 2].AOI.XPos + (double)pEdgeAOI[ 2].AOI.XSize * 0.5) * 0.5;

        // Y is  center of the x AOI
        P1Y = (double)pEdgeAOI[ 0].AOI.YPos + (double)pEdgeAOI[ 0].AOI.YSize * 0.5;

      } else {   // is YAIPS_EDGEPM_MODE_XXY

        // X is center of the y AOI
        P1X = (double)pEdgeAOI[ 2].AOI.XPos + (double)pEdgeAOI[ 2].AOI.XSize * 0.5;

        // Y is average of center of the 2 x AOIs
        P1Y = ((double)pEdgeAOI[ 0].AOI.YPos + (double)pEdgeAOI[ 0].AOI.YSize * 0.5 + (double)pEdgeAOI[ 1].AOI.YPos + (double)pEdgeAOI[ 1].AOI.YSize * 0.5) * 0.5;
      }

      P2X = MATRIX_TRANSFORM_X( pEdgePM->T_Matrix, P1X, P1Y);
      P2Y = MATRIX_TRANSFORM_Y( pEdgePM->T_Matrix, P1X, P1Y);

      // Delta after transformation is the offset

      pEdgePM->DeltaX = P2X - P1X;
      pEdgePM->DeltaY = P2Y - P1Y;

    } else {  // Have one or too points (only translation)

      pEdgePM->T_Matrix[ A11] = 1.0;
      pEdgePM->T_Matrix[ A12] = 0.0;
      pEdgePM->T_Matrix[ A21] = 0.0;
      pEdgePM->T_Matrix[ A22] = 1.0;
      pEdgePM->T_Matrix[ B1]  = pEdgePM->DeltaX;
      pEdgePM->T_Matrix[ B2]  = pEdgePM->DeltaY;

    }
  }

  ierr = 0;    // OK

exitPoint:

  ierr2 = removeVectors(&vtmp32_1, &vtmp32_2, &vtmp32_3, &vtmp16_1);
  if( ierr == 0 && ierr2 != 0) {
    // sets syserror itself
    ierr = ierr2;
  }

  // Set text depending from  return code

  pEdgePM->CheckText[  sizeof( pEdgePM->CheckText) - 1] = '\0'; // Ensure proper end of string
  switch( ierr) {

  case 0:   // Have a position

    if( Teach_mode != 0) {             // Teach mode

      sprintf( pEdgePM->CheckText, LangStringLookup( "&Utils_EdgePM_InspPM1=OK"));

    } else {                           // Inspection mode

      // Check tolerance

      float DeltaX, DeltaY;
      int ErrX, ErrY;

      DeltaX = pEdgePM->DeltaX * YaIPS_Calib_UPP_X;    // Deviation in units
      if( DeltaX < 0) DeltaX = - DeltaX;               // Make absolute value
      ErrX = DeltaX > pEdgePM->MaxDevX;                // Check over threshold

      DeltaY = pEdgePM->DeltaY * YaIPS_Calib_UPP_X;    // Deviation in units
      if( DeltaY < 0) DeltaY = - DeltaY;               // Make absolute value
      ErrY = DeltaY > pEdgePM->MaxDevY;                // Check over threshold

      if( ErrX || ErrY) {                              // Position deviation to high

        ierr = YAIPS_EDGEDM_ERR_INSP_TOL;     // Set tolerance violation
      }

      if( nAOIs <= 2) {   // Only position deltas

        sprintf( pEdgePM->CheckText, LangStringLookup( "%+.2f%s/%+.2f%s %s"),
                                                       pEdgePM->DeltaX * YaIPS_Calib_UPP_X, ErrX ? "!" : "",
                                                       pEdgePM->DeltaY * YaIPS_Calib_UPP_Y, ErrY ? "!" : "",
                                                       pYaIPS_Calib_Unit2String());
      } else {            // Position deltas + angle

        sprintf( pEdgePM->CheckText, LangStringLookup( "%+.2f%s/%+.2f%s %s, %+.2f °"),
                                                       pEdgePM->DeltaX * YaIPS_Calib_UPP_X, ErrX ? "!" : "",
                                                       pEdgePM->DeltaY * YaIPS_Calib_UPP_Y, ErrY ? "!" : "",
                                                       pYaIPS_Calib_Unit2String(), pEdgePM->DeltaA);
      }
    }
    break;
  default:
    YaIPS_CheckCode2Text( ierr, pEdgePM->CheckText, sizeof( pEdgePM->CheckText));
    break;
  }

  pEdgePM->CheckError = ierr;

  return( ierr);
}

/************************************************************************************
* YaIPS_EdgeAOI_Draw
*
* Draw an edge AOI
*
* NOTE: Make a fl_push_clip() call before calling this function
*
*/

void YaIPS_EdgeAOI_Draw( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                         YaIPS_EdgeAOI_t *pEdgeAOI,    // Pointer to EdgeAOI data
                         int Teach_mode,               // 0 = inspection mode, 1 = teach mode
                         Fl_Color DrawColor,           // Color for drawing
                         char *pText,                  // Text for caption
                         int   orientation,            // 0: x-distance, 1: y-distance, 2: orientation is given parameter
                         int   OrientationAngle)       // orientation angle for mode (180 .. - 180 Grad)
{
  int LineWidth;
  int x1, y1, OffX, OffY, DrawText;
  int x1Temp, y1Temp, x2Temp, y2Temp, x3Temp, y3Temp, x4Temp, y4Temp;
  double x1TempD, y1TempD, x2TempD, y2TempD;
  int AOIx1, AOIy1, AOIx2, AOIy2, AOIxx, AOIyy, CentX, CentY;
  int mdx, mdy, mw, mh;
  float relPeak;
  static char MyLineDashes[] = { 4, 4, 0};
  static Fl_Color ColPeak1 = fl_rgb_color( 0,  64, 255);
  static Fl_Color ColPeak2 = fl_rgb_color( 0, 255,   0);

  // Preparations

  MyLineDashes[ 0] = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 4.0 + 0.5);
  if( MyLineDashes[ 0] < 4) {
    MyLineDashes[ 0] = 4;
  }
  MyLineDashes[ 1] =  MyLineDashes[ 0];

  x1 = pYaIPS_ImageDisp->BigImage_sx;
  y1 = pYaIPS_ImageDisp->BigImage_sy;

  LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

  // Peak position used for drawing peak marker

  if( Teach_mode != 0) {                // Teach mode

    relPeak = pEdgeAOI->refPeak;

  } else {                              // Inspection mode

    relPeak = pEdgeAOI->LastPeak;
  }

  // Points relative to image

  OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
  OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

  // Prepare font size

  DrawText = false;                                            // Preset, do not draw text

  if( pYaIPS_ImageDisp->PixelImageToScreen >= 0.33 &&          // Screen resolution is NOT to tiny
      pText != NULL && *pText != '\0') {                       // and have a text

    int TempFontSize;

    DrawText = true;                                           // Draw crcdf text

    TempFontSize = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 16.0 + 0.5);  // * 12.0

    if( TempFontSize < 10) {
      TempFontSize = 10;
    }

    fl_font( FL_HELVETICA, TempFontSize);
  }

  // Draw AOIs

  fl_line_style( 0, LineWidth);   // Set line width
  fl_color( DrawColor);           // Color

  switch( orientation) {
  case 0:       // horizontal, x-distance
  case 1:       // vertical, y-distance

    fl_line_style( 0, LineWidth);   // Set line width

    AOIx1 = (int)( (pEdgeAOI->AOI.XPos - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
    AOIy1 = (int)( (pEdgeAOI->AOI.YPos - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

    AOIxx = (int)( pEdgeAOI->AOI.XSize * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    AOIyy = (int)( pEdgeAOI->AOI.YSize * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

    AOIx2 = AOIx1 + AOIxx - 1;
    AOIy2 = AOIy1 + AOIyy - 1;

    // Draw rectangle
    fl_rect( AOIx1, AOIy1, AOIxx, AOIyy);

    // Draw center lines

    CentX = (AOIx1 + AOIx2) / 2;
    CentY = (AOIy1 + AOIy2) / 2;

    // Draw nominal value marker

    if( orientation == 0)  {                 // horizontal, x-distance

      int xNom, Delta3, Delta6;

#ifdef use_again  // 09.07.2025 RR: Replaced nominal position by 1/2 search length
      xNom = pEdgeAOI->nomPos;
#else
      xNom = pEdgeAOI->AOI.XSize / 2;          // Nominal position by 1/2 search length
#endif
      if( xNom < 0) {
        xNom = 0;
      }
      if( xNom >= pEdgeAOI->AOI.XSize) {
        xNom = pEdgeAOI->AOI.XSize - 1;
      }

      x1TempD = (pEdgeAOI->AOI.XPos + xNom - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;

      Delta3 = dto32( 3 * NOM_MARKER_FAK * pYaIPS_ImageDisp->PixelImageToScreen);
      if( Delta3 < 2) Delta3 = 2;
      Delta6 = dto32( 6 * NOM_MARKER_FAK * pYaIPS_ImageDisp->PixelImageToScreen);
      if( Delta6 < 2) Delta6 = 4;

#ifdef use_again
      fl_line( x1TempD - Delta3, AOIy1, x1TempD, AOIy1 + Delta6);
      fl_line( x1TempD + Delta3, AOIy1, x1TempD, AOIy1 + Delta6);
      fl_line( x1TempD, AOIy1 - Delta6, x1TempD, AOIy1 + Delta6);
#else
      fl_line( x1TempD, AOIy1 - Delta6, x1TempD, AOIy1 + Delta6);
      fl_line( x1TempD, AOIy2 - Delta6, x1TempD, AOIy2 + Delta6);
#endif

#ifdef _DEBUG

      // For debug also visualize search range edges

      if( Teach_mode != 0) {                // Teach mode

        fl_line_style( 0);              // Reset to default

        Delta6 = Delta6 * 2 / 3;

        xNom = pEdgeAOI->AOI.XSize / 2 - EDGE_DM_SEARCH_RANGE;          // Nominal position by 1/2 search length

        if( xNom < 0) {
          xNom = 0;
        }
        if( xNom >= pEdgeAOI->AOI.XSize) {
          xNom = pEdgeAOI->AOI.XSize - 1;
        }

        x1TempD = (pEdgeAOI->AOI.XPos + xNom - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;

        fl_line( x1TempD, AOIy1 - Delta6, x1TempD, AOIy1 + Delta6);
        fl_line( x1TempD, AOIy2 - Delta6, x1TempD, AOIy2 + Delta6);

        xNom = pEdgeAOI->AOI.XSize / 2 + EDGE_DM_SEARCH_RANGE;          // Nominal position by 1/2 search length

        if( xNom < 0) {
          xNom = 0;
        }
        if( xNom >= pEdgeAOI->AOI.XSize) {
          xNom = pEdgeAOI->AOI.XSize - 1;
        }

        x1TempD = (pEdgeAOI->AOI.XPos + xNom - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;

        fl_line( x1TempD, AOIy1 - Delta6, x1TempD, AOIy1 + Delta6);
        fl_line( x1TempD, AOIy2 - Delta6, x1TempD, AOIy2 + Delta6);
      }
#endif

    } else {                                 // vertical, y-distance

      int yNom, Delta3, Delta6;

#ifdef use_again  // 09.07.2025 RR: Replaced nominal position by 1/2 search length
      yNom = pEdgeAOI->nomPos;
#else
      yNom = pEdgeAOI->AOI.YSize / 2;          // Nominal position by 1/2 search length
#endif
      if( yNom < 0) {
        yNom = 0;
      }
      if( yNom >= pEdgeAOI->AOI.YSize) {
        yNom = pEdgeAOI->AOI.YSize - 1;
      }

      y1TempD = (pEdgeAOI->AOI.YPos + yNom - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;

      Delta3 = dto32( 3 * NOM_MARKER_FAK * pYaIPS_ImageDisp->PixelImageToScreen);
      if( Delta3 < 2) Delta3 = 2;
      Delta6 = dto32( 6 * NOM_MARKER_FAK * pYaIPS_ImageDisp->PixelImageToScreen);
      if( Delta6 < 2) Delta6 = 4;

#ifdef use_again
      fl_line( AOIx1, y1TempD - Delta3, AOIx1 + Delta6, y1TempD);
      fl_line( AOIx1, y1TempD + Delta3, AOIx1 + Delta6, y1TempD);
      fl_line( AOIx1 - Delta6, y1TempD, AOIx1 + Delta6, y1TempD);
#else
      fl_line( AOIx1 - Delta6, y1TempD, AOIx1 + Delta6, y1TempD);
      fl_line( AOIx2 - Delta6, y1TempD, AOIx2 + Delta6, y1TempD);
#endif

#ifdef _DEBUG

      // For debug also visualize search range edges

      if( Teach_mode != 0) {                // Teach mode

        fl_line_style( 0);              // Reset to default

        Delta6 = Delta6 * 2 / 3;

        yNom = pEdgeAOI->AOI.YSize / 2 - EDGE_DM_SEARCH_RANGE;          // Nominal position by 1/2 search length

        if( yNom < 0) {
          yNom = 0;
        }
        if( yNom >= pEdgeAOI->AOI.YSize) {
          yNom = pEdgeAOI->AOI.YSize - 1;
        }

        y1TempD = (pEdgeAOI->AOI.YPos + yNom - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;
        fl_line( AOIx1 - Delta6, y1TempD, AOIx1 + Delta6, y1TempD);
        fl_line( AOIx2 - Delta6, y1TempD, AOIx2 + Delta6, y1TempD);

        yNom = pEdgeAOI->AOI.YSize / 2 + EDGE_DM_SEARCH_RANGE;          // Nominal position by 1/2 search length

        if( yNom < 0) {
          yNom = 0;
        }
        if( yNom >= pEdgeAOI->AOI.YSize) {
          yNom = pEdgeAOI->AOI.YSize - 1;
        }

        y1TempD = (pEdgeAOI->AOI.YPos + yNom - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;
        fl_line( AOIx1 - Delta6, y1TempD, AOIx1 + Delta6, y1TempD);
        fl_line( AOIx2 - Delta6, y1TempD, AOIx2 + Delta6, y1TempD);
      }
#endif
    }

    // Mark search direction

    fl_line_style( 0);              // Reset to default

    x1Temp = dto32( 16 * pYaIPS_ImageDisp->PixelImageToScreen);   // Length mark

    if( orientation == 0)  {                 // horizontal, x-distance

      // center line
      fl_line( AOIx1, CentY, AOIx2, CentY);

      if( pEdgeAOI->r2l_b2t == 0) {       // left->right

        fl_line( AOIx2 - x1Temp, CentY - x1Temp,
                 AOIx2, CentY,
                 AOIx2 - x1Temp, CentY + x1Temp);

      } else {                            // right->left

        fl_line( AOIx1 + x1Temp, CentY - x1Temp,
                 AOIx1, CentY,
                 AOIx1 + x1Temp, CentY + x1Temp);
      }

    } else {                                 // vertical, y-distance

      // center line
      fl_line( CentX, AOIy1, CentX, AOIy2);

      if( pEdgeAOI->r2l_b2t == 0) {       // bottom->top

        fl_line( CentX - x1Temp, AOIy2 - x1Temp,
                 CentX, AOIy2,
                 CentX + x1Temp, AOIy2 - x1Temp);

      } else {                            // right->left

        fl_line( CentX - x1Temp, AOIy1 + x1Temp,
                 CentX, AOIy1,
                 CentX + x1Temp, AOIy1 + x1Temp);
      }
    }

    // Draw text

    if( DrawText && pText != NULL && *pText != '\0') {   // Draw text

      x1Temp = AOIx1;
      y1Temp = AOIy1;

      fl_text_extents( pText, mdx, mdy, mw, mh);

      if( y1Temp - y1 + mdy < 4) {    // To near to upper border
        y1Temp += mh + 5;             // Show below upper frame
        x1Temp += 4;
      } else {                       // Fits above upper frame
        y1Temp -= 4;
      }
      fl_draw( pText, x1Temp, y1Temp);
    }

    // Draw lines at peak positions

    if( orientation == 0)  {                 // horizontal, x-distance

      if( relPeak >= (float)0.0) {          // peak valid

        x1Temp = dto32( relPeak * pYaIPS_ImageDisp->PixelImageToScreen );

        fl_color( ColPeak1);           // Color
        fl_line( AOIx1 + x1Temp, AOIy1, AOIx1 + x1Temp, AOIy2);

        fl_color( ColPeak2);           // Color
        fl_line_style( FL_DOT, 0, MyLineDashes);
        fl_line( AOIx1 + x1Temp, AOIy1, AOIx1 + x1Temp, AOIy2);
      }

    } else {                                 // vertical, y-distance

      if( relPeak >= (float)0.0) {    // peak valid


        y1Temp = dto32( relPeak * pYaIPS_ImageDisp->PixelImageToScreen );

        fl_color( ColPeak1);           // Color
        fl_line( AOIx1, AOIy1 + y1Temp, AOIx2, AOIy1 + y1Temp);

        fl_color( ColPeak2);           // Color
        fl_line_style( FL_DOT, 0, MyLineDashes);
        fl_line( AOIx1, AOIy1 + y1Temp, AOIx2, AOIy1 + y1Temp);
      }
    }

    break;

  default:      // Free orientation
    {
      double xAP, yAP, xBP, yBP;
      double xCL0, yCL0, xCL1, yCL1, xA, xB, xC, xD, yA, yB, yC, yD; /* border coordinates */
      double dxScanDir, dyScanDir, dxSideDir, dySideDir;
      int    nSample, nQuer;
      T_FreeAngleInfo FreeAngleInfo;

      getWinCoordinates( &FreeAngleInfo, &pEdgeAOI->AOI, OrientationAngle, YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y, 2,
                         pYaIPS_ImageDisp->BigImage_iw, pYaIPS_ImageDisp->BigImage_ih, 0.0, &nSample, &nQuer,
                         &xCL0, &yCL0, &xCL1, &yCL1, &xA, &xB, &xC, &xD, &yA, &yB, &yC, &yD,
                         &xAP, &yAP, &xBP, &yBP,
                         &dxScanDir, &dyScanDir, &dxSideDir, &dySideDir);

      dxScanDir *= pYaIPS_ImageDisp->PixelImageToScreen;
      dyScanDir *= pYaIPS_ImageDisp->PixelImageToScreen;
      dxSideDir *= pYaIPS_ImageDisp->PixelImageToScreen;
      dySideDir *= pYaIPS_ImageDisp->PixelImageToScreen;

      x1Temp = (int)( (xA - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
      y1Temp = (int)( (yA - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

      x2Temp = (int)( (xB - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
      y2Temp = (int)( (yB - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

      x3Temp = (int)( (xC - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
      y3Temp = (int)( (yC - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

      x4Temp = (int)( (xD - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
      y4Temp = (int)( (yD - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

      fl_line( x1Temp, y1Temp, x2Temp, y2Temp);
      fl_line( x2Temp, y2Temp, x4Temp, y4Temp);
      fl_line( x4Temp, y4Temp, x3Temp, y3Temp);
      fl_line( x3Temp, y3Temp, x1Temp, y1Temp);

#ifdef use_again
      gpDrawWin_rectText( header, FreeAngleInfo.HeaderTextXPos, FreeAngleInfo.HeaderTextYPos,
                          0, 0,
                          pYaIPS_ImageDisp->BigImage_iw, pYaIPS_ImageDisp->BigImage_ih, usedDrawColor);
#else

      // Draw text

      if( DrawText && pText != NULL && *pText != '\0') {   // Draw text

        x1Temp = (int)( (FreeAngleInfo.TextXPosHigh - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
        y1Temp = (int)( (FreeAngleInfo.TextYPosHigh - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

        fl_text_extents( pText, mdx, mdy, mw, mh);

        if( y1Temp - y1 + mdy < 4) {    // To near to upper border

          // Show below lower point
          x1Temp = (int)( (FreeAngleInfo.TextXPosLow - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
          y1Temp = (int)( (FreeAngleInfo.TextYPosLow - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

          y1Temp += mh + 5;             // Show below upper frame
        } else {                       // Fits above upper frame
          y1Temp -= 4;
        }
        fl_draw( pText, x1Temp, y1Temp);
      }
#endif

      // Draw nominal value marker

      if( 1) {
        double lNom, Delta3, Delta6;

#ifdef use_again  // 09.07.2025 RR: Replaced nominal position by 1/2 search length
        lNom = pEdgeAOI->nomPos;
#else
        lNom = nSample / 2;          // Nominal position by 1/2 search length
#endif
        if( lNom < 0) {
          lNom = 0;
        }
        if( lNom >= nSample) {
          lNom = nSample - 1;
        }

        getWinCoordinates( &FreeAngleInfo, &pEdgeAOI->AOI, OrientationAngle, YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y, 1,
                           pYaIPS_ImageDisp->BigImage_iw, pYaIPS_ImageDisp->BigImage_ih, (double)lNom, &nSample, &nQuer,
                           &xCL0, &yCL0, &xCL1, &yCL1, &xA, &xB, &xC, &xD, &yA, &yB, &yC, &yD,
                           &xAP, &yAP, &xBP, &yBP,
                           &dxScanDir, &dyScanDir, &dxSideDir, &dySideDir);

        x1TempD = (xAP - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;
        y1TempD = (yAP - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;

        x2TempD = (xBP - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;
        y2TempD = (yBP - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;

        Delta3 = dto32( 3 * NOM_MARKER_FAK * pYaIPS_ImageDisp->PixelImageToScreen);
        if( Delta3 < 2) Delta3 = 2;
        Delta6 = dto32( 6 * NOM_MARKER_FAK * pYaIPS_ImageDisp->PixelImageToScreen);
        if( Delta6 < 2) Delta6 = 4;

#ifdef use_again
        if( 1) {

          // draw above rectangle
          fl_line( dto32(x2TempD + dxScanDir * - Delta3),
                   dto32(y2TempD + dyScanDir * - Delta3),
                   dto32(x2TempD + dxScanDir *  0 + dxSideDir * - Delta6),
                   dto32(y2TempD + dyScanDir *  0 + dySideDir * - Delta6));

          fl_line( dto32(x2TempD + dxScanDir *  Delta3),
                   dto32(y2TempD + dyScanDir *  Delta3),
                   dto32(x2TempD + dxScanDir *  0 + dxSideDir * - Delta6),
                   dto32(y2TempD + dyScanDir *  0 + dySideDir * - Delta6));

          fl_line( dto32(x2TempD + dxScanDir *  0 + dxSideDir * - Delta6),
                   dto32(y2TempD + dyScanDir *  0 + dySideDir * - Delta6),
                   dto32(x2TempD + dxScanDir *  0 + dxSideDir *  Delta6),
                   dto32(y2TempD + dyScanDir *  0 + dySideDir *  Delta6));
        } else {

          // draw below rectangle

          fl_line( dto32(x1TempD + dxScanDir * - Delta3),
                   dto32(y1TempD + dyScanDir * - Delta3),
                   dto32(x1TempD + dxScanDir *  0 + dxSideDir *  Delta6),
                   dto32(y1TempD + dyScanDir *  0 + dySideDir *  Delta6));

          fl_line( dto32(x1TempD + dxScanDir *  Delta3),
                   dto32(y1TempD + dyScanDir *  Delta3),
                   dto32(x1TempD + dxScanDir *  0 + dxSideDir *  Delta6),
                   dto32(y1TempD + dyScanDir *  0 + dySideDir *  Delta6));

          fl_line( dto32(x1TempD + dxScanDir *  0 + dxSideDir *  Delta6),
                   dto32(y1TempD + dyScanDir *  0 + dySideDir *  Delta6),
                   dto32(x1TempD + dxScanDir *  0 + dxSideDir * - Delta6),
                   dto32(y1TempD + dyScanDir *  0 + dySideDir * - Delta6));

        }
#else
        fl_line( dto32(x2TempD + dxScanDir *  0 + dxSideDir * - Delta6),
                 dto32(y2TempD + dyScanDir *  0 + dySideDir * - Delta6),
                 dto32(x2TempD + dxScanDir *  0 + dxSideDir *  Delta6),
                 dto32(y2TempD + dyScanDir *  0 + dySideDir *  Delta6));

        fl_line( dto32(x1TempD + dxScanDir *  0 + dxSideDir *  Delta6),
                 dto32(y1TempD + dyScanDir *  0 + dySideDir *  Delta6),
                 dto32(x1TempD + dxScanDir *  0 + dxSideDir * - Delta6),
                 dto32(y1TempD + dyScanDir *  0 + dySideDir * - Delta6));
#endif

#ifdef _DEBUG

      // For debug also visualize search range edges
      fl_line_style( 0);              // Reset to default

      if( Teach_mode != 0) {                // Teach mode

        fl_line_style( 0);              // Reset to default

        Delta6 = Delta6 * 2 / 3;

        lNom = nSample / 2 - EDGE_DM_SEARCH_RANGE;          // Nominal position by 1/2 search length

        if( lNom < 0) {
          lNom = 0;
        }
        if( lNom >= nSample) {
          lNom = nSample - 1;
        }

        getWinCoordinates( &FreeAngleInfo, &pEdgeAOI->AOI, OrientationAngle, YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y, 1,
                           pYaIPS_ImageDisp->BigImage_iw, pYaIPS_ImageDisp->BigImage_ih, (double)lNom, &nSample, &nQuer,
                           &xCL0, &yCL0, &xCL1, &yCL1, &xA, &xB, &xC, &xD, &yA, &yB, &yC, &yD,
                           &xAP, &yAP, &xBP, &yBP,
                           &dxScanDir, &dyScanDir, &dxSideDir, &dySideDir);

        x1TempD = (xAP - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;
        y1TempD = (yAP - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;

        x2TempD = (xBP - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;
        y2TempD = (yBP - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;

        fl_line( dto32(x2TempD + dxScanDir *  0 + dxSideDir * - Delta6),
                 dto32(y2TempD + dyScanDir *  0 + dySideDir * - Delta6),
                 dto32(x2TempD + dxScanDir *  0 + dxSideDir *  Delta6),
                 dto32(y2TempD + dyScanDir *  0 + dySideDir *  Delta6));

        fl_line( dto32(x1TempD + dxScanDir *  0 + dxSideDir *  Delta6),
                 dto32(y1TempD + dyScanDir *  0 + dySideDir *  Delta6),
                 dto32(x1TempD + dxScanDir *  0 + dxSideDir * - Delta6),
                 dto32(y1TempD + dyScanDir *  0 + dySideDir * - Delta6));

        lNom = nSample / 2 + EDGE_DM_SEARCH_RANGE;          // Nominal position by 1/2 search length

        if( lNom < 0) {
          lNom = 0;
        }
        if( lNom >= nSample) {
          lNom = nSample - 1;
        }

        getWinCoordinates( &FreeAngleInfo, &pEdgeAOI->AOI, OrientationAngle, YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y, 1,
                           pYaIPS_ImageDisp->BigImage_iw, pYaIPS_ImageDisp->BigImage_ih, (double)lNom, &nSample, &nQuer,
                           &xCL0, &yCL0, &xCL1, &yCL1, &xA, &xB, &xC, &xD, &yA, &yB, &yC, &yD,
                           &xAP, &yAP, &xBP, &yBP,
                           &dxScanDir, &dyScanDir, &dxSideDir, &dySideDir);

        x1TempD = (xAP - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;
        y1TempD = (yAP - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;

        x2TempD = (xBP - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;
        y2TempD = (yBP - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;

        fl_line( dto32(x2TempD + dxScanDir *  0 + dxSideDir * - Delta6),
                 dto32(y2TempD + dyScanDir *  0 + dySideDir * - Delta6),
                 dto32(x2TempD + dxScanDir *  0 + dxSideDir *  Delta6),
                 dto32(y2TempD + dyScanDir *  0 + dySideDir *  Delta6));

        fl_line( dto32(x1TempD + dxScanDir *  0 + dxSideDir *  Delta6),
                 dto32(y1TempD + dyScanDir *  0 + dySideDir *  Delta6),
                 dto32(x1TempD + dxScanDir *  0 + dxSideDir * - Delta6),
                 dto32(y1TempD + dyScanDir *  0 + dySideDir * - Delta6));
      }
#endif
      }

      // Mark search direction

      fl_line_style( 0);              // Reset to default

      if( 1) {

        x1TempD = (xCL0 - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;
        y1TempD = (yCL0 - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;

        x2TempD = (xCL1 - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;
        y2TempD = (yCL1 - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;

        // center line
        fl_line( dto32(x1TempD), dto32(y1TempD), dto32(x2TempD), dto32(y2TempD));

        // arrow
        if( pEdgeAOI->r2l_b2t == LTOR) {

          fl_line( dto32(x2TempD), dto32(y2TempD),
                   dto32(x2TempD - dxScanDir * 8 + dxSideDir * 16),
                   dto32(y2TempD - dyScanDir * 8 + dySideDir * 16));

          fl_line( dto32(x2TempD), dto32(y2TempD),
                   dto32(x2TempD - dxScanDir * 8 - dxSideDir * 16),
                   dto32(y2TempD - dyScanDir * 8 - dySideDir * 16));
        } else {

          fl_line( dto32(x1TempD), dto32(y1TempD),
                   dto32(x1TempD + dxScanDir * 8 + dxSideDir * 16),
                   dto32(y1TempD + dyScanDir * 8 + dySideDir * 16));

          fl_line( dto32(x1TempD), dto32(y1TempD),
                   dto32(x1TempD + dxScanDir * 8 - dxSideDir * 16),
                   dto32(y1TempD + dyScanDir * 8 - dySideDir * 16));
        }
      }

      // draw lines at peak positions
      if( relPeak >= (float)0.0) {    // peak valid

        getWinCoordinates( &FreeAngleInfo, &pEdgeAOI->AOI, OrientationAngle, YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y, 1,
                           pYaIPS_ImageDisp->BigImage_iw, pYaIPS_ImageDisp->BigImage_ih, relPeak, &nSample, &nQuer,
                           &xCL0, &yCL0, &xCL1, &yCL1, &xA, &xB, &xC, &xD, &yA, &yB, &yC, &yD,
                           &xAP, &yAP, &xBP, &yBP,
                           &dxScanDir, &dyScanDir, &dxSideDir, &dySideDir);


        x1TempD = (xAP - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;
        y1TempD = (yAP - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;

        x2TempD = (xBP - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + x1;
        y2TempD = (yBP - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + y1;

        fl_color( ColPeak1);           // Color
        fl_line( dto32( x1TempD), dto32( y1TempD), dto32( x2TempD), dto32( y2TempD));

        fl_color( ColPeak2);           // Color
        fl_line_style( FL_DOT, 0, MyLineDashes);
        fl_line( dto32( x1TempD), dto32( y1TempD), dto32( x2TempD), dto32( y2TempD));
      }
    }

    break;

  }  // end switch()

  // Finish up

  fl_line_style( 0);   // Reset to default
}

/************************************************************************************
 * YaIPS_EdgeAOI_MouseCC
 *
 * EdgeAOI rectangle clip and check for mouse selection.
 *
 * The AOI rectangle is relative to image displayed on the screen 'BigImage_iw/-ih'.
 *
 *   pAOI            Point to AOI to test
 *   distanceToBeat  For first call must be set to -1.
 *                   An exit with new best distance, this distance is stored to
 *                   this variable. A successive call with a other aoi must
 *                   beat this one to get selected.
 *   pCursor         Return selection cursor depending on distance to points
 *   pDeltaAdd       Return bit mask where to add the delta
 *
 * return:  < 0  Error
 *            0  Mouse not inside window or not nearer than best distance
 *            1  New best distance
 */

int YaIPS_EdgeAOI_MouseCC( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                           YaIPS_EdgeAOI_t *pEdgeAOI,  // Pointer to EdgeAOI data
                           int orientation,            // 0: x-distance, 1: y-distance, 2: orientation is given parameter
                           int OrientationAngle,       // orientation angle for mode (180 .. - 180 Grad)
                           int *distanceToBeat,        // In Out: Distance to beat
                           int *pCursor,               // Out: Cursor shape
                           int *pDeltaAdd)             // Out: Where to add mouse delta
{
  int ierr, xMouse, yMouse, i, dist, Cursor, DeltaAdd;
  int xs, ys, xx, yy, bx, deltaMax, IndexBest;
  double xAP, yAP, xBP, yBP;
  double x0, y0, x1, y1, xA, xB, xC, xD, yA, yB, yC, yD; /* border coordinates */
  double dxScanDir, dyScanDir, dxSideDir, dySideDir;
  int    nSample, nQuer;
  T_FreeAngleInfo FreeAngleInfo;

  switch( orientation) {

  case WIN_PAR_ORIENT_HORIZONTAL:
  case WIN_PAR_ORIENT_VERTIKAL:

    ierr = YaIPS_ImageDispAoiRectCC( pYaIPS_ImageDisp, &pEdgeAOI->AOI,
                                     distanceToBeat, pCursor, pDeltaAdd);

    return( ierr);
    break;

  default:

    // WIN_PAR_ORIENT_PARAMETER

    if( pYaIPS_ImageDisp->pImage_Box == NULL ||          // Security test, have no big image box
        pYaIPS_ImageDisp->BigImage_Calc_OK == false) {   // Size calculations failed

      return( -1);    // Return error
    }

    xMouse = (int)( pYaIPS_ImageDisp->MouseX / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);  // Mouse relative to displayed screen part
    yMouse = (int)( pYaIPS_ImageDisp->MouseY / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

    xMouse += (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);   // Add Offset to displayed screen part
    yMouse += (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

    getWinCoordinates( &FreeAngleInfo, &pEdgeAOI->AOI, OrientationAngle, YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y, 0,
                       pYaIPS_ImageDisp->BigImage_iw, pYaIPS_ImageDisp->BigImage_ih, 0.0, &nSample, &nQuer,
                       &x0, &y0, &x1, &y1, &xA, &xB, &xC, &xD, &yA, &yB, &yC, &yD,
                       &xAP, &yAP, &xBP, &yBP,
                       &dxScanDir, &dyScanDir, &dxSideDir, &dySideDir);


    xs = FreeAngleInfo.SurroundingRectangle.XPos;
    ys = FreeAngleInfo.SurroundingRectangle.YPos;
    xx = FreeAngleInfo.SurroundingRectangle.XSize;
    yy = FreeAngleInfo.SurroundingRectangle.YSize;

    if( xs <= xMouse + 1 && ys <= yMouse + 1 && xs + xx > xMouse - 1 && ys + yy > yMouse - 1) {

      double distF, distMin;

      // we are in the surrounding rectangle

      // are we inside the surrounding rectangle ?

      distMin = 0.0;

      for( i = 0; i < 4; i++) {

        distF =  pointToLineDistance( i, xMouse, yMouse,
                                      FreeAngleInfo.xP[ i], FreeAngleInfo.yP[ i],
                                      FreeAngleInfo.xP[ (i + 1) % 4], FreeAngleInfo.yP[ (i + 1) % 4]);

        if( i == 0 || distF < distMin) {   // get minimum distance
          distMin = distF;
        }
      }

      if( distMin < -1.0) {     // one outside one of the sides
        break;                  // break switch
      }

      dist = dto32( distMin);

      // ...

      if( *distanceToBeat == -1 || dist < *distanceToBeat) {        /* position and border            */

        // save new distance to beat
        *distanceToBeat = dist;

        Cursor   = 0;
        DeltaAdd = 0;

        // get the shortest line

        distMin   = 0.0;

        for( i = 0; i < 4; i++) {

          distF =  PointDistance( FreeAngleInfo.xP[ i], FreeAngleInfo.yP[ i],
                                  FreeAngleInfo.xP[ (i + 1) % 4], FreeAngleInfo.yP[ (i + 1) % 4]);

          if( i == 0 || distF < distMin) {   // get minimum distance
            distMin   = distF;
          }
        }

        // one quart of shortest line for corner distance decision

#ifdef use_again
        deltaMax = 12;
#else
        deltaMax = (int)(12 / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
#endif

        bx = dto32( ceil( distMin / 4));
        if( bx > deltaMax) bx = deltaMax;
        if( bx < 1) bx = 1;

        // test to nearest point

        distMin = 99999999.0;
        IndexBest = -1;

        for( i = 0; i < 4; i++) {

          distF =  PointDistance( xMouse, yMouse, FreeAngleInfo.xP[ i], FreeAngleInfo.yP[ i]);

          if( distF <= bx && distF < distMin) {   // near enough at point, get minimum distance
            distMin = distF;
            IndexBest = i;
          }
        }

        if( IndexBest >= 0) {     // near a point

          DeltaAdd = FreeAngleInfo.CursorPointAction[ IndexBest];
          Cursor   = FreeAngleInfo.CursorPointShape[ IndexBest];

        }  else {

          // find nearest line

          distMin = 99999999.0;
          IndexBest = -1;

          for( i = 0; i < 4; i++) {

            distF =  pointToLineDistance( i, xMouse, yMouse,
              FreeAngleInfo.xP[ i], FreeAngleInfo.yP[ i],
              FreeAngleInfo.xP[ (i + 1) % 4], FreeAngleInfo.yP[ (i + 1) % 4]);


            if( distF <= bx && distF < distMin) {   // near enough at point, get minimum distance
              distMin = distF;
              IndexBest = i;
            }

            if( IndexBest >= 0) {     // near a point

              DeltaAdd = FreeAngleInfo.CursorLineAction[ IndexBest];
              Cursor   = FreeAngleInfo.CursorLineShape[ IndexBest];

            } else {

              // must be inside, move window

              DeltaAdd = 0x0f;
              Cursor   = FL_CURSOR_MOVE;  //x/ FL_CURSOR_HAND;          // cursor shape: hand
            }
          }
        }

        *pCursor = Cursor;
        *pDeltaAdd = DeltaAdd;

        return( 1);   // return new best distance
      }
    }

    ierr = 0;
    break;
  } // End switch

  return( 0);       // Mouse not inside window or not nearer than best distance
}

/************************************************************************************
 * YaIPS_EdgeAOI_DeltaAdd
 *
 * Add position change to EdgeAOI
 *
 * The AOI rectangle is relative to image displayed on the screen 'BigImage_iw/-ih'.
 *
 *   pAOI            Point to AOI
 *   AoiDeltaAdd     Where to add mouse delta
 *   Delta_x         Delta in X direction
 *   Delta_y         Delta in y direction
 *
 * return:  < 0  Error
 *            0  OK
 */
int YaIPS_EdgeAOI_DeltaAdd( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                            YaIPS_EdgeAOI_t *pEdgeAOI,  // Pointer to EdgeAOI data
                            int orientation,            // 0: x-distance, 1: y-distance, 2: orientation is given parameter
                            int OrientationAngle,       // orientation angle for mode (180 .. - 180 Grad)
                            int AoiDeltaAdd,            // Where to add mouse delta
                            int Delta_x,                // Delta in X direction
                            int Delta_y)                // Delta in y direction
{
  int ierr, LineNR, xMouse, yMouse;
  Fl_YaIPS_AOI_t *pAOI;
  int AbsAngle, OrgAngle, xSel, ySel, xxSel, yySel;
  double distF, DeltaX, DeltaY;
  double xAP, yAP, xBP, yBP;
  double x0, y0, x1, y1, xP[ 4], yP[ 4], Cx0, Cy0, Cx1, Cy1, Cx2, Cy2; //x/ xA, xB, xC, xD, yA, yB, yC, yD; /* border coordinates */
  double dxScanDir, dyScanDir, dxSideDir, dySideDir;
  int    nSample, nQuer;
  T_FreeAngleInfo FreeAngleInfo;

  pAOI = &pEdgeAOI->AOI;

  switch( orientation) {

  case WIN_PAR_ORIENT_HORIZONTAL:
  case WIN_PAR_ORIENT_VERTIKAL:

    ierr = YaIPS_ImageDispAoiRectDeltaAdd( pYaIPS_ImageDisp, pAOI,
                                           AoiDeltaAdd, Delta_x, Delta_y);

    return( ierr);
    break;

  default:

    // WIN_PAR_ORIENT_PARAMETER

    if( AoiDeltaAdd == 0x0f) {           // Move

      // Add to the points

      pAOI->XPos += Delta_x;
      pAOI->YPos += Delta_y;

    } else {                                // change side or edges

      OrgAngle = OrientationAngle;
      AbsAngle = OrgAngle;              // get absolute angle
      if(  AbsAngle < 0) {             // absolute value

        AbsAngle = - AbsAngle;
      }

      xMouse = (int)( pYaIPS_ImageDisp->MouseX / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);  // Mouse relative to displayed screen part
      yMouse = (int)( pYaIPS_ImageDisp->MouseY / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

      xMouse += (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);   // Add Offset to displayed screen part
      yMouse += (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

      getWinCoordinates( &FreeAngleInfo, &pEdgeAOI->AOI, OrientationAngle, YaIPS_Calib_UPP_X / YaIPS_Calib_UPP_Y, 0,
                         pYaIPS_ImageDisp->BigImage_iw, pYaIPS_ImageDisp->BigImage_ih, 0.0, &nSample, &nQuer,
                         &x0, &y0, &x1, &y1, xP + 0, xP + 1, xP + 3, xP + 2, yP + 0, yP + 1, yP + 3, yP + 2,
                         &xAP, &yAP, &xBP, &yBP,
                         &dxScanDir, &dyScanDir, &dxSideDir, &dySideDir);

       // setup, we have 1 (for lines) or 2 (for corners) passes

      DeltaX = 0;
      DeltaY = 0;

      xSel = pAOI->XPos;
      ySel = pAOI->YPos;
      xxSel = pAOI->XSize;
      yySel = pAOI->YSize;

actionLPressedRestart2:

      // get testing point/line

      LineNR = (AoiDeltaAdd & 0x03);                     // lower 2 bits

      Cx1 = xP[ LineNR];
      Cy1 = yP[ LineNR];
      Cx2 = xP[ (LineNR + 1)  & 0x03];
      Cy2 = yP[ (LineNR + 1)  & 0x03];

      // Position of mouse (relative to one of the points of the line)

      Cx0 = Cx1 + Delta_x; //x/ SumChangeX;
      Cy0 = Cy1 + Delta_y; //x/ SumChangeY;

      distF = pointToLineDistance( 0, Cx0, Cy0, Cx1, Cy1,  Cx2, Cy2);

      distF = 0 - distF;                           // must be negated

      if( AoiDeltaAdd & 0x0004) {                  // changes XX if set

        if( distF >= 0) {

          xxSel = pAOI->XSize + (int)(distF + 0.5);
        } else {
          xxSel = pAOI->XSize + (int)(distF - 0.5);
        }

        if( xxSel < YAIPS_IDISP_AOI_MIN_SIZE) {          // Clipping

          xxSel = YAIPS_IDISP_AOI_MIN_SIZE;
          distF = (xxSel - pAOI->XSize);
        }

        if( xxSel >= pYaIPS_ImageDisp->BigImage_iw) {   // Clipping

          xxSel = (pYaIPS_ImageDisp->BigImage_iw - 1);
          distF = (xxSel - pAOI->XSize);
        }

        if( LineNR == 0) {

          // OK for -44 .. 44
          DeltaX = DeltaX - (distF * dxScanDir);
          DeltaX = DeltaX - (distF * (1.0 - dxScanDir) * 0.5);
          DeltaY = DeltaY - (distF * dyScanDir * 0.5);
        }

        if( LineNR == 1) {

          // OK for >= 45
          // OK for <= -45
          DeltaX = DeltaX + (distF * dxSideDir);
          DeltaX = DeltaX - (distF * (1.0 + dxSideDir) * 0.5);
          DeltaY = DeltaY + (distF * dySideDir * 0.5);
        }

        if( LineNR == 2) {

          // OK for -44 .. 44
          DeltaX = DeltaX - (distF * (1.0 - dxScanDir) * 0.5);
          DeltaY = DeltaY + (distF * dyScanDir * 0.5);
        }

        if( LineNR == 3) {

          // OK for <= -45
          // OK for >= 45
          DeltaX = DeltaX - (distF * dxSideDir);
          DeltaX = DeltaX - (distF * (1.0 - dxSideDir) * 0.5);
          DeltaY = DeltaY - (distF * dySideDir * 0.5);
        }

      } else {                                       // change YY

        if( distF >= 0) {

          yySel = pAOI->YSize + (int)(distF + 0.5);
        } else {
          yySel = pAOI->YSize + (int)(distF - 0.5);
        }

        if( yySel < YAIPS_IDISP_AOI_MIN_SIZE) {          // Clipping

          yySel = YAIPS_IDISP_AOI_MIN_SIZE;
          distF = yySel - pAOI->YSize;
        }

        if( yySel >= pYaIPS_ImageDisp->BigImage_ih) {   // Clipping

          yySel = pYaIPS_ImageDisp->BigImage_ih - 1;
          distF = yySel - pAOI->YSize;
        }

        if( LineNR == 0) {

          // OK for <= -45
          // OK for >= 45
          DeltaX = DeltaX - (distF * dxScanDir * 0.5);
          DeltaY = DeltaY - (distF * (1.0 + dyScanDir) * 0.5);
        }

        if( LineNR == 1) {

          // OK for -44 .. 44
          DeltaX = DeltaX + (distF * dxSideDir * 0.5);
          DeltaY = DeltaY - (distF * (1.0 - dySideDir) * 0.5);
        }

        if( LineNR == 2) {

          // OK for >= 45
          // OK for <= -45
          DeltaX = DeltaX + (distF * dxScanDir * 0.5);
          DeltaY = DeltaY - (distF * (1.0 - dyScanDir) * 0.5);
        }

        if( LineNR == 3) {

          // OK for -44 .. 44
          DeltaX = DeltaX - (distF * (dxSideDir * 0.5));
          DeltaY = DeltaY - (distF * dySideDir);
          DeltaY = DeltaY - (distF * (1.0 - dySideDir) * 0.5);
        }
      }

      AoiDeltaAdd = (AoiDeltaAdd >> 8);         // shift down to get second part ( for corners)
      if( AoiDeltaAdd > 0) {                    // have something to do

        goto actionLPressedRestart2;            // restart for the second time
      }

      if( DeltaX >= 0) {

        xSel = pAOI->XPos + (int)(DeltaX + 0.5);
      } else {
        xSel = pAOI->XPos + (int)(DeltaX - 0.5);
      }

      if( DeltaY >= 0) {

        ySel = pAOI->YPos + (int)(DeltaY + 0.5);
      } else {
        ySel = pAOI->YPos + (int)(DeltaY - 0.5);
      }

      if( xSel < 0) {                     // Clipping

        xSel = 0;
      }

      if( (xSel + xxSel) >= pYaIPS_ImageDisp->BigImage_iw) {   // Clipping

        xSel = pYaIPS_ImageDisp->BigImage_iw - xxSel - 1;
      }

      if( ySel < 0) {                     // Clipping

        ySel = 0;
      }

      if( (ySel + yySel) >= pYaIPS_ImageDisp->BigImage_ih) {   // Clipping

        ySel = pYaIPS_ImageDisp->BigImage_ih - yySel - 1;
      }

      pAOI->XPos = xSel;
      pAOI->YPos = ySel;
      pAOI->XSize = xxSel;
      pAOI->YSize = yySel;
    }

    ierr = 0;       // OK
    break;
  } // End switch

  return( ierr);
}

/****************************** End Of File ******************************/
