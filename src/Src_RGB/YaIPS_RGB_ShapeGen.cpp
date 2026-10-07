/****************************************************************************

  YaIPS_RGB_ShapeGen.cpp

  Fl_RGB_Image image processing.
  Shape generation code

 11.04.2025 RR: First edition of this file.
 30.09.2026 RR: * YaIPS_RGB_CopyBgndToOverlay()
                  Optimization when resizing images.
                  Whenever appropriate, the source image is scaled down
                  by powers of two. As a result, the subsequent
                  nearest-neighbor resizing produces better results.

*****************************************************************************
*/

#define USE_OVERLAY_SCALE_HACK     1  // Define this to use a hack for better scale results.

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
* YaIPS_RGB_ShapeGenSub
* Generate an image with a shape. Is used only in this souce file.
*
* xxArg, yyArg  In: Size of image
* pShapeGen     In: shape generation parameter
*
* return  != NULL  OK
*         NULL     Error
****************************************************************************
*/

YaIPS_RGB_ShapeGen_List_t ShapeGen_List[] = {

    { (char *)"&ShapeType_NONE=---",                                       YAIPS_SHAPE_GEN_TYPE_NONE },          // No shape
    // Shapes in main menu
    { (char *)"&ShapeType_MA_RECTANGLE=Rectangle",                            YAIPS_SHAPE_GEN_TYPE_RECTANGLE },  // Rectangle
    { (char *)"&ShapeType_MA_ELLIPSE=Ellipse",                             YAIPS_SHAPE_GEN_TYPE_ELLIPSE },       // Ellipse
    // Basic shape submenu
    { (char *)"&ShapeType_BS_RECTANGLE=Basic shapes/Rectangle",            YAIPS_SHAPE_GEN_TYPE_RECTANGLE },     // Rectangle
    { (char *)"&ShapeType_BS_SQUARE=Basic shapes/Square",                  YAIPS_SHAPE_GEN_TYPE_SQUARE },        // Square
    { (char *)"&ShapeType_BS_ELLIPSE=Basic shapes/Ellipse",                YAIPS_SHAPE_GEN_TYPE_ELLIPSE },       // Ellipse
    { (char *)"&ShapeType_BS_CIRCLE=Basic shapes/Circle",                  YAIPS_SHAPE_GEN_TYPE_CIRCLE },        // Circle
    { (char *)"&ShapeType_BS_TRIANGLE=Basic shapes/Triangle",              YAIPS_SHAPE_GEN_TYPE_TRIANGLE,       0,  0, -100, 100 },  // Triangle
    { (char *)"&ShapeType_BS_RECT_ROUNDED=Basic shapes/Rounded rectangle", YAIPS_SHAPE_GEN_TYPE_RECT_ROUNDED,   0, 10, -100, 100 },  // Rectangle with rounded corner, argument is corner radius
    { (char *)"&ShapeType_BS_SQUAE_ROUNDED=Basic shapes/Rounded square",   YAIPS_SHAPE_GEN_TYPE_SQUAE_ROUNDED,  0, 10, -100, 100 },  // Square with rounded corner, argument is corner radius
    { (char *)"&ShapeType_BS_PARALLEOGRAM=Basic shapes/Parallelogram",     YAIPS_SHAPE_GEN_TYPE_PARALLEOGRAM,   0, 10, -100, 100 },  // Parallelogram, argument is slant
    { (char *)"&ShapeType_BS_TRAPEZ=Basic shapes/Trapez",                  YAIPS_SHAPE_GEN_TYPE_TRAPEZ,         0, 25, -100, 100 },  // Trapez, argument is slant
    { (char *)"&ShapeType_BS_DIAMOND=Basic shapes/Diamond",                YAIPS_SHAPE_GEN_TYPE_DIAMOND },                           // Diamond
    { (char *)"&ShapeType_BS_HEART=Basic shapes/Heart",                    YAIPS_SHAPE_GEN_TYPE_HEART,          0,  0,    0, 100 },  // Heart
    // Arrow submenu
    { (char *)"&ShapeType_ARROW_LEFT=Arrow/Arrow left",                    YAIPS_SHAPE_GEN_TYPE_ARROW_LEFT,     0, 50,    0, 100 },  // Arrow pointing left
    { (char *)"&ShapeType_ARROW_RIGHT=Arrow/Arrow right",                  YAIPS_SHAPE_GEN_TYPE_ARROW_RIGHT,    0, 50,    0, 100 },  // Arrow pointing right
    { (char *)"&ShapeType_ARROW_UP=Arrow/Arrow up",                        YAIPS_SHAPE_GEN_TYPE_ARROW_UP,       0, 50,    0, 100 },  // Arrow pointing up
    { (char *)"&ShapeType_ARROW_DOWN=Arrow/Arrow down",                    YAIPS_SHAPE_GEN_TYPE_ARROW_DOWN,     0, 50,    0, 100 },  // Arrow pointing down
    // Star submenu
    { (char *)"&ShapeType_STAR_4=Star/Star 4-pointed",                     YAIPS_SHAPE_GEN_TYPE_STAR_4,         0, 50,   10, 100 },  // Star with 4 points, argument is sharpness
    { (char *)"&ShapeType_STAR_5=Star/Star 5-pointed",                     YAIPS_SHAPE_GEN_TYPE_STAR_5,         0, 50,   10, 100 },  // Star with 5 points, argument is sharpness
    { (char *)"&ShapeType_STAR_6=Star/Star 6-pointed",                     YAIPS_SHAPE_GEN_TYPE_STAR_6,         0, 50,   10, 100 },  // Star with 6 points, argument is sharpness
    { (char *)"&ShapeType_STAR_8=Star/Star 8-pointed",                     YAIPS_SHAPE_GEN_TYPE_STAR_8,         0, 50,   10, 100 },  // Star with 8 points, argument is sharpness
    { (char *)"&ShapeType_STAR_12=Star/Star 12-pointed",                   YAIPS_SHAPE_GEN_TYPE_STAR_12,        0, 50,   10, 100 },  // Star with 12 points, argument is sharpness
    { (char *)"&ShapeType_STAR_24=Star/Star 24-pointed",                   YAIPS_SHAPE_GEN_TYPE_STAR_24,        0, 50,   10, 100 }   // Star with 24 points, argument is sharpness
};

int nShapeGen_List = sizeof( ShapeGen_List) / sizeof( YaIPS_RGB_ShapeGen_List_t);

/***************************************************************************
* YaIPS_RGB_ShapeGenSub
* Generate an image with a shape. Is used only in this souce file.
*
* xxArg, yyArg  In: Size of image
* pShapeGen     In: shape generation parameter
*
* return  != NULL  OK
*         NULL     Error
****************************************************************************
*/

#ifdef _DEBUG
static int LineStyleFlag = 0; //FL_DASH;   // TEST, line overdraw of shape border
#else
static int LineStyleFlag = 0;
#endif

static void draw_heart( YaIPS_RGB_ShapeGen_Par_t *pShapeGen,
                       int x, int y, int xx, int yy,
                       int DrawShape,                       // Flag: draw the shape
                       int DrawLine)                        // Flag: draw the line
{
  int i, nPoints;
  double AngleThis, xx2, yy2, xCenter, yCenter, xThis, yThis;
  float ShapeArgFloat;

  nPoints = (xx + yy + 1) / 2;

  xx2 = xx * 0.5;
  yy2 = yy * 0.5;

  xCenter = x + xx2;
  yCenter = y + yy2;

  ShapeArgFloat = pShapeGen->ShapeArg / 100.0;

  if( DrawShape) {           // Draw the shape

    fl_begin_polygon();

    for( i = 0; i < nPoints; i++) {

      AngleThis = 2.0 * M_PI * (double)i / (double)(nPoints - 1);

      xThis = pow( sin( AngleThis), 3.0);
      yThis = cos( AngleThis)
              - 5.0 / 13.0 * cos( 2.0 * AngleThis)
              - 2.0 / 13.0 * cos( 3.0 * AngleThis)
              - (1.0 - ShapeArgFloat) / 13.0 * cos( 4.0 * AngleThis);

      xThis = xCenter + xThis * xx2;
      yThis = yCenter - yThis * yy2 * 0.895 - yy2 * 0.175;

      fl_vertex( xThis, yThis);
    }

    fl_end_polygon();
  }

  if( DrawLine) {            // Draw the lines

    fl_line_style( LineStyleFlag, pShapeGen->LineWidth);

    fl_begin_loop();

    for( i = 0; i < nPoints; i++) {

      AngleThis = 2.0 * M_PI * (double)i / (double)(nPoints - 1);

      xThis = pow( sin( AngleThis), 3.0);
      yThis = cos( AngleThis)
              - 5.0 / 13.0 * cos( 2.0 * AngleThis)
              - 2.0 / 13.0 * cos( 3.0 * AngleThis)
              - (1.0 - ShapeArgFloat) / 13.0 * cos( 4.0 * AngleThis);

      xThis = xCenter + xThis * xx2;
      yThis = yCenter - yThis * yy2 * 0.895 - yy2 * 0.175;

      fl_vertex( xThis, yThis);
    }

    fl_end_loop();
  }
}

static void draw_star( YaIPS_RGB_ShapeGen_Par_t *pShapeGen,
                       int x, int y, int xx, int yy,
                       int nPoints,                         // Number of star points
                       int DrawShape,                       // Flag: draw the shape
                       int DrawLine)                        // Flag: draw the line
{
  int i;
  double AngleThis, xCenter, yCenter, xThis, yThis;
  float ShapeArgFloat;

  xCenter = x + xx * 0.5;
  yCenter = y + yy * 0.5;

  ShapeArgFloat = pShapeGen->ShapeArg / 100.0;

  if( DrawShape) {           // Draw the shape

    fl_begin_polygon();

    for( i = 0; i <= nPoints * 2; i++) {

      AngleThis = 2.0 * M_PI * (double)i / (double)(nPoints * 2);

      if( i & 1) {

        xThis = xCenter + sin( AngleThis) * xx * 0.5 * ShapeArgFloat;
        yThis = yCenter + cos( AngleThis) * yy * 0.5 * ShapeArgFloat;

      } else {

        xThis = xCenter + sin( AngleThis) * xx * 0.5;
        yThis = yCenter + cos( AngleThis) * yy * 0.5;
      }

      fl_vertex( xThis, yThis);
    }

    fl_end_polygon();
  }

  if( DrawLine) {            // Draw the lines

    fl_line_style( LineStyleFlag, pShapeGen->LineWidth);

    fl_begin_loop();

    for( i = 0; i <= nPoints * 2; i++) {

      AngleThis = 2.0 * M_PI * (double)i / (double)(nPoints * 2);

      if( i & 1) {

        xThis = xCenter + sin( AngleThis) * xx * 0.5 * ShapeArgFloat;
        yThis = yCenter + cos( AngleThis) * yy * 0.5 * ShapeArgFloat;

      } else {

        xThis = xCenter + sin( AngleThis) * xx * 0.5;
        yThis = yCenter + cos( AngleThis) * yy * 0.5;
      }

      fl_vertex( xThis, yThis);
    }

    fl_end_loop();
  }
}

static void draw_arrow( YaIPS_RGB_ShapeGen_Par_t *pShapeGen,
                       int x, int y, int xx, int yy,
                       int ShapeType,                       // Shape type
                       int DrawShape,                       // Flag: draw the shape
                       int DrawLine)                        // Flag: draw the line
{
  int i;
  double xLeft, xRight, yTop, yBottom, xx2, yy2;
  double xCenter, yCenter, dSpike, dBar;
  double ShapeArgFloat;
  double Px[ 8], Py[ 8];

  xLeft   = x + 0.5;
  xRight  = x + xx - 1 + 0.5;
  yTop    = y + 0.5;
  yBottom = y + yy - 1 + 0.5;

  xx2 = (xRight - xLeft);
  yy2 = (yBottom - yTop);

  xCenter = (xLeft + xRight) * 0.5;
  yCenter = (yTop + yBottom) * 0.5;

  ShapeArgFloat = pShapeGen->ShapeArg / 100.0;

  switch( ShapeType) {

  case YAIPS_SHAPE_GEN_TYPE_ARROW_LEFT:   // Arrow pointing left
  default:

    dSpike = yy2;
    if( dSpike > xx2 * 0.75) dSpike = xx2 * 0.75;

    dBar = yy2 * 0.5 * ShapeArgFloat;

    Px[ 0] = xLeft;          Py[ 0] = yCenter;
    Px[ 1] = xLeft + dSpike; Py[ 1] = yTop;
    Px[ 2] = xLeft + dSpike; Py[ 2] = yCenter - dBar;
    Px[ 3] = xRight;         Py[ 3] = yCenter - dBar;
    Px[ 4] = xRight;         Py[ 4] = yCenter + dBar;
    Px[ 5] = xLeft + dSpike; Py[ 5] = yCenter + dBar;
    Px[ 6] = xLeft + dSpike; Py[ 6] = yBottom;
    Px[ 7] = xLeft;          Py[ 7] = yCenter;
    break;

  case YAIPS_SHAPE_GEN_TYPE_ARROW_RIGHT:  // Arrow pointing right

    dSpike = yy2;
    if( dSpike > xx2 * 0.75) dSpike = xx2 * 0.75;

    dBar = yy2 * 0.5 * ShapeArgFloat;

    Px[ 0] = xRight;          Py[ 0] = yCenter;
    Px[ 1] = xRight - dSpike; Py[ 1] = yTop;
    Px[ 2] = xRight - dSpike; Py[ 2] = yCenter - dBar;
    Px[ 3] = xLeft;           Py[ 3] = yCenter - dBar;
    Px[ 4] = xLeft;           Py[ 4] = yCenter + dBar;
    Px[ 5] = xRight - dSpike; Py[ 5] = yCenter + dBar;
    Px[ 6] = xRight - dSpike; Py[ 6] = yBottom;
    Px[ 7] = xRight;          Py[ 7] = yCenter;
    break;

  case YAIPS_SHAPE_GEN_TYPE_ARROW_UP:     // Arrow pointing up

    dSpike = xx2;
    if( dSpike > yy2 * 0.75) dSpike = yy2 * 0.75;

    dBar = xx2 * 0.5 * ShapeArgFloat;

    Px[ 0] = xCenter;         Py[ 0] = yTop;
    Px[ 1] = xLeft;           Py[ 1] = yTop + dSpike;
    Px[ 2] = xCenter - dBar;  Py[ 2] = yTop + dSpike;
    Px[ 3] = xCenter - dBar;  Py[ 3] = yBottom;
    Px[ 4] = xCenter + dBar;  Py[ 4] = yBottom;
    Px[ 5] = xCenter + dBar;  Py[ 5] = yTop + dSpike;
    Px[ 6] = xRight;          Py[ 6] = yTop + dSpike;
    Px[ 7] = xCenter;         Py[ 7] = yTop;
    break;

  case YAIPS_SHAPE_GEN_TYPE_ARROW_DOWN:   // Arrow pointing down

    dSpike = xx2;
    if( dSpike > yy2 * 0.75) dSpike = yy2 * 0.75;

    dBar = xx2 * 0.5 * ShapeArgFloat;

    Px[ 0] = xCenter;         Py[ 0] = yBottom;
    Px[ 1] = xLeft;           Py[ 1] = yBottom - dSpike;
    Px[ 2] = xCenter - dBar;  Py[ 2] = yBottom - dSpike;
    Px[ 3] = xCenter - dBar;  Py[ 3] = yTop;
    Px[ 4] = xCenter + dBar;  Py[ 4] = yTop;
    Px[ 5] = xCenter + dBar;  Py[ 5] = yBottom - dSpike;
    Px[ 6] = xRight;          Py[ 6] = yBottom - dSpike;
    Px[ 7] = xCenter;         Py[ 7] = yBottom;
    break;
  }


  if( DrawShape) {           // Draw the shape

    fl_begin_polygon();

    for( i = 0; i < 8; i++) {

      fl_vertex( Px[ i], Py[ i]);
    }

    fl_end_polygon();
  }

  if( DrawLine) {            // Draw the lines

    fl_line_style( LineStyleFlag, pShapeGen->LineWidth);

    fl_begin_loop();

    for( i = 0; i < 8; i++) {

      fl_vertex( Px[ i], Py[ i]);
    }

    fl_end_loop();
  }
}

static Fl_RGB_Image *pShapeGenSub( int xxArg, int yyArg,                 // In: Size of image
                                   YaIPS_RGB_ShapeGen_Par_t *pShapeGen,  // In: shape generation parameter
                                   int DrawShape,                        // Flag: draw the shape
                                   int DrawLine)                         // Flag: draw the line
{
  int x, y, xx, yy, x1, y1, x2, y2, x3, y3, x4, y4, ShapeArgInt, LineIndent;
  float ShapeArgFloat;
  Fl_Image_Surface *pSurface;
  Fl_RGB_Image *pImgReturn;

  pSurface = new Fl_Image_Surface( xxArg, yyArg);    // Create surface

  if( pSurface == NULL) {                      // Security test

    return( NULL);
  }

  Fl_Surface_Device::push_current( pSurface);

  // Color Background

  x = 0;
  y = 0;
  xx = xxArg;
  yy = yyArg;

  fl_line_style( 0);                         // Reset to default

  fl_color( FL_BLACK);                       // Black background
  fl_rectf( x, y, xx, yy);

  // Calculate line indent
  // Must be calculated also for shapes, so indent independent from argument.

  LineIndent = 0;

  if( pShapeGen->ShapeType != YAIPS_SHAPE_GEN_TYPE_NONE &&              // We have a shape selected
      (pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_DRAW_LINE) != 0) {  // Do we generate lines

    LineIndent = (pShapeGen->LineWidth + 1) / 2;

    if( LineIndent < 1) {         // Clip minimum value

      LineIndent = 1;
    }

    x += LineIndent;
    y += LineIndent;

    xx -= LineIndent * 2;
    yy -= LineIndent * 2;
  }

  // Generate shape

  fl_color( FL_WHITE);             // White shapes

  switch( pShapeGen->ShapeType) {

  default:

  case YAIPS_SHAPE_GEN_TYPE_NONE:

    // Nothing to do here. The image is black.

    break;


  case YAIPS_SHAPE_GEN_TYPE_SQUARE:          // Square

    if( xx > yy) {

      x += (xx - yy) / 2;

      xx = yy;

    } else {

      y += (yy - xx) / 2;

      yy = xx;
    }

    /* no break */

  case YAIPS_SHAPE_GEN_TYPE_RECTANGLE:       // Rectangle

    if( DrawShape) {           // Draw the shape

      fl_rectf( x, y, xx, yy);
    }

    if( DrawLine) {            // Draw the lines

      fl_line_style( LineStyleFlag, pShapeGen->LineWidth);

      fl_rect( x, y, xx, yy);
    }

    break;

  case YAIPS_SHAPE_GEN_TYPE_CIRCLE:          // Circle

    if( xx > yy) {

      x += (xx - yy) / 2;

      xx = yy;

    } else {

      y += (yy - xx) / 2;

      yy = xx;
    }

    /* no break */

  case YAIPS_SHAPE_GEN_TYPE_ELLIPSE:         // Ellipse

    if( DrawShape) {           // Draw the shape

      fl_pie( x, y, xx, yy, 0.0, 360.0);
    }

    if( DrawLine) {            // Draw the lines

      fl_line_style( LineStyleFlag, pShapeGen->LineWidth);

#ifdef use_again
      fl_arc( x, y, xx, yy, 0.0, 360.0);
#else // HACK: fl_loop() make problems with thick lines.
      double xRad, yRad, xCenter, yCenter, DeltaPerimeter;
      double xThis, yThis, AngleThis;
      int iRad, nRad;

      xRad = xx * 0.5;
      yRad = yy * 0.5;

      xCenter = x + xRad;
      yCenter = y + yRad;

      if( xx > yy) {
        DeltaPerimeter = xx * 0.02;
        nRad = (int)(xx * M_PI / DeltaPerimeter + 0.5);
      } else {
        DeltaPerimeter = yy * 0.02;
        nRad = (int)(yy * M_PI / DeltaPerimeter + 0.5);
      }

      fl_begin_loop();

      for( iRad = 0; iRad < nRad; iRad++) {

        AngleThis = 2.0 * M_PI * (double)iRad / (double)nRad;

        xThis = xCenter + sin( AngleThis) * xRad;
        yThis = yCenter + cos( AngleThis) * yRad;
        fl_vertex( xThis, yThis);
      }

      fl_end_loop();
#endif
    }

    break;

  case YAIPS_SHAPE_GEN_TYPE_TRIANGLE:       // Triangle

    x1 = x + xx / 2;  // Top center point
    y1 = y;

    x3 = x;           // Bottom left point
    y3 = y + yy - 1;

    x4 = x + xx - 1;      // Bottom right point
    y4 = y + yy - 1;

    ShapeArgFloat = pShapeGen->ShapeArg * xx / 200.0;   // Range - 100 .. 100 %

    if( ShapeArgFloat >= 0.0) {      // Round to nearest integer
      ShapeArgInt = (int)(ShapeArgFloat + 0.5);
    } else {
      ShapeArgInt = (int)(ShapeArgFloat - 0.5);
    }

    if( ShapeArgInt < - (xx / 2)) {  // Clip to minimum

      ShapeArgInt = - (xx / 2);
    }

    if( ShapeArgInt > xx / 2) {      // Clip to maximum

      ShapeArgInt = xx / 2;
    }

    x1 += ShapeArgInt;

    if( x1 < x3) {

      x1 = x3;
    }

    if( x1 > x4) {

      x1 = x4;
    }

    if( DrawShape) {           // Draw the shape

      fl_polygon( x1, y1, x4, y4, x3, y3);
    }

    if( DrawLine) {            // Draw the lines

      fl_line_style( LineStyleFlag, pShapeGen->LineWidth);

#ifdef use_again
      fl_loop( x1, y1, x4, y4, x3, y3);
#else // HACK: fl_loop() make problems with thick lines.
      fl_begin_loop();
      fl_vertex( x1, y1);
      fl_vertex( x4, y4);
      fl_vertex( x3, y3);
      fl_end_loop();
#endif
    }

    break;

  case YAIPS_SHAPE_GEN_TYPE_SQUAE_ROUNDED:   // Square with rounded corner, argument is corner radius

    if( xx > yy) {

      x += (xx - yy) / 2;

      xx = yy;

    } else {

      y += (yy - xx) / 2;

      yy = xx;
    }

    /* no break */

  case YAIPS_SHAPE_GEN_TYPE_RECT_ROUNDED:    // Rectangle with rounded corner, argument is corner radius

    if( xx <= yy) {
      ShapeArgFloat = pShapeGen->ShapeArg * xx / 200.0;   // Range - 100 .. 100 %
    } else {
      ShapeArgFloat = pShapeGen->ShapeArg * yy / 200.0;   // Range - 100 .. 100 %
    }

    if( ShapeArgFloat >= 0.0) {      // Round to nearest integer
      ShapeArgInt = (int)(ShapeArgFloat + 0.5);
    } else {
      ShapeArgInt = (int)(ShapeArgFloat - 0.5);
    }

    int InvertCorner;

    InvertCorner = false;

    if( ShapeArgInt < 0) {           // Corners inside

      ShapeArgInt = - ShapeArgInt;   // Make absolute value
      InvertCorner = true;
    }

    if( ShapeArgInt > xx / 2) {      // Clip to maximum

      ShapeArgInt = xx / 2;
    }

    if( ShapeArgInt > yy / 2) {      // Clip to maximum

      ShapeArgInt = yy / 2;
    }

    if( ShapeArgInt <= 1) {          // no corner radius

      // Draw a rectangle

      if( DrawShape) {               // Draw the shape

        fl_rectf( x, y, xx, yy);
      }

      if( DrawLine) {            // Draw the lines

        fl_line_style( LineStyleFlag, pShapeGen->LineWidth);

        fl_rect( x, y, xx, yy);
      }

    } else {

      // HACK: fl_rounded_rectf() and fl_rounded_rect() make problems with greater corner radius.

      double Rad, xCenter, yCenter, DeltaPerimeter;
      double xThis, yThis, AngleThis;
      int iRad, nRad;

      Rad = ShapeArgInt;
      DeltaPerimeter = Rad * 0.02;
      nRad = (int)(xx * M_PI / DeltaPerimeter + 0.5);
      nRad = ( nRad + 3) / 4;       // 1/4

      x1 = x;             // Left side
      x2 = x + xx - 1;    // Right side
      y1 = y;             // Top side
      y2 = y + yy - 1;    // Bottom side

      if( DrawShape) {           // Draw the shape

        fl_begin_polygon();

        fl_vertex( x1 + Rad, y1);    // Upper straight side
        fl_vertex( x2 - Rad, y1);

        if( InvertCorner) {
          xCenter = x2;          // Right upper corner
          yCenter = y1;
          for( iRad = nRad - 1; iRad >= 1; iRad--) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter - sin( AngleThis) * Rad;
            yThis = yCenter + cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }
        } else {
          xCenter = x2 - Rad;          // Right upper corner
          yCenter = y1 + Rad;
          for( iRad = 1; iRad < nRad; iRad++) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter + sin( AngleThis) * Rad;
            yThis = yCenter - cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }
        }

        fl_vertex( x2, y1 + Rad);    // Right straight side
        fl_vertex( x2, y2 - Rad);

        if( InvertCorner) {
          xCenter = x2;          // Right bottom corner
          yCenter = y2;
          for( iRad = 1; iRad < nRad; iRad++) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter - sin( AngleThis) * Rad;
            yThis = yCenter - cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }
        } else {
          xCenter = x2 - Rad;          // Right bottom corner
          yCenter = y2 - Rad;
          for( iRad = nRad - 1; iRad >= 1; iRad--) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter + sin( AngleThis) * Rad;
            yThis = yCenter + cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }
        }

        fl_vertex( x2 - Rad, y2);    // Bottom straight side
        fl_vertex( x1 + Rad, y2);

        if( InvertCorner) {
          xCenter = x1;          // Left bottom corner
          yCenter = y2;
          for( iRad = nRad - 1; iRad >= 1; iRad--) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter + sin( AngleThis) * Rad;
            yThis = yCenter - cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }

          fl_vertex( x1, y2 - Rad);    // Left straight side
          fl_vertex( x1, y1 + Rad);
        } else {
          xCenter = x1 + Rad;          // Left bottom corner
          yCenter = y2 - Rad;
          for( iRad = 1; iRad < nRad; iRad++) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter - sin( AngleThis) * Rad;
            yThis = yCenter + cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }

          fl_vertex( x1, y2 - Rad);    // Left straight side
          fl_vertex( x1, y1 + Rad);
        }

        if( InvertCorner) {
          xCenter = x1;          // Left upper corner
          yCenter = y1;
          for( iRad = 1; iRad < nRad; iRad++) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter + sin( AngleThis) * Rad;
            yThis = yCenter + cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }
        } else {
          xCenter = x1 + Rad;          // Left upper corner
          yCenter = y1 + Rad;
          for( iRad = nRad - 1; iRad >= 1; iRad--) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter - sin( AngleThis) * Rad;
            yThis = yCenter - cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }
        }

        fl_end_polygon();
      }

      if( DrawLine) {            // Draw the lines

        fl_line_style( LineStyleFlag, pShapeGen->LineWidth);

        fl_begin_loop();

        fl_vertex( x1 + Rad, y1);    // Upper straight side
        fl_vertex( x2 - Rad, y1);

        if( InvertCorner) {
          xCenter = x2;          // Right upper corner
          yCenter = y1;
          for( iRad = nRad - 1; iRad >= 1; iRad--) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter - sin( AngleThis) * Rad;
            yThis = yCenter + cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }
        } else {
          xCenter = x2 - Rad;          // Right upper corner
          yCenter = y1 + Rad;
          for( iRad = 1; iRad < nRad; iRad++) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter + sin( AngleThis) * Rad;
            yThis = yCenter - cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }
        }

        fl_vertex( x2, y1 + Rad);    // Right straight side
        fl_vertex( x2, y2 - Rad);

        if( InvertCorner) {
          xCenter = x2;          // Right bottom corner
          yCenter = y2;
          for( iRad = 1; iRad < nRad; iRad++) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter - sin( AngleThis) * Rad;
            yThis = yCenter - cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }
        } else {
          xCenter = x2 - Rad;          // Right bottom corner
          yCenter = y2 - Rad;
          for( iRad = nRad - 1; iRad >= 1; iRad--) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter + sin( AngleThis) * Rad;
            yThis = yCenter + cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }
        }

        fl_vertex( x2 - Rad, y2);    // Bottom straight side
        fl_vertex( x1 + Rad, y2);

        if( InvertCorner) {
          xCenter = x1;          // Left bottom corner
          yCenter = y2;
          for( iRad = nRad - 1; iRad >= 1; iRad--) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter + sin( AngleThis) * Rad;
            yThis = yCenter - cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }

          fl_vertex( x1, y2 - Rad);    // Left straight side
          fl_vertex( x1, y1 + Rad);
        } else {
          xCenter = x1 + Rad;          // Left bottom corner
          yCenter = y2 - Rad;
          for( iRad = 1; iRad < nRad; iRad++) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter - sin( AngleThis) * Rad;
            yThis = yCenter + cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }

          fl_vertex( x1, y2 - Rad);    // Left straight side
          fl_vertex( x1, y1 + Rad);
        }

        if( InvertCorner) {
          xCenter = x1;          // Left upper corner
          yCenter = y1;
          for( iRad = 1; iRad < nRad; iRad++) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter + sin( AngleThis) * Rad;
            yThis = yCenter + cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }
        } else {
          xCenter = x1 + Rad;          // Left upper corner
          yCenter = y1 + Rad;
          for( iRad = nRad - 1; iRad >= 1; iRad--) {

            AngleThis = 0.5 * M_PI * (double)iRad / (double)nRad;

            xThis = xCenter - sin( AngleThis) * Rad;
            yThis = yCenter - cos( AngleThis) * Rad;
            fl_vertex( xThis, yThis);
          }
        }

        fl_end_loop();
      }
    }

    break;

  case YAIPS_SHAPE_GEN_TYPE_PARALLEOGRAM:    // Parallelogram, argument is slant

    x1 = x + 1;         // Top left point
    y1 = y + 1;

    x2 = x + xx + 1;    // Top right point
    y2 = y + 1;

    x3 = x + 1;         // Bottom left point
    y3 = y + yy + 1;

    x4 = x + xx + 1;    // Bottom right point
    y4 = y + yy + 1;

    ShapeArgFloat = pShapeGen->ShapeArg * 0.95 * xx / 100.0;   // Range - 100 .. 100 %

    if( ShapeArgFloat >= 0.0) {      // Round to nearest integer
      ShapeArgInt = (int)(ShapeArgFloat + 0.5);
    } else {
      ShapeArgInt = (int)(ShapeArgFloat - 0.5);
    }

    if( ShapeArgInt < - (xx - 1)) {  // Clip to minimum

      ShapeArgInt = - (xx - 1);
    }

    if( ShapeArgInt > xx - 1) {      // Clip to maximum

      ShapeArgInt = xx - 1;
    }

    if( ShapeArgInt >= 0) {

      x1 += ShapeArgInt;
      x4 -= ShapeArgInt;

    } else {

      x2 += ShapeArgInt;
      x3 -= ShapeArgInt;
    }

    if( DrawShape) {           // Draw the shape

      fl_polygon( x1, y1, x2, y2, x4, y4, x3, y3);
    }

    if( DrawLine) {            // Draw the lines

      fl_line_style( LineStyleFlag, pShapeGen->LineWidth);

#ifdef use_again
      fl_loop( x1, y1, x2, y2, x4, y4, x3, y3);
#else // HACK: fl_loop() make problems with thick lines.
      fl_begin_loop();
      fl_vertex( x1, y1);
      fl_vertex( x2, y2);
      fl_vertex( x4, y4);
      fl_vertex( x3, y3);
      fl_end_loop();
#endif
    }

    break;

  case YAIPS_SHAPE_GEN_TYPE_TRAPEZ:          // Trapez, argument is slant

    x1 = x + 1;         // Top left point
    y1 = y + 1;

    x2 = x + xx + 1;    // Top right point
    y2 = y + 1;

    x3 = x + 1;         // Bottom left point
    y3 = y + yy + 1;

    x4 = x + xx + 1;    // Bottom right point
    y4 = y + yy + 1;

    ShapeArgFloat = pShapeGen->ShapeArg * xx / 200.0;   // Range - 100 .. 100 %

    if( ShapeArgFloat >= 0.0) {      // Round to nearest integer
      ShapeArgInt = (int)(ShapeArgFloat + 0.5);
    } else {
      ShapeArgInt = (int)(ShapeArgFloat - 0.5);
    }

    if( ShapeArgInt < - (xx / 2)) {  // Clip to minimum

      ShapeArgInt = - (xx / 2);
    }

    if( ShapeArgInt > xx / 2) {      // Clip to maximum

      ShapeArgInt = xx / 2;
    }

    if( ShapeArgInt >= 0) {

      x1 += ShapeArgInt;
      x2 -= ShapeArgInt;

    } else {

      x3 -= ShapeArgInt;
      x4 += ShapeArgInt;
    }

    if( DrawShape) {           // Draw the shape

      fl_polygon( x1, y1, x2, y2, x4, y4, x3, y3);
    }

    if( DrawLine) {            // Draw the lines

      fl_line_style( LineStyleFlag, pShapeGen->LineWidth);

#ifdef use_again
      fl_loop( x1, y1, x2, y2, x4, y4, x3, y3);
#else // HACK: fl_loop() make problems with thick lines.
      fl_begin_loop();
      fl_vertex( x1, y1);
      fl_vertex( x2, y2);
      fl_vertex( x4, y4);
      fl_vertex( x3, y3);
      fl_end_loop();
#endif
    }

    break;

   case YAIPS_SHAPE_GEN_TYPE_DIAMOND:        // Diamond

     x1 = x + xx / 2 + 1;    // Top left point
     y1 = y + 1;

     x2 = x + xx + 1;        // Top right point
     y2 = y + yy / 2 + 1;

     x3 = x1;                // Bottom left point
     y3 = y + yy + 1;

     x4 = x + 1;             // Bottom right point
     y4 = y2;

     if( DrawShape) {           // Draw the shape

       fl_polygon( x1, y1, x2, y2, x3, y3, x4, y4);
     }

     if( DrawLine) {            // Draw the lines

       fl_line_style( LineStyleFlag, pShapeGen->LineWidth);

#ifdef use_again
       fl_loop( x1, y1, x2, y2, x3, y3, x4, y4);
#else // HACK: fl_loop() make problems with thick lines.
      fl_begin_loop();
      fl_vertex( x1, y1);
      fl_vertex( x2, y2);
      fl_vertex( x3, y3);
      fl_vertex( x4, y4);
      fl_end_loop();
#endif
     }

     break;

   case YAIPS_SHAPE_GEN_TYPE_HEART:        // Heart

     draw_heart( pShapeGen, x, y, xx, yy, DrawShape, DrawLine);

     break;

   case YAIPS_SHAPE_GEN_TYPE_STAR_4:         // Star with 4 points, argument is sharpness

     draw_star( pShapeGen, x, y, xx, yy, 4, DrawShape, DrawLine);

     break;

   case YAIPS_SHAPE_GEN_TYPE_STAR_5:         // Star with 5 points, argument is sharpness

     draw_star( pShapeGen, x, y, xx, yy, 5, DrawShape, DrawLine);

     break;

   case YAIPS_SHAPE_GEN_TYPE_STAR_6:         // Star with 6 points, argument is sharpness

     draw_star( pShapeGen, x, y, xx, yy, 6, DrawShape, DrawLine);

     break;

   case YAIPS_SHAPE_GEN_TYPE_STAR_8:         // Star with 8 points, argument is sharpness

     draw_star( pShapeGen, x, y, xx, yy, 8, DrawShape, DrawLine);

     break;

   case YAIPS_SHAPE_GEN_TYPE_STAR_12:        // Star with 12 points, argument is sharpness

     draw_star( pShapeGen, x, y, xx, yy, 12, DrawShape, DrawLine);

     break;

   case YAIPS_SHAPE_GEN_TYPE_STAR_24:        // Star with 24 points, argument is sharpness

     draw_star( pShapeGen, x, y, xx, yy, 24, DrawShape, DrawLine);

     break;

   case YAIPS_SHAPE_GEN_TYPE_ARROW_LEFT:   // Arrow pointing left
   case YAIPS_SHAPE_GEN_TYPE_ARROW_RIGHT:  // Arrow pointing right
   case YAIPS_SHAPE_GEN_TYPE_ARROW_UP:     // Arrow pointing up
   case YAIPS_SHAPE_GEN_TYPE_ARROW_DOWN:   // Arrow pointing down

     draw_arrow( pShapeGen, x, y, xx, yy, pShapeGen->ShapeType, DrawShape, DrawLine);

     break;
  }

  // Finish up

  pImgReturn = pSurface->image();
  delete pSurface;

  Fl_Surface_Device::pop_current();

  fl_line_style( 0);                         // Reset to default

  return( pImgReturn);                       // Return OK
}

/***************************************************************************
* utf8_decode
* Return number of bytes for an UTF8 coded character.
*
****************************************************************************
*/

int utf8_decode( char *s, uint32_t *out_codepoint) {
  uint32_t cp;
  int len;

  unsigned char c = (unsigned char)s[0];

  if (c < 0x80) {                 // 1-byte ASCII
    if( out_codepoint != NULL) {
      *out_codepoint = c;
    }
    return 1;
  }
  else if ((c & 0xE0) == 0xC0) {  // 2-byte sequence
    cp = c & 0x1F;
    len = 2;
  }
  else if ((c & 0xF0) == 0xE0) {  // 3-byte sequence
    cp = c & 0x0F;
    len = 3;
  }
  else if ((c & 0xF8) == 0xF0) {  // 4-byte sequence
    cp = c & 0x07;
    len = 4;
  }
  else {
    return -1; // invalid first byte
  }

  // Validate continuation bytes
  for( int i = 1; i < len; i++) {
    if ((s[i] & 0xC0) != 0x80)
        return -1; // invalid continuation byte
    cp = (cp << 6) | (s[i] & 0x3F);
  }

  if( out_codepoint != NULL) {
    *out_codepoint = cp;
  }
  return len;
}

/***************************************************************************
* pShapeGenText
* Generate the text for a shape
*
****************************************************************************
*/

#define GEN_TEXT_MAX_LINES 512           // Max this number of lines supported

static Fl_RGB_Image *pShapeGenText( int xxArg, int yyArg,                 // In: Size of image
                                    YaIPS_RGB_ShapeGen_Par_t *pShapeGen,  // In: shape generation parameter
                                    Fl_Text_Buffer *pTextBuffer)          // Holds text to display inside the overlay
{
  int x, y, xx, yy;
  int TextBufferLenth, UnderlineAddY, UnderlineAddX, UnderlineLines;
  Fl_Image_Surface *pSurface;
  Fl_RGB_Image *pImgReturn;

  TextBufferLenth = pTextBuffer == 0 ? 0 : pTextBuffer->length();

  if( TextBufferLenth <= 0) {     // have no text to display

    return NULL;
  }

  pSurface = new Fl_Image_Surface( xxArg, yyArg);    // Create surface

  if( pSurface == NULL) {                            // Security test

    return( NULL);
  }

  Fl_Surface_Device::push_current( pSurface);

  // Color Background

  x = 0;
  y = 0;
  xx = xxArg;
  yy = yyArg;

  fl_line_style( 0);                         // Reset to default

  fl_color( FL_BLACK);                       // Black background
  fl_rectf( x, y, xx, yy);

  // Generate text

  fl_color( FL_WHITE);             // White shapes

  // Walk the lines

  YaIPS_FontBase_t *pFontBase;
  int FontHeight, FontFace, FontStyleBold, FontStyleItalic, FontStyleUnderl;

  pFontBase = pYaIPS_Utils_FontsLookup( pShapeGen->FontName);

  FontHeight = pShapeGen->FontSize;

  FontStyleBold   = (pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_FONT_BOLD_ON) != 0;
  FontStyleItalic = (pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_FONT_ITALIC_ON) != 0;
  FontStyleUnderl = (pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_FONT_UNDERL_ON) != 0;

  if( pFontBase != NULL) {             // Got a font

    if( FontStyleBold && FontStyleItalic && pFontBase->FontNr_bold_italic >= 0) {

      FontFace = pFontBase->FontNr_bold_italic;

    } else if( FontStyleItalic && pFontBase->FontNr_italic >= 0) {

      FontFace = pFontBase->FontNr_italic;

    } else if( FontStyleBold && pFontBase->FontNr_bold >= 0) {

      FontFace = pFontBase->FontNr_bold;

    } else {

      FontFace = pFontBase->FontNr_regular;
    }

  } else {                             // No font

    if( FontStyleBold && FontStyleItalic) {

      FontFace = FL_HELVETICA_BOLD_ITALIC;

    } else if( FontStyleItalic) {

      FontFace = FL_HELVETICA_ITALIC;

    } else if( FontStyleBold) {

      FontFace = FL_HELVETICA_BOLD;

    } else {

      FontFace = FL_HELVETICA;           // Fall back
    }
  }

  fl_font( FontFace, FontHeight);      // Until know it better, use this font

  UnderlineAddY   = (fl_descent() + 1) / 2;
  UnderlineAddX  = (pShapeGen->SpacingChar + 1) / 2;
  UnderlineLines = ((FontHeight - 8) / 15) + 1;

  char *pTextBegin, *pTextAfterEnd, *pLine;
  int nLines, LineNr, nBytesPerChar, Pass, TempInt;
  int LineLenghtTab[ GEN_TEXT_MAX_LINES];             // Line lenght table
  double CharWidthDouble;
  int CharWidthInt, FirstCharDone;

  pTextBegin = pTextBuffer->text_range( 0, TextBufferLenth);
  pTextAfterEnd = pTextBegin + TextBufferLenth;

  nLines = 0;

  for( Pass = 0; Pass < 2; Pass++) {           // Loop two times over text

    LineNr = 0;

    if( Pass == 0) {         // First pass

      LineLenghtTab[ LineNr] = 0;
    }

    pLine = pTextBegin;
    FirstCharDone = false;

    for( ; ; ) {

      if( pLine >= pTextAfterEnd) {            // End of last line

        break;
      }

      // Walk the characters in an UTF8 text

      x = 0;
      y = FontHeight;

      if( Pass != 0) {         // Second pass

        switch( pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_MASK) {
        default:
        case YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_LEFT:
          x = 0;                                        // Indent from left side
          x += pShapeGen->IndentHor;
          break;

        case YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_CENTER:
          x = (xxArg - LineLenghtTab[ LineNr]) / 2;     // Center text
          x += pShapeGen->IndentHor;
          break;

        case YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_RIGHT:
          x = xxArg - 1 - LineLenghtTab[ LineNr];       // Indent from right side
          x -= pShapeGen->IndentHor;
          break;
        }

        // Calculate height of text block
        TempInt = nLines * FontHeight;
        if( nLines > 2 && pShapeGen->SpacingLine > 0) {

          TempInt += (nLines - 1) * pShapeGen->SpacingLine;
        }

        switch( pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_MASK) {
        default:
        case YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_TOP:
          y = FontHeight;                // Indent from top side
          y += pShapeGen->IndentVer;
          break;

        case YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_CENTER:
          y = (yyArg - TempInt) / 2 + FontHeight;     // Center text
          y -= pShapeGen->IndentVer;
          break;

        case YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_BOTTOM:
          y = yyArg - TempInt + FontHeight;  // Indent from bottom side
          y -= pShapeGen->IndentVer;
          break;
        }
      }

      for( ; ; ) {

        if( pLine >= pTextAfterEnd) {            // End of last line

          break;
        }

        if( *pLine == '\n') {      // end of line

          pLine += 1;

          LineNr += 1;

          if( LineNr >= GEN_TEXT_MAX_LINES) {   // Test for line buffer overflow

            break;
          }

          x = 0;

          if( Pass == 0) {         // First pass

            nLines = LineNr;
            LineLenghtTab[ LineNr] = 0;

          } else {                 // Second pass

            switch( pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_MASK) {
            default:
            case YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_LEFT:
              x = 0;                                        // Indent from left side
              x += pShapeGen->IndentHor;
              break;

            case YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_CENTER:
              x = (xxArg - LineLenghtTab[ LineNr]) / 2;     // Center text
              x += pShapeGen->IndentHor;
              break;

            case YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_RIGHT:
              x = xxArg - 1 - LineLenghtTab[ LineNr];       // Indent from right side
              x -= pShapeGen->IndentHor;
              break;
            }

            y += FontHeight + pShapeGen->SpacingLine;
          }

          FirstCharDone = false;
          continue;
        }

        if( LineNr >= GEN_TEXT_MAX_LINES) {   // Test for line buffer overflow

          break;
        }

        nBytesPerChar = utf8_decode( pLine);
        if( nBytesPerChar <= 0) {

          //x/printf("Invalid UTF-8 sequence\n");
          break;
        }

        //x/printf("U+%04X (bytes: %d)\n", cp, n);

        if( FirstCharDone && pShapeGen->SpacingChar > 0) {  // After first char

          x += pShapeGen->SpacingChar;           // Add character spacing

          if( Pass == 0) {         // First pass

             LineLenghtTab[ LineNr] += pShapeGen->SpacingChar;
           }
        } else {

          FirstCharDone = true;
        }

        CharWidthDouble = fl_width( pLine, nBytesPerChar);

        CharWidthInt = (int)(CharWidthDouble + 0.75);

        if( Pass == 0) {         // First pass

          LineLenghtTab[ LineNr] += CharWidthInt;

          if( nLines < LineNr + 1) {

            nLines = LineNr + 1;
          }

        } else {         // Second pass

          fl_draw( pLine, nBytesPerChar, x, y);

          if( FontStyleUnderl) {

            int iLine, OffsetY;

            for( iLine = 0; iLine < UnderlineLines; iLine++) {

              if( iLine == 0) {

                OffsetY = 0;

              } else if( (iLine & 0x01) != 0) {

                OffsetY = (iLine + 1) / 2;

              } else {

                OffsetY = - (iLine  / 2);
              }

              fl_xyline( x - UnderlineAddX, y + UnderlineAddY + OffsetY, x + CharWidthInt + UnderlineAddX);
            }
          }
        }


        x += CharWidthInt;

        pLine += nBytesPerChar;
      }
    }
  }

  free( pTextBegin);

  // Finish up

  pImgReturn = pSurface->image();
  delete pSurface;

  Fl_Surface_Device::pop_current();

  fl_line_style( 0);                         // Reset to default

  return( pImgReturn);                       // Return OK
}

/***************************************************************************
* YaIPS_RGB_CopyBgndToOverlay
* Copy background image to overlay.
* NOTE: The destination image is always preset wit 0 or 255.
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

static int YaIPS_RGB_CopyBgndToOverlay( YaIPS_RGB_ImgD_t *piDst,  // In: output image
                                    int xx, int yy,               // In: Size of image
                                    Fl_RGB_Image *pSrcArg)        // In: Pointer to pointer to RGB color image
{
  YaIPS_RGB_ImgD_t iSrc;
  Fl_RGB_Image *pSrc;           // Source image
  int ierr, x, y;
  double ScaleFacX, ScaleFacY;
  int xx1, yy1, xSrc, ySrc, xRem, yRem, TempI;
  int xOff, AlphaVal;
  double xSrcD1, ySrcD1;
  uchar *d8, *p8s1, *p8s2;
#ifdef USE_OVERLAY_SCALE_HACK // Use a hack for better scale results.
  Fl_RGB_Image *pTmp = NULL;
#endif

  pSrc = pSrcArg;               // Preset pointer to source image

  // Convert loaded image

  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error

    goto ExitPoint;
  }

#ifdef USE_OVERLAY_SCALE_HACK // Use a hack for better scale results.
  int SizeShiftX, SizeShiftY, xx2, yy2;

  ScaleFacX = (double)xx / (double)iSrc.xx;
  ScaleFacY = (double)yy / (double)iSrc.yy;

  if( ScaleFacX <= 0.5 || ScaleFacY <= 0.5) {

    // Calculate shrink

    SizeShiftX = 0;
    SizeShiftY = 0;

    while( ScaleFacX <= 0.5 && (iSrc.xx >> (0 - SizeShiftX + 1)) >= 16) {

      ScaleFacX *= 2.0;
      SizeShiftX -= 1;
    }

    while( ScaleFacY <= 0.5 && (iSrc.yy >> (0 - SizeShiftY + 1)) >= 16) {

      ScaleFacY *= 2.0;
      SizeShiftY -= 1;
    }

    ierr = YaIPS_RGB_Geo_Resize2( &pTmp, pSrc, SizeShiftX, SizeShiftY);
    if( ierr != 0)  {                           // Check for error

      goto ExitPoint;
    }

    xx2 = iSrc.xx >> (0 - SizeShiftX);
    yy2 = iSrc.yy >> (0 - SizeShiftY);

    if( xx2 << (0 - SizeShiftX) != iSrc.xx) {                   // Error

      ScaleFacX = ScaleFacX * (float)iSrc.xx / (xx2 << (0 - SizeShiftX)); // Correct size
    }

    if( yy2 << (0 - SizeShiftY) != iSrc.yy) {                   // Error

      ScaleFacY = ScaleFacY * (float)iSrc.yy / (yy2 << (0 - SizeShiftY)); // Correct size
    }

    pSrc = pTmp;                              // Use temporary image as source

    // Convert shrinked image

    ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
    if( ierr != 0)  {                           // Check for error

      goto ExitPoint;
    }
  }
#endif

  ScaleFacX = (double)iSrc.xx / (double)xx;
  ScaleFacY = (double)iSrc.yy / (double)yy;

  xx1 = iSrc.xx - 1;         // need a border of 1 for bilinear interpolation
  yy1 = iSrc.yy - 1;

  for( y = 0; y < yy; y++) {

    d8 = RGB_pixad( 0,  y, piDst);

    ySrcD1 = y * ScaleFacY;

    // float coordinates to integer
    // + remainder (for bilinear interpolation), has 4 afterpoint bits

    ySrc = (int)(ySrcD1 * 16.0);        // with afterpoint digits
    TempI = ySrc & 0xfffffff0;          // next lowest without afterpoint
    yRem  = ySrc - TempI;               // remainder
    ySrc  = ySrc >> 4;                  // before point

    if( ySrc < 0 || ySrc > yy1) {       // out of source image

      continue;
    }

    for( x = 0; x < xx; x++) {

      xSrcD1 = x * ScaleFacX;

      // float coordinates to integer
      // + remainder (for bilinear interpolation), has 4 afterpoint bits

      xSrc = (int)(xSrcD1 * 16.0);        // with afterpoint digits
      TempI = xSrc & 0xfffffff0;          // next lowest without afterpoint
      xRem  = xSrc - TempI;               // remainder
      xSrc  = xSrc >> 4;                  // before point

      if( xSrc < 0 || xSrc > xx1) {       // out of source image

        continue;
      }

      // get data from source image

      p8s1 = (uchar *)RGB_pixad( xSrc, ySrc, &iSrc);
      p8s2 = ySrc == yy1 ? p8s1 : p8s1 + iSrc.ld;
      xOff = xSrc == xx1 ? 0 : iSrc.d;

      // Processing depending from source image type

      switch( iSrc.d) {

      default: // unknown image type

        goto ExitPoint;
        break;

      case 1: // Black/white image

        // make bilinear interpolation

        TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                 ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

        d8[ 0] = TempI;
        d8[ 1] = TempI;
        d8[ 2] = TempI;

        // d8[ 3] = NewAlpha;    // Keep alpha in destination

        break;

      case 2: // Black/white image with alpha

        // make bilinear interpolation

        AlphaVal = (((( p8s1[1] * (16 - xRem) + p8s1[ xOff + 1] * xRem) >> 4) * (16 - yRem)) +
                    ((( p8s2[1] * (16 - xRem) + p8s2[ xOff + 1] * xRem) >> 4) * yRem)) >> 4;


        TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                 ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

        d8[ 0] = TempI;
        d8[ 1] = TempI;
        d8[ 2] = TempI;

        d8[ 3] = AlphaVal;

        break;

      case 3: // Color image

        // make bilinear interpolation

        TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                 ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

        d8[ 0] = TempI;

        p8s1++; p8s2++;

        TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                 ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

        d8[ 1] = TempI;

        p8s1++; p8s2++;

        TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                 ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

        d8[ 2] = TempI;

        // d8[ 3] = NewAlpha;    // Keep alpha in destination

        break;

      case 4: // Color image with alpha

        AlphaVal = (((( p8s1[3] * (16 - xRem) + p8s1[ xOff + 3] * xRem) >> 4) * (16 - yRem)) +
                    ((( p8s2[3] * (16 - xRem) + p8s2[ xOff + 3] * xRem) >> 4) * yRem)) >> 4;

        TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                 ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

        d8[ 0] = TempI;

        p8s1++; p8s2++;

        TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                 ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

        d8[ 1] = TempI;

        p8s1++; p8s2++;

        TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                 ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

        d8[ 2] = TempI;

        d8[ 3] = AlphaVal;

        break;

      }   // end switch( iSrc.d)

      d8 += 4;                               // Destination is color with alpha

    }
  }

  ierr = 0;  // Return OK

ExitPoint:

#ifdef USE_OVERLAY_SCALE_HACK // Use a hack for better scale results.

  if( pTmp != NULL) {              // used a temporary image

    pTmp->release();               // Release image data
  }
#endif

  return( ierr);
}

/***************************************************************************
* YaIPS_RGB_ShapeGen
* Generate an image with a shape
*
* ppDst         Out: Pointer to pointer to RGB color image
* xx, yy        In: Size of image
* pShapeGen     In: shape generation parameter
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_ShapeGen( Fl_RGB_Image **ppDst,                 // Out: Pointer to pointer to RGB color image
                        int xx, int yy,                       // In: Size of image
                        YaIPS_RGB_ShapeGen_Par_t *pShapeGen,  // In: shape generation parameter
                        Fl_RGB_Image **ppShadow,              // Out: Pointer to pointer to RGB shadow image
                        Fl_RGB_Image **ppBGndFile,            // For background type YAIPS_SHAPE_GEN_BGND_IMAGE: Loaded background image file
                        Fl_Text_Buffer *pTextBuffer)          // Holds text to display inside the overlay
{
  Fl_RGB_Image *pDst, *pShadow, *pImgTmp;
  uchar *pDataDst, *s8, *d8;
  int ierr, x, y, SizeInBytes, DrawShape, DrawLine, DrawText, NewAlpha, OldAlpha, UseLineColor, BgndImageWithAlpha;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  uchar r, g, b;

  if( ppDst == NULL) {                         // Security test

    return( -110);
  }

  // Ensure destination image has the correct size

  pDst = *ppDst;                               // Get pointer to destination image

  if( pDst == NULL ||                          // Have NO image
      xx != pDst->data_w() ||                  // Something different
      yy != pDst->data_h() ||
      4 != pDst->d()) {

    if( pDst) {                                // Have an image

      pDst->release();                         // Release old memory
    }

    // Allocate memory for image

    SizeInBytes = xx * yy * 4;                 // Color image with alpha part

    pDataDst = new uchar[ SizeInBytes];

    if( pDataDst == NULL) {                    // Security test

      return( -115);
    }

    // Create image
    pDst = new Fl_RGB_Image( pDataDst, xx, yy, 4);

    if( pDst == NULL) {                        // Security test

      return( -116);
    }

    pDst->alloc_array = 1;                     // Flag, data is allocated

    *ppDst = pDst;                             // Set pointer to destination image
  }

  // Convert to YaIPS_RGB_ImgD_t image descriptor

  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Generate the background

  DrawShape = false;
  DrawLine  = false;
  DrawText  = false;

  if( pShapeGen->ShapeType != YAIPS_SHAPE_GEN_TYPE_NONE) {                      // We have a shape selected

    DrawShape = (pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_DRAW_SHAPE) != 0; // Do we draw the filled shape

    DrawLine  = (pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_DRAW_LINE) != 0;  // Do we generate lines

  } else {

    DrawShape = false;
    DrawLine = false;
  }

  DrawText = pTextBuffer != NULL && pTextBuffer->length() > 0;    // Have a text buffer and any bytes in the text buffer

  // Prepare background

  BgndImageWithAlpha = false;

  switch( pShapeGen->BGndType) {

  default:
  case YAIPS_SHAPE_GEN_BGND_COLOR:  // Background: color

    if( *ppBGndFile != NULL) {        // Have a background file loaded

      (*ppBGndFile)->release();       // Can release the image

      *ppBGndFile = NULL;             // Flag image is released
    }

    ierr = YaIPS_RGB_SetColor( pDst,
                               pShapeGen->BGndCol_LT, pShapeGen->BGndCol_RT,
                               pShapeGen->BGndCol_LB, pShapeGen->BGndCol_RB,
                               pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_CBIT_MASK,
                               DrawShape || DrawLine || DrawText ? 0 : 255);     // Without shape the alpha set else alpha is reset

    break;

  case YAIPS_SHAPE_GEN_BGND_IMAGE:

    if( *ppBGndFile == NULL &&                  // Have no file image loaded
        pShapeGen->BGndFileName[ 0] != '\0') {  // and have a file name

      // load the image file file

      *ppBGndFile = YaIPS_Image_Read( pShapeGen->BGndFileName);   // Try to load an image
    }

    // Preset overlay with black

    ierr = YaIPS_RGB_SetColor( pDst, 0, 0, 0, 0, 0,                  // Color black
#ifdef use_again
                               /*DrawShape || DrawLine || DrawText ? 0 :*/ 255);     // Without shape the alpha set else alpha is reset
#else
                               DrawShape || DrawLine || DrawText ? 0 : 255);     // Without shape the alpha set else alpha is reset
#endif

    // Copy background image
    if( *ppBGndFile != NULL) {         // Have an image file loaded

      BgndImageWithAlpha = (*ppBGndFile)->d() == 2 || (*ppBGndFile)->d() == 4;

      YaIPS_RGB_CopyBgndToOverlay( &iDst, xx, yy, *ppBGndFile);
    }

    break;

  case YAIPS_SHAPE_GEN_BGND_WINDOW:  // Background: Tool windows

    if( *ppBGndFile != NULL) {        // Have a background file loaded

      (*ppBGndFile)->release();       // Can release the image

      *ppBGndFile = NULL;             // Flag image is released
    }

    // Check for valid window output image

    Fl_RGB_Image *pImg_In;

    pImg_In = NULL;       // Will be set if there is a valid image

    // NOTE: Argument 'DstWinIdNr' is not needed to check.
    YaIPS_ToolWinInputCheck( -1, pShapeGen->BGnd_WinIdNr, NULL, &pImg_In, NULL);

    // Preset overlay with black

    ierr = YaIPS_RGB_SetColor( pDst, 0, 0, 0, 0, 0,                  // Color black
#ifdef use_again
                               /*DrawShape || DrawLine || DrawText ? 0 :*/ 255);     // Without shape the alpha set else alpha is reset
#else
                               DrawShape || DrawLine || DrawText ? 0 : 255);     // Without shape the alpha set else alpha is reset
#endif

    // Copy background image
    if( pImg_In != NULL) {         // Have an image file loaded

      BgndImageWithAlpha = pImg_In->d() == 2 || pImg_In->d() == 4;

      YaIPS_RGB_CopyBgndToOverlay( &iDst, xx, yy, pImg_In);
    }

    break;
  }

  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Prepare shape generation

  if( DrawShape) {                                     // Generate a shape

    // Draw a white shape image without lines
    // NOTE: no lines are drawn

    pImgTmp = NULL;    // Have no image until now
    pImgTmp = pShapeGenSub( xx, yy, pShapeGen, true, false);   // draw shape only

    if( pImgTmp == NULL) {                     // No image generated

      return( -120);
    }

    if( pImgTmp->d() != 3) {                   // Expect an color image here

      return( -121);
    }

    // Convert white shape to alpha

    ierr = YaIPS_RGB_to_ImgD( pImgTmp, &iSrc);
    if( ierr != 0)  {                           // Check for error
      return( ierr);
    }

    for( y = 0; y < yy; y++) {

      d8 = RGB_pixad( 0,  y, &iDst);
      s8 = RGB_pixad( 0,  y, &iSrc);

      for( x = 0; x < xx; x++) {

        NewAlpha = s8[ 0];                       // Use this as new alpha

        if( BgndImageWithAlpha) {

          if( NewAlpha < 255) {                  // Have a line drawn

            OldAlpha = d8[ 3];                   // Current alpha value

            if( NewAlpha < OldAlpha) {           // is higher then current alpha
              d8[ 3] = NewAlpha;                 // Set higher value
            }
          }

        } else {

          if( NewAlpha > 0) {                    // Have a line drawn

            OldAlpha = d8[ 3];                   // Current alpha value

            if( NewAlpha > OldAlpha) {           // is higher then current alpha
              d8[ 3] = NewAlpha;                 // Set higher value
            }
          }
        }

        d8 += 4;                               // Destination is color with alpha
        s8 += 3;                               // Source is color only
      }
    }

    pImgTmp->release();                        // Release the temporary image
  }

  if( DrawLine) {  // Generate shape outline part

    // Draw a white shape image without lines
    // NOTE: no lines are drawn

    pImgTmp = NULL;    // Have no image until now
    pImgTmp = pShapeGenSub( xx, yy, pShapeGen, false, true);   // draw line only

    if( pImgTmp == NULL) {                     // No image generated

      return( -120);
    }

    if( pImgTmp->d() != 3) {                   // Expect an color image here

      return( -121);
    }

    // Convert white outline to alpha and colored outline

    Fl::get_color( pShapeGen->LineColor, r, g, b);             // Convert color to RGB values

    UseLineColor = (pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_LINE_COL_USE) != 0;  // Use the line color

    ierr = YaIPS_RGB_to_ImgD( pImgTmp, &iSrc);
    if( ierr != 0)  {                           // Check for error
      return( ierr);
    }

    for( y = 0; y < yy; y++) {

      d8 = RGB_pixad( 0,  y, &iDst);
      s8 = RGB_pixad( 0,  y, &iSrc);

      for( x = 0; x < xx; x++) {

        NewAlpha = s8[ 0];                     // Use this as new alpha

        if( NewAlpha > 0) {                    // Have a line drawn

          OldAlpha = d8[ 3];                   // Current alpha value

          if( NewAlpha > OldAlpha) {           // is higher then current alpha
            d8[ 3] = NewAlpha;                 // Set higher value
          }

          if( UseLineColor) {                  // Use the line color

            // Merge outline to image

            if( NewAlpha == 255) {

              d8[ 0] = r;
              d8[ 1] = g;
              d8[ 2] = b;

            } else {

              if( OldAlpha == 255) {

                d8[ 0] = (NewAlpha * r + (255 - NewAlpha) * d8[ 0]) / 255;
                d8[ 1] = (NewAlpha * g + (255 - NewAlpha) * d8[ 1]) / 255;
                d8[ 2] = (NewAlpha * b + (255 - NewAlpha) * d8[ 2]) / 255;

              } else {     // Must be line outside the shape, line color is blended by alpha

                d8[ 0] = r;
                d8[ 1] = g;
                d8[ 2] = b;
              }
            }
          }
        }

        d8 += 4;                               // Destination is color with alpha
        s8 += 3;                               // Source is color only
      }
    }

    pImgTmp->release();                        // Release the temporary image
  }

  if( DrawText) {  // Generate Text

    // Draw a white text

    pImgTmp = pShapeGenText( xx, yy, pShapeGen, pTextBuffer);   // draw text only

    if( pImgTmp == NULL) {                     // No image generated

      return( -120);
    }

    if( pImgTmp->d() != 3) {                   // Expect an color image here

      return( -121);
    }

    // Convert white text to alpha and colored text

    Fl::get_color( pShapeGen->FontColor, r, g, b);             // Convert color to RGB values

    UseLineColor = (pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_FONT_COL_USE) != 0;  // Use the font color

    ierr = YaIPS_RGB_to_ImgD( pImgTmp, &iSrc);
    if( ierr != 0)  {                           // Check for error
      return( ierr);
    }

    for( y = 0; y < yy; y++) {

      d8 = RGB_pixad( 0,  y, &iDst);
      s8 = RGB_pixad( 0,  y, &iSrc);

      for( x = 0; x < xx; x++) {

        NewAlpha = s8[ 0];                     // Use this as new alpha

        if( NewAlpha > 0) {                    // Have a line drawn

          OldAlpha = d8[ 3];                   // Current alpha value

          if( NewAlpha > OldAlpha) {           // is higher then current alpha
            d8[ 3] = NewAlpha;                 // Set higher value
          }

          if( UseLineColor) {                  // Use the line color

            // Merge outline to image

            if( NewAlpha == 255) {

              d8[ 0] = r;
              d8[ 1] = g;
              d8[ 2] = b;

            } else {

              if( OldAlpha == 255) {

                d8[ 0] = (NewAlpha * r + (255 - NewAlpha) * d8[ 0]) / 255;
                d8[ 1] = (NewAlpha * g + (255 - NewAlpha) * d8[ 1]) / 255;
                d8[ 2] = (NewAlpha * b + (255 - NewAlpha) * d8[ 2]) / 255;

              } else {     // Must be line outside the shape, line color is blended by alpha

                d8[ 0] = r;
                d8[ 1] = g;
                d8[ 2] = b;
              }
            }
          }
        }

        d8 += 4;                               // Destination is color with alpha
        s8 += 3;                               // Source is color only
      }
    }

    pImgTmp->release();                        // Release the temporary image
  }

  if( (pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_SHADOW_USE) == 0 ||   // Generate NO shadow image
      pShapeGen->ShadowTrans >= 100) {                                    // 100 % transparent

    // Ensure there is no shadow image

    if( *ppShadow != NULL) {        // Have a shadow image

      (*ppShadow)->release();       // Can release the image

      *ppShadow = NULL;             // Flag image is released
    }

  } else {      // Generate shadow image

    if( pShapeGen->ShadowBlur >= 2) {    // Blur image

      int KernelSize, KernelHalf;

      KernelSize = pShapeGen->ShadowBlur;               // Get kernel size for blur filter

      KernelSize |= 1;                                  // Must be odd
      if( KernelSize < 3) {                             // Ensure minimum value
        KernelSize = 3;
      }

      KernelHalf = KernelSize / 2;                      // Half kernel size

      // Create temporary image

      pImgTmp = NULL;    // Have no image until now
      ierr = YaIPS_RGB_ImageSetSize( &pImgTmp, pDst->w() + 2 * KernelHalf, pDst->h() + 2 * KernelHalf, pDst->d());

      if( ierr != 0)  {                           // Check for error
        return( ierr);
      }

      YaIPS_RGB_SetVal( pImgTmp, 0);              // Black the image

      ierr = YaIPS_RGB_CopyInImg( pImgTmp, pDst, KernelHalf, KernelHalf); // Copy in the destination image

      if( ierr != 0)  {                           // Check for error
        pImgTmp->release();                        // Release the temporary image
        pImgTmp = NULL;
        return( ierr);
      }

      ierr = YaIPS_RGB_GaussXY( ppShadow, pImgTmp, KernelSize, YAIPS_GAUSXY_MODE_XY);

      pImgTmp->release();                        // Release the temporary image
      pImgTmp = NULL;

      if( ierr != 0)  {                           // Check for error
        return( ierr);
      }

    } else {                            // No blur

      ierr = YaIPS_RGB_CopyImg( ppShadow, pDst);  // Simply copy destination image
    }

    if( ierr != 0)  {                           // Check for error
      return( ierr);
    }

    pShadow =  *ppShadow;                       // Get pointer to shadow image

    ierr = YaIPS_RGB_to_ImgD( pShadow, &iSrc);
    if( ierr != 0)  {                           // Check for error
      return( ierr);
    }

    xx = iSrc.xx;                               // Size may have changed. Update size.
    yy = iSrc.yy;

    // Set specific color of shadow

    if( (pShapeGen->ShapeFlags & YAIPS_SHAPE_GEN_FLAG_SHADOW_COL_USE) != 0) {  // Use shadow color

      Fl::get_color( pShapeGen->ShadowColor, r, g, b);             // Convert color to RGB values

      for( y = 0; y < yy; y++) {

        s8 = RGB_pixad( 0,  y, &iSrc);

        for( x = 0; x < xx; x++) {

          s8[ 0] = r;
          s8[ 1] = g;
          s8[ 2] = b;

          s8 += iSrc.d;                               // Destination is color with alpha
        }
      }
    }

    if( pShapeGen->ShadowTrans > 0) {           // Have a reduced transparency

      int TempMult;

      // Shadow transparency, modify alpha of shadow image

      if( pShapeGen->ShadowTrans >= 100) {      // 100 % transparent

        TempMult = 0;

      } else {

        TempMult = (100 - pShapeGen->ShadowTrans) * ( (1 << 16) / 100);
      }

      for( y = 0; y < yy; y++) {

        s8 = RGB_pixad( 0,  y, &iSrc);

        for( x = 0; x < xx; x++) {

          NewAlpha = (s8[ 3] * TempMult) >> 16;  // Multiply with transparency factgor

          s8[ 3] = NewAlpha;                     // Replace

          s8 += iSrc.d;                               // Destination is color with alpha
        }
      }
    }
  }

  return( 0);                                   // Return OK
}

/***************************************************************************
* YaIPS_RGB_OverlaySub
* Overlay image
*
* pDst             Point to RGB color image. Must exist and may have an alpha channel.
* pOverlay         Point to overlay data
*
* return     0 OK
*            1 Overlay is out of image boundaries, it is not drawn.
*          < 0 Error
****************************************************************************
*/

static int YaIPS_RGB_OverlaySub( Fl_RGB_Image *pDst,       // Pointer to RGB color image. Must exist and may have an alpha channel.
                                 Fl_RGB_Image *pSrc,       // Point to overlay data
                                 int xPos, int yPos,       // Where to overlay
                                 float Rotation,           // Rotation
                                 float AlphaMult)          // Alpha multiplier %, range 0.0 .. 100.0.

{
  int ierr, nByteSrc, nByteDst;
  int xxDst, yyDst, xxSrc, yySrc, xSrc, ySrc, xmSrc;
  int AlphaVal, AlphaMultInt;
  YaIPS_RGB_ImgD_t iDst, iSrc;

  // Convert destination image
  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get destination image data

  xxDst    = iDst.xx;
  yyDst    = iDst.yy;
  //x/xmDst    = iDst.ld;
  nByteDst = iDst.d;

  // Destination must be a color image

  if( nByteDst < 3 || nByteDst > 4) {    // is not color image ?

    return( -102);
  }

  // Convert source image
  ierr = YaIPS_RGB_to_ImgD( pSrc, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get source image data

  xxSrc    = iSrc.xx;
  yySrc    = iSrc.yy;
  xmSrc    = iSrc.ld;
  nByteSrc = iSrc.d;

  if( Rotation >= 360.0) {             // A rotation of 360 degree should be handled like one with 0 degree

    Rotation -= 360.0;
  }

  // Alpha multiplier %, range 0.0 .. 100.0

  AlphaMultInt = (int)(AlphaMult * 2.56 + 0.5);
  if( AlphaMultInt <   0) AlphaMultInt = 0;
  if( AlphaMultInt > 256) AlphaMultInt = 256;

  if( AlphaMultInt == 0) {     // Image is not visible

    return( 0);   // Return OK
  }

  // ...

  if( Rotation == 0.0) {               // Use fast variant of overlay code

    int ySrcBegin, ySrcEnd, xSrcBegin, xSrcEnd;
    uchar *p8d, *p8s;

    // Some preparations

    xSrcBegin = 0;
    xSrcEnd   = xxSrc;

    if( xPos < 0) {                     // Is over left side

      xSrcBegin = - xPos;               // begin later
      xPos = 0;
    }

    if( xPos + xSrcEnd >= xxDst) {     // Is over right side

      xSrcEnd = xxDst - xPos;          // Need less lines to draw
    }

    ySrcBegin = 0;
    ySrcEnd = yySrc;

    if( yPos < 0) {                     // Is over top side

      ySrcBegin = - yPos;               // begin later
      //x/yPos = 0;
    }

    if( yPos + ySrcEnd >= yyDst) {     // Is over bottom side

      ySrcEnd = yyDst - yPos;          // Need less lines to draw
    }

    if( xSrcEnd <= 0 ||              // Check for overlay out of image boundaries
        ySrcEnd <= 0) {

      return( 1);
    }

    // Processing depending from source image type

    switch( nByteSrc) {

    default: // unknown image type

      return( -103);
      break;

    case 1: // Black/white image

      AlphaVal = (255 * AlphaMultInt) >> 8;

      for( ySrc = ySrcBegin; ySrc < ySrcEnd; ySrc++) {

        p8d = (uchar *)RGB_pixad( xPos, ySrc + yPos, &iDst);
        p8s = (uchar *)RGB_pixad( xSrcBegin, ySrc, &iSrc);

        if( AlphaVal == 255) {

          for( xSrc = xSrcBegin; xSrc < xSrcEnd; xSrc++) {

            p8d[ 0] = p8s[ 0];
            p8d[ 1] = p8s[ 0];
            p8d[ 2] = p8s[ 0];

            p8d += nByteDst;
            p8s += nByteSrc;
          }

        } else if( AlphaVal > 0) {

          for( xSrc = xSrcBegin; xSrc < xSrcEnd; xSrc++) {

            p8d[ 0] = (p8d[ 0] * (255 - AlphaVal) + p8s[ 0] * AlphaVal) / 255;
            p8d[ 1] = (p8d[ 1] * (255 - AlphaVal) + p8s[ 0] * AlphaVal) / 255;
            p8d[ 2] = (p8d[ 2] * (255 - AlphaVal) + p8s[ 0] * AlphaVal) / 255;

            p8d += nByteDst;
            p8s += nByteSrc;
          }
        }
      }

      break;

    case 2: // Black/white image with alpha

      for( ySrc = ySrcBegin; ySrc < ySrcEnd; ySrc++) {

        p8d = (uchar *)RGB_pixad( xPos, ySrc + yPos, &iDst);
        p8s = (uchar *)RGB_pixad( xSrcBegin, ySrc, &iSrc);

        for( xSrc = xSrcBegin; xSrc < xSrcEnd; xSrc++) {

          AlphaVal = (p8s[ 1] * AlphaMultInt) >> 8;

          if( AlphaVal > 0) {

            if( AlphaVal == 255) {

              p8d[ 0] = p8s[ 0];
              p8d[ 1] = p8s[ 0];
              p8d[ 2] = p8s[ 0];

            } else {

              p8d[ 0] = (p8d[ 0] * (255 - AlphaVal) + p8s[ 0] * AlphaVal) / 255;
              p8d[ 1] = (p8d[ 1] * (255 - AlphaVal) + p8s[ 0] * AlphaVal) / 255;
              p8d[ 2] = (p8d[ 2] * (255 - AlphaVal) + p8s[ 0] * AlphaVal) / 255;
            }
          }

          p8d += nByteDst;
          p8s += nByteSrc;
        }
      }

      break;

    case 3: // Color image

      AlphaVal = (255 * AlphaMultInt) >> 8;

      for( ySrc = ySrcBegin; ySrc < ySrcEnd; ySrc++) {

        p8d = (uchar *)RGB_pixad( xPos, ySrc + yPos, &iDst);
        p8s = (uchar *)RGB_pixad( xSrcBegin, ySrc, &iSrc);

        if( AlphaVal == 255) {

          for( xSrc = xSrcBegin; xSrc < xSrcEnd; xSrc++) {

            p8d[ 0] = p8s[ 0];
            p8d[ 1] = p8s[ 1];
            p8d[ 2] = p8s[ 2];

            p8d += nByteDst;
            p8s += nByteSrc;
          }
        } else if( AlphaVal > 0) {

          for( xSrc = xSrcBegin; xSrc < xSrcEnd; xSrc++) {

            p8d[ 0] = (p8d[ 0] * (255 - AlphaVal) + p8s[ 0] * AlphaVal) / 255;
            p8d[ 1] = (p8d[ 1] * (255 - AlphaVal) + p8s[ 1] * AlphaVal) / 255;
            p8d[ 2] = (p8d[ 2] * (255 - AlphaVal) + p8s[ 2] * AlphaVal) / 255;

            p8d += nByteDst;
            p8s += nByteSrc;
          }
        }
      }

      break;

    case 4: // Color image with alpha

      for( ySrc = ySrcBegin; ySrc < ySrcEnd; ySrc++) {

        p8d = (uchar *)RGB_pixad( xPos, ySrc + yPos, &iDst);
        p8s = (uchar *)RGB_pixad( xSrcBegin, ySrc, &iSrc);

        for( xSrc = xSrcBegin; xSrc < xSrcEnd; xSrc++) {

          AlphaVal = (p8s[ 3] * AlphaMultInt) >> 8;

          if( AlphaVal > 0) {

            if( AlphaVal == 255) {

              p8d[ 0] = p8s[ 0];
              p8d[ 1] = p8s[ 1];
              p8d[ 2] = p8s[ 2];

            } else {

              p8d[ 0] = (p8d[ 0] * (255 - AlphaVal) + p8s[ 0] * AlphaVal) / 255;
              p8d[ 1] = (p8d[ 1] * (255 - AlphaVal) + p8s[ 1] * AlphaVal) / 255;
              p8d[ 2] = (p8d[ 2] * (255 - AlphaVal) + p8s[ 2] * AlphaVal) / 255;
            }
          }

          p8d += nByteDst;
          p8s += nByteSrc;
        }
      }

      break;
    }

  } else {               // Use variant with overlay and scale

    int xx1, yy1, xx2, yy2, xDst, yDst, xSrc, ySrc, xRem, yRem, TempI;
    double xx2D, yy2D, Angle, AngleSin, AngleCos, CenterX, CenterY;
    int yDstBegin, yDstEnd, xDstBegin, xDstEnd, xOff;
    double xSrcIn, ySrcIn, xSrcD1, ySrcD1;
    uchar *p8d, *p8s1, *p8s2;

    xx1 = xxSrc - 1;         // need a border of 1 for bilinear interpolation
    yy1 = yySrc - 1;

    xx2 = xxSrc / 2;         // image center
    yy2 = yySrc / 2;

    xx2D = xx2;
    yy2D = yy2;

    // rotation

    Angle = Rotation * M_PI / 180.0;                 // convert rotation from degree to radiant
    //x/Angle = -1.0 * Angle;                            // change direction, we work backwards

    AngleSin = sin( Angle);
    AngleCos = cos( Angle);

    // Get data for rotated AOI

    int x1T, y1T, x2T, y2T, x3T, y3T, x4T, y4T, xTemp, yTemp;

    x1T = xPos;
    y1T = yPos;

    x2T = xPos + xxSrc - 1;
    y2T = y1T;

    x4T = x2T;
    y4T = yPos + yySrc - 1;

    x3T = x1T;
    y3T = y4T;

    CenterX = (x1T + x2T) * 0.5;
    CenterY = (y1T + y4T) * 0.5;

    xTemp = (int)floor( (CenterX - x1T) * AngleCos + (CenterY - y1T) * AngleSin + CenterX);
    yTemp = (int)floor( (CenterY - y1T) * AngleCos - (CenterX - x1T) * AngleSin + CenterY);

    xDstBegin = xTemp;
    xDstEnd   = xTemp;
    yDstBegin = yTemp;
    yDstEnd   = yTemp;

    xTemp = (int)ceil( (CenterX - x1T) * AngleCos + (CenterY - y1T) * AngleSin + CenterX);
    yTemp = (int)ceil( (CenterY - y1T) * AngleCos - (CenterX - x1T) * AngleSin + CenterY);

    if( xTemp > xDstEnd  ) xDstEnd   = xTemp;
    if( yTemp > yDstEnd  ) yDstEnd   = yTemp;

    xTemp = (int)floor( (CenterX - x2T) * AngleCos + (CenterY - y2T) * AngleSin + CenterX);
    yTemp = (int)floor( (CenterY - y2T) * AngleCos - (CenterX - x2T) * AngleSin + CenterY);

    if( xTemp < xDstBegin) xDstBegin = xTemp;
    if( yTemp < yDstBegin) yDstBegin = yTemp;

    xTemp = (int)ceil( (CenterX - x2T) * AngleCos + (CenterY - y2T) * AngleSin + CenterX);
    yTemp = (int)ceil( (CenterY - y2T) * AngleCos - (CenterX - x2T) * AngleSin + CenterY);

    if( xTemp > xDstEnd  ) xDstEnd   = xTemp;
    if( yTemp > yDstEnd  ) yDstEnd   = yTemp;

    xTemp = (int)floor( (CenterX - x3T) * AngleCos + (CenterY - y3T) * AngleSin + CenterX);
    yTemp = (int)floor( (CenterY - y3T) * AngleCos - (CenterX - x3T) * AngleSin + CenterY);

    if( xTemp < xDstBegin) xDstBegin = xTemp;
    if( yTemp < yDstBegin) yDstBegin = yTemp;

    xTemp = (int)ceil( (CenterX - x3T) * AngleCos + (CenterY - y3T) * AngleSin + CenterX);
    yTemp = (int)ceil( (CenterY - y3T) * AngleCos - (CenterX - x3T) * AngleSin + CenterY);

    if( xTemp > xDstEnd  ) xDstEnd   = xTemp;
    if( yTemp > yDstEnd  ) yDstEnd   = yTemp;

    xTemp = (int)floor( (CenterX - x4T) * AngleCos + (CenterY - y4T) * AngleSin + CenterX);
    yTemp = (int)floor( (CenterY - y4T) * AngleCos - (CenterX - x4T) * AngleSin + CenterY);

    if( xTemp < xDstBegin) xDstBegin = xTemp;
    if( yTemp < yDstBegin) yDstBegin = yTemp;

    xTemp = (int)ceil( (CenterX - x4T) * AngleCos + (CenterY - y4T) * AngleSin + CenterX);
    yTemp = (int)ceil( (CenterY - y4T) * AngleCos - (CenterX - x4T) * AngleSin + CenterY);

    if( xTemp > xDstEnd  ) xDstEnd   = xTemp;
    if( yTemp > yDstEnd  ) yDstEnd   = yTemp;

    // Some preparations

    if( xDstBegin < 0) {               // Is over left side

      xDstBegin = 0;                   // begin later
    }

    if( xDstEnd >= xxDst) {            // Is over right side

      xDstEnd = xxDst - 1;             // Need less lines to draw
    }

    if( yDstBegin < 0) {               // Is below bottom side

      yDstBegin = 0;                   // begin later
    }

    if( yDstEnd >= yyDst) {            // Is over bottom side

      yDstEnd = yyDst - 1;             // Need less lines to draw
    }

    if( xDstEnd <= 0 ||                // Check for overlay out of image boundaries
        yDstEnd <= 0) {

      return( 1);
    }

    // transform the image

    AlphaVal = (255 * AlphaMultInt) >> 8;   // Alpha value for source images without alpha channel

    for( yDst = yDstBegin; yDst <= yDstEnd; yDst++) {

      p8d = (uchar *)RGB_pixad( xDstBegin, yDst, &iDst);

      xSrcIn = xDstBegin;
      ySrcIn = yDst;

      // Relative to image center
      xSrcIn = xSrcIn - CenterX;
      ySrcIn = ySrcIn - CenterY;

      for( xDst = xDstBegin; xDst <= xDstEnd; xDst++, p8d += nByteDst, xSrcIn += 1.0) {

        // rotation of image

        xSrcD1 = xSrcIn * AngleCos - ySrcIn * AngleSin;
        ySrcD1 = ySrcIn * AngleCos + xSrcIn * AngleSin;

        // relative to left upper corner

        xSrcD1 = xSrcD1 + xx2D;
        ySrcD1 = ySrcD1 + yy2D;

        // float coordinates to integer
        // + remainder (for bilinear interpolation), has 4 afterpoint bits

        xSrc = (int)(xSrcD1 * 16.0);        // with afterpoint digits
        TempI = xSrc & 0xfffffff0;          // next lowest without afterpoint
        xRem  = xSrc - TempI;               // remainder
        xSrc  = xSrc >> 4;                  // before point

        if( xSrc < 0 || xSrc > xx1) {       // out of source image

          continue;
        }

        ySrc = (int)(ySrcD1 * 16.0);        // with afterpoint digits
        TempI = ySrc & 0xfffffff0;          // next lowest without afterpoint
        yRem  = ySrc - TempI;               // remainder
        ySrc  = ySrc >> 4;                  // before point

        if( ySrc < 0 || ySrc > yy1) {       // out of source image

          continue;
        }

        // get data from source image

        p8s1 = (uchar *)RGB_pixad( xSrc, ySrc, &iSrc);
        p8s2 = ySrc == yy1 ? p8s1 : p8s1 + xmSrc;
        xOff = xSrc == xx1 ? 0 : nByteSrc;

        // Processing depending from source image type

        switch( nByteSrc) {

        default: // unknown image type

          return( -103);
          break;

        case 1: // Black/white image

          // make bilinear interpolation

          TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                   ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

          if( AlphaVal == 255) {

            p8d[ 0] = TempI;
            p8d[ 1] = TempI;
            p8d[ 2] = TempI;

          } else {

            p8d[ 0] = (p8d[ 0] * (255 - AlphaVal) + TempI * AlphaVal) / 255;
            p8d[ 1] = (p8d[ 1] * (255 - AlphaVal) + TempI * AlphaVal) / 255;
            p8d[ 2] = (p8d[ 2] * (255 - AlphaVal) + TempI * AlphaVal) / 255;
          }

          break;

        case 2: // Black/white image with alpha

          // make bilinear interpolation

          AlphaVal = (((( p8s1[1] * (16 - xRem) + p8s1[ xOff + 1] * xRem) >> 4) * (16 - yRem)) +
                      ((( p8s2[1] * (16 - xRem) + p8s2[ xOff + 1] * xRem) >> 4) * yRem)) >> 4;

          AlphaVal = (AlphaVal * AlphaMultInt) >> 8;

          TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                   ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

          if( AlphaVal > 0) {

            if( AlphaVal == 255) {

              p8d[ 0] = TempI;
              p8d[ 1] = TempI;
              p8d[ 2] = TempI;

            } else {

              p8d[ 0] = (p8d[ 0] * (255 - AlphaVal) + TempI * AlphaVal) / 255;
              p8d[ 1] = (p8d[ 1] * (255 - AlphaVal) + TempI * AlphaVal) / 255;
              p8d[ 2] = (p8d[ 2] * (255 - AlphaVal) + TempI * AlphaVal) / 255;
            }
          }

          break;

        case 3: // Color image

          // make bilinear interpolation

          TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                   ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

          if( AlphaVal == 255) {

            p8d[ 0] = TempI;

          } else {

            p8d[ 0] = (p8d[ 0] * (255 - AlphaVal) + TempI * AlphaVal) / 255;
          }

          p8s1++; p8s2++;

          TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                   ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

          if( AlphaVal == 255) {

            p8d[ 1] = TempI;

          } else {

            p8d[ 1] = (p8d[ 1] * (255 - AlphaVal) + TempI * AlphaVal) / 255;
          }

          p8s1++; p8s2++;

          TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                   ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

          if( AlphaVal == 255) {

            p8d[ 2] = TempI;

          } else {

            p8d[ 2] = (p8d[ 2] * (255 - AlphaVal) + TempI * AlphaVal) / 255;
          }

          break;

        case 4: // Color image with alpha

          AlphaVal = (((( p8s1[3] * (16 - xRem) + p8s1[ xOff + 3] * xRem) >> 4) * (16 - yRem)) +
                      ((( p8s2[3] * (16 - xRem) + p8s2[ xOff + 3] * xRem) >> 4) * yRem)) >> 4;

          AlphaVal = (AlphaVal * AlphaMultInt) >> 8;

          if( AlphaVal > 0) {

            TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                     ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

            if( AlphaVal == 255) {

              p8d[ 0] = TempI;

            } else {

              p8d[ 0] = (p8d[ 0] * (255 - AlphaVal) + TempI * AlphaVal) / 255;
            }

            p8s1++; p8s2++;

            TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                     ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

            if( AlphaVal == 255) {

              p8d[ 1] = TempI;

            } else {

              p8d[ 1] = (p8d[ 1] * (255 - AlphaVal) + TempI * AlphaVal) / 255;
            }

            p8s1++; p8s2++;

            TempI = (((( p8s1[0] * (16 - xRem) + p8s1[ xOff] * xRem) >> 4) * (16 - yRem)) +
                     ((( p8s2[0] * (16 - xRem) + p8s2[ xOff] * xRem) >> 4) * yRem)) >> 4;

            if( AlphaVal == 255) {

              p8d[ 2] = TempI;

            } else {

              p8d[ 2] = (p8d[ 2] * (255 - AlphaVal) + TempI * AlphaVal) / 255;
            }
          }

          break;
        }
      }
    }
  }

  return( 0);   // Return OK
}

/***************************************************************************
* YaIPS_RGB_Overlay
* Overlay image
*
* pDst             Point to RGB color image. Must exist and may have an alpha channel.
* pOverlay         Point to overlay data
*
* return     0 OK
*            1 Overlay is out of image boundaries, it is not drawn.
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Overlay( Fl_RGB_Image *pDst,               // Pointer to RGB color image. Must exist and may have an alpha channel.
                       YaIPS_OverlayData_t *pOverlay)    // Point to overlay data
{
  int ierr;

  //
  // Overlay shadow image first
  //

  if( pOverlay->pImgShadow != NULL)  {   // Have a shadow image

    double Angle, AngleSin, AngleCos;
    int XOffset, YOffset, KernelHalf;

    XOffset = 0;
    YOffset = 0;
    KernelHalf = 0;

    Angle = pOverlay->ShapeGen.ShadowAngle * M_PI / 180.0;      // convert rotation from degree to radiant
    AngleSin = sin( Angle);
    AngleCos = cos( Angle);

    XOffset += (int)round( pOverlay->ShapeGen.ShadowDist * AngleCos);
    YOffset -= (int)round( pOverlay->ShapeGen.ShadowDist * AngleSin);

    // Blurred shadow images are bigger then the overlay image.
    // Correct position for blurred shadows.

    if( pOverlay->ShapeGen.ShadowBlur >= 2) {    // Blur image

      int KernelSize;

      KernelSize = pOverlay->ShapeGen.ShadowBlur;       // Get kernel size for blur filter

      KernelSize |= 1;                                  // Must be odd
      if( KernelSize < 3) {                             // Ensure minimum value
        KernelSize = 3;
      }

      KernelHalf = KernelSize / 2;                      // Half kernel size

      XOffset -= KernelHalf;
      YOffset -= KernelHalf;
    }

    ierr = YaIPS_RGB_OverlaySub( pDst, pOverlay->pImgShadow,
                                 pOverlay->AOI.XPos + XOffset, pOverlay->AOI.YPos + YOffset,
                                 pOverlay->ShapeGen.RotAngle, pOverlay->ShapeGen.AlphaMult);

    if( ierr != 0)  {   // Have an error

      goto ExitPoint;
    }
  }

  //
  // Overlay image part
  //

  ierr = YaIPS_RGB_OverlaySub( pDst, pOverlay->pImgOverlay,
                               pOverlay->AOI.XPos, pOverlay->AOI.YPos,
                               pOverlay->ShapeGen.RotAngle, pOverlay->ShapeGen.AlphaMult);

ExitPoint:

  return( ierr);       // Return OK
}

/***************************************************************************
* YaIPS_RGB_Alpha_Replace
* Replace an alpha channel by an image
*
* pDst             Point to RGB image.
*                  Must exist and must have an alpha channel.
* ShapeType        Shape type, see #defines YAIPS_SHAPE_GEN_TYPE_XXX
* ShapeArg         Argument for shape
* CopyMode         How to copy alpha data
* AlphaMult        Alpha multiplier %, range 0.0 .. 100.0.
* AlphaOp          Alpha operator
*                  0 : Set alpha of shape
*                  1 : Set minimum of alpha in pDst image and shape
*
* return     0 OK
*          < 0 Error
****************************************************************************
*/

int YaIPS_RGB_Alpha_Replace( Fl_RGB_Image *pDst,       // Pointer to RGB image. Must exist and must have an alpha channel.
                             int ShapeType,            // Shape type, see #defines YAIPS_SHAPE_GEN_TYPE_XXX
                             float ShapeArg,           // Argument for shape
                             float AlphaMult,          // Alpha multiplier %, range 0.0 .. 100.0.
                             int AlphaOp)              // Alpha operator
{
  int ierr, x, y, xxDst, yyDst, nByteDst, AlphaMultInt, iAlpha, NewAlpha;
  Fl_RGB_Image *pImgTmp;
  YaIPS_RGB_ImgD_t iDst, iSrc;
  YaIPS_RGB_ShapeGen_Par_t ShapeGenData;
  uchar *s8, *d8;

  // Convert destination image
  ierr = YaIPS_RGB_to_ImgD( pDst, &iDst);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  // Get destination image data

  xxDst    = iDst.xx;
  yyDst    = iDst.yy;
  //x/xmDst    = iDst.ld;
  nByteDst = iDst.d;

  // Destination must have an alpha channel

  if( nByteDst == 2) {             // Black and white with alpha

    iAlpha = 1;                    // Index for alpha byte part

  } else if( nByteDst == 4) {      // Color image

    iAlpha = 3;                    // Index for alpha byte part

  } else {                         // Image has no alpha

    return( -102);                 // Has no alpha channel
  }

  // Alpha multiplier %, range 0.0 .. 100.0

  AlphaMultInt = (int)(AlphaMult * 2.56 + 0.5);
  if( AlphaMultInt <   0) AlphaMultInt = 0;
  if( AlphaMultInt > 256) AlphaMultInt = 256;

  if( ShapeType <= YAIPS_SHAPE_GEN_TYPE_NONE) {

    ierr = YaIPS_RGB_SetAlpha( &iDst, 255);

    return( ierr);
  }

  // Prepare shape generation

  memset( &ShapeGenData, 0, sizeof( ShapeGenData));    // Zero all data

  ShapeGenData.ShapeType = ShapeType;
  ShapeGenData.ShapeArg  = ShapeArg;

  // Draw a white shape image without lines
  // NOTE: no lines are drawn

  pImgTmp = NULL;    // Have no image until now
  pImgTmp = pShapeGenSub( xxDst, yyDst, &ShapeGenData, true, false);   // draw shape only

  if( pImgTmp == NULL) {                     // No image generated

    return( -120);
  }

  if( pImgTmp->d() != 3) {                   // Expect an color image here

    return( -121);
  }

  // Convert white shape to alpha

  ierr = YaIPS_RGB_to_ImgD( pImgTmp, &iSrc);
  if( ierr != 0)  {                           // Check for error
    return( ierr);
  }

  for( y = 0; y < yyDst; y++) {

    d8 = RGB_pixad( 0,  y, &iDst);
    s8 = RGB_pixad( 0,  y, &iSrc);

    switch( AlphaOp) {

    default:
    case YAIPS_ALPHA_REPLEACE_OP_SET:     // Alpha operator: Set alpha of shape

      for( x = 0; x < xxDst; x++) {

        d8[ iAlpha] = (s8[ 0] * AlphaMultInt) >> 8;    // Use this as new alpha

        d8 += nByteDst;
        s8 += 3;                 // Source is color only
      }

      break;

    case YAIPS_ALPHA_REPLEACE_OP_MIN:     // Alpha operator: Set minimum of alpha in pDst image and shape

      for( x = 0; x < xxDst; x++) {

        NewAlpha = d8[ iAlpha];
        if( NewAlpha > s8[ 0]) {
           NewAlpha = s8[ 0];
        }

        d8[ iAlpha] = (NewAlpha * AlphaMultInt) >> 8;    // Use this as new alpha

        d8 += nByteDst;
        s8 += 3;                 // Source is color only
      }

      break;
    } // end switch( AlphaOp)
  }

  pImgTmp->release();                        // Release the temporary image

  ierr = 0;                         // Return OK

  return( ierr);
}

/******************************** End Of File ********************************/


