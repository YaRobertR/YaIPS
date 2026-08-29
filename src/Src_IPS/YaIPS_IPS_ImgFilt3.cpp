/* **************************************************************************
/@
/@ Short-title: integer 3*3 filter routine : free configuration
/@
/@ ==========================================================================
/@
/@ INDEX
/@ # Mifilt3     # ifilt3       #   integer 3*3 filter routines
/@
/@ USER DESCRIPTION
/@.. ifilt3   srcim dstim exp offset c0 c1 c2 c3 c4 c5 c6 c7 c8
/@
/@.     3*3 filter with free configuration of (integer !) coefficients.
/@
/@      Temporary summation is done with 32bit precision.
/@      Quicker than version ffilt3 using sfloat's.
/@
/@      ALGORITHM
/@      ---------+--------------------------------
/@               |  c0 c1 c2
/@      ifilt3   |  c3 c4 c5     * 2**exp + offset  linear 3*3 filter
/@               |  c6 c7 c8
/@      ---------+--------------------------------
/@
/@      PARAMETER
/@      srcim, dstim       source and destination images
/@      exp                (int16) additional gain exponent:  gain = 2**exp
/@      offset             (int16) additional offset
/@      c0 .. c8           (int16) coefficients
/@
/@      RESTRICTIONS
/@      Only the following combinations of data types are allowed:
/@        "srcim"  "dstim"
/@         int8     int8
/@         int32    int32
/@ 
/@      In place calculation is not possible.
/@      For int32 images an arithmetic overflow can occur.
/@
/@      SEE ALSO 
/@
/@ FUNCTION DESCRIPTION
/@   #include "portab.h"
/@   #include "sip.h"
/@   #include "tstpr.h"
/@
/@   int ifilt3 (srcim,dstim,exp,offset,c0,c1,c2,c3,c4,c5,c6,c7,c8)
/@     Timages *srcim, *dstim;              source and destination images
/@     int16    exp;                        gain = 2**exp
/@     int32    offset;                     offset
/@     int16    c0,c1,c2,c3,c4,c5,c6,c7,c8; coefficients
/@
/@ REFERENCES
/@   ffilt3.c
/@
/@ MODIFICATIONS
/@   V 1.00 : First edition released.
/@   V 2.01 : additional offset. SIP IIe. US
/@   V 2.02 : layout of short help
/@            summation in 32bit
/@   V 3.00 : use of btoi()
/@   V 3.01 : int32 images support too
/@ *************************************************************************/

#include <windows.h>
#include <math.h>

#include "YaIPS_IPS_Interface.h" // 15.05.2025 RR: Need this for IPS defines
#include "YaIPS_LanguageStrings.h"  // Language string definitions

/*===================== IP ROUTINE ========================================*/

/*----------------------ALL PURPOSE----------------------------------------*/

int ifilt3( Timages *src, Timages *dst, int exp, int offset,
            int c0, int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8)
{
  register int    *c;
  register int32  t;
  register int    jump, x, y;
  int             xmin, ymin, coeff[9], texp;
  
  if(uticheck( src,DV_HOST,TY_ANY)) return(-2);
  if(uticheck( dst,DV_HOST,TY_ANY)) return(-3);
  
  xmin = utxx2min(src,dst) - 2;
  ymin = utyy2min(src,dst) - 2;
  
  jump = getxm( src ) ;
  
  c = coeff;
  *c++=c0;*c++=c1;*c++=c2;*c++=c5;*c++=c8;*c++=c7;*c++=c6;*c++=c3;*c++=c4;
  texp = (exp >= 0) ? exp : -exp;
  
  if ((getyp(src) == TY_BYTE) && (getyp(dst) == TY_BYTE)) {
    register int8   *s8, *d8;

    for (y = 0; y < ymin; y++) {
      if(iabort()) break;                     /* check CNTRL C */
      s8 = pixad(0, y  , src);
      d8 = pixad(1, y+1, dst);
      x = xmin; 
      while (--x >= 0) {                                       /* x y */
        c = coeff;
        t  = *c++ * ( btoi(*s8++) ) ;                          /* 0 0 */
        t += *c++ * ( btoi(*s8++) ) ;                          /* 1 0 */
        t += *c++ * ( btoi(*s8  ) ) ; s8 += jump ;             /* 2 0 */
        t += *c++ * ( btoi(*s8  ) ) ; s8 += jump ;             /* 2 1 */
        t += *c++ * ( btoi(*s8  ) ) ;                          /* 2 2 */
        t += *c++ * ( btoi(*--s8) ) ;                          /* 1 2 */
        t += *c++ * ( btoi(*--s8) ) ; s8 -= jump ;             /* 0 2 */
        t += *c++ * ( btoi(*s8++) ) ;                          /* 0 1 */
        t += *c++ * ( btoi(*s8  ) ) ; s8 -= jump ;             /* 1 1 */
        t = (exp >= 0) ? t << texp : t >> texp;
        t += offset;
        bclip(t,d8);
        d8++;
      }
    }
    frame( dst,(int16)1,0L,xmin+2,ymin+2);   /*extrapolate 1pixel boundary */
    return ( 0 );
  } else if ((getyp(src) == TY_INT32) && (getyp(dst) == TY_INT32)) {
    register int32  *s32, *d32;

    for (y = 0; y < ymin; y++) {
      if(iabort()) break;                     /* check CNTRL C */
      s32 = (int32 *)pixadt(0, y  , src, int32);
      d32 = (int32 *)pixadt(1, y+1, dst, int32);
      x = xmin; 
      while (--x >= 0) {                                       /* x y */
        c = coeff;
        t  = *c++ * ( *s32++ ) ;                               /* 0 0 */
        t += *c++ * ( *s32++ ) ;                               /* 1 0 */
        t += *c++ * ( *s32   ) ; s32 += jump ;                 /* 2 0 */
        t += *c++ * ( *s32   ) ; s32 += jump ;                 /* 2 1 */
        t += *c++ * ( *s32   ) ;                               /* 2 2 */
        t += *c++ * ( *--s32 ) ;                               /* 1 2 */
        t += *c++ * ( *--s32 ) ; s32 -= jump ;                 /* 0 2 */
        t += *c++ * ( *s32++ ) ;                               /* 0 1 */
        t += *c++ * ( *s32   ) ; s32 -= jump ;                 /* 1 1 */
        t = (exp >= 0) ? t << texp : t >> texp;
        t += offset;
        *d32++ = t;
      }
    }
    frame( dst, (int16)1, 0L, xmin+2, ymin+2);   /*extrapolate 1pixel boundary */
    return ( 0 );
  } else {

    errstring = ERR_IPS_DATA_COMP_NO_SUPP;

    return(-1);
  }
}

/***************************************************************************/
