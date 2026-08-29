/* **************************************************************************
/@
/@ Short-title: quality of cdf-correlation
/@
/@ ==========================================================================
/@
/@ INDEX
/@
/@ # Mcdf2_txyqual   # cdf_qual2    # teach quality of cdf correlation
/@ # Mcdf2_txqual    # cdf_qual2    # teach quality of x-unidirectional cdf 
/@ # Mcdf2_tyqual    # cdf_qual2    # teach quality of y-unidirectional cdf
/@ # Mcdf2_pqual     # cdf_qual2    # processing quality of cdf correlation
/@
/@ USER DESCRIPTION
/@
/@.. cdf2_tsetup w1lin w1sqrt w1sq w2lin w2sqrt w2sq w3lin w3sqrt w3sq wsum
/@.     Setup teach quality weights
/@
/@      PARAMETERS
/@      w1lin           (sfloat) linear weight of peakness quality (default 1.0)
/@      w1sqrt          (sfloat) square root weight of peakness quality (default 0.0)
/@      w1sq            (sfloat) square weight of peakness quality (default 0.0)
/@      w2lin           (sfloat) linear weight of neighbour peak quality (default 1.0)
/@      w2sqrt          (sfloat) square root weight of neighbour peak quality (default 0.0)
/@      w2sq            (sfloat) square weight of neighbour peak quality (default 0.0)
/@      w3lin           (sfloat) linear weight of contrast quality (default 0.75)
/@      w3sqrt          (sfloat) square root weight of contrast quality (default 0.25)
/@      w3sq            (sfloat) square weight of contrast quality (default 0.0)
/@      wsum            (sfloat) total quality multiplicative weight (default 300.0)
/@      wq1compq2       (sfloat) compensation weight of q2 with 1/q1 (default 0.0)
/@
/@.. cdf2_txyqual srcim x0 y0 norma thres mode
/@.     Teach quality of cdf correlation
/@
/@.. cdf2_txqual srcim x0 y0 norma thres mode
/@.     Teach quality of x-unidirectional cdf correlation
/@
/@.. cdf2_tyqual srcim x0 y0 norma thres mode
/@.     Teach quality of y-unidirectional cdf correlation
/@
/@.. cdf2_pqual srcim thres norm norma
/@.     Processing quality of cdf correlation
/@
/@      1.: teach quality functions:
/@
/@      The quality of a correlation reference pattern is computed.
/@      'srcim' is the autocorrelation result image of the reference pattern.
/@      3 subqualities are computed:
/@
/@      - pattern distinctiveness (contrast)
/@      - bidirectional direction distinctiveness
/@      - uniqueness of pattern relative to neighbourhood
/@
/@      cdf2_txyqual:
/@      cdf2_txqual:
/@      cdf2_tyqual:  a teach quality is computed out of a combination
/@                    of the 3 subqualities. This teach quality has to
/@                    lie above the given threshold. The quality norm factor
/@                    is returned.
/@
/@      cdf2_pqual:   The correlation optimum is normalized by the quality
/@                    norm factor. It has to lie above the given threshold.
/@
/@      Typical values of the quality thresholds are:
/@      3 for teach threshold      
/@      7 for processing threshold
/@
/@      PARAMETERS
/@      srcim           (sfloat) correlation result image from cdf
/@      x0 y0           (int16)  minimum position returned by cdf
/@      norma           (int32)  area of correlation object for normalization
/@      thres           (int16)  quality threshold
/@      norm            (int16)  quality norm factor (max - min of teach
/@                               quality correlation image)
/@      mode            (int16)  0: only computation of quality norm factor
/@                               1: quality check
/@
/@      RESTRICTIONS
/@      cdf2_txyqual: minimum size of srcim for teach quality: 7x7 pixel
/@      cdf2_txqual:  y-size of correl must be exactly 5, x-size > 7
/@      cdf2_tyqual:  x-size of correl must be exactly 5, y-size > 7
/@   
/@      SEE ALSO
/@
/@ FUNCTION DESCRIPTION
/@
/@     #include <math.h>  
/@     #include "portab.h"
/@     #include "sip.h"
/@     #include "tstpr.h"
/@
/@     int cdf2_testSetup(w1lin, w1sqrt, w1sq, w2lin, w2sqrt, w2sq, 
/@                       w3lin, w3sqrt, w3sq, wtotal, wcomp12)
/@       sfloat w1lin, w1sqrt, w1sq, w2lin, w2sqrt, w2sq, 
/@              w3lin, w3sqrt, w3sq, wtotal, wcomp12;
/@     int cdf2_txyqual(srcim, x0, y0, norma, thres, retqual, mode)
/@     int cdf2_txqual(srcim, x0, y0, norma, thres, retqual, mode)
/@     int cdf2_tyqual(srcim, x0, y0, norma, thres, retqual, mode)
/@     int cdf2_pqual(srcim, thres, norm, norma)
/@     int cdf2_pqual2(srcim, thres, norm, norma, wr_inFactor)
/@       Timages *srcim;
/@       int16 x0, y0;
/@       int16 thres, dirthres;
/@       int32 norma;
/@       int16 norm, mode;
/@       double wr_inFactor;
/@       int16 *retqual, *retdirqual;
/@
/@
/@ RETURN VALUES
/@
/@      All routines return positive values or 0 after successful execution.
/@      In case of error a negative value is returned.
/@
/@      special error codes:
/@      -50  bad pattern
/@      -51  bad pattern contrast
/@      -52  pattern not unique
/@      -53  pattern not usable
/@
/@ MODIFICATIONS
/@ V 1.00 : First edition released.
/@ V 1.01 : mode introduced
/@ V 1.02 : contrast quality enhanced (problems with low contrast patterns)
/@ V 1.03 : bug fixed: if x0/y0 are too near image border
/@ V 1.04 : special error codes
/@ V 1.05 : cdf_testSetup
/@ V 1.06 : quality calculation from alike+
/@ V 1.07 : lin/sqrt-weights
/@ V 1.08 : library version (all cdf_ functions to cdf2_), 
/@          filename cdf_qual.c -> cdf_qual2.c
/@ V 1.09 : For X or Y Search Patterns, findBiggestNeighbour got wrong
/@          neighbour value in some rare cases.
/@          --> AreaType QUAL_AREATYPE_xxx
/@ V 1.10 : bug fix: cdf2_tqual(), preset 'mean = 0.0' before summation loop (was mean = absmin) 
/@ V 1.11 : * Argument nomra from int16 changed to int32
/@            for functions cdf2_tqual(), cdf2_txyqual(), cdf2_txqual(), cdf2_tyqual(), cdf2_pqual()
/@            Got problems with big windows (area > 0x7fff)
/@          * Include sipb1_cpp_if.h/ppcstdsip_cpp_if.h to reduce compiler errors
/@ V 1.12 : * Absmax calculation reworked
/@              This value is used for correlation processing quality (during inspection)
/@            See function AverageMaximumGet().
/@            ==> Done only for 1D Correlations
/@ V 1.13 : * Also mean calculation reworked
/@            Use minimum of mean on left/right side or upper/lower side.
/@              This value is used for teach quality processing.
/@            ==> Was needed to give 'unsymetric patterns' an lower quality.
/@            The unsmetric mean value only influence the result of qual3
/@            which is a quality for contrast.
/@            Renamed AverageMaximumGet() to AdaptMeanMaximumSymetry()
/@            ==> Done only for 1D Correlations
/@ V 1.14 : * Quality improvements from V 1.12/V 1.13 framed with
/@               #ifdef USE_QUAL_IMPOVEMENTS
/@            and commented out
/@               #define USE_QUAL_IMPOVEMENTS
/@            ==> for current Madag-Update, I don't want this new untested thigs
/@ V 1.15 : * Von libPPCstdsipV2 nach DefaultProject kopiert
/@            f�r Test ohne Einflu� auf andere Projekte
/@            and removed comment before
/@              #define USE_NEIGHBOUR_WEIGHTS 1
/@            ==> For Madag release I don' want new, untested things.
/@ V 1.16 : * Got problems with linker in DefaultProject application.
/@            * renamed file from cdf_qual2.c to cdf_qual2.cpp
/@            * made some condional compilung if this is a .cpp file
/@              to make this file compilable. See COMPILE_AS_CPP_FILE
/@          !! ==> rename back to .c if copy back this file to libPPCstdsipV2
/@ V 1.17 : * AdaptMeanMaximumSymetry() also process 2D correleation fields now.
/@ V 1.20 : * Nach Ende Arbeiten f�r CAP-Optimierungen diese Source wieder
/@            zur�ckkopiert von DefaultProject nach libPPCstdsipV2.
/@ V 1.21 : * New function cdf2_pqual2() for calculating the processing quality.
/@            The addinonal argument to cdf2_pqual() is the white reference factor.
/@            The white refecene factor is multiplied in quality the the
/@            inspection quality get's the same value for unchanged and changed
/@            white referece value (darken the image to the half --> save
/@            processing quality).
/@          * Allow teach quality test to have a distance to border of 1 (before was 2).
/@            cdf2_tqual2(), new function, copy of cdf2_tqual() with modifications
/@            cdf2_txyqual2(), same as cdf2_txyqual() only call cdf2_tqual2().
/@            cdf2_txqual2(), same as cdf2_txqual() only call cdf2_tqual2().
/@            cdf2_tyqual2(), same as cdf2_tyqual() only call cdf2_tqual2().
/@            ==> Rename the functions cdf2_tXXqual() used until now to cdf2_tXXqual()
/@                to use the new feature.
/@          
/@ *************************************************************************/

#define USE_QUAL_IMPOVEMENTS 1    // 23.03.2010 RR: define this to use the quality improments fromn AdaptMeanMaximumSymetry()

#include <windows.h>
#include <math.h>

#include "YaIPS_IPS_Interface.h" // 15.05.2025 RR: Need this for IPS defines
#include "YaIPS_LanguageStrings.h"  // Language string definitions

/*======================IP ROUTINE ========================================*/

/* default weights used */
static sfloat wlin1  = (sfloat)1.0;
static sfloat wsqrt1 = (sfloat)0.0;
static sfloat wsq1   = (sfloat)0.0;
static sfloat wlin2  = (sfloat)1.0;
static sfloat wsqrt2 = (sfloat)0.0;
static sfloat wsq2   = (sfloat)0.0;
static sfloat wlin3  = (sfloat)1.0;
static sfloat wsqrt3 = (sfloat)0.0;
static sfloat wsq3   = (sfloat)0.0;
static sfloat wsum   = (sfloat)300.0;
static sfloat wq1compq2 = (sfloat)0.0;

#define QUAL_AREATYPE_XY    0   // Area is for XY search
#define QUAL_AREATYPE_X     1   // Area is for X search
#define QUAL_AREATYPE_Y     2   // Area is for Y search

/***************************************************************

  AdaptMeanMaximumSymetry()

  Get maxima values from left/right and top/bottom sides of
  centerpoint and average this values.

  NOTE: pAbsmax and pMean already points to the maximum value
        in the correlation field. Only overriede it, if you
        are shure that we have proper results.
*/

static int AdaptMeanMaximumSymetry( Timages *srcim, int x0, int y0, sfloat *pAbsmax, sfloat *pMean, int AreaType) /*
=========================================================================== */
{
  int xx, yy, x, y;
  sfloat *psf1;

  if (uticheck(srcim, DV_HOST, TY_SFLOAT)) return(-1);
  xx = (int)getxx(srcim);  
  yy = (int)getyy(srcim);
  if (x0 < 0 || x0 >= xx || y0 < 0 || y0 >= yy) return(-2);

  PRINTF4(" AdaptMeanMaximumSymetry() start search at %d %d, imsize %d x %d\n", x0, y0, xx, yy);

  switch( AreaType) {
  default:
  case QUAL_AREATYPE_XY:

    // to the right

    {

      sfloat MaxValLT, MaxValRT, MaxValLB, MaxValRB;
      sfloat MeanValLT, MeanValRT, MeanValLB, MeanValRB;
      int    nPixelLT,  nPixelRT, nPixelLB,  nPixelRB;

      psf1 = (sfloat *)pixadt( x0, y0, srcim, sfloat);

      MaxValLT  = *psf1;       // preset with center point
      MaxValRT  = *psf1;
      MaxValLB  = *psf1;
      MaxValRB  = *psf1;

      MeanValLT = 0.0;
      MeanValRT = 0.0;
      MeanValLB = 0.0;
      MeanValRB = 0.0;

      nPixelLT = 0;
      nPixelRT = 0;
      nPixelLB = 0;
      nPixelRB = 0;

      // top side

      for (y = 0; y <= y0; y++) {

        // left top corner

        psf1 = (sfloat *)pixadt( 0, y, srcim, sfloat);

        for (x = 0; x <= x0; x++, psf1++) {

          if (*psf1 > MaxValLT) {
            MaxValLT = *psf1;
          }
          MeanValLT += *psf1;
          nPixelLT  += 1;
        }

        // right top corner

        psf1 = (sfloat *)pixadt( x0, y, srcim, sfloat);

        for (x = x0; x < xx; x++, psf1++) {

          if (*psf1 > MaxValRT) {
            MaxValRT = *psf1;
          }
          MeanValRT += *psf1;
          nPixelRT  += 1;
        }
      }

      // bottom side

      for (y = y0; y < yy; y++) {

        // left bottom corner

        psf1 = (sfloat *)pixadt( 0, y, srcim, sfloat);

        for (x = 0; x <= x0; x++, psf1++) {

          if (*psf1 > MaxValLB) {
            MaxValLB = *psf1;
          }
          MeanValLB += *psf1;
          nPixelLB  += 1;
        }

        // right bottom corner

        psf1 = (sfloat *)pixadt( x0, y, srcim, sfloat);

        for (x = x0; x < xx; x++, psf1++) {

          if (*psf1 > MaxValRB) {
            MaxValRB = *psf1;
          }
          MeanValRB += *psf1;
          nPixelRB  += 1;
        }
      }

      if( nPixelLT > 0 && nPixelRT > 0 && nPixelLB > 0 && nPixelRB > 0) {  // Have maxima/mean from all corners

        *pAbsmax = (MaxValLT + MaxValRT + MaxValLB + MaxValRB) * 0.25f;   // Average maxima

        MeanValLT = MeanValLT / nPixelLT;    // average
        MeanValRT = MeanValRT / nPixelRT;    // average
        MeanValLB = MeanValLB / nPixelLB;    // average
        MeanValRB = MeanValRB / nPixelRB;    // average

        // store lower mean

        *pMean = MeanValLT;

        if( *pMean > MeanValRT) {
          *pMean = MeanValRT;
        }
        if( *pMean > MeanValLB) {
          *pMean = MeanValLB;
        }
        if( *pMean > MeanValRB) {
          *pMean = MeanValRB;
        }
      }

      PRINTF5(" AdaptMeanMaximumSymetry() New maxim value %f from LT %f RT %f LB %f RB %f\n", *pAbsmax,  MaxValLT,  MaxValRT,  MaxValLB,  MaxValRB);
      PRINTF5(" AdaptMeanMaximumSymetry() New  mean value %f from LT %f RT %f LB %f RB %f\n",   *pMean, MeanValLT, MeanValRT, MeanValLB, MeanValRB);

    }

    break;

  case QUAL_AREATYPE_X:

    // to the right

    {

      sfloat MaxValL, MaxValR;
      sfloat MeanValL, MeanValR;
      int    nPixelL,  nPixelR;

      psf1 = (sfloat *)pixadt( x0, y0, srcim, sfloat);

      MaxValR  = *psf1;
      MeanValR = *psf1;     // preset with center point
      nPixelR  = 0;

      for (x = x0 + 1; x < xx; x++) {
        psf1++;
        if (*psf1 > MaxValR) {
          MaxValR = *psf1;
        }
        MeanValR += *psf1;
        nPixelR  += 1;
      }

      // to the left

      psf1 = (sfloat *)pixadt( x0, y0, srcim, sfloat);

      MaxValL  = *psf1;
      MeanValL = *psf1;     // preset with center point
      nPixelL  = 0;

      for (x = 0; x < x0; x++) {
        --psf1;
        if (*psf1 > MaxValL) {
          MaxValL = *psf1;
        }
        MeanValL += *psf1;
        nPixelL  += 1;
      }

      if( nPixelR > 0 && nPixelL > 0) {          // Have maxima/mean from both sided

        *pAbsmax = (MaxValR + MaxValL) * 0.5f;   // Average maxima

        MeanValR = MeanValR / ( nPixelR + 1);    // average
        MeanValL = MeanValL / ( nPixelL + 1);    // average

        // store lower mean

        if( MeanValR < MeanValL) {
          *pMean = MeanValR;
        } else {
          *pMean = MeanValL;
        }
      }

      PRINTF3(" AdaptMeanMaximumSymetry() New maxim value %f from Left %f and Right %f\n", *pAbsmax,  MaxValL,  MaxValR);
      PRINTF3(" AdaptMeanMaximumSymetry() New  mean value %f from Left %f and Right %f\n",   *pMean, MeanValL, MeanValR);

    }
    break;

  case QUAL_AREATYPE_Y:

    // to the bottom

    {

      sfloat MaxValT,  MaxValB;
      sfloat MeanValT, MeanValB;
      int    nPixelT,  nPixelB;

      psf1 = (sfloat *)pixadt( x0, y0, srcim, sfloat);

      MaxValB  = *psf1;
      MeanValB = *psf1;     // preset with center point
      nPixelB  = 0;

      for (y = y0 + 1; y < yy; y++) {
        psf1 = (sfloat *)pixadt(x0, y, srcim, sfloat);
        if (*psf1 > MaxValB) {
          MaxValB = *psf1;
        }
        MeanValB += *psf1;
        nPixelB  += 1;
      }

      // to the top

      psf1 = (sfloat *)pixadt( x0, y0, srcim, sfloat);

      MaxValT  = *psf1;
      MeanValT = *psf1;     // preset with center point
      nPixelT  = 0;

      for (y = 0; y < y0; y++) {
        psf1 = (sfloat *)pixadt(x0, y, srcim, sfloat);
        if (*psf1 > MaxValT) {
          MaxValT = *psf1;
        }
        MeanValT += *psf1;
        nPixelT  += 1;
      }

      if( nPixelB > 0 && nPixelT > 0) {          // Have maxima/mean from both sided

        *pAbsmax = (MaxValB + MaxValT) * 0.5f;   // Average maxima

        MeanValB = MeanValB / ( nPixelB + 1);    // average
        MeanValT = MeanValT / ( nPixelT + 1);    // average

        // store lower mean

        if( MeanValB < MeanValT) {
          *pMean = MeanValB;
        } else {
          *pMean = MeanValT;
        }
      }

      PRINTF3(" AdaptMeanMaximumSymetry() New maxim value %f from Top %f and Bottom %f\n", *pAbsmax, MaxValT, MaxValB);
      PRINTF3(" AdaptMeanMaximumSymetry() New  mean value %f from Top %f and Bottom %f\n",   *pMean, MeanValT, MeanValB);

    }

    break;
  }

//exitPoint:

  return( 0);

} /* AdaptMeanMaximumSymetry() */

/***************************************************************
  findBiggestNeighbour()
*/

static int findBiggestNeighbour(Timages *srcim, int x0, int y0, int *retX, int *retY, sfloat *retVal, int AreaType) /*
============================================================================ */
{
  int ierr;
  int xx, yy, x, y, i, xTest, yTest;
  Timages *imTmp = IMNULL;
  sfloat *psf0, *psf1, absmax, absmin;
  int xmin, ymin;
  int level, len, pixelChecked, stillInPeak;

  *retX = -1;
  *retY = -1;

  if (uticheck(srcim, DV_HOST, TY_SFLOAT)) return(-1);
  xx = (int)getxx(srcim);  
  yy = (int)getyy(srcim);
  if (x0 < 0 || x0 >= xx || y0 < 0 || y0 >= yy) return(-2);

  PRINTF4("findBiggestNeighbour() start search at %d %d, imsize %d x %d\n", x0, y0, xx, yy);

  imTmp = im_ucreateMem( NULL, TY_SFLOAT, DV_HOST, (int16)xx, (int16)yy, NULL);
  if (imTmp == IMNULL) {
    ierr = -3;
    goto exitPoint;
  }

  /* copy srcim to imTmp */
  for (y = 0; y < yy; y++) {
    psf0 = (sfloat *)pixadt(0, y, srcim, sfloat);
    psf1 = (sfloat *)pixadt(0, y, imTmp, sfloat);
    memcpy((void *)psf1, (void *)psf0, sizeof(sfloat) * xx);
  }

  absmax = (sfloat)0.0;
  for (y = 0; y < yy; y++) {
    psf1 = (sfloat *)pixadt(0, y, imTmp, sfloat);
    for (x = 0; x < xx; x++) {
      if (*psf1 > absmax) absmax = *psf1;
      psf1++;
    }
  }

  psf1 = (sfloat *)pixadt(x0, y0, imTmp, sfloat);
  if (*psf1 < (sfloat)0.0001) *psf1 = (sfloat)0.0001;
  *psf1 = - *psf1;                     /* level 0 */

  for (level = 1; ; level++) {
    /* start point */
    x = x0 - level;
    y = y0 - level;
    len = 2 * level + 1;
    pixelChecked = 0;
    stillInPeak = FALSE;

    /* to the right, do not the last pixel (is done to the bottom) */
    for (i = 0; i < len - 1; i++, x++) {
      if (x < 0 || x >= xx || y < 0 || y >= yy) continue; /* pixel outside */

      pixelChecked += 1;

      /* special corner: look the diagonal pixel */
      if (i == 0) {
        xTest = x + 1;
      } else {
        xTest = x;
      }
      yTest = y + 1;
   
      psf1 = (sfloat *)pixadt(xTest, yTest, imTmp, sfloat);
      if (*psf1 < (sfloat)0) {
        /* test pixel was tested in level before */
        psf0 = (sfloat *)pixadt(x, y, imTmp, sfloat);
        if (level == 1 || *psf0 > (- *psf1)) {
          /* we are still on the peak edge */
          if (*psf0 < (sfloat)0.0001) *psf0 = (sfloat)0.0001;
          *psf0 = - *psf0;
          stillInPeak = TRUE;
        } else {
          /* end reached, leave pixel unchanged */
          /* *psf0 = absmax; */
        }
      }

    }
    /* x--; */

    /* to the bottom, do not the last pixel (is done to the left)  */
    for (i = 0; i < len - 1; i++, y++) {
      if (x < 0 || x >= xx || y < 0 || y >= yy) continue; /* pixel outside */

      pixelChecked += 1;

      /* special corner: look the diagonal pixel */
      if (i == 0) {
        yTest = y + 1;
      } else {
        yTest = y;
      }
      xTest = x - 1;
   
      psf1 = (sfloat *)pixadt(xTest, yTest, imTmp, sfloat);
      if (*psf1 < (sfloat)0) {
        /* test pixel was tested in level before */
        psf0 = (sfloat *)pixadt(x, y, imTmp, sfloat);
        if (level == 1 || *psf0 > (- *psf1)) {
          /* we are still on the peak edge */
          if (*psf0 < (sfloat)0.0001) *psf0 = (sfloat)0.0001;
          *psf0 = - *psf0;
          stillInPeak = TRUE;
        } else {
          /* end reached, leave pixel unchanged */
          /* *psf0 = absmax; */
        }
      }

    }
    /* y--; */

    /* to the left, do not the last pixel (is done to the top)  */
    for (i = 0; i < len - 1; i++, x--) {
      if (x < 0 || x >= xx || y < 0 || y >= yy) continue; /* pixel outside */

      pixelChecked += 1;

      /* special corner: look the diagonal pixel */
      if (i == 0) {
        xTest = x - 1;
      } else {
        xTest = x;
      }
      yTest = y - 1;
   
      psf1 = (sfloat *)pixadt(xTest, yTest, imTmp, sfloat);
      if (*psf1 < (sfloat)0) {
        /* test pixel was tested in level before */
        psf0 = (sfloat *)pixadt(x, y, imTmp, sfloat);
        if (level == 1 || *psf0 > (- *psf1)) {
          /* we are still on the peak edge */
          if (*psf0 < (sfloat)0.0001) *psf0 = (sfloat)0.0001;
          *psf0 = - *psf0;
          stillInPeak = TRUE;
        } else {
          /* end reached, leave pixel unchanged */
          /* *psf0 = absmax; */
        }
      }

    }
    /* x++; */

    /* to the top, do not the last pixel (is done to the right)  */
    for (i = 0; i < len - 1; i++, y--) {
      if (x < 0 || x >= xx || y < 0 || y >= yy) continue; /* pixel outside */

      pixelChecked += 1;

      /* special corner: look the diagonal pixel */
      if (i == 0) {
        yTest = y - 1;
      } else {
        yTest = y;
      }
      xTest = x + 1;
   
      psf1 = (sfloat *)pixadt(xTest, yTest, imTmp, sfloat);
      if (*psf1 < (sfloat)0) {
        /* test pixel was tested in level before */
        psf0 = (sfloat *)pixadt(x, y, imTmp, sfloat);
        if (level == 1 || *psf0 > (- *psf1)) {
          /* we are still on the peak edge */
          if (*psf0 < (sfloat)0.0001) *psf0 = (sfloat)0.0001;
          *psf0 = - *psf0;
          stillInPeak = TRUE;
        } else {
          /* end reached, leave pixel unchanged */
          /* *psf0 = absmax; */
        }
      }

    }
    /* y++; */

    if (pixelChecked == 0) break;
    if (stillInPeak == FALSE) break;

  }

  for (y = 0; y < yy; y++) {
    psf1 = (sfloat *)pixadt(0, y, imTmp, sfloat);
    for (x = 0; x < xx; x++) {
      if (*psf1 < (sfloat)0.0) *psf1 = absmax;
      psf1++;
    }
  }

  /* now search the minimum */

  xmin = ymin = -1;
  absmin = absmax;

  switch( AreaType) {
  default:
  case QUAL_AREATYPE_XY:
    for (y = 0; y < yy; y++) {
      psf1 = (sfloat *)pixadt(0, y, imTmp, sfloat);
      for (x = 0; x < xx; x++) {
        if (*psf1 < absmin) {
          absmin = *psf1;
          xmin = x;
          ymin = y;
        }
        psf1++;
      }
    }
    break;
  case QUAL_AREATYPE_X:
    psf1 = (sfloat *)pixadt(0, y0, imTmp, sfloat);
    for (x = 0; x < xx; x++) {
      if (*psf1 < absmin) {
        absmin = *psf1;
        xmin = x;
        ymin = y;
      }
      psf1++;
    }
    break;
  case QUAL_AREATYPE_Y:
    for (y = 0; y < yy; y++) {
      psf1 = (sfloat *)pixadt(x0, y, imTmp, sfloat);
      if (*psf1 < absmin) {
        absmin = *psf1;
        xmin = x;
        ymin = y;
      }
    }
    break;
  }

  *retX = xmin;
  *retY = ymin;

  if (xmin < 0 || ymin < 0) {
    ierr = 1;
  } else {
    ierr = 0;
    PRINTF2("findBiggestNeighbour: retVal %f, absmax %f\n", absmin, absmax);
    *retVal = absmin;
  }

exitPoint:

  if (imTmp != IMNULL) im_remove(imTmp);

  return(ierr);

} /* findBiggestNeighbour() */

/***************************************************************
  cdf2_tqual()
*/

static int cdf2_tqual( Timages *srcim, int16 x0, int16 y0, int32 norma, sfloat *qual1, sfloat *qual2, sfloat *qual3, int16 mode, int16 AreaType) /* 
================================================= compute teach qualities */
{
  int ierr;
  int x2, y2;
  register int x, y, xx, yy, xm;
  register sfloat absmin, min55;
  sfloat minn55, mean, meanUnsymetric, absmax;
  register sfloat *ps;  
  
  if (uticheck(srcim, DV_HOST, TY_SFLOAT)) return(-1);
  xx = (int)getxx(srcim);  
  yy = (int)getyy(srcim);  
  xm = getxm(srcim);

  if (yy < 5) {
    if ((x0 < 2) || (x0 >= (xx - 2))) {
      errstring = (char *)"bad pattern";
      return(-50);
    }
  } else if (xx < 5) {
    if ((y0 < 2) || (y0 >= (yy - 2))) {
      errstring = (char *)"bad pattern";
      return(-50);
    }
  } else {
    if ((x0 < 2) || (y0 < 2) || (x0 >= (xx - 2)) || (y0 >= (yy - 2))) {
      errstring = (char *)"bad pattern";
      return(-50);
    }
  }

  if (norma < 1) {
    errstring = (char *)"norma too small";
    return(-3);
  }

  /* search absolute minimum and maximum in correlation image */
  absmin = *(pixadt(0, 0, srcim, sfloat));              /* init minimum */
  absmax = absmin;
  mean = 0.0;
  for (y = 0; y < yy; y++) {
    if (iabort()) break;                                /* check CTRL C  */
    ps = pixadt(0, y, srcim, sfloat);
    x = xx;
    while (--x >= 0) {
      if (*ps < absmin) absmin = *ps;
      if (*ps > absmax) absmax = *ps;
      mean += *ps;
      ps++;
    }
  }
  mean /= (sfloat)(xx * yy);

  /* 18.03.2010 RR: Average Maximas on each side */

  meanUnsymetric = mean;    // preset with mean value

#ifdef USE_QUAL_IMPOVEMENTS   // 23.03.2010 RR: use the quality improments ?
  ierr = AdaptMeanMaximumSymetry( srcim, x0, y0, &absmax, &meanUnsymetric, AreaType);
#endif

  /* ... */

  if (!mode) { /* compute only absmin and absmax */
    /* normalization on object area */
    absmin /= (sfloat)norma;
    absmax /= (sfloat)norma;
    return((int)fto16(absmax - absmin));
  }
 
  /* search minimum of 5x5 neighbourhood around 3x3 kernel */
  min55 = absmax;
  if (yy < 5) {
    ps = pixadt(x0 - 2, 0, srcim, sfloat);
    for (y = 0; y < yy; y++, ps += xm) if (*ps < min55) min55 = *ps; 
    ps = pixadt(x0 + 2, 0, srcim, sfloat);
    for (y = 0; y < yy; y++, ps += xm) if (*ps < min55) min55 = *ps; 
  } else if (xx < 5) {
    ps = pixadt(0, y0 - 2, srcim, sfloat);
    for (x = 0; x < xx; x++, ps++) if (*ps < min55) min55 = *ps; 
    ps = pixadt(0, y0 + 2, srcim, sfloat);
    for (x = 0; x < xx; x++, ps++) if (*ps < min55) min55 = *ps; 
  } else {
    ps = pixadt(x0 - 2, y0 - 2, srcim, sfloat);
    for (x = 0; x < 5; x++, ps++) if (*ps < min55) min55 = *ps; 
    ps = pixadt(x0 - 2, y0 + 2, srcim, sfloat);
    for (x = 0; x < 5; x++, ps++) if (*ps < min55) min55 = *ps; 
    ps = pixadt(x0 - 2, y0 - 1, srcim, sfloat);
    for (y = 1; y < 4; y++, ps += xm) if (*ps < min55) min55 = *ps; 
    ps = pixadt(x0 + 2, y0 - 1, srcim, sfloat);
    for (y = 1; y < 4; y++, ps += xm) if (*ps < min55) min55 = *ps; 
  }

  /* search minimum outside 5x5 field */
  ierr = findBiggestNeighbour(srcim, x0, y0, &x2, &y2, &minn55, AreaType);

  PRINTF4("findBiggestNeighbour returns ierr %d, x %d y %d val %f\n",
           ierr, x2, y2, minn55);

  if (ierr < 0) return(ierr);

  if (ierr > 0) {              /* no neighbour found */
    minn55 = absmax;
  }

  /* normalization on object area */
  absmin /= (sfloat)norma;
  absmax /= (sfloat)norma;
  min55  /= (sfloat)norma;
  minn55 /= (sfloat)norma;
  mean   /= (sfloat)norma;
  meanUnsymetric /= (sfloat)norma;

  PRINTF5("absmin %f absmax %f min55 %f minn55 %f mean %d\n", absmin, absmax,
    min55, minn55, mean);

  /* pattern distinctiveness (contrast) */
  /* qual3 = (absmax - absmin)/128.; if (*qual3 > 1.0) *qual3 = 1.0; */
  /* enhance contrast quality */
  *qual3 = (sfloat)((meanUnsymetric - absmin)/64.);
  if (*qual3 < 0.2) {
    *qual3 = *qual3 * *qual3 * (sfloat)5.0;
  }
  /* *qual3 *= 2.0; */
  if (*qual3 > 1.0) *qual3 = 1.0;
  if (*qual3 <= 0.) {
    errstring = (char *)"bad pattern contrast";
    return(-51);
  }

  if (mean - absmin <= (sfloat)0.001) {
    *qual1 = 0.0;
  } else {
    *qual1 = (sfloat)(1. - (mean - min55)/(mean - absmin));
    if (*qual1 < 0.1) {
      *qual1 = *qual1 * *qual1 * (sfloat)10.0;
    }
    *qual1 *= (sfloat)2.0;
    if (*qual1 > (sfloat)1.0) *qual1 = (sfloat)1.0;
  }

  if (mean - absmin <= (sfloat)0.001) {
    *qual2 = 0.0;
  } else {
    *qual2 = (minn55 - absmin) / (mean - absmin);
    if (*qual2 < 0.25) {
      *qual2 = (sfloat)(*qual2 * *qual2 * *qual2 * *qual2 * 32.0);
    } else if (*qual2 < 0.5) {
      *qual2 = *qual2 * *qual2 * (sfloat)2.0;
    }
    *qual2 *= 2.0;
    if (*qual2 > 1.0) *qual2 = 1.0;
  }

  // weight
  *qual3 = wlin3 * *qual3 + wsqrt3 * (sfloat)sqrt((double)*qual3) + wsq3 * *qual3 * *qual3;
  if (*qual3 > 1.0) *qual3 = 1.0;
  if (*qual3 < 0.0) *qual3 = 0.0;

  *qual1 = wlin1 * *qual1 + wsqrt1 * (sfloat)sqrt((double)*qual1) + wsq1 * *qual1 * *qual1;
  if (*qual1 > 1.0) *qual1 = 1.0;
  if (*qual1 < 0.0) *qual1 = 0.0;

  *qual2 = wlin2 * *qual2 + wsqrt2 * (sfloat)sqrt((double)*qual2) + wsq2 * *qual2 * *qual2;
  if (*qual1 > 0.00001) {
    *qual2 = (sfloat)((1.0 - wq1compq2) + (1.0 / *qual1) * wq1compq2) * *qual2;
  }
  if (*qual2 > 1.0) *qual2 = 1.0;
  if (*qual2 < 0.0) *qual2 = 0.0;

  PRINTF4("qual1 %f qual2 %f qual3 %f (%f)\n", *qual1, *qual2, *qual3,
           (absmax - absmin)/128.);

  return((int)fto16(absmax - absmin));

} /* static int cdf2_tqual() */

/***************************************************************
  cdf2_tqual2()
*/

static int cdf2_tqual2( Timages *srcim, int16 x0, int16 y0, int32 norma, sfloat *qual1, sfloat *qual2, sfloat *qual3, int16 mode, int16 AreaType) /* 
================================================= compute teach qualities */
{
  int ierr;
  int x2, y2;
  register int x, y, xx, yy, xm;
  register sfloat absmin, min55;
  sfloat minn55, mean, meanUnsymetric, absmax;
  register sfloat *ps;  
  
  if (uticheck(srcim, DV_HOST, TY_SFLOAT)) return(-1);
  xx = (int)getxx(srcim);  
  yy = (int)getyy(srcim);  
  xm = getxm(srcim);

#ifdef use_again
  if (yy < 5) {
    if ((x0 < 2) || (x0 >= (xx - 2))) {
      errstring = (char *)"bad pattern";
      return(-50);
    }
  } else if (xx < 5) {
    if ((y0 < 2) || (y0 >= (yy - 2))) {
      errstring = (char *)"bad pattern";
      return(-50);
    }
  } else {
    if ((x0 < 2) || (y0 < 2) || (x0 >= (xx - 2)) || (y0 >= (yy - 2))) {
      errstring = (char *)"bad pattern";
      return(-50);
    }
  }
#else
  // 05.07.2011 RR: Changed tolerance to border from 2 to 1 pixel
  if (yy < 5) {
    if ((x0 < 1) || (x0 >= (xx - 1))) {
      errstring = (char *)"bad pattern";
      return(-50);
    }
  } else if (xx < 5) {
    if ((y0 < 1) || (y0 >= (yy - 1))) {
      errstring = (char *)"bad pattern";
      return(-50);
    }
  } else {
    if ((x0 < 1) || (y0 < 1) || (x0 >= (xx - 1)) || (y0 >= (yy - 1))) {
      errstring = (char *)"bad pattern";
      return(-50);
    }
  }
#endif

  if (norma < 1) {
    errstring = (char *)(char *)"norma too small";
    return(-3);
  }

  /* search absolute minimum and maximum in correlation image */
  absmin = *(pixadt(0, 0, srcim, sfloat));              /* init minimum */
  absmax = absmin;
  mean = 0.0;
  for (y = 0; y < yy; y++) {
    if (iabort()) break;                                /* check CTRL C  */
    ps = pixadt(0, y, srcim, sfloat);
    x = xx;
    while (--x >= 0) {
      if (*ps < absmin) absmin = *ps;
      if (*ps > absmax) absmax = *ps;
      mean += *ps;
      ps++;
    }
  }
  mean /= (sfloat)(xx * yy);

  /* 18.03.2010 RR: Average Maximas on each side */

  meanUnsymetric = mean;    // preset with mean value

#ifdef USE_QUAL_IMPOVEMENTS   // 23.03.2010 RR: use the quality improments ?
  ierr = AdaptMeanMaximumSymetry( srcim, x0, y0, &absmax, &meanUnsymetric, AreaType);
#endif

  /* ... */

  if (!mode) { /* compute only absmin and absmax */
    /* normalization on object area */
    absmin /= (sfloat)norma;
    absmax /= (sfloat)norma;
    return((int)fto16(absmax - absmin));
  }
 
  /* search minimum of 5x5 neighbourhood around 3x3 kernel */
  min55 = absmax;
#ifdef use_again
  if (yy < 5) {
    ps = pixadt(x0 - 2, 0, srcim, sfloat);
    for (y = 0; y < yy; y++, ps += xm) if (*ps < min55) min55 = *ps; 
    ps = pixadt(x0 + 2, 0, srcim, sfloat);
    for (y = 0; y < yy; y++, ps += xm) if (*ps < min55) min55 = *ps; 
  } else if (xx < 5) {
    ps = pixadt(0, y0 - 2, srcim, sfloat);
    for (x = 0; x < xx; x++, ps++) if (*ps < min55) min55 = *ps; 
    ps = pixadt(0, y0 + 2, srcim, sfloat);
    for (x = 0; x < xx; x++, ps++) if (*ps < min55) min55 = *ps; 
  } else {
    ps = pixadt(x0 - 2, y0 - 2, srcim, sfloat);
    for (x = 0; x < 5; x++, ps++) if (*ps < min55) min55 = *ps; 
    ps = pixadt(x0 - 2, y0 + 2, srcim, sfloat);
    for (x = 0; x < 5; x++, ps++) if (*ps < min55) min55 = *ps; 
    ps = pixadt(x0 - 2, y0 - 1, srcim, sfloat);
    for (y = 1; y < 4; y++, ps += xm) if (*ps < min55) min55 = *ps; 
    ps = pixadt(x0 + 2, y0 - 1, srcim, sfloat);
    for (y = 1; y < 4; y++, ps += xm) if (*ps < min55) min55 = *ps; 
  }
#else
  // 05.07.2011 RR: Changed tolerance to border from 2 to 1 pixel
  // There is not many data to process so can keep the test for inside stupid.
  if (yy < 5) {
    ps = pixadt(x0 - 2, 0, srcim, sfloat);
    for (y = 0; y < yy; y++, ps += xm) {
      if( x >= 0 && x < xx && y >= 0 && y < yy) {  // inside image?
        if (*ps < min55) min55 = *ps; 
      }
    }
    ps = pixadt(x0 + 2, 0, srcim, sfloat);
    for (y = 0; y < yy; y++, ps += xm) {
      if( x >= 0 && x < xx && y >= 0 && y < yy) {  // inside image?
        if (*ps < min55) min55 = *ps; 
      }
    }
  } else if (xx < 5) {
    ps = pixadt(0, y0 - 2, srcim, sfloat);
    for (x = 0; x < xx; x++, ps++) {
      if( x >= 0 && x < xx && y >= 0 && y < yy) {  // inside image?
        if (*ps < min55) min55 = *ps; 
      }
    }
    ps = pixadt(0, y0 + 2, srcim, sfloat);
    for (x = 0; x < xx; x++, ps++) {
      if( x >= 0 && x < xx && y >= 0 && y < yy) {  // inside image?
        if (*ps < min55) min55 = *ps; 
      }
    }
  } else {
    ps = pixadt(x0 - 2, y0 - 2, srcim, sfloat);
    for (x = 0; x < 5; x++, ps++) {
      if( x >= 0 && x < xx && y >= 0 && y < yy) {  // inside image?
        if (*ps < min55) min55 = *ps; 
      }
    }
    ps = pixadt(x0 - 2, y0 + 2, srcim, sfloat);
    for (x = 0; x < 5; x++, ps++) {
      if( x >= 0 && x < xx && y >= 0 && y < yy) {  // inside image?
        if (*ps < min55) min55 = *ps; 
      }
    }
    ps = pixadt(x0 - 2, y0 - 1, srcim, sfloat);
    for (y = 1; y < 4; y++, ps += xm) {
      if( x >= 0 && x < xx && y >= 0 && y < yy) {  // inside image?
        if (*ps < min55) min55 = *ps; 
      }
    }
    ps = pixadt(x0 + 2, y0 - 1, srcim, sfloat);
    for (y = 1; y < 4; y++, ps += xm) {
      if( x >= 0 && x < xx && y >= 0 && y < yy) {  // inside image?
        if (*ps < min55) min55 = *ps; 
      }
    }
  }
#endif

  /* search minimum outside 5x5 field */
  ierr = findBiggestNeighbour(srcim, x0, y0, &x2, &y2, &minn55, AreaType);

  PRINTF4("findBiggestNeighbour returns ierr %d, x %d y %d val %f\n",
           ierr, x2, y2, minn55);

  if (ierr < 0) return(ierr);

  if (ierr > 0) {              /* no neighbour found */
    minn55 = absmax;
  }

  /* normalization on object area */
  absmin /= (sfloat)norma;
  absmax /= (sfloat)norma;
  min55  /= (sfloat)norma;
  minn55 /= (sfloat)norma;
  mean   /= (sfloat)norma;
  meanUnsymetric /= (sfloat)norma;

  PRINTF5("absmin %f absmax %f min55 %f minn55 %f mean %d\n", absmin, absmax,
    min55, minn55, mean);

  /* pattern distinctiveness (contrast) */
  /* qual3 = (absmax - absmin)/128.; if (*qual3 > 1.0) *qual3 = 1.0; */
  /* enhance contrast quality */
  *qual3 = (sfloat)((meanUnsymetric - absmin)/64.);
  if (*qual3 < 0.2) {
    *qual3 = *qual3 * *qual3 * (sfloat)5.0;
  }
  /* *qual3 *= 2.0; */
  if (*qual3 > 1.0) *qual3 = 1.0;
  if (*qual3 <= 0.) {
    errstring = (char *)"bad pattern contrast";
    return(-51);
  }

  if (mean - absmin <= (sfloat)0.001) {
    *qual1 = 0.0;
  } else {
    *qual1 = (sfloat)(1. - (mean - min55)/(mean - absmin));
    if (*qual1 < 0.1) {
      *qual1 = *qual1 * *qual1 * (sfloat)10.0;
    }
    *qual1 *= (sfloat)2.0;
    if (*qual1 > (sfloat)1.0) *qual1 = (sfloat)1.0;
  }

  if (mean - absmin <= (sfloat)0.001) {
    *qual2 = 0.0;
  } else {
    *qual2 = (minn55 - absmin) / (mean - absmin);
    if (*qual2 < 0.25) {
      *qual2 = (sfloat)(*qual2 * *qual2 * *qual2 * *qual2 * 32.0);
    } else if (*qual2 < 0.5) {
      *qual2 = *qual2 * *qual2 * (sfloat)2.0;
    }
    *qual2 *= 2.0;
    if (*qual2 > 1.0) *qual2 = 1.0;
  }

  // weight
  *qual3 = wlin3 * *qual3 + wsqrt3 * (sfloat)sqrt((double)*qual3) + wsq3 * *qual3 * *qual3;
  if (*qual3 > 1.0) *qual3 = 1.0;
  if (*qual3 < 0.0) *qual3 = 0.0;

  *qual1 = wlin1 * *qual1 + wsqrt1 * (sfloat)sqrt((double)*qual1) + wsq1 * *qual1 * *qual1;
  if (*qual1 > 1.0) *qual1 = 1.0;
  if (*qual1 < 0.0) *qual1 = 0.0;

  *qual2 = wlin2 * *qual2 + wsqrt2 * (sfloat)sqrt((double)*qual2) + wsq2 * *qual2 * *qual2;
  if (*qual1 > 0.00001) {
    *qual2 = (sfloat)((1.0 - wq1compq2) + (1.0 / *qual1) * wq1compq2) * *qual2;
  }
  if (*qual2 > 1.0) *qual2 = 1.0;
  if (*qual2 < 0.0) *qual2 = 0.0;

  PRINTF4("qual1 %f qual2 %f qual3 %f (%f)\n", *qual1, *qual2, *qual3,
           (absmax - absmin)/128.);

  return((int)fto16(absmax - absmin));

} /* static int cdf2_tqual2() */

static int cdf2_test_tqual(int16 *retqual, int16 thres, sfloat qual1, sfloat qual2, sfloat qual3) /* test thres
============================================================== */
{
  register int minqual;

  *retqual = fto16(qual1 * qual2 * qual3 * wsum);

  PRINTF3("TEST: qual1 %f qual2 %f qual3 %f\n", qual1, qual2, qual3);

  if (*retqual < thres) {
    minqual = 1;
    if (qual2 < qual1 && qual2 < qual3) minqual = 2; 
    if (qual3 < qual2 && qual3 < qual1) minqual = 3; 
    switch(minqual) {
      case 1:
        errstring = (char *)"pattern not usable";
        return(-53);
      case 2:
        errstring = (char *)"pattern not unique";
        return(-52);
      case 3:
        errstring = (char *)"bad pattern contrast";
        return(-51);
    }
  }

  return(0);

} /* static int cdf2_test_tqual() */

int cdf2_testSetup(double w1lin, double w1sqrt, double w1sq, double w2lin, double w2sqrt, double w2sq, double w3lin, double w3sqrt, double w3sq, double wtotal, double wcomp12) /* setup test weights
================================================================================================= */
{
  wlin1  = (sfloat)w1lin;
  wsqrt1 = (sfloat)w1sqrt;
  wsq1   = (sfloat)w1sq;
  wlin2  = (sfloat)w2lin;
  wsqrt2 = (sfloat)w2sqrt;
  wsq2   = (sfloat)w2sq;
  wlin3  = (sfloat)w3lin;
  wsqrt3 = (sfloat)w3sqrt;
  wsq3   = (sfloat)w3sq;
  wsum   = (sfloat)wtotal;
  wq1compq2 = (sfloat)wcomp12;

  return(0);

} /* int cdf2_testSetup() */

int cdf2_txyqual( Timages *srcim, int16 x0, int16 y0,
                  int32 norma, int16 thres, int16 *retqual, int16 mode) /*
================================================================================================= */
{
  register int ierr, norm;
  sfloat qual1, qual2, qual3;

  *retqual = 0;

  if (getxx(srcim) < 7 || getyy(srcim) < 7) {
    errstring = (char *)"srcim dimension too small (min 7x7)";
    return(-10);
  }

  ierr = cdf2_tqual(srcim, x0, y0, norma, &qual1, &qual2, &qual3, mode, QUAL_AREATYPE_XY);
  if (ierr < 0) return(ierr);
  norm = ierr;

  if (!mode) return(norm);                                /* no quality test */ 

  if( (ierr = cdf2_test_tqual(retqual, thres, qual1, qual2, qual3)) != 0)
    return(ierr);

  return(norm);

} /* int cdf2_txyqual() */

int cdf2_txyqual2( Timages *srcim, int16 x0, int16 y0,
                  int32 norma, int16 thres, int16 *retqual, int16 mode) /*
================================================================================================= */
{
  register int ierr, norm;
  sfloat qual1, qual2, qual3;

  *retqual = 0;

  if (getxx(srcim) < 7 || getyy(srcim) < 7) {
    errstring = (char *)"srcim dimension too small (min 7x7)";
    return(-10);
  }

  ierr = cdf2_tqual2(srcim, x0, y0, norma, &qual1, &qual2, &qual3, mode, QUAL_AREATYPE_XY);
  if (ierr < 0) return(ierr);
  norm = ierr;

  if (!mode) return(norm);                                /* no quality test */ 

  if( (ierr = cdf2_test_tqual(retqual, thres, qual1, qual2, qual3)) != 0)
    return(ierr);

  return(norm);

} /* int cdf2_txyqual2() */

int cdf2_txqual( Timages *srcim, int16 x0, int16 y0,
                 int32 norma, int16 thres, int16 *retqual, int16 mode) /*
================================================================================================= */
{
  register int ierr, norm;
  sfloat qual1, qual2, qual3;

  *retqual = 0;

  if (getxx(srcim) < 7) {
    errstring = (char *)"srcim x-dimension too small (min 7)";
    return(-11);
  }
  if (getyy(srcim) < 1) {
    errstring = (char *)"srcim y-dimension too small (min 1)";
    return(-12);
  }

  ierr = cdf2_tqual(srcim, x0, y0, norma, &qual1, &qual2, &qual3, mode, QUAL_AREATYPE_X);
  if (ierr < 0) return(ierr);
  norm = ierr;

  if (!mode) return(norm);                                /* no quality test */ 

  /* qual1 = 0.5 - qual1; if (qual1 < 0.) qual1 = 0.; now compensated */

  if( (ierr = cdf2_test_tqual(retqual, thres, qual1, qual2, qual3)) != 0)
    return(ierr);

  return(norm);

} /* int cdf2_txqual() */

int cdf2_txqual2( Timages *srcim, int16 x0, int16 y0,
                  int32 norma, int16 thres, int16 *retqual, int16 mode) /*
================================================================================================= */
{
  register int ierr, norm;
  sfloat qual1, qual2, qual3;

  *retqual = 0;

  if (getxx(srcim) < 7) {
    errstring = (char *)"srcim x-dimension too small (min 7)";
    return(-11);
  }
  if (getyy(srcim) < 1) {
    errstring = (char *)"srcim y-dimension too small (min 1)";
    return(-12);
  }

  ierr = cdf2_tqual2(srcim, x0, y0, norma, &qual1, &qual2, &qual3, mode, QUAL_AREATYPE_X);
  if (ierr < 0) return(ierr);
  norm = ierr;

  if (!mode) return(norm);                                /* no quality test */ 

  /* qual1 = 0.5 - qual1; if (qual1 < 0.) qual1 = 0.; now compensated */

  if( (ierr = cdf2_test_tqual(retqual, thres, qual1, qual2, qual3)) != 0)
    return(ierr);

  return(norm);

} /* int cdf2_txqual2() */

int cdf2_tyqual( Timages *srcim, int16 x0, int16 y0,
                 int32 norma, int16 thres, int16 *retqual, int16 mode) /*
================================================================================================= */
{
  register int ierr, norm;
  sfloat qual1, qual2, qual3;

  *retqual = 0;

  if (getyy(srcim) < 7) {
    errstring = (char *)"srcim y-dimension too small (min 7)";
    return(-13);
  }
  if (getxx(srcim) < 1) {
    errstring = (char *)"srcim x-dimension too small (min 1)";
    return(-14);
  }

  ierr = cdf2_tqual(srcim, x0, y0, norma, &qual1, &qual2, &qual3, mode, QUAL_AREATYPE_Y);
  if (ierr < 0) return(ierr);
  norm = ierr;

  if (!mode) return(norm);                                /* no quality test */ 

  /* qual1 = 0.5 - qual1; if (qual1 < 0.) qual1 = 0.; now compensated */

  if( (ierr = cdf2_test_tqual(retqual, thres, qual1, qual2, qual3)) != 0)
    return(ierr);

  return(norm);

} /* int cdf2_tyqual() */

int cdf2_tyqual2( Timages *srcim, int16 x0, int16 y0,
                  int32 norma, int16 thres, int16 *retqual, int16 mode) /*
================================================================================================= */
{
  register int ierr, norm;
  sfloat qual1, qual2, qual3;

  *retqual = 0;

  if (getyy(srcim) < 7) {
    errstring = (char *)"srcim y-dimension too small (min 7)";
    return(-13);
  }
  if (getxx(srcim) < 1) {
    errstring = (char *)"srcim x-dimension too small (min 1)";
    return(-14);
  }

  ierr = cdf2_tqual2(srcim, x0, y0, norma, &qual1, &qual2, &qual3, mode, QUAL_AREATYPE_Y);
  if (ierr < 0) return(ierr);
  norm = ierr;

  if (!mode) return(norm);                                /* no quality test */ 

  /* qual1 = 0.5 - qual1; if (qual1 < 0.) qual1 = 0.; now compensated */

  if( (ierr = cdf2_test_tqual(retqual, thres, qual1, qual2, qual3)) != 0)
    return(ierr);

  return(norm);

} /* int cdf2_tyqual2() */

int cdf2_pqual( Timages *srcim, int16 thres, int16 norm, int32 norma) /*
================================================================================================= */
{
  register int x, y, xx, yy;
  register sfloat absmin, absmax, *ps, qualfact;
  register int16 qual;

  if (uticheck(srcim, DV_HOST, TY_SFLOAT)) return(-1);
  xx = (int)getxx(srcim);  
  yy = (int)getyy(srcim);  

  PRINTF2("cdf2_pqual: xx %d yy %d\n", xx, yy);

  /* search absolute minimum and maximum in correlation image */
  absmin = *pixadt(0, 0, srcim, sfloat);                 /* init minimum */
  absmax = absmin;
  for (y = 0; y < yy; y++) {
    if (iabort()) break;                                /* check CTRL C  */
    ps = pixadt(0, y, srcim, sfloat);
    x = xx;
    while (--x >= 0) {
      if (*ps < absmin) absmin = *ps;
      if (*ps > absmax) absmax = *ps;
      ps++;
    }
  }

  absmax /= (float)norma;
  absmin /= (float)norma;

  PRINTF4("absmax %f, absmin %f, norma %d, norm %d\n", absmax, absmin, 
          norma, norm);

  qualfact = (sfloat)((255. - absmin)/255.);
  if (qualfact > 1.) qualfact = 1.;
  if (qualfact < 0.) qualfact = 0.;
  qual = fto16(((100. * (absmax - absmin))/(sfloat)norm) * qualfact);

  PRINTF2("quality %d, factor %f\n", qual, qualfact);

  if (qual < thres) {
    errstring = (char *)"bad pattern contrast";
    return(-51);
  }

  return(qual);

} /* int cdf2_pqual() */

int cdf2_pqual2( Timages *srcim, int16 thres, int16 norm, int32 norma, double wr_inFactor) /*
================================================================================================= */
{
  register int x, y, xx, yy;
  register sfloat absmin, absmax, *ps, qualfact;
  register int16 qual;

  if (uticheck(srcim, DV_HOST, TY_SFLOAT)) return(-1);
  xx = (int)getxx(srcim);  
  yy = (int)getyy(srcim);  

  PRINTF2("cdf2_pqual: xx %d yy %d\n", xx, yy);

  /* search absolute minimum and maximum in correlation image */
  absmin = *pixadt(0, 0, srcim, sfloat);                 /* init minimum */
  absmax = absmin;
  for (y = 0; y < yy; y++) {
    if (iabort()) break;                                /* check CTRL C  */
    ps = pixadt(0, y, srcim, sfloat);
    x = xx;
    while (--x >= 0) {
      if (*ps < absmin) absmin = *ps;
      if (*ps > absmax) absmax = *ps;
      ps++;
    }
  }

  // correct with area

  absmax /= (float)norma;
  absmin /= (float)norma;

  // correct whith white refence factor

  absmax *= (float)wr_inFactor;
  absmin *= (float)wr_inFactor;

  // ...

  PRINTF5("absmax %f, absmin %f, norma %d, norm %d, WrFac %.2lf\n", absmax, absmin, 
          norma, norm, wr_inFactor);

  qualfact = (sfloat)((255. - absmin)/255.);
  if (qualfact > 1.) qualfact = 1.;
  if (qualfact < 0.) qualfact = 0.;
  qual = fto16(((100. * (absmax - absmin))/(sfloat)norm) * qualfact);

  PRINTF2("quality %d, factor %f\n", qual, qualfact);

  if (qual < thres) {
    errstring = (char *)"bad pattern contrast";
    return(-51);
  }

  return(qual);

} /* int cdf2_pqual2() */

/************************************* E O F ********************************/
