/****************************************************************************

  YaIPS_RGB_FilterNxN.cpp

  Fl_RGB_Image image processing.
  3 x 3 filter processing

 11.04.2025 RR: First edition of this file.
 18.08.2025 RR: Allow in place filter (source and destination is the
                same image).

*****************************************************************************
*/

#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <math.h>

#include <FL/Fl.H>
#include <FL/Fl_Image.H>

// ...

#include "YaIPS_RGB_Interface.h"

/***************************************************************************
* Defines and structures
****************************************************************************
*/

#define YAIPS_RESULT_SCHIFT   24            // Use this shift factor to use integer multiply

// Used for the computing main loop
typedef struct {
  int Offset;               // Offset in memory
  int Coefficient;          // Coefficient
} T_YaIPS_Filter_compute;

/***************************************************************************
* YaIPS_RGB_FilterNxN_Def
* N*N filter with free configuration of (integer !) coefficients.
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* Flags        Some flags
* ResMultArg   Result multiplier
* Offset       Add this offset to the result
* KernelSize   3 .. 13, odd only
* nFilterDef   Number of filter definitions
* pFilterDef   Table of filter definitions.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_FilterNxN_Def( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                             Fl_RGB_Image *pSrc,   // Source image
                             int Flags,            // Some Flags
                             float ResMultArg,     // Result multiplier
                             int Offset,           // Add this offset to the result
                             int KernelSize,       // Kernel size
                             int nFilterDef,       // Number of filter definitions
                             T_YaIPS_FilterDef *pFilterDef)
{
  int ierr, d, x, y, nByt, KernelSize2;
  int xmin, ymin, jump, t, iFilter, nFilterCompute, InPlaceCalc;
  T_YaIPS_Filter_compute *pFilterCompute, FilterCompute[ YAIPS_FILTER_SIZE_MAX_2];
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s80, *d80, *s8, *d8;
  long long ResMultArgLong, ResMultArgLong2;

  // Check Parameter

  if( KernelSize < YAIPS_FILTER_SIZE_MIN ||       // Check kernel size
      KernelSize > YAIPS_FILTER_SIZE_MAX) {

    return( -10);
  }

  if( (KernelSize & 0x01) == 0) {               // Kernel is NOT odd

    return( -11);
  }

  if( nFilterDef < 1 ||                         // Check number of filter definitions
      nFilterDef > YAIPS_FILTER_SIZE_MAX_2) {

    return( -12);
  }

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Ensure that pPDst image has the same size and same pixel amount as pSrc
  ierr = YaIPS_RGB_EnsureSameSize( ppDst, pSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Check for in place calculation

  InPlaceCalc = pSrc == pDst;                // Check source and destination are the same

  // Get image data

  xmin = RGB_min( iDst.xx, iSrc.xx);
  ymin = RGB_min( iDst.yy, iSrc.yy);
  jump = iSrc.ld;
  nByt = iSrc.d;

  // Prepare ...

  KernelSize2 = KernelSize / 2;               // 1/2 kernel size. Is also border width

  xmin -= KernelSize2 + KernelSize2;          // Process only inner pixels
  ymin -= KernelSize2 + KernelSize2;

  pFilterCompute = FilterCompute;             // Convert to faster compute data
  nFilterCompute = 0;
  for( iFilter = 0; iFilter < nFilterDef; iFilter++) {

    if( pFilterDef[ iFilter].Coefficient == 0) {   // Skip this

      continue;
    }

    pFilterCompute->Offset = (pFilterDef[ iFilter].OffY + KernelSize2) * jump + (pFilterDef[ iFilter].OffX + KernelSize2) * nByt;
    pFilterCompute->Coefficient = pFilterDef[ iFilter].Coefficient;

    nFilterCompute += 1;
    pFilterCompute++;
  }

  // Multiply as integer

  // Convert to 64 bit integer
  ResMultArgLong = (long long)(ResMultArg * (1 << YAIPS_RESULT_SCHIFT));
  ResMultArgLong2 = ResMultArgLong / 2;

  // Filter ...

  for( y = 0; y < ymin; y++) {

    s80 = RGB_pixad( 0, y, &iSrc);

    if( InPlaceCalc) {                                         // In place calculation

      d80 = RGB_pixad( 0, 0, &iDst);                           // Start at left upper corner
    } else {

      d80 = RGB_pixad( KernelSize2, y + KernelSize2, &iDst);   // Start at filter center
    }

    for( d = 0; d < nByt; d++) {

      s8 = s80 + d;
      d8 = d80 + d;

      for( x = 0; x < xmin; x++) {

        t  = 0;                           // Sum up

        pFilterCompute = FilterCompute;             // Convert to faster compute data
        for( iFilter = 0; iFilter < nFilterCompute; iFilter++, pFilterCompute++) {

          t += pFilterCompute->Coefficient * s8[ pFilterCompute->Offset];
        }

        if( t >= 0) {

          t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
        } else {

          t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
        }

        if( (Flags & YAIPS_FILTER_FLAG_RES_ABS) != 0 &&  // Make a absolute result
            t < 0) {                                 // And have a negative value

          t = 0 - t;                                 // Make it positive
        }

        t += Offset;                                 // Add offset
        RGB_bclip( t, d8);                           // Clip and store

        s8 += nByt;
        d8 += nByt;
      }
    }
  }

  if( InPlaceCalc) {                                 // In place calculation

    // Copy filter result to proper place

    for( y = - 1; y >= 0; y--) {                     // Backward

      d8 =  RGB_pixad( 0, 0, &iDst);                           // Source: Start at left upper corner

      d80 = RGB_pixad( KernelSize2, y + KernelSize2, &iDst);   // Destination: Start at filter center

      memcpy( d80, d8, xmin * d);
    }
  }

  // Extrapolate frame
  ierr = YaIPS_RGB_Frame( pDst, KernelSize2, 0);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_FilterNxN_Vec
* N*N filter with free configuration of (integer !) coefficients.
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* Flags        Some flags
* ResMultArg   Result multiplier
* Offset       Add this offset to the result
* KernelSize   3 .. 13, odd only
* pKernel      Point to filter kernel (vector of KernelSize² integers)
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_FilterNxN_Vec( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                            Fl_RGB_Image *pSrc,   // Source image
                            int Flags,            // Some Flags
                            float ResMultArg,     // Result multiplier
                            int Offset,           // Add this offset to the result
                            int KernelSize,       // Kernel size
                            int *pKernel)         // Point to filter kernel (vector of KernelSize² integers)
{
  T_YaIPS_FilterDef *pFilterDef, FilterDef[ YAIPS_FILTER_SIZE_MAX_2];
  int ierr, dx, dy, nFilterDef, KernelSize2, SumCoef;
  double ResMult;

  // Check Parameter

  if( KernelSize < YAIPS_FILTER_SIZE_MIN ||       // Check kernel size
      KernelSize > YAIPS_FILTER_SIZE_MAX) {

    return( -10);
  }

  if( (KernelSize & 0x01) == 0) {               // Kernel is NOT odd

    return( -11);
  }

  // Convert vector to more dense filter definitions

  KernelSize2 = KernelSize / 2;               // 1/2 kernel size. Is also border width

  pFilterDef = FilterDef;                     // Convert to filter definition data
  nFilterDef = 0;

  SumCoef = 0;
  for( dy = - KernelSize2; dy <= KernelSize2; dy++) {

    for( dx = - KernelSize2; dx <= KernelSize2; dx++, pKernel++) {

      if( *pKernel == 0) {                   // Skip this

        continue;
      }

      pFilterDef->OffX = dx;
      pFilterDef->OffY = dy;
      pFilterDef->Coefficient = *pKernel;

      SumCoef += *pKernel;

      nFilterDef += 1;
      pFilterDef++;
    }
  }

  // compute the result multiplier by 1 / sum of coefficients
  if( (Flags & YAIPS_FILTER_FLAG_RES_MULT_AUTO) != 0) {

    ResMult = 1.0;       // Preset with sane value

    if( SumCoef != 0) {

      ResMult = 1.0 / (double)SumCoef;
    }
  } else {

    ResMult = ResMultArg;
  }

  ierr = YaIPS_RGB_FilterNxN_Def( ppDst, pSrc, Flags, ResMult, Offset,
                                 KernelSize, nFilterDef, FilterDef);

  return( ierr);
}

/**************************************************************************
* YaIPS_RGB_Filt_Gauss3x3
* 3*3 Gaussian lowpass filter
*
* ppDst      Pointer to pointer to RGB image
* pSrc       Source image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Filt_Gauss3x3( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                             Fl_RGB_Image *pSrc)      // Source image
{
  int ierr;
  static int Vector[ 3 * 3] = { 1, 2, 1,      // Gauss kernel
                                2, 4, 2,
                                1, 2, 1
  };

  ierr = YaIPS_RGB_FilterNxN_Vec( ppDst, pSrc, YAIPS_FILTER_FLAG_RES_MULT_AUTO,
                                  0, 0,         // ResMult + Offset
                                  3, Vector);   // Filter kernel

  return( ierr);                                // Return
}

/**************************************************************************
* YaIPS_RGB_Filt_GaussNxN
* N*N Gaussian lowpass filter
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* KernelSize   3 .. 13, odd only
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Filt_GaussNxN( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                             Fl_RGB_Image *pSrc,      // Source image
                             int KernelSize)          // Kernel size
{
  int ierr, dx, dy, KernelSize2, RadiusI;
  double Coeff;
  int *pKernel;
  int Vector[ YAIPS_FILTER_SIZE_MAX_2];            // Vector for max kernel
  static float RadiusMax = 6.0;                   // Max radius supported
  static float CoeffTab[ 8] = { 100.0, 94.4, 77.7, 61.1, 33.3, 22.2, 16.6, 0.0 };
  float Radius, Remainder;

  // Check Parameter

  if( KernelSize < YAIPS_FILTER_SIZE_MIN ||       // Check kernel size
      KernelSize > YAIPS_FILTER_SIZE_MAX) {

    return( -10);
  }

  if( (KernelSize & 0x01) == 0) {               // Kernel is NOT odd

    return( -11);
  }

  // Build the kernel

  KernelSize2 = KernelSize / 2;               // 1/2 kernel size. Is also border width

  // Evaluate and store operator values

  memset( Vector, 0, sizeof( Vector));        // Preset coefficients with zero

#ifdef use_again
#ifdef _DEBUG
  printf( "YaIPS_RGB_Filt_Gauss %2d x %2d\n", KernelSize, KernelSize);
  printf( "---------------------------\n");
#endif
#endif

  pKernel = Vector;
  for( dy = - KernelSize2; dy <= KernelSize2; dy++) {

    for( dx = - KernelSize2; dx <= KernelSize2; dx++, pKernel++) {

      Radius = sqrt( dx * dx + dy * dy);                   // Distance from kernel center

      Radius = (Radius * RadiusMax) / (float)KernelSize2;  // Normalize to max radius

      RadiusI = (int)Radius;                               // Rounded down

      if( RadiusI >= 7) {                                  // Outside ring

        Coeff = 0.0;

      } else {

        Remainder = Radius - RadiusI;

        // Weighted add of coefficients
        Coeff = CoeffTab[ RadiusI] * (1.0 - Remainder) + CoeffTab[ RadiusI + 1] * Remainder;
      }

      *pKernel = (int)(Coeff + 0.5);

#ifdef use_again
#ifdef _DEBUG
      printf( " %4d", *pKernel);
#endif
#endif
    }

#ifdef use_again
#ifdef _DEBUG
    printf( "\n");
#endif
#endif
  }

  // ...

  ierr = YaIPS_RGB_FilterNxN_Vec( ppDst, pSrc, YAIPS_FILTER_FLAG_RES_MULT_AUTO,
                                 0, 0,                  // ResMult + Offset
                                 KernelSize, Vector);   // Filter kernel

  return( ierr);                               // Return
}

/**************************************************************************
* YaIPS_RGB_Filt_Lowpass3x3
* 3*3 lowpass filter. 9 coefficients are used.
*
* ppDst      Pointer to pointer to RGB image
* pSrc       Source image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Filt_Lowpass3x3( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                               Fl_RGB_Image *pSrc)      // Source image
{
  int ierr;
  static int Vector[ 3 * 3] = { 1, 1, 1,      // Box kernel
                                1, 1, 1,
                                1, 1, 1
  };

  ierr = YaIPS_RGB_FilterNxN_Vec( ppDst, pSrc, YAIPS_FILTER_FLAG_RES_MULT_AUTO,
                                 0, 0,         // ResMult + Offset
                                 3, Vector);   // Filter kernel

  return( ierr);                               // Return
}

/**************************************************************************
* YaIPS_RGB_Filt_LowpassNxN
* N*N lowpass filter
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* KernelSize   3 .. 13, odd only
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Filt_LowpassNxN( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                               Fl_RGB_Image *pSrc,      // Source image
                               int KernelSize)          // Kernel size
{
  int ierr, dx, dy, KernelSize2;
  int *pKernel;
  int Vector[ YAIPS_FILTER_SIZE_MAX_2];            // Vector for max kernel

  // Check Parameter

  if( KernelSize < YAIPS_FILTER_SIZE_MIN ||       // Check kernel size
      KernelSize > YAIPS_FILTER_SIZE_MAX) {

    return( -10);
  }

  if( (KernelSize & 0x01) == 0) {               // Kernel is NOT odd

    return( -11);
  }

  // Build the kernel

  KernelSize2 = KernelSize / 2;               // 1/2 kernel size. Is also border width

  pKernel = Vector;
  for( dy = - KernelSize2; dy <= KernelSize2; dy++) {

    for( dx = - KernelSize2; dx <= KernelSize2; dx++, pKernel++) {

      *pKernel = 1;
    }
  }

  // ...

  ierr = YaIPS_RGB_FilterNxN_Vec( ppDst, pSrc, YAIPS_FILTER_FLAG_RES_MULT_AUTO,
                                 0, 0,                  // ResMult + Offset
                                 KernelSize, Vector);   // Filter kernel

  return( ierr);                               // Return
}

/**************************************************************************
* YaIPS_RGB_Filt_EdgeDiff
* Edge difference filter
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* Direction    Direction of filter
* ResMultArg   Result multiplier, default should be 1.0
* Offset       Add this offset to the result
* AbsRes       If true, make absolute result before offset addition.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Filt_EdgeDiff( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                             Fl_RGB_Image *pSrc,      // Source image
                             int Direction,           // Direction, see defines YAIPS_FILTER_EDGE_DIFF_xxx
                             float ResMultArg,        // Result multiplier
                             int Offset,              // Add this offset to the result
                             int AbsRes)              // If true, absolute result
{
  int ierr, KernelSize, Flags;
  int *pVector;
  // Horizontal difference
  static int VectorHor[ 3 * 3] = {  1,  2,  1,
                                     0,  0,  0,
                                    -1, -2, -1
  };
  // Vertical difference
  static int VectorVer[ 3 * 3] = {  1,  0, -1,
                                     2,  0, -2,
                                     1,  0, -1
  };
  // Diagonally difference 1
  static int VectorDiag1[ 5 * 5] = {  0,  0,  1,  0,  0,
                                      0,  2,  0,  0,  0,
                                      1,  0,  0,  0, -1,
                                      0,  0,  0, -2,  0,
                                      0,  0, -1,  0,  0,
  };
  // Diagonal difference 2
  static int VectorDiag2[ 5 * 5] = {  0,  0,  1,  0,  0,
                                      0,  0,  0,  2,  0,
                                     -1,  0,  0,  0,  1,
                                      0, -2,  0,  0,  0,
                                      0,  0, -1,  0,  0,
  };

  switch( Direction) {

  default:
  case YAIPS_FILTER_EDGE_DIFF_HOR:
    KernelSize = 3;
    pVector = VectorHor;
    break;

  case YAIPS_FILTER_EDGE_DIFF_VER:
    KernelSize = 3;
    pVector = VectorVer;
    break;

  case YAIPS_FILTER_EDGE_DIFF_DIAG1:
    KernelSize = 5;
    pVector = VectorDiag1;
    break;

  case YAIPS_FILTER_EDGE_DIFF_DIAG2:
    KernelSize = 5;
    pVector = VectorDiag2;
    break;
  }

  // Flag for absolute mode
  if( AbsRes) {
    Flags = YAIPS_FILTER_FLAG_RES_ABS;
  } else {
    Flags = 0;
  }

  // Filter ...
  ierr = YaIPS_RGB_FilterNxN_Vec( ppDst, pSrc, Flags,
                                 ResMultArg * 0.25, Offset,           // ResMult + Offset
                                 KernelSize, pVector);   // Filter kernel

  return( ierr);                               // Return
}

/**************************************************************************
* YaIPS_RGB_Laplace3x3
* 3*3 Laplacian highpass filter
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* ResMultArg   Result multiplier, default should be 1.0
* Offset       Add this offset to the result
* AbsRes       If true, make absolute result before offset addition.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Laplace3x3( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                          Fl_RGB_Image *pSrc,      // Source image
                          float ResMultArg,        // Result multiplier
                          int Offset,              // Add this offset to the result
                          int AbsRes)              // If true, absolute result

{
  int ierr, Flags;
  static int Vector[ 3 * 3] = { 0,-1, 0,         // Laplace kernel
                               -1, 4,-1,
                                0,-1, 0
  };

  // Flag for absolute mode
  if( AbsRes) {
    Flags = YAIPS_FILTER_FLAG_RES_ABS;
  } else {
    Flags = 0;
  }

  // Filter ...
  ierr = YaIPS_RGB_FilterNxN_Vec( ppDst, pSrc, Flags,
                                 ResMultArg, Offset,  // ResMult + Offset
                                 3, Vector);   // Filter kernel

  return( ierr);                               // Return
}

/******************************** End Of File ********************************/

