/****************************************************************************

  YaIPS_Utils_Histo.cpp

  Histogram utilities

 22.02.2025 RR: First edition of this file.

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
using namespace std;

#include "YaIPS.h"

/************************************************************************************
 * YaIPS_Histo_SingleCalc
 *
 * Calculate statistical values.
 *
 *
 */
static void YaIPS_Histo_SingleCalc( Fl_YaIPS_Histo_t *pHisto)  // Pointer to histogram
{
  int i;
  double DTemp, Sum, Sum2;

  // Max values for histograms

  pHisto->HistMax   = 0;
  pHisto->nPoints   = 0;
  pHisto->Average   = 0.0;
  pHisto->StdDev    = 0.0;
  Sum  = 0.0;
  Sum2 = 0.0;

  for( i = 0; i < YAIPS_HISTO_N_POINTS; i++) {

    pHisto->nPoints += pHisto->HistoTable[ i];          // Sum up number of points

    DTemp = (double)pHisto->HistoTable[ i] * i;         // Sum up for average
    Sum  += DTemp;
    Sum2 += DTemp * i;                                  // Sum square values

    if( pHisto->HistoTable[ i] > pHisto->HistMax) {     // Get maximum value

      pHisto->HistMax = pHisto->HistoTable[ i];
    }
  }

  // calculate average and standard deviation

  if( pHisto->nPoints > 0) {

    pHisto->Average = (float)( Sum / pHisto->nPoints);
  }

  if( pHisto->nPoints > 1) {

    pHisto->StdDev = (float)sqrt( (Sum2 - (Sum * Sum) / (double)pHisto->nPoints) / (double)(pHisto->nPoints - 1));
  }
}

/************************************************************************************
 * YaIPS_Histo_Measure
 *
 * Measure histogram data from an image.
 *
 * If size of aoi (area of interest) is zero measures complete image
 *
 */
void YaIPS_Histo_Measure( Fl_RGB_Image *pImage_Img,          // Pointer to image envelop
                          Fl_YaIPS_Histo_RGB_t *pHisto_RGB,  // Pointer to RGB histogram
                          int AoiX, int AoiY,                // AOI rectangle start point
                          int AoiW, int AoiH)                // AOI rectangle size
{
  uchar *pSrcLine, *pSrcLine2;
  int d, line_d, x, y, w, h;

  // Reset measured data

  memset( pHisto_RGB, 0, sizeof( Fl_YaIPS_Histo_RGB_t));

  // Get image data

  d = pImage_Img->d();
  line_d = pImage_Img->ld() ? pImage_Img->ld() : pImage_Img->data_w() * d;

  w = pImage_Img->data_w();
  h = pImage_Img->data_h();

  pSrcLine = (uchar *)pImage_Img->data()[ 0];

  // Check aoi arguments

  if( AoiW <= 0 || AoiH <= 0) {                     // Any aoi size is zero

    AoiX = 0;                                       // Use all
    AoiY = 0;
    AoiW = w;
    AoiH = h;

  } else {                                          // Check/clip aoi values

    if( AoiW >= w) {

      AoiW = w;
      AoiX = 0;

    } else {

      if( AoiX < 0) {

        AoiX = 0;

      } else if( AoiX > w - AoiW) {

        AoiX = w - AoiW;
      }
    }

    if( AoiH >= h) {

      AoiH = h;
      AoiY = 0;

    } else {

      if( AoiY < 0) {

        AoiY = 0;

      } else if( AoiY > h - AoiH) {

        AoiY = h - AoiH;
      }
    }
  }

  // Point to first pixel of aoi

  pSrcLine += line_d * AoiY + AoiX * d;

  // ...

  if( d >= 3) {                                     // RGB Image

    pHisto_RGB->nHistos = 3;                        // Calculate 3 histograms

    for( y = 0; y < AoiH; y++) {

      pSrcLine2 = pSrcLine;

      for( x = 0; x < AoiW; x++) {

        pHisto_RGB->R.HistoTable[ pSrcLine2[ 0] & 0xff] += 1;
        pHisto_RGB->G.HistoTable[ pSrcLine2[ 1] & 0xff] += 1;
        pHisto_RGB->B.HistoTable[ pSrcLine2[ 2] & 0xff] += 1;

        pSrcLine2 += d;
      }

      pSrcLine += line_d;
    }

  } else if( d >= 1) {                              // BW Image

    pHisto_RGB->nHistos = 1;                        // Calculate 1 histogram

    for( y = 0; y < AoiH; y++) {

      pSrcLine2 = pSrcLine;

      for( x = 0; x < AoiW; x++) {

        pHisto_RGB->R.HistoTable[ pSrcLine2[ 0] & 0xff] += 1;

        pSrcLine2 += d;
      }

      pSrcLine += line_d;
    }
  }

  // Calculate statistical values for the hisograms

  YaIPS_Histo_SingleCalc( &pHisto_RGB->R);
  YaIPS_Histo_SingleCalc( &pHisto_RGB->G);
  YaIPS_Histo_SingleCalc( &pHisto_RGB->B);
}

/************************************************************************************
 * YaIPS_Histo_Measure2
 *
 * Measure histogram data from an image.
 *
 * ColorSpace   In: Color space for color images NORMAL, BW, R, G or B
 * StepFast:    In: If > 1 speed up measurement by using less pixels.
 *                  Use only every 'StepFast* pixel and line to speed up
 *                  measurement. Range is from 0 .. 8;
 *
 * If size of aoi (area of interest) is zero measures complete image
 *
 */
void YaIPS_Histo_Measure2( Fl_RGB_Image *pImage_Img,          // Pointer to image envelop
                           Fl_YaIPS_Histo_RGB_t *pHisto_RGB,  // Pointer to RGB histogram
                           int ColorSpace,                    // In: Color space for color images NORMAL, BW, R, G or B
                           int StepFast,                      // In: If > 1 speed up measurement by using less pixels
                           int AoiX, int AoiY,                // AOI rectangle start point
                           int AoiW, int AoiH)                // AOI rectangle size
{
  uchar *pSrcLine, *pSrcLine2;
  int d, line_d, x, y, w, h;

  // Check arguments

  if( StepFast < 1) {

    StepFast = 1;
  }

  if( StepFast > 8) {

    StepFast = 8;
  }

  // Reset measured data

  memset( pHisto_RGB, 0, sizeof( Fl_YaIPS_Histo_RGB_t));

  // Get image data

  d = pImage_Img->d();
  line_d = pImage_Img->ld() ? pImage_Img->ld() : pImage_Img->data_w() * d;

  w = pImage_Img->data_w();
  h = pImage_Img->data_h();

  pSrcLine = (uchar *)pImage_Img->data()[ 0];

  // Check aoi arguments

  if( AoiW <= 0 || AoiH <= 0) {                     // Any aoi size is zero

    AoiX = 0;                                       // Use all
    AoiY = 0;
    AoiW = w;
    AoiH = h;

  } else {                                          // Check/clip aoi values

    if( AoiW >= w) {

      AoiW = w;
      AoiX = 0;

    } else {

      if( AoiX < 0) {

        AoiX = 0;

      } else if( AoiX > w - AoiW) {

        AoiX = w - AoiW;
      }
    }

    if( AoiH >= h) {

      AoiH = h;
      AoiY = 0;

    } else {

      if( AoiY < 0) {

        AoiY = 0;

      } else if( AoiY > h - AoiH) {

        AoiY = h - AoiH;
      }
    }
  }

  // Point to first pixel of aoi

  pSrcLine += line_d * AoiY + AoiX * d;

  // Multiply in factor for faster walk of image

  d *= StepFast;
  line_d *= StepFast;

  // ...

  if( d >= 3) {                                     // RGB Image

    switch( ColorSpace) {

    case YAIPS_DISP_COLMOD_INVALID:    // Value not valid
    case YAIPS_DISP_COLMOD_NORMAL:     // Normal RGB image

      pHisto_RGB->nHistos = 3;                        // Calculate 3 histograms

      for( y = 0; y < AoiH; y += StepFast) {

        pSrcLine2 = pSrcLine;

        for( x = 0; x < AoiW; x++) {

          pHisto_RGB->R.HistoTable[ pSrcLine2[ 0] & 0xff] += 1;
          pHisto_RGB->G.HistoTable[ pSrcLine2[ 1] & 0xff] += 1;
          pHisto_RGB->B.HistoTable[ pSrcLine2[ 2] & 0xff] += 1;

          pSrcLine2 += d;
        }

        pSrcLine += line_d;
      }
      break;

    case YAIPS_DISP_COLMOD_BW:     // Convert color image to BW

      pHisto_RGB->nHistos = 1;                        // Calculate 1 histogram

      for( y = 0; y < AoiH; y += StepFast) {

        int PixVal;

        pSrcLine2 = pSrcLine;

        for( x = 0; x < AoiW; x++) {

          PixVal = ((pSrcLine2[ 0] & 0xff) * 76 + (pSrcLine2[ 1] & 0xff) * 150 + (pSrcLine2[ 2] & 0xff) * 30) >> 8;

          pHisto_RGB->R.HistoTable[ PixVal] += 1;

          pSrcLine2 += d;
        }

        pSrcLine += line_d;
      }
      break;

    case YAIPS_DISP_COLMOD_R:          // Red component

      pHisto_RGB->nHistos = 1;                        // Calculate 1 histogram

      for( y = 0; y < AoiH; y += StepFast) {

        pSrcLine2 = pSrcLine;

        for( x = 0; x < AoiW; x++) {

          pHisto_RGB->R.HistoTable[ pSrcLine2[ 0] & 0xff] += 1;

          pSrcLine2 += d;
        }

        pSrcLine += line_d;
      }
      break;

    case YAIPS_DISP_COLMOD_G:          // Green component

      pHisto_RGB->nHistos = 1;                        // Calculate 1 histogram

      for( y = 0; y < AoiH; y += StepFast) {

        pSrcLine2 = pSrcLine;

        for( x = 0; x < AoiW; x++) {

          pHisto_RGB->R.HistoTable[ pSrcLine2[ 1] & 0xff] += 1;

          pSrcLine2 += d;
        }

        pSrcLine += line_d;
      }
      break;

    case YAIPS_DISP_COLMOD_B:          // Blue component

      pHisto_RGB->nHistos = 1;                        // Calculate 1 histogram

      for( y = 0; y < AoiH; y += StepFast) {

        pSrcLine2 = pSrcLine;

        for( x = 0; x < AoiW; x++) {

          pHisto_RGB->R.HistoTable[ pSrcLine2[ 2] & 0xff] += 1;

          pSrcLine2 += d;
        }

        pSrcLine += line_d;
      }
      break;

    }  // End switch switch( ColorSpace)

  } else if( d >= 1) {                              // BW Image

    pHisto_RGB->nHistos = 1;                        // Calculate 1 histogram

    for( y = 0; y < AoiH; y += StepFast) {

      pSrcLine2 = pSrcLine;

      for( x = 0; x < AoiW; x++) {

        pHisto_RGB->R.HistoTable[ pSrcLine2[ 0] & 0xff] += 1;

        pSrcLine2 += d;
      }

      pSrcLine += line_d;
    }
  }

  // Calculate statistical values for the hisograms

  YaIPS_Histo_SingleCalc( &pHisto_RGB->R);
  YaIPS_Histo_SingleCalc( &pHisto_RGB->G);
  YaIPS_Histo_SingleCalc( &pHisto_RGB->B);
}

/************************************************************************************
 * hivalFixp
 *
 * Get percent min/max histogram alue.
 *
 */

static int hivalFixp( int *pHist, int nPercent, int size, int dir)
{
  int *ps;
  int i, sum, end, num, lastSum;
  long long mean, lastMean;

  ps = pHist;
  num = YAIPS_HISTO_N_POINTS;
  sum = 0;
  mean = 0;
  lastSum = 0;
  lastMean = 0;
  end = (nPercent * size)/100L;
  if (end <= 0) end = 1;

  switch(dir) {

    case 0: /* ----------------- upstairs ---------------------- */

    i = 0;
    while (1) {
      if (sum >= end) {
        i--;
        mean = lastMean + (long long)(end - lastSum) * (long long)i;
        sum  = end;
        break;
      }
      if (i >= num) {
        i--;
        break;
      }
      lastSum  = sum;
      lastMean = mean;
      sum += *ps;
      mean += (long long)*ps++ * (long long)i;
      i++;
    }
    break;

    default: /* ----------------- downstairs -------------------- */

    ps += num - 1; /* begin with last element */
    i = num - 1;
    while (1) {
      if (sum >= end) {
        i++;
        mean = lastMean + (long long)(end - lastSum) * (long long)i;
        sum  = end;
        break;
      }
      if (i < 0) {
        i++;
        break;
      }
      lastSum  = sum;
      lastMean = mean;
      sum += *ps;
      mean += (long long)*ps-- * (long long)i;
      i--;
    }
    break;

  } /* end of switch() */

  if (sum <= 0) sum = 1; /* be sure, prevent division by zero with empty vector */

  return( (int)dto32( (double)mean / (double)sum));
}

/************************************************************************************
 * YaIPS_Histo_GetVal
 *
 * Get red/green/blue values from an image AOI
 *
 * If size of aoi (area of interest) is zero measures complete image
 *
 */

int YaIPS_Histo_GetVal( Fl_RGB_Image *pImage_Img,         // Pointer to image envelop
                        Fl_YaIPS_AOI_t *pAOI,             // The measurement AOI
                        int Mode,                         // Average/Min/Max
                        int Percent,                      // Min/Max percent value
                        int *pValR,                       // Place red value here
                        int *pValG,                       // Place green value here
                        int *pValB)                       // Place blue value here
{
  Fl_YaIPS_Histo_RGB_t TempHisto;           // Temporary histogramm

  *pValR = 0;      // Set sane result values
  *pValG = 0;
  *pValB = 0;

  YaIPS_Histo_Measure( pImage_Img, &TempHisto, pAOI->XPos, pAOI->YPos, pAOI->XSize, pAOI->YSize);

  if( TempHisto.nHistos <= 0) {      // No histogram data

    return( -1);
  }

  if( Mode == YAIPS_HISTO_MODE_MIN) {

    // Minimum percent value
    *pValR = hivalFixp( TempHisto.R.HistoTable, Percent, pAOI->XSize * pAOI->YSize, 0);

    if( pImage_Img->d() >= 3) {

      *pValG = hivalFixp( TempHisto.G.HistoTable, Percent, pAOI->XSize * pAOI->YSize, 0);
      *pValB = hivalFixp( TempHisto.B.HistoTable, Percent, pAOI->XSize * pAOI->YSize, 0);
    }

  } else if( Mode == YAIPS_HISTO_MODE_MAX) {

    // Maximum percent value
    *pValR = hivalFixp( TempHisto.R.HistoTable, Percent, pAOI->XSize * pAOI->YSize, 1);

    if( pImage_Img->d() >= 3) {

      *pValG = hivalFixp( TempHisto.G.HistoTable, Percent, pAOI->XSize * pAOI->YSize, 1);
      *pValB = hivalFixp( TempHisto.B.HistoTable, Percent, pAOI->XSize * pAOI->YSize, 1);
    }

  } else {     // Must be mode average

    // return average results
    *pValR = dto32( TempHisto.R.Average);
    *pValG = dto32( TempHisto.G.Average);
    *pValB = dto32( TempHisto.B.Average);
  }

  return( 0);   // Return OK
}

/************************************************************************************
 * YaIPS_Histo_Draw
 *
 * Draw histogram.
 *
 */
void YaIPS_Histo_Draw( YaIPS_Fl_Box *pYaIPS_Box,         // Draw into this box widget
                      Fl_YaIPS_Histo_RGB_t *pHisto_RGB,  // Pointer to RGB histogram
                      int RefX, int RefY,                // Left upper reference point for drawing
                      int CurvesYY,                      // Height of curves
                      char *pHeader1,                    // Optional header text
                      char *pHeader2,                    // Optional header text
                      Fl_Color OW_Line,                  // If != 0 use this as line color
                      Fl_Color OW_Text,                  // If != 0 use this as text color
                      Fl_Color OW_CurveBW)               // If != 0 use this as color for a BW histogram
{
  int i, x1, y1, x2, x3, y2, y3, MaxVal, MaxVal2;
  int *pHisto;
  int mw, mh, AvTextX, AvTextY, LineWidth;
  char TempString[ 256];

  if( pYaIPS_Box == NULL) {                   // Security test

    return;
  }

  if( pYaIPS_Box->w() < YAIPS_HISTO_N_POINTS + 4 ||         // Security test minimum size
      pYaIPS_Box->h() < CurvesYY + 4) {

    return;
  }

   fl_push_clip( pYaIPS_Box->x(), pYaIPS_Box->y(), pYaIPS_Box->w(), pYaIPS_Box->h());

  // Draw a RGB Histogram

  //x/Fl_Color ColLine = FL_BLACK, ColText = FL_BLACK, ColR = FL_RED, ColG = FL_GREEN - 2, ColB = FL_BLUE + 2, ColBW = FL_DARK3;
  Fl_Color ColLine = FL_DARK1 + 1, ColText = FL_WHITE, ColBW = FL_WHITE;
  Fl_Color ColR = fl_rgb_color( 255, 0, 0), ColG = fl_rgb_color( 0, 255, 0), ColB = fl_rgb_color( 0, 188, 255);
  static char MyLineDashes[] = { 4, 4, 0};

  // Color overwrites

  if( OW_Line != 0) {
    ColLine = OW_Line;
  }
  if( OW_Text != 0) {
    ColText = OW_Text;
  }
  if( OW_CurveBW != 0) {
    ColBW = OW_CurveBW;
  }

  // Used line width in pixel for histograms, AOIs

  LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

  x1 = pYaIPS_Box->x() + RefX;
  y1 = pYaIPS_Box->y() + RefY;

  x2 = x1 + YAIPS_HISTO_N_POINTS - 1;
  y2 = y1 + CurvesYY - 1;

  AvTextX = pYaIPS_Box->x() + 4;
  AvTextY = y1 - 8;

  fl_color( ColLine);

  fl_line_style( 0, 0);

  // Horizontal axis

  fl_line( x1 - 1, y2 + 1, x2, y2 + 1);
  fl_line( x2 - 4, y2 + 1 - 3, x2, y2 + 1);
  fl_line( x2 - 4, y2 + 1 + 3, x2, y2 + 1);

  // Vertical axis
  fl_line( x1 - 1, y1, x1 - 1, y2);
  fl_line( x1 - 1, y1, x1 - 1 - 4, y1 + 3);
  fl_line( x1 - 1, y1, x1 - 1 + 4, y1 + 3);

  // Vertical lines
  fl_line_style( FL_DOT, 0, MyLineDashes);
  fl_font( FL_HELVETICA, 14);

  // Text

  fl_color( ColText);

  x3 = x1;
  fl_draw( "0", x3 + -1, y2 + 16);

  x3 = x1 + 64;
  fl_draw( "64", x3 - 8, y2 + 16);

  x3 = x1 + 128;
  fl_draw( "128", x3 - 12, y2 + 16);

  x3 = x1 + 196;
  fl_draw( "196", x3 - 12, y2 + 16);

  x3 = x1 + 255;
  fl_draw( "255", x3 - 22, y2 + 16);

  fl_color( ColLine);

  x3 = x1 + 64;
  fl_line( x3, y1, x3, y2);

  x3 = x1 + 128;
  fl_line( x3, y1, x3, y2);

  x3 = x1 + 196;
  fl_line( x3, y1, x3, y2);

  x3 = x1 + 255;
  fl_line( x3, y1, x3, y2);

  if( pHisto_RGB->nHistos <= 0) {            // Have nothing to draw

    goto ExitPoint;
  }

  fl_line_style( 0, LineWidth);   // Set line width

  // Draw optional vertical lines

  if( pHisto_RGB->MarkCol1 != 0 && pHisto_RGB->MarkPos1 >= 0 && pHisto_RGB->MarkPos1 < YAIPS_HISTO_N_POINTS) {

    x3 = x1 + pHisto_RGB->MarkPos1;
    fl_color( pHisto_RGB->MarkCol1);
    fl_line( x3, y1, x3, y2);
  }

  if( pHisto_RGB->MarkCol2 != 0 && pHisto_RGB->MarkPos2 >= 0 && pHisto_RGB->MarkPos2 < YAIPS_HISTO_N_POINTS) {

    x3 = x1 + pHisto_RGB->MarkPos2;
    fl_color( pHisto_RGB->MarkCol2);
    fl_line( x3, y1, x3, y2);
  }

  if( pHisto_RGB->MarkCol3 != 0 && pHisto_RGB->MarkPos3 >= 0 && pHisto_RGB->MarkPos3 < YAIPS_HISTO_N_POINTS) {

    x3 = x1 + pHisto_RGB->MarkPos3;
    fl_color( pHisto_RGB->MarkCol3);
    fl_line( x3, y1, x3, y2);
  }

  // DrawHistograms

  MaxVal = pHisto_RGB->R.HistMax;
  if( pHisto_RGB->G.HistMax > MaxVal) MaxVal = pHisto_RGB->G.HistMax;
  if( pHisto_RGB->B.HistMax > MaxVal) MaxVal = pHisto_RGB->B.HistMax;
  MaxVal2 = MaxVal / 2;

  if( pHisto_RGB->nHistos >= 1) {

    // Draw curve

    pHisto = pHisto_RGB->R.HistoTable;

    fl_color( pHisto_RGB->nHistos == 1 ? ColBW : ColR);

    fl_begin_line();
    for( i = 0; i < YAIPS_HISTO_N_POINTS; i++) {

      y3 = y2 - (pHisto[ i] * CurvesYY + MaxVal2) / MaxVal;

      fl_vertex( x1 + i, y3);
    }
    fl_end_line();

    // Draw average value

    sprintf( TempString, "%5.1f/%5.1f", pHisto_RGB->R.Average, pHisto_RGB->R.StdDev);
    mw = mh = 0;
    fl_measure( TempString, mw, mh);
    fl_draw( TempString, AvTextX, AvTextY);
    AvTextX += mw + 16;
  }

  if( pHisto_RGB->nHistos >= 2) {

    // Draw curve

    pHisto = pHisto_RGB->G.HistoTable;

    fl_color( ColG);

    fl_begin_line();
    for( i = 0; i < YAIPS_HISTO_N_POINTS; i++) {

      y3 = y2 - (pHisto[ i] * CurvesYY + MaxVal2) / MaxVal;

      fl_vertex( x1 + i, y3);
    }
    fl_end_line();

    // Draw average value

    sprintf( TempString, "%5.1f/%5.1f", pHisto_RGB->G.Average, pHisto_RGB->G.StdDev);
    mw = mh = 0;
    fl_measure( TempString, mw, mh);
    fl_draw( TempString, AvTextX, AvTextY);
    AvTextX += mw + 16;
  }

  if( pHisto_RGB->nHistos >= 3) {

    // Draw curve

    pHisto = pHisto_RGB->B.HistoTable;

    fl_color( ColB);

    fl_begin_line();
    for( i = 0; i < YAIPS_HISTO_N_POINTS; i++) {

      y3 = y2 - (pHisto[ i] * CurvesYY + MaxVal2) / MaxVal;

      fl_vertex( x1 + i, y3);
    }
    fl_end_line();

    // Draw average value

    sprintf( TempString, "%5.1f/%5.1f", pHisto_RGB->B.Average, pHisto_RGB->B.StdDev);
    mw = mh = 0;
    fl_measure( TempString, mw, mh);
    fl_draw( TempString, AvTextX, AvTextY);
    AvTextX += mw + 16;
  }

  // Optional header text

  if( pHeader1 != NULL && pHeader1[ 0] != 0) {

    AvTextX = pYaIPS_Box->x() + 4;

    fl_color( FL_WHITE);
    fl_draw( pHeader1, AvTextX, AvTextY - 40);
  }

  if( pHeader2 != NULL && pHeader2[ 0] != 0) {

    AvTextX = pYaIPS_Box->x() + 4;

    fl_color( FL_WHITE);
    fl_draw( pHeader2, AvTextX, AvTextY - 20);
  }

ExitPoint:

  fl_line_style( 0);   // Reset to default
  fl_pop_clip();
}

/****************************** End Of File ******************************/
