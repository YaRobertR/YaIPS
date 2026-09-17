/****************************************************************************

  YaIPS_RGB_Canny.cpp

  Fl_RGB_Image image processing.
  Canny edge filter

 23.04.2025 RR: First edition of this file.

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

#define YAIPS_RESULT_SCHIFT   10            // Use this shift factor to use integer multiply

/*--------------- Define constants for high precision ---------------------*/
#define PREC 16                         /* Precision 2^16 = 65536          */
#define FPREC (float)65536.0            /* Precision for float variables   */
#define ROUND (int32)32768              /* 0.5 * 2^PREC         */
#define VALMAX 255                      /* maximal grey value in images     */

/*---------- Define constants for approximation of sqrt(v^2+h^2)  ----------*/
#define C1     (int)27146       /* C1 = (sqrt(2)-1) * 2^PREC          */
#define C2     (int)38390       /* C2 = (2-sqrt(2)) * 2^PREC          */
#define RPREC  (int)8           /* Precision 2^8 = 256             */
#define CROUND (int)8388608     /* 0.5 * (2^PREC + 2^RPREC)           */
#define CRANGE (int)181         /* correction factor for fullrange:   */
                                /* VALMAX/sqrt(255^2+255^2) * 2^RPREC */
                                /* here:         255/360.62 * 256     */

/*--------- Define angle constants for direction of the gradient -----------*/
#define A0         (int)1                 /*   0 degrees */
#define A90        (int)64                /*  90 degrees */
#define A180       (int)128               /* 180 degrees */
#define A270       (int)192               /* 270 degrees */
#define A360       (int)256       /*   0 degrees */
#define NOEDGE     (int)0

/*----- Define look-up table for evaluation of the gradient direction ------*/
static uchar atantab[372] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                             0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                             0,  0,  0,  0,  0,  0,  0,  0, 32, 33, 33, 34,
                            34, 35, 35, 36, 37, 37, 37, 38, 38, 39, 39, 40,
                            40, 40, 41, 41, 42, 42, 42, 43, 43, 43, 43, 44,
                            44, 44, 45, 45, 45, 45, 46, 46, 46, 46, 47, 47,
                            47, 47, 47, 48, 48, 48, 48, 48, 48, 49, 49, 49,
                            49, 49, 49, 50, 50, 50, 50, 50, 50, 50, 51, 51,
                            51, 51, 51, 51, 51, 51, 52, 52, 52, 52, 52, 52,
                            52, 52, 52, 53, 53, 53, 53, 53, 53, 53, 53, 53,
                            53, 53, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
                            54, 54, 54, 55, 55, 55, 55, 55, 55, 55, 55, 55,
                            55, 55, 55, 55, 55, 55, 55, 55, 56, 56, 56, 56,
                            56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
                            56, 56, 56, 56, 57, 57, 57, 57, 57, 57, 57, 57,
                            57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57, 57,
                            57, 57, 57, 57, 57, 57, 57, 58, 58, 58, 58, 58,
                            58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
                            58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58, 58,
                            58, 58, 58, 58, 58, 58, 58, 58, 59, 59, 59, 59,
                            59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59,
                            59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59,
                            59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59,
                            59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59,
                            59, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                            60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                            60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                            60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                            60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                            60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                            60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60 };


/*---- Define look-up table for interpolation of the gradient magnitude ----*/
static uchar ipoltab[256]= {  0,  0,  3,  5,  6,  8,  9, 11, 13, 14, 16, 18,
                            19, 21, 23, 25, 27, 28, 30, 32, 34, 36, 38, 41,
                            43, 45, 47, 50, 53, 55, 58, 61,  0,  2,  3,  5,
                             6,  8,  9, 11, 13, 14, 16, 18, 19, 21, 23, 25,
                            27, 28, 30, 32, 34, 36, 38, 41, 43, 45, 47, 50,
                            53, 55, 58, 61,  0,  2,  3,  5,  6,  8,  9, 11,
                            13, 14, 16, 18, 19, 21, 23, 25, 27, 28, 30, 32,
                            34, 36, 38, 41, 43, 45, 47, 50, 53, 55, 58, 61,
                             0,  2,  3,  5,  6,  8,  9, 11, 13, 14, 16, 18,
                            19, 21, 23, 25, 27, 28, 30, 32, 34, 36, 38, 41,
                            43, 45, 47, 50, 53, 55, 58, 61,  0,  2,  3,  5,
                             6,  8,  9, 11, 13, 14, 16, 18, 19, 21, 23, 25,
                            27, 28, 30, 32, 34, 36, 38, 41, 43, 45, 47, 50,
                            53, 55, 58, 61,  0,  2,  3,  5,  6,  8,  9, 11,
                            13, 14, 16, 18, 19, 21, 23, 25, 27, 28, 30, 32,
                            34, 36, 38, 41, 43, 45, 47, 50, 53, 55, 58, 61,
                             0,  2,  3,  5,  6,  8,  9, 11, 13, 14, 16, 18,
                            19, 21, 23, 25, 27, 28, 30, 32, 34, 36, 38, 41,
                            43, 45, 47, 50, 53, 55, 58, 61,  0,  2,  3,  5,
                             6,  8,  9, 11, 13, 14, 16, 18, 19, 21, 23, 25,
                            27, 28, 30, 32, 34, 36, 38, 41, 43, 45, 47, 50,
                            53, 55, 58, 61 };


/*---------- Macro definition for int8 output data clipping ----------------*/
/*           (necessary, when exp != 0)             */

//x/#define clip32to8( val32, d8) { if((val32) > 255) *(d8) = 255; else if((val32) < 0) *(d8) = 0;  else *(d8) = (val32);}

#define clip32to8( val32, d8)  { if((val32) > 255) *(d8) = 255;  \
          else if((val32) < 0L) *(d8) = 0L; \
          else *(d8) = (val32);}

/***************************************************************************
* gradient()
*
* NOTE: Have both or only one of the output images.
*       All valid images have the same size.
*
****************************************************************************
*/
static void gradient( YaIPS_RGB_ImgD_t *pSrc,
                      YaIPS_RGB_ImgD_t *pDst1,
                      YaIPS_RGB_ImgD_t *pDst2,
                      int modus,
                      float ResMultArg)     // Result multiplier
{
  uchar *sh1, *sh2, *sv1, *sv2;    /* pointer to operator (srcim)  */
  uchar *dvalue, *ddir;            /* pointer to destination image */
  uchar *src_start, *dst1_start, *dst2_start;
  int sjump, d1jump, d2jump;       /* store row length of images   */
  int h, v, ah, av;                /* horiz and vertic differences */
  int index, dir;                  /* table index and destination  */
  long long value;
  int preci;                       /* precission                   */
  int x, y;                        /* counter variables            */
  int xmax, ymax;                  /* dimensions of image          */
  int iByte, nByte;
#ifdef YAIPS_RESULT_SCHIFT // Use integer multiply
  long ResMultArgLong, ResMultArgLong2;
#endif

#ifdef use_again
  preci = (int)(PREC+RPREC-exp);   /* precision: 2^preci          */
#else
  // 24.04.2025 RR: Until know it better used without exponent or multiplier
  preci = (int)(PREC+RPREC);       /* precision: 2^preci          */
#endif
  xmax = pSrc->xx - 2;             /* set image dimensions         */
  ymax = pSrc->yy - 2;

  nByte = pSrc->d;

#ifdef YAIPS_RESULT_SCHIFT // Use integer multiply
  // Convert to 64 bit integer
  ResMultArgLong = (long)(ResMultArg * (1 << YAIPS_RESULT_SCHIFT));
  ResMultArgLong2 = ResMultArgLong / 2;
#endif

  /*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
  /*<<<<<<<<<<<<<<<<<<<<  Modus: 1   only dstim  >>>>>>>>>>>>>>>>>>>>>>>>>>>*/
  /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

  if (modus == 1) {

    sjump = pSrc->ld;                         /* initialize pointer       */
    d1jump = pDst1->ld;

    /*------------------------- start filter loop --------------------------*/

    /*---------------------------- row by row ------------------------------*/
    for( iByte = 0; iByte < nByte; iByte++) {

      src_start = RGB_pixad( 0, 2, pSrc);
      dst1_start = RGB_pixad( 1, 1, pDst1);

      for(y = 1; y < ymax; y++) {

        /*------------- Initialize pixel array of filter window --------------*/
        sh1 = src_start + iByte;
        sh2 = sh1 + 2 * nByte;
        sv1 = sh1 + nByte - sjump;
        sv2 = sh1 + nByte + sjump;

        dvalue = dst1_start + iByte;    /* set new pointer in magnitude image */

        /*-------------------- Process row -----------------------------------*/
        x = xmax;
        while( --x >= 0 ) {

          v = RGB_btoi(*sv2) - RGB_btoi(*sv1);  /* get vertical gradient */
          h = RGB_btoi(*sh2) - RGB_btoi(*sh1);  /*get horizontal gradient*/

          sv1 += nByte;
          sv2 += nByte;
          sh1 += nByte;
          sh2 += nByte;

          /*------------------------------------------------------------------*/
          /*-------------- Approximation of sqrt(v^2+h^2)  -------------------*/
          /*------------------------------------------------------------------*/
          /*    gain = pow(2.0,(double)exp);            */
          /*    *dvalue++ = (int8)(sqrt(v * v + h * h) * 0.70711 + 0.5 );     */
          /*------------------------------------------------------------------*/
          if (v == 0 && h == 0) {     /* no edge found            */
            *dvalue = 0;              /* set zero to dstim      */
            dvalue += nByte;
          } else {
            av = RGB_abs(v);         /* absolut ver. gradient    */
            ah = RGB_abs(h);         /* absolut hor. gradient    */
            value = (C1 * (av + ah) + C2 * RGB_max(av,ah));
#ifdef YAIPS_RESULT_SCHIFT // Use integer multiply
            if( value >= 0) {
              value = (value * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT;
            } else {
              value = (value * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT;
            }
#endif
            value = ((value * CRANGE + CROUND) >> preci);
            clip32to8( value, dvalue); /* clipping to [0,VALMAX]   */
            dvalue += nByte;
          }         /* and set value to dstim   */
        }           /* terminate x-loop         */
        src_start += sjump;                   /* set pointers to new row  */
        dst1_start += d1jump;
      }             /* terminate y-loop         */

      /*----------------------------------------------------------------------*/
      /*---------------------- Process last row ------------------------------*/
      /*----------------------------------------------------------------------*/
      sh1 = src_start + iByte;                /* initialize pixel array   */
      sh2 = sh1 + 2 * nByte;
      sv1 = sh1 + nByte - sjump;
      sv2 = sh1 + nByte;

      dvalue = dst1_start + iByte;            /* set pointer in magnitude image */

      x = xmax;

      while( --x >= 0 ) {

        v = RGB_btoi(*sv2) - RGB_btoi(*sv1);  /* get vertical gradient   */
        h = RGB_btoi(*sh2) - RGB_btoi(*sh1);  /* get horizontal gradient */

        sv1 += nByte;
        sv2 += nByte;
        sh1 += nByte;
        sh2 += nByte;

        /*--------------- Approximation of sqrt(v^2+h^2)  --------------------*/
        if (v == 0 && h == 0) {     /* no edge found            */
          *dvalue = 0;              /* set zero to dstim      */
          dvalue += nByte;
        } else {
          av = RGB_abs(v);          /* absolute ver. gradient   */
          ah = RGB_abs(h);          /* absolute hor. gradient   */
          value = (C1 * (av + ah) + C2 * RGB_max(av,ah));
#ifdef YAIPS_RESULT_SCHIFT // Use integer multiply
          if( value >= 0) {
            value = (value * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT;
          } else {
            value = (value * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT;
          }
#endif
          value = ((value * CRANGE + CROUND) >> preci);
          clip32to8( value, dvalue);   /* clipping to [0,VALMAX]   */
          dvalue += nByte;
        }                 /* and set value to dstim   */
      }             /* terminate x-loop         */
    }
    /*---------------------- delete 1pixel boundary ------------------------*/
#ifdef use_again
    frame( dst1,(int16)-1,(int32)0,xmax+2,ymax+2);
#else
    YaIPS_RGB_Frame( pDst1, -1, 0);
#endif
  }  /* end only dstim */

  /*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
  /*<<<<<<<<<<<<<<<<<<<<  Modus: 2   only dirim  >>>>>>>>>>>>>>>>>>>>>>>>>>>*/
  /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
  else if (modus == 2) {

    sjump = pSrc->ld;                         /* initialize pointer       */
    d2jump = pDst2->ld;

    /*------------------------ start filter loop ---------------------------*/

    /*-------------------------- row by row --------------------------------*/
    for( iByte = 0; iByte < nByte; iByte++) {

      src_start = RGB_pixad( 0, 2, pSrc);
      dst2_start = RGB_pixad( 1, 1, pDst2);

      for(y = 1; y < ymax; y++) {

        /*-------------- Initialize pixel array of filter window -------------*/
        sh1 = src_start + iByte;
        sh2 = sh1 + 2 * nByte;
        sv1 = sh1 + nByte - sjump;
        sv2 = sh1 + nByte + sjump;

        ddir = dst2_start + iByte;              /* set new pointer in direction image */

        /*-------------------- Process row -----------------------------------*/
        x = xmax;
        while( --x >= 0 ) {

          v = RGB_btoi(*sv2) - RGB_btoi(*sv1);  /* get vertical gradient */
          h = RGB_btoi(*sh2) - RGB_btoi(*sh1);  /*get horizontal gradient*/

          sv1 += nByte;
          sv2 += nByte;
          sh1 += nByte;
          sh2 += nByte;

          /*------------------------------------------------------------------*/
          /*------------ Evaluate the direction of the gradient  -------------*/
          /*------------------------------------------------------------------*/
          /*  if (v == 0 && h == 0) *ddir++ = (int8)NOEDGE;       */
          /*  else {                  */
          /*    fdir = -atan2(v,h) * 180.0 / M_PI;          */
          /*    if (fdir < 0) fdir = 360.0 + fdir;          */
          /*    dir = (int32)(((fdir * 256.0) / 360.0) + 0.5);        */
          /*    if (dir == 0L || dir == 256L) dir = (int32)A0;        */
          /*    *ddir++ = (int8)dir;              */
          /*  }                         */
          /*------------------------------------------------------------------*/

          if (h == 0) {                 /* only vertical    */
            if (v == 0) *ddir = NOEDGE;     /* no edge found    */
            else if (v > 0) *ddir = A270;
            else *ddir = A90;
            ddir += nByte;
          }
          else if (v == 0) {              /* only horizontal  */
            if (h > 0) *ddir = A0;
            else *ddir = A180;
            ddir += nByte;
          }
          else {
            av = RGB_abs(v); ah = RGB_abs(h);        /* absolut gradients */
            if (av > ah ) {
              index = ( ((av<<6) + ah) / (ah << 1) );   /* get table index  */
              if      (index > 2607) dir = A90;
              else if (index >  868) dir = A90 - 1;
              else if (index >  520) dir = A90 - 2;
              else if (index >  371) dir = A90 - 3;
              else                   dir = RGB_btoi( atantab[index]);
            }
            else  {
              index = ( ((ah<<6) + av) / (av << 1) );
              if      (index > 2607) if (h > 0L) {
                         if (v < 0L) dir = A90 - 63; /* no zero */
                                       else dir = A90 - 65;
                                     }
                       else dir = A90 - 64;
              else if (index >  868) dir = A90 - 63;
              else if (index >  520) dir = A90 - 62;
              else if (index >  371) dir = A90 - 61;
              else                   dir = A90 - RGB_btoi(atantab[index]);
            }

            if (v < 0) {          /* get other quadrants by */
              if (h < 0) *ddir = (uchar)(A180 - dir);  /* mirroring vertical */
              else *ddir = (uchar)dir;         /* no mirroring       */
              ddir += nByte;
            }
            else {
              if (h > 0)  *ddir = (uchar)(A360 - dir); /* mirroring horizont.*/
              else *ddir = (uchar)(A180 + dir);        /* mirroring ver.+hor.*/
              ddir += nByte;
            }
          }
        }             /* terminate x-loop         */
        src_start += sjump;                   /* set pointers to new row  */
        dst2_start += d2jump;
      }             /* terminate y-loop         */

      /*----------------------------------------------------------------------*/
      /*---------------------- Process last row ------------------------------*/
      /*----------------------------------------------------------------------*/
      sh1 = src_start + iByte;                          /* initialize pointer       */
      sh2 = sh1 + 2 * nByte;
      sv1 = sh1 + nByte - sjump;
      sv2 = sh1 + nByte;

      ddir = dst2_start + iByte;        /* set pointer in direction image */

      x = xmax;
      while( --x >= 0 ) {

        v = RGB_btoi(*sv2) - RGB_btoi(*sv1);  /* get vertical gradient   */
        h = RGB_btoi(*sh2) - RGB_btoi(*sh1);  /* get horizontal gradient */

        sv1 += nByte;
        sv2 += nByte;
        sh1 += nByte;
        sh2 += nByte;

         /*------------- Evaluate the direction of the gradient  --------------*/
        if (h == 0) {                   /* only vertical    */
          if (v == 0) *ddir = NOEDGE;     /* no edge found    */
          else if (v > 0) *ddir = A270;
          else *ddir = A90;
          ddir += nByte;
        }
        else if (v == 0) {              /* only horizontal  */
          if (h > 0) *ddir = A0;
          else *ddir = A180;
          ddir += nByte;
        }
        else {
          av = RGB_abs(v); ah = RGB_abs(h);         /* absolut gradients  */
          if (av > ah ) {
            index = ( ((av<<6) + ah) / (ah << 1) );     /* get table index    */
            if      (index > 2607) dir = A90;
            else if (index >  868) dir = A90 - 1;
            else if (index >  520) dir = A90 - 2;
            else if (index >  371) dir = A90 - 3;
            else                   dir = RGB_btoi( atantab[index]);
          }
          else  {
            index = ( ((ah<<6) + av) / (av << 1) );
            if      (index > 2607) if (h > 0L) {
                                     if (v < 0L) dir = A90 - 63;   /* no zero */
                                     else dir = A90 - 65;
                                   }
                                   else dir = A90 - 64;
             else if (index >  868) dir = A90 - 63;
             else if (index >  520) dir = A90 - 62;
             else if (index >  371) dir = A90 - 61;
             else                   dir = A90 - RGB_btoi( atantab[index]);
          }

          if (v < 0) {                              /* get other quadrants by */
            if (h < 0) *ddir = (uchar)(A180 - dir);    /* mirroring vertical */
            else *ddir = (uchar)dir;                   /* no mirroring       */
            ddir += nByte;
          }
          else {
            if (h > 0)  *ddir = (uchar)(A360 - dir); /* mirroring horizontal */
            else *ddir = (uchar)(A180 + dir);        /* mirroring ver.+ hor. */
            ddir += nByte;
          }
        }
      }                                             /* terminate x-loop */
    }
    /*---------------------- delete 1pixel boundary ------------------------*/
#ifdef use_again
    frame(dst2,(int16)-1,(int32)0,xmax+2,ymax+2);
#else
    YaIPS_RGB_Frame( pDst2, -1, 0);
#endif
  }  /* end only DIRIM */

  /*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
  /*<<<<<<<<<<<<<<<<<<<<  Modus: 3   dstim + dirim  >>>>>>>>>>>>>>>>>>>>>>>>*/
  /*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
  else {

    sjump  = pSrc->ld;                        /* initialize pointer       */
    d1jump  = pDst1->ld;
    d2jump  = pDst2->ld;

    /*--------------------- start filter loop ------------------------------*/

    /*------------------------ row by row ----------------------------------*/
    for( iByte = 0; iByte < nByte; iByte++) {

      src_start  = RGB_pixad( 0, 2, pSrc);
      dst1_start = RGB_pixad( 1, 1, pDst1);
      dst2_start = RGB_pixad( 1, 1, pDst2);

      for(y = 1; y < ymax; y++) {

        /*-------------- Initialize pixel array of filter window -------------*/
        sh1 = src_start + iByte;
        sh2 = sh1 + 2 * nByte;
        sv1 = sh1 + nByte - sjump;
        sv2 = sh1 + nByte + sjump;

        dvalue = dst1_start + iByte;            /* set new pointer in magnitude image */
        ddir = dst2_start + iByte;              /* set new pointer in direction image */

        /*------------------------- Process row ------------------------------*/
        x = xmax;
        while( --x >= 0 ) {

          v = RGB_btoi(*sv2) - RGB_btoi(*sv1);  /* get vertical gradient */
          h = RGB_btoi(*sh2) - RGB_btoi(*sh1);  /*get horizontal gradient*/

          sv1 += nByte;
          sv2 += nByte;
          sh1 += nByte;
          sh2 += nByte;

          /*------------------------------------------------------------------*/
          /*-------------- Approximation of sqrt(v^2+h^2)  -------------------*/
          /*------------------------------------------------------------------*/
          if (v == 0 && h == 0) {     /* no edge found            */
            *dvalue = 0;            /* set zero to dstim      */
            dvalue += nByte;
          } else {
            av = RGB_abs(v);      /* absolut ver. gradient    */
            ah = RGB_abs(h);      /* absolut hor. gradient    */
            value = (C1 * (av + ah) + C2 * RGB_max(av,ah));
#ifdef YAIPS_RESULT_SCHIFT // Use integer multiply
            if( value >= 0) {
              value = (value * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT;
            } else {
              value = (value * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT;
            }
#endif
            value = ((value * CRANGE + CROUND) >> preci);
            clip32to8( value, dvalue); /* clipping to [0,VALMAX]   */
            dvalue += nByte;
          }         /* and set value to dstim   */

          /*------------------------------------------------------------------*/
          /*------------ Evaluate the direction of the gradient  -------------*/
          /*------------------------------------------------------------------*/
          if (h == 0) {                 /* only vertical    */
            if (v == 0) *ddir = NOEDGE;     /* no edge found    */
            else if (v > 0) *ddir = A270;
            else *ddir = A90;
            ddir += nByte;
          }
          else if (v == 0) {              /* only horizontal  */
            if (h > 0) *ddir = A0;
            else *ddir = A180;
            ddir += nByte;
          }
          else {
            if (av > ah ) {
              index = ( ((av<<6) + ah) / (ah << 1) );   /* get table index  */
              if      (index > 2607) dir = A90;
              else if (index >  868) dir = A90 - 1;
              else if (index >  520) dir = A90 - 2;
              else if (index >  371) dir = A90 - 3;
              else                   dir = RGB_btoi( atantab[index]);
            }
            else  {
              index = ( ((ah<<6) + av) / (av << 1) );
              if      (index > 2607) if (h > 0L) {
                         if (v < 0L) dir = A90 - 63; /* no zero */
                                       else dir = A90 - 65;
                                     }
                       else dir = A90 - 64;
              else if (index >  868) dir = A90 - 63;
              else if (index >  520) dir = A90 - 62;
              else if (index >  371) dir = A90 - 61;
              else                   dir = A90 - RGB_btoi( atantab[index]);
            }

            if (v < 0) {          /* get other quadrants by */
              if (h < 0) *ddir = (uchar)(A180 - dir);  /* mirroring vertical */
              else *ddir = (uchar)dir;         /* no mirroring       */
              ddir += nByte;
            }
            else {
              if (h > 0)  *ddir = (uchar)(A360 - dir); /* mirroring horizont.*/
              else *ddir = (uchar)(A180 + dir);        /* mirroring ver.+hor.*/
              ddir += nByte;
            }
          }
        }             /* terminate x-loop         */
        src_start  += sjump;                  /* set pointers to new row  */
        dst1_start += d1jump;
        dst2_start += d2jump;
      }             /* terminate y-loop */

      /*----------------------------------------------------------------------*/
      /*---------------------- Process last row ------------------------------*/
      /*----------------------------------------------------------------------*/
      sh1 = src_start + iByte;                /* initialize pointer       */
      sh2 = sh1 + 2 * nByte;
      sv1 = sh1 + nByte - sjump;
      sv2 = sh1 + nByte;

      dvalue = dst1_start + iByte;      /* set pointer in magnitude image */
      ddir = dst2_start + iByte;        /* set pointer in direction image */

      x = xmax;
      while( --x >= 0 ) {

        v = RGB_btoi(*sv2) - RGB_btoi(*sv1);  /* get vertical gradient   */
        h = RGB_btoi(*sh2) - RGB_btoi(*sh1);  /* get horizontal gradient */

        sv1 += nByte;
        sv2 += nByte;
        sh1 += nByte;
        sh2 += nByte;

        /*--------------- Approximation of sqrt(v^2+h^2)  --------------------*/
        if (v == 0 && h == 0) {     /* no edge found            */
          *dvalue = 0;              /* set zero to dstim      */
          dvalue += nByte;
        } else {
          av = RGB_abs(v);      /* absolut ver. gradient    */
          ah = RGB_abs(h);      /* absolut hor. gradient    */
          value = (C1 * (av + ah) + C2 * RGB_max(av,ah));
#ifdef YAIPS_RESULT_SCHIFT // Use integer multiply
          if( value >= 0) {
            value = (value * ResMultArgLong + ResMultArgLong2) >> YAIPS_RESULT_SCHIFT;
          } else {
            value = (value * ResMultArgLong - ResMultArgLong2) >> YAIPS_RESULT_SCHIFT;
          }
#endif
          value = ((value * CRANGE + CROUND) >> preci);
          clip32to8( value, dvalue);   /* clipping to [0,VALMAX]   */
          dvalue += nByte;
        }                 /* and set value to dstim   */

        /*------------- Evaluate the direction of the gradient  --------------*/
        if (h == 0) {                   /* only vertical    */
          if (v == 0) *ddir = NOEDGE;     /* no edge found    */
          else if (v > 0) *ddir = A270;
          else *ddir = A90;
          ddir += nByte;
        }
        else if (v == 0) {              /* only horizontal  */
          if (h > 0) *ddir = A0;
          else *ddir = A180;
          ddir += nByte;
        }
        else {
          if (av > ah ) {
            index = ( ((av<<6) + ah) / (ah << 1) );     /* get table index  */
            if      (index > 2607) dir = A90;
            else if (index >  868) dir = A90 - 1;
            else if (index >  520) dir = A90 - 2;
            else if (index >  371) dir = A90 - 3;
            else                   dir = RGB_btoi( atantab[index]);
          }
          else  {
            index = ( ((ah<<6) + av) / (av << 1) );
            if      (index > 2607) if (h > 0L) {
                                     if (v < 0L) dir = A90 - 63;   /* no zero */
                                     else dir = A90 - 65;
                                   }
                                   else dir = A90 - 64;
             else if (index >  868) dir = A90 - 63;
             else if (index >  520) dir = A90 - 62;
             else if (index >  371) dir = A90 - 61;
             else                   dir = A90 - RGB_btoi( atantab[index]);
          }

          if (v < 0) {                              /* get other quadrants by */
            if (h < 0) *ddir = (A180 - dir);    /* mirroring vertical */
            else *ddir = dir;                   /* no mirroring       */
            ddir += nByte;
          }
          else {
            if (h > 0)  *ddir = (A360 - dir); /* mirroring horizontal */
            else *ddir = (A180 + dir);        /* mirroring ver.+ hor. */
            ddir += nByte;
          }
        }
      }                                             /* terminate x-loop */
    }
    /*--------------------- delete 1pixel boundary -------------------------*/
#ifdef use_again
    frame(dst1,(int16)-1,(int32)0,xmax+2,ymax+2);
    frame(dst2,(int16)-1,(int32)0,xmax+2,ymax+2);
#else
    YaIPS_RGB_Frame( pDst1, -1, 0);
    YaIPS_RGB_Frame( pDst2, -1, 0);
#endif
  }  /* end DSTIM + DIRIM */

  return;
}

/***************************************************************************
* delnomax()
*
* only with modus 3
*
* NOTE: Always have all three images as argument and all
*       images have have the same sizes.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/
static int delnomax( YaIPS_RGB_ImgD_t *pSrc1,
                     YaIPS_RGB_ImgD_t *pSrc2,
                     YaIPS_RGB_ImgD_t *pDst)
{
  uchar *row_store, *row_store_start;     /* pointer to row_store */
  uchar *row_work, *row_work_start, *temp;  /* pointer to row_work  */
  uchar *s, *sdir, *dvalue;           /* pointer to srcim and dstim */
  uchar *s_start, *sdir_start, *d_start;
  int   sjump, sdirjump, djump, iByte, nByte;
  uchar *g_a, *g_b, *g_c, *g_f, *g_g, *g_h, *g_i;    /* pointer to srcim */
  unsigned int g, g1, g2, sdir32, ipola, ipolb, g_d;
  int   x, y, w;       /* counter variables*/
  int   xmax, ymax;         /* image dimensions */

  xmax = pSrc1->xx;           /* set image dimensions     */
  ymax = pSrc2->yy - 2;
  sdirjump = pSrc2->ld;     /* initialize pointer       */
  sjump  = pSrc1->ld;
  djump  = pDst->ld;

  nByte = pSrc1->d;

  /*------------------- Allocate memory for temporary rows -----------------*/

  row_store = (uchar *)malloc( xmax);
  if( row_store == NULL) return(-6);
  row_store_start = row_store;

  row_work = (uchar *)malloc( xmax);
  if (row_work == NULL) {
    free( row_store);
    return(-6);
  }
  row_work_start = row_work;

  /*--------------------- Copy 1st row to temporary row --------------------*/

  for( iByte = 0; iByte < nByte; iByte++) {

    sdir_start = RGB_pixad( 1, 1, pSrc2);
    s_start = RGB_pixad( 1, 1, pSrc1);
    d_start = RGB_pixad( 1, 1, pDst);

    sdir_start += iByte;
    s_start += iByte;
    d_start += iByte;

    s = s_start - sjump - 1;      /* initialize pointer       */

    w = xmax;
    row_work = row_work_start;
    while ( --w >= 0 ) {
      *row_work++ = *s;      /* copy values to row_work  */
      s += nByte;
    }

    /*--------------------------- start filter loop --------------------------*/

    /*------------------------------ row by row ------------------------------*/

    for( y = 1; y <= ymax; y++) {

      dvalue = d_start;       /* set new pointer in destination image */
      sdir = sdir_start;              /* set new pointer in direction image */

      /*---------------- Copy row of source image to temporary row -----------*/
      s = s_start - nByte;        /* initialize pointer       */
      row_store = row_store_start;
      w = xmax;
      while ( --w >= 0 ) {
        *row_store++ = *s;    /* copy values to row_store  */
        s += nByte;
      }

      /*---------------- Initialize pointer to neighbour gradients -----------*/
      s = s_start;                  /* pointer to srcim         */
      g_a = row_work_start;     /* pointer to row_work      */
      g_b = row_work_start + 1;
      g_c = row_work_start + 2;
      g_f = s + nByte;        /* pointer to srcim         */
      g_g = s + sjump - nByte;
      g_h = s + sjump;
      g_i = s + sjump + nByte;
      g_d = RGB_btou(*(s - nByte));    /* get last value           */

      /*-------------------------- Process row -------------------------------*/
      x = xmax - 2;
      while( --x >= 0 ) {
        sdir32 = RGB_btoi(*sdir);
        sdir += nByte;
        if (sdir32 == 0) {
          g_d = 0;          /* no edge found */
        } else {
          ipola = (unsigned int)(64 - ipoltab[sdir32]);    /* get weighting values */
          ipolb = (unsigned int)ipoltab[sdir32];
          switch(sdir32 >> 5) {           /* select sector    */
          case 0:   /*  0 <= angle < 44 */
            g1 = ipola * RGB_btou(*g_f) + ipolb * RGB_btou(*g_c);
            g2 = ipola * g_d + ipolb * RGB_btou(*g_g);
            break;
          case 1:     /* 45 <= angle < 90 */
            g1 = ipola * RGB_btou(*g_c) + ipolb * RGB_btou(*g_b);
            g2 = ipola * RGB_btou(*g_g) + ipolb * RGB_btou(*g_h);
            break;
          case 2:     /* 90 <= angle < 135 */
            g1 = ipola * RGB_btou(*g_b) + ipolb * RGB_btou(*g_a);
            g2 = ipola * RGB_btou(*g_h) + ipolb * RGB_btou(*g_i);
            break;
          case 3:   /* 135 <= angle < 180 */
            g1 = ipola * RGB_btou(*g_a) + ipolb * g_d;
            g2 = ipola * RGB_btou(*g_i) + ipolb * RGB_btou(*g_f);
            break;
          case 4:   /*  180 <= angle < 225 */
            g1 = ipola * g_d + ipolb * RGB_btou(*g_g);
            g2 = ipola * RGB_btou(*g_f) + ipolb * RGB_btou(*g_c);
            break;
          case 5:     /* 225 <= angle < 270 */
            g1 = ipola * RGB_btou(*g_g) + ipolb * RGB_btou(*g_h);
            g2 = ipola * RGB_btou(*g_c) + ipolb * RGB_btou(*g_b);
            break;
          case 6:     /* 270 <= angle < 315 */
            g1 = ipola * RGB_btou(*g_h) + ipolb * RGB_btou(*g_i);
            g2 = ipola * RGB_btou(*g_b) + ipolb * RGB_btou(*g_a);
            break;
          case 7:   /* 315 <= angle < 360 */
            g1 = ipola * RGB_btou(*g_i) + ipolb * RGB_btou(*g_f);
            g2 = ipola * RGB_btou(*g_a) + ipolb * g_d;
            break;
          }         /* end of switch dir        */

          g_d = RGB_btou(*s);        /* store akt. value         */
          g = g_d << 6;       /* get (gradient * 64)      */
          if ((g < g1) || (g < g2)) *dvalue = 0;  /* eliminate non maximum    */
        }           /* end of dir != 0
            */
        dvalue += nByte;         /* increase dstim pointer   */
        s += nByte;              /* increase srcim pointer   */
        g_a++; g_b++; g_c++;
        g_f += nByte; g_g += nByte; g_h += nByte; g_i += nByte;
      }           /* terminate x-loop         */
      temp = row_work_start;         /* swap row_work and row_store */
      row_work_start = row_store_start;
      row_store_start = temp;
      s_start += sjump;       /* set pointers to new row  */
      sdir_start += sdirjump;
      d_start += djump;
    }           /* terminate y-loop         */
  }

  /*-------------------- Free memory for temporary rows --------------------*/

  free(row_store_start);
  free(row_work_start);

  return( 0);                                 // Return OK
}


/***************************************************************************
* copylikegauss()
*
****************************************************************************
*/
static void copylikegauss( YaIPS_RGB_ImgD_t *pSrc,
                           YaIPS_RGB_ImgD_t *pDst)
{
  int   y;          /* counter variable */
  uchar *s_start, *d_start;
  int   ymax, sjump;

  /*------------------------ Initialize variables --------------------------*/

  //x/xmax = pSrc->xx;        /* set image dimensions          */
  ymax = pSrc->yy - 1;
  d_start = RGB_pixad( 0, 1, pDst);    /* shift image in dstim by one row */
  s_start = RGB_pixad( 0, 0, pSrc);

  sjump = pSrc->ld;

  /*-------------------------- row by row ----------------------------------*/

  for( y = 0; y < ymax; y++) {          /* last row is lost         */

    d_start = RGB_pixad( 0, y + 1, pDst);    /* shift image in dstim by one row */
    s_start = RGB_pixad( 0, y, pSrc);

    memcpy( d_start, s_start, sjump);
  }

  return;
}

/***************************************************************************
* YaIPS_RGB_Canny
* Canny edge filter
*
* ppDst        Destination (absolute amount) image
* ppDir        Direction image
* pSrc         Source image
* sigma        Sigma for gaussian
* ResMultArg   Result multiplier, default should be 1.0
* mode         If true, maxima elimination
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Canny( Fl_RGB_Image **ppDst, // Out: Destination (absolute amount) image
                    Fl_RGB_Image **ppDir, // Out: Direction image
                    Fl_RGB_Image *pSrc,   // Source image
                    float sigma,          // Sigma for gaussian
                    float ResMultArg,     // Result multiplier
                    int mode)             // If true, maxima elimination
{
  int image_mode, ierr;
  YaIPS_RGB_ImgD_t iDst, iDir, iSrc;
  Fl_RGB_Image *pTempDir;

  // Reset image mode and other variables
  image_mode = 0;
  pTempDir = NULL;                            // Have no temporary direction image

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  if( ppDst != NULL) {                        // Have a destination image

    image_mode = 1;

    // Ensure that pPDst image has the same size and same pixel amount as pSrc
    ierr = YaIPS_RGB_EnsureSameSize( ppDst, pSrc);
    if( ierr != 0)  {                         // Check for error
      return( ierr);
    }

    ierr = YaIPS_RGB_to_ImgD( *ppDst, &iDst);
    if( ierr != 0)  {                           // Check for error
      return( ierr);
    }
  }

  if( ppDir != NULL) {                        // Have a direction image

    image_mode += 2;

    // Ensure that pPDst image has the same size and same pixel amount as pSrc
    ierr = YaIPS_RGB_EnsureSameSize( ppDir, pSrc);
    if( ierr != 0)  {                         // Check for error
      return( ierr);
    }

    ierr = YaIPS_RGB_to_ImgD( *ppDir, &iDir);
    if( ierr != 0)  {                         // Check for error
      return( ierr);
    }
  } else if( mode != 0) {                     // maxima elimination and no direction image

    image_mode += 2;

    // Ensure that pPDst image has the same size and same pixel amount as pSrc
    ierr = YaIPS_RGB_EnsureSameSize( &pTempDir, pSrc);
    if( ierr != 0)  {                         // Check for error
      return( ierr);
    }

    ierr = YaIPS_RGB_to_ImgD( pTempDir, &iDir);
    if( ierr != 0)  {                         // Check for error
      return( ierr);
    }
  }

  // Check parameters

#ifdef use_again
  if( sigma < 0.57 && sigma > 0.0) return(-7);
#endif
  if (mode != 0 && image_mode != 3) return(-9);

  switch(image_mode) {
    case 0:
      return(-5);
      break;
    case 1: /*------------ only dstim -------------*/

      if (sigma > 0.0) {
        ierr = YaIPS_RGB_GaussXY_sub( &iSrc, &iDst, sigma, 1, YAIPS_GAUSXY_MODE_XY);
        if (ierr < 0) return(ierr);
      }
      else copylikegauss( &iSrc, &iDst); /* copy image in dstim like gaussian */
      gradient( &iDst, &iDst, NULL, image_mode, ResMultArg);
      break;

    case 2: /*------------ only dirim -------------*/

      if (sigma > 0.0) {
        ierr = YaIPS_RGB_GaussXY_sub( &iSrc, &iDir, sigma, 1, YAIPS_GAUSXY_MODE_XY);
        if (ierr < 0) return(ierr);
      }
      else copylikegauss( &iSrc, &iDir); /* copy image in dirim like gaussian */
      gradient( &iDir, NULL, &iDir, image_mode, ResMultArg);
      break;

    case 3: /*--------- dstim + dirim ------------*/

      if (sigma > 0.0) {
        ierr = YaIPS_RGB_GaussXY_sub( &iSrc, &iDst, sigma, 1, YAIPS_GAUSXY_MODE_XY);
        if (ierr < 0) return(ierr);
      }
      else copylikegauss( &iSrc, &iDst); /* copy image in dstim like gaussian */
      gradient( &iDst, &iDst, &iDir, image_mode, ResMultArg);
      if (mode > 0) {
        ierr = delnomax( &iDst, &iDir, &iDst);
        if (ierr < 0) return(ierr);
      }
      break;
  } /* end switch image_mode */

  if( pTempDir != NULL) {                     // Used a temporary direction image

    pTempDir->release();
  }

  return( 0);                                 // Return OK
}

/******************************** End Of File ********************************/

