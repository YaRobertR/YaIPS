/****************************************************************************

  YaIPS_RGBContour.cpp

  Fl_RGB_Image image processing.
  Contour filter. Separates contours from plane

 01.08.2025 RR: First edition of this file.

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

/***************************************************************************
* YaIPS_RGB_Contour
* Contour filter.
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* PlaneThres   Plane threshold. (|one neighbor - central| > plane threshold) -> contour
* ContourOp    Type of operation. See YAIPS_RGB_CONTOUR_XXX defines.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Contour( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                       Fl_RGB_Image *pSrc,   // Source image
                       int PlaneThres,       // Plane threshold
                       int ContourOp)        // Type of operation
{
  int ierr, x, y, nBytesSrc, nBytesDst, iColor, nColors, SrcHasAlpha, DstHasAlpha;
  int xx, yy, jumpSrc, t1, OutVal, Center;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  int Off00, Off01, Off02, Off10, Off11, Off12, Off20, Off21, Off22;
  uchar *s80, *d80, *s8, *d8;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data

  xx        = iSrc.xx;
  yy        = iSrc.yy;
  jumpSrc   = iSrc.ld;
  nBytesSrc = iSrc.d;

  nColors  = iSrc.d >= 3 ? 3 : 1;

  SrcHasAlpha = iSrc.d == 2 || iSrc.d == 4;

  // Destination has alpha if source has alpha or operator sets alpha
  DstHasAlpha = SrcHasAlpha || ContourOp == YAIPS_RGB_CONTOUR_LP_ALPHA;

  nBytesDst = nColors + DstHasAlpha;

  // Ensure that pPDst image has the same size and optional alpha mask
  ierr = YaIPS_RGB_ImageSetSize( ppDst, xx, yy, nBytesDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Prepare ...

  Off00 = 0;
  Off01 = nBytesSrc;
  Off02 = nBytesSrc + nBytesSrc;
  Off10 = Off00 + jumpSrc;
  Off11 = Off01 + jumpSrc;
  Off12 = Off02 + jumpSrc;
  Off20 = Off10 + jumpSrc;
  Off21 = Off11 + jumpSrc;
  Off22 = Off12 + jumpSrc;

  // Filter ...

  for( y = 1; y < yy - 1; y++) {

    s80 = RGB_pixad( 0, y - 1, &iSrc);
    d80 = RGB_pixad( 1,     y, &iDst);

    for( x = 1; x < xx - 1; x++, s80 += nBytesSrc, d80 += nBytesDst) {

      OutVal = 0;                         // Preset plane

      s8 = s80;

      for( iColor = 0; iColor < nColors; iColor++, s8++) {

        Center = s8[ Off11];

        t1 = s8[ Off00] - Center;
        if( t1 < 0) t1 = - t1;
        if( t1 > PlaneThres) goto contour;

        t1 = s8[ Off01] - Center;
        if( t1 < 0) t1 = - t1;
        if( t1 > PlaneThres) goto contour;

        t1 = s8[ Off02] - Center;
        if( t1 < 0) t1 = - t1;
        if( t1 > PlaneThres) goto contour;

        t1 = s8[ Off10] - Center;
        if( t1 < 0) t1 = - t1;
        if( t1 > PlaneThres) goto contour;

        t1 = s8[ Off12] - Center;
        if( t1 < 0) t1 = - t1;
        if( t1 > PlaneThres) goto contour;

        t1 = s8[ Off20] - Center;
        if( t1 < 0) t1 = - t1;
        if( t1 > PlaneThres) goto contour;

        t1 = s8[ Off21] - Center;
        if( t1 < 0) t1 = - t1;
        if( t1 > PlaneThres) goto contour;

        t1 = s8[ Off22] - Center;
        if( t1 < 0) t1 = - t1;
        if( t1 > PlaneThres) goto contour;
      }

      // Plane

      goto plane;

contour:               // Got contour

      OutVal = 255;

plane:                // Got contour

      switch( ContourOp) {
      case YAIPS_RGB_CONTOUR_CONTOUR:   // Set 255 for contour, 0 for plane

        d8 = d80;
        for( iColor = 0; iColor < nColors; iColor++, d8++) {

          *d8 = OutVal;
        }

        if( SrcHasAlpha) {              // If source has alpha destination has alpha too

          *d8 = s80[ nColors];          // Copy alpha
        }

       break;

       case YAIPS_RGB_CONTOUR_PLANE:    // Set 0 for contour, 255 for plane

         OutVal = 255 - OutVal;         // Negate

         d8 = d80;
         for( iColor = 0; iColor < nColors; iColor++, d8++) {

           *d8 = OutVal;
         }

         if( SrcHasAlpha) {             // If source has alpha destination has alpha too

           *d8 = s80[ nColors];         // Copy alpha
         }

         break;

       case YAIPS_RGB_CONTOUR_LOWPASS:  // Lowpass for plane pixels
       case YAIPS_RGB_CONTOUR_LP_ALPHA: // Lowpass for plane pixels and set edge bit in alpha mask

         if( OutVal < 128)  {           // Is plane

           // average neighbors
           d8 = d80;
           s8 = s80;
           for( iColor = 0; iColor < nColors; iColor++, s8++, d8++) {

             t1 = (s8[ Off00] + s8[ Off01] + s8[ Off02] + s8[ Off10] + s8[ Off11] + s8[ Off12] + s8[ Off20] + s8[ Off21] + s8[ Off22] + 4) / 9;

             RGB_bclip( t1, d8);                      // Clip and store
           }

         } else {                       // Is contour

           // Copy center pixel
           d8 = d80;
           s8 = s80;
           for( iColor = 0; iColor < nColors; iColor++, s8++, d8++) {

             *d8 = s8[ Off11];
           }
         }

         if( ContourOp == YAIPS_RGB_CONTOUR_LP_ALPHA) {    // Destination has alpha

           if( SrcHasAlpha) {                    // Source has alpha

             t1 = *s8;                           // Get source alpha

           } else {                              // Source has no alpha

             t1 = 0;                             // Use 0
           }

           if( OutVal < 128)  {                  // Is plane

             *d8 = t1 & ~YAIPS_REFMASK_CONTOUR;  // Reset contour bit
           } else {                              // Is contour

             *d8 = t1 | YAIPS_REFMASK_CONTOUR;   // Set contour bit
           }

         } else if( SrcHasAlpha) {               // If source has alpha destination has alpha too

           *d8 = *s8;                            // Copy alpha
         }

         break;
       }  // End case

    }   // End for( x = 1 ...
  }     // End for( y = 1 ...

  // Extrapolate frame
  ierr = YaIPS_RGB_Frame( pDst, 1, 0);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  return( 0);                                 // Return OK
}

/******************************** End Of File ********************************/

