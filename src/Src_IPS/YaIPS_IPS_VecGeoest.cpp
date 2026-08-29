/* **************************************************************************
/@
/@ Short-title: estimation of geometric transformation using passpoints
/@
/@ ==========================================================================
/@
/@ INDEX
/@
/@ # Mvgeoest     # vgeoest     # estimation of geometric transformation
/@
/@ USER DESCRIPTION
/@.. vgeoest srcvec1 srcvec2 dstvec rlimit nmax pmode
/@
/@.     estimation of geometric transformation  x1 = A * x2 + b
/@.     using at least 3 passpoint pairs in 'srcvec1', 'srcvec2'.
/@
/@      'srcvec1' and  'srcvec2' contain the coordinates of at least 3
/@      passpoint pairs in coordinates of two different twodimensional
/@      geometric systems 1 and 2. The estimation of the geometric
/@      transformation system
/@
/@      /  \   /       \   /  \   /  \     a11 a12  rotation matrix
/@      |x1|   |a11 a12|   |x2|   |b1|     a21 a22
/@      |  | = |       | * |  | + |  |            T
/@      |y1|   |a21 a22|   |y2|   |b2|     [b1 b2]  translation vector
/@      \  /   \       /   \  /   \  /
/@
/@      is computed and returned in vector 'dstvec':
/@               index      value
/@                 1         a11
/@                 2         a12
/@                 3         a21
/@                 4         a22
/@                 5         b1
/@                 6         b2
/@
/@      ALGORITHM
/@
/@      1. The coordinates of three points out of
/@         'srcvec2' are copied into the temporary vector 'vtmp'.
/@         point 2 is the point with maximum x-distance to point 1 and
/@         point 3 is the point with maximum y-distance to point 1 (!=p2).
/@
/@      2. if 'nmax' == 0 -> nmax = 7.
/@
/@      3. Compute estimation of geometric transform system
/@                T             T         T
/@         [x2 y2] = A * [x1 y1] + [b1 b2]
/@
/@         using points in 'srcvec1' and 'vtmp'.
/@
/@      4. Transform all points of srcvec1 into system 2 with estimated
/@         transform system (vector v1t).
/@
/@      5. Compute the least square deviation of the estimation
/@
/@                    n
/@         a = 1/n * sum(sqrt(dxi * dxi + dyi * dyi))
/@                   i=1
/@
/@         with n = number of passpoints
/@         dxi = x2i - x1ti
/@         dyi = y2i - y1ti
/@
/@      6. If variance of least square deviation (reference last pass)
/@         < 'rlimit' or |least square deviation| < 1e-10 -> 8
/@
/@      7. compute the regression straights of the x- and y- deviation:
/@
/@         Regression straight REGX : (x2 - x1t) = f(y2)
/@         Regression straight REGY : (y2 - y1t) = f(x2)
/@
/@         correct the 3 points in 'vtmp' as follows:
/@
/@         xnew = REGX(yold) + xold
/@         ynew = REGX(xold) + yold
/@
/@         goto -> 3.
/@
/@      8. Compute the estimated geometric transform system
/@                T             T         T
/@         [x1 y1] = A * [x2 y2] + [b1 b2]
/@
/@         using points in 'srcvec1' and 'vtmp'.
/@
/@       9. write a11, a12, a21, a22, b1, b2 in 'dstvec'.
/@
/@
/@      PARAMETERS
/@      srcvec1         (int32)  vector passpoint coordinates system 1
/@      srcvec2         (int32)  vector passpoint coordinates system 2
/@      dstvec          (sfloat) vector with result
/@      rlimit          (int32)  relative limit variation of least square
/@                               deviation of transformed points (with
/@                               estimated system) between two estimation
/@                               passes ( in o/ooooo)
/@      nmax            (int16)  maximum number of estimation passes
/@      pmode           (int16)  printout mode:
/@                               0: no printouts
/@                               1: least square deviation of each estimation
/@                               2: 1 + x- and y-deviation of each passpoint
/@                                  in each estimation pass
/@
/@      RESTRICTIONS
/@      For estimation at least 3 passpoint pairs are necessary
/@      Pixel are assumed to be square pixel !
/@
/@      SEE ALSO
/@      vptrans.
/@
/@ FUNCTION DESCRIPTION
/@
/@     #include "portab.h"
/@     #include "sip.h"
/@     #include "sipve.h"
/@     #include "tstpr.h"
/@
/@     int vgeoest(srcvec1,srcvec2,dstvec,rlimit,nmax,pmode,v1t,vtmp)
/@     Tvector *v1t, *vtmp;     if NULL, created inside
/@     Tvector *srcvec1;        vector ( int32 )
/@     Tvector *srcvec2;        vector ( int32 )
/@     Tvector *dstvec;         resultvector (sfloat )
/@     int32 rlimit;
/@     int16 nmax;
/@     int16 pmode;
/@
/@ RETURN VALUES
/@
/@      All routines return 0 after successful execution.
/@      In case of error a negative value is returned.
/@
/@      -10 srcvec1 or srcvec2 has less than 6 values
/@      -11 number of items in srcvec1 and srcvec2 doesn't match
/@      -12 error in ve_ucreate
/@      -13 no solution possible, wrong passpoint location
/@
/@      ierr error in utvcheck/ve_alloc
/@
/@ MODIFICATIONS
/@ V 1.00 : First edition released.
/@ V 3.30 : SIP3c standard, bug fixed in automatic passpoint search
/@ V 3.31 : internal vectors created only if not given from outside (v1t,vtmp)
/@ V 3.32 : bug fix: max test modified to >=
/@ *************************************************************************/

#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "YaIPS_IPS_Interface.h" // 15.05.2025 RR: Need this for IPS defines
#include "YaIPS_LanguageStrings.h"  // Language string definitions

/*======================IP ROUTINE ========================================*/

// utility
#define absol(a) (((a) < (0.0)) ? (-a):(a))

int vgeoest( Tvector *srcvec1,  /* passpoints system 1                */
             Tvector *srcvec2,  /* passpoints system 2                */
             Tvector *dstvec,   /* resultvector                       */
             int32 rlimit,      /* relative limit of least square dev.*/
             int16 nmax,        /* maximal number of estimation passes*/
             int16 pmode,       /* printout mode                      */
             Tvector *v1t,      /* if (Tvector *)0, created inside    */
             Tvector *vtmp)     /* if (Tvector *)0, created inside    */
{
  int ierr;
  register int32 *psrc1;                   /* pointer on srcvec1          */
  register int32 *psrc2;                   /* pointer on srcvec2          */
  sfloat *pdst;                            /* pointer on dstvec           */
  int16 ncoords;                           /* number of passpoint-coord.  */
  int16 npoints;                           /* number of passpoints        */
  int16 nph;                               /* index on center-point       */
  int16 np;                                /* index on last point         */
  register lfloat *ptrlf;                  /* pointer                     */
  register int32  *ptri32;                 /* pointer                     */
  lfloat *ptmp;                            /* pointer on vtmp data base   */
  register lfloat *p1t;                    /* pointer on v1t              */
  register int16 i;                        /* loop-counter                */
  register int16 count;                    /* loop-counter                */
  lfloat det;                              /* determinante A              */
  lfloat tmpa[2][2];                       /* A                           */
  lfloat tmpb[2];                          /* b                           */
  lfloat tmpmatrix[3][3];                  /* temporary matrix            */
  lfloat tmp,tmpx,tmpy,tmpxt,tmpyt;        /* temporary parameters        */
  lfloat regxsxi,regxsxixi,regxsyi,regxsxiyi;          /* regression sums */
  lfloat regysxi,regysxixi,regysyi,regysxiyi;          /* regression sums */
  lfloat sxx,sxxy,exx,exy;                 /* regression coefficients     */
  lfloat syx,syxy,eyx,eyy;                 /* regression coefficients     */
  lfloat lastlsq=0.0;                      /* history least square dev    */
  int32 max, tmp32;                        /* for passpointsearch         */
  int vec_created = 0;

  /*----------------------------------------------------------------------*/
  /* check input parameters                                               */
  /*----------------------------------------------------------------------*/
  if( (ierr = utvcheck(srcvec1,DV_HOST,TY_INT32)) != 0) return(ierr);
  if( (ierr = utvcheck(srcvec2,DV_HOST,TY_INT32)) != 0) return(ierr);
  if( (ierr = utvcheck(dstvec,DV_HOST,TY_SFLOAT)) != 0) return(ierr);
  if((ncoords = (int16)vgetnm(srcvec1)) < 6 ) return(-10);
  if((int16)vgetnm(srcvec2) != ncoords) return(-11);
  npoints = ncoords >> 1;
  PRINTF0("GEOEST: input parameters ok\n");
  /*----------------------------------------------------------------------*/
  /* init data base pointers                                              */
  /*----------------------------------------------------------------------*/
  psrc1 = (int32 *)vgetpm(srcvec1);        /* pointer on srcvec1          */
  psrc2 = (int32 *)vgetpm(srcvec2);        /* pointer on srcvec2          */
  if( (ierr = ve_alloc(dstvec,6L,(int16)sizeof(sfloat),TY_SFLOAT)) != 0) return(ierr);
  pdst = (sfloat *)vgetpm(dstvec);         /* pointer on dstvec           */
  PRINTF0("GEOEST: data base pointers initialized\n");
  /*----------------------------------------------------------------------*/
  /* get indices of best passpoints                                       */
  /*----------------------------------------------------------------------*/
  np = nph = 0;
  /* get index of point with maximal x-distance from first (nph)          */
  max = 0;
  for(i = 2;i < ncoords;i +=2) {
    if((tmp32 = *(psrc1 + i) - *psrc1) < 0) tmp32 = -tmp32;
    if(tmp32 >= max) {
      nph = i;
      max = tmp32;
    }
  }
  /* get index of point with maximal y-distance from first (np) != nph    */
  max = 0;
  psrc1++;
  for(i = 2;i < ncoords;i +=2) {
    if((tmp32 = *(psrc1 + i) - *psrc1) < 0) tmp32 = -tmp32;
    if((tmp32 >= max)&&(i != nph)) {
      np = i;
      max = tmp32;
    }
  }
  psrc1--;
  if((nph == 0)||(np == 0)) return(-13);
  PRINTF4("GEOEST: ncoords: %d npoints: %d nph: %d np: %d\n",ncoords,npoints,
	   nph,np);
  /*----------------------------------------------------------------------*/
  /* copy 3 passpoints out of srcvec2 in vtmp                             */
  /*----------------------------------------------------------------------*/
  if (vtmp == (Tvector *)0) {
    if((vtmp = ve_ucreate(DV_HOST)) == VENULL) return(-12);
    vec_created |= 1;
  }
  if (v1t == (Tvector *)0) {
    if((v1t  = ve_ucreate(DV_HOST)) == VENULL) {
      ierr = -12;
      goto errortmp;
    }
    vec_created |= 2;
  }
  if(( ierr = ve_alloc(vtmp,6L,(int16)sizeof(lfloat),TY_LFLOAT)) != 0) {
    goto error;
  }
  if(( ierr = ve_alloc(v1t,vgetnm(srcvec1),(int16)sizeof(lfloat),TY_LFLOAT)) != 0) {
    goto error;
  }

  ptmp  = (lfloat *)vgetpm(vtmp);          /* pointer on vtmp data base   */
  p1t   = (lfloat *)vgetpm(v1t);           /* pointer on v1t              */

  ptrlf = ptmp;
  *ptrlf++ = (lfloat)*psrc2;
  *ptrlf++ = (lfloat)*(psrc2 + 1);
  *ptrlf++ = (lfloat)*(psrc2 + nph);
  *ptrlf++ = (lfloat)*(psrc2 + nph + 1);
  *ptrlf++ = (lfloat)*(psrc2 + np);
  *ptrlf   = (lfloat)*(psrc2 + np + 1);
  vputnm(vtmp,6L);
  PRINTF0("GEOEST: passpoints copied\n");
  PRINTF2("passpoint11: %ld %ld\n",*psrc1,*(psrc1 + 1));
  PRINTF2("passpoint12: %ld %ld\n",*(psrc1 + nph),*(psrc1 + nph + 1));
  PRINTF2("passpoint13: %ld %ld\n",*(psrc1 + np),*(psrc1 + np + 1));
  PRINTF2("passpoint21: %lf %lf\n",*ptmp,*(ptmp + 1));
  PRINTF2("passpoint22: %lf %lf\n",*(ptmp + 2),*(ptmp + 3));
  PRINTF2("passpoint23: %lf %lf\n",*(ptmp + 4),*(ptmp + 5));
  /*----------------------------------------------------------------------*/
  /* begin estimation passes                                              */
  /*----------------------------------------------------------------------*/
  if(nmax > 0) {
    PRINTF0("GEOEST: begin correction of estimated system\n");
    /*--------------------------------------------------------------------*/
    /* compute matrix for computation estimated geom.transf.syst. 1 -> 2  */
    /*--------------------------------------------------------------------*/
    det = (lfloat)*psrc1         *   *(psrc1 + nph + 1) -
	  (lfloat)*psrc1         *   *(psrc1 + np  + 1) +
	  (lfloat)*(psrc1 + nph) *   *(psrc1 + np  + 1) -
	  (lfloat)*(psrc1 + nph) *   *(psrc1 + 1)       +
	  (lfloat)*(psrc1 + np)  *   *(psrc1 + 1)       -
	  (lfloat)*(psrc1 + np)  *   *(psrc1 + nph + 1);
    PRINTF1("GEOEST: determinante A 1->2 = %lf\n",det);

    if ((det < 1e-20)&&(det > -1e-20)) {
      ierr = -14;
      goto error;
    }

    tmpmatrix[0][0] = ((lfloat)*(psrc1 + nph + 1) - *(psrc1 + np  + 1))/det;
    tmpmatrix[0][1] = ((lfloat)*(psrc1 + np  + 1) - *(psrc1 + 1)      )/det;
    tmpmatrix[0][2] = ((lfloat)*(psrc1 + 1)       - *(psrc1 + nph + 1))/det;

    tmpmatrix[1][0] = ((lfloat)*(psrc1 + np)  - *(psrc1 + nph))/det;
    tmpmatrix[1][1] = ((lfloat)*psrc1         - *(psrc1 + np) )/det;
    tmpmatrix[1][2] = ((lfloat)*(psrc1 + nph) - *psrc1        )/det;

    tmpmatrix[2][0] = ((lfloat)*(psrc1 + nph) * *(psrc1 + np  + 1) -
		       (lfloat)*(psrc1 + np)  * *(psrc1 + nph + 1))/det;
    tmpmatrix[2][1] = ((lfloat)*(psrc1 + np)  * *(psrc1 + 1)       -
		       (lfloat)*psrc1         * *(psrc1 + np  + 1))/det;
    tmpmatrix[2][2] = ((lfloat)*psrc1         * *(psrc1 + nph + 1) -
		       (lfloat)*(psrc1 + nph) * *(psrc1 + 1)      )/det;
    /*--------------------------------------------------------------------*/
    /* do nmax times                                                      */
    /*--------------------------------------------------------------------*/
    for(count = 0;count < nmax;count++) {
      PRINTF1("inside estimation-loop %d. pass\n",(count + 1));
      if(iabort()) { ierr = 0; goto error; }
      /*------------------------------------------------------------------*/
      /* compute estimation of geometric transform system 1 -> 2          */
      /*------------------------------------------------------------------*/
      tmpa[0][0] = tmpmatrix[0][0] * *ptmp +
	       tmpmatrix[0][1] * *(ptmp + 2) + tmpmatrix[0][2] * *(ptmp + 4);
      tmpa[0][1] = tmpmatrix[1][0] * *ptmp +
	       tmpmatrix[1][1] * *(ptmp + 2) + tmpmatrix[1][2] * *(ptmp + 4);
      tmpa[1][0] = tmpmatrix[0][0] * *(ptmp + 1) +
	       tmpmatrix[0][1] * *(ptmp + 3) + tmpmatrix[0][2] * *(ptmp + 5);
      tmpa[1][1] = tmpmatrix[1][0] * *(ptmp + 1) +
	       tmpmatrix[1][1] * *(ptmp + 3) + tmpmatrix[1][2] * *(ptmp + 5);
      tmpb[0]    = tmpmatrix[2][0] * *ptmp +
	       tmpmatrix[2][1] * *(ptmp + 2) + tmpmatrix[2][2] * *(ptmp + 4);
      tmpb[1]    = tmpmatrix[2][0] * *(ptmp + 1) +
	       tmpmatrix[2][1] * *(ptmp + 3) + tmpmatrix[2][2] * *(ptmp + 5);
      PRINTF0("GEOEST: estimated geometric transform system 1 -> 2 : \n");
      PRINTF3("%lf %lf   %lf\n",tmpa[0][0],tmpa[0][1],tmpb[0]);
      PRINTF3("%lf %lf   %lf\n",tmpa[1][0],tmpa[1][1],tmpb[1]);
      /*------------------------------------------------------------------*/
      /* transform all points out of srcvec1 -> v1t                       */
      /*------------------------------------------------------------------*/
      for(i = 0;i < ncoords;i += 2) {

	      *(p1t + i)     = tmpa[0][0] * *(psrc1 + i) + tmpa[0][1] * *(psrc1 + i + 1) + tmpb[0];
	      *(p1t + i + 1) = tmpa[1][0] * *(psrc1 + i) + tmpa[1][1] * *(psrc1 + i + 1) + tmpb[1];
      }
      vputnm(v1t,(int32)ncoords);

      /*------------------------------------------------------------------*/
      /* compute least square deviation of points(v1t) and points(srcvec2)*/
      /*------------------------------------------------------------------*/
      tmp = (lfloat)0.;
      ptrlf  = p1t;
      ptri32 = psrc2;
      for(i = 0;i < ncoords;i += 2) {
        tmpx = *ptri32++ - *ptrlf++;
        tmpy = *ptri32++ - *ptrlf++;
        tmp += (tmpx * tmpx + tmpy * tmpy);

#ifdef use_again
        if(pmode >= 2)
          printfx(
              "%3d. point: x-deviation = %12.5f y-deviation = %12.5f [unit2]\n",
              ((i>>1) + 1),(sfloat)(*(psrc2+i) - *(p1t+i)),
              (sfloat)(*(psrc2+i+1) - *(p1t+i+1)));
#endif
      }
      tmp /= (lfloat)npoints;                                  /* norming */

#ifdef use_again
      if(pmode >= 1)
        printfx("%3d. estimation: least square deviation = %15.8lf [unit2^2]\n",
            (count + 1),tmp);
#endif

      /*------------------------------------------------------------------*/
      /* criterium for end of estimation                                  */
      /*------------------------------------------------------------------*/
      if(tmp < 1e-10) break;
      if(((absol((tmp - lastlsq)/tmp) * 1000000.) <= (lfloat)rlimit)&&
          (count > 0)) break;                            /* break for-loop */
      lastlsq = tmp;
      /*------------------------------------------------------------------*/
      /* compute regression straights  REGX: (x2 - x1t) = f(y2)           */
      /*                               REGY: (y2 - y1t) = f(x2)           */
      /*------------------------------------------------------------------*/
      regxsxi = regxsxixi = regxsyi = regxsxiyi = (lfloat)0.;
      regysxi = regysxixi = regysyi = regysxiyi = (lfloat)0.;

      ptrlf  = p1t;
      ptri32 = psrc2;
      for(i = 0;i < ncoords;i += 2) {
        tmpx  = *ptri32++;
        tmpy  = *ptri32++;
        tmpxt = tmpx - *ptrlf++;
        tmpyt = tmpy - *ptrlf++;
        regxsxi   += tmpy;
        regxsyi   += tmpxt;
        regxsxixi += tmpy * tmpy;
        regxsxiyi += tmpy * tmpxt;
        regysxi   += tmpx;
        regysyi   += tmpyt;
        regysxixi += tmpx * tmpx;
        regysxiyi += tmpx * tmpyt;
      }

      sxx  = regxsxixi - (regxsxi * regxsxi)/npoints;
      sxxy = regxsxiyi - (regxsxi * regxsyi)/npoints;
      exx  = regxsxi/npoints;
      exy  = regxsyi/npoints;
      PRINTF4("sxx=%lf, sxxy=%lf, exx=%lf, exy=%lf\n",sxx,sxxy,exx,exy);
      if ((sxx < 1e-20)&&(sxx > -1e-20)) {
        ierr = -16;
        goto error;
      }

      syx  = regysxixi - (regysxi * regysxi)/npoints;
      syxy = regysxiyi - (regysxi * regysyi)/npoints;
      eyx  = regysxi/npoints;
      eyy  = regysyi/npoints;
      PRINTF4("syx=%lf, syxy=%lf, eyx=%lf, eyy=%lf\n",syx,syxy,eyx,eyy);
      if ((syx < 1e-20)&&(syx > -1e-20)) {
        ierr = -17;
        goto error;
      }

      /*------------------------------------------------------------------*/
      /* correct the coordinates of the points in vtmp                    */
      /* regression straight y = sxy/sx * (x - ex) + ey                   */
      /*------------------------------------------------------------------*/
      ptrlf = ptmp;
      tmpx = *ptrlf++;
      tmpy = *ptrlf++;
      *ptmp       = tmpx + sxxy * ((tmpy - exx)/sxx) + exy;
      *(ptmp + 1) = tmpy + syxy * ((tmpx - eyx)/syx) + eyy;
      tmpx = *ptrlf++;
      tmpy = *ptrlf++;
      *(ptmp + 2) = tmpx + sxxy * ((tmpy - exx)/sxx) + exy;
      *(ptmp + 3) = tmpy + syxy * ((tmpx - eyx)/syx) + eyy;
      tmpx = *ptrlf++;
      tmpy = *ptrlf;
      *(ptmp + 4) = tmpx + sxxy * ((tmpy - exx)/sxx) + exy;
      *(ptmp + 5) = tmpy + syxy * ((tmpx - eyx)/syx) + eyy;
    } /* end of for */
    /*--------------------------------------------------------------------*/
  } /* end of if */
  /*----------------------------------------------------------------------*/
  /* compute estimated geometric transform system vtmp -> vsrc1           */
  /*----------------------------------------------------------------------*/
  det = *ptmp       *  *(ptmp + 3) - *ptmp       *  *(ptmp + 5) +
        *(ptmp + 2) *  *(ptmp + 5) - *(ptmp + 2) *  *(ptmp + 1) +
        *(ptmp + 4) *  *(ptmp + 1) - *(ptmp + 4) *  *(ptmp + 3);
  PRINTF1("GEOEST: before end, determinante A 2->1 = %lf\n",det);

  if ((det < 1e-20)&&(det > -1e-20)) {
    ierr = -15;
    goto error;
  }

  tmpmatrix[0][0] = (*(ptmp + 3) - *(ptmp + 5))/det;
  tmpmatrix[0][1] = (*(ptmp + 5) - *(ptmp + 1))/det;
  tmpmatrix[0][2] = (*(ptmp + 1) - *(ptmp + 3))/det;

  tmpmatrix[1][0] = (*(ptmp + 4) - *(ptmp + 2))/det;
  tmpmatrix[1][1] = (*ptmp       - *(ptmp + 4))/det;
  tmpmatrix[1][2] = (*(ptmp + 2) - *ptmp      )/det;

  tmpmatrix[2][0] = (*(ptmp + 2) * *(ptmp + 5) -
      *(ptmp + 4) * *(ptmp + 3))/det;
  tmpmatrix[2][1] = (*(ptmp + 4) * *(ptmp + 1) -
      *ptmp       * *(ptmp + 5))/det;
  tmpmatrix[2][2] = (*ptmp       * *(ptmp + 3) -
      *(ptmp + 2) * *(ptmp + 1))/det;

  *pdst       = (sfloat)(tmpmatrix[0][0] * *psrc1 + tmpmatrix[0][1] * *(psrc1 + nph) + tmpmatrix[0][2] * *(psrc1 + np));
  *(pdst + 1) = (sfloat)(tmpmatrix[1][0] * *psrc1 + tmpmatrix[1][1] * *(psrc1 + nph) + tmpmatrix[1][2] * *(psrc1 + np));
  *(pdst + 2) = (sfloat)(tmpmatrix[0][0] * *(psrc1 + 1) + tmpmatrix[0][1] * *(psrc1 + nph + 1) + tmpmatrix[0][2] * *(psrc1 + np + 1));
  *(pdst + 3) = (sfloat)(tmpmatrix[1][0] * *(psrc1 + 1) + tmpmatrix[1][1] * *(psrc1 + nph + 1) + tmpmatrix[1][2] * *(psrc1 + np + 1));
  *(pdst + 4) = (sfloat)(tmpmatrix[2][0] * *psrc1 + tmpmatrix[2][1] * *(psrc1 + nph) + tmpmatrix[2][2] * *(psrc1 + np));
  *(pdst + 5) = (sfloat)(tmpmatrix[2][0] * *(psrc1 + 1) + tmpmatrix[2][1] * *(psrc1 + nph + 1) + tmpmatrix[2][2] * *(psrc1 + np + 1));

  vputnm(dstvec,6L);
  PRINTF0("GEOEST: final geometric transform system 2 -> 1 : \n");
  PRINTF3("%lf %lf   %lf\n",*pdst,*(pdst + 1),*(pdst + 4));
  PRINTF3("%lf %lf   %lf\n",*(pdst + 2),*(pdst + 3),*(pdst + 5));
  /*----------------------------------------------------------------------*/
  ierr = 0;
  error:
  if (vec_created & 2) ve_remove(v1t);
  errortmp:
  if (vec_created & 1) ve_remove(vtmp);
  return(ierr);
}

/****************************** end of file ********************************/
