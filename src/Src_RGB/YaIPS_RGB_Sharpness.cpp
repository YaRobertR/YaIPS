/****************************************************************************

  YaIPS_RGB_Sharpness.cpp

  Fl_RGB_Image image processing.
  Image sharpness filter

 18.08.2025 RR: First edition of this file.

*****************************************************************************
*/

#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <FL/Fl.H>
#include <FL/Fl_Image.H>

// ...

#include "YaIPS_RGB_Interface.h"

/***************************************************************************
* Defines and structures
****************************************************************************
*/

/***************************************************************************
* YaIPS_RGB_Sharpness
* Image sharpness filter, uses an intermediate image.
*
* ppDst           Pointer to pointer to RGB image
* pSrc            Source image
* Radius          The number of pixels that are taken into account around each edge.
* StrengthPercent The amount by which the contrast of the pixels in the image is increased [%].
* Difference      The brightness difference between neighboring pixels so that they are adjusted.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Sharpness( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                         Fl_RGB_Image *pSrcIn, // Source image
                         int Radius,           // The number of pixels that are taken into account around each edge.
                         int StrengthPercent,  // The amount by which the contrast of the pixels in the image is increased [%].
                         int Difference)       // The brightness difference between neighboring pixels so that they are adjusted.
{
  Fl_RGB_Image *pDst, *pSrc, *pTmp;
  YaIPS_RGB_ImgD_t iDst, iSrc, iTmp;
  int ierr, x, y, xx, yy, d, nByte, jump, KernelSize, t1, StrengthFac, Center, BorderBits;
  int Off00, Off01, Off02, Off10, Off11, Off12, Off20, Off21, Off22;
  uchar *d80, *s80, *t80, *d8, *s8, *t8;

  pTmp = NULL;

  // Check parameter

  if( Radius < 1 || Radius > 15) {                     // Check range

    ierr = -200;
    return( ierr);
  }

  if( StrengthPercent < 0 || StrengthPercent > 1000) {  // Check range

    ierr = -201;
    return( ierr);
  }

  if( Difference < 0 || Difference > 256) {            // Check range

    ierr = -202;
    return( ierr);
  }

  // Ensure that pPDst image has the same size and same pixel amount as pSrc
  ierr = YaIPS_RGB_EnsureSameSize( ppDst, pSrcIn);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image
  pSrc = pSrcIn;                              // Get pointer to source image

  // Gauss filter the image

  KernelSize = Radius * 2 + 1;

  // Check kernel size
  KernelSize |= 1;                                  // Must be odd
  if( KernelSize < 3) {                             // Ensure minimum value
    KernelSize = 3;
  }

  ierr = YaIPS_RGB_GaussXY( &pTmp, pSrc, KernelSize, YAIPS_GAUSXY_MODE_XY);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Image processing.
  // All images have the same size.

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_to_ImgD( pTmp, &iTmp);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data

  xx    = YaIPS_RGB_xx3min( &iDst, &iSrc, &iTmp);
  yy    = YaIPS_RGB_yy3min( &iDst, &iSrc, &iTmp);
  jump  = iSrc.ld;
  nByte = iSrc.d;

  // Prepare ...

  StrengthFac = (StrengthPercent * 1024) / 100;    // Prepare for a divide by 1024

  Off10 = - nByte;
  Off11 = 0;
  Off12 = nByte;
  Off00 = Off10 - jump;
  Off01 = Off11 - jump;
  Off02 = Off12 - jump;
  Off20 = Off10 + jump;
  Off21 = Off11 + jump;
  Off22 = Off12 + jump;

  // Filter ...

  for( y = 0; y < yy; y++) {

    d80 = RGB_pixad( 0,  y, &iDst);
    s80 = RGB_pixad( 0,  y, &iSrc);
    t80 = RGB_pixad( 0,  y, &iTmp);

    BorderBits = 0;
    if( y <= 0) {
      BorderBits |= 0x10;
    }
    if( y >= yy - 1) {
      BorderBits |= 0x20;
    }

    for( x = 0; x < xx; x++, d80 += nByte, s80 += nByte, t80 += nByte) {

      BorderBits &= 0xf0;
      if( x <= 0) {
        BorderBits |= 0x01;
      }
      if( x >= xx - 1) {
        BorderBits |= 0x02;
      }

      s8 = s80;

      for( d = 0; d < nByte; d++, s8++) {

        Center = s8[ Off11];

        if( (BorderBits & 0x11) == 0) {

          t1 = s8[ Off00] - Center;
          if( t1 < 0) t1 = - t1;
          if( t1 >= Difference) goto sharpen;
        }

        if( (BorderBits & 0x10) == 0) {

          t1 = s8[ Off01] - Center;
          if( t1 < 0) t1 = - t1;
          if( t1 >= Difference) goto sharpen;
        }

        if( (BorderBits & 0x12) == 0) {

          t1 = s8[ Off02] - Center;
          if( t1 < 0) t1 = - t1;
          if( t1 >= Difference) goto sharpen;
        }

        if( (BorderBits & 0x01) == 0) {

          t1 = s8[ Off10] - Center;
          if( t1 < 0) t1 = - t1;
          if( t1 >= Difference) goto sharpen;
        }

        if( (BorderBits & 0x02) == 0) {

          t1 = s8[ Off12] - Center;
          if( t1 < 0) t1 = - t1;
          if( t1 >= Difference) goto sharpen;
        }

        if( (BorderBits & 0x20) == 0) {

          t1 = s8[ Off20] - Center;
          if( t1 < 0) t1 = - t1;
          if( t1 >= Difference) goto sharpen;
        }

        if( (BorderBits & 0x21) == 0) {

          t1 = s8[ Off21] - Center;
          if( t1 < 0) t1 = - t1;
          if( t1 >= Difference) goto sharpen;
        }

        if( (BorderBits & 0x22) == 0) {

          t1 = s8[ Off22] - Center;
          if( t1 < 0) t1 = - t1;
          if( t1 >= Difference) goto sharpen;
        }
      }

      // No sharpen

      d8 = d80;
      s8 = s80;

      for( d = 0; d < nByte; d++, d8++, s8++) {

        d8[ Off11] = s8[ Off11];
      }

      continue;

sharpen:
      // Sharpen the image

      d8 = d80;
      s8 = s80;
      t8 = t80;

      for( d = 0; d < nByte; d++, d8++, s8++, t8++) {

        t1 = t8[ Off11] - s8[ Off11];

        t1 = (int)s8[ Off11] - ((t1 * StrengthFac) >> 10);

        if( t1 < 0) {
          t1 = 0;
        }

        if( t1 > 255) {
          t1 = 255;
        }

        d8[ Off11] = t1;
      }
    }
  }

  // ...

  if( pTmp != NULL) {              // Used temporary image

    pTmp->release();               // Release image data
  }

  return( 0);                                 // Return OK
}

/******************************** End Of File ********************************/
