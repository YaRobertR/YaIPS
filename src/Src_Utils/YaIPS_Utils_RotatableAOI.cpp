/****************************************************************************

  YaIPS_Utils_RotatableAOI.cpp

  Support for rotatable AOI

 26.11.2025 RR: First edition of this file.

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

#include "YaIPS.h"

/************************************************************************************
* Support for rotatable AOI
*/

/*
 =================
 pointToLineDistance

// Find the distance between a point (x0,y0) and a line specified by
// two points (x1, y1), (x2, y2)
 =================
 */
#ifdef use_again
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
#else
static double pointToLineDistance( int LineNr, double x0, double y0, double x1, double y1, double x2, double y2, double *pFootX, double *pFootY)
{
  double distF, len2, dx, dy, t;

  dx = x2-x1;
  dy = y2-y1;

  len2 = dx * dx + dy * dy;

  if( len2 > 0.0) {

    // Distance from line
    distF = (dx * (y1-y0) - (x1-x0) * dy) / sqrt( len2);

    distF = - distF;   // negate here

    // Foot point

    if( pFootX != 0 && pFootY != NULL) {

      // Compute projection parameter t

      t = ((x0 - x1) * dx + (y0 - y1) * dy) / len2;

      *pFootX = x1 + t * dx;
      *pFootY = y1 + t * dy;
    }
  } else {
    distF = 0.0;

    if( pFootX != 0 && pFootY != NULL) {

      *pFootX = x0;
      *pFootY = y0;
    }
  }

#ifdef _DEBUG
  //printf2Console( "Dist %d: %7.2f\n", LineNr, distF);
#endif

  return( distF);
}
#endif

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

static int getWinMoreData( T_FreeAngleInfo *pFreeAngleInfo, Fl_YaIPS_AOI_t *pWinDesc,
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

#ifdef use_again
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
#else

  LowXIndex = 0;
#endif

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

int YaIPS_RotatableAOI_Coordinates( T_FreeAngleInfo * pFreeAngleInfo, Fl_YaIPS_AOI_t *pWinDesc,
                              double OrientationAngleArg,
                              double *retx0, double *rety0, double *retx1, double *rety1,
                              double *retxA, double *retxB, double *retxC, double *retxD,
                              double *retyA, double *retyB, double *retyC, double *retyD,
                              int *retnSample, int *retnQuer,
                              double *pdxScanDir, double *pdyScanDir, double *pdxSideDir, double *pdySideDir) /*
============================================================================================= */
// mode=0: calculate window border coordinates only
// mode=1: calculate peak position coordinates only
// mode=2: calculate both
{
  double xC0, yC0, xC1, yC1, OrientationAngle;
  double dxScanDir, dyScanDir, dxSideDir, dySideDir;
  int nSample, nQuer;
  double CenterX, CenterY, LengthScanDir, LengthSideDir;
  double xA, xB, xC, xD, yA, yB, yC, yD;

  OrientationAngle = OrientationAngleArg;
  if( OrientationAngle > 180.0) {       // Change 0 .. 360.0 to -180.0 .. 180.0
    OrientationAngle = OrientationAngle - 360.0;
  }

  dxScanDir =   cos((double)OrientationAngle * M_PI / 180.0);
  dyScanDir = - sin((double)OrientationAngle * M_PI / 180.0);

  nSample       = pWinDesc->XSize;
  nQuer         = pWinDesc->YSize;
  LengthScanDir = pWinDesc->XSize /*- 1.0*/;
  LengthSideDir = pWinDesc->YSize /*- 1.0*/;

  // calculate window center point, compensate 1 pixel for border drawing

  CenterX = (double)pWinDesc->XPos + (double)pWinDesc->XSize * 0.5;
  CenterY = (double)pWinDesc->YPos + (double)pWinDesc->YSize * 0.5;

  // calculate window center line

  xC0 = CenterX - dxScanDir * (LengthScanDir * 0.5);
  yC0 = CenterY - dyScanDir * (LengthScanDir * 0.5);
  xC1 = CenterX + dxScanDir * (LengthScanDir * 0.5);
  yC1 = CenterY + dyScanDir * (LengthScanDir * 0.5);

  // get slope to the side

  dxSideDir =   cos((double)(OrientationAngle + 90) * M_PI / 180.0);
  dySideDir = - sin((double)(OrientationAngle + 90) * M_PI / 180.0);

  xA = (double)xC0 - dxSideDir * (LengthSideDir * 0.5);
  yA = (double)yC0 - dySideDir * (LengthSideDir * 0.5);
  xB = (double)xC0 + dxSideDir * (LengthSideDir * 0.5);
  yB = (double)yC0 + dySideDir * (LengthSideDir * 0.5);
  xC = (double)xC1 - dxSideDir * (LengthSideDir * 0.5);
  yC = (double)yC1 - dySideDir * (LengthSideDir * 0.5);
  xD = (double)xC1 + dxSideDir * (LengthSideDir * 0.5);
  yD = (double)yC1 + dySideDir * (LengthSideDir * 0.5);

  // optional returns

  if( retx0 != (double *)NULL) *retx0 = xC0;
  if( rety0 != (double *)NULL) *rety0 = yC0;
  if( retx1 != (double *)NULL) *retx1 = xC1;
  if( rety1 != (double *)NULL) *rety1 = yC1;
  if( retxA != (double *)NULL) *retxA = xA;
  if( retyA != (double *)NULL) *retyA = yA;
  if( retxB != (double *)NULL) *retxB = xB;
  if( retyB != (double *)NULL) *retyB = yB;
  if( retxC != (double *)NULL) *retxC = xC;
  if( retyC != (double *)NULL) *retyC = yC;
  if( retxD != (double *)NULL) *retxD = xD;
  if( retyD != (double *)NULL) *retyD = yD;

  if( retnSample != (int *)NULL) *retnSample = nSample;
  if( retnQuer != (int *)NULL) *retnQuer = nQuer;
  if( pdxScanDir != (double *)NULL) *pdxScanDir = dxScanDir;
  if( pdyScanDir != (double *)NULL) *pdyScanDir = dyScanDir;
  if( pdxSideDir != (double *)NULL) *pdxSideDir = dxSideDir;
  if( pdySideDir != (double *)NULL) *pdySideDir = dySideDir;

  // collect same more info

  pFreeAngleInfo->CenterX = CenterX;
  pFreeAngleInfo->CenterY = CenterY;
  pFreeAngleInfo->OrientationAngle = OrientationAngle;

  // get more date

  getWinMoreData( pFreeAngleInfo, pWinDesc,
                  xA, xB, xC, xD, yA, yB, yC, yD);

  return(0);
}

/************************************************************************************
* YaIPS_RotatableAOI_Draw
*
* Draw a rotable AOI
*
* NOTE: Make a fl_push_clip() call before calling this function
*
*/

void YaIPS_RotatableAOI_Draw( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                              Fl_YaIPS_AOI_t *pWinDesc,     // Pointer to AOI data
                              Fl_Color DrawColor,           // Color for drawing
                              char *pText,                  // Text for caption
                              float OrientationAngle)       // orientation angle
{
  int LineWidth;
  int x1, y1, OffX, OffY, DrawText;
  //x/double dxScanDir, dyScanDir, dxSideDir, dySideDir;
  double xCL0, yCL0, xCL1, yCL1;
  double xA, xB, xC, xD, yA, yB, yC, yD; /* border coordinates */
  int x1Temp, y1Temp, x2Temp, y2Temp, x3Temp, y3Temp, x4Temp, y4Temp;
  T_FreeAngleInfo FreeAngleInfo;

  // Preparations

  x1 = pYaIPS_ImageDisp->BigImage_sx;
  y1 = pYaIPS_ImageDisp->BigImage_sy;

  LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

  // Points relative to image

  OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
  OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

  // Prepare font size

  DrawText = false;                                            // Preset, do not draw text

  if( pYaIPS_ImageDisp->PixelImageToScreen >= 0.33) {          // Is NOT to tiny

    int TempFontSize;

    DrawText = true;                                           // Draw text

    TempFontSize = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 16.0 + 0.5);

    if( TempFontSize < 10) {
      TempFontSize = 10;
    }

    fl_font( FL_HELVETICA, TempFontSize);
  }

  fl_color( DrawColor);
  fl_line_style( 0, LineWidth);   // Set line width

  YaIPS_RotatableAOI_Coordinates( &FreeAngleInfo, pWinDesc, OrientationAngle,
                                  &xCL0, &yCL0, &xCL1, &yCL1, &xA, &xB, &xC, &xD, &yA, &yB, &yC, &yD);
  // ...

#ifdef use_again

  x1Temp = (int)( (xA - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
  y1Temp = (int)( (yA - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

  x2Temp = (int)( (xB - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
  y2Temp = (int)( (yB - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

  x3Temp = (int)( (xC - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
  y3Temp = (int)( (yC - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

  x4Temp = (int)( (xD - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
  y4Temp = (int)( (yD - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

#else

  double CenterX, CenterY, AngleSin, AngleCos;
  int x1T, y1T, x2T, y2T, x3T, y3T, x4T, y4T;

  x1T = (pWinDesc->XPos * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
  y1T = (pWinDesc->YPos * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

  x2T = ((pWinDesc->XPos + pWinDesc->XSize) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) - 1;
  y2T = y1T;

  x4T = x2T;
  y4T = ((pWinDesc->YPos + pWinDesc->YSize) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) - 1;

  x3T = x1T;
  y3T = y4T;

  CenterX = (x1T + x2T) * 0.5;
  CenterY = (y1T + y4T) * 0.5;

  AngleSin = sin((double)OrientationAngle * M_PI / 180.0);
  AngleCos = cos((double)OrientationAngle * M_PI / 180.0);

  x1Temp = (int)( (CenterX - x1T) * AngleCos + (CenterY - y1T) * AngleSin + CenterX + 0.5);
  y1Temp = (int)( (CenterY - y1T) * AngleCos - (CenterX - x1T) * AngleSin + CenterY + 0.5);

  x2Temp = (int)( (CenterX - x2T) * AngleCos + (CenterY - y2T) * AngleSin + CenterX + 0.5);
  y2Temp = (int)( (CenterY - y2T) * AngleCos - (CenterX - x2T) * AngleSin + CenterY + 0.5);

  x3Temp = (int)( (CenterX - x3T) * AngleCos + (CenterY - y3T) * AngleSin + CenterX + 0.5);
  y3Temp = (int)( (CenterY - y3T) * AngleCos - (CenterX - x3T) * AngleSin + CenterY + 0.5);

  x4Temp = (int)( (CenterX - x4T) * AngleCos + (CenterY - y4T) * AngleSin + CenterX + 0.5);
  y4Temp = (int)( (CenterY - y4T) * AngleCos - (CenterX - x4T) * AngleSin + CenterY + 0.5);

  // ...

  x1Temp = (int)( - OffX * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1Temp + x1;
  y1Temp = (int)( - OffY * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1Temp + y1;

  x2Temp = (int)( - OffX * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x2Temp + x1;
  y2Temp = (int)( - OffY * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y2Temp + y1;

  x3Temp = (int)( - OffX * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x3Temp + x1;
  y3Temp = (int)( - OffY * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y3Temp + y1;

  x4Temp = (int)( - OffX * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x4Temp + x1;
  y4Temp = (int)( - OffY * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y4Temp + y1;
#endif

  fl_line( x1Temp, y1Temp, x2Temp, y2Temp);
  fl_line( x2Temp, y2Temp, x4Temp, y4Temp);
  fl_line( x4Temp, y4Temp, x3Temp, y3Temp);
  fl_line( x3Temp, y3Temp, x1Temp, y1Temp);

  // Draw label and quality of correlation to screen

  if( DrawText && pText != NULL && *pText != '\0') {   // Draw text

    int mdx, mdy, mw, mh;

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
}

/************************************************************************************
 * YaIPS_RotatableAOI_MouseCC
 *
 * Rotatable AOI rectangle clip and check for mouse selection.
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

int YaIPS_RotatableAOI_MouseCC( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                           Fl_YaIPS_AOI_t *pWinDesc,   // Pointer to AOI data
                           float OrientationAngle,     // orientation angle
                           int *distanceToBeat,        // In Out: Distance to beat
                           int *pCursor,               // Out: Cursor shape
                           int *pDeltaAdd)             // Out: Where to add mouse delta
{
  int ierr, xMouse, yMouse, i, dist, Cursor, DeltaAdd;
  int xs, ys, xx, yy, bx, deltaMax, IndexBest;
  double x0, y0, x1, y1, xA, xB, xC, xD, yA, yB, yC, yD; /* border coordinates */
  T_FreeAngleInfo FreeAngleInfo;

  if( pYaIPS_ImageDisp->pImage_Box == NULL ||          // Security test, have no big image box
      pYaIPS_ImageDisp->BigImage_Calc_OK == false) {   // Size calculations failed

    return( -1);    // Return error
  }

  xMouse = (int)( pYaIPS_ImageDisp->MouseX / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);  // Mouse relative to displayed screen part
  yMouse = (int)( pYaIPS_ImageDisp->MouseY / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

  xMouse += (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);   // Add Offset to displayed screen part
  yMouse += (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

  YaIPS_RotatableAOI_Coordinates( &FreeAngleInfo, pWinDesc, OrientationAngle,
                                  &x0, &y0, &x1, &y1, &xA, &xB, &xC, &xD, &yA, &yB, &yC, &yD);


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
                                    FreeAngleInfo.xP[ (i + 1) % 4], FreeAngleInfo.yP[ (i + 1) % 4],
                                    NULL, NULL);

      if( i == 0 || distF < distMin) {   // get minimum distance
        distMin = distF;
      }
    }

    if( distMin < -1.0) {     // one outside one of the sides

      goto ExitPoint;                  // break switch
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
            FreeAngleInfo.xP[ (i + 1) % 4], FreeAngleInfo.yP[ (i + 1) % 4],
            NULL, NULL);

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

ExitPoint:

  ierr = 0;

  return( ierr);       // Mouse not inside window or not nearer than best distance
}

/************************************************************************************
 * YaIPS_RotatableAOI_DeltaAdd
 *
 * Add position change to rotatable AOI
 *
 * The AOI rectangle is relative to image displayed on the screen 'BigImage_iw/-ih'.
 *
 *   pAOI            Point to AOI
 *   AoiDeltaAdd     Where to add mouse delta
 *   Delta_x_arg     Delta in X direction
 *   Delta_y_arg     Delta in y direction
 *
 * return:  < 0  Error
 *            0  OK
 */
int YaIPS_RotatableAOI_DeltaAdd( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                                 Fl_YaIPS_AOI_t *pAOI,       // Pointer to AOI data
                                 float OrientationAngle,     // orientation angle
                                 int AoiDeltaAdd,            // Where to add mouse delta
                                 int Delta_x_arg,            // Delta in X direction
                                 int Delta_y_arg,            // Delta in y direction
                                 int RatioLocked,            // True for locked size ratio
                                 int RatioXX,                // Used as size ratio reference
                                 int RatioYY)
{
  int ierr, xMouse, yMouse;
  int xSel, ySel, xxSel, yySel, Have2Lines;
  double DeltaX, DeltaY;
  double x0, y0, x1, y1, xP[ 4], yP[ 4];
  double dxScanDir, dyScanDir, dxSideDir, dySideDir;
  int iLine, nLines, LineNR, LineNR_ToDo[ 4], TempDeltaAdd;
  double distF, distF_ToDo[ 4], xxSelAdd, yySelAdd, Fx, Fy;
  T_FreeAngleInfo FreeAngleInfo;

  xSel = pAOI->XPos;
  ySel = pAOI->YPos;
  xxSel = pAOI->XSize;
  yySel = pAOI->YSize;

  if( AoiDeltaAdd == 0x0f) {           // Move

    // Add to the points

    xSel += Delta_x_arg;
    ySel += Delta_y_arg;

  } else {                                // change side or edges

    xMouse = (int)( pYaIPS_ImageDisp->MouseX / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);  // Mouse relative to displayed screen part
    yMouse = (int)( pYaIPS_ImageDisp->MouseY / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

    xMouse += (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);   // Add Offset to displayed screen part
    yMouse += (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

    YaIPS_RotatableAOI_Coordinates( &FreeAngleInfo, pAOI, OrientationAngle,
                                    &x0, &y0, &x1, &y1, xP + 0, xP + 1, xP + 3, xP + 2, yP + 0, yP + 1, yP + 3, yP + 2,
                                    NULL, NULL,
                                    &dxScanDir, &dyScanDir, &dxSideDir, &dySideDir);

    // Prepare for locked size ratio

    if( RatioXX <= 0 || RatioYY <= 0) {  // Security check. If one of this is zero, disable locked.

      RatioLocked = false;
    }

     // setup, we have 1 (for lines) or 2 (for corners) passes

    DeltaX = 0;
    DeltaY = 0;

    Have2Lines = ( AoiDeltaAdd & 0xff00) != 0;    // Have a second line to process

    // get testing point/line

    nLines = 0;

    if( ! RatioLocked) {           // Ratio is NOT locked

      LineNR = (AoiDeltaAdd & 0x03);                     // lower 2 bits

      distF = pointToLineDistance( 0, xP[ LineNR] + Delta_x_arg, yP[ LineNR] + Delta_y_arg, xP[ LineNR], yP[ LineNR], xP[ (LineNR + 1)  & 0x03], yP[ (LineNR + 1)  & 0x03], NULL, NULL);

      distF = 0 - distF;                         // must be negated

      // Store line for later processing
      LineNR_ToDo[ nLines] = LineNR;
      distF_ToDo[ nLines] = distF;
      nLines += 1;

      if( Have2Lines) {     // Have two lines to process

        // A corner  is selected

        TempDeltaAdd = AoiDeltaAdd >> 8;

        LineNR = (TempDeltaAdd & 0x03);                     // lower 2 bits

        distF = pointToLineDistance( 0, xP[ LineNR] + Delta_x_arg, yP[ LineNR] + Delta_y_arg, xP[ LineNR], yP[ LineNR], xP[ (LineNR + 1)  & 0x03], yP[ (LineNR + 1)  & 0x03], NULL, NULL);

        distF = 0 - distF;                         // must be negated

        // Store line for later processing
        LineNR_ToDo[ nLines] = LineNR;
        distF_ToDo[ nLines]  = distF;
        nLines += 1;
      }

    } else {                       // Ratio is locked

      LineNR = (AoiDeltaAdd & 0x03);               // lower 2 bits

      if( ! Have2Lines) {          // Have on line to process

        // A side line is selected

        distF = pointToLineDistance( 0, xP[ LineNR] + Delta_x_arg, yP[ LineNR] + Delta_y_arg, xP[ LineNR], yP[ LineNR], xP[ (LineNR + 1)  & 0x03], yP[ (LineNR + 1)  & 0x03], NULL, NULL);

        distF = 0 - distF;                         // must be negated

        // Store line for later processing
        LineNR_ToDo[ nLines] = LineNR;
        distF_ToDo[ nLines] = distF;
        nLines += 1;

        // Add change other  side lines

        if( AoiDeltaAdd & ACTION_FA_CHANGE_XX) {  // Change XX size

          distF = distF * RatioYY / RatioXX;

        } else {                                  // Change YY size

          distF = distF * RatioXX / RatioYY;
        }

        LineNR_ToDo[ nLines] = (LineNR + 1) & 0x03;
        distF_ToDo[ nLines] = distF * 0.5;
        nLines += 1;

        LineNR_ToDo[ nLines] = (LineNR + 3) & 0x03;
        distF_ToDo[ nLines] = distF * 0.5;
        nLines += 1;

      } else  {     // Have two line to process

        // A corner  is selected


        // Get foot point to diagonal corner
        pointToLineDistance( 0, xP[ LineNR] + Delta_x_arg, yP[ LineNR] + Delta_y_arg, xP[ LineNR], yP[ LineNR], xP[ (LineNR + 2)  & 0x03], yP[ (LineNR + 2)  & 0x03], &Fx, &Fy);

#ifdef use_again
#ifdef _DEBUG

        YaIPS_ImageDispStrDebug( &YaIPS_BigImageDisp, "F %.3f/%.3f", Fx - xP[ LineNR], Fy - yP[ LineNR]);

        YaIPS_BigImageDisp.pImage_Box->redraw();
#endif
#endif
        distF = pointToLineDistance( 0, Fx, Fy, xP[ LineNR], yP[ LineNR], xP[ (LineNR + 1)  & 0x03], yP[ (LineNR + 1)  & 0x03], NULL, NULL);

        distF = 0 - distF;                         // must be negated

        // Store line for later processing
        LineNR_ToDo[ nLines] = LineNR;
        distF_ToDo[ nLines] = distF;
        nLines += 1;

        TempDeltaAdd = AoiDeltaAdd >> 8;

        LineNR = (TempDeltaAdd & 0x03);                     // lower 2 bits

        distF = pointToLineDistance( 0, Fx, Fy, xP[ LineNR], yP[ LineNR], xP[ (LineNR + 1)  & 0x03], yP[ (LineNR + 1)  & 0x03], NULL, NULL);

        distF = 0 - distF;                         // must be negated

        // Store line for later processing
        LineNR_ToDo[ nLines] = LineNR;
        distF_ToDo[ nLines] = distF;
        nLines += 1;
      }
    }

#ifdef use_again
#ifdef _DEBUG
    char TempStringShort[ 64], TempStringLong[ 256];

    strcpy( TempStringLong, "");

    for( iLine = 0; iLine < nLines; iLine++) {

      LineNR = LineNR_ToDo[ iLine];
      distF  = distF_ToDo[ iLine];

      if( iLine > 0) {

        strcat( TempStringLong, ",  ");
      }

      sprintf( TempStringShort, "LNr %d %s %.3f", LineNR, (LineNR & 0x01) ? "YY" : "XX", distF);
      strcat( TempStringLong, TempStringShort);
    }

    sprintf( TempStringShort, " D %3d/%3d Org %3d/%3d Size %3d x %3d",
                                                    Delta_x_arg, Delta_y_arg, pAOI->XPos, pAOI->YPos, pAOI->XSize, pAOI->YSize);
    strcat( TempStringLong, TempStringShort);

    YaIPS_ImageDispStrInfo( &YaIPS_BigImageDisp, TempStringLong);

    YaIPS_BigImageDisp.pImage_Box->redraw();
#endif
#endif

    // Process the lines

    xxSelAdd = 0.0;
    yySelAdd = 0.0;

    for( iLine = 0; iLine < nLines; iLine++) {

      LineNR = LineNR_ToDo[ iLine];
      distF  = distF_ToDo[ iLine];

      if( LineNR == 0) {

        // XX Change

        xxSelAdd += distF;

        // OK for -44 .. 44
        DeltaX = DeltaX - (distF * dxScanDir);
        DeltaX = DeltaX - (distF * (1.0 - dxScanDir) * 0.5);
        DeltaY = DeltaY - (distF * dyScanDir * 0.5);

      } else if( LineNR == 1) {

        // YY Change

        yySelAdd += distF;

        // OK for -44 .. 44
        DeltaX = DeltaX + (distF * dxSideDir * 0.5);
        DeltaY = DeltaY - (distF * (1.0 - dySideDir) * 0.5);

      } else if( LineNR == 2) {

        // XX Change

        xxSelAdd += distF;

        // OK for -44 .. 44
        DeltaX = DeltaX - (distF * (1.0 - dxScanDir) * 0.5);
        DeltaY = DeltaY + (distF * dyScanDir * 0.5);

      } else if( LineNR == 3) {

        // YY Change

        yySelAdd += distF;

        // OK for -44 .. 44
        DeltaX = DeltaX - (distF * (dxSideDir * 0.5));
        DeltaY = DeltaY - (distF * dySideDir);
        DeltaY = DeltaY - (distF * (1.0 - dySideDir) * 0.5);
      }
    }

    // Clip Size

    xxSel = pAOI->XSize + lround(xxSelAdd);
    yySel = pAOI->YSize + lround(yySelAdd);

    if( xxSel < YAIPS_IDISP_AOI_MIN_SIZE) {          // Clipping

      xxSel = YAIPS_IDISP_AOI_MIN_SIZE;
      //x/distF1 = (xxSel - pAOI->XSize);
    }

    if( xxSel > pYaIPS_ImageDisp->BigImage_iw) {   // Clipping

      xxSel = (pYaIPS_ImageDisp->BigImage_iw);
      //x/distF1 = (xxSel - pAOI->XSize);
    }

    if( yySel < YAIPS_IDISP_AOI_MIN_SIZE) {          // Clipping

      yySel = YAIPS_IDISP_AOI_MIN_SIZE;
      //x/distF1 = yySel - pAOI->YSize;
    }

    if( yySel > pYaIPS_ImageDisp->BigImage_ih) {   // Clipping

      yySel = pYaIPS_ImageDisp->BigImage_ih;
      //x/distF1 = yySel - pAOI->YSize;
    }

    // Check correction of radio

    if( RatioLocked) {

      if( RatioXX >= RatioYY) {

        if( yySel != (RatioYY * xxSel + RatioXX / 2) / RatioXX) {

          yySel = (RatioYY * xxSel + RatioXX / 2) / RatioXX;
        }

      } else {

        if( xxSel != (RatioXX * yySel + RatioYY / 2) / RatioYY) {

          xxSel = (RatioXX * yySel + RatioYY / 2) / RatioYY;
        }

      }

      if( xxSel < YAIPS_IDISP_AOI_MIN_SIZE) {          // Clipping

        xxSel = YAIPS_IDISP_AOI_MIN_SIZE;
      }

      if( xxSel > pYaIPS_ImageDisp->BigImage_iw) {   // Clipping

        xxSel = (pYaIPS_ImageDisp->BigImage_iw);
      }

      if( yySel < YAIPS_IDISP_AOI_MIN_SIZE) {          // Clipping

        yySel = YAIPS_IDISP_AOI_MIN_SIZE;
      }

      if( yySel > pYaIPS_ImageDisp->BigImage_ih) {   // Clipping

        yySel = pYaIPS_ImageDisp->BigImage_ih;
      }
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
  }

  // Store change position/size back to AOI

  pAOI->XPos = xSel;
  pAOI->YPos = ySel;
  pAOI->XSize = xxSel;
  pAOI->YSize = yySel;

  // Clip Position to center of AOI

  YaIPS_RotatableAOI_Clip( pYaIPS_ImageDisp->BigImage_iw, pYaIPS_ImageDisp->BigImage_ih, pAOI);

  ierr = 0;       // OK

  return( ierr);
}

/************************************************************************************
 * YaIPS_RotatableAOI_Clip
 *
 * Clip rotatable AOI against image boundaries.
 * The center of the AOI is clipped to stay inside the image.
 *
 *   ImgXX, ImgYY       Size of image
 *   pAOI               Point to AOI to test
 *
 * return:    0  OK
 *            1  Something clipped
 */

int YaIPS_RotatableAOI_Clip( int ImgXX, int ImgYY,       // Size of image
                             Fl_YaIPS_AOI_t *pAOI)       // Point to AOI to test
{
  int RedrawOnExit;

  RedrawOnExit = false;

  // Clip size x
  if( pAOI->XSize < YAIPS_IDISP_AOI_MIN_SIZE) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->XSize = YAIPS_IDISP_AOI_MIN_SIZE;
  }

  if( pAOI->XSize > ImgXX){
    RedrawOnExit = true;                                 // Something clipped
    pAOI->XSize = ImgXX;
  }

  // Clip position x
  if( pAOI->XPos < pAOI->XSize / -2) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->XPos = pAOI->XSize / -2;
  }

  if( pAOI->XPos > ImgXX - pAOI->XSize / 2) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->XPos = ImgXX - pAOI->XSize / 2 - 1;
  }

  // Clip size y
  if( pAOI->YSize < YAIPS_IDISP_AOI_MIN_SIZE){
    RedrawOnExit = true;                                 // Something clipped
    pAOI->YSize = YAIPS_IDISP_AOI_MIN_SIZE;
  }

  if( pAOI->YSize > ImgYY){
    RedrawOnExit = true;                                 // Something clipped
    pAOI->YSize = ImgYY;
  }

  // Clip position y
  if( pAOI->YPos < pAOI->YSize / -2) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->YPos = pAOI->YSize / -2;
  }

  if( pAOI->YPos > ImgYY - pAOI->YSize / 2) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->YPos = ImgYY - pAOI->YSize / 2 - 1;
  }

  return( RedrawOnExit);
}

/************************************************************************************
 * YaIPS_RotatableAOI_ClipGuiUpdate
 *
 * AOI rectangle clip against image boundaries and update GUI input elements of the AOI.
 * The center of the AOI is clipped to stay inside the image.
 *
 * The AOI rectangle is relative to image displayed on the screen 'BigImage_iw/-ih'.
 *
 *   pAOI               Point to AOI to test
 *   mgXX, ImgYY        Size of image
 *   pAOI_X, pAOI_Y     GUI input elements
 *   pAOI_XX, pAOI_YY
 *
 * return:    0  OK
 *            1  One of the GUI elements have been changed.
 */

int YaIPS_RotatableAOI_ClipGuiUpdate( Fl_YaIPS_AOI_t *pAOI,       // Point to AOI to test
                                      int ImgXX, int ImgYY,       // Size of image
                                      void *pAOI_X_Arg,           // GUI input elements, must be a IqeFl_Int_Input pointer
                                      void *pAOI_Y_Arg,
                                      void *pAOI_XX_Arg,
                                      void *pAOI_YY_Arg)
{
  IqeFl_Int_Input *pAOI_X, *pAOI_Y, *pAOI_XX, *pAOI_YY;
  int RedrawOnExit;

  pAOI_X  = (IqeFl_Int_Input *)pAOI_X_Arg;
  pAOI_Y  = (IqeFl_Int_Input *)pAOI_Y_Arg;
  pAOI_XX = (IqeFl_Int_Input *)pAOI_XX_Arg;
  pAOI_YY = (IqeFl_Int_Input *)pAOI_YY_Arg;

  RedrawOnExit = false;

  // Maximums/minimums AOI Inputs

  if( pAOI_X->Max != ImgXX - 1) {       // Maximum is not correct
    pAOI_X->Max = ImgXX - 1;
  }

  if( pAOI_X->Min != ImgXX / -2) {      // Minimum is not correct
    pAOI_X->Min = ImgXX / -2;
  }

  if( pAOI_Y->Max != ImgYY - 1) {       // Maximum is not correct
    pAOI_Y->Max = ImgYY - 1;
  }

  if( pAOI_Y->Min != ImgYY / -2) {      // Minimum is not correct
    pAOI_Y->Min = ImgYY / -2;
  }

  if( pAOI_XX->Max != ImgXX) {          // Maximum is not correct
    pAOI_XX->Max = ImgXX;
  }

  if( pAOI_YY->Max != ImgYY) {          // Maximum is not correct
    pAOI_YY->Max = ImgYY;
  }

  // Clip size x
  if( pAOI->XSize < pAOI_XX->Min){

    RedrawOnExit = true;                              // Something has changed

    pAOI->XSize = pAOI_XX->Min;
  }

  if( pAOI->XSize > ImgXX){

    RedrawOnExit = true;                              // Something has changed

    pAOI->XSize = ImgXX;
  }

  // Clip position x
  if( pAOI->XPos < pAOI->XSize / -2) {

    RedrawOnExit = true;                              // Something has changed

    pAOI->XPos = pAOI->XSize / -2;
  }

  if( pAOI->XPos > ImgXX - pAOI->XSize / 2) {

    RedrawOnExit = true;                              // Something has changed

    pAOI->XPos = ImgXX - pAOI->XSize / 2 - 1;
  }

  // Clip size y
  if( pAOI->YSize < pAOI_YY->Min){

    RedrawOnExit = true;                              // Something has changed

    pAOI->YSize = pAOI_YY->Min;
  }

  if( pAOI->YSize > ImgYY){

    RedrawOnExit = true;                              // Something has changed

    pAOI->YSize = ImgYY;
  }

  // Clip position y
  if( pAOI->YPos < pAOI->YSize / -2) {

    RedrawOnExit = true;                              // Something has changed

    pAOI->YPos = pAOI->YSize / -2;
  }

  if( pAOI->YPos > ImgYY - pAOI->YSize / 2) {

    RedrawOnExit = true;                              // Something has changed

    pAOI->YPos = ImgYY - pAOI->YSize / 2 - 1;
  }

  // Update changed values

  if( pAOI->XPos != pAOI_X->GetValue()) {

    RedrawOnExit = true;                              // Something has changed

    pAOI_X->SetValue( pAOI->XPos);                    // Update on GUI
    pAOI_X->redraw();
  }

  if( pAOI->YPos != pAOI_Y->GetValue()) {

    RedrawOnExit = true;                              // Something has changed

    pAOI_Y->SetValue( pAOI->YPos);                    // Update on GUI
    pAOI_Y->redraw();
  }

  if( pAOI->XSize != pAOI_XX->GetValue()) {

    RedrawOnExit = true;                              // Something has changed

    pAOI_XX->SetValue( pAOI->XSize);                  // Update on GUI
    pAOI_XX->redraw();
  }

  if( pAOI->YSize != pAOI_YY->GetValue()) {

    RedrawOnExit = true;                              // Something has changed

    pAOI_YY->SetValue( pAOI->YSize);                  // Update on GUI
    pAOI_YY->redraw();
  }

  return( RedrawOnExit);
}

/****************************** End Of File ******************************/
