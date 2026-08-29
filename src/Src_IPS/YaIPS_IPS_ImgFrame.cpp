/* **************************************************************************
/@
/@ Short-title: generate frame around window
/@
/@ ==========================================================================
/@
/@ INDEX
/@   # Mframe    # frame     # boundary treatment
/@
/@ USER DESCRIPTION
/@.. frame  image width value xsize ysize
/@
/@.     Set or extend the boundary of an image:
/@      * set boundary with "value"  if "width" <  0
/@      * extrapolate "image"        if "width" >  0
/@
/@      If the given frame sizes "xsize" and "ysize" are zero ,
/@      the sizes of "image" are taken.
/@      If given size is less than "image" size, a smaller frame is drawn
/@      positioned to origin in the upper left corner.
/@
/@      ALGORITHM
/@
/@      PARAMETER
/@      image              image
/@      width              width of boundary = abs(width)
/@                         > 0:  extrapolate mode
/@                         < 0:  set mode
/@      value              value to set if "width" has negative sign,
/@                         dummy else
/@      xsize              all over xsize of frame
/@      ysize              all over ysize of frame
/@
/@      RESTRICTIONS
/@      Only the following data types are allowed:
/@        "image"
/@         int8
/@         int16
/@         int32
/@         sfloat
/@
/@      Always in place.
/@
/@      SEE ALSO 
/@
/@ FUNCTION DESCRIPTION
/@   #include "portab.h"
/@   #include "sip.h"
/@
/@   int frame(image, width, value, xsize, ysize)
/@     Timages *image;          source and destination image
/@     int16  width;            abs(width) = width of boundary
/@     int32  value;            value to set if width has negative sign
/@                              dummy else
/@     int16  xsize;            all over xsize of frame
/@     int16  ysize;            all over ysize of frame
/@
/@   RETURN VALUES
/@   Negativ in case of error.
/@
/@ REFERENCES
/@   framenm.c
/@
/@ MODIFICATIONS
/@   V 1.00 : First edition released.
/@   V 2.00 : extended for 16/32-bit images. SIP II
/@   V 2.01 : also for smaller sizes then size of dst. SIP IIb +
/@            Configurierung : image TY_BYTE | ... statt TY_ANY (P.S OCT 87)
/@            Modified pointer arithmetik in extrapolate (*--d18 = *d18)
/@   V 2.02 : SIP IIe. US
/@   V 3.00 : removed local bclip8/16 macros
/@   V 3.10 : sfloat supported too
/@   V 3.11 : int16 inner loop variables ->int
/@ ********************************************************************/

#include <windows.h>
#include <math.h>

#include "YaIPS_IPS_Interface.h" // 15.05.2025 RR: Need this for IPS defines
#include "YaIPS_LanguageStrings.h"  // Language string definitions

#define min(x,y)         (  (x) < (y)    ? (x) :  (y) )

/*===================== IP ROUTINE ========================================*/

int frame( Timages *dst, int wid, int val, int xx, int yy)
{
  register int     w, x, y;
  register int8   *s18,  *s28,  *d18,  *d28;
  register int16  *s116, *s216, *d116, *d216;
  register int32  *s132, *s232, *d132, *d232;
  register sfloat *s1sf, *s2sf, *d1sf, *d2sf;
  int16    v16;
  int      xmin, ymin, jump;
  int8     v8;

  if(uticheck(dst,DV_HOST,TY_ANY)) return(-3);
  xmin = getxx(dst);
  ymin = getyy(dst);
  jump = getxm(dst) ;
  xmin = (xx) ? (min(xx,xmin)) : xmin;      /* frame size <= size of dst */
  ymin = (yy) ? (min(yy,ymin)) : ymin;

  if (  getyp(dst)  == TY_BYTE )
  {
    if (wid > 0)       /* extrapolate */
    {
      for (w=0; w < wid; w++)
      {
	s18 = pixad(0, wid-w, dst);
	s28 = pixad(0, ymin-wid-1+w, dst);
	d18 = s18-jump;
	d28 = s28+jump;
	x = xmin; while ( --x >= 0 )
	{
	  *d18++ = *s18++;                     /* upper boundary */
	  *d28++ = *s28++;                     /* lower boundary */
	}
	d18 =  pixad(wid-w-1, 0, dst);         /* left boundary */
	d28 =  pixad(xmin-wid+w, 0, dst);      /* right boudary */
	y = ymin; while ( --y >= 0 )
	{
	  *d18 = *(d18 +1);
	  *d28 = *(d28 -1);
	  d18 += jump;
	  d28 += jump;
	}
      }
      return(0);
    }
    else              /* clear/set with value */
    {
      bclip8(val,&v8);
      w = -wid; while ( --w >= 0 )
      {
	d18 = pixad(0, w, dst);
	d28 = pixad(0, ymin-w-1, dst);
	x = xmin; while ( --x >= 0 )
	{
	  *d18++ = v8; *d28++ = v8;
	}
	d18 =  pixad(w, 0, dst);
	d28 =  pixad(xmin-w-1, 0, dst);
	y = ymin; while ( --y >= 0 )
	{
	  *d18 = v8;
	  *d28 = v8;
	  d18 += jump;
	  d28 += jump;
	}
      }
      return(0);
    }
  }
  else if (  getyp(dst) == TY_INT16 )
  {
    if (wid > 0)       /* extrapolate */
    {
      for (w=0; w < wid; w++)
      {
	s116 = pixadt(0, wid-w, dst, int16);
	s216 = pixadt(0, ymin-wid-1+w, dst, int16);
	d116 = s116-jump;
	d216 = s216+jump;
	x = xmin; while ( --x >= 0 )
	{
	  *d116++ = *s116++; *d216++ = *s216++;
	}
	d116 =  pixadt(wid-w-1, 0, dst, int16);         /* left boundary */
	d216 =  pixadt(xmin-wid+w, 0, dst, int16);      /* right boudary */
	y = ymin; while ( --y >= 0 )
	{
	  *d116 = *(d116 +1);
	  *d216 = *(d216 -1);
	  d116 += jump;
	  d216 += jump;
	}
      }
      return(0);
    }
    else              /* clear/set with value */
    {
      bclip16(val,&v16);
      w = -wid; while ( --w >= 0 )
      {
	d116 = pixadt(0, w, dst, int16);
	d216 = pixadt(0, ymin-w-1, dst, int16);
	x = xmin; while ( --x >= 0 )
	{
	  *d116++ = v16; *d216++ = v16;
	}
	y = ymin; while ( --y >= 0 )
	{
	  d116 = pixadt(w, y, dst,int16);
	  d216 = pixadt(xmin-w-1, y, dst,int16);
	  *d116 = *d216 = v16;
	}
      }
      return(0);
    }
  }
  else if (  getyp(dst) == TY_INT32 )
  {
    if (wid > 0)       /* extrapolate */
    {
      for (w=0; w < wid; w++)
      {
	s132 = pixadt(0, wid-w, dst, int32);
	s232 = pixadt(0, ymin-wid-1+w, dst, int32);
	d132 = s132-jump;
	d232 = s232+jump;
	x = xmin; while ( --x >= 0 )
	{
	  *d132++ = *s132++; *d232++ = *s232++;
	}
	d132 =  pixadt(wid-w-1, 0, dst, int32);         /* left boundary */
	d232 =  pixadt(xmin-wid+w, 0, dst, int32);      /* right boudary */
	y = ymin; while ( --y >= 0 )
	{
	  *d132 = *(d132 +1);
	  *d232 = *(d232 -1);
	  d132 += jump;
	  d232 += jump;
	}
      }
      return(0);
    }
    else              /* clear/set with value */
    {
      w = -wid; while ( --w >= 0 )
      {
	d132 = pixadt(0, w, dst, int32);
	d232 = pixadt(0, ymin-w-1, dst, int32);
	x = xmin; while ( --x >= 0 )
	{
	  *d132++ =  val; *d232++ = val;
	}
	y = ymin; while ( --y >= 0 )
	{
	  d132 = pixadt(w, y, dst,int32);
	  d232 = pixadt(xmin-w-1, y, dst,int32);
	  *d132 = *d232 = val;
	}
      }
      return(0);
    }
  }
  else if (  getyp(dst) == TY_SFLOAT )
  {
    if (wid > 0)       /* extrapolate */
    {
      for (w=0; w < wid; w++)
      {
	s1sf = pixadt(0, wid-w, dst, sfloat);
	s2sf = pixadt(0, ymin-wid-1+w, dst, sfloat);
	d1sf = s1sf-jump;
	d2sf = s2sf+jump;
	x = xmin; while ( --x >= 0 )
	{
	  *d1sf++ = *s1sf++; *d2sf++ = *s2sf++;
	}
	d1sf =  pixadt(wid-w-1, 0, dst, sfloat);         /* left boundary */
	d2sf =  pixadt(xmin-wid+w, 0, dst, sfloat);      /* right boudary */
	y = ymin; while ( --y >= 0 )
	{
	  *d1sf = *(d1sf +1);
	  *d2sf = *(d2sf -1);
	  d1sf += jump;
	  d2sf += jump;
	}
      }
      return(0);
    }
    else              /* clear/set with value */
    {
      w = -wid; while ( --w >= 0 )
      {
	d1sf = pixadt(0, w, dst, sfloat);
	d2sf = pixadt(0, ymin-w-1, dst, sfloat);
	x = xmin; while ( --x >= 0 )
	{
	  *d1sf++ =  (sfloat)val; *d2sf++ = (sfloat)val;
	}
	y = ymin; while ( --y >= 0 )
	{
	  d1sf = pixadt(w, y, dst,sfloat);
	  d2sf = pixadt(xmin-w-1, y, dst,sfloat);
	  *d1sf = *d2sf = (sfloat)val;
	}
      }
      return(0);
    }
  }
  else
  {

    errstring = ERR_IPS_DATA_COMP_NO_SUPP;
    return(-1);
  }
}

/***************************************************************************/
