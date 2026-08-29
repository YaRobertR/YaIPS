/* *********************************************************************
/@
/@ Short-title: runlength code functions
/@
/@ ======================================================================
/@
/@ INDEX:
/@   # Mrl2t_setup    # rl2t_setup    # setup rlcode mode   
/@   # Mrl2t_code     # rl2t_code     # binarize & runlength code int8/16 image
/@   # Mrl2t_codesf   # rl2t_codesf   # binarize & runlength code sfloat image
/@   # Mrl2t_decode   # rl2t_decode   # decode runlength coded image
/@   # Mrl2t_label    # rl2t_label    # label runlenght coded image
/@   # Mrl2t_invert   # rl2t_invert   # invert runlenght coded image
/@   # Mrl2t_meas     # rl2t_meas     # measure runlenght coded label
/@   # Mrl2t_measarea # rl2t_measarea # measure runlenght coded label
/@   # Mrl2t_eros     # rl2t_eros     # erosion in runlength coded image
/@   # Mrl2t_labmea   # rl2t_labmea   # label & measure runlength coded image
/@
/@ USER DESCRIPTION
/@.. rl2t_code   isrc idst thres mode
/@
/@.     Binarize and code 8 or 16 bit image to runlength code
/@
/@      "rl2t_code" binarizes and converts a greyvalue image to a runlenght 
/@      coded form.
/@      Coding is done rowwise.
/@
/@      PARAMETER
/@      isrc           int8/int16  source image
/@      idst           int8        destination image
/@      thres          int16       binarization threshold
/@      mode           int16       0: code if >= thres, 
/@                                 1: code if < thres
/@
/@      RESTRICTIONS
/@      y-size of 'idst' must be equal to y-size of 'isrc'.
/@      Inplace calculation is not possible. 
/@
/@      SEE ALSO
/@      rl2t_codesf, rl2t_decode, rl2t_label.
/@
/@.. rl2t_codesf src dst thres1 thres2 mode
/@
/@.     Binarize and code sfloat image to runlength code
/@
/@      "rl2t_codesf" binarizes and converts a greyvalue image to a  
/@      runlenght coded form.
/@      Coding is done rowwise.
/@
/@      PARAMETER
/@      isrc           sfloat  source image, type sfloat
/@      idst           int8    destination image, type int8
/@      thres1/thres2  sfloat  binarization threshold
/@      mode           int16   0: code if >= thres1, 
/@                             1: code if < thres1
/@                             2: code if >= thres1 && <  thres2
/@                             3: code if <  thres1 || >= thres2
/@
/@      RESTRICTIONS
/@
/@      SEE ALSO
/@      rl2t_code, rl2t_decode, rl2t_label.
/@
/@.. rl2t_setup overlap flags
/@
/@.     Setup rltlabel mode 
/@
/@      overlap = 0: 4-connectivity;
/@      overlap = 1: 8-connectivity;
/@
/@      flags & 1: vertical wrap around in rl2t_labmea
/@      flags & 2: enter labels in rlc image in rl2t_labmea
/@      flags & 4: don't store objects in destination vector in rl2t_labmea
/@
/@      PARAMETER
/@      overlap        int16         
/@      flags          int16                
/@
/@      RESTRICTIONS
/@
/@      SEE ALSO
/@      rl2t_label, rl2t_labmea.
/@
/@.. rl2t_decode isrc idst label backcol labelcol
/@
/@.     Decode runlength code to image
/@
/@      "rl2t_decode" converts a runlenght coded labelled image back to a 
/@      grey level image.
/@      If label >  0 only the named object with "label" is extracted.
/@      If label <= 0 all labels are decoded.
/@      The background is drawn in color "backcol", decoded objects
/@      will be drawn in color "labelcol".
/@      If label < 0 the decoded objects will be drawn in a color
/@      equal to their label id.
/@
/@      PARAMETER
/@      isrc           int8    source image
/@      idst           int8    destination image
/@      label          int16   label
/@      backcol        int16   background color
/@      labelcol       int16   label color
/@
/@      RESTRICTIONS
/@
/@      SEE ALSO
/@      rl2t_code, rl2t_label, rl2t_labmea.
/@
/@.. rl2t_label idst xstart ystart labstart labmax
/@
/@.     Label runlength coded image inplace
/@
/@      ALGORITHM
/@      "idst" is assumed to be a runlenght coded image as calculated
/@      by "rl2t_code".
/@      Objects in "idst" are labelled starting with value "labstart"
/@      until "labmax" is reached.
/@      "xstart" and "ystart" are points in the corresponding
/@      image, where the labelling procedure starts.
/@      If "xstart" and "ystart" are negative, the upper left corner is
/@      used as startpoint.
/@
/@      PARAMETER
/@      idst            int8   source and dest. image with runlength code
/@      xstart          int16  x-position start point for label
/@      ystart          int16  y-position start point for label
/@      labstart        int16  label to start with
/@      labmax          int16  stop if this label has been used
/@
/@      RESTRICTIONS
/@
/@      SEE ALSO
/@      rl2t_code, rl2t_setup, rl2t_labmea.
/@
/@.. rl2t_invert isrc idst label xmax
/@
/@.     Invert labelled runlength coded image
/@
/@      "rl2t_invert" inverts a runlenght coded image produced by 
/@      "rl2t_code" and "rl2t_label" or "rl2t_labmea".
/@      If label >  0 only the named object with "label" is inverted.
/@      If label <= 0 all labels are inverted.
/@      All appertaining to the object 'label' becomes background,
/@      the rest becomes foreground (is runlength coded).
/@
/@      PARAMETER
/@      isrc         int8       source image
/@      idst         int8       destination image
/@      label        int16      label to become background
/@      xmax         int16      x-size of uncoded original
/@
/@      RESTRICTIONS
/@      A frame around isrc with colour of label is required.
/@
/@      SEE ALSO
/@      rl2t_code, rl2t_label, rl2t_labmea.
/@
/@.. rl2t_meas isrc label sv_area sv_xg sv_yg sv_minx sv_maxx sv_miny sv_maxy
/@
/@.     Measure runlenght coded object
/@
/@      In a runlenght coded and labelled image, area, center of gravity
/@      and enclosing rectangle of the object 'label' is calculated and 
/@      writted to shell variables sv_*.
/@
/@      ALGORITHM
/@
/@      PARAMETER
/@      isrc         int8       source image, type int8
/@      label        int16      label of object to be measured
/@      sv_area      string     shell variable for area
/@      sv_xg        string     shell variable for x-gravity point 
/@                              (4 bit precision)
/@      sv_yg        string     shell variable for y-gravity point
/@                              (4 bit precision)
/@      sv_minx      string     shell variable for minimum x
/@      sv_maxx      string     shell variable for maximum x
/@      sv_miny      string     shell variable for minimum y
/@      sv_maxy      string     shell variable for maximum y
/@
/@      RESTRICTIONS
/@
/@      SEE ALSO
/@      rl2t_code, rl2t_label, rl2t_labmea.
/@
/@.. rl2t_measarea isrc label sv_area 
/@
/@.     Measure area of runlenght coded object
/@
/@      In a run lenght coded and labelled image, area of the object 
/@      'label' is calculated and writted to shell variable sv_area.
/@
/@      ALGORITHM
/@
/@      PARAMETER
/@      isrc         int8       source image, type int8
/@      label        int16      label of object to be measured
/@      sv_area      string     shell variable for area
/@
/@      RESTRICTIONS
/@
/@      SEE ALSO
/@      rl2t_meas.
/@
/@.. rl2t_labmea isrc vdst iwork minarea sv_nobj
/@
/@.     Label and measure runlength coded image
/@
/@      All objects in 'isrc' with a size >= 'minarea' are labelled
/@      and measured. Its measurement results are entered into 
/@      object description vector 'vdst'.
/@      'iwork' is a temporary needed work image.
/@
/@      The format of the 32 bit object description vector 'vdst' is as
/@      follows:
/@
/@        index   what
/@        0       xmin of enclosing rectangle first object
/@        1       xmax of enclosing rectangle first object
/@        2       ymin of enclosing rectangle first object
/@        3       ymax of enclosing rectangle first object
/@        4       x-coordinate of gravity point first object
/@                (4 bit precision)
/@        5       y-coordinate of gravity point first object
/@                (4 bit precision)
/@        6       area of first object
/@        7       label of first object
/@        ------------------------------
/@        8       measurement results second object
/@        .....
/@        ------------------------------
/@        16      measurement results third object
/@        .....
/@
/@      PARAMETER
/@      isrc         int8       source image, runlength coded
/@      vdst         int32      destination vector, object description
/@      iwork        int8       work image (temporary objects)
/@      minarea      int16      minimum area of an object to be labelled
/@      sv_nobj      string     shell variable for number of objects
/@ 
/@      RESTRICTIONS
/@
/@      The x-size of iwork must be a multiple of 32.
/@      The (y-size of iwork) / 32 determines the maximum number of objects
/@      in one image row.
/@
/@      SEE ALSO 
/@      rl2t_setup, rl2t_code.
/@
/@.. rl2t_eros isrc idst kmode
/@
/@.     Erode runlength coded image
/@
/@      "rl2t_eros" erodes a runlenght coded image produced by 
/@      "rl2t_code". 
/@
/@      kmode = 0:     .  b  .
/@                     d  e  f     min( b,d,e,f,h )
/@                     .  h  .
/@
/@      kmode = 1:     a  b  c
/@                     d  e  f     min( a,b,c,d,e,f,g,h,i )
/@                     g  h  i
/@
/@
/@      PARAMETER
/@      isrc        int8       source image
/@      idst        int8       destination image
/@      kmode       int16      kernel mode
/@
/@      RESTRICTIONS
/@
/@      SEE ALSO
/@      rl2t_code.
/@
/@.. rl2t_eros2 isrc idst kmode xmax
/@
/@.     Erode runlength coded image
/@
/@      "rl2t_eros2" erodes a runlenght coded image produced by 
/@      "rl2t_code". 
/@      rl2t_eros erodes also from the border inside.
/@      If xmax has a value > 0 rl2t_eros2 keeps a frame around
/@      the image.
/@
/@      kmode = 0:     .  b  .
/@                     d  e  f     min( b,d,e,f,h )
/@                     .  h  .
/@
/@      kmode = 1:     a  b  c
/@                     d  e  f     min( a,b,c,d,e,f,g,h,i )
/@                     g  h  i
/@
/@
/@      PARAMETER
/@      isrc        int8       source image
/@      idst        int8       destination image
/@      kmode       int16      kernel mode
/@      xmax        int16      x-size of uncoded original
/@
/@      RESTRICTIONS
/@
/@      SEE ALSO
/@      rl2t_code.
/@
/@ FUNCTION DESCRIPTION
/@   #include "portab.h"
/@   #include "sip.h"
/@   #include "sipve.h"
/@
/@   int rl2t_setup(overlap)                 setup
/@     int overlap;
/@
/@   int rl2t_setup2(overlap, flags)         setup version 2
/@     int overlap;
/@     int flags;
/@
/@   int rl2t_rdoverlap(overlap)             read overlap
/@     int *overlap;
/@
/@   int rl2t_rdflags(flags)                 read flags
/@     int *flags;
/@
/@   int rl2t_code (src, dst, thres, mode)
/@     Timages *src, *dst;                  source and destination images
/@     int16 thres, mode;
/@
/@   int rl2t_codeRGB (src, dst, rgb, thres, mode)
/@     Timages *src, *dst;                  source and destination images, source must be a TY_INT32
/@     int16 rgb;                           0 for red, 1 for green, 2 for blue component
/@     int16 thres, mode;
/@
/@   int rl2t_codeRGB2(src, dst, rgb, thres, mode)
/@     Timages *src, *dst;                  source and destination images, source must be a TY_INT32
/@     int ThresRLow, ThresRHigh;           R Color must be inside this tolerance
/@     int ThresGLow, ThresGHigh;           and G Color must be inside this tolerance
/@     int ThresBLow, ThresBHigh;           and B Color must be inside this tolerance
/@
/@   int rl2t_codesf (src, dst, thres1, thres2, mode)
/@     Timages *src, *dst;                  source and destination images
/@     int16 mode;
/@	   sfloat thres1, thres2;
/@
/@   int rl2t_decode (src, dst, label, backcol, labelcol)
/@     Timages *src, *dst;                  source and destination images
/@     int16 label, backcol, labelcol;
/@
/@   Turbo decode RLC image. Object 'label' is colored with 'labelcol',
/@   the rest with 'backcol'.
/@   int rl2t_tdecode (src, dst, label, backcol, labelcol)
/@     Timages *src, *dst;                  source and destination images
/@     int16 label, backcol, labelcol;
/@
/@   int rl2t_label(dst, xstart, ystart, labstart, labmax, bolab, bopix)
/@     Timages *dst;         source and destination image
/@     int16 xstart,ystart;  start point for label, upper left corner if neg
/@     int16 labstart;       label to start with
/@     int16 labmax;         stop if this label has been used
/@     int16 *bolab;         returns label of biggest object
/@     int32 *bopix;         returns area of biggest object
/@
/@   int rl2t_invert(src, dst, label, xmax)
/@     Timages *src, *dst;                  source and destination images
/@     int16 label, xmax;
/@
/@   int rl2t_ExractLabelRLCs(src, dst, label)
/@     Timages *src, *dst;                  source and destination images
/@
/@     Extract the RLC's for a specific object:
/@
/@   int rl2t_meas(src, label, area, xgrav, ygrav, xmin, xmax, ymin, ymax);
/@     Timages *isrc;                       source image
/@     int32   *area;
/@     int16   label,*xgrav,*ygrav;
/@     int16   *xmin,*xmax,*ymin,*ymax;
/@ 
/@   int rl2t_meas_area(src, label, area);
/@     Timages *isrc;                       source image
/@     int32   *area;
/@     int16   label;
/@ 
/@   int rl2t_labmea(src, dst, work, minarea)
/@    Timages *src, *work;
/@    Tvector *dst;
/@    int32 minarea;
/@
/@   For labelling, a working list, a free list and an object list 
/@   (destination image) are hold. The working list is allocated
/@   temporarily.
/@
/@   Working (and free) list:
/@                               +-----------------------------+
/@   firstempty +----+   +----+  |    +----+   +----+   +----+ |  +----+
/@   ---------->|next|-->|next|--+ +->|next|-->|next|-->|nil | +->|nil |
/@              |    |   |    |    |  |    |   |    |   |    |    |    |
/@              |    |   |    |    |  |nil |<--|prev|<--|prev|    |    |
/@   first      +----+   +----+    |  +----+   +----+   +----+    +----+
/@   ------------------------------+
/@
/@   +----------------------------------+ \
/@   | measurement variables  next prev | |
/@   | measurement variables  next prev |  > y-size of 'work'
/@   | ...                              | |
/@   +----------------------------------+ /
/@
/@   The destination object list is organized rowwise.
/@    
/@   int rl2t_eros(src, dst, kmode)
/@     Timages *src, *dst;              source and destination images
/@     int16 kmode;
/@
/@   int rl2t_eros2(src, dst, kmode, xmax)
/@     Timages *src, *dst;              source and destination images
/@     int16 kmode, xmax;
/@
/@   RETURN VALUES
/@   Negativ in case of error.
/@
/@ *************************************************************************/


#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "YaIPS_IPS_Interface.h"    // 15.05.2025 RR: Need this for IPS defines
#include "YaIPS_LanguageStrings.h"  // Language string definitions

/*=========================================================================*/

int rl2overlap = RL2CONNECTIVITY_4;     // Label overlap mode
int rl2flags = 0;                       // Flags for run length coding things

/*=========================================================================*/

int rl2t_setup( int overlap) /* setup rltlabel overlap
====================== */
{
  rl2overlap = overlap;
  return(0);

} /* int rl2t_setup() */

int rl2t_setup2( int overlap, int flags) /* setup rltlabel overlap and special flags
============================== */
{
  rl2overlap = overlap;
  rl2flags = flags;
  return(0);

} /* int rl2t_setup() */

int rl2t_rdoverlap( int *overlap) /* read setup overlap 
========================== */
{
  *overlap = rl2overlap;
  return(0);

} /* rl2t_rdoverlap() */

int rl2t_rdflags( int *flags) /* read setup flags 
====================== */
{
  *flags = rl2flags;
  return(0);

} /* rl2t_rdflags() */

#ifdef use_again     // 15.05.2025 RR: Not supported for YaIPS package
static int rl2t_code16( Timages *src, Timages *dst, int16 thres, int16 mode) /* Build RLC from 16 bit image
============================================ */
{
  register int   x, y, maxx, free, xa, yy;
  register int16    *pr;              /* Read pointer in src    */
  register Trl2desc *pRLC;            /* Write pointer in dst   */

  maxx = getxx(src);
  yy = getyy(src);
  for(y = 0; y < yy; y++ ) {

    if(iabort()) return(0);                           /* check CNTRL C */

    pr   = (int16 *)pixadt(0, y, src, int16);
    pRLC = (Trl2desc *)pixad(0, y, dst);
    free = getxx(dst);                                 /* Bytes free in Dest */
    x = 0;
    for(;;) {
      if (mode) {
        while(x < maxx && pr[x] >= thres) x++;
      } else {
        while(x < maxx && pr[x] < thres) x++;
      }
      if(x >= maxx) break;              /* Nothing found at end of line */
      xa = x;                           /* remember start of object     */
      if (mode) {
        while(x < maxx && pr[x] < thres) x++;
      } else {
        while(x < maxx && pr[x] >= thres) x++;
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

  return(0);

} /* static int rl2t_code16() */
#endif // use_again // 15.05.2025 RR: Not supported for YaIPS package

#ifdef use_again     // 15.05.2025 RR: Not supported for YaIPS package
int rl2t_codesf( Timages *src, Timages *dst, sfloat thres1, sfloat thres2, int16 mode) /* Build RLC from sfloat image
============================================== */
{
  register int   x, y, maxx, free, xa, yy;
  register sfloat *pr;                /* Read pointer in src    */
  register Trl2desc *pRLC;            /* Write pointer in dst   */

  if(uticheck(src, DV_HOST, TY_SFLOAT)) return(-2);
  if(uticheck(dst, DV_HOST, TY_ANY)) return(-3);

  if(getyy(src) != getyy(dst)) {
    errstring = ERR_IPS_YSIZE_SRC_DST_DIFF;
    return(-2);
  }
  maxx = getxx(src);
  yy = getyy(src);
  for(y = 0; y < yy; y++ ) {
    if(iabort()) return(0);                           /* check CNTRL C */
    pr = (sfloat *)pixadt(0, y, src, sfloat);
    pRLC = (Trl2desc *)pixad(0, y, dst);
    free = getxx(dst);                                 /* Bytes free in Dest */
    x = 0;
    for(;;) {
      switch(mode) {
      case 3:
        while(x < maxx && pr[x] >= thres1 && pr[x] < thres2) x++;
        break;
      case 2:
        while(x < maxx && (pr[x] < thres1 || pr[x] >= thres2)) x++;
        break;
      case 1:
        while(x < maxx && pr[x] >= thres1) x++;
        break;
      default:
        while(x < maxx && pr[x] < thres1) x++;
        break;
      }
      if(x >= maxx) break;              /* Nothing found at end of line */
      xa = x;                           /* remember start of object     */
      switch(mode) {
      case 3:
        while(x < maxx && (pr[x] < thres1 || pr[x] >= thres2)) x++;
        break;
      case 2:
        while(x < maxx && pr[x] >= thres1 && pr[x] < thres2) x++;
        break;
      case 1:
        while(x < maxx && pr[x] < thres1) x++;
        break;
      default:
        while(x < maxx && pr[x] >= thres1) x++;
        break;
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

  return(0);

} /* int rl2t_codesf() */
#endif // use_again // 15.05.2025 RR: Not supported for YaIPS package

int rl2t_code( Timages *src, Timages *dst, int16 thres, int16 mode) /* Build runlength code from image
=================================== */
{
  register int   x, y, maxx, free, xa, yy;
  register unsigned int8 *pr;         /* Read pointer in src    */
  register Trl2desc *pRLC;            /* Write pointer in dst   */

  if(uticheck(src, DV_HOST, TY_ANY)) return(-2);
  if(uticheck(dst, DV_HOST, TY_ANY)) return(-3);

  if(getyy(src) != getyy(dst)) {
    errstring = ERR_IPS_YSIZE_SRC_DST_DIFF;
    return(-2);
  }

#ifdef use_again     // 15.05.2025 RR: Not supported for YaIPS package
  if ((getyp(src) == TY_INT16) && (getyp(dst) == TY_BYTE )) {
    return(rl2t_code16(src, dst, thres, mode));
  } else
#endif // use_again // 15.05.2025 RR: Not supported for YaIPS package
  if ((getyp(src) == TY_BYTE) && (getyp(dst) == TY_BYTE )) {
    /* code follows */
  } else {
    errstring = ERR_IPS_DATA_COMP_NO_SUPP;
    return(-1);
  }

  yy = getyy(src);
  maxx = getxx(src);
  for(y = 0; y < yy; y++ ) {
    if(iabort()) return(0);                           /* check CNTRL C */
    pr = (unsigned int8 *)pixad(0, y, src);
    pRLC = (Trl2desc *)pixad(0, y, dst);
    free = getxx(dst);                           /* Bytes free in Dest */
    x = 0;
    for(;;) {
      if (mode) {
        while(x < maxx && (pr[x] & 0xff) >= thres) x++;
      } else {
        while(x < maxx && (pr[x] & 0xff) < thres) x++;
      }
      if(x >= maxx) break;              /* Nothing found at end of line */
      xa = x;                           /* remember start of object     */
      if (mode) {
        while(x < maxx && (pr[x] & 0xff) < thres) x++;
      } else {
        while(x < maxx && (pr[x] & 0xff) >= thres) x++;
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
      if ((rl2flags & RL2AREA_SUPBPIX) == 0) {
        // pixel area
        pRLC->rlxaSubPix = pRLC->rlxa << RL2T_SUBPIX_SHIFT;  /* in subpixel */
        pRLC->rlxeSubPix = pRLC->rlxe << RL2T_SUBPIX_SHIFT;  /* in subpixel */
        pRLC->rlxeSubPix += RL2T_SUBPIX_FAC - 1;             /* point to end of this pixel */
      } else {
        
        int32 v1, v2, vBase;

        // supbixel area

        // begin of RLC

        if( pRLC->rlxa == 0) {   // start at addes 0

          // start at begin of image
          pRLC->rlxaSubPix = 0; // is also at address 0
        } else {

          // have one pixel before

          v1 = pr[ pRLC->rlxa - 1];
          v2 = pr[ pRLC->rlxa];

          if (mode) {

            // NOTE: v1 > v2 

            vBase = v1 - v2 + 1;
            pRLC->rlxaSubPix = (RL2T_SUBPIX_FAC * (v1 - (thres - 1)) + (vBase >> 1)) / vBase;
           
            pRLC->rlxaSubPix += pRLC->rlxa << RL2T_SUBPIX_SHIFT;
          } else {

            // NOTE: v1 < v2 

            vBase = v2 - v1 + 1;
            pRLC->rlxaSubPix = (RL2T_SUBPIX_FAC * (thres - v1) + (vBase >> 1)) / vBase;
            
            pRLC->rlxaSubPix += pRLC->rlxa << RL2T_SUBPIX_SHIFT;
          }
        }

        // end of RLC

        if( pRLC->rlxe >= maxx - 1) {   // at last pixel

          // end at end of image
          pRLC->rlxeSubPix = (maxx << RL2T_SUBPIX_SHIFT) - 1; // is last subpixel
        } else {

          // have one pixel after

          v1 = pr[ pRLC->rlxe];
          v2 = pr[ pRLC->rlxe + 1];

          if (mode) {

            // NOTE: v1 < v2 

            vBase = v2 - v1 + 1;
            pRLC->rlxeSubPix = (RL2T_SUBPIX_FAC * ((thres - 1) - v1) + (vBase >> 1)) / vBase;
            
            pRLC->rlxeSubPix += pRLC->rlxe << RL2T_SUBPIX_SHIFT;
          } else {

            // NOTE: v1 > v2 

            vBase = v1 - v2 + 1;
            pRLC->rlxeSubPix = (RL2T_SUBPIX_FAC * (v1 - thres) + (vBase >> 1)) / vBase;
            
            pRLC->rlxeSubPix += pRLC->rlxe << RL2T_SUBPIX_SHIFT;
          }
        }
      }
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

  return(0);

} /* int rl2t_code() */

#ifdef use_again     // 15.05.2025 RR: Not supported for YaIPS package
int rl2t_codeRGB( Timages *src, Timages *dst, int16 rgb, int16 thres, int16 mode) /* Build runlength code from image
=================================== */
{
  register int   x, y, maxx, free, xa, yy;
  register unsigned int8 *pr;         /* Read pointer in src    */
  register Trl2desc *pRLC;            /* Write pointer in dst   */
  Trgb *pRGB;

  if(uticheck(src, DV_HOST, TY_ANY)) return(-2);
  if(uticheck(dst, DV_HOST, TY_ANY)) return(-3);

  if(getyy(src) != getyy(dst)) {
    errstring = ERR_IPS_YSIZE_SRC_DST_DIFF;
    return(-2);
  }

  if ((getyp(src) == TY_INT32) && (getyp(dst) == TY_BYTE )) {
    /* code follows */
  } else {
    errstring = ERR_IPS_DATA_COMP_NO_SUPP;
    return(-1);
  }

  yy = getyy(src);
  maxx = getxx(src);
  for(y = 0; y < yy; y++ ) {
    if(iabort()) return(0);                           /* check CNTRL C */

    // get address of source line
    pRGB = (Trgb *)pixadt(0, y, src, int32);

    if( rgb == 1) {          // green
      pr = (unsigned int8 *)&pRGB->g;
    } else if( rgb == 2) {   // blue
      pr = (unsigned int8 *)&pRGB->b;
    } else {                 // red or not known
      pr = (unsigned int8 *)&pRGB->r;
    }

    // ...
    pRLC = (Trl2desc *)pixad(0, y, dst);
    free = getxx(dst);                           /* Bytes free in Dest */
    x = 0;
    for(;;) {
      if (mode) {
        while(x < maxx && (pr[x << 2] & 0xff) >= thres) x++;
      } else {
        while(x < maxx && (pr[x << 2] & 0xff) < thres) x++;
      }
      if(x >= maxx) break;              /* Nothing found at end of line */
      xa = x;                           /* remember start of object     */
      if (mode) {
        while(x < maxx && (pr[x << 2] & 0xff) < thres) x++;
      } else {
        while(x < maxx && (pr[x << 2] & 0xff) >= thres) x++;
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
      if ((rl2flags & RL2AREA_SUPBPIX) == 0) {
        // pixel area
        pRLC->rlxaSubPix = pRLC->rlxa << RL2T_SUBPIX_SHIFT;  /* in subpixel */
        pRLC->rlxeSubPix = pRLC->rlxe << RL2T_SUBPIX_SHIFT;  /* in subpixel */
        pRLC->rlxeSubPix += RL2T_SUBPIX_FAC - 1;             /* point to end of this pixel */
      } else {
        
        int32 v1, v2, vBase;

        
        
        // supbixel area

        // begin of RLC

        if( pRLC->rlxa == 0) {   // start at addes 0

          // start at begin of image
          pRLC->rlxaSubPix = 0; // is also at address 0
        } else {

          // have one pixel before

          v1 = pr[ (pRLC->rlxa - 1) << 2];
          v2 = pr[ pRLC->rlxa << 2];

          if (mode) {

            // NOTE: v1 > v2 

            vBase = v1 - v2 + 1;
            pRLC->rlxaSubPix = (RL2T_SUBPIX_FAC * (v1 - (thres - 1)) + (vBase >> 1)) / vBase;
           
            pRLC->rlxaSubPix += pRLC->rlxa << RL2T_SUBPIX_SHIFT;
          } else {

            // NOTE: v1 < v2 

            vBase = v2 - v1 + 1;
            pRLC->rlxaSubPix = (RL2T_SUBPIX_FAC * (thres - v1) + (vBase >> 1)) / vBase;
            
            pRLC->rlxaSubPix += pRLC->rlxa << RL2T_SUBPIX_SHIFT;
          }
        }

        // end of RLC

        if( pRLC->rlxe >= maxx - 1) {   // at last pixel

          // end at end of image
          pRLC->rlxeSubPix = (maxx << RL2T_SUBPIX_SHIFT) - 1; // is last subpixel
        } else {

          // have one pixel after

          v1 = pr[ pRLC->rlxe << 2];
          v2 = pr[ (pRLC->rlxe + 1) << 2];

          if (mode) {

            // NOTE: v1 < v2 

            vBase = v2 - v1 + 1;
            pRLC->rlxeSubPix = (RL2T_SUBPIX_FAC * ((thres - 1) - v1) + (vBase >> 1)) / vBase;
            
            pRLC->rlxeSubPix += pRLC->rlxe << RL2T_SUBPIX_SHIFT;
          } else {

            // NOTE: v1 > v2 

            vBase = v1 - v2 + 1;
            pRLC->rlxeSubPix = (RL2T_SUBPIX_FAC * (v1 - thres) + (vBase >> 1)) / vBase;
            
            pRLC->rlxeSubPix += pRLC->rlxe << RL2T_SUBPIX_SHIFT;
          }
        }
      }
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

  return(0);

} /* int rl2t_codeRGB() */
#endif // use_again // 15.05.2025 RR: Not supported for YaIPS package

#ifdef use_again     // 15.05.2025 RR: Not supported for YaIPS package
int rl2t_codeRGB2( Timages *src, Timages *dst,     /* Build runlength code from image */
                   int ThresRLow, int ThresRHigh,  // R Color must be inside this tolerance
                   int ThresGLow, int ThresGHigh,  // and G Color must be inside this tolerance
                   int ThresBLow, int ThresBHigh)  // and B Color must be inside this tolerance
{
  register int   x, y, maxx, free, xa, yy;
  register Trl2desc *pRLC;            /* Write pointer in dst   */
  Trgb *pRGB;

  if(uticheck(src, DV_HOST, TY_ANY)) return(-2);
  if(uticheck(dst, DV_HOST, TY_ANY)) return(-3);

  if(getyy(src) != getyy(dst)) {
    errstring = ERR_IPS_YSIZE_SRC_DST_DIFF;
    return(-2);
  }

  if ((getyp(src) == TY_INT32) && (getyp(dst) == TY_BYTE )) {
    /* code follows */
  } else {
    errstring = ERR_IPS_DATA_COMP_NO_SUPP;
    return(-1);
  }

  yy = getyy(src);
  maxx = getxx(src);
  for(y = 0; y < yy; y++ ) {
    if(iabort()) return(0);                           /* check CNTRL C */

    // get address of source line
    pRGB = (Trgb *)pixadt(0, y, src, int32);

    // ...
    pRLC = (Trl2desc *)pixad(0, y, dst);
    free = getxx(dst);                           /* Bytes free in Dest */
    x = 0;
    for(;;) {

      while( x < maxx &&
        ((pRGB->r & 0xff) < ThresRLow || (pRGB->r & 0xff) > ThresRHigh ||    // R Color is outside this tolerance
         (pRGB->g & 0xff) < ThresGLow || (pRGB->g & 0xff) > ThresGHigh ||    // or G Color is outside this tolerance
         (pRGB->b & 0xff) < ThresBLow || (pRGB->b & 0xff) > ThresBHigh) ) {  // or B Color is outside this tolerance
          
        pRGB++;
        x++;
      }

      if(x >= maxx) break;              /* Nothing found at end of line */
      xa = x;                           /* remember start of object     */

      while( x < maxx &&
        ((pRGB->r & 0xff) >= ThresRLow && (pRGB->r & 0xff) <= ThresRHigh &&    // R Color must be inside this tolerance
         (pRGB->g & 0xff) >= ThresGLow && (pRGB->g & 0xff) <= ThresGHigh &&    // and G Color must be inside this tolerance
         (pRGB->b & 0xff) >= ThresBLow && (pRGB->b & 0xff) <= ThresBHigh) ) {  // and B Color must be inside this tolerance
          
        pRGB++;
        x++;
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
      if ((rl2flags & RL2AREA_SUPBPIX) == 0) {
        // pixel area
        pRLC->rlxaSubPix = pRLC->rlxa << RL2T_SUBPIX_SHIFT;  /* in subpixel */
        pRLC->rlxeSubPix = pRLC->rlxe << RL2T_SUBPIX_SHIFT;  /* in subpixel */
        pRLC->rlxeSubPix += RL2T_SUBPIX_FAC - 1;             /* point to end of this pixel */
      } else {
        
        // supbixel area

        // begin of RLC

        if( pRLC->rlxa == 0) {   // start at addes 0

          // start at begin of image
          pRLC->rlxaSubPix = 0; // is also at address 0
        } else {

          // NO subpixel support

          pRLC->rlxaSubPix = pRLC->rlxa << RL2T_SUBPIX_SHIFT;
        }

        // end of RLC

        if( pRLC->rlxe >= maxx - 1) {   // at last pixel

          // end at end of image
          pRLC->rlxeSubPix = (maxx << RL2T_SUBPIX_SHIFT) - 1; // is last subpixel
        } else {

          // NO subpixel support
            
          pRLC->rlxeSubPix = pRLC->rlxe << RL2T_SUBPIX_SHIFT;
        }
      }
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

  return(0);

} /* int rl2t_codeRGB2() */
#endif // use_again // 15.05.2025 RR: Not supported for YaIPS package

#ifdef use_again     // 15.05.2025 RR: Not supported for YaIPS package
int rl2t_codeRGB3( Timages *src, Timages *dst,           /* Build runlength code from image */
                   int MainColorNr,                          // reference color with heightes value, o = R, 1 = G, 2 = B
                   int MainThresLow, int MainThresHigh,      // value range for main color component
                   int Radio1ThresLow, int Radio1ThresHigh,  // range for other color component
                   int Radio2ThresLow, int Radio2ThresHigh)  // range for other color component
{
  register int   x, y, maxx, free, xa, yy, MainColor, Radio1Value, Radio2Value;
  register Trl2desc *pRLC;            /* Write pointer in dst   */
  Trgb *pRGB;

  if(uticheck(src, DV_HOST, TY_ANY)) return(-2);
  if(uticheck(dst, DV_HOST, TY_ANY)) return(-3);

  if(getyy(src) != getyy(dst)) {
    errstring = ERR_IPS_YSIZE_SRC_DST_DIFF;
    return(-2);
  }

  if ((getyp(src) == TY_INT32) && (getyp(dst) == TY_BYTE )) {
    /* code follows */
  } else {
    errstring = ERR_IPS_DATA_COMP_NO_SUPP;
    return(-1);
  }

  // ...

  if( MainThresLow < 1) {         // main color must minimum have this value

    MainThresLow = 1;             // clip to have no division by zero
  }

  // ...

  yy = getyy(src);
  maxx = getxx(src);
  for(y = 0; y < yy; y++ ) {
    if(iabort()) return(0);                           /* check CNTRL C */

    // get address of source line
    pRGB = (Trgb *)pixadt(0, y, src, int32);

    // ...
    pRLC = (Trl2desc *)pixad(0, y, dst);
    free = getxx(dst);                           /* Bytes free in Dest */
    x = 0;

    if( MainColorNr == 0) {         // main color is red

      for(;;) {

        while( x < maxx) {

          MainColor = (pRGB->r & 0xff);

          if( MainColor < MainThresLow || MainColor > MainThresHigh) {         // main Color is outside this tolerance

            pRGB++;
            x++;
            continue;
          }

          Radio1Value = (pRGB->g & 0xff) * (1 << 10) / MainColor;

          if( Radio1Value < Radio1ThresLow || Radio1Value > Radio1ThresHigh) {  // radio 1 value is outside this tolerance

            pRGB++;
            x++;
            continue;
          }

          Radio2Value = (pRGB->b & 0xff) * (1 << 10) / MainColor;
          if( Radio2Value < Radio2ThresLow || Radio2Value > Radio2ThresHigh) {  // radio 2 value is outside this tolerance

            pRGB++;
            x++;
            continue;
          }

          // got one
          break;
        }

        if(x >= maxx) break;              /* Nothing found at end of line */
        xa = x;                           /* remember start of object     */

        while( x < maxx) {

          MainColor = (pRGB->r & 0xff);

          if( MainColor >= MainThresLow && MainColor <= MainThresHigh) {         // main Color is inside this tolerance

            Radio1Value = (pRGB->g & 0xff) * (1 << 10) / MainColor;

            if( Radio1Value >= Radio1ThresLow && Radio1Value <= Radio1ThresHigh) {  // radio 1 value is onside this tolerance

              Radio2Value = (pRGB->b & 0xff) * (1 << 10) / MainColor;
              if( Radio2Value >= Radio2ThresLow && Radio2Value <= Radio2ThresHigh) {  // radio 2 value is inside this tolerance

                pRGB++;
                x++;
              } else {

                break;
              }
            } else {
              break;
            }
          } else {
            break;
          }

          // got one in range, test next
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

        // pixel area (NO subpixel support)
        pRLC->rlxaSubPix = pRLC->rlxa << RL2T_SUBPIX_SHIFT;  /* in subpixel */
        pRLC->rlxeSubPix = pRLC->rlxe << RL2T_SUBPIX_SHIFT;  /* in subpixel */
        pRLC->rlxeSubPix += RL2T_SUBPIX_FAC - 1;             /* point to end of this pixel */
        pRLC++;
      } /* end for x */

    } else if( MainColorNr == 1) {  // main color is green

      for(;;) {

        while( x < maxx) {

          MainColor = (pRGB->g & 0xff);

          if( MainColor < MainThresLow || MainColor > MainThresHigh) {         // main Color is outside this tolerance

            pRGB++;
            x++;
            continue;
          }

          Radio1Value = (pRGB->r & 0xff) * (1 << 10) / MainColor;

          if( Radio1Value < Radio1ThresLow || Radio1Value > Radio1ThresHigh) {  // radio 1 value is outside this tolerance

            pRGB++;
            x++;
            continue;
          }

          Radio2Value = (pRGB->b & 0xff) * (1 << 10) / MainColor;
          if( Radio2Value < Radio2ThresLow || Radio2Value > Radio2ThresHigh) {  // radio 2 value is outside this tolerance

            pRGB++;
            x++;
            continue;
          }

          // got one
          break;
        }

        if(x >= maxx) break;              /* Nothing found at end of line */
        xa = x;                           /* remember start of object     */

        while( x < maxx) {

          MainColor = (pRGB->g & 0xff);

          if( MainColor >= MainThresLow && MainColor <= MainThresHigh) {         // main Color is inside this tolerance

            Radio1Value = (pRGB->r & 0xff) * (1 << 10) / MainColor;

            if( Radio1Value >= Radio1ThresLow && Radio1Value <= Radio1ThresHigh) {  // radio 1 value is onside this tolerance

              Radio2Value = (pRGB->b & 0xff) * (1 << 10) / MainColor;
              if( Radio2Value >= Radio2ThresLow && Radio2Value <= Radio2ThresHigh) {  // radio 2 value is inside this tolerance

                pRGB++;
                x++;
              } else {

                break;
              }
            } else {
              break;
            }
          } else {
            break;
          }

          // got one in range, test next
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

        // pixel area (NO subpixel support)
        pRLC->rlxaSubPix = pRLC->rlxa << RL2T_SUBPIX_SHIFT;  /* in subpixel */
        pRLC->rlxeSubPix = pRLC->rlxe << RL2T_SUBPIX_SHIFT;  /* in subpixel */
        pRLC->rlxeSubPix += RL2T_SUBPIX_FAC - 1;             /* point to end of this pixel */
        pRLC++;
      } /* end for x */

    } else {                        // main color must be blue

      for(;;) {

        while( x < maxx) {

          MainColor = (pRGB->b & 0xff);

          if( MainColor < MainThresLow || MainColor > MainThresHigh) {         // main Color is outside this tolerance

            pRGB++;
            x++;
            continue;
          }

          Radio1Value = (pRGB->r & 0xff) * (1 << 10) / MainColor;

          if( Radio1Value < Radio1ThresLow || Radio1Value > Radio1ThresHigh) {  // radio 1 value is outside this tolerance

            pRGB++;
            x++;
            continue;
          }

          Radio2Value = (pRGB->g & 0xff) * (1 << 10) / MainColor;
          if( Radio2Value < Radio2ThresLow || Radio2Value > Radio2ThresHigh) {  // radio 2 value is outside this tolerance

            pRGB++;
            x++;
            continue;
          }

          // got one
          break;
        }

        if(x >= maxx) break;              /* Nothing found at end of line */
        xa = x;                           /* remember start of object     */

        while( x < maxx) {

          MainColor = (pRGB->b & 0xff);

          if( MainColor >= MainThresLow && MainColor <= MainThresHigh) {         // main Color is inside this tolerance

            Radio1Value = (pRGB->r & 0xff) * (1 << 10) / MainColor;

            if( Radio1Value >= Radio1ThresLow && Radio1Value <= Radio1ThresHigh) {  // radio 1 value is onside this tolerance

              Radio2Value = (pRGB->g & 0xff) * (1 << 10) / MainColor;
              if( Radio2Value >= Radio2ThresLow && Radio2Value <= Radio2ThresHigh) {  // radio 2 value is inside this tolerance

                pRGB++;
                x++;
              } else {

                break;
              }
            } else {
              break;
            }
          } else {
            break;
          }

          // got one in range, test next
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

        // pixel area (NO subpixel support)
        pRLC->rlxaSubPix = pRLC->rlxa << RL2T_SUBPIX_SHIFT;  /* in subpixel */
        pRLC->rlxeSubPix = pRLC->rlxe << RL2T_SUBPIX_SHIFT;  /* in subpixel */
        pRLC->rlxeSubPix += RL2T_SUBPIX_FAC - 1;             /* point to end of this pixel */
        pRLC++;
      } /* end for x */

    }

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

  return(0);

} /* int rl2t_codeRGB3() */
#endif // use_again // 15.05.2025 RR: Not supported for YaIPS package

int rl2t_decode( Timages *src, Timages * dst, int16 label, int16 bcol, int16 lcol) /* decode runlength coded image
=========================================== */
{
  register int x, maxx, free, col;
  int yoff;
  register int8 *pw;
  register Trl2desc *pRLC_read, *pRLC_last;      /* read pointer in dst   */
  int foundany;

  foundany = 0;
  if(uticheck(src, DV_HOST, TY_ANY)) return(-2);
  if(uticheck(dst, DV_HOST, TY_ANY)) return(-3);

  if (!((getyp(src) == TY_BYTE) && (getyp(dst) == TY_BYTE ))) {
    errstring = ERR_IPS_DATA_COMP_NO_SUPP;
    return(-1);
  }

  maxx = getxx(dst);
  for(yoff = 0; yoff < getyy(src); yoff++) {
    if(iabort()) return(0);                     /* check CNTRL C */
    pRLC_read = (Trl2desc *)pixad(0, yoff, src);
    pw = pixad(0, yoff, dst);
    free = getxx(src);
    x = 0;
    for(;;) {
      free -= sizeof(Trl2desc);
      if(free < 0) {                            /* no memory free in line */
        errstring = ERR_IPS_XS_SRC_OVERFLOW;
        return(RL2TOOLS_MEMOVFL);
      }

      // ...

      pRLC_last = pRLC_read;   // point to current RLC
      pRLC_read++;             // advance read pointer (for loop breaks)

      // ...

      if(pRLC_last->rlxa < 0) break;   /* End of List         */
      if(pRLC_last->rlxa >= maxx || pRLC_last->rlxe >= maxx ||
        pRLC_last->rlxa < 0     || pRLC_last->rlxe < 0     ||
        pRLC_last->rlxa >  pRLC_last->rlxe ) {
          PRINTF4("rl2t_decode: xa %d xe %d, thres: %d...%d\n",
            pRLC_last->rlxa, pRLC_last->rlxe, 0, maxx);
          errstring = ERR_IPS_XA_XE_RLC_ILLEGAL;
          return(-5);
      }
      while(x < pRLC_last->rlxa) {
        *pw++ = (int8)bcol;             /* fill to begin of line */
        x++;
      }
      col = bcol;
      if(label > 0) {
        if(label == pRLC_last->rlab) {
          col = lcol;
          foundany = 1;
        }
      } else if(label == 0) {
        col = lcol;
        foundany = 1;
      } else {                     /* label < 0 */
        col = pRLC_last->rlab;
        foundany = 1;
      }
      while(x <= pRLC_last->rlxe) {
        *pw++ = col;             /* fill line */
        x++;
      }
    }
    while(x < maxx) {
      *pw++ = (int8)bcol;             /* fill to end of line */
      x++;
    }
  }
#ifdef use_again
  if(foundany == 0) {
    errstring = ERR_IPS_LABEL_NOT_FOUND;
    return(-6);
  }
#else
  // 25.05.2025 RR: Only check for drawn labels if request to draw a specific label
  if( label > 0 && foundany == 0) {
    errstring = ERR_IPS_LABEL_NOT_FOUND;
    return(-6);
  }
#endif
  return(0);

} /* int rl2t_decode() */

int rl2t_tdecode( Timages *src, Timages *dst, int16 label, int16 bcol, int16 lcol) /* turbo decode RLC image
                                     ===========================================     NOTE: no checks are done,
                                                                                     only bcol/lcol are used */
{
  register int x;
  register int16 col;
  register int maxx, yy;
  int yoff;
  register int8 *pw;
  register Trl2desc *pRLC_read, *pRLC_last;      /* read pointer in dst   */

  if(uticheck(src, DV_HOST, TY_ANY)) return(-2);
  if(uticheck(dst, DV_HOST, TY_ANY)) return(-3);

  if (!((getyp(src) == TY_BYTE) && (getyp(dst) == TY_BYTE ))) {
    errstring = ERR_IPS_DATA_COMP_NO_SUPP;
    return(-1);
  }

  yy = getyy(dst);
  maxx = getxx(dst);
  for(yoff = 0; yoff < yy; yoff++) {
    if(iabort()) return(0);                     /* check CNTRL C */
    pRLC_read = (Trl2desc *)pixad(0, yoff, src);
    pw = pixad(0, yoff, dst);
    x = 0;
    for(;;) {

      // ...

      pRLC_last = pRLC_read;   // point to current RLC
      pRLC_read++;             // advance read pointer (for loop breaks)

      // ...

      if(pRLC_last->rlxa < 0) break;   /* End of List         */
      while(x < pRLC_last->rlxa) {
        *pw++ = (int8)bcol;             /* fill to begin of line */
        x++;
      }
      col = bcol;
      if(label == pRLC_last->rlab) col = lcol;
      while(x <= pRLC_last->rlxe) {
        *pw++ = (int8)col;             /* fill line */
        x++;
      }
    }
    while(x < maxx) {
      *pw++ = (int8)bcol;             /* fill to end of line */
      x++;
    }
  }
  return(0);

} /* int rl2t_tdecode() */

/* ======================= rl2t_label ====================================
This is a simpler version of rllab.dsa to simulate and for documentation
of the algorithm.

Variables are static so we can access them in LINSCN 

======================================================================== */

typedef struct {
  int16 label;            /* current processed label               */
  int16 ycur;             /* current line to test (desc. is label) */
  int16 ychk;             /* current line to test (desc. is 0)     */
  int16 foundone;         /* TRUE if anything found in LINSCN      */
  int16 firstfound;       /* first line something found in scan    */
  int16 lastfound;        /* last line something found in scan     */
  int16 xfnd,yfnd;        /* position of first label */
  int32 npix;
} Trl2t_label_desc;

static Trl2t_label_desc rltld;

/* ------------------------- static labler functions ------------------- */

/*--------------------------------------------------------------------
LINSCN
Scans the CUR line for a descriptor, which is already labelled
with LABEL.
Scans the CHK line (next line) for a descriptor, which is still
unlabelled.
Checks if both descriptors overlap. Reads another descriptor if not.
Else, labels the new CHK descriptor and updates DRAM and statistics.
------------------------------------------------------------------------- */
static int LINSCN( Timages *dst, int overlap) /*
================================ */
{
  int16 curxa, curxe, curlab;      /* Descriptor in current line */
  int16 chkxa, chkxe, chklab;      /* Descriptor off check line  */
#ifdef use_again
  int16 xchk;                      /* position of LABEL (not XA) */
#else
  Trl2desc *pcur, *pchk, *xchk;
#endif

  chkxa = -1;
  /* start - preincrement + label_offset  */
#ifdef use_again
  xchk = - (int)(sizeof(Trl2desc) / sizeof(int16)) + 2;
#else
  xchk = (Trl2desc *)pixad(0,rltld.ychk,dst);
  xchk -= 1;  // preincrement
#endif
  rltld.foundone = FALSE;          /* Nothing found until now in this line */
  pcur = (Trl2desc *)pixad(0,rltld.ycur,dst);
  pchk = (Trl2desc *)pixad(0,rltld.ychk,dst);

curnxt:
  curxa  = pcur->rlxa;
  curxe  = pcur->rlxe;
  curlab = pcur->rlab;
  pcur++;

  if(curxa < 0)       goto lineol;
  if(curlab != rltld.label) goto curnxt;
  if( chkxa >= 0) goto testit;          /* CHK descriptor missing ? */

chknxt:
#ifdef use_again
  xchk += sizeof(Trl2desc) / sizeof(int16);       /* update position */
#else
  xchk += 1;       /* update position */
#endif
  chkxa  = pchk->rlxa;
  chkxe  = pchk->rlxe;
  chklab = pchk->rlab;
  pchk++;

  if(chkxa < 0)   goto lineol;
  if(chklab != 0) goto chknxt;

testit:                          /* check if the descriptors overlap */
  if(chkxe < curxa - overlap) goto chknxt;
  if(curxe < chkxa - overlap) goto curnxt;

  /* now we have an overlap ,
  an unlabelled CHK and a CUR descriptor with LABEL */
  chklab = rltld.label;
  rltld.npix += chkxe + 1 - chkxa;
#ifdef use_again
  pw = (int16 *)pixad(xchk * 2,rltld.ychk,dst);
  *pw = chklab;                           /* do not update chkxa,chkxe */
#else
  xchk->rlab = chklab;                      /* update label, do not update chkxa,chkxe */
#endif
  // ...
  rltld.foundone = TRUE;                  /* anything found in this line*/
  if(rltld.firstfound < 0) 
    rltld.firstfound = rltld.ycur;        /* first line something found */
  rltld.lastfound = rltld.ychk;           /* last line something found  */
  goto chknxt;                            /* thisone is updated, try next */

lineol:                         /* end of line */
  return(0);

} /* static int LINSCN() */
/*-----------------------------------------------------------------
LABFND
Done as in microcode. Seems to do right !!
Change: TABSET wird hier ausserhalb von LABFND gemacht. Besser.
NPIX,NDESC werden im jetzigen microcode nicht updated,
sondern sogar NACH Aufruf von labfnd geloescht !
------------------------------------------------------------------ */
static int LABFND( Timages *dst) /*
======================= */
{
  int16 curxa, curxe, curlab;
  int16 yoff, xoff;
  Trl2desc *pr1, *pw;

  /* PRINTF3("\nLABFND: rltld.xfnd = %3d rltld.yfnd = %3d label = %3d\n",
  rltld.xfnd,rltld.yfnd,rltld.label); */
  yoff = rltld.yfnd - 1;
fndlop:
  yoff++;
  if(yoff >= getyy(dst)) goto fndend;

  pr1 = (Trl2desc *)pixad(0,yoff,dst);
#ifdef use_again
  xoff = - (int)(sizeof(Trl2desc) / sizeof(int16)) + 2; /* index to label */
#else
  xoff = -1;  /* preincrement */
#endif
fndnxt:
#ifdef use_again
  xoff += sizeof(Trl2desc) / sizeof(int16);       /* update position */
#else
  xoff += 1;                                      /* update position */
#endif
  curxa  = pr1->rlxa;
  curxe  = pr1->rlxe;
  curlab = pr1->rlab;
  pr1++; 

  if(curxa < 0)    goto fndeol;
  if(curlab != 0)  goto fndnxt;

  /* NOW WE GOT IT */
#ifdef use_again
  pw = (int16 *)pixad(xoff * 2,yoff,dst);
  *pw = rltld.label;
#else
  pw = (Trl2desc *)pixad(xoff * sizeof(Trl2desc),yoff,dst);
  pw->rlab = rltld.label;
#endif

  rltld.yfnd = yoff;           /* remember position of found descript */
  rltld.xfnd = curxe;          /* or CURXA ???  */
  rltld.npix += curxe + 1 - curxa;

  /* PRINTF3("LABFND: found lab rltld.xfnd = %3d rltld.yfnd = %3d label = %3d\n",
  rltld.xfnd,rltld.yfnd,rltld.label);
  PRINTF3("LABFND: found label curxa = %3d curxe = %3d curlab = %3d\n",
  curxa,curxe,curlab); */
  return(0);                   /* return success */

fndeol:
  rltld.xfnd = -1;             /* use XFND only in the first line */
  goto fndlop;

fndend:
  /* PRINTF0("LABFND: nothing found\n"); */
  return(-1);          /* nothing found */

} /* static int LABFND() */

/* ------------------------- public labler ------------------------------- */

int rl2t_label( Timages *dst, int16 xstart, int16 ystart,
                      int16 labmin, int16 labmax, int16 *bolab, int32 *bopix) /*
========================================================== */
{
  int16 firstscan;              /* first line to scan             */
  int16 lastscan;               /* last line to scan              */
  int   ierr;                   /* usually DSP D-register         */
  int overlap;          /* 0 = 4-connectivity, 1 = 8-connectivity */

  rl2t_rdoverlap(&overlap);

  rltld.label = labmin;
  rltld.xfnd = xstart;
  rltld.yfnd = ystart;
  *bopix = 0;
  *bolab = 0;

lablop:
  rltld.npix = 0;                      /* clear statistics */
  ierr = LABFND(dst);
  if(ierr < 0) goto labend;
  firstscan = rltld.yfnd;              /* begin of scan    */
  lastscan = rltld.yfnd;               /* end of scan      */

scndown:
  if(iabort()) return(0);              /* check CNTRL C */
  /* PRINTF2("SCAN DOWN line %3d to %3d\n",firstscan,lastscan); */
  rltld.ycur = firstscan;              /* test line is begin of scan */
  rltld.ychk = firstscan + 1;          /* check line is next line    */
  rltld.firstfound = -1;               /* reset lineindex            */
scnd1:
  if(iabort()) return(0);              /* check CNTRL C              */
  if(rltld.ychk >= getyy(dst)) goto scnd2; /* image border ?         */
  LINSCN(dst, overlap);                /* test rltld.ycur against rltld.ychk */
  rltld.ycur++;
  rltld.ychk++;
  if( rltld.ycur <= lastscan ) goto scnd1;/* end of loop ?           */
  if(rltld.foundone) goto scnd1;       /* something in last line ?   */

scnd2:
  if( rltld.firstfound < 0) goto scnend;/* nothing found in last scan ?*/
  firstscan = rltld.lastfound;
  lastscan = rltld.firstfound;

/*  scnup:  */
  if(iabort()) return(0);              /* check CNTRL C */
  /* PRINTF2("SCAN UP   line %3d to %3d\n",firstscan,lastscan); */
  rltld.ycur = firstscan;              /* test line is begin of scan */
  rltld.ychk = firstscan - 1;          /* check line is brevious line*/
  rltld.firstfound = -1;               /* reset lineindex            */
scnu1:
  if(iabort()) return(0);              /* check CNTRL C              */
  if(rltld.ychk < 0) goto scnu2;       /* image border ?             */
  LINSCN(dst, overlap);                /* test rltld.ycur against rltld.ychk */
  rltld.ycur--;
  rltld.ychk--;
  if( rltld.ycur >= lastscan ) goto scnu1; /* end of loop ?          */
  if(rltld.foundone) goto scnu1;       /* something in last line ?   */

scnu2:
  if( rltld.firstfound < 0) goto scnend;/* nothing found in last scan ?*/
  firstscan = rltld.lastfound;
  lastscan = rltld.firstfound;
  goto scndown;                        /* do next scan               */

scnend:
  if(rltld.npix > *bopix) {            /* label with greatest area */
    *bopix = rltld.npix;
    *bolab = rltld.label;
  }
  (rltld.label) += 1;
  if(rltld.label < labmax) goto lablop;
labend:
  PRINTF3("label: %d labels, greatest area = %d for label %d\n", 
    rltld.label-labmin,*bopix,*bolab);

  return(rltld.label - labmin); /* Return number of labelled objects */

  return(ierr);

} /* int rl2t_label() */

int rl2t_invert( Timages *src, Timages *dst, int16 label, int16 xmax) /* invert runlength coded image
===================================== */
{
  register int x, y, free, xact, xx, xact_subpixel;
  register Trl2desc *ps, *pd;       /* pointer to src/dst    */

  if(uticheck(src,DV_HOST,TY_INT8)) return(-1);
  if(uticheck(dst,DV_HOST,TY_INT8)) return(-2);

  if(getyy(src) != getyy(dst)) {
    errstring = ERR_IPS_YSIZE_SRC_DST_DIFF;
    return(-3);
  }
  xx = getxx(src);

  for(y = 0; y < getyy(src); y++ ) {
    if(iabort()) return(0);                  /* check CNTRL C */
    ps = (Trl2desc *)pixad(0,y,src);
    pd = (Trl2desc *)pixad(0,y,dst);
    free = getxx(dst);                       /* free bytes in dst */
    xact = 0;
    xact_subpixel = 0;

    for(x = 0; x < xx; x += sizeof(Trl2desc)/sizeof(int16), ps++) {

      if (ps->rlab < 0) {                  /* end of row */

        if (xact < xmax) {
          /* store new rlcode */ 
          free -= sizeof(Trl2desc);          /* - size of descriptor */
          if(free < 0) {                    /* no memory free in line  ? */
            errstring = ERR_IPS_XS_DST_OVERFLOW;
            return(RL2TOOLS_MEMOVFL);
          }
          pd->rlxa = xact; 
          pd->rlxe = xmax - 1;  
          pd->rlab = 0;
          pd->rlxaSubPix = xact_subpixel;  /* in subpixel */
          pd->rlxeSubPix = (xmax << RL2T_SUBPIX_SHIFT) - 1; // is last subpixel
          pd++;
        }

        /* store EOL code */ 
        free -= sizeof(Trl2desc);          /* - size of descriptor */
        if(free < 0) {                    /* no memory free in line  ? */
          errstring = ERR_IPS_XS_DST_OVERFLOW;
          return(RL2TOOLS_MEMOVFL);
        }
        pd->rlxa = -1;  
        pd->rlxe = -1;  
        pd->rlab = -1;        
        pd->rlxaSubPix = -1;
        pd->rlxeSubPix = -1;
        break;                            /* take next row */

      } else if ( label <= 0 || ps->rlab == label) {  // invert all objects or invert this object

        if (ps->rlxa > xact) {
          /* store new rlcode */ 
          free -= sizeof(Trl2desc);          /* - size of descriptor */
          if(free < 0) {                    /* no memory free in line  ? */
            errstring = ERR_IPS_XS_DST_OVERFLOW;
            return(RL2TOOLS_MEMOVFL);
          }
          pd->rlxa = xact;  
          pd->rlxe = ps->rlxa - 1;  
          pd->rlab = 0;
          pd->rlxaSubPix = xact_subpixel;       /* in subpixel */
          pd->rlxeSubPix = ps->rlxaSubPix - 1;  /* in subpixel */
          pd++;
        }
        xact = ps->rlxe + 1;
        xact_subpixel = ps->rlxeSubPix - 1;

      } 
      /* ignore if (ps->rlab != label) */

    } /* end of for x */
  } /* end for y */

  return(0);

} /* int rl2t_invert() */

int rl2t_ExractLabelRLCs( Timages *src, Timages *dst, int16 label, int32 ymin, int32 ymax) /* Extract the RLC's for a specific object
===================================== */
{
  register int x, y, free, xx;
  register Trl2desc *ps, *pd;         /* pointer to src/dst    */
  Trl2desc EolRLC;                    /* code for end of line RLC */

  if(uticheck(src,DV_HOST,TY_INT8)) return(-1);
  if(uticheck(dst,DV_HOST,TY_INT8)) return(-2);

  if(getyy(src) != getyy(dst)) {
    errstring = ERR_IPS_YSIZE_SRC_DST_DIFF;
    return(-3);
  }
  xx = getxx(src);

  // prepare end of line RLC

  memset( &EolRLC, 0, sizeof( Trl2desc));

  EolRLC.rlxa = -1;  
  EolRLC.rlxe = -1;  
  EolRLC.rlab = -1;        
  EolRLC.rlxaSubPix = -1;
  EolRLC.rlxeSubPix = -1;

  // ...

  for(y = 0; y < getyy(src); y++ ) {

    if(iabort()) return(0);                  /* check CNTRL C */
    ps = (Trl2desc *)pixad(0,y,src);
    pd = (Trl2desc *)pixad(0,y,dst);

    if( y < ymin || y > ymax) {              // outside the RLC range of the object

      memcpy( pd, &EolRLC, sizeof( Trl2desc));  // set in a end of line code
      continue;                                 // try next line
    }

    free = getxx(dst);                       /* free bytes in dst */

    for(x = 0; x < xx; x += sizeof(Trl2desc)/sizeof(int16), ps++) {

      if (ps->rlab < 0) {                  /* end of row */

        /* store EOL code */ 
        free -= sizeof(Trl2desc);          /* - size of descriptor */
        if(free < 0) {                    /* no memory free in line  ? */
          errstring = ERR_IPS_XS_DST_OVERFLOW;
          return(RL2TOOLS_MEMOVFL);
        }
#ifdef use_again
        pd->rlxa = -1;  
        pd->rlxe = -1;  
        pd->rlab = -1;        
        pd->rlxaSubPix = -1;
        pd->rlxeSubPix = -1;
#else
        memcpy( pd, &EolRLC, sizeof( Trl2desc));
#endif
        break;                            /* take next row */

     } else if ( label <= 0 || ps->rlab == label) {  // extrat all objects or extract this object

        /* copy rlcode */ 
        free -= sizeof(Trl2desc);          /* - size of descriptor */
        if(free < 0) {                     /* no memory free in line  ? */
          errstring = ERR_IPS_XS_DST_OVERFLOW;
          return(RL2TOOLS_MEMOVFL);
        }
#ifdef use_again
        pd->rlxa = ps->rlxa;  
        pd->rlxe = ps->rlxe;  
        pd->rlab = ps->rlab;
        pd->rlid = ps->rlid;
        pd->rlxaSubPix = ps->rlxaSubPix;
        pd->rlxeSubPix = ps->rlxeSubPix;
#else
        memcpy( pd, ps, sizeof( Trl2desc));
#endif
        pd++;
      } 
      /* ignore if (ps->rlab != label) */

    } /* end of for x */
  } /* end for y */

  return(0);

} /* int rl2t_ExractLabelRLCs() */

int rl2t_meas( Timages *src, int16 label, int32 *area,
               int16 *xgrav, int16 *ygrav, int16 *xmin, int16 *xmax, int16 *ymin, int16 *ymax) /* Measure label
 ============================================================ */
{
  register int yoff, free;
#ifdef use_again
  int32 xgsum, ygsum, x;
#else
  // 22.11.2011 RR: use 64 bit integers to sum up.
  long long xgsum, ygsum;
#endif
  register Trl2desc *pRLC_read, *pRLC_last;      /* read pointer in dst   */

  if(uticheck(src,DV_HOST,TY_ANY)) return(-2);

  if (!(getyp(src) == TY_BYTE)) {
    errstring = ERR_IPS_DATA_COMP_NO_SUPP; return(-1);
  }

  *area = 0;
  *xmin = getxx(src) + 1;
  *xmax = 0;
  *ymin = getyy(src) + 1;
  *ymax = 0;
  xgsum = ygsum = 0;

  for(yoff = 0; yoff < getyy(src) ; yoff++) {
    if(iabort()) return(0);                       /* check CNTRL C */
    pRLC_read = (Trl2desc *)pixad(0,yoff,src);
    free = getxx(src);
    //x/x = 0;
    for(;;) {
      free -= sizeof(Trl2desc);
      if(free < 0) {                              /* no memory free in line */

        errstring = ERR_IPS_MEM_OVERFLOW;
        return( RL2TOOLS_MEMOVFL);
      }

      // ...

      pRLC_last = pRLC_read;   // point to current RLC
      pRLC_read++;             // advance read pointer (for loop breaks)

      // ...

      if(pRLC_last->rlxa < 0) break;                    /* End of List            */
      if(pRLC_last->rlab != label) continue;            /* Labelvalue ok ?        */
      *area += pRLC_last->rlxe - pRLC_last->rlxa + 1;         /* Count Area             */
      if(yoff < *ymin) *ymin = yoff;
      if(yoff > *ymax) *ymax = yoff;
      if(pRLC_last->rlxa < *xmin) *xmin = pRLC_last->rlxa;
      if(pRLC_last->rlxe > *xmax) *xmax = pRLC_last->rlxe;
      xgsum += ((long)(pRLC_last->rlxe - pRLC_last->rlxa + 1) *
        (long)(pRLC_last->rlxe + pRLC_last->rlxa)) / 2;
      ygsum += (long)(pRLC_last->rlxe - pRLC_last->rlxa + 1) * (long)yoff;

    }
  }

  *xgrav = (int16)((xgsum * 16 + *area / 2) / *area);
  *ygrav = (int16)((ygsum * 16 + *area / 2) / *area);

#ifdef use_again
  PRINTF1("** label %d: ",label);
  PRINTF1(" area %10ld",*area);
  PRINTF2(" x from %3d to %3d",*xmin,*xmax);
  PRINTF2(" y from %3d to %3d",*ymin,*ymax);
  PRINTF2(" grav: %d/%d\n",*xgrav,*ygrav);
#endif

  return(0);

} /* int rl2t_meas() */

int rl2t_meas_area( Timages *src, int16 label, int32 *area) /* Measure area
================================= */
{
  register int32 yoff, free, xx, yy;
  //x/ int32 x;
  register Trl2desc *pRLC_read, *pRLC_last;      /* read pointer in dst   */

  if(uticheck(src,DV_HOST,TY_ANY)) return(-2);

  if (!(getyp(src) == TY_BYTE)) {
    errstring = ERR_IPS_DATA_COMP_NO_SUPP; return(-1);
  }

  *area = 0;
  xx = getxx(src);
  yy = getyy(src);

  for(yoff = 0; yoff < yy; yoff++) {
    pRLC_read = (Trl2desc *)pixad(0, yoff, src);
    free = xx;
    //x/x = 0;
    for(;;) {
      free -= sizeof(Trl2desc);
      if(free < 0) {                              /* no memory free in line */

        errstring = ERR_IPS_MEM_OVERFLOW;
        return( RL2TOOLS_MEMOVFL);
      }

      // ...

      pRLC_last = pRLC_read;   // point to current RLC
      pRLC_read++;             // advance read pointer (for loop breaks)

      // ...

      if(pRLC_last->rlxa < 0) break;                    /* End of List            */
      if(pRLC_last->rlab != label) continue;            /* Labelvalue ok ?        */
      *area += pRLC_last->rlxe - pRLC_last->rlxa + 1;         /* Count Area             */
    }
  }

#ifdef use_again
  PRINTF1("** label %d: ",label);
  PRINTF1(" area %10ld",*area);
#endif

  return(0);

} /* int rl2t_meas_area() */

#define NO_ELEMENT		-1

#define RL2TLM_OBJECT_LIMIT	1

typedef struct {
  int32 first;
  int32 firstempty;
  int32 max_objects;
  int32 num_objects;
  int32 num_underlimit;
  Trl2obj *pwork; /* work pointer (Object/empty list) */
  int32 xmwork;
  int32 label;
} Trl2t_labmea_desc;

static Trl2t_labmea_desc rltlmd = {
  NO_ELEMENT,
  NO_ELEMENT,
  0,
  0,
  0,
  (Trl2obj *)0,
  0,
  0
};

static void init_work_list() /* init work list
                             ============================ */
{
  register int32 i;
  register Trl2obj *p;

  /* don't use element 0 (conflicts with RL2T_NOLABEL) */
  rltlmd.firstempty = rltlmd.xmwork;
  rltlmd.first = NO_ELEMENT;
  p = rltlmd.pwork + rltlmd.xmwork;

  /* init empty list */
  for (i = 1; i < rltlmd.max_objects; i++) {
    p->next = (i + 1) * rltlmd.xmwork;
    p += rltlmd.xmwork;
  }

  /* init last element of empty list */
  p->next = NO_ELEMENT;

  return;
}

/* init measurement */
static void meas_init( int32 act, int16 xa, int16 xe, int16 ypos, int16 label, int32 rlxaSubPix, int32 rlxeSubPix)
{
  register Trl2obj *pw;

  pw = rltlmd.pwork + act;

  pw->xmin = xa;
  pw->xmax = xe;
  pw->ymin = ypos;
  pw->ymax = ypos;
  pw->area_Pixel    = xe - xa + 1;

  if ((rl2flags & RL2AREA_SUPBPIX) == 0 || rlxaSubPix < 0 || rlxeSubPix < 0 ) { // no subpixel or no valid area
    
    // pixel area
    pw->area_SubPixel = (xe - xa + 1) << RL2T_SUBPIX_SHIFT;  // preset
  } else {

    // supbixel area
    pw->area_SubPixel = rlxeSubPix - rlxaSubPix + 1;
  }
  pw->xgrav = ((int32)(xe - xa + 1) * (int32)(xe + xa)) / 2;
  pw->ygrav = (int32)(xe - xa + 1) * (int32)ypos;
  pw->label = label;

  return;
}

/* update measurement */
static void meas_update( int32 act, int16 xa, int16 xe, int16 ypos, int32 rlxaSubPix, int32 rlxeSubPix)
{
  register Trl2obj *pw;

  pw = rltlmd.pwork + act;

  /* PRINTF1("meas_update %d\n", act); */
  if (pw->xmin > xa) pw->xmin = xa;
  if (pw->xmax < xe) pw->xmax = xe;
  pw->ymax = ypos;

  pw->area_Pixel += xe - xa + 1;

  if ((rl2flags & RL2AREA_SUPBPIX) == 0 || rlxaSubPix < 0 || rlxeSubPix < 0 ) { // no subpixel or no valid area

    // pixel area
    pw->area_SubPixel += (xe - xa + 1) << RL2T_SUBPIX_SHIFT;  // preset
  } else {

    // supbixel area
    pw->area_SubPixel += rlxeSubPix - rlxaSubPix + 1;
  }

  pw->xgrav += ((int32)(xe - xa + 1) * (int32)(xe + xa)) / 2;
  pw->ygrav += (int32)(xe - xa + 1) * (int32)ypos;

  return;
}

static void meas_reunion( int32 newly, int32 old, int flags, int32 yy) /* object reunion
============================================= */
{
  register Trl2obj *pw, *pwo;

  pw = rltlmd.pwork + newly;
  pwo = rltlmd.pwork + old;

  /* put old together with new -> new */ 

  if (pw->xmin > pwo->xmin) pw->xmin = pwo->xmin;
  if (pw->xmax < pwo->xmax) pw->xmax = pwo->xmax;
  if (flags & RL2VWRAPAROUND) {
    if (pw->ymin == 0) {   /* new = pure top row object */
      pw->ymin += yy;      
      pw->ymax += yy;      

      // use pixel area
      pw->ygrav = (pw->ygrav * 16 + pw->area_Pixel / 2) / pw->area_Pixel;  
      pw->ygrav += yy * 16;
      pw->ygrav = (pw->ygrav * pw->area_Pixel + 8) / 16;
    }
  }
  if (pw->ymin > pwo->ymin) pw->ymin = pwo->ymin;
  if (pw->ymax < pwo->ymax) pw->ymax = pwo->ymax;

  pw->area_Pixel += pwo->area_Pixel;
  pw->area_SubPixel += pwo->area_SubPixel;
  pw->xgrav += pwo->xgrav;
  pw->ygrav += pwo->ygrav;

  return;
}

static void rlc_reunion( int16 newlab, int16 newid, int16 oldlab, int32 y, Timages *rlc) /*
====================================================== */
{
  register Trl2desc *p;

  if (y < 0 || y >= getyy(rlc)) {
    /* PRINTF1("WARNING: rlc_reunion(): y = %d\n", y); */
    return;
  }
  p = (Trl2desc *)pixad(0, y, rlc);

  while(p->rlxa >= 0) {
    if (p->rlab == oldlab) {
      p->rlid = newid;
      p->rlab = newlab;
    }
    p++;
  }

  return;
}

static void rlc_reunionAll( int16 newlab, int16 newid, int32 old, int16 oldlab, int32 ymax, Timages *rlc) /*
================================================================= */
{
  register Trl2obj *pwo;
  register Trl2desc *p;
  register int32 y;

  if (ymax < 0 || ymax >= getyy(rlc)) {
    /* PRINTF1("WARNING: rlc_reunion(): y = %d\n", y); */
    return;
  }

  if (old < 0) {
    y = 0; /* start with image begin */
  } else {
    pwo = rltlmd.pwork + old;
    y = pwo->ymin;
  }

  /* reunion starting from old->ymin */
  for (; y <= ymax; y++) {
    p = (Trl2desc *)pixad(0, y, rlc);

    while(p->rlxa >= 0) {
      if (p->rlab == oldlab) {
        p->rlid = newid;
        p->rlab = newlab;
      }
      p++;
    }
  }

  return;
}

static int32 object_w_append() /* append new element out 
                               ===============================   of empty list
                               the object is always appended at the begin
                               if empty list is empty, clean up with worked
                               off objects */
{
  int32 act;
  register Trl2obj *pw;

  if (rltlmd.firstempty == NO_ELEMENT) goto error; /* empty free list */

  act = (int32)rltlmd.firstempty;
  pw = rltlmd.pwork + act;

  rltlmd.firstempty = pw->next;
  pw->next = rltlmd.first;
  pw->prev = NO_ELEMENT;
  if (rltlmd.first != NO_ELEMENT) (rltlmd.pwork + rltlmd.first)->prev = act;
  rltlmd.first = act;

  /* PRINTF1("object_w_append(): new %d\n", act); */
  return(act);

error:
errstring = ERR_IPS_OBJ_BUF_OVERFLOW;
  return(-1);
}

static void object_w_delete( int32 act) /* delete object out of work list
================================ */
{
  int32 next, prev;

  next = (rltlmd.pwork + act)->next;
  prev = (rltlmd.pwork + act)->prev;

  if (prev == NO_ELEMENT) rltlmd.first = next;
  else (rltlmd.pwork + prev)->next = next; 
  if (next != NO_ELEMENT) (rltlmd.pwork + next)->prev = prev; 

  (rltlmd.pwork + act)->next = rltlmd.firstempty;
  rltlmd.firstempty = act;

  return;
}

static int object_store( int32 act, Tvector *obj, int32 minarea, int32 maxarea, int flags) /* store object in destination list
=================================================    and free work object */
{
  int ierr;
  register Trl2objdst *pobj;
  register Trl2obj *pw;

  pw = rltlmd.pwork + act;

  /* PRINTF2("FSV: object_store, area %d (min %d)\n", pw->area, minarea); */
  /* check if area is big enough */
  if (pw->area_Pixel < minarea) {      // Below minimum area
    if (pw->area_Pixel <= 0) goto end; /* shouldn't happen */
    (rltlmd.num_underlimit)++;
    goto end;
  }

  if ( maxarea > minarea &&            // Maximum area is over minimum area
       pw->area_Pixel > maxarea) {     // and object area is over maximum area
    if (pw->area_Pixel <= 0) goto end; /* shouldn't happen */
    (rltlmd.num_underlimit)++;
    goto end;
  }

  if (flags & RL2DONTSTORE) {
    (rltlmd.num_objects)++;
    goto end;
  }

  /* check if obj has enough space */
  if ((flags & RL2NO_OBJ_REALLOC) == 0) {   // is realloction allowed

    ierr = ve_realloc(obj, (rltlmd.num_objects + 1) * (sizeof(Trl2objdst) / sizeof(int32)), sizeof(int32), TY_INT32);
    if (ierr != 0) return(ierr);
  }

  if ((int32)vgetln(obj) < (int32)((rltlmd.num_objects + 1) *
    (sizeof(Trl2objdst) / sizeof(int32)))) {
      PRINTF0("FSV WARNING: object limit reached\n");
      ierr = RL2TLM_OBJECT_LIMIT;
      goto error;
  }
  pobj = (Trl2objdst *)vgetpm(obj);
  pobj += rltlmd.num_objects;
  /* PRINTF5(" object store %d, 0x%08lx (%d >= %d), act %d\n",
  rltlmd.num_objects, pobj,
  (int32)vgetln(obj), 
  (rltlmd.num_objects + 1) * (sizeof(Trl2objdst) / sizeof(int32)), act);*/

  pobj->xmin = pw->xmin;  
  pobj->xmax = pw->xmax;  
  pobj->ymin = pw->ymin;  
  pobj->ymax = pw->ymax;  

  // use pixel area
  pobj->xgrav = (int32)((pw->xgrav * 16 + pw->area_Pixel / 2) / pw->area_Pixel);  
  pobj->ygrav = (int32)((pw->ygrav * 16 + pw->area_Pixel / 2) / pw->area_Pixel);  

  if ((rl2flags & RL2AREA_SUPBPIX) == 0) {
    pobj->area = pw->area_Pixel; 
  } else {
    pobj->area = pw->area_SubPixel; 
  }
  pobj->label = pw->label; 
  /* PRINTF5("  %d %d %d %d %d\n", pobj->xmin, pobj->xmax, 
  pobj->ymin, pobj->ymax, pobj->area); */

  // 16.05.2025 RR: Zero extra data.
  pobj->ExData1 = 0;
  pobj->ExData2 = 0;

  (rltlmd.num_objects)++;
  vputnm(obj, rltlmd.num_objects * (sizeof(Trl2objdst) / sizeof(int32)));

end:
  ierr = 0;
error:
  object_w_delete(act);
  /* PRINTF1("FSV: object_store, total # obj: %d\n", rltlmd.num_objects); */

  return(ierr);
}
static int object_store_all( Tvector *obj, int32 minarea, int32 maxarea, int flags) /* store all work objects in
====================================================================    dst list */
{
  int ierr;
  int32 act, next;

  act = rltlmd.first;

  while(act != NO_ELEMENT) {
    next = (rltlmd.pwork + act)->next;
    ierr = object_store(act, obj, minarea, maxarea, flags);
    if (ierr != 0) {
      return(ierr);
    }
    act = next;
  }

  return(0);
}

static int object_cleanup( Tvector *obj, int32 minarea, int32 maxarea, int16 y, int flags) /* store all worked off
==========================================================================  */
{
  int ierr;
  int32 act, next;

  act = rltlmd.first;

  while(act != NO_ELEMENT) {
    next = (rltlmd.pwork + act)->next;
    /* PRINTF2("FSV: object_cleanup checking %d, next %d\n", act, next); */
    if (y > (rltlmd.pwork + act)->ymax) { /* if wasn't updated in this row */
      if (flags & RL2VWRAPAROUND) {
        if ((rltlmd.pwork + act)->ymin > 0) {
          /* leave the top row touching objects in the work list */
          ierr = object_store(act, obj, minarea, maxarea, flags);
          if (ierr != 0) {
            return(ierr);    
          }
        }
      } else {
        ierr = object_store(act, obj, minarea, maxarea, flags);
        if (ierr != 0) {
          return(ierr);    
        }
      }
    }
    act = next;
  }

  return(0);
}

static int get_first_line( Timages *rlc) /* get first line into work list
===============================*/
{
  int32 act;
  register Trl2desc *p;

  p = (Trl2desc *)pixad(0, 0, rlc);

  /* enter line in work list */
  while(p->rlxa >= 0) {
    act = object_w_append();
    if (act < 0) return(act);
    (rltlmd.label)++;
    p->rlab = rltlmd.label;        /* label */
    p->rlid = act / rltlmd.xmwork; /* list index */
    meas_init(act, p->rlxa, p->rlxe, 0, p->rlab, p->rlxaSubPix, p->rlxeSubPix);
    p++;
  }

  return(0);

}

static int object_lab( Timages *rlc, Tvector *obj, int32 minarea, int32 maxarea, int overlap, int flags) /* label rlc image
========================================================= */
{
  int ierr;
  register int32 y, yy;
  int16 act;
  Trl2desc *p, *pl;

  yy = getyy(rlc);

  for (y = 1; y < yy; y++) {

    pl = (Trl2desc *)pixad(0, y - 1, rlc); 
    p  = (Trl2desc *)pixad(0, y, rlc); 

    /* now we try to label actual row with last row */
    while(1) {

      if (p->rlxa < 0) {          /* actual line at end */

        ierr = object_cleanup(obj, minarea, maxarea, y, flags);
        if (ierr != 0) {
          return(ierr);
        }
        break;                               /* this row is done */

      } else if ((pl->rlxa < 0) ||  /* last line at end */
        (p->rlxe + overlap < pl->rlxa)) { /* actual before last line */

          if (p->rlab == RL2T_NOLABEL) { /* open new working object */
            act = object_w_append();
            if (act < 0) return(act);
            (rltlmd.label)++;
            p->rlab = rltlmd.label;        /* label */
            p->rlid = act / rltlmd.xmwork; /* list index */
            meas_init(act, p->rlxa, p->rlxe, (int16)y, p->rlab, p->rlxaSubPix, p->rlxeSubPix);
          }
          p++;                      /* update actual pointer */

      } else if (pl->rlxe + overlap < p->rlxa) { /* last line before actual */

        pl++;                     /* update last line pointer */

      } else {                    /* last and actual touch */

        /* the actual and last field touch, so label the actual field */
        if (p->rlab == RL2T_NOLABEL) {

          p->rlid = pl->rlid;
          p->rlab = pl->rlab;
          meas_update((int32)p->rlid * rltlmd.xmwork, p->rlxa, p->rlxe, 
            (int16)y, p->rlxaSubPix, p->rlxeSubPix);

        } else if (p->rlab != pl->rlab) {

          /* we have to do a label reunion */
          /* PRINTF2("FSV reunion, new %d old %d\n", 
          (int32)p->rlid * rltlmd.xmwork, 
          (int32)pl->rlid * rltlmd.xmwork); */
          meas_reunion((int32)p->rlid * rltlmd.xmwork,           /* new */ 
            (int32)pl->rlid * rltlmd.xmwork, 0, yy);  /* old */
          object_w_delete((int32)pl->rlid * rltlmd.xmwork);
          if (flags & RL2ENTERLABELS) {
            rlc_reunionAll(p->rlab, p->rlid, 
              (int32)pl->rlid * rltlmd.xmwork, pl->rlab, y, rlc);
          } else {
            rlc_reunion(p->rlab, p->rlid, pl->rlab, y, rlc);
            if (flags & RL2VWRAPAROUND) {
              /* for wrap around update the labels in the top row too */
              rlc_reunion(p->rlab, p->rlid, pl->rlab, 0, rlc);
            }
            /* do the reunion in the last line at last, because pl->rlab is
            overwritten */
            rlc_reunion(p->rlab, p->rlid, pl->rlab, y-1, rlc);
          }

        } /* else { p->rlab == pl->rlab,circular!, they are just unified */

        /* update pointer of leftside field */
        if (p->rlxe > pl->rlxe) pl++;
        else                    p++;

      }
    }
  }

  /* for wrap around try to do reunions between top and bottom row */
  if (flags & RL2VWRAPAROUND) {

    pl = (Trl2desc *)pixad(0, yy - 1, rlc); 
    p  = (Trl2desc *)pixad(0, 0, rlc); 

    /* now we try to label last top with bottom row */
    while(1) {

      if ((p->rlxa < 0) || (pl->rlxa < 0)) {     /* one line at end */

        break;                                   /* done */

      } else if ((p->rlxe + overlap < pl->rlxa)) { /* actual before last line */

        p++;                      /* update actual pointer */

      } else if (pl->rlxe + overlap < p->rlxa) { /* last line before actual */

        pl++;                     /* update last line pointer */

      } else {                    /* last and actual touch */

        /* the actual and last field touch, so label the actual field */
        if (p->rlab != pl->rlab) {

          /* we have to do a label reunion */
          meas_reunion((int32)p->rlid * rltlmd.xmwork,               /* new */ 
            (int32)pl->rlid * rltlmd.xmwork, flags, yy);  /* old */
          object_w_delete((int32)pl->rlid * rltlmd.xmwork);
          if (flags & RL2ENTERLABELS) {
#ifdef use_again
            rlc_reunionAll(p->rlab, p->rlid, (int32)-1, pl->rlid, yy - 1, rlc);
#else
            rlc_reunionAll(p->rlab, p->rlid, (int32)-1, pl->rlab, yy - 1, rlc);
#endif
          } else {
            rlc_reunion(p->rlab, p->rlid, pl->rlab, 0, rlc);
            /* do the reunion in the last line at last, because pl->rlab is
            overwritten */
            rlc_reunion(p->rlab, p->rlid, pl->rlab, yy - 1, rlc);
          }

        } /* else { p->rlab == pl->rlab, circular!, they are just unified */

        /* update pointer of leftside field */
        if (p->rlxe > pl->rlxe) pl++;
        else                    p++;

      }
    }
  }


  return(0);

}

int rl2t_labmea( Timages *src_rlc, Tvector *dst_obj, Timages *work_obj, int32 minarea, int32 maxarea) /* label & meas
=================================================== */
{
  int ierr;
  int overlap, flags;

  rl2t_rdoverlap(&overlap);
  rl2t_rdflags(&flags);

  if(uticheck(src_rlc, DV_HOST,TY_INT8)) {
    errstring = ERR_IPS_PARAM_ILLEGAL; // (char *)"bad rlc image";
    return(-1);
  }
  if ((flags & RL2DONTSTORE) == 0) {
    if(utvcheck(dst_obj,DV_HOST,TY_INT32)) {
      errstring = ERR_IPS_PARAM_ILLEGAL; // (char *)"bad obj vector";
      return(-2);
    }
  }
  if(uticheck(work_obj, DV_HOST,TY_INT8)) {
    errstring = ERR_IPS_PARAM_ILLEGAL; // (char *)"bad work image";
    return(-3);
  }
  if (getxx(work_obj) < sizeof(Trl2obj)) {
    errstring = ERR_IPS_PARAM_BAD; // (char *)"work image too small";
    return(-4);
  }

  rltlmd.max_objects = getyy(work_obj) - 1; /* element 0 is unused */
  rltlmd.pwork = (Trl2obj *)pixad(0, 0, work_obj);
  rltlmd.xmwork = getxm(work_obj);

  if (rltlmd.xmwork % sizeof(Trl2obj)) {
    errstring = ERR_IPS_PARAM_BAD; // (char *)"bad aligned memory (not multiple of 32)";
    return(-5);
  }
  rltlmd.xmwork /= sizeof(Trl2obj); /* for update (Trl2obj *)pwork pointer */

  if ((flags & RL2DONTSTORE) == 0) {

    if ((flags & RL2NO_OBJ_REALLOC) == 0) {   // is realloction allowed

      ierr = ve_realloc(dst_obj, sizeof(Trl2objdst)/sizeof(int32), sizeof(int32), TY_INT32);
      if (ierr != 0) {
        return(ierr);
      }
    }

    /* at least space for one object */
    if ((long long unsigned int)vgetln(dst_obj) < (sizeof(Trl2objdst) / sizeof(int32))) {

      if ((flags & RL2NO_OBJ_REALLOC) == 0) {   // is realloction allowed

        // return what was returned before 'RL2NO_OBJ_REALLOC'
        return(RL2TLM_OBJECT_LIMIT);

      } else {

        // We test for one object, if there is none, there was
        // no pre-allocation of the vector.
        // --> return an error

        return( -10);    // return error, not enough preallocated objects
      }
    }
    vputnm(dst_obj, 0L);  /* empty dst list */
  }
  rltlmd.num_objects = 0;
  rltlmd.num_underlimit = 0;
  rltlmd.label = 0;

  /* make hole work list to a free list */
  init_work_list();
  /* PRINTF0("FSV: rl2t_labmea: init_work_list() done\n"); */

  /* take first RLC line, initialize work objects */
  ierr = get_first_line(src_rlc);
  if (ierr != 0) goto error;
  /* PRINTF0("FSV: rl2t_labmea: get_first_line() done\n"); */

  /* now analyze rest of RLC image */
  ierr = object_lab(src_rlc, dst_obj, minarea, maxarea, overlap, flags);
  if (ierr != 0) {
    if (ierr != RL2TLM_OBJECT_LIMIT) goto error;
  }
  /* PRINTF1("FSV: rl2t_labmea: object_lab() done with %d\n", ierr); */

/*  end:  */

  /* store all work objects in destination object list */
  ierr = object_store_all(dst_obj, minarea, maxarea, flags);
  if ( ierr != 0) {
    if (ierr != RL2TLM_OBJECT_LIMIT) goto error;
  }
  /* PRINTF1("FSV: rl2t_labmea: object_store_all() done with %d\n", ierr); */

  ierr = rltlmd.num_objects;

error:
  /* im_remove(work_obj); */
  PRINTF1("under limit: %d\n", rltlmd.num_underlimit);
  return(ierr);
}

static int rl2t_eros_overlap( Trl2desc *act, Trl2desc *next, Trl2desc **psln, int16 kmode) /*
==================================================== */
{
  while (1) {
    if ((*psln)->rlab < 0) {

      return(0);                                 /* line end */

    } else if ((*psln)->rlxe < act->rlxa) {

      (*psln)++; /* take next rlc */
      continue;   /* check next one */

    } else if ((*psln)->rlxa > act->rlxe) {

      return(0);                                 /* passed */

    } else {

      /* overlap */
      if ((*psln)->rlxa <= act->rlxa) {
        (act->rlxa)++;
      } else {
        act->rlxa = (*psln)->rlxa + kmode;
      }
      if ((*psln)->rlxe >= act->rlxe) {
        (act->rlxe)--;
      } else {
        next->rlxe = act->rlxe;  /* store the real end */
        act->rlxe = (*psln)->rlxe - kmode;
        /* next->rlxa = (*psln)->rlxe - kmode + 2; */
        next->rlxa = (*psln)->rlxe + 1;
      }
      return(1);
    }
  }

} /* static int rl2t_eros_overlap() */

int rl2t_eros2( Timages *src, Timages *dst, // erode runlength coded image
                int16 kmode,   // 0: +, 1: block
                int16 xmax)    // if > 0, keep frame
{
  register int   x, y, free, xx;
  register Trl2desc *ps, *pd; 
  Trl2desc *psl0, *psn0, act1, next1, act2, next2; 

  if(uticheck(src,DV_HOST,TY_INT8)) return(-1);
  if(uticheck(dst,DV_HOST,TY_INT8)) return(-2);

  if(getyy(src) != getyy(dst)) {
    errstring = ERR_IPS_YSIZE_SRC_DST_DIFF;
    return(-3);
  }
  xx = getxx(src);

  if( xmax > 0) {    // keep frame

    /* fill first row */
    pd = (Trl2desc *)pixad(0, 0, dst);
    pd->rlxa = 0;
    pd->rlxe = xmax - 1;  
    pd->rlab = 0;

    /* end of line */
    pd++;
    pd->rlxa = -1;
    pd->rlxe = -1;
    pd->rlab = -1;
  } else {

    /* delete first row */
    pd = (Trl2desc *)pixad(0, 0, dst);
    pd->rlxa = -1;
    pd->rlxe = -1;
    pd->rlab = -1;
  }

  for(y = 1; y < getyy(src) - 1; y++ ) {
    if(iabort()) return(0);                  /* check CNTRL C */
    psl0 = (Trl2desc *)pixad(0, y-1, src);
    ps = (Trl2desc *)pixad(0, y, src);
    psn0 = (Trl2desc *)pixad(0, y+1, src);
    pd = (Trl2desc *)pixad(0, y, dst);
    free = getxx(dst);                       /* free bytes in dst */
    for(x = 0; x < xx; x += sizeof(Trl2desc)/sizeof(int16), ps++) {

      if (ps->rlab < 0) {                  /* end of row */
        break;
      } else if ((ps->rlxe - ps->rlxa) < 2) { 
        continue;
      } else {

        act1.rlxa = ps->rlxa;
        act1.rlxe = ps->rlxe;
        do {
          next1.rlxa = 0;
          next1.rlxe = 0;
          /* --------------- check last line for overlap ---------------- */
          if (rl2t_eros_overlap(&act1, &next1, &psl0, kmode)) {
            act2.rlxa = act1.rlxa - 1;
            act2.rlxe = act1.rlxe + 1;
            next2.rlxa = 0;
            next2.rlxe = 0;
            /* --------------- check next line for overlap ---------------- */
            if (rl2t_eros_overlap(&act2, &next2, &psn0, kmode)) {
              /* take maximum of actx.rlxa, minimum of actx.rlxe */
              if (act2.rlxa > act1.rlxa) act1.rlxa = act2.rlxa;
              if (act2.rlxe < act1.rlxe) act1.rlxe = act2.rlxe;
              if (act1.rlxa <= act1.rlxe) {
                /* store new rlcode */ 
                free -= sizeof(Trl2desc);          /* - size of descriptor */
                if(free < 0) {               /* no memory free in line  ? */
                  errstring = ERR_IPS_XS_DST_OVERFLOW;
                  return(RL2TOOLS_MEMOVFL);
                }
                pd->rlxa = act1.rlxa;  
                pd->rlxe = act1.rlxe;  
                pd->rlab = 0;

                if( xmax > 0) {    // keep frame
                  if( pd->rlxa == 1) {

                    pd->rlxa = 0;
                  }
                  if( pd->rlxe == xmax - 2) {

                    pd->rlxe = xmax - 1;
                  }
                }
                pd++;
              }
            }
            if (next2.rlxa) {
              if (next1.rlxa) {
                if (next2.rlxa < next1.rlxa) next1.rlxa = next2.rlxa;
              } else {
                next1.rlxa = next2.rlxa;
                next1.rlxe = next2.rlxe;
              }
            }
            if (next1.rlxa) {
              act1.rlxa = next1.rlxa;
              act1.rlxe = next1.rlxe;
              act2.rlxa = next1.rlxa;
              act2.rlxe = next1.rlxe;
            }
          }
        } while (next1.rlxa); 
      } 

    } /* end of for x */
    pd->rlxa = -1;  
    pd->rlxe = -1;  
    pd->rlab = -1;
  } /* end for y */

  if( xmax > 0) {    // keep frame

    /* fill first row */
    pd = (Trl2desc *)pixad(0, getyy(dst) - 1, dst);
    pd->rlxa = 0;
    pd->rlxe = xmax - 1;  
    pd->rlab = 0;

    /* end of line */
    pd++;
    pd->rlxa = -1;
    pd->rlxe = -1;
    pd->rlab = -1;
  } else {

    /* delete last row */
    pd = (Trl2desc *)pixad(0, getyy(dst) - 1, dst);
    pd->rlxa = -1;
    pd->rlxe = -1;
    pd->rlab = -1;
  }

  return(0);

} /* int rl2t_eros2() */

int rl2t_eros( Timages *src, Timages *dst, // erode runlength coded image
               int16 kmode)                // 0: +, 1: block
{
  int ierr; 

  ierr = rl2t_eros2(src, dst, kmode, 0);

  return( ierr);

} /* int rl2t_eros() */

/*-----------------------------------------------------
 * rl2t_sumRLCs
 *
 * Sum up RLCs in a given AOI.
 * Does the same as a rowsom or colsum call in a
 * binary image, but much faster.
 *-----------------------------------------------------
*/
int rl2t_sumRLCs( Timages *srcim,                 // src image (hold run length codes)
                  Tvector *dstvec,                // destination vector
                  int label,                      // 0 from all label, else from specific label
                  int xStart, int yStart,         // start of AOI
                  int xSize, int ySize,           // size of AOI
                  int XY_Mode,                    // XY_Mode 0 = rowsum (horizontal), else colsum (vertical)
                  int InvertFlag,                 // if != 0, invert the sum (is like negating the RLC's)
                  int xmax)                       // x-size of uncoded original
{
  register int x, i;
  int yoff, nitem, xEnd, vStart, vEnd;
  //x/int yEnd;
  register Trl2desc *pRLC_read;      /* read pointer in dst   */
  int32 *vp32;

  if(uticheck( srcim, DV_HOST, TY_ANY)) return(-1);
  if(utvcheck( dstvec,DV_HOST,TY_INT32)) return(-2);

  if( getyp( srcim) != TY_BYTE) {                // src image must be of this type
    errstring = ERR_IPS_DATA_COMP_NO_SUPP;
    return(-3);
  }

  // replace 0 sizes by size form AOI start to image end

  if( xSize <= 0) {                  // Size is 0

    xSize = xmax - xStart;           // get size until rest of image (uncoded original)
    if( xSize < 1) {                 // security clipping

      xSize = 1;
    }
  }

  if( ySize <= 0) {                  // Size is 0

    ySize = getyy( srcim) - yStart;  // get size until rest of image (uncoded original)
    if( ySize < 1) {                 // security clipping

      ySize = 1;
    }
  }

  // securtiy test coordinates

  if( xStart < 0 || yStart < 0 ||
      xStart + xSize > xmax ||
      yStart + ySize > getyy( srcim)) {

    errstring = ERR_IPS_PARAM_ILLEGAL;
    return(-4);
  }

  // ...

  xEnd = xStart + xSize - 1;        // last coordinate in AOI
  //x/yEnd = yStart + ySize - 1;

  // Allocate destination vector and clear it

  if( XY_Mode == 0) {               // mode rowsum
    nitem = xSize;
  } else {                          // mode colsum
    nitem = ySize;
  }

  if( ve_alloc( dstvec, nitem, (int16)sizeof(int32), TY_INT32)) {

    return(-5);
  }

  vp32 = (int32 *)vgetpm( dstvec);   // begin of vector

  memset( vp32, 0, sizeof( int32) * nitem); // clear

  // Walk the RLC's

  for( yoff = 0; yoff < ySize; yoff++) {    // All lines in the AOI

    pRLC_read = (Trl2desc *)pixad( 0, yStart + yoff, srcim);  // Relativ to begin of yStart

    for( x = 0; x < getxx( srcim); x += sizeof(Trl2desc), pRLC_read++) {   // Over all RLC's in image

      if( pRLC_read->rlab < 0) {            // End of row

        break;
      }

      if( label > 0 && pRLC_read->rlab != label) {   // Want a specific labe and this is not the label we want

        continue;
      }

      if( pRLC_read->rlxa <= xEnd &&          // RLC is inside AOI
          pRLC_read->rlxe >= xStart) {

        vStart = pRLC_read->rlxa - xStart;    // Vector start position
        if( vStart < 0) vStart = 0;
        if( vStart >= xSize) vStart = xSize - 1;

        vEnd = pRLC_read->rlxe - xStart;      // Vector end position
        if( vEnd < 0) vStart = 0;
        if( vEnd >= xSize) vEnd = xSize - 1;

        if( XY_Mode == 0) {                   // mode rowsum

          for( i = vStart; i <= vEnd; i++) {  // Over length of RLC overlapping with AOI

            vp32[ i] += 1;                    // One more for the vector
          }

        } else {                              // mode colsum

          vp32[ yoff] += vEnd - vStart + 1;   // Sum up overlapping part
        }
      }
    }
  }

  if( InvertFlag) {                           // Want inverted sum's

    if( XY_Mode == 0) {                       // mode rowsum

      for( i = 0; i < nitem; i++) {           // Over length of vector

         vp32[ i] = ySize - vp32[ i];         // Invert in Y
      }
    } else {                                  // mode colsum

      for( i = 0; i < nitem; i++) {           // Over length of vector

         vp32[ i] = xSize - vp32[ i];         // Invert in X
      }
    }
  }

  vputnm( dstvec, nitem);                     // store number of items

  return(0);

} /* int rl2t_sumRLCs() */

/*-----------------------------------------------------
 * rl2t_CountPixelsRLCs
 *
 * Sum up lenght of RLCs inside a given AOI.
 * The result is the equivalent to the sum of pixels
 * RLC's overlapping the given AOI.
 *-----------------------------------------------------
*/
int rl2t_CountPixelsRLCs(
                  Timages *srcim,                 // src image (hold run length codes)
                  int *pCountPixels,              // OUTPUT: Sum of Pixel equivalent RLC'S in AOI
                  int label,                      // 0 from all label, else from specific label
                  int xStart, int yStart,         // start of AOI
                  int xSize, int ySize,           // size of AOI
                  int xmax)                       // x-size of uncoded original
{
  register int x;
  int yoff, xEnd, vStart, vEnd;
  //x/int yEnd;
  register Trl2desc *pRLC_read;      /* read pointer in dst   */

  *pCountPixels = 0;             // Reset result

  if(uticheck( srcim, DV_HOST, TY_ANY)) return(-1);

  if( getyp( srcim) != TY_BYTE) {                // src image must be of this type
    errstring = ERR_IPS_DATA_COMP_NO_SUPP;
    return(-3);
  }

  // replace 0 sizes by size form AOI start to image end

  if( xSize <= 0) {                  // Size is 0

    xSize = xmax - xStart;           // get size until rest of image (uncoded original)
    if( xSize < 1) {                 // security clipping

      xSize = 1;
    }
  }

  if( ySize <= 0) {                  // Size is 0

    ySize = getyy( srcim) - yStart;  // get size until rest of image (uncoded original)
    if( ySize < 1) {                 // security clipping

      ySize = 1;
    }
  }

  // security test coordinates

  if( xStart < 0 || yStart < 0 ||
      xStart + xSize > xmax ||
      yStart + ySize > getyy( srcim)) {

    errstring = ERR_IPS_PARAM_ILLEGAL;
    return(-3);
  }

  // ...

  xEnd = xStart + xSize - 1;        // last coordinate in AOI
  //x/yEnd = yStart + ySize - 1;

  // Walk the RLC's

  for( yoff = 0; yoff < ySize; yoff++) {    // All lines in the AOI

    pRLC_read = (Trl2desc *)pixad( 0, yStart + yoff, srcim);  // Relativ to begin of yStart

    for( x = 0; x < getxx( srcim); x += sizeof(Trl2desc), pRLC_read++) {   // Over all RLC's in image

      if( pRLC_read->rlab < 0) {            // End of row

        break;
      }

      if( label > 0 && pRLC_read->rlab != label) {   // Want a specific labe and this is not the label we want

        continue;
      }

      if( pRLC_read->rlxa <= xEnd &&          // RLC is inside AOI
          pRLC_read->rlxe >= xStart) {

        vStart = pRLC_read->rlxa - xStart;    // Start position, relative to AOI
        if( vStart < 0) vStart = 0;
        if( vStart >= xSize) vStart = xSize - 1;

        vEnd = pRLC_read->rlxe - xStart;      // End position, relative to AOI
        if( vEnd < 0) vStart = 0;
        if( vEnd >= xSize) vEnd = xSize - 1;

        *pCountPixels += vEnd - vStart + 1;   // Sum up
      }
    }
  }

  return(0);

} /* int rl2t_CountPixelsRLCs() */

/****************************************************************************
* rl2t_Erode
*
* Erodes RLCs.
*
* piRLC_src:   Pointer to RLC image pointer
*              On entry, this are the source RLC's
*              On exit, the eroded RLC's are placed here.
* piRLC_dst:   Pointer to RLC image pointer
*              Is used to play ping/pong while eroding RLC's
*              images.
* nErode       number of erode runs, must be >= 0
*
* return:   0  OK
*         < 0  error
*
*****************************************************************************
*/

int rl2t_Erode( Timages **piRLC_src,  // Pointer to RLC image pointer. Src in, result out,
                Timages **piRLC_dst,  // Pointer to RLC image pointer. Use for temp processing.
                int nErode)           // # of erosion steps
{
  int ierr, i;
  Timages *iRLC_tmp;

  for( i = 0; i < nErode; i++) {

    // erode

    ierr = rl2t_eros2( *piRLC_src, *piRLC_dst, 1, 0);  // use kernel mode for 8 neighbours, no frame at image border

    if( ierr < 0) {     // Error ?
      return( ierr);     // Return the error to the caller
    }

    // exchange source and destination
    iRLC_tmp = *piRLC_src; *piRLC_src = *piRLC_dst; *piRLC_dst = iRLC_tmp;
  }

  return( 0);    // return OK
}

/****************************************************************************
* rl2t_Erode2
*
* Erodes RLCs, add border.
*
* piRLC_src:   Pointer to RLC image pointer
*              On entry, this are the source RLC's
*              On exit, the eroded RLC's are placed here.
* piRLC_dst:   Pointer to RLC image pointer
*              Is used to play ping/pong while eroding RLC's
*              images.
* nErode       number of erode runs, must be >= 0
* xmax:        no frame or black border: 0
*              frame or white border: x-size of uncoded original image
*
* return:   0  OK
*         < 0  error
*
*****************************************************************************
*/

int rl2t_Erode2( Timages **piRLC_src,  // Pointer to RLC image pointer. Src in, result out,
                 Timages **piRLC_dst,  // Pointer to RLC image pointer. Use for temp processing.
                 int nErode,           // # of erosion steps
                 int xmax)             // 0 or x-size of uncoded original image
{
  int ierr, i;
  Timages *iRLC_tmp;

  for( i = 0; i < nErode; i++) {

    // erode

    ierr = rl2t_eros2( *piRLC_src, *piRLC_dst, 1, xmax);  // use kernel mode for 8 neighbours, with frame at image border

    if( ierr < 0) {     // Error ?
      return( ierr);     // Return the error to the caller
    }

    // exchange source and destination
    iRLC_tmp = *piRLC_src; *piRLC_src = *piRLC_dst; *piRLC_dst = iRLC_tmp;
  }

  return( 0);    // return OK
}

/****************************************************************************
* rl2t_Dilate
*
* Dilate RLCs.
*
* piRLC_src:   Pointer to RLC image pointer
*              On entry, this are the source RLC's
*              On exit, the dilated RLC's are placed here.
* piRLC_dst:   Pointer to RLC image pointer
*              Is used to play ping/pong while dilating RLC's
*              images.
* nDilate      number of dilate runs, must be >= 0
* label:       On entry, a specifix label can be extracted from the
*              RLC's placed in *piRLC_src.
*              If label >  0 only the named object with "label" is used.
*              If label <= 0 all labels are used.
* xmax:        x-size of uncoded original image
*
* return:   0  OK
*         < 0  error
*
*****************************************************************************
*/

int rl2t_Dilate( Timages **piRLC_src,  // Pointer to RLC image pointer. Src in, result out,
                 Timages **piRLC_dst,  // Pointer to RLC image pointer. Use for temp processing.
                 int nDilate,          // # of dilate steps
                 int label,            // > 0: use specific object, <= 0: use all objects
                 int xmax)             // x-size of uncoded original image
{
  int ierr, i;
  Timages *iRLC_tmp;

  if( nDilate > 0) {                   // Have something to do

    // The dilation is done as: inverting, erosion, inverting

    ierr = rl2t_invert( *piRLC_src, *piRLC_dst, label, xmax);

    if( ierr < 0) {     // Error ?
      return( ierr);     // Return the error to the caller
    }

    // exchange source and destination
    iRLC_tmp = *piRLC_src; *piRLC_src = *piRLC_dst; *piRLC_dst = iRLC_tmp;

    // erode

    for( i = 0; i < nDilate; i++) {

      ierr = rl2t_eros2( *piRLC_src, *piRLC_dst, 1, 0);  // use kernel mode for 8 neighbours, no frame at image border

      if( ierr < 0) {     // Error ?
        return( ierr);     // Return the error to the caller
      }

      // exchange source and destination
      iRLC_tmp = *piRLC_src; *piRLC_src = *piRLC_dst; *piRLC_dst = iRLC_tmp;
    }

    ierr = rl2t_invert( *piRLC_src, *piRLC_dst, 0, xmax);

    if( ierr < 0) {     // Error ?
      return( ierr);     // Return the error to the caller
    }

    // exchange source and destination
    iRLC_tmp = *piRLC_src; *piRLC_src = *piRLC_dst; *piRLC_dst = iRLC_tmp;
  }

  return( 0);    // return OK
}

/****************************************************************************
* rl2t_Dilate2
*
* Dilate RLCs, add border.
*
* piRLC_src:   Pointer to RLC image pointer
*              On entry, this are the source RLC's
*              On exit, the dilated RLC's are placed here.
* piRLC_dst:   Pointer to RLC image pointer
*              Is used to play ping/pong while dilating RLC's
*              images.
* nDilate      number of dilate runs, must be >= 0
* label:       On entry, a specifix label can be extracted from the
*              RLC's placed in *piRLC_src.
*              If label >  0 only the named object with "label" is used.
*              If label <= 0 all labels are used.
* xmax:        x-size of uncoded original image
*
* return:   0  OK
*         < 0  error
*
*****************************************************************************
*/

int rl2t_Dilate2( Timages **piRLC_src,  // Pointer to RLC image pointer. Src in, result out,
                  Timages **piRLC_dst,  // Pointer to RLC image pointer. Use for temp processing.
                  int nDilate,          // # of dilate steps
                  int label,            // > 0: use specific object, <= 0: use all objects
                  int xmax)             // x-size of uncoded original image
{
  int ierr, i;
  Timages *iRLC_tmp;

  if( nDilate > 0) {                   // Have something to do

    // The dilation is done as: inverting, erosion, inverting

    ierr = rl2t_invert( *piRLC_src, *piRLC_dst, label, xmax);

    if( ierr < 0) {     // Error ?
      return( ierr);     // Return the error to the caller
    }

    // exchange source and destination
    iRLC_tmp = *piRLC_src; *piRLC_src = *piRLC_dst; *piRLC_dst = iRLC_tmp;

    // erode

    for( i = 0; i < nDilate; i++) {

      ierr = rl2t_eros2( *piRLC_src, *piRLC_dst, 1, xmax);  // use kernel mode for 8 neighbours, with frame at image border

      if( ierr < 0) {     // Error ?
        return( ierr);     // Return the error to the caller
      }

      // exchange source and destination
      iRLC_tmp = *piRLC_src; *piRLC_src = *piRLC_dst; *piRLC_dst = iRLC_tmp;
    }

    ierr = rl2t_invert( *piRLC_src, *piRLC_dst, 0, xmax);

    if( ierr < 0) {     // Error ?
      return( ierr);     // Return the error to the caller
    }

    // exchange source and destination
    iRLC_tmp = *piRLC_src; *piRLC_src = *piRLC_dst; *piRLC_dst = iRLC_tmp;
  }

  return( 0);    // return OK
}

/****************************************************************************
* rl2t_RemoveLabelRLCs
*
* Remove RLC's of a specific object from a RLC image.
*
* piRLC_src:   Pointer to src RLC image
* pObj:        Remove RLC's for this object
*
* return:   0  OK
*         < 0  error
*
*****************************************************************************
*/

int rl2t_RemoveLabelRLCs( Timages *iRLC_src,   // Pointer to src RLC image
                          Trl2objdst *pObj)    // Remove RLC's for this object
{
  int x, y, xx;
  int16 label;
  register Trl2desc *ps;
  register Trl2desc *pd;

  // Scan the RLC's in the rectangle around the object

  xx = getxx( iRLC_src);

  label = pObj->label;               // Get label

  for( y = pObj->ymin; y <= pObj->ymax; y++ ) {

    ps = (Trl2desc *)pixad( 0, y, iRLC_src);
    pd = ps;

    for(x = 0; x < xx; x += sizeof(Trl2desc)/sizeof(int16), ps++) {

      if (ps->rlab < 0) {                      // end of row for this line

        if( ps != pd) {                        // We have already skipped some RLC's
          memcpy( pd, ps, sizeof( Trl2desc));  // Copy down the end marker
        }
        break;                                 // take next row
      }

      if( ps->rlab != label) {                 // Is not the skipped object

        if( ps != pd) {                        // We have already skipped some RLC's
          memcpy( pd, ps, sizeof( Trl2desc));  // Copy down
        }

        pd++;
      }
    }
  }

  return( 0);   // return OK
}

/****************************************************************************
* rl2t_IsolateLabelRLC
*
* Keep RLCs of a specific object, remove RLCs from other objects.
*
* piRLC_src:   Pointer to src RLC image
* pObj:        Remove RLC's for this object
*
* return:   0  OK
*         < 0  error
*
*****************************************************************************
*/

int rl2t_IsolateLabelRLC( Timages *iRLC_src,   // Pointer to src RLC image
                          Trl2objdst *pObj)    // Remove RLC's for this object
{
  int x, y, xx, yy;
  int16 label;
  register Trl2desc *ps;
  register Trl2desc *pd;

  // Scan the RLC's in the rectangle around the object

  xx = getxx( iRLC_src);
  yy = getyy( iRLC_src);

  label = pObj->label;               // Get label

  for( y = 0; y < yy; y++ ) {

    ps = (Trl2desc *)pixad( 0, y, iRLC_src);
    pd = ps;

    for(x = 0; x < xx; x += sizeof(Trl2desc)/sizeof(int16), ps++) {

      if (ps->rlab < 0) {                      // end of row for this line

        if( ps != pd) {                        // We have already skipped some RLC's
          memcpy( pd, ps, sizeof( Trl2desc));  // Copy down the end marker
        }
        break;                                 // take next row
      }

      if( ps->rlab == label) {                 // Keep this object

        if( ps != pd) {                        // We have already skipped some RLC's
          memcpy( pd, ps, sizeof( Trl2desc));  // Copy down
        }

        pd++;
      }
    }
  }

  return( 0);   // return OK
}

/****************************************************************************
* rl2t_ResetLabelRLCs
*
* Resets the labels in a run length coded image.
*
* If a run length coded image was labeled, the RLCs hold
* information of the labeling.
* After RLC manipulation and a new labeling this information
* has to be removed, otherwise the new labeling fails.
*
* piRLC_src:   Pointer to src RLC image
*
* return:   0  OK
*         < 0  error
*
*****************************************************************************
*/

int rl2t_ResetLabelRLCs( Timages *iRLC_src)   // Pointer to src RLC image
{
  int x, y, xx, yy;
  Trl2desc *ps;

  // Scan the RLC's in the rectangle around the object

  xx = getxx( iRLC_src);
  yy = getyy( iRLC_src);

  for( y = 0; y < yy; y++ ) {

    ps = (Trl2desc *)pixad( 0, y, iRLC_src);

    for(x = 0; x < xx; x += sizeof(Trl2desc)/sizeof(int16), ps++) {

      if (ps->rlab < 0) {                      // end of row for this line

        break;                                 // take next row
      }

      ps->rlab = 0;                        /* write dummy Label            */
      ps->rlid = 0;                        /* write dummy index            */
    }
  }

  return( 0);   // return OK
}

/******************************** E O F *****************************/
