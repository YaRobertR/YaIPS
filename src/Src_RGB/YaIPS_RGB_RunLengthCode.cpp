/****************************************************************************

  YaIPS_RGB_RunLengthCode.cpp

  Fl_RGB_Image image processing.
  Run length code interface

 15.05.2025 RR: First edition of this file.

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

#include "YaIPS.h"

/***************************************************************************
* Last histogram and threshold results leftover by YaIPS_RGB_RLC_Code()
* with a bimodal threshold mode.
****************************************************************************
*/
Fl_YaIPS_Histo_RGB_t YaIPS_RGB_RLC_BM_Histo;
int YaIPS_RGB_RLC_BM_Thres, YaIPS_RGB_RLC_BM_Max1, YaIPS_RGB_RLC_BM_Max2;

/***************************************************************************
* YaIPS_RGB_RLC_Code
* Binaries an image to run length codes.
*
* ppRLC1       Out: Pointer to pointer to image for run length codes
*                   NOTE: Pointer to image must be set to NULL before
*                         calling this function the first time.
*                         Release of image must be handled elsewhere.
* pSrc         In: Source image
* ColorSpace   In: Color space for color images BW, R, G or B
* BinThres1    In: Binarization 1. threshold
* BinThres2    In: Binarization 2. threshold
* BinMode      In: Binarization mode. See also YAIPS_RLC_BINMODE_xxx defines.
*                  0: code if >= BinThres1
*                  1: code if <  BinThres1
*                  2: code if >= BinThres1 && <  BinThres2
*                  3: code if <  BinThres1 || >= BinThres2
*                  4: code if chroma difference < BinThres1, BinThres2 holds RGB reference value
*                  5: code if chroma difference >= BinThres1, BinThres2 holds RGB reference value
*                  6: code if >= bimodal threshold (automatic computation)
*                  7: code if <  bimodal threshold (automatic computation)
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

#define YAIPS_IHS_THRES_I      20   // Threshold for intensity
#define YAIPS_IHS_THRES_S      20   // Threshold for intensity

int YaIPS_RGB_RLC_Code( Timages **ppRLC1,        // Out: Point to pointer to image for run length codes
                        Fl_RGB_Image *pSrc,      // In: Source image
                        int ColorSpace,          // In: Color space for color images BW, R, G or B
                        int BinThres1,           // In: Binarization 1. threshold
                        int BinThres2,           // In: Binarization 2. threshold
                        int BinMode,             // In: Binarization mode
                        int *pOutUsedXX,         // Out: used source image width. Depends from AOI width
                        int AOI_X, int AOI_Y,    // Optional in: AOI left upper corner
                        int AOI_XX, int AOI_YY)  // Optional in: AOI size
{
  int ierr, rl2flags_use, xx, yy,ImgXX, ImgYY, RLCxx;
  YaIPS_RGB_ImgD_t iSrc;
  Timages *iRLC1;

  // Check pointer to run length coded

  if( ppRLC1 == NULL) {                           // Check pointer to run length codes

    ierr = -100;
    goto exitPoint;
  }

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Check AOI

  ImgXX = iSrc.xx;
  ImgYY = iSrc.yy;

  // Clip size x
  if( AOI_XX <= 0) {                       // Size of image
    AOI_XX = ImgXX;                        // --> Use full size
  }

  if( AOI_XX > ImgXX){
    AOI_XX = ImgXX;
  }

  // Clip position x
  if( AOI_XX == 0 || AOI_XX >= ImgXX) {   // is full size

    AOI_X = 0;                             // Always set to 0

  } else {

    if( AOI_X < 0) {

      AOI_X = 0;
    }

    if( AOI_X > ImgXX - AOI_XX) {

      AOI_X = ImgXX - AOI_XX;
    }
  }

  // Clip size y
  if( AOI_YY <= 0) {                       // Size of image
    AOI_YY = ImgYY;                        // --> Use full size
  }

  if( AOI_YY > ImgYY){
    AOI_YY = ImgYY;
  }

  // Clip position y
  if( AOI_YY == 0 || AOI_YY >= ImgYY) {   // is full size

    AOI_Y = 0;                             // Always set to 0

  } else {

    if( AOI_Y < 0) {

      AOI_Y = 0;
    }

    if( AOI_Y > ImgYY - AOI_YY) {

      AOI_Y = ImgYY - AOI_YY;
    }
  }

  if( AOI_XX != ImgXX || AOI_YY != ImgYY) {   // Use an AOI

    // Set AOI

    // Define an image AOI on an existing image
    // NOTE: overwrite source image descriptor
    YaIPS_ImgD_AOI( &iSrc, &iSrc, AOI_X, AOI_Y, AOI_XX, AOI_YY);
  }

  xx = iSrc.xx;
  yy = iSrc.yy;

  if( pOutUsedXX != NULL ) {            // Return used image width

    *pOutUsedXX = xx;
  }

  // Have to compute bimodal threshold

  if( BinMode == YAIPS_RLC_BINMODE_GE_BM_AUTO ||     // Any of the bimodal threshold modes
      BinMode == YAIPS_RLC_BINMODE_L_BM_AUTO) {

    Tvector TempHistVec;
    int TempThres;

    // Change to pure threshold mode

    BinMode = BinMode - YAIPS_RLC_BINMODE_GE_BM_AUTO + YAIPS_RLC_BINMODE_GE_T1;

    TempThres = 128;        // Default threshold

    // Calculate the histogramm

    YaIPS_Histo_Measure2( pSrc,                              // Pointer to image envelop
                          &YaIPS_RGB_RLC_BM_Histo,           // Pointer to RGB histogram
                          ColorSpace,                        // In: Color space for color images NORMAL, BW, R, G or B
                          4,                                 // In: If > 1 speed up measurement by using less pixels
                          AOI_X, AOI_Y,                      // AOI rectangle start point
                          AOI_XX, AOI_YY);                   // AOI rectangle size

    if( YaIPS_RGB_RLC_BM_Histo.nHistos == 1) {       // Expect only one histogram here

      // Double the first histogram. This will be later lowpass filtered by bithres2().
      // The two histograms may be displayed later.
      memcpy( &YaIPS_RGB_RLC_BM_Histo.G, &YaIPS_RGB_RLC_BM_Histo.R, sizeof( YaIPS_RGB_RLC_BM_Histo.R));

      YaIPS_RGB_RLC_BM_Histo.nHistos = 2;

      // The second histogram is lowpass filtered by bithres2()
      ve_Mem2Vec( &TempHistVec, DV_HOST, (anypnt)YaIPS_RGB_RLC_BM_Histo.G.HistoTable, YAIPS_HISTO_N_POINTS, sizeof(int32), TY_INT32);

      ierr = bithres2( &TempHistVec, BinThres1, 30,
                       &YaIPS_RGB_RLC_BM_Thres, &YaIPS_RGB_RLC_BM_Max1, &YaIPS_RGB_RLC_BM_Max2);

      if( ierr == 0) {     // Was OK

        TempThres = YaIPS_RGB_RLC_BM_Thres;
      }
    } else {

      // Invalidate histogram
      YaIPS_RGB_RLC_BM_Histo.nHistos = 0;
    }

    // Set threshold used by pure threshold mode

    BinThres1 = TempThres;
  }

  // Runlength code the image

  rl2flags_use = RL2ENTERLABELS;          //  | RL2AREA_SUPBPIX
  rl2flags_use |= RL2NO_OBJ_REALLOC;      // rl2t_labmea() makes no object vector realloc, vector is pre-allocated

  rl2t_setup2( RL2CONNECTIVITY_8, rl2flags_use);

  // Allocate temporary images

  iRLC1 = *ppRLC1;                         // Get pointer to image
  *ppRLC1 = NULL;                          // Invalidate this image

  RLCxx = ((xx + 1) / 2) * sizeof(Trl2desc);  // Size for RLC image

  if( iRLC1 != NULL &&                     // Have pointer to an image
      ( getxm( iRLC1) != RLCxx ||          // and size mismatch
          getym( iRLC1) != yy)) {

    im_remove( iRLC1);                     // Remove the image

    iRLC1 = NULL;
  }

  if( iRLC1 == NULL) {                     // Have no image

    iRLC1 = im_ucreateMem( NULL, TY_INT8, DV_HOST, RLCxx, yy, NULL);

    if( iRLC1 == IMNULL) {

#ifdef use_again
      sprintf( errbuffer, "internal error: error %d creating image", ierr);
      errstring = errbuffer;
#else
      errstring = ERR_IPS_IMG_CREATE;
#endif

      ierr = -100;
      goto exitPoint;
    }
  }

  *ppRLC1 = iRLC1;                         // Set pointer to image

  //
  // Runlength code the image
  // This is modeled after rl2t_code() in YaIPS_IPS_RL2Tools.cpp
  //

//x/#define USE_CHROMA_KEY 1   // Define this to use the chroma key

  int   x, y, maxx, free, xa, Src_d, Off_d, PixVal;
  int r, g, b, r_key, g_key, b_key;
#ifdef USE_CHROMA_KEY
  int cb, cr, cb_key, cr_key;
#else
  int ci, ch, cs, i_key, h_key, s_key, dh;
#endif
  uchar *pr;                 /* Read pointer in src    */
  Trl2desc *pRLC;            /* Write pointer in dst   */

  if( getyy( iRLC1) != iSrc.yy) {          // Images must have the same height

    errstring = ERR_IPS_YSIZE_SRC_DST_DIFF;
    goto exitPoint;
  }

  if( (getyp( iRLC1) != TY_BYTE) ||        // Check bytes per pixel
      iSrc.d < 1) {

    errstring = ERR_IPS_DATA_COMP_NO_SUPP;
    return(-1);
  }

  yy = iSrc.yy;
  maxx = iSrc.xx;
  Src_d = iSrc.d;

  // Check color space

  Off_d = 0;                       // NO byte offset for color images

  if (Src_d >= 3) {                // Have a color image

    if( BinMode == YAIPS_RLC_BINMODE_L_CHROMA ||     // Any of the chroma binarization modes
        BinMode == YAIPS_RLC_BINMODE_GE_CHROMA) {

      ColorSpace = YAIPS_DISP_COLMOD_BW;  // Binarization of color image

      r_key = BinThres2 & 0xff;
      g_key = (BinThres2 >> 8) & 0xff;
      b_key = (BinThres2 >> 16) & 0xff;

#ifdef USE_CHROMA_KEY
      cb_key = 128 + ((28 * b_key - 43 * r_key - 85 * g_key + 128) >> 8);    // cb = (int) round(128 + -0.168736*r - 0.331264*g + 0.5*b);
      cr_key = 128 + ((128 * r_key - 107 * g_key - 21 * b_key + 128) >> 8);  // cr = (int) round(128 + 0.5*r - 0.418688*g - 0.081312*b);

      BinThres1 = BinThres1 * BinThres1;    // Square this so we need no square root later
#else
      YaIPS_RGB_Color_RGB2IHS( &i_key, &h_key, &s_key, r_key, g_key, b_key);

      if( BinThres1 > 127) {    // Clip to this value

        BinThres1 = 127;
      }
#endif

    } else {                       // any other binarization mode

      switch( ColorSpace) {

      case YAIPS_DISP_COLMOD_NORMAL:     // Normal RGB image
      case YAIPS_DISP_COLMOD_BW:         // BW image
      default:

        ColorSpace = YAIPS_DISP_COLMOD_BW;  // Convert color image to BW
        break;

      case YAIPS_DISP_COLMOD_R:          // Red component

        Off_d = 0;                       // Byte offset for red color component
        break;

      case YAIPS_DISP_COLMOD_G:          // Green component

        Off_d = 1;                       // Byte offset for red green component
        break;

      case YAIPS_DISP_COLMOD_B:          // Blue component

        Off_d = 2;                       // Byte offset for red blue component
        break;
      } // end switch
    }

  } else {                                       // Have a BW image

    ColorSpace = YAIPS_DISP_COLMOD_NORMAL;       // No color modification
  }

  // ...

  for(y = 0; y < yy; y++ ) {

    pr = RGB_pixad( 0,  y, &iSrc);
    pr += Off_d;                                 // Add byte offset for color component

    pRLC = (Trl2desc *)pixad( 0, y, iRLC1);
    free = getxx( iRLC1);                        /* Bytes free in Dest */
    x = 0;
    for(;;) {

      if( ColorSpace == YAIPS_DISP_COLMOD_BW) {  // Binarization of color image

        switch( BinMode) {
        case YAIPS_RLC_BINMODE_L_CHROMA:
          for( ; ; ) {
            r = pr[0];
            g = pr[1];
            b = pr[2];

#ifdef USE_CHROMA_KEY
            cb = 128 + ((28 * b - 43 * r - 85 * g + 128) >> 8);    // cb = (int) round(128 + -0.168736*r - 0.331264*g + 0.5*b);
            cr = 128 + ((128 * r - 107 * g - 21 * b + 128) >> 8);  // cr = (int) round(128 + 0.5*r - 0.418688*g - 0.081312*b);

            r = cb_key - cb;          // Difference of chroma values
            b = cr_key - cr;

            PixVal = r * r + b * b;   // Sum of squares

            if(x < maxx && PixVal >= BinThres1) ; else break;
#else
            if(x >= maxx) break;   // End of line

            YaIPS_RGB_Color_RGB2IHS( &ci, &ch, &cs, r, g, b);

            // difference in angle space
            if( ch >= h_key) {
              dh = ch - h_key;
            } else {
              dh = h_key - ch;
            }
            if( dh > 128) {   // circle wrap around
              dh = 256 - dh;
            }

            PixVal = ci >= YAIPS_IHS_THRES_I && cs >= YAIPS_IHS_THRES_S && dh < BinThres1;

            if( ! PixVal) {
              // is OK
            } else {
              break;
            }
#endif

            pr += Src_d; x++;
          }
          break;
        case YAIPS_RLC_BINMODE_GE_CHROMA:
          for( ; ; ) {
            r = pr[0];
            g = pr[1];
            b = pr[2];

#ifdef USE_CHROMA_KEY
            cb = 128 + ((28 * b - 43 * r - 85 * g + 128) >> 8);    // cb = (int) round(128 + -0.168736*r - 0.331264*g + 0.5*b);
            cr = 128 + ((128 * r - 107 * g - 21 * b + 128) >> 8);  // cr = (int) round(128 + 0.5*r - 0.418688*g - 0.081312*b);

            r = cb_key - cb;          // Difference of chroma values
            b = cr_key - cr;

            PixVal = r * r + b * b;   // Sum of squares

            if(x < maxx && PixVal < BinThres1) ; else break;
#else
            if(x >= maxx) break;   // End of line

            YaIPS_RGB_Color_RGB2IHS( &ci, &ch, &cs, r, g, b);

            // difference in angle space
            if( ch >= h_key) {
              dh = ch - h_key;
            } else {
              dh = h_key - ch;
            }
            if( dh > 128) {   // circle wrap around
              dh = 256 - dh;
            }

            PixVal = ci >= YAIPS_IHS_THRES_I && cs >= YAIPS_IHS_THRES_S && dh < BinThres1;

            if( PixVal) {
              // is OK
            } else {
              break;
            }
#endif

            pr += Src_d; x++;
          }
          break;
        case YAIPS_RLC_BINMODE_L_T1_OR_GE_T2:
          for( ; ; ) {
            PixVal = (pr[0] * 76 + pr[1] * 150 + pr[2] * 30) >> 8;
            if( x < maxx && PixVal >= BinThres1 && PixVal < BinThres2) {
              // is OK
            } else {
              break;
            }
            pr += Src_d; x++;
          }
          break;
        case YAIPS_RLC_BINMODE_GE_T1_AND_L_T2:
          for( ; ; ) {
            PixVal = (pr[0] * 76 + pr[1] * 150 + pr[2] * 30) >> 8;
            if(x < maxx && (PixVal < BinThres1 || PixVal >= BinThres2)) {
              // is OK
            } else {
              break;
            }
            pr += Src_d; x++;
          }
          break;
        case YAIPS_RLC_BINMODE_L_T1:
          for( ; ; ) {
            PixVal = (pr[0] * 76 + pr[1] * 150 + pr[2] * 30) >> 8;
            if(x < maxx && PixVal >= BinThres1) {
              // is OK
            } else {
              break;
            }
            pr += Src_d; x++;
          }
          break;
        default:
          for( ; ; ) {
            PixVal = (pr[0] * 76 + pr[1] * 150 + pr[2] * 30) >> 8;
            if(x < maxx && PixVal < BinThres1) {
              // is OK
            } else {
              break;
            }
            pr += Src_d; x++;
          }
          break;
        }

        if(x >= maxx) break;              /* Nothing found at end of line */
        xa = x;                           /* remember start of object     */

        switch( BinMode) {
        case YAIPS_RLC_BINMODE_L_CHROMA:
          for( ; ; ) {
            r = pr[0];
            g = pr[1];
            b = pr[2];

#ifdef USE_CHROMA_KEY
            cb = 128 + ((28 * b - 43 * r - 85 * g + 128) >> 8);    // cb = (int) round(128 + -0.168736*r - 0.331264*g + 0.5*b);
            cr = 128 + ((128 * r - 107 * g - 21 * b + 128) >> 8);  // cr = (int) round(128 + 0.5*r - 0.418688*g - 0.081312*b);

            r = cb_key - cb;          // Difference of chroma values
            b = cr_key - cr;

            PixVal = r * r + b * b;   // Sum of squares

            if(x < maxx && PixVal < BinThres1) {
              // is OK
            } else {
              break;
            }
#else
            if(x >= maxx) break;   // End of line

            YaIPS_RGB_Color_RGB2IHS( &ci, &ch, &cs, r, g, b);

            // difference in angle space
            if( ch >= h_key) {
              dh = ch - h_key;
            } else {
              dh = h_key - ch;
            }
            if( dh > 128) {   // circle wrap around
              dh = 256 - dh;
            }

            PixVal = ci >= YAIPS_IHS_THRES_I && cs >= YAIPS_IHS_THRES_S && dh < BinThres1;

            if( PixVal) {
              // is OK
            } else {
              break;
            }
#endif

            pr += Src_d; x++;
          }
          break;
        case YAIPS_RLC_BINMODE_GE_CHROMA:
          for( ; ; ) {
            r = pr[0];
            g = pr[1];
            b = pr[2];

#ifdef USE_CHROMA_KEY
            cb = 128 + ((28 * b - 43 * r - 85 * g + 128) >> 8);    // cb = (int) round(128 + -0.168736*r - 0.331264*g + 0.5*b);
            cr = 128 + ((128 * r - 107 * g - 21 * b + 128) >> 8);  // cr = (int) round(128 + 0.5*r - 0.418688*g - 0.081312*b);

            r = cb_key - cb;          // Difference of chroma values
            b = cr_key - cr;

            PixVal = r * r + b * b;   // Sum of squares

            if(x < maxx && PixVal >= BinThres1) {
              // is OK
            } else {
              break;
            }
#else
            if(x >= maxx) break;   // End of line

            YaIPS_RGB_Color_RGB2IHS( &ci, &ch, &cs, r, g, b);

            // difference in angle space
            if( ch >= h_key) {
              dh = ch - h_key;
            } else {
              dh = h_key - ch;
            }
            if( dh > 128) {   // circle wrap around
              dh = 256 - dh;
            }

            PixVal = ci >= YAIPS_IHS_THRES_I && cs >= YAIPS_IHS_THRES_S && dh < BinThres1;

            if( ! PixVal) {
              // is OK
            } else {
              break;
            }
#endif

            pr += Src_d; x++;
          }
          break;
        case YAIPS_RLC_BINMODE_L_T1_OR_GE_T2:
          for( ; ; ) {
            PixVal = (pr[0] * 76 + pr[1] * 150 + pr[2] * 30) >> 8;
            if(x < maxx && (PixVal < BinThres1 || PixVal >= BinThres2)) {
              // is OK
            } else {
              break;
            }
            pr += Src_d; x++;
          }
          break;
        case YAIPS_RLC_BINMODE_GE_T1_AND_L_T2:
          for( ; ; ) {
            PixVal = (pr[0] * 76 + pr[1] * 150 + pr[2] * 30) >> 8;
            if(x < maxx && PixVal >= BinThres1 && PixVal < BinThres2) {
              // is OK
            } else {
              break;
            }
            pr += Src_d; x++;
          }
          break;
        case YAIPS_RLC_BINMODE_L_T1:
          for( ; ; ) {
            PixVal = (pr[0] * 76 + pr[1] * 150 + pr[2] * 30) >> 8;
            if(x < maxx && PixVal < BinThres1) {
              // is OK
            } else {
              break;
            }
            pr += Src_d; x++;
          }
          break;
        default:
          for( ; ; ) {
            PixVal = (pr[0] * 76 + pr[1] * 150 + pr[2] * 30) >> 8;
            if(x < maxx && PixVal >= BinThres1) {
              // is OK
            } else {
              break;
            }
            pr += Src_d; x++;
          }
          break;
        }

      } else {                                   // Have BW image or use one color component

        switch( BinMode) {
        case YAIPS_RLC_BINMODE_L_T1_OR_GE_T2:
          while(x < maxx && *pr >= BinThres1 && *pr < BinThres2) { pr += Src_d; x++; }
          break;
        case YAIPS_RLC_BINMODE_GE_T1_AND_L_T2:
          while(x < maxx && (*pr < BinThres1 || *pr >= BinThres2)) { pr += Src_d; x++; }
          break;
        case YAIPS_RLC_BINMODE_L_T1:
          while(x < maxx && *pr >= BinThres1) { pr += Src_d; x++; }
          break;
        default:
          while(x < maxx && *pr < BinThres1) { pr += Src_d; x++; }
          break;
        }

        if(x >= maxx) break;              /* Nothing found at end of line */
        xa = x;                           /* remember start of object     */

        switch( BinMode) {
        case YAIPS_RLC_BINMODE_L_T1_OR_GE_T2:
          while(x < maxx && (*pr < BinThres1 || *pr >= BinThres2)) { pr += Src_d; x++; }
          break;
        case YAIPS_RLC_BINMODE_GE_T1_AND_L_T2:
          while(x < maxx && *pr >= BinThres1 && *pr < BinThres2) { pr += Src_d; x++; }
          break;
        case YAIPS_RLC_BINMODE_L_T1:
          while(x < maxx && *pr < BinThres1) { pr += Src_d; x++; }
          break;
        default:
          while(x < maxx && *pr >= BinThres1) { pr += Src_d; x++; }
          break;
        }
      }

      free -= sizeof(Trl2desc);          /* - place for descriptor       */
      if(free < 0) {                    /* no memory free in line       */
        errstring = ERR_IPS_XS_DST_OVERFLOW;
        return(RL2TOOLS_MEMOVFL);
      }
      pRLC->rlxa = xa;                       /* write start Object           */
      pRLC->rlxe = x - 1;                    /* write end Object             */
      pRLC->rlab = 0;                        /* write dummy Label            */
      pRLC->rlid = 0;                        /* write dummy index            */

      // supbixel area, NO subpixel support
      pRLC->rlxaSubPix = pRLC->rlxa << RL2T_SUBPIX_SHIFT;  /* in subpixel */
      pRLC->rlxeSubPix = pRLC->rlxe << RL2T_SUBPIX_SHIFT;  /* in subpixel */
      pRLC->rlxeSubPix += RL2T_SUBPIX_FAC - 1;             /* point to end of this pixel */

      pRLC++;
    } /* end for x */

    free -= sizeof(Trl2desc);            /* - place for descriptor       */
    if(free < 0) {                      /* no memory free in line       */
      errstring = ERR_IPS_XS_DST_OVERFLOW;
      return(RL2TOOLS_MEMOVFL);
    }
    pRLC->rlxa = -1;                         /* EOL descriptor               */
    pRLC->rlxe = -1;
    pRLC->rlab = -1;
    pRLC->rlid = -1;
    pRLC->rlxaSubPix = -1;
    pRLC->rlxeSubPix = -1;
    pRLC++;
  } /* end for y */

  ierr = 0;             // Return OK

  // End work

exitPoint:

  return( ierr);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_RLC_LabelMeas
* Label run length codes and measure the objects.
*
* vObj         Out: vector with extracted objects
*                   NOTE: Must be initialized and preallocated before
*                         calling this function.
* iRLC1        In: Image with run length codes
* AreaMin      In: Minimum area of an object to be labeled
* AreaMax      In: Maximum area of an object to be labeled
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_RLC_LabelMeas( Tvector *vObj,          // In Out: vector with extracted objects
                             Timages *iRLC1,         // In: Image with run length codes
                             int AreaMin,            // In: Minimum area of an object to be labeled
                             int AreaMax)            // In: Maximum area of an object to be labeled
{
  int ierr, rl2flags_use, xx;
  Timages *iWork = IMNULL;

  xx = (getxm( iRLC1) * 2) / sizeof(Trl2desc);     // Reconstruct width of original source image

  // Runlength code the image

  rl2flags_use = RL2ENTERLABELS;          //  | RL2AREA_SUPBPIX
  rl2flags_use |= RL2NO_OBJ_REALLOC;      // rl2t_labmea() makes no object vector realloc, vector is pre-allocated

  rl2t_setup2( RL2CONNECTIVITY_8, rl2flags_use);

  iWork = im_ucreateMem( NULL, TY_INT8, DV_HOST, sizeof(Trl2obj), (xx + 1) / 2, NULL);
  if( iWork == IMNULL) {

    sprintf( errbuffer, LangStringLookup( "&RGB_Rlc_LabelMeas1=Internal error: error creating image"));
    errstring = errbuffer;

    ierr = -102;
    goto exitPoint;
  }

  ierr = rl2t_labmea( iRLC1, vObj, iWork, AreaMin, AreaMax);
  if( ierr < 0) {

    sprintf( errbuffer, LangStringLookup( "&RGB_Rlc_LabelMeas2=Error %d in rl2t_labmea()"), ierr);
    errstring = errbuffer;

    ierr = -102;
    goto exitPoint;
  }

  ierr = 0;             // Return OK

  // End work

exitPoint:

  if( iWork != IMNULL) {

    im_remove( iWork);
  }

  return( ierr);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_RLC_CodeMeas
* Binaries an image to run length codes, label and measure the objects.
*
* vObj         Out: vector with extracted objects
*                   NOTE: Must be initialized and preallocated before
*                         calling this function.
* pSrc         In: Source image
* ColorSpace   In: Color space for color images BW, R, G or B
* BinThres1    In: Binarization 1. threshold
* BinThres2    In: Binarization 2. threshold
* BinMode      In: Binarization mode:
*                  0: code if >= BinThres1
*                  1: code if < BinThres1
*                  2: code if >= BinThres1 && <  BinThres2
*                  3: code if <  BinThres1 || >= BinThres2
* AreaMin      In: Minimum area of an object to be labeled
* AreaMax      In: Maximum area of an object to be labeled
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_RLC_CodeMeas( Tvector *vObj,          // In Out: vector with extracted objects
                            Fl_RGB_Image *pSrc,     // In: Source image
                            int ColorSpace,          // In: Color space for color images BW, R, G or B
                            int BinThres1,           // In: Binarization 1. threshold
                            int BinThres2,           // In: Binarization 2. threshold
                            int BinMode,            // In: Binarization mode
                            int AreaMin,            // In: Minimum area of an object to be labeled
                            int AreaMax,            // In: Maximum area of an object to be labeled
                            int AOI_X, int AOI_Y,   // Optional in: AOI left upper corner
                            int AOI_XX, int AOI_YY) // Optional in: AOI size
{
  int ierr;
  Timages *iRLC1 = IMNULL;
  Timages *iWork = IMNULL;

  // Binaries an image to run length codes
  ierr = YaIPS_RGB_RLC_Code( &iRLC1, pSrc, ColorSpace, BinThres1,BinThres2, BinMode,
                             NULL, AOI_X, AOI_Y, AOI_XX, AOI_YY);
  if( ierr != 0) {

    goto exitPoint;
  }

  ierr = YaIPS_RGB_RLC_LabelMeas( vObj, iRLC1, AreaMin, AreaMax);

  if( ierr != 0) {

    goto exitPoint;
  }

  ierr = 0;             // Return OK

  // End work

exitPoint:

  if( iRLC1 != IMNULL) {

    im_remove( iRLC1);
  }

  if( iWork != IMNULL) {

    im_remove( iWork);
  }

  return( ierr);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_RLC_Decode
* Converts a run length coded labeled image to a grey level image.
*
* ppDst        Pointer to pointer to RGB image
*              NOTE: Point to to RGB image must be initialized to NULL;
*                    Image is allocated if not existing and reallocated
*                    if size is different than needed.
* iRLC1        In: Image with run length codes
* label        In: If label >  0 only the named object with "label" is extracted.
*                  Decoded objects will be drawn in color "lcol".
*                  If label <= 0 all labels are decoded.
*                  Decoded objects will be drawn in a color equal to their label id.
* bcol         In: background color. Must be a 8 bit value.
* lcol         In: label color. Must be a 8 bit value.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_RLC_Decode( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                          Timages *iRLC1,       // In: Image with run length codes
                          int label,            // In: Minimum area of an object to be labeled
                          int bcol,             // In: background color. Must be a 8 bit value.
                          int lcol)             // In: label color. Must be a 8 bit value.
{
  int ierr, xx, yy;
  Timages iDst;

  xx = (getxm( iRLC1) * 2) / sizeof(Trl2desc);     // Reconstruct width of original source image
  yy = getym( iRLC1);                              // Reconstruct height of original source image

  // Ensure that pPDst image has the same size as the original source image. Bytes per pixel will be 1.

  ierr = YaIPS_RGB_ImageSetSize( ppDst, xx, yy, 1);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_to_IPS( &iDst,*ppDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Decode runlength codes

  ierr = rl2t_decode( iRLC1, &iDst, label, bcol, lcol);
  if( ierr < 0) {

    sprintf( errbuffer, LangStringLookup( "&RGB_Rlc_Decode1=Error %d in rl2t_decode()"), ierr);
    errstring = errbuffer;

    ierr = -102;
    goto exitPoint;
  }

  ierr = 0;             // Return OK

  // End work

exitPoint:

  return( ierr);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_RLC_GetBright
*
* Measure brightness of labeled objects.
*
* The measured brightness is placed in the 'ExData1' data element of the
* object data.
* Black/white images get one value and color images 3 values (r is low byte,
* g and b are the next bytes).
* A black/white image has the forth byte set to 0, a color image has the
* forth byte set to 1.
*
* vObj         In Out: vector with extracted objects from previous label run
* iRLC1        In: Image with run length codes
* pSrc         In: Source image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

// Measure brightness of labeled objects.
int YaIPS_RGB_RLC_GetBright( Tvector *vObj,           // In Out: vector with extracted objects
                             Timages *iRLC1,          // In: Point to image with run length codes
                             Fl_RGB_Image *pSrc)      // In: Source image
{
  int ierr, iObj, nObj, ImgYY, SumSpanR, SumSpanG, SumSpanB, SumArea;
  int xmin, xmax, rx, ry, rxx, Src_d, Off_d, IsColorImage;
  long long SumObjR, SumObjG, SumObjB;     // 64 bit counts
  Trl2objdst *pObjBase, *pObj;
  YaIPS_RGB_ImgD_t iSrc;
  int16 label;
  register Trl2desc *ps;
  uchar *pr;                 /* Read pointer in src    */

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pObjBase = (Trl2objdst *)vgetpm( vObj);

  nObj = (int)(vgetnm( vObj) / (sizeof(Trl2objdst) / sizeof(int32)));
  pObj = pObjBase;

  ImgYY = iSrc.yy;

  if( ImgYY != getyy( iRLC1)) {
    errstring = LangStringLookup( "&RGB_Rlc_GetBright1=Height of src and RLCs different");
    return(-3);
  }

  // Prepare things for source image

  Src_d = iSrc.d;

  // Check color space

  Off_d = 0;                       // NO byte offset for color images

  IsColorImage = Src_d >= 3;       // Have a color image

  // Loop over all objects

  rxx = getxx( iRLC1);

  for( iObj = 0; iObj < nObj; iObj++, pObj++) {

    // Loop over all RLCs of this object

    label = pObj->label;               // Get label

    SumObjR = SumObjG = SumObjB = 0;   // Reset RGB sums for object
    SumArea = 0;

    for( ry = pObj->ymin; ry <= pObj->ymax; ry++ ) {

      ps = (Trl2desc *)pixad( 0, ry, iRLC1);

      for( rx = 0; rx < rxx; rx += sizeof(Trl2desc)/sizeof(int16), ps++) {

        if (ps->rlab < 0) {                      // end of row for this line

          break;                                 // take next row
        }

        if( ps->rlab == label) {                 // Measure for this object

          xmin = pObj->xmin;
          xmax = pObj->xmax;

          SumArea += xmax - xmin + 1;            // Sum up number of pixels

          pr = RGB_pixad( xmin,  ry, &iSrc);
          pr += Off_d;                           // Add byte offset for color component

          SumSpanR = SumSpanG = SumSpanB = 0;    // Reset RGB sums for RLC span

          if( IsColorImage) {                    // Binarization of color image

            for( ; xmin <= xmax; xmin++) {

              SumSpanR += pr[ 0];
              SumSpanG += pr[ 1];
              SumSpanB += pr[ 2];
              pr += Src_d;
            }

            SumObjR += SumSpanR;
            SumObjG += SumSpanG;
            SumObjB += SumSpanB;

          } else {                                 // Have BW image or use one color component

            for( ; xmin <= xmax; xmin++) {

              SumSpanR += *pr;
              pr += Src_d;
            }

            SumObjR += SumSpanR;
          }
        }
      }
    }

    // Store RGB values for object

    if( SumArea <= 0) {   // No pixel

      SumSpanR = 0;
      SumSpanG = 0;
      SumSpanB = 0;

    } else {              // Average gray values

      SumSpanR = (SumObjR + (SumArea >> 1)) / SumArea;
      SumSpanG = (SumObjG + (SumArea >> 1)) / SumArea;
      SumSpanB = (SumObjB + (SumArea >> 1)) / SumArea;

      if( SumSpanR > 255) SumSpanR = 255;
      if( SumSpanG > 255) SumSpanG = 255;
      if( SumSpanB > 255) SumSpanB = 255;
    }

    pObj->ExData1 = (SumSpanR) | (SumSpanG << 8) | (SumSpanB << 16);

    if( IsColorImage) {                        // Was binarization of color image

      pObj->ExData1 |= 0x01000000;             // Flag color in fourth byte
    }
  }

  ierr = 0;       // Return OK

  return( ierr);
}

/******************************** End Of File ********************************/

