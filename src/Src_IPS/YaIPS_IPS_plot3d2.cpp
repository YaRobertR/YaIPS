/****************************************************************************

  YaIPS_IPS_plot3d2.cpp

  3-d-plot of image

 26.02.2025 RR: First edition of this file.

*****************************************************************************
*/

/****************************************************************************
 /@
 /@      ALGORITHM
 /@
 /@      A 3-d-plot of 'srcim' is done into 'dstim'. The plot shows the
 /@      greyvalue over x and y.
 /@      The plot is drawn either as grid plot, (greyvalue) surface plot or
 /@      both.
 /@      Additionally, the greyvalue surface plot can be inverted.
 /@      The greyvalue surface plot is drawn with maximum possible dissolution,
 /@      the grid plot with a dissolution of nx * ny. Between the grid points
 /@      linear interpolated lines are drawn.
 /@      Normal greyvalue surface plot is light at top and dark at bottom,
 /@      inverted greyvalue surface plot dark at top and light at bottom.
 /@      The view direction is defined by azimuth (0-359) and
 /@      elevation (0-90).
 /@      basecolor, gridcolor, backcolor are valable only for grid plot.
 /@      If basecolor or backcolor is `-1`, the base-lines or
 /@      the backgound-lines are not drawn.
 /@      A scaling factor for greyvalue height scaling can be given.
 /@      The plot is drawn directly on the frame buffer.
 /@
 /@      PARAMETERS
 /@      srcim     (int8)   image  (defined on host)
 /@      dstim     (int8)   image  (defined on fb or host)
 /@      nx, ny    (int16)  number of grid lines in x/y direction
 /@      azim      (int16)  view angle horizontal (0..359)
 /@      elev      (int16)  view angle vertical (0..90)
 /@      basecolor (int16)  draw color baseline, front and left side (-1.. )
 /@      gridcolor (int16)  draw color grid (0.. )
 /@      backcolor (int16)  draw color background lines (-1.. )
 /@      scale     (sfloat) scale factor for greyvalue scaling
 /@      grid      (int16)  0: off
 /@                         1: linear interpolated
 /@                         2: exact (not availlable)
 /@      surface   (int16)  0: off
 /@                         1: on
 /@                         2: inverted
 /@
 /@      RESTRICTIONS
 /@
 /@      Note for standalone: For the skyline of the plot memory is allocated.
 /@      Surface plot is allowed for 1 <= elev <= 90 only!!
 /@
 /@
 /@   int plot3d(srcim,dstim,nx,ny,azim,elev,bcol,gcol,lcol,scale)
 /@      Timage  *srcim, *dstim;
 /@      int16 nx, ny, azim, elev, bcol, gcol, lcol;
 /@      sfloat scale;
 /@
 /@      Grid plot:
 /@      Sampling distance is (getxx(srcim)-1)/(nx-1) in x-direction and
 /@      (getyy(srcim)-1)/(ny-1) in y-direction.
 /@      Nearest neighbour interpolation is used.
 /@
 /@   int g_plot3d(srcim,dstim,azim,elev,scale,inv)
 /@     Timages *srcim, *dstim;
 /@     int16 azim, elev;
 /@     sfloat scale;
 /@     int inv;
 /@
 /@     Greyvalue surface plot.
 /@
 /@   RETURN VALUES
 /@   Negative in case of error.
 /@   -2 : "bad image definition of srcim"
 /@   -3 : "bad image definition of dstim"
 /@   -4 : "FB is off"
 /@   -5 : "graphic not initialized"
 /@   -10 : "illegal angle combination"
 /@
 /@ *************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <sysinfoapi.h>

#include "YaIPS.h"
#include "YaIPS_IPS_Interface.h"

#define PREC 16
#define DPREC (double)65536
#define ROUND (int32)32768
#define VALMAX 255
#define BACK_LINES 8

#ifndef MAXFLOAT
#define MAXFLOAT  3.402823466385288598e+38F
#endif
#ifndef MINFLOAT
#define MINFLOAT  1.175494350822287508e-38F
#endif

#define DRAW_IMAGE_INDENT          0     // Indent plot from border
#define PLOT3D_AZIMUT_ANGLE_ADD  270     // Add to azimuth angle to adapt view zero degree angle

// --------------------------------------------------------------------------------------

static int gp_lastx, gp_lasty;         // Last line move points

static uchar *gp_pImgData;              // Pointer to image data
static int   gp_Image_d;
static int   gp_Image_ld;

typedef struct {
  uchar r;
  uchar g;
  uchar b;
} Tplot3DCol;

static Tplot3DCol gp_col;

static uchar *gp_pLookupR, *gp_pLookupG, *gp_pLookupB;          // Gray value to color conversion

typedef struct {
   int rxa;             /* origin */
   int rya;             /* origin */
   int rxx;             /* size   */
   int ryy;             /* size   */
} Tgrect;

static Tgrect gp_clip_region;  // Clip region. Corners of rectangle are inside drawing area.

// setting regions

void gp_srec( Tgrect *r, int x, int y, int xs, int ys)
{
  if(xs < 0 ) { x += xs + 1; xs = - xs; }   /* no neg direction */
  if(ys < 0 ) { y += ys + 1; ys = - ys; }
  r->rxa = x;
  r->rya = y;
  r->rxx = xs;
  r->ryy = ys;
}

void gp_sxrec( Tgrect *r, int x1, int y1, int x2, int y2)
{
  int tmp;

  if(x2 < x1) { tmp = x1; x1 = x2; x2 = tmp; }
  if(y2 < y1) { tmp = y1; y1 = y2; y2 = tmp; }

  r->rxa = x1;
  r->rya = y1;
  r->rxx = x2 - x1 + 1;
  r->ryy = y2 - y1 + 1;
}

// Clipping region things

int gp_pinrec( Tgrect *r, int x, int y)
{
  if((x >= r->rxa          ) &&
     (x <  r->rxa + r->rxx ) &&
     (y >= r->rya          ) &&
     (y <  r->rya + r->ryy ))
     return(TRUE);

  return(FALSE);
}

int gp_touchrec( Tgrect *a, Tgrect *b)
{

  if((a->rxa >= b->rxa + b->rxx ) ||
     (a->rya >= b->rya + b->ryy ) ||
     (a->rxa + a->rxx <= b->rxa ) ||
     (a->rya + a->ryy <= b->rya )) {

       // no touch
       return(FALSE);
  }

  // touch
  return(TRUE);
}

// Drawing functions

static int ppix( int x, int y, int val) /* quicker */
{
#ifdef use_again
  if (!gp_pinrec( &gp_clip_region, x, y)) {
    return (0);
  }
  return (gl_ppix(x + gp_attr.gp_orgx, y + gp_attr.gp_orgy, val));
#else

  uchar *pPixel;

  if (!gp_pinrec( &gp_clip_region, x, y)) {
    return (0);
  }

  pPixel = gp_pImgData + y * gp_Image_ld + x * gp_Image_d;

  pPixel[ 0] = gp_col.r;
  pPixel[ 1] = gp_col.g;
  pPixel[ 2] = gp_col.b;

  return( 0);
#endif
}

// Draw gray value pixel
static int g_ppix( int x, int y, int val) /* quicker */
{
#ifdef use_again
  if (!gp_pinrec( &gp_clip_region, x, y)) {
    return (0);
  }
  return (gl_ppix(x + gp_attr.gp_orgx, y + gp_attr.gp_orgy, val));
#else

  uchar *pPixel;

  if (!gp_pinrec( &gp_clip_region, x, y)) {
    return (0);
  }

  pPixel = gp_pImgData + y * gp_Image_ld + x * gp_Image_d;

  if (val < 0) {            // Clip value
    val = 0;
  }
  if (val > VALMAX) {
    val = VALMAX;
  }

#ifdef use_again
  pPixel[ 0] = val;
  pPixel[ 1] = val;
  pPixel[ 2] = val;
#else
  pPixel[ 0] = gp_pLookupR[ val];
  pPixel[ 1] = gp_pLookupG[ val];
  pPixel[ 2] = gp_pLookupB[ val];
#endif

  return( 0);
#endif
}

/*-------------------------------------------------------------------------*/

static int plot_drchk( int rx1, int ry1, int check, int *psky)
{
  int ierr;

  if( psky == NULL) {     // No skyline

    ierr = ppix(rx1, ry1, 1);

    return( ierr);
  }

  if (check) { /* sky check */
    if (ry1 < *(psky + rx1)) {
      /* draw and store */
      if ((ierr = (ppix(rx1, ry1, 1))))
        return (ierr);
      *(psky + rx1) = ry1;
    }
  } else { /* no sky check, draw and store */
    if ((ierr = ppix(rx1, ry1, 1)))
      return (ierr);
    if (ry1 < *(psky + rx1))
      *(psky + rx1) = ry1;
  }

  return (0);
}

/*-------------------------------------------------------------------------*/

int plot_drawto( int x, int y, int *psky, int check) {
  int ierr;
  int rx1, ry1; /* start point of line      */
  int dx, dy; /* axes of line             */
  int adx, ady; /* distance                 */
  int xa, ya; /* direction                */
  int d; /* distance already done    */
  int incr1, incr2; /* distance per step        */
  Tgrect rec; /* clipping rectangle       */

  PRINTF2("drawto %d %d\n",x, y);

#ifdef use_again
  if (ierr = gl_swmod(gp_attr.lwmod))
    return (ierr); /* set line attr */
  if (ierr = gl_scol0(gp_attr.lcol0))
    return (ierr);
  if (ierr = gl_scol1(gp_attr.lcol1))
    return (ierr);
#endif

  rx1 = gp_lastx; /* start point of line   */
  ry1 = gp_lasty;
  PRINTF2("  starting at %d %d\n",rx1,ry1);
  /* draw first point */
  if ((ierr = plot_drchk(rx1, ry1, check, psky)))
    return (ierr);

  gp_sxrec(&rec, rx1, ry1, x, y);
  if (!gp_touchrec( &gp_clip_region, &rec)) {
    PRINTF0("gp_drawto: clipped\n");
    return (0);
  }

  dx = x - rx1; /* lenght of axes */
  dy = y - ry1;
  if (dx == 0 && dy == 0)
    return (0); /* startpoint == endpoint */

  if (dx < 0) { /* x-direction: backward  */
    xa = -1;
    adx = -dx;
  } else { /* x-direction: forward   */
    xa = 1;
    adx = dx;
  }
  if (dy < 0) { /* negativ y-direction    */
    ya = -1;
    ady = -dy;
  } else { /* positive y-direction   */
    ya = 1;
    ady = dy;
  }
  if (adx > ady) { /* more horizontal than vertical */
    incr1 = ady << 1;
    d = incr1 - adx;
    incr2 = incr1 - (adx << 1);
    while (rx1 != x) { /* while endpoint not reached */
      rx1 += xa;
      if (d < 0) {
        d += incr1;
      } else {
        ry1 += ya;
        d += incr2;
      }
      if ((ierr = plot_drchk(rx1, ry1, check, psky)))
        return (ierr);
    }
  } else { /* more vertical than horizontal */
    incr1 = adx << 1;
    d = incr1 - ady;
    incr2 = incr1 - (ady << 1);
    while (ry1 != y) { /* while endpoint not reached */
      ry1 += ya;
      if (d < 0) {
        d += incr1;
      } else {
        rx1 += xa;
        d += incr2;
      }
      if ((ierr = plot_drchk(rx1, ry1, check, psky)))
        return (ierr);
    }
  }

  gp_lastx = x; /* set endpoint of line     */
  gp_lasty = y;

  /* draw last point */
  if ((ierr = plot_drchk(rx1, ry1, check, psky)))
    return (ierr);

  return (0);
}

/*-------------------------------------------------------------------------*/

static int32 dval;

int g_plot_drawto( int x, int y, int *psky, int val /*, Timages *dstim*/)
{
  int yp, ye;
  int vprec;

  ye = y;

  yp = gp_lasty;
  if (yp > *psky)
    yp = *psky;

  /* PRINTF2("yp %d val %d\n", yp, val); */
  vprec = (val << PREC);
  while (ye < yp) {

    val = ((vprec + ROUND ) >> PREC);

    if (val < 0) {            // Clip value
      val = 0;
    }
    if (val > VALMAX) {
      val = VALMAX;
    }

    if (ye >= 0) {

      g_ppix( x, ye, val);
    }
    ye++;
    vprec -= dval;

  }
  if ((y < *psky) && (y >= 0))
    *psky = y; /* store new skyline value */

  return (0);
}

/*-------------------------------------------------------------------------*/

int plot_moveto( int x, int y) {

  gp_lastx = x;
  gp_lasty = y;
  return (0);
}

/*-------------------------------------------------------------------------
 * Greyvalue surface plot
*/

int g_plot3d( Timages *srcim,
              Fl_RGB_Image *pDrawImage,                   // Draw into this image
              Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,       // Only used for StrDebug
              int azim, int elev, sfloat scale, int inv)
{

  int *skyline = NULL, *lastline = NULL, *firstline = NULL, *baseline = NULL;;
  int *psky, *plast;
  int8 *psim;                          /* pointer to image data */
  int x, y;
  /* ----- source image ------------- */
  int dxs, dys;                              /* increment */
  int xposs, yposs;                          /* subpixel prec. position */
  int xxs, yys;
  int xps, yps;                              /* position */
  /* ----- destination image ------------- */
  int xposdini, yposdini;                    /* begin of line */
  int dxdi, dydi;                            /* for a step in inner loop */
  int dxdo, dydo;                            /* for a step in outher loop */
  int xposd, yposd;                          /* subpixel prec. position */
  int xxd, yyd;
  int xpd, ypd;                              /* position */

  double x0,y0,y2,x3,y3,x1,y1;                 /* factors of coordinates */
  double azimb,elevb,sinazim,cosazim,sinelev,coselev;

  int view;
  int scale16;                               /* scale with 16 bit prec */
  int scalefactor;
  int incr;
  int nx, ny;
  int val;                                   /* value */
  int ierr;

  ierr = 0;

  if(uticheck(srcim, DV_HOST,TY_BYTE)) return(-2);
  //x/if(uticheck(dstim, DV_HOST|DV_FBIML,TY_BYTE)) return(-3);
  //x/if (fbisoff()) return(-4);

  //x/if ((!gr_iniflg) && gl_bpmask) drawmask = gl_bpimask;
  //x/else drawmask = 0xffff;

  xxs = getxx(srcim);
  yys = getyy(srcim);

  xxd = pDrawImage->data_w() - DRAW_IMAGE_INDENT * 2;
  yyd = pDrawImage->data_h() - DRAW_IMAGE_INDENT * 2;

#ifdef use_again
  // make it square

  if( xxs >= yys) {

    xxd = (xxd * yys) / xxs;
  } else {

    yyd = (yyd * xxs) / yys;
  }
#endif

  /* conversion of angles */
  view = azim / 90;
  azim = azim % 90;
  PRINTF2("\n Azimuth-Winkel: %d  View: %d\n",azim,view);
  azimb = (((double)azim * M_PI)/180.);
  elevb = (((double)elev * M_PI)/180.);
  cosazim = cos(azimb);
  sinazim = sin(azimb);
  coselev = cos(elevb);
  sinelev = sin(elevb);

  /* calculate factors of basepoints */
  x0 = cosazim/(sinazim+cosazim);
  y0 = 1.;
  x1 = sinazim/(sinazim+cosazim);
  y1 = sinelev/(sinelev+coselev);
  y2 = x1 * y1;  /* x2 = 0.0 */
  x3 = 1.;
  y3 = x0 * y1;

  y1 = 1. - y1;
  y2 = 1. - y2;
  y3 = 1. - y3;

  /* define greyvalue scalefactor */
  scalefactor = dto32(((y1 * (double)yyd) * DPREC)/(double)(VALMAX + 1));
  PRINTF2("yyd %d y1 %lf\n", yyd, y1);
  if (elev >= 90) dval = VALMAX * DPREC;
  else dval = dto32(((double)(VALMAX + 1) * DPREC)/((double)yyd * y1));
  //x/if (inv) dval = -dval;
  PRINTF2("scalefactor %ld dval %ld\n",scalefactor, dval);

  scale16 = fto32(scale * DPREC);

  /* define and init skyline */

  skyline = (int*) malloc( xxd * sizeof(int));
  if (skyline == NULL)
    goto error;
  psky = skyline;

  for (x = 0; x < xxd; x++) {
    *psky++ = (yyd - 1);
  }

  psky = skyline;

  incr = (int32)getxm(srcim);

  /* define and init lastline-buffer */

  lastline = (int*) malloc( xxd * sizeof(int));
  if (lastline == NULL)
    goto error2;
  plast = lastline;

  for (x = 0; x < xxd; x++) {
    *plast++ = -1;
  }

  plast = lastline;

  /* define and init firstline-buffer */

  firstline = (int*) malloc( xxd * sizeof(int));
  if (firstline == NULL)
    goto error2;

  for (x = 0; x < xxd; x++) {
    *(firstline + x) = -1;
  }

  /* define and init baseline-buffer */

  baseline = (int*) malloc( xxd * sizeof(int));
  if (baseline == NULL)
    goto error2;

  for (x = 0; x < xxd; x++) {
    *(baseline + x) = -1;
  }

  /* define startpoint and dx/y in dstim */
  xposdini = dto32((double)(xxd - 1) * x0 * DPREC);
  yposdini = ((int32)(yyd - 1)) << PREC;

  gp_srec( &gp_clip_region, 0, 0, pDrawImage->data_w(), pDrawImage->data_h());

  gp_pImgData = (uchar *)pDrawImage->data()[ 0];
  gp_Image_d  = pDrawImage->d();
  gp_Image_ld = pDrawImage->ld() ? pDrawImage->ld() : pDrawImage->data_w() * pDrawImage->d();

  // Indent from Border

  gp_pImgData += gp_Image_ld * DRAW_IMAGE_INDENT + gp_Image_d * DRAW_IMAGE_INDENT;

  // ...

  switch (view) {
  case 0: /* look from 0 - 89 degrees */
    dxdi = dto32((double) (xxd - 1) * (x3 - x0) * DPREC);
    dydi = dto32((double) (yyd - 1) * (y0 - y3) * DPREC);
    dxdo = dto32((double) (xxd - 1) * x0 * DPREC);
    dydo = dto32((double) (yyd - 1) * (y0 - y2) * DPREC);

    PRINTF4("dxdo %ld dydo %ld dxdi %ld dydi %ld\n", dxdo, dydo, dxdi, dydi);
    if (dxdo > dydo) {
      dydo = dto32((double) dydo / ((double) dxdo / DPREC ));
      dxdo = (int32) DPREC;
      ny = dto16((double) (xxd - 1) * x0) + 1;
    } else if (dxdo < dydo) {
      dxdo = dto32((double) dxdo / ((double) dydo / DPREC ));
      dydo = (int32) DPREC;
      ny = dto16((double) (yyd - 1) * (y0 - y2)) + 1;
    } else {
      ierr = -10;
      goto error2;
    }
    if (dxdi > dydi) {
      dydi = dto32((double) dydi / ((double) dxdi / DPREC ));
      dxdi = (int32) DPREC;
      nx = dto16((double) (xxd - 1) * (x3 - x0)) + 1;
    } else if (dydi > dxdi) {
      dxdi = dto32((double) dxdi / ((double) dydi / DPREC ));
      dydi = (int32) DPREC;
      nx = dto16((double) (yyd - 1) * (y0 - y3)) + 1;
    } else {
      ierr = -10;
      goto error2;
    }

    PRINTF2("nx %d ny %d\n", nx, ny);
    dxs = ((int32) (xxs - 1) << PREC) / (int32) (nx - 1);
    dys = ((int32) (yys - 1) << PREC) / (int32) (ny - 1);

    yposs = 0L;
    for (y = 0; y < ny; y++, yposs += dys, xposdini -= dxdo, yposdini -= dydo) {

      //x/if (iabort()) goto end;

      /* init positions on linebegin */
      xposs = (int32) (xxs - 1) << PREC;
      xposd = xposdini;
      yposd = yposdini;

      psim = (int8*) pixad( ((xposs + ROUND) >> PREC),
                            ((yposs + ROUND) >> PREC), srcim);

      for (x = nx - 1; x >= 0;
          x--, xposs -= dxs, xposd += dxdi, yposd -= dydi, psim -= (xps
              - ((xposs + ROUND ) >> PREC))) {

        /* get position in source */
        xps = ((xposs + ROUND ) >> PREC);

        if (scale16 >= 0L)
          val = ((int32) btoi(*psim) * scale16 + ROUND ) >> PREC;
        else
          val = ((((int32) btoi(*psim) - (int32) VALMAX) * scale16 + ROUND )
              >> PREC);

        if (inv)
          val = VALMAX - val;

        /* get position in destination */
        xpd = ((xposd + ROUND ) >> PREC);

        if( xpd >= xxd) {                // Clip to maximum
          xpd = xxd - 1;
        }

        PRINTF2("gw %d val %d\n", (int)btoi(*psim), (int)val);
        ypd = ((yposd - val * scalefactor + ROUND ) >> PREC);

        /* draw greyvalues */
        if (*(plast + xpd) < 0) {
          /* first entry in this column, so draw point only */
          if (ypd > 0) {
            g_ppix(xpd, ypd, val);
          }

          // Remember first point drawn
          if( *(firstline + xpd) < 0) {

            *(firstline + xpd) = ypd;
            *(baseline + xpd)  = ((yposd + ROUND ) >> PREC);
          }
        } else {
          /* draw tower from last point in this column to actual point */
          plot_moveto(xpd, *(plast + xpd));
          g_plot_drawto(xpd, ypd, psky + xpd, val /*, dstim*/);
        }

        /* store position in destination */
        *(plast + xpd) = ypd;

      }
    }
    break;
  case 1: /* look from 90 - 179 degrees */
    dxdi = dto32((double) (xxd - 1) * (x3 - x0) * DPREC);
    dydi = dto32((double) (yyd - 1) * (y0 - y3) * DPREC);
    dxdo = dto32((double) (xxd - 1) * x0 * DPREC);
    dydo = dto32((double) (yyd - 1) * (y0 - y2) * DPREC);

    if (dxdo > dydo) {
      dydo = dto32((double) dydo / ((double) dxdo / DPREC ));
      dxdo = (int32) DPREC;
      nx = dto16((double) (xxd - 1) * x0) + 1;
    } else if (dydo > dxdo) {
      dxdo = dto32((double) dxdo / ((double) dydo / DPREC ));
      dydo = (int32) DPREC;
      nx = dto16((double) (yyd - 1) * (y0 - y2)) + 1;
    } else {
      ierr = -10;
      goto error2;
    }
    if (dxdi > dydi) {
      dydi = dto32((double) dydi / ((double) dxdi / DPREC ));
      dxdi = (int32) DPREC;
      ny = dto16((double) (xxd - 1) * (x3 - x0)) + 1;
    } else if (dydi > dxdi) {
      dxdi = dto32((double) dxdi / ((double) dydi / DPREC ));
      dydi = (int32) DPREC;
      ny = dto16((double) (yyd - 1) * (y0 - y3)) + 1;
    } else {
      ierr = -10;
      goto error2;
    }

    dxs = ((int32) (xxs - 1) << PREC) / (int32) (nx - 1);
    dys = ((int32) (yys - 1) << PREC) / (int32) (ny - 1);

    xposs = 0L;
    for (x = 0; x < nx; x++, xposs += dxs, xposdini -= dxdo, yposdini -= dydo) {

      //x/if (iabort()) goto end;

      /* init positions on linebegin */
      yposs = 0L;
      xposd = xposdini;
      yposd = yposdini;

      psim = (int8*) pixad( ((xposs + ROUND) >> PREC),
                            ((yposs + ROUND) >> PREC), srcim);

      for (y = 0; y < ny;
          y++, yposs += dys, xposd += dxdi, yposd -= dydi, psim +=
              (((yposs + ROUND ) >> PREC) - yps) * incr) {

        /* get position in source */
        yps = ((yposs + ROUND ) >> PREC);

        /* get position in destination */
        xpd = ((xposd + ROUND ) >> PREC);

        if( xpd >= xxd) {                // Clip to maximum
          xpd = xxd - 1;
        }

        if (scale16 >= 0L)
          val = ((int32) btoi(*psim) * scale16 + ROUND ) >> PREC;
        else
          val = ((((int32) btoi(*psim) - (int32) VALMAX) * scale16 + ROUND )
              >> PREC);

        if (inv)
          val = VALMAX - val;

        ypd = ((yposd - val * scalefactor + ROUND ) >> PREC);

        /* draw greyvalue point or tower */
        if (*(plast + xpd) < 0) {
          /* first entry in this column, so draw point only */
          if (ypd > 0) {
            g_ppix(xpd, ypd, val);
          }

          // Remember first point drawn
          if( *(firstline + xpd) < 0) {

            *(firstline + xpd) = ypd;
            *(baseline + xpd)  = ((yposd + ROUND ) >> PREC);
          }
        } else {
          /* draw tower from last point in this column to actual point */
          plot_moveto(xpd, *(plast + xpd));
          g_plot_drawto(xpd, ypd, psky + xpd, val /*, dstim*/);
        }

        /* store position in destination */
        *(plast + xpd) = ypd;

      }
    }
    break;
  case 2: /* look from 180 - 269 degrees */
    dxdi = dto32((double) (xxd - 1) * (x3 - x0) * DPREC);
    dydi = dto32((double) (yyd - 1) * (y0 - y3) * DPREC);
    dxdo = dto32((double) (xxd - 1) * x0 * DPREC);
    dydo = dto32((double) (yyd - 1) * (y0 - y2) * DPREC);

    if (dxdo > dydo) {
      dydo = dto32((double) dydo / ((double) dxdo / DPREC ));
      dxdo = (int32) DPREC;
      ny = dto16((double) (xxd - 1) * x0) + 1;
    } else if (dydo > dxdo) {
      dxdo = dto32((double) dxdo / ((double) dydo / DPREC ));
      dydo = (int32) DPREC;
      ny = dto16((double) (yyd - 1) * (y0 - y2)) + 1;
    } else {
      ierr = -10;
      goto error2;
    }
    if (dxdi > dydi) {
      dydi = dto32((double) dydi / ((double) dxdi / DPREC ));
      dxdi = (int32) DPREC;
      nx = dto16((double) (xxd - 1) * (x3 - x0)) + 1;
    } else if (dydi > dxdi) {
      dxdi = dto32((double) dxdi / ((double) dydi / DPREC ));
      dydi = (int32) DPREC;
      nx = dto16((double) (yyd - 1) * (y0 - y3)) + 1;
    } else {
      ierr = -10;
      goto error2;
    }

    dxs = ((int32) (xxs - 1) << PREC) / (int32) (nx - 1);
    dys = ((int32) (yys - 1) << PREC) / (int32) (ny - 1);

    yposs = (int32) (yys - 1) << PREC;
    for (y = (ny - 1); y >= 0; y--, yposs -= dys, xposdini -= dxdo, yposdini -=
        dydo) {

      //x/if (iabort()) goto end;

      /* init positions on linebegin */
      xposs = 0L;
      xposd = xposdini;
      yposd = yposdini;

      psim = (int8*) pixad( ((xposs + ROUND) >> PREC),
                            ((yposs + ROUND) >> PREC), srcim);

      for (x = 0; x < nx;
          x++, xposs += dxs, xposd += dxdi, yposd -= dydi, psim +=
              (((xposs + ROUND ) >> PREC) - xps)) {

        /* get position in source */
        xps = ((xposs + ROUND ) >> PREC);

        /* get position in destination */
        xpd = ((xposd + ROUND ) >> PREC);

        if( xpd >= xxd) {                // Clip to maximum
          xpd = xxd - 1;
        }

        if (scale16 >= 0L)
          val = ((int32) btoi(*psim) * scale16 + ROUND ) >> PREC;
        else
          val = ((((int32) btoi(*psim) - (int32) VALMAX) * scale16 + ROUND )
              >> PREC);

        if (inv)
          val = VALMAX - val;

        ypd = ((yposd - val * scalefactor + ROUND ) >> PREC);

        /* draw greyvalue point or tower */
        if (*(plast + xpd) < 0) {
          /* first entry in this column, so draw point only */
          if (ypd > 0) {
            g_ppix(xpd, ypd, val);
          }

          // Remember first point drawn
          if( *(firstline + xpd) < 0) {

            *(firstline + xpd) = ypd;
            *(baseline + xpd)  = ((yposd + ROUND ) >> PREC);
          }
        } else {
          /* draw tower from last point in this column to actual point */
          plot_moveto(xpd, *(plast + xpd));
          g_plot_drawto(xpd, ypd, psky + xpd, val /*, dstim*/);
        }

        /* store position in destination */
        *(plast + xpd) = ypd;

      }
    }
    break;
  case 3: /* look from 270 - 359 degrees */
    dxdi = dto32((double) (xxd - 1) * (x3 - x0) * DPREC);
    dydi = dto32((double) (yyd - 1) * (y0 - y3) * DPREC);
    dxdo = dto32((double) (xxd - 1) * x0 * DPREC);
    dydo = dto32((double) (yyd - 1) * (y0 - y2) * DPREC);

    if (dxdo > dydo) {
      dydo = dto32((double) dydo / ((double) dxdo / DPREC ));
      dxdo = (int32) DPREC;
      nx = dto16((double) (xxd - 1) * x0) + 1;
    } else if (dydo > dxdo) {
      dxdo = dto32((double) dxdo / ((double) dydo / DPREC ));
      dydo = (int32) DPREC;
      nx = dto16((double) (yyd - 1) * (y0 - y2)) + 1;
    } else {
      ierr = -10;
      goto error2;
    }
    if (dxdi > dydi) {
      dydi = dto32((double) dydi / ((double) dxdi / DPREC ));
      dxdi = (int32) DPREC;
      ny = dto16((double) (xxd - 1) * (x3 - x0)) + 1;
    } else if (dydi > dxdi) {
      dxdi = dto32((double) dxdi / ((double) dydi / DPREC ));
      dydi = (int32) DPREC;
      ny = dto16((double) (yyd - 1) * (y0 - y3)) + 1;
    } else {
      ierr = -10;
      goto error2;
    }

    dxs = ((int32) (xxs - 1) << PREC) / (int32) (nx - 1);
    dys = ((int32) (yys - 1) << PREC) / (int32) (ny - 1);

    xposs = (int32) (xxs - 1) << PREC;
    for (x = (nx - 1); x >= 0; x--, xposs -= dxs, xposdini -= dxdo, yposdini -=
        dydo) {

      //x/if (iabort()) goto end;

      /* init positions on linebegin */
      yposs = (int32) (yys - 1) << PREC;
      xposd = xposdini;
      yposd = yposdini;

      psim = (int8*) pixad( ((xposs + ROUND) >> PREC),
                            ((yposs + ROUND) >> PREC), srcim);

      for (y = (ny - 1); y >= 0; y--, yposs -= dys, xposd += dxdi, yposd -=
          dydi, psim -= (yps - ((yposs + ROUND ) >> PREC)) * incr) {

        /* get position in source */
        yps = ((yposs + ROUND ) >> PREC);

        /* get position in destination */
        xpd = ((xposd + ROUND ) >> PREC);

        if( xpd >= xxd) {                // Clip to maximum
          xpd = xxd - 1;
        }

        if (scale16 >= 0L)
          val = ((int32) btoi(*psim) * scale16 + ROUND ) >> PREC;
        else
          val = ((((int32) btoi(*psim) - (int32) VALMAX) * scale16 + ROUND )
              >> PREC);

        if (inv)
          val = VALMAX - val;

        ypd = ((yposd - val * scalefactor + ROUND ) >> PREC);

        /* draw greyvalue point or tower */
        if (*(plast + xpd) < 0) {
          /* first entry in this column, so draw point only */
          if (ypd > 0) {
            g_ppix(xpd, ypd, val);
          }

          // Remember first point drawn
          if( *(firstline + xpd) < 0) {

            *(firstline + xpd) = ypd;
            *(baseline + xpd)  = ((yposd + ROUND ) >> PREC);
          }
        } else {
          /* draw tower from last point in this column to actual point */
          plot_moveto(xpd, *(plast + xpd));
          g_plot_drawto(xpd, ypd, psky + xpd, val /*, dstim*/);
        }

        /* store position in destination */
        *(plast + xpd) = ypd;

      }
    }
    break;
  } /* end of switch */

#ifndef use_again
  // Draw outside lines

  // draw color side
  gp_col.r = 128;
  gp_col.g = 128;
  gp_col.b =   0;

  int p1x;

  p1x = dto16((double) (xxd - 1) * (1.0 - x1));

  for( x = 0; x < xxd; x++) {

    if( *(firstline + x) >= 0 &&  *(baseline + x) >= 0) {   // Have an entry

      if( x >= p1x && gp_col.r != 80) {

        gp_col.r = 80;
        gp_col.g = 80;
        gp_col.b =  0;
      }

      plot_moveto( x, *(firstline + x));
      plot_drawto( x, *(baseline + x), NULL, 1);
    }
  }
#endif

//x/end:

  ierr = 0;

error2:

  if( lastline != NULL) {

    free( lastline);
  }

  if( firstline != NULL) {

    free( firstline);
  }

error:

  if( skyline != NULL) {

    free( skyline);
  }

   return(ierr);
}

/*-------------------------------------------------------------------------
 * Grid plot
*/

int plot3d( Timages *srcim,
            Fl_RGB_Image *pDrawImage,              // Draw into this image
            int nx, int ny, int azim,
            int elev /*, int16 bcol, int16 gcol, int16 lcol*/, sfloat scale, int DrawBackLines)
{
  Tplot3DCol bcol = { 255,   0,   0};  // draw color baseline, front and left side (-1.. )
  Tplot3DCol gcol = {   0, 255,   0};  // draw color grid (0.. )
  Tplot3DCol lcol = {   0,   0, 255};  // draw color background lines (-1.. )

  int8 *psim;                          /* pointer to image data */
  int *skyline = NULL, *lastline = NULL;
  int *psky, *plast;
  int x, y;
  /* ----- source image ------------- */
  int xxs, yys; /* size */
  int dxs, dys; /* increment */
  int xposs, yposs; /* subpixel prec. position */
  int xps, yps; /* position */
  /* ----- destination image ------------- */
  int xxd, yyd; /* step */
  int dxdi, dydi; /* for a step in inner loop */
  int dxdo, dydo; /* for a step in outher loop */
  int xposdini, yposdini; /* begin of line */
  int xposd, yposd; /* subpixel prec. position */
  int xpd, ypd; /* position */
  int view;
  sfloat steig1, steig2;

  int scale16; /* scale with 16 bit prec */
  int scalefactor;
  int val; /* value */

  int dist_bline, i; /* background lines */

  double azimb, elevb, sinazim, cosazim, sinelev, coselev;
  double x0, y0, x1, y1, y2, x3, y3; /* factors of coordinates */
  int p1x, p1y, p2y, p3y; /* pos of corners */

  int incr;
  int ierr;

  ierr = 0;

  if (uticheck(srcim, DV_HOST, TY_BYTE)) return (-2);
  //x/if(uticheck(dstim, DV_HOST|DV_FBIML,TY_BYTE)) return(-3);
  //x/if (fbisoff()) return(-4);
  //x/if (gr_iniflg) return(-5);

  xxs = getxx(srcim);
  yys = getyy(srcim);

  xxd = pDrawImage->data_w() - DRAW_IMAGE_INDENT * 2;
  yyd = pDrawImage->data_h() - DRAW_IMAGE_INDENT * 2;

  dxs = ((int32) (xxs - 1) << PREC) / (int32) (nx - 1);
  dys = ((int32) (yys - 1) << PREC) / (int32) (ny - 1);

  /* conversion of angles */
  view = azim / 90;
  azim = azim % 90;
  PRINTF2("\n Azimuth-Winkel: %d  View: %d\n",azim,view);
  azimb = (((double) azim * M_PI) / 180.);
  elevb = (((double) elev * M_PI) / 180.);
  cosazim = cos(azimb);
  sinazim = sin(azimb);
  coselev = cos(elevb);
  sinelev = sin(elevb);

  /* calculate factors of basepoints */
  x0 = cosazim / (sinazim + cosazim);
  y0 = 1.;
  x1 = sinazim / (sinazim + cosazim);
  y1 = sinelev / (sinelev + coselev);
  y2 = x1 * y1; /* x2 = 0.0 */
  x3 = 1.;
  y3 = x0 * y1;

  y1 = 1. - y1;
  y2 = 1. - y2;
  y3 = 1. - y3;

  /* define scalefactor */
  scale16 = fto32(scale * DPREC);
  scalefactor = dto32(((y1 * (double) yyd) * DPREC ) / (double) (VALMAX + 1));
  PRINTF1("scalefactor %ld\n",scalefactor);

  /* define and init skyline */

  skyline = (int*) malloc( xxd * sizeof(int));
  if (skyline == NULL)
    goto error;
  psky = skyline;

  for (x = 0; x < xxd; x++) {
    *psky++ = yyd - 1;
  }

  psky = skyline;

  incr = (int32) getxm(srcim);

  /* define and init lastline-buffer */
  x = nx;
  if (ny > x)
    x = ny;

  lastline = (int*) malloc(x * 2 * sizeof(int));
  if (lastline == NULL)
    goto error2;

  /* define startpoint and dx/y in dstim */
  xposdini = dto32((double) (xxd - 1) * x0 * DPREC);
  yposdini = ((int32) (yyd - 1)) << PREC;

#ifdef use_again
  gp_begin();
  gp_org(getxa(dstim),getya(dstim));
  gp_clip(0,0,xxd-1,yyd-1);
#else

  gp_srec( &gp_clip_region, 0, 0, pDrawImage->data_w(), pDrawImage->data_h());

  gp_pImgData = (uchar *)pDrawImage->data()[ 0];
  gp_Image_d  = pDrawImage->d();
  gp_Image_ld = pDrawImage->ld() ? pDrawImage->ld() : pDrawImage->data_w() * pDrawImage->d();

  // Indent from Border

  gp_pImgData += gp_Image_ld * DRAW_IMAGE_INDENT + gp_Image_d * DRAW_IMAGE_INDENT;

#endif
  p1x = dto16((double) (xxd - 1) * x1);
  p1y = dto16((double) (yyd - 1) * y1);
  p2y = dto16((double) (yyd - 1) * y2);
  p3y = dto16((double) (yyd - 1) * y3);

  // Check: raw background lines only

  if( DrawBackLines) {

    goto DrawBackgroundLines;
  }

  /* draw baselines */
  gp_col = bcol;

  plot_moveto(0, p2y);
  plot_drawto( ((xposdini + ROUND ) >> PREC),
               ((yposdini + ROUND ) >> PREC), psky, 0);
  plot_drawto(dto16((double) (xxd - 1)), p3y, psky, 0);

  gp_col = gcol;

  plot_moveto( ((xposdini + ROUND ) >> PREC),
               ((yposdini + ROUND ) >> PREC));

  switch (view) {
  case 0: /* look from 0 - 89 degrees */
    dxdi = dto32((double) (xxd - 1) * (x3 - x0) * DPREC) / (int32) (nx - 1);
    dydi = dto32((double) (yyd - 1) * (y0 - y3) * DPREC) / (int32) (nx - 1);
    dxdo = dto32((double) (xxd - 1) * x0 * DPREC) / (int32) (ny - 1);
    dydo = dto32((double) (yyd - 1) * (y0 - y2) * DPREC) / (int32) (ny - 1);

    yposs = 0L;
    for (y = 0; y < ny; y++, yposs += dys, xposdini -= dxdo, yposdini -= dydo) {

#ifdef use_again
      if (iabort())
        goto end;
#endif

      /* init positions on linebegin */
      xposs = (int32) (xxs - 1) << PREC;
      xposd = xposdini;
      yposd = yposdini;

      /* init pointers on linebegin */
      plast = lastline;
      psim = (int8*) pixad( ((xposs + ROUND) >> PREC),
                            ((yposs + ROUND) >> PREC), srcim);

      for (x = nx - 1; x >= 0;
          x--, xposs -= dxs, xposd += dxdi, yposd -= dydi, psim -= (xps
              - ((xposs + ROUND ) >> PREC))) {

        /* get position in source */
        xps = ((xposs + ROUND ) >> PREC);
        yps = ((yposs + ROUND ) >> PREC);

        /* get position in destination */
        xpd = ((xposd + ROUND ) >> PREC);
        ypd = ((yposd + ROUND ) >> PREC);

        /* draw vertical lines too */
        if (x == (nx - 1) || y == 0)
          plot_moveto(xpd, ypd);

        if (scale16 >= 0L)
          val = ((int32) btoi(*psim) * scale16 + ROUND ) >> PREC;
        else
          val = ((((int32) btoi(*psim) - (int32) VALMAX) * scale16 - ROUND )
              >> PREC);

        ypd -= ((val * scalefactor + ROUND ) >> PREC);

        /* draw grid lines */
        if (y > 0) {
          if (x < (nx - 1)) {
            /* draw additionally line to last point */
            if (xpd == *(plast - 2))
              steig1 = MAXFLOAT;
            else
              steig1 = (sfloat) (ypd - *(plast - 1))
                  / (sfloat) (xpd - *(plast - 2));
            if (steig1 < 0.0)
              steig1 = -steig1;
            if (xpd == *(plast))
              steig2 = MAXFLOAT;
            else
              steig2 = (sfloat) (*(plast + 1) - ypd)
                  / (sfloat) (*(plast) - xpd);
            if (steig2 < 0.0)
              steig2 = -steig2;
            if (steig2 > steig1) {
              if (ypd <= *(plast + 1)) { /* draw upstairs with check */
                plot_moveto(*plast, *(plast + 1));
                plot_drawto(xpd, ypd, psky, 1);
              } else {
                plot_moveto(xpd, ypd);
                plot_drawto(*plast, *(plast + 1), psky, 1);
              }
              if (*(plast - 1) <= ypd) { /* draw upstairs */
                plot_moveto(xpd, ypd);
                plot_drawto(*(plast - 2), *(plast - 1), psky, 1);
              } else {
                plot_moveto(*(plast - 2), *(plast - 1));
                plot_drawto(xpd, ypd, psky, 1);
              }
            } else {
              if (*(plast - 1) <= ypd) { /* draw upstairs */
                plot_moveto(xpd, ypd);
                plot_drawto(*(plast - 2), *(plast - 1), psky, 1);
              } else {
                plot_moveto(*(plast - 2), *(plast - 1));
                plot_drawto(xpd, ypd, psky, 1);
              }
              if (ypd <= *(plast + 1)) { /* draw upstairs with check */
                plot_moveto(*plast, *(plast + 1));
                plot_drawto(xpd, ypd, psky, 1);
              } else {
                plot_moveto(xpd, ypd);
                plot_drawto(*plast, *(plast + 1), psky, 1);
              }
            }
          } else {
            /* store for background lines */

            if( (bcol.r | bcol.g | bcol.b) != 0) {

              gp_col = bcol;

              plot_drawto(xpd, ypd, psky, 1);

              gp_col = gcol;

            } else {
              plot_moveto(xpd, ypd);
            }
            plot_drawto(*plast, *(plast + 1), psky, 0);
          }
        } else { /* first line */
          if( (bcol.r | bcol.g | bcol.b) != 0) {

            gp_col = bcol;

            plot_drawto(xpd, ypd, psky, 1);

            gp_col = gcol;

          } else {
            plot_moveto(xpd, ypd);
          }
          if (x < (nx - 1)) {

            /* draw additionally line to last point */
            plot_drawto(*(plast - 2), *(plast - 1), psky, 0);
          }
        }

        /* store position in destination */
        *plast++ = xpd;
        *plast++ = ypd;

      }
    }
    break;
  case 1: /* look from 90 - 179 degrees */
    dxdi = dto32((double) (xxd - 1) * (x3 - x0) * DPREC) / (int32) (ny - 1);
    dydi = dto32((double) (yyd - 1) * (y0 - y3) * DPREC) / (int32) (ny - 1);
    dxdo = dto32((double) (xxd - 1) * x0 * DPREC) / (int32) (nx - 1);
    dydo = dto32((double) (yyd - 1) * (y0 - y2) * DPREC) / (int32) (nx - 1);

    xposs = 0L;
    for (x = 0; x < nx; x++, xposs += dxs, xposdini -= dxdo, yposdini -= dydo) {

#ifdef use_again
      if (iabort())
        goto end;
#endif

      /* init positions on linebegin */
      yposs = 0L;
      xposd = xposdini;
      yposd = yposdini;

      /* init pointers on linebegin */
      plast = lastline;
      psim = (int8 *)pixad( ((xposs + ROUND) >> PREC),
                            ((yposs + ROUND) >> PREC), srcim);

      for (y = 0; y < ny;
          y++, yposs += dys, xposd += dxdi, yposd -= dydi, psim +=
              (((yposs + ROUND ) >> PREC) - yps) * incr) {

        /* get position in source */
        xps = ((xposs + ROUND ) >> PREC);
        yps = ((yposs + ROUND ) >> PREC);

        /* get position in destination */
        xpd = ((xposd + ROUND ) >> PREC);
        ypd = ((yposd + ROUND ) >> PREC);

        /* draw vertical lines too */
        if (x == 0 || y == 0)
          plot_moveto(xpd, ypd);

        if (scale16 >= 0L)
          val = ((int32) btoi(*psim) * scale16 + ROUND ) >> PREC;
        else
          val = ((((int32) btoi(*psim) - (int32) VALMAX) * scale16 - ROUND )
              >> PREC);

        ypd -= ((val * scalefactor + ROUND ) >> PREC);

        /* draw grid lines */
        if (x > 0) {
          if (y > 0) {
            /* draw additionally line to last point */
            if (xpd == *(plast - 2))
              steig1 = MAXFLOAT;
            else
              steig1 = (sfloat) (ypd - *(plast - 1))
                  / (sfloat) (xpd - *(plast - 2));
            if (steig1 < 0.0)
              steig1 = -steig1;
            if (xpd == *(plast))
              steig2 = MAXFLOAT;
            else
              steig2 = (sfloat) (*(plast + 1) - ypd)
                  / (sfloat) (*(plast) - xpd);
            if (steig2 < 0.0)
              steig2 = -steig2;
            if (steig2 > steig1) {
              if (ypd <= *(plast + 1)) { /* draw upstairs with check */
                plot_moveto(*plast, *(plast + 1));
                plot_drawto(xpd, ypd, psky, 1);
              } else {
                plot_moveto(xpd, ypd);
                plot_drawto(*plast, *(plast + 1), psky, 1);
              }
              if (*(plast - 1) <= ypd) { /* draw upstairs */
                plot_moveto(xpd, ypd);
                plot_drawto(*(plast - 2), *(plast - 1), psky, 1);
              } else {
                plot_moveto(*(plast - 2), *(plast - 1));
                plot_drawto(xpd, ypd, psky, 1);
              }
            } else {
              if (*(plast - 1) <= ypd) { /* draw upstairs */
                plot_moveto(xpd, ypd);
                plot_drawto(*(plast - 2), *(plast - 1), psky, 1);
              } else {
                plot_moveto(*(plast - 2), *(plast - 1));
                plot_drawto(xpd, ypd, psky, 1);
              }
              if (ypd <= *(plast + 1)) { /* draw upstairs with check */
                plot_moveto(*plast, *(plast + 1));
                plot_drawto(xpd, ypd, psky, 1);
              } else {
                plot_moveto(xpd, ypd);
                plot_drawto(*plast, *(plast + 1), psky, 1);
              }
            }
          } else {
            /* store for background lines */

            if( (bcol.r | bcol.g | bcol.b) != 0) {

              gp_col = bcol;

              plot_drawto(xpd, ypd, psky, 1);

              gp_col = gcol;

            } else {
              plot_moveto(xpd, ypd);
            }
            plot_drawto(*plast, *(plast + 1), psky, 0);
          }
        } else { /* first line */
          if( (bcol.r | bcol.g | bcol.b) != 0) {

            gp_col = bcol;

            plot_drawto(xpd, ypd, psky, 1);

            gp_col = gcol;

          } else {
            plot_moveto(xpd, ypd);
          }
          if (y > 0) {
            /* store for background lines */

            /* draw additionally line to last point */
            plot_drawto(*(plast - 2), *(plast - 1), psky, 0);
          }
        }

        /* store position in destination */
        *plast++ = xpd;
        *plast++ = ypd;

      }
    }
    break;
  case 2: /* look from 180 - 269 degrees */
    dxdi = dto32((double) (xxd - 1) * (x3 - x0) * DPREC) / (int32) (nx - 1);
    dydi = dto32((double) (yyd - 1) * (y0 - y3) * DPREC) / (int32) (nx - 1);
    dxdo = dto32((double) (xxd - 1) * x0 * DPREC) / (int32) (ny - 1);
    dydo = dto32((double) (yyd - 1) * (y0 - y2) * DPREC) / (int32) (ny - 1);

    yposs = (int32) (yys - 1) << PREC;
    for (y = (ny - 1); y >= 0; y--, yposs -= dys, xposdini -= dxdo, yposdini -=
        dydo) {

#ifdef use_again
      if (iabort())
        goto end;
#endif

      /* init positions on linebegin */
      xposs = 0L;
      xposd = xposdini;
      yposd = yposdini;

      /* init pointers on linebegin */
      plast = lastline;
      psim = (int8 *)pixad( ((xposs + ROUND) >> PREC),
                            ((yposs + ROUND) >> PREC), srcim);

      for (x = 0; x < nx;
          x++, xposs += dxs, xposd += dxdi, yposd -= dydi, psim +=
              (((xposs + ROUND ) >> PREC) - xps)) {

        /* get position in source */
        xps = ((xposs + ROUND ) >> PREC);
        yps = ((yposs + ROUND ) >> PREC);

        /* get position in destination */
        xpd = ((xposd + ROUND ) >> PREC);
        ypd = ((yposd + ROUND ) >> PREC);

        /* draw vertical lines too */
        if (x == 0 || y == (ny - 1))
          plot_moveto(xpd, ypd);

        if (scale16 >= 0L)
          val = ((int32) btoi(*psim) * scale16 + ROUND ) >> PREC;
        else
          val = ((((int32) btoi(*psim) - (int32) VALMAX) * scale16 - ROUND )
              >> PREC);

        ypd -= ((val * scalefactor + ROUND ) >> PREC);

        /* draw grid lines */
        if (y < (ny - 1)) {
          if (x > 0) {
            /* draw additionally line to last point */
            if (xpd == *(plast - 2))
              steig1 = MAXFLOAT;
            else
              steig1 = (sfloat) (ypd - *(plast - 1))
                  / (sfloat) (xpd - *(plast - 2));
            if (steig1 < 0.0)
              steig1 = -steig1;
            if (xpd == *(plast))
              steig2 = MAXFLOAT;
            else
              steig2 = (sfloat) (*(plast + 1) - ypd)
                  / (sfloat) (*(plast) - xpd);
            if (steig2 < 0.0)
              steig2 = -steig2;
            if (steig2 > steig1) {
              if (ypd <= *(plast + 1)) { /* draw upstairs with check */
                plot_moveto(*plast, *(plast + 1));
                plot_drawto(xpd, ypd, psky, 1);
              } else {
                plot_moveto(xpd, ypd);
                plot_drawto(*plast, *(plast + 1), psky, 1);
              }
              if (*(plast - 1) <= ypd) { /* draw upstairs */
                plot_moveto(xpd, ypd);
                plot_drawto(*(plast - 2), *(plast - 1), psky, 1);
              } else {
                plot_moveto(*(plast - 2), *(plast - 1));
                plot_drawto(xpd, ypd, psky, 1);
              }
            } else {
              if (*(plast - 1) <= ypd) { /* draw upstairs */
                plot_moveto(xpd, ypd);
                plot_drawto(*(plast - 2), *(plast - 1), psky, 1);
              } else {
                plot_moveto(*(plast - 2), *(plast - 1));
                plot_drawto(xpd, ypd, psky, 1);
              }
              if (ypd <= *(plast + 1)) { /* draw upstairs with check */
                plot_moveto(*plast, *(plast + 1));
                plot_drawto(xpd, ypd, psky, 1);
              } else {
                plot_moveto(xpd, ypd);
                plot_drawto(*plast, *(plast + 1), psky, 1);
              }
            }
          } else {
            /* store for background lines */

            if( (bcol.r | bcol.g | bcol.b) != 0) {

              gp_col = bcol;

              plot_drawto(xpd, ypd, psky, 1);

              gp_col = gcol;

            } else {
              plot_moveto(xpd, ypd);
            }
            plot_drawto(*plast, *(plast + 1), psky, 0);
          }
        } else { /* first line */
          if( (bcol.r | bcol.g | bcol.b) != 0) {

            gp_col = bcol;

            plot_drawto(xpd, ypd, psky, 1);

            gp_col = gcol;

          } else {
            plot_moveto(xpd, ypd);
          }
          if (x > 0) {
            /* store for background lines */

            /* draw additionally line to last point */
            plot_drawto(*(plast - 2), *(plast - 1), psky, 0);
          }
        }

        /* store position in destination */
        *plast++ = xpd;
        *plast++ = ypd;

      }
    }
    break;
  case 3: /* look from 270 - 359 degrees */
    dxdi = dto32((double) (xxd - 1) * (x3 - x0) * DPREC) / (int32) (ny - 1);
    dydi = dto32((double) (yyd - 1) * (y0 - y3) * DPREC) / (int32) (ny - 1);
    dxdo = dto32((double) (xxd - 1) * x0 * DPREC) / (int32) (nx - 1);
    dydo = dto32((double) (yyd - 1) * (y0 - y2) * DPREC) / (int32) (nx - 1);

    xposs = (int32) (xxs - 1) << PREC;
    for (x = (nx - 1); x >= 0; x--, xposs -= dxs, xposdini -= dxdo, yposdini -=
        dydo) {

#ifdef use_again
      if (iabort())
        goto end;
#endif

      /* init positions on linebegin */
      yposs = (int32) (yys - 1) << PREC;
      xposd = xposdini;
      yposd = yposdini;

      /* init pointers on linebegin */
      plast = lastline;
      psim = (int8 *)pixad( ((xposs + ROUND) >> PREC),
                            ((yposs + ROUND) >> PREC), srcim);

      for (y = (ny - 1); y >= 0; y--, yposs -= dys, xposd += dxdi, yposd -=
          dydi, psim -= (yps - ((yposs + ROUND ) >> PREC)) * incr) {

        /* get position in source */
        xps = ((xposs + ROUND ) >> PREC);
        yps = ((yposs + ROUND ) >> PREC);

        /* get position in destination */
        xpd = ((xposd + ROUND ) >> PREC);
        ypd = ((yposd + ROUND ) >> PREC);

        /* draw vertical lines too */
        if (x == (nx - 1) || y == (ny - 1))
          plot_moveto(xpd, ypd);

        if (scale16 >= 0L)
          val = ((int32) btoi(*psim) * scale16 + ROUND ) >> PREC;
        else
          val = ((((int32) btoi(*psim) - (int32) VALMAX) * scale16 - ROUND )
              >> PREC);

        ypd -= ((val * scalefactor + ROUND ) >> PREC);

        /* draw grid lines */
        if (x < (nx - 1)) {
          if (y < (ny - 1)) {
            /* draw additionally line to last point */
            if (xpd == *(plast - 2))
              steig1 = MAXFLOAT;
            else
              steig1 = (sfloat) (ypd - *(plast - 1))
                  / (sfloat) (xpd - *(plast - 2));
            if (steig1 < 0.0)
              steig1 = -steig1;
            if (xpd == *(plast))
              steig2 = MAXFLOAT;
            else
              steig2 = (sfloat) (*(plast + 1) - ypd)
                  / (sfloat) (*(plast) - xpd);
            if (steig2 < 0.0)
              steig2 = -steig2;
            if (steig2 > steig1) {
              if (ypd <= *(plast + 1)) { /* draw upstairs with check */
                plot_moveto(*plast, *(plast + 1));
                plot_drawto(xpd, ypd, psky, 1);
              } else {
                plot_moveto(xpd, ypd);
                plot_drawto(*plast, *(plast + 1), psky, 1);
              }
              if (*(plast - 1) <= ypd) { /* draw upstairs */
                plot_moveto(xpd, ypd);
                plot_drawto(*(plast - 2), *(plast - 1), psky, 1);
              } else {
                plot_moveto(*(plast - 2), *(plast - 1));
                plot_drawto(xpd, ypd, psky, 1);
              }
            } else {
              if (*(plast - 1) <= ypd) { /* draw upstairs */
                plot_moveto(xpd, ypd);
                plot_drawto(*(plast - 2), *(plast - 1), psky, 1);
              } else {
                plot_moveto(*(plast - 2), *(plast - 1));
                plot_drawto(xpd, ypd, psky, 1);
              }
              if (ypd <= *(plast + 1)) { /* draw upstairs with check */
                plot_moveto(*plast, *(plast + 1));
                plot_drawto(xpd, ypd, psky, 1);
              } else {
                plot_moveto(xpd, ypd);
                plot_drawto(*plast, *(plast + 1), psky, 1);
              }
            }
          } else {
            /* store for background lines */

            if( (bcol.r | bcol.g | bcol.b) != 0) {

              gp_col = bcol;

              plot_drawto(xpd, ypd, psky, 1);

              gp_col = gcol;

            } else {
              plot_moveto(xpd, ypd);
            }
            plot_drawto(*plast, *(plast + 1), psky, 0);
          }
        } else { /* first line */
          if( (bcol.r | bcol.g | bcol.b) != 0) {

            gp_col = bcol;

            plot_drawto(xpd, ypd, psky, 1);

            gp_col = gcol;

          } else {
            plot_moveto(xpd, ypd);
          }
          if (y < (ny - 1)) {
            /* store for background lines */

            /* draw additionally line to last point */
            plot_drawto(*(plast - 2), *(plast - 1), psky, 0);
          }
        }

        /* store position in destination */
        *plast++ = xpd;
        *plast++ = ypd;

      }
    }
    break;
  } /* end of switch */

end:

  ierr = 0;

//x/errorg:

error2:

  if( lastline != NULL) {

    free( lastline);
  }

error:

  if( skyline != NULL) {

    free( skyline);
  }

  return (ierr);

DrawBackgroundLines:

  /* draw background lines */
  if( (lcol.r | lcol.g | lcol.b) != 0) {

    gp_col = lcol;

    plot_moveto(0, p2y);
    plot_drawto(0, p2y - p1y, psky, 0);
    plot_moveto(p1x, p1y);
    plot_drawto(p1x, p1y - p1y, psky, 0);
    plot_moveto(xxd - 1, p3y);
    plot_drawto(xxd - 1, p3y - p1y, psky, 0);

    for (i = 0; i <= BACK_LINES; i++) {
      dist_bline = (p1y * i) / BACK_LINES;
      if (p2y >= p3y) { /* draw 1st steile line */
        plot_moveto(0, p2y - dist_bline);
        plot_drawto(p1x, p1y - dist_bline, psky, 1);
        plot_moveto(xxd - 1, p3y - dist_bline);
        plot_drawto(p1x, p1y - dist_bline, psky, 1);
      } else {
        plot_moveto(xxd - 1, p3y - dist_bline);
        plot_drawto(p1x, p1y - dist_bline, psky, 1);
        plot_moveto(0, p2y - dist_bline);
        plot_drawto(p1x, p1y - dist_bline, psky, 1);
      }
    }
    if (elev == 90) {
      plot_moveto(0, p2y);
      plot_drawto(p1x, p1y, psky, 0);
      plot_drawto(xxd - 1, p3y, psky, 0);
    }
    if (azim == 0) {
      plot_moveto(0, *psky - 1);
      plot_drawto(p1x, *(psky + p1x), psky, 0);
    }
  } /* if lcol */

  goto end;
}

/************************************************************************************
 * YaIPS_ImageDispToGray
 *
 * Get a gray image from the display area.
 *
 * *pXX    Return size of image data here
 * *pYY
 *
 * Return: NULL     error
 *         else     pointer to allocated memory
 *               true: always update the display image
 */

uchar *YaIPS_ImageDispToGray( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, int *pXX, int *pYY)
{
  uchar *pDataSrc, PixVal;
  uchar *pGrayImage;                       // Pointer to image data returned
  int DisplayColModStyle;

  pGrayImage = NULL;                       // NO gray image

  // Allocate gray image

  pGrayImage = (uchar *)malloc( pYaIPS_ImageDisp->BigImage_sw * pYaIPS_ImageDisp->BigImage_sh);

  if( pGrayImage == NULL) {            // Security check

    return( NULL);
  }

  *pXX = pYaIPS_ImageDisp->BigImage_sw;
  *pYY = pYaIPS_ImageDisp->BigImage_sh;

  // Check alpha usage
  DisplayColModStyle = pYaIPS_ImageDisp->DisplayColMod.Style;

  if( DisplayColModStyle == YAIPS_DISP_COLMOD_A &&    // use alpha
      pYaIPS_ImageDisp->pImage_Img->d() != 2 &&       // and no alpha component
      pYaIPS_ImageDisp->pImage_Img->d() != 4) {

    DisplayColModStyle = YAIPS_DISP_COLMOD_NORMAL;    // Use normal channel
  }

  //
  // Copy image data
  //

  pDataSrc = (uchar *)pYaIPS_ImageDisp->pImage_Img->data()[ 0];

  if( pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_1_1) {  // 1:1 or enlarged

    int SizeShift, SizeFactor, y, x, Src_d, line_d_src, line_d_dst, BigImage_sw, BigImage_sh, SizeLineDst;
    int SubImage_x_int, SubImage_y_int;
    uchar *pSrcLine, *pDstLine;
    int Small_h, Small_w, x2, y2;
    uchar *pSrcLine2, *pDstLine2;

    BigImage_sw = pYaIPS_ImageDisp->BigImage_sw;
    BigImage_sh = pYaIPS_ImageDisp->BigImage_sh;

    SizeShift  = pYaIPS_ImageDisp->DisplayResolution - 1;       // Shift is enlargement by power of 2
    SizeFactor = 1 << SizeShift;                               // Enlargement factor
    Src_d = pYaIPS_ImageDisp->pImage_Img->d();
    line_d_src = pYaIPS_ImageDisp->pImage_Img->ld() ? pYaIPS_ImageDisp->pImage_Img->ld() : pYaIPS_ImageDisp->pImage_Img->data_w() * Src_d;
    line_d_dst = BigImage_sw;

    SizeLineDst = BigImage_sw;

    SubImage_x_int = (int)(pYaIPS_ImageDisp->SubImage_x + 0.5);
    SubImage_y_int = (int)(pYaIPS_ImageDisp->SubImage_y + 0.5);

    pSrcLine = pDataSrc + SubImage_y_int * line_d_src + SubImage_x_int * Src_d;
    pDstLine = pGrayImage;

    Small_h = BigImage_sh >> SizeShift;
    Small_w = BigImage_sw >> SizeShift;

    for (y = 0; y < Small_h; y++) {

      // Build first line in destination

      pSrcLine2 = pSrcLine;
      pDstLine2 = pDstLine;

      if( DisplayColModStyle == YAIPS_DISP_COLMOD_A) {  // Show alpha channel and image has an alpha channel

        for( x = 0; x < Small_w; x++) {

          if( Src_d == 2) {

            PixVal = pSrcLine2[1];
          } else {

            PixVal = pSrcLine2[3];
          }

          for( x2 = 0; x2 < SizeFactor; x2++) {

            *pDstLine2++ = PixVal;
          }

          pSrcLine2 += Src_d;
        }

      } else if (Src_d >= 3) {                // Have a color image

        for( x = 0; x < Small_w; x++) {

          switch( DisplayColModStyle) {

          case YAIPS_DISP_COLMOD_NORMAL:     // Normal RGB image
          case YAIPS_DISP_COLMOD_BW:         // BW image
          default:

            // A fast black white conversion. Intensity part of an IHS conversion.
            PixVal = (pSrcLine2[0] * 76 + pSrcLine2[1] * 150 + pSrcLine2[2] * 30) >> 8;
            break;

          case YAIPS_DISP_COLMOD_R:          // Red component

            PixVal = pSrcLine2[0];
            break;

          case YAIPS_DISP_COLMOD_G:          // Green component

            PixVal = pSrcLine2[1];
            break;

          case YAIPS_DISP_COLMOD_B:          // Blue component

            PixVal = pSrcLine2[2];
            break;
          } // end switch

          if( Src_d == 4) {                  // Color with alpha

            //
            // 10.03.2025 RR: Reading png RGB images with alpha dims the RGB values with low alpha values.
            //                This is not done for png BW images with alpha value.
            //                To compensate for this, we use this HACK.

            PixVal = PixVal * pSrcLine2[3] / 255;      // Dim value with low alpha
          }

          for( x2 = 0; x2 < SizeFactor; x2++) {

            *pDstLine2++ = PixVal;
          }

          pSrcLine2 += Src_d;
        }

      } else {                                       // Have a BW image

        for( x = 0; x < Small_w; x++) {

          // A fast black white conversion. Intensity part of an IHS conversion.
          PixVal = pSrcLine2[0];

          if( Src_d == 2) {                            // BW with alpha

            //
            // 10.03.2025 RR: Reading png RGB images with alpha dims the RGB values with low alpha values.
            //                This is not done for png BW images with alpha value.
            //                To compensate for this, we use this HACK.

            PixVal = PixVal * pSrcLine2[1] / 255;      // Dim value with low alpha
          }

          for( x2 = 0; x2 < SizeFactor; x2++) {

            *pDstLine2++ = PixVal;
          }

          pSrcLine2 += Src_d;
        }
      }

      // Copy first line

      pSrcLine2 = pDstLine;

      pDstLine2 = pDstLine;
      pDstLine += line_d_dst;

      for (y2 = 1; y2 < SizeFactor; y2++) {

        memcpy(pDstLine, pSrcLine2, SizeLineDst);

        pDstLine += line_d_dst;
      }

      pSrcLine += line_d_src;
    }

  } else {    // Must be YAIPS_DISP_RESOLUTION_AUTO

    // OK, need to resize the image data
    uchar         *new_ptr;       // Pointer into new array
    int           dx, dy,         // Destination coordinates
                  Src_d,          // Bytes per pixel
                  line_d;         // stride from line to line

    Src_d = pYaIPS_ImageDisp->pImage_Img->d();
    line_d = pYaIPS_ImageDisp->pImage_Img->ld() ? pYaIPS_ImageDisp->pImage_Img->ld() : pYaIPS_ImageDisp->pImage_Img->data_w() * Src_d;

#ifdef use_again

    // 13.02.2025 RR: Nearest neighbor scaling tested. It works.
    // Nearest neighbor scaling (FL_RGB_SCALING_NEAREST)

    const uchar   *old_ptr;       // Pointer into old array
    int         c,              // Channel number
                sy,             // Source coordinate
                xerr, yerr,     // X & Y errors
                xmod, ymod,     // X & Y moduli
                xstep, ystep;   // X & Y step increments

    // Figure out Bresenham step/modulus values...
    xmod   = pYaIPS_ImageDisp->BigImage_iw % pYaIPS_ImageDisp->BigImage_sw;
    xstep  = (pYaIPS_ImageDisp->BigImage_iw / pYaIPS_ImageDisp->BigImage_sw) * d;
    ymod   = pYaIPS_ImageDisp->BigImage_ih % pYaIPS_ImageDisp->BigImage_sh;
    ystep  = pYaIPS_ImageDisp->BigImage_ih / pYaIPS_ImageDisp->BigImage_sh;

    // Scale the image using a nearest-neighbor algorithm...
    for (dy = pYaIPS_ImageDisp->BigImage_sh, sy = 0, yerr = pYaIPS_ImageDisp->BigImage_sh, new_ptr = pDataDst; dy > 0; dy --) {
      for (dx = pYaIPS_ImageDisp->BigImage_sw, xerr = pYaIPS_ImageDisp->BigImage_sw, old_ptr = pDataSrc + sy * line_d; dx > 0; dx --) {

        for( c = 0; c < d; c ++) *new_ptr++ = old_ptr[c];

        old_ptr += xstep;
        xerr    -= xmod;

        if (xerr <= 0) {
          xerr    += pYaIPS_ImageDisp->BigImage_sw;
          old_ptr += d;
        }
      }

      sy   += ystep;
      yerr -= ymod;
      if (yerr <= 0) {
        yerr += pYaIPS_ImageDisp->BigImage_sh;
        sy ++;
      }
    }

#else

    // Bilinear scaling (FL_RGB_SCALING_BILINEAR)

    const float xscale = (pYaIPS_ImageDisp->BigImage_iw - 1) / (float) pYaIPS_ImageDisp->BigImage_sw;
    const float yscale = (pYaIPS_ImageDisp->BigImage_ih - 1) / (float) pYaIPS_ImageDisp->BigImage_sh;

    for( dy = 0; dy < pYaIPS_ImageDisp->BigImage_sh; dy++) {

      float oldy = dy * yscale;

      if (oldy >= pYaIPS_ImageDisp->BigImage_ih)
          oldy = float( pYaIPS_ImageDisp->BigImage_ih - 1);

      const float yfract = oldy - (unsigned) oldy;

      for( dx = 0; dx < pYaIPS_ImageDisp->BigImage_sw; dx++) {

        new_ptr = pGrayImage + dy * pYaIPS_ImageDisp->BigImage_sw + dx;

        float oldx = dx * xscale;
        if (oldx >= pYaIPS_ImageDisp->BigImage_iw)
          oldx = float( pYaIPS_ImageDisp->BigImage_iw - 1);
        const float xfract = oldx - (unsigned) oldx;

        const unsigned leftx = (unsigned)oldx;
        const unsigned lefty = (unsigned)oldy;
        const unsigned rightx = (unsigned)(oldx + 1 >= pYaIPS_ImageDisp->BigImage_iw ? oldx : oldx + 1);
        const unsigned righty = (unsigned)oldy;
        const unsigned dleftx = (unsigned)oldx;
        const unsigned dlefty = (unsigned)(oldy + 1 >= pYaIPS_ImageDisp->BigImage_ih ? oldy : oldy + 1);
        const unsigned drightx = (unsigned)rightx;
        const unsigned drighty = (unsigned)dlefty;

        uchar *pleft, *pright, *pdownleft, *pdownright;
        pleft      = pDataSrc + lefty * line_d + leftx * Src_d;
        pright     = pDataSrc + righty * line_d + rightx * Src_d;
        pdownleft  = pDataSrc + dlefty * line_d + dleftx * Src_d;
        pdownright = pDataSrc + drighty * line_d + drightx * Src_d;

        uchar left, right, downleft, downright;

        if( DisplayColModStyle == YAIPS_DISP_COLMOD_A) {  // Show alpha channel and image has an alpha channle

          if( Src_d == 2) {

            left      =      pleft[ 1];
            right     =     pright[ 1];
            downleft  =  pdownleft[ 1];
            downright = pdownright[ 1];

          } else {

            left      =      pleft[ 3];
            right     =     pright[ 3];
            downleft  =  pdownleft[ 3];
            downright = pdownright[ 3];
          }

        } else if (Src_d >= 3) {                // Have a color image

          switch( DisplayColModStyle) {

          case YAIPS_DISP_COLMOD_NORMAL:     // Normal RGB image
          case YAIPS_DISP_COLMOD_BW:         // BW image
          default:

            // A fast black white conversion. Intensity part of an IHS conversion.
            left      = (     pleft[ 0] * 76 +      pleft[ 1] * 150 +      pleft[ 2] * 30) >> 8;
            right     = (    pright[ 0] * 76 +     pright[ 1] * 150 +     pright[ 2] * 30) >> 8;
            downleft  = ( pdownleft[ 0] * 76 +  pdownleft[ 1] * 150 +  pdownleft[ 2] * 30) >> 8;
            downright = (pdownright[ 0] * 76 + pdownright[ 1] * 150 + pdownright[ 2] * 30) >> 8;
            break;

          case YAIPS_DISP_COLMOD_R:          // Red component

            left      =      pleft[ 0];
            right     =     pright[ 0];
            downleft  =  pdownleft[ 0];
            downright = pdownright[ 0];
            break;

          case YAIPS_DISP_COLMOD_G:          // Green component

            left      =      pleft[ 1];
            right     =     pright[ 1];
            downleft  =  pdownleft[ 1];
            downright = pdownright[ 1];
            break;

          case YAIPS_DISP_COLMOD_B:          // Blue component

            left      =      pleft[ 2];
            right     =     pright[ 2];
            downleft  =  pdownleft[ 2];
            downright = pdownright[ 2];
            break;
          } // end switch

        } else {                                       // Have a BW image

          left      =      pleft[ 0];
          right     =     pright[ 0];
          downleft  =  pdownleft[ 0];
          downright = pdownright[ 0];
        }

        const float leftf = 1 - xfract;
        const float rightf = xfract;
        const float upf = 1 - yfract;
        const float downf = yfract;

        if( Src_d == 4) {                            // Color with alpha

          uchar PixVal, AlphaVal;

          //
          // 10.03.2025 RR: Reading png RGB images with alpha dims the RGB values wit low alpha values.
          //                This is not done for png BW images with alpha value.
          //                To compensate for this, we use this HACK.

          PixVal = (uchar)((left * leftf + right * rightf) * upf + (downleft * leftf + downright * rightf) * downf);

          left      =      pleft[ 3];
          right     =     pright[ 3];
          downleft  =  pdownleft[ 3];
          downright = pdownright[ 3];

          AlphaVal = (uchar)((left * leftf + right * rightf) * upf + (downleft * leftf + downright * rightf) * downf);

          PixVal = PixVal * AlphaVal / 255;      // Dim value with low alpha

          *new_ptr = PixVal;

        } else if( Src_d == 2) {                      // BW with alpha

          uchar PixVal, AlphaVal;

          //
          // 10.03.2025 RR: Reading png RGB images with alpha dims the RGB values wit low alpha values.
          //                This is not done for png BW images with alpha value.
          //                To compensate for this, we use this HACK.

          PixVal = (uchar)((left * leftf + right * rightf) * upf + (downleft * leftf + downright * rightf) * downf);

          left      =      pleft[ 1];
          right     =     pright[ 1];
          downleft  =  pdownleft[ 1];
          downright = pdownright[ 1];

          AlphaVal = (uchar)((left * leftf + right * rightf) * upf + (downleft * leftf + downright * rightf) * downf);

          PixVal = PixVal * AlphaVal / 255;      // Dim value with low alpha

          *new_ptr = PixVal;

        } else {

          *new_ptr = (uchar)((left * leftf + right * rightf) * upf + (downleft * leftf + downright * rightf) * downf);
        }
      }
    }
#endif
  }

  return( pGrayImage);
}

/************************************************************************************
 * YaIPS_ImageDispPlot3D
 *
 * Get a gray image from the display area.
 *
 * *pXX    Return size of image data here
 * *pYY
 *
 * Return: NULL     error
 *         else     pointer to allocated memory
 *               true: always update the display image
 */

void YaIPS_ImageDispPlot3D( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, Fl_RGB_Image *pDrawImage)
{
  uchar *pGrayImage;
  int XX, YY;                  // Size of gray image
  int azim, elev;
  Timages TempImageIPS;

  // Point to lookup tables

  gp_pLookupR = pYaIPS_ImageDisp->DisplayColMod.LookupR;
  gp_pLookupG = pYaIPS_ImageDisp->DisplayColMod.LookupG;
  gp_pLookupB = pYaIPS_ImageDisp->DisplayColMod.LookupB;

  // Get gray image
  pGrayImage = YaIPS_ImageDispToGray( pYaIPS_ImageDisp, &XX, &YY);

  if( pGrayImage == NULL) {         // Security test

    return;
  }

  // Construct a IPS image
  im_ucreateMem( &TempImageIPS, TY_BYTE, DV_HOST, XX, YY, pGrayImage);
  TempImageIPS.Flags |= IPS_IMG_FLAG_ALLOC_MEMORY;    // Set flag, image memory allocated

  // Preset memory

  uchar r, g, b;
  uchar *pImg, *pImg2;
  int d, ld, x, y, xx, yy, nx, ny;

  pImg = (uchar *)pDrawImage->data()[ 0];
  d    = pDrawImage->d();
  ld   = pDrawImage->ld() ? pDrawImage->ld() : pDrawImage->data_w() * pDrawImage->d();

  if( d >= 3) {                                     // Have a RGB image

#ifdef _DEBUG
    r = g = b = 128;
#else
    Fl::get_color( YaIPS_Color_IMG_BGND, r, g, b);   // Background color
#endif

    xx = pDrawImage->data_w();
    yy = pDrawImage->data_h();

    // Color first line

    pImg2 = pImg;
    for( x = 0; x < xx; x++) {

      pImg2[ 0] = r;
      pImg2[ 1] = g;
      pImg2[ 2] = b;

      pImg2 += d;
    }

    if( d == 4) {                     // Image has an alpha part

      pImg2 = pImg;
      for( x = 0; x < xx; x++) {

        pImg2[ 3] = 0xff;

        pImg2 += d;
      }
    }

    // Copy to other lines

    pImg2 = pImg;
    for( y = 1; y < yy; y++) {

      pImg2 += ld;

      memcpy( pImg2, pImg, ld);
    }

  } else {                                // NO RGB image

    memset( (void *)pDrawImage->data()[ 0], 0, pDrawImage->data_w() * pDrawImage->data_h() * pDrawImage->d());
  }

  // Add to azimuth angle to adapt view zero degree angle

  azim = pYaIPS_ImageDisp->Plot3D_Azimuth;
  elev = pYaIPS_ImageDisp->Plot3D_Elevation;

  azim += PLOT3D_AZIMUT_ANGLE_ADD;

  // Clip parameters

  if( azim <    0) azim += 360;
  if( azim >= 360) azim -= 360;

  if( azim <   0) azim =   0;
  if( azim > 359) azim = 359;

  if( elev <   0) elev =   0;
  if( elev >  90) elev =  90;


  nx = (XX + 4 ) / 16;
  ny = (YY + 4 ) / 16;

  // Plot background lines
  plot3d( &TempImageIPS, pDrawImage, nx, ny, azim, elev, 1.0, true);

  // Plot gray values
  g_plot3d( &TempImageIPS, pDrawImage, pYaIPS_ImageDisp, azim, elev, 1.0, pYaIPS_ImageDisp->Plot3D_Inverted);

  // Plot lines
  if (pYaIPS_ImageDisp->Plot3D_DrawGrid) {

    plot3d( &TempImageIPS, pDrawImage, nx, ny, azim, elev, 1.0, false);
  }

  im_removeMem( &TempImageIPS);

}

/***************************************************************************/

