/****************************************************************************

  YaIPS_RGB_Crcdf.cpp

  Fl_RGB_Image image processing.
  Cross difference function of images

 06.05.2025 RR: First edition of this file.

*****************************************************************************
*/

#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <FL/Fl.H>
#include <FL/Fl_Image.H>

// ...

#include "YaIPS_RGB_Interface.h"

/***************************************************************************
* Defines and structures
****************************************************************************
*/

#define YAIPS_BIMG_OBJ_ENLARGE    2    // Enlarge labeled objects after labeling
#define YAIPS_BIMG_OBJ_SEARCH     2    // Add search area to scene objects

#define YAIPS_BIMG_OBJ_TOL_PLUS   2    // Size tolerance
#define YAIPS_BIMG_OBJ_TOL_MINUS  2    // Size tolerance

#define YAIPS_BIMG_OBJ_BORDER_ADD (YAIPS_BIMG_OBJ_ENLARGE + YAIPS_BIMG_OBJ_SEARCH)    // Extra border to search for. Used for multiple objects in scene

/***************************************************************************
* Global variables
****************************************************************************
*/

static int *pabsDiff8Tab0 = (int *)NULL;
static int absDiff8Tab[511];

/***************************************************************************
* YaIPS_RGB_Crcdf_Calc
* cross difference function
*
* ppCorImg     Optional out: Correlation image.
* pScnImg      In: Scene image. Can be BW or RGB.
* ScnByte      In: What byte component to use from the scene image
* pObjImg      In: Object image. Must have 1 byte per pixel.
* pBestcor     Out: Best correlation result
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Crcdf_Calc( Fl_RGB_Image **ppCorImg,       // Optional out: Correlation image
                          YaIPS_RGB_ImgD_t *pScnImg,     // In: Scene image
                          int ScnByte,                   // In: What byte component to use from the scene image
                          YaIPS_RGB_ImgD_t *pObjImg,     // In: Object image. Must have 1 byte per pixel.
                          T_YaIPS_Res_Crcdf *pBestcor)   // Out: Best correlation result
{
  YaIPS_RGB_ImgD_t iCorImg;
  int ierr, sum, ox, sBytes;
  uchar *pSceneLine, *pScene, *pSceneObjLine, *pSceneObj;
  uchar *pObj, *pObjLine;
  int oy;
  int xmo, xms, xmc;
  int xxo, yyo, xxc, yyc;
  int bestsum, maxsum, objval, bestx, besty;
  int cy, cx;
  int *pCorrLine, *pCorr;

  // First preparations

  pBestcor->Valid   = false;        // Correlation results are NOT valid

  /* PRINTF0("FSV: gcrcdf32()\n"); */
  /*-------------- check input parameters of correlation function ----*/

  // Object image must have 1 byte per pixel

  if( pObjImg->d != 1) {                      // Must have same 1 byte per pixel

    if( errstring == NULL) {                  // No error until now

      sprintf( errbuffer, LangStringLookup( "&RGB_Crcdf_Err1=Object not a BW image!"));

      errstring = errbuffer;
    }

    return( -112);
  }

  // Check scene image bytes per pixel

  if( ScnByte < 0 || ScnByte >= pScnImg->d) {  // Check to byte per pixel

    if( errstring == NULL) {                   // No error until now

      sprintf( errbuffer, LangStringLookup( "&RGB_Crcdf_Err2=Bytes in scene %d < 0 or >= %d"), ScnByte, pScnImg->d);

      errstring = errbuffer;
    }

    return( -113);
  }

  // Check max size of object

  if( (pObjImg->xx * pObjImg->yy) > 0x007fffff) {

    if( errstring == NULL) {                  // No error until now

      errstring =  LangStringLookup( "&RGB_Crcdf_Err3=Object image too large");
      return(-20);
    }
  }

  // Object may not be larger than scene

  if (pObjImg->xx > pScnImg->xx || pObjImg->yy > pScnImg->yy) {

    if( errstring == NULL) {                  // No error until now

      errstring = LangStringLookup( "&RGB_Crcdf_Err4=Object larger than scene or AOI");
      return(-22);
    }
  }

  /*----------------------- INITIALIZE -------------------------------*/

  if (pabsDiff8Tab0 == (int *)NULL) {
    pabsDiff8Tab0 = absDiff8Tab;
    for (ox = 255; ox >= 0; ox--) {
      *pabsDiff8Tab0++ = ox;
    }
    for (ox = 1; ox <= 255; ox++) {
      *pabsDiff8Tab0++ = ox;
    }
    pabsDiff8Tab0 = &absDiff8Tab[255];
  }

  bestx = besty = 0;
  bestsum = MAXINT32;
  maxsum  = 0;

  /* TRICK: the row offsets are multiplied with 4 and the increment of pointers
     is done with a (int8 *) cast, so the processor doesn't shift the offset
     before each pointer increment */
  xmo = pObjImg->ld;
  xms = pScnImg->ld;
  xxo = pObjImg->xx;
  yyo = pObjImg->yy;

  sBytes = pScnImg->d;      // Scene image bytes per pixel

  /*------- Size of correlation image ----------*/

  xxc = pScnImg->xx - xxo + 1;
  yyc = pScnImg->yy - yyo + 1;
  xmc = 1;                            // Need this to avoid compiler waring

  if( ppCorImg != NULL) {             // Output correlation image

    // Ensure output image with proper size
    ierr = YaIPS_RGB_ImageSetSize( ppCorImg, xxc, yyc, 4);
    if( ierr != 0)  {                           // Check for error
      return( ierr);
    }

    // Check correlation image
    ierr = YaIPS_RGB_to_ImgD( *ppCorImg, &iCorImg);
    if( ierr != 0)  {                           // Check for error
      return( ierr);
    }

    xmc = iCorImg.ld / 4;                      // Use for integer pointer, so can divide by 4

    pCorrLine  = RGB_pixadt( 0, 0, &iCorImg, int);

  } else {

    pCorrLine = NULL;
  }

  /* PRINTF3("FSV: xmo %d xmc %d xms %d\n", xmo, xmc, xms); */

  /* ----------------- STANDARD CORRELATION ----------------------------- */

  pSceneLine = RGB_pixad( 0, 0, pScnImg);
  pSceneLine += ScnByte;                                // Offset to proper byte

  for(cy = 0; cy < yyc; cy++) {

    pScene = pSceneLine;
    pCorr  = pCorrLine;

    for (cx = 0; cx < xxc; cx++) {

      // Walk the object

      sum = 0;
      pObjLine      = RGB_pixad( 0, 0, pObjImg);
      pSceneObjLine = pScene;

      for(oy = 0; oy < yyo; oy++) {

        pObj = pObjLine;
        pSceneObj = pSceneObjLine;

        ox = xxo;

        while (ox >= 4) {
          sum += pabsDiff8Tab0[ (int)pObj[ 0] - (int)pSceneObj[ 0]];
          sum += pabsDiff8Tab0[ (int)pObj[ 1] - (int)pSceneObj[ sBytes]];
          sum += pabsDiff8Tab0[ (int)pObj[ 2] - (int)pSceneObj[ 2 * sBytes]];
          sum += pabsDiff8Tab0[ (int)pObj[ 3] - (int)pSceneObj[ 3 * sBytes]];
          ox -= 4;
          pObj += 4;
          pSceneObj += 4 * sBytes;
        }

        while (ox >= 1) {
          sum += pabsDiff8Tab0[ (int)pObj[ 0] - (int)pSceneObj[ 0]];
          ox -= 1;
          pObj += 1;
          pSceneObj += 1 * sBytes;
        }

        pObjLine += xmo;
        pSceneObjLine += xms;
      }

      if( sum < bestsum) {              // Catch best
        bestx = cx;
        besty = cy;
        bestsum = sum;
      }

      if( sum > maxsum) {               // Catch maximum value
        maxsum = sum;
      }

      if( ppCorImg != NULL) {           // Output correlation image

        *pCorr = sum;                   // Store data
        pCorr++;
      }

      pScene += sBytes;
    }

    pSceneLine += xms;
    pCorrLine += xmc;                   // NOTE: this is also OK if we have no correlation image
  }

  /* ----------------- get object value ------------------------------------- */
  // Simply sum up the values

  objval = 0;

  pObjLine = RGB_pixad( 0, 0, pObjImg);

  for( cy = 0; cy < pObjImg->yy; cy++) {

    pObj = pObjLine;

    for( cx = 0; cx < pObjImg->xx; cx++) {

      objval += *pObj;

      pObj += 1;
    }

    pObjLine += xmo;
  }

  /* ----------------- store result ------------------------------------- */

  pBestcor->Valid     = true;        // Correlation results are valid
  pBestcor->bestval   = bestsum;
  pBestcor->maxval    = maxsum;
  pBestcor->objval    = objval;
  pBestcor->ObjSizeXX = xxo;
  pBestcor->ObjSizeYY = yyo;

  pBestcor->xpos      = bestx;
  pBestcor->ypos      = besty;

  pBestcor->Label[ 0] = '\0';

  // Calculate correlation quality

  pBestcor->Quality = 0.0;                                               // Preset very bad result

  if( pBestcor->objval > 0) {                                            // Security test

    pBestcor->Quality = pBestcor->objval - pBestcor->bestval;            // Relative to object value
    if( pBestcor->Quality < 0.0) {                                       // Clip negative values
      pBestcor->Quality = 0.0;
    }
    pBestcor->Quality = (100.0 * pBestcor->Quality) / pBestcor->objval;  // Make a kind of percent
  }

  return(0);
}

/***************************************************************************
* YaIPS_RGB_Crcdf_Calc
* cross difference function with single object
*
* ppCorImg     Out: Correlation image
* pScnImg      In: Scene image. Can be BW or RGB.
* ScnByte      In: What byte component to use from the scene image
* pObjImg      In: Object image. Must have 1 byte per pixel.
* pBestcor     Out: Best correlation result
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Crcdf_Calc( Fl_RGB_Image **ppCorImg,       // Out: Correlation image
                          Fl_RGB_Image *pScnImg,         // In: Scene image
                          int ScnByte,                   // In: What byte component to use from the scene image
                          Tvector *vObj,                 // In: optional vector with extracted objects
                          Fl_RGB_Image *pObjImg,         // In: Object image. Must have 1 byte per pixel.
                          T_YaIPS_Res_Crcdf *pBestcor,   // Out: Best correlation result
                          int AOI_X, int AOI_Y,          // Optional in: AOI left upper corner
                          int AOI_XX, int AOI_YY)        // Optional in: AOI size
{
  YaIPS_RGB_ImgD_t iScnImg, iScnImgAOI, iObjImg;
  int ierr, ImgXX, ImgYY;

  // First preparations

  pBestcor->Valid   = false;                  // Correlation results are NOT valid

  // Check scene image
  ierr = YaIPS_RGB_to_ImgD( pScnImg, &iScnImg);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Check object image
  ierr = YaIPS_RGB_to_ImgD( pObjImg, &iObjImg);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Object image must have 1 byte per pixel

  if( iObjImg.d != 1) {                      // Must have same 1 byte per pixel

    if( errstring == NULL) {                  // No error until now

      sprintf( errbuffer, LangStringLookup( "&RGB_Crcdf_Err1=Object not a BW image!"));

      errstring = errbuffer;
    }

    return( -112);
  }

  // Check AOI

  ImgXX = iScnImg.xx;
  ImgYY = iScnImg.yy;

  // Clip size x
  if( AOI_XX <= 0) {                       // Size of image
    AOI_XX = ImgXX;                        // --> Use full size
  }

  if( AOI_XX > ImgXX){
    AOI_XX = ImgXX;
  }

  // Clip position x
  if( AOI_XX == 0 || AOI_XX >= ImgXX) {   // is full size

    AOI_X = 0;                             // Always set to 0

  } else {

    if( AOI_X < 0) {

      AOI_X = 0;
    }

    if( AOI_X > ImgXX - AOI_XX) {

      AOI_X = ImgXX - AOI_XX;
    }
  }

  // Clip size y
  if( AOI_YY <= 0) {                       // Size of image
    AOI_YY = ImgYY;                        // --> Use full size
  }

  if( AOI_YY > ImgYY){
    AOI_YY = ImgYY;
  }

  // Clip position y
  if( AOI_YY == 0 || AOI_YY >= ImgYY) {   // is full size

    AOI_Y = 0;                             // Always set to 0

  } else {

    if( AOI_Y < 0) {

      AOI_Y = 0;
    }

    if( AOI_Y > ImgYY - AOI_YY) {

      AOI_Y = ImgYY - AOI_YY;
    }
  }

  memcpy( &iScnImgAOI, &iScnImg, sizeof( YaIPS_RGB_ImgD_t));

  if( AOI_XX != ImgXX || AOI_YY != ImgYY) {   // Use an AOI

    // Set AOI

    iScnImgAOI.pD = iScnImg.pD + AOI_Y * iScnImg.ld + AOI_X * iScnImg.d;

    iScnImgAOI.xx = AOI_XX;
    iScnImgAOI.yy = AOI_YY;
  }

  if( vObj == NULL) {                         // NO vector with extracted objects

    ierr = YaIPS_RGB_Crcdf_Calc( ppCorImg, &iScnImgAOI, ScnByte, &iObjImg, pBestcor);

  } else {                                    // Vector with extracted objects

    int nSceneObj, iSceneObj, x1, y1, x2, y2, xx, yy, xx2, yy2;
    uchar *pAoiD;                          // Base data pointer for AOI
    Trl2objdst *pSceneObj;

    // Output no correlation image. Ensure the image is deleted

    if( *ppCorImg != NULL) {                // Have an image

      (*ppCorImg)->release();               // Release the image
      *ppCorImg = NULL;
    }

    // ...

    pAoiD = iScnImgAOI.pD;                  // Point to left upper corner of AOI

    nSceneObj = (int)(vgetnm(vObj) / (sizeof(Trl2objdst) / sizeof(int32)));

    pSceneObj = (Trl2objdst *)vgetpm(vObj);

    for( iSceneObj = 0; iSceneObj < nSceneObj; iSceneObj++, pSceneObj++) {

      pSceneObj->ExData1 = 0;             // Set no error

      // Set AOI, extend are by some pixel to each side

      x1 = pSceneObj->xmin - YAIPS_BIMG_OBJ_BORDER_ADD;
      y1 = pSceneObj->ymin - YAIPS_BIMG_OBJ_BORDER_ADD;

      x2 = pSceneObj->xmax + YAIPS_BIMG_OBJ_BORDER_ADD;
      y2 = pSceneObj->ymax + YAIPS_BIMG_OBJ_BORDER_ADD;

      xx2 = x2 - x1 + 1 - YAIPS_BIMG_OBJ_SEARCH - YAIPS_BIMG_OBJ_SEARCH;   // Size for later tolerance size check
      yy2 = y2 - y1 + 1 - YAIPS_BIMG_OBJ_SEARCH - YAIPS_BIMG_OBJ_SEARCH;

      if( x1 < 0) {
        x2 -= x1;
        x1 = 0;
      }

      if( y1 < 0) {
        y2 -= y1;
        y1 = 0;
      }

      if( x2 >= AOI_XX) {

        x1 -= x2 - (AOI_XX - 1);
        x2 = AOI_XX - 1;
      }

      if( y2 >= AOI_YY) {

        y1 -= y2 - (AOI_YY - 1);
        y2 = AOI_YY - 1;
      }

      xx = x2 - x1 + 1;
      yy = y2 - y1 + 1;

      // Check AOI size

      if( xx > AOI_XX || yy > AOI_YY) {           // To large for AOI

        pSceneObj->ExData1 = -1;                  // Error, to object to large for AOI

        continue;
      }

      // Check size tolerance area to object
      if( xx2 < iObjImg.xx - YAIPS_BIMG_OBJ_TOL_MINUS ||
          xx2 > iObjImg.xx + YAIPS_BIMG_OBJ_TOL_PLUS ||
          yy2 < iObjImg.yy - YAIPS_BIMG_OBJ_TOL_MINUS ||
          yy2 > iObjImg.yy + YAIPS_BIMG_OBJ_TOL_PLUS) {

        pSceneObj->ExData1 = -1;                  // Error, size don't fit

        continue;
      }

      // Set AOI

      iScnImgAOI.pD = pAoiD + y1 * iScnImg.ld + x1 * iScnImg.d;

      iScnImgAOI.xx = xx;
      iScnImgAOI.yy = yy;

      ierr = YaIPS_RGB_Crcdf_Calc( NULL, &iScnImgAOI, ScnByte, &iObjImg, pBestcor);

      if( ierr < 0) {                             // Error during correlation

        pSceneObj->ExData1 = ierr;                // Error, to object to large for AOI

        continue;
      }

      pSceneObj->ExData1 = 0;                     // Correlation was OK
      pSceneObj->ExData2 = (int)(pBestcor->Quality * 10.0 + 0.5);  // Set correlation quality
    }
  }

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_Crcdf_Calc
* Cross difference function with multiple objects
*
* ppCorImg     Out: Correlation image
* pScnImg      In: Scene image. Can be BW or RGB.
* ScnByte      In: What byte component to use from the scene image
* pObjImg      In: Object image. Must have 1 byte per pixel.
* pBestcor     Out: Best correlation result
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/
// Cross difference function with multiple objects
int YaIPS_RGB_Crcdf_Calc( Fl_RGB_Image **ppCorImg,         // Out: Correlation image
                          Fl_RGB_Image *pScnImg,           // In: Scene image
                          int ScnByte,                     // In: What byte component to use from the scene image
                          Tvector *vObj,                   // In: optional vector with extracted objects
                          Fl_RGB_Image *pObjImg,           // In: Object image. Must have 1 byte per pixel.
                          T_YaIPS_Crcdf_ObjOpt *pObjOpt,   // In: Object options
                          T_YaIPS_Res_Crcdf *pBestcor,     // Out: Best correlation result
                          int AOI_X, int AOI_Y,            // Optional in: AOI left upper corner
                          int AOI_XX, int AOI_YY)          // Optional in: AOI size
{
  YaIPS_RGB_ImgD_t iScnImg, iScnImgAOI, iObjImg, iObjImg2;
  int ierr, iObj, ImgXX, ImgYY;
  T_YaIPS_Crcdf_OO_1 *pObj1;
  T_YaIPS_Res_Crcdf BestcorTemp;

  // First preparations

  pBestcor->Valid   = false;            // Correlation results are NOT valid

  // ...

  if( pObjOpt == NULL) {                // Security test

    return( -1);
  }

  // Check scene image
  ierr = YaIPS_RGB_to_ImgD( pScnImg, &iScnImg);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Check object image
  ierr = YaIPS_RGB_to_ImgD( pObjImg, &iObjImg);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Object image must have 1 byte per pixel

  if( iObjImg.d != 1) {                      // Must have same 1 byte per pixel

    if( errstring == NULL) {                  // No error until now

      sprintf( errbuffer, LangStringLookup( "&RGB_Crcdf_Err1=Object not a BW image!"));

      errstring = errbuffer;
    }

    return( -112);
  }

  // Check AOI

  ImgXX = iScnImg.xx;
  ImgYY = iScnImg.yy;

  // Clip size x
  if( AOI_XX <= 0) {                       // Size of image
    AOI_XX = ImgXX;                        // --> Use full size
  }

  if( AOI_XX > ImgXX){
    AOI_XX = ImgXX;
  }

  // Clip position x
  if( AOI_XX == 0 || AOI_XX >= ImgXX) {   // is full size

    AOI_X = 0;                             // Always set to 0

  } else {

    if( AOI_X < 0) {

      AOI_X = 0;
    }

    if( AOI_X > ImgXX - AOI_XX) {

      AOI_X = ImgXX - AOI_XX;
    }
  }

  // Clip size y
  if( AOI_YY <= 0) {                       // Size of image
    AOI_YY = ImgYY;                        // --> Use full size
  }

  if( AOI_YY > ImgYY){
    AOI_YY = ImgYY;
  }

  // Clip position y
  if( AOI_YY == 0 || AOI_YY >= ImgYY) {   // is full size

    AOI_Y = 0;                             // Always set to 0

  } else {

    if( AOI_Y < 0) {

      AOI_Y = 0;
    }

    if( AOI_Y > ImgYY - AOI_YY) {

      AOI_Y = ImgYY - AOI_YY;
    }
  }

  memcpy( &iScnImgAOI, &iScnImg, sizeof( YaIPS_RGB_ImgD_t));

  if( AOI_XX != ImgXX || AOI_YY != ImgYY) {   // Use an AOI

    // Set AOI

    iScnImgAOI.pD = iScnImg.pD + AOI_Y * iScnImg.ld + AOI_X * iScnImg.d;

    iScnImgAOI.xx = AOI_XX;
    iScnImgAOI.yy = AOI_YY;
  }

  // Check for no objects in object option data

  if( pObjOpt->NumObjects < 1 &&       // Less then one object
      vObj == NULL) {                  // and no scene objects

    // Process a single object
    ierr = YaIPS_RGB_Crcdf_Calc( ppCorImg, &iScnImgAOI, ScnByte, &iObjImg, pBestcor);

    return( ierr);
  }

  // Have multiple objects or multiple objects in scene.
  // Output no correlation image. Ensure the image is deleted

  if( *ppCorImg != NULL) {                // Have an image

    (*ppCorImg)->release();               // Release the image
    *ppCorImg = NULL;
  }

  if( vObj == NULL) {                         // NO vector with extracted objects

    //
    // Try all objects
    //

    pObj1 = pObjOpt->Object;

    for( iObj = 0; iObj < pObjOpt->NumObjects; iObj++, pObj1++) {

      ierr = YaIPS_ImgD_AOI( &iObjImg2, &iObjImg, pObj1->x, pObj1->y, pObj1->xx, pObj1->yy);

      if( ierr != 0) {    // Check for error

        return( ierr);
      }

      ierr = YaIPS_RGB_Crcdf_Calc( NULL, &iScnImgAOI, ScnByte, &iObjImg2, &BestcorTemp);

      if( ierr != 0) {    // Check for error

        return( ierr);
      }

      if( iObj == 0 ||                                  // First object
          BestcorTemp.Quality > pBestcor->Quality) {    // or other has a better value

        memcpy( pBestcor, &BestcorTemp, sizeof( T_YaIPS_Res_Crcdf));

        strcpy( pBestcor->Label, pObj1->Label);         // Copy label text
      }
    }

  } else {                                    // Vector with extracted objects

    int nSceneObj, iSceneObj, x1, y1, x2, y2, xx, yy, xx2, yy2;
    uchar *pAoiD;                          // Base data pointer for AOI
    Trl2objdst *pSceneObj;

    pBestcor->Quality = -1.0;                // Preset very bad quality

    // ...

    pAoiD = iScnImgAOI.pD;                   // Point to left upper corner of AOI

    nSceneObj = (int)(vgetnm(vObj) / (sizeof(Trl2objdst) / sizeof(int32)));

    pSceneObj  = (Trl2objdst *)vgetpm(vObj);

    for( iSceneObj = 0; iSceneObj < nSceneObj; iSceneObj++, pSceneObj++) {

      pSceneObj->ExData1 = 0;             // Set no error

      // Set AOI, extend are by some pixel to each side

      x1 = pSceneObj->xmin - YAIPS_BIMG_OBJ_BORDER_ADD;
      y1 = pSceneObj->ymin - YAIPS_BIMG_OBJ_BORDER_ADD;

      x2 = pSceneObj->xmax + YAIPS_BIMG_OBJ_BORDER_ADD;
      y2 = pSceneObj->ymax + YAIPS_BIMG_OBJ_BORDER_ADD;

      xx2 = x2 - x1 + 1 - YAIPS_BIMG_OBJ_SEARCH - YAIPS_BIMG_OBJ_SEARCH;   // Size for later tolerance size check
      yy2 = y2 - y1 + 1 - YAIPS_BIMG_OBJ_SEARCH - YAIPS_BIMG_OBJ_SEARCH;

      if( x1 < 0) {
        x2 -= x1;
        x1 = 0;
      }

      if( y1 < 0) {
        y2 -= y1;
        y1 = 0;
      }

      if( x2 >= AOI_XX) {

        x1 -= x2 - (AOI_XX - 1);
        x2 = AOI_XX - 1;
      }

      if( y2 >= AOI_YY) {

        y1 -= y2 - (AOI_YY - 1);
        y2 = AOI_YY - 1;
      }

      xx = x2 - x1 + 1;
      yy = y2 - y1 + 1;

      // Check AOI size

      if( xx > AOI_XX || yy > AOI_YY) {           // To large for AOI

        pSceneObj->ExData1 = -1;                  // Error, to object to large for AOI

        continue;
      }

      // Set AOI

      iScnImgAOI.pD = pAoiD + y1 * iScnImg.ld + x1 * iScnImg.d;

      iScnImgAOI.xx = xx;
      iScnImgAOI.yy = yy;

      //
      // Try all objects
      //

      int   BestObjIdx;
      float BestObjQuality;     // Correlation quality. 0 is very bad, 100 is very good.

      BestObjQuality = -1.0;     // Preset very bad quality

      pObj1 = pObjOpt->Object;

      BestObjIdx = -1;

      for( iObj = 0; iObj < pObjOpt->NumObjects; iObj++, pObj1++) {

        // Check size tolerance area to object
        if( xx2 < pObj1->xx - YAIPS_BIMG_OBJ_TOL_MINUS ||
            xx2 > pObj1->xx + YAIPS_BIMG_OBJ_TOL_PLUS ||
            yy2 < pObj1->yy - YAIPS_BIMG_OBJ_TOL_MINUS ||
            yy2 > pObj1->yy + YAIPS_BIMG_OBJ_TOL_PLUS) {

          continue;
        }

        ierr = YaIPS_ImgD_AOI( &iObjImg2, &iObjImg, pObj1->x, pObj1->y, pObj1->xx, pObj1->yy);

        if( ierr != 0) {                                  // Check for error

          continue;                                       // Skip errors
        }

        ierr = YaIPS_RGB_Crcdf_Calc( NULL, &iScnImgAOI, ScnByte, &iObjImg2, &BestcorTemp);

        if( ierr < 0) {                                   // Error during correlation

          pSceneObj->ExData1 = ierr;                      // Error, to object to large for AOI

          continue;
        }

        // Test for best object for this scene AOI
        if( BestObjIdx <  0 ||                            // First object
            BestcorTemp.Quality > BestObjQuality) {       // or other has a better value

          BestObjIdx = iObj;                              // Set object index
          BestObjQuality = BestcorTemp.Quality;           // Latch better value
        }

        if( BestcorTemp.Quality > pBestcor->Quality) {    // or other has a better value

          memcpy( pBestcor, &BestcorTemp, sizeof( T_YaIPS_Res_Crcdf));

          pBestcor->Label[ 0] = '\0';
        }
      }

      if( BestObjIdx < 0) {                               // Have no best correlation

        pSceneObj->ExData1 = -2;                          // Error, to object to large for AOI

        continue;
      }

      pSceneObj->ExData1 = BestObjIdx;                    // Best matching object
      pSceneObj->ExData2 = (int)(BestObjQuality * 10.0 + 0.5);  // Set correlation quality
    }
  }

  return( 0); // Done
}

/***************************************************************************
* YaIPS_RGB_Crcdf_Conv2BW
* Convert integer (4 byte) correlation image to a BW (1 Byte) image.
*
* ppDst        Out: Pointer to converted BW image
* pSrc         In: correlation image (4 bytes)
* NormValue    In: Highest value in correlation image. Is converted to a value of 255.
*                  If <= 0, the output will only set to 0
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Crcdf_Conv2BW( Fl_RGB_Image **ppDst,          // Out: Pointer to converted BW image (1 bytes)
                             Fl_RGB_Image *pSrc,            // In: correlation image (4 bytes)
                             T_YaIPS_Res_Crcdf *pBestcor)   // In: Best correlation result
{
  int ierr, x, y, xx, yy, t, NormValue, NormValue2, objval;
  //x/int MaxVal;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  int *pCorr;
  uchar *pDst;

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Source must have 4 bytes per pixel

  if( iSrc.d != 4) {

    return( -200);
  }

  xx = iSrc.xx;
  yy = iSrc.yy;

  // Ensure output image with proper size and bytes per pixel

  ierr = YaIPS_RGB_ImageSetSize( ppDst, xx, yy, 1);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  ierr = YaIPS_RGB_to_ImgD( *ppDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // ...

  //x/MaxVal    = pBestcor->maxval;
  objval    = pBestcor->objval;

  NormValue = objval;                                  // Difference is normalize value

  if( NormValue < 1) {                                 // No reasonable maximum

    NormValue = 0;                                     // Set to zero

  } else if( NormValue < 255) {                        // Have low values

    NormValue = 255;                                   // Enlarge to maximum of 255
  }

  NormValue2 = NormValue / 2;

  for( y = 0; y < yy; y++) {

    pCorr = RGB_pixadt( 0, y, &iSrc, int);
    pDst  = RGB_pixad( 0, y, &iDst);

    if( NormValue <= 0) {                // Set to zero

      for( x = 0; x < xx; x++) {

        *pDst = 0;                       // Store

        pDst++;
      }

    } else {

      for( x = 0; x < xx; x++) {

        t = NormValue - *pCorr;                 // negate

        if( t <= 0) {

          *pDst = 0;

        } else {

          t = (t * 255 + NormValue2) / NormValue; // Normalize height

          RGB_hclip( t, pDst);                    // Clip and store
        }

        pCorr++;
        pDst++;
      }
    }
  }

  return( 0); // Done
}

/***************************************************************************
* YaIPS_RGB_Crcdf_ObjOptions
* Get object options for a correlation object image
*
* pObjOpt      Out: Object options
* pSrc         In: object image (1 byte per pixel)
* pFileName    In: File name with path of object image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

// Get object options for a correlation object image
int YaIPS_RGB_Crcdf_ObjOptions( T_YaIPS_Crcdf_ObjOpt *pObjOpt, // Out: Object options
                                Fl_RGB_Image *pSrc,            // In: object image (1 byte per pixel)
                                char *pFileName)               // In: File name with path of object image
{
  int ierr, x, y, x2, y2, LineNr, StringLen, NumObjLabel;
  YaIPS_RGB_ImgD_t iSrc;
  char TempFileName[ MAX_FILENAME_LEN];
  char *p;
  FILE *pFile;
  char line[ 1024];
  uchar BorderPixel, *pLine, *pLine2;
  int BorderLineThis, BorderLineBefore, BorderRowThis, BorderRowBefore, RowYTop, RowYBot, RowXLeft, RowXRight;
  int RowYTop2, RowYBot2, BorderLinePart;
  T_YaIPS_Crcdf_OO_1 *pObj1;

  // Reset output

  pObjOpt->NumObjects = 0;                    // Reset # objects

  // Check source first
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Source must have 1 bytes per pixel

  if( iSrc.d != 1) {

    return( -200);
  }

  // Try to open option file.
  // Has the same base name as the image file, the file extension is 'txt'.

  memset( TempFileName, 0, sizeof( TempFileName));
  strncpy( TempFileName, pFileName, sizeof( TempFileName) - 1);

  p = strchr( TempFileName, '.');        // Get last point

  if( p == NULL) {                       // None found

    return( -201);
  }

  p += 1;                                // Point after '.'

  if( ( int)(p - TempFileName) >= (int)( sizeof( TempFileName) - 4)) {   // Not enough space for extension

    return( -202);
  }

  strcpy( p, "txt");                     // Add extension

  pFile = fl_fopen( TempFileName, "rt"); // Try to open file

  if( pFile == NULL) {                   // File not found ?

    return( -202);
  }

  // Have a text file. Read in the data

  // Test for file magic

  if( !fgets( line, sizeof line, pFile)) {

    ierr = -210;                                  // Problem reading line
    goto ErrorExit;
  }

  if( memcmp( line, "Crcdf-Options", 13)) {

    ierr = -210;                                  // File magic mismatch
    goto ErrorExit;
  }

  // Parse the file

  LineNr = 1;

  NumObjLabel = 0;                               // Rest count for objects with label

  memset( pObjOpt, 0, sizeof( T_YaIPS_Crcdf_ObjOpt));  // Zero complete output

  for( ; ; ) {

    // Try to read next line

    if( !fgets(line, sizeof line, pFile)) {
      break;
    }

    // Got the next line

    LineNr += 1;

    if( line[ 0] == '/' && line[ 1] == '/') {   // is comment line

      continue;
    }

    // remove line feed at end of line

    StringLen = strlen( line);

    while( StringLen > 0 && line[ StringLen - 1] == '\n') {

      line[ StringLen - 1] = '\0';  //  Set end of string

      StringLen -= 1;
    }

    // ..

    if( line[ 0] == '\0') {   // is empty line
      continue;
    }

    // Check for label

    if( strncasecmp( line, "Label: ", 7) == 0) {   // Is a label

      p = line + 7;                      // Point to label

      if( NumObjLabel < YAIPS_CRCDF_OOPT_MAX_OBJECTS) {   // Table not full

        strncpy( pObjOpt->Object[ NumObjLabel].Label, p, sizeof( pObjOpt->Object[ 0].Label));

        NumObjLabel += 1;                // Have one more
      }
    }
  }

  // Done with option file processing

  fclose( pFile);                        // Close file
  pFile = NULL;                          // Mark as close

  // Isolate objects inside the object image file
  // We assume objects surrounded by a so called border pixels.
  // Lines of objects must be separated by image lines with only border pixels.
  // In such a line, the objects are also separated by rows of border pixels.
  // NOTE: Always the top left pixel must be a border pixel. The value to test for border
  //       pixels is picked from here.

  BorderPixel = *iSrc.pD;                // Left upper pixel is used for scanning object borders

  pLine = iSrc.pD;

  BorderLineThis = true;                // Initialize parser
  RowYTop = 0;

  for( y = 0; y < iSrc.yy; y++, pLine += iSrc.ld) {        // Walk all lines

    BorderLineBefore = BorderLineThis;    // Check result of line before

    // Check line to have only border pixels

    BorderLineThis = true;
    for( x = 0; x < iSrc.xx; x++) {     // Walk all rows

      if( pLine[ x] != BorderPixel) {

        BorderLineThis = false;
        break;
      }
    }

    if( y == 0) {                       // Was first line

      // First line must be a border line !

      if( ! BorderLineThis) {           // Not all are border pixels

        ierr = -211;                    // First line is no border line
        goto ErrorExit;
      }
    }

    if( BorderLineBefore && ! BorderLineThis) {  // First line with no border

      RowYTop = y;                      // Remember row top line
      continue;
    }

    if( BorderLineBefore == BorderLineThis) {  // No check change

      continue;
    }

    // If we came to here, we have a number of lines with no borders.

    RowYBot = y - 1;                      // Row bottom line

    // Scan for rows of border pixels

    BorderRowThis = true;                 // Initialize parser

    for( x = 0; x < iSrc.xx; x++) {       // Walk all rows

      BorderRowBefore = BorderRowThis;    // Check result of line before

      pLine2 = RGB_pixad( 0, RowYTop, &iSrc);

      BorderRowThis = true;
      for( y2 = RowYTop; y2 <= RowYBot; y2++, pLine2 += iSrc.ld) {     // Walk all lines

        if( pLine2[ x] != BorderPixel) {

          BorderRowThis = false;
          break;
        }
      }

      if( x == 0) {                       // Was first row

        // First row must be a border line !

        if( ! BorderRowThis) {           // Not all are border pixels

          ierr = -212;                   // First row is no border row
          goto ErrorExit;
        }
      }

      if( BorderRowBefore && ! BorderRowThis) {  // First row with no border

        RowXLeft = x;                      // Remember line left row
        continue;
      }

      if( BorderRowBefore == BorderRowThis) {  // No check change

        continue;
      }

      // If we came to here, we have a number of rows with no borders.

      RowXRight = x - 1;                      // Line left row

      // Try to adapt top position

      pLine2 = RGB_pixad( 0, RowYTop, &iSrc);

      RowYTop2 = RowYTop;

      BorderLinePart = true;
      for( y2 = RowYTop; y2 <= RowYBot; y2++, pLine2 += iSrc.ld) {     // Walk all lines

        for( x2 = RowXLeft; x2 <= RowXLeft; x2++) {       // Walk this line part

          if( pLine2[ x2] != BorderPixel) {

            RowYTop2 = y2;

            BorderLinePart = false;
            break;
          }
        }

        if( ! BorderLinePart) {
          break;
        }
      }

      // Try to adapt bottom position

      pLine2 = RGB_pixad( 0, RowYBot, &iSrc);

      RowYBot2 = RowYBot;

      BorderLinePart = true;
      for( y2 = RowYBot; y2 >= RowYTop; y2--, pLine2 -= iSrc.ld) {     // Walk all lines

        for( x2 = RowXLeft; x2 <= RowXLeft; x2++) {       // Walk this line part

          if( pLine2[ x2] != BorderPixel) {

            RowYBot2 = y2;

            BorderLinePart = false;
            break;
          }
        }

        if( ! BorderLinePart) {
          break;
        }
      }

      // Have a object here.
      // NOTE: Until now, assume that all objects have the same size

      if( pObjOpt->NumObjects < YAIPS_CRCDF_OOPT_MAX_OBJECTS) {   // Table not full

        pObj1 = pObjOpt->Object + pObjOpt->NumObjects;

        // Store data about object
        pObj1->x = RowXLeft;
        pObj1->y = RowYTop2;
        pObj1->xx = RowXRight - RowXLeft + 1;
        pObj1->yy = RowYBot2 - RowYTop2 + 1;

        pObjOpt->NumObjects += 1;           // Have one more

        if( pObjOpt->NumObjects > NumObjLabel) {   // have no label for this object

          sprintf( pObj1->Label, "%d", pObjOpt->NumObjects); // Set in its number
        }
      }
    }
  }

  // Done

  ierr = 0;                              // No error

ErrorExit:

  if( pFile != NULL) {                   // File is not closed until noe

    fclose( pFile);                      // Close file
  }

  return( ierr);                         // Return
}

/***************************************************************************
* YaIPS_RGB_Crcdf_ObjOptions
* Get object options for a correlation object image.
* Label objects do adjust size. The labeling must use the same
* parameters as used for the scene labeling.
*
* pObjOpt      Out: Object options
* pSrc         In: object image (1 byte per pixel)
* pFileName    In: File name with path of object image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

// Get object options for a correlation object image
int YaIPS_RGB_Crcdf_ObjOptions( T_YaIPS_Crcdf_ObjOpt *pObjOpt, // Out: Object options
                                Fl_RGB_Image *pObjImg,         // In: object image (1 byte per pixel)
                                char *pFileName,               // In: File name with path of object image
                                int BinThres,                  // In: Binarization threshold
                                int BinMode,                   // In: Binarization mode, 0: objects >= Thres, 1: code if < Thres
                                int AreaMin,                   // In: Minimum area of an object to be labeled
                                int AreaMax)                   // In: Maximum area of an object to be labeled
{
  int ierr, iObj, nSceneObj, iSceneObj, iBestObj, LenghtBest, LenghtThis;
  int x1, x2, y1, y2;
  Tvector *vObj = NULL;                    // If != NULL, objects in scene
  T_YaIPS_Crcdf_OO_1 *pObj1;
  Trl2objdst *pSceneObj;

  // ...

  if( pObjOpt == NULL) {                // Security test

    return( -1);
  }

  // Get object options from.
  ierr = YaIPS_RGB_Crcdf_ObjOptions( pObjOpt, pObjImg, pFileName);

  if( ierr != 0) {          // There are no options

    // Make a single object from the imae

    pObjOpt->NumObjects = 0;

    pObj1 = pObjOpt->Object + pObjOpt->NumObjects;

    // Store data about object
    pObj1->x = 0;
    pObj1->y = 0;
    pObj1->xx = pObjImg->data_w();
    pObj1->yy = pObjImg->data_h();

    pObjOpt->NumObjects += 1;           // Have one more

    pObj1->Label[ 0] = '\0';
  }

  // Allocate a vector for the objects

  vObj = ve_ucreate( DV_HOST);

  if( vObj != VENULL) {               // Have the vector

    // preallocate the max number of objects

    ierr = ve_alloc( vObj , 10 * (sizeof(Trl2objdst) / sizeof(int32)), sizeof(int32), TY_INT32);

    if( ierr != 0) {                                 // ERROR allocating data

      //x/sprintf( errbuffer, "error %d allocating vector", ierr);

      goto ErrorExit;
    }

  } else {                                           // Error creating the vector

    //x/errstring = (char *)"failed to create vector";

    ierr = -200;
    goto ErrorExit;
  }

  // Label all the objects

  pObj1 = pObjOpt->Object;

  for( iObj = 0; iObj < pObjOpt->NumObjects; iObj++, pObj1++) {

    // Label this object
    ierr = YaIPS_RGB_RLC_CodeMeas( vObj, pObjImg,
                                   BinThres, BinMode, AreaMin, AreaMax,
                                   pObj1->x, pObj1->y, pObj1->xx, pObj1->yy);

    if( ierr != 0) {   // Have an error

      continue;        // Skip this error
    }

    // Walk the binaries objects and pick the largest (city block length).

    nSceneObj = (int)(vgetnm( vObj) / (sizeof(Trl2objdst) / sizeof(int32)));

    pSceneObj  = (Trl2objdst *)vgetpm( vObj);

    LenghtBest = 0;                      // No best length
    iBestObj   = -1;                     // No best scene object

    for( iSceneObj = 0; iSceneObj < nSceneObj; iSceneObj++, pSceneObj++) {

      LenghtThis = (pSceneObj->xmax - pSceneObj->xmin + 1) + (pSceneObj->ymax - pSceneObj->ymin + 1);

      if( LenghtThis > LenghtBest) {

        iBestObj   = iSceneObj;
        LenghtBest = LenghtThis;
      }
    }

    if( LenghtBest > 0) {                // Have a best scene object

      pSceneObj  = (Trl2objdst *)vgetpm( vObj);
      pSceneObj += iBestObj;

      // Take over the object. Enlarge on each side.

      x1 = pSceneObj->xmin - YAIPS_BIMG_OBJ_ENLARGE;
      x2 = pSceneObj->xmax + YAIPS_BIMG_OBJ_ENLARGE;
      y1 = pSceneObj->ymin - YAIPS_BIMG_OBJ_ENLARGE;
      y2 = pSceneObj->ymax + YAIPS_BIMG_OBJ_ENLARGE;

      if( x1 < 0) {
        x1 = 0;
      }
      if( x2 >= pObj1->xx) {
        x2 = pObj1->xx - 1;
      }

      if( y1 < 0) {
        y1 = 0;
      }
      if( y2 >= pObj1->yy) {
        y2 = pObj1->yy - 1;
      }

      pObj1->x += x1;
      pObj1->y += y1;
      pObj1->xx = x2 - x1 + 1;
      pObj1->yy = y2 - y1 + 1;
    }
  }

  // ...

  ierr = 0;                     // Return no error

ErrorExit:

  if( vObj != NULL) {       // Object vector was used

    ve_remove( vObj);       // Release the vector
  }

  return( ierr);
}

/******************************** End Of File ********************************/
