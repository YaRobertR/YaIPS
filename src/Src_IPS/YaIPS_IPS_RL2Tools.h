/* *********************************************************************
/@
/@ Short-title: include file for rltools.c
/@
/@ *************************************************************************/

#ifndef _RL2TOOLS_H_
#define _RL2TOOLS_H_

#include "YaIPS_IPS_Interface.h"   // 15.05.2025 RR: Need this for IPS defines

#define RL2TOOLS_MEMOVFL		-123	/* memory overflow */

#define RL2T_SUBPIX_FAC       16  /* supbixel factor */
#define RL2T_SUBPIX_FAC_2      8  /* supbixel factor half, used for rounding */
#define RL2T_SUBPIX_SHIFT      4  /* supbixel factor as shift */

struct Srl2desc {
  int16 rlxa;             /* first pixel of field */
  int16 rlxe;             /* last pixel inside field */
  int16 rlab;             /* contains label */
  int16 rlid;             /* index for turbo labler */
  int32 rlxaSubPix;       /* for RL2AREA_SUPBPIX, first subpixel of field */
  int32 rlxeSubPix;       /* for RL2AREA_SUPBPIX, last subpixel inside field */
};
typedef struct Srl2desc Trl2desc;

#define RL2T_NOLABEL 0x0000    /* Sign Bit !! negative if set ! */

struct Srl2meas {          /* measurement results */
  int32 area;
  int32 xgrav;
  int32 ygrav;
  int32 xmin;  
  int32 xmax;  
  int32 ymin;  
  int32 ymax;
};
typedef struct Srl2meas Trl2meas;

/* working/free list of rl2t_labmea */
typedef struct {
  int16 xmin; 
  int16 xmax;
  int16 ymin;
  int16 ymax;
#ifdef use_again
  int32 xgrav;
  int32 ygrav;
  int32 area;
#else
  // 22.11.2011 RR: use 64 bit integers to sum up.
  long long xgrav;
  long long ygrav;
  int32 area_Pixel;
  int32 area_SubPixel;
#endif
  int32 next; /* index of next element */
  int32 prev; /* index of previous element, only used in working list */
  int32 label;
} Trl2obj;

/* destination list of rl2t_labmea */
typedef struct {
#ifdef use_again
  int32 xmin; 
  int32 xmax;
  int32 ymin;
  int32 ymax;
  int32 xgrav;
  int32 ygrav;
  int32 area;
  int32 label;
#else
  // 16.05.2025 RR: Reduce size of this. Get space for 8 bytes.
  unsigned int16 xmin;
  unsigned int16 xmax;
  unsigned int16 ymin;
  unsigned int16 ymax;
  int32 xgrav;
  int32 ygrav;
  int32 area;
  int32 label;
  // 16.05.2025 RR: Use this 8 bytes for some extra data.
  int32 ExData1;
  int32 ExData2;
#endif
} Trl2objdst;

// rl2t_setup variables and defines

// Label overlap mode
#define RL2CONNECTIVITY_4 0
#define RL2CONNECTIVITY_8 1

// Flags for run length coding things
#define RL2VWRAPAROUND     0x0001   // handle image wrap around in Y
#define RL2ENTERLABELS     0x0002   // enter label ID's
#define RL2DONTSTORE       0x0004   // don't store the objects
#define RL2AREA_SUPBPIX    0x0008   // area is in subpixel
#define RL2NO_OBJ_REALLOC  0x0010   // rl2t_labmea() makes no object vector realloc, vector must be pre-allocated

// Allow external access to set variables
extern int rl2overlap;     // Label overlap mode
extern int rl2flags;       // Flags for run length coding things

/*****************************************************************************
* externals
*****************************************************************************/

extern int rl2t_setup( int overlap);
extern int rl2t_setup2( int overlap, int flags);
extern int rl2t_rdoverlap( int *overlap);
extern int rl2t_rdflags( int *flags);
extern int rl2t_codesf( Timages *src, Timages *dst, sfloat thres1, sfloat thres2, int16 mode);
extern int rl2t_code( Timages *src, Timages *dst, int16 thres, int16 mode);
extern int rl2t_codeRGB( Timages *src, Timages *dst, int16 rgb, int16 thres, int16 mode);
extern int rl2t_codeRGB2( Timages *src, Timages *dst, int ThresRLow, int ThresRHigh, int ThresGLow, int ThresGHigh, int ThresBLow, int ThresBHigh);
extern int rl2t_codeRGB3( Timages *src, Timages *dst, int MainColorNr, int MainThresLow, int MainThresHigh, int Radio1ThresLow,  int Radio1ThresHigh, int Radio2ThresLow, int Radio2ThresHigh);
extern int rl2t_decode( Timages *src, Timages *dst, int16 label, int16 bcol, int16 lcol);
extern int rl2t_tdecode( Timages *src, Timages *dst, int16 label, int16 bcol, int16 lcol);
extern int rl2t_label( Timages *dst, int16 xstart, int16 ystart,
                      int16 labmin, int16 labmax, int16 *bolab, int32 *bopix);
extern int rl2t_invert( Timages *src, Timages *dst, int16 label, int16 xmax);
extern int rl2t_ExractLabelRLCs( Timages *src, Timages *dst, int16 label, int32 ymin, int32 ymax);
extern int rl2t_meas( Timages *src, int16 label, int32 *area,
                     int16 *xgrav, int16 *ygrav, int16 *xmin, int16 *xmax, int16 *ymin, int16 *ymax);
extern int rl2t_meas_area( Timages *src, int16 label, int32 *area);
extern int rl2t_labmea( Timages *src_rlc, Tvector *dst_obj, Timages *work_obj, int32 minarea, int32 maxarea);
extern int rl2t_eros( Timages *src, Timages *dst, int16 kmode);
extern int rl2t_eros2( Timages *src, Timages *dst, int16 kmode, int16 xmax);

// Sum up RLCs in a given AOI. Is like colsum()/rowsum().
extern int rl2t_sumRLCs(                          // Sum up RLCs in a given AOI.
                  Timages *srcim,                 // src image (hold run length codes)
                  Tvector *dstvec,                // destination vector
                  int label,                      // 0 from all label, else from specific label
                  int xStart, int yStart,         // start of AOI
                  int xSize, int ySize,           // size of AOI
                  int XY_Mode,                    // XY_Mode 0 = rowsum (horizontal), else colsum (vertical)
                  int InvertFlag,                 // if != 0, invert the sum (is like negating the RLC's)
                  int xmax);                      // x-size of uncoded original

// Sum up length of RLCs inside a given AOI.
extern int rl2t_CountPixelsRLCs(
                  Timages *srcim,                 // src image (hold run length codes)
                  int *pCountPixels,              // OUTPUT: Sum of Pixel equivalent RLC'S in AOI
                  int label,                      // 0 from all label, else from specific label
                  int xStart, int yStart,         // start of AOI
                  int xSize, int ySize,           // size of AOI
                  int xmax);                      // x-size of uncoded original

// Erodes RLCs.
int rl2t_Erode( Timages **piRLC_src,  // Pointer to RLC image pointer. Src in, result out,
                Timages **piRLC_dst,  // Pointer to RLC image pointer. Use for temp processing.
                int nErode);          // # of erosion steps

// Erodes RLCs, add border.
int rl2t_Erode2( Timages **piRLC_src,  // Pointer to RLC image pointer. Src in, result out,
                 Timages **piRLC_dst,  // Pointer to RLC image pointer. Use for temp processing.
                 int nErode,           // # of erosion steps
                 int xmax);            // x-size of uncoded original image

// Dilate RLCs.
int rl2t_Dilate( Timages **piRLC_src,  // Pointer to RLC image pointer. Src in, result out,
                 Timages **piRLC_dst,  // Pointer to RLC image pointer. Use for temp processing.
                 int nDilate,          // # of dilate steps
                 int label,            // > 0: use specific object, <= 0: use all objects
                 int xmax);            // x-size of uncoded original image

// Dilate RLCs, add border.
int rl2t_Dilate2( Timages **piRLC_src,  // Pointer to RLC image pointer. Src in, result out,
                  Timages **piRLC_dst,  // Pointer to RLC image pointer. Use for temp processing.
                  int nDilate,          // # of dilate steps
                  int label,            // > 0: use specific object, <= 0: use all objects
                  int xmax);            // x-size of uncoded original image

// Remove RLC's of a specific object from a RLC image.
int rl2t_RemoveLabelRLCs( Timages *iRLC_src,   // Pointer to src RLC image
                          Trl2objdst *pObj);   // Remove RLC's for this object

// Keep RLCs of a specific object, remove RLCs from other objects
int rl2t_IsolateLabelRLC( Timages *iRLC_src,   // Pointer to src RLC image
                          Trl2objdst *pObj);   // Remove RLC's for this object

// Resets the labels in a run length coded image
int rl2t_ResetLabelRLCs( Timages *iRLC_src);  // Pointer to src RLC image

#endif /* _RL2TOOLS_H_ */

/********************* End Of File ***************************************/
