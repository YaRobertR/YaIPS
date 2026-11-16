/****************************************************************************

  YaIPS_Utils_ColRowSum.cpp

  Column/Row summation utilities

 20.03.2025 RR: First edition of this file.
 09.10.2026 RR: * YaIPS_ColRowSum_Measure()
                  Added argument color channel to function.
                  This also allows to make profiles from one color channel
                  or from the alpha channel.
 12.10.2026 RR: * YaIPS_ColRowSum_SingleDraw()
                  Added coded to draw the brightness profile smoothed
                  and optional draw the slopes of the profiles.

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

/************************************************************************************
 * YaIPS_ColRowSum_Prepare
 *
 * Prepare profile data
 *
 * return:   0: OK
 *         < 0: Error
 *
 */
static int YaIPS_ColRowSum_Prepare( Fl_YaIPS_ColRowSum_t *pColRowSum, int nElements)
{
  int *pData, i;

  if( pColRowSum->pData != NULL &&            // Have a pointer to data
      pColRowSum->nElements == nElements) {   // and same number of elements

    // Already have prober vector

  } else {

    if( pColRowSum->pData != NULL) {          // Have a vector with different size

      free( pColRowSum->pData);               // Free it

      pColRowSum->pData = NULL;
      pColRowSum->nElements = 0;
    }

    if( nElements > 0) {                      // Have minimum one element

      pColRowSum->pData = (int *)malloc( sizeof( int) * nElements);

      if( pColRowSum->pData == NULL) {       // Security test

        return( -10);   // Error
      }

      pColRowSum->nElements = nElements;
    }
  }

  // Zero the data

  if( pColRowSum->pData != NULL &&            // Have a pointer to data
      pColRowSum->nElements > 0) {            // and some elements

    pData = pColRowSum->pData;

    for( i = 0; i < pColRowSum->nElements; i++) {

      *pData++ = 0;
    }
  }

  // Reset other things

  pColRowSum->ValueMax = 0;
  pColRowSum->ValueMin = 0;

  pColRowSum->ColProfile = YAIPS_COLOR_CURVE_BW;    // Preset color

  return( 0);   // Return OK
}

/************************************************************************************
 * YaIPS_ColRowSum_SingleCalc
 *
 * Calculate statistical values.
 *
 */
static void YaIPS_ColRowSum_SingleCalc( Fl_YaIPS_ColRowSum_t *pColRowSum,  // Pointer to profile
                                       int ValNormalize)                 // If >= 0 normalize values
{
  int i, ValNormalize2;
  int *pData;

  // Min/Max values for profile

  pColRowSum->ValueMax = 0;
  pColRowSum->ValueMin = 0;

  if( pColRowSum->pData == NULL ||                      // Security test
      pColRowSum->nElements <= 0) {                     // or empty vector

    return;
  }

  // ...

  ValNormalize2 = ValNormalize / 2;

  pData = pColRowSum->pData;

  for( i = 0; i < pColRowSum->nElements; i++) {

    if( ValNormalize > 0) {                  // Have to normalize the data

      *pData = (*pData + ValNormalize2) / ValNormalize;
    }

    if( i == 0) {                            // Is first element

      pColRowSum->ValueMax = *pData;
      pColRowSum->ValueMin = *pData;

    } else {                                 // other elements

      if( *pData > pColRowSum->ValueMax) {

        pColRowSum->ValueMax = *pData;
      }

      if( *pData < pColRowSum->ValueMin) {

        pColRowSum->ValueMin = *pData;
      }
    }

    pData += 1;
  }
}

/************************************************************************************
 * YaIPS_ColRowSum_Measure
 *
 * Make column/row sum profiles from an image.
 *
 * If size of aoi (area of interest) is zero measures complete image
 *
 * return:   0: OK
 *         < 0: Error
 *
 */
int YaIPS_ColRowSum_Measure( Fl_RGB_Image *pImage_Img,                  // Pointer to image envelop
                            Fl_YaIPS_ColRowSum_RGB_t *pColRowSum_RGB,   // Pointer to RGB profiles
                            int Flags,                                  // Flag bits
                            int ColChannel,                             // Color channel selection, see YAIPS_DISP_COLMOD_XXX defines
                            int AoiX, int AoiY,                         // AOI rectangle start point
                            int AoiW, int AoiH)                         // AOI rectangle size
{
  uchar *pSrcLine, *pSrcLine2;
  int ierr, d, line_d, x, y, w, h, iChannel, nElements, ValNormalize;

  // Check parameter

  if( pImage_Img == NULL) {                         // have no image

    return( -1);
  }

  // Reset measured data

  if( (Flags & YAIPS_CRSUM_FLAG_RESET) != 0) {         // Reset the data envelope to zero

    memset( pColRowSum_RGB, 0, sizeof( Fl_YaIPS_ColRowSum_RGB_t));
  }

  pColRowSum_RGB->Flags = Flags;                      // Copy flags

  // Get image info

  d = pImage_Img->d();
  line_d = pImage_Img->ld() ? pImage_Img->ld() : pImage_Img->data_w() * d;

  w = pImage_Img->data_w();
  h = pImage_Img->data_h();

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

  // Check color channel

  if( ColChannel < 0 || ColChannel > YAIPS_DISP_COLMOD_MAX) {  // Check out of range

    ColChannel = YAIPS_DISP_COLMOD_NORMAL;    // Use normal channel
  }

  if( ColChannel == YAIPS_DISP_COLMOD_A &&    // use alpha
      d != 2 && d != 4) {                     // and no alpha component

    ColChannel = YAIPS_DISP_COLMOD_NORMAL;    // Use normal channel
  }

  // Check for column / row sum type

  if( (Flags & YAIPS_CRSUM_FLAG_COLSUM) != 0) {        // Make a column sum profile

    nElements    = AoiW;
    ValNormalize = AoiH;

  } else if( (Flags & YAIPS_CRSUM_FLAG_ROWSUM) != 0) { // Make a row sum profile

    nElements    = AoiH;
    ValNormalize = AoiW;

  } else {                                            // undefined

    return( -2);
  }

  if( (Flags & YAIPS_CRSUM_FLAG_NORMALIZE) <= 0) {     // No normalize of values

    ValNormalize = 0;                                 // Reset this
  }

  // Allocate profile data

  ierr = YaIPS_ColRowSum_Prepare( &pColRowSum_RGB->R, nElements);
  if( ierr != 0) {    // Return on error
    return( ierr);
  }

  ierr = YaIPS_ColRowSum_Prepare( &pColRowSum_RGB->G, d >= 3 ? nElements : 0);
  if( ierr != 0) {    // Return on error
    return( ierr);
  }

  ierr = YaIPS_ColRowSum_Prepare( &pColRowSum_RGB->B, d >= 3 ? nElements : 0);
  if( ierr != 0) {    // Return on error
    return( ierr);
  }

  // Get image data

  pSrcLine = (uchar *)pImage_Img->data()[ 0];

  // Point to first pixel of aoi

  pSrcLine += line_d * AoiY + AoiX * d;

  // ...

  if( d >= 3) {                        // RGB Image

    switch( ColChannel) {

    default:
    case YAIPS_DISP_COLMOD_NORMAL:     // Normal RGB image

      pColRowSum_RGB->nProfiles = 3;                  // Calculate 3 profiles

      pColRowSum_RGB->R.ColProfile = YAIPS_COLOR_CURVE_R; // Color for channel
      pColRowSum_RGB->G.ColProfile = YAIPS_COLOR_CURVE_G; // Color for channel
      pColRowSum_RGB->B.ColProfile = YAIPS_COLOR_CURVE_B; // Color for channel

      if( (Flags & YAIPS_CRSUM_FLAG_COLSUM) != 0) {    // Make a column sum profile

        for( y = 0; y < AoiH; y++) {

          pSrcLine2 = pSrcLine;

          for( x = 0; x < AoiW; x++) {

            pColRowSum_RGB->R.pData[ x] += pSrcLine2[ 0];
            pColRowSum_RGB->G.pData[ x] += pSrcLine2[ 1];
            pColRowSum_RGB->B.pData[ x] += pSrcLine2[ 2];

            pSrcLine2 += d;
          }

          pSrcLine += line_d;
        }

      } else {                                        // Make a row sum profile

        for( y = 0; y < AoiH; y++) {

          pSrcLine2 = pSrcLine;

          for( x = 0; x < AoiW; x++) {

            pColRowSum_RGB->R.pData[ y] += pSrcLine2[ 0];
            pColRowSum_RGB->G.pData[ y] += pSrcLine2[ 1];
            pColRowSum_RGB->B.pData[ y] += pSrcLine2[ 2];

            pSrcLine2 += d;
          }

          pSrcLine += line_d;
        }
      }

      break;

    case YAIPS_DISP_COLMOD_BW:         // BW image

      pColRowSum_RGB->nProfiles = 1;                   // Calculate 1 profile

      pColRowSum_RGB->R.ColProfile = YAIPS_COLOR_CURVE_BW; // Color for channel

      if( (Flags & YAIPS_CRSUM_FLAG_COLSUM) != 0) {    // Make a column sum profile

        for( y = 0; y < AoiH; y++) {

          pSrcLine2 = pSrcLine;

          for( x = 0; x < AoiW; x++) {

            // A fast black white conversion. Intensity part of an IHS conversion.
            pColRowSum_RGB->R.pData[ x] += (pSrcLine2[ 0] * 76 + pSrcLine2[ 1] * 150 + pSrcLine2[ 2] * 30) >> 8;

            pSrcLine2 += d;
          }

          pSrcLine += line_d;
        }

      } else {                                        // Make a row sum profile

        for( y = 0; y < AoiH; y++) {

          pSrcLine2 = pSrcLine;

          for( x = 0; x < AoiW; x++) {

            pColRowSum_RGB->R.pData[ y] += (pSrcLine2[ 0] * 76 + pSrcLine2[ 1] * 150 + pSrcLine2[ 2] * 30) >> 8;

            pSrcLine2 += d;
          }

          pSrcLine += line_d;
        }
      }

      break;

    case YAIPS_DISP_COLMOD_R:          // Red component
    case YAIPS_DISP_COLMOD_G:          // Green component
    case YAIPS_DISP_COLMOD_B:          // Blue component
    case YAIPS_DISP_COLMOD_A:          // Alpha component

      pColRowSum_RGB->nProfiles = 1;                   // Calculate 1 profile

      if( ColChannel == YAIPS_DISP_COLMOD_R) {
        pColRowSum_RGB->R.ColProfile = YAIPS_COLOR_CURVE_R; // Color for channel
      } else if( ColChannel == YAIPS_DISP_COLMOD_G) {
        pColRowSum_RGB->R.ColProfile = YAIPS_COLOR_CURVE_G; // Color for channel
      } else if( ColChannel == YAIPS_DISP_COLMOD_B) {
        pColRowSum_RGB->R.ColProfile = YAIPS_COLOR_CURVE_B; // Color for channel
      } else if( ColChannel == YAIPS_DISP_COLMOD_A) {
        pColRowSum_RGB->R.ColProfile = YAIPS_COLOR_CURVE_A; // Color for channel
      } else {
        pColRowSum_RGB->R.ColProfile = YAIPS_COLOR_CURVE_BW; // Color for channel
      }

      iChannel = ColChannel - YAIPS_DISP_COLMOD_R;     // Channel in pixel

      if( (Flags & YAIPS_CRSUM_FLAG_COLSUM) != 0) {    // Make a column sum profile

        for( y = 0; y < AoiH; y++) {

          pSrcLine2 = pSrcLine;

          for( x = 0; x < AoiW; x++) {

            // A fast black white conversion. Intensity part of an IHS conversion.
            pColRowSum_RGB->R.pData[ x] += pSrcLine2[ iChannel];

            pSrcLine2 += d;
          }

          pSrcLine += line_d;
        }

      } else {                                        // Make a row sum profile

        for( y = 0; y < AoiH; y++) {

          pSrcLine2 = pSrcLine;

          for( x = 0; x < AoiW; x++) {

            pColRowSum_RGB->R.pData[ y] += pSrcLine2[ iChannel];

            pSrcLine2 += d;
          }

          pSrcLine += line_d;
        }
      }
      break;
    }

  } else if( d >= 1) {                              // BW Image

    switch( ColChannel) {

    default:

      pColRowSum_RGB->nProfiles = 1;                  // Calculate 1 profile

      pColRowSum_RGB->R.ColProfile = YAIPS_COLOR_CURVE_BW; // Color for channel

      if( (Flags & YAIPS_CRSUM_FLAG_COLSUM) != 0) {    // Make a column sum profile

        for( y = 0; y < AoiH; y++) {

          pSrcLine2 = pSrcLine;

          for( x = 0; x < AoiW; x++) {

            pColRowSum_RGB->R.pData[ x] += pSrcLine2[ 0];

            pSrcLine2 += d;
          }

          pSrcLine += line_d;
        }

      } else {                                        // Make a row sum profile

        for( y = 0; y < AoiH; y++) {

          pSrcLine2 = pSrcLine;

          for( x = 0; x < AoiW; x++) {

            pColRowSum_RGB->R.pData[ y] += pSrcLine2[ 0];

            pSrcLine2 += d;
          }

          pSrcLine += line_d;
        }
      }
      break;

    case YAIPS_DISP_COLMOD_A:          // Alpha component

      pColRowSum_RGB->nProfiles = 1;                  // Calculate 1 profile

      pColRowSum_RGB->R.ColProfile = YAIPS_COLOR_CURVE_A; // Color for channel

      if( (Flags & YAIPS_CRSUM_FLAG_COLSUM) != 0) {    // Make a column sum profile

        for( y = 0; y < AoiH; y++) {

          pSrcLine2 = pSrcLine;

          for( x = 0; x < AoiW; x++) {

            pColRowSum_RGB->R.pData[ x] += pSrcLine2[ 1];

            pSrcLine2 += d;
          }

          pSrcLine += line_d;
        }

      } else {                                        // Make a row sum profile

        for( y = 0; y < AoiH; y++) {

          pSrcLine2 = pSrcLine;

          for( x = 0; x < AoiW; x++) {

            pColRowSum_RGB->R.pData[ y] += pSrcLine2[ 1];

            pSrcLine2 += d;
          }

          pSrcLine += line_d;
        }
      }
      break;
    }
  }

  // Calculate statistical values for the histograms

  YaIPS_ColRowSum_SingleCalc( &pColRowSum_RGB->R, ValNormalize);
  YaIPS_ColRowSum_SingleCalc( &pColRowSum_RGB->G, ValNormalize);
  YaIPS_ColRowSum_SingleCalc( &pColRowSum_RGB->B, ValNormalize);

  return( 0);     // Return OK
}

/************************************************************************************
 * YaIPS_ColRowSum_SingleDraw
 *
 * Draw a single column / row sum profile.
 *
 * NOTE: Make a fl_push_clip() call before calling this function
 *
 */
static int YaIPS_ColRowSum_SingleDraw( int *pDataArg,                             // Point to data
                                       int nElements,                             // number of elements in data vector
                                       int x1, int y1,                            // Left upper drawing reference point
                                       int x2, int y2,                            // Right bottom drawing reference point
                                       int w, int h,                              // With / height of drawing box
                                       int Flags,                                 // Flag bits, see YAIPS_CRSUM_FLAG_XXX defines
                                       Fl_Color ColProfile,                       // Color for the profile drawing
                                       int LenghtSample, int LengthFirst,         // Length of all/first sample to draw
                                       int LineWidth,                             // With of profile lines
                                       Fl_YaIPS_ColRowSum_RGB_t *pColRowSum_RGB,  // Pointer to RGB profiles, needed for smooth and difference
                                       int DrawSmooth, int DrawSlope)              // Draw smooth or difference of profiles
{
  int ierr, i, x3, y3, LenghtThis;
  int *pData;
  int16 filter[ 9], nFilter, exp;
  static char MyLineDashes[] = { 3, 2, 0};


  pData = pDataArg;

  if( DrawSmooth) {

    Tvector VecSrc, Temp1;

    // Create temporary vector for smooth
    ierr = YaIPS_ColRowSum_Prepare( &pColRowSum_RGB->Temp1, nElements);
    if( ierr != 0) {            // Check for error
      goto ErrorExit;
    }

    // Prepare source data
    ierr = ve_Mem2Vec( &VecSrc, DV_HOST, (anypnt)pData, nElements, sizeof(int32), TY_INT32);
    if( ierr != 0) {            // Check for error
      goto ErrorExit;
    }

    // Prepare temp1 data
    ierr = ve_Mem2Vec( &Temp1, DV_HOST, (anypnt)pColRowSum_RGB->Temp1.pData, nElements, sizeof(int32), TY_INT32);
    if( ierr != 0) {            // Check for error
      goto ErrorExit;
    }

    // Small gauss filter

  #ifdef use_again
    nFilter = 3;
    filter[0] = 1; filter[1] = 2; filter[2] = 1;
    exp = -2;
  #else
    //nFilter = 7;
    //filter[0] = 1; filter[1] = 3; filter[2] = 7; filter[3] = 10; filter[4] = 7; filter[5] = 3; filter[6] = 1;
    //exp = -5;

    nFilter = 9;
    filter[0] = filter[8] = 2;
    filter[1] = filter[7] = 4;
    filter[2] = filter[6] = 8;
    filter[3] = filter[5] = 11;
    filter[4] = 14;
    exp = -6;
  #endif

    if( (ierr = vfiltn( &VecSrc, &Temp1, exp, 0, nFilter, filter)) != 0) {   // NOTE: Divide filter result by 4 to get normalized values

      ierr = -4;
      goto ErrorExit;
    }

    // Draw smoothed data

    pData = pColRowSum_RGB->Temp1.pData;
  }

  fl_line_style( 0, LineWidth);   // Set line width

  fl_color( ColProfile);

  fl_begin_line();

  if( (Flags & YAIPS_CRSUM_FLAG_COLSUM) != 0) {        // Make a column sum profile

    LenghtThis = LengthFirst;

    x3 = x1;

    for( i = 0; i < nElements; i++) {

      y3 = y2 - (pData[ i] * h + 128) / 256;

      if( LenghtSample <= 1) {

        x3 = x1 + i * (w - 1) / (nElements - 1);
        fl_vertex( x3, y3);

      } else {

        if( LenghtSample > 1) {         // Scaled up

          fl_vertex( x3, y3);
          if( x3 == x2) {
            break;
          }

          x3 += LenghtThis;

          if( x3 > x2) {
            x3 = x2;
            fl_vertex( x3, y3);
            break;
          }

          fl_vertex( x3, y3);

          x3 += 1;
          if( x3 > x2) {
            break;
          }

          LenghtThis = LenghtSample - 1;
        }
      }
    }

  } else {       // Make a row sum profile

    LenghtThis = LengthFirst;

    y3 = y1;

    for( i = 0; i < nElements; i++) {

      x3 = x1 + (pData[ i] * w + 128) / 256;

      if( LenghtSample <= 1) {

        y3 = y1 + i * (h - 1) / (nElements - 1);
        fl_vertex( x3, y3);

      } else {

        if( LenghtSample > 1) {         // Scaled up

          fl_vertex( x3, y3);
          if( y3 == y2) {
            break;
          }

          y3 += LenghtThis;

          if( y3 > y2) {

            y3 = y2;
            fl_vertex( x3, y3);

            break;
          }

          fl_vertex( x3, y3);
          y3 += 1;
          if( y3 > y2) {
            break;
          }

          LenghtThis = LenghtSample - 1;
        }
      }
    }
  }

  fl_end_line();


  // Draw difference line

  if( DrawSlope) {

    Tvector VecSrc, Temp1;

    // Create temporary vector for smooth
    ierr = YaIPS_ColRowSum_Prepare( &pColRowSum_RGB->Temp2, nElements);
    if( ierr != 0) {            // Check for error
      goto ErrorExit;
    }

    // Prepare source data
    ierr = ve_Mem2Vec( &VecSrc, DV_HOST, (anypnt)pData, nElements, sizeof(int32), TY_INT32);
    if( ierr != 0) {            // Check for error
      goto ErrorExit;
    }

    // Prepare temp2 data
    ierr = ve_Mem2Vec( &Temp1, DV_HOST, (anypnt)pColRowSum_RGB->Temp2.pData, nElements, sizeof(int32), TY_INT32);
    if( ierr != 0) {            // Check for error
      goto ErrorExit;
    }

    // Difference filter

  #ifdef use_again
    nFilter = 3;
    filter[0] = -1; filter[1] = 0; filter[2] = 1;
    exp =  0;
  #else
    nFilter = 7;
    filter[0] = -1; filter[1] = -2; filter[2] = -1; filter[3] = 0; filter[4] = 1; filter[5] = 2; filter[6] = 1;
    exp = -2;
  #endif

    if( (ierr = vfiltn( &VecSrc, &Temp1, exp, 0, nFilter, filter)) != 0) {

      ierr = -4;
      goto ErrorExit;
    }

    // Draw difference filter

    pData = pColRowSum_RGB->Temp2.pData;

    fl_line_style( 0, LineWidth, MyLineDashes);   // Set line width

    fl_color( fl_color_average( ColProfile, FL_WHITE, 0.33));

    fl_begin_line();

    if( (Flags & YAIPS_CRSUM_FLAG_COLSUM) != 0) {        // Make a column sum profile

      LenghtThis = LengthFirst;

      x3 = x1;

      for( i = 0; i < nElements; i++) {

        y3 = y2 - ((pData[ i] * h + 128) / 256 + 128);

        if( LenghtSample <= 1) {

          x3 = x1 + i * (w - 1) / (nElements - 1);
          fl_vertex( x3, y3);

        } else {

          if( LenghtSample > 1) {         // Scaled up

            fl_vertex( x3, y3);
            if( x3 == x2) {
              break;
            }

            x3 += LenghtThis;

            if( x3 > x2) {
              x3 = x2;
              fl_vertex( x3, y3);
              break;
            }

            fl_vertex( x3, y3);

            x3 += 1;
            if( x3 > x2) {
              break;
            }

            LenghtThis = LenghtSample - 1;
          }
        }
      }

    } else {       // Make a row sum profile

      LenghtThis = LengthFirst;

      y3 = y1;

      for( i = 0; i < nElements; i++) {

        x3 = x1 + ((pData[ i] * w + 128) / 256 + 128);

        if( LenghtSample <= 1) {

          y3 = y1 + i * (h - 1) / (nElements - 1);
          fl_vertex( x3, y3);

        } else {

          if( LenghtSample > 1) {         // Scaled up

            fl_vertex( x3, y3);
            if( y3 == y2) {
              break;
            }

            y3 += LenghtThis;

            if( y3 > y2) {

              y3 = y2;
              fl_vertex( x3, y3);

              break;
            }

            fl_vertex( x3, y3);
            y3 += 1;
            if( y3 > y2) {
              break;
            }

            LenghtThis = LenghtSample - 1;
          }
        }
      }
    }

    fl_end_line();
  }

  ierr = 0;    // Return OK

ErrorExit:

  return( ierr);
}

/************************************************************************************
 * YaIPS_ColRowSum_Draw
 *
 * Draw a column / row sum profile.
 *
 * NOTE: Make a fl_push_clip() call before calling this function
 *
 */
void YaIPS_ColRowSum_Draw( Fl_YaIPS_ColRowSum_RGB_t *pColRowSum_RGB, // Pointer to RGB profiles
                          int x1, int y1,                            // Left upper drawing reference point
                          int w, int h,                              // With / height of drawing box
                          int LenghtSample, int LengthFirst)         // Length of all/first sample to draw
{
  int x2, x3, y2, y3, LineWidth;
  int DrawSmooth, DrawSlope;                 // Smooth profile and slope of profiles

  DrawSmooth = (pColRowSum_RGB->Flags & YAIPS_CRSUM_FLAG_DISP_SMOOTH) != 0;
  DrawSlope  = (pColRowSum_RGB->Flags & YAIPS_CRSUM_FLAG_DISP_SLOPE) != 0;

  // Draw RGB Profiles

  Fl_Color ColFrame = YaIPS_Color_GRA_FRAME;
  static char MyLineDashes[] = { 4, 4, 0};

  x2 = x1 + w - 1;
  y2 = y1 + h - 1;

  // Used line width in pixel for histograms, AOIs

  LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

  // Prepare frame drawing

  fl_color( ColFrame);
  fl_font( FL_HELVETICA, 14);

  // Dashed lines

  fl_line_style( FL_DOT, 0, MyLineDashes);

  if( (pColRowSum_RGB->Flags & YAIPS_CRSUM_FLAG_COLSUM) != 0) {        // Make a column sum profile

    y3 = y2 - 64;
    fl_line( x1, y3, x2, y3);

    y3 = y2 - 128;
    fl_line( x1, y3, x2, y3);

    y3 = y2 - 192;
    fl_line( x1, y3, x2, y3);

    fl_draw( "255", x1 + 4, y1 + 15);
    fl_draw( "0", x1 + 4, y2 - 4);

  } else {

    x3 = x1 + 64;
    fl_line( x3, y1, x3, y2);

    x3 =  x1 + 128;
    fl_line( x3, y1, x3, y2);

    x3 =  x1 + 192;
    fl_line( x3, y1, x3, y2);

    fl_draw( "255", x2 - 27, y1 + 15);
    fl_draw( "0", x1 + 4, y1 + 15);
  }

  fl_line_style( 0, LineWidth);   // Set line width

  // Frame around profile

  fl_color( ColFrame);

  fl_rect( x1, y1, w, h);

  if( pColRowSum_RGB->nProfiles <= 0) {            // Have nothing to draw

    goto ExitPoint;
  }

  if( pColRowSum_RGB->R.nElements <= 0) {          // Have no profile. Checking the first profile only is enough

    goto ExitPoint;
  }

  // Draw profiles

  if( pColRowSum_RGB->nProfiles >= 1) {

    YaIPS_ColRowSum_SingleDraw( pColRowSum_RGB->R.pData, pColRowSum_RGB->R.nElements, x1, y1, x2, y2, w, h, pColRowSum_RGB->Flags,
                                pColRowSum_RGB->R.ColProfile, LenghtSample, LengthFirst, LineWidth,
                                pColRowSum_RGB, DrawSmooth, DrawSlope);
  }

  if( pColRowSum_RGB->nProfiles>= 2) {

    YaIPS_ColRowSum_SingleDraw( pColRowSum_RGB->G.pData, pColRowSum_RGB->G.nElements, x1, y1, x2, y2, w, h, pColRowSum_RGB->Flags,
                                pColRowSum_RGB->G.ColProfile, LenghtSample, LengthFirst, LineWidth,
                                pColRowSum_RGB, DrawSmooth, DrawSlope);
  }

  if( pColRowSum_RGB->nProfiles >= 3) {

    YaIPS_ColRowSum_SingleDraw( pColRowSum_RGB->B.pData, pColRowSum_RGB->B.nElements, x1, y1, x2, y2, w, h, pColRowSum_RGB->Flags,
                                pColRowSum_RGB->B.ColProfile, LenghtSample, LengthFirst, LineWidth,
                                pColRowSum_RGB, DrawSmooth, DrawSlope);
  }

ExitPoint:

  fl_line_style( 0);   // Reset to default
}

/****************************** End Of File ******************************/
