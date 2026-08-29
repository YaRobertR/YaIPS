/* ************************************************************************
/@
/@ Name: YaIPS_IPS_VecBithres.cpp
/@
/@ Short-title: bipolar threshold
/@
/@ ==========================================================================
/@
/@ INDEX
/@   # Mbithres     # bithres      # bipolar threshold
/@
/@ USER DESCRIPTION
/@.. bithres  histvec thresPerc minDist sv_thres sv_max1 sv_max2
/@
/@.     Bipolar threshold.
/@
/@      ALGORITHM
/@
/@      The histogram is gauss-filtered in an iteration loop up to
/@      50 times. The iteration loop is stopped, if two maximum peaks 
/@      with a minimum distance of 'minDist' are left in the smoothed
/@      histogram. If after 50 iterations still more than 2 maximum peaks 
/@      are left, the biggest two are taken.
/@      The index at 'thresPerc' between the two maximum peaks is returned
/@      as 'sv_thres'.
/@
/@      PARAMETERS
/@      histvec       source vector containing histogram
/@      thresPerc     threshold position between peaks, -1: minimum between
/@      minDist       minimum distance between peaks
/@      sv_thres      shell variable for return of threshold
/@      sv_max1       shell variable for return of index of maximum peak
/@      sv_max2       shell variable for return of index of second maximum peak
/@
/@      RESTRICTIONS
/@      Only the following combinations of data types are allowed:
/@        "histvec"
/@         int32
/@
/@      In place calculation is possible.
/@
/@      SEE ALSO 
/@
/@ FUNCTION DESCRIPTION
/@   #include "portab.h"
/@   #include "sip.h"
/@
/@   int bithres(histvec, thresPerc, minDist, retThres, retMax)
/@     Tvector *histvec;
/@     int16   thresPerc, minDist, *retThres, *retMax;
/@
/@   int bithres2(histvec, thresPerc, minDist, retThres, retMax1, retMax2)
/@     Tvector *histvec;
/@     int16   thresPerc, minDist, *retThres, *retMax1, *retMax2;
/@
/@   RETURN VALUES
/@   Negative in case of error.
/@   0: OK
/@   1: histogram is not bimodal
/@
/@ REFERENCES
/@
/@ ********************************************************************/

#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "YaIPS_IPS_Interface.h" // 15.05.2025 RR: Need this for IPS defines

/***************************************************************************
* bithres2
*
* Calculate bipolar threshold
*
****************************************************************************
*/

#define NUM_ITER 50

int bithres2( Tvector *histvec, int thresPerc, int minDist,
              int *retThres, int *retMax1, int *retMax2)
{
  int ierr;
  register int i, nitems;
  Tvector *vtmp1 = VENULL, *vtmp2 = VENULL, *vtmp16 = VENULL, *vtmp16last = VENULL;
  register int32 *p32;
  register int16 *p16;
  int16 filter[9];
  int32 best, second, vbest, vsecond, start, end, sumLo, sumHi;

  if(utvcheck(histvec,DV_HOST,TY_INT32)) return(-1);

  *retThres = 128;  // Initialize
  if( retMax1 != NULL) {
    *retMax1  = 128;
  }
  if( retMax2 != NULL) {
    *retMax2  = 128;
  }

  vtmp1 = ve_ucreate(DV_HOST);
  if (vtmp1 == VENULL) {
    ierr = -2;
    goto errorExit;
  }
  if( (ierr = ve_alloc(vtmp1, vgetnm(histvec) + 2, (int16)sizeof(int32), TY_INT32))) return(ierr);

  vtmp2 = ve_ucreate(DV_HOST);
  if (vtmp2 == VENULL) {
    ierr = -3;
    goto errorExit;
  }
  if( (ierr = ve_alloc(vtmp2, vgetnm(histvec) + 2, (int16)sizeof(int32), TY_INT32))) return(ierr);

  vtmp16 = ve_ucreate(DV_HOST);
  if (vtmp16 == VENULL) {
    ierr = -4;
    goto errorExit;
  }
  vtmp16last = ve_ucreate(DV_HOST);
  if (vtmp16last == VENULL) {
    ierr = -5;
    goto errorExit;
  }

  nitems = vgetnm(histvec);

  // copy histogram to vtmp1, make vtmp1 larger at start and end
  p32 = (int32 *)vgetpm(vtmp1);
  p32[0] = 0;
  vputnm(vtmp1, 1);
  if( (ierr = vcopy( histvec, vtmp1, 0, -1, 1))) {  // append
    goto errorExit;
  }
  p32 = (int32 *)vgetpm(vtmp1);
  p32[nitems + 1] = 0;
  vputnm(vtmp1, nitems + 2);

  // Gauss-lowpass 9: 2 4 8 11 14 11 8 4 2               * 1/64
  filter[0] = filter[8] = 2;
  filter[1] = filter[7] = 4;
  filter[2] = filter[6] = 8;
  filter[3] = filter[5] = 11;
  filter[4] = 14;

  for (i = 0; i < NUM_ITER; i++) {

    // do the gauss-Lowpass
    if( (ierr = vfiltn( vtmp1, vtmp2, -6, 0, 9, filter))) {
      goto errorExit;
    }
    // copy back
    if( (ierr = vcopy( vtmp2, vtmp1, 0, -1, 0))) {
      goto errorExit;
    }

    // vminmax doesn't take peaks at the border, be sure first and last value are smaller
    // and nitems - 1 values 
    p32 = (int32 *)vgetpm(vtmp1);
    if (p32[0] >= p32[1] && p32[0] > 0) p32[0] = p32[1] - 1;
    if (p32[nitems + 1] >= p32[nitems] && p32[nitems + 1] > 0) p32[nitems + 1] = p32[nitems] - 1;

    // find all peaks
    if( (ierr = vminmax(vtmp1, vtmp16, 1, nitems / 2, minDist))) {
      goto errorExit;
    }

    PRINTF2("iter %d: %d maxima\n", i, vgetnm(vtmp16));

    if (i > 0 && vgetnm(vtmp16) < 2) {
      // take previous
      if( (ierr = vcopy(vtmp16last, vtmp16, 0, -1, 0))) {
        goto errorExit;
      }

      if (vgetnm(vtmp16) < 2) {
        ierr = 1;                 // not bimodal
        goto errorExit;
      }

      break;
    }

    // copy new filtered histogram to srcvec
    if( (ierr = vcopy(vtmp1, histvec, 1, nitems, 0))) {
      goto errorExit;
    }

    if (vgetnm(vtmp16) < 2) {
      ierr = 1;                 // not bimodal
      goto errorExit;
    }

    if (vgetnm(vtmp16) <= 2) break;

    // remember
    if( (ierr = vcopy(vtmp16, vtmp16last, 0, -1, 0))) {
      goto errorExit;
    }

  }

  if( vgetnm(vtmp16) > 2) {
    // take the biggest two
    vbest = -1;
    vsecond = -1;
    best = -1;
    second = -1;
    p16 = (int16 *)vgetpm(vtmp16);
    p32 = (int32 *)vgetpm(vtmp1);
    for (i = 0; i < vgetnm(vtmp16); i++) {
      if (p32[p16[i]] > vbest) {
        vsecond = vbest;
        second = best;
        vbest = p32[p16[i]];
        best = p16[i];
      } else {
        if (p32[p16[i]] > vsecond) {
          vsecond = p32[p16[i]];
          second = p16[i];
        }
      }
    }
    vputnm(vtmp16, 0);
    if (best >= 0) {
      p16[0] = best;
      vputnm(vtmp16, 1);
      if (second >= 0) {
        p16[1] = second;
        vputnm(vtmp16, 2);
      }
    }
  }

  p16 = (int16 *)vgetpm(vtmp16);

  if (vgetnm(vtmp16) >= 2) { // only == 2 here possible
    
    if (p16[0] < p16[1]) {
      start = p16[0];
      end   = p16[1];
    } else {
      start = p16[1];
      end   = p16[0];
    }

    // correct the shift of 1 position
    start -= 1;
    if (start < 0) start = 0;                  // be sure
    if (start >= nitems) start = nitems - 1;
    end -= 1;
    if (end < 0) end = 0;                  // be sure
    if (end >= nitems) end = nitems - 1;

    if (thresPerc < 0) {

      // take as thres the minimum between

      //x/PRINTF2("%d ... %d\n", start, end);

      if (end <= start) {
        ierr = -10;
        goto errorExit;
      }

      if( (ierr = vcopy(histvec, vtmp1, start, (end - start + 1), 0))) {
        goto errorExit;
      }

      // find global minimum
      if( (ierr = vminmax(vtmp1, vtmp16, 0, 1, 0))) {
        goto errorExit;
      }

      p16 = (int16 *)vgetpm(vtmp16);
      *retThres = start + p16[0];
      //x/PRINTF3("%d ... %d -> %d (min)\n", start, end, *retThres);

    } else {
      *retThres = start + ((int32)(end - start) * (int32)thresPerc + 50) / 100;
      //x/PRINTF4("%d ... %d -> %d (%d [%%])\n", start, end, *retThres, thresPerc);
    }


    p32 = (int32 *)vgetpm(histvec);
    sumLo = 0;
    for (i = 0; i < *retThres; i++) {
      sumLo += p32[i];
    }
    sumHi = 0;
    for (i = *retThres + 1; i < nitems; i++) {
      sumHi += p32[i];
    }       
    //x/PRINTF2("areaLo %d areaHi %d\n", sumLo, sumHi);

    if (sumLo > sumHi) {
      if( retMax1 != NULL) {
        *retMax1 = start;
      }
      if( retMax2 != NULL) {
        *retMax2 = end;
      }
    } else {
      if( retMax1 != NULL) {
        *retMax1 = end;
      }
      if( retMax2 != NULL) {
        *retMax2 = start;
      }
    }

  } else {

    ierr = 1;                 // not bimodal
    goto errorExit;

  }

  ierr = 0;

errorExit:

  if (vtmp1      != VENULL) ve_remove(vtmp1);
  if (vtmp2      != VENULL) ve_remove(vtmp2);
  if (vtmp16     != VENULL) ve_remove(vtmp16);
  if (vtmp16last != VENULL) ve_remove(vtmp16last);

  return( ierr);
}
/***************************************************************************/
