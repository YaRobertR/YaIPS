/****************************************************************************

  YaIPS_RGB_Utils.cpp

  Fl_RGB_Image image processing.
  Some clue code to interface RGB ('Fl_RGB_Image') code.

 11.04.2025 RR: First edition of this file.

*****************************************************************************
*/

#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <math.h>

#include <FL/Fl.H>
#include <FL/Fl_Image.H>

// ...

#include "YaIPS_RGB_Interface.h"

/***************************************************************************
* YaIPS_RGB_to_ImgD
*
* Convert Fl_RGB_Image to image data used for RGB image processing.
*
*  return:   0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_to_ImgD( Fl_RGB_Image *pRGB,      // Point to RGB image
                       YaIPS_RGB_ImgD_t *pImgD)  // Point to RGB image data
{

  if( pImgD == NULL) {                         // Security test

    return( -100);
  }

  memset( pImgD, 0, sizeof( YaIPS_RGB_ImgD_t)); // Zero memory

  if( pRGB == NULL) {                          // Security test

    return( -101);
  }

  // Get image data
  pImgD->d  = pRGB->d();
  pImgD->xx = pRGB->data_w();
  pImgD->yy = pRGB->data_h();
  pImgD->ld = pRGB->ld() ? pRGB->ld() : pImgD->xx * pImgD->d;
  pImgD->pD = (uchar *)pRGB->data()[ 0];

  if( pImgD->d < 1 || pImgD->d > 4) {          // Check for reasonable size

    return( -102);
  }

  return( 0);                                  // Return OK
}

/***************************************************************************
* YaIPS_RGB_EnsureSameSize
*
* Ensure that pPDst image has the same size and same bytes per pixel
* as the pSrc image.
* If pDst is NULL image memory is allocated else
* if sizes are different, free old and allocate new memory.
*
* ppDst      Pointer to pointer to RGB image
* pSrc       Source image
*
*  return:   0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_EnsureSameSize( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                              Fl_RGB_Image *pSrc)      // In:  Source image
{
  Fl_RGB_Image *pDst;
  uchar *pDataDst;
  int SizeInBytes;

  if( ppDst == NULL) {                         // Security test
    return( -110);
  }

  if( pSrc == NULL) {                          // Security test
    return( -111);
  }

  pDst = *ppDst;                               // Get pointer to destination image

  if( pDst == NULL ||                          // Have NO image
      pSrc->data_w() != pDst->data_w() ||      // Something different
      pSrc->data_h() != pDst->data_h() ||
      pSrc->d() != pDst->d()) {

    if( pDst) {                                // Have an image

      pDst->release();                         // Release old memory
    }

    // Allocate memory for image

    SizeInBytes = pSrc->data_w() * pSrc->data_h() * pSrc->d();

    pDataDst = new uchar[ SizeInBytes];

    if( pDataDst == NULL) {                    // Security test

      return( -115);
    }

    // Create image
    pDst = new Fl_RGB_Image( pDataDst, pSrc->data_w(), pSrc->data_h(), pSrc->d());

    if( pDst == NULL) {                        // Security test

      return( -116);
    }

    pDst->alloc_array = 1;                     // Flag, data is allocated

    *ppDst = pDst;                             // Set pointer to destination image

  }

  return( 0);                                  // Return OK
}

/***************************************************************************
* YaIPS_RGB_ImageSetSize
*
* Set size and bytes per pixel of a RGB image.
* If pDst is NULL image memory is allocated else
* if sizes are different, free old and allocate new memory.
*
* ppDst      Pointer to pointer to RGB image
* xx         In: Width of image
* yy         In: Height of image
* d          In: Bytes per pixel (1..4)
*
*  return:   0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_ImageSetSize( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                            int xx,                  // In: Width of image
                            int yy,                  // In: Height of image
                            int d)                   // In: Bytes per pixel (1..4)
{
  Fl_RGB_Image *pDst;
  uchar *pDataDst;
  int SizeInBytes;

  if( ppDst == NULL) {                         // Security test
    return( -110);
  }

  if( d < 1 || d > 4) {                        // Check range of bytes per pixel

    return( -111);
  }

  pDst = *ppDst;                               // Get pointer to destination image

  if( pDst == NULL ||                          // Have NO image
      xx != pDst->data_w() ||                  // Something different
      yy != pDst->data_h() ||
      d != pDst->d()) {

    if( pDst) {                                // Have an image

      pDst->release();                         // Release old memory
    }

    // Allocate memory for image

    SizeInBytes = xx * yy * d;

    pDataDst = new uchar[ SizeInBytes];

    if( pDataDst == NULL) {                    // Security test

      return( -115);
    }

    // Create image
    pDst = new Fl_RGB_Image( pDataDst, xx, yy, d);

    if( pDst == NULL) {                        // Security test

      return( -115);
    }

    pDst->alloc_array = 1;                     // Flag, data is allocated

    *ppDst = pDst;                             // Set pointer to destination image

  }

  return( 0);                                  // Return OK
}

/***************************************************************************
* YaIPS_RGB_EnsureMinSize2
*
* Ensure that pPDst image has the minimum size of the source images.
* Sources must have the same bytes per pixel.
* If pDst is NULL image memory is allocated else
* if sizes are different, free old and allocate new memory.
*
* ppDst      Pointer to pointer to RGB image
* pSrc1      1. source image
* pSrc2      2. source image
*
*  return:   0 OK
*          < 0 Error
****************************************************************************
*/

// Ensure that pPDst image has the minimum size of the source images. Sources must have the same bytes per pixel.
int YaIPS_RGB_EnsureMinSize2( Fl_RGB_Image **ppDst,    // Out: Pointer to pointer to RGB image
                              Fl_RGB_Image *pSrc1,     // In:  1. source image
                              Fl_RGB_Image *pSrc2)     // In:  2. ource image
{
  Fl_RGB_Image *pDst;
  uchar *pDataDst;
  int SizeInBytes, MinXX, MinYY;

  if( ppDst == NULL) {                         // Security test
    return( -110);
  }

  if( pSrc1 == NULL) {                         // Security test
    return( -111);
  }

  if( pSrc2 == NULL) {                         // Security test
    return( -111);
  }

  // Sources must have the same bytes per pixel

  if( pSrc1->d() != pSrc2->d()) {             // Must have same bytes per pixel

    if( errstring == NULL) {                  // No error until now

      sprintf( errbuffer, LangStringLookup( "&RGB_Utils_Txt1=Bytes/pixel: %d != %d"), pSrc1->d(), pSrc2->d());

      errstring = errbuffer;
    }

    return( -112);
  }

  // Get minimum sizes

  if( pSrc1->data_w() < pSrc2->data_w()) {
    MinXX = pSrc1->data_w();
  } else {
    MinXX = pSrc2->data_w();
  }

  if( pSrc1->data_h() < pSrc2->data_h()) {
    MinYY = pSrc1->data_h();
  } else {
    MinYY = pSrc2->data_h();
  }

  // ...

  pDst = *ppDst;                               // Get pointer to destination image

  if( pDst == NULL ||                          // Have NO image
      MinXX != pDst->data_w() ||               // Something different
      MinYY != pDst->data_h() ||
      pSrc1->d() != pDst->d()) {

    if( pDst) {                                // Have an image

      pDst->release();                         // Release old memory
    }

    // Allocate memory for image

    SizeInBytes = MinXX * MinYY * pSrc1->d();

    pDataDst = new uchar[ SizeInBytes];

    if( pDataDst == NULL) {                    // Security test

      return( -115);
    }

    // Create image
    pDst = new Fl_RGB_Image( pDataDst, MinXX, MinYY, pSrc1->d());

    if( pDst == NULL) {                        // Security test

      return( -115);
    }

    pDst->alloc_array = 1;                     // Flag, data is allocated

    *ppDst = pDst;                             // Set pointer to destination image

  }

  return( 0);                                  // Return OK
}

/***************************************************************************
* YaIPS_RGB_xx2min
*
* Returns the smaller xx-size of the 2 images
*
****************************************************************************
*/

int YaIPS_RGB_xx2min( YaIPS_RGB_ImgD_t *pImgD1,  // Point to ImgD image
                      YaIPS_RGB_ImgD_t *pImgD2)  // Point to ImgD image
{
  int TempVal;

  TempVal = pImgD1->xx;

  if( TempVal > pImgD2->xx) {

    TempVal = pImgD2->xx;
  }

  return( TempVal);
}

/***************************************************************************
* YaIPS_RGB_yy2min
*
* Returns the smaller xx-size of the 2 images
*
****************************************************************************
*/

int YaIPS_RGB_yy2min( YaIPS_RGB_ImgD_t *pImgD1,  // Point to ImgD image
                      YaIPS_RGB_ImgD_t *pImgD2)  // Point to ImgD image
{
  int TempVal;

  TempVal = pImgD1->yy;

  if( TempVal > pImgD2->yy) {

    TempVal = pImgD2->yy;
  }

  return( TempVal);
}

/***************************************************************************
* YaIPS_RGB_xx3min
*
* Returns the smaller xx-size of the 3 images
*
****************************************************************************
*/

int YaIPS_RGB_xx3min( YaIPS_RGB_ImgD_t *pImgD1,  // Point to ImgD image
                      YaIPS_RGB_ImgD_t *pImgD2,  // Point to ImgD image
                      YaIPS_RGB_ImgD_t *pImgD3)  // Point to ImgD image
{
  int TempVal;

  TempVal = pImgD1->xx;

  if( TempVal > pImgD2->xx) {

    TempVal = pImgD2->xx;
  }

  if( TempVal > pImgD3->xx) {

    TempVal = pImgD3->xx;
  }

  return( TempVal);
}

/***************************************************************************
* YaIPS_RGB_yy3min
*
* Returns the smaller xx-size of the 3 images
*
****************************************************************************
*/

int YaIPS_RGB_yy3min( YaIPS_RGB_ImgD_t *pImgD1,  // Point to ImgD image
                      YaIPS_RGB_ImgD_t *pImgD2,  // Point to ImgD image
                      YaIPS_RGB_ImgD_t *pImgD3)  // Point to ImgD image
{
  int TempVal;

  TempVal = pImgD1->yy;

  if( TempVal > pImgD2->yy) {

    TempVal = pImgD2->yy;
  }

  if( TempVal > pImgD3->yy) {

    TempVal = pImgD3->yy;
  }

  return( TempVal);
}

/***************************************************************************
* YaIPS_ImgD_AOI
*
* Set AOI of an imgD image descriptor
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/
int YaIPS_ImgD_AOI( YaIPS_RGB_ImgD_t *pImgA,       // Out: AOI image
                    YaIPS_RGB_ImgD_t *pImgR,       // In: Root image
                    int x, int y, int xx, int yy)  // In: AOI
{
  if( pImgA == NULL) {               // Security test
    return( -1);
  }

  if( pImgR == NULL) {               // Security test
    return( -2);
  }

  if( xx < 1 || xx > pImgR->xx) {    // Security test
    return( -3);
  }

  if( yy < 1 || yy > pImgR->yy) {    // Security test
    return( -4);
  }

  if( x < 0 || x + xx > pImgR->xx) { // Security test
    return( -5);
  }

  if( y < 0 || y + yy > pImgR->yy) { // Security test
    return( -5);
  }

  if( pImgA != pImgR) {         // Source and destination are different

    memcpy( pImgA, pImgR, sizeof( YaIPS_RGB_ImgD_t));  // Copy data first
  }

  // Set AOI

  pImgA->pD += (y * pImgR->ld) + (x * pImgR->d);

  pImgA->xx = xx;
  pImgA->yy = yy;

  return( 0);                        // return OK
}

/***************************************************************************
* YaIPS_ImgD_AOI
*
* Set AOI of an imgD image descriptor
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/
int YaIPS_ImgD_AOI( YaIPS_RGB_ImgD_t *pImgA,       // Out: AOI image
                    YaIPS_RGB_ImgD_t *pImgR,       // In: Root image
                    Fl_YaIPS_AOI_t *pAOI)          // In: AOI

{
  int ierr;

  ierr = YaIPS_ImgD_AOI( pImgA, pImgR, pAOI->XPos, pAOI->YPos, pAOI->XSize, pAOI->YSize);

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_Color2Val
*
* Convert FLTK color to integer value.
* The value holds the convert color depending from the 'bytes per pixel'.
*
*     nBytes   Value out
*          1        000B   Black and white
*          2        00AB   Black and white with alpha
*          3        0BGR   Color
*          4        ABGR   Color with alpha
*
*     0 = zero, B = black and white conversion of color,
*     BGR = blue green red color component, A = alpha.
*
* nBytes     # bytes per pixel. Valid range is 1 .. 4.
* Color      FLTK Color
* Alpha      Alpha value. Is only used for pixels with alpha.
*
* return     Converter value
*
****************************************************************************
*/

int YaIPS_RGB_Color2Val( int nBytes,          // # bytes per pixel
                         Fl_Color Color,      // FLTK Color
                         int Alpha)           // Alpha value
{
  uchar r,g,b;
  int Value;

  if( nBytes != 2 &&                          // Pixel has no alpha value
      nBytes != 4) {

    Alpha = 0;                                // Reset alpha to zero

  } else {                                    // Pixel with alpha

    if( Alpha < 0) {                          // Clip to lower range

      Alpha = 0;
    }

    if( Alpha > 255) {                        // Clip to higher range

      Alpha = 255;
    }
  }

  Fl::get_color( Color, r, g, b);             // Convert color to RGB values

  if( nBytes >= 3) {                          // Is a color pixel

    Value = r;                                // Lowest byte
    Value |= g << 8;
    Value |= b << 16;
    Value |= (Alpha & 0xff) << 24;            // Alpha is highest byte

  } else {                                    // Black and white pixel

    Value = (r * 76 + g * 150 + b * 30) >> 8; // Lowest byte, a fast black white conversion.
    Value |= (Alpha & 0xff) << 8;             // Alpha is next byte
  }

  return( Value);                             // Return the converted color
}

/***************************************************************************
* YaIPS_RGB_SetVal
* Set all image pixels to value
*
* pImgD      pointer to YaIPS_RGB_ImgD_t image
* value      value to set
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_SetVal( YaIPS_RGB_ImgD_t *pImgD,  // Destination image
                      int value)                // value to set
{
  int x, y;
  int xmin, ymin, iByte, nByte;
  uchar *s8;
  uchar v4[ 4];

  if( pImgD == NULL)  {                         // Security test

    return( -1);
  }

  xmin  = pImgD->xx;
  ymin  = pImgD->yy;
  //x/jump  = pImgD->ld;
  nByte = pImgD->d;

  v4[ 0] = value & 0xff;
  v4[ 1] = (value >>  8) & 0xff;
  v4[ 2] = (value >> 16) & 0xff;
  v4[ 3] = (value >> 24) & 0xff;

  for( y = 0; y < ymin; y++) {

    s8 = RGB_pixad( 0, y, pImgD);

    for( x = 0; x < xmin; x++) {

      for( iByte = 0; iByte < nByte; iByte++) {
         *(s8 + iByte) = v4[ iByte];
       }

       s8 += nByte;
     }
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_SetVal
* Set all image pixels to value
*
* pDst       pointer to Fl_RGB_Image image
* value      value to set
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_SetVal( Fl_RGB_Image *pDst,  // Destination image
                      int value)           // value to set
{
  int ierr;
  YaIPS_RGB_ImgD_t ImgD;

  ierr = YaIPS_RGB_to_ImgD( pDst, &ImgD);

  if( ierr != 0)  {                           // Check for error

    return( ierr);
  }

  ierr = YaIPS_RGB_SetVal( &ImgD, value);

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_SetVal
* Fill image with an integer value for each image corner.
* Interpolate color between corners.
*
* pImgD          pointer to YaIPS_RGB_ImgD_t image
* Value_LT       Value for left top image corner
* Value_RT       Value for right top image corner
* Value_LB       Value for left bottom image corner
* Value_RB       Value for right bottom image corner
* CornerBits     On bit for each corner to use
*                LT = 0x08, RT = 0x04, LB = 0x02, RB = 0x01,
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_SetVal( YaIPS_RGB_ImgD_t *pImgD,  // Destination image
                      int Value_LT,             // Value for left top image corner
                      int Value_RT,             // Value for right top image corner
                      int Value_LB,             // Value for left bottom image corner
                      int Value_RB,             // Value for right bottom image corner
                      int CornerBits)           // On bit for each corner to use
{
  int x, y;
  int xmin, ymin, iByte, nByte, TempInt;
  uchar *s8;
  uchar v4_LT[ 4], v4_RT[ 4], v4_LB[ 4], v4_RB[ 4], v4_Left[ 4], v4_Right[ 4];

  if( pImgD == NULL)  {                         // Security test

    return( -1);
  }

  xmin  = pImgD->xx;
  ymin  = pImgD->yy;
  //x/jump  = pImgD->ld;
  nByte = pImgD->d;

  v4_LT[ 0] = Value_LT & 0xff;
  v4_LT[ 1] = (Value_LT >>  8) & 0xff;
  v4_LT[ 2] = (Value_LT >> 16) & 0xff;
  v4_LT[ 3] = (Value_LT >> 24) & 0xff;

  v4_RT[ 0] = Value_RT & 0xff;
  v4_RT[ 1] = (Value_RT >>  8) & 0xff;
  v4_RT[ 2] = (Value_RT >> 16) & 0xff;
  v4_RT[ 3] = (Value_RT >> 24) & 0xff;

  v4_LB[ 0] = Value_LB & 0xff;
  v4_LB[ 1] = (Value_LB >>  8) & 0xff;
  v4_LB[ 2] = (Value_LB >> 16) & 0xff;
  v4_LB[ 3] = (Value_LB >> 24) & 0xff;

  v4_RB[ 0] = Value_RB & 0xff;
  v4_RB[ 1] = (Value_RB >>  8) & 0xff;
  v4_RB[ 2] = (Value_RB >> 16) & 0xff;
  v4_RB[ 3] = (Value_RB >> 24) & 0xff;

  switch( CornerBits & 0x0f) {

  case 0: // No corner bit set

    CornerBits = 0x08;               // Default to left top corner

    if( nByte >= 0) {       // Color image

      v4_LT[ 0] = 0;        // Black
      v4_LT[ 1] = 0;
      v4_LT[ 2] = 0;
      //x/ v4_LT[ 3] = 0xff;     // Alpha value

    } else {                // Black and white image

      v4_LT[ 0] = 0;        // Black
      //x/v4_LT[ 1] = 0xff;     // Alpha value
      v4_LT[ 2] = 0;
      v4_LT[ 3] = 0;
    }
    break;

  case 0x04:    // only right top used

    CornerBits = 0x08;      // Set to left top corner

    v4_LT[ 0] = v4_RT[ 0];  // Copy to left top color value
    v4_LT[ 1] = v4_RT[ 1];
    v4_LT[ 2] = v4_RT[ 2];
    v4_LT[ 3] = v4_RT[ 3];
    break;

  case 0x02:    // only left bottom used

    CornerBits = 0x08;      // Set to left top corner

    v4_LT[ 0] = v4_LB[ 0];  // Copy to left top color value
    v4_LT[ 1] = v4_LB[ 1];
    v4_LT[ 2] = v4_LB[ 2];
    v4_LT[ 3] = v4_LB[ 3];
    break;

  case 0x01:    // only right bottom used

    CornerBits = 0x08;      // Set to left top corner

    v4_LT[ 0] = v4_RB[ 0];
    v4_LT[ 1] = v4_RB[ 1];
    v4_LT[ 2] = v4_RB[ 2];
    v4_LT[ 3] = v4_RB[ 3];
    break;

  case 0x03:  // Corner bits LB RB

    v4_LT[ 0] = v4_LB[ 0];
    v4_LT[ 1] = v4_LB[ 1];
    v4_LT[ 2] = v4_LB[ 2];
    v4_LT[ 3] = v4_LB[ 3];

    v4_RT[ 0] = v4_RB[ 0];
    v4_RT[ 1] = v4_RB[ 1];
    v4_RT[ 2] = v4_RB[ 2];
    v4_RT[ 3] = v4_RB[ 3];
    break;

  case 0x05:  // Corner bits RT RB

    v4_LT[ 0] = v4_RT[ 0];
    v4_LT[ 1] = v4_RT[ 1];
    v4_LT[ 2] = v4_RT[ 2];
    v4_LT[ 3] = v4_RT[ 3];

    v4_LB[ 0] = v4_RB[ 0];
    v4_LB[ 1] = v4_RB[ 1];
    v4_LB[ 2] = v4_RB[ 2];
    v4_LB[ 3] = v4_RB[ 3];
    break;

  case 0x06:  // Corner bits RT LB

    v4_LT[ 0] = (v4_RT[ 0] + v4_LB[ 0] + 1) / 2;
    v4_LT[ 1] = (v4_RT[ 1] + v4_LB[ 1] + 1) / 2;
    v4_LT[ 2] = (v4_RT[ 2] + v4_LB[ 2] + 1) / 2;
    v4_LT[ 3] = (v4_RT[ 3] + v4_LB[ 3] + 1) / 2;

    v4_RB[ 0] = v4_LT[ 0];
    v4_RB[ 1] = v4_LT[ 1];
    v4_RB[ 2] = v4_LT[ 2];
    v4_RB[ 3] = v4_LT[ 3];
    break;

  case 0x07:  // Corner bits RT LB RB

    v4_LT[ 0] = (v4_LB[ 0] + v4_RT[ 0] + 1) / 2;
    v4_LT[ 1] = (v4_LB[ 1] + v4_RT[ 1] + 1) / 2;
    v4_LT[ 2] = (v4_LB[ 2] + v4_RT[ 2] + 1) / 2;
    v4_LT[ 3] = (v4_LB[ 3] + v4_RT[ 3] + 1) / 2;
    break;

  case 0x09:  // Corner bits LT RB

    v4_RT[ 0] = (v4_LT[ 0] + v4_RB[ 0] + 1) / 2;
    v4_RT[ 1] = (v4_LT[ 1] + v4_RB[ 1] + 1) / 2;
    v4_RT[ 2] = (v4_LT[ 2] + v4_RB[ 2] + 1) / 2;
    v4_RT[ 3] = (v4_LT[ 3] + v4_RB[ 3] + 1) / 2;

    v4_LB[ 0] = v4_RT[ 0];
    v4_LB[ 1] = v4_RT[ 1];
    v4_LB[ 2] = v4_RT[ 2];
    v4_LB[ 3] = v4_RT[ 3];
    break;

  case 0x0a:  // Corner bits LT LB

    v4_RT[ 0] = v4_LT[ 0];
    v4_RT[ 1] = v4_LT[ 1];
    v4_RT[ 2] = v4_LT[ 2];
    v4_RT[ 3] = v4_LT[ 3];

    v4_RB[ 0] = v4_LB[ 0];
    v4_RB[ 1] = v4_LB[ 1];
    v4_RB[ 2] = v4_LB[ 2];
    v4_RB[ 3] = v4_LB[ 3];
    break;

  case 0x0b:  // Corner bits LT LB RB

    v4_RT[ 0] = (v4_LT[ 0] + v4_RB[ 0] + 1) / 2;
    v4_RT[ 1] = (v4_LT[ 1] + v4_RB[ 1] + 1) / 2;
    v4_RT[ 2] = (v4_LT[ 2] + v4_RB[ 2] + 1) / 2;
    v4_RT[ 3] = (v4_LT[ 3] + v4_RB[ 3] + 1) / 2;
    break;

  case 0x0c:  // Corner bits LT RT

    v4_LB[ 0] = v4_LT[ 0];
    v4_LB[ 1] = v4_LT[ 1];
    v4_LB[ 2] = v4_LT[ 2];
    v4_LB[ 3] = v4_LT[ 3];

    v4_RB[ 0] = v4_RT[ 0];
    v4_RB[ 1] = v4_RT[ 1];
    v4_RB[ 2] = v4_RT[ 2];
    v4_RB[ 3] = v4_RT[ 3];
    break;

  case 0x0d:  // Corner bits LT RT RB

    v4_LB[ 0] = (v4_LT[ 0] + v4_RB[ 0] + 1) / 2;
    v4_LB[ 1] = (v4_LT[ 1] + v4_RB[ 1] + 1) / 2;
    v4_LB[ 2] = (v4_LT[ 2] + v4_RB[ 2] + 1) / 2;
    v4_LB[ 3] = (v4_LT[ 3] + v4_RB[ 3] + 1) / 2;
    break;

  case 0x0e:  // Corner bits LT RT LB

    v4_RB[ 0] = (v4_LB[ 0] + v4_RT[ 0] + 1) / 2;
    v4_RB[ 1] = (v4_LB[ 1] + v4_RT[ 1] + 1) / 2;
    v4_RB[ 2] = (v4_LB[ 2] + v4_RT[ 2] + 1) / 2;
    v4_RB[ 3] = (v4_LB[ 3] + v4_RT[ 3] + 1) / 2;
    break;

  case 0x0f:  // Corner bits all
  default:
    // Nothing to do here
    break;
  }

  if( (CornerBits & 0x0f) == 0x08) {   // No or one corner used

    // All single edge colors should end up here

    for( y = 0; y < ymin; y++) {

      s8 = RGB_pixad( 0, y, pImgD);

      for( x = 0; x < xmin; x++) {

        for( iByte = 0; iByte < nByte; iByte++) {
           *(s8 + iByte) = v4_LT[ iByte];
         }

         s8 += nByte;
       }
    }

  } else {                              // Two or more colors

    // Process a color for each corner

    for( y = 0; y < ymin; y++) {

      for( iByte = 0; iByte < nByte; iByte++) {

        TempInt = (v4_LT[ iByte] * (ymin - 1 - y) + v4_LB[ iByte] * y + ymin / 2) / (ymin - 1);
        if( TempInt > 255) TempInt = 255;
        v4_Left[ iByte] = TempInt;

        TempInt = (v4_RT[ iByte] * (ymin - 1 - y) + v4_RB[ iByte] * y + ymin / 2) / (ymin - 1);
        if( TempInt > 255) TempInt = 255;
        v4_Right[ iByte] = TempInt;
      }

      s8 = RGB_pixad( 0, y, pImgD);

      for( x = 0; x < xmin; x++) {

        for( iByte = 0; iByte < nByte; iByte++) {

          TempInt = (v4_Left[ iByte] * (xmin - 1 - x) + v4_Right[ iByte] * x + xmin / 2) / (xmin - 1);
          if( TempInt > 255) TempInt = 255;
           *(s8 + iByte) = TempInt;
         }

         s8 += nByte;
       }
    }
  }

  return( 0);                           // Return OK
}

/***************************************************************************
* YaIPS_RGB_SetVal
* Fill image with an integer value for each image corner.
* Interpolate color between corners.
*
* pDst           pointer to Fl_RGB_Image image
* Value_LT       Value for left top image corner
* Value_RT       Value for right top image corner
* Value_LB       Value for left bottom image corner
* Value_RB       Value for right bottom image corner
* CornerBits     On bit for each corner to use
*                LT = 0x08, RT = 0x04, LB = 0x02, RB = 0x01,
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_SetVal( Fl_RGB_Image *pDst,       // Destination image
                      int Value_LT,             // Value for left top image corner
                      int Value_RT,             // Value for right top image corner
                      int Value_LB,             // Value for left bottom image corner
                      int Value_RB,             // Value for right bottom image corner
                      int CornerBits)           // On bit for each corner to use
{
  int ierr;
  YaIPS_RGB_ImgD_t ImgD;

  ierr = YaIPS_RGB_to_ImgD( pDst, &ImgD);

  if( ierr != 0)  {                           // Check for error

    return( ierr);
  }

  ierr = YaIPS_RGB_SetVal( &ImgD, Value_LT, Value_RT, Value_LB, Value_RB, CornerBits);

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_SetColor
* Fill image with a color for each image corner.
* Interpolate color between corners.
*
* pImgD          pointer to YaIPS_RGB_ImgD_t image
* Color_LT       Color for left top image corner
* Color_RT       Color for right top image corner
* Color_LB       Color for left bottom image corner
* Color_RB       Color for right bottom image corner
* CornerBits     On bit for each corner to use
*                LT = 0x08, RT = 0x04, LB = 0x02, RB = 0x01
* Alpha          Optional alpha value. Depends from bytes per pixel.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_SetColor( YaIPS_RGB_ImgD_t *pImgD,  // Destination image
                        Fl_Color Color_LT,        // Color for left top image corner
                        Fl_Color Color_RT,        // Color for right top image corner
                        Fl_Color Color_LB,        // Color for left bottom image corner
                        Fl_Color Color_RB,        // Color for right bottom image corner
                        int CornerBits,           // On bit for each corner to use
                        int Alpha)                // Optional alpha value
{
  int ierr, Value_LT, Value_RT, Value_LB, Value_RB;

  if( pImgD == NULL)  {                         // Security test

    return( -1);
  }

  Value_LT = YaIPS_RGB_Color2Val( pImgD->d, Color_LT, Alpha);
  Value_RT = YaIPS_RGB_Color2Val( pImgD->d, Color_RT, Alpha);
  Value_LB = YaIPS_RGB_Color2Val( pImgD->d, Color_LB, Alpha);
  Value_RB = YaIPS_RGB_Color2Val( pImgD->d, Color_RB, Alpha);

  ierr = YaIPS_RGB_SetVal( pImgD, Value_LT, Value_RT, Value_LB, Value_RB, CornerBits);

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_SetColor
* Fill image with a color for each image corner.
* Interpolate color between corners.
*
* pDst           pointer to Fl_RGB_Image image
* Color_LT       Color for left top image corner
* Color_RT       Color for right top image corner
* Color_LB       Color for left bottom image corner
* Color_RB       Color for right bottom image corner
* CornerBits     On bit for each corner to use
*                LT = 0x08, RT = 0x04, LB = 0x02, RB = 0x01
* Alpha          Optional alpha value. Depends from bytes per pixel.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_SetColor( Fl_RGB_Image *pDst,       // Destination image
                        Fl_Color Color_LT,        // Color for left top image corner
                        Fl_Color Color_RT,        // Color for right top image corner
                        Fl_Color Color_LB,        // Color for left bottom image corner
                        Fl_Color Color_RB,        // Color for right bottom image corner
                        int CornerBits,           // On bit for each corner to use
                        int Alpha)                // Optional alpha value
{
  int ierr, Value_LT, Value_RT, Value_LB, Value_RB;
  YaIPS_RGB_ImgD_t ImgD;

  ierr = YaIPS_RGB_to_ImgD( pDst, &ImgD);

  if( ierr != 0)  {                           // Check for error

    return( ierr);
  }

  Value_LT = YaIPS_RGB_Color2Val( ImgD.d, Color_LT, Alpha);
  Value_RT = YaIPS_RGB_Color2Val( ImgD.d, Color_RT, Alpha);
  Value_LB = YaIPS_RGB_Color2Val( ImgD.d, Color_LB, Alpha);
  Value_RB = YaIPS_RGB_Color2Val( ImgD.d, Color_RB, Alpha);

  ierr = YaIPS_RGB_SetVal( &ImgD, Value_LT, Value_RT, Value_LB, Value_RB, CornerBits);

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_SetAlpha
* Set all image pixels alpha value
*
* pImgD      pointer to YaIPS_RGB_ImgD_t image
* Alpha      Alpha value to set
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_SetAlpha( YaIPS_RGB_ImgD_t *pImgD,  // Destination image
                        int Alpha)                // Alpha value to set
{
  int x, y;
  int xmin, ymin, iAlpha, nByte;
  uchar *s8;

  if( pImgD == NULL)  {                         // Security test

    return( -1);
  }

  xmin  = pImgD->xx;
  ymin  = pImgD->yy;
  //x/jump  = pImgD->ld;
  nByte = pImgD->d;

  if( nByte == 2) {                // Black and white with alpha

    iAlpha = 1;                    // Index for alpha byte part

  } else if( nByte == 4) {         // Color image

    iAlpha = 3;                    // Index for alpha byte part

  } else {                         // Image has no alpha

    return( 0);                    // Simply return OK
  }

  for( y = 0; y < ymin; y++) {

    s8 = RGB_pixad( 0, y, pImgD);

    for( x = 0; x < xmin; x++) {

      s8[ iAlpha] = Alpha;

      s8 += nByte;
    }
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_SetAlpha
* Set all image pixels alpha value
*
* pDst       pointer to Fl_RGB_Image image
* Alpha      Alpha value to set
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_SetAlpha( Fl_RGB_Image *pDst,       // Destination image
                        int Alpha)                // Alpha value to set
{
  int ierr;
  YaIPS_RGB_ImgD_t ImgD;

  ierr = YaIPS_RGB_to_ImgD( pDst, &ImgD);

  if( ierr != 0)  {                           // Check for error

    return( ierr);
  }

  ierr = YaIPS_RGB_SetAlpha( &ImgD, Alpha);

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_CopyImg
* Copy image
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_CopyImg( Fl_RGB_Image **ppDst, // Out: Pointer to pointer to RGB image
                       Fl_RGB_Image *pSrc)   // Source image
{
  int ierr, y, xx, yy, nByte, SizeOfLine;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Ensure that pPDst image has the same size and same pixel amount as pSrc
  ierr = YaIPS_RGB_EnsureSameSize( ppDst, pSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Source and destination has the same size and the same number of bytes per pixel

  xx    = iDst.xx;                            // All images have the same size
  yy    = iDst.yy;
  nByte = iDst.d;                             // All images have same bytes per pixel

  SizeOfLine = xx * nByte;                    // Size of one line of data

  for( y = 0; y < yy; y++) {

    d8 = RGB_pixad( 0,  y, &iDst);
    s8 = RGB_pixad( 0,  y, &iSrc);

    memcpy( d8, s8, SizeOfLine);
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_CopyInImg
* Copy smaller image into bigger.
*
* pDst         Destination image must exist
* pSrc         Source image must exist
* OffX, OffY   Left upper corner in destination image.
*
* NOTE: Until no, source image must fit into destination image.
* NOTE: Destination and source image must have the same number of bytes.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_CopyInImg( Fl_RGB_Image *pDst,   // Destination image must exist
                         Fl_RGB_Image *pSrc,   // Source image must exist
                         int OffX, int OffY)   // Offset in destination image
{
  int ierr, y, xx, yy, nByte, SizeOfLine;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar *s8, *d8;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Check number of bytes

  if( iDst.d != iSrc.d) {                    // Number of bytes is different

    return( -100);
  }

  // Check source fit into destination image

  if( OffX < 0 || OffY < 0 ||
      OffX + iSrc.xx > iDst.xx ||
      OffY + iSrc.yy > iDst.yy) {

    return( -101);
  }

  // Source and destination has the same size and the same number of bytes per pixel

  xx    = iSrc.xx;                            // All images have the same size
  yy    = iSrc.yy;
  nByte = iSrc.d;                             // All images have same bytes per pixel

  SizeOfLine = xx * nByte;                    // Size of one line of data

  for( y = 0; y < yy; y++) {

    d8 = RGB_pixad( OffX,  y + OffY, &iDst);
    s8 = RGB_pixad( 0,  y, &iSrc);

    memcpy( d8, s8, SizeOfLine);
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_RGB_CutOut
* Cut out a rectangular image part.
*
* ppDst        Pointer to pointer to RGB image
* pSrc         Source image
* pSrcAOI      Cut out this image part
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_CutOut( Fl_RGB_Image **ppDst,      // Out: Pointer to pointer to RGB image
                      Fl_RGB_Image *pSrc,        // Source image
                      Fl_YaIPS_AOI_t *pSrcAOI)   // Cut out this image part
{
  int ierr, y, xx, yy, nByte, SizeOfLine;
  Fl_RGB_Image *pDst;
  YaIPS_RGB_ImgD_t iDst, iSrc, iSrcAOI;
  uchar *s8, *d8;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Setup AOIs

  ierr = YaIPS_ImgD_AOI( &iSrcAOI, &iSrc, pSrcAOI);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Ensure that pPDst image has the same size and same pixel amount as pSrc
  ierr = YaIPS_RGB_ImageSetSize( ppDst, iSrcAOI.xx, iSrcAOI.yy, iSrcAOI.d);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  pDst = *ppDst;                              // Get pointer to destination image

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Source and destination has the same size and the same number of bytes per pixel

  xx    = iDst.xx;                            // All images have the same size
  yy    = iDst.yy;
  nByte = iDst.d;                             // All images have same bytes per pixel

  SizeOfLine = xx * nByte;                    // Size of one line of data

  for( y = 0; y < yy; y++) {

    d8 = RGB_pixad( 0,  y, &iDst);
    s8 = RGB_pixad( 0,  y, &iSrcAOI);

    memcpy( d8, s8, SizeOfLine);
  }

  return( 0);                                 // Return OK
}

/******************************** End Of File ********************************/


