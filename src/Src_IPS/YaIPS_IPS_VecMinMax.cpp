/* **************************************************************************
/@
/@ Short-title: indices of maxima/minima (extrema) in vector
/@
/@ ==========================================================================
/@
/@ INDEX
/@
/@ # Mvminmax   # vminmax    # indices of maxima/minima (extrema) in vector
/@ # Mvmmsort   # vmmsort    # sort extremas according value
/@
/@ USER DESCRIPTION
/@.. vminmax srcvec resvec mode numextr mdist
/@
/@.     Indices of maxima and minima (extrema) of vector
/@
/@.. vmmsort srcvec resvec mode sortmode
/@
/@.     Sort extremas according value
/@
/@      First, all minima/maxima are searched. Then these minima/maxima
/@      are checked to have a minimum distance of "mdist" indices from each
/@      other. If the distance is shorter, only the better minima/maxima
/@      are selected. If more then "numextr" minima/maxima are found,
/@      the minima/maxima will be
/@        - sorted by value of the minima/maxima
/@        - sorted by index
/@      At last the number of minima/maxima is set to "numextr".
/@
/@      PARAMETERS
/@      srcvec          (int32) vector
/@      resvec          (int16) vector with result.
/@      mode            (int16) 0 = min, 1 = max
/@      numextr         (int16) max number of extrema
/@      mdist           (int16) minimum distance between minima/maxima
/@      sortmode        (int16) 0: according value, 1: left to right, 2: right to left
/@
/@      RESTRICTIONS
/@      "srcvec" must be of type int32.
/@      Minima/maxima found by "vminmax" are defined to be extrema, i.e
/@      they must have values greater resp. smaller to the left and to
/@      the right.
/@
/@         "0"  in   "1  1  0  1  1"   will be found as minimum at index=2,
/@         "1"  in   "0  1  0  0  0"   will be found as maximum at index=1,
/@      You will not detect
/@         "0"  in   "1  1  1  1  0"   as minimum,
/@         "1"  in   "0  0  0  0  1"   as maximum,
/@         "1"  in   "1  1  1  1  1"   as minimum or maximum.
/@
/@      SEE ALSO
/@      vrange, vedge, vaindex, vgetval.
/@
/@ FUNCTION DESCRIPTION
/@   #include "portab.h"
/@   #include "sip.h"
/@   #include "sipve.h"
/@   #include "tstpr.h"
/@
/@   int vminmax(srcvec,resultvec,mode,num,mdist)
/@   Tvector *srcvec;      vector ( vector type: int32 )
/@   Tvector *resultvec;   resultvector ( vector type: int16 )
/@   int16  mode;          0 = min, 1 = max.
/@   int16  num;           max number of minimas/maximas
/@   int16  mdist;         minimum distance between minimas/maximas
/@
/@   int vmmsort2(srcvec,resultvec,mode,sortmode)
/@   int vmmsort(srcvec,resultvec,mode)
/@   Tvector *srcvec;      vector ( vector type: int32 )
/@   Tvector *resultvec;   result ( vector type: int16 )
/@   int16  mode;          0 = min, 1 = max.
/@   int16 sortmode;       0: according value, 1: left to right, 2: right to left
/@
/@
/@ RETURN VALUES
/@      All routines return 0 after successful execution.
/@      In case of error a negative value is returned.
/@
/@      -16   Can not allocate resultvec (no memory)
/@      -18   To many entries in srcvec (max 32676 allowed)
/@      -19   mdist less then 0
/@      -20   num less then 1
/@      -121  srcvec wrong type
/@      -123  resultvec wrong type
/@
/@ *************************************************************************/

#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "YaIPS_IPS_Interface.h" // 15.05.2025 RR: Need this for IPS defines
#include "YaIPS_LanguageStrings.h"  // Language string definitions

/***************************************************************************
* vminmax
*
* indices of maxima/minima (extrema) in vector
*
****************************************************************************
*/

int vminmax( Tvector *srcvec,  // Source vector ( vector type: int32 )
             Tvector *desvec,  // Destination/result vector ( vector type: int16 )
             int mode,         // Mode: 0 = min, 1 = max
             int maxe,         // Max number of minimas/maximas
             int fang)         // minimum distance between minimas/maximas
{
  int i ;
  int maxind;             /* maximum index in vector     */
  int32 nitem;                     /* number of elements in result vector */
  int32 *psrc;            /* pointer to srcvec data      */
  int32 data, ldata;
  int16 *perg;                     /* pointer to desvec data      */
  int   flanke, lflanke,index,lindex,eindex;
  int16 temp, step;
  boolean exchange;

  if(utvcheck(srcvec,DV_HOST,TY_INT32)) return(-121);
  if(utvcheck(desvec,DV_HOST,TY_INT16)) return(-123);

  /* if not enough items -> returns with 0, dstvec empty */
  if( vgetnm(srcvec) < 1 ) {          /* check number of data */
    vputnm(desvec,0L);
    return(0);     /* no extrema, no error */
  }

  if( vgetnm(srcvec) > MAXINT16 ) {          /* check number of data */
    errstring = ERR_IPS_VEC_MORE_32676; //(char *)"vector has more than 32676 values";
    return(-18);
  }
  if( fang < 0 ) {                       /* check parameter */
    errstring = ERR_IPS_PARAM_BAD; //(char *)"mdist less then 0";
    return(-19);
  }
  if( maxe < 1 ) {                       /* check parameter */
    errstring = ERR_IPS_PARAM_BAD; //(char *)"Number of extrema less then 1";
    return(-20);
  }
  maxind = (int16)(vgetnm(srcvec) - 1);
  PRINTF1("srcvec has %d items\n",(int)(maxind + 1));


  psrc = (int32 *)vgetpm(srcvec);        /* pointer to srcvec  */


  for(step = 0;step <= 1; step++) {
    PRINTF1("Durchlauf %d\n",step);
    nitem = 0;
    if(iabort()) break;                  /* check CNTRL C */
    data = psrc[0];
    lindex  = 0;
    lflanke = 0;
    /* lflanke = (mode==1)  ?  1 : 2; *//* simulate first slope:  no! AM */
    for(i = 1; i <= maxind; i++) {
      if (i <= maxind)  {
        ldata = data;
        data = psrc[i];
        flanke = 0;
        if(data > ldata) {
          index = i;
          flanke = 1;
        } else if(data < ldata) {
          index = i - 1;
          flanke = 2;
        }
      }
      /*          */
      /*      else {                          */     /* simulate last slope */
      /*         if (mode == 0)  {    */
      /*            index  = i;   */
      /*            flanke = 1;   */
      /*         } else if (mode == 1)  { */
      /*            index  = i - 1;   */
      /*            flanke = 2;   */
      /*         }        */
      /*      }                    AM: vminmax must not find global min/max !! */

      PRINTF2("fl(%d)=%d: ",i,flanke);
      if( flanke > 0) {
        PRINTF3("slopetype %d at index %d, lastslope=%d\n",flanke,index,lflanke);
        if( (lflanke==1 && flanke==2 && mode==1) ||
          (lflanke==2 && flanke==1 && mode==0)    ) {
          if (!(lindex == 0 && index >= maxind)) {  /* no equal values */
            eindex = (index + lindex) >> 1;       /* extremum index */
            PRINTF3("Minimum/Maximum detected bei %d (%d, %d)\n",eindex,index,lindex);
            nitem ++;
            if(step != 0 ) {
              *perg++ = eindex;
            }
          }
        }
        lindex = index;
        lflanke = flanke;
      }
    }

    PRINTF1("Number of Minima/Maxima %ld\n",nitem);
    if(step == 0 ) {
      if (nitem == 0L){
        vputnm(desvec,0L);
        return(0);     /* no extrema, no error */
      }
      if (vgetln(desvec) < nitem){
        if(ve_alloc(desvec,nitem,(int16)sizeof(int16),TY_INT16)) return(-16);
      }
      perg = (int16 *)vgetpm(desvec);    /* pointer to resultvector */
    } else {
      vputnm(desvec,nitem);
    }
  }
  perg = (int16 *)vgetpm(desvec);    /* pointer to result vector */
  psrc = (int32 *)vgetpm(srcvec);        /* pointer to srcvec  */

  if(fang > 0) {                     /* check mdist  */
    int startindex = 0;              /* Start Suche bei Extrema Suche   */

    for(;;) {
      lindex = 0;
      step = -1;                     /* Flag fuer 1 Gefundene Max/Minim */
      exchange = FALSE;              /* No nichts zu Testen ?           */
      PRINTF1("Startindex %d\n",startindex);
      for(index = startindex; index < nitem; index++) {
        if(perg[index] < 0) {               /* Wurde schon bearbeited  */
          if(step < 0) continue;            /* Noch Suche erstes Datum */
          else break;                       /* Anfang schon getested ? */
        }
        if(step < 0) {                      /* Erstes Gefunden Extrema */
          step = 0;
          data = psrc[perg[index]];
          startindex = index;               /* Erstes Datum bei Suche  */
          lindex = index;
        } else {
          exchange = TRUE;                   /* Es ist etwas zu Testen */
          if(mode > 0) {                     /* Maximum Suche */
            if(psrc[perg[index]] > data) {   /* Merke Bestes Maximum */
              data = psrc[perg[index]];
              lindex = index;
            }
          } else {                           /* Minimum Suche */
            if(psrc[perg[index]] < data) {   /* Merke Bestes Minimum */
              data = psrc[perg[index]];
              lindex = index;
            }
          }
        }
      }
      PRINTF5("Startidx %d, lidx %d, idx %d, exchange %d,step = %d\n",
        startindex,lindex,index,exchange,step);
      if(step < 0) break;                    /* Alles Abgearbeited ? */

      if(exchange) {                         /* Ist etwas zu Testen ? */
        for(index = lindex + 1; index < nitem; index++) { /* Test fang rechts*/
          if(perg[index] < 0) break;       /* Ausserhalb von Testbereich*/
          step = perg[lindex] - perg[index];
          if(step < 0) step = - step;
          if(step > fang) break;           /* Ausserhalb von Fangbereich  */
          perg[index] = - maxind - 3;      /* Entferne innerhalb Fangbereich */
        }
        for(index = lindex - 1; index >= 0; index--) {
          if(perg[index] < 0) break;       /* Ausserhalb von Testbereich  */
          step = perg[lindex] - perg[index];
          if(step < 0) step = - step;
          if(step > fang) break;           /* Ausserhalb von Fangbereich  */
          perg[index] = - maxind - 3;      /* Entferne innerhalb Fangbereich */
        }
      }
      perg[lindex] = - perg[lindex] - 1;   /* Markiere als Bearbeited       */
    }
    step = 0;                              /* Kopiere uebrige Eintraege */
    for(index = 0; index < nitem; index++) {
      if(perg[index] == - maxind - 3) continue; /* wurde entfernt  */
      if(perg[index] < 0)                  /* Als bearbeited Markiert ? */
        perg[index] = - perg[index] -1;    /* Korriegiere Bearbeitungsflag */
      perg[step] = perg[index];            /* Kopiere Eintrag */
      step++;
    }
    PRINTF1("Fangbereichsueberpruefung, %d Eintraege entfernt\n",(int)(nitem-step));
    nitem = step;
  }

  if( nitem > maxe) {                /* Max 'maxe' extrema     */

    PRINTF0("Search maximum number of extrema\n");

    if (maxe == 1)  {
      PRINTF1("Search only global extremum, nitem=%d\n",nitem);
      data = psrc[perg[0]];
      for(index = 1; index < nitem; index++) {
        if (  (mode == 0 && psrc[perg[index]] < data) ||
          (mode == 1 && psrc[perg[index]] > data)     )  {
          perg[0] = perg[index];
          data = psrc[perg[index]];
        }
      }
    } else {
      do   {                              /* Bubble sort by values */
        if(iabort()) break;                  /* check CNTRL C */
        exchange = FALSE;
        for(i = 0; i < nitem - 1; i++) {
          if((mode == 0 && psrc[perg[i+1]] < psrc[perg[i]] ) ||
            (mode == 1 && psrc[perg[i+1]] > psrc[perg[i]] ) ) {
            temp = perg[i];
            perg[i] = perg[i+1];
            perg[i+1] = temp;
            exchange = TRUE;
          }
        }
      } while(exchange);
      do {                               /* Bubble sort by index */
        if(iabort()) break;                  /* check CNTRL C */
        exchange = FALSE;
        for(i = 0; i < maxe - 1; i++) {
          if(perg[i] > perg[i+1] ) {
            temp = perg[i];
            perg[i] = perg[i+1];
            perg[i+1] = temp;
            exchange = TRUE;
          }
        }
      } while(exchange);
    }
    nitem = maxe;
  }
  vputnm(desvec,nitem);

  return(0);
}

/***************************************************************************
* vmmsort2
*
* sort extremas according value
*
****************************************************************************
*/

int vmmsort2( Tvector *srcvec,  // Source vector ( vector type: int32 )
              Tvector *resvec,  // Destination/result vector ( vector type: int16 )
              int mode,         // Mode: 0 = min, 1 = max
              int sortmode)     // Sort mode: 0: according value, 1: left to right, 2: right to left
{
  int32 *psrc;            /* pointer to srcvec data      */
  int i ;
  int32 nitem;                     /* number of elements in result vector */
  int16 *perg;                     /* pointer to resvec data      */
  boolean exchange;
  int16 temp;

  if( utvcheck( srcvec,DV_HOST,TY_INT32)) return(-121);
  if( utvcheck( resvec,DV_HOST,TY_INT16)) return(-123);

  if( vgetnm(resvec) < 2 ) {          /* check number of data */
    return(0); /* done */
  }

  nitem = vgetnm(resvec);
  psrc = (int32 *)vgetpm(srcvec);        /* pointer to srcvec  */
  perg = (int16 *)vgetpm(resvec);        /* pointer to resvec  */

  switch(sortmode) {
  case 0:
    do   {                              /* Bubble sort by values */
      if(iabort()) break;                  /* check CNTRL C */
      exchange = FALSE;
      for(i = 0; i < nitem - 1; i++) {
        if((mode == 0 && psrc[perg[i+1]] < psrc[perg[i]] ) ||
           (mode == 1 && psrc[perg[i+1]] > psrc[perg[i]] ) ) {
          temp = perg[i];
          perg[i] = perg[i+1];
          perg[i+1] = temp;
          exchange = TRUE;
        }
      }
    } while(exchange);
    break;
  case 1:
    do   {                              /* Bubble sort by index left to right */
      if(iabort()) break;                  /* check CNTRL C */
      exchange = FALSE;
      for(i = 0; i < nitem - 1; i++) {
        if(perg[i+1] < perg[i]) {
          temp = perg[i];
          perg[i] = perg[i+1];
          perg[i+1] = temp;
          exchange = TRUE;
        }
      }
    } while(exchange);
    break;
  case 2:
    do   {                              /* Bubble sort by index left to right */
      if(iabort()) break;                  /* check CNTRL C */
      exchange = FALSE;
      for(i = 0; i < nitem - 1; i++) {
        if(perg[i+1] > perg[i]) {
          temp = perg[i];
          perg[i] = perg[i+1];
          perg[i+1] = temp;
          exchange = TRUE;
        }
      }
    } while(exchange);
    break;
  }

  return(0);
}

/***************************************************************************
* vmmsort
*
* sort extremas according value
*
****************************************************************************
*/

int vmmsort2( Tvector *srcvec,  // Source vector ( vector type: int32 )
              Tvector *resvec,  // Destination/result vector ( vector type: int16 )
              int mode)         // Mode: 0 = min, 1 = max
{
  return( vmmsort2( srcvec ,resvec, mode, 0));
}

/******************************** E O F *****************************/
