/* **************************************************************************
/@
/@ Short-title: Vector filter routines
/@
/@ ==========================================================================
/@
/@ INDEX
/@   # Mvfiltn   # vfiltn    # general vector filter
/@   # Mvlowpa   # vlowpa    # lowpass vector filter
/@
/@ USER DESCRIPTION
/@.. vfiltn vsrc vdst exp offset n c0 c1 .. c24
/@.     Vector filter with "n" integer coefficients "c0 ...".
/@      The source vector "vsrc" is filtered, shifted by "exp" bits,
/@      biased by "offset" and written into the destination vector "vdst".
/@      Boundary treatment :
/@      At the left and right boundaries, the source vector is expanded.
/@
/@      ALGORITHM
/@      dst[x] = sum (src[x+j-m] * c[j]) * 2^exp + offset
/@              with:   j = 0 .. n-1;   m = n/2;        n should be odd
/@
/@      PARAMETER
/@      vsrc            vector of type int32
/@      vdst            vector of type int32
/@      exp             shift count (-8 ... 8)
/@      offset          result bias
/@      n               # coefficients (1 ... 25)
/@      c0 ...          coefficients (integer)
/@
/@      RESTRICTIONS
/@      In place calculation is not possible.
/@
/@      SEE ALSO
/@      vwrfiltn, vlowpa.
/@
/@
/@.. vlowpa  vsrc vdst cnt
/@.     Vector lowpass filter.
/@
/@      The source vector "vsrc" is filtered and written into the
/@      destination vector "vdst".
/@      There are 8 filter coefficients of constant value 1.
/@      The result is normalized (divided by 8 with rounding).
/@      At the left and right boundaries, the source vector is expanded.
/@
/@      Because of the even kernel size, the filter is un_symmetric.
/@      This was done by purpose to equalize any differences between
/@      odd and even values during histogram or colsum/rowsum filtering.
/@      A filter with odd kernel size could not do this.
/@      Filtering a vector one time leads to a shift of extrema of
/@      a half index towards the start of the vector.
/@
/@      "cnt" determines how many times the data of "srcvec" will be
/@      filtered.
/@      Filter direction will change with every run:
/@      The first  run starts at the beginning of "srcvec",
/@      the second run starts at the end  of "dstvec", and so on.
/@      If "cnt" is even, extrema will not be dislocated by filtering.
/@
/@      ALGORITHM
/@      dst[x] = (src[x-3] + src[x-2] + src[x-1] + src[x] +
/@                src[x+1] + src[x+2] + src[x+3] + src[x+4] + 4)/8;
/@
/@      PARAMETER
/@      vsrc             vector of type int32
/@      vdst             vector of type int32
/@      cnt              (int16) number of runs
/@
/@      RESTRICTIONS
/@      In place calculation is not possible.
/@ 
/@      SEE ALSO 
/@
/@ FUNCTION DESCRIPTION
/@
/@   #include "portab.h"
/@   #include "sip.h"
/@   #include "tstpr.h"
/@
/@   int vfiltn (src, dst, exp, offset, n, c)
/@      Tvector *src, *dst;
/@      int16 exp, offset, n, *c;
/@
/@   int vlowpa (src, dst, cnt)
/@      Tvector *src, *dst;
/@      int16   cnt;
/@
/@      vlowpa creates and allocates a vectopr for temporary use
/@      only if the number of filtering passes is greater than one.
/@      The best way for using vlowpa in a standalone environment
/@      where a critical memory fragmentation has to be feared is
/@      to call vlowpa repeatedly either with a previously allocated
/@      temporay vector of with changing source and destination vectors.
/@      Notice that extrema will be shifted to the end of the destination
/@      vector by a half index each time vector data are filtered.
/@
/@   RETURN VALUES
/@   All routines return 0 after successful execution.
/@   In case of an error a message is stored in "errstring" and
/@   a negative value is returned.
/@
/@ *************************************************************************/

#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "YaIPS_IPS_Interface.h" // 15.05.2025 RR: Need this for IPS defines
#include "YaIPS_LanguageStrings.h"  // Language string definitions

//----------------------------------------

#define N       25

/*===================== IP ROUTINES =======================================*/

/*--------------------- vfiltn --------------------------------------------*/

// General vector filter
int vfiltn( Tvector *src, Tvector *dst, int exp, int offset, int n, int16 *c)
{
  register int32 *s, *d;
  register int i;
  register int16 *p;
  register int32 l;
  register int x, xmin, xmnm, m=n/2;

  if (utvcheck (src, DV_HOST, TY_INT32)) return (-1);
  if (utvcheck (dst, DV_HOST, TY_INT32)) return (-2);

  if (n < 0)
     {errstring = ERR_IPS_PARAM_BAD; //(char *)"Number of filter coefficients must be positive!";
      return (-4);}

  xmin = vgetnm (src);
  if ( xmin < (n/2 + 1) )
     {errstring = ERR_IPS_VEC_TOO_SHORT;
      return (-4);}

  if (ve_alloc (dst, (int32)xmin, (int16)sizeof(int32), TY_INT32)) return (-3);

  s = (int32 *) vgetpm (src);
  d = (int32 *) vgetpm (dst);

  xmnm = xmin - (n - m);

  for (x=0; x<xmin; x++)
  {
      if (x < m) {                       /* start of vector */

        for (l=0, p=c, i=0; i<=m-x; i++) l += *p++;
        l *= *s++;
        while (i++ < n)                  l += *p++    *  *s++;
        s -= n - m + x;
      }

      else if (x <= xmnm) {               /* middle of vector */

         for (l=0, p=c, i=n; i>0; i--)     l += *p++    *  *s++;
         s -= n - 1;
      }

      else {                             /* end of vector */

         for (l=0, p=c, i=0; i<xmin-x+m; i++)   l += *p++    *  *s++;
         while (i++ < n)                        l += *p++    *  *(s-1);
         s -= xmin - x + m - 1;
      }

      if (exp < 0) l >>= -exp;
      else         l <<=  exp;

      l += offset;
      *d++ = l;
  }
  vputnm (dst, (int32) xmin);

  return (0);
}

/*--------------------- vlowpa --------------------------------------------*/

// Lowpass vector filter
int vlowpa( Tvector *srcvec, Tvector *dstvec, int cnt)
{
  int32 abuf[10];                                 /* Anfangsbuffer */
  int32 ebuf[11];                                 /* Endebuffer */
  int32 len;                                      /* Laenge der Vektoren */
  register int i,j,k;                             /* Laufvariablen */
  register int32 *ps;
  register int32 *pd;
  int32    *ps0, *pd0;
  Tvector  *tmpvec = NULL;

  if (utvcheck (srcvec, DV_HOST, TY_INT32)) return (-2);
  if (utvcheck (dstvec, DV_HOST, TY_INT32)) return (-3);

  len = vgetnm (srcvec);
  if (ve_alloc (dstvec, len, (int16) sizeof (int32), TY_INT32)) return (-13);

  if (cnt > 1)  {           /* allocate only for more than one filter passes */
    if ((tmpvec = ve_ucreate(DV_HOST)) == VENULL)   return(-4);
    if (ve_alloc (tmpvec, len, (int16) sizeof (int32), TY_INT32)) return (-14);
    vputnm(tmpvec,len);
  }

  ps0 = (int32 *) vgetpm (srcvec);
  pd0 = (int32 *) vgetpm (dstvec);

  for (k=1; k<=cnt; k++)  {

    ps = ps0;             /* init src and dst pointer */
    pd = pd0;

    if (k & 0x01)  {      /*   destination is dstvec, direction: +   */

      PRINTF0("filter from srcvec/tmpvec into dstvec\n");
      /* Randproblem Anfang */

      abuf[0] = abuf[1] = abuf[2] = abuf[3] = *ps++;
      for(i = 4; i <= 9; i++)
        abuf[i] = *ps++;

      PRINTF0("abuf: ");
      for(i = 0; i <= 9; i++)    PRINTF1("%2ld, ",abuf[i]);
      PRINTF0("\n");

      for(j = 0; j <= 2; j++)
        *pd++ = (abuf[j+0] + abuf[j+1] + abuf[j+2] + abuf[j+3] + abuf[j+4] +
           abuf[j+5] + abuf[j+6] + abuf[j+7] + 4) >> 3;

      /* Mitte des Vektors */

      ps = ps0;
      for(j = 3; j <= len - 5; j++, ps++)
        *pd++ = (ps[0] + ps[1] + ps[2] + ps[3] + ps[4] + ps[5] +
           ps[6] + ps[7] + 4) >> 3;

      /* Randproblem Ende */

      for(i = 0; i <= 5; i++)
        ebuf[i] = *ps++;
      ebuf[6] = ebuf[7] = ebuf[8] = ebuf[9] = ebuf[10] = *ps;

      PRINTF0("ebuf: ");
      for(i = 0; i <= 10; i++)    PRINTF1("%2ld, ",ebuf[i]);
      PRINTF0("\n");

      for(j = 0; j <= 3; j++)
        *pd++ = (ebuf[j+0] + ebuf[j+1] + ebuf[j+2] + ebuf[j+3] + ebuf[j+4] +
           ebuf[j+5] + ebuf[j+6] + ebuf[j+7] + 4) >> 3 ;

      if (cnt > 1)  {
         ps0 = (int32 *)vgetpm(dstvec);  ps0 += len - 1;
         pd0 = (int32 *)vgetpm(tmpvec);  pd0 += len - 1;
      }

    } else  {               /*   destination is tmpvec, direction: -   */

      PRINTF0("filter from dstvec into tmpvec\n");

      /* Randproblem Anfang */

      abuf[9] = abuf[8] = abuf[7] = abuf[6] = *ps--;
      for(i = 5; i >= 0; i--)
        abuf[i] = *ps--;

      PRINTF0("abuf: ");
      for(i = 0; i <= 9; i++)    PRINTF1("%2ld, ",abuf[i]);
      PRINTF0("\n");

      for(j = 2; j >= 0; j--)
        *pd-- = (abuf[j+0] + abuf[j+1] + abuf[j+2] + abuf[j+3] + abuf[j+4] +
           abuf[j+5] + abuf[j+6] + abuf[j+7] + 4) >> 3;

      /* Mitte des Vektors */

      ps = ps0;    ps -= 7;
      for(j = 3; j <= len - 5; j++, ps--)
        *pd-- = (ps[0] + ps[1] + ps[2] + ps[3] + ps[4] + ps[5] +
           ps[6] + ps[7] + 4) >> 3;

      /* Randproblem Ende */

      ps = ps0;    ps -= len - 7;     /* len - 1 - 6 */
      for(i = 10; i >= 5; i--)
        ebuf[i] = *ps--;
      ebuf[0] = ebuf[1] = ebuf[2] = ebuf[3] = ebuf[4] = *ps;

      PRINTF0("ebuf: ");
      for(i = 0; i <= 10; i++)    PRINTF1("%2ld, ",ebuf[i]);
      PRINTF1("ps00=%ld\n",*(ps0-(len-1)));

      for(j = 3; j >= 0; j--)
        *pd-- = (ebuf[j+0] + ebuf[j+1] + ebuf[j+2] + ebuf[j+3] + ebuf[j+4] +
           ebuf[j+5] + ebuf[j+6] + ebuf[j+7] + 4) >> 3 ;

      /* set start position for ps and pd */
      ps0 = (int32 *)vgetpm(tmpvec);
      pd0 = (int32 *)vgetpm(dstvec);

    }
  }

    /* copy from tmpvec into dstvec */
  if (!(cnt & 0x01))  {          /* if not: destination is dstvec */
    PRINTF0("COPY from tmpvec into dstvec\n");
    ps = (int32 *)vgetpm(tmpvec);
    pd = (int32 *)vgetpm(dstvec);
    i = len;
    while (--i >= 0)  *pd++ = *ps++;
  }

  if (cnt > 1)  {
    ve_remove(tmpvec);
  }

  vputnm(dstvec,len);

  return (0);
}

/******************************** E O F *****************************/
