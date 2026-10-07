/****************************************************************************

  YaIPS_RGB_Interface.h

  'Fl_RGB_Image' image processing. interface include file.
  Some clue code to interface RGB ('Fl_RGB_Image') code.

 11.04.2025 RR: First edition of this file.

*****************************************************************************
*/

#ifndef YAIPS_RGB_INTERFACE_H_
#define YAIPS_RGB_INTERFACE_H_

#include <FL/Fl.H>
#include <FL/Fl_Image.H>

#include "YaIPS.h"

//--------------------------------------------------------------------------
// Max file name is used from some functions.
// Ensure it is defined.
//--------------------------------------------------------------------------

#ifndef MAX_FILENAME_LEN
#define MAX_FILENAME_LEN 2048                       // length of filenames with path
#endif

/***************************************************************************
* Used for error handling
*
* Is set on some error return of RGB functions.
* Reset to NULL before calling and test after call.
****************************************************************************
*/

extern char *errstring;         // global variable points to error message
extern char errbuffer[];        // global variable, buffer for error string

//--------------------------------------------------------------------------
// Definitions and defines
//--------------------------------------------------------------------------

// RGB image data used for image processing
typedef struct {
  uchar *pD;      // Base data pointer
  int d;          // Number of bytes
  int ld;         // Line with in bytes
  int xx;         // With in pixels
  int yy;         // Height in pixel
} YaIPS_RGB_ImgD_t;

// Integer 2D coordinates
typedef struct {
  int x;          // With in pixels
  int y;          // Height in pixel
} YaIPS_XY_int;

// Float 2D coordinates
typedef struct {
  float x;        // With in pixels
  float y;        // Height in pixel
} YaIPS_XY_float;

//--------------------------------------------------------------------------
// Macro definition for pixel access to a YaIPS_RGB_ImgD_t image
//--------------------------------------------------------------------------

// Pixel address
#define RGB_pixad( x, y, pImgD)  ((pImgD)->pD + (pImgD)->ld * (y) + (pImgD)->d * (x))

#define RGB_pixadt( x, y, pImgD, dtype)  ((dtype *)((pImgD)->pD + (pImgD)->ld * (y) + (pImgD)->d * (x)))

// Clip and write output value via pointer
#define RGB_hclip( inval, outptr) {if((inval) > 255)     \
                   *(outptr) = 255; else *(outptr) = (inval);}

#define RGB_lclip( inval, outptr) {if((inval) <   0)     \
                   *(outptr) =   0; else *(outptr) = (inval);}

#define RGB_bclip( inval, outptr) {if((inval) > 255)     \
                   *(outptr) = 255; else if((inval) < 0)   \
                   *(outptr) =   0; else *(outptr) = (inval);}

// Type conversions
#define RGB_btoi(byte)  ((int)(unsigned char)(byte))  /* conv. to (int) NOT (int16)*/
#define RGB_btou(byte) ((unsigned int)((unsigned)byte)) /* conv. to (unsigned int)*/

// Stupid abs, min, max macros
#define RGB_abs( val)        ((val) > 0 ? (val) : -(val))
#define RGB_min( val1, val2) ((val1) < (val2) ? (val1) : (val2))
#define RGB_max( val1, val2) ((val1) > (val2) ? (val1) : (val2))

//--------------------------------------------------------------------------
// Externals YaIPS_RGB_Utils.cpp
//--------------------------------------------------------------------------

// Convert Fl_RGB_Image to image data used for RGB image processing.
int YaIPS_RGB_to_ImgD( Fl_RGB_Image *pRGB,       // Point to RGB image
                      YaIPS_RGB_ImgD_t *pImgD);  // Point to RGB image data

// Set size and bytes per pixel of a RGB image
int YaIPS_RGB_ImageSetSize( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                            int xx,                  // In: Width of image
                            int yy,                  // In: Height of image
                            int d);                  // In: Bytes per pixel (1..4)

// Ensure that pPDst image has the same size and same bytes per pixel
int YaIPS_RGB_EnsureSameSize( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                              Fl_RGB_Image *pSrc);     // In:  Source image

// Ensure that pPDst image has the minimum size of the source images. Sources must have the same bytes per pixel.
int YaIPS_RGB_EnsureMinSize2( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                              Fl_RGB_Image *pSrc1,     // In:  1. source image
                              Fl_RGB_Image *pSrc2);    // In:  2. ource image

// Returns the smaller xx-size of the 2 images
int YaIPS_RGB_xx2min( YaIPS_RGB_ImgD_t *pImgD1,  // Point to ImgD image
                      YaIPS_RGB_ImgD_t *pImgD2); // Point to ImgD image

// Returns the smaller xx-size of the 2 images
int YaIPS_RGB_yy2min( YaIPS_RGB_ImgD_t *pImgD1,  // Point to ImgD image
                      YaIPS_RGB_ImgD_t *pImgD2); // Point to ImgD image

// Returns the smaller xx-size of the 3 images
int YaIPS_RGB_xx3min( YaIPS_RGB_ImgD_t *pImgD1,  // Point to ImgD image
                      YaIPS_RGB_ImgD_t *pImgD2,  // Point to ImgD image
                      YaIPS_RGB_ImgD_t *pImgD3); // Point to ImgD image

// Returns the smaller xx-size of the 3 images
int YaIPS_RGB_yy3min( YaIPS_RGB_ImgD_t *pImgD1,  // Point to ImgD image
                      YaIPS_RGB_ImgD_t *pImgD2,  // Point to ImgD image
                      YaIPS_RGB_ImgD_t *pImgD3); // Point to ImgD image

// Set AOI of an imgD image descriptor
int YaIPS_ImgD_AOI( YaIPS_RGB_ImgD_t *pImgA,       // Out: AOI image
                    YaIPS_RGB_ImgD_t *pImgR,       // In: Root image
                    int x, int y, int xx, int yy); // In: AOI

// Set AOI of an imgD image descriptor
int YaIPS_ImgD_AOI( YaIPS_RGB_ImgD_t *pImgA,       // Out: AOI image
                    YaIPS_RGB_ImgD_t *pImgR,       // In: Root image
                    Fl_YaIPS_AOI_t *pAOI);         // In: AOI

// Convert FLTK color to integer value
int YaIPS_RGB_Color2Val( int nBytes,          // # bytes per pixel
                         Fl_Color Color,      // FLTK Color
                         int Alpha = 0xff);   // Alpha value

// Set all image pixels to value
int YaIPS_RGB_SetVal( YaIPS_RGB_ImgD_t *pImgD,  // Destination image
                      int value);               // value to set

// Set all image pixels to value
int YaIPS_RGB_SetVal( Fl_RGB_Image *pDst,      // Destination image
                      int value);              // value to set

// Defines for set value corner bits

#define YAIPS_SETVAL_CORNER_BIT_MASK 0x0f       // Corner bit all
#define YAIPS_SETVAL_CORNER_BIT_LT   0x08       // Corner bit left top
#define YAIPS_SETVAL_CORNER_BIT_RT   0x04       // Corner bit right top
#define YAIPS_SETVAL_CORNER_BIT_LB   0x02       // Corner bit left bottom
#define YAIPS_SETVAL_CORNER_BIT_RB   0x01       // Corner bit right bottom

// Fill image with an integer value for each image corner. Interpolate color between corners.
int YaIPS_RGB_SetVal( YaIPS_RGB_ImgD_t *pImgD,  // Destination image
                      int Value_LT,             // Value for left top image corner
                      int Value_RT,             // Value for right top image corner
                      int Value_LB,             // Value for left bottom image corner
                      int Value_RB,             // Value for right bottom image corner
                      int CornerBits);          // On bit for each corner to use

// Fill image with an integer value for each image corner. Interpolate color between corners.
int YaIPS_RGB_SetVal( Fl_RGB_Image *pDst,       // Destination image
                      int Value_LT,             // Value for left top image corner
                      int Value_RT,             // Value for right top image corner
                      int Value_LB,             // Value for left bottom image corner
                      int Value_RB,             // Value for right bottom image corner
                      int CornerBits);          // On bit for each corner to use

// Fill image with a color for each image corner. Interpolate color between corners.
int YaIPS_RGB_SetColor( YaIPS_RGB_ImgD_t *pImgD,  // Destination image
                        Fl_Color Color_LT,        // Color for left top image corner
                        Fl_Color Color_RT,        // Color for right top image corner
                        Fl_Color Color_LB,        // Color for left bottom image corner
                        Fl_Color Color_RB,        // Color for right bottom image corner
                        int CornerBits,           // On bit for each corner to use
                        int Alpha = 0xff);        // Optional alpha value

// Fill image with a color for each image corner. Interpolate color between corners.
int YaIPS_RGB_SetColor( Fl_RGB_Image *pDst,       // Destination image
                        Fl_Color Color_LT,        // Color for left top image corner
                        Fl_Color Color_RT,        // Color for right top image corner
                        Fl_Color Color_LB,        // Color for left bottom image corner
                        Fl_Color Color_RB,        // Color for right bottom image corner
                        int CornerBits,           // On bit for each corner to use
                        int Alpha = 0xff);        // Optional alpha value

// Set all image pixels alpha value
int YaIPS_RGB_SetAlpha( YaIPS_RGB_ImgD_t *pImgD,  // Destination image
                        int Alpha);               // Alpha value to set

// Set all image pixels alpha value
int YaIPS_RGB_SetAlpha( Fl_RGB_Image *pDst,      // Destination image
                        int Alpha);              // Alpha value to set

// Copy image
int YaIPS_RGB_CopyImg( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                       Fl_RGB_Image *pSrc);  // Source image

// Copy smaller image into bigger.
int YaIPS_RGB_CopyInImg( Fl_RGB_Image *pDst,   // Destination image must exist
                         Fl_RGB_Image *pSrc,   // Source image must exist
                         int OffX, int OffY);  // Offset in destination image

// Cut out a rectangular image part
int YaIPS_RGB_CutOut( Fl_RGB_Image **ppDst,      // Out: Pointer to pointer to RGB image
                      Fl_RGB_Image *pSrc,        // Source image
                      Fl_YaIPS_AOI_t *pSrcAOI);  // Cut out this image part

//--------------------------------------------------------------------------
// YaIPS_RGB_BoxNxN.cpp
//--------------------------------------------------------------------------

// N*N box filter, box lowpass filter

int YaIPS_RGB_BoxNxN( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                     Fl_RGB_Image *pSrc,   // Source image
                     int KernelSizeX,      // Kernel Size
                     int KernelSizeY);     // Kernel Size

//--------------------------------------------------------------------------
// YaIPS_RGB_Canny.cpp
//--------------------------------------------------------------------------

// Canny edge filter
int YaIPS_RGB_Canny( Fl_RGB_Image **ppDst, // Out: Destination (absolute amount) image
                    Fl_RGB_Image **ppDir, // Out: Direction image
                    Fl_RGB_Image *pSrc,   // Source image
                    float sigma,          // Sigma for gaussian
                    float ResMultArg,     // Result multiplier
                    int mode);            // If true, maxima elimination

//--------------------------------------------------------------------------
// YaIPS_RGB_Color.cpp
//--------------------------------------------------------------------------

// IHS conversion tables may be used elsewhere
extern uchar YaIPS_ColMod_atantab[256];
extern uchar YaIPS_ColMod_sintab[64];
extern uchar YaIPS_ColMod_costab[64];

// Convert color RGB to IHS
void YaIPS_RGB_Color_RGB2IHS( uchar *pIHS,    // Out: Pointer to three Bytes for IHS value
                              uchar *pRGB);   // In: Pointer to three bytes RGB value
// Convert color RGB to IHS
void YaIPS_RGB_Color_RGB2IHS( int *pI, int *pH, int *pS,    // Out: Converted IHS value
                              int R, int G, int B);         // In: RGB value, range 0 ... 255

// Convert color IHS to RGB
void YaIPS_RGB_Color_IHS2RGB( uchar *pRGB,    // Out: Pointer to three bytes RGB value
                              uchar *pIHS);   // In: Pointer to three Bytes for IHS value
// Convert color IHS to RGB
void YaIPS_RGB_Color_IHS2RGB( int *pR, int *pG, int *pB,    // Out: Converted RGB value
                              int I, int H, int S);         // In: IHS value, range 0 ... 255

//  Convert RGB image to IHS
int YaIPS_RGB_Color_RGB2IHS( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to IHS image
                             Fl_RGB_Image *pSrc);  // In: Pointer to RGB image

// Convert IHS image to RGB
int YaIPS_RGB_Color_IHS2RGB( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                             Fl_RGB_Image *pSrc);  // In: Pointer to IHS image

// Adjust intensity, hue and saturation
int YaIPS_RGB_Color_IHS_Adjust( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                                Fl_RGB_Image *pSrc,   // In: Pointer to RGB image
                                int IHS_IntAdjust,    // In: Intensity adjust +- 100 %
                                int IHS_HueAdjust,    // In: hue adjust +- 100 %
                                int IHS_SatAdjust);   // In: saturation adjust +- 100 %

// Min/max of the color components to a single channel output image
int YaIPS_RGB_Color_MinMax( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to IHS image
                            Fl_RGB_Image *pSrc,   // In: Pointer to RGB image
                            int Mode);            // In: 0 = min, 1 = max

// Simple color conversions

#define YAIPS_COLOR_RGB_2_R          0    // Get red color component
#define YAIPS_COLOR_RGB_2_G          1    // Get green color component
#define YAIPS_COLOR_RGB_2_B          2    // Get blue color component
#define YAIPS_COLOR_GET_ALPHA        3    // Get alpha from BW or RGB image
#define YAIPS_COLOR_RGB_2_I          4    // Convert image RGB to intensity
#define YAIPS_COLOR_RGB_2_H          5    // Convert image RGB to hue
#define YAIPS_COLOR_RGB_2_S          6    // Convert image RGB to saturation
#define YAIPS_COLOR_RGB_MIN          7    // Minimum of color components
#define YAIPS_COLOR_RGB_MAX          8    // Maximum of color components
#define YAIPS_COLOR_RGB_2_BGR        9    // Convert RGB to BGR image or vice versa. In place conversion is supported.

#define YAIPS_COLOR_RGB_N_CONV      10    // Number of simple color conversions

int YaIPS_RGB_Color_ConvSimple( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to destination image
                                Fl_RGB_Image *pSrc,   // In: Pointer to source image
                                int ColorOp);         // In: what to do, see #define YAIPS_COLOR_xxx

// Convert number of bytes
int YaIPS_RGB_Color_DestD( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to destination image
                           Fl_RGB_Image *pSrc,   // In: Pointer to source image
                           int Dst_nBytes,       // # bytes of the destination image
                           int Alpha = 0xff);    // Optional alpha value

// Color conversion with matrix

typedef struct {
  char *pName;                                    // If != NULL, point to friendly name for matrix
  int R_Offset; float R_MultR, R_MultG, R_MultB;  // Matrix coefficients of the red channel
  int G_Offset; float G_MultR, G_MultG, G_MultB;  // Matrix coefficients of the green channel
  int B_Offset; float B_MultR, B_MultG, B_MultB;  // Matrix coefficients of the blue channel
} T_YaIPS_ColorMatrix;

extern T_YaIPS_ColorMatrix ColorMatrix_List[];   // Pointer to color matrix list
extern int nColorMatrix_List;                    // Size of color matrix list

int YaIPS_RGB_Color_ConvMatrix( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to IHS image
                                Fl_RGB_Image *pSrc,   // In: Pointer to RGB image
                                int R_Offset, float R_MultR, float R_MultG, float R_MultB,  // In: Matrix coefficients of the red channel
                                int G_Offset, float G_MultR, float G_MultG, float G_MultB,  // In: Matrix coefficients of the green channel
                                int B_Offset, float B_MultR, float B_MultG, float B_MultB); // In: Matrix coefficients of the blue channel

int YaIPS_RGB_Color_ConvMatrix( Fl_RGB_Image **ppDst,               // Out: Pointer to pointer to IHS image
                                Fl_RGB_Image *pSrc,                 // In: Pointer to RGB image
                                T_YaIPS_ColorMatrix *pColorMatrix); // In: Pointer to color matrix

// LUT table conversion

#ifndef YAIPS_LUT_N_POINTS   // Ensure we have this define
#define YAIPS_LUT_N_POINTS   256                      // Number of entries in a LUT
#endif

int YaIPS_RGB_Color_ConvLUT( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to IHS image
                             Fl_RGB_Image *pSrc,   // In: Pointer to RGB image
                             uchar *pLookupR,      // In: Point to red lookup table (256 bytes)
                             uchar *pLookupG,      // In: Point to green lookup table (256 bytes)
                             uchar *pLookupB,      // In: Point to blue lookup table (256 bytes)
                             int BlendBW = 0);     // In: Used for color images only.
                                                   //     Range = 0 ... 100. 0 = use Color pixels. 100 = use BW pixels.


// Make a offset + gain change LUT table
int YaIPS_RGB_Color_MakeLUT_OffsetGain( uchar *pLookupR,      // Out: Point to red lookup table (256 bytes)
                                        uchar *pLookupG,      // Out: Point to green lookup table (256 bytes)
                                        uchar *pLookupB,      // Out: Point to blue lookup table (256 bytes)
                                        int   OffsetR,        // In: Add offset to red color channel
                                        int   OffsetG,        // In: Add offset to green color channel
                                        int   OffsetB,        // In: Add offset to blue color channel
                                        float GainChR,        // In: gain change for red color channel
                                        float GainChG,        // In: gain change for green color channel
                                        float GainChB);       // In: gain change for blue color channel

// Make a gamma LUT table
int YaIPS_RGB_Color_MakeLUT_Gamma( uchar *pLookupR,      // Out: Point to red lookup table (256 bytes)
                                   uchar *pLookupG,      // Out: Point to green lookup table (256 bytes)
                                   uchar *pLookupB,      // Out: Point to blue lookup table (256 bytes)
                                   float GammaR,         // In: Gamma for red color channel
                                   float GammaG,         // In: Gamma for green color channel
                                   float GammaB);        // In: Gamma for blue color channel

// Modify contrast with LUT table
int YaIPS_RGB_Color_MakeLUT_Contrast( uchar *pLookupR,      // Out: Point to red lookup table (256 bytes)
                                      uchar *pLookupG,      // Out: Point to green lookup table (256 bytes)
                                      uchar *pLookupB,      // Out: Point to blue lookup table (256 bytes)
                                      int Contrast1in,      // In: Contrast 1. point in value
                                      int Contrast1out,     // In: Contrast 1. point out value
                                      int Contrast2in,      // In: Contrast 2. point in value
                                      int Contrast2out);    // In: Contrast 2. point out value

// Make duotone LUT table
int YaIPS_RGB_Color_MakeLUT_Duotone( uchar *pLookupR,       // Out: Point to red lookup table (256 bytes)
                                     uchar *pLookupG,       // Out: Point to green lookup table (256 bytes)
                                     uchar *pLookupB,       // Out: Point to blue lookup table (256 bytes)
                                     Fl_Color Color_Bright, // Color for highlights
                                     Fl_Color Color_Dark,   // Color for shadows
                                     int contrast = 0,      // Contrast range -100 ... 100 (0 = no change)
                                     int brightness = 0);   // Brightness range -100 ... 100 (0 = no change)

// Mix color channels to create an image
int YaIPS_RGB_MixChannels( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to IHS image
                           Fl_RGB_Image *pSrc,   // In: Pointer to common input image
                           Fl_RGB_Image *pInG = NULL,   // In: Pointer to optional red channel
                           Fl_RGB_Image *pInB = NULL,   // In: Pointer to optional blue channel
                           Fl_RGB_Image *pInA = NULL);  // In: Pointer to optional alpha channel

// Generate an alpha mask from color

#define YAIPS_RGB_CHROMA_OUT_INP_A        0  // Processed input image with alpha mask, 4 bytes per pixel.
#define YAIPS_RGB_CHROMA_OUT_ALPHA        1  // Alpha mask only, 1 byte per pixel.
#define YAIPS_RGB_CHROMA_OUT_BLEND        2  // Input image 1 overlaid to input image 2, 3 bytes per pixel.
#define YAIPS_RGB_CHROMA_OUT_INP_P        3  // Processed input image, 3 bytes per pixel.
#define YAIPS_RGB_CHROMA_OUT_COL_B        4  // Processed input image alpha blended to color, 3 bytes per pixel.

#define YAIPS_RGB_CHROMA_FLAG_SMOOTHSTEP 0x0001  // Improves edge transitions
#define YAIPS_RGB_CHROMA_FLAG_DESPILL_O  0x0002  // removes color residue from the outside of the mask and the edges
#define YAIPS_RGB_CHROMA_FLAG_DESPILL_I  0x0004  // removes color residue from the inside of the mask

int YaIPS_RGB_ChromaKey( Fl_RGB_Image **ppDst,            // Out: Pointer to pointer to RGB image.
                         Fl_RGB_Image *pSrc,              // In: Pointer to RGB image
                         Fl_Color KeyColor,               // In: The key color
                         int HueThres, int HueFade,       // In: Hue threshold and fade (range 0 .. 100)
                         int SatThres, int SatFade,       // In: Saturation threshold and fade (range 0 .. 100)
                         int IDaThres, int IDaFade,       // In: Intensity dark threshold and fade (range 0 .. 100)
                         int IBrThres, int IBrFade,       // In: Intensity bright threshold and fade (range 0 .. 100)
                         int Flags = 0,                   // In: Optional processing steps
                         int OutputType = 0,              // In: Type of output image
                         Fl_RGB_Image *pBGnd = NULL,      // In: Background image used for 'YAIPS_RGB_CHROMA_OUT_BLEND'
                         Fl_Color BlendColor = 248);      // In: The color used for 'YAIPS_RGB_CHROMA_OUT_COL_B'

//--------------------------------------------------------------------------
// YaIPS_RGB_Contour.cpp
//--------------------------------------------------------------------------

#define YAIPS_RGB_CONTOUR_CONTOUR      0  // Set 255 for contour, 0 for plane
#define YAIPS_RGB_CONTOUR_PLANE        1  // Set 0 for contour, 255 for plane
#define YAIPS_RGB_CONTOUR_LOWPASS      2  // Lowpass for plane pixels
#define YAIPS_RGB_CONTOUR_LP_ALPHA     3  // Lowpass for plane pixels and set edge bit in alpha mask

// Separates contours from plane
int YaIPS_RGB_Contour( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                       Fl_RGB_Image *pSrc,   // Source image
                       int PlaneThres,       // Plane threshold
                       int ContourOp);       // Type of operation

//--------------------------------------------------------------------------
// YaIPS_RGB_Crcdf.cpp
//--------------------------------------------------------------------------

// Result of crcdf function
typedef struct {
  int Valid;         // If != 0 the results are valid
  int bestval;       // Quality of result
  int maxval;        // Maximum values of all correlation results
  int objval;        // Value for object correlated with zero
  int ObjSizeXX;     // Width of object in pixels
  int ObjSizeYY;     // Height of object in pixels
  // Position of result
  int xpos;          // X coordinate
  int ypos;          // Y coordinate
  float Quality;     // Correlation quality. 0 is very bad, 100 is very good.
  char Label[ 64];   // Name for object
} T_YaIPS_Res_Crcdf;

// Result of crcdf object option data

#define YAIPS_CRCDF_OOPT_MAX_OBJECTS 128 // Max number of objects in one image file

typedef struct {
  int x;            // X position left upper corner in image file
  int y;            // Y position left upper corner in image file
  int xx;           // X size in image file
  int yy;           // Y size in image file

  char Label[ 64];  // Name for object
} T_YaIPS_Crcdf_OO_1;

typedef struct {
  int NumObjects;    // Number of objects
  T_YaIPS_Crcdf_OO_1 Object[ YAIPS_CRCDF_OOPT_MAX_OBJECTS];  // Save objects here
} T_YaIPS_Crcdf_ObjOpt;

// Cross difference function
int YaIPS_RGB_Crcdf_Calc( Fl_RGB_Image **ppCorImg,       // Optional out: Correlation image
                          YaIPS_RGB_ImgD_t *pScnImg,     // In: Scene image
                          int ScnByte,                   // In: What byte component to use from the scene image
                          YaIPS_RGB_ImgD_t *pObjImg,     // In: Object image. Must have 1 byte per pixel.
                          T_YaIPS_Res_Crcdf *pBestcor);  // Out: Best correlation result

// Cross difference function with single object
int YaIPS_RGB_Crcdf_Calc( Fl_RGB_Image **ppCorImg,         // Out: Correlation image
                          Fl_RGB_Image *pScnImg,           // In: Scene image
                          int ScnByte,                     // In: What byte component to use from the scene image
                          Tvector *vObj,                   // In: optional vector with extracted objects
                          Fl_RGB_Image *pObjImg,           // In: Object image. Must have 1 byte per pixel.
                          T_YaIPS_Res_Crcdf *pBestcor,     // Out: Best correlation result
                          int AOI_X = 0, int AOI_Y = 0,    // Optional in: AAOI left upper corner
                          int AOI_XX = 0, int AOI_YY = 0); // Optional in: AAOI size

// Cross difference function with multiple objects
int YaIPS_RGB_Crcdf_Calc( Fl_RGB_Image **ppCorImg,         // Out: Correlation image
                          Fl_RGB_Image *pScnImg,           // In: Scene image
                          int ScnByte,                     // In: What byte component to use from the scene image
                          Tvector *vObj,                   // In: optional vector with extracted objects
                          Fl_RGB_Image *pObjImg,           // In: Object image. Must have 1 byte per pixel.
                          T_YaIPS_Crcdf_ObjOpt *pObjOpt,   // In: Object options
                          T_YaIPS_Res_Crcdf *pBestcor,     // Out: Best correlation result
                          int AOI_X = 0, int AOI_Y = 0,    // AOptional in: AOI left upper corner
                          int AOI_XX = 0, int AOI_YY = 0); // Optional in: AAOI size

// Convert integer (4 byte) correlation image to a BW (1 Byte) image
int YaIPS_RGB_Crcdf_Conv2BW( Fl_RGB_Image **ppDst,          // Out: Pointer to converted BW image (1 bytes)
                             Fl_RGB_Image *pSrc,            // In: correlation image (4 bytes)
                             T_YaIPS_Res_Crcdf *pBestcor);  // In: Best correlation result

// Get object options for a correlation object image
int YaIPS_RGB_Crcdf_ObjOptions( T_YaIPS_Crcdf_ObjOpt *pObjOpt, // Out: Object options
                                Fl_RGB_Image *pSrc,            // In: object image (1 byte per pixel)
                                char *pFileName);              // In: File name with path of object image

// Get object options for a correlation object image. Label objects do adjust size.
int YaIPS_RGB_Crcdf_ObjOptions( T_YaIPS_Crcdf_ObjOpt *pObjOpt, // Out: Object options
                                Fl_RGB_Image *pSrc,            // In: object image (1 byte per pixel)
                                char *pFileName,               // In: File name with path of object image
                                int BinThres,                  // In: Binarization threshold
                                int BinMode,                   // In: Binarization mode, 0: objects >= Thres, 1: code if < Thres
                                int AreaMin,                   // In: Minimum area of an object to be labeled
                                int AreaMax);                  // In: Maximum area of an object to be labeled

//--------------------------------------------------------------------------
// YaIPS_RGB_Combine.cpp
//--------------------------------------------------------------------------

// Combine operators
#define YAIPS_COMBINE_OP_ADD       0   // Addition
#define YAIPS_COMBINE_OP_W_ADD     1   // Weighted addition
#define YAIPS_COMBINE_OP_SUB_1_2   2   // Subtraction 1. source - 2. source
#define YAIPS_COMBINE_OP_SUB_2_1   3   // Subtraction 2. source - 1. source
#define YAIPS_COMBINE_OP_SUB_ABS   4   // Subtraction with absolute value
#define YAIPS_COMBINE_OP_MULT      5   // Multiplication
#define YAIPS_COMBINE_OP_MIN       6   // Minimum value
#define YAIPS_COMBINE_OP_MAX       7   // Maximum value
#define YAIPS_COMBINE_OP_AVG       8   // Average images
#define YAIPS_COMBINE_OP_AND       9   // AND images
#define YAIPS_COMBINE_OP_OR       10   // OR images
#define YAIPS_COMBINE_OP_XOR      11   // XOR images
#define YAIPS_COMBINE_OP_CMP_EQ   12   // Compare images ==
#define YAIPS_COMBINE_OP_CMP_NE   13   // Compare images !=
#define YAIPS_COMBINE_OP_CMP_GT   14   // Compare images >
#define YAIPS_COMBINE_OP_CMP_LE   15   // Compare images <=
#define YAIPS_COMBINE_OP_CMP_GE   16   // Compare images >=
#define YAIPS_COMBINE_OP_CMP_LT   17   // Compare images <
#define YAIPS_COMBINE_OP_SHADING  18   // Image shading
#define YAIPS_COMBINE_OP_FADE     19   // Fade between two images
#define YAIPS_COMBINE_OP_FADEIMG1 20   // Fade between two images by third image. First is overlaid.
#define YAIPS_COMBINE_OP_FADEIMG2 21   // Fade between two images by third image. Second is overlaid.

// Alpha operators
#define YAIPS_COMBINE_ALPHA_NO      0   // No alpha. Strip any existing alpha.
#define YAIPS_COMBINE_ALPHA_KEEP_1  1   // Keep alpha from first input.
#define YAIPS_COMBINE_ALPHA_KEEP_2  2   // Keep alpha from second input.
#define YAIPS_COMBINE_ALPHA_MIN     3   // If both inputs have alpha, output minimum of alpha values.
#define YAIPS_COMBINE_ALPHA_MAX     4   // If both inputs have alpha, output maximum of alpha values.
#define YAIPS_COMBINE_ALPHA_BUTTON_MAX   (YAIPS_COMBINE_ALPHA_MAX + 1)  // Number of alpha operator radio buttons

// Combine two images
int YaIPS_RGB_Combine( Fl_RGB_Image **ppDst,        // Out: Pointer to pointer to RGB image
                       Fl_RGB_Image *pSrc1,         // Source image
                       Fl_RGB_Image *pSrc2,         // Source image
                       int Operator,                // Type of operation
                       int Alpha_Op,                // Alpha operator
                       float ResMultArg = 1.0,      // Result multiplier
                       int Offset  = 0,             // Add this offset to the result
                       float MultArg2 = 0.0,        // Optional: 2. multiplier. Is used by weighted add.
                       Fl_RGB_Image *pSrc3 = NULL); // Optional: a third source image

// Calculation with constants operators
#define YAIPS_CALC_CONST_OP_ADD       0   // Addition
#define YAIPS_CALC_CONST_OP_SUB_I_C   1   // Subtraction source image - constant
#define YAIPS_CALC_CONST_OP_SUB_C_I   2   // Subtraction constant - source image
#define YAIPS_CALC_CONST_OP_SUB_ABS   3   // Subtraction with absolute value
#define YAIPS_CALC_CONST_OP_MULT      4   // Multiplication
#define YAIPS_CALC_CONST_OP_MIN       5   // Minimum value
#define YAIPS_CALC_CONST_OP_MAX       6   // Maximum value
#define YAIPS_CALC_CONST_OP_AVG       7   // Average images
#define YAIPS_CALC_CONST_OP_AND       8   // AND images
#define YAIPS_CALC_CONST_OP_OR        9   // OR images
#define YAIPS_CALC_CONST_OP_XOR      10   // XOR images
#define YAIPS_CALC_CONST_OP_CMP_EQ   11   // Compare images ==
#define YAIPS_CALC_CONST_OP_CMP_NE   12   // Compare images !=
#define YAIPS_CALC_CONST_OP_CMP_GT   13   // Compare images >
#define YAIPS_CALC_CONST_OP_CMP_LE   14   // Compare images <=
#define YAIPS_CALC_CONST_OP_CMP_GE   15   // Compare images >=
#define YAIPS_CALC_CONST_OP_CMP_LT   16   // Compare images <

#define YAIPS_CALC_CONST_FLAGS_COLOR  0x0001 // Flag bit: Process color part
#define YAIPS_CALC_CONST_FLAGS_ALPHA  0x0002 // Flag bit: Process alpha part

// Simple mathematical calculation of an image and a constant.
int YaIPS_RGB_CalcConst( Fl_RGB_Image **ppDst,       // Out: Pointer to pointer to RGB image
                         Fl_RGB_Image *pSrc,         // Source image
                         int Operator,               // Type of operation
                         int r, int g, int b, int a, // Constants used for Calculation. Hold RGBA values.
                         int Alpha_Op,               // Alpha operator
                         int Flags,                  // Flags
                         float ResMultArg = 1.0,     // Result multiplier
                         int Offset = 0);            // Add this offset to the result

//--------------------------------------------------------------------------
// YaIPS_RGB_FilterNxN.cpp
//--------------------------------------------------------------------------

// Filter kernel sizes
// NOTE: All filter kernels are odd
#define YAIPS_FILTER_SIZE_MIN      3   // Minimum kernel size
#define YAIPS_FILTER_SIZE_MAX     13   // Maximum kernel size
#define YAIPS_FILTER_SIZE_MAX_2  (YAIPS_FILTER_SIZE_MAX * YAIPS_FILTER_SIZE_MAX)  // Square maximum kernel size

#define YAIPS_FILTER_FLAG_RES_MULT_AUTO 0x0001   // If set, compute the result multiplier by 1 / sum of coefficients
#define YAIPS_FILTER_FLAG_RES_ABS       0x0002   // If set, make a absolute result

// Filter kernel definition
typedef struct {
  int OffX;                // +- Offset from center
  int OffY;                // +- Offset from center
  int Coefficient;         // Coefficient
} T_YaIPS_FilterDef;

// N*N filter with free configuration of (integer !) coefficients.
int YaIPS_RGB_FilterNxN_Def( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                             Fl_RGB_Image *pSrc,   // Source image
                             int Flags,            // Some Flags
                             float ResMultArg,     // Result multiplier
                             int Offset,           // Add this offset to the result
                             int KernelSize,       // Kernel size
                             int nFilterDef,       // Number of filter definitions
                             T_YaIPS_FilterDef *pFilterDef);


// N*N filter with free configuration of (integer !) coefficients.
int YaIPS_RGB_FilterNxN_Vec( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                             Fl_RGB_Image *pSrc,   // Source image
                             int Flags,            // Some Flags
                             float ResMultArg,     // Result multiplier
                             int Offset,           // Add this offset to the result
                             int KernelSize,       // Kernel size
                             int *pKernel);        // Point to filter kernel (vector of KernelSize� integers)

// 3*3 Gaussian lowpass filter
int YaIPS_RGB_Filt_Gauss3x3( Fl_RGB_Image **ppDst,      // Out: Pointer to pointer to RGB image
                            Fl_RGB_Image *pSrc);       // Source image

// N*N Gaussian lowpass filter
int YaIPS_RGB_Filt_GaussNxN( Fl_RGB_Image **ppDst,      // Out: Pointer to pointer to RGB image
                             Fl_RGB_Image *pSrc,        // Source image
                             int KernelSize);           // Kernel size

// 3*3 lowpass filter. 9 coefficients are used.
int YaIPS_RGB_Filt_Lowpass3x3( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                               Fl_RGB_Image *pSrc);     // Source image

// N*N lowpass filter
int YaIPS_RGB_Filt_LowpassNxN( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                               Fl_RGB_Image *pSrc,      // Source image
                               int KernelSize);         // Kernel size

// 3*3 Laplacian highpass filter.
int YaIPS_RGB_Laplace3x3( Fl_RGB_Image **ppDst,        // Out: Pointer to pointer to RGB image
                          Fl_RGB_Image *pSrc,          // Source image
                          float ResMultArg,            // Result multiplier
                          int Offset,                  // Add this offset to the result
                          int AbsRes);                 // If true, absolute result

// Edge difference filter

#define YAIPS_FILTER_EDGE_DIFF_HOR    0   // Horizontal difference
#define YAIPS_FILTER_EDGE_DIFF_VER    1   // Vertical difference
#define YAIPS_FILTER_EDGE_DIFF_DIAG1  2   // Diagonally difference 1
#define YAIPS_FILTER_EDGE_DIFF_DIAG2  3   // Diagonally difference 2

int YaIPS_RGB_Filt_EdgeDiff( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                            Fl_RGB_Image *pSrc,      // Source image
                            int Direction,           // Direction, see defines YAIPS_FILTER_EDGE_DIFF_xxx
                            float ResMultArg,        // Result multiplier
                            int Offset,              // Add this offset to the result
                            int AbsRes);             // If true, absolute result

//--------------------------------------------------------------------------
// YaIPS_RGB_Frame.cpp
//--------------------------------------------------------------------------

// Generate frame around window
int YaIPS_RGB_Frame( YaIPS_RGB_ImgD_t *pImgD,  // source and destination image
                     int width,                // abs(width) = width of boundary
                     int value);               // value to set if width has negative sign else dummy

// Generate frame around window
int YaIPS_RGB_Frame( Fl_RGB_Image *pDst,  // source and destination image
                     int width,           // abs(width) = width of boundary
                     int value);          // value to set if width has negative sign else dummy

//--------------------------------------------------------------------------
// YaIPS_RGB_GaussXY.cpp
//--------------------------------------------------------------------------

// N*N Gaussian lowpass filter

#define YAIPS_GAUSXY_MODE_X   0  // 1*N X/horizontal gaussian lowpass filter
#define YAIPS_GAUSXY_MODE_Y   1  // Y/vertical gaussian lowpass filter
#define YAIPS_GAUSXY_MODE_XY  2  // XY gaussian lowpass filter

// Common usable filter
int YaIPS_RGB_GaussXY( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                       Fl_RGB_Image *pSrc,      // Source image
                       int KernelSize,          // Kernel size
                       int ModeXY);             // XY Mode

// Specialized calling
int YaIPS_RGB_GaussXY_sub( YaIPS_RGB_ImgD_t *pSrc,
                           YaIPS_RGB_ImgD_t *pDst,
                           float g_sigma,
                           int yOff,
                           int ModeXY);             // XY Mode

//--------------------------------------------------------------------------
// YaIPS_RGB_GeoTransform.cpp
//--------------------------------------------------------------------------

// Resize by a power of 2
int YaIPS_RGB_Geo_Resize2( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                           Fl_RGB_Image *pSrc,   // Source image
                           int SizeShiftArg);    // Power of 2, < 0 is shrink > 0 is enlarge

int YaIPS_RGB_Geo_Resize2( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                           Fl_RGB_Image *pSrc,   // Source image
                           int SizeShiftArgX,    // Power of 2 for X, < 0 is shrink > 0 is enlarge
                           int SizeShiftArgY);   // Power of 2 for Y, < 0 is shrink > 0 is enlarge

// Mirror image in X, Y or X and Y
int YaIPS_RGB_Geo_Mirror( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                          Fl_RGB_Image *pSrc,   // Source image
                          int MirrorOp);        // Mirror 0 = X, 1 = Y, 2 = X and Y

// Rotate image by +-90 or 180 degrees
int YaIPS_RGB_Geo_Rotate90( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                            Fl_RGB_Image *pSrc,   // Source image
                            int RotateOp);        // Rotate  0 = 0�, 1 = 90� clockwise, 2 = 180�, 3 = 90� counterclockwise

// Lens Correction, rotation and minor size or offset changes
int YaIPS_RGB_Geo_Lens( Fl_RGB_Image **ppDst,     // Out: Pointer to pointer to RGB image
                        Fl_RGB_Image *pSrc,       // Source image
                        int OutsiteColor,         // Color for the area outside an image
                        int OutsiteBlend,         // If set, outside area is alpha blended
                        int32 PLensFacArg,        // Lens correction factor [in units of 1.0E-10]
                        float Rotation,           // Rotation correction [Degree]
                        float SizeCorrPer,        // Size correction percent [%]
                        int xDelta, int yDelta);  // Position correction [pixel]

// Warp 4 points in source to 4 points in destination.
int YaIPS_RGB_Geo_Warp_4_Points( Fl_RGB_Image **ppDst,       // Out: Pointer to pointer to RGB image
                                 Fl_RGB_Image *pSrc,         // Source image
                                 int OutsiteColor,           // Color for the area outside an image
                                 int OutsiteBlend,           // If set, outside area is alpha blended
                                 YaIPS_XY_float *pSrcPoints, // Point to 4 source points x + y
                                 YaIPS_XY_float *pDstPoints, // Point to 4 destination points x + y
                                 int dstWidth,               // Destination with if > 0 else compute from pDstPoints
                                 int dstHeight);             // Destination height if > 0 else compute from pDstPoints

// Geometric transformation setup

// pivot mode
#define GE_PIVORG   0       // origin
#define GE_PIVCTR   1       // center
#define GE_PIVPAR   2       // arbitrary

int YaIPS_RGB_Geo_TransSetup( int pivmod,                        // Pivot mode for transformation
                              float xps = 0.0, float yps = 0.0,  // Arbitrary source image pivot point, only used if "pivmod" = 2
                              float xpd = 0.0, float ypd = 0.0); // Arbitrary destination image pivot point, only used if "pivmod" = 2

// Rotation and translation
int YaIPS_RGB_Geo_Rotate( Fl_RGB_Image **ppDst,                // Out: Pointer to pointer to RGB image
                          Fl_RGB_Image *pSrc,                  // Source image
                          int OutsiteColor,                    // Color for the area outside an image
                          int OutsiteBlend,                    // If set, outside area is alpha blended
                          float Rotation,                      // Rotation clockwise [Degree]
                          int xxDst = -1, int yyDst = -1,      // Size for destination. Any 0: use size of source. Any < 0: adapt size.
                          float xps = -1.0, float yps = -1.0,  // Center of rotation in source image
                          float xpd = -1.0, float ypd = -1.0); // Center of rotations in destination image

// Image transformation by parallel projection
int YaIPS_RGB_Geo_Transform( Fl_RGB_Image **ppDst,        // Out: Pointer to pointer to RGB image
                            Fl_RGB_Image *pSrc,           // Source image
                            int OutsiteColor,             // Color for the area outside an image
                            int OutsiteBlend,         // If set, outside area is alpha blended
                            float Rotation,               // Rotation clockwise [Degree]
                            float xscal, float yscal,     // Scale image horizontal and vertical
                            float xshift, float yshift,   // Shift image horizontal and vertical
                            int xxDst = -1, int yyDst = -1,      // Size for destination. Any 0: use size of source. Any < 0: adapt size.
                            float xps = -1.0, float yps = -1.0,  // Center of rotation in source image
                            float xpd = -1.0, float ypd = -1.0); // Center of rotations in destination image

//--------------------------------------------------------------------------
// YaIPS_RGB_Linesum.cpp
//--------------------------------------------------------------------------

// Sum up grey values in a column
int YaIPS_RGB_Colsum( YaIPS_RGB_ImgD_t *pSrc,   // Source image
                      Tvector *dstvec,      // Destination vector for linesums
                      int ByteComponent,    // What byte to use from the pixel
                      int mode);            // Normalization mode. 0 = sum, 1 = average

// Sum up grey values in a column
int YaIPS_RGB_Colsum( Fl_RGB_Image *pSrc,   // Source image
                      Tvector *dstvec,      // Destination vector for linesums
                      int ByteComponent,    // What byte to use from the pixel
                      int mode);            // Normalization mode. 0 = sum, 1 = average

// Sum up grey values in a row
int YaIPS_RGB_Rowsum( YaIPS_RGB_ImgD_t *pSrc,   // Source image
                      Tvector *dstvec,      // Destination vector for linesums
                      int ByteComponent,    // What byte to use from the pixel
                      int mode);            // Normalization mode. 0 = sum, 1 = average

// Sum up grey values in a row
int YaIPS_RGB_Rowsum( Fl_RGB_Image *pSrc,   // Source image
                      Tvector *dstvec,      // Destination vector for linesums
                      int ByteComponent,    // What byte to use from the pixel
                      int mode);            // Normalization mode. 0 = sum, 1 = average

// Sum up grey values in a line
int YaIPS_RGB_Linesum( YaIPS_RGB_ImgD_t *pSrc,   // Source image
                       Tvector *dstvec,      // Destination vector for linesums
                       int ByteComponent,    // What byte to use from the pixel
                       int x0, int y0,       // Point from
                       int x1, int y1,       // Point to
                       int nsamp,            // Number of samples. 0 = set automatically else must be >= 2.
                       int width,            // Pixels to sum up or average vertical to line direction
                       int mode,             // Normalization mode. 0 = sum, 1 = average, 2 = min, 3 = max
                       double ytox);         // y/x pixel relation

// Sum up grey values in a line
int YaIPS_RGB_Linesum( Fl_RGB_Image *pSrc,   // Source image
                       Tvector *dstvec,      // Destination vector for linesums
                       int ByteComponent,    // What byte to use from the pixel
                       int x0, int y0,       // Point from
                       int x1, int y1,       // Point to
                       int nsamp,            // Number of samples. 0 = set automatically else must be >= 2.
                       int width,            // Pixels to sum up or average vertical to line direction
                       int mode,             // Normalization mode. 0 = sum, 1 = average, 2 = min, 3 = max
                       double ytox);         // y/x pixel relation

// Sum up grey values in a line with bilinear pixel values
int YaIPS_RGB_LinesumBiLin( YaIPS_RGB_ImgD_t *pSrc,   // Source image
                       Tvector *dstvec,      // Destination vector for linesums
                       int ByteComponent,    // What byte to use from the pixel
                       float x0, float y0,     // Point from
                       float x1, float y1,     // Point to
                       int nsamp,            // Number of samples. 0 = set automatically else must be >= 2.
                       int width,            // Pixels to sum up or average vertical to line direction
                       int mode,             // Normalization mode. 0 = sum, 1 = average
                       double ytox);         // y/x pixel relation

// Sum up grey values in a line with bilinear pixel values
int YaIPS_RGB_LinesumBiLin( Fl_RGB_Image *pSrc,   // Source image
                       Tvector *dstvec,      // Destination vector for linesums
                       int ByteComponent,    // What byte to use from the pixel
                       float x0, float y0,     // Point from
                       float x1, float y1,     // Point to
                       int nsamp,            // Number of samples. 0 = set automatically else must be >= 2.
                       int width,            // Pixels to sum up or average vertical to line direction
                       int mode,             // Normalization mode. 0 = sum, 1 = average
                       double ytox);         // y/x pixel relation

//--------------------------------------------------------------------------
// YaIPS_RGB_Morphology.cpp
//--------------------------------------------------------------------------

#define YAIPS_RGB_MORPH_EROSION      0  // Minimum of neighbors
#define YAIPS_RGB_MORPH_DILATION     1  // Maximum of neighbors
#define YAIPS_RGB_MORPH_MEDIAN       2  // Median of neighbors
#define YAIPS_RGB_MORPH_CLOSING      3  // Dilation followed by an erosion
#define YAIPS_RGB_MORPH_OPENING      4  // Erosion followed by a dilation
#define YAIPS_RGB_MORPH_GRADIENT     5  // Maximum minus minimum of neighbors

// * 3*3 morphology filter, NO opening and closing
int YaIPS_RGB_Morphology( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                          Fl_RGB_Image *pSrc,   // Source image
                          int MorphOp);         // Morphology operator

// 3*3 morphology filter, uses an intermediate image
int YaIPS_RGB_Morphology( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                          Fl_RGB_Image *pSrc,   // Source image
                          int MorphOp,          // Morphology operator
                          int MoprhRuns);       // Number of runs

//--------------------------------------------------------------------------
// YaIPS_RGB_Posterization.cpp
//--------------------------------------------------------------------------

#define YAIPS_RLC_POSTER_MAX_LEVEL        32   // Max nLevels value

int YaIPS_RGB_PosterizeSimple( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                               Fl_RGB_Image *pSrc,   // Source image
                               int nLevels,          // In: Number of levels
                               int Average = false); // In: If set average colors

// A more complex image posterization
int YaIPS_RGB_PosterizeEx1( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                            Fl_RGB_Image *pSrc,      // Source image
                            int i_maxcolors,         // Maximum colors to extract
                            int i_algorithm,         // Algorithm type
                            int i_parameter);        // Algorithm parameter

// k-Means posterization algorithm
int YaIPS_RGB_PosterizeKmeans( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                               Fl_RGB_Image *pSrc,   // Source image
                               int nLevels,          // In: Number of levels
                               int nIterations);     // In: If set average colors

// Blend edges into already posterized image
int YaIPS_RGB_PosterizeEdges( Fl_RGB_Image *pDst,    // InOut: Already posterized.
                              Fl_RGB_Image *pSrc,    // In: Source image
                              float CannySigma,      // In: Sigma for gaussian filter
                              float CannyResMult,    // In: Edge strength
                              unsigned int EdgeCol,  // In: Edge color
                              int EdgeHighlight = 0, // In: Highlights stronger edges. 0 = no, 100 = max highlight.
                              int EdgeStrength = 0); // In: Edge strength. 0 = no, 100 max strength.

//--------------------------------------------------------------------------
// YaIPS_RGB_RunLengthCode.cpp
//--------------------------------------------------------------------------

// defines for YaIPS_RGB_RLC_Code()/YaIPS_RGB_RLC_CodeMeas() binarization mode
#define YAIPS_RLC_BINMODE_GE_T1          0  // >= BinThres1
#define YAIPS_RLC_BINMODE_L_T1           1  // < BinThres1
#define YAIPS_RLC_BINMODE_GE_T1_AND_L_T2 2  // >= BinThres1 && <  BinThres2
#define YAIPS_RLC_BINMODE_L_T1_OR_GE_T2  3  // < BinThres1 || >=  BinThres2
#define YAIPS_RLC_BINMODE_L_CHROMA       4  // < chroma threshold
#define YAIPS_RLC_BINMODE_GE_CHROMA      5  // >= chroma threshold
#define YAIPS_RLC_BINMODE_GE_BM_AUTO     6  // >= bimodal threshold (automatic computation)
#define YAIPS_RLC_BINMODE_L_BM_AUTO      7  // <  bimodal threshold (automatic computation)

// Binaries an image to run length codes
int YaIPS_RGB_RLC_Code( Timages **ppRLC1,        // Out: Point to pointer to image for run length codes
                        Fl_RGB_Image *pSrc,      // In: Source image
                        int ColorSpace,          // In: Color space for color images BW, R, G or B
                        int BinThres1,           // In: Binarization 1. threshold
                        int BinThres2,           // In: Binarization 2. threshold
                        int BinMode,             // In: Binarization mode
                        int *pOutUsedXX = NULL,  // Out: used source image width. Depends from AOI width
                        int AOI_X = 0, int AOI_Y = 0,    // Optional in: AAOI left upper corner
                        int AOI_XX = 0, int AOI_YY = 0); // Optional in: AAOI size

// Label run length codes and measure the objects
int YaIPS_RGB_RLC_LabelMeas( Tvector *vObj,          // In Out: vector with extracted objects
                             Timages *iRLC1,         // In: Point to image with run length codes
                             int AreaMin,            // In: Minimum area of an object to be labeled
                             int AreaMax);           // In: Maximum area of an object to be labeled

// Binaries an image to run length codes, label and measure the objects
int YaIPS_RGB_RLC_CodeMeas( Tvector *vObj,        // In Out: vector with extracted objects
                            Fl_RGB_Image *pSrc,   // In: Source image
                            int ColorSpace,          // In: Color space for color images BW, R, G or B
                            int BinThres1,           // In: Binarization 1. threshold
                            int BinThres2,           // In: Binarization 2. threshold
                            int BinMode,             // In: Binarization mode
                            int AreaMin,          // In: Minimum area of an object to be labeled
                            int AreaMax,          // In: Maximum area of an object to be labeled
                            int AOI_X = 0, int AOI_Y = 0,    // Optional in: AAOI left upper corner
                            int AOI_XX = 0, int AOI_YY = 0); // Optional in: AAOI size

// Converts a run length coded labeled image to a grey level image.
int YaIPS_RGB_RLC_Decode( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                          Timages *iRLC1,       // In: Image with run length codes
                          int label,            // In: Minimum area of an object to be labeled
                          int bcol,             // In: background color. Must be a 8 bit value.
                          int lcol);            // In: label color. Must be a 8 bit value.

// Converts a run length coded labeled image to a grey level image.
int YaIPS_RGB_RLC_Decode( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                          Timages *iRLC1,       // In: Image with run length codes
                          int label,            // In: Minimum area of an object to be labeled
                          int bcol,             // In: background color. Must be a 8 bit value.
                          int lcol);            // In: label color. Must be a 8 bit value.

// Measure brightness of labeled objects
int YaIPS_RGB_RLC_GetBright( Tvector *vObj,           // In Out: vector with extracted objects
                             Timages *iRLC1,          // In: Point to image with run length codes
                             Fl_RGB_Image *pSrc);     // In: Source image

//--------------------------------------------------------------------------
// YaIPS_RGB_ShapeGen.cpp
//--------------------------------------------------------------------------

// Shape types

#define YAIPS_SHAPE_GEN_TYPE_NONE            0  // No shape generation
#define YAIPS_SHAPE_GEN_TYPE_RECTANGLE       1  // Rectangle
#define YAIPS_SHAPE_GEN_TYPE_SQUARE          2  // Square
#define YAIPS_SHAPE_GEN_TYPE_ELLIPSE         3  // Ellipse
#define YAIPS_SHAPE_GEN_TYPE_CIRCLE          4  // Circle
#define YAIPS_SHAPE_GEN_TYPE_TRIANGLE        5  // Triangle
#define YAIPS_SHAPE_GEN_TYPE_RECT_ROUNDED   10  // Rectangle with rounded corner, argument is corner radius
#define YAIPS_SHAPE_GEN_TYPE_SQUAE_ROUNDED  11  // Square with rounded corner, argument is corner radius
#define YAIPS_SHAPE_GEN_TYPE_PARALLEOGRAM   12  // Parallelogram, argument is slant
#define YAIPS_SHAPE_GEN_TYPE_TRAPEZ         13  // Trapez, argument is slant
#define YAIPS_SHAPE_GEN_TYPE_DIAMOND        14  // Diamond
#define YAIPS_SHAPE_GEN_TYPE_HEART          15  // Heart
#define YAIPS_SHAPE_GEN_TYPE_STAR_4         20  // Star with 4 points, argument is sharpness
#define YAIPS_SHAPE_GEN_TYPE_STAR_5         21  // Star with 5 points, argument is sharpness
#define YAIPS_SHAPE_GEN_TYPE_STAR_6         22  // Star with 6 points, argument is sharpness
#define YAIPS_SHAPE_GEN_TYPE_STAR_8         23  // Star with 8 points, argument is sharpness
#define YAIPS_SHAPE_GEN_TYPE_STAR_12        24  // Star with 12 points, argument is sharpness
#define YAIPS_SHAPE_GEN_TYPE_STAR_24        25  // Star with 24 points, argument is sharpness
#define YAIPS_SHAPE_GEN_TYPE_ARROW_LEFT     30  // Arrow pointing left
#define YAIPS_SHAPE_GEN_TYPE_ARROW_RIGHT    31  // Arrow pointing right
#define YAIPS_SHAPE_GEN_TYPE_ARROW_UP       32  // Arrow pointing up
#define YAIPS_SHAPE_GEN_TYPE_ARROW_DOWN     33  // Arrow pointing down

// Background types

#define YAIPS_SHAPE_GEN_BGND_COLOR           0  // Background: color
#define YAIPS_SHAPE_GEN_BGND_IMAGE           1  // Background: Image loaded from file
#define YAIPS_SHAPE_GEN_BGND_WINDOW          2  // Background: Tool windows

// Shape flags

// Corner use bits for background color
#define YAIPS_SHAPE_GEN_FLAG_CBIT_MASK YAIPS_SETVAL_CORNER_BIT_MASK     // Corner bit all
#define YAIPS_SHAPE_GEN_FLAG_CBIT_LT   YAIPS_SETVAL_CORNER_BIT_LT       // Corner bit left top
#define YAIPS_SHAPE_GEN_FLAG_CBIT_RT   YAIPS_SETVAL_CORNER_BIT_RT       // Corner bit right top
#define YAIPS_SHAPE_GEN_FLAG_CBIT_LB   YAIPS_SETVAL_CORNER_BIT_LB       // Corner bit left bottom
#define YAIPS_SHAPE_GEN_FLAG_CBIT_RB   YAIPS_SETVAL_CORNER_BIT_RB       // Corner bit right bottom

#define YAIPS_SHAPE_GEN_FLAG_DRAW_SHAPE         0x00000010     // Draw shape
#define YAIPS_SHAPE_GEN_FLAG_DRAW_LINE          0x00000020     // Draw lines around the shape
#define YAIPS_SHAPE_GEN_FLAG_LINE_COL_USE       0x00000040     // Use the line color to draw the shape outline
#define YAIPS_SHAPE_GEN_FLAG_FONT_COL_USE       0x00000080     // Use the text color to color the characters
#define YAIPS_SHAPE_GEN_FLAG_FONT_BOLD_ON       0x00000100     // Use bold variant of font
#define YAIPS_SHAPE_GEN_FLAG_FONT_ITALIC_ON     0x00000200     // Use italic variant of font
#define YAIPS_SHAPE_GEN_FLAG_FONT_UNDERL_ON     0x00000400     // Use underline

#define YAIPS_SHAPE_GEN_FLAG_AOI_SIZE_RATIO     0x00001000     // If set, AOI size ratio is locked

#define YAIPS_SHAPE_GEN_FLAG_SHADOW_USE         0x00010000     // Use shadow
#define YAIPS_SHAPE_GEN_FLAG_SHADOW_COL_USE     0x00020000     // Use the shadow color, else use the image part as shadow

// Bits for horizontal align
#define YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_MASK     0x00300000     // Horizontal align bit mask
#define YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_SHIFT            20     // Horizontal align bit shift
#define YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_LEFT     0x00000000     // Horizontal align to left side
#define YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_CENTER   0x00100000     // Horizontal align to center
#define YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_RIGHT    0x00200000     // Horizontal align to right side

// Bits for vertical align
#define YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_MASK     0x00c00000     // Vertical align bit mask
#define YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_SHIFT            22     // Vertical align bit shift
#define YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_TOP      0x00000000     // Vertical align to top side
#define YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_CENTER   0x00400000     // Vertical align to center
#define YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_BOTTOM   0x00800000     // Vertical align to bottom side

// List of shapes with feature settings

typedef struct {

  char *pName;            // Name of shape
  int  Type;              // Type of shape
  int  ListFlags;         // Specific list flags
  int  ShapeArgDef;       // Shape argument default value
  int  ShapeArgMin;       // Shape argument minimum/maximum value
  int  ShapeArgMax;       // If both are 0 arguments is not used
  int  LineStyle;         // Specific line style for outline

} YaIPS_RGB_ShapeGen_List_t;

extern YaIPS_RGB_ShapeGen_List_t ShapeGen_List[];   // Pointer to shape list
extern int nShapeGen_List;                          // Size of shape list

// Parameter for shape generation.

typedef struct {

  // General parameters

  float RotAngle;         // orientation angle
  float AlphaMult;        // Alpha multiplier: 0.0 ... 100.0.

  int ShapeFlags;         // Flag bits

  int AOI_XX_Locked, AOI_YY_Locked;     // Locked AOI size ratio. See flag: YAIPS_SHAPE_GEN_FLAG_AOI_SIZE_RATIO

  // Background

  int BGndType;           // Background type, see #defines YAIPS_SHAPE_GEN_BGND_XXX

  // YAIPS_SHAPE_GEN_BGND_COLOR
  unsigned int BGndCol_LT;   // Value for left top image corner
  unsigned int BGndCol_RT;   // Value for right top image corner
  unsigned int BGndCol_LB;   // Value for left bottom image corner
  unsigned int BGndCol_RB;   // Value for right bottom image corner

  // YAIPS_SHAPE_GEN_BGND_IMAGE
  char BGndFileName[ FILENAME_MAX]; // File name of last loaded image file. This is inclusive path and file extension.

  // YAIPS_SHAPE_GEN_BGND_WINDOW
  int BGnd_WinIdNr;          // Window ID nr for background
  int BGnd_Win_Change;       // Background window has changed

  // Shape drawing
  int ShapeType;          // Shape type, see #defines YAIPS_SHAPE_GEN_TYPE_XXX
  float ShapeArg;         // Argument for shape

  // Line drawing
  int LineType;           // Line type
  unsigned int LineColor; // Line color
  int LineWidth;          // Width of line

  // Text drawing
  char FontName[ YAIPS_FONT_NAME_SIZE];  // Base name of font
  int  FontStyle;                        // 0 = regular, 1 = bold, 2 = italic, 3 = bold_italic. See FL_BOLD, FL_ITALIC and FL_BOLD_ITALIC defines.
  int  FontSize;                         // Size of font
  unsigned int FontColor;                // Font color
  int  IndentHor;                        // Horizontal indent. Can be negative for center align.
  int  IndentVer;                        // Vertical indent. Can be negative for center align.
  int  SpacingLine;                      // Additional space between lines
  int  SpacingChar;                      // Additional space between characters

  // Shadow

  float ShadowAngle;                     // Angle of shadow
  int   ShadowDist;                      // Distance of shadow
  unsigned int ShadowColor;              // Shadow color
  int   ShadowBlur;                      // Shadow blur, 0 = blur 0ff
  int   ShadowTrans;                     // Set the shadow's transparency using a value between 0% (opaque) and 100% (transparent).

} YaIPS_RGB_ShapeGen_Par_t;

// Overlay work data

typedef struct {

  // Parameter

  char Name[ 64];                       // Name of overlay
  int Type;                             // Type of overlay
  Fl_YaIPS_AOI_t AOI;                   // Aoi in pixel

  float PosX, PosY, SizeX, SizeY;       // Aoi in units

  int   AoiUnit;                        // Calibration unit 0 = pixel, 1 = mm, 2 = cm ...
                                        // Used for Position and size above.
                                        // Used to check of global calibration unit change.

  // Shape parameter

  YaIPS_RGB_ShapeGen_Par_t ShapeGen;

  // Used during work

  int ParChanged;                       // IF != 0, any parameter has changed --> recreate the image
  Fl_RGB_Image *pImgOverlay;            // Intermediate image used during drawing
  Fl_RGB_Image *pImgShadow;             // If != NULL, intermediate shadow image used during drawing
  Fl_RGB_Image *pBGndFile;              // Loaded background image file
  Fl_Text_Buffer *pTextBuffer;          // Holds text to display inside the overlay

} YaIPS_OverlayData_t;

// Return number of bytes for an UTF8 coded character
int utf8_decode( char *s, uint32_t *out_codepoint = NULL);

// Generate an image with a shape
int YaIPS_RGB_ShapeGen( Fl_RGB_Image **ppDst,                // Out: Pointer to pointer to RGB color image
                        int xx, int yy,                      // In: Size of image.
                        YaIPS_RGB_ShapeGen_Par_t *pShapeGen, // In: shape generation parameter
                        Fl_RGB_Image **ppShadow,             // Out: Pointer to pointer to RGB shadow image
                        Fl_RGB_Image **ppBGndFile,           // For background type YAIPS_SHAPE_GEN_BGND_IMAGE: Loaded background image file
                        Fl_Text_Buffer *pTextBuffer);        // Holds text to display inside the overlay


// Overlay image
int YaIPS_RGB_Overlay( Fl_RGB_Image *pDst,               // Pointer to RGB color image. Must exist and may have an alpha channel.
                       YaIPS_OverlayData_t *pOverlay);   // Point to overlay data

// Replace an alpha channel by an image

#define YAIPS_ALPHA_REPLEACE_OP_SET  0     // Alpha operator: Set alpha of shape
#define YAIPS_ALPHA_REPLEACE_OP_MIN  1     // Alpha operator: Set minimum of alpha in pDst image and shape

int YaIPS_RGB_Alpha_Replace( Fl_RGB_Image *pDst,       // Pointer to RGB image. Must exist and must have an alpha channel.
                             int ShapeType,            // Shape type, see #defines YAIPS_SHAPE_GEN_TYPE_XXX
                             float ShapeArg,           // Argument for shape
                             float AlphaMult = 100.0,  // Alpha multiplier %, range 0.0 .. 100.0.
                             int   AlphaOp = 0);       // Alpha operator

//--------------------------------------------------------------------------
// YaIPS_RGB_Sharpness.cpp
//--------------------------------------------------------------------------

// Image sharpness filter
int YaIPS_RGB_Sharpness( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                         Fl_RGB_Image *pSrc,   // Source image
                         int Radius,           // The number of pixels that are taken into account around each edge.
                         int StrengthPercent,  // The amount by which the contrast of the pixels in the image is increased [%].
                         int Difference);      // The brightness difference between neighboring pixels so that they are adjusted.

//--------------------------------------------------------------------------
// YaIPS_RGB_Sobel3x3.cpp
//--------------------------------------------------------------------------

int YaIPS_RGB_Sobel3x3( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                        Fl_RGB_Image *pSrc,   // Source image
                        float ResMultArg);    // Result multiplier

#endif /* YAIPS_RGB_INTERFACE_H_ */

/******************************** End Of File ********************************/
