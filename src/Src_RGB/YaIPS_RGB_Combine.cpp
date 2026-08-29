/****************************************************************************

  YaIPS_RGB_Combine.cpp

  Fl_RGB_Image image processing.
  Combine two images

 25.04.2025 RR: First edition of this file.

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

#define YAIPS_RESULT_SCHIFT   14            // Use this shift factor to use integer multiply

/***************************************************************************
* YaIPS_RGB_Combine
* Combine two images
*
* If sources sizes are different, the destination image gets the size
* of the first source image. The second image is resized to the size
* of the first source image.
*
* ppDst        Pointer to pointer to RGB image
* pSrc1        1. source image
* pSrc2        2. source image
* Operator     Type of operation
* Alpha_Op     Alpha operator
* ResMultArg   Result multiplier, default should be 1.0
* Offset       Add this offset to the result
* MultArg2     2. multiplier. Is used my weighted add.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Combine( Fl_RGB_Image **ppDst,   // Out: Pointer to pointer to RGB image
                       Fl_RGB_Image *pSrc1,    // Source image
                       Fl_RGB_Image *pSrc2,    // Source image
                       int Operator,           // Type of operation
                       int Alpha_Op,           // Alpha operator
                       float ResMultArg,       // Result multiplier
                       int Offset,             // Add this offset to the result
                       float MultArg2,         // Optional: 2. multiplier. Is used my weighted add.
                       Fl_RGB_Image *pSrc3)    // Optional: a third source image
{
  int ierr, x, y, iByte, nByteSrc1All, nByteSrc2All, nByteSrc3All, nByteSrc1CollC, nByteSrc2CollC, nByteSrc3CollC;
  int nByteSrc1Step, nByteSrc2Step, nByteSrc3Step;
  int nByteDstAll, nByteDstCollC, AlphaSrc1, AlphaSrc2, AlphaSrc3, AlphaDst;
  int xx, yy, t, t2, Src2xx, Src2yy, Src3xx, Src3yy;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc1, iSrc2, iSrc3;
  uchar *s18, *s28, *s38, *d8, TempAlpha;
  uchar *pLineSource1, *pLineSource2, *pLineSource3, *pL;
  long ResMultArgLong, ResMultArgLong2;
  long MultArg2Long, MultArg2Long2;

  // Check 1. source first
  ierr = YaIPS_RGB_to_ImgD( pSrc1, &iSrc1);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Check 2. source
  ierr = YaIPS_RGB_to_ImgD( pSrc2, &iSrc2);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data
  // NOTE: source 1 and destination has the same width and height.
  //       Number of bytes may be different, depends from alpha.


  nByteSrc1All = iSrc1.d;                     // # bytes 1. source
  xx = iSrc1.xx;                              // Destination and 1. source image have the same size
  yy = iSrc1.yy;

  nByteSrc2All = iSrc2.d;                     // # bytes 2. source
  Src2xx = iSrc2.xx;                          // Size of second source may differ
  Src2yy = iSrc2.yy;

  // Check optional 3. source

  if( Operator == YAIPS_COMBINE_OP_FADEIMG1 || // Need optional 3. source
      Operator == YAIPS_COMBINE_OP_FADEIMG2) {

    // Check 3. source
    ierr = YaIPS_RGB_to_ImgD( pSrc3, &iSrc3);
    if( ierr != 0)  {                           // Check for error
      return( ierr);
    }

  } else {

    memset( &iSrc3, 0, sizeof( iSrc3));         // zero data
  }

  nByteSrc3All = iSrc3.d;                     // # bytes 3. source
  Src3xx = iSrc3.xx;                          // Size of third source may differ
  Src3yy = iSrc3.yy;

  // Manage color channels and alpha channel usage

  AlphaSrc1 = nByteSrc1All == 2 || nByteSrc1All == 4 ? 1 : 0; // Source 1 has alpha
  AlphaSrc2 = nByteSrc2All == 2 || nByteSrc2All == 4 ? 1 : 0; // Source 2 has alpha
  AlphaSrc3 = nByteSrc3All == 2 || nByteSrc3All == 4 ? 1 : 0; // Source 2 has alpha

  nByteSrc1CollC = nByteSrc1All - AlphaSrc1;                  // Source 1 number of color channels
  nByteSrc2CollC = nByteSrc2All - AlphaSrc2;                  // Source 2 number of color channels
  nByteSrc3CollC = nByteSrc3All - AlphaSrc3;                  // Source 3 number of color channels

  if( nByteSrc1All >= 3 || nByteSrc2All >= 3) {  // Have any color image

    nByteDstAll = 3;                             // Output is a color image too

  } else {                                       // Output is a bw image

    nByteDstAll = 1;                             // Output is a bw image too
  }

  nByteDstCollC = nByteDstAll;                   // Set number of color channels for output image

  switch( Alpha_Op) {
  case YAIPS_COMBINE_ALPHA_NO:      // No alpha. Strip any existing alpha.
  default:

    AlphaDst = 0;                   // Output has no alpha
    break;

  case YAIPS_COMBINE_ALPHA_KEEP_1:  // Keep alpha from first input.

    AlphaDst = AlphaSrc1;           // Output has alpha if input 1 has alpha
    break;

  case YAIPS_COMBINE_ALPHA_KEEP_2:  // Keep alpha from second input.

    AlphaDst = AlphaSrc2;           // Output has alpha if input 2 has alpha
    break;

  case YAIPS_COMBINE_ALPHA_MIN:     // If both inputs have alpha, output minimum of alpha values.
  case YAIPS_COMBINE_ALPHA_MAX:     // If both inputs have alpha, output maximum of alpha values.

    if( AlphaSrc1 == 0 && AlphaSrc2 == 0) {         // None of the inputs has alpha

      AlphaDst = 0;                                 // Output has no alpha

    } else if( AlphaSrc1 > 0 && AlphaSrc2 == 0) {   // 1. input has alpha an 2. input has no alpha

      if( Alpha_Op == YAIPS_COMBINE_ALPHA_MAX) {    // Maximum alpha

        AlphaDst = 0;                               // Output has no alpha

      } else {

        Alpha_Op = YAIPS_COMBINE_ALPHA_KEEP_1;      // Switch operator to keep 1
        AlphaDst = 1;                               // Output has alpha
      }

    } else if( AlphaSrc1 == 0 && AlphaSrc2 > 0) {   // 1. input has no alpha an 2. input has alpha

      if( Alpha_Op == YAIPS_COMBINE_ALPHA_MAX) {    // Maximum alpha

        AlphaDst = 0;                               // Output has no alpha

      } else {

        Alpha_Op = YAIPS_COMBINE_ALPHA_KEEP_2;      // Switch operator to keep 2
        AlphaDst = 1;                               // Output has alpha
      }

    } else {                                        // Both inputs has alpha

      AlphaDst = 1;                                 // Output has alpha
    }

    break;
  }

  nByteDstAll += AlphaDst;                          // Add alpha channel (if any)

  // Ensure that pPDst image has the same size and same pixel amount as pSrc1
  ierr = YaIPS_RGB_ImageSetSize( ppDst, xx, yy, nByteDstAll);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Multiply as integer

  // Convert to 32 bit integer

  if( Operator == YAIPS_COMBINE_OP_MULT) {     // Multiply

    ResMultArg = ResMultArg / 256.0;           // divide by 256 to normalize result
  }

  if( Operator == YAIPS_COMBINE_OP_FADE) {     // Fade between two images

    // ResMultArg has the range 0.0 ... 1.0
    // For 0.0 the first image is full visible, for 1.0 the second image is full visible

    if( ResMultArg < 0.0) {                    // Ensure proper range

      ResMultArg = 0.0;
    }

    if( ResMultArg > 1.0) {                    // Ensure proper range

      ResMultArg = 1.0;
    }

    MultArg2 = 1.0 - ResMultArg;
  }

  ResMultArgLong = (long)(ResMultArg * (1 << YAIPS_RESULT_SCHIFT));
  ResMultArgLong2 = ResMultArgLong / 2;

  MultArg2Long = (long)(MultArg2 * (1 << YAIPS_RESULT_SCHIFT));
  MultArg2Long2 = MultArg2Long / 2;

  // Prepare resize of first image if color channels are different from

  pLineSource1 = NULL;                       // No memory allocated

  nByteSrc1Step = nByteSrc1All;

  if( nByteSrc1CollC != nByteDstCollC) {     // Must convert bw to color

    nByteSrc1Step = nByteDstCollC + AlphaSrc1;

    pLineSource1 = (uchar *)malloc( xx * nByteSrc1Step);

    if( pLineSource1 == NULL) {              // Allocation has failed

      return( -100);
    }
  }

  // Prepare resize of second image if size is different from first source image

  pLineSource2 = NULL;                       // No memory allocated

  nByteSrc2Step = nByteSrc2All;

  if( (nByteSrc2CollC != nByteDstCollC) ||   // Must convert bw to color
       xx != Src2xx || yy != Src2yy) {       // or size is different

    nByteSrc2Step = nByteDstCollC + AlphaSrc2;

    pLineSource2 = (uchar *)malloc( xx * nByteSrc2Step);

    if( pLineSource2 == NULL) {              // Allocation has failed

      return( -101);
    }
  }

  // Prepare resize of third image if size is different from first source image

  pLineSource3 = NULL;                       // No memory allocated

  nByteSrc3Step = 1;

  if( pSrc3 != NULL) {                        // Use optional 3. source

    nByteSrc3Step = nByteSrc3All;

    if( (nByteSrc3CollC != nByteDstCollC) ||   // Must convert bw to color
         xx != Src3xx || yy != Src3yy ||       // or size is different
         (AlphaSrc3 && (Operator == YAIPS_COMBINE_OP_FADEIMG1 || Operator == YAIPS_COMBINE_OP_FADEIMG2))) {

      nByteSrc3Step = nByteDstCollC + AlphaSrc3;

      pLineSource3 = (uchar *)malloc( xx * nByteSrc3Step);

      if( pLineSource3 == NULL) {              // Allocation has failed

        return( -103);
      }
    }
  }

  // Combine ...

  for( y = 0; y < yy; y++) {

    d8  = RGB_pixad( 0,  y, &iDst);

    // Prepare a line for first input

    if( pLineSource1 == NULL) {  // First and second source image have the same size

      s18 = RGB_pixad( 0,  y, &iSrc1);

    } else {                     // First and second source image have the different sizes

      // Convert bw to color

      pL = pLineSource1;

      s18 = RGB_pixad( 0,  y, &iSrc1);

      for( x = 0; x < xx; x++) {

        pL[ 0] = *s18;
        pL[ 1] = *s18;
        pL[ 2] = *s18;

        if( AlphaSrc1) {

          pL[ 3] = s18[ 1];
        }

        pL += nByteSrc1Step;
        s18 += nByteSrc1All;
      }

      // Use data from converted line for combine operations
      s18 = pLineSource1;
    }

    // Prepare second input image

    if( pLineSource2 == NULL) {  // First and second source image have the same size

      s28 = RGB_pixad( 0,  y, &iSrc2);

    } else {                     // First and second source image have the different sizes

      // Resize one line of data

      // We make a nearest neighbor interpolation

      pL = pLineSource2;

      for( x = 0; x < xx; x++) {

        s28 = RGB_pixad( ((x * Src2xx) / xx), (y * Src2yy) / yy, &iSrc2);

        if( nByteSrc2CollC != nByteDstCollC) {   // Must convert bw to color

          pL[ 0] = *s28;
          pL[ 1] = *s28;
          pL[ 2] = *s28;

          if( AlphaSrc2) {

            pL[ 3] = s28[ 1];
          }

          pL += nByteSrc2Step;

        } else {

          for( iByte = 0; iByte < nByteDstCollC; iByte++) {

            *pL++ = *s28++;
          }

          if( AlphaSrc2) {

            *pL++ = *s28++;
          }
        }
      }

      // Use data from resized image for combine operations
      s28 = pLineSource2;
    }

    // Prepare third input image

    s38 = NULL;

    if( pSrc3 != NULL) {                        // Use optional 3. source

      // The fade image operators prefer the alpha channel as mix image

      if( (AlphaSrc3 && (Operator == YAIPS_COMBINE_OP_FADEIMG1 || Operator == YAIPS_COMBINE_OP_FADEIMG2))) {

        // Resize one line of data

        // We make a nearest neighbor interpolation

        pL = pLineSource3;

        for( x = 0; x < xx; x++) {

          s38 = RGB_pixad( ((x * Src3xx) / xx), (y * Src3yy) / yy, &iSrc3);

          TempAlpha = s38[ nByteSrc3CollC];                      // Get the temp alpha

          for( iByte = 0; iByte < nByteSrc3Step; iByte++) {

            *pL++ = TempAlpha;
          }
        }

        // Use data from resized image for combine operations
        s38 = pLineSource3;

      } else {

        if( pLineSource3 == NULL) {  // First and third source image have the same size

          s38 = RGB_pixad( 0,  y, &iSrc3);

        } else {                     // First and second source image have the different sizes

          // Resize one line of data

          // We make a nearest neighbor interpolation

          pL = pLineSource3;

          for( x = 0; x < xx; x++) {

            s38 = RGB_pixad( ((x * Src3xx) / xx), (y * Src3yy) / yy, &iSrc3);

            if( nByteSrc3CollC != nByteDstCollC) {   // Must convert bw to color

              pL[ 0] = *s38;
              pL[ 1] = *s38;
              pL[ 2] = *s38;

              if( AlphaSrc3) {

                pL[ 3] = s38[ 1];
              }

              pL += nByteSrc3Step;

            } else {

              for( iByte = 0; iByte < nByteDstCollC; iByte++) {

                *pL++ = *s38++;
              }

              if( AlphaSrc3) {

                *pL++ = *s38++;
              }
            }
          }

          // Use data from resized image for combine operations
          s38 = pLineSource3;
        }
      }
    }

    // ...

    switch( Operator) {

    case YAIPS_COMBINE_OP_ADD:     // Addition
    default:
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = (int)s18[ iByte] + (int)s28[ iByte];

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_W_ADD:     // Weighted addition
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          // Multiplication of first input

          t = (int)s18[ iByte];

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          // Multiplication of second input

          t2 = (int)s28[ iByte];

          if( MultArg2Long != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t2 >= 0) {
              t2 = (int)((t2 * MultArg2Long + MultArg2Long2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t2 = (int)((t2 * MultArg2Long - MultArg2Long2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += t2;                                     // Weighted add
          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_SUB_1_2:     // Subtraction 1. source - 2. source
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = (int)s18[ iByte] - (int)s28[ iByte];

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_SUB_2_1:     // Subtraction 2. source - 1. source
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = (int)s28[ iByte] - (int)s18[ iByte];

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_SUB_ABS: // Subtraction with absolute value
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = (int)s18[ iByte] - (int)s28[ iByte];

          if( t < 0) {
            t = - t;
          }

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_MULT:    // Multiplication
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = (int)s18[ iByte] * (int)s28[ iByte];

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_MIN:     // Minimum value
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = (int)s18[ iByte];
          if( t > (int)s28[ iByte]) {
            t = (int)s28[ iByte];
          }

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_MAX:     // Maximum value
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = (int)s18[ iByte];
          if( t < (int)s28[ iByte]) {
            t = (int)s28[ iByte];
          }

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_AVG:     // Average images
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = ((int)s18[ iByte] + (int)s28[ iByte] + 1) >> 1;

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_AND:     // AND images
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = (int)s18[ iByte] & (int)s28[ iByte];

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_OR:      // OR images
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = (int)s18[ iByte] | (int)s28[ iByte];

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_XOR:     // XOR images
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = (int)s18[ iByte] ^ (int)s28[ iByte];

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_CMP_EQ:    // Compare images ==
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = ((int)s18[ iByte] == (int)s28[ iByte]) * 255;

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_CMP_NE:     // Compare images !=
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = ((int)s18[ iByte] != (int)s28[ iByte]) * 255;

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_CMP_GT:     // Compare images >
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = ((int)s18[ iByte] > (int)s28[ iByte]) * 255;

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_CMP_LE:     // Compare images <=
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = ((int)s18[ iByte] <= (int)s28[ iByte]) * 255;

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_CMP_GE:     // Compare images >=
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = ((int)s18[ iByte] >= (int)s28[ iByte]) * 255;

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_CMP_LT:     // Compare images <
      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = ((int)s18[ iByte] < (int)s28[ iByte]) * 255;

          if( ResMultArgLong != (1 << YAIPS_RESULT_SCHIFT)) {        // NOT factor 1.0
            if( t >= 0) {
              t = (int)((t * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            } else {
              t = (int)((t * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT);
            }
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_SHADING:     // Image shading

      {
        int Round;

        // Prepare reference brightness

        if( Offset > 255) {             // Clip maximum value

          Offset = 255;

        } else if( Offset < 128) {      // Clip minimum value

          Offset = 128;
        }

        Round = 1 << (10 - 1);          // Factor 0.5

        // ...

        for( x = 0; x < xx; x++) {
          for( iByte = 0; iByte < nByteDstCollC; iByte++) {

            // Shading image
            t2 = (int)s28[ iByte];

            if( t2 <= 0) {

              t2 = (4 << 10);              // maximum factor is 4

            } else {

              t2 = (Offset << 10) / t2;

              if( t2 < (1 << (10 - 1))) {  // minimum factor is 0.5
                t2 = (1 << (10 - 1));
              }

              if( t2 > (4 << 10)) {        // maximum factor is 4
                t2 = (4 << 10);
              }
            }

            // Shade 1. image
            t = ((int)s18[ iByte] * t2 + Round) >> 10;

            // Store

            RGB_bclip( t, d8 + iByte);                   // Clip and store
          }

          d8  += nByteDstAll;
          s18 += nByteSrc1Step;
          s28 += nByteSrc2Step;
        }
      }
      break;

    case YAIPS_COMBINE_OP_FADE:     // Fade between two images

      // ResMultArg has the range 0.0 ... 1.0
      // For 0.0 the first image is full visible, for 1.0 the second image is full visible

      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          t = (int)(((int)s28[ iByte] * ResMultArgLong + (int)s18[ iByte] * MultArg2Long) >> YAIPS_RESULT_SCHIFT);

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
      }
      break;

    case YAIPS_COMBINE_OP_FADEIMG1: // Fade between two images by third image

      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          int AlphaVal;

          AlphaVal = s38[ iByte];

          if( AlphaVal <= 0) {

            t = s28[ iByte];

          } else if( AlphaVal >= 255) {

            t = s18[ iByte];

          } else {

            t = (s28[ iByte] * (255 - AlphaVal) + s18[ iByte] * AlphaVal) / 255;
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
        s38 += nByteSrc3Step;
      }
      break;

    case YAIPS_COMBINE_OP_FADEIMG2: // Fade between two images by third image

      for( x = 0; x < xx; x++) {
        for( iByte = 0; iByte < nByteDstCollC; iByte++) {

          int AlphaVal;

          AlphaVal = s38[ iByte];

          if( AlphaVal <= 0) {

            t = s18[ iByte];

          } else if( AlphaVal >= 255) {

            t = s28[ iByte];

          } else {

            t = (s18[ iByte] * (255 - AlphaVal) + s28[ iByte] * AlphaVal) / 255;
          }

          t += Offset;                                 // Add offset
          RGB_bclip( t, d8 + iByte);                   // Clip and store
        }

        d8  += nByteDstAll;
        s18 += nByteSrc1Step;
        s28 += nByteSrc2Step;
        s38 += nByteSrc3Step;
      }
      break;

    }  // end switch( Operator)

    if( AlphaDst > 0) {     // Destination has alpha output

      d8  = RGB_pixad( 0,  y, &iDst);
      d8 += nByteDstCollC;  // Point to alpha

      if( pLineSource1 == NULL) {  // First and second source image have the same size

        s18 = RGB_pixad( 0,  y, &iSrc1);

      } else {                     // First and second source image have the different sizes

        // Use data from converted line for combine operations
        s18 = pLineSource1;
      }

      s18 += nByteSrc1Step -1;    // Point to alpha

      if( pLineSource2 == NULL) {  // First and second source image have the same size

        s28 = RGB_pixad( 0,  y, &iSrc2);

      } else {                     // First and second source image have the different sizes

        // Use data from resized image for combine operations
        s28 = pLineSource2;
      }

      s28 += nByteSrc2Step -1;    // Point to alpha

      switch( Alpha_Op) {
      case YAIPS_COMBINE_ALPHA_NO:      // No alpha. Strip any existing alpha.
      default:

        // Nothing to do here
        break;

      case YAIPS_COMBINE_ALPHA_KEEP_1:  // Keep alpha from first input.

        for( x = 0; x < xx; x++) {

          *d8 = *s18;

          d8  += nByteDstAll;
          s18 += nByteSrc1Step;
        }
        break;

      case YAIPS_COMBINE_ALPHA_KEEP_2:  // Keep alpha from second input.

        for( x = 0; x < xx; x++) {

          *d8 = *s28;

          d8  += nByteDstAll;
          s28 += nByteSrc2Step;
        }
        break;

      case YAIPS_COMBINE_ALPHA_MIN:     // If both inputs have alpha, output minimum of alpha values.

        for( x = 0; x < xx; x++) {

          if( *s18 < *s28) {

            *d8 = *s18;
          } else {

            *d8 = *s28;
          }

          d8  += nByteDstAll;
          s18 += nByteSrc1Step;
          s28 += nByteSrc2Step;
        }
        break;

      case YAIPS_COMBINE_ALPHA_MAX:     // If both inputs have alpha, output maximum of alpha values.

        for( x = 0; x < xx; x++) {

          if( *s18 > *s28) {

            *d8 = *s18;
          } else {

            *d8 = *s28;
          }

          d8  += nByteDstAll;
          s18 += nByteSrc1Step;
          s28 += nByteSrc2Step;
        }
        break;
      }
    }
  }

  if( pLineSource1 != NULL) {                 // Free allocated memory

    free( pLineSource1);
  }

  if( pLineSource2 != NULL) {                 // Free allocated memory

    free( pLineSource2);
  }

  if( pLineSource3 != NULL) {                 // Free allocated memory

    free( pLineSource3);
  }

  return( 0);                                 // Return OK
}

/******************************** End Of File ********************************/

