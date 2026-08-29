/****************************************************************************

  YaIPS_RGB_Linesum.cpp

  Fl_RGB_Image image processing.
  Sum up grey values linewise

 07.07.2025 RR: First edition of this file.

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

#define  NONORM      (int16)0
#define  ISNORM      (int16)1

#define PREC  16
#define PRECI   65536
#define PRECF   65536.0
#define ROUND 32768

/***************************************************************************
* normalize
* Normalize vector values
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* ResMultArg   Result multiplier, default should be 1.0
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/
static int normalize( int32 *vp32, int32 vlen, int32 normval)

{
  register int32 *hvp, *hend;

  hvp = vp32;
  hend = hvp + vlen;
  while( hvp < hend) {

     *hvp++ /= normval;
  }

  return(0);
}

/***************************************************************************
* YaIPS_RGB_Colsum
* Sum up grey values in a column
*
* pSrc            Source image
* sumvec          Destination vector for linesums
* ByteComponent   What byte to use from the pixel
* mode            Normalization mode. 0 = sum, 1 = average.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Colsum( YaIPS_RGB_ImgD_t *pSrc, // Source image
                      Tvector *dstvec,        // Destination vector for linesums
                      int ByteComponent,      // What byte to use from the pixel
                      int mode)               // Normalization mode. 0 = sum, 1 = average
{
  int32 *hvp32;
  int d, x, y, yy, nitem;
  uchar *p8s;
  int32 *vp32;

  if( ByteComponent < 0 || ByteComponent >= pSrc->d) {  // Security test of range

    return( -11);
  }

  if(utvcheck(dstvec,DV_HOST,TY_INT32)) return(-12);

  nitem = pSrc->xx;
  yy = pSrc->yy;

  d = pSrc->d;

  if( ve_alloc( dstvec,nitem,(int16)sizeof(int32),TY_INT32)) return(-13);

  vp32 = hvp32 = (int32 *) vgetpm(dstvec);

  for ( x=0L; x<nitem; x++ )  *hvp32++ = 0L;    /* clear */
  hvp32 = vp32;

  for( y = 0; y < yy; y++)  {

    hvp32 = vp32;
    p8s = RGB_pixad( 0, y, pSrc);
    p8s += ByteComponent;             // Offset with byte component
    x = nitem;
    while( x >= 8) {
      *hvp32++ += *p8s; p8s += d;
      *hvp32++ += *p8s; p8s += d;
      *hvp32++ += *p8s; p8s += d;
      *hvp32++ += *p8s; p8s += d;
      *hvp32++ += *p8s; p8s += d;
      *hvp32++ += *p8s; p8s += d;
      *hvp32++ += *p8s; p8s += d;
      *hvp32++ += *p8s; p8s += d;
      x -= 8;
    }
    while( x > 0) {
      *hvp32++ += *p8s; p8s += d;
      x -= 1;
    }
  }

  if ( mode == ISNORM ) normalize(vp32,nitem,yy);
  vputnm(dstvec,nitem);         /* store number of items */

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Colsum
* Sum up grey values in a column
*
* pSrc            Source image
* sumvec          Destination vector for linesums
* ByteComponent   What byte to use from the pixel
* mode            Normalization mode. 0 = sum, 1 = average.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Colsum( Fl_RGB_Image *pSrc,   // Source image
                      Tvector *dstvec,      // Destination vector for linesums
                      int ByteComponent,    // What byte to use from the pixel
                      int mode)             // Normalization mode. 0 = sum, 1 = average
{
  YaIPS_RGB_ImgD_t iSrc;
  int ierr;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_Colsum( &iSrc, dstvec, ByteComponent, mode);

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_Rowsum
* Sum up grey values in a row
*
* pSrc            Source image
* sumvec          Destination vector for linesums
* ByteComponent   What byte to use from the pixel
* mode            Normalization mode. 0 = sum, 1 = average.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Rowsum( YaIPS_RGB_ImgD_t *pSrc, // Source image
                      Tvector *dstvec,        // Destination vector for linesums
                      int ByteComponent,      // What byte to use from the pixel
                      int mode)               // Normalization mode. 0 = sum, 1 = average
{
  int32 *hvp32;
  int d, x, y, xx, nitem;
  uchar *p8s;
  int32 *vp32;

  if( ByteComponent < 0 || ByteComponent >= pSrc->d) {  // Security test of range

    return( -11);
  }

  if(utvcheck(dstvec,DV_HOST,TY_INT32)) return(-12);

  nitem = pSrc->yy;
  xx = pSrc->xx;

  d = pSrc->d;

  if( ve_alloc( dstvec,nitem,(int16)sizeof(int32),TY_INT32)) return(-13);

  vp32 = hvp32 = (int32 *) vgetpm(dstvec);

  for ( x=0L; x<nitem; x++ )  *hvp32++ = 0L;    /* clear */
  hvp32 = vp32;

  for( y = 0; y < nitem; y++, hvp32++)  {

    p8s = RGB_pixad( 0, y, pSrc);
    p8s += ByteComponent;             // Offset with byte component
    x = xx;
    while( x >= 8) {
      *hvp32 += *p8s; p8s += d;
      *hvp32 += *p8s; p8s += d;
      *hvp32 += *p8s; p8s += d;
      *hvp32 += *p8s; p8s += d;
      *hvp32 += *p8s; p8s += d;
      *hvp32 += *p8s; p8s += d;
      *hvp32 += *p8s; p8s += d;
      *hvp32 += *p8s; p8s += d;
      x -= 8;
    }
    while( x > 0) {
      *hvp32 += *p8s; p8s += d;
      x -= 1;
    }
  }

  if ( mode == ISNORM ) normalize( vp32, nitem, xx);
  vputnm(dstvec,nitem);         /* store number of items */

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Rowsum
* Sum up grey values in a row
*
* pSrc            Source image
* sumvec          Destination vector for linesums
* ByteComponent   What byte to use from the pixel
* mode            Normalization mode. 0 = sum, 1 = average.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Rowsum( Fl_RGB_Image *pSrc,   // Source image
                      Tvector *dstvec,      // Destination vector for linesums
                      int ByteComponent,    // What byte to use from the pixel
                      int mode)             // Normalization mode. 0 = sum, 1 = average
{
  YaIPS_RGB_ImgD_t iSrc;
  int ierr;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_Colsum( &iSrc, dstvec, ByteComponent, mode);

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Linesum
* Sum up grey values in a line
*
* Sum up grey values of "image" vertical to line direction.
* Write the results into "dstvec".
*
* pSrc            Source image
* sumvec          Destination vector for linesums
* ByteComponent   What byte to use from the pixel
* x0, y0,         Point from
* x1, y1,         Point to
* nsamp,          Number of samples
*                 0 = set nsamp automatically { nsamp = sqrt(dx^2 + dy^2) }
*                 else must be >= 2.
* width,          Pixels to sum up or average vertical to line direction
* mode            Normalization mode. 0 = sum, 1 = average, 2 = min, 3 = max.
* ytox            y/x pixel relation
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Linesum( YaIPS_RGB_ImgD_t *pSrc, // Source image
                       Tvector *dstvec,        // Destination vector for linesums
                       int ByteComponent,      // What byte to use from the pixel
                       int x0, int y0,         // Point from
                       int x1, int y1,         // Point to
                       int nsamp,              // Number of samples. 0 = set automatically else must be >= 2.
                       int width,              // Pixels to sum up or average vertical to line direction
                       int mode,               // Normalization mode. 0 = sum, 1 = average, 2 = min, 3 = max
                       double ytox)            // y/x pixel relation
{
  int32 *vp;             /* vector pointer       */
  int32 *vp_start, nitem;
  int32 ox, oy, oxs, oys, oxe, oye;     /* corordinates of line     */
  int32 dx, dy;                   /* axis of line             */
  int32 oincx, oincy;             /* distance per outer step  */
  int32 w, dbx, dby, xmax, ymax;
  int32 i, ix, iy, n, temp;
  int32 iincx, iincy;      /* distance per inner step  */
  int32 xa, xb, xc, xd, ya, yb, yc, yd;   /* border corordinates      */
  uchar *p8s;

  if( ByteComponent < 0 || ByteComponent >= pSrc->d) {  // Security test of range

    return( -11);
  }

  if(utvcheck(dstvec,DV_HOST,TY_INT32)) return(-12);

  xmax = pSrc->xx - 1;
  ymax = pSrc->yy - 1;

  if ((x0 < 0) || (x0 > (int16)xmax)) return(-4);   /* range check */
  if ((y0 < 0) || (y0 > (int16)ymax)) return(-4);
  if ((x1 < 0) || (x1 > (int16)xmax)) return(-4);
  if ((y1 < 0) || (y1 > (int16)ymax)) return(-4);

  dx = (int32)(x1 - x0);                    /* lenght of x-axes */
  dy = (int32)(y1 - y0);                    /* lenght of y-axes */
  if ( dx == 0 && dy == 0 ) return(-5);     /* startpoint == endpoint */

  oxs = ((int32)x0 << PREC) + ROUND;    /* start, end coordinates */
  oys = ((int32)y0 << PREC) + ROUND;    /* with high precission   */
  oxe = ((int32)x1 << PREC) + ROUND;
  oye = ((int32)y1 << PREC) + ROUND;

  if (nsamp == 0) {         /* set nsamp automatically */
    nitem = dto32( sqrt( (double)( dx * dx + dy * dy)) ) + 1L;
  }
  else {
    if (nsamp < 2)
      return(-10);
    else
      nitem = (int32)(nsamp);
  }

  /* increments for outer line loop */
  oincx = ((dx << PREC) / (nitem - 1L));
  oincy = ((dy << PREC) / (nitem - 1L));

  /* increments for inner line loop */

  /* geometric increments for inner line loop */
  /* we have to do the ytox-correction two times:
     - he user gives the start and end point of the scaled line, so we
     have to recorrect the angle of the line to a ytox of 1.0 (division
     inside atan2)
     - then we have to correct the angle and length of the vertical line
     (multiplication of iincx)
  */
  iincx = dto32( cos(atan2((double)dx / ytox, (double)dy)) * -PRECF * ytox);
  iincy = dto32( (sin(atan2((double)dx / ytox, (double)dy)) * PRECF));

  /* compute border point coordinates */
  w = ((int32)width - 1) << (PREC - 1);
  dbx = (w >> (PREC-1)) * (iincx >> 1); /* x-distance from (x0,y0) to border */
  dby = (w >> (PREC-1)) * (iincy >> 1); /* y-distance from (x0,y0) to border */
  xa = oxs - dbx; ya = oys - dby;
  xb = oxs + dbx; yb = oys + dby;
  xc = oxe - dbx; yc = oye - dby;
  xd = oxe + dbx; yd = oye + dby;

  xmax <<= PREC; ymax <<= PREC;
  xmax += ROUND; ymax += ROUND;
  if ((xa < 0) || (xa > xmax)) return(-6);   /* range check point A */
  if ((ya < 0) || (ya > ymax)) return(-6);
  if ((xb < 0) || (xb > xmax)) return(-6);   /* range check point B */
  if ((yb < 0) || (yb > ymax)) return(-6);
  if ((xc < 0) || (xc > xmax)) return(-6);   /* range check point C */
  if ((yc < 0) || (yc > ymax)) return(-6);
  if ((xd < 0) || (xd > xmax)) return(-6);   /* range check point D */
  if ((yd < 0) || (yd > ymax)) return(-6);

  if (ve_alloc(dstvec, nitem, (int16)sizeof(int32), TY_INT32)) return(-9);

  vp_start = vp = (int32 *)vgetpm(dstvec);

  if (mode == 2) {            /* minimum */
    for (i=0L; i < nitem; i++)  *vp++ = MAXINT32;    /* preset vector */
  } else if (mode == 3) {     /* maximum */
    for (i=0L; i < nitem; i++)  *vp++ = MININT32;    /* preset vector */
  } else {
    for (i=0L; i < nitem; i++)  *vp++ = 0L;          /* clear vector */
  }
  vp = vp_start;

  ox = xa; oy = ya;     /* start points for outer loop   */

  for( n = 0; n < nitem; n++) {   /* while endpoint not reached    */

    ix = ox; iy = oy;     /* start points for inner loop   */

    for( i = 0; i < width; i++ ) {

      p8s = RGB_pixad( (int)(ix >> PREC), (int)(iy >> PREC), pSrc);
      p8s += ByteComponent;             // Offset with byte component

      temp = *p8s;

      if (mode == 2) {            /* minimum */
        if (temp < *vp) *vp = temp;
      } else if (mode == 3) {     /* maximum */
        if (temp > *vp) *vp = temp;
      } else {
        *vp += temp;
      }

      ix += iincx; iy += iincy;
    }

    vp++;
    ox += oincx; oy += oincy;
  }

  if ( mode == ISNORM ) normalize( vp_start, nitem, width);
  vputnm(dstvec,nitem);         /* store number of items */

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_Linesum
* Sum up grey values in a line
*
* Sum up grey values of "image" vertical to line direction.
* Write the results into "dstvec".
*
* pSrc            Source image
* sumvec          Destination vector for linesums
* ByteComponent   What byte to use from the pixel
* x0, y0,         Point fromYaIPS_RGB_LinesumBiLin
* x1, y1,         Point to
* nsamp,          Number of samples
*                 0 = set nsamp automatically { nsamp = sqrt(dx^2 + dy^2) }
*                 else must be >= 2.
* width,          Pixels to sum up or average vertical to line direction
* mode            Normalization mode. 0 = sum, 1 = average, 2 = min, 3 = max.
* ytox            y/x pixel relation
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Linesum( Fl_RGB_Image *pSrc,   // Source image
                       Tvector *dstvec,      // Destination vector for linesums
                       int ByteComponent,    // What byte to use from the pixel
                       int x0, int y0,       // Point from
                       int x1, int y1,       // Point to
                       int nsamp,            // Number of samples. 0 = set automatically else must be >= 2.
                       int width,            // Pixels to sum up or average vertical to line direction
                       int mode,             // Normalization mode. 0 = sum, 1 = average, 2 = min, 3 = max
                       double ytox)          // y/x pixel relation
{
  YaIPS_RGB_ImgD_t iSrc;
  int ierr;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_Linesum( &iSrc, dstvec, ByteComponent, x0, y0, x1, y1, nsamp, width, mode, ytox);

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_LinesumBiLin
* Sum up grey values in a line with bilinear pixel values
*
* Sum up grey values of "image" vertical to line direction.
* Write the results into "dstvec".
*
* pSrc            Source image
* sumvec          Destination vector for linesums
* ByteComponent   What byte to use from the pixel
* x0, y0,         Point from
* x1, y1,         Point to
* nsamp,          Number of samples
*                 0 = set nsamp automatically { nsamp = sqrt(dx^2 + dy^2) }
*                 else must be >= 2.
* width,          Pixels to sum up or average vertical to line direction
* mode            Normalization mode. 0 = sum, 1 = average
* ytox            y/x pixel relation
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_LinesumBiLin( YaIPS_RGB_ImgD_t *pSrc, // Source image
                            Tvector *dstvec,        // Destination vector for linesums
                            int ByteComponent,      // What byte to use from the pixel
                            float x0, float y0,     // Point from
                            float x1, float y1,     // Point to
                            int nsamp,              // Number of samples. 0 = set automatically else must be >= 2.
                            int width,              // Pixels to sum up or average vertical to line direction
                            int mode,               // Normalization mode. 0 = sum, 1 = average
                            double ytox)            // y/x pixel relation
{
  int32 *vp;                 /* vector pointer       */
  int32 *vp_start, nitem;
  double ox, oy;                  /* coordinates of line */
  double dx, dy;                  /* axis of line             */
  double oincx, oincy;            /* distance per outer step  */
  double w, dbx, dby;
  int32 xmax, ymax;
  int32 wR, wU, ixI, iyI, i, n;
  double ix, iy, iincx, iincy;
  uchar *ps, *psNNO, *psNNU;
  double xa, xb, xc, xd, ya, yb, yc, yd;/* border coordinates */
  int d, line_d;

  if( ByteComponent < 0 || ByteComponent >= pSrc->d) {  // Security test of range

    return( -11);
  }

  if (utvcheck(dstvec,DV_HOST,TY_INT32)) {

    return(-3);
  }

  xmax = pSrc->xx - 1;
  ymax = pSrc->yy - 1;

  if ((fto32(x0) < 1) || (fto32(x0) >= xmax) || /* range check */
      (fto32(y0) < 1) || (fto32(y0) >= ymax) ||
      (fto32(x1) < 1) || (fto32(x1) >= xmax) ||
      (fto32(y1) < 1) || (fto32(y1) >= ymax)) {
    errstring = LangStringLookup( "&RGB_Geo_Linesum1=Line touching image border");
    return(-4);
  }

  dx = (double)x1 - (double)x0;             /* lenght of x-axes */
  dy = (double)y1 - (double)y0;             /* lenght of y-axes */
  if ( dx == 0.0 && dy == 0.0 ) {
    errstring = LangStringLookup( "&RGB_Geo_Linesum2=Startpoint and endpoint are equal");
    return(-5);
  }

  if (nsamp == 0) {         /* set nsamp automatically */
    nitem = dto32(sqrt((double)dx * dx + dy * dy)) + 1;
  } else {
    nitem = (int32)nsamp;
  }
  if (nitem <= 1) {
    errstring = LangStringLookup( "&RGB_Geo_Linesum3=# of samples is out of range (0 or 2..MAXINT16)");
    return(-10);
  }

  /* increments for outer line loop */
  oincx = dx / (double)(nitem - 1);
  oincy = dy / (double)(nitem - 1);

  /* increments for inner line loop */

  /* geometric increments for inner line loop
     we have to do the ytox-correction two times:
     - the user gives the start and end point of the scaled line, so we
     have to recorrect the angle of the line to a ytox of 1.0 (division
     inside atan2)
     - then we have to correct the angle and length of the vertical line
     (multiplication of iincx)
  */
  iincx = - cos(atan2(dx / (double)ytox, dy)) * ytox;
  iincy = sin(atan2(dx / (double)ytox, dy));

  /* compute border point coordinates */
  w = (double)(width - 1) / 2.0;
  dbx = w * iincx;                /* x-distance from (x0,y0) to border */
  dby = w * iincy;                /* y-distance from (x0,y0) to border */
  xa = (double)x0 - dbx; ya = (double)y0 - dby;
  xb = (double)x0 + dbx; yb = (double)y0 + dby;
  xc = (double)x1 - dbx; yc = (double)y1 - dby;
  xd = (double)x1 + dbx; yd = (double)y1 + dby;

  if ((fto32(xa) < 1) || (fto32(xa) >= xmax) ||             /* range check point A */
      (fto32(ya) < 1) || (fto32(ya) >= ymax) ||
      (fto32(xb) < 1) || (fto32(xb) >= xmax) ||             /* range check point B */
      (fto32(yb) < 1) || (fto32(yb) >= ymax) ||
      (fto32(xc) < 1) || (fto32(xc) >= xmax) ||             /* range check point C */
      (fto32(yc) < 1) || (fto32(yc) >= ymax) ||
      (fto32(xd) < 1) || (fto32(xd) >= xmax) ||             /* range check point D */
      (fto32(yd) < 1) || (fto32(yd) >= ymax)) {
    errstring = LangStringLookup( "&RGB_Geo_Linesum4=Pixel outside image (width is too large)");
    return(-6);
  }

  if (ve_alloc(dstvec, nitem, (int16)sizeof(int32), TY_INT32)) {
    errstring = ERR_IPS_NO_MEM;
    return(-9);
  }

  vp_start = vp = (int32 *)vgetpm(dstvec);

  for (i = 0; i < nitem; i++) *vp++ = 0;                  /* clear vector */
  vp = vp_start;

  ox = xa; oy = ya;                      /* start points for outer loop   */
  ps = RGB_pixad( 0, 0, pSrc);
  ps += ByteComponent;                 // Offset with byte component

  d = pSrc->d;
  line_d = pSrc->ld;

  for (n = 0; n < nitem; n++) {          /* while endpoint not reached    */
    if (iabort()) break;                 /* check CTRL C                  */
    ix = ox; iy = oy;                    /* start points for inner loop   */
    for(i = 0; i < width; i++) {

      ixI = (int)ix;
      iyI = (int)iy;
      wR = (int32)((ix - (double)ixI) * PRECF + 0.5);
      wU = (int32)((iy - (double)iyI) * PRECF + 0.5);

      psNNO = ps + ixI * d + iyI * line_d;
      psNNU = psNNO + line_d;

      *vp += ((((btoi(psNNO[0]) * (PRECI - wR) + btoi(psNNO[d]) * wR + ROUND) >> PREC) * (PRECI - wU)) +
              (((btoi(psNNU[0]) * (PRECI - wR) + btoi(psNNU[d]) * wR + ROUND) >> PREC) * wU) + ROUND) >> PREC;

      ix += iincx; iy += iincy;
    }

    vp++;
    ox += oincx; oy += oincy;
  }

  if ( mode == ISNORM ) normalize( vp_start, nitem, width);
  vputnm(dstvec,nitem);         /* store number of items */


  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_LinesumBiLin
* Sum up grey values in a line with bilinear pixel values
*
* Sum up grey values of "image" vertical to line direction.
* Write the results into "dstvec".
*
* pSrc            Source image
* sumvec          Destination vector for linesums
* ByteComponent   What byte to use from the pixel
* x0, y0,         Point from
* x1, y1,         Point to
* nsamp,          Number of samples
*                 0 = set nsamp automatically { nsamp = sqrt(dx^2 + dy^2) }
*                 else must be >= 2.
* width,          Pixels to sum up or average vertical to line direction
* mode            Normalization mode. 0 = sum, 1 = average
* ytox            y/x pixel relation
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_LinesumBiLin( Fl_RGB_Image *pSrc,   // Source image
                            Tvector *dstvec,      // Destination vector for linesums
                            int ByteComponent,    // What byte to use from the pixel
                            float x0, float y0,     // Point from
                            float x1, float y1,     // Point to
                            int nsamp,            // Number of samples. 0 = set automatically else must be >= 2.
                            int width,            // Pixels to sum up or average vertical to line direction
                            int mode,             // Normalization mode. 0 = sum, 1 = average
                            double ytox)          // y/x pixel relation
{
  YaIPS_RGB_ImgD_t iSrc;
  int ierr;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_LinesumBiLin( &iSrc, dstvec, ByteComponent, x0, y0, x1, y1, nsamp, width, mode, ytox);

  return( ierr);
}

/******************************** End Of File ********************************/

