/****************************************************************************

  YaIPS_RGB_Sobel3x3.cpp

  Fl_RGB_Image image processing.
  Geometric transformations of Images

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

/***************************************************************************
* SetOutsiteColor
*
* Set outside fill color for color variable to on one image pixel
****************************************************************************
*/
static void SetOutsiteColor( uchar *pFillColor,    // pints to 4 bytes
                             int nByte,            // Number of bytes / color components of a pixel
                             int OutsiteColor,     // Fl_Color value
                             int OutsiteBlend)     // If set, outside area is alpha blended
{
  uchar r, g, b, Alpha;

  Fl::get_color( (Fl_Color)OutsiteColor, r, g, b);

  Alpha = OutsiteBlend ? 0 : 255;

  if( nByte >= 3) {          // Is a color image

    pFillColor[ 0] = r;
    pFillColor[ 1] = g;
    pFillColor[ 2] = b;
    pFillColor[ 3] = Alpha;  // Optional alpha

  } else {                   // Is a black and white image

    pFillColor[ 0] = (r * 76 + g * 150 + b * 30) >> 8;  // Calculate intensity
    pFillColor[ 1] = Alpha;  // Optional alpha
    pFillColor[ 2] = Alpha;  // Set a reasonable value. Should not be used.
    pFillColor[ 3] = Alpha;  // Set a reasonable value. Should not be used.
  }
}

/***************************************************************************
* YaIPS_RGB_Geo_Resize2
* Resize by a power of 2.
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* SizeShiftArg    Power of 2, < 0 is shrink > 0 is enlarge
*              0 = 1:1, 1 = * 2, 2 = * 4, -1 = / 2, -2 = / 4, ...
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Geo_Resize2( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                           Fl_RGB_Image *pSrc,   // Source image
                           int SizeShiftArg)     // Power of 2, < 0 is shrink > 0 is enlarge
{
  int ierr, x, y, sx, sy, d, nByt, SizeFac, SizeShift;
  int sxx, syy, dxx, dyy, sld, dld, Area2;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s80, *d80, *s8, *d8;
  int *pLine = NULL, *pL;                     // Point to line with pixels

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  //

  if( SizeShiftArg == 0) {                    // no size change

    ierr = YaIPS_RGB_CopyImg( ppDst, pSrc);   // simply copy image

    return( ierr);
  }

  // Get image data

  sxx = iSrc.xx;
  syy = iSrc.yy;
  sld = iSrc.ld;
  nByt = iSrc.d;

  if( SizeShiftArg > 0) {                     // Enlarge

    SizeShift = SizeShiftArg;

    SizeFac = 1 << SizeShift;                 // Size factor

    dxx = sxx << SizeShift;
    dyy = syy << SizeShift;

  } else {                                    // Shrink

    SizeShift = - SizeShiftArg;

    SizeFac = 1 << SizeShift;                 // Size factor

    dxx = sxx >> SizeShift;
    dyy = syy >> SizeShift;
  }

  Area2 = (SizeFac * SizeFac) / 2;            // 1/2 area, used for rounding

  // Check size

  if( dxx > 0x7fff || dyy > 0x7fff ) {

    if( errstring == NULL) {                 // No error until now

      errstring = LangStringLookup( "&RGB_Geo_Resize1=New image too big");
    }

    ierr = -101;
    goto ErrorExit;
  }

  if( dxx < 8 || dyy < 8 ) {

    if( errstring == NULL) {                 // No error until now

      errstring = LangStringLookup( "&RGB_Geo_Resize2=New image too small");
    }

    ierr = -102;
    goto ErrorExit;
  }

  // Create destination image
  // NOTE: nByt must be >= 1 and <= 4 here

  ierr = YaIPS_RGB_ImageSetSize( ppDst, dxx, dyy, nByt);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  dld = iDst.ld;

  // Resize

  if( SizeShiftArg > 0) {                     // Enlarge

    int SizeOfLine;

    SizeOfLine = dxx * nByt;                  // Size of one destination line of data

    s80 = RGB_pixad( 0, 0, &iSrc);
    d80 = RGB_pixad( 0, 0, &iDst);

    for( y = 0; y < syy; y++) {

      s8 = s80;
      d8 = d80;

      // Enlarge first destination line

      for( x = 0; x < sxx; x++) {

        for( sx = 0; sx < SizeFac; sx++) {

          for( d = 0; d < nByt; d++) {
            *d8++ = s8[ d];
          }
        }

        s8 += nByt;
      }

      // Copy other destination lines

      d8 = d80;            // Begin of first line
      d80 += dld;          // Next line

      for( sy = 1; sy < SizeFac; sy++) {

        memcpy( d80, d8, SizeOfLine);

        d80 += dld;        // Next destination line
      }

      s80 += sld;          // Next source line
    }

  } else {                                    // Shrink

    int SizeOfLine;

    SizeOfLine = dxx * nByt;                  // Size of one destination line of data

    // Allocate a line buffer for summation
    pLine = (int *)malloc( SizeOfLine * sizeof( int));

    if( pLine == NULL) {

      if( errstring == NULL) {                  // No error until now

        errstring = ERR_IPS_NO_MEM;
      }

      ierr = -103;
      goto ErrorExit;
    }

    s80 = RGB_pixad( 0, 0, &iSrc);
    d80 = RGB_pixad( 0, 0, &iDst);

    for( y = 0; y < dyy; y++) {

      d8 = d80;

      // Shrink destination line

      memset( pLine, 0, SizeOfLine * sizeof( int));   // Zero sum

      for( sy = 0; sy < SizeFac; sy++) {

        pL = pLine;
        s8 = s80;

        for( x = 0; x < dxx; x++) {

          for( sx = 0; sx < SizeFac; sx++) {

            for( d = 0; d < nByt; d++) {
              pL[ d] += s8[ d];
            }
            s8 += nByt;
          }

          pL += nByt;
        }

        s80 += sld;          // Next source line
      }

      pL = pLine;

      for( x = 0; x < dxx; x++) {

        for( d = 0; d < nByt; d++) {

          *d8++ = (*pL++ + Area2) >> (SizeShift + SizeShift);
        }
      }

      d80 += dld;           // Next destination line
    }
  }

  ierr = 0;                         // Return OK

ErrorExit:

  if( pLine != NULL) {              // Data was allocated

    free( pLine);
  }

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_Geo_Mirror
* Mirror image in X, Y or X and Y
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* MirrorOp     0 = X, 1 = Y, 2 = X and Y
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Geo_Mirror( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                          Fl_RGB_Image *pSrc,   // Source image
                          int MirrorOp)         // Mirror 0 = X, 1 = Y, 2 = X and Y
{
  int ierr, iByte, nByte, SrcStep;
  int x, y, xx, yy;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8;

  // Check arguments

  if( MirrorOp < 0 || MirrorOp > 2) {

    return( -100);          // Mirror argument is out of range
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

  // Get image data
  // NOTE: source and destination has the same size and number of bytes

  xx = iSrc.xx;
  yy = iSrc.yy;
  nByte = iSrc.d;

  // Mirror

  for( y = 0; y < yy; y++) {

    if( MirrorOp == 0) {      // Mirror X

      s8 = RGB_pixad( xx - 1, y, &iSrc);
      d8 = RGB_pixad( 0, y, &iDst);

      SrcStep = - nByte;

    } else if( MirrorOp == 1) {    // Mirror Y

      s8 = RGB_pixad( 0, yy - y - 1, &iSrc);
      d8 = RGB_pixad( 0, y, &iDst);

      SrcStep = nByte;

    } else {                  // Mirror X and Y

      s8 = RGB_pixad( xx - 1, yy - y - 1, &iSrc);
      d8 = RGB_pixad( 0, y, &iDst);

      SrcStep = - nByte;
    }

    if( nByte == 1) {

      for( x = 0; x < xx; x++) {

        *d8 = *s8;

        d8 += nByte;
        s8 += SrcStep;
      }

    } else if( nByte == 2) {

      for( x = 0; x < xx; x++) {

        d8[ 0] = s8[ 0];
        d8[ 1] = s8[ 1];

        d8 += nByte;
        s8 += SrcStep;
      }

    } else if( nByte == 3) {

      for( x = 0; x < xx; x++) {

        d8[ 0] = s8[ 0];
        d8[ 1] = s8[ 1];
        d8[ 2] = s8[ 2];

        d8 += nByte;
        s8 += SrcStep;
      }

    } else {

      for( x = 0; x < xx; x++) {

        for( iByte = 0; iByte < nByte; iByte++) {

          d8[ iByte] = s8[ iByte];
        }

        d8 += nByte;
        s8 += SrcStep;
      }
    }
  }

  ierr = 0;                         // Return OK

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_Geo_Rotate90
* Rotate image by +-90 or 180 degrees
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* RotateOp     Rotate 0 = 0°, 1 = 90° clockwise, 2 = 180°, 3 = 90° counterclockwise
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Geo_Rotate90( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                            Fl_RGB_Image *pSrc,   // Source image
                            int RotateOp)         // Rotate  0 = 0°, 1 = 90° clockwise, 2 = 180°, 3 = 90° counterclockwise
{
  int ierr, iByte, nByte, SrcStep;
  int x, y, xxs, yys, xxd, yyd;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8;

  // Check arguments

  if( RotateOp < 0 || RotateOp > 3) {

    return( -100);          // Mirror argument is out of range
  }

  // Same cases are done from other functions

  if( RotateOp == 0)  {     // Rotate 0°

    ierr = YaIPS_RGB_CopyImg( ppDst, pSrc);

    return( ierr);
  }

  if( RotateOp == 2)  {     // Rotate 180°

    ierr = YaIPS_RGB_Geo_Mirror( ppDst, pSrc, 2);

    return( ierr);
  }

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data

  xxs = iSrc.xx;
  yys = iSrc.yy;
  nByte = iSrc.d;

  // Ensure that pPDst image has exchanged sizes and same pixel amount as pSrc
  ierr = YaIPS_RGB_ImageSetSize( ppDst, yys, xxs, nByte);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data
  // NOTE: xxd == yys and yyd = xxs

  xxd = iDst.xx;
  yyd = iDst.yy;

  // Mirror

  for( y = 0; y < yyd; y++) {

    if( RotateOp == 1) {      // Rotate 90° clockwise

      s8 = RGB_pixad( y, yys - 1, &iSrc);
      d8 = RGB_pixad( 0, y, &iDst);

      SrcStep = - nByte * xxs;

    } else {                  // Rotate 90° counterclockwise

      s8 = RGB_pixad( yyd - y - 1, 0, &iSrc);
      d8 = RGB_pixad(  0, y, &iDst);

      SrcStep = nByte * xxs;
    }

    if( nByte == 1) {

      for( x = 0; x < xxd; x++) {

        *d8 = *s8;

        d8 += nByte;
        s8 += SrcStep;
      }

    } else if( nByte == 2) {

      for( x = 0; x < xxd; x++) {

        d8[ 0] = s8[ 0];
        d8[ 1] = s8[ 1];

        d8 += nByte;
        s8 += SrcStep;
      }

    } else if( nByte == 3) {

      for( x = 0; x < xxd; x++) {

        d8[ 0] = s8[ 0];
        d8[ 1] = s8[ 1];
        d8[ 2] = s8[ 2];

        d8 += nByte;
        s8 += SrcStep;
      }

    } else {

      for( x = 0; x < xxd; x++) {

        for( iByte = 0; iByte < nByte; iByte++) {

          d8[ iByte] = s8[ iByte];
        }

        d8 += nByte;
        s8 += SrcStep;
      }
    }
  }

  ierr = 0;                         // Return OK

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_Geo_Lens
* Lens Correction, rotation and minor size or offset changes
*
* ppDst            Pointer to pointer to RGB image
* pSrc             Source image
* PLensFacArg      Lens correction factor [in units of 1.0E-10]
* Rotation         Rotation correction [Degree]
* SizeCorrPer      Size correction percent [%]
* xDelta, yDelta   Position correction [pixel]
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Geo_Lens( Fl_RGB_Image **ppDst,     // Out: Pointer to pointer to RGB image
                        Fl_RGB_Image *pSrc,       // Source image
                        int OutsiteColor,         // Color for the area outside an image
                        int OutsiteBlend,         // If set, outside area is alpha blended
                        int32 PLensFacArg,        // Lens correction factor [in units of 1.0E-10]
                        float Rotation,           // Rotation correction [Degree]
                        float SizeCorrPer,        // Size correction percent [%]
                        int xDelta, int yDelta)   // Position correction [pixel]
{
  int ierr, iByte, nByteSrc, nByteDst, AddAlpha;
  int xx, yy, xx1, yy1, xx2, yy2, xDst, yDst, xSrc, ySrc, xmSrc, xRem, yRem, TempI;
  uchar *p8d, *p8s1, *p8s2;
  double PLensFac, xx2D, yy2D, r2, TempD, Angle, SinAngle, CosAngle, ScaleFac;
  double xSrcIn, ySrcIn, xSrcD1, ySrcD1;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar Outsite_FillColor[ 4];

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data
  // NOTE: source and destination has the same width and height.
  //       Number of bytes may be different, depends from alpha.

  xx       = iSrc.xx;
  yy       = iSrc.yy;
  xmSrc    = iSrc.ld;
  nByteSrc = iSrc.d;

  // Manage alpha channel usage

  AddAlpha = false;                       // Preset no alpha

  if( nByteSrc == 1 || nByteSrc == 3) {   // Source has no alpha

    nByteDst   = nByteSrc;

    if( OutsiteBlend) {                   // Must add alpha

      nByteDst += 1;

      AddAlpha = true;                    // Has to add an alpha channel
    }

  } else {                                // Source has alpha

    nByteDst   = nByteSrc;
  }

  // Ensure that pPDst image has the same size and same pixel amount as pSrc
  ierr = YaIPS_RGB_ImageSetSize( ppDst, xx, yy, nByteDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_to_ImgD( *ppDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // ...

  xx1 = xx - 1;         // need a border of 1 for bilinear interpolation
  yy1 = yy - 1;

  xx2 = xx / 2;         // image center
  yy2 = yy / 2;

  xx2D = xx2;
  yy2D = yy2;

  // rotation

  Angle = Rotation * M_PI / 180.0;                 // convert rotation from degree to radiant
  Angle = -1.0 * Angle;                            // change direction, we work backwards

  SinAngle = sin( Angle);
  CosAngle = cos( Angle);

  // lens factor

  PLensFac = PLensFacArg * 1.0e-10;                // adjust lens correction factor

  if( PLensFac >= 0.0) {

    // compensate for size increase by use of lens factor correction

    if( xx2D < yy2D) {      // what is more inside

      r2 = xx2D * xx2D;
      TempD = 1.0 / (1.0 + r2 * PLensFac);
      ScaleFac = 1.0 / TempD;
    } else {

      r2 = yy2D * yy2D;
      TempD = 1.0 / (1.0 + r2 * PLensFac);
      ScaleFac = 1.0 / TempD;
    }
  } else {

    ScaleFac = 1.0;
  }

  // argument for size correction in percent

  ScaleFac -= SizeCorrPer * 0.01;      // Percent to factor

  // Prepare outside fill color

  SetOutsiteColor( Outsite_FillColor, nByteSrc, OutsiteColor, OutsiteBlend);

  // transform the image

  for( yDst = 0; yDst < yy; yDst++) {

    p8d = (uchar *)RGB_pixad( 0, yDst, &iDst);

    // Prepare offset compensation

    xSrc = - xDelta - xx2;
    ySrc = yDst - yDelta - yy2;

    xSrcIn = xSrc;     // convert to float
    ySrcIn = ySrc;     // convert to float

    for( xDst = 0; xDst < xx; xDst++, p8d += nByteDst, xSrcIn += 1.0) {

      // rotation of image

      xSrcD1 = xSrcIn * CosAngle - ySrcIn * SinAngle;
      ySrcD1 = ySrcIn * CosAngle + xSrcIn * SinAngle;

      // inverse lens correction

      r2 = xSrcD1 * xSrcD1 + ySrcD1 * ySrcD1;

      TempD = ScaleFac / (1.0 + r2 * PLensFac);

      xSrcD1 = xSrcD1 * TempD;
      ySrcD1 = ySrcD1 * TempD;

      // NOTE: Can to inverse lens correction a second time to get better results

      // relative to left upper corner

      xSrcD1 = xSrcD1 + xx2D;
      ySrcD1 = ySrcD1 + yy2D;

      // float coordinates to integer
      // + remainder (for bilinear interpolation), has 4 afterpoint bits

      xSrc = (int)(xSrcD1 * 16.0);        // with afterpoint digits
      TempI = xSrc & 0xfffffff0;          // next lowest without afterpoint
      xRem  = xSrc - TempI;               // remainder
      xSrc  = xSrc >> 4;                  // before point

      ySrc = (int)(ySrcD1 * 16.0);        // with afterpoint digits
      TempI = ySrc & 0xfffffff0;          // next lowest without afterpoint
      yRem  = ySrc - TempI;               // remainder
      ySrc  = ySrc >> 4;                  // before point

      // get data from source image
      // NOTE: We keep a minimum distance of 1 from right/lower image side
      //       for bilinear interpolation.

      if( xSrc < 0 || xSrc >= xx1 || ySrc < 0 || ySrc >= yy1) { // out of source image

        if( xSrc == xx1) {                  // Last pixel in line

          if( ySrc >= 0 && ySrc < yy1) {    // One more line above

            p8s1 = (uchar *)RGB_pixad( xSrc, ySrc, &iSrc);
            p8s2 = p8s1 + xmSrc;

            for( iByte = 0; iByte < nByteSrc; iByte++) {

              // make bilinear interpolation

              p8d[ iByte] =  ( p8s1[0] * (16 - yRem) +
                               p8s2[0] * yRem) >> 4;

              p8s1++;
              p8s2++;
            }

            if( AddAlpha) {          // Inside image and need to set alpha

              p8d[ iByte] = 255;
            }

          } else if( ySrc == yy1) {                 // Left upper corner pixel

            p8s1 = (uchar *)RGB_pixad( xSrc, ySrc, &iSrc);

            for( iByte = 0; iByte < nByteSrc; iByte++) {

              // make bilinear interpolation

              p8d[ iByte] =  p8s1[0];

              p8s1++;
            }

            if( AddAlpha) {          // Inside image and need to set alpha

              p8d[ iByte] = 255;
            }

          } else {

            // out of source image, use fill color
            for( iByte = 0; iByte < nByteDst; iByte++) {
              p8d[ iByte] = Outsite_FillColor[ iByte];
            }
          }

        } else if( ySrc == yy1) {                 // Last line

          if( xSrc >= 0 && xSrc < xx1) {

            p8s1 = (uchar *)RGB_pixad( xSrc, ySrc, &iSrc);

            for( iByte = 0; iByte < nByteSrc; iByte++) {

              // make bilinear interpolation

              p8d[ iByte] = ( p8s1[0] * (16 - xRem) + p8s1[ nByteSrc] * xRem) >> 4;

              p8s1++;
            }

            if( AddAlpha) {          // Inside image and need to set alpha

              p8d[ iByte] = 255;
            }

          } else {

            // out of source image, use fill color
            for( iByte = 0; iByte < nByteDst; iByte++) {
              p8d[ iByte] = Outsite_FillColor[ iByte];
            }
          }

        } else {

          // out of source image, use fill color
          for( iByte = 0; iByte < nByteDst; iByte++) {
            p8d[ iByte] = Outsite_FillColor[ iByte];
          }
        }
      } else {

        // inside source image

        p8s1 = (uchar *)RGB_pixad( xSrc, ySrc, &iSrc);
        p8s2 = p8s1 + xmSrc;

        for( iByte = 0; iByte < nByteSrc; iByte++) {

          // make bilinear interpolation

          p8d[ iByte] = (((( p8s1[0] * (16 - xRem) + p8s1[ nByteSrc] * xRem) >> 4) * (16 - yRem)) +
                         ((( p8s2[0] * (16 - xRem) + p8s2[ nByteSrc] * xRem) >> 4) * yRem)) >> 4;

          p8s1++;
          p8s2++;
        }

        if( AddAlpha) {          // Inside image and need to set alpha

          p8d[ iByte] = 255;
        }
      }
    }
  }


  ierr = 0;                         // Return OK

//x/ErrorExit:

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_Geo_TransSetup
* Geometric transformation setup
*
* pivmod      Pivot mode for transformation
*                0 - origins of src and dst is (0.0, 0.0)
*                    (not center of pixel (0,0) which would be (0.5,0.5)!)
*                1 - center of src, center of dst
*                2 - arbitrary pivot points (center of given pixels!)
*                    given in (xps,yps) and (xpd,ypd).
* xps, yps,   Arbitrary source image pivot point, only used if "pivmod" = 2
* xpd, ypd    Arbitrary destination image pivot point, only used if "pivmod" = 2
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

typedef struct Sge_setup {
  int  pivmod;                   /* pivot mode */
  float xps;                     /* pivot point src */
  float yps;
  float xpd;                     /* pivot point dst */
  float ypd;
} Tge_setup;

static Tge_setup  ge_setup = {   // Default setup
           GE_PIVCTR,            /* center */
           (sfloat)0.0,          /* dummy pivot points */
           (sfloat)0.0,
           (sfloat)0.0,
           (sfloat)0.0,
};

int YaIPS_RGB_Geo_TransSetup( int pivmod,            // Pivot mode for transformation
                              float xps, float yps,  // Arbitrary source image pivot point, only used if "pivmod" = 2
                              float xpd, float ypd)  // Arbitrary destination image pivot point, only used if "pivmod" = 2
{
  if( pivmod < 0 || pivmod > 2) {   // Security test range

    return( -1);
  }

  ge_setup.pivmod = pivmod;
  ge_setup.xps    = xps;
  ge_setup.yps    = yps;
  ge_setup.xpd    = xpd;
  ge_setup.ypd    = ypd;

  return( 0);
}

/***************************************************************************
* YaIPS_RGB_Geo_TransMM
* Make transformation matrix and create destination image.
*
* pGeMatrix,              // Out: transformation matrix
* pSrc                    // In: Source image
* xxDst, int yyDst        // Size for destination. Any 0: use size of source. Any < 0: adapt size.
* phi,                    // Angle
* xscal, yscal,           // Scale
* xshift, yshift          // Shift
* xshear, yshear          // Shear
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

typedef struct Sge_matrix {
  int   progid;                   /* program identifier */
  int   xxs;                      /* size of source, */
  int   yys;
  int   xxd;                      /* size of destination */
  int   yyd;                      /* where A and B are evaluated from */
  int   pivmod;                   /* pivot mode */
  float xps;                      /* pivot point of source*/
  float yps;
  float xpd;                      /* pivot point of destination */
  float ypd;
  lfloat MAT[12];                 /* matrix values, use depending on progid*/
} Tge_matrix;

#define DEGtoRAD         ( (double)M_PI_4 / (double)45. )

// program identifier
#define GE_GEOP     0      /* parallel projection */
#define GE_GEOX     1      /* 1D transformation in x */
#define GE_GEOC     2      /* central projection */
#define GE_GEO3D    3      /* 3D projection */

int YaIPS_RGB_Geo_TransMM( Tge_matrix *pGeMatrix,        // Out: transformation matrix
                           Fl_RGB_Image *pSrc,           // In: Source image
                           int xxDst, int yyDst,         // Size for destination. Any 0: use size of source. Any < 0: adapt size.
                           float phi,                    // Angle
                           float xscal, float yscal,     // Scale
                           float xshift, float yshift)   // Shift
{
  YaIPS_RGB_ImgD_t iSrc;
  float      xps, yps, xpd, ypd;
  int        xxs, xxd, yys, yyd;
  double     a11, a12, a21, a22, b1, b2;
  int        pivmod;
  int        ierr;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data

  xxs = iSrc.xx;
  yys = iSrc.yy;

  // Create destination image
  // NOTE: nByt must be >= 1 and <= 4 here

  if( xxDst == 0 || yyDst == 0) {          // Any of destination sizes is zero

    // Use source size

    xxd = xxs;
    yyd = yys;

  } else if( xxDst < 0 || yyDst < 0) {    // Any of destination sizes is < zero

    // Adapt size

    xxd = xxs;
    yyd = yys;

  } else {                                // Otherwise

    // Use the arguments

    xxd = xxDst;
    yyd = yyDst;
  }

  pivmod  = ge_setup.pivmod;
  xps     = ge_setup.xps;
  yps     = ge_setup.yps;
  xpd     = ge_setup.xpd;
  ypd     = ge_setup.ypd;

  if( xxDst < 0 || yyDst < 0) {    // Any of destination sizes is < zero

    // Adapt size

    double a11r, a12r, a21r, a22r;   // Rotate
    double a11s, a12s, a21s, a22s;   // scale
    double xDouble1, yDouble1, xDouble2, yDouble2;

    int i, xIn, yIn, xTemp, yTemp, xMin, xMax, yMin, yMax;

    a11r = a22r = cos( - (double)phi * DEGtoRAD);
    a12r = sin( - (double)phi * DEGtoRAD);
    a21r = - a12r;

    a11s = xscal;              /*  cos(phi)/xscal */
    a12s = 0.0;                /*  sin(phi)/yscal */
    a21s = 0.0;                /* -sin(phi)/xscal */
    a22s = yscal;              /*  cos(phi)/yscal */

    for( i = 0; i < 4; i++) {

      switch( i) {

      case 0:
      default:
        xIn = 0;
        yIn = 0;
        break;

      case 1:
        xIn = xxs - 1;
        yIn = 0;
        break;

      case 2:
        xIn = 0;
        yIn = yys - 1;
        break;

      case 3:
        xIn = xxs - 1;
        yIn = yys - 1;
        break;
      }

      xDouble1 = a11r * xIn + a12r * yIn;
      yDouble1 = a21r * xIn + a22r * yIn;

      xDouble2 = a11s * xDouble1 + a12s * yDouble1;
      yDouble2 = a21s * xDouble1 + a22s * yDouble1;

      if( i == 0) {

        xTemp = (int)floor( xDouble2);
        yTemp = (int)floor( yDouble2);

        xMin = xTemp;
        yMin = yTemp;

        xTemp = (int)ceil( xDouble2);
        yTemp = (int)ceil( yDouble2);

        xMax = xTemp;
        yMax = yTemp;

      } else {

        xTemp = (int)floor( xDouble2);
        yTemp = (int)floor( yDouble2);

        if( xTemp < xMin) xMin = xTemp;
        if( yTemp < yMin) yMin = yTemp;

        xTemp = (int)ceil( xDouble2);
        yTemp = (int)ceil( yDouble2);

        if( xTemp > xMax) xMax = xTemp;
        if( yTemp > yMax) yMax = yTemp;
      }
    }

    xxd = xMax - xMin + 1;
    yyd = yMax - yMin + 1;
  }

  a11 = a22 = cos((double)phi*DEGtoRAD);
  a12 = a21 = sin((double)phi*DEGtoRAD);
  a11 /= xscal;              /*  cos(phi)/xscal */
  a12 /= yscal;              /*  sin(phi)/yscal */
  a21 /= -xscal;             /* -sin(phi)/xscal */
  a22 /= yscal;              /*  cos(phi)/yscal */

  if( pivmod == GE_PIVORG) {          /* pivots = origins */

    xps = yps = xpd =  ypd = 0.0;
    b1 = b2 = 0.0;                    /* not center of pixel 0,0 */

  } else {

    if( pivmod == GE_PIVCTR) {        /* pivots = center */
      xps = (float)xxs*.5;
      yps = (float)yys*.5;
      xpd = (float)xxd*.5;
      ypd = (float)yyd*.5;
    }

    b1 = (double)xps - a11 * (double)xpd - a12 * (double)ypd;/* subpixel */
    b2 = (double)yps - a21 * (double)xpd - a22 * (double)ypd;/* accuracy */
  }

  b1 -= (double)xshift;                          /* additional shift */
  b2 -= (double)yshift;

  pGeMatrix->progid = GE_GEOP;                   /* internal matrix */
  pGeMatrix->xxs    = xxs;     pGeMatrix->yys    = yys;
  pGeMatrix->xxd    = xxd;     pGeMatrix->yyd    = yyd;
  pGeMatrix->pivmod = pivmod;
  pGeMatrix->xps    = xps;     pGeMatrix->yps    = yps;
  pGeMatrix->xpd    = xpd;     pGeMatrix->ypd    = ypd;
  pGeMatrix->MAT[0] = a11;     pGeMatrix->MAT[1] = a12;
  pGeMatrix->MAT[2] = a21;     pGeMatrix->MAT[3] = a22;
  pGeMatrix->MAT[4] = b1;      pGeMatrix->MAT[5] = b2;

  return(0);
}

/***************************************************************************
* YaIPS_RGB_Geo_TransPM
* Parallel projection with transformation matrix.
*
* pDst         Destination image. Memory for image must be allocated !
* pSrc         Source image
* pGeMatrix,              // Out: transformation matrix
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Geo_TransPM( Fl_RGB_Image **ppDst,         // Out: Pointer to pointer to RGB image
                           Fl_RGB_Image *pSrc,    // In: Source image
                           Tge_matrix *pGeMatrix, // In: transformation matrix
                           int OutsiteColor,      // In: Color for the area outside an image
                           int OutsiteBlend)     // If set, outside area is alpha blended
{
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *p8d, *p8s1, *p8s2;
  double xsf,ysf;
  double a11, a12, a21, a22, b1, b2;
  int    x ,y;
  int    progid;
  //x/int pivmod;
  int      xxs, yys, xxd, yyd;                          /* actual sizes */
  //x/int      gxxs, gyys, gxxd, gyyd; /* sizes matrix values are calc from */
  //x/float    xps, yps, xpd, ypd;                                /* pivots */
  int ierr, iByte, xx1, yy1, xSrc, ySrc, xmSrc, xRem, yRem, TempI;
  int nByteSrc, nByteDst, AddAlpha;
  uchar Outsite_FillColor[ 4];

  // Check source
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data

  xxs      = iSrc.xx;
  yys      = iSrc.yy;
  xmSrc    = iSrc.ld;
  nByteSrc = iSrc.d;

  xx1 = xxs - 1;         // need a border of 1 for bilinear interpolation
  yy1 = yys - 1;

  // Manage alpha channel usage

  AddAlpha = false;                       // Preset no alpha

  if( nByteSrc == 1 || nByteSrc == 3) {   // Source has no alpha

    nByteDst   = nByteSrc;

    if( OutsiteBlend) {                   // Must add alpha

      nByteDst += 1;

      AddAlpha = true;                    // Has to add an alpha channel
    }

  } else {                                // Source has alpha

    nByteDst   = nByteSrc;
  }

  // Create/Check destination image
  ierr = YaIPS_RGB_ImageSetSize( ppDst, pGeMatrix->xxd, pGeMatrix->yyd, nByteDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_to_ImgD( *ppDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  xxd = iDst.xx;
  yyd = iDst.yy;

  progid = pGeMatrix->progid;
  //x/gxxs   = pGeMatrix->xxs;        gyys = pGeMatrix->yys;
  //x/gxxd   = pGeMatrix->xxd;        gyyd = pGeMatrix->yyd;
  //x/pivmod = pGeMatrix->pivmod;
  //x/xps    = pGeMatrix->xps;        yps  = pGeMatrix->yps;
  //x/xpd    = pGeMatrix->xpd;        ypd  = pGeMatrix->ypd;
  a11    = pGeMatrix->MAT[0];     a12  = pGeMatrix->MAT[1];
  a21    = pGeMatrix->MAT[2];     a22  = pGeMatrix->MAT[3];
  b1     = pGeMatrix->MAT[4];     b2   = pGeMatrix->MAT[5];

  if( progid != GE_GEOP) {
    if( errstring == NULL) {                  // No error until now
      errstring = LangStringLookup( "&RGB_Geo_TransPM2=Transformation matrix not valid for this routine");
    }
    return(-12);
  }

  // Prepare outside fill color

  SetOutsiteColor( Outsite_FillColor, nByteSrc, OutsiteColor, OutsiteBlend);

  // ...

  for( y = 0; y < yyd; y++) {

    p8d = RGB_pixad( 0, y, &iDst);

    /* calculate from center of a dest pixel to exact src point */
#ifdef use_again
    xsf = a11 * .5 + a12 * ((double)y + .5) + b1;
    ysf = a21 * .5 + a22 * ((double)y + .5) + b2;
#else
    // 07.06.2025 RR: Use left upper corner of pixel
    // ==> For a 1:1 transformation we get a copy of the source image
    xsf = /* a11 * .5 + */ a12 * ((double)y /* + .5 */) + b1;
    ysf = /* a21 * .5 + */ a22 * ((double)y /* + .5 */) + b2;
#endif

    /* avoid switching in innermost loop */

    for( x = 0; x < xxd; p8d += nByteDst, x++, xsf += a11, ysf += a21)  {

      // float coordinates to integer
      // + remainder (for bilinear interpolation), has 4 afterpoint bits

      xSrc = (int)(xsf * 16.0);           // with afterpoint digits
      TempI = xSrc & 0xfffffff0;          // next lowest without afterpoint
      xRem  = xSrc - TempI;               // remainder
      xSrc  = xSrc >> 4;                  // before point

      ySrc = (int)(ysf * 16.0);          // with afterpoint digits
      TempI = ySrc & 0xfffffff0;          // next lowest without afterpoint
      yRem  = ySrc - TempI;               // remainder
      ySrc  = ySrc >> 4;                  // before point

      // get data from source image
      // NOTE: We keep a minimum distance of 1 from right/lower image side
      //       for bilinear interpolation.

      if( xSrc < 0 || ySrc < 0 || xSrc >= xx1 || ySrc >= yy1) { // out of source image

        if( xSrc == xx1) {                  // Last pixel in line

          if( ySrc >= 0 && ySrc < yy1) {    // One more line above

            p8s1 = (uchar *)RGB_pixad( xSrc, ySrc, &iSrc);
            p8s2 = p8s1 + xmSrc;

            for( iByte = 0; iByte < nByteSrc; iByte++) {

              // make bilinear interpolation

              p8d[ iByte] =  ( p8s1[0] * (16 - yRem) +
                               p8s2[0] * yRem) >> 4;

              p8s1++;
              p8s2++;
            }

            if( AddAlpha) {          // Inside image and need to set alpha

              p8d[ iByte] = 255;
            }

          } else if( ySrc == yy1) {                 // Left upper corner pixel

            p8s1 = (uchar *)RGB_pixad( xSrc, ySrc, &iSrc);

            for( iByte = 0; iByte < nByteSrc; iByte++) {

              // make bilinear interpolation

              p8d[ iByte] =  p8s1[0];

              p8s1++;
            }

            if( AddAlpha) {          // Inside image and need to set alpha

              p8d[ iByte] = 255;
            }

          } else {

            // out of source image, use fill color
            for( iByte = 0; iByte < nByteDst; iByte++) {
              p8d[ iByte] = Outsite_FillColor[ iByte];
            }
          }

        } else if( ySrc == yy1) {                 // Last line

          if( xSrc >= 0 && xSrc < xx1) {

            p8s1 = (uchar *)RGB_pixad( xSrc, ySrc, &iSrc);

            for( iByte = 0; iByte < nByteSrc; iByte++) {

              // make bilinear interpolation

              p8d[ iByte] = ( p8s1[0] * (16 - xRem) + p8s1[ nByteSrc] * xRem) >> 4;

              p8s1++;
            }

            if( AddAlpha) {          // Inside image and need to set alpha

              p8d[ iByte] = 255;
            }

          } else {

            // out of source image, use fill color
            for( iByte = 0; iByte < nByteDst; iByte++) {
              p8d[ iByte] = Outsite_FillColor[ iByte];
            }

          }

        } else {

          // out of source image, use fill color
          for( iByte = 0; iByte < nByteDst; iByte++) {
            p8d[ iByte] = Outsite_FillColor[ iByte];
          }
        }
      } else {

        // inside source image

        p8s1 = (uchar *)RGB_pixad( xSrc, ySrc, &iSrc);
        p8s2 = p8s1 + xmSrc;

        for( iByte = 0; iByte < nByteSrc; iByte++) {

          // make bilinear interpolation

          p8d[ iByte] = (((( p8s1[0] * (16 - xRem) + p8s1[nByteSrc] * xRem) >> 4) * (16 - yRem)) +
                         ((( p8s2[0] * (16 - xRem) + p8s2[nByteSrc] * xRem) >> 4) * yRem)) >> 4;

          p8s1++;
          p8s2++;
        }

        if( AddAlpha) {          // Inside image and need to set alpha

          p8d[ iByte] = 255;
        }
      }
    }
  }

  return(0);
}

/***************************************************************************
* YaIPS_RGB_Geo_Rotate
* Rotation and translation
*
* ppDst            Pointer to pointer to RGB image
* pSrc             Source image
* Rotation         Rotation clockwise [Degree]
* xxDst, yyDst     // Size for destination. Any 0: use size of source. Any < 0: adapt size.
*
*                  Optional pivot points. If all points < 0 the center of the images are used.
* xSrc, ySrc       Arbitrary center of rotation in source image
* xdst, yDst       Arbitrary center of rotations in destination image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

// Rotation and translation
int YaIPS_RGB_Geo_Rotate( Fl_RGB_Image **ppDst,          // Out: Pointer to pointer to RGB image
                          Fl_RGB_Image *pSrc,            // Source image
                          int OutsiteColor,              // Color for the area outside an image
                          int OutsiteBlend,         // If set, outside area is alpha blended
                          float Rotation,                // Rotation clockwise [Degree]
                          int xxDst, int yyDst,          // // Size for destination. Any 0: use size of source. Any < 0: adapt size.
                          float xps, float yps,          // Center of rotation in source image
                          float xpd, float ypd)          // Center of rotations in destination image
{
  Tge_matrix GeMatrix;
  int ierr;

  // Setup transformation
  if( xps < 0 && yps < 0 && xpd < 0 && ypd < 0) {     // Use center of images

    YaIPS_RGB_Geo_TransSetup( GE_PIVCTR, 0.0, 0.0, 0.0, 0.0);

  } else {                                                // Arbitrary pivot point

    YaIPS_RGB_Geo_TransSetup( GE_PIVPAR, xps, yps, xpd, ypd);
  }

  //Make transformation matrix and create destination image.
  ierr = YaIPS_RGB_Geo_TransMM( &GeMatrix, pSrc, xxDst, yyDst,
                                Rotation, 1.0, 1.0, 0.0, 0.0);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_Geo_TransPM( ppDst, pSrc, &GeMatrix, OutsiteColor, OutsiteBlend);    /* may return pos retval */

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_Geo_Transform
* Image transformation by parallel projection.
*
* ppDst            Pointer to pointer to RGB image
* pSrc             Source image
* Rotation         Rotation clockwise [Degree]
* xscal, yscal,    Scale image horizontal and vertical
* xshift, yshift,  Shift image horizontal and vertical
* xshear, yshear,  Shear image horizontal and vertical
* xxDst, yyDst     Size for destination image. If <= 0 the source size is used
*
*                  Optional pivot points. If all points < 0 the center of the images are used.
* xSrc, ySrc       Arbitrary center of rotation in source image
* xdst, yDst       Arbitrary center of rotations in destination image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Geo_Transform( Fl_RGB_Image **ppDst,        // Out: Pointer to pointer to RGB image
                            Fl_RGB_Image *pSrc,           // Source image
                            int OutsiteColor,             // Color for the area outside an image
                            int OutsiteBlend,             // If set, outside area is alpha blended
                            float Rotation,               // Rotation clockwise [Degree]
                            float xscal, float yscal,     // Scale image horizontal and vertical
                            float xshift, float yshift,   // Shift image horizontal and vertical
                            int xxDst, int yyDst,         // Size for destination. Any 0: use size of source. Any < 0: adapt size.
                            float xps, float yps,         // Center of rotation in source image
                            float xpd, float ypd)         // Center of rotations in destination image
{
  Tge_matrix GeMatrix;
  int ierr;

  // Setup transformation
  if( xps < 0 && yps < 0 && xpd < 0 && ypd < 0) {     // Use center of images

    YaIPS_RGB_Geo_TransSetup( GE_PIVCTR, 0.0, 0.0, 0.0, 0.0);

  } else {                                                // Arbitrary pivot point

    YaIPS_RGB_Geo_TransSetup( GE_PIVPAR, xps, yps, xpd, ypd);
  }

  //Make transformation matrix and create destination image.
  ierr = YaIPS_RGB_Geo_TransMM( &GeMatrix, pSrc, xxDst, yyDst,
                                Rotation, xscal, yscal, xshift, yshift);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_Geo_TransPM( ppDst, pSrc, &GeMatrix, OutsiteColor, OutsiteBlend);    /* may return pos retval */

  return( ierr);
}

/******************************** End Of File ********************************/

