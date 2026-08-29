/****************************************************************************

  YaIPS_RGB_Morphology.cpp

  Fl_RGB_Image image processing.
  3 x 3 morphology filter

 03.06.2025 RR: First edition of this file.

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
* YaIPS_RGB_Morphology
* 3*3 morphology filter, NO opening and closing
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* MorphOp      Morphology operator
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Morphology( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                          Fl_RGB_Image *pSrc,   // Source image
                          int MorphOp)          // Morphology operator
{
  int ierr, d, x, y, nByt, KernelSize, KernelSize2;
  int xmin, ymin, jump, t1, t2, t3;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  int Off00, Off01, Off02, Off10, Off11, Off12, Off20, Off21, Off22;
  uchar *s80, *d80, *s8, *d8;

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

  // Filter ...

  for( y = 0; y < ymin; y++) {

    s80 = RGB_pixad(           0,  y             , &iSrc);
    d80 = RGB_pixad( KernelSize2, y + KernelSize2, &iDst);

    for( d = 0; d < nByt; d++) {

      s8 = s80 + d;
      d8 = d80 + d;

      switch( MorphOp) {
      default:

        // Operator not supported

        if( errstring == NULL) {                  // No error until now

          sprintf( errbuffer, LangStringLookup( "&RGB_Morph1=Filter is not supported"));

          errstring = errbuffer;
        }

        return(-99);


        break;

      case YAIPS_RGB_MORPH_EROSION:

        for( x = 0; x < xmin; x++) {

          t1 = s8[ Off00];
          t2 = s8[ Off01]; if( t2 < t1) t1 = t2;
          t2 = s8[ Off02]; if( t2 < t1) t1 = t2;
          t2 = s8[ Off10]; if( t2 < t1) t1 = t2;
          t2 = s8[ Off11]; if( t2 < t1) t1 = t2;
          t2 = s8[ Off12]; if( t2 < t1) t1 = t2;
          t2 = s8[ Off20]; if( t2 < t1) t1 = t2;
          t2 = s8[ Off21]; if( t2 < t1) t1 = t2;
          t2 = s8[ Off22]; if( t2 < t1) t1 = t2;

          *d8 = t1;                 // Store

          s8 += nByt;
          d8 += nByt;
        }
        break;

      case YAIPS_RGB_MORPH_DILATION:

        for( x = 0; x < xmin; x++) {

          t1 = s8[ Off00];
          t2 = s8[ Off01]; if( t2 > t1) t1 = t2;
          t2 = s8[ Off02]; if( t2 > t1) t1 = t2;
          t2 = s8[ Off10]; if( t2 > t1) t1 = t2;
          t2 = s8[ Off11]; if( t2 > t1) t1 = t2;
          t2 = s8[ Off12]; if( t2 > t1) t1 = t2;
          t2 = s8[ Off20]; if( t2 > t1) t1 = t2;
          t2 = s8[ Off21]; if( t2 > t1) t1 = t2;
          t2 = s8[ Off22]; if( t2 > t1) t1 = t2;

          *d8 = t1;                 // Store

          s8 += nByt;
          d8 += nByt;
        }
        break;

      case YAIPS_RGB_MORPH_MEDIAN:

        {
          int v[ 9];
          int i, iBot, iTop, Exchange;

          for( x = 0; x < xmin; x++) {

            // Get the 9 values

            v[ 0] = s8[ Off00];
            v[ 1] = s8[ Off01];
            v[ 2] = s8[ Off02];
            v[ 3] = s8[ Off10];
            v[ 4] = s8[ Off11];
            v[ 5] = s8[ Off12];
            v[ 6] = s8[ Off20];
            v[ 7] = s8[ Off21];
            v[ 8] = s8[ Off22];

            // Sort

            iBot = 0;
            iTop = 8;

            for( ; ; ) {

              Exchange = false;

              // Bubble up
              for( i = iBot; i < iTop; i++) {

                if( v[ i] > v[ i + 1]) {

                  Exchange = true;

                  t1 =  v[ i + 1];
                  v[ i + 1] =  v[ i];
                  v[ i] = t1;
                }
              }

              if( ! Exchange) {

                break;
              }

              iTop -= 1;   // For sure, top element is the maximum

              // Bubble down
              for( i = iTop - 1; i >= iBot; i--) {

                if( v[ i] > v[ i + 1]) {

                  Exchange = true;

                  t1 =  v[ i + 1];
                  v[ i + 1] =  v[ i];
                  v[ i] = t1;
                }
              }

              if( ! Exchange) {

                break;
              }

              iBot += 1;  // For sure, bottom element is the minimum
            }

            // Store

            t1 =  v[ 4];              // Get the median value

            *d8 = t1;                 // Store

            s8 += nByt;
            d8 += nByt;
          }
        }
        break;

      case YAIPS_RGB_MORPH_GRADIENT:

        for( x = 0; x < xmin; x++) {

          t1 = s8[ Off00];
          t3 = t1;
          t2 = s8[ Off01]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off02]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off10]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off11]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off12]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off20]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off21]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off22]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;

          *d8 = t3 - t1;      // Store

          s8 += nByt;
          d8 += nByt;
        }
        break;

      }  // End switch()
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
* YaIPS_RGB_Morphology
* 3*3 morphology filter, uses an intermediate image
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* MorphOp      Morphology operator
* MoprhRuns    Number of runs. If < 1 no filtering, the source is copied to the destination.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Morphology( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                          Fl_RGB_Image *pSrcIn, // In: Source image
                          int MorphOp,          // Morphology operator
                          int MoprhRuns)        // Number of runs
{
  int ierr, iRun;
  Fl_RGB_Image *pDst, *pSrc, *pTmp, *pExchange;

  pTmp = NULL;

  // Ensure that pPDst image has the same size and same pixel amount as pSrc
  ierr = YaIPS_RGB_EnsureSameSize( ppDst, pSrcIn);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  if( MoprhRuns <= 0) {                       // No filter run

    ierr = YaIPS_RGB_CopyImg( ppDst, pSrcIn);

    return( 0);
  }

  pDst = *ppDst;                              // Get pointer to destination image
  pSrc = pSrcIn;                              // Get pointer to source image

  switch( MorphOp) {
  case YAIPS_RGB_MORPH_CLOSING:

    // dilation
    for( iRun = 0; iRun < MoprhRuns; iRun++) {

      if( pDst == pSrcIn) {            // Need temporary image

        // Ensure that temporary image has the same size and same pixel amount as pSrc
        ierr = YaIPS_RGB_EnsureSameSize( &pTmp, pSrcIn);
        if( ierr != 0)  {                           // Check for error
          return( ierr);
        }

        pDst = pTmp;
      }

      ierr = YaIPS_RGB_Morphology( &pDst, pSrc, YAIPS_RGB_MORPH_DILATION);

      if( ierr != 0) {   // Error

        return( ierr);
      }

      // exchange source and destination

      pExchange = pSrc; pSrc = pDst; pDst = pExchange;
    }

    // Erosion
    for( iRun = 0; iRun < MoprhRuns; iRun++) {

      if( pDst == pSrcIn) {            // Need temporary image

        // Ensure that temporary image has the same size and same pixel amount as pSrc
        ierr = YaIPS_RGB_EnsureSameSize( &pTmp, pSrcIn);
        if( ierr != 0)  {                           // Check for error
          return( ierr);
        }

        pDst = pTmp;
      }

      ierr = YaIPS_RGB_Morphology( &pDst, pSrc, YAIPS_RGB_MORPH_EROSION);

      if( ierr != 0) {   // Error

        return( ierr);
      }

      // exchange source and destination

      pExchange = pSrc; pSrc = pDst; pDst = pExchange;
    }
    break;

  case YAIPS_RGB_MORPH_OPENING:

    // Erosion
    for( iRun = 0; iRun < MoprhRuns; iRun++) {

      if( pDst == pSrcIn) {            // Need temporary image

        // Ensure that temporary image has the same size and same pixel amount as pSrc
        ierr = YaIPS_RGB_EnsureSameSize( &pTmp, pSrcIn);
        if( ierr != 0)  {                           // Check for error
          return( ierr);
        }

        pDst = pTmp;
      }

      ierr = YaIPS_RGB_Morphology( &pDst, pSrc, YAIPS_RGB_MORPH_EROSION);

      if( ierr != 0) {   // Error

        return( ierr);
      }

      // exchange source and destination

      pExchange = pSrc; pSrc = pDst; pDst = pExchange;
    }

    // dilation
    for( iRun = 0; iRun < MoprhRuns; iRun++) {

      if( pDst == pSrcIn) {            // Need temporary image

        // Ensure that temporary image has the same size and same pixel amount as pSrc
        ierr = YaIPS_RGB_EnsureSameSize( &pTmp, pSrcIn);
        if( ierr != 0)  {                           // Check for error
          return( ierr);
        }

        pDst = pTmp;
      }

      ierr = YaIPS_RGB_Morphology( &pDst, pSrc, YAIPS_RGB_MORPH_DILATION);

      if( ierr != 0) {   // Error

        return( ierr);
      }

      // exchange source and destination

      pExchange = pSrc; pSrc = pDst; pDst = pExchange;
    }
    break;

  default:

    for( iRun = 0; iRun < MoprhRuns; iRun++) {

      if( pDst == pSrcIn) {            // Need temporary image

        // Ensure that temporary image has the same size and same pixel amount as pSrc
        ierr = YaIPS_RGB_EnsureSameSize( &pTmp, pSrcIn);
        if( ierr != 0)  {                           // Check for error
          return( ierr);
        }

        pDst = pTmp;
      }

      ierr = YaIPS_RGB_Morphology( &pDst, pSrc, MorphOp);

      if( ierr != 0) {   // Error

        return( ierr);
      }

      // exchange source and destination

      pExchange = pSrc; pSrc = pDst; pDst = pExchange;
    }
    break;
  }  // end switch()

  *ppDst = pSrc;

  if( pDst != pSrcIn) {            // Used temporary image

    pDst->release();               // Release image data
  }

  return( 0);                                 // Return OK
}

/******************************** End Of File ********************************/

