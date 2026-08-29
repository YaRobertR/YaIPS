/* **************************************************************************
/@
/@ Name: extpos.c      Package: sip
/@
/@ Short-title: get extremum position with subpixel precision
/@
/@ ==========================================================================
/@
/@ INDEX
/@   # -        # extpos    #  extremum position with subpixel precision
/@   # -        # extposv   #  extr.pos. with subpixel prec.; vector version
/@
/@ USER DESCRIPTION
/@
/@      SEE ALSO 
/@
/@ FUNCTION DESCRIPTION
/@
/@   no PA-routine available at the moment
/@
/@   extpos   srcim x  y   imode xp yp
/@   extposv  srcim vector imode
/@
/@      Interpolate extremum position with subpixel precision.
/@      (preliminary version)
/@
/@      ALGORITHM
/@      * quadratic interpolation for 3 point data horizontal and vertical
/@        or
/@      * least square fit and quadratic interpolation for 5 or 7 point data
/@        horizontal and vertical.
/@
/@      Additional diagonal interpolation is supported too.
/@
/@      If the size of 'srcim' is 1 in one dimension, an unidirectional
/@      interpolation in the other direction is performed.
/@
/@      PARAMETERS
/@      srcim       source image
/@      x, y        estimated coordinates of extremum (integer precision)
/@      vector      vector containing correlation value and position
/@                  (as resulting from correlation functions: "bestcorvec")
/@                  and to store xp*16., yp*16. as unsigned integer
/@      imode       interpolation mode
/@                    3|5|7[d]: 3, 5, or 7 point biquadratic interpolation
/@                    with additional diagonal interpolation if "d" is set
/@                    4: straight line cutpoint interpolation
/@      xp, yp      resulting extremum position (subpixel precision), float
/@
/@      RESTRICTIONS
/@      Precision of 1e-3
/@      Only the following data types are allowed:
/@        "srcim"     "vector"
/@         int8    }
/@         int16   }   FFuncval or nil
/@         int32   }
/@         sfloat  }
/@
/@   #include <stdio.h>
/@   #include <math.h>
/@   #include "portab.h"
/@   #include "sip.h"
/@   #include "sipve.h"
/@   #include "crrel.h"
/@
/@   int extpos(srcim,x,y,imode,xp,yp)
/@      Timages   *srcim;             source image
/@      int16      x, y;              estimated extremum position
/@      string     imode;             interpolation mode
/@      sfloat     *xp, *yp;          interpolated extremum position
/@
/@   int extposv(srcim,vector,imode,xp,yp)
/@      Timages   *srcim;             source image
/@      Tvector   *vector;            vector of type Sffuncval storing
/@                                    position and to store the interpolated
/@                                    extremum position
/@      string     imode;             interpolation mode
/@      sfloat     *xp, *yp;          interpolated extremum position
/@
/@   The vector "vector" consists of two float function structures
/@   "ffuncval" pointed to by "bestval" and "secval". The structure is
/@   of the form:
/@     struct Sffuncval
/@     {
/@       int16  fxpos, fypos;
/@       sfloat ffuncval;
/@     };
/@   This vector also is used to store the extremum position with subpixel
/@   precision, exactly as 16 times position, rounded and converted to int16
/@   (int16)(position*16.+.5)
/@   If no extremum is found, MAXINT16 is returned as extremum position.
/@   As function value at the interpolated extremum the function value
/@   of the best match will be taken.
/@   The vector is defined in the include file sipve.h.
/@
/@   Interpolation of image extremum using either
/@    -  the quadratic interpolation for 3 point data:
/@         For exactly 3 points given the difference of the parabel
/@         extremum to the intermediate point is calculated.
/@    -  the straight line cutpoint interpolation for 4 point data:
/@         through 4 points 2 straight lines are layed. The interpolated 
/@         extremum is located at the cutpoint of the 2 straight lines.
/@    -  the least square method for a quadratic equation for 5 or more
/@       point data:
/@         A 3*3- matrix for linear regression, the coefficients
/@         of the quadratic equation and the absolute location of the
/@         extremum are calculated.
/@
/@   If the diagonal mode is used, an additionally interpolation is done
/@   between the SE and NW-bound diagonals. The interpolated extremum is
/@   located at the mean value of XY- and diagonal direction.
/@   If all interpolation points do have the same values an error
/@   message is returned.
/@   If all interpolation points of one direction  do have the same values,
/@   the coordinate of that direction is left unchanged.
/@   If the extremum is located to near the image border, so that an inter-
/@   polation is not possible, an error occours.
/@
/@   RETURN VALUES
/@   Negative in case of error.
/@   +1 if all points of same value and no extremum is found
/@   -10 if extremum is on image border and no interpolation is possible 
/@
/@ REFERENCES
/@   correlation routines: crrel.c crrel.h
/@
/@ MODIFICATIONS
/@   V 1.00 : First edition released : SIP IIc Standard
/@   V 2.00 : SIP IId; extposv. US
/@   V 3.00 : SIP IIe. no PA. US
/@   V 3.30 : SIP 3c. interpolated extr. height same as best extr. height
/@   V 3.31 : changed MAXINT to MAXINT16
/@   V 3.32 : some corrections in error handling, allow unidirectional
/@            interpolation, allow TY_INT32 images
/@   V 3.33 : new interpolation mode 4sc
/@ *************************************************************************/

#include <windows.h>
#include <math.h>

#include "YaIPS_IPS_Interface.h" // 15.05.2025 RR: Need this for IPS defines
#include "YaIPS_LanguageStrings.h"  // Language string definitions

#define   MAXGUIDE      10
#define   XYDIR         1
#define   DIAGDIR       2
#define   N_MATRIX      3

/*===================== IP ROUTINE ========================================*/

/* ======================================================================== */

static lfloat scinter( lfloat y1, lfloat y2, lfloat y3, lfloat y4) /* cutpoint of 2 straights through
=================================================================    (0/y1), (1/y2) and (2/y3), (3/y4) */
{
  if ((y2 - y1 - y4 + y3) == 0.) return(1.5); /* both straights have slope 0 */
  return((3. * y3 - 2. * y4 - y1)/(y2 - y1 - y4 + y3));
}

/*=========================================================================*/
static find_xguidept( Timages *image,
                      int16  x0,
                      int16  y0,
                      lfloat *guide1,
                      int16  n)
{
  register int8   *x8p;
  register int16  *x16p;
  register int32  *x32p;
  register sfloat *xsfp;
  register int16   i;
  int16    xmin,xmax,dmax;

  dmax = n/2;                   /* int-division ! */

  xmin = x0-dmax;  xmax = x0+dmax;
  if (xmin < 0 || xmax >= getxx(image)) return(-10);
  if (y0 < 0   || y0 >=   getyy(image)) return(-10);

  if (getyp(image) == TY_BYTE) {
    x8p = pixadt(xmin,y0,image,int8);
    for (i=0; i<n; i++) guide1[i] = (double)btoi(*x8p++);
  } else if (getyp(image) == TY_INT16) {
    x16p = pixadt(xmin,y0,image,int16);
    for (i=0; i<n; i++) guide1[i] = (double)*x16p++;
  } else if (getyp(image) == TY_INT32) {
    x32p = pixadt(xmin,y0,image,int32);
    for (i=0; i<n; i++) guide1[i] = (double)*x32p++;
  } else if (getyp(image) == TY_SFLOAT) {
    xsfp = pixadt(xmin,y0,image,sfloat);
    for (i=0; i<n; i++) guide1[i] = (double)*xsfp++;
  }
  for ( i=0; i<n-1; i++ )
    if (guide1[i] != guide1[i+1])
      break;
  return( (i < n-1) ? 0 : 1 );
}

/*=========================================================================*/
static find_yguidept( Timages  *image,
                      int16  x0,
                      int16  y0,
                      lfloat *guide2,
                      int16  n)
{
  register int8   *y8p;
  register int16  *y16p;
  register int32  *y32p;
  register sfloat *ysfp;
  register int16   i;
  int16    rowinc=getxm(image);
  int16    ymin,ymax,dmax;

  dmax = n/2;                   /* int-division ! */

  ymin = y0-dmax;  ymax = y0+dmax;
  if (x0   < 0 || x0   >= getxx(image)) return(-10);
  if (ymin < 0 || ymax >= getyy(image)) return(-10);

  if (getyp(image) == TY_BYTE) {
    y8p = pixadt(x0,ymin,image,int8);
    for (i=0; i<n; i++,y8p+=rowinc) guide2[i] = (double)btoi(*y8p);
  } else if (getyp(image) == TY_INT16) {
    y16p = pixadt(x0,ymin,image,int16);
    for (i=0; i<n; i++,y16p+=rowinc) guide2[i] = (double)*y16p;
  } else if (getyp(image) == TY_INT32) {
    y32p = pixadt(x0,ymin,image,int32);
    for (i=0; i<n; i++,y32p+=rowinc) guide2[i] = (double)*y32p;
  } else if (getyp(image) == TY_SFLOAT) {
    ysfp = pixadt(x0,ymin,image,sfloat);
    for (i=0; i<n; i++,ysfp+=rowinc) guide2[i] = (double)*ysfp;
  }
  for (i=0; i<n-1; i++)
    if (guide2[i] != guide2[i+1])
      break;
  return( (i < n-1) ? 0 : 1 );
}

/*=========================================================================*/
static find_guidept( Timages *image,
                     int16   x0,
                     int16   y0,
                     int16   dir,
                     lfloat  *guide1,
                     lfloat  *guide2,
                     int16   n)
{
  register int8   *x8p,  *y8p;
  register int16  *x16p, *y16p;
  register int32  *x32p, *y32p;
  register sfloat *xsfp, *ysfp;
  register int16   i;
  int16    rowinc=getxm(image);
  int16    xmin,xmax,ymin,ymax,
     dmax;
  dmax = n/2;                   /* int-division ! */

  xmin = x0-dmax;  xmax = x0+dmax;
  ymin = y0-dmax;  ymax = y0+dmax;
  if ( xmin < 0 || ymin < 0 || xmax >= getxx(image) || ymax>= getyy(image) )
     return(-10);

  if (getyp(image) == TY_BYTE) {
    if (dir==XYDIR) {
      x8p = pixadt(xmin,y0,image,int8);
      y8p = pixadt(x0,ymin,image,int8);
      for (i=0; i<n; i++,x8p++,y8p+=rowinc) {
  guide1[i] = (double) btoi(*x8p);
  guide2[i] = (double) btoi(*y8p);
      }
    } else if (dir==DIAGDIR) {
      x8p = pixadt(xmin,ymin,image,int8);   /* SE-direction */
      y8p = pixadt(xmin,ymax,image,int8);   /* NE-direction */
      for (i=0; i<n; i++,x8p+=rowinc+1,y8p-=rowinc-1) {
  guide1[i] = (double) btoi(*x8p);
  guide2[i] = (double) btoi(*y8p);
      }
    }
  } else if (getyp(image) == TY_INT16) {
    if (dir==XYDIR) {
      x16p = pixadt(xmin,y0,image,int16);
      y16p = pixadt(x0,ymin,image,int16);
      for (i=0; i<n; i++,x16p++,y16p+=rowinc) {
  guide1[i] = (double) *x16p;
  guide2[i] = (double) *y16p;
      }
    } else if (dir==DIAGDIR) {
      x16p = pixadt(xmin,ymin,image,int16);  /* SE-direction */
      y16p = pixadt(xmin,ymax,image,int16);  /* NE-direction */
      for (i=0; i<n; i++,x16p+=rowinc+1,y16p-=rowinc-1) {
  guide1[i] = (double) *x16p;
  guide2[i] = (double) *y16p;
      }
    }
  } else if (getyp(image) == TY_INT32) {
    if (dir==XYDIR) {
      x32p = pixadt(xmin,y0,image,int32);
      y32p = pixadt(x0,ymin,image,int32);
      for (i=0; i<n; i++,x32p++,y32p+=rowinc) {
  guide1[i] = (double) *x32p;
  guide2[i] = (double) *y32p;
      }
    } else if (dir==DIAGDIR) {
      x32p = pixadt(xmin,ymin,image,int32);  /* SE-direction */
      y32p = pixadt(xmin,ymax,image,int32);  /* NE-direction */
      for (i=0; i<n; i++,x32p+=rowinc+1,y32p-=rowinc-1) {
  guide1[i] = (double) *x32p;
  guide2[i] = (double) *y32p;
      }
    }
  } else if (getyp(image) == TY_SFLOAT) {
    if (dir==XYDIR) {
      xsfp = pixadt(xmin,y0,image,sfloat);
      ysfp = pixadt(x0,ymin,image,sfloat);
      for (i=0; i<n; i++,xsfp++,ysfp+=rowinc) {
  guide1[i] = (double) *xsfp;
  guide2[i] = (double) *ysfp;
      }
    }
    else if (dir==DIAGDIR) {
      xsfp = pixadt(xmin,ymin,image,sfloat); /* SE-direction */
      ysfp = pixadt(xmin,ymax,image,sfloat); /* NE-direction */
      for (i=0; i<n; i++,xsfp+=rowinc+1,ysfp-=rowinc-1) {
  guide1[i] = (double) *xsfp;
  guide2[i] = (double) *ysfp;
      }
    }
  }
  for (i=0; i<n-1; i++)
    if (guide1[i] != guide1[i+1] || guide2[i] != guide2[i+1])
      break;
  return( (i < n-1) ? 0 : 1 );
}

/*=========================================================================*/
static int extpossc( Timages  *image,
              int16     x0, int16 y0,        /* inaccurate location  */
              char     *mode,                /* interpolation method */
              sfloat   *xi, sfloat *yi)
{
  lfloat    xguide[MAXGUIDE];
  lfloat    yguide[MAXGUIDE];
  lfloat    guide[MAXGUIDE];
  register int ierr = 0, i, unimode;

  if (strcmp( mode,CR_INT4_SC)) {
    errstring = (char *)"this mode is not implemented";  return(-1);
  }

  for( i = 0; i < MAXGUIDE; i++) {

    xguide[ i] = 0;    // Keep compiler silent (no warning about use of uninitialized variable)
  }

  unimode = 0;

  if (getyy(image) == 1) {
    /* special case: unidirectional interpolation in x */
    unimode = 1;
    if ((ierr = find_xguidept(image,x0,y0,xguide,3)) < 0)
      goto no_interpolation;
    *yi = y0;
  } else if (getxx(image) == 1) {
    /* special case: unidirectional interpolation in y */
    unimode = 2;
    if ((ierr = find_yguidept(image,x0,y0,yguide,3)) < 0)
      goto no_interpolation;
    *xi = x0;
  } else {
    /* standard case: bidirectional interpolation */
    if ((ierr = find_guidept(image,x0,y0,XYDIR,xguide,yguide,3)) < 0)
      goto no_interpolation;
  }

  if (!(unimode & 2)) {
    /* x-direction */
    if (xguide[0] < xguide[2]) {
      if ((ierr = find_xguidept(image,x0-2,y0,&guide[0],1)) < 0)
        goto no_interpolation;
      for (i = 0; i < 3; i++) guide[i+1] = xguide[i];
      *xi = x0 + scinter(guide[0],guide[1],guide[2], guide[3]) - 2.0;
    } else {
      if ((ierr = find_xguidept(image,x0+2,y0,&guide[3],1)) < 0)
        goto no_interpolation;
      for (i = 0; i < 3; i++) guide[i] = xguide[i];
      *xi = x0 + scinter(guide[0],guide[1],guide[2], guide[3]) - 1.0;
    }
  }

  if (!(unimode & 1)) {
    /* y-direction */
    if (yguide[0] < yguide[2]) {
      if ((ierr = find_yguidept(image,x0,y0-2,&guide[0],1)) < 0)
        goto no_interpolation;
      for (i = 0; i < 3; i++) guide[i+1] = yguide[i];
      *yi = y0 + scinter(guide[0],guide[1],guide[2], guide[3]) - 2.0;
    } else {
      if ((ierr = find_yguidept(image,x0,y0+2,&guide[3],1)) < 0)
        goto no_interpolation;
      for (i = 0; i < 3; i++) guide[i] = yguide[i];
      *yi = y0 + scinter(guide[0],guide[1],guide[2], guide[3]) - 1.0;
    }
  }

  return(0);

no_interpolation:
    *xi = x0;
    *yi = y0;
    if (ierr == -10)
      errstring = (char *)"extremum on image border, no interpolation possible";
    return(ierr);

} /* int extpossc() */

/*=========================================================================*/

static void mk_intpolequ( lfloat matrix[][N_MATRIX],
                     lfloat coeffvec[],
                     int16  p0,
                     lfloat *guide,
                     int16  pn)
{
  register int16 i,j;
  double   xsumex[MAXGUIDE],ysumex[MAXGUIDE],
     pexp, px;

  for ( i=0; i<pn; i++ )
  {  xsumex[i] = 0.;  ysumex[i] = 0.;  }

  xsumex[0] = pn;

  for ( i=0,px=p0-(pn-1)/2; i<pn; i++,px++ )       /* p is coordinate of */
  {                                              /* guiding point */
    ysumex[0]  += guide[i];
    pexp  = px;   xsumex[1] += pexp;   ysumex[1]  += pexp * guide[i];
    pexp *= px;   xsumex[2] += pexp;   ysumex[2]  += pexp * guide[i];
    pexp *= px;   xsumex[3] += pexp;
    pexp *= px;   xsumex[4] += pexp;
  }
  for ( i=0; i<N_MATRIX; i++ )
  {
    coeffvec[i] = ysumex[i];
    for( j=0; j<=i; j++ )
      matrix[i][j] = matrix[j][i] = xsumex[i+j];
  }
}

/*-------------------------------------------------------------------------*/
static lfloat quintpts( int16   p0,
                        lfloat *guide,
                        int16   n)
{
  lfloat matrix[N_MATRIX][N_MATRIX];
  lfloat coeffvec[N_MATRIX];

  mk_intpolequ( matrix,coeffvec,p0,guide,n);

  if ( (linequ( matrix[ 0], N_MATRIX, coeffvec, 1) ) < 0 )
  {
    errstring = (char *)"no solution possible";  return((lfloat)p0);
  }
  return( ( coeffvec[2] != 0 ) ?
  ( (-coeffvec[1])/(2*coeffvec[2]) ) : (lfloat)p0 );
}

/*-------------------------------------------------------------------------*/
static sfloat quinter( lfloat a, lfloat b, lfloat c)
{
  if( 2. * b == a + c) {

    return( (sfloat)0.0);
  }

  return( (sfloat) (((c - a)/2.) / (2. * b - a - c)));
}

/*------------------- interpolation ---------------------------------------*/

/*=========================================================================*/

static int extpos3r( Timages  *image,
                     int16   x0, int16 y0,               /* inaccurate location  */
                     char    *mode,                /* interpolation method */
                     sfloat  *xi, sfloat*yi)
{
  lfloat    xguide[MAXGUIDE];
  lfloat    yguide[MAXGUIDE];
  lfloat    diff0, diff1;
  register int ierr = 0, i, unimode;

  if (strcmp(mode,CR_INT3_R)) {
    errstring = (char *)"this mode is not implemented";  return(-1);
  }

  for( i = 0; i < MAXGUIDE; i++) {

    xguide[ i] = 0;    // Keep compiler silent (no warning about use of uninitialized variable)
  }

  unimode = 0;

  if (getyy(image) == 1) {
    /* special case: unidirectional interpolation in x */
    unimode = 1;
    if ((ierr = find_xguidept(image,x0,y0,xguide,3)) < 0)
      goto no_interpolation;
    *yi = y0;
  } else if (getxx(image) == 1) {
    /* special case: unidirectional interpolation in y */
    unimode = 2;
    if ((ierr = find_yguidept(image,x0,y0,yguide,3)) < 0)
      goto no_interpolation;
    *xi = x0;
  } else {
    /* standard case: bidirectional interpolation */
    if ((ierr = find_guidept(image,x0,y0,XYDIR,xguide,yguide,3)) < 0)
      goto no_interpolation;
  }

  if (!(unimode & 2)) {
    /* x-direction */
    diff0 = abs(xguide[0] - xguide[1]);
    diff1 = abs(xguide[2] - xguide[1]);
    if (diff0 < diff1) {
      *xi = x0 - 0.5 * (1.0 - diff0/diff1);
    } else if (diff0 > diff1) {
      *xi = x0 + 0.5 * (1.0 - diff1/diff0);
    } else { /* diff0 == diff1 */
      *xi = x0;
    }
  }

  if (!(unimode & 1)) {
    diff0 = abs(yguide[0] - yguide[1]);
    diff1 = abs(yguide[2] - yguide[1]);
    if (diff0 < diff1) {
      *yi = y0 - 0.5 * (1.0 - diff0/diff1);
    } else if (diff0 > diff1) {
      *yi = y0 + 0.5 * (1.0 - diff1/diff0);
    } else { /* diff0 == diff1 */
      *yi = y0;
    }
  }

  return(0);

no_interpolation:
    *xi = x0;
    *yi = y0;
    if (ierr == -10)
      errstring = (char *)"extremum on image border, no interpolation possible";
    return(ierr);

} /* int extpos3r() */

int extpos( Timages  *image,
            int16   x0, int16 y0,        /* inaccurate location  */
            char    *mode,                /* interpolation method */
            sfloat  *xi, sfloat *yi)
{
  lfloat    xguide[MAXGUIDE],yguide[MAXGUIDE];
  sfloat    x1, y1, x2, y2, d;
  int16     dmode, ptnum;
  register int ierr, ierrD;

  if (!strcmp(mode,CR_INT4_SC)) {   /* straight cut interpolation */
    return(extpossc(image,x0,y0,mode,xi,yi));
  }
  if (!strcmp(mode,CR_INT3_R)) {    /* neighbour relation interpolation */
    return(extpos3r(image,x0,y0,mode,xi,yi));
  }

  ierrD = 1; /* for error handling of non 'D' modes */

  if      (!strcmp(mode,CR_INT3 ))  { dmode = 0; ptnum = 3; }
  else if (!strcmp(mode,CR_INT3D))  { dmode = 1; ptnum = 3; }
  else if (!strcmp(mode,CR_INT5 ))  { dmode = 0; ptnum = 5; }
  else if (!strcmp(mode,CR_INT5D))  { dmode = 1; ptnum = 5; }
  else if (!strcmp(mode,CR_INT7 ))  { dmode = 0; ptnum = 7; }
  else if (!strcmp(mode,CR_INT7D))  { dmode = 1; ptnum = 7; }
  else {
    errstring = (char *)"this mode is not implemented";  return(-1);
  }

  if ( (!strcmp(mode,CR_INT3)) || (!strcmp(mode,CR_INT3D)) ) {
    /* CR_INT3/CR_INT3D */
    if (getyy(image) == 1) {
      /* special case: unidirectional interpolation in x */
      *yi = y0;
      if ((ierr = find_xguidept(image,x0,y0,xguide,ptnum)) == 0) {
        *xi = x0 + quinter(xguide[0],xguide[1],xguide[2]);
      } else {
        *xi = x0;
      }
    } else if (getxx(image) == 1) {
      /* special case: unidirectional interpolation in y */
      *xi = x0;
      if ((ierr = find_yguidept(image,x0,y0,yguide,ptnum)) == 0) {
        *yi = y0 + quinter(yguide[0],yguide[1],yguide[2]);
      } else {
        *yi = y0;
      }
    } else {
      /* standard case: bidirectional interpolation */
      if ((ierr = find_guidept(image,x0,y0,XYDIR,xguide,yguide,ptnum)) == 0) {
        *xi = x0 + quinter(xguide[0],xguide[1],xguide[2]);
        *yi = y0 + quinter(yguide[0],yguide[1],yguide[2]);
      } else {
        *xi = x0; *yi = y0;
      }
      if (ierr >= 0 && dmode == 1) {
        /* if we had all points with same value for XYDIR, we continue search
           of extremum in DIAGDIR */
        if ((ierrD = find_guidept(image,x0,y0,DIAGDIR,xguide,yguide,ptnum))
          == 0) {
          d = quinter(xguide[0],xguide[1],xguide[2]) / 2;
	  *xi += d;  *yi += d;
          d = quinter(yguide[0],yguide[1],yguide[2]) / 2;
	  *xi += d;  *yi -= d;
        }
      }
    }
  } else {
    if (getyy(image) == 1) {
      /* special case: unidirectional interpolation in x */
      *yi = y0;
      if ((ierr = find_xguidept(image,x0,y0,xguide,ptnum)) == 0) {
        *xi = quintpts(x0,xguide,ptnum);
      } else {
        *xi = x0;
      }
    } else if (getxx(image) == 1) {
      /* special case: unidirectional interpolation in y */
      *xi = x0;
      if ((ierr = find_yguidept(image,x0,y0,yguide,ptnum)) == 0) {
        *yi = quintpts(y0,yguide,ptnum);
      } else {
        *yi = y0;
      }
    } else {
      /* standard case: bidirectional interpolation */
      if ((ierr = find_guidept(image,x0,y0,XYDIR,xguide,yguide,ptnum)) == 0) {
        *xi = quintpts(x0,xguide,ptnum);
        *yi = quintpts(y0,yguide,ptnum);
      } else {
        *xi = x0; *yi = y0;
      }
      if (ierr >= 0 && dmode == 1 ) {
        if ((ierrD = find_guidept(image,x0,y0,DIAGDIR,xguide,yguide,ptnum))
          == 0) {
	  x1 = quintpts(x0,xguide,ptnum);    /* Diagonal NW->SE : incr(x) */
	  y1 = y0 + (x1 - x0);                               /*   incr(y) */
	  x2 = quintpts(x0,yguide,ptnum);    /* Diagonal SW->NE : incr(x) */
	  y2 = y0 - (x2 - x0);                               /*   decr(y) */
	  *xi = (*xi + (x1 + x2)/2) /2;  *yi = (*yi + (y1 + y2)/2) /2;
        }
      }
    }
  }
  if (ierr == 1 && ierrD == 1) {
    ierr = 1;
    errstring = (char *)"all points of same value: no extremum";
  } else if (ierr == -10 || ierrD == -10)
     errstring = (char *)"extremum on image border, no interpolation possible";
  return(ierr);
}

/*=========================================================================*/

int extposv( Timages *image,
             Tvector *vector,
             char    *mode,
             sfloat  *xi,
             sfloat  *yi)
{
  Tffuncval *bestval, *extposval ;
  int16    x, y;
  int      ierr;
  sfloat   fval;

  if(utvcheck(vector,DV_HOST,TY_FUNCVAL)) return(-15);
             /* increase size by 1 */
  if(ve_realloc(vector,(int32)3L,sizeof(Tffuncval),TY_FUNCVAL))
            return(-16);
  bestval = extposval = (Tffuncval *)vgetpm(vector) ;
  fval = bestval->ffuncval;

  extposval += 2 ;
  x = bestval->fxpos;                            /* estimated position */
  y = bestval->fypos;
  if ((ierr=extpos(image,x,y,mode,xi,yi)) < 0) return (ierr) ;
           /* subpixel position unsigned int16 */
  if (ierr > 0)
  {
    extposval->fxpos=MAXINT16;                      /* no extremum found */
    extposval->fypos=MAXINT16;
  }
  else   /* ierr = 0 */
  {
    extposval->fxpos=(int16)(fto32(*xi*16.) & 0xFFFFL); /* fto32 rounds to */
    extposval->fypos=(int16)(fto32(*yi*16.) & 0xFFFFL);     /* nearest int */
  }
  extposval->ffuncval = fval;                     /* same as bestval */
  vputnm(vector,(int32)3L);                       /* put vector length */
  return(ierr);
}

/***************************************************************************/
