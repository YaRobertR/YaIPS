/****************************************************************************

  YaIPS_IPS_Utils.cpp

  IPS (Image Processing Software) interface code.
  Some clue code to interface IPS (Image Processing Software) code.

 26.02.2025 RR: First edition of this file.

*****************************************************************************
*/

#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
//x/#include <largeint.h>
#include <math.h>
#include <time.h>

// ...

#include "YaIPS_IPS_Interface.h"
#include "YaIPS_LanguageStrings.h"  // Language string definitions

/***************************************************************************
* Used for error handling
*
* Is set on (most) error return of ips functions.
* Reset to NULL before calling and test after call.
****************************************************************************
*/

char *errstring;         // global variable points to error message
char errbuffer[ 256];    // global variable, buffer for error string

/***************************************************************************
*
* get current date and time
*
* Example: get date and time
*
*  char DateBuf[80], TimeBuf[80];
*  ipsdateEx( DateBuf, sizeof( DateBuf), 'd', "dd'.'MM'.'yyyy");
*  ipsdateEx( TimeBuf, sizeof( TimeBuf), 't', "HH':'mm':'ss");
****************************************************************************
*/

int ipsdateEx( char *pResBuf, int SizeOfResbuf, int mode, char *pFormat)
{
  int PrimaryLanguageId = LANG_NEUTRAL;
  int SubLanguageId = SUBLANG_DEFAULT;
  LCID   LanguageId, Locale;

  LanguageId = MAKELANGID( PrimaryLanguageId, SubLanguageId);
  Locale = MAKELCID( LanguageId, SORT_DEFAULT);

  if (mode == 'd') {                /* store date */
    if( GetDateFormat( Locale, 0, NULL, pFormat,
                       pResBuf, SizeOfResbuf) == 0) {
      strcpy( pResBuf, "Error 'GetDateFormat'");
    }
  } else {                          /* store time */
    if( GetTimeFormat( Locale,
                       TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER, NULL, pFormat,
                       pResBuf, SizeOfResbuf) == 0) {
      strcpy( pResBuf, "Error 'GetTimeFormat'");
    }
  }
  
  return(0);
}

/***************************************************************************
*
* Collection of data conversion routines
*
*    Avoids overflow exception and system crash.
*    Clips values to MAXINT16/32 range.
*       if(inval >=0) inval  += 0.5;            Rounding
*       else          inval  -= 0.5;
*       if(inval > MAXINT) return(MAXINT)
*       if(inval < -MAXINT) return(-MAXINT)
*       else                return((int)inval));
*    Rounded conversion !!
****************************************************************************
*/

int16 dto16( lfloat x)

{
 if((x) < 0) (x) -= 0.5; else (x) += 0.5;
 if ( x >= (double)MAXINT16)  return ((int16)MAXINT16);
 if ( x <= -(double)MAXINT16) return ((int16)-MAXINT16);
 else return ( (int16) x);
}

int16 fto16( sfloat x)
{
 if((x) < 0) (x) -= 0.5; else (x) += 0.5;
 if ( x >= (float)MAXINT16)  return ((int16)MAXINT16);
 if ( x <= -(float)MAXINT16) return ((int16)-MAXINT16);
 else return ( (int16) x );
}

int32 dto32( lfloat x)
{
 if((x) < 0) (x) -= 0.5; else (x) += 0.5;
 if ( x >= (double)MAXINT32)  return ((int32)MAXINT32);
 if ( x <= -(double)MAXINT32) return ((int32)-MAXINT32);
 else return ( (int32) x );
}

int32 fto32( sfloat x)
{
 if((x) < 0) (x) -= 0.5; else (x) += 0.5;
 if ( x >= (float)MAXINT32)  return ((int32)MAXINT32);
 if ( x <= -(float)MAXINT32) return ((int32)-MAXINT32);
 else return ( (int32) x );
}

/***************************************************************************
*
* Some common used subroutines
*
****************************************************************************
*/

/*-------------------------------------------------------------------------
*  int uticheck( Timages *pim, int16 dev, int16 typ)
*
*     Returns 1 if the "datatyp" or the "device" do
*     NOT (!) match the datatyp and device of the image.
*     The datatype must be one of the standard types,
*     or standard types ored together. If types are ored,
*     the check is ok if the image's type is equal to one of them.
*     If any of both types is TY_ANY, the match is ok.
*/

int uticheck( Timages *pim, int16 dev, int16 typ) {
  if (pim == IMNULL) {

    errstring = ERR_IPS_IMG_PTR_NULL;
    return (-10);
  }

  //x/PRINTF4("uticheck: idev %x dev %x   ityp %x typ %x \n", pim->imdev, dev, pim->imtyp, typ);

  /* datatype must be one of the standard types.
   It must match exactly the type stored
   in the image description (equal operator).
   If any of the type is TY_ANY, the match is ok. */

  if ((typ != TY_ANY ) && (pim->imtyp != TY_ANY )) {
    if ((pim->imtyp & TY_STRUCT ) || (typ & TY_STRUCT )) {
      errstring = (char *)"structure types not allowed for images";
      return (-12);
    }

#ifdef oldversion
    if(pim->imtyp != typ) {
      errstring = ERR_IPS_IMG_BAD_TYPE;
      return(-13);
    }
#endif
    /* checking the bits instead of comparing equality is only
     possible because no TY_STRUCT are allowed, and standard
     datatypes are defined as bits. Also only one of the
     operands may have more than one bit set. Here there is no
     danger, because images can be of one single datatype only
     This new feature allows to or dataypes in uticheck:
     uticheck(dst......,TY_BYTE | TY_INT16)
     P.S  */
    if (!(pim->imtyp & typ)) { /* test the bit */
      errstring = ERR_IPS_IMG_BAD_TYPE;
      return (-13);
    }
  }
  return (0);
}

/*-------------------------------------------------------------------------
* return min xx-size of 2 images
*/

int utxx2min( Timages *im1, Timages *im2)
{
  return (getxx(im1) < getxx(im2) ? getxx(im1) : getxx(im2) );
}

/*-------------------------------------------------------------------------
* return min yy-size of 2 images
*/

int utyy2min( Timages *im1, Timages *im2)
{
  return (getyy(im1) < getyy(im2) ? getyy(im1) : getyy(im2) );
}

/*-------------------------------------------------------------------------
* return min xx-size of 2 images
*/

int utxx3min(Timages *im1, Timages *im2, Timages *im3)
{
  int mmm;

  mmm = getxx(im1);
  if (getxx(im2) < mmm) mmm = getxx(im2);
  if (getxx(im3) < mmm) mmm = getxx(im3);
  return(mmm);
}

/*-------------------------------------------------------------------------
* return min yy-size of 3 images
*/

int utyy3min( Timages *im1, Timages *im2, Timages *im3)
{
  int mmm;

  mmm = getyy(im1);
  if (getyy(im2) < mmm) mmm = getyy(im2);
  if (getyy(im3) < mmm) mmm = getyy(im3);
  return(mmm);
}

/*--------------------------------------------------------------------
  convert data type ID (TY_xxx defines) to sizeof()

  return:    0 error
          >= 1 size in bytes.
*/

int cvsize( int16 typ)
{
  switch(typ) {
    case TY_BYTE:   return(sizeof(int8)  );
    case TY_INT16:  return(sizeof(int16) );
    case TY_INT32:  return(sizeof(int32) );
    case TY_SFLOAT: return(sizeof(sfloat));
    case TY_LFLOAT: return(sizeof(lfloat));
    default:        errstring = ERR_IPS_DATA_TYPE_NO_STD;
        return(0);
  }
}

/*--------------------------------------------------------------------
check if new coordinates are  inside memory of image
NOTE:  the original version assumed that all root images
have origin equal zero. This assumption could be made
because it is part of the general IPS philosophy that
root images describe physical memory, which does not have
an offset to something else (offset = distance from ground
or what ??).
The corrected version is inefficient, because it has to
pass the whole image tree during each call to get the origin
of the real root image. This is due to the @@#$%%^& ITEX extension
of the image_create function with offsets for  the origin.
*/
int im_inside( Timages *pim, int32 xnew, int32 ynew, int32 xsiz, int32 ysiz)
 {
  int ierr = 0;
  Timages *root;
  int32 xend, yend;

  if (pim == IMNULL) {                    // Security test

    errstring = ERR_IPS_IMG_PTR_NULL;

    return (-12);
  }

  root = pim; /* search up the tree for the final root image */

#ifdef use_again
  while (root->imparen != IMNULL)
    root = root->imparen;
#endif

  /* now check origin and size against the root */
  xend = (int32) (root->imxm + root->imxa);
  yend = (int32) (root->imym + root->imya);
  if ((xnew < (int32) (root->imxa)) || (xnew > xend))
    ierr = -1;
  else if ((ynew < (int32) (root->imya)) || (ynew > yend))
    ierr = -2;
  else if ((xsiz <= 0) || (xnew + xsiz > xend))
    ierr = -3;
  else if ((ysiz <= 0) || (ynew + ysiz > yend))
    ierr = -4;
  if( ierr)
    errstring = ERR_IPS_IMG_NOT_INSIDE_MEM;

  return (ierr);
}

/*--------------------------------------------------------------------
  Create an image using optional allocated memory

  pim    Pointer to image data envelop.
         If NULL allocate new memory else reuse memory.
  pMem   Pointer to memory for image data.
         If NULL allocate new memory use as image memory
*/

Timages *im_ucreateMem( Timages *pim, int16 typ, int16 dev, unsigned int16 xsiz, unsigned int16 ysiz, void *pMem)

{
  int16 datsiz;
  int32 mem;

  // Get bytes per pixel
  
  if( (datsiz = cvsize( typ)) <= 0) {           // Security test

    errstring = ERR_IPS_DATA_TYPE_ILLEGAL;

    return( IMNULL);  /* illegal data type */
  }

  // Optional header allocation

  if( pim == IMNULL) {                          // Have no image data envelope

    // Allocate image data envelope
    
    pim = (Timages *)malloc( sizeof( Timages));
    
    if( pim == IMNULL) {                        // Out of memory

      errstring = ERR_IPS_NO_MEM; // (char *)"im_ucreateMem: out of memory for image header";

      return(IMNULL);
    }

    memset( pim, 0, sizeof( Timages));          // zero all

    pim->Flags |= IPS_IMG_FLAG_ALLOC_HEADER;    // Set flag, header allocated

  } else {

    memset( pim, 0, sizeof( Timages));          // zero all

    pim->Flags &= ~ IPS_IMG_FLAG_ALLOC_HEADER;  // Reset flag, header allocated
  }
  
  // Optinal data allocation
  
  if( pMem == NULL) {                          // Have no point to image memory

    mem = (int32)xsiz * (int32)ysiz * (int32)datsiz;

    if( mem <= 0L) {

      errstring = ERR_IPS_MEM_BAD_SIZE; // (char *)"im_ucreateMem: bad sizes for memory alloc";

      if( pim->Flags &IPS_IMG_FLAG_ALLOC_HEADER) { // Was header allocated

        free( pim);                            // Release header envelop
      }

      return( IMNULL);
    }

    pMem = malloc( mem);

    if( pMem == NULL) {

      errstring = ERR_IPS_NO_MEM; //(char *)"im_ucreateMem: out of memory for image memory";

      if( pim->Flags & IPS_IMG_FLAG_ALLOC_HEADER) { // Was header allocated

        free( pim);                            // Release header envelop
      }

      return( IMNULL);
    }

    pim->Flags |= IPS_IMG_FLAG_ALLOC_MEMORY;    // Set flag, image memory allocated

  } else {

    pim->Flags &= ~ IPS_IMG_FLAG_ALLOC_MEMORY;  // Reset flag, image memory allocated
  }

  pim->imdat = (char *)pMem;        /* init pointer to DATA */
  pim->imxm = xsiz;
  pim->imym = ysiz;
  pim->imtyp = typ;

  pim->imxa = 0;                    /* image part */
  pim->imya = 0;
  pim->imxx = xsiz;
  pim->imyy = ysiz;

  return( pim);
}

/*--------------------------------------------------------------------
  Define an image AOI on an existing image

  pim     Pointer to image data envelop.
          If NULL allocate new memory else reuse memory.
  parent  Define AOI on this image.
*/

Timages *im_udefineMem( Timages *pim, Timages *parent, unsigned int16 xrel, unsigned int16 yrel, unsigned int16 xsiz, unsigned int16 ysiz)

{
  int32 xnew, ynew;

  // Get bytes per pixel

  if( parent == IMNULL) {                       // Security test

    errstring = ERR_IPS_IMG_PTR_NULL;

    return( IMNULL);  /* illegal data type */
  }

  // Optional header allocation

  if( pim == IMNULL) {                          // Have no image data envelope

    // Allocate image data envelope

    pim = (Timages *)malloc( sizeof( Timages));

    if( pim == IMNULL) {                        // Out of memory

      errstring = ERR_IPS_NO_MEM; // (char *)"im_udefineMem: out of memory for image header";

      return(IMNULL);
    }

    memset( pim, 0, sizeof( Timages));          // zero all

    pim->Flags |= IPS_IMG_FLAG_ALLOC_HEADER;    // Set flag, header allocated

  } else {

    memset( pim, 0, sizeof( Timages));          // zero all

    pim->Flags &= ~ IPS_IMG_FLAG_ALLOC_HEADER;  // Reset flag, header allocated
  }

  xnew = (int32)parent->imxa + (int32)xrel;
  ynew = (int32)parent->imya + (int32)yrel;

  if( im_inside( parent, xnew, ynew, xsiz, ysiz)) {

    errstring = ERR_IPS_IMG_NOT_INSIDE_MEM; // (char *)"im_ucreateMem: AOI don't fit";

    return(IMNULL);
  }


  pim->imdat = parent->imdat;          /* inherit memory */
  pim->imxm  = parent->imxm;
  pim->imym  = parent->imym;
  pim->imtyp = parent->imtyp;

  pim->imxa  = xnew;                   /* set image description */
  pim->imya  = ynew;
  pim->imxx  = xsiz;
  pim->imyy  = ysiz;

  return( pim);
}

/*--------------------------------------------------------------------
  Release memory which was allocated for this image
*/
int im_removeMem( Timages *pim)
{

  if( pim == IMNULL) {

    errstring = ERR_IPS_IMG_PTR_NULL;
    return(-12);
  }

  if( pim->Flags &IPS_IMG_FLAG_ALLOC_MEMORY) { // Was image memory allocated

    free( pim->imdat);                         // Release image memory
  }

  if( pim->Flags &IPS_IMG_FLAG_ALLOC_HEADER) { // Was header allocated

    free( pim);                                // Release header envelop
  }

  return( 0);
}

/*--------------------------------------------------------------------
  Release memory which was allocated for this image.
*/
int im_remove( Timages *pim)
{
  int ierr;

  ierr = im_removeMem( pim);

  return( ierr);
}

/*--------------------------------------------------------------------
  Release memory which was allocated for this image. Use pointer to image pointer.
*/
int im_remove( Timages **ppim)
{
  Timages *pim;
  int ierr;

  if( ppim == NULL) {     // Security test, pointer to pointer to image

    errstring = ERR_IPS_IMG_PTR_NULL;
    return(-12);
  }

  pim = *ppim;           // Get pointer to image

  if( pim == NULL) {     // Pointer is zero

    return( 0);          // Nothing to do, return OK
  }

  ierr = im_removeMem( pim);

  *ppim = NULL;          // Reset pointer

  return( ierr);
}

/*--------------------------------------------------------------------
  Create an unnamed vector without memory. Reuse existing vector data or allocate new vector data.

  pve    Pointer to image data envelop.
         If NULL allocate new memory else reuse memory.
*/

Tvector *ve_ucreateMem( Tvector *pve, int16  dev, int32  type)
{

  //x/PRINTF1("ve_ucrea: %d \n",dev);

  if( (dev&DV_FBANY) ) {
     errstring = ERR_IPS_DEVICE_NOT_FOR_THIS_OP; return( VENULL );
  }

  //x/sipGlobalDataGetOwnership();          /* exclusive access of this thread do sip global data structure */

#ifdef use_again
  if((pve = ve_get()) == VENULL) {

    //x/sipGlobalDataReleaseOwnership();    /* release ownership of sip global data structure */

    return(VENULL);  /* out of space */
  }
#else

  if( pve == VENULL) {                     // Need to allocate memory

    pve = (Tvector *)malloc( sizeof( Tvector));

    if( pve == VENULL) {                   // Security test

      //x/sipGlobalDataReleaseOwnership();    /* release ownership of sip global data structure */

      return(VENULL);  /* out of space */
    }

    memset( pve, 0, sizeof( Tvector));          // zero all

    pve->Flags |= IPS_VEC_FLAG_ALLOC_HEADER;    // Set flag, header allocated

  } else {

    memset( pve, 0, sizeof( Tvector));          // zero all
  }
#endif

  pve->vedat = NULL;              /* init pointer to DATA */
  pve->vetyp  = (int16)TY_ANY;       /* still undefined  */
       /* type MUST be marked to reserve the
         description element !! */
  pve->vesizof = 0;
  pve->vedev  = (int16)dev;
  pve->veleng = (int32)0L;

  //x/sipGlobalDataReleaseOwnership();    /* release ownership of sip global data structure */

  return( pve);
}

/*--------------------------------------------------------------------
  Preset a vector data element with existing memory.
  NO memory is allocated for this. NO ve_remove() is needed.

  pve    Pointer to image data envelop.
         If NULL allocate new memory else reuse memory.

  return:   0 OK
          < 0 Error
*/

int ve_Mem2Vec( Tvector *pve, int16  dev,
                anypnt  vedat,          /* base address to DATA */
                int32    nitems,        /* number of items */
                int16 sizofitem,        /* size of one item in bytes */
                int16       typ)        /* type as defined in sipve.h */
{

  //x/PRINTF1("ve_ucrea: %d \n",dev);

  if( (dev&DV_FBANY) ) {
     errstring = ERR_IPS_DEVICE_NOT_FOR_THIS_OP; return( -1);
  }

  memset( pve, 0, sizeof( Tvector));          // zero all

  // ...

  pve->vedat  = vedat;              /* init pointer to DATA */
  pve->vetyp  = (int16)typ;         /* set vector type */
                                    /* type MUST be marked to reserve the
                                       description element !! */
  pve->vesizof = (int16)sizofitem;
  pve->vedev   = (int16)dev;
  pve->veleng  = (int32)nitems;
  pve->venumb  = (int32)nitems;

  return( 0);
}

/*--------------------------------------------------------------------
  Create an unnamed vector without memory.
*/

Tvector *ve_ucreate( int16 dev)
{
  Tvector *pve;

  pve = ve_ucreateMem( NULL, dev, TY_ANY);

  return(pve);
}

/*--------------------------------------------------------------------
  Allocate data space to a vector description
*/
int ve_alloc( Tvector *pve,           /* description*/
              int32    nitems,        /* number of items */
              int16 sizofitem,        /* size of one item in bytes */
              int16       typ)        /* type as defined in sipve.h */
{
   int32 mem, vmem;
   int32 rsize;                 /* rounded sizeof */
   anypnt  pdat;

   //x/PRINTF3("ve_alloc: nitems %ld size %d type 0x%x \n", nitems,sizofitem,typ);

   if(pve == VENULL) {errstring= ERR_IPS_VEC_PTR_NULL; return(-13);}

   if((pve->vetyp != TY_ANY) && (pve->vetyp != typ ))
   {
      errstring = ERR_IPS_DATA_TYPE_CONFLICT; return(-21);
   }
   /* the following vector components are new only in case
      the vector did not have any data space  connected before
      The type is checked to avoid updating of LENG and SIZOFITEM
      which could result in trailing bytes at the end, due to rounding
      problems
      The number of stored items is cleared, to prevent anybody from
      reusing previously stored items after a ve_alloc.
      To enlarge an existing vector use ve_realloc() --to be implemented--
   */
   pve->vetyp   = (int16)typ;
   pve->vesizof = (int16)sizofitem;
   pve->venumb  = (int32)0;

   /* Note on Alignment:
  The C-compiler should round up the number of bytes returned
  by the sizeof() operator so that no alignment problems
  can occur.
  This rounding is assumed here.
   */
   rsize = sizofitem;                   /* no rounding done */

   if(pve->vedev == DV_HOST) {          /* if memory in host: */

     mem = nitems *(long)rsize;   /* number of bytes */
     if(mem <= 0L) {
       errstring = ERR_IPS_MEM_BAD_SIZE; //(char *)"bad sizes for memory alloc"; return(-10);
     }

     /*if new memory request is larger
       than allocated memory: free old memory */

     vmem = pve->veleng * (long)rsize;  /* maybe zero */

     if(vmem < mem) {

       if( pve->Flags &IPS_VEC_FLAG_ALLOC_MEMORY) { // Was memory allocated

         if( pve->vedat != (anypnt)NULL) free(pve->vedat);
       }

       pdat = (anypnt)malloc( mem);

       if( pdat == NULL) {
         errstring = ERR_IPS_NO_MEM; //(char *)"out of memory"; return(-11);
       }

       pve->Flags |= IPS_VEC_FLAG_ALLOC_MEMORY;    // Set flag, memory allocated

       pve->vedat = pdat;              /* init pointer to DATA */
       pve->veleng  = (int32)nitems;
     }
   }

   return(0);
}

/*--------------------------------------------------------------------
  Reallocate additional data space to a vector description
  the previously stored data are NOT deleted.
*/
int ve_realloc( Tvector *pve,           /* description*/
                int32    nitems,        /* number of items */
                int16 sizofitem,        /* size of one item in bytes */
                int16       typ)        /* type as defined in sipve.h */
{
   char *pold;
   int32 nold,noldb;
   int ierr, DataWasAllocated;

   //x/PRINTF3("ve_realloc: nitems %ld size %d type 0x%x \n", nitems,sizofitem,typ);

   if(pve == VENULL) {errstring= ERR_IPS_VEC_PTR_NULL; return(-13);}

   if(pve->veleng >= nitems) return(0);  /* no need */
   if(pve->venumb <= (int32)0)
   {
      return( ve_alloc(pve,nitems,sizofitem,typ) );  /* was empty before */
   }

   DataWasAllocated = (pve->Flags &IPS_VEC_FLAG_ALLOC_MEMORY) != 0; // Was memory allocated

   pold = pve->vedat;           /* remember old data space address */
   nold  = pve->venumb;
   noldb = pve->venumb * pve->vesizof;  /* in bytes */
   pve->vedat = NULL;           /* a trick so I can use ve_alloc ..*/
   pve->veleng  = (int32)0;     /*   ..and free the old memory later myself*/
   pve->venumb  = (int32)0;
   ierr = ve_alloc(pve,nitems,sizofitem,typ);  /* alloc new memory */
   if(ierr < 0) {

     if( DataWasAllocated) {
       free(pold);
     }
     return(ierr);  /* vector descr.is reset */
   }

   memcpy( pve->vedat,pold, (int)noldb);

   if( DataWasAllocated) {
     free( pold);                  /* no free the old data space */
   }
   pve->venumb  = (int32)(nold);

   return(0);
}

/*--------------------------------------------------------------------
  Release memory which was allocated for this vector.
*/
int ve_remove( Tvector *pve)
{

   //x/PRINTF0("ve_rem: remove memory of: \n");
   if( pve == VENULL) {

     errstring = ERR_IPS_VEC_PTR_NULL;
     return(-13);
   }


#ifdef use_again     // 15.05.2025 RR: Not supported for YaIPS package
   if(pve->vehead  != (Thead *)NULL)
   {
         hd_put(pve->vehead);             /* put back header if any */
   }
#endif // use_again // 15.05.2025 RR: Not supported for YaIPS package

   if(pve->vedev == DV_HOST)                    /* free host memory */
   {
       if(pve->vedat != NULL) {                /* memory may not be allocated*/
         free(pve->vedat);
       }
   }

#ifdef use_again     // 15.05.2025 RR: Not supported for YaIPS package
   //x/ve_put(pve);                                 /* give back to table */
#else
  if( pve->Flags &IPS_VEC_FLAG_ALLOC_HEADER) { // Was header allocated

   free( pve);                                 // Release header envelop
 }
#endif

  return(0);
}

/*--------------------------------------------------------------------
  Release memory which was allocated for this vector. Use pointer to vector pointer.
*/
int ve_remove( Tvector **ppve)
{
  Tvector *pve;
  int ierr;

  if( ppve == NULL) {     // Security test, pointer to pointer to vector

    errstring = ERR_IPS_VEC_PTR_NULL;
    return(-13);
  }

  pve = *ppve;           // Get pointer to vector

  if( pve == NULL) {     // Pointer is zero

    return( 0);          // Nothing to do, return OK
  }

  ierr = ve_remove( pve);

  *ppve = NULL;          // Reset pointer

  return( ierr);
}

/*-------------------------------------------------------------------------
 Check device and data type of a vector

      Returns 1 if the "datatyp" or the "device" do
      NOT (!) match the datatyp and device of the vector.
      It must match exactly the type stored
      in the vector description (equal operator).
      NO oring of datatypes allowed here !
      If any of both types is TY_ANY, the match is ok.
*/

int utvcheck( Tvector *pve,   // Pointer to vector
              int16 dev,      // Check for this device
              int16 typ)      // check for this data type
{
  if(pve == VENULL)  {
    errstring = ERR_IPS_VEC_PTR_NULL;
    return(-20);
  }

  //x/PRINTF4("utvcheck: idev %x dev %x   ityp %x typ %x \n", pve->vedev,dev,pve->vetyp,typ);

  if(!(pve->vedev & dev)) {
    errstring = ERR_IPS_VEC_DEVICE_WRONG;
    return(-21);
  }

  /* datatype must match exactly the type stored
     in the vector description (equal operator).
     If any of the type is TY_ANY, the match is ok. */

  if((typ != TY_ANY) && (pve->vetyp != TY_ANY)) {

    if(pve->vetyp != typ) {
      errstring = ERR_IPS_VEC_BAD_TYPE;
      return(-22);
    }
  }

  return(0);
}

/***************************************************************************
* YaIPS_RGB_to_IPS
* Convert a RGB image to an IPS image.
*
* pDst         Out: Fill image descriptor with converted data.
*                   May create a new image or reuse memory from
*                   the source image.
* pSrc         In: Source image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_to_IPS( Timages *pDst,        // Out: Converted image descriptor
                      Fl_RGB_Image *pSrc)   // In: Source image
{
  void *pMem;
  int xx, yy, ld;

  pMem = (void *)pSrc->data()[ 0];;
  xx = pSrc->data_w();
  yy = pSrc->data_h();
  ld = pSrc->ld() ? pSrc->ld() : xx * pSrc->d();

  if( pSrc->d() == 1) {      // Is a BW image

    // Use existing memory, no memory allocation needed.
    im_ucreateMem( pDst, TY_BYTE, DV_HOST, xx, yy, pMem);

    pDst->imxm = ld;        // Ensure proper X jump

    return( 0);             // Return OK
  }

  // Can't convert to IPS image

  return( -1);
}

/******************************** End Of File ********************************/
