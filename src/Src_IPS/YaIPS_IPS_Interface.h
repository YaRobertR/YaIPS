/****************************************************************************

  YaIPS_IPS_Interface.h

 IPS (Image Processing Software) interface include file.
  Some clue code to interface IPS (Image Processing Software) code.

 26.02.2025 RR: First edition of this file.

*****************************************************************************
*/

#ifndef YAIPS_IPS_INTERFACE_H_
#define YAIPS_IPS_INTERFACE_H_

// Need this defines for use of Fl_RGB_Image.
#include <FL/Fl.H>
#include <FL/Fl_Image.H>

//--------------------------------------------------------------------------
//
// Support for test prints
//
//--------------------------------------------------------------------------

#if     TEST == 0               /* no test printouts */

#define PRINTF0(p0)
#define PRINTF1(p0,p1)
#define PRINTF2(p0,p1,p2)
#define PRINTF3(p0,p1,p2,p3)
#define PRINTF4(p0,p1,p2,p3,p4)
#define PRINTF5(p0,p1,p2,p3,p4,p5)

#define WAITINP()

/*-------------------------------------------------------------------------*/

#else   /* TEST0 */

 /* unconditioned test printouts */

#define PRINTF0(p0)                     printf (p0)
#define PRINTF1(p0,p1)                  printf (p0,p1)
#define PRINTF2(p0,p1,p2)               printf (p0,p1,p2)
#define PRINTF3(p0,p1,p2,p3)            printf (p0,p1,p2,p3)
#define PRINTF4(p0,p1,p2,p3,p4)         printf (p0,p1,p2,p3,p4)
#define PRINTF5(p0,p1,p2,p3,p4,p5)      printf (p0,p1,p2,p3,p4,p5)

#define WAITINP()                      (printf ("WAIT"), getchar ())

#endif

//--------------------------------------------------------------------------
//
// Simulate IPS environment
//
//--------------------------------------------------------------------------

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#define IMNULL  ((Timages *) 0)
#define VENULL  ((Tvector *) 0)

#define register                 // Replace the keyword register to nothing. This keyword make compiler problems.

#define int8   char              // NOTE 15.05.2025 RR: char is unsigned
#define uint8  unsigned char
#define int16  short
#define int32  long
#define uint32 unsigned long
#define sfloat float
#define lfloat double
//x/#define string char *
typedef char *anypnt;     /* may point to anything, least alignment
                                restrictions with char pointer */

#define btoi(byte)  ((int)(unsigned char)(byte))  /* conv. to (int) NOT (int16)*/
#define btol(byte)  ((long)((unsigned)byte))      /* conv. to (long) NOT (int32)*/
#define btou(byte) ((unsigned int)((unsigned)byte)) /* conv. to (unsigned int)*/

#define iabort()   (0)        // iabort() functions make nothing

#define T_WinDesc Fl_YaIPS_AOI_t   // Fl_YaIPS_AOI_t has same structure elements. T_WinDesc is used in some IPS functions.

/*
/@  DOKU
/@  Simages
/@      imtype   Data type (TY_xxxx), standard types only, no TY_STRUCT
/@      imboff   bit offset of image in FB, e.g:
/@                 int8-image created on 16bit-FB may have a bit offset
/@                 of 0 up to 8 bits.
/@      imdev    Device    (DV_xxxx)
/@              With the following 3 ptrs a tree structure is build
/@      parent
/@      brother  link images to a list
/@      child    link all childs to a linear list
/@
/@      header   backlink necessary for delete operations
/@               and name
*/

#define IPS_IMG_FLAG_ALLOC_HEADER  0x0001
#define IPS_IMG_FLAG_ALLOC_MEMORY  0x0002

struct Simages {                 /* DESCRIPTION of an image  */
				  /* memory description (redundant): */
  unsigned int16 Flags;     /* Flag bits: see IPS_IMG_FLAG_xxx defines */
	anypnt  imdat;            /* base address to DATA */
	unsigned int16 imxm;      /* memory x size        */
	unsigned int16 imym;      /* memory y size        */
	int16 imtyp;              /* data type            */
	//x/int16 imboff;             /* bit offset           */
				  /* image description:     */
	unsigned int16 imxa;      /* image x origin       */
	unsigned int16 imya;      /* image y origin       */
	unsigned int16 imxx;      /* image x size         */
	unsigned int16 imyy;      /* image y size         */
				  /* image link features :   */
	//x/struct Simages *imparen;         /* parent image         */
	//x/struct Simages *imbroth;         /* brother list         */
	//x/struct Simages *imchild;         /* child   list         */

	//x/struct Shead   *imhead;          /* back_link to header  */
};
typedef struct Simages Timages;

/*
/@  DOKU
/@  Svector
/@      vetyp    Data type (TY_xxxx  or TY_STRUCT | number)
/@      vedev    Device    (DV_xxxx)
/@
/@      vesizof         sizeof(item) in bytes
/@      veleng          Length of allocated data space in items.
/@      venumb          Number of stored Items. Is initialised to
/@                      zero after creation of the vector.
/@                      All routines which store data in the DATA area
/@                      should store the number of valid items with
/@                      vputnm(ve,nitme) at the end of the routine
/@                      In case of any error, they should set this
/@                      number to zero, to mark an empty/invalid vector.
/@
/@  The data type of special structures is defined by or-ing the
/@  TY_STRUCT bit with an unique number, e.g
/@      define  TY_MEAS  (TY_STRUCT | ( 2 << TY_STSHIFT))
/@  All bits in
/@  This special data type should be defined in sipve.h, but may
/@  remain private for special  applications.
*/

#define IPS_VEC_FLAG_ALLOC_HEADER  0x0001
#define IPS_VEC_FLAG_ALLOC_MEMORY  0x0002

struct Svector {                 /* DESCRIPTION of an vector */
  unsigned int16 Flags;     /* Flag bits: see IPS_VEC_FLAG_xxx defines */
          /* memory description (redundant): */
  anypnt  vedat;            /* base address to DATA */
  int16 vetyp;              /* data type            */
  int16 vedev;              /* device               */

  int16 vesizof;            /* sizeof(item)         */
  int32 veleng;             /* length of vector in items */
  int32 venumb;             /* number of items stored in DATA */
  //x/struct Shead   *vehead;   /* back_link to header  */
};
typedef struct Svector Tvector;

/*------------------------------------------------------------------
* Data type  definitions
*/
#define TY_UNDEF    (int16)0          /* MUST be zero = init after start*/
#define TY_ANY      (int16)(-1)       /* disable check */
              /* all standard data types: */
#define TY_BYTE     (int16)0x1        /* same as int8 */
#define TY_INT8     (int16)0x1        /* same as byte */
#define TY_INT16    (int16)0x2
#define TY_INT32    (int16)0x4
#define TY_SFLOAT   (int16)0x8
#define TY_LFLOAT   (int16)0x10
#define TY_STANDARD (int16)(TY_INT8 | TY_INT16 | TY_INT32 | TY_SFLOAT | TY_LFLOAT)

#define TY_IMPTR    (int16)0x20        /* Timages pointer */
#define TY_VEPTR    (int16)0x40        /* Tvector pointer */
               /* special types   */
#define TY_NIL      (int16)0x2000      /* type for NULL pointer */
#define TY_STRUCT   (int16)0x4000      /* this bit defines structure types*/
#define TY_STSHIFT  (int16)7           /* shift structure number above
            standard types  */
               /* macro for special structure types*/
#define TY_STRUCTDEF(number)  (TY_STRUCT | ((number) << TY_STSHIFT))

/*------------------------------------------------------------------
* Device definitions
*/

#define DV_NRMASK      (int16)0xF      /* mask for device number 0..15 */

#define DV_FBANY       (int16)0x4000   /* framebuffer device   */
#define DV_FBIML       (int16)0x8000   /* frame buffer device supported by IMLIB */

#define DV_HOST        (int16)0x20     /* general HOST bit  */

#define DV_FBW32       (int16)0x0040   /* general WINDOWS screen bit (8 or 24 bit) */

/*------------------------------------------------------------------
  macro definition for access of elements in an image structure
*/
#define getxa(image) (int)((image)->imxa)
#define getya(image) (int)((image)->imya)
#define getxx(image) (int)((image)->imxx)
#define getyy(image) (int)((image)->imyy)
#define getpm(image)      ((image)->imdat)
#define getxm(image) (int)((image)->imxm)
#define getym(image) (int)((image)->imym)
#define getyp(image) (int)((image)->imtyp)
//x/#define getbo(image) (int)((image)->imboff)

/*------------------------------------------------------------------
  macro definition for access of elements in an vector structure
*/
#define vgetpm(vector)      ((vector)->vedat)
#define vgetln(vector)      ((vector)->veleng)
#define vgetnm(vector)      ((vector)->venumb)
#define vgetyp(vector)      ((vector)->vetyp)
#define vgetsz(vector)      ((vector)->vesizof)
#define vgetdv(vector)      ((vector)->vedev)
#define vputnm(vector,num) (((vector)->venumb) = (int32)(num))

// ...

/*-------------------------------------------------------------------------*/
/*
/@ macro definition for pixel access in the host memory
/@     pixel(x,y,image)
/@     pixad(x,y,image)
/@     pixadt(x,y,image,type)
/@       int32    x,y;        address   of pixel
/@       Timages *image;      image
/@       'type' is the string of a data type  for casting,never a variable
/@ Note: because 'pixel', 'pixad' and 'pixadt'  are macros, their
/@      type varies with the data type of the image.
/@      Accordingly, the type of 'x' and 'y' is not strictly enforced
/@      and casted to (int32) for address calculation anyway.
/@ Example:   pdest = pixadt(1,8,destimage,(int8));
*/
#define pixel(x,y,image) (*((image)->imdat +                                 \
			   (image)->imxm * (int32) ((image)->imya + (y)) +   \
					       (image)->imxa + (x)))

#define pixad(x,y,image)  ((image)->imdat +                                 \
			   (image)->imxm * (int32) ((image)->imya + (y)) +  \
					       (image)->imxa + (x))

#define pixadt(x,y,image,dtype)  ((dtype *)((image)->imdat) +            \
			   (image)->imxm * (int32)((image)->imya + (y)) +  \
					       (image)->imxa + (x))

/*-------------------------------------------------------------------------*/
/* macro definition for output data clipping
   These macros clip any input data type to any output data type.
   Range of output:  0....255
*/
#define hclip(inval,outptr) {if((inval) > 255)     \
          *(outptr) = 255; else *(outptr) = (inval);}

#define lclip(inval,outptr) {if((inval) <   0)     \
          *(outptr) =   0; else *(outptr) = (inval);}

#define bclip(inval,outptr) {if((inval) > 255)     \
          *(outptr) = 255; else if((inval) < 0)   \
          *(outptr) =   0; else *(outptr) = (inval);}

#define hclip8(i32,d8)   {*(d8) = ((i32) > 255) ?  255 : (i32);}

#define bclip8(i32,d8)   { if((i32) > 255L) *(d8) = 255;  \
                           else if((i32) < 0L) *(d8) = 0; \
                           else *(d8) = (i32); }

#define hclip16(i32,d16) {*(d16) = ((i32) > MAXINT16) ?  MAXINT16 : (i32);}

#define bclip16(i32,d16) { if((i32) > MAXINT16) *(d16) = MAXINT16;  \
                           else if((i32) < -MAXINT16) *(d16) = -MAXINT16; \
                           else *(d16) = (i32); }

// Other things

#define INTERPOL1              (char *)"0"
#define INTERPOL3_R            (char *)"3r"

#define NOSUBSAMPLING          0
#define SUBSAMPLING            1

#define CR_INT0                      "0"        /* no subpixel precision */
#define CR_INT3                      "3"           /* subpixel precision */
#define CR_INT3D                     "3d"         /* interpolation modes */
#define CR_INT3_R                    "3r"
#define CR_INT4_SC                   "4sc"
#define CR_INT5                      "5"
#define CR_INT5D                     "5d"
#define CR_INT7                      "7"
#define CR_INT7D                     "7d"

/* FUNCVAL
   is the result of a function, returning values related to
   coordinates in an image.
   Usually the vector data are sorted with ffuncval as key.
   or only a subset of all coordinates stored.
   (otherwise a float image would be appropriate to hold
    the results)
*/
#define TY_FUNCVAL      TY_STRUCTDEF(2)
struct Sffuncval {
        int16  fxpos ;
        int16  fypos ;
        sfloat ffuncval ;
};
typedef struct Sffuncval Tffuncval ;

//--------------------------------------------------------------------------
//
// Externals YaIPS_IPS_Utils.cpp
//
//--------------------------------------------------------------------------

/***************************************************************************
* Used for error handling
*
* Is set on (most) error return of ips functions.
* Reset to NULL before calling and test after call.
****************************************************************************
*/

extern char *errstring;         // global variable points to error message
extern char errbuffer[];        // global variable, buffer for error string

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

int ipsdateEx( char *pResBuf, int SizeOfResbuf, int mode, char *pFormat);

/***************************************************************************
 *
 * Data conversion routines
 *
 ****************************************************************************
 */

int16 dto16( lfloat x);

int16 fto16( sfloat x);

int32 dto32( lfloat x);

int32 fto32( sfloat x);

/***************************************************************************
 *
 * Some common used subroutines
 *
 ****************************************************************************
 */

// check device and datatype of an image
int uticheck( Timages *pim, int16 dev, int16 typ);

// return min xx-size of 2 images
int utxx2min( Timages *im1, Timages *im2);

// return min yy-size of 2 images
int utyy2min( Timages *im1, Timages *im2);

// return min xx-size of 3 images
int utxx3min( Timages *im1, Timages *im2, Timages *im3);

// return min yy-size of 3 images
int utyy3min( Timages *im1, Timages *im2, Timages *im3);

// convert data type ID (TY_xxx defines) to sizeof()
int cvsize( int16 typ);

// Create an image using optional allocated memory
Timages *im_ucreateMem( Timages *pim, int16 typ, int16 dev, unsigned int16 xsiz, unsigned int16 ysiz, void *pMem);

// Define an image AOI on an existing image
Timages *im_udefineMem( Timages *pim, Timages *parent, unsigned int16 xrel, unsigned int16 yrel, unsigned int16 xsiz, unsigned int16 ysiz);

// Release memory which was allocated for this image
int im_removeMem( Timages *pim);
int im_remove( Timages *pim);
int im_remove( Timages **ppim);      // Use pointer to image pointer.

// Create an unnamed vector without memory. Reuse existing vector data or allocate new vector data.
Tvector *ve_ucreateMem( Tvector *pve, int16  dev, int32  type);

// Create an unnamed vector without memory.
Tvector *ve_ucreate( int16  dev);

// Allocate data space to a vector description
int ve_alloc( Tvector *pve,           /* description*/
              int32    nitems,        /* number of items */
              int16 sizofitem,        /* size of one item in bytes */
              int16       typ);       /* type as defined in sipve.h */

// Preset a vector data element with existing memory.
// NO memory is allocated for this. NO ve_remove() is needed.

int ve_Mem2Vec( Tvector *pve, int16  dev,
                anypnt  vedat,          /* base address to DATA */
                int32    nitems,        /* number of items */
                int16 sizofitem,        /* size of one item in bytes */
                int16       typ);       /* type as defined in sipve.h */

// Reallocate additional data space to a vector description
// the previously stored data are NOT deleted
int ve_realloc( Tvector *pve,           /* description*/
                int32    nitems,        /* number of items */
                int16 sizofitem,        /* size of one item in bytes */
                int16       typ);       /* type as defined in sipve.h */

// Release memory which was allocated for this vector.
int ve_remove( Tvector *pve);
int ve_remove( Tvector **ppve);        // Use pointer to vector pointer.

// Check device and data type of a vector
int utvcheck( Tvector *pve,   // Pointer to vector
              int16 dev,      // Check for this device
              int16 typ);     // check for this data type

// Convert a RGB image to an IPS image
int YaIPS_RGB_to_IPS( Timages *pDst,        // Out: Converted image descriptor
                      Fl_RGB_Image *pSrc);  // In: Source image

//
// YaIPS_IPS_Cdf_Qual2.cpp
//

int cdf2_testSetup( double w1lin, double w1sqrt, double w1sq, double w2lin, double w2sqrt, double w2sq,
                    double w3lin, double w3sqrt, double w3sq, double wtotal, double wcomp12); /* setup test weights */

int cdf2_txyqual( Timages *srcim, int16 x0, int16 y0,
                  int32 norma, int16 thres, int16 *retqual, int16 mode);

int cdf2_txyqual2( Timages *srcim, int16 x0, int16 y0,
                   int32 norma, int16 thres, int16 *retqual, int16 mode);

int cdf2_txqual( Timages *srcim, int16 x0, int16 y0,
                 int32 norma, int16 thres, int16 *retqual, int16 mode);

int cdf2_txqual2( Timages *srcim, int16 x0, int16 y0,
                  int32 norma, int16 thres, int16 *retqual, int16 mode);

int cdf2_tyqual( Timages *srcim, int16 x0, int16 y0,
                 int32 norma, int16 thres, int16 *retqual, int16 mode);

int cdf2_tyqual2( Timages *srcim, int16 x0, int16 y0,
                  int32 norma, int16 thres, int16 *retqual, int16 mode);

int cdf2_pqual2( Timages *srcim, int16 thres, int16 norm, int32 norma, double wr_inFactor);

int cdf2_pqual( Timages *srcim, int16 thres, int16 norm, int32 norma);

//
// YaIPS_IPS_Extpos.cpp
//

// extremum position with subpixel precision
int extpos( Timages  *image,
            int16   x0, int16 y0,        /* inaccurate location  */
            char    *mode,               /* interpolation method */
            sfloat  *xi, sfloat *yi);

// extr.pos. with subpixel prec.; vector version
int extposv( Timages *image,
             Tvector *vector,
             char    *mode,
             sfloat  *xi,
             sfloat  *yi);

//
// YaIPS_IPS_ImgFilt3.cpp
//

int ifilt3( Timages *src, Timages *dst, int exp, int offset,
            int c0, int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8);

//
// YaIPS_IPS_ImgFrame.cpp
//

int frame( Timages *dst, int wid, int val, int xx, int yy);

//
// YaIPS_IPS_Linequ.cpp
//

// Linear equations system solver
int linequ( double *sys,
            int n,
            double *vec,
            int m);

//
// YaIPS_IPS_RL2Tools.cpp
//

// Include runlength code interface
#include "YaIPS_IPS_RL2Tools.h"

//
// YaIPS_IPS_VecBithres.cpp
//

int bithres2( Tvector *histvec, int thresPerc, int minDist,
              int *retThres, int *retMax1 = NULL, int *retMax2 = NULL);

//
// YaIPS_IPS_VecCopy.cpp
//

// copy part of a vector into another vector
int vcopy( Tvector *vsrc, Tvector *vdst, int begin, int num, int mode);

//
// YaIPS_IPS_VecFilter.cpp
//

// General vector filter
int vfiltn( Tvector *src, Tvector *dst, int exp, int offset, int n, int16 *c);

// Lowpass vector filter
int vlowpa( Tvector *srcvec, Tvector *dstvec, int cnt);

//
// YaIPS_IPS_VecUparcal.cpp
//

// Number of vector items needed
#define IPS_VGEOEST_SRC_N   6  // srcvec1/2: minimum number points of type TY_INT32
#define IPS_VGEOEST_DST_N   6  // dstvec: n points of type TY_SFLOAT

int vgeoest( Tvector *srcvec1,  /* passpoints system 1                */
             Tvector *srcvec2,  /* passpoints system 2                */
             Tvector *dstvec,   /* resultvector                       */
             int32 rlimit,      /* relative limit of least square dev.*/
             int16 nmax,        /* maximal number of estimation passes*/
             int16 pmode,       /* printout mode                      */
             Tvector *v1t,      /* if (Tvector *)0, created inside    */
             Tvector *vtmp);    /* if (Tvector *)0, created inside    */

//
// YaIPS_IPS_VecFilter.cpp
//

// indices of maxima/minima (extrema) in vector
int vminmax( Tvector *srcvec,  // Source vector ( vector type: int32 )
             Tvector *desvec,  // Destination/result vector ( vector type: int16 )
             int mode,         // Mode: 0 = min, 1 = max
             int maxe,         // Max number of minimas/maximas
             int fang);        // minimum distance between minimas/maximas

// sort extremas according value
int vmmsort2( Tvector *srcvec,  // Source vector ( vector type: int32 )
              Tvector *resvec,  // Destination/result vector ( vector type: int16 )
              int mode,         // Mode: 0 = min, 1 = max
              int sortmode);    // Sort mode: 0: according value, 1: left to right, 2: right to left

// sort extremas according value
int vmmsort2( Tvector *srcvec,  // Source vector ( vector type: int32 )
              Tvector *resvec,  // Destination/result vector ( vector type: int16 )
              int mode);        // Mode: 0 = min, 1 = max

//
// YaIPS_IPS_VecUparcal.cpp
//

// Number of vector items needed
#define IPS_VUPARCAL_REF_N   8  // refvec: n points of type TY_INT32
#define IPS_VUPARCAL_TEST_N  4  // testvec: n points of type TY_INT32
#define IPS_VUPARCAL_DST_N   6  // dstvec: n points of type TY_SFLOAT

int vuparcal( Tvector *refvec,   /* passpoints ref system              */
              Tvector *testvec,  /* passpoints test system             */
              Tvector *dstvec,   /* resultvector                       */
              float cxty);       /* x to y pixel relation of camera    */

#endif /* YAIPS_IPS_INTERFACE_H_ */

/******************************** End Of File ********************************/
