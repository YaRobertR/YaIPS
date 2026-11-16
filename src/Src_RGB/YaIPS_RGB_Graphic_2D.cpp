/****************************************************************************

  YaIPS_RGB_Graphic_2D.cpp

  Fl_RGB_Image image processing.
  Draw graphics into image.

  24.10.2026 RR: * First edition of this file.
                   Draw graphics into image. Is used to draw check masks
                   into a bit layer of an reference image.

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
* Local variables
*
****************************************************************************
*/

#define UNDO_MAX_LEVELS 32            // Max number of undo levels

struct Sgrect    {
   int rxa;             /* origin */
   int rya;             /* origin */
   int rxx;             /* size   */
   int ryy;             /* size   */
};
typedef struct Sgrect Tgrect;

// If drawing is initialized all elements are != 0
static YaIPS_RGB_ImgD_t CurrImg;     // Current image we work with
static YaIPS_G2D_UnDo_Buffer_t *pCurrUnDo;  // Pointer to current undo data

static Tgrect ClipRect;              // Current clip rectangle
static uchar BitMask[ 4];            // Pixel mask to write to
static uchar BitIMask[ 4];           // Inverted pixel mask to write to
static int iFirstUsed, iLastUsed;    // Fist, last used of the mask bytes
static int   wmod;                   // Pixel write mode
static int   LineState;              // State line drawing. 0 = in chain, 1 = start of chain + draw start point, 2 = start of chain
static int   LineLastX;              // X of last line point
static int   LineLastY;              // Y of last line point

// Check for image setup

#define IsInitalized() { if( CurrImg.pD == NULL) return( YAIPS_G2D_ERR_NO_INIT); }

// Range tests of coordinates

#define rangetestx( x) { if(((x) < 0) || ((x) >= CurrImg.xx)) return( YAIPS_G2D_ERR_XY_RANGE); }
#define rangetesty( y) { if(((y) < 0) || ((y) >= CurrImg.yy)) return( YAIPS_G2D_ERR_XY_RANGE); }

// Range check of coordinates. Returns OK for out of range.

#define rangecheckx( x) { if(((x) < 0) || ((x) >= CurrImg.xx)) return( 0); }
#define rangechecky( y) { if(((y) < 0) || ((y) >= CurrImg.yy)) return( 0); }

/***************************************************************************
* Some support functions
****************************************************************************
*/

/**************************************************************************
* gp_sxrec()
*
* Initialize rectangle by giving two corners.
* The origin is the upper left corner, sizes are positive.
*
***************************************************************************
*/
static void gp_sxrec( Tgrect *r, int x1, int y1, int x2, int y2)
{
  int tmp;

  if(x2 < x1) { tmp = x1; x1 = x2; x2 = tmp; }
  if(y2 < y1) { tmp = y1; y1 = y2; y2 = tmp; }

  r->rxa = x1;
  r->rya = y1;
  r->rxx = x2 - x1 + 1;
  r->ryy = y2 - y1 + 1;
}

/**************************************************************************
* gp_touchrec()
*
* returns TRUE  if rectangles overlap
*         FALSE otherwise
***************************************************************************
*/
static int gp_touchrec( Tgrect *a, Tgrect *b)
{

  if((a->rxa >= b->rxa + b->rxx ) ||
     (a->rya >= b->rya + b->ryy ) ||
     (a->rxa + a->rxx <= b->rxa ) ||
     (a->rya + a->ryy <= b->rya )) {

    // no touch
    return( FALSE);
  }

  // touch
  return( TRUE);
}

/***************************************************************************
* YaIPS_Graphic_2D_init
* Setup drawing for a specific image and optional setup undo buffer.
*
* pImgD      pointer to YaIPS_RGB_ImgD_t image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_G2D_init( YaIPS_RGB_ImgD_t *pImgD, int Mask, YaIPS_G2D_UnDo_Buffer_t *pUnDo, int nBuffer)
{
  int Buff_Size;

  // Simply copy image descriptor

  memcpy( &CurrImg, pImgD, sizeof( YaIPS_RGB_ImgD_t));

  YAIPS_G2D_INT_2_BYTES( BitMask, Mask);

  Mask = ~ Mask;

  YAIPS_G2D_INT_2_BYTES( BitIMask, Mask);

  wmod = YAIPS_G2D_WMOD_SET;                  // Set default write mode

  gp_sxrec( &ClipRect, 0 ,0 , CurrImg.xx - 1, CurrImg.yy - 1);

  // Analyze bit mask for supported data

  int UndoMode, i, nBytesUsed, BitShift;

  nBytesUsed = 0;
  iFirstUsed = 0;
  iLastUsed  = 0;
  BitShift   = 0;

  for( i = 0; i < 4; i++) {

    if( BitMask[ i] != 0) {       // This byte is used for graphic

      if( nBytesUsed == 0) {      // None until now

        iFirstUsed = i;           // Remember first byte used
      }

      iLastUsed = i;              // Last byte used

      nBytesUsed += 1;
    }
  }

  UndoMode = YAIPS_G2D_UNDO_MODE_BYTE;   // Save all masked bytes

  if( nBytesUsed == 1) {                 // Only one byte masked

    // Check for single bit set

    for( i = 0; i < 8; i++) {

      if( BitMask[ iFirstUsed] == (1 << i)) {  // This bit is set

        UndoMode = YAIPS_G2D_UNDO_MODE_BIT;    // Save all masked bytes

        BitShift = i;                          // Remember bit shift
      }
    }
  }

  if( UndoMode == YAIPS_G2D_UNDO_MODE_BYTE) {  // Save all masked bytes

    Buff_Size = CurrImg.xx * CurrImg.yy * nBytesUsed;

  } else {                                    // Only on mask bit is set

    Buff_Size = (CurrImg.xx * CurrImg.yy + 7) / 8;
  }

  // Manage undo buffer

  pCurrUnDo = NULL;                               // Reset pointer to current undo buffer

  if( pUnDo != NULL) {

    if( pUnDo->pBuffer != NULL) {                 // Undo buffer is initialized

      if( pUnDo->pImg_Data   != CurrImg.pD ||     // Something has changed
          pUnDo->Img_XX      != CurrImg.xx ||
          pUnDo->Img_YY      != CurrImg.yy ||
          pUnDo->Img_NBytes  != CurrImg.d  ||
          pUnDo->Img_nBuffer != nBuffer    ||
          pUnDo->Img_Mask    != Mask) {

        free( pUnDo->pBuffer);                    // Free undo buffer

        memset( pUnDo, 0, sizeof( YaIPS_G2D_UnDo_Buffer_t));  // Zero data
      }
    }

    if( pUnDo->pBuffer == NULL) {                 // Undo buffer is not initialized

      pUnDo->pImg_Data   = CurrImg.pD;
      pUnDo->Img_XX      = CurrImg.xx;
      pUnDo->Img_YY      = CurrImg.yy;
      pUnDo->Img_NBytes  = CurrImg.d;
      pUnDo->Img_nBuffer = nBuffer;
      pUnDo->Img_Mask    = Mask;

      pUnDo->UnDoFIFO_Size = Buff_Size;

      if( nBuffer > UNDO_MAX_LEVELS) {            // Clip to upper limit

        nBuffer = UNDO_MAX_LEVELS;
      }

      pUnDo->UnDoFIFO_Max = 1;

      while( nBuffer > 1) {                       // Add more memory

        if( (int)(pUnDo->UnDoFIFO_Size * (pUnDo->UnDoFIFO_Max + 1)) < 0) {   // No buffer size overflow

          break;
        }

        nBuffer -= 1;
        pUnDo->UnDoFIFO_Max += 1;
      }

      pUnDo->pBuffer = (uchar *)malloc( pUnDo->UnDoFIFO_Size * pUnDo->UnDoFIFO_Max);  // (Try to) allocate buffer

#ifdef use_again
      pUnDo->UnDoFIFO_N       = 1;
      pUnDo->UnDoFIFO_Put     = 1;
#else
      pUnDo->UnDoFIFO_N       = 0;
      pUnDo->UnDoFIFO_Put     = 0;
#endif
      pUnDo->UnDoFIFO_LastPut = pUnDo->UnDoFIFO_Put;
      pUnDo->UnDoFIFO_LastN   = pUnDo->UnDoFIFO_N;

      pUnDo->UndoMode   = UndoMode;
      pUnDo->BitShift   = BitShift;
    }

    if( pUnDo->pBuffer != NULL) {           // Buffer allocation was OK

      pCurrUnDo = pUnDo;                    // Set pointer to current undo buffer
    }
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_Graphic_2D_init
* Setup drawing for a specific image and optional setup undo buffer.
*
* pDst       pointer to Fl_RGB_Image image
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_G2D_init( Fl_RGB_Image *pDst, int Mask, YaIPS_G2D_UnDo_Buffer_t *pUnDo, int nBuffer)
{
  int ierr;
  YaIPS_RGB_ImgD_t ImgD;

  ierr = YaIPS_RGB_to_ImgD( pDst, &ImgD);

  if( ierr != 0)  {                           // Check for error

    return( ierr);
  }

  ierr = YaIPS_G2D_init( &ImgD, Mask, pUnDo, nBuffer);

  return( ierr);
}

/***************************************************************************
* YaIPS_Graphic_2D_end
* End drawing into an image and release undo buffer
*
****************************************************************************
*/
void YaIPS_G2D_end()
{

  // Zero
  memset( &CurrImg, 0, sizeof( YaIPS_RGB_ImgD_t));

  if( pCurrUnDo != NULL) {              // Pointer to undo buffer is set

    if( pCurrUnDo->pBuffer != NULL) {   // A undo buffer as allocated

      free( pCurrUnDo->pBuffer);        // Free undo buffer

      pCurrUnDo->pBuffer = NULL;        // Buffer is freed
    }

    pCurrUnDo = NULL;                   // NO undo
  }
}

/***************************************************************************
* YaIPS_G2D_swmod
* Set write mode
*
* mode       One of the pixel write modes
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_G2D_swmod( int mode)
{

  if( mode < YAIPS_G2D_WMOD_FIRST ||   // Check range of argument
      mode > YAIPS_G2D_WMOD_LAST) {

    wmod = YAIPS_G2D_WMOD_SET;         // set default

    return( YAIPS_G2D_ERR_WMOD);       // Error
  }

  wmod = mode;

  return( 0);
}

/***************************************************************************
* YaIPS_G2D_WPix
* Write pixel
*
* x, y       Coordinates inside image
* Value      Write this value
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_G2D_WPix( int x, int y, int Value)
{
  int iPix;
  uchar ValBytes[ 4];  // Pixel value
  uchar *pPix;

  IsInitalized();      // Check 2D graphic is initialized

  rangecheckx( x);     // Check coordinate to be in range
  rangechecky( y);     // Check coordinate to be in range

  YAIPS_G2D_INT_2_BYTES( ValBytes, Value);

  pPix = RGB_pixad( x, y, &CurrImg);

  switch( wmod) {

  case YAIPS_G2D_WMOD_SET: // Set value

    for( iPix = 0; iPix < CurrImg.d; iPix++) {

      pPix[ iPix] = (pPix[ iPix] & BitIMask[ iPix]) | (ValBytes[ iPix] & BitMask[ iPix]);
    }

    break;

  case YAIPS_G2D_WMOD_XOR: // Invert pixel with mask bits

    for( iPix = 0; iPix < CurrImg.d; iPix++) {

      pPix[ iPix] = pPix[ iPix] ^ BitMask[ iPix];
    }

    break;

  default:

    return( YAIPS_G2D_ERR_WMOD);    // Unknown write mode
  }

  return( 0);         // Return OK
}

/***************************************************************************
* YaIPS_G2D_WPixLineX
* Write horizontal line
*
* x, y       Coordinates inside image
* Value      Write this value
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_G2D_WPixLineX( int x1, int x2, int y, int Value)
{
  int iPix, tmp;
  uchar ValBytes[ 4];  // Pixel value
  uchar *pPix;

  IsInitalized();      // Check 2D graphic is initialized

  rangechecky( y);     // Check coordinate to be in range

  if( x2 < x1) { tmp = x1; x1 = x2; x2 = tmp; }   // sort x coordinates

  if( x2 < 0 || x1 >= CurrImg.xx) {

    return( 0);
  }

  if( x1 < 0) {

    x1 = 0;
  }

  if( x2 >= CurrImg.xx)  {

    x2 = CurrImg.xx - 1;
  }

  YAIPS_G2D_INT_2_BYTES( ValBytes, Value);

  pPix = RGB_pixad( x1, y, &CurrImg);

  switch( wmod) {

  case YAIPS_G2D_WMOD_SET: // Set value

    while( x1 <= x2) {

      for( iPix = 0; iPix < CurrImg.d; iPix++) {

        pPix[ iPix] = (pPix[ iPix] & BitIMask[ iPix]) | (ValBytes[ iPix] & BitMask[ iPix]);
      }

      x1 += 1;

      pPix += CurrImg.d;
    }

    break;

  case YAIPS_G2D_WMOD_XOR: // Invert pixel with mask bits

    while( x1 <= x2) {

      for( iPix = 0; iPix < CurrImg.d; iPix++) {

        pPix[ iPix] = pPix[ iPix] ^ BitMask[ iPix];
      }

      x1 += 1;

      pPix += CurrImg.d;
    }

    break;

  default:

    return( YAIPS_G2D_ERR_WMOD);    // Unknown write mode
  }

  return( 0);         // Return OK
}

/***************************************************************************
* YaIPS_G2D_WPixLineY
* Write vertical line
*
* x, y       Coordinates inside image
* Value      Write this value
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_G2D_WPixLineY( int x, int y1, int y2, int Value)
{
  int iPix, tmp;
  uchar ValBytes[ 4];  // Pixel value
  uchar *pPix;

  IsInitalized();      // Check 2D graphic is initialized

  rangecheckx( x);     // Check coordinate to be in range

  if( y2 < y1) { tmp = y1; y1 = y2; y2 = tmp; }   // sort y coordinates

  if( y2 < 0 || y1 >= CurrImg.yy) {

    return( 0);
  }

  if( y1 < 0) {

    y1 = 0;
  }

  if( y2 >= CurrImg.yy)  {

    y2 = CurrImg.yy - 1;
  }

  YAIPS_G2D_INT_2_BYTES( ValBytes, Value);

  pPix = RGB_pixad( x, y1, &CurrImg);

  switch( wmod) {

  case YAIPS_G2D_WMOD_SET: // Set value

    while( y1 <= y2) {

      for( iPix = 0; iPix < CurrImg.d; iPix++) {

        pPix[ iPix] = (pPix[ iPix] & BitIMask[ iPix]) | (ValBytes[ iPix] & BitMask[ iPix]);
      }

      y1 += 1;

      pPix += CurrImg.ld;
    }

    break;

  case YAIPS_G2D_WMOD_XOR: // Invert pixel with mask bits

    while( y1 <= y2) {

      for( iPix = 0; iPix < CurrImg.d; iPix++) {

        pPix[ iPix] = pPix[ iPix] ^ BitMask[ iPix];
      }

      y1 += 1;

      pPix += CurrImg.ld;
    }

    break;

  default:

    return( YAIPS_G2D_ERR_WMOD);    // Unknown write mode
  }

  return( 0);         // Return OK
}

/***************************************************************************
* YaIPS_G2D_LintTo
* Draw line starting at the last point
*
* x, y       Coordinates inside image
* Value      Write this value
*
* return     0 OK
*          < 0 Error
*
* ATTENTION:  sequence of calls
* ---------
*    either:
*             YaIPS_G2D_MoveTo(x,y)   set start point
*             YaIPS_G2D_LineTo(x,y)   draw line to x,y
*             YaIPS_G2D_LineTo(x,y)   ........
*    or:
*             YaIPS_G2D_LineBegin()   Set chain flag
*             YaIPS_G2D_LineTo(x,y)   set start point of chain
*             YaIPS_G2D_LineTo(x,y)   draw line to x,y
*             YaIPS_G2D_LineTo(x,y)   ........
*
*    The second method is useful if you read your coordinates from
*    a vector, because the first point is treated exactly as the
*    other points.
*
*    For closed chains use gp_xbline instead of gp_bline and
*    gp_xmoveto instead of gp_moveto. In that case the first
*    point of the chain will not be drawn at the start of chain
*    but at the end.
****************************************************************************
*/

int YaIPS_G2D_LineTo( int x, int y, int Value)
{
  int rx1, ry1;              /* start point of line      */
  int dx, dy;                /* axes of line             */
  int adx, ady;              /* distance                 */
  int xa, ya;                /* direction                */
  int d;                     /* distance already done    */
  int incr1, incr2;          /* distance per step        */
  Tgrect rec;                /* clipping rectangle       */

  IsInitalized();      // Check 2D graphic is initialized

  if( LineState > 0 ) {        /* only set start point */

    LineLastX = x;            // Set end point of last line drawn
    LineLastY = y;
    if( LineState == 1) {

      YaIPS_G2D_WPix( x, y, Value);
    }

    LineState = 0;

    return(0);
  }

  rx1 = LineLastX;       /* start point of line   */
  ry1 = LineLastY;

  gp_sxrec( &rec, rx1, ry1, x, y);

  if( ! gp_touchrec( &ClipRect, &rec)) {

    // clipped
    return(0);
  }

  dx = x - rx1;                  /* Length of axes */
  dy = y - ry1;
  if ( dx == 0 && dy == 0 )
     return(0);                   /* start point == end point */

  if( dx < 0 )  {                 /* x-direction: backward  */
    xa = -1;
    adx = -dx;
  } else {                        /* x-direction: forward   */
    xa = 1;
    adx = dx;
  }
  if( dy < 0 )  {                 /* Negative y-direction    */
    ya = -1;
    ady = -dy;
  } else {                        /* Positive y-direction   */
    ya = 1;
    ady = dy;
  }
  if( adx > ady ) {               /* more horizontal than vertical */
    incr1 = ady << 1;
    d = incr1 - adx;
    incr2 = incr1 - ( adx << 1 );
    while( rx1 != x ) {           /* while end point not reached */
      rx1 += xa;
      if( d < 0 ) {
        d += incr1;
      } else {
        ry1 += ya;
        d += incr2;
      }

      YaIPS_G2D_WPix( rx1, ry1, Value);
    }
  } else {                      /* more vertical than horizontal */
    incr1 = adx << 1;
    d = incr1 - ady;
    incr2 = incr1 - ( ady << 1 );
    while( ry1 != y ) {           /* while end point not reached */
      ry1 += ya;
      if( d < 0 ) {
        d += incr1;
      } else {
        rx1 += xa;
        d += incr2;
      }

      YaIPS_G2D_WPix( rx1, ry1, Value);
    }
  }

  LineLastX = x;            // Set end point of last line drawn
  LineLastY = y;

  return(0);
}

/*********************************************************************
* YaIPS_G2D_LineBegin()
*
* Mark start of line chain.
***********************************************************************
*/
int YaIPS_G2D_LineBegin()
{

  IsInitalized();      // Check 2D graphic is initialized

  LineState = 1;       /* flag for start of chain  */
  return(0);
}

/*********************************************************************
* int YaIPS_G2D_XLineBegin()
*
* Mark start of line chain for closed polygons
***********************************************************************
*/
int YaIPS_G2D_XLineBegin()
{

  IsInitalized();      // Check 2D graphic is initialized

  LineState = 2;       /* flag for start of chain  */
  return(0);
}

/*********************************************************************
* YaIPS_G2D_MoveTo()
*
* Set start point of line.
***************************************************************************
*/
int YaIPS_G2D_MoveTo( int x, int y, int Value)
{
  int ierr;

  IsInitalized();      // Check 2D graphic is initialized

  ierr = YaIPS_G2D_LineBegin();          /* set flag for start of chain */
  if(  ierr != 0) {
    return(ierr);
  }

  ierr = YaIPS_G2D_LineTo( x, y, Value); /* set start point  */

  return( ierr);
}

/*********************************************************************
* YaIPS_G2D_XMoveTo()
*
* Set start point of line for closed polygons
***************************************************************************
*/
int YaIPS_G2D_XMoveTo( int x, int y, int Value)
{
  int ierr;

  IsInitalized();      // Check 2D graphic is initialized

  ierr = YaIPS_G2D_XLineBegin();         /* set flag for start of chain */
  if(  ierr != 0) {
    return(ierr);
  }

  ierr = YaIPS_G2D_LineTo( x, y, Value); /* set start point  */

  return( ierr);
}

/*********************************************************************
* YaIPS_G2D_Line()
*
* Draw a line
***************************************************************************
*/
int YaIPS_G2D_Line( int x0, int y0, int x1, int y1, int Value)
{
  int ierr;

  IsInitalized();            // Check 2D graphic is initialized

  if( y0 == y1) {            // Is a horizontal line

    ierr = YaIPS_G2D_WPixLineX( x0, x1, y0, Value);

  } else if( x0 == y0) {     // Is a vertical line

    ierr = YaIPS_G2D_WPixLineY( x0, y0, y1, Value);

  } else {

    ierr = YaIPS_G2D_MoveTo( x0, y0, Value);  // Line starts herr
    if(  ierr != 0) {
      return(ierr);
    }

    ierr = YaIPS_G2D_LineTo( x1, y1, Value);  // Line ends here
  }

  return( ierr);
}

/*********************************************************************
* YaIPS_G2D_Line()
*
* Draw a thick line
***************************************************************************
*/
int YaIPS_G2D_Line( int x0, int y0, int x1, int y1, int width, int Value)
{
  int ierr;
  int dx, dy;              /* axes of line             */
  int adx, ady;            /* distance                 */
  int xa, ya;              /* direction                */
  int d;          /* distance already done    */
  int incr1, incr2;        /* distance per step        */

  IsInitalized();      // Check 2D graphic is initialized

  dx = x1 - x0;                  /* lenght of axes */

  // Check for thin lines

  if( width <= 1) {              // Thin line

    ierr = YaIPS_G2D_Line( x0, y0, x1, y1, Value);

    return( ierr);
  }

  // ..

  dy = y1 - y0;

  if( dx == 0 && dy == 0 ) {

    if( width <= 1) {

      YaIPS_G2D_Line( x0, y0, x0, y0, Value);
    } else {
      YaIPS_G2D_Ellipse( x0, y0, width >> 1, width >> 1, Value, TRUE);
    }

    return(0);                   /* startpoint == endpoint */
  }

  if( dx < 0 )  {                 /* x-direction: backward  */
    xa = -1;
    adx = -dx;
  } else {                        /* x-direction: forward   */
    xa = 1;
    adx = dx;
  }
  if( dy < 0 )  {                 /* Negative y-direction    */
    ya = -1;
    ady = -dy;
  } else {                        /* Positive y-direction   */
    ya = 1;
    ady = dy;
  }
  if( adx > ady ) {               /* more horizontal than vertical */
    incr1 = ady << 1;
    d = incr1 - adx;
    incr2 = incr1 - ( adx << 1 );
    while( x0 != x1 ) {           /* while end point not reached */
      x0 += xa;
      if( d < 0 ) {
        d += incr1;
      } else {
        y0 += ya;
        d += incr2;
      }
      if (width <= 1) {
        YaIPS_G2D_Line( x0, y0, x0, y0, Value);
      } else {
        YaIPS_G2D_Ellipse( x0, y0, width >> 1, width >> 1, Value, TRUE);
      }
    }
  } else {                      /* more vertical than horizontal */
    incr1 = adx << 1;
    d = incr1 - ady;
    incr2 = incr1 - ( ady << 1 );
    while( y0 != y1 ) {           /* while endpoint not reached */
      y0 += ya;
      if( d < 0 ) {
        d += incr1;
      } else {
        x0 += xa;
        d += incr2;
      }
      if (width <= 1) {
        YaIPS_G2D_Line( x0, y0, x0, y0, Value);
      } else {
        YaIPS_G2D_Ellipse( x0, y0, width >> 1, width >> 1, Value, TRUE);
      }
    }
  }

  return(0);
}

/*********************************************************************
* YaIPS_G2D_Rectangle()
*
* Draw a rectangle
***************************************************************************
*/
int YaIPS_G2D_Rectangle( int x1, int y1, int x2, int y2, int Value, int Fill)
{
  int ierr, tmp;
  Tgrect rec;                         /* clipping rectangle       */

  IsInitalized();      // Check 2D graphic is initialized

  if(x2 < x1) { tmp = x1; x1 = x2; x2 = tmp; }  // Ensure 1. point is top right
  if(y2 < y1) { tmp = y1; y1 = y2; y2 = tmp; }

  gp_sxrec( &rec, x1, y1, x2, y2);

  if( ! gp_touchrec( &ClipRect, &rec)) {

    // clipped
    return(0);
  }

  if( Fill) {                                     // Draw a filled rectangle

    for( ; y1 <= y2; y1++) {

      if( y1 < 0) {

        continue;
      }

      if( y1 >= ClipRect.rya + ClipRect.ryy) {

        break;
      }

      ierr = YaIPS_G2D_Line( x1, y1, x2, y1, Value);
      if(  ierr != 0) {
        return(ierr);
      }
    }

  } else {                                        // Draw rectangle frames

    ierr = YaIPS_G2D_Line( x1, y1, x2, y1, Value);   // Top line
    if(  ierr != 0) {
      return(ierr);
    }

    if( rec.ryy == 1) {                             // Was only one line

      return( 0);    // Return OK
    }

    ierr = YaIPS_G2D_Line( x1, y2, x2, y2, Value);  // Bottom line
    if(  ierr != 0) {
      return(ierr);
    }

    if( rec.ryy == 2) {                             // Was only two lines thick

      return( 0);    // Return OK
    }

    ierr = YaIPS_G2D_Line( x1, y1 + 1, x1, y2 - 1, Value);  // Left line
    if(  ierr != 0) {
      return(ierr);
    }

    if( rec.rxx == 1) {                             // Was only one line

      return( 0);    // Return OK
    }

    ierr = YaIPS_G2D_Line( x2, y1 + 1, x2, y2 - 1, Value);  // Left line
    if(  ierr != 0) {
      return(ierr);
    }
  }

  return( 0);    // Return OK
}

/*********************************************************************
* YaIPS_G2D_Ellipse()
*
* Draw an ellipse
***************************************************************************
*/

#define   PREC5      20
#define   PRECMASK5  0x0fffffL
#define   PREC6      24
#define   PRECMASK6  0x0ffffffL

#define   PREC       PREC6
#define   PRECMASK   PRECMASK6

#define   SLOPE      1

#define   ONE        ( (int32)0x01 << PREC )
#define   MAXSLOPE   ONE

/*-------------------------------------------------------------------------*/

static int pqupix( int x0, int y0, int x, int y, int draw_line, int Value)
{
  int ierr;
  int xf,yf,nf;

  if ( (ierr = YaIPS_G2D_WPix( x0 + x, y0 + y, Value)) )
      return(ierr);
  if ( (ierr = YaIPS_G2D_WPix( x0 + x, y0 - y, Value)) )
      return(ierr);
  if ( (ierr = YaIPS_G2D_WPix( x0 - x, y0 + y, Value)) )
      return(ierr);
  if ( (ierr = YaIPS_G2D_WPix( x0 - x, y0 - y, Value)) )
      return(ierr);

  if( draw_line) {

     yf = y0;
     xf = x0 - x;

     nf = (x<<1) - 1;

     if ( (ierr = YaIPS_G2D_Line( xf + 1, yf + (y-1), xf + 1 + nf, yf + (y-1), Value)) )     return(ierr);

     if ( (ierr = YaIPS_G2D_Line( xf + 1, yf - (y-1), xf + 1 + nf, yf - (y-1), Value)) )     return(ierr);
  }

  return(0);
}

/*-------------------------------------------------------------------------*/

int YaIPS_G2D_Ellipse( int x0, int y0, int xr, int yr, int Value, int Fill)
{
  int ierr;
  int x,y;
  unsigned int alpha,beta,tmp;
  unsigned int    xri,yri;
  int    xf,yf,nf;

  IsInitalized();      // Check 2D graphic is initialized

  if (xr <= 0 || yr <= 0)   return(0);

  /*---------------------------------------------------------------------*/
  /* Equation of ellipsis: (x * x)/(xr * xr) + (y * y)/(yr * yr) = 1     */
  /* alpha is the value of the equation of the next discrete pixel out-  */
  /* side the ellipsis contour, beta is the value of the next discrete   */
  /* pixel inside. The nearer to the contour is choosen.                 */
  /*---------------------------------------------------------------------*/

  xri = ONE / (xr*xr);
  yri = ONE / (yr*yr);

  x = xr;
  y = 1;

  /* draw the two points left and right in the middle (y) of the ellipsis */
  if ( (ierr = YaIPS_G2D_WPix( x0 + x, y0, Value)) )
      return(ierr);
  if ( (ierr = YaIPS_G2D_WPix( x0 - x, y0, Value)) )
      return(ierr);

  /* fill the horizontal middle line (y) of the ellipsis */
  if( Fill) {

     xf = x0 -x;
     yf = y0;
     nf = (xr << 1) - 1;

     if( (ierr = YaIPS_G2D_Line( xf + 1, yf, xf + 1 + nf, yf, Value)) ) return(ierr);
  }

  /* draw rest of ellipse except upper and lower row */
  while( y < yr )
  {
    tmp = yri * y * y;
    alpha = xri * x * x + tmp;                  /* actual pixel (outside) */
    beta = xri * (x - 1) * (x - 1) + tmp;       /* pixel inside */

    while (( alpha > ONE ) &&
     ( ( (beta < ONE) && ((alpha - ONE) > (ONE - beta)) ) ||
         (beta > ONE) ) ) {
       /* the inner pixel is nearer */
       --x;
       alpha = beta;
       beta = xri * (x - 1) * (x - 1) + tmp;

       /* draw only contour points */
       if( (ierr = pqupix( x0, y0, x, y, FALSE, Value))) return(ierr);
    }

    /* the outer pixel is nearer */

    if (y == 1) {
      /* prevent to fill the center line twice */
      if( ( ierr = pqupix( x0, y0, x, y, FALSE, Value))) return(ierr);
    }

    y++;

    if( ( ierr = pqupix( x0, y0, x, y, Fill, Value)))  return(ierr);

  }             /* end while */

  /* draw upper and lower row */
  while ( x > 0 )
  {
     if( ( ierr = pqupix( x0, y0, x, y, FALSE, Value)))  return(ierr);

     x--;
  }
  /* draw the two points (up and down) in the middle (x) of the ellipsis */
  if ( (ierr = YaIPS_G2D_WPix( x0, y0 + y, Value)) )
      return(ierr);
  if ( (ierr = YaIPS_G2D_WPix( x0, y0 - y, Value)) )
      return(ierr);

  return( 0);    // Return OK
}

/*********************************************************************
* YaIPS_G2D_Ellipse()
*
* Fill a (closed) graphic object.
***************************************************************************
*/

#define FILL_COORDBUF 256

// Compare pixel value for equal
static int YaIPS_G2D_Pixel_IsEqual( uchar *pPix1, uchar *pPix2)
{
  int iPix;

  for( iPix = 0; iPix < CurrImg.d; iPix++) {

    if( (pPix1[ iPix] & BitMask[ iPix]) != (pPix2[ iPix] & BitMask[ iPix]) ) {

      return( false);
    }
  }

  return( true);
}

int YaIPS_G2D_Fill( int x0, int y0, int FillValue, int PickLimit, int LimitValue)
{
  int  fillpx[ FILL_COORDBUF];
  int  fillpy[ FILL_COORDBUF];
  uchar LimitBytes[ 4], FillBytes[ 4]; // Pixel values
  uchar *pPix;
  int *xfsp,*yfsp;      /* pointer to store   coordinates */
  int *xfrp,*yfrp;      /* pointer to restore coordinates */
  int x, y;
  int ierr, iPix, IsEqual;
  int *pre;                    /* pointer to end of x-buffer */
  int xsave,ysave, xleft,xright;

  IsInitalized();      // Check 2D graphic is initialized

  rangecheckx( x0);    // Check coordinate to be in range
  rangechecky( y0);    // Check coordinate to be in range

  if( PickLimit == false) {

    YAIPS_G2D_INT_2_BYTES( LimitBytes, LimitValue);

  } else {

    // Pick limit color

    pPix = RGB_pixad( x0, y0, &CurrImg);

    for( iPix = 0; iPix < 4; iPix++) {

      if( iPix < CurrImg.d) {

        LimitBytes[ iPix] = pPix[ iPix] & BitMask[ iPix];

      } else {

        LimitBytes[ iPix] = 0;
      }
    }
  }

  YAIPS_G2D_INT_2_BYTES( FillBytes, FillValue);


  xfsp = xfrp = fillpx;
  yfsp = yfrp = fillpy;
  pre = xfsp + FILL_COORDBUF - 1;

  /* seed pixel */
  *xfsp++ = x0;
  *yfsp++ = y0;

  while( xfsp != xfrp ) {

     x = *xfrp++;                          /* pop pixels */
     y = *yfrp++;

     if ( xfrp > pre ) {                   /* wrap around */
       xfrp = fillpx;  yfrp = fillpy;
     }

     xsave = x++;                          /* fill the span to the right */

     /* get hole line */

     pPix = RGB_pixad( xsave, y, &CurrImg);

     if( YaIPS_G2D_Pixel_IsEqual( pPix, FillBytes)) {
        /* i'm not the first on this position */
       continue;
     }

     // Fill to the right

     pPix = RGB_pixad( x, y, &CurrImg);

     for( ; x < CurrImg.xx; x++, pPix += CurrImg.d) {

       IsEqual = YaIPS_G2D_Pixel_IsEqual( pPix, LimitBytes);
       if ( (PickLimit == false && IsEqual == true) ||
            (PickLimit != false && IsEqual != true)) break;

       // Set color
       for( iPix = 0; iPix < CurrImg.d; iPix++) {

         pPix[ iPix] = (pPix[ iPix] & BitIMask[ iPix]) | (FillBytes[ iPix] & BitMask[ iPix]);
       }
     }

     xright = x - 1;
     x = xsave;

     // Fill to the left

     pPix = RGB_pixad( x, y, &CurrImg);

     for( ; x >= 0; x--, pPix -= CurrImg.d) {

       IsEqual = YaIPS_G2D_Pixel_IsEqual( pPix, LimitBytes);
       if ( (PickLimit == false && IsEqual == true) ||
            (PickLimit != false && IsEqual != true)) break;

       // Set color
       for( iPix = 0; iPix < CurrImg.d; iPix++) {

         pPix[ iPix] = (pPix[ iPix] & BitIMask[ iPix]) | (FillBytes[ iPix] & BitMask[ iPix]);
       }
     }

     xleft = x + 1;
     x = xsave;

     ysave = y;

     /* check scan line above and below */
     for( y = ysave + 1; y >= ysave-1; y -= 2) {

       if( y <  0)             break;
       if( y >=  CurrImg.yy)   continue;

       x = xleft;

       pPix = RGB_pixad( x, y, &CurrImg);

       while( x <= xright) {

         IsEqual = YaIPS_G2D_Pixel_IsEqual( pPix, LimitBytes);
         if ( (PickLimit == false && IsEqual == true) ||
              (PickLimit != false && IsEqual != true) ||
              YaIPS_G2D_Pixel_IsEqual( pPix, FillBytes)) {

             x++;
             pPix += CurrImg.d;

          } else {

             while( x <= xright ) {

               IsEqual = YaIPS_G2D_Pixel_IsEqual( pPix, LimitBytes);
               if ( (PickLimit == false && IsEqual == true) ||
                    (PickLimit != false && IsEqual != true)) break;

               x++;
               pPix += CurrImg.d;
             }

             *xfsp++ = x-1;  /* push extreme right pixel */
             *yfsp++ = y;

             if ( xfsp > pre ) {                        /* wrap around */
               xfsp = fillpx;  yfsp = fillpy;
             }
             if( xfsp == xfrp) {
               //x/printf2Console("warning: gp_fill: overflow ");
             }
             x++;
             pPix += CurrImg.d;
          }
       }               /* end x-loop scancheck */
     }                 /* end y-loop scancheck */
  }                 /* end while */
  ierr = 0;

//x/error:

  return(ierr);
}

/***************************************************************************
* YaIPS_RGB_Frame
* Generate frame around image
*
* width      width of boundary = abs(width)
*            > 0:  extrapolate mode
*            < 0:  set mode
* value       value to set if "width" has negative sign else dummy
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_G2D_Frame( int width,                // abs(width) = width of boundary
                     int value)                // value to set if width has negative sign else dummy
{
  int w, d, x, y;
  int xmin, ymin, jump, nByt;
  uchar *s18,  *s28,  *d18,  *d28;
  uchar v4[ 4];

  // Get image data

  xmin = CurrImg.xx;
  ymin = CurrImg.yy;
  jump = CurrImg.ld;
  nByt = CurrImg.d;

  if( width > 0) {                            // extrapolate

    for( w = 0; w < width; w++) {

      s18 = RGB_pixad( 0, width - w, &CurrImg);
      s28 = RGB_pixad( 0, ymin - width - 1 + w, &CurrImg);
      d18 = s18 - jump;
      d28 = s28 + jump;

      for( x = 0; x < xmin; x++) {

        for( d = 0; d < nByt; d++) {

          *d18 = (*d18 & BitIMask[ d]) | (*s18++ & BitMask[ d]);  /* upper boundary */
          *d28 = (*d28 & BitIMask[ d]) | (*s28++ & BitMask[ d]);  /* lower boundary */

          d18 += 1;
          d28 += 1;
        }
      }

      d18 =  RGB_pixad( width - w - 1, 0, &CurrImg);    /* left boundary */
      d28 =  RGB_pixad( xmin - width + w, 0, &CurrImg); /* right boundary */

      for( y = 0; y < ymin; y++) {

        for( d = 0; d < nByt; d++) {
          *(d18 + d) = (*(d18 + d) & BitIMask[ d]) | (*(d18 + nByt + d) & BitMask[ d]);
          *(d28 + d) = (*(d28 + d) & BitIMask[ d]) | (*(d28 - nByt + d) & BitMask[ d]);
        }
        d18 += jump;
        d28 += jump;
      }
    }

    return(0);

  } else  {                                   // clear/set with value

    w = -width;

    v4[ 0] = value & 0xff;
    v4[ 1] = (value >>  8) & 0xff;
    v4[ 2] = (value >> 16) & 0xff;
    v4[ 3] = (value >> 24) & 0xff;

    while ( --w >= 0 ) {

      d18 = RGB_pixad( 0, w, &CurrImg);
      d28 = RGB_pixad( 0, ymin-w-1, &CurrImg);

      x = xmin;
      while ( --x >= 0 ) {

        for( d = 0; d < nByt; d++) {
          *(d18 + d) = (*(d18 + d) & BitIMask[ d]) | (v4[ d] & BitMask[ d]);
          *(d28 + d) = (*(d28 + d) & BitIMask[ d]) | (v4[ d] & BitMask[ d]);
        }

        d18 += nByt;
        d28 += nByt;
      }

      d18 =  RGB_pixad(w, 0, &CurrImg);
      d28 =  RGB_pixad(xmin-w-1, 0, &CurrImg);

      y = ymin;
      while ( --y >= 0 ) {

        for( d = 0; d < nByt; d++) {

          *(d18 + d) = (*(d18 + d) & BitIMask[ d]) | (v4[ d] & BitMask[ d]);
          *(d28 + d) = (*(d28 + d) & BitIMask[ d]) | (v4[ d] & BitMask[ d]);
        }
        d18 += jump;
        d28 += jump;
      }
    }

    return(0);
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_G2D_Morphology
* Inplace 3*3 morphology filter
*
* MorphOp      Morphology operator, see YAIPS_RGB_MORPH_XXX defines
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_G2D_Morphology( int MorphOp)          // Morphology operator, see YAIPS_RGB_MORPH_XXX defines
{
  int ierr, d, x, y, nByt, KernelSize, KernelSize2;
  int xmin, ymin, jump, t1, t2, t3;
  int Off00, Off01, Off02, Off10, Off11, Off12, Off20, Off21, Off22;
  uchar *s80, *d80, *s8, *d8;
  uchar *pLineBuffer = NULL;           // Temporary line buffer
  uchar *pLine0, *pLine1, *pLine2, *pLineNext, CurrMask, CurrIMask;

  IsInitalized();      // Check 2D graphic is initialized

  KernelSize = 3;

  // Get image data

  xmin = CurrImg.xx;
  ymin = CurrImg.yy;
  jump = CurrImg.ld;
  nByt = CurrImg.d;

  // Prepare ...

  KernelSize2 = KernelSize / 2;               // 1/2 kernel size. Is also border width

  xmin -= KernelSize2 + KernelSize2;          // Process only inner pixels
  ymin -= KernelSize2 + KernelSize2;

  Off00 = 0;
  Off01 = nByt;
  Off02 = nByt + nByt;
  Off10 = Off00 + jump;
  Off11 = Off01 + jump;
  Off12 = Off02 + jump;
  Off20 = Off10 + jump;
  Off21 = Off11 + jump;
  Off22 = Off12 + jump;

  // Allocate line buffer for 3 lines

  pLineBuffer = (uchar *)malloc( CurrImg.xx * nByt * 3);

  if( pLineBuffer == NULL) {

    return( -90);            // ERROR, memory allocation failed
  }

  pLine0 = pLineBuffer;
  pLine1 = pLine0 + CurrImg.xx * nByt;
  pLine2 = pLine1 + CurrImg.xx * nByt;

  // Copy the first two lines

  s80 = RGB_pixad( 0,  0, &CurrImg);
  memcpy( pLine0, s80, CurrImg.xx * nByt);

  s80 = RGB_pixad( 0,  1, &CurrImg);
  memcpy( pLine1, s80, CurrImg.xx * nByt);

  pLineNext = pLine2;      // Next line to copy

  // Filter ...

  for( y = 0; y < ymin; y++) {

    // Copy next line into the circular line buffer

    s80 = RGB_pixad( 0, y + 2, &CurrImg);
    memcpy( pLineNext, s80, CurrImg.xx * nByt);

    // Next line to copy to
    if( pLineNext == pLine2) {
      pLineNext = pLine0;
    } else if( pLineNext == pLine0) {
      pLineNext = pLine1;
    } else {
      pLineNext = pLine2;
    }

    // ...

    s80 = pLine0;
    d80 = RGB_pixad( KernelSize2, y + KernelSize2, &CurrImg);

    for( d = 0; d < nByt; d++) {

      CurrMask  = BitMask[ d];   // Bit mask for this channel
      CurrIMask = BitIMask[ d];  // Inverted mask

      if( CurrMask == 0) {       // Don't process this channel

        continue;
      }

      s8 = s80 + d;
      d8 = d80 + d;

      switch( MorphOp) {
      default:

        // Operator not supported

        if( errstring == NULL) {                  // No error until now

          sprintf( errbuffer, LangStringLookup( "&RGB_Morph1=Filter is not supported"));

          errstring = errbuffer;
        }

        return(-99);


        break;

      case YAIPS_RGB_MORPH_EROSION:

        for( x = 0; x < xmin; x++) {

          t1 = s8[ Off00] & CurrMask;
          t2 = s8[ Off01] & CurrMask; if( t2 < t1) t1 = t2;
          t2 = s8[ Off02] & CurrMask; if( t2 < t1) t1 = t2;
          t2 = s8[ Off10] & CurrMask; if( t2 < t1) t1 = t2;
          t2 = s8[ Off11] & CurrMask; if( t2 < t1) t1 = t2;
          t2 = s8[ Off12] & CurrMask; if( t2 < t1) t1 = t2;
          t2 = s8[ Off20] & CurrMask; if( t2 < t1) t1 = t2;
          t2 = s8[ Off21] & CurrMask; if( t2 < t1) t1 = t2;
          t2 = s8[ Off22] & CurrMask; if( t2 < t1) t1 = t2;

          *d8 = (*d8 & CurrIMask) | t1;      // Store

          s8 += nByt;
          d8 += nByt;
        }
        break;

      case YAIPS_RGB_MORPH_DILATION:

        for( x = 0; x < xmin; x++) {

          t1 = s8[ Off00] & CurrMask;
          t2 = s8[ Off01] & CurrMask; if( t2 > t1) t1 = t2;
          t2 = s8[ Off02] & CurrMask; if( t2 > t1) t1 = t2;
          t2 = s8[ Off10] & CurrMask; if( t2 > t1) t1 = t2;
          t2 = s8[ Off11] & CurrMask; if( t2 > t1) t1 = t2;
          t2 = s8[ Off12] & CurrMask; if( t2 > t1) t1 = t2;
          t2 = s8[ Off20] & CurrMask; if( t2 > t1) t1 = t2;
          t2 = s8[ Off21] & CurrMask; if( t2 > t1) t1 = t2;
          t2 = s8[ Off22] & CurrMask; if( t2 > t1) t1 = t2;

          *d8 = (*d8 & CurrIMask) | t1;      // Store

          s8 += nByt;
          d8 += nByt;
        }
        break;

      case YAIPS_RGB_MORPH_MEDIAN:

        {
          int v[ 9];
          int i, iBot, iTop, Exchange;

          for( x = 0; x < xmin; x++) {

            // Get the 9 values

            v[ 0] = s8[ Off00] & CurrMask;
            v[ 1] = s8[ Off01] & CurrMask;
            v[ 2] = s8[ Off02] & CurrMask;
            v[ 3] = s8[ Off10] & CurrMask;
            v[ 4] = s8[ Off11] & CurrMask;
            v[ 5] = s8[ Off12] & CurrMask;
            v[ 6] = s8[ Off20] & CurrMask;
            v[ 7] = s8[ Off21] & CurrMask;
            v[ 8] = s8[ Off22] & CurrMask;

            // Sort

            iBot = 0;
            iTop = 8;

            for( ; ; ) {

              Exchange = false;

              // Bubble up
              for( i = iBot; i < iTop; i++) {

                if( v[ i] > v[ i + 1]) {

                  Exchange = true;

                  t1 =  v[ i + 1];
                  v[ i + 1] =  v[ i];
                  v[ i] = t1;
                }
              }

              if( ! Exchange) {

                break;
              }

              iTop -= 1;   // For sure, top element is the maximum

              // Bubble down
              for( i = iTop - 1; i >= iBot; i--) {

                if( v[ i] > v[ i + 1]) {

                  Exchange = true;

                  t1 =  v[ i + 1];
                  v[ i + 1] =  v[ i];
                  v[ i] = t1;
                }
              }

              if( ! Exchange) {

                break;
              }

              iBot += 1;  // For sure, bottom element is the minimum
            }

            // Store

            t1 =  v[ 4];              // Get the median value

            *d8 = (*d8 & CurrIMask) | t1;      // Store

            s8 += nByt;
            d8 += nByt;
          }
        }
        break;

      case YAIPS_RGB_MORPH_GRADIENT:

        for( x = 0; x < xmin; x++) {

          t1 = s8[ Off00];
          t3 = t1;
          t2 = s8[ Off01]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off02]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off10]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off11]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off12]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off20]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off21]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;
          t2 = s8[ Off22]; if( t2 < t1) t1 = t2; if( t2 > t3) t3 = t2;

          *d8 = t3 - t1;      // Store

          s8 += nByt;
          d8 += nByt;
        }
        break;

      }  // End switch()
    }
  }

  // Free temporary line buffer

  free( pLineBuffer);

  // Extrapolate frame
  ierr = YaIPS_G2D_Frame( KernelSize2, 0);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  return( 0);                                 // Return OK
}

/***************************************************************************
* YaIPS_G2D_UnDo_Save
* Save the graphic from the image to the undo FIFO
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_G2D_UnDo_Save()
{
  int x, y, iPix, ByteMask, ByteData, TempBitMask;
  uchar *pFIFOimage, *pPix;

  IsInitalized();                      // Check 2D graphic is initialized

  if( pCurrUnDo == NULL ||             // Check undo FIFO is initialized
      pCurrUnDo->pBuffer == NULL) {

    return( YAIPS_G2D_ERR_NO_UNDO);
  }

  // Restore info from last undo

  pCurrUnDo->UnDoFIFO_Put = pCurrUnDo->UnDoFIFO_LastPut;
  pCurrUnDo->UnDoFIFO_N   = pCurrUnDo->UnDoFIFO_LastN;

  // put image data

  pFIFOimage = pCurrUnDo->pBuffer + pCurrUnDo->UnDoFIFO_Put * pCurrUnDo->UnDoFIFO_Size;

  if( pCurrUnDo->UndoMode == YAIPS_G2D_UNDO_MODE_BYTE) {   // Save all masked bytes

    for( y = 0; y < CurrImg.yy; y++) {

      pPix  = RGB_pixad( 0, y, &CurrImg);

      for( x = 0; x < CurrImg.xx; x++) {

        for( iPix = iFirstUsed; iPix <= iLastUsed; iPix++) {

          if( BitMask[ iPix] == 0) {           // This  byte is not used

            continue;
          }

          *pFIFOimage++ = pPix[ iPix];
        }

        pPix += CurrImg.d;
      }
    }

  } else {                                                 // Only on mask bit is set

    ByteMask = 0x01;
    ByteData = 0;

    TempBitMask = BitMask[ iFirstUsed];

    for( y = 0; y < CurrImg.yy; y++) {

      pPix  = RGB_pixad( 0, y, &CurrImg);

      for( x = 0; x < CurrImg.xx; x++) {

        if( pPix[ iFirstUsed] & TempBitMask) {

          ByteData |= ByteMask;
        }

        ByteMask <<= 1;                    // Shift one bit

        if( ByteMask > 0x80) {             // Overflow from last time

          *pFIFOimage++ = ByteData;        // Store to FIFO

          ByteMask = 0x01;
          ByteData = 0;
        }

        pPix += CurrImg.d;
      }
    }

    if( ByteMask != 0x01) {                // Some data to store

      *pFIFOimage++ = ByteData;            // Store to FIFO
    }
  }

  // advance count, put pointer

  if( pCurrUnDo->UnDoFIFO_N < pCurrUnDo->UnDoFIFO_Max) {           // not have reached max FIFO count

    pCurrUnDo->UnDoFIFO_N += 1;                            // we have one level more pushed into the FIFO
  }

  pCurrUnDo->UnDoFIFO_Put += 1;                            // advance put pointer

  if( pCurrUnDo->UnDoFIFO_Put >= pCurrUnDo->UnDoFIFO_Max) {        // wrap around case ?

    pCurrUnDo->UnDoFIFO_Put = 0;
  }

  // save info for undo

  pCurrUnDo->UnDoFIFO_LastPut = pCurrUnDo->UnDoFIFO_Put;
  pCurrUnDo->UnDoFIFO_LastN   = pCurrUnDo->UnDoFIFO_N;

  return( 0);      // Return OK
}

/***************************************************************************
* YaIPS_G2D_UnDo_NextPrev
*
* Move the pointer in the undo FIFO and restore image if pointer has moved.
*
* Direction   +1: move to next, -1: move to previous
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_G2D_UnDo_NextPrev( int Direction)  // +1: move to next, -1: move to previous
{
  int x, y, iPix, ByteMask, ByteData, TempBitMask, TempBitIMask, iDispFIFO;
  uchar *pFIFOimage, *pPix;

  IsInitalized();                      // Check 2D graphic is initialized

  if( pCurrUnDo == NULL ||             // Check undo FIFO is initialized
      pCurrUnDo->pBuffer == NULL) {

    return( YAIPS_G2D_ERR_NO_UNDO);
  }

  // decode, which direction we have to advance

  if( Direction < 0) {                                // Next ?

    if( pCurrUnDo->UnDoFIFO_LastN <= 1) {             // already reached the bottom

      return( 0);                                     // Nothing to do, return OK
    }

    pCurrUnDo->UnDoFIFO_LastN -= 1;                   // one less

    pCurrUnDo->UnDoFIFO_LastPut -= 1;                 // one before ?
    if( pCurrUnDo->UnDoFIFO_LastPut < 0) {            // wrap around ?

      pCurrUnDo->UnDoFIFO_LastPut = pCurrUnDo->UnDoFIFO_Max - 1;
    }

  } else if( Direction > 0) {                         // Previous ?

    if( pCurrUnDo->UnDoFIFO_LastN >= pCurrUnDo->UnDoFIFO_N) {  // already reached the top

      return( 0);                                     // Nothing to do, return OK
    }

    pCurrUnDo->UnDoFIFO_LastN += 1;                   // one more

    pCurrUnDo->UnDoFIFO_LastPut += 1;                 // one after ?
    if( pCurrUnDo->UnDoFIFO_LastPut >= pCurrUnDo->UnDoFIFO_Max) {     // wrap around ?

      pCurrUnDo->UnDoFIFO_LastPut = 0;
    }

  } else {

    // Direction has a not defined value

    return( 0);                                       // return OK
  }

  // We display one before

  iDispFIFO = pCurrUnDo->UnDoFIFO_LastPut;
  iDispFIFO -= 1;                                     // one before ?
  if( iDispFIFO < 0) {                                // wrap around ?

    iDispFIFO = pCurrUnDo->UnDoFIFO_Max - 1;
  }

  // get info, what to display

  pFIFOimage = pCurrUnDo->pBuffer + iDispFIFO * pCurrUnDo->UnDoFIFO_Size;

  if( pCurrUnDo->UndoMode == YAIPS_G2D_UNDO_MODE_BYTE) {   // Save all masked bytes

    for( y = 0; y < CurrImg.yy; y++) {

      pPix  = RGB_pixad( 0, y, &CurrImg);

      for( x = 0; x < CurrImg.xx; x++) {

        for( iPix = iFirstUsed; iPix <= iLastUsed; iPix++) {

          if( BitMask[ iPix] == 0) {           // This  byte is not used

            continue;
          }

          pPix[ iPix] = (pPix[ iPix] & BitIMask[ iPix]) | (*pFIFOimage & BitMask[ iPix]);

          pFIFOimage += 1;
        }

        pPix += CurrImg.d;
      }
    }

  } else {                                                 // Only on mask bit is set

    ByteMask = 0x01;
    ByteData = 0;

    TempBitMask = BitMask[ iFirstUsed];
    TempBitIMask = BitIMask[ iFirstUsed];

    for( y = 0; y < CurrImg.yy; y++) {

      pPix  = RGB_pixad( 0, y, &CurrImg);

      for( x = 0; x < CurrImg.xx; x++) {

        if( ByteMask == 0x01) {               // First bit

          ByteData = *pFIFOimage++;           // Get byte from FIFO
        }

        pPix[ iFirstUsed] &= TempBitIMask;    // Reset bit in destination

        if( ByteData & ByteMask) {            // Bit is set in source ?

          pPix[ iFirstUsed] |= TempBitMask;   // Set bit in destination
        }

        ByteMask <<= 1;                       // Shift one bit

        if( ByteMask > 0x80) {                // Overflow from last time

          ByteMask = 0x01;
        }

        pPix += CurrImg.d;
      }
    }
  }

  return( 0);      // Return OK
}

/***************************************************************************
* YaIPS_G2D_UnDo_Enables
*
* Get info about enables for next and previous undo button.
*
* return   < 0 Error
*         >= 0 and  Bit 0 set     There is a Next
*         >= 0 and  Bit 1 set     There is a Previous
****************************************************************************
*/

int YaIPS_G2D_UnDo_Enables()
{
  int ierr;

  IsInitalized();                      // Check 2D graphic is initialized

  if( pCurrUnDo == NULL ||             // Check undo FIFO is initialized
      pCurrUnDo->pBuffer == NULL) {

    return( YAIPS_G2D_ERR_NO_UNDO);
  }

  // ...

  ierr  = 0;   // preset none enabled

  if( pCurrUnDo->UnDoFIFO_LastN < pCurrUnDo->UnDoFIFO_N) {  // not reached the top

    ierr |= 0x01;                                           // enable Next
  }

  if( pCurrUnDo->UnDoFIFO_LastN > 1) {                      // more before

    ierr |= 0x02;                                           // enable Previous
  }

  return( ierr);
}

/******************************** End Of File ********************************/

