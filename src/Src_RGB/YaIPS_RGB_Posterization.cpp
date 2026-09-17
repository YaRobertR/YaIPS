/****************************************************************************

  YaIPS_RGB_Posterization.cpp

  Convert an image into a poster.

 11.09.2026 RR: First edition of this file.
 16.09.2026 RR: Done with final improvements.

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
* YaIPS_RGB_PosterizeSimple
* Simple image posterization.
*
* piDst        Pointer to pointer to RGB image
* piSrc        Source image
*
* nLevels      In: Number of color levels. Range is from 2 .. 32
* Average      In: If set average colors
*
* return    >= Number of colors in the posterized output image.
*          < 0 Error
****************************************************************************
*/

typedef struct {                  // Histogram of regions

  unsigned int nSamples;          // Number of samples
  unsigned int SumR, SumG, SumB;  // Sum up color values
} T_Average;

#define MAX_AVERAGE_STATIC       125    // Max number of static average histogram regions

int YaIPS_RGB_PosterizeSimple( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                               Fl_RGB_Image *pSrc,   // Source image
                               int nLevels,          // In: Number of levels
                               int Average)          // In: If set average colors
{
  int ierr, x, y, xx, yy, nBytesSrc, nColorsSrc, SrcHasAlpha;
  int nColorsOut;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8;

  // Check/Clip parameter

  if( nLevels < 2) {

    nLevels = 2;
  }

  if( nLevels > YAIPS_RLC_POSTER_MAX_LEVEL) {

    nLevels = YAIPS_RLC_POSTER_MAX_LEVEL;
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

  // Get image data of source

  xx    = iSrc.xx;
  yy    = iSrc.yy;

  nBytesSrc  = iSrc.d;

  nColorsSrc = iSrc.d >= 3 ? 3 : 1;

  //x/nBytesSrc  = iSrc.d;

  SrcHasAlpha = iSrc.d == 2 || iSrc.d == 4;

  nColorsOut = 0;

  if( Average) {                              // Average color regions

    // Calculate lookup table for fast lookup

    int iLevel, iLookup, ThisLevel;
    uchar LookUpIndex[ 256];

    iLookup = 0;

    for( iLevel = 1; iLevel < nLevels; iLevel++) {

      ThisLevel = iLevel * 256 / nLevels;

      while( iLookup < ThisLevel) {

        LookUpIndex[ iLookup++] = iLevel - 1;
      }

    }

    while( iLookup < 256) {

      LookUpIndex[ iLookup++] = nLevels - 1;
    }

    T_Average AverageHisto[ MAX_AVERAGE_STATIC];   // Space
    T_Average *pHisto, *pHisto2;
    int nLevels2, nLevels3, Index;

    nLevels2 = nLevels * nLevels;
    nLevels3 = nLevels * nLevels * nLevels;

    if( nColorsSrc >= 3 && nLevels3 > MAX_AVERAGE_STATIC) {  // Color need maybe more than MAX_AVERAGE_STATIC Elements

      pHisto = (T_Average *)malloc( sizeof( T_Average) * nLevels3);

      if( pHisto == NULL) {                                  // Allocation failed

        return( -100);
      }

    } else {                                                 // Can use elements as global variable

      pHisto = AverageHisto;
    }

    // Convert ...

    if( nColorsSrc == 1) {        // BW image

      memset( pHisto, 0, sizeof( T_Average) * nLevels);   // Rest histogram

      // Sum up values

      for( y = 0; y < yy; y++) {

        s8 = RGB_pixad( 0, y, &iSrc);

        for( x = 0; x < xx; x++) {

          Index = LookUpIndex[ *s8];

          pHisto2 = pHisto + Index;

          pHisto2->SumR     += *s8;
          pHisto2->nSamples += 1;

          s8 += nBytesSrc;
        }
      }

      // Average

      pHisto2 = pHisto;
      for( Index = 0; Index < nLevels; Index++, pHisto2++)  {

        if( pHisto2->nSamples > 1) {

          pHisto2->SumR = (pHisto2->SumR + pHisto2->nSamples / 2) / pHisto2->nSamples;
        }

        if( pHisto2->nSamples > 0) {

          nColorsOut += 1;
        }
      }

      // Convert

      for( y = 0; y < yy; y++) {

        s8 = RGB_pixad( 0, y, &iSrc);
        d8 = RGB_pixad( 0, y, &iDst);

        for( x = 0; x < xx; x++) {

          Index = LookUpIndex[ *s8++];

          pHisto2 = pHisto + Index;

          *d8++ = pHisto2->SumR;

          if( SrcHasAlpha) {        // Copy alpha

            *d8++ = *s8++;
          }
        }
      }

    } else {                      // RGB image

      memset( pHisto, 0, sizeof( T_Average) * nLevels3);   // Rest histogram

      // Sum up values

      for( y = 0; y < yy; y++) {

        s8 = RGB_pixad( 0, y, &iSrc);

        for( x = 0; x < xx; x++) {

          Index = LookUpIndex[ s8[0]] + LookUpIndex[ s8[1]] * nLevels + LookUpIndex[ s8[2]] * nLevels2;

          pHisto2 = pHisto + Index;

          pHisto2->SumR     += s8[0];
          pHisto2->SumG     += s8[1];
          pHisto2->SumB     += s8[2];
          pHisto2->nSamples += 1;

          s8 += nBytesSrc;
        }
      }

      // Average

      pHisto2 = pHisto;
      for( Index = 0; Index < nLevels3; Index++, pHisto2++)  {

        if( pHisto2->nSamples > 1) {

          pHisto2->SumR = (pHisto2->SumR + pHisto2->nSamples / 2) / pHisto2->nSamples;
          pHisto2->SumG = (pHisto2->SumG + pHisto2->nSamples / 2) / pHisto2->nSamples;
          pHisto2->SumB = (pHisto2->SumB + pHisto2->nSamples / 2) / pHisto2->nSamples;
        }

        if( pHisto2->nSamples > 0) {

          nColorsOut += 1;
        }
      }

      // Convert

      for( y = 0; y < yy; y++) {

        s8 = RGB_pixad( 0, y, &iSrc);
        d8 = RGB_pixad( 0, y, &iDst);

        for( x = 0; x < xx; x++) {

          Index = LookUpIndex[ s8[0]] + LookUpIndex[ s8[1]] * nLevels + LookUpIndex[ s8[2]] * nLevels2;

          pHisto2 = pHisto + Index;

          *d8++ = pHisto2->SumR;
          *d8++ = pHisto2->SumG;
          *d8++ = pHisto2->SumB;

          s8 += 3;

          if( SrcHasAlpha) {        // Copy alpha

            *d8++ = *s8++;
          }
        }
      }
    }

    if( pHisto != AverageHisto) {     // Have allocated memory ?

      free( pHisto);
    }

  } else {                                    // No color average

    // Calculate lookup table for fast lookup

    int iLevel, iLookup, ThisLevel, LastLevel;
    uchar LookUp[ 256];

    iLookup = 0;
    LastLevel = 0;

    for( iLevel = 1; iLevel < nLevels; iLevel++) {

      ThisLevel = iLevel * 256 / nLevels;

      while( iLookup < ThisLevel) {

        LookUp[ iLookup++] = LastLevel;
      }

      LastLevel = ThisLevel;
    }

    while( iLookup < 256) {

      LookUp[ iLookup++] = 255;
    }

    // Convert ...

    if( nColorsSrc == 1) {        // BW image

      nColorsOut = nLevels;

    } else {

      nColorsOut = nLevels * nLevels * nLevels;
    }

    for( y = 0; y < yy; y++) {

      s8 = RGB_pixad( 0, y, &iSrc);
      d8 = RGB_pixad( 0, y, &iDst);

      if( nColorsSrc == 1) {        // BW image

        for( x = 0; x < xx; x++) {

          *d8++ = LookUp[ *s8++];

          if( SrcHasAlpha) {        // Copy alpha

            *d8++ = *s8++;
          }
        }

      } else {                      // RGB image

        for( x = 0; x < xx; x++) {

          *d8++ = LookUp[ *s8++];
          *d8++ = LookUp[ *s8++];
          *d8++ = LookUp[ *s8++];

          if( SrcHasAlpha) {        // Copy alpha

            *d8++ = *s8++;
          }
        }
      }
    }
  }

  return( nColorsOut);       // Return colors in the posterized output image
}

/*--------------------------------------------------------------------------
* Color analyze functions
**--------------------------------------------------------------------------
*/

#define CVIM_LUT_MAX_TABLES    64       /* max number of LUT tables          */
#define CVIM_LUT_MAX_NAME      64       /* max length of LUT name            */
#define CVIM_LUT_MAX_COLORS   256       /* max number of colors in LUT       */
#define CVIM_LUT_INVERSE_BITS   6       /* bit precision of inverse LUT      */
#define CVIM_LUT_INVERSE_ROUND (( 1 << (8 - CVIM_LUT_INVERSE_BITS)) / 2)

typedef struct {                        /* a single LUT entry                */
  uint8 r, g, b;                        /* the RGB value of this entry       */
  uint8 dummy;
} CVIM_LUTENTRY_T;

typedef struct {                        /* LUT data structure                */
  //x/char  Name[ CVIM_LUT_MAX_NAME];       /* name of LUT                       */
  //x/int16 nUsed;                          /* use count of this LUT             */
  int16 ValidPixelDepth;                /* valid pixels                      */
  int16 nColors;                        /* Colors used in LUT                */
  CVIM_LUTENTRY_T LUT[ CVIM_LUT_MAX_COLORS]; /* the LUT table                */

  int16 InverseNeedRecalc;              /* inverse LUT must be recalculated  */
  uint8 *pInverseLUT;                   /* point to inverse LUT              */
} CVIM_LUT_T;

/* cvim_lut */

#define CVIM_ERR_LUT_NOFREE     -900    /* no LUT table free                 */
#define CVIM_ERR_LUT_HANDLE     -901    /* LUT handle not OK                 */
#define CVIM_ERR_LUT_PARAM      -902    /* parameter not OK                  */
#define CVIM_ERR_LUT_CANTOPEN   -903    /* cannot open file                  */
#define CVIM_ERR_LUT_READ       -904    /* error read from file              */
#define CVIM_ERR_LUT_FILEDATA   -905    /* data in file not OK               */
#define CVIM_ERR_LUT_NCOLORS    -906    /* number of colors not OK           */
#define CVIM_ERR_LUT_CANTCREATE -907    /* cannot create file                */
#define CVIM_ERR_LUT_WRITE      -908    /* error write to file               */

/* create LUT --------------------------------------------------------------*/

static int cvim_lutcreate( CVIM_LUT_T *pLUT, int i_valid_pixel_depth)
{

  /* test parameter */
  if( i_valid_pixel_depth < 1 || i_valid_pixel_depth > 8) {
    PRINTF0("cvim_lutcreate: parameter i_valid_pixel_depth out of range\n");
    return( CVIM_ERR_LUT_PARAM);
  }

  /* prepare LUT table */

  memset( pLUT, 0, sizeof( CVIM_LUT_T));         /* reset all data */

  //x/strcpy( pLUT->Name, "Created");                /* name of LUT */
  //x/pLUT->nUsed             = 1;                   /* one user (the first) */
  pLUT->ValidPixelDepth   = i_valid_pixel_depth; /* valid pixel depth */
  pLUT->InverseNeedRecalc = TRUE;                /* recalc for inverse */

  return( 0);                                    /* return OK */

} /* int cvim_lut_init() */

/* close LUT ----------------------------------------------------------------*/

static int cvim_lutclose( CVIM_LUT_T *pLUT)
{

  /* have to free inverse LUT ? */
  if( pLUT->pInverseLUT != (uint8 *)NULL) {

    free( pLUT->pInverseLUT);
    pLUT->pInverseLUT = (uint8 *)NULL;
  }

  return( 0);

} /* int cvim_lutclose() */

/* reset number of colors of LUT -------------------------------------------*/

static int cvim_lutreset( CVIM_LUT_T *pLUT)
{

  pLUT->nColors = 0;

  return( 0);
} /* int cvim_lutreset() */

/* set color entry of LUT ---------------------------------------------------*/

static int cvim_lutsetcolor( CVIM_LUT_T *pLUT, int i_index,
                             int i_r, int i_g, int i_b, int i_reserved)
{

  if( i_index < 0 || i_index >= CVIM_LUT_MAX_COLORS ||
      i_index > pLUT->nColors ||
      i_index >= (1 << pLUT->ValidPixelDepth)) {
    PRINTF0("cvim_lutsetcolor: parameter i_index out of range\n");
    return( CVIM_ERR_LUT_PARAM);
  }

  if( i_index == pLUT->nColors) {                /* one after last color */
    pLUT->nColors += 1;                          /* append new color */
  }

  pLUT->LUT[ i_index].r = i_r;
  pLUT->LUT[ i_index].g = i_g;
  pLUT->LUT[ i_index].b = i_b;
  pLUT->LUT[ i_index].dummy = i_reserved;

  pLUT->InverseNeedRecalc = TRUE;                 /* inverse need recalc */

  return( 0);

} /* int cvim_lutsetcolor() */

/* get pointer to invers LUT (also recalcs inverse LUT) --------------------*/

static uint8 *cvim_lutgetpinverse ( CVIM_LUT_T *pLUT)
{
  int i, nInvColors, r, g, b, ColIndex;
  int rCol, gCol, bCol, TempSquareIndex;
  uint8 *pInLut;
  register long ColDist, LastColDist, *pgDistTab, *pbDistTab;
  long bDistTab[ CVIM_LUT_MAX_COLORS], gDistTab[ CVIM_LUT_MAX_COLORS];
  register CVIM_LUTENTRY_T *pLUTEntry;
  long SquareTab[ 512];

  nInvColors = (1 << CVIM_LUT_INVERSE_BITS);      /* number of inverse cols. */

  if( pLUT->pInverseLUT == (uint8 *)NULL) {       /* have to alloc inverse */
    pLUT->pInverseLUT = (uint8 *)malloc( nInvColors * nInvColors * nInvColors);

    if( pLUT->pInverseLUT == (uint8 *)NULL) {     /* test for allocated */
      PRINTF0("cvim_lutgetpinverse: can't malloc inverse LUT\n");
      return( (uint8 *)NULL);
    }

    pLUT->InverseNeedRecalc = TRUE;               /* new inverse need recalc */
  }

  /* can return, if no recalc needed */
  if( pLUT->InverseNeedRecalc == FALSE) {         /* need no recalc */
    return( pLUT->pInverseLUT);                   /* pointer to inverse LUT */
  }

  /* recalc inverse LUT */

  PRINTF0("generate inverse lut\n");

  /* square table to avoid multiplication subroutine call on some machines */
  for( i = 0; i < 256; i++) {
    SquareTab[ 256 + i] = (long)i * (long)i;
    SquareTab[ 256 - i] = SquareTab[ 256 + i];
  }

  pInLut = pLUT->pInverseLUT;                     /* begin of inverse LUT */

  for( b = 0; b < nInvColors; b++) {

    bCol = (b << (8 - CVIM_LUT_INVERSE_BITS));
    if( bCol > 255) {
      bCol = 255;
    }

    pLUTEntry = pLUT->LUT;                  /* point to first LUT entry */
    pbDistTab = bDistTab;
    for( i = 0; i < pLUT->nColors; i++) {
      *pbDistTab++ = SquareTab[ 256 + bCol - btoi( pLUTEntry->b)];

      pLUTEntry++;                         /* point to next LUT Entry */
    }

    for( g = 0; g < nInvColors; g++) {

      gCol = (g << (8 - CVIM_LUT_INVERSE_BITS));
      if( gCol > 255) {
        gCol = 255;
      }

      pLUTEntry = pLUT->LUT;                /* point to first LUT entry */
      pgDistTab = gDistTab;
      pbDistTab = bDistTab;
      for( i = 0; i < pLUT->nColors; i++) {
        *pgDistTab++ = SquareTab[ 256 + gCol - btoi( pLUTEntry->g)] +
                           *pbDistTab++;

        pLUTEntry++;                       /* point to next LUT Entry */
      }

      for( r = 0; r < nInvColors; r++) {

        rCol = (r << (8 - CVIM_LUT_INVERSE_BITS));
        if( rCol > 255) {
          rCol = 255;
        }

        LastColDist = MAXLONG;
        ColIndex = 0;
        pLUTEntry = pLUT->LUT;                  /* point to first LUT entry */
        pgDistTab = gDistTab;
        TempSquareIndex = 256 + rCol;

        for( i = 0; i < pLUT->nColors; i++) {

          /* this is faster than using sqrt(). The argument of */
          /* the sqrt is, like the result of sqrt, monoton ascending */

          ColDist = *pgDistTab++ +
                        SquareTab[ TempSquareIndex - btoi( pLUTEntry->r)];

          if( ColDist < LastColDist) {   /* if less than last color ... */
            ColIndex = i;                /* ... this is the new color */
            LastColDist = ColDist;
          }

          pLUTEntry++;                   /* point to next LUT Entry */
        }

        *pInLut++ = ColIndex;
      }
    }
  }

  pLUT->InverseNeedRecalc = FALSE;                /* inverse recalc is done */

  PRINTF0("generate inverse lut done\n");

  return( pLUT->pInverseLUT);                     /* pointer to inverse LUT */

} /* int cvim_lutgetpinverse() */


// Defines 3D LUT

#define CVIM_LUTANA_MAX_COLORBITS 6
#define CVIM_LUTANA_MAX_COLORBITSSQ (CVIM_LUTANA_MAX_COLORBITS + \
                                     CVIM_LUTANA_MAX_COLORBITS)
#define CVIM_LUTANA_MAX_COLORS    (1 << CVIM_LUTANA_MAX_COLORBITS)
#define CVIM_LUTANA_ROUND         ((1 << (8 - CVIM_LUTANA_MAX_COLORBITS)) / 2)

#define CVIM_LUTANA_COLIDX(col) \
  ((col + CVIM_LUTANA_ROUND) > 255 ? \
  (255 >> (8 - CVIM_LUTANA_MAX_COLORBITS)) : \
  ((col + CVIM_LUTANA_ROUND) >> (8 - CVIM_LUTANA_MAX_COLORBITS)))

#define CVIM_LUTANA_IDXTOCOL(col) \
  ((col) << (8 - CVIM_LUTANA_MAX_COLORBITS))

#define CVIM_LUTANA_IDXTOCOLHI(col) \
  (CVIM_LUTANA_IDXTOCOL(col) + (CVIM_LUTANA_ROUND - 1))

#define CVIM_LUTANA_IDXTOCOLLO(col) \
  ((((col) << (8 - CVIM_LUTANA_MAX_COLORBITS)) - CVIM_LUTANA_ROUND) < 0 ? \
  0 : (((col) << (8 - CVIM_LUTANA_MAX_COLORBITS)) - CVIM_LUTANA_ROUND))

#define CVIM_LUTANA_HISTIDX(r, g, b) \
  ((r) + \
  ((g) << CVIM_LUTANA_MAX_COLORBITS) + \
  ((b) << CVIM_LUTANA_MAX_COLORBITSSQ))

#define CVIM_LUTANA_BACKGROUNDRAD     32
#define CVIM_LUTANA_BACKGROUNDRADTOL  16

#define CVIM_LUTANA_REDMAX       1
#define CVIM_LUTANA_GREENMAX     2
#define CVIM_LUTANA_BLUEMAX      4

#define CVIM_LUTANA_ALPHAMAX     1
#define CVIM_LUTANA_BETAMAX      2
#define CVIM_LUTANA_RADMAX       4

typedef enum cvim_lutana_algo_t {       /* LUT analysis algorithm            */
  cvim_lutana_popularity = 1,           /* popularity algorithm              */
  cvim_lutana_document                  /* document algorithm                  */
} CVIM_LUTANA_ALGO_T;

typedef struct {
  int initialized;
  uint32 *phisto3d;               /* 3-D histogramm, 5 bit per color */
  uint32 *phisto3dTmp;  /* 3-D histogramm temporary, 5 bit per color */
  CVIM_LUTANA_ALGO_T algorithm;                /* analysis algorithm */
} T_lutanaDesc;

static T_lutanaDesc lutanaDesc = { 0, NULL, NULL};

#define CLUSTER_DONE 1
#define CLUSTER_DELETE 2

typedef struct {
  int32 x;
  int32 z;
  int32 xx;
  int32 zz;
  uint32 sum;
  uint32 dummy;
} T_xzRect;

typedef struct {
  T_xzRect xzRect[CVIM_LUTANA_MAX_COLORS];   /* actual rectangle per radius */
  int32 xmin;
  int32 xmax;
  int32 ymin;
  int32 ymax;
  int32 zmin;
  int32 zmax;
  int32 flags;
  uint32 volume;
  uint32 xGrav;
  uint32 yGrav;
  uint32 zGrav;
  int32 r;
  int32 g;
  int32 b;
} T_clusterDesc;

// Error codes

#define CVIM_ERR_LUTANA_PIXFORMAT -1000   /* unsupported pixel format        */
#define CVIM_ERR_LUTANA_SLEMPTY   -1001   /* empty scanline (no data)        */
#define CVIM_ERR_LUTANA_NINIT     -1002   /* not initialized                 */
#define CVIM_ERR_LUTANA_ALLOC     -1003   /* out of memory                   */
#define CVIM_ERR_LUTANA_INVLUT    -1004   /* invalid LUT handle              */
#define CVIM_ERR_LUTANA_PARAM     -1005   /* illegal parameter               */

/* -------------------------- private -------------------------------------- */

static int cvim_lutanaMemMax( uint32 *ps,
                              uint32 *pd,
                              int inc,
                              int len,
                              int flag)
{
  int i, j;
  int xa, xe, valLast, val;

  xa = -1;
  val = *ps;
  if (val > 0) xa = 0;
  for (i = 0; i < len; i++) {
    valLast = val;
    val = *ps;
    ps += inc;
    if (val > valLast) {
      xa = i;
    } else if (val < valLast) {
      if (xa >= 0) {
        xe = i - 1;
        for (j = xa; j <= xe; j++) {
          pd[j * inc] |= flag;
        }
        xa = -1;
      }
    }
  }
  if (xa >= 0) {
    xe = i - 1;
    for (j = xa; j <= xe; j++) {
      pd[j * inc] |= flag;
    }
    xa = -1;
  }

  return(0);
}

#define ERODE  16

static int cvim_lutanaMaxInXZPlane( uint32 *ps,
                                    uint32 *pd,
                                    int y)
{
  int x, z;
  int xa, xe;
  int za, ze;

  /* search all peaks in x/z plane at y */
  for (x = 0; x < CVIM_LUTANA_MAX_COLORS; x++) {
    for (z = 0; z < CVIM_LUTANA_MAX_COLORS; z++) {
      pd[CVIM_LUTANA_HISTIDX(x, y, z)] = 0;
    }
  }

  for (x = 0; x < CVIM_LUTANA_MAX_COLORS; x++) {
    cvim_lutanaMemMax(&ps[CVIM_LUTANA_HISTIDX(x, y, 0)],
                      &pd[CVIM_LUTANA_HISTIDX(x, y, 0)],
                      CVIM_LUTANA_MAX_COLORS * CVIM_LUTANA_MAX_COLORS,
                      CVIM_LUTANA_MAX_COLORS, CVIM_LUTANA_BETAMAX);
  }
  for (z = 0; z < CVIM_LUTANA_MAX_COLORS; z++) {
    cvim_lutanaMemMax(&ps[CVIM_LUTANA_HISTIDX(0, y, z)],
                      &pd[CVIM_LUTANA_HISTIDX(0, y, z)],
                      1, CVIM_LUTANA_MAX_COLORS, CVIM_LUTANA_ALPHAMAX);
  }

  /* erode once without loose of peaks */
  for (x = 0; x < CVIM_LUTANA_MAX_COLORS; x++) {
    za = -1;
    for (z = 0; z < CVIM_LUTANA_MAX_COLORS; z++) {
      if ((pd[CVIM_LUTANA_HISTIDX(x, y, z)] &
           (CVIM_LUTANA_BETAMAX | CVIM_LUTANA_ALPHAMAX)) ==
          (CVIM_LUTANA_BETAMAX | CVIM_LUTANA_ALPHAMAX)) {
        if (za < 0) za = z;
      } else {
        if (za >= 0) {
          ze = z - 1;
          if (ze - za > 1) {
            /* more than 2 pixels, so erode it */
            pd[CVIM_LUTANA_HISTIDX(x, y, za)] |= ERODE;
            pd[CVIM_LUTANA_HISTIDX(x, y, ze)] |= ERODE;
          } else if (ze - za > 0) {
            /* 2 pixels, so erode one of them */
            pd[CVIM_LUTANA_HISTIDX(x, y, za)] |= ERODE;
          }
          za = -1;
        }
      }
    }
  }
  for (z = 0; z < CVIM_LUTANA_MAX_COLORS; z++) {
    xa = -1;
    for (x = 0; x < CVIM_LUTANA_MAX_COLORS; x++) {
      if ((pd[CVIM_LUTANA_HISTIDX(x, y, z)] &
           (CVIM_LUTANA_BETAMAX | CVIM_LUTANA_ALPHAMAX)) ==
          (CVIM_LUTANA_BETAMAX | CVIM_LUTANA_ALPHAMAX)) {
        if (xa < 0) xa = x;
      } else {
        if (xa >= 0) {
          xe = x - 1;
          if (xe - xa > 1) {
            /* more than 2 pixels, so erode it */
            pd[CVIM_LUTANA_HISTIDX(xa, y, z)] |= ERODE;
            pd[CVIM_LUTANA_HISTIDX(xe, y, z)] |= ERODE;
          } else if (xe - xa > 0) {
            /* 2 pixels, so erode one of them */
            pd[CVIM_LUTANA_HISTIDX(xa, y, z)] |= ERODE;
          }
          xa = -1;
        }
      }
    }
  }
  /* extract pure peaks */
  for (x = 0; x < CVIM_LUTANA_MAX_COLORS; x++) {
    for (z = 0; z < CVIM_LUTANA_MAX_COLORS; z++) {
      if ((pd[CVIM_LUTANA_HISTIDX(x, y, z)] &
           (CVIM_LUTANA_BETAMAX | CVIM_LUTANA_ALPHAMAX)) ==
          (CVIM_LUTANA_BETAMAX | CVIM_LUTANA_ALPHAMAX) &&
          (pd[CVIM_LUTANA_HISTIDX(x, y, z)] & ERODE) == 0) {
        pd[CVIM_LUTANA_HISTIDX(x, y, z)] = ps[CVIM_LUTANA_HISTIDX(x, y, z)];
      } else {
        pd[CVIM_LUTANA_HISTIDX(x, y, z)] = 0;
      }
    }
  }

  return(0);
}

static int cvim_lutanaXZGetClusterSum( uint32 *ps,
                                       int x0, int z0, int x1, int z1, int y,
                                       uint32 *retSum, uint32 *retNPix)
{
  int32 x, z;
  uint32 sum, nPix;

  sum = 0;
  nPix = 0;
  /* get sum of values in cube */
  for (x = x0; x <= x1; x++) {
    for (z = z0; z <= z1; z++) {
      sum += ps[CVIM_LUTANA_HISTIDX(x, y, z)];
      nPix++;
    }
  }

  *retSum = sum;
  *retNPix = nPix;

  return(0);
}

static int cvim_lutanaClusterMeas(uint32 *ps, int y0, int y1,
                                   T_xzRect *pxzRect,
                                   uint32 *retVolume, uint32 *retNPix,
                                   uint32 *retXGrav, uint32 *retYGrav,
                                   uint32 *retZGrav,
                                   int32 *retXMin, int32 *retXMax,
                                   int32 *retZMin, int32 *retZMax)
{
  int32 x, y, z, x0, x1, z0, z1;
  uint32 val, vol, nPix, xGrav, yGrav, zGrav;
  int32 xmin, xmax, zmin, zmax;

  vol = 0;
  nPix = 0;
  xGrav = 0;
  yGrav = 0;
  zGrav = 0;
  xmin = CVIM_LUTANA_MAX_COLORS - 1;
  xmax = 0;
  zmin = CVIM_LUTANA_MAX_COLORS - 1;
  zmax = 0;
  /* measure in xz-planes */
  for (y = y0; y <= y1; y++) {
    x0 = pxzRect[y].x;
    z0 = pxzRect[y].z;
    x1 = pxzRect[y].x + pxzRect[y].xx - 1;
    z1 = pxzRect[y].z + pxzRect[y].zz - 1;
    if (pxzRect[y].x < xmin) {
      xmin = pxzRect[y].x;
    }
    if (pxzRect[y].x + pxzRect[y].xx - 1 > xmax) {
      xmax = pxzRect[y].x + pxzRect[y].xx - 1;
    }
    if (pxzRect[y].z < zmin) {
      zmin = pxzRect[y].z;
    }
    if (pxzRect[y].z + pxzRect[y].zz - 1 > zmax) {
      zmax = pxzRect[y].z + pxzRect[y].zz - 1;
    }
    for (x = x0; x <= x1; x++) {
      for (z = z0; z <= z1; z++) {
        val = ps[CVIM_LUTANA_HISTIDX(x, y, z)];
        vol += val;
        xGrav += x * val;
        yGrav += y * val;
        zGrav += z * val;
        nPix++;
      }
    }
  }

  *retVolume = vol;
  *retNPix = nPix;
  if (vol <= 0) vol = 1;
  *retXGrav = xGrav / vol;
  *retYGrav = yGrav / vol;
  *retZGrav = zGrav / vol;
  *retXMin = xmin;
  *retXMax = xmax;
  *retZMin = zmin;
  *retZMax = zmax;

  return(0);
}

static int cvim_lutanaXZClusterMeasUpdate(uint32 *ps, int x0, int z0,
                                           int x1, int z1, int y,
                                           uint32 *retVolume, uint32 *retNPix,
                                           uint32 *retXGrav, uint32 *retYGrav,
                                           uint32 *retZGrav, uint32 *retSum)
{
  int32 x, z;
  uint32 val, vol, nPix, xGrav, yGrav, zGrav;

  vol = 0;
  nPix = 0;
  xGrav = 0;
  yGrav = 0;
  zGrav = 0;
  /* measure in xz-plane */
  for (x = x0; x <= x1; x++) {
    for (z = z0; z <= z1; z++) {
      val = ps[CVIM_LUTANA_HISTIDX(x, y, z)];
      vol += val;
      xGrav += x * val;
      yGrav += y * val;
      zGrav += z * val;
      nPix++;
    }
  }

  *retVolume += vol;
  *retNPix += nPix;
  *retXGrav += xGrav;
  *retYGrav += yGrav;
  *retZGrav += zGrav;
  *retSum = vol;

  return(0);
}

static int cvim_lutanaMemSpalt(uint32 *ps, uint32 *pd, int size,
                                int inc, int len)
{
  int i, TPsiz;
  int32 sum, *ps0;

  TPsiz = 2 * size + 1;

  len -= 2 * size + 1;
  if (len < 0) return(-1);

  ps0 = (int32 *)ps;
  sum = 0;
  /* get start value */
  for (i = 0; i < size + 1; i++) {
    sum += *ps;
    ps += inc;
  }
  *pd = sum / (size + 1);
  pd += inc;

  /* filter marching in */
  for (i = 0; i < size; i++) {
    sum += *ps;
    ps += inc;
    *pd = sum / (size + 2 + i);
    pd += inc;
  }

  /* filter marching through */
  for (i = 0; i < len; i++) {
    sum += *ps;
    ps += inc;
    sum -= *ps0;
    ps0 += inc;
    *pd = sum / TPsiz;
    pd += inc;
  }

  /* filter marching out */
  for (i = 1; i <= size; i++) {
    sum -= *ps0;
    ps0 += inc;
    *pd = sum / (TPsiz - i);
    pd += inc;
  }

  return(0);
}

static int cvim_lutanaSpaltLP(uint32 *ps, uint32 *pd, int size)
{
  int x, y, z;

  /* in r/b-plane along r-axis */
  for (y = 0; y < CVIM_LUTANA_MAX_COLORS; y++) {
    for (z = 0; z < CVIM_LUTANA_MAX_COLORS; z++) {
      cvim_lutanaMemSpalt(&ps[CVIM_LUTANA_HISTIDX(0, y, z)],
                          &pd[CVIM_LUTANA_HISTIDX(0, y, z)],
                          size, 1, CVIM_LUTANA_MAX_COLORS);
    }
  }
  /* in r/b-plane along b-axis */
  for (y = 0; y < CVIM_LUTANA_MAX_COLORS; y++) {
    for (x = 0; x < CVIM_LUTANA_MAX_COLORS; x++) {
      cvim_lutanaMemSpalt(&pd[CVIM_LUTANA_HISTIDX(x, y, 0)],
                          &ps[CVIM_LUTANA_HISTIDX(x, y, 0)],
                          size, CVIM_LUTANA_MAX_COLORS * CVIM_LUTANA_MAX_COLORS,
                          CVIM_LUTANA_MAX_COLORS);
    }
  }
  /* in g/b-plane along g-axis */
  for (x = 0; x < CVIM_LUTANA_MAX_COLORS; x++) {
    for (z = 0; z < CVIM_LUTANA_MAX_COLORS; z++) {
      cvim_lutanaMemSpalt(&ps[CVIM_LUTANA_HISTIDX(x, 0, z)],
                          &pd[CVIM_LUTANA_HISTIDX(x, 0, z)],
                          size, CVIM_LUTANA_MAX_COLORS,
                          CVIM_LUTANA_MAX_COLORS);
    }
  }

  return(0);
}

static int cvim_lutanaHistMax(uint32 *ps, uint32 *valMax,
                               int *xMax, int *yMax, int *zMax)
{
  int x, y, z, px, py, pz;
  uint32 maxVal;

  /* get maximum */
  maxVal = 0;
  px = py = pz = -1;
  for (x = 0; x < CVIM_LUTANA_MAX_COLORS; x++) {
    for (y = 0; y < CVIM_LUTANA_MAX_COLORS; y++) {
      for (z = 0; z < CVIM_LUTANA_MAX_COLORS; z++) {
        if (ps[CVIM_LUTANA_HISTIDX(x, y, z)] > maxVal) {
          maxVal = ps[CVIM_LUTANA_HISTIDX(x, y, z)];
          px = x;
          py = y;
          pz = z;
        }
      }
    }
  }

  *valMax = maxVal;
  *xMax = px;
  *yMax = py;
  *zMax = pz;

  return(0);
}

// RGB to polar coordinates transformation
static int cvim_rgb2pol(int r, int g, int b, int *alpha, int *beta, int *rad)
{
  double dr, dg, db, dtemp;

  dr = (double)r;
  dg = (double)g;
  db = (double)b;
  dtemp = dr * dr + dg * dg;

  /* alpha and beta are surely in the range 0...255, 40.42536 = 127.0 / M_PI */
  if (r == 0) {
    *alpha = 0;
  } else {
    *alpha = (int)(atan2(dg, dr) * 162.338041953 + 0.5);
  }
  if (b == 0) {
    *beta = 0;
  } else {
    *beta  = (int)(atan2(sqrt(dtemp), db) * 162.338041953 + 0.5);
  }
  *rad = (int)(sqrt( dtemp + db * db) * (255.0 / 441.67295593) + 0.5);

  return(0);
}

// polar coordinates to RGB transformation
static int cvim_pol2rgb(int alpha, int beta, int rad, int *r, int *g, int *b)
{
  register double dalpha, dbeta, drad;

  dalpha = (double)alpha;
  dbeta  = (double)beta;
  drad   = (double)rad;

  dalpha = dalpha / 162.338041953;
  dbeta  = dbeta  / 162.338041953;
  drad   = drad   / (255.0 / 441.67295593);

  *r = (int)(drad * sin(dbeta) * cos(dalpha) + 0.5);
  if (*r < 0) *r = 0;
  if (*r > 255) *r = 255;
  *g = (int)(drad * sin(dbeta) * sin(dalpha) + 0.5);
  if (*g < 0) *g = 0;
  if (*g > 255) *g = 255;
  *b = (int)(drad * cos(dbeta) + 0.5);
  if (*b < 0) *b = 0;
  if (*b > 255) *b = 255;

  return(0);
}

static int cvim_rgb2polLightHist(uint32 *ps, uint32 *pd,
                                  int r0, int g0, int b0)
{
  int r, g, b, rTmp, gTmp, bTmp;
  int alpha, beta, rad;

  /* first reset histogramm */
  for (r = 0; r < CVIM_LUTANA_MAX_COLORS *
                  CVIM_LUTANA_MAX_COLORS *
                  CVIM_LUTANA_MAX_COLORS; r++) {
    pd[r] = 0;
  }

  for (r = 0; r < CVIM_LUTANA_MAX_COLORS; r++) {
    for (g = 0; g < CVIM_LUTANA_MAX_COLORS; g++) {
      for (b = 0; b < CVIM_LUTANA_MAX_COLORS; b++) {

        rTmp = r0 - CVIM_LUTANA_IDXTOCOL(r);
        if (rTmp < 0) rTmp = 0;
        gTmp = g0 - CVIM_LUTANA_IDXTOCOL(g);
        if (gTmp < 0) gTmp = 0;
        bTmp = b0 - CVIM_LUTANA_IDXTOCOL(b);
        if (bTmp < 0) bTmp = 0;

        cvim_rgb2pol( rTmp, gTmp, bTmp, &alpha, &beta, &rad);

        pd[CVIM_LUTANA_HISTIDX(CVIM_LUTANA_COLIDX(alpha),
                               CVIM_LUTANA_COLIDX(rad),
                               CVIM_LUTANA_COLIDX(beta))] +=
          ps[CVIM_LUTANA_HISTIDX(r, g, b)];
      }
    }
  }

  return(0);
}

static uint32 cvim_lutanaClusterDelete( uint32 *ps, int size,
                                        int px, int py, int pz)
{
  int32 x, y, z, x0, y0, z0, x1, y1, z1;
  double dx, dy, dz, radSquare, size2;

  size2 = size * size;              // Square

  x0 = px - size;
  if (x0 < 0) x0 = 0;
  x1 = px + size;
  if (x1 >= CVIM_LUTANA_MAX_COLORS) x1 = CVIM_LUTANA_MAX_COLORS - 1;
  y0 = py - size;
  if (y0 < 0) y0 = 0;
  y1 = py + size;
  if (y1 >= CVIM_LUTANA_MAX_COLORS) y1 = CVIM_LUTANA_MAX_COLORS - 1;
  z0 = pz - size;
  if (z0 < 0) z0 = 0;
  z1 = pz + size;
  if (z1 >= CVIM_LUTANA_MAX_COLORS) z1 = CVIM_LUTANA_MAX_COLORS - 1;

  /* delete values in sphere */
  for (x = x0; x <= x1; x++) {
    dx = (double)(x - px);
    for (y = y0; y <= y1; y++) {
      dy = (double)(y - py);
      for (z = z0; z <= z1; z++) {
        dz = (double)(z - pz);
        radSquare = dx * dx + dy * dy + dz * dz;   // Square radius
        if( radSquare > size2) continue;           // outside sphere
        ps[CVIM_LUTANA_HISTIDX(x, y, z)] = 0;
      }
    }
  }

  return(0);
}


static int cvim_lutanaPopularity( uint32 *ps, CVIM_LUT_T *pLUT, int maxColors,
                                  int clusterSize)
{
  int i;
  uint32 max;
  int rMax, gMax, bMax;
  int ierr;

  /* reset LUT */
  ierr = cvim_lutreset( pLUT);
  if( ierr != 0) {

    return(ierr);
  }

  pLUT->InverseNeedRecalc = TRUE;

  /* do for each color */
  for (i = 0; i < maxColors; i++) {
    /* search maximum in histogramm */
    cvim_lutanaHistMax(ps, &max, &rMax, &gMax, &bMax);

    if (rMax < 0 || gMax < 0 || bMax < 0) break; /* no more color found */

    /* enter color in LUT */
    ierr = cvim_lutsetcolor( pLUT, i,
                             CVIM_LUTANA_IDXTOCOL(rMax),
                             CVIM_LUTANA_IDXTOCOL(gMax),
                             CVIM_LUTANA_IDXTOCOL(bMax), 0);

    if( ierr != 0) {

      return(ierr);
    }

    /* delete color cluster out of histogramm */
    cvim_lutanaClusterDelete(ps, CVIM_LUTANA_COLIDX(clusterSize) / 2,
                             rMax, gMax, bMax);

  }

  return(0);

}

static int cvim_lutanaClusterOverlap( T_clusterDesc *pcl, int numCluster,
                                      int index, int x, int xx, int z, int zz)
{
  int i;
  int xmin, xmax, zmin, zmax;

  /* first check border */
  if (x < 0 || x + xx > CVIM_LUTANA_MAX_COLORS ||
      z < 0 || z + zz > CVIM_LUTANA_MAX_COLORS) return(TRUE);

  for (i = 0; i < numCluster; i++) {
    if (i == index) continue;
    if (pcl[i].flags & CLUSTER_DONE) continue;
    /* take the rectangle from the last processed step (this or last) */
    xmin = pcl[i].xzRect[pcl[i].ymin].x;
    xmax = pcl[i].xzRect[pcl[i].ymin].x + pcl[i].xzRect[pcl[i].ymin].xx - 1;
    zmin = pcl[i].xzRect[pcl[i].ymin].z;
    zmax = pcl[i].xzRect[pcl[i].ymin].z + pcl[i].xzRect[pcl[i].ymin].zz - 1;
    if (x <= xmax && (x + xx - 1) >= xmin &&
        z <= zmax && (z + zz - 1) >= zmin) {
      return(TRUE);
    }
  }

  return(FALSE);

}

#define GROW_LEFT      1
#define SHRINK_LEFT    2
#define GROW_RIGHT     4
#define SHRINK_RIGHT   8
#define GROW_TOP       16
#define SHRINK_TOP     32
#define GROW_BOTTOM    64
#define SHRINK_BOTTOM  128

typedef struct {
  int32 x;
  int32 z;
  uint32 val;
  int32 dummy;
} T_peakDesc;

static int cvim_lutanaDocument( uint32 *ps, uint32 *pTmp, CVIM_LUT_T *pLUT,
                                int maxColors, int backgroundSeparation)
{
  int i, j, alpha, rad, beta, peak;
  uint32 nPix, max;
  int rMax, gMax, bMax, r, g, b;
  int ierr, backgroundRad;
  T_clusterDesc *pClusterList = (T_clusterDesc *)NULL;
  int clusterListLen, numCluster, tmpNumCluster;
  int exchange;
  int found, action;
  uint32 sum0, sumP1, sumM1;
  T_clusterDesc *pcl, tmpCl;
  T_peakDesc *pPeakList = (T_peakDesc *)NULL;
  T_peakDesc tmpPl;
  int peakListLen, numPeak;

  if (backgroundSeparation < 0 || backgroundSeparation > 100) {
    return(CVIM_ERR_LUTANA_PARAM);
  }

  /* reset LUT */
  ierr = cvim_lutreset(pLUT);
  if( ierr != 0) {

    return(ierr);
  }

  pLUT->InverseNeedRecalc = TRUE;

  /* search maximum in histogramm (background) */
  cvim_lutanaHistMax(ps, &max, &rMax, &gMax, &bMax);

  backgroundRad = CVIM_LUTANA_COLIDX(CVIM_LUTANA_BACKGROUNDRAD) +
    ((backgroundSeparation - 50) *
     CVIM_LUTANA_COLIDX(CVIM_LUTANA_BACKGROUNDRADTOL)) / 50;

  PRINTF1("cluster Background has radius %d\n", backgroundRad - 1);

  rMax = CVIM_LUTANA_IDXTOCOL(rMax);
  gMax = CVIM_LUTANA_IDXTOCOL(gMax);
  bMax = CVIM_LUTANA_IDXTOCOL(bMax);

  /* enter color in LUT */
#ifdef use_again
  if (ierr = cvim_lutsetcolor(luthandle, 0, rMax, gMax, bMax,
                              CVIM_LUTANA_IDXTOCOLHI(backgroundRad - 1))) {
    return(ierr);
  }
#else
  ierr = cvim_lutsetcolor( pLUT, 0, rMax, gMax, bMax, 0);
  if( ierr != 0) {

    return(ierr);
  }
#endif

  /* transform into polarcoordinate 3D histogramm (alpha, rad, beta) */
  PRINTF3("rMax %d, gMax %d, bMax %d\n", rMax, gMax, bMax);
  cvim_rgb2polLightHist(ps, pTmp, rMax, gMax, bMax);

  /* lowpass */
  cvim_lutanaSpaltLP(pTmp, ps, 1);

#if TEST != 0
  if (cvim_tstpr) {
    cvim_testdraw3Dhisto(ps, 1, 1);
    cvim_testdraw3Dhisto(ps, 2, 1);
  }
#endif

  /* allocate cluster list */
  numCluster = 0;
  clusterListLen = maxColors;
  pClusterList = (T_clusterDesc *)malloc(clusterListLen * sizeof(T_clusterDesc));
  if (pClusterList == (T_clusterDesc *)NULL) {
    ierr = CVIM_ERR_LUTANA_ALLOC;
    goto error;
  }

  /* allocate peak list */
  numPeak = 0;
  peakListLen = maxColors;
  pPeakList = (T_peakDesc *)malloc(peakListLen * sizeof(T_peakDesc));
  if (pPeakList == (T_peakDesc *)NULL) {
    ierr = CVIM_ERR_LUTANA_ALLOC;
    goto error;
  }

  /* walk through all alpha/beta-planes beginning with maximum radius */
  for (rad = CVIM_LUTANA_MAX_COLORS - 1; rad >= backgroundRad; rad--) {

    PRINTF1("***Rad %d\n", rad);

    cvim_lutanaMaxInXZPlane(ps, pTmp, rad);

    numPeak = 0;
    /* now do for each peak in the alpha/beta plane */
    for (alpha = 0; alpha < CVIM_LUTANA_MAX_COLORS; alpha++) {
      for (beta = 0; beta < CVIM_LUTANA_MAX_COLORS; beta++) {
        if (pTmp[CVIM_LUTANA_HISTIDX(alpha, rad, beta)]) {
          /* peak found, enter in peak list */
          if (numPeak + 1 > peakListLen) {
            pPeakList = (T_peakDesc *)realloc((anypnt)pPeakList,
                                  (peakListLen + 10) * sizeof(T_peakDesc) /*,
                                  peakListLen * sizeof(T_peakDesc)*/);
            peakListLen += 10;
          }
          pPeakList[numPeak].x = alpha;
          pPeakList[numPeak].z = beta;
          pPeakList[numPeak].val = pTmp[CVIM_LUTANA_HISTIDX(alpha, rad, beta)];
          numPeak++;
        }
      }
    }
    /* Bubble sort peaks according value */
    do {
      exchange = FALSE;
      for(i = 0; i < numPeak - 1; i++) {
        if (pPeakList[i].val < pPeakList[i + 1].val) {
          memcpy(&tmpPl, &pPeakList[i + 1], sizeof(T_peakDesc));
          memcpy(&pPeakList[i + 1], &pPeakList[i], sizeof(T_peakDesc));
          memcpy(&pPeakList[i], &tmpPl, sizeof(T_peakDesc));
          exchange = TRUE;
        }
      }
    } while(exchange);

    PRINTF1("%d peaks: -----------------\n", numPeak);
    for (peak = 0; peak < numPeak; peak++) {
      alpha = pPeakList[peak].x;
      beta  = pPeakList[peak].z;

      PRINTF4("peak at %d/%d, val %d (%d)\n", alpha, beta,
        pTmp[CVIM_LUTANA_HISTIDX(alpha, rad, beta)],
        ps[CVIM_LUTANA_HISTIDX(alpha, rad, beta)]);

      /* check, if it's inside a cluster out of the cluster list */
      found = -1;
      for (i = 0; i < numCluster; i++) {
        if (pClusterList[i].flags & CLUSTER_DONE) continue;
        if (alpha >= pClusterList[i].xzRect[pClusterList[i].ymin].x &&
            alpha <= pClusterList[i].xzRect[pClusterList[i].ymin].x +
                     pClusterList[i].xzRect[pClusterList[i].ymin].xx - 1 &&
            beta  >= pClusterList[i].xzRect[pClusterList[i].ymin].z &&
            beta  <= pClusterList[i].xzRect[pClusterList[i].ymin].z +
                     pClusterList[i].xzRect[pClusterList[i].ymin].zz - 1) {
          found = i;
          break;
        }
      }

      if (found >= 0) {
        if (pClusterList[found].ymin == rad) {
          /* cluster was just processed for this rad */
          continue;
        }
      }

      if (found < 0) {
        /* make a new cluster entry */
        if (numCluster + 1 > clusterListLen) {
          pClusterList = (T_clusterDesc *)realloc((anypnt)pClusterList,
                                  (clusterListLen + 10) * sizeof(T_clusterDesc)/*,
                                  clusterListLen * sizeof(T_clusterDesc)*/);
          clusterListLen += 10;
        }
        if (pClusterList == (T_clusterDesc *)NULL) {
          ierr = CVIM_ERR_LUTANA_ALLOC;
          goto error;
        }
        pcl = &pClusterList[numCluster];
        pcl->xzRect[rad].x   = alpha;
        pcl->xzRect[rad].xx  = 1;
        pcl->xzRect[rad].z   = beta;
        pcl->xzRect[rad].zz  = 1;
        pcl->xzRect[rad].sum = 0;
        pcl->xmin = alpha;
        pcl->xmax = alpha;
        pcl->ymin = rad;
        pcl->ymax = rad;
        pcl->zmin = beta;
        pcl->zmax = beta;
        pcl->volume = 0;
        pcl->xGrav = 0;
        pcl->yGrav = 0;
        pcl->zGrav = 0;

        pcl->flags = 0;

        found = numCluster;
        numCluster++;
      } else {
        /* update cluster */
        pcl = &pClusterList[found];
        pcl->ymin = rad;

        pcl->xzRect[rad].x  = pcl->xzRect[rad + 1].x;
        pcl->xzRect[rad].xx = pcl->xzRect[rad + 1].xx;
        pcl->xzRect[rad].z  = pcl->xzRect[rad + 1].z;
        pcl->xzRect[rad].zz = pcl->xzRect[rad + 1].zz;

      }

      /* try to grow or shrink cluster in xz */
      pcl = &pClusterList[found];

      action = 0;

      if (cvim_lutanaClusterOverlap(pClusterList, numCluster, found,
                                    pcl->xzRect[rad].x - 1,
                                    pcl->xzRect[rad].xx + 1,
                                    pcl->xzRect[rad].z,
                                    pcl->xzRect[rad].zz) == FALSE) {
        /* try to grow the cluster left */
        cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x,
                                   pcl->xzRect[rad].z,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1,
                                   rad, &sum0, &nPix);
        cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x - 1,
                                   pcl->xzRect[rad].z,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1,
                                   rad, &sumP1, &nPix);
        if (pcl->xzRect[rad].xx > 1) {
          cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x + 1,
                                   pcl->xzRect[rad].z,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1,
                                   rad, &sumM1, &nPix);
        } else {
          sumM1 = 0;
        }
        /* if falling edge and more than 1 % area grow -> grow left,
           else if less than 1 % area shrink -> shrink left */
        if (/*sumP1 - sum0 < sum0 - sumM1 && */(sumP1 - sum0) * 100 > sum0) {
          action |= GROW_LEFT;
        } else if ((sum0 - sumM1) * 100 < sumM1) {
          action |= SHRINK_LEFT;
        }
      }
      if (cvim_lutanaClusterOverlap(pClusterList, numCluster, found,
                                    pcl->xzRect[rad].x,
                                    pcl->xzRect[rad].xx + 1,
                                    pcl->xzRect[rad].z,
                                    pcl->xzRect[rad].zz) == FALSE) {
        /* try to grow the cluster right */
        cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x,
                                   pcl->xzRect[rad].z,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1,
                                   rad, &sum0, &nPix);
        cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x,
                                   pcl->xzRect[rad].z,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1,
                                   rad, &sumP1, &nPix);
        if (pcl->xzRect[rad].xx > 1) {
          cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x,
                                   pcl->xzRect[rad].z,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 2,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1,
                                   rad, &sumM1, &nPix);
        } else {
          sumM1 = 0;
        }
        /* if falling edge and more than 1 % area grow -> grow right,
           else if less than 1 % area shrink -> shrink right */
        if (/*sumP1 - sum0 < sum0 - sumM1 && */(sumP1 - sum0) * 100 > sum0) {
          action |= GROW_RIGHT;
        } else if ((sum0 - sumM1) * 100 < sumM1) {
          action |= SHRINK_RIGHT;
        }
      }
      if (cvim_lutanaClusterOverlap(pClusterList, numCluster, found,
                                    pcl->xzRect[rad].x,
                                    pcl->xzRect[rad].xx,
                                    pcl->xzRect[rad].z - 1,
                                    pcl->xzRect[rad].zz + 1) == FALSE) {
        /* try to grow the cluster top */
        cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x,
                                   pcl->xzRect[rad].z,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1,
                                   rad, &sum0, &nPix);
        cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x,
                                   pcl->xzRect[rad].z - 1,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1,
                                   rad, &sumP1, &nPix);
        if (pcl->xzRect[rad].zz > 1) {
          cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x,
                                   pcl->xzRect[rad].z + 1,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1,
                                   rad, &sumM1, &nPix);
        } else {
          sumM1 = 0;
        }
        /* if falling edge and more than 1 % area grow -> grow top,
           else if less than 1 % area shrink -> shrink top */
        if (/*sumP1 - sum0 < sum0 - sumM1 && */(sumP1 - sum0) * 100 > sum0) {
          action |= GROW_TOP;
        } else if ((sum0 - sumM1) * 100 < sumM1) {
          action |= SHRINK_TOP;
        }
      }
      if (cvim_lutanaClusterOverlap(pClusterList, numCluster, found,
                                    pcl->xzRect[rad].x,
                                    pcl->xzRect[rad].xx,
                                    pcl->xzRect[rad].z,
                                    pcl->xzRect[rad].zz + 1) == FALSE) {
        /* try to grow the cluster bottom */
        cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x,
                                   pcl->xzRect[rad].z,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1,
                                   rad, &sum0, &nPix);
        cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x,
                                   pcl->xzRect[rad].z,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz,
                                   rad, &sumP1, &nPix);
        if (pcl->xzRect[rad].zz > 1) {
          cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x,
                                   pcl->xzRect[rad].z,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 2,
                                   rad, &sumM1, &nPix);
        } else {
          sumM1 = 0;
        }
        /* if falling edge and more than 1 % area grow -> grow bottom,
           else if less than 1 % area shrink -> shrink bottom */
        if (/*sumP1 - sum0 < sum0 - sumM1 && */(sumP1 - sum0) * 100 > sum0) {
          action |= GROW_BOTTOM;
        } else if ((sum0 - sumM1) * 100 < sumM1) {
          action |= SHRINK_BOTTOM;
        }
      }

      /* adapt actual rectangle */
      if (action & GROW_LEFT) {
        pcl->xzRect[rad].x = pcl->xzRect[rad].x - 1;
        pcl->xzRect[rad].xx = pcl->xzRect[rad].xx + 1;
      } else if (action & SHRINK_LEFT) {
        pcl->xzRect[rad].x = pcl->xzRect[rad].x + 1;
        pcl->xzRect[rad].xx = pcl->xzRect[rad].xx - 1;
      }
      if (action & GROW_RIGHT) {
        pcl->xzRect[rad].xx = pcl->xzRect[rad].xx + 1;
      } else if (action & SHRINK_RIGHT) {
        pcl->xzRect[rad].xx = pcl->xzRect[rad].xx - 1;
      }
      if (action & GROW_TOP) {
        pcl->xzRect[rad].z = pcl->xzRect[rad].z - 1;
        pcl->xzRect[rad].zz = pcl->xzRect[rad].zz + 1;
      } else if (action & SHRINK_TOP) {
        pcl->xzRect[rad].z = pcl->xzRect[rad].z + 1;
        pcl->xzRect[rad].zz = pcl->xzRect[rad].zz - 1;
      }
      if (action & GROW_BOTTOM) {
        pcl->xzRect[rad].zz = pcl->xzRect[rad].zz + 1;
      } else if (action & SHRINK_BOTTOM) {
        pcl->xzRect[rad].zz = pcl->xzRect[rad].zz - 1;
      }

      /* measure rectangle (update values) */

      /* adapt minmax rectangle */
      if (pcl->xzRect[rad].x < pcl->xmin) {
        pcl->xmin = pcl->xzRect[rad].x;
      }
      if (pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1 > pcl->xmax) {
        pcl->xmax = pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1;
      }
      if (pcl->xzRect[rad].z < pcl->zmin) {
        pcl->zmin = pcl->xzRect[rad].z;
      }
      if (pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1 > pcl->zmax) {
        pcl->zmax = pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1;
      }

      cvim_lutanaXZClusterMeasUpdate(ps, pcl->xzRect[rad].x,
                               pcl->xzRect[rad].z,
                               pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1,
                               pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1,
                               rad, &pcl->volume, &nPix,
                               &pcl->xGrav, &pcl->yGrav, &pcl->zGrav,
                               &pcl->xzRect[rad].sum);

#if TEST != 0
      if (cvim_tstpr) {
        cvim_testdraw3DXZrect(rad, pcl->xzRect[rad].x,
                              pcl->xzRect[rad].xx, pcl->xzRect[rad].z,
                              pcl->xzRect[rad].zz, 1, 255);
      }
#endif

    }

    /* check for unprocessed clusters for this rad */
    for (i = 0; i < numCluster; i++) {
      if (pClusterList[i].ymin > rad) {
        pClusterList[i].flags |= CLUSTER_DONE;
      }
    }

  }

  PRINTF1("---------------- %d cluster found\n", numCluster);

  /* calculate measurement values */
  for(i = 0; i < numCluster ; i++) {
    pcl = &pClusterList[i];
    if (pClusterList[i].volume <= 0) pClusterList[i].volume = 1; /* be sure */
    pClusterList[i].xGrav /= pClusterList[i].volume;
    pClusterList[i].yGrav /= pClusterList[i].volume;
    pClusterList[i].zGrav /= pClusterList[i].volume;
  }

#ifdef use_again
  /* Bubble sort clusters according volume */
  do {
    exchange = FALSE;
    for(i = 0; i < numCluster - 1; i++) {
      if (pClusterList[i].volume < pClusterList[i + 1].volume) {
        memcpy(&tmpCl, &pClusterList[i + 1], sizeof(T_clusterDesc));
        memcpy(&pClusterList[i + 1], &pClusterList[i], sizeof(T_clusterDesc));
        memcpy(&pClusterList[i], &tmpCl, sizeof(T_clusterDesc));
        exchange = TRUE;
      }
    }
  } while(exchange);

  /* watch only the best (maxColors - 1) clusters */
  if (numCluster > maxColors - 1) numCluster = maxColors - 1;

  /* try to cut very long clusters (in radial direction),
     don't watch the last cluster because it would become only smaller */
  tmpNumCluster = numCluster - 1;
#else
  /* try to cut very long clusters (in radial direction) */
  tmpNumCluster = numCluster;
#endif
  for (i = 0; i < tmpNumCluster; i++) {

    int sideRatio, rTol, clusterRadSoll, clusterRad, radMin, radius0, radius1, lastCluster, nSubClusters;
    uint32 min;

    pcl = &pClusterList[i];

    clusterRad = pcl->ymax - pcl->ymin + 1;
    sideRatio = clusterRad /
             (((pcl->xmax - pcl->xmin + 1) + (pcl->zmax - pcl->zmin + 1)) / 2);
    nSubClusters = 1 + sideRatio / 2;
    radius0 = pcl->ymin;
    radius1 = pcl->ymax;

    PRINTF5("%d: alpha: %d...%d, beta: %d...%d\n", i + 1, pcl->xmin, pcl->xmax,
                                               pcl->zmin, pcl->zmax);
    PRINTF3("rad: %d...%d, sideratio %d\n", pcl->ymin, pcl->ymax, sideRatio);
    if (nSubClusters > 1 && clusterRad > (4 * nSubClusters)) {
      PRINTF3("cut cluster %d, sideRatio %d into %d parts \n", i + 1,
              sideRatio, nSubClusters);
#if TEST != 0
      for (j = pcl->ymax; j >= pcl->ymin; j--) {
        PRINTF1("%d ", pcl->xzRect[j].sum);
      }
      PRINTF0("\n");
#endif
      rTol = clusterRad / (nSubClusters * 4);
      lastCluster = -1;
      for (j = 1; j < nSubClusters; j++) {
        clusterRadSoll = radius0 + (clusterRad * j) / nSubClusters;
        min = 0xffffffff;
        radMin = -1;
        for (rad = clusterRadSoll - rTol; rad <= clusterRadSoll + rTol; rad++) {
#ifdef use_again
          cvim_lutanaXZGetClusterSum(ps, pcl->xzRect[rad].x,
                                   pcl->xzRect[rad].z,
                                   pcl->xzRect[rad].x + pcl->xzRect[rad].xx - 1,
                                   pcl->xzRect[rad].z + pcl->xzRect[rad].zz - 1,
                                   rad, &sum0, &nPix);
#else
          sum0 = pcl->xzRect[rad].sum;
#endif
          PRINTF1("%d ", sum0);
          if (sum0 < min) {
            min = sum0;
            radMin = rad;
          }
        }
        PRINTF3("\nsearch rad %d...%d, found %d\n", clusterRadSoll - rTol,
                clusterRadSoll + rTol, radMin);

        if (radMin < 0) continue;

        if (j == 1) {
          /* modify actual cluster */
          pcl->ymax = radMin;
          cvim_lutanaClusterMeas(ps, pcl->ymin, pcl->ymax, pcl->xzRect,
                                 &pcl->volume, &nPix,
                                 &pcl->xGrav, &pcl->yGrav, &pcl->zGrav,
                                 &pcl->xmin, &pcl->xmax,
                                 &pcl->zmin, &pcl->zmax);
          lastCluster = i;
        } else {
          pClusterList[lastCluster].ymax = radMin;
          cvim_lutanaClusterMeas(ps, pClusterList[lastCluster].ymin,
                                 pClusterList[lastCluster].ymax,
                                 pClusterList[lastCluster].xzRect,
                                 &pClusterList[lastCluster].volume, &nPix,
                                 &pClusterList[lastCluster].xGrav,
                                 &pClusterList[lastCluster].yGrav,
                                 &pClusterList[lastCluster].zGrav,
                                 &pClusterList[lastCluster].xmin,
                                 &pClusterList[lastCluster].xmax,
                                 &pClusterList[lastCluster].zmin,
                                 &pClusterList[lastCluster].zmax);
        }
        /* append a new cluster entry */
        if (numCluster + 1 > clusterListLen) {
          pClusterList = (T_clusterDesc *)realloc((anypnt)pClusterList,
                                  (clusterListLen + 10) * sizeof(T_clusterDesc)/*,
                                  clusterListLen * sizeof(T_clusterDesc)*/);
          clusterListLen += 10;
        }
        if (pClusterList == (T_clusterDesc *)NULL) {
          ierr = CVIM_ERR_LUTANA_ALLOC;
          goto error;
        }
        pcl = &pClusterList[i];

        memcpy(pClusterList[numCluster].xzRect, pcl->xzRect,
               CVIM_LUTANA_MAX_COLORS * sizeof(T_xzRect));
        pClusterList[numCluster].ymin = radMin + 1;
        pClusterList[numCluster].flags = 0;

        lastCluster = numCluster;
        numCluster++;

      }
      if (lastCluster >= 0) {
        pClusterList[lastCluster].ymax = radius1;
        cvim_lutanaClusterMeas(ps, pClusterList[lastCluster].ymin,
                               pClusterList[lastCluster].ymax,
                               pClusterList[lastCluster].xzRect,
                               &pClusterList[lastCluster].volume, &nPix,
                               &pClusterList[lastCluster].xGrav,
                               &pClusterList[lastCluster].yGrav,
                               &pClusterList[lastCluster].zGrav,
                               &pClusterList[lastCluster].xmin,
                               &pClusterList[lastCluster].xmax,
                               &pClusterList[lastCluster].zmin,
                               &pClusterList[lastCluster].zmax);
      }
    }
  }

  /* transform gravity points back to RGB space */
  for (i = 0; i < numCluster; i++) {

    pcl = &pClusterList[i];
    alpha = pcl->xGrav;
    beta  = pcl->zGrav;
    rad   = pcl->yGrav;

    /* convert back to rgb space */
    cvim_pol2rgb(CVIM_LUTANA_IDXTOCOL(alpha),
                 CVIM_LUTANA_IDXTOCOL(beta),
                 CVIM_LUTANA_IDXTOCOL(rad),
                 &r, &g, &b);

    /* relative to background */
    r = rMax - r;
    if (r < 0) r = 0;
    g = gMax - g;
    if (g < 0) g = 0;
    b = bMax - b;
    if (b < 0) b = 0;

    pcl->r = r;
    pcl->g = g;
    pcl->b = b;

    PRINTF4("%d: %d %d %d\n", i + 1, r, g, b);
    PRINTF4("alpha: %d...%d, beta: %d...%d\n", pcl->xmin, pcl->xmax,
                                               pcl->zmin, pcl->zmax);
    PRINTF2("rad: %d...%d\n", pcl->ymin, pcl->ymax);
  }

  /* if we have more than the maxColors - 1 colors, unify clusters */
  while (numCluster > maxColors - 1) {
    double dist, minDist;
    int minI, minJ;
    int dr, dg, db;

    minDist = 999999.9;
    minI = -1;
    minJ = -1;
    for(i = 0; i < numCluster - 1; i++) {
      for(j = i + 1; j < numCluster; j++) {
        dr = pClusterList[i].r - pClusterList[j].r;
        dg = pClusterList[i].g - pClusterList[j].g;
        db = pClusterList[i].b - pClusterList[j].b;
        dist = sqrt((double)dr * dr + (double)dg * dg + (double)db * db);
        if (dist < minDist) {
          minDist = dist;
          minI = i;
          minJ = j;
        }
      }
    }
    if (minI < 0 || minJ < 0) break; /* should not happen */

    i = minI;
    j = minJ;

    PRINTF2("unify clusters %d and %d\n", i, j);
    PRINTF4("rgb: %d %d %d, vol %d\n", pClusterList[i].r, pClusterList[i].g,
            pClusterList[i].b, pClusterList[i].volume);
    PRINTF4("rgb: %d %d %d, vol %d\n", pClusterList[j].r, pClusterList[j].g,
            pClusterList[j].b, pClusterList[j].volume);

    /* update cluster i */
    pClusterList[i].r = (pClusterList[i].r * pClusterList[i].volume +
                         pClusterList[j].r * pClusterList[j].volume) /
                         (pClusterList[i].volume + pClusterList[j].volume);
    if (pClusterList[i].r > 255) pClusterList[i].r = 255;
    if (pClusterList[i].r < 0) pClusterList[i].r = 0;
    pClusterList[i].g = (pClusterList[i].g * pClusterList[i].volume +
                         pClusterList[j].g * pClusterList[j].volume) /
                         (pClusterList[i].volume + pClusterList[j].volume);
    if (pClusterList[i].g > 255) pClusterList[i].g = 255;
    if (pClusterList[i].g < 0) pClusterList[i].g = 0;
    pClusterList[i].b = (pClusterList[i].b * pClusterList[i].volume +
                         pClusterList[j].b * pClusterList[j].volume) /
                         (pClusterList[i].volume + pClusterList[j].volume);
    if (pClusterList[i].b > 255) pClusterList[i].b = 255;
    if (pClusterList[i].b < 0) pClusterList[i].b = 0;

    pClusterList[i].volume += pClusterList[j].volume;

    PRINTF4("updated: %d %d %d, vol %d\n", pClusterList[i].r, pClusterList[i].g,
            pClusterList[i].b, pClusterList[i].volume);

    if (j < numCluster - 1) {
      memcpy(&pClusterList[j], &pClusterList[numCluster - 1],
             sizeof(T_clusterDesc));
    }

    numCluster--;

  }

  /* Bubble sort clusters according volume */
  do {
    exchange = FALSE;
    for(i = 0; i < numCluster - 1; i++) {
      if (pClusterList[i].volume < pClusterList[i + 1].volume) {
        memcpy(&tmpCl, &pClusterList[i + 1], sizeof(T_clusterDesc));
        memcpy(&pClusterList[i + 1], &pClusterList[i], sizeof(T_clusterDesc));
        memcpy(&pClusterList[i], &tmpCl, sizeof(T_clusterDesc));
        exchange = TRUE;
      }
    }
  } while(exchange);

  for (i = 0; i < numCluster; i++) {
    if (i >= (maxColors - 1)) break;

    pcl = &pClusterList[i];
#ifdef use_again
    alpha = pcl->xGrav;
    beta  = pcl->zGrav;
    rad   = pcl->yGrav;

    /* convert back to rgb space */
    cvim_pol2rgb(CVIM_LUTANA_IDXTOCOL(alpha),
                 CVIM_LUTANA_IDXTOCOL(beta),
                 CVIM_LUTANA_IDXTOCOL(rad),
                 &r, &g, &b);

    /* relative to background */
    r = rMax - r;
    if (r < 0) r = 0;
    g = gMax - g;
    if (g < 0) g = 0;
    b = bMax - b;
    if (b < 0) b = 0;
    PRINTF4("%d: %d %d %d\n", i + 1, r, g, b);
    PRINTF4("alpha: %d...%d, beta: %d...%d\n", pcl->xmin, pcl->xmax,
                                               pcl->zmin, pcl->zmax);
    PRINTF2("rad: %d...%d\n", pcl->ymin, pcl->ymax);
    /* enter color in LUT (first is background) */
    if (ierr = cvim_lutsetcolor(luthandle, i + 1, r, g, b, 0)) {
      goto error;
    }
#else

    PRINTF4("%d: %d %d %d\n", i + 1, pcl->r, pcl->g, pcl->b);
    PRINTF4("alpha: %d...%d, beta: %d...%d\n", pcl->xmin, pcl->xmax,
                                               pcl->zmin, pcl->zmax);
    PRINTF2("rad: %d...%d\n", pcl->ymin, pcl->ymax);

    /* enter color in LUT (first is background) */
    ierr = cvim_lutsetcolor( pLUT, i + 1, pcl->r, pcl->g, pcl->b, 0);
    if( ierr != 0) {
      goto error;
    }

#endif

  }


  ierr = 0;
error:

  if (pPeakList != (T_peakDesc *)NULL) {
    free((void *)pPeakList);
  }
  if (pClusterList != (T_clusterDesc *)NULL) {
    free((void *)pClusterList);
  }

  return(ierr);

}

/***************************************************************************
* cvim_lutanainit()
*
* initialize LUT analysis
****************************************************************************
*/

static int cvim_lutanainit()
{
  int i;

  if( lutanaDesc.initialized) goto exitPoint;

  lutanaDesc.phisto3d = (uint32 *)malloc(CVIM_LUTANA_MAX_COLORS *
                                             CVIM_LUTANA_MAX_COLORS *
                                             CVIM_LUTANA_MAX_COLORS *
                                             sizeof(uint32));
  if (lutanaDesc.phisto3d == (uint32 *)NULL) {
    return(CVIM_ERR_LUTANA_ALLOC);
  }
  lutanaDesc.phisto3dTmp = (uint32 *)malloc(CVIM_LUTANA_MAX_COLORS *
                                                CVIM_LUTANA_MAX_COLORS *
                                                CVIM_LUTANA_MAX_COLORS *
                                                sizeof(uint32));
  if (lutanaDesc.phisto3dTmp == (uint32 *)NULL) {
    free((anypnt)lutanaDesc.phisto3d);
    return(CVIM_ERR_LUTANA_ALLOC);
  }

  for (i = 0; i < CVIM_LUTANA_MAX_COLORS *
                  CVIM_LUTANA_MAX_COLORS *
                  CVIM_LUTANA_MAX_COLORS; i++) {
    lutanaDesc.phisto3d[i] = 0;
  }

exitPoint:
  lutanaDesc.initialized++;
  return(0);

} /* int cvim_lutanainit() */

/***************************************************************************
* cvim_lutana()
*
* initialize LUT analysis
****************************************************************************
*/

static int cvim_lutana( YaIPS_RGB_ImgD_t *pISrc)
{
  int x, y, xx, yy, d;
  uchar *s8;

  if (lutanaDesc.initialized == 0) return(CVIM_ERR_LUTANA_NINIT);

  xx = pISrc->xx;
  yy = pISrc->yy;
  d  = pISrc->d;

  if( d < 0 || d > 4)                     // Check color format out of range
    return(CVIM_ERR_LUTANA_PIXFORMAT);

  /* add to 3D RGB histogram */

  for( y = 0; y < yy; y++) {

    s8 = RGB_pixad( 0, y, pISrc);

    if( d >= 3) {                         // Is a color image

      for( x = 0; x < xx; x++) {


        lutanaDesc.phisto3d[
              CVIM_LUTANA_HISTIDX( CVIM_LUTANA_COLIDX( btoi( s8[0])),
                                   CVIM_LUTANA_COLIDX( btoi( s8[1])),
                                   CVIM_LUTANA_COLIDX( btoi( s8[2])))] += 1;

        s8 += d;
      }

    } else {                              // Is a black white image

      for( x = 0; x < xx; x++) {


        lutanaDesc.phisto3d[
              CVIM_LUTANA_HISTIDX( CVIM_LUTANA_COLIDX( btoi( s8[0])),
                                   CVIM_LUTANA_COLIDX( btoi( s8[0])),
                                   CVIM_LUTANA_COLIDX( btoi( s8[0])))] += 1;

        s8 += d;
      }
    }
  }

  return(0);

} /* int cvim_lutana() */

/***************************************************************************
* cvim_lutanaend()
*
* LUT analysis, perform the real analysis
****************************************************************************
*/

static int cvim_lutanaend( CVIM_LUT_T       *pLUT,           /* LUT to write to */
                           int              i_maxcolors,     /* maximum colors to extract */
                           CVIM_LUTANA_ALGO_T i_algorithm,   /* algorithm                 */
                           int              i_parameter)     /* algorithm parameter       */
{
  int ierr;

  if (lutanaDesc.initialized == 0) return(CVIM_ERR_LUTANA_NINIT);

#if TEST != 0
  if (cvim_tstpr) {
    cvim_testdraw3Dhisto(lutanaDesc.phisto3d, 0, 1);
  }
#endif


  switch( i_algorithm) {
    case (int)cvim_lutana_popularity:
      cvim_lutanaSpaltLP(lutanaDesc.phisto3d, lutanaDesc.phisto3dTmp, 2);

#if TEST != 0
      if (cvim_tstpr) {
        cvim_testdraw3Dhisto(lutanaDesc.phisto3dTmp, 1, 1);
      }
#endif
      ierr = cvim_lutanaPopularity( lutanaDesc.phisto3dTmp,
                                    pLUT, i_maxcolors,
                                    i_parameter);

      if( ierr != 0) {

        return(ierr);
      }
      break;

    case (int)cvim_lutana_document:
      ierr = cvim_lutanaDocument( lutanaDesc.phisto3d,
                                  lutanaDesc.phisto3dTmp,
                                  pLUT, i_maxcolors, i_parameter);

      if( ierr != 0) {

        return(ierr);
      }
      break;

    default:
      return(CVIM_ERR_LUTANA_PARAM);
      break;
  }

  if (lutanaDesc.phisto3d) free((anypnt)lutanaDesc.phisto3d);
  lutanaDesc.phisto3d = (uint32)NULL;
  if (lutanaDesc.phisto3dTmp) free((anypnt)lutanaDesc.phisto3dTmp);
  lutanaDesc.phisto3dTmp = (uint32)NULL;

  lutanaDesc.initialized = 0;

  return(0);

} /* int cvim_lutanaend() */

/***************************************************************************
* YaIPS_RGB_PosterizeEx1
* A more complex image posterization
*
* piDst        Pointer to pointer to RGB image
* piSrc        Source image
*
* nLevels      Number of color levels. Range is from 2 .. 32
* ColorSpace   Used color space for posterization
*              Supported until now is YAIPS_DISP_COLMOD_NORMAL and YAIPS_DISP_COLMOD_BW.
* i_maxcolors  Maximum colors to extract
* i_algorithm  Algorithm type
*                    0 = 'Popularity' algorithm. The most frequent colors are determined from
*                        an RGB histogram.
*                 else = 'Drawing' algorithm. This algorithm is specifically optimized for
*                        documents and drawings on a light background.
* i_parameter  Algorithm parameter
*              Range 0 .. 100.
*
* return    >= Number of colors in the posterized output image.
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_PosterizeEx1( Fl_RGB_Image **ppDst,             // Out: Pointer to pointer to RGB image
                            Fl_RGB_Image *pSrc,               // Source image
                            int i_maxcolors,                  // Maximum colors to extract */
                            int i_algorithm,                  // Algorithm type
                            int i_parameter)                  // Algorithm parameter
{
  int ierr, x, y, xx, yy, nColorsSrc, SrcHasAlpha;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8;
  uint8 *pInvLut;
  CVIM_LUT_T TempLUT = { 0};

  // Check/Clip parameter

  if( i_maxcolors < 2) {

    i_maxcolors = 2;
  }

  if( i_maxcolors > YAIPS_RLC_POSTER_MAX_LEVEL) {

    i_maxcolors = YAIPS_RLC_POSTER_MAX_LEVEL;
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

  // Get image data of source

  xx    = iSrc.xx;
  yy    = iSrc.yy;

  nColorsSrc = iSrc.d >= 3 ? 3 : 1;

  //x/nBytesSrc  = iSrc.d;

  SrcHasAlpha = iSrc.d == 2 || iSrc.d == 4;

  // Setup LUT

  cvim_lutcreate( &TempLUT, 8);               // Create LUT

  /* initialize color analyze */

  ierr = cvim_lutanainit();

  if( ierr != 0) {

   //x/printf("cvim_lutanainit() returns %d\n", ierr);

    goto exit_point_7;
  }

  /* feed the image data to the color analyzer */

  ierr = cvim_lutana( &iSrc);

  if( ierr != 0) {

    //x/printf("cvim_lutana() returns %d\n", ierr);
    goto exit_point_7;
  }

  /* finish the color analyze */

  ierr = cvim_lutanaend( &TempLUT, i_maxcolors,
                         i_algorithm == 0 ? cvim_lutana_popularity : cvim_lutana_document, i_parameter);

  if( ierr != 0) {

    //x/printf("cvim_lutanaend() returns %d\n", ierr);
    goto exit_point_7;
  }

  // Convert LUT back as RGB Image

  pInvLut = cvim_lutgetpinverse( &TempLUT);

  if( pInvLut == (uint8 *)NULL) {

    //x/PRINTF0("cvim_slconvert: can't get inverse LUT\n");
    goto exit_point_7;
  }

  for( y = 0; y < yy; y++) {

    int rVal, gVal, bVal, InvIndex, LutIndex;

    s8 = RGB_pixad( 0, y, &iSrc);
    d8 = RGB_pixad( 0, y, &iDst);

    if( nColorsSrc == 1) {        // BW image

      for( x = 0; x < xx; x++) {

        rVal = (*s8++ & 0xff) + CVIM_LUT_INVERSE_ROUND;

        rVal >>= (8 - CVIM_LUT_INVERSE_BITS);

        InvIndex = (rVal << (CVIM_LUT_INVERSE_BITS * 2)) |
                   (rVal << CVIM_LUT_INVERSE_BITS) | rVal;

        LutIndex = pInvLut[ InvIndex];

        *d8++ = TempLUT.LUT[ LutIndex].r;

        if( SrcHasAlpha) {        // Copy alpha

          *d8++ = *s8++;
        }
      }

    } else {                      // RGB image

      for( x = 0; x < xx; x++) {

        rVal = (*s8++ & 0xff) + CVIM_LUT_INVERSE_ROUND;
        gVal = (*s8++ & 0xff) + CVIM_LUT_INVERSE_ROUND;
        bVal = (*s8++ & 0xff) + CVIM_LUT_INVERSE_ROUND;

        if( rVal > 255) rVal = 255;
        if( gVal > 255) gVal = 255;
        if( bVal > 255) bVal = 255;

        rVal >>= (8 - CVIM_LUT_INVERSE_BITS);
        gVal >>= (8 - CVIM_LUT_INVERSE_BITS);
        bVal >>= (8 - CVIM_LUT_INVERSE_BITS);

        InvIndex = (bVal << (CVIM_LUT_INVERSE_BITS * 2)) |
                   (gVal << CVIM_LUT_INVERSE_BITS) | rVal;

        LutIndex = pInvLut[ InvIndex];

        *d8++ = TempLUT.LUT[ LutIndex].r;
        *d8++ = TempLUT.LUT[ LutIndex].g;
        *d8++ = TempLUT.LUT[ LutIndex].b;

        if( SrcHasAlpha) {        // Copy alpha

          *d8++ = *s8++;
        }
      }
    }
  }

  ierr = TempLUT.nColors;                     // Return colors in the posterized output image

exit_point_7:                                 // Error exit point

  cvim_lutclose( &TempLUT);

  return( ierr);
}

/*--------------------------------------------------------------------------
* k-Means posterization algorithm
*---------------------------------------------------------------------------
*/


/***************************************************************************
* Help functions: RGB ↔ Lab
****************************************************************************
*/

static float pivot(float n) {
    return (n > 0.008856f) ? powf(n, 1.0f/3.0f) : (7.787f*n + 16.0f/116.0f);
}

static void rgb_to_lab(unsigned char R, unsigned char G, unsigned char B,
                float* L, float* A, float* Bc)
{
    float r = R/255.0f;
    float g = G/255.0f;
    float b = B/255.0f;

    // Gamma Correction
    r = (r > 0.04045f) ? powf((r+0.055f)/1.055f, 2.4f) : r/12.92f;
    g = (g > 0.04045f) ? powf((g+0.055f)/1.055f, 2.4f) : g/12.92f;
    b = (b > 0.04045f) ? powf((b+0.055f)/1.055f, 2.4f) : b/12.92f;

    // RGB → XYZ
    float X = r*0.4124f + g*0.3576f + b*0.1805f;
    float Y = r*0.2126f + g*0.7152f + b*0.0722f;
    float Z = r*0.0193f + g*0.1192f + b*0.9505f;

    // Standardization to D65
    float Xn = X / 0.95047f;
    float Yn = Y / 1.00000f;
    float Zn = Z / 1.08883f;

    float fx = pivot(Xn);
    float fy = pivot(Yn);
    float fz = pivot(Zn);

    *L  = 116.0f * fy - 16.0f;
    *A  = 500.0f * (fx - fy);
    *Bc = 200.0f * (fy - fz);
}

static float invpivot(float n) {
    float n3 = n*n*n;
    return (n3 > 0.008856f) ? n3 : (n - 16.0f/116.0f)/7.787f;
}

static void lab_to_rgb( float L, float A, float Bc,
                        unsigned char* R, unsigned char* G, unsigned char* B)
{
    float fy = (L + 16.0f)/116.0f;
    float fx = A/500.0f + fy;
    float fz = fy - Bc/200.0f;

    float X = 0.95047f * invpivot(fx);
    float Y = 1.00000f * invpivot(fy);
    float Z = 1.08883f * invpivot(fz);

    float r =  3.2406f*X - 1.5372f*Y - 0.4986f*Z;
    float g = -0.9689f*X + 1.8758f*Y + 0.0415f*Z;
    float b =  0.0557f*X - 0.2040f*Y + 1.0570f*Z;

    // Gamma
    r = (r > 0.0031308f) ? 1.055f*powf(r, 1.0f/2.4f) - 0.055f : 12.92f*r;
    g = (g > 0.0031308f) ? 1.055f*powf(g, 1.0f/2.4f) - 0.055f : 12.92f*g;
    b = (b > 0.0031308f) ? 1.055f*powf(b, 1.0f/2.4f) - 0.055f : 12.92f*b;

    *R = (unsigned char)(fmaxf(0.0f, fminf(1.0f, r)) * 255);
    *G = (unsigned char)(fmaxf(0.0f, fminf(1.0f, g)) * 255);
    *B = (unsigned char)(fmaxf(0.0f, fminf(1.0f, b)) * 255);
}

/***************************************************************************
* k-Means to Lab color space
****************************************************************************
*/

typedef struct {
    float L, A, B;
} LabColor;

static float lab_dist(LabColor a, LabColor b) {

  return (a.L-b.L)*(a.L-b.L) +
         (a.A-b.A)*(a.A-b.A) +
         (a.B-b.B)*(a.B-b.B);
}

static int choose_center_kmeanspp( float *dist, LabColor* lab, int pixels, LabColor* centers, int count)
{

  // Distance from each pixel to the nearest existing center
  for (int p = 0; p < pixels; p++) {

    float best = 1e30f;

    for (int c = 0; c < count; c++) {

      float d = lab_dist(lab[p], centers[c]);
      if (d < best) best = d;
    }

    dist[p] = best;
  }

  // Sum of Distances
  float sum = 0.0f;
  for (int p = 0; p < pixels; p++) {

    sum += dist[p];
  }

  // Random value proportional to distance
  float r = ((float)rand() / RAND_MAX) * sum;

  // Select a pixel
  float acc = 0.0f;

  for (int p = 0; p < pixels; p++) {

    acc += dist[p];

    if (acc >= r) {

      return p;
    }
  }

  return pixels - 1;
}

/***************************************************************************
* YaIPS_RGB_PosterizeSimple
* k-Means posterization algorithm.
*
* piDst        Pointer to pointer to RGB image
* piSrc        Source image
*
* nLevels      In: Number of color levels. Range is from 2 .. 32
* Average      In: If set average colors
*
* return    >= Number of colors in the posterized output image.
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_PosterizeKmeans( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                               Fl_RGB_Image *pSrc,   // Source image
                               int nLevels,          // In: Number of levels
                               int nIterations)      // In: If set average colors

{
  int ierr, x, y, width, height, nBytesSrc, SrcHasAlpha;
  int first, label;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8, R, G, B;
  int *pLabels, *labels = NULL;
  LabColor *pCenters, *centers = NULL;
  LabColor *pLab, *lab = NULL;
  float *dist = NULL;

  // Check/Clip parameter

  if( nLevels < 2) {

    nLevels = 2;
  }

  if( nLevels > YAIPS_RLC_POSTER_MAX_LEVEL) {

    nLevels = YAIPS_RLC_POSTER_MAX_LEVEL;
  }

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  if( iSrc.d < 3) {                           // No color image

    return( -4711); // Source image is no color image)
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

  // Get image data of source

  width  = iSrc.xx;
  height = iSrc.yy;

  nBytesSrc  = iSrc.d;

  //x/nBytesSrc  = iSrc.d;

  SrcHasAlpha = iSrc.d == 2 || iSrc.d == 4;

  int pixels = width * height;

  lab = (LabColor *)malloc( sizeof(LabColor) * pixels);

  if( lab == NULL) {

    ierr = 100;     // Memory allocation failed
    goto ExitPoint;
  }

  // RGB → Lab

  pLab = lab;
  for( y = 0; y < height; y++) {

    s8 = RGB_pixad( 0, y, &iSrc);

    for( x = 0; x < width; x++) {

      rgb_to_lab( s8[0], s8[1], s8[2],
                  &pLab->L, &pLab->A, &pLab->B);

      s8 += nBytesSrc;
      pLab++;
    }
  }

  centers = (LabColor *)malloc( sizeof( LabColor) * nLevels);

  if( centers == NULL) {

    ierr = 101;     // Memory allocation failed
    goto ExitPoint;
  }

  labels = (int *)malloc( sizeof(int) * pixels);

  if( labels == NULL) {

    ierr = 102;     // Memory allocation failed
    goto ExitPoint;
  }

#ifdef use_again
  // Initialization
  for (int i = 0; i < nLevels; i++) {

    int r = rand() % pixels;
    centers[i] = lab[r];
  }
#else
  // k-Means++ Initialization
  first = rand() % pixels;
  centers[0] = lab[first];

  dist =(float *)malloc(sizeof(float) * pixels);

  if( dist == NULL) {

    ierr = 103;     // Memory allocation failed
    goto ExitPoint;
  }

  for (int i = 1; i < nLevels; i++) {

    int idx = choose_center_kmeanspp( dist, lab, pixels, centers, i);
    centers[i] = lab[idx];
  }
#endif

  // k-Means Iterations
  for (int it = 0; it < nIterations; it++) {

    // Best match
    pLab = lab;
    pLabels = labels;
    for( int p = 0; p < pixels; p++) {

      float best = 1e30;
      int bestLabel = 0;

      pCenters = centers;
      for (int i = 0; i < nLevels; i++) {

        float d = lab_dist( *pLab, *pCenters++);

        if( d < best) {

          best = d;
          bestLabel = i;
        }
      }

      pLab++;

      *pLabels++ = bestLabel;
    }

    // Recalculate centers
#ifdef use_again
    LabColor* sum = (LabColor *)calloc( nLevels, sizeof(LabColor));
    if( sum == NULL) {

      ierr = 102;     // Memory allocation failed
      goto ExitPoint;
    }

    int* count = (int *)calloc( nLevels, sizeof(int));
    if( count == NULL) {

      ierr = 102;     // Memory allocation failed
      goto ExitPoint;
    }
#else
    // nLevels is clipped to maximum.
    // Fixed length table is faster then malloc and free
    LabColor sum[ YAIPS_RLC_POSTER_MAX_LEVEL];
    int count[ YAIPS_RLC_POSTER_MAX_LEVEL];

    memset( sum, 0, sizeof( sum));
    memset( count, 0, sizeof( count));
#endif

    for (int p = 0; p < pixels; p++) {

      label = labels[p];
      sum[label].L += lab[p].L;
      sum[label].A += lab[p].A;
      sum[label].B += lab[p].B;
      count[label]++;
    }

    for (int i = 0; i < nLevels; i++) {

      if (count[i] > 1) {

        centers[i].L = sum[i].L / count[i];
        centers[i].A = sum[i].A / count[i];
        centers[i].B = sum[i].B / count[i];
      }
    }

#ifdef use_again
    free(sum);
    free(count);
#endif
  }

  // Create poster image

  pLabels = labels;

  for( y = 0; y < height; y++) {

    s8 = RGB_pixad( 0, y, &iSrc);
    d8 = RGB_pixad( 0, y, &iDst);

    for( x = 0; x < width; x++) {

      label = *pLabels++;

      lab_to_rgb( centers[label].L, centers[label].A, centers[label].B,
                   &R, &G, &B);

      d8[0] = R;
      d8[1] = G;
      d8[2] = B;

      d8 += 3;

      if( SrcHasAlpha) {        // Copy alpha
        s8 += 3;
        *d8++ = *s8++;
      }
    }
  }

  ierr = nLevels;

ExitPoint:

  if( lab != NULL) {
    free( lab);
  }

  if( centers != NULL) {
    free( centers);
  }

  if( labels != NULL) {
    free( labels);
  }

  if( dist != NULL) {
    free( dist);
  }

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_PosterizeEdges
*
* Blend edges into already posterized image.
*
* piDst        InOut: Already posterized
* piSrc        In: Source image
*
* CannySigma    In: Sigma for gaussian filter
* CannyResMult  In: Edge strength
* EdgeCol       In: Edge color
* EdgeHighlight In: Highlights stronger edges. 0 = no, 100 = max highlight.
* EdgeStrength  In: Edge strength. 0 = no, 100 max strength.
*
* return    >= Number of colors in the posterized output image.
*          < 0 Error
****************************************************************************
*/
int YaIPS_RGB_PosterizeEdges( Fl_RGB_Image *pDst,   // InOut: Already posterized.
                              Fl_RGB_Image *pSrc,   // In: Source image
                              float CannySigma,     // In: Sigma for gaussian filter
                              float CannyResMult,   // In: Edge strength
                              unsigned int EdgeCol, // In: Edge color
                              int EdgeHighlight,    // In: Highlights stronger edges. 0 = no, 100 = max highlight.
                              int EdgeStrength)     // In: Edge strength. 0 = no, 100 max strength.
{
  int ierr, x, y, xx, yy, nBytesSrc, nColorsSrc, a;
  uchar r, g, b;
  Fl_RGB_Image *pTmp = NULL;
  YaIPS_RGB_ImgD_t iDst, iSrc, iTmp;
  uchar *s8, *d8;
  uchar LutContrast[ YAIPS_LUT_N_POINTS];

  // Check destination first, must exist
  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Source and destination must have same size and number of pixels

  if( iDst.xx != iSrc.xx ||
      iDst.yy != iSrc.yy ||
      iDst.d != iSrc.d) {

    return( -100);
  }

  // Create temporary edge image

  ierr = YaIPS_RGB_Canny( &pTmp, NULL, pSrc, CannySigma, CannyResMult, 0);

  if( ierr != 0) {

    goto ErrorExit;
  }

  // Check temporary image
  ierr = YaIPS_RGB_to_ImgD( pTmp, &iTmp);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get image data of source

  xx    = iSrc.xx;
  yy    = iSrc.yy;

  nBytesSrc  = iSrc.d;

  nColorsSrc = iSrc.d >= 3 ? 3 : 1;

  // Build contrast lookup table

  if( EdgeHighlight <   0) EdgeHighlight =   0;
  if( EdgeHighlight > 100) EdgeHighlight = 100;

  if( EdgeStrength <   0) EdgeStrength =   0;
  if( EdgeStrength > 100) EdgeStrength = 100;

  if( EdgeHighlight == 0 && EdgeStrength == 0) {     // Default values

    // Build 1:1 LUT
    for( int i = 0; i < YAIPS_LUT_N_POINTS; i++) {

      LutContrast[ i] = i;
    }

  } else {

    // Build contrast LUT

    int Contrast1in, Contrast1out, Contrast2in, Contrast2out;

    Contrast1in  = EdgeHighlight * 2;
    Contrast1out = 0;

    Contrast2in  = 255 - ((255 - Contrast1in) * EdgeStrength + 50) / 100;
    Contrast2out = 255;

    for( int i = 0; i < YAIPS_LUT_N_POINTS; i++) {

      if( i <= Contrast1in) {

        a = Contrast1out;

      } else if( i >= Contrast2in) {

        a = Contrast2out;

      } else {

        a = Contrast1out + ( Contrast2out - Contrast1out) * (i - Contrast1in) / (Contrast2in - Contrast1in);
      }

      if (a <   0) a = 0;
      if (a > 255) a = 255;
      LutContrast[ i ] = (uchar)a;
    }
  }

  // Color of edge

  Fl::get_color( EdgeCol, r, g, b);             // Convert color to RGB values

  if( nColorsSrc == 1) {        // BW image

    r = (r * 76 + g * 150 + b * 30) >> 8;      // A fast black white conversion.
  }

  // Merge edges into posterized image

  for( y = 0; y < yy; y++) {

    s8 = RGB_pixad( 0, y, &iTmp);
    d8 = RGB_pixad( 0, y, &iDst);

    if( nColorsSrc == 1) {        // BW image

      for( x = 0; x < xx; x++) {

        a = s8[ 0];

        if( a > 3) {             // Minimum edge strength

          a = LutContrast[ a];

          d8[ 0] = (r * a + d8[ 0] * (255 - a)) / 255;
        }

        s8 += nBytesSrc;
        d8 += nBytesSrc;
      }

    } else {                      // RGB image

      for( x = 0; x < xx; x++) {

        a = s8[ 0];
        if( s8[ 1] > a) a = s8[ 1];
        if( s8[ 2] > a) a = s8[ 2];

        if( a > 3) {             // Minimum edge strength

          a = LutContrast[ a];

          d8[ 0] = (r * a + d8[ 0] * (255 - a)) / 255;
          d8[ 1] = (g * a + d8[ 1] * (255 - a)) / 255;
          d8[ 2] = (b * a + d8[ 2] * (255 - a)) / 255;
        }

        s8 += nBytesSrc;
        d8 += nBytesSrc;
      }
    }
  }


  ierr = 0;   // OK

ErrorExit:

  if( pTmp != NULL) {               // Release temporary image

    pTmp->release();
  }

  return( ierr);
}

/******************************** End Of File ********************************/

