/****************************************************************************

  YaIPS_RGB_Frame.cpp

  Fl_RGB_Image image processing.
  Generate frame around image

 10.04.2025 RR: First edition of this file.

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
* YaIPS_RGB_Frame
* Generate frame around window
*
* pImgD      pointer to YaIPS_RGB_ImgD_t image
* width      width of boundary = abs(width)
*            > 0:  extrapolate mode
*            < 0:  set mode
* value       value to set if "width" has negative sign else dummy
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Frame( YaIPS_RGB_ImgD_t *pImgD,  // source and destination image
                     int width,                // abs(width) = width of boundary
                     int value)                // value to set if width has negative sign else dummy
{
  int w, d, x, y;
  int xmin, ymin, jump, nByt;
  uchar *s18,  *s28,  *d18,  *d28;
  uchar v4[ 4];

  xmin = pImgD->xx;
  ymin = pImgD->yy;
  jump = pImgD->ld;
  nByt = pImgD->d;

  if( width > 0) {                            // extrapolate

    for( w = 0; w < width; w++) {

      s18 = RGB_pixad( 0, width - w, pImgD);
      s28 = RGB_pixad( 0, ymin - width - 1 + w, pImgD);
      d18 = s18-jump;
      d28 = s28+jump;

      for( x = 0; x < xmin; x++) {

        for( d = 0; d < nByt; d++) {

          *d18++ = *s18++;                     /* upper boundary */
          *d28++ = *s28++;                     /* lower boundary */
        }
      }

      d18 =  RGB_pixad( width - w - 1, 0, pImgD);    /* left boundary */
      d28 =  RGB_pixad( xmin - width + w, 0, pImgD); /* right boundary */

      for( y = 0; y < ymin; y++) {

        for( d = 0; d < nByt; d++) {
          *(d18 + d) = *(d18 + nByt + d);
          *(d28 + d) = *(d28 - nByt + d);
        }
        d18 += jump;
        d28 += jump;
      }
    }

    return(0);

  } else  {                                   // clear/set with value

    w = -width;

    v4[ 0] = value & 0xff;
    v4[ 1] = (value >>  8) & 0xff;
    v4[ 2] = (value >> 16) & 0xff;
    v4[ 3] = (value >> 24) & 0xff;

    while ( --w >= 0 ) {

      d18 = RGB_pixad( 0, w, pImgD);
      d28 = RGB_pixad( 0, ymin-w-1, pImgD);

      x = xmin;
      while ( --x >= 0 ) {

        for( d = 0; d < nByt; d++) {
          *(d18 + d) = v4[ d];
          *(d28 + d) = v4[ d];
        }

        d18 += nByt;
        d28 += nByt;
      }

      d18 =  RGB_pixad(w, 0, pImgD);
      d28 =  RGB_pixad(xmin-w-1, 0, pImgD);

      y = ymin;
      while ( --y >= 0 ) {

        for( d = 0; d < nByt; d++) {
          *(d18 + d) = v4[ d];
          *(d28 + d) = v4[ d];
        }
        d18 += jump;
        d28 += jump;
      }
    }

    return(0);
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Frame
* Generate frame around window
*
* pDst       pointer to Fl_RGB_Image image
* width      width of boundary = abs(width)
*            > 0:  extrapolate mode
*            < 0:  set mode
* value       value to set if "width" has negative sign else dummy
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Frame( Fl_RGB_Image *pDst,  // source and destination image
                     int width,           // abs(width) = width of boundary
                     int value)           // value to set if width has negative sign else dummy
{
  int ierr;
  YaIPS_RGB_ImgD_t ImgD;

  ierr = YaIPS_RGB_to_ImgD( pDst, &ImgD);

  if( ierr != 0)  {                           // Check for error

    return( ierr);
  }

  ierr = YaIPS_RGB_Frame( &ImgD, width, value);

  return( ierr);
}

/******************************** End Of File ********************************/

