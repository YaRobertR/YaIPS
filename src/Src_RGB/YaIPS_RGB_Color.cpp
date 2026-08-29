/****************************************************************************

  YaIPS_RGB_Color.cpp

  Fl_RGB_Image image processing.
  Color things

 01.06.2025 RR: First edition of this file.

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

#define C1        4142         /* (int32)(sqrt(2) - 1) * 10000) */
#define C2        5858         /* (int32)(2 - sqrt(2)) * 10000) */
#define CSCORR     144         /* 255/max(u,v) fuer R|G|B != 0  */
                               /* correction for full range of cs */

#define F10000     9961
#define F1000      996
          /* angles : */
#define A90        64              /*  90 degrees */
#define A180       128             /* 180 degrees */
#define A270       192             /* -90 degrees */

#define lbclip(inval,outptr)  {if ((inval) > 255L)                      \
                               *(outptr) = 255L; else if ((inval) < 0L) \
                               *(outptr) = 0L;   else *(outptr)=(inval); }

#define lmclip(inval,outptr)  {if ((inval) > 127L)                          \
                               *(outptr) = 127L;  else if ((inval) < -128L) \
                               *(outptr) = -128L; else *(outptr)=(inval); }

#define abs(x)                ( ((x) < 0)  ?  -(x) : (x) )

//------------------ Lookup tables for sin/cos and tan --------------------

uchar YaIPS_ColMod_atantab[256] = {
    0,    5,   10,   15,   19,   23,   26,   29,
   32,   34,   37,   38,   40,   42,   43,   44,
   45,   46,   47,   48,   48,   49,   50,   50,
   51,   51,   52,   52,   53,   53,   53,   54,
   54,   54,   55,   55,   55,   55,   56,   56,
   56,   56,   56,   57,   57,   57,   57,   57,
   57,   57,   58,   58,   58,   58,   58,   58,
   58,   58,   58,   59,   59,   59,   59,   59,
   59,   59,   59,   59,   59,   59,   59,   59,
   59,   60,   60,   60,   60,   60,   60,   60,
   60,   60,   60,   60,   60,   60,   60,   60,
   60,   60,   60,   60,   60,   61,   61,   61,
   61,   61,   61,   61,   61,   61,   61,   61,
   61,   61,   61,   61,   61,   61,   61,   61,
   61,   61,   61,   61,   61,   61,   61,   61,
   61,   61,   61,   61,   61,   61,   61,   61
};

uchar YaIPS_ColMod_sintab[64] = {
     0,    6,   12,   18,   25,   31,   37,   43,
    49,   56,   62,   68,   74,   80,   86,   92,
    97,  103,  109,  115,  120,  126,  131,  136,
   142,  147,  152,  157,  162,  167,  171,  176,
   181,  185,  189,  193,  197,  201,  205,  209,
   212,  216,  219,  222,  225,  228,  231,  234,
   236,  238,  241,  243,  244,  246,  248,  249,
   251,  252,  253,  254,  254,  255,  255,  255,
};

uchar YaIPS_ColMod_costab[64] = {
   255,  255,  255,  254,  254,  253,  252,  251,
   249,  248,  246,  244,  243,  241,  238,  236,
   234,  231,  228,  225,  222,  219,  216,  212,
   209,  205,  201,  197,  193,  189,  185,  181,
   176,  171,  167,  162,  157,  152,  147,  142,
   136,  131,  126,  120,  115,  109,  103,   97,
    92,   86,   80,   74,   68,   62,   56,   49,
    43,   37,   31,   25,   18,   12,    6,    0,
};

/************************************************************************************
 * YaIPS_RGB_Color_RGB2IHS
 *
 * Convert color RGB to IHS
 *
 * pIHS   Out: Pointer to three Bytes for IHS value
 * pRGB   In: Pointer to three bytes RGB value
 *
 */

void YaIPS_RGB_Color_RGB2IHS( uchar *pIHS,    // Out: Pointer to three Bytes for IHS value
                              uchar *pRGB)    // In: Pointer to three bytes RGB value
{
  int ci, cu, cv, tmp, ch, cs, acu, acv;

  // RGB -> YUV
  ci = (2990 * pRGB[ 0] + 5870 * pRGB[ 1] + 1140 * pRGB[ 2]) / F10000;
  cu = (6000 * pRGB[ 0] - 2800 * pRGB[ 1] - 3200 * pRGB[ 2]) / F1000;
  cv = (2100 * pRGB[ 0] - 5200 * pRGB[ 1] + 3100 * pRGB[ 2]) / F1000;
  // YUV -> IHS
  acu = abs(cu);
  acv = abs(cv);
  if (cv == 0 && cu == 0) {
    ch = 0;
  } else if (cu == 0) {
    ch = (cv < 0)  ?  -A180 : 0;
  } else if (cv == 0) {
    ch = (cu < 0)  ?  -A90  : A90;
  } else {
    tmp = (acu > acv) ? ((acu<<3)/acv) : ((acv<<3)/acu);
    if      (tmp >=  920) ch = A90;
    else if (tmp >=  218) ch = A90 - 1;
    else if (tmp >=  131) ch = A90 - 2;
    else if (tmp >=  128) ch = A90 - 3;
    else                  ch = YaIPS_ColMod_atantab[tmp];
    if (acu < acv)        ch = A90 - ch;
    if (cu < 0)  ch = (cv < 0)  ?  (ch-A180) : (-ch);
    else if (cv < 0)  ch = A180 - ch;
  }
  cs = (((C1 * (acu + acv)) / 10000
       + (C2 * ((acu > acv) ? acu : acv)) / 10000) * CSCORR ) / 1000;

  lbclip(ci, pIHS + 0);
  lmclip(ch, pIHS + 1);
  lbclip(cs, pIHS + 2);
}

/************************************************************************************
 * YaIPS_RGB_Color_RGB2IHS
 *
 * Convert color RGB to IHS
 *
 */

void YaIPS_RGB_Color_RGB2IHS( int *pI, int *pH, int *pS,    // Out: Converted IHS value
                              int R, int G, int B)          // In: RGB value, range 0 ... 255
{
  uchar IHS[ 3];
  uchar RGB[ 3];

  RGB[ 0] = R;
  RGB[ 1] = G;
  RGB[ 2] = B;

  YaIPS_RGB_Color_RGB2IHS( IHS, RGB);

  *pI = IHS[ 0];
  *pH = IHS[ 1];
  *pS = IHS[ 2];
}

/************************************************************************************
 * YaIPS_RGB_Color_IHS2RGB
 *
 * Convert color IHS to RGB
 *
 * pIHS   Out: Pointer to three Bytes for IHS value
 * pRGB   In: Pointer to three bytes RGB value
 *
 */

void YaIPS_RGB_Color_IHS2RGB( uchar *pRGB,    // Out: Pointer to three bytes RGB value
                              uchar *pIHS)    // In: Pointer to three Bytes for IHS value
{
  int cr, cg, cb ,cy ,cu ,cv, ci, ch, cs, tch;

  ci = pIHS[ 0];
  ch = (signed char)pIHS[ 1];    // Is a signed value
  cs = pIHS[ 2];

  cy = ci;
  tch = abs(ch);                                 /* sin */
  if ( tch >= A90 )   cu = cs * YaIPS_ColMod_sintab[A180-tch];
  else                cu = cs * YaIPS_ColMod_sintab[tch];
  if ( ch < 0 )       cu = -cu;
  cu = (cu * 100) / (255 * CSCORR);
  if ( ch < 0 )      ch = -ch;                   /* cos */
  if ( ch >= A90)    cv = cs * YaIPS_ColMod_costab[A180-tch];
  else               cv = cs * YaIPS_ColMod_costab[tch];
  cv = (cv * 100) / (255 * CSCORR);
  if ( ch > A90 )       cv = -cv;

  cr = (1000 * cy +  886 * cu +  590 * cv) / 1000;
  cg = (1000 * cy -  330 * cu -  630 * cv) / 1000;
  cb = (1000 * cy - 1166 * cu + 1750 * cv) / 1000;

  lbclip( cr, pRGB + 0);
  lbclip( cg, pRGB + 1);
  lbclip( cb, pRGB + 2);
}

/************************************************************************************
 * YaIPS_RGB_Color_IHS2RGB
 *
 * Convert color IHS to RGB
 *
 */

void YaIPS_RGB_Color_IHS2RGB( int *pR, int *pG, int *pB,    // Out: Converted RGB value
                              int I, int H, int S)          // In: IHS value, range 0 ... 255
{
  uchar IHS[ 3];
  uchar RGB[ 3];

  IHS[ 0] = I;
  IHS[ 1] = H;
  IHS[ 2] = S;

  YaIPS_RGB_Color_IHS2RGB( RGB, IHS);

  *pR = RGB[ 0];
  *pG = RGB[ 1];
  *pB = RGB[ 2];
}

/***************************************************************************
* YaIPS_RGB_Color_RGB2IHS
*
* Convert RGB image to IHS
*
* ppDst        Out: Pointer to pointer to IHS image
* pSrc         In: Pointer to RGB image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Color_RGB2IHS( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to IHS image
                             Fl_RGB_Image *pSrc)   // In: Pointer to RGB image
{
  int ierr, x, y, nByt;
  int xmin, ymin;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  if( iSrc.d < 3) {        // This works only for color images

    if( errstring == NULL) {                   // No error until now

      sprintf( errbuffer, LangStringLookup( "&RGB_Color_NoRGB=No color image!"));

      errstring = errbuffer;
    }

    return( -100);
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

  xmin = iSrc.xx;
  ymin = iSrc.yy;
  nByt = iSrc.d;

  // Convert ...

  for( y = 0; y < ymin; y++) {

    s8 = RGB_pixad( 0, y, &iSrc);
    d8 = RGB_pixad( 0, y, &iDst);

    for( x = 0; x < xmin; x++) {

      YaIPS_RGB_Color_RGB2IHS( d8, s8);

      if( nByt > 3) {   // Has an alpha

        d8[ 3] = s8[ 3];
      }

      s8 += nByt;
      d8 += nByt;
    }
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Color_IHS2RGB
*
* Convert IHS image to RGB
*
* ppDst        Out: Pointer to pointer to RGB image
* pSrc         In: Pointer to IHS image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Color_IHS2RGB( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                             Fl_RGB_Image *pSrc)   // In: Pointer to IHS image
{
  int ierr, x, y, nByt;
  int xmin, ymin;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  if( iSrc.d < 3) {        // This works only for color images

    if( errstring == NULL) {                   // No error until now

      sprintf( errbuffer, LangStringLookup( "&RGB_Color_NoRGB=No color image!"));

      errstring = errbuffer;
    }

    return( -100);
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

  xmin = iSrc.xx;
  ymin = iSrc.yy;
  nByt = iSrc.d;

  // Convert ...

  for( y = 0; y < ymin; y++) {

    s8 = RGB_pixad( 0, y, &iSrc);
    d8 = RGB_pixad( 0, y, &iDst);

    for( x = 0; x < xmin; x++) {

      YaIPS_RGB_Color_IHS2RGB( d8, s8);

      if( nByt > 3) {   // Has an alpha

        d8[ 3] = s8[ 3];
      }

      s8 += nByt;
      d8 += nByt;
    }
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Color_IHS_Adjust
*
* Adjust intensity, hue and saturation
*
* ppDst          Out: Pointer to pointer to IHS image
* pSrc           In: Pointer to RGB image
* IHS_IntAdjust  In: Intensity adjust +- 100 %
* IHS_HueAdjust  In: hue adjust +- 100 %
* IHS_SatAdjust  In: saturation adjust +- 100 %
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Color_IHS_Adjust( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to IHS image
                                Fl_RGB_Image *pSrc,   // In: Pointer to RGB image
                                int IHS_IntAdjust,    // In: Intensity adjust +- 100 %
                                int IHS_HueAdjust,    // In: hue adjust +- 100 %
                                int IHS_SatAdjust)    // In: saturation adjust +- 100 %
{
  int ierr, x, y, nByt;
  int xmin, ymin, t, Int_Multiply, Sat_Multiply, Hue_Add;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8, Pixes_IHS[ 3];

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  if( iSrc.d < 3) {        // This works only for color images

    if( errstring == NULL) {                   // No error until now

      sprintf( errbuffer, LangStringLookup( "&RGB_Color_NoRGB=No color image!"));

      errstring = errbuffer;
    }

    return( -100);
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

  xmin = iSrc.xx;
  ymin = iSrc.yy;
  nByt = iSrc.d;

  // Prepare conversion

  Int_Multiply = ((100 + IHS_IntAdjust) * (1 << 10) + 50) / 100;     // Percent to factor multiplied with 1024
  if( Int_Multiply < 0) {
    Int_Multiply = 0;
  }

  Sat_Multiply = ((100 + IHS_SatAdjust) * (1 << 10) + 50) / 100;     // Percent to factor multiplied with 1024
  if( Sat_Multiply < 0) {
    Sat_Multiply = 0;
  }

  if( IHS_HueAdjust < -100) IHS_HueAdjust = -100; // Clip range
  if( IHS_HueAdjust >  100) IHS_HueAdjust =  100;
  Hue_Add = (IHS_HueAdjust * 128 + 50) / 100;

  // Convert ...

  for( y = 0; y < ymin; y++) {

    s8 = RGB_pixad( 0, y, &iSrc);
    d8 = RGB_pixad( 0, y, &iDst);

    for( x = 0; x < xmin; x++) {

      YaIPS_RGB_Color_RGB2IHS( Pixes_IHS, s8);   // RGB --> IHS

      // Intensity
      t = Pixes_IHS[ 0];
      t = (t * Int_Multiply + (1 << 9)) >> 10;
      if( t > 255) {
        t = 255;
      }
      Pixes_IHS[ 0] = t;

      // Hue
      t = Pixes_IHS[ 1] + Hue_Add;
      if( t > 255) {
        t -= 256;
      } else if( t < 0) {
        t += 256;
      }
      Pixes_IHS[ 1] = t;

      // Saturation
      t = Pixes_IHS[ 2];
      t = (t * Sat_Multiply + (1 << 9)) >> 10;
      if( t > 255) {
        t = 255;
      }
      Pixes_IHS[ 2] = t;

      YaIPS_RGB_Color_IHS2RGB( d8, Pixes_IHS);   // IHS --> RGB

      if( nByt > 3) {   // Has an alpha

        d8[ 3] = s8[ 3];
      }

      s8 += nByt;
      d8 += nByt;
    }
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Color_ConvSimple
*
* Simple color conversions
*
* ppDst        Out: Pointer to pointer to destination image
* pSrc         In: Pointer to source image
* ColorOp      In: what to do, see #define YAIPS_COLOR_xxx
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Color_ConvSimple( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to destination image
                                Fl_RGB_Image *pSrc,   // In: Pointer to source image
                                int ColorOp)          // In: what to do, see #define YAIPS_COLOR_xxx
{
  int ierr, x, y, SrcD, DstD;
  int xmin, ymin;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8, t;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  if( ColorOp == YAIPS_COLOR_GET_ALPHA) {     // Image must have an alpha component

    if( iSrc.d != 2 && iSrc.d != 4) {         // Has no alpha

      if( errstring == NULL) {                // No error until now

        sprintf( errbuffer, LangStringLookup( "&RGB_Color_NoAlpha=No alpha channel!"));

        errstring = errbuffer;
      }

      return( -100);
    }

  } else {                                   // Must be a color image

    if( iSrc.d < 3) {        // This works only for color images

      if( errstring == NULL) {                   // No error until now

        sprintf( errbuffer, LangStringLookup( "&RGB_Color_NoRGB=No color image!"));

        errstring = errbuffer;
      }

      return( -101);
    }
  }

  // Bytes per pixel for destination

  if( ColorOp == YAIPS_COLOR_RGB_2_BGR) {    // Convert RGB to BGR image or vice versa

    DstD = iSrc.d;                           // Output has same bytes per pixel

  } else {

    // Others have only one byte per pixel

    DstD = 1;
  }

  // Ensure that pPDst image has the same size and 1 byte per pixel
  ierr = YaIPS_RGB_ImageSetSize( ppDst, iSrc.xx, iSrc.yy, DstD);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data

  xmin = iSrc.xx;
  ymin = iSrc.yy;
  SrcD = iSrc.d;

  // Convert ...

  for( y = 0; y < ymin; y++) {

    s8 = RGB_pixad( 0, y, &iSrc);
    d8 = RGB_pixad( 0, y, &iDst);

    switch( ColorOp) {                     // Minimum

    case YAIPS_COLOR_RGB_2_R:    // Get red color component

      for( x = 0; x < xmin; x++) {
        *d8++ = s8[ 0];
        s8 += SrcD;
      }

      break;

    case YAIPS_COLOR_RGB_2_G:    // Get green color component

      for( x = 0; x < xmin; x++) {
        *d8++ = s8[ 1];
        s8 += SrcD;
      }

      break;

    case YAIPS_COLOR_RGB_2_B:    // Get blue color component

      for( x = 0; x < xmin; x++) {
        *d8++ = s8[ 2];
        s8 += SrcD;
      }

      break;

    case YAIPS_COLOR_GET_ALPHA:    // Get alpha from BW or RGB image

      if( iSrc.d == 2) {           // BW image with alpha

        for( x = 0; x < xmin; x++) {
          *d8++ = s8[ 1];
          s8 += SrcD;
        }

      } else {                     // Must be color image with alpha

        for( x = 0; x < xmin; x++) {
          *d8++ = s8[ 3];
          s8 += SrcD;
        }
      }

      break;

    case YAIPS_COLOR_RGB_2_I:    // Convert image RGB to intensity

      for( x = 0; x < xmin; x++) {

        // A fast black white conversion. Intensity part of an IHS conversion.
        *d8++ = (s8[ 0] * 76 + s8[ 1] * 150 + s8[ 2] * 30) >> 8;
        s8 += SrcD;
      }

      break;

    case YAIPS_COLOR_RGB_2_H:    // Convert image RGB to hue
      {
        int cu, cv, tmp, ch, acu, acv;

        for( x = 0; x < xmin; x++) {

          // Code stripped down from YaIPS_RGB_Color_RGB2IHS()

          // RGB -> UV
          cu = (6000 * s8[ 0] - 2800 * s8[ 1] - 3200 * s8[ 2]) / F1000;
          cv = (2100 * s8[ 0] - 5200 * s8[ 1] + 3100 * s8[ 2]) / F1000;
          // YUV -> IHS
          acu = abs(cu);
          acv = abs(cv);
          if (cv == 0 && cu == 0) {
            ch = 0;
          } else if (cu == 0) {
            ch = (cv < 0)  ?  -A180 : 0;
          } else if (cv == 0) {
            ch = (cu < 0)  ?  -A90  : A90;
          } else {
            tmp = (acu > acv) ? ((acu<<3)/acv) : ((acv<<3)/acu);
            if      (tmp >=  920) ch = A90;
            else if (tmp >=  218) ch = A90 - 1;
            else if (tmp >=  131) ch = A90 - 2;
            else if (tmp >=  128) ch = A90 - 3;
            else                  ch = YaIPS_ColMod_atantab[tmp];
            if (acu < acv)        ch = A90 - ch;
            if (cu < 0)  ch = (cv < 0)  ?  (ch-A180) : (-ch);
            else if (cv < 0)  ch = A180 - ch;
          }

          lmclip( ch, d8);

          d8 += 1;
          s8 += SrcD;
        }
      }
      break;

    case YAIPS_COLOR_RGB_2_S:    // Convert image RGB to saturation
      {
        int cu, cv, cs, acu, acv;

        for( x = 0; x < xmin; x++) {

          // Code stripped down from YaIPS_RGB_Color_RGB2IHS()

          // RGB -> UV
          cu = (6000 * s8[ 0] - 2800 * s8[ 1] - 3200 * s8[ 2]) / F1000;
          cv = (2100 * s8[ 0] - 5200 * s8[ 1] + 3100 * s8[ 2]) / F1000;

          acu = abs(cu);
          acv = abs(cv);

          cs = (((C1 * (acu + acv)) / 10000
               + (C2 * ((acu > acv) ? acu : acv)) / 10000) * CSCORR ) / 1000;

          lbclip( cs, d8);

          d8 += 1;
          s8 += SrcD;
        }
      }
      break;

    case YAIPS_COLOR_RGB_MIN:    // Minimum of color components

     for( x = 0; x < xmin; x++) {

       t = s8[ 0];
       if( s8[ 1] < t) t = s8[ 1];
       if( s8[ 2] < t) t = s8[ 2];

       *d8++ = t;
       s8 += SrcD;
     }

     break;

    case YAIPS_COLOR_RGB_MAX:    // Maximum of color components

     for( x = 0; x < xmin; x++) {

       t = s8[ 0];
       if( s8[ 1] > t) t = s8[ 1];
       if( s8[ 2] > t) t = s8[ 2];

       *d8++ = t;
       s8 += SrcD;
     }

     break;

    case YAIPS_COLOR_RGB_2_BGR:    // Convert RGB to BGR image or vice versa

      if( *ppDst == pSrc) {        // Output and input image is the same image

        for( x = 0; x < xmin; x++) {

          // Source and destination has the same bytes per pixel value.
          // If always have a color image here. Alpha is supported.

          t = s8[ 0];
          d8[ 0] = s8[ 2];
          d8[ 2] = t;

          // No need to process alpha

          d8 += DstD;
          s8 += SrcD;
        }

      } else {

        for( x = 0; x < xmin; x++) {

          // Source and destination has the same bytes per pixel value.
          // If always have a color image here. Alpha is supported.

          d8[ 0] = s8[ 2];
          d8[ 1] = s8[ 1];
          d8[ 2] = s8[ 0];

          if( SrcD > 3) {    // Has alpha

            d8[ 3] = s8[ 3];
          }

          d8 += DstD;
          s8 += SrcD;
        }
      }
    }  // end switch

  }  // end for( y ...

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Color_DestD
*
* Convert number of bytes.
*
* ppDst        Out: Pointer to pointer to destination image
* pSrc         In: Pointer to source image
* Dst_nBytes   In: # bytes of the destination image
* Alpha        In: Optional alpha value. Depends from bytes per pixel.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Color_DestD( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to destination image
                           Fl_RGB_Image *pSrc,   // In: Pointer to source image
                           int Dst_nBytes,       // In: # bytes of the destination image
                           int Alpha)            // In: Optional alpha value. Depends from bytes per pixel.
{
  int ierr, x, y, SrcS, SrcD, BW_S, BW_D, Alpha_S, Alpha_D;
  int xmin, ymin;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8;

  if( Dst_nBytes < 1 || Dst_nBytes > 4) {     // Check # bytes is OK

    return( -100);                            // Return error
  }

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  if( Dst_nBytes == iSrc.d) {                 // Has same number of bytes

    ierr = YaIPS_RGB_CopyImg( ppDst, pSrc);   // Make a simple image copy

    return( ierr);
  }

  // Bytes per pixel are different

  // Set size and bytes per pixel for destination image
  ierr = YaIPS_RGB_ImageSetSize( ppDst, iSrc.xx, iSrc.yy, Dst_nBytes);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data

  xmin = iSrc.xx;
  ymin = iSrc.yy;
  SrcS = iSrc.d;
  SrcD = Dst_nBytes;

  //

  BW_S = SrcS <= 2;                 // Image is black white
  BW_D = SrcD <= 2;

  Alpha_S = SrcS == 2 || SrcS == 4; // Image has an alpha channel
  Alpha_D = SrcD == 2 || SrcD == 4;

  // Convert ...

  for( y = 0; y < ymin; y++) {

    // Convert color

    s8 = RGB_pixad( 0, y, &iSrc);
    d8 = RGB_pixad( 0, y, &iDst);

    if( BW_S == true && BW_D == true) {           // Source is BW Destination is BW

      for( x = 0; x < xmin; x++) {

        d8[ 0] = s8[ 0];

        s8 += SrcS;
        d8 += SrcD;
      }

    } else if( BW_S == true && BW_D == false) {   // Source is BW Destination is RGB

      for( x = 0; x < xmin; x++) {

        d8[ 0] = s8[ 0];
        d8[ 1] = s8[ 0];
        d8[ 2] = s8[ 0];

        s8 += SrcS;
        d8 += SrcD;
      }

    } else if( BW_S == false && BW_D == true) {   // Source is RGB Destination is BW

      for( x = 0; x < xmin; x++) {

        // A fast black white conversion. Intensity part of an IHS conversion.
        d8[ 0] = (s8[ 0] * 76 + s8[ 1] * 150 + s8[ 2] * 30) >> 8;

        s8 += SrcS;
        d8 += SrcD;
      }

    } else if( BW_S == false && BW_D == false) {  // Source is RGB Destination is RGB

      for( x = 0; x < xmin; x++) {

        d8[ 0] = s8[ 0];
        d8[ 1] = s8[ 1];
        d8[ 2] = s8[ 2];

        s8 += SrcS;
        d8 += SrcD;
      }
    }

    // Convert alpha

    if( Alpha_D) {                          // Destination has alpha

      s8 = RGB_pixad( 0, y, &iSrc);
      d8 = RGB_pixad( 0, y, &iDst);

      d8 += SrcD - 1;                      // Add offset do alpha channel

      if( Alpha_S) {                       // Source has alpha to

        // Copy alpha

        s8 += SrcS - 1;                    // Add offset do alpha channel

        for( x = 0; x < xmin; x++) {

          d8[ 0] = s8[ 0];

          s8 += SrcS;
          d8 += SrcD;
        }

      } else {                             // Source has NO alpha channel

        // Set alpha argument

        for( x = 0; x < xmin; x++) {

          d8[ 0] = Alpha;

          d8 += SrcD;
        }
      }
    }

  }  // end for( y ...

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Color_ConvMatrix
*
* Some predefined color matrices.
*
****************************************************************************
*/

T_YaIPS_ColorMatrix ColorMatrix_List[] = {   // Color matrix list

  // Classic effects

  { (char *)"Classic effects/1:1",
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  },

  { (char *)"Classic effects/Invert",
    255, -1, 0, 0,
    255, 0, -1, 0,
    255, 0, 0, -1
  },

  { (char *)"Classic effects/Sepia classic",
    0, 0.39, 0.77, 0.19,
    0, 0.35, 0.69, 0.17,
    0, 0.27, 0.53, 0.13
  },

  { (char *)"Classic effects/Sepia light",
    0, 0.6, 0.4, 0.2,
    0, 0.3, 0.6, 0.1,
    0, 0.2, 0.3, 0.4
  },

  { (char *)"Classic effects/Bleach Bypass",
    0, 1.5, -0.5, -0.5,
    0, -0.5, 1.5, -0.5,
    0, -0.5, -0.5, 1.5
  },

  { (char *)"Classic effects/High Contrast",
    0, 1.4, -0.2, -0.2,
    0, -0.2, 1.4, -0.2,
    0, -0.2, -0.2, 1.4
  },

  { (char *)"Classic effects/Low Contrast",
    0, 0.8, 0.1, 0.1,
    0, 0.1, 0.8, 0.1,
    0, 0.1, 0.1, 0.8
  },

  { (char *)"Classic effects/Desaturate 1",
    0, 0.64, 0.18, 0.18,
    0, 0.18, 0.64, 0.18,
    0, 0.18, 0.18, 0.64
  },

  { (char *)"Classic effects/Desaturate 2",
    0, 0.5, 0.25, 0.25,
    0, 0.25, 0.5, 0.25,
    0, 0.25, 0.25, 0.5
  },

  { (char *)"Classic effects/Saturation",
    0, 1.3, -0.1, -0.1,
    0, -0.1, 1.3, -0.1,
    0, -0.1, -0.1, 1.3
  },

  // Tint and temperature

  { (char *)"Tint and temperature/Warm 1",
    0, 1.2, 0.1, 0,
    0, 0, 1.1, 0,
    0, 0, 0, 0.9
  },

  { (char *)"Tint and temperature/Warm 2",
    0, 1.3, 0.1, 0,
    0, 0.1, 1.2, 0,
    0, 0, 0, 0.85
  },

  { (char *)"Tint and temperature/Cold 1",
    0, 0.9, 0, 0.1,
    0, 0, 0.9, 0.1,
    0, 0.1, 0.1, 1.2
  },

  { (char *)"Tint and temperature/Cold 2",
    0, 0.8, 0, 0.2,
    0, 0, 0.85, 0.15,
    0, 0.2, 0.2, 1.3
  },

  { (char *)"Tint and temperature/Magenta Shift",
    0, 1.0, 0.2, 0.2,
    0, 0, 0.8, 0.2,
    0, 0, 0, 0.8
  },

  { (char *)"Tint and temperature/Cyan Shift",
    0, 0.8, 0.2, 0,
    0, 0.2, 1.0, 0,
    0, 0.2, 0.2, 1.0
  },

  { (char *)"Tint and temperature/Reddish Tint",
    0, 1.2, 0.1, 0.1,
    0, 0.1, 0.9, 0.1,
    0, 0.1, 0.1, 0.9
  },

  { (char *)"Tint and temperature/Greenish Tint",
    0, 0.9, 0.1, 0.1,
    0, 0.1, 1.2, 0.1,
    0, 0.1, 0.1, 0.9
  },

  { (char *)"Tint and temperature/Bluish Tint",
    0, 0.9, 0.1, 0.2,
    0, 0.1, 0.9, 0.2,
    0, 0.2, 0.2, 1.2
  },

  { (char *)"Tint and temperature/Red Boost",
    0, 1.3, 0.1, 0.0,
    0, 0, 1, 0,
    0, 0, 0, 1
  },

  { (char *)"Tint and temperature/Green Boost",
    0, 1, 0, 0,
    0, 0.1, 1.3, 0.1,
    0, 0, 0, 1
  },

  { (char *)"Tint and temperature/Blue Boost",
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0.1, 0.1, 1.3
  },

  // Film-Looks

  { (char *)"Film-Looks/Technicolor",
    0, 1.91, -0.85, -0.06,
    0, -0.54, 1.42, -0.06,
    0, 0.0, -0.02, 1.02
  },

  { (char *)"Film-Looks/Kodak 2395",
    0,  1.25, -0.25, 0.0,
    0, -0.1, 1.2, -0.1,
    0,  0.0, -0.05, 1.05
  },

  { (char *)"Film-Looks/Fuji F-125",
    0, 1.1, 0.05, -0.05,
    0, 0.0, 1.0, 0.0,
    0, -0.05, 0.0, 1.1
  },

   { (char *)"Film-Looks/Vintage Fade",
    0, 0.6, 0.2, 0.2,
    0, 0.2, 0.6, 0.2,
    0, 0.2, 0.2, 0.6
  },

  { (char *)"Film-Looks/Vintage Greenish",
    0, 0.8, 0.2, 0.1,
    0, 0.1, 0.9, 0.2,
    0, 0.1, 0.2, 0.8
  },

  { (char *)"Film-Looks/Bleached Skin Tone",
    0, 1.1, 0.1, 0.0,
    0, 0.0, 1.0, 0.0,
    0, 0.0, 0.0, 0.9
  },

  { (char *)"Film-Looks/Soft Film",
    0, 0.9, 0.05, 0.05,
    0, 0.05, 0.9, 0.05,
    0, 0.05, 0.05, 0.9
  },

  { (char *)"Film-Looks/Hard Film",
    0, 1.2, -0.1, -0.1,
    0, -0.1, 1.2, -0.1,
    0, -0.1, -0.1, 1.2
  },

  // Duotone

  { (char *)"Duotone/Black-Blue",
      0, 0.1, 0.1, 0.1,
      0, 0.1, 0.1, 0.1,
      0, 0.3, 0.6, 0.1
  },

  { (char *)"Duotone/Black-Cyan",
      0, 0.1, 0.1, 0.1,
      0, 0.3, 0.6, 0.1,
      0, 0.3, 0.6, 0.1
  },

  { (char *)"Duotone/Black-Green",
    0, 0.1, 0.1, 0.1,
    0, 0.3, 0.6, 0.1,
    0, 0.1, 0.1, 0.1
  },

  { (char *)"Duotone/Black-Red",
    0, 0.3, 0.6, 0.1,
    0, 0.1, 0.1, 0.1,
    0, 0.1, 0.1, 0.1
  },

  { (char *)"Duotone/Black-Yellow",
    0, 0.3, 0.6, 0.1,
    0, 0.3, 0.6, 0.1,
    0, 0.1, 0.1, 0.1
  },

  { (char *)"Duotone/Green-Yellow",
    0, 0.2, 0.2, 1.0,
    0, 1.0, 0.2, 0.6,
    0, 0.1, 0.1, 0.1
  },

  { (char *)"Duotone/Blue–Pink",
    0, 0.2, 1.0, 0.1,
    0, 0.2, 0.2, 0.1,
    0, 1.0, 0.6, 0.1
  },

  { (char *)"Duotone/Brown–Beige",
    0, 0.6, 0.9, 0.1,
    0, 0.3, 0.8, 0.1,
    0, 0.1, 0.6, 0.1
  },

  { (char *)"Duotone/Brown–Blue",
    0, 0.6, 0.2, 0.1,
    0, 0.3, 0.2, 0.1,
    0, 0.1, 1.0, 0.1
  },

  { (char *)"Duotone/Pink–Blue",
    0, 1.0, 0.2, 0.6,
    0, 0.2, 0.2, 1.0,
    0, 0.1, 0.1, 0.1
  },

  { (char *)"Duotone/White-Blue",
    0, 0.8, 0.6, 0.4,
    0, 0.8, 0.6, 0.4,
    0, 1.0, 1.0, 1.0
  },

  { (char *)"Duotone/White–Red",
    0, 1.0, 1.0, 1.0,
    0, 0.8, 0.6, 0.4,
    0, 0.8, 0.6, 0.4
  },

#ifdef use_again
  { (char *)"",
    0, ,
    0, ,
    0,
  },

#endif
};

int nColorMatrix_List = sizeof( ColorMatrix_List) / sizeof( T_YaIPS_ColorMatrix); // Size of color matrix list

/***************************************************************************
* YaIPS_RGB_Color_ConvMatrix
*
* Color conversion with matrix.
*
* ppDst        Out: Pointer to pointer to RGB image
* pSrc         In: Pointer to IHS image
* R_Offset, R_MultR, R_MultG, R_MultB  // In: Matrix coefficients of the red channel
* G_Offset, G_MultR, G_MultG, G_MultB  // In: Matrix coefficients of the green channel
* B_Offset, B_MultR, B_MultG, B_MultB  // In: Matrix coefficients of the blue channel
*
* Each output pixel is calculated as follows:
*   R_dst = R_Offset + (R_MultR * R_src) + (R_MultG * G_src) + (R_MultB * B_src)
*   G_dst = G_Offset + (G_MultR * R_src) + (G_MultG * G_src) + (G_MultB * B_src)
*   B_dst = B_Offset + (B_MultR * R_src) + (B_MultG * G_src) + (B_MultB * B_src)
* X_dst are the output (destination) pixels, X_src are the input (source) pixels
*
* A default matrix would is:
*   0  1.0 0.0 0.0
*   0  0.0 1.0 0.0
*   0  0.0 0.0 1.0
* This one makes no changes (copies input to output without modification).
*
*  NOTE: Input image must be a color image.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

#define YAIPS_MATRIX_MULT_SHIFT   10                                // Shift for matrix multiplier
#define YAIPS_MATRIX_MULT_FAC     (1 << YAIPS_MATRIX_MULT_SHIFT)    // Factor for matrix multiplier

int YaIPS_RGB_Color_ConvMatrix( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to IHS image
                                Fl_RGB_Image *pSrc,   // In: Pointer to RGB image
                                int R_Offset, float R_MultR, float R_MultG, float R_MultB,  // In: Matrix coefficients of the red channel
                                int G_Offset, float G_MultR, float G_MultG, float G_MultB,  // In: Matrix coefficients of the green channel
                                int B_Offset, float B_MultR, float B_MultG, float B_MultB)  // In: Matrix coefficients of the blue channel
{
  int ierr, x, y, nByte, t;
  int xmin, ymin;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8;
  int iR_MultR, iR_MultG, iR_MultB;
  int iG_MultR, iG_MultG, iG_MultB;
  int iB_MultR, iB_MultG, iB_MultB;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  if( iSrc.d < 3) {        // This works only for color images

    if( errstring == NULL) {                   // No error until now

      sprintf( errbuffer, LangStringLookup( "&RGB_Color_NoRGB=No color image!"));

      errstring = errbuffer;
    }

    return( -101);
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

  xmin = iSrc.xx;
  ymin = iSrc.yy;
  nByte = iSrc.d;

  // Prepare multipliers

  iR_MultR = (int)( R_MultR * YAIPS_MATRIX_MULT_FAC);
  iR_MultG = (int)( R_MultG * YAIPS_MATRIX_MULT_FAC);
  iR_MultB = (int)( R_MultB * YAIPS_MATRIX_MULT_FAC);
  iG_MultR = (int)( G_MultR * YAIPS_MATRIX_MULT_FAC);
  iG_MultG = (int)( G_MultG * YAIPS_MATRIX_MULT_FAC);
  iG_MultB = (int)( G_MultB * YAIPS_MATRIX_MULT_FAC);
  iB_MultR = (int)( B_MultR * YAIPS_MATRIX_MULT_FAC);
  iB_MultG = (int)( B_MultG * YAIPS_MATRIX_MULT_FAC);
  iB_MultB = (int)( B_MultB * YAIPS_MATRIX_MULT_FAC);

  // Convert ...

  for( y = 0; y < ymin; y++) {

    s8 = RGB_pixad( 0, y, &iSrc);
    d8 = RGB_pixad( 0, y, &iDst);

    for( x = 0; x < xmin; x++) {

      t = R_Offset + (((iR_MultR * s8[ 0]) + (iR_MultG * s8[ 1]) + (iR_MultB * s8[ 2])) >> YAIPS_MATRIX_MULT_SHIFT);
      lbclip( t, d8 + 0);

      t = G_Offset + (((iG_MultR * s8[ 0]) + (iG_MultG * s8[ 1]) + (iG_MultB * s8[ 2])) >> YAIPS_MATRIX_MULT_SHIFT);
      lbclip( t, d8 + 1);

      t = B_Offset + (((iB_MultR * s8[ 0]) + (iB_MultG * s8[ 1]) + (iB_MultB * s8[ 2])) >> YAIPS_MATRIX_MULT_SHIFT);
      lbclip( t, d8 + 2);

      if( nByte > 3) {   // Has an alpha

        d8[ 3] = s8[ 3];
      }

      d8 += nByte;
      s8 += nByte;
    }

  }  // end for( y ...

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Color_ConvMatrix
*
* Color conversion with matrix.
*
* ppDst        Out: Pointer to pointer to RGB image
* pSrc         In: Pointer to IHS image
* pColorMatrix In: Pointer to color matrix
*
* Each output pixel is calculated as follows:
*   R_dst = R_Offset + (R_MultR * R_src) + (R_MultG * G_src) + (R_MultB * B_src)
*   G_dst = G_Offset + (G_MultR * R_src) + (G_MultG * G_src) + (G_MultB * B_src)
*   B_dst = B_Offset + (B_MultR * R_src) + (B_MultG * G_src) + (B_MultB * B_src)
* X_dst are the output (destination) pixels, X_src are the input (source) pixels
*
* A default matrix would is:
*   0  1.0 0.0 0.0
*   0  0.0 1.0 0.0
*   0  0.0 0.0 1.0
* This one makes no changes (copies input to output without modification).
*
*  NOTE: Input image must be a color image.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Color_ConvMatrix( Fl_RGB_Image **ppDst,              // Out: Pointer to pointer to IHS image
                                Fl_RGB_Image *pSrc,                // In: Pointer to RGB image
                                T_YaIPS_ColorMatrix *pColorMatrix) // In: Pointer to color matrix
{
  int ierr;

  ierr = YaIPS_RGB_Color_ConvMatrix( ppDst, pSrc,
      pColorMatrix->R_Offset, pColorMatrix->R_MultR, pColorMatrix->R_MultG, pColorMatrix->R_MultB,  // Matrix coefficients of the red channel
      pColorMatrix->G_Offset, pColorMatrix->G_MultR, pColorMatrix->G_MultG, pColorMatrix->G_MultB,  // Matrix coefficients of the green channel
      pColorMatrix->B_Offset, pColorMatrix->B_MultR, pColorMatrix->B_MultG, pColorMatrix->B_MultB); // Matrix coefficients of the blue channel

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_Color_ConvLUT
*
* LUT table conversion
*
* ppDst        Out: Pointer to pointer to IHS image
* pSrc         In: Pointer to RGB image
* pLookupR     In: Point to red lookup table (256 bytes)
* pLookupG     In: Point to green lookup table (256 bytes)
* pLookupB     In: Point to blue lookup table (256 bytes)
* BlendBW      In: For color images only.
*                  Range = 0 ... 100. 0 = use Color pixels. 100 = use BW pixels.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Color_ConvLUT( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to IHS image
                             Fl_RGB_Image *pSrc,   // In: Pointer to RGB image
                             uchar *pLookupR,      // In: Point to red lookup table (256 bytes)
                             uchar *pLookupG,      // In: Point to green lookup table (256 bytes)
                             uchar *pLookupB,      // In: Point to blue lookup table (256 bytes)
                             int BlendBW)          // In: Used for color images only.
                                                   //     Range = 0 ... 100. 0 = use Color pixels. 100 = use BW pixels.
{
  int ierr, x, y, nBytesSrc, nBytesDst, AlphaSrc;
  int xx, yy, i, LUTs_are_different, Pixel_BW;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data

  xx = iSrc.xx;
  yy = iSrc.yy;
  nBytesSrc = iSrc.d;

  // Check that the 3 LUTs are different

  LUTs_are_different = false;

  for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {

    if( pLookupR[ i] !=  pLookupG[ i] ||
        pLookupR[ i] !=  pLookupB[ i]) {

      LUTs_are_different = true;
      break;
    }
  }

  AlphaSrc = 1 - (nBytesSrc & 0x01);    // Set to 1 if source has an alpha channel

  if( nBytesSrc >= 3 ||            // Source is color
      LUTs_are_different) {        // Source is black white and LUTs are different

    nBytesDst = 3 + AlphaSrc;      // Destination is a color image

  } else {                         // Source is a black white image and LUTs are equal

    nBytesDst = 1 + AlphaSrc;      // Destination is a black white image
  }

  // Ensure that pPDst image has the same size and proper byte per pixel
  ierr = YaIPS_RGB_ImageSetSize( ppDst, xx, yy, nBytesDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Convert ...

  for( y = 0; y < yy; y++) {

    s8 = RGB_pixad( 0, y, &iSrc);
    d8 = RGB_pixad( 0, y, &iDst);

    if( nBytesSrc >= 3) {             // Source is color, destination is color

      if( BlendBW <= 0) {             // Use Color pixels

        for( x = 0; x < xx; x++) {

          d8[ 0] = pLookupR[ s8[ 0]];
          d8[ 1] = pLookupG[ s8[ 1]];
          d8[ 2] = pLookupB[ s8[ 2]];

          if( AlphaSrc) {             // Copy alpha

            d8[ 3] = s8[ 3];
          }

          d8 += nBytesDst;
          s8 += nBytesSrc;
        }

      } else if( BlendBW >= 100) {    // Use BW pixels

        for( x = 0; x < xx; x++) {

          // A fast black white conversion. Intensity part of an IHS conversion.
          Pixel_BW = (s8[ 0] * 76 + s8[ 1] * 150 + s8[ 2] * 30) >> 8;

          d8[ 0] = pLookupR[ Pixel_BW];
          d8[ 1] = pLookupG[ Pixel_BW];
          d8[ 2] = pLookupB[ Pixel_BW];

          if( AlphaSrc) {             // Copy alpha

            d8[ 3] = s8[ 3];
          }

          d8 += nBytesDst;
          s8 += nBytesSrc;
        }

      } else {                        // Blend between color and BW pixels

        for( x = 0; x < xx; x++) {

          // A fast black white conversion. Intensity part of an IHS conversion.
          Pixel_BW = (s8[ 0] * 76 + s8[ 1] * 150 + s8[ 2] * 30) >> 8;

          d8[ 0] = ((100 - BlendBW) * pLookupR[ s8[ 0]] + BlendBW * pLookupR[ Pixel_BW]) / 100;
          d8[ 1] = ((100 - BlendBW) * pLookupG[ s8[ 1]] + BlendBW * pLookupG[ Pixel_BW]) / 100;
          d8[ 2] = ((100 - BlendBW) * pLookupB[ s8[ 2]] + BlendBW * pLookupB[ Pixel_BW]) / 100;

          if( AlphaSrc) {             // Copy alpha

            d8[ 3] = s8[ 3];
          }

          d8 += nBytesDst;
          s8 += nBytesSrc;
        }
      }

    } else if( LUTs_are_different) {  // Source is black white, destination is color

      for( x = 0; x < xx; x++) {

        d8[ 0] = pLookupR[ s8[ 0]];
        d8[ 1] = pLookupG[ s8[ 0]];
        d8[ 2] = pLookupB[ s8[ 0]];

        if( AlphaSrc) {               // Copy alpha

          d8[ 3] = s8[ 1];
        }

        d8 += nBytesDst;
        s8 += nBytesSrc;
      }

    } else {                          // Source is a black white, destination is black and white

      for( x = 0; x < xx; x++) {

        // All three LUTs are equal

        d8[ 0] = pLookupR[ s8[ 0]];

        if( AlphaSrc) {               // Copy alpha

          d8[ 1] = s8[ 1];
        }

        d8 += nBytesDst;
        s8 += nBytesSrc;
      }
    }
  }  // end for( y ...

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Color_MakeLUT_OffsetGain
*
* Make a offset + gain change LUT table
*
* pLookupR     Out: Point to red lookup table (256 bytes)
* pLookupG     Out: Point to green lookup table (256 bytes)
* pLookupB     Out: Point to blue lookup table (256 bytes)
*
* OffsetR      In: Add offset to red color channel
* OffsetG      In: Add offset to green color channel
* OffsetG      In: Add offset to blue color channel
*              Offset  range -256 ... 256
*
* GainChR      In: gain change for red color channel
* GainChG      In: gain change for green color channel
* GainChB      In: gain change for blue color channel
*              Gain range -100.0 ... 100.0 percent
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Color_MakeLUT_OffsetGain( uchar *pLookupR,      // Out: Point to red lookup table (256 bytes)
                                        uchar *pLookupG,      // Out: Point to green lookup table (256 bytes)
                                        uchar *pLookupB,      // Out: Point to blue lookup table (256 bytes)
                                        int   OffsetR,        // In: Add offset to red color channel
                                        int   OffsetG,        // In: Add offset to green color channel
                                        int   OffsetB,        // In: Add offset to blue color channel
                                        float GainChR,        // In: gain change for red color channel
                                        float GainChG,        // In: gain change for green color channel
                                        float GainChB)        // In: gain change for blue color channel
{
  int i, value;

  GainChR *= 0.01;            // Convert from percent to factor
  GainChG *= 0.01;
  GainChB *= 0.01;

  if( GainChR < -1.0) GainChR = -1.0;   // Clip to minimum
  if( GainChG < -1.0) GainChG = -1.0;
  if( GainChB < -1.0) GainChB = -1.0;

  for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {

#ifdef use_again
    value = (int)( i + OffsetR + (i * GainChR) + 0.5); // Round
    if (value > 255) value = 255;
    if (value <   0) value = 0;
    pLookupR[ i ] = (uchar)value;

    value = (int)( i + OffsetG + (i * GainChG) + 0.5); // Round
    if (value > 255) value = 255;
    if (value <   0) value = 0;
    pLookupG[ i ] = (uchar)value;

    value = (int)( i + OffsetB + (i * GainChB) + 0.5); // Round
    if (value > 255) value = 255;
    if (value <   0) value = 0;
    pLookupB[ i ] = (uchar)value;
#else

    if( OffsetR >= 0) {
      value = (int)( i + OffsetR + (i * GainChR) + 0.5); // Round
    } else {
      value = i + OffsetR;
      if( value < 0) value = 0;
      value = (int)( value + (value * GainChR) + 0.5); // Round
    }
    if (value > 255) value = 255;
    if (value <   0) value = 0;
    pLookupR[ i ] = (uchar)value;

    if( OffsetG >= 0) {
      value = (int)( i + OffsetG + (i * GainChG) + 0.5); // Round
    } else {
      value = i + OffsetG;
      if( value < 0) value = 0;
      value = (int)( value + (value * GainChG) + 0.5); // Round
    }
    if (value > 255) value = 255;
    if (value <   0) value = 0;
    pLookupG[ i ] = (uchar)value;

    if( OffsetB >= 0) {
      value = (int)( i + OffsetB + (i * GainChB) + 0.5); // Round
    } else {
      value = i + OffsetB;
      if( value < 0) value = 0;
      value = (int)( value + (value * GainChB) + 0.5); // Round
    }
    if (value > 255) value = 255;
    if (value <   0) value = 0;
    pLookupB[ i ] = (uchar)value;
#endif
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Color_MakeLUT_Gamma
*
* Make a gamma LUT table
*
* pLookupR     Out: Point to red lookup table (256 bytes)
* pLookupG     Out: Point to green lookup table (256 bytes)
* pLookupB     Out: Point to blue lookup table (256 bytes)
* GammaR       In: Gamma for red color channel
* GammaG       In: Gamma for green color channel
* GammaB       In: Gamma for blue color channel
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Color_MakeLUT_Gamma( uchar *pLookupR,      // Out: Point to red lookup table (256 bytes)
                                   uchar *pLookupG,      // Out: Point to green lookup table (256 bytes)
                                   uchar *pLookupB,      // Out: Point to blue lookup table (256 bytes)
                                   float GammaR,         // In: Gamma for red color channel
                                   float GammaG,         // In: Gamma for green color channel
                                   float GammaB)         // In: Gamma for blue color channel
{
  int i, value;
  double normalized, corrected;

  for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {

    normalized = (double)i / 255.0;

    corrected = pow( normalized, 1.0 / GammaR);
    value = (int)( corrected * 255.0 + 0.5); // Round
    if (value > 255) value = 255;
    pLookupR[ i ] = (uchar)value;

    corrected = pow( normalized, 1.0 / GammaG);
    value = (int)( corrected * 255.0 + 0.5); // Round
    if (value > 255) value = 255;
    pLookupG[ i ] = (uchar)value;

    corrected = pow( normalized, 1.0 / GammaB);
    value = (int)( corrected * 255.0 + 0.5); // Round
    if (value > 255) value = 255;
    pLookupB[ i ] = (uchar)value;
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Color_MakeLUT_Contrast
*
* Modify contrast with LUT table
*
* pLookupR     Out: Point to red lookup table (256 bytes)
* pLookupG     Out: Point to green lookup table (256 bytes)
* pLookupB     Out: Point to blue lookup table (256 bytes)
* Contrast1in  In: Contrast 1. point in value
* Contrast1out In: Contrast 1. point out value
* Contrast2in  In: Contrast 2. point in value
* Contrast2out In: Contrast 2. point out value
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Color_MakeLUT_Contrast( uchar *pLookupR,      // Out: Point to red lookup table (256 bytes)
                                      uchar *pLookupG,      // Out: Point to green lookup table (256 bytes)
                                      uchar *pLookupB,      // Out: Point to blue lookup table (256 bytes)
                                      int Contrast1in,      // In: Contrast 1. point in value
                                      int Contrast1out,     // In: Contrast 1. point out value
                                      int Contrast2in,      // In: Contrast 2. point in value
                                      int Contrast2out)     // In: Contrast 2. point out value
{
  int i, value;

  for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {

    if( i <= Contrast1in) {

      value = Contrast1out;

    } else if( i >= Contrast2in) {

      value = Contrast2out;

    } else {

      value = Contrast1out + ( Contrast2out - Contrast1out) * (i - Contrast1in) / (Contrast2in - Contrast1in);
    }

    if (value <   0) value = 0;
    if (value > 255) value = 255;
    pLookupR[ i ] = (uchar)value;
    pLookupG[ i ] = (uchar)value;
    pLookupB[ i ] = (uchar)value;
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Color_MakeLUT_Duotone
*
* Make duotone LUT table
*
* pLookupR     Out: Point to red lookup table (256 bytes)
* pLookupG     Out: Point to green lookup table (256 bytes)
* pLookupB     Out: Point to blue lookup table (256 bytes)
* Color_Bright  In: Contrast 1. point in value
* Color_Dark    In: Contrast 1. point out value
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Color_MakeLUT_Duotone( uchar *pLookupR,       // Out: Point to red lookup table (256 bytes)
                                     uchar *pLookupG,       // Out: Point to green lookup table (256 bytes)
                                     uchar *pLookupB,       // Out: Point to blue lookup table (256 bytes)
                                     Fl_Color Color_Bright, // Color for highlights
                                     Fl_Color Color_Dark,   // Color for shadows
                                     int contrast,          // Contrast range -100 ... 100 (0 = no change)
                                     int brightness)        // Brightness range -100 ... 100 (0 = no change)
{
  int i, value, MulFact;
  uchar Br_r, Br_g, Br_b, Da_r, Da_g, Da_b;
  double gray;

  // Standard contrast formula
  const double contrast_factor =
      (259.0 * (contrast + 255.0)) / (255.0 * (259.0 - contrast));

  Fl::get_color( Color_Bright, Br_r, Br_g, Br_b);    // Convert color to RGB values
  Fl::get_color(   Color_Dark, Da_r, Da_g, Da_b);    // Convert color to RGB values

  for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {

    gray = i;                                          // Gray value
    gray += brightness * 2.55;                         // Brightness (-100..100 -> -255..255) */
    gray = contrast_factor * (gray - 128.0) + 128.0;   // Contrast (scaling around a mean of 128)

    if( gray <   0.0) gray = 0.0;                      // Clamp
    if( gray > 255.0) gray = 255.0;

    // Interpolate between shadow and highlight colors

    MulFact = (int)round( 1024 * gray / 255.0);        // Prepare multiplier

    value = (int)Da_r + (( MulFact * ((int)Br_r - (int)Da_r) + 511) >> 10);
    if (value <   0) value = 0;
    if (value > 255) value = 255;
    pLookupR[ i ] = (uchar)value;

    value = (int)Da_g + (( MulFact * ((int)Br_g - (int)Da_g) + 511) >> 10);
    if (value <   0) value = 0;
    if (value > 255) value = 255;
    pLookupG[ i ] = (uchar)value;

    value = (int)Da_b + (( MulFact * ((int)Br_b - (int)Da_b) + 511) >> 10);
    if (value <   0) value = 0;
    if (value > 255) value = 255;
    pLookupB[ i ] = (uchar)value;
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_MixChannels
*
* Mix color channels to create an image
*
* ppDst        Out: Pointer to pointer to IHS image
* pSrc         In: Pointer to common input image
*                  This can be a color or black/white image.
*                  Also it can have an alpha channel.
* pInG         In: Pointer to optional red channel
*                  This image must have only one channel.
* pInB         In: Pointer to optional blue channel
*                  This image must have only one channel.
* pInA         In: Pointer to optional alpha channel
*                  If this image has a alpha channel, this is used.
*                  Otherwise the image must be a BW image.
*
* The output image always get the same with and height as the pSr Image.
* If the size of the other images differs, they are accessed using
* 'Nearest Neighbor' interpolation.
*
* Allowed combinations:  RGB is a color image, BW is a black/white image
*   pSrc    pInG    pInB    pInA    Output
*   BW      ---     ---     ---     Remove alpha channel from input image
*   RGB     ---     ---     ---     Remove alpha channel from input image
*   BW      ---     ---     BW      Add/replace alpha channel
*   RGB     ---     ---     BW      Add/replace alpha channel
*   BW      BW      BW      ---     Create RGB image without alpha
*   BW      BW      BW      BW      Create RGB image with alpha
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_MixChannels( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to IHS image
                           Fl_RGB_Image *pSrc,   // In: Pointer to common input image
                           Fl_RGB_Image *pInG,   // In: Pointer to optional red channel
                           Fl_RGB_Image *pInB,   // In: Pointer to optional blue channel
                           Fl_RGB_Image *pInA)   // In: Pointer to optional alpha channel
{
  int ierr, xx, yy, x, y, nBytesSrc, nBytesDst, nBytesA, AlphaDst, nBytesG, nBytesB, idxA;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc, iInG, iInB, iInA;
  uchar *s8, *d8, *g8, *b8, *a8;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // ...

  nBytesG = 0;     // Avoid compiler warning about not set variable
  nBytesB = 0;

  // Get image data

  xx = iSrc.xx;
  yy = iSrc.yy;
  nBytesSrc = iSrc.d;

  // Check for alpha image

  AlphaDst = 0;                                  // Preset no alpha output

  if( pInA != NULL) {

    AlphaDst = 1;                                // Create output image with alpha channel

    // Check alpha image to be a black/white image
    ierr = YaIPS_RGB_to_ImgD( pInA, &iInA);
    if( ierr != 0)  {                            // Check for error
      return( ierr);
    }

    nBytesA = iInA.d;

    idxA = 0;                                    // Go for single byte alpha image

    if( nBytesA == 2 || nBytesA == 4) {          // Has an input image with alpha

      idxA = nBytesA - 1;                        // Access alpha byte

    } else if( iInA.d != 1) {                    // Must be a black/white image

      if( errstring == NULL) {                   // No error until now
        errstring = LangStringLookup( "&RGB_Color_AlphaNoBW=Alpha no BW image!");
      }
      return( -120);
    }
  }

  // Check working mode

  if( pInG == NULL && pInB == NULL)  {           // Remove/add or replace alpha

    if( nBytesSrc >= 3) {                        // Source is color

      nBytesDst = 3 + AlphaDst;                  // Destination is a color image

    } else {                                     // Source is a black white image

      nBytesDst = 1 + AlphaDst;                  // Destination is a black white image
    }

  } else if( pInG != NULL && pInB != NULL)  {    // Create RGB image from BW images

    nBytesDst = 3 + AlphaDst;                    // Destination is a color image

    // Check Src/R image to be a black/white image
    if( iSrc.d >= 3) {                           // Must be a black/white image
      if( errstring == NULL) {                   // No error until now
        errstring = LangStringLookup( "&RGB_Color_InputNoBW=No BW image input!");
      }
      return( -121);
    }

    // Check G image to be a black/white image
    ierr = YaIPS_RGB_to_ImgD( pInG, &iInG);
    if( ierr != 0)  {                            // Check for error
      return( ierr);
    }

    nBytesG = iInG.d;

    if( iInG.d > 2) {                           // Must be a black/white image
      if( errstring == NULL) {                   // No error until now
        errstring = LangStringLookup( "&RGB_Color_InputGNoBW=G no BW image!");
      }
      return( -122);
    }

    // Check B image to be a black/white image
    ierr = YaIPS_RGB_to_ImgD( pInB, &iInB);
    if( ierr != 0)  {                            // Check for error
      return( ierr);
    }

    nBytesB = iInB.d;

    if( iInB.d > 2) {                           // Must be a black/white image
      if( errstring == NULL) {                   // No error until now
        errstring = LangStringLookup( "&RGB_Color_InputBNoBW=B no BW image!");
      }
      return( -123);
    }

  } else {                                       // No supported combination

    if( errstring == NULL) {                   // No error until now
      errstring = ERR_IPS_NO_SUPP;
    }
    return( -124);
  }

  // Ensure that pPDst image has the same size and proper byte per pixel
  ierr = YaIPS_RGB_ImageSetSize( ppDst, xx, yy, nBytesDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Convert ...

  if( pInG == NULL && pInB == NULL)  {           // Remove/add or replace alpha

    if( nBytesSrc >= 3) {                        // Source is color

      for( y = 0; y < yy; y++) {

        d8 = RGB_pixad( 0,  y, &iDst);
        s8 = RGB_pixad( 0,  y, &iSrc);

        if( AlphaDst) {

          a8 = RGB_pixad( 0, (y * iInA.yy) / yy, &iInA);
        }

        for( x = 0; x < xx; x++) {

          d8[ 0] = s8[ 0];
          d8[ 1] = s8[ 1];
          d8[ 2] = s8[ 2];

          if( AlphaDst) {

            d8[ 3] = a8[ ((x * iInA.xx) / xx) * nBytesA + idxA];
          }

          d8 += nBytesDst;
          s8 += nBytesSrc;
        }
      }

    } else {                                     // Source is a black white image

      for( y = 0; y < yy; y++) {

        d8 = RGB_pixad( 0,  y, &iDst);
        s8 = RGB_pixad( 0,  y, &iSrc);

        if( AlphaDst) {

          a8 = RGB_pixad( 0, (y * iInA.yy) / yy, &iInA);
        }

        for( x = 0; x < xx; x++) {

          d8[ 0] = s8[ 0];

          if( AlphaDst) {

            d8[ 1] = a8[ ((x * iInA.xx) / xx) * nBytesA + idxA];
          }

          d8 += nBytesDst;
          s8 += nBytesSrc;
        }
      }

    }

  } else {                                       // must be creation of RGB image from BW images

    for( y = 0; y < yy; y++) {

      d8 = RGB_pixad( 0,  y, &iDst);
      s8 = RGB_pixad( 0,  y, &iSrc);
      g8 = RGB_pixad( 0,  y, &iInG);
      b8 = RGB_pixad( 0,  y, &iInB);

      if( AlphaDst) {

        a8 = RGB_pixad( 0, (y * iInA.yy) / yy, &iInA);
      }

      for( x = 0; x < xx; x++) {

        d8[ 0] = s8[ 0];
        d8[ 1] = g8[ ((x * iInG.xx) / xx) * nBytesG];
        d8[ 2] = b8[ ((x * iInB.xx) / xx) * nBytesB];

        if( AlphaDst) {

          d8[ 3] = a8[ ((x * iInA.xx) / xx) * nBytesA + idxA];
        }

        d8 += nBytesDst;
        s8 += nBytesSrc;
      }
    }
  }

  return( 0);                                    // Return OK
}

/***************************************************************************
* YaIPS_RGB_ChromaKey
*
* Generate an alpha mask from color.
*
* ppDst               Out: Pointer to pointer to IHS image
* pSrc                In: Pointer to RGB image
* KeyColor            In: The key color
* HueThres, HueFade   In: Hue threshold and fade (range 0 .. 100). Default 30, 10.
* SatThres, SatFade   In: Saturation threshold and fade (range 0 .. 100). Default 30, 10.
* IDaThres, IDaFade   In: Intensity dark threshold and fade (range 0 .. 50). Default 20, 10.
* IBrThres, IBrFade   In: Intensity dark threshold and fade (range 0 .. 50). Default 10, 10.
*
* return     0 OK
*            1 Error, source image is no color image
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_ChromaKey( Fl_RGB_Image **ppDst,            // Out: Pointer to pointer to RGB image.
                         Fl_RGB_Image *pSrc,              // In: Pointer to RGB image
                         Fl_Color KeyColor,               // In: The key color
                         int HueThres, int HueFade,       // In: Hue threshold and fade (range 0 .. 100)
                         int SatThres, int SatFade,       // In: Saturation threshold and fade (range 0 .. 100)
                         int IDaThres, int IDaFade,       // In: Intensity dark threshold and fade (range 0 .. 100)
                         int IBrThres, int IBrFade,       // In: Intensity bright threshold and fade (range 0 .. 100)
                         int Flags,                       // In: Optional processing steps
                         int OutputType,                  // In: Type of output image
                         Fl_RGB_Image *pBGnd,             // In: Background image used for 'YAIPS_RGB_CHROMA_OUT_BLEND'
                         Fl_Color BlendColor)             // In: The color used for 'YAIPS_RGB_CHROMA_OUT_COL_B'
{
  int ierr, x, y, nBytesSrc, nBytesDst, nByteSrc2;
  int xx, yy, Src2xx, Src2yy;
  int ci, ch, cs, i_key, h_key, s_key, dh, a, HueBoth, SatBoth, IDaBoth, IBrBoth, Temp, Temp2;
  int Despill_ic, Despill_i1, Despill_i2, Despill_Max12, Spill, Despill_Thres_Low, Despill_Thres_High;
  unsigned char rgb[ 3], r_key, g_key, b_key, r_blend, g_blend, b_blend;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc, iSrc2;
  uchar *s8, *d8, *s28, *pLineSource2, *pL;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data

  xx = iSrc.xx;
  yy = iSrc.yy;
  nBytesSrc = iSrc.d;

  if( nBytesSrc < 3) {       // Check source to be a color image

    return( 1);              // Return error
  }

  // Set some dummy value to this variables to avoid compiler warning 'use of uninitialized variable'.

  nByteSrc2 = 0;
  Src2xx = 0;
  Src2yy = 0;
  s28 = NULL;

  // Check background image

  if( OutputType == YAIPS_RGB_CHROMA_OUT_BLEND) {

    if( pBGnd == NULL) {                          // Have no image

      OutputType = YAIPS_RGB_CHROMA_OUT_COL_B;    // Change to blend with color

      BlendColor = 0;                             // Set color to black

    } else {

      // Check source first
      ierr = YaIPS_RGB_to_ImgD( pBGnd, &iSrc2);
      if( ierr != 0)  {                           // Check for error
        return( ierr);
      }

      nByteSrc2 = iSrc2.d;                        // # bytes 2. source
      Src2xx = iSrc2.xx;                          // Size of second source may differ
      Src2yy = iSrc2.yy;

      if( nByteSrc2 < 3) {       // Check to be a color image

        return( 1);              // Return error
      }
    }
  }

  // Bytes per pixel for output imae

  switch( OutputType) {
  default:
  case YAIPS_RGB_CHROMA_OUT_INP_A:  // Processed input image with alpha mask, 4 bytes per pixel.

    nBytesDst = 4;
    break;

  case YAIPS_RGB_CHROMA_OUT_ALPHA:  // Alpha mask only, 1 byte per pixel.

    nBytesDst = 1;
    break;

  case YAIPS_RGB_CHROMA_OUT_BLEND:  // Input image 1 overlaid to input image 2, 3 bytes per pixel.
  case YAIPS_RGB_CHROMA_OUT_INP_P:  // Processed input image, 3 bytes per pixel.
  case YAIPS_RGB_CHROMA_OUT_COL_B:  // Processed input image alpha blended to color, 3 bytes per pixel.

    nBytesDst = 3;
    break;
  }

  // Ensure that pPDst image has the same size and proper byte per pixel
  ierr = YaIPS_RGB_ImageSetSize( ppDst, xx, yy, nBytesDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Prepare parameter

  Fl::get_color( KeyColor, r_key, g_key, b_key);

  Fl::get_color( BlendColor, r_blend, g_blend, b_blend);

  // Despill get dominant color component

  if( g_key >= r_key && g_key >= b_key) { // Green

    Despill_i1 = 0;   // r
    Despill_ic = 1;   // g
    Despill_i2 = 2;   // b

  } else if( r_key && g_key >= b_key) {   // Red

    Despill_ic = 0;   // r
    Despill_i1 = 1;   // g
    Despill_i2 = 2;   // b

  } else {                                // Blue

    Despill_i1 = 0;   // r
    Despill_i2 = 1;   // g
    Despill_ic = 2;   // b
  }

  // Despill alpha range to check

  switch( Flags & (YAIPS_RGB_CHROMA_FLAG_DESPILL_O | YAIPS_RGB_CHROMA_FLAG_DESPILL_I)) {

  default:
  case (YAIPS_RGB_CHROMA_FLAG_DESPILL_O | YAIPS_RGB_CHROMA_FLAG_DESPILL_I):    // Both bits are set
    Despill_Thres_Low = 0;
    Despill_Thres_High = 256;
    break;

  case YAIPS_RGB_CHROMA_FLAG_DESPILL_O:    // outside of the mask and the edges
    Despill_Thres_Low = 0;
    Despill_Thres_High = 254;
    break;

  case YAIPS_RGB_CHROMA_FLAG_DESPILL_I:    // inside of the mask
    Despill_Thres_Low = 254;
    Despill_Thres_High = 256;
    break;
  }

  // Prepare chroma key

  YaIPS_RGB_Color_RGB2IHS( &i_key, &h_key, &s_key, r_key, g_key, b_key);

  // Prepare parameter

  if( HueThres > 100) {    // Clip to this value

    HueThres = 100;
  }
  HueThres = HueThres / 2; // Reduce to usable value

  if( HueFade > 100) {     // Clip to this value
    HueFade = 100;
  }
  HueFade = HueFade / 2;   // Reduce to usable value

  HueBoth = HueThres + HueFade;

  if( SatThres > 100) {    // Clip to this value
    SatThres = 100;
  }

  if( SatFade > 100) {     // Clip to this value
    SatFade = 100;
  }

  SatBoth = SatThres + SatFade;

  if( IDaThres > 100) {    // Clip to this value

    IDaThres = 100;
  }

  if( IDaFade > 100) {     // Clip to this value
    IDaFade = 100;
  }

  IDaBoth = IDaThres + IDaFade;

  if( IBrThres > 100) {    // Clip to this value
    IBrThres = 100;
  }

  if( IBrFade > 100) {     // Clip to this value
    IBrFade = 100;
  }

  IBrBoth = IBrThres + IBrFade;

  // Prepare resize of second image if size is different from first source image

  pLineSource2 = NULL;                       // No memory allocated

  if( pBGnd != NULL &&                      // Have a background image
      (xx != Src2xx || yy != Src2yy)) {     // Size is different

    pLineSource2 = (uchar *)malloc( xx * nByteSrc2);

    if( pLineSource2 == NULL) {              // Allocation has failed

      return( -101);
    }
  }

  // Convert ...

  for( y = 0; y < yy; y++) {

    // Source and destination

    s8 = RGB_pixad( 0, y, &iSrc);
    d8 = RGB_pixad( 0, y, &iDst);

    // Prepare second input image

    if( OutputType == YAIPS_RGB_CHROMA_OUT_BLEND) {

      if( pLineSource2 == NULL) {  // First and second source image have the same size

        s28 = RGB_pixad( 0,  y, &iSrc2);

      } else {                     // First and second source image have the different sizes

        // Resize one line of data

        // We make a nearest neighbor interpolation

        pL = pLineSource2;

        for( x = 0; x < xx; x++) {

          s28 = RGB_pixad( ((x * Src2xx) / xx), (y * Src2yy) / yy, &iSrc2);

          // We only need the color component. This is checked before.

          pL[ 0] = s28[ 0];
          pL[ 1] = s28[ 1];
          pL[ 2] = s28[ 2];

          pL += nByteSrc2;
        }
      }

      // Use data from resized image for combine operations
      s28 = pLineSource2;
   }

   for( x = 0; x < xx; x++) {

      rgb[ 0] = s8[ 0];           // Get color part
      rgb[ 1] = s8[ 1];
      rgb[ 2] = s8[ 2];

      YaIPS_RGB_Color_RGB2IHS( &ci, &ch, &cs, rgb[ 0], rgb[ 1], rgb[ 2]);

      // difference in angle space
      if( ch >= h_key) {
        dh = ch - h_key;
      } else {
        dh = h_key - ch;
      }
      if( dh > 128) {   // circle wrap around
        dh = 256 - dh;
      }

      // Base alpha
      if( dh <= HueThres) {

        a = 0;                          // Masked off

      } else if( dh >= HueBoth) {

        a = 255;                        // Visible

      } else {

        // NOTE: If HueFade is 0 we never come to her

        a = (dh - HueThres) * 255 / HueFade;
      }

      // Saturation: Protect pixels without saturation

      if( cs < SatThres) {              // Low Saturation

        a = 255;                        // Visible

      } else if( cs < SatBoth) {        // Fade off

        Temp = (cs - SatThres) * 256 / SatFade;
        a = (255 * (256 - Temp) + (a * Temp)) >> 8;
      }

      // Intensity: protect dark and light pixels

      if( ci < IDaThres) {              // Low intensity

        a = 255;                        // Visible

      } else if( ci < IDaBoth) {        // Fade off

        Temp = (ci - IDaThres) * 256 / IDaFade;
        a = (255 * (256 - Temp) + (a * Temp)) >> 8;

      } else if( ci > 255 - IBrThres) { // High intensity

        a = 255;                        // Visible

      } else if( ci > 255 - IBrBoth) {  // Fade off

        Temp = ((255 - IBrThres) - ci) * 256 / IBrFade;
        a = (255 * (256 - Temp) + (a * Temp)) >> 8;
      }

      // Smoothstep preprocessing

      if( (Flags & YAIPS_RGB_CHROMA_FLAG_SMOOTHSTEP) != 0 &&  // Is set ?
          a > 0 && a < 255) {

        // x =  x * x * (3.0f - 2.0f * x);  // x is in the range 0.0 .. 1.0
        a = a * a * (3 * 255 - 2 * a) / (255 * 255);
        if( a < 0) {
          a = 0;
        }
        if( a > 255) {
          a = 255;
        }
      }

      // Despill preprocessing

      if( (Flags & (YAIPS_RGB_CHROMA_FLAG_DESPILL_O | YAIPS_RGB_CHROMA_FLAG_DESPILL_I)) != 0 &&  // Is set ?
           a >= Despill_Thres_Low && a <= Despill_Thres_High) {

#ifdef use_again
        if( rgb[ Despill_i1] >= rgb[ Despill_i2]) {
          Despill_Max12 = rgb[ Despill_i1];
        } else {

          Despill_Max12 = rgb[ Despill_i2];
        }
#else
        Despill_Max12 = (rgb[ Despill_i1] + rgb[ Despill_i2]) >> 1;
#endif

        Spill = rgb[ Despill_ic] - Despill_Max12;

        if( Spill >= 0) {

          // stronger with colored edges
#ifdef use_again
#ifndef use_again
          Temp = 166 - ((a * 166) >> 8);
#else
          Temp = 166 - ((0 * 166) >> 8);
#endif


          Temp2 = (int)rgb[ Despill_ic] - ((Spill * Temp) >> 8);
          if( Temp2 < 0) {
            Temp2 = 0;
          }
          rgb[ Despill_ic] = Temp2;

          rgb[ Despill_i1] += (Spill * Temp) >> 10;
          rgb[ Despill_i2] += (Spill * Temp) >> 10;
#else
#ifdef use_again
          Temp = 256 - ((a * 256) >> 8);
#else
          Temp = 256 - ((0 * 256) >> 8);
#endif


          Temp2 = (int)rgb[ Despill_ic] - ((Spill * Temp) >> 8);
          if( Temp2 < 0) {
            Temp2 = 0;
          }
          rgb[ Despill_ic] = Temp2;

          //x/rgb[ Despill_i1] += (Spill * Temp) >> 10;
          //x/rgb[ Despill_i2] += (Spill * Temp) >> 10;
#endif
        }
      }

      // Output

      switch( OutputType) {
      default:
      case YAIPS_RGB_CHROMA_OUT_INP_A:  // Processed input image with alpha mask, 4 bytes per pixel.

        d8[ 0] = rgb[ 0];          // Copy color part
        d8[ 1] = rgb[ 1];
        d8[ 2] = rgb[ 2];

        d8[ 3] = a;                // Output alpha
        break;

      case YAIPS_RGB_CHROMA_OUT_ALPHA:  // Alpha mask only, 1 byte per pixel.

        d8[ 0] = a;                // Output alpha
        break;

      case YAIPS_RGB_CHROMA_OUT_BLEND:  //  Input image 1 overlaid to input image 2, 3 bytes per pixel.

        d8[ 0] = (rgb[ 0] * a + s28[ 0] * (255 - a)) / 255;
        d8[ 1] = (rgb[ 1] * a + s28[ 1] * (255 - a)) / 255;
        d8[ 2] = (rgb[ 2] * a + s28[ 2] * (255 - a)) / 255;

        s28 += nByteSrc2;
        break;

      case YAIPS_RGB_CHROMA_OUT_INP_P:  // Processed input image, 3 bytes per pixel.

        d8[ 0] = rgb[ 0];          // Copy color part
        d8[ 1] = rgb[ 1];
        d8[ 2] = rgb[ 2];
        break;

      case YAIPS_RGB_CHROMA_OUT_COL_B:  // Input image alpha blended to color, 3 bytes per pixel.

        d8[ 0] = (rgb[ 0] * a + r_blend * (255 - a)) / 255;
        d8[ 1] = (rgb[ 1] * a + g_blend * (255 - a)) / 255;
        d8[ 2] = (rgb[ 2] * a + b_blend * (255 - a)) / 255;
        break;
      }

      s8 += nBytesSrc;
      d8 += nBytesDst;
    }
  }

  if( pLineSource2 != NULL) {                 // Free allocated memory

    free( pLineSource2);
  }

  return( 0);
}

/******************************** End Of File ********************************/

