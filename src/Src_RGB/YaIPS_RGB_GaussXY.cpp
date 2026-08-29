/****************************************************************************

  YaIPS_RGB_GaussXY.cpp

  Fl_RGB_Image image processing.
  1xN, Nx1, NxN Gaussian lowpass filter

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

/* for gaussian() filter */
#define GPREC 22                        /* Precision 2^22 = 4194304        */
#define FGPREC (float)4194304.0         /* Precision for float variables   */
#define GROUND (int)2097152             /* 0.5 * 2^GPREC        */

/***************************************************************************
* YaIPS_RGB_GaussXY_sub()
*
* NOTE: All valid images have the same size.
*
* ModeXY       0: 1*N X/horizontal gaussian lowpass filter
*              1: N*1 Y/vertical gaussian lowpass filter
*              2: NxN XY gaussian lowpass filter
*
****************************************************************************
*/

int YaIPS_RGB_GaussXY_sub( YaIPS_RGB_ImgD_t *pSrc,
                           YaIPS_RGB_ImgD_t *pDst,
                           float g_sigma,
                           int yOff,
                           int ModeXY)              // XY Mode
{
  uchar *s, *ss, *s_op, *s_start; /* pointer to source image        */
  uchar *d, *d_start;             /* pointer to destination image   */
  uchar *col, *col_start;         /* pointer to temporary column    */
  int *op;                        /* pointer to operator            */
  int *op_start;                  /* pointer to begin of operator   */
  int   half_op, op_width = 0;    /* dimensions of operator         */
  int temp_data, data, sum;       /* variables for integration      */
  double g_factor, g_exp, gain;   /* var. for evaluation of gaussian */
  int   i, j, x, y, w, iter;      /* counter variables               */
  int xmax, ymax, sjump, djump;   /* dimensions of images            */
  int r_bound, l_bound;           /* variables for boundary processing*/
  int limit;                      /* dimensions of operator           */
  int xx, yy, iByte, nByte, op_width_1;

  col_start = NULL;              // No memory allocated until now
  op_start  = NULL;

  /*------------------------------------------------------------------------*/
  /*------------------- Initialize operator with Gaussian ------------------*/
  /*------------------------------------------------------------------------*/

  g_factor =  1.0 / (sqrt(2.0 * M_PI) * g_sigma);
  g_exp = -1.0 / (2.0 * g_sigma * g_sigma);

  /*----------------------- compute operator width -------------------------*/

  limit = (int)((2.0 * g_sigma) + 0.5);   /* 2*sigma = 95% + round */
  op_width = (limit * 2) + 1;
  half_op = op_width / 2;

  /*--------------------- Allocate memory for operator ---------------------*/

  op = (int *)malloc( (half_op + 1) * sizeof(int));

  if (op == NULL) {
    errstring = ERR_IPS_NO_MEM;
    return(-6);
  }

  /*------------------- Evaluate and store operator values -----------------*/
  gain = 1.0;
  for (iter = 0; iter < 2; iter++) {
    sum = 0; /* we use only 95% of the gaussian curve, so we need a gain to
                reach the filter coefficient sum 1.0 */
    for(i=0, j=limit; i < limit; i++, j--) {
      op[i] = (int)( (gain * g_factor * exp(g_exp * j * j) * FGPREC) + 0.5 );
      sum += op[i];
      //x/PRINTF3("op[%d] = %d (%lf) \n", i, op[i], (double)op[i] / FGPREC);
    }
    sum *= 2;    /* whole kernel */
    op[i] = (int)( (gain * g_factor * FGPREC) + 0.5 );/* max operator value */
    sum += op[i];
    //x/PRINTF3("op[%d] = %d (%lf) \n", i, op[i], (double)op[i] / FGPREC);
    //x/PRINTF2("        %d (%lf) \n", sum, (double)sum / FGPREC);

    gain *= FGPREC / (double)sum;  /* gain to compensate smaller kernel */
    //x/PRINTF1(" gain %lf\n", gain);
  }

  //x/PRINTF1("gaussian operator width = %d \n", op_width);

 /*------------------------ Initialize variables --------------------------*/
  xx = YaIPS_RGB_xx2min( pSrc, pDst);
  yy = YaIPS_RGB_yy2min( pSrc, pDst);
  xmax = xx - 1;             /* set image dimensions      */
  ymax = yy - 1;
  djump = pDst->ld;
  sjump = pSrc->ld;

  nByte = pSrc->d;

  op_start = op;

  /*------------------- check image size > op_width ------------------------*/
  // NOTE 29.04.2025 RR: The '-1' in the check below is done only for
  //                     the error output. Otherwise the image size
  //                     can be lower than the kernel size.

  if ( op_width > xmax - 1 || op_width > ymax - 1 ) {

    //x/PRINTF4("sigma = %lf, op_width %d, xmax %d , ymax %d \n", g_sigma, op_width, xmax, ymax);
    free( op_start);

    if( errstring == NULL) {                  // No error until now

      if( op_width > xmax - 1) {              // To big

        sprintf( errbuffer, LangStringLookup( "&RGB_Filter_Gauss1=Width %d >= filter %d"), xx, op_width);

      } else if( op_width > ymax - 1) {       // To big

        sprintf( errbuffer, LangStringLookup( "&RGB_Filter_Gauss2=Height %d >= filter %d"), yy, op_width);

      } else {

        sprintf( errbuffer, LangStringLookup( "&RGB_Filter_Gauss3=Image >= filter %d"), op_width);
      }
      errstring = errbuffer;

    }
    return(-8);
  }

  /*------------------------------------------------------------------------*/
  /*--------------------------- X Convolution ------------------------------*/
  /*------------------------------------------------------------------------*/

  if( ModeXY != YAIPS_GAUSXY_MODE_Y) {             // Have a mode X or XY

    l_bound = (op_width / 2) - 1;
    r_bound = xmax - l_bound;

    op_width_1 = (op_width - 1) * nByte;

    /*-------------------------- row by row ----------------------------------*/
    for( iByte = 0; iByte < nByte; iByte++) {

      d_start = RGB_pixad( 0, yOff, pDst);     /* shift image in dstim by yOff rows */
      s_start = RGB_pixad( 0, 0, pSrc);

      d_start += iByte;
      s_start += iByte;

      for (y=yOff; y < yy; y++) {  /* last row is lost         */

        s_op = s_start;       /* initialize pointer       */
        d = d_start;
        op = op_start;
        j = half_op + 1;

        /*--------------------- Process left boundary --------------------------*/
        for (x=0; x <= l_bound; x++, j--) {
          data = 0;               /* initialize variables     */
          i = 0;
          s = s_op;               /* initialize pointer       */
          for (i=0; i < j; i++) {
            data += *op++;        /* add operator-values      */
          }           /* for extrapolation        */
          data *= RGB_btoi(*s);    /* multiply with 1st value  */
          s += nByte;
          --op;
          for (i=j; i < op_width; i++) {
            if (i > half_op)          /* only half kernel stored  */
              --op;
            else
              ++op;
            data += ( *op * RGB_btoi(*s) );  /* add weighted components  */
            s += nByte;
          }
          *d = (uchar)((data+GROUND) >> GPREC);  /* set value to dstim       */
          d += nByte;
        }

        /*---------------------- Process middle part ---------------------------*/
        for (x=l_bound+1; x < r_bound; x++) {
          data = 0;         /* initialize variables     */
          i = half_op;
          op = op_start;        /* initialize pointer       */
          s = s_op;
          s_op += nByte;
          ss = s + op_width_1;
          while (--i >= 0) {      /* add weighted components  */
            data += ( *op++ * (RGB_btoi(*s) + RGB_btoi(*ss)) );
            s += nByte;
            ss -= nByte;
          }
          data += ( *op++ * RGB_btoi(*s) );  /* add maximal coefficient  */

          *d = (uchar)((data+GROUND) >> GPREC);  /* set value to dstim       */
          d += nByte;
        }

        /*---------------------- Process right boundary ------------------------*/
        j = op_width - 2;
        for (x=r_bound; x <= xmax; x++, j--) {
          data = temp_data = 0;     /* initialize variables     */
          i = 0;
          s = s_op;       /* initialize pointer       */
          s_op += nByte;
          op = op_start - 1;
          for (i=0; i < j; i++) {
            if (i > half_op)
              --op;
            else
              ++op;
            data += ( *op * RGB_btoi(*s) );  /* add weighted components  */
            s += nByte;
          }
          for (i=j; i < op_width; i++) {
            if (i > half_op)      /* only half kernel stored  */
              temp_data += *(--op);   /* add operator-values      */
            else            /* for extraploation        */
              temp_data += *(++op);
          }
          data += (temp_data * RGB_btoi(*s));    /* multiply with last value */

          *d = (uchar)((data+GROUND) >> GPREC);  /* set value to dstim       */
          d += nByte;
        }             /* terminate x-loop         */
        s_start += sjump;                     /* set pointers to new row  */
        d_start += djump;
      }             /* terminate y-loop         */
    }
  }

  /*------------------------------------------------------------------------*/
  /*--------------------------- Y Convolution ------------------------------*/
  /*------------------------------------------------------------------------*/

  if( ModeXY != YAIPS_GAUSXY_MODE_X) {             // Have a mode Y or XY

    /*------------------- Allocate memory for temporary column ---------------*/
    col = (uchar *)malloc( (int)((yy - yOff + op_width - 1) * sizeof(uchar)) );

    if (col == NULL) {
      free( op_start);
      errstring = ERR_IPS_NO_MEM;
      return(-6);
    }
    col_start = col;

    /*--------------------------- column by column ---------------------------*/
    for (x=0; x <= xmax; x++) {

      /*----------------------- Initialize variables -------------------------*/
      for( iByte = 0; iByte < nByte; iByte++) {

        d_start = RGB_pixad( x, yOff, pDst);

        d_start += iByte;

        col = col_start;
        s_start = col_start;
        d = d_start;

        /*----------- Copy column of source image to temporary column ----------*/

        if( ModeXY != YAIPS_GAUSXY_MODE_Y) {             // Have a mode X or XY

          // Already have a filtered source image. Image is in destination image

          ss = d;

          for (w=0; w < half_op; w++) *col++ = *ss;  /* fill col for right boundary */

          for (w=yOff; w < yy; w++) {
            *col++ = *ss;       /* copy grey value to col   */
            ss += djump;        /* go to next value         */
          }
          ss -= djump;        /* last pixel in column     */

          for (w=0; w<half_op; w++) *col++ = *ss;  /* fill col for right boundary */

        } else {                                      // Have a mode X or XY

          // Have no X filter run above. Fill with source image

          ss = RGB_pixad( x, yOff, pSrc);
          ss += iByte;

          for (w=0; w < half_op; w++) *col++ = *ss;  /* fill col for right boundary */

          for (w=yOff; w < yy; w++) {
            *col++ = *ss;       /* copy grey value to col   */
            ss += sjump;        /* go to next value         */
          }
          ss -= sjump;        /* last pixel in column     */

          for (w=0; w<half_op; w++) *col++ = *ss;  /* fill col for right boundary */
        }

        /*------------------- Process temporary column -------------------------*/
        for (w=yOff; w < yy; w++) {
          data = 0;         /* initialize variables     */
          i = half_op;
          op = op_start;        /* initialize pointer       */
          s = s_start++;
          ss = s + op_width - 1;
          while (--i >= 0) {      /* add weighted components  */
            data += ( *op++ * (RGB_btoi(*s++) + RGB_btoi(*ss--)) );
          }
          data += ( *op * RGB_btoi(*s) );    /* add maximum coefficient  */
          *d = (uchar)((data+GROUND) >> GPREC);  /* set value to dstim       */
          d += djump;       /* set pointer to new row   */
        }             /* terminate w-loop         */

        d_start += nByte;                            /* set pointer to new col   */
      }
    }
  }               /* terminate x-loop         */

  /*------------------ Free memory for temporary column ---------------------*/

  if( col_start != NULL) {
    free( col_start);
  }

  if( op_start != NULL) {
    free( op_start);
  }
  return (0);
}

/***************************************************************************
* YaIPS_RGB_GaussXY
* N*N Gaussian lowpass filter.
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* KernelSize   Horizontal kernel size, must be odd and >= 3
* ModeXY       0: 1*N X/horizontal gaussian lowpass filter
*              1: N*1 Y/vertical gaussian lowpass filter
*              2: NxN XY gaussian lowpass filter
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_GaussXY( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                       Fl_RGB_Image *pSrc,      // Source image
                       int KernelSize,          // Kernel size
                       int ModeXY)              // XY Mode
{
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  int ierr;
  float g_sigma;

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

  // Check kernel size
  KernelSize |= 1;                                  // Must be odd
  if( KernelSize < 3) {                             // Ensure minimum value
    KernelSize = 3;
  }

  // Calculate sigma used by gaussian_sub()
  g_sigma = (KernelSize - 1) / 4.0;

  return( YaIPS_RGB_GaussXY_sub( &iSrc, &iDst, g_sigma, 0, ModeXY));
}

/******************************** End Of File ********************************/
