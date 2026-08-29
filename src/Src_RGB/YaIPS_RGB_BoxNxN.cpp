/****************************************************************************

  YaIPS_RGB_BoxNxN.cpp

  Fl_RGB_Image image processing.
  1xN, Nx1, NxN box filter

 28.04.2025 RR: First edition of this file.

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
* YaIPS_RGB_BoxNx1
* 1*N box filter, horizontal box lowpass filter
*
* piDst        Pointer to pointer to RGB image
* piSrc        Source image
*
* NOTE: Inplace calculation is possible.
* NOTE: Source and destination have same sizes.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

static int YaIPS_RGB_BoxNx1( YaIPS_RGB_ImgD_t *piDst, // Destination image
                            YaIPS_RGB_ImgD_t *piSrc, // Source image
                            int KernelSize)         // Kernel Size
{
  int x, y, iByte, nByte, KernelSize2;
  int xx, yy, xxLine, sum, TPsiz;
  uchar *s8b, *s8a, *d8, *d80, *s80;
  uchar *pLine = NULL;

  // Get image data

  xx    = RGB_min( piDst->xx, piSrc->xx);
  yy    = RGB_min( piDst->yy, piSrc->yy);
  nByte = piSrc->d;

  xxLine = xx;

  KernelSize |= 1;                            // Ensure odd kernel size
  KernelSize2 = KernelSize / 2;               // 1/2 kernel size. Is also border width

  if (xx < KernelSize) {                      // Security test

    if( errstring == NULL) {                  // No error until now

      sprintf( errbuffer, LangStringLookup( "&RGB_Filter_Box1=Width %d >= filter %d"), xx, KernelSize);

      errstring = errbuffer;
    }

    return(-4);
  }

  // Allocate one line of data

  pLine = (uchar *)malloc( xx);

  if( pLine == NULL) {                        // Error

    return(-5);
  }

  // ...

  xx -= 2 * KernelSize2 + 1;
  TPsiz = 2 * KernelSize2 + 1;

  // Filter ...

  for( y = 0; y < yy; y++) {

    for( iByte = 0; iByte < nByte; iByte++) {

      s80 = RGB_pixad( 0, y, piSrc);            // get destination pointer
      d80 = RGB_pixad( 0, y, piDst);            // get source pointer

      s80 += iByte;
      d80 += iByte;

      // Fill line
      s8a = s80;
      s8b  = pLine;
      for (x = 0; x < xxLine; x++) {
        *s8b++ = *s8a;
        s8a += nByte;
      }

      d8  = d80;
      s8a = pLine;
      s8b = pLine;

      sum = 0;

      // start value
      for (x = 0; x < KernelSize2 + 1; x++) {
        sum += RGB_btoi(*s8b++);
      }
      *d8 = sum / (KernelSize2 + 1);
      d8 += nByte;

      // Run-in
      for (x = 0; x < KernelSize2; x++) {
        sum += RGB_btoi(*s8b++);
        *d8 = sum / (KernelSize2 + 2 + x);
        d8 += nByte;
      }

      // Run-through
      for (x = 0; x < xx; x++) {
        sum += RGB_btoi(*s8b++);
        sum -= RGB_btoi(*s8a++);
        *d8 = sum / TPsiz;
        d8 += nByte;
      }

      // run-out
      for (x = 1; x <= KernelSize2; x++) {
        sum -= RGB_btoi(*s8a++);
        *d8 = sum / (TPsiz - x);
        d8 += nByte;
      }
    }
  }

  free( pLine);

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Box1xN
* N*1 box filter, vertical box lowpass filter
*
* piDst        Pointer to pointer to RGB image
* piSrc        Source image
*
* NOTE: Inplace calculation is possible.
* NOTE: Source and destination have same sizes.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

static int YaIPS_RGB_Box1xN( YaIPS_RGB_ImgD_t *piDst, // Destination image
                            YaIPS_RGB_ImgD_t *piSrc, // Source image
                            int KernelSize)         // Kernel Size
{
  int x, y, iByte, nByte, KernelSize2;
  int xx, yy, yyColumn, sjump, djump, sum, TPsiz;
  uchar *s8b, *s8a, *d8, *d80, *s80;
  uchar *pColumn = NULL;

  // Get image data

  xx    = RGB_min( piDst->xx, piSrc->xx);
  yy    = RGB_min( piDst->yy, piSrc->yy);
  sjump = piSrc->ld;
  djump = piDst->ld;
  nByte = piSrc->d;

  yyColumn = yy;

  KernelSize |= 1;                            // Ensure odd kernel size
  KernelSize2 = KernelSize / 2;               // 1/2 kernel size. Is also border width

  if (yy < KernelSize) {                      // Security test

    if( errstring == NULL) {                  // No error until now

      sprintf( errbuffer, LangStringLookup( "&RGB_Filter_Box2=Height %d >= filter %d"), yy, KernelSize);

      errstring = errbuffer;
    }

    return(-4);
  }
  // Allocate one column of data

  pColumn = (uchar *)malloc( yy);

  if( pColumn == NULL) {                      // Error

    return(-5);
  }

  // ...

  yy -= 2 * KernelSize2 + 1;
  TPsiz = 2 * KernelSize2 + 1;

  // Filter ...

  for( x = 0; x < xx; x++) {

    for( iByte = 0; iByte < nByte; iByte++) {

      s80 = RGB_pixad( x, 0, piSrc);            // get destination pointer
      d80 = RGB_pixad( x, 0, piDst);            // get source pointer

      s80 += iByte;
      d80 += iByte;

      // Fill column
      s8a = s80;
      s8b  = pColumn;
      for (y = 0; y < yyColumn; y++) {
        *s8b++ = *s8a;
        s8a += sjump;
      }

      d8  = d80;
      s8a = pColumn;
      s8b = pColumn;

      sum = 0;

      // start value
      for (y = 0; y < KernelSize2 + 1; y++) {
        sum += RGB_btoi(*s8b++);
      }
      *d8 = sum / (KernelSize2 + 1);
      d8  += djump;

      // Run-in
      for (y = 0; y < KernelSize2; y++) {
        sum += RGB_btoi(*s8b++);
        *d8 = sum / (KernelSize2 + 2 + y);
        d8  += djump;
      }

      // Run-through
      for (y = 0; y < yy; y++) {
        sum += RGB_btoi(*s8b++);
        sum -= RGB_btoi(*s8a++);
       *d8 = sum / TPsiz;
       d8  += djump;
      }

      // run-out
      for (y = 1; y <= KernelSize2; y++) {
        sum -= RGB_btoi(*s8a++);
        *d8 = sum / (TPsiz - y);
        d8  += djump;
      }
    }
  }

  free( pColumn);

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Box1xN
* N*N box filter, box lowpass filter
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* KernelSizeX  Horizontal kernel size, must be odd and >= 3
* KernelSizeY  Vertical kernel size, must be odd and >= 3
*
* NOTE: Inplace calculation is possible.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_BoxNxN( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                     Fl_RGB_Image *pSrc,   // Source image
                     int KernelSizeX,      // Kernel Size
                     int KernelSizeY)      // Kernel Size
{
  int ierr;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc, *piSrc2;

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

  piSrc2 = &iSrc;                            // Preset source image

  if( KernelSizeX > 1) {

    ierr = YaIPS_RGB_BoxNx1( &iDst, &iSrc, KernelSizeX);

    if( ierr != 0) {       // Error ?

      return( ierr);
    }

    // Filter above is processed. New source is the desiontation image

    piSrc2 = &iDst;                          // Set destination image as next source image
  }

  if( KernelSizeY > 1) {

    ierr = YaIPS_RGB_Box1xN( &iDst, piSrc2, KernelSizeY);

    if( ierr != 0) {       // Error ?

      return( ierr);
    }
  }

  return( 0);                                 // Return OK
}

/******************************** End Of File ********************************/

