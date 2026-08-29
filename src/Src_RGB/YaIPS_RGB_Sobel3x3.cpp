/****************************************************************************

  YaIPS_RGB_Sobel3x3.cpp

  Fl_RGB_Image image processing.
  3 x 3 sobel filter

 11.04.2025 RR: First edition of this file.

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

#define YAIPS_RESULT_SCHIFT   16            // Use this shift factor to use integer multiply

/***************************************************************************
* YaIPS_RGB_Sobel3x3
* 3*3 sobel filter
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* ResMultArg   Result multiplier, default should be 1.0
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Sobel3x3( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                        Fl_RGB_Image *pSrc,   // Source image
                        float ResMultArg)     // Result multiplier
{
  int ierr, d, x, y, nByt, KernelSize, KernelSize2;
  int xmin, ymin, jump, t1, t2;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  int Off00, Off01, Off02, Off10, Off11, Off12, Off20, Off21, Off22;
  uchar *s80, *d80, *s8, *d8;
#ifdef YAIPS_RESULT_SCHIFT // Use integer multiply
  long ResMultArgLong, ResMultArgLong2;
#endif

  KernelSize = 3;

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

  // Get image data

  xmin = RGB_min( iDst.xx, iSrc.xx);
  ymin = RGB_min( iDst.yy, iSrc.yy);
  jump = iSrc.ld;
  nByt = iSrc.d;

  // Prepare ...

  KernelSize2 = KernelSize / 2;               // 1/2 kernel size. Is also border width

  xmin -= KernelSize2 + KernelSize2;          // Process only inner pixels
  ymin -= KernelSize2 + KernelSize2;

  Off00 = 0;
  Off01 = nByt;
  Off02 = nByt + nByt;
  Off10 = Off00 + jump;
  Off11 = Off01 + jump;
  Off12 = Off02 + jump;
  Off20 = Off10 + jump;
  Off21 = Off11 + jump;
  Off22 = Off12 + jump;

  // Correct result multiplier

  ResMultArg *= 0.125;

  // Multiply as integer

#ifdef YAIPS_RESULT_SCHIFT // Use integer multiply
  // Convert to 64 bit integer
  ResMultArgLong = (long)(ResMultArg * (1 << YAIPS_RESULT_SCHIFT));
  ResMultArgLong2 = ResMultArgLong / 2;
#endif
  // Filter ...

  for( y = 0; y < ymin; y++) {

    s80 = RGB_pixad(           0,  y             , &iSrc);
    d80 = RGB_pixad( KernelSize2, y + KernelSize2, &iDst);

    for( d = 0; d < nByt; d++) {

      s8 = s80 + d;
      d8 = d80 + d;

      for( x = 0; x < xmin; x++) {

        t1 = s8[ Off00] + s8[ Off02] - s8[ Off20] - s8[ Off22] + ((s8[ Off01] - s8[ Off21]) << 2);

        t2 = s8[ Off00] + s8[ Off20] - s8[ Off02] - s8[ Off22] + ((s8[ Off10] - s8[ Off12]) << 2);

        if( t1 < 0) {
          t1 = - t1;
        }
        if( t2 < 0) {
          t2 = - t2;
        }

        t1 += t2;

#ifdef YAIPS_RESULT_SCHIFT // Use integer multiply
        if( t1 >= 0) {

          t1 = (int)((t1 * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
        } else {

          t1 = (int)((t1 * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
        }
#else
        if( t >= 0) {

          t1 = (int) (t1 * ResMultArg + 0.5);
        } else {

          t1 = (int) (t1 * ResMultArg - 0.5);
        }
#endif

        RGB_bclip( t1, d8);                      // Clip and store

        s8 += nByt;
        d8 += nByt;
      }
    }
  }

  // Extrapolate frame
  ierr = YaIPS_RGB_Frame( pDst, KernelSize2, 0);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  return( 0);                                 // Return OK
}

/******************************** End Of File ********************************/

