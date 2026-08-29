/* **************************************************************************
/@
/@ Name: YaIPS_IPS_VecCopy.cpp
/@
/@ Short-title: copy part of a vector into another vector
/@
/@ ==========================================================================
/@
/@ INDEX
/@
/@  # Mvcopy      # vcopy      # copy part of a vector into another vector
/@
/@ USER DESCRIPTION
/@.. vcopy    srcvec dstvec srcbegin srcnumber mode
/@
/@.     Copy part of "srcvec" specified by startindex "srcbegin" and
/@.     item number "srcnumber" into "dstvec".
/@
/@      If mode = 0: "dstvec" is cleared and data are copied starting
/@                   at the beginning of "dstvec".
/@      if mode = 1: copied data are appended to "dstvec".
/@
/@      The range of startindex "srcbegin" is 0 .... len-1,
/@        with  len = number of stored items in "srcvec".
/@      If "srcnumber" = -1, all data of "srcvec" are copied.
/@
/@      All data types are allowed.
/@
/@      ALGORITHM
/@
/@      PARAMETERS
/@      srcvec, dstvec  source and destination vectors
/@      srcbegin        (int32) start index for copy (1 ...)
/@      srcnumber       (int32) number of items to copy
/@      mode            (int16) copy mode
/@
/@      RESTRICTIONS
/@
/@      'dstvec' must have the same data type as 'srcvec'.
/@      Inplace operation is allowed.
/@
/@      SEE ALSO 
/@      vsetval, vgetval.
/@      vconv.
/@
/@ FUNCTION DESCRIPTION
/@   #include "portab.h"
/@   #include "sip.h"
/@   #include "sipve.h"
/@   #include "tstpr.h"
/@
/@   int vcopy(srcvec,dstvec,srcbegin,srcnumber,mode)
/@      Tvector *srcvec,*dstvec;
/@      int32 srcbegin, srcnumber;
/@      int16 mode;
/@
/@      For "dstvec" data space will be allocated using ve_alloc().
/@
/@ RETURN VALUE
/@
/@ negativ in case of error
/@
/@  -2: utvcheck-error of srcvec
/@  -3: utvcheck-error of dstvec
/@  -4: incompatible vectortypes
/@  -5: srcvec empty
/@  -6: 'begin' outside allowed range
/@  -7: 'num' outside allowed range
/@
/@ REFERENCES
/@
/@ *************************************************************************/

#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "YaIPS_IPS_Interface.h" // 15.05.2025 RR: Need this for IPS defines

int vcopy( Tvector *vsrc, Tvector *vdst, int begin, int num, int mode)
{
  int32 srcnm, dstnm;            /* number of stored (valid) items         */
  register int32 i;              /* index                                  */
  int16 srctype, dsttype;        /* type of vsrc, vdst                     */
  int err;

  /* vsrc and vdst must be of type TY_STANDARD or TY_ANY */
  if((utvcheck(vsrc,DV_HOST,TY_INT8))&&(utvcheck(vsrc,DV_HOST,TY_INT16))&&
     (utvcheck(vsrc,DV_HOST,TY_INT32))&&(utvcheck(vsrc,DV_HOST,TY_SFLOAT))&&
     (utvcheck(vsrc,DV_HOST,TY_LFLOAT))) return(-2);

  if((srcnm = vgetnm(vsrc)) <= 0L) return(-5);
  if (begin >= srcnm) return(-6);

  if (num < 0) num = (srcnm - begin);    /* all elements up to srcvecend */
  if ((begin + num) > srcnm) return(-7);

  PRINTF2("vcopy: vectors and input data ok, begin %ld, num %ld\n",begin,num);

  srctype = vgetyp(vsrc);
  PRINTF1("vcopy: srcvec type: %d\n",(int)srctype);
  dsttype = vgetyp(vdst);
  if(dsttype != TY_ANY) {
   if((utvcheck(vdst,DV_HOST,TY_INT8))&&(utvcheck(vdst,DV_HOST,TY_INT16))&&
      (utvcheck(vdst,DV_HOST,TY_INT32))&&(utvcheck(vdst,DV_HOST,TY_SFLOAT))&&
      (utvcheck(vdst,DV_HOST,TY_LFLOAT))) return(-3);
   if(srctype != dsttype) return(-4);
  }
  PRINTF1("vcopy: dstvec type: %d\n",(int)vgetyp(vdst));
  /*-----------------------------------------------------------------------*/
  /* get pointer adresses and realloc data of dstvec and copy */
  if (mode == 0) {
    dstnm = 0L;             /* in case mode == 0, delete dstvec */
    if (num > vgetln(vdst)) {
      if( (err = ve_alloc(vdst,num,(int16)cvsize(srctype),srctype))) {
        return(err);
      }
    }
  } else {
    dstnm = vgetnm(vdst);
    if ((num + dstnm) > vgetln(vdst)) {
      if( (err = ve_realloc(vdst,(dstnm + num),(int16)cvsize(srctype),srctype))) {
        return(err);
      }
    }
  }
  //x/PRINTF1("vcopy: dstvec type: %d\n",(int)vgetyp(vdst));
  //x/PRINTF1("vcopy: number items dstvec before loop: %ld\n",dstnm);

  switch (srctype) {
    case TY_INT8:
      {
        int8 *psrc, *pdst;             /* base adress of src & dst vector data   */

        psrc = ((int8 *)vgetpm(vsrc) + begin);
	      pdst = ((int8 *)vgetpm(vdst) + dstnm);
	      for( i = num;i > 0L;i--) {
	        *pdst++ =   *psrc++; }
      }
	  break;
    case TY_INT16:
      {
        int16 *psrc, *pdst;             /* base adress of src & dst vector data   */

        psrc = ((int16 *)vgetpm(vsrc) + begin);
	      pdst = ((int16 *)vgetpm(vdst) + dstnm);
	      for( i = num;i > 0L;i--) {
	        *pdst++ =  *psrc++; }
      }
	  break;
    case TY_INT32:
      {
        int32 *psrc, *pdst;             /* base adress of src & dst vector data   */

        psrc = ((int32 *)vgetpm(vsrc) + begin);
	      pdst = ((int32 *)vgetpm(vdst) + dstnm);
	      for( i = num;i > 0L;i--) {
	        *pdst++ =  *psrc++; }
    }
	  break;
    case TY_SFLOAT:
      {
        sfloat *psrc, *pdst;             /* base adress of src & dst vector data   */

        psrc = ((sfloat *)vgetpm(vsrc) + begin);
	      pdst = ((sfloat *)vgetpm(vdst) + dstnm);
	      for(i = num;i > 0L;i--) {
	        *pdst++ = *psrc++; }
      }
	  break;
    case TY_LFLOAT:
      {
        lfloat *psrc, *pdst;             /* base adress of src & dst vector data   */

        psrc = ((lfloat *)vgetpm(vsrc) + begin);
	      pdst = ((lfloat *)vgetpm(vdst) + dstnm);
	      for(i = 0L;i < num;i++) {
	        *pdst++ = *psrc++; }
      }
	  break;
  }
  //x/PRINTF0("vcopy: ok\n");
  /*-----------------------------------------------------------------------*/
  /* update number of stored items in vdst */
  vputnm( vdst,(dstnm + num));
  //x/PRINTF1("vcopy: number items dstvec after loop: %ld\n",(dstnm + num));

  return(0);
}

/*******************************end of file*********************************/

