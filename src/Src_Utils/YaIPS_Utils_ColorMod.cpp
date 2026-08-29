/****************************************************************************

  YaIPS_Utils_ColorMod.cpp

  Color modification utilities

 24.02.2025 RR: First edition of this file.

*****************************************************************************
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <sysinfoapi.h>

#include <iostream>
using namespace std;

#include "YaIPS.h"

/************************************************************************************
 * YaIPS_ColModInterpolateLUT
 *
 * Interpolate lookup table values
 *
 */

static void YaIPS_ColModInterpolateLUT( int (*pTable)[4],    // point to lookup table
                           int SizeTable,     // Size of table in bytes
                           uchar *pLookupR,   // Point to red lookup table ( YAIPS_LUT_N_POINTS entries)
                           uchar *pLookupG,   // Point to green lookup table ( YAIPS_LUT_N_POINTS entries)
                           uchar *pLookupB)   // Point to blue lookup table ( YAIPS_LUT_N_POINTS entries)

{
  int i, iTable, n, d1, d2;

  n = SizeTable / (sizeof( int) * 4);

  iTable = 0;

  for (i = 0; i < YAIPS_LUT_N_POINTS; i++) {

    if (i >= pTable[iTable + 1][0] && iTable < n - 2) {

      iTable += 1;
    }

    d1 = i - pTable[iTable][0];
    d2 = pTable[iTable + 1][0] - pTable[iTable][0];

    pLookupR[i] = pTable[iTable][1] + (d1 * (pTable[iTable + 1][1] - pTable[iTable][1]) + d2 / 2) / d2;
    pLookupG[i] = pTable[iTable][2] + (d1 * (pTable[iTable + 1][2] - pTable[iTable][2]) + d2 / 2) / d2;
    pLookupB[i] = pTable[iTable][3] + (d1 * (pTable[iTable + 1][3] - pTable[iTable][3]) + d2 / 2) / d2;
  }
}

/************************************************************************************
 * YaIPS_ColModCalcLUT
 *
 * Calculate lookup table
 *
 * return: 0   Have a 1:1 lookup table only
 *         1   Have a modified lookup table
 *
 */



char * YaIPS_ColMod_LUT_table[] =
{  (char *)"&Utils_ColMod_LUT_None=---",
   (char *)"&Utils_ColMod_LUT_01=Color steps",    // Farbstufen
   (char *)"&Utils_ColMod_LUT_02=Color gradient", // Farbverlauf
   (char *)"&Utils_ColMod_LUT_03=RGB",            // RGB
   (char *)"&Utils_ColMod_LUT_04=Warmth",         // Wärme
   (char *)"&Utils_ColMod_LUT_05=Fire",           // Feuer
   (char *)"&Utils_ColMod_LUT_06=Ice",            // Eis
   (char *)"&Utils_ColMod_LUT_07=Spectrum",       // Spektrum
   (char *)"&Utils_ColMod_LUT_08=Cool",           // Kühl
   (char *)"&Utils_ColMod_LUT_09=Jet",            // Jet
   (char *)"&Utils_ColMod_LUT_10=Iron",           // Eisen
   (char *)"&Utils_ColMod_LUT_11=Cold Hot",       // Kalt Heiß
   (char *)"&Utils_ColMod_LUT_12=Hue",            // Hue
};

int YaIPS_ColMod_nLUT_tables = sizeof( YaIPS_ColMod_LUT_table) / sizeof( char *);

int YaIPS_ColModCalcLUT( Fl_YaIPS_ColMod_t *pDisplayColMod, int ForceStdLut)  // Display image color modification. See YAIPS_DISP_COLMOD_xxx
{
  int i, AnyLookup;
  uchar *pLookupR;                   // Point to red lookup table ( YAIPS_LUT_N_POINTS entries)
  uchar *pLookupG;                   // Point to green lookup table ( YAIPS_LUT_N_POINTS entries)
  uchar *pLookupB;                   // Point to blue lookup table ( YAIPS_LUT_N_POINTS entries)

  // Prepare lookup table for color invert or false color

  pLookupR = pDisplayColMod->LookupR;
  pLookupG = pDisplayColMod->LookupG;
  pLookupB = pDisplayColMod->LookupB;

  // Preset no lookup modification

  AnyLookup = false;

  // Force the std LUT

  if( ForceStdLut) {

    for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {       // 1:1 lookup table

      pLookupR[ i] = i;
      pLookupG[ i] = i;
      pLookupB[ i] = i;
    }

    return( AnyLookup);
  }

  // Calculate LUT

  if( pDisplayColMod->FalseColor) {                 // False color

    AnyLookup = true;

    // If we come to here, we have a black/white lookup table.

    switch( pDisplayColMod->FalseColor) {

    default:
    case 1:                               // Color steps

      for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {

        switch (i & 0xf0) {

        case 0x00:
          pLookupR[i] = 0;
          pLookupG[i] = 0;
          pLookupB[i] = 0;
          break;

        case 0x10:
          pLookupR[i] = 85;
          pLookupG[i] = 58;
          pLookupB[i] = 58;
          break;

        case 0x20:
          pLookupR[i] = 0;
          pLookupG[i] = 0;
          pLookupB[i] = 170;
          break;

        case 0x30:
          pLookupR[i] = 85;
          pLookupG[i] = 85;
          pLookupB[i] = 255;
          break;

        case 0x40:
          pLookupR[i] = 170;
          pLookupG[i] = 0;
          pLookupB[i] = 0;
          break;

        case 0x50:
          pLookupR[i] = 255;
          pLookupG[i] = 85;
          pLookupB[i] = 85;
          break;

        case 0x60:
          pLookupR[i] = 170;
          pLookupG[i] = 0;
          pLookupB[i] = 170;
          break;

        case 0x70:
          pLookupR[i] = 255;
          pLookupG[i] = 85;
          pLookupB[i] = 255;
          break;

        case 0x80:
          pLookupR[i] = 0;
          pLookupG[i] = 170;
          pLookupB[i] = 0;
          break;

        case 0x90:
          pLookupR[i] = 85;
          pLookupG[i] = 255;
          pLookupB[i] = 85;
          break;

        case 0xA0:
          pLookupR[i] = 0;
          pLookupG[i] = 170;
          pLookupB[i] = 170;
          break;

        case 0xB0:
          pLookupR[i] = 85;
          pLookupG[i] = 255;
          pLookupB[i] = 255;
          break;

        case 0xC0:
          pLookupR[i] = 170;
          pLookupG[i] = 170;
          pLookupB[i] = 0;
          break;

        case 0xD0:
          pLookupR[i] = 255;
          pLookupG[i] = 255;
          pLookupB[i] = 85;
          break;

        case 0xE0:
          pLookupR[i] = 170;
          pLookupG[i] = 170;
          pLookupB[i] = 170;
          break;

        case 0xF0:
          pLookupR[i] = 255;
          pLookupG[i] = 255;
          pLookupB[i] = 255;
          break;
        }
      }

      break;

    case 2:
      {
        static int FC_Table[][ 4] = {      // Soft color steps
            {   0,   0,   0,   0 },
            {  16,  85,  85,  85 },
            {  32,   0,   0, 170 },
            {  48,  85,  85, 255 },
            {  64, 170,   0,   0 },
            {  80, 255,  85,  85 },
            {  96, 170,   0, 170 },
            { 112, 255,  85, 255 },
            { 128,   0, 170,   0 },
            { 144,  85, 255,  85 },
            { 160,   0, 170, 170 },
            { 176,  85, 255, 255 },
            { 192, 170, 170,   0 },
            { 208, 255, 128,   0 },
            { 224, 255, 255,   0 },
            { 240, 170, 170, 170 },
            { 255, 255, 255, 255 },
        };

        YaIPS_ColModInterpolateLUT( FC_Table, sizeof( FC_Table), pLookupR, pLookupG, pLookupB);
      }
      break;

    case 3:
      {
        static int FC_Table[][ 4] = {      // Warmth
            {   0,   0,   0,  64 },
            {  31,   0,   0, 255 },
            {  64,   0, 128, 128 },
            { 112,   0, 255,   0 },
            { 159, 255, 255,   0 },
            { 223, 255,   0,   0 },
            { 255, 255, 255, 255 },
        };

        YaIPS_ColModInterpolateLUT( FC_Table, sizeof( FC_Table), pLookupR, pLookupG, pLookupB);
      }
      break;

    case 4:
      {
        static int FC_Table[][ 4] = {  // RGB Variations 0: 3 2 1
            {   0,   0,   0,   0 },
            {  36,   0,   0, 255 },
            {  73,   0, 255,   0 },
            { 109,   0, 255, 255 },
            { 156, 255,   0,   0 },
            { 182, 255,   0, 255 },
            { 219, 255, 255,   0 },
            { 255, 255, 255, 255 },
        };

        YaIPS_ColModInterpolateLUT( FC_Table, sizeof( FC_Table), pLookupR, pLookupG, pLookupB);
      }
      break;

    case 5:
      {
        static int FC_Table[][ 4] = {      // Fire
            {   0,   0,   0,  16 },
            {  16,   1,   0,  96 },
            {  56, 122,   0, 227 },
            {  96, 195,   0,  93 },
            { 127, 238,  76,   0 },
            { 144, 255, 117,   0 },
            { 208, 234, 193,   0 },
            { 255, 255, 255, 255 },
        };

        YaIPS_ColModInterpolateLUT( FC_Table, sizeof( FC_Table), pLookupR, pLookupG, pLookupB);
      }
      break;

    case 6:
      {
        static int FC_Table[][ 4] = {      // Ice
            {   0,   0, 156, 140 },
            {  40,   0, 196, 176 },
            {  88, 112, 125, 246 },
            { 100, 146, 100, 250 },
            { 121, 203,  87, 249 },
            { 136, 229,  97, 230 },
            { 145, 243,  94, 229 },
            { 152, 250,  93, 222 },
            { 200, 250,  54, 114 },
            { 232, 251,   0,  64 },
            { 255, 255,   0,  27 },
        };

        YaIPS_ColModInterpolateLUT( FC_Table, sizeof( FC_Table), pLookupR, pLookupG, pLookupB);
      }
      break;

    case 7:
      {
        static int FC_Table[][ 4] = {      // Spectrum
            {   0, 255,   0,   0 },
            {  42, 255, 255,   0 },
            {  85,   0, 255,   0 },
            { 128,   0, 255, 255 },
            { 170,   0,   0, 255 },
            { 213, 255,   0, 255 },
            { 255, 255,   0,   0 },
        };

        YaIPS_ColModInterpolateLUT( FC_Table, sizeof( FC_Table), pLookupR, pLookupG, pLookupB);
      }
      break;

    case 8:
      {
        static int FC_Table[][ 4] = {      // Cool
            {   0,   1,   3,   1 },
            {  64,   1, 128, 127 },
            {  96,  64,  64, 191 },
            { 128, 128,   0, 255 },
            { 160, 191,  64, 128 },
            { 193, 255, 128,   0 },
            { 255, 255, 255, 246 },
        };

        YaIPS_ColModInterpolateLUT( FC_Table, sizeof( FC_Table), pLookupR, pLookupG, pLookupB);
      }
      break;

    case 9:
      {
        static int FC_Table[][ 4] = {      // Jet
            {   0,   0,   0, 131 },
            {  31,   0,   0, 255 },
            {  95,   0, 255, 255 },
            { 159, 255, 255,   0 },
            { 223, 255,   0,   0 },
            { 255, 128,   0,   0 },
        };

        YaIPS_ColModInterpolateLUT( FC_Table, sizeof( FC_Table), pLookupR, pLookupG, pLookupB);
      }
      break;

    case 10:
      {
        static int FC_Table[][ 4] = {      // Iron
            {   0,   0,   0,  64 },
            {  51,   0,   0, 196 },
            { 102, 196,   0, 196 },
            { 153, 255,   0,   0 },
            { 204, 255, 255,   0 },
            { 255, 255, 255, 255 },
        };

        YaIPS_ColModInterpolateLUT( FC_Table, sizeof( FC_Table), pLookupR, pLookupG, pLookupB);
      }
      break;

    case 11:
      {
        static int FC_Table[][ 4] = {      // old/Hot
            {   0,   0,   0, 255 },
            {  32,  32,  32,  32 },
            { 223, 255, 255, 255 },
            { 255, 255,  30,  30 },
        };

        YaIPS_ColModInterpolateLUT( FC_Table, sizeof( FC_Table), pLookupR, pLookupG, pLookupB);
      }
      break;

    case 12:
      {
        static int FC_Table[][ 4] = {      // Hue
            {   0, 255,   0, 184 },
            {  21, 255,   0, 255 },
            {  50, 255,   0,   0 },
            {  96, 255, 255,   0 },
            { 149,   0, 255,   0 },
            { 178,   0, 255, 255 },
            { 224,   0,   0, 255 },
            { 255, 255,   0, 184 },
        };

        YaIPS_ColModInterpolateLUT( FC_Table, sizeof( FC_Table), pLookupR, pLookupG, pLookupB);
      }
      break;
    }

  } else {                                          // No false color

    for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {       // 1:1 lookup table

      pLookupR[ i] = i;
      pLookupG[ i] = i;
      pLookupB[ i] = i;
    }
  }

  if( pDisplayColMod->Invert) {                     // Invert color

    AnyLookup = true;

    for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {

      pLookupR[ i] = 255 - pLookupR[ i];
      pLookupG[ i] = 255 - pLookupG[ i];
      pLookupB[ i] = 255 - pLookupB[ i];
    }
  }

  if( pDisplayColMod->Darken) {                     // Darken color

    AnyLookup = true;

    for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {

      pLookupR[ i] = (pLookupR[ i] + 1) / 2;
      pLookupG[ i] = (pLookupG[ i] + 1) / 2;
      pLookupB[ i] = (pLookupB[ i] + 1) / 2;
    }
  }

  return( AnyLookup);
}

/************************************************************************************
 * YaIPS_ColModCheckLUT
 *
 * Recalculate LUT if an update is needed.
 *
 * ForceUpdate: false: only check for LUT change and update if needed.
 *               true: always update the LUT
 *
 * return: 0   Have a 1:1 lookup table only
 *         1   Have a modified lookup table
 *
 */

void YaIPS_ColModCheckLUT( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, int ForceUpdate)
{
  int AnyLookup, ForceStdLut;

  ForceStdLut = (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_DO_DISP_MODIFY) == 0; // force Std LUT

  if( ForceUpdate ||                                                  // Force update of LUT
      (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_LUT_CHANGED) != 0) {  // LUT has changed and need and must be recalculated

    pYaIPS_ImageDisp->Flags &= ~YAIPS_IDISP_FLAG_LUT_CHANGED;    // Reset need for recalculation

    AnyLookup = YaIPS_ColModCalcLUT( &pYaIPS_ImageDisp->DisplayColMod, ForceStdLut);

    if( AnyLookup) {

      pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_LUT_ACTIVE;     // Set LUT is active, no 1:1 LUT used

    } else {

      pYaIPS_ImageDisp->Flags &= ~YAIPS_IDISP_FLAG_LUT_ACTIVE;    // Set LUT is active, 1:1 LUT used
    }
  }
}

/************************************************************************************
 * YaIPS_ColMod_LUT_Draw
 *
 * Draw LUT.
 *
 */
void YaIPS_ColMod_LUT_Draw( YaIPS_Fl_Box *pYaIPS_Box,          // Draw into this box widget
                           Fl_YaIPS_ColMod_t *pDisplayColMod,  // Pointer to LUT data
                           int RefX, int RefY,                 // Left upper reference point for drawing
                           int DrawBackground)                 // If true, draw a background
{
  int i, x1, y1, x2, x3, y2, y3;
  uchar *pLUTdata;
  int LUTs_are_different, LineWidth;

  if( pYaIPS_Box == NULL) {                   // Security test

    return;
  }

  if( pYaIPS_Box->w() < YAIPS_LUT_N_POINTS + 14 ||         // Security test minimum size
      pYaIPS_Box->h() < YAIPS_LUT_N_POINTS + 26) {

    return;
  }

   fl_push_clip( pYaIPS_Box->x(), pYaIPS_Box->y(), pYaIPS_Box->w(), pYaIPS_Box->h());

  // Draw a LUT

  //x/Fl_Color ColLine = FL_BLACK, ColText = FL_BLACK, ColR = FL_RED, ColG = FL_GREEN - 2, ColB = FL_BLUE + 2, ColBW = FL_DARK3, ColBW = FL_WHITE;
  Fl_Color ColLine = FL_DARK1 + 1, ColText = FL_WHITE;
  Fl_Color ColR = fl_rgb_color( 255, 0, 0), ColG = fl_rgb_color( 0, 255, 0), ColB = fl_rgb_color( 0, 188, 255);
  Fl_Color ColBGnd = fl_rgb_color( 128, 128, 128);
  static char MyLineDashes[] = { 4, 4, 0};

  x1 = pYaIPS_Box->x() + RefX;
  y1 = pYaIPS_Box->y() + RefY;

  x2 = x1 + YAIPS_LUT_N_POINTS - 1;
  y2 = y1 + YAIPS_LUT_N_POINTS - 1;

  if( DrawBackground) {

    fl_color( ColBGnd);

    fl_rectf( x1 - 7, y1 - 4, x2 - x1 + 12, y2 - y1 + 22);
  }

  // Draw false color bar

  for( i = 0; i < 256; i++) {

    fl_color( fl_rgb_color( pDisplayColMod->LookupR[ i],
                            pDisplayColMod->LookupG[ i],
                            pDisplayColMod->LookupB[ i]));

    fl_xyline( x1 - 7, y2 - i, x1 - 3);
  }

  // Horizontal axis

  fl_color( ColLine);

  fl_line( x1 - 1, y2 + 1, x2, y2 + 1);
  fl_line( x2 - 4, y2 + 1 - 3, x2, y2 + 1);
  fl_line( x2 - 4, y2 + 1 + 3, x2, y2 + 1);

  // Vertical axis
  fl_line( x1 - 1, y1, x1 - 1, y2);
  fl_line( x1 - 1, y1, x1 - 1 - 4, y1 + 3);
  fl_line( x1 - 1, y1, x1 - 1 + 4, y1 + 3);

  // Draw lines
  fl_line_style( FL_DOT, 0, MyLineDashes);
  fl_font( FL_HELVETICA, 14);

  // Horizontal lines

  fl_color( ColText);

  x3 = x1;
  fl_draw( "0", x3 + -1, y2 + 16);

  x3 = x1 + 64;
  fl_draw( "64", x3 - 8, y2 + 16);

  x3 = x1 + 128;
  fl_draw( "128", x3 - 12, y2 + 16);

  x3 = x1 + 196;
  fl_draw( "196", x3 - 12, y2 + 16);

  x3 = x1 + YAIPS_LUT_N_POINTS - 1;
  fl_draw( "255", x3 - 22, y2 + 16);

  fl_color( ColLine);

  x3 = x1 + 64;
  fl_line( x3, y1, x3, y2);

  x3 = x1 + 128;
  fl_line( x3, y1, x3, y2);

  x3 = x1 + 196;
  fl_line( x3, y1, x3, y2);

  x3 = x1 + YAIPS_LUT_N_POINTS - 1;
  fl_line( x3, y1, x3, y2);

  // Vertical lines

  fl_color( ColText);

  //x/y3 = y2;
  //x/fl_draw( "0", x1 + 4, y3 + 16);

  y3 = y2 - 64;
  fl_draw( "64", x1 + 4, y3 + 16);

  y3 = y2 - 128;
  fl_draw( "128", x1 + 4, y3 + 16);

  y3 = y2 - 196;
  fl_draw( "196", x1 + 4, y3 + 16);

  y3 = y2 - YAIPS_LUT_N_POINTS - 1;
  fl_draw( "255", x1 + 4, y3 + 16);

  fl_color( ColLine);

  y3 = y2 - 64;
  fl_line( x2, y3, x1, y3);

  y3 = y2 - 128;
  fl_line( x2, y3, x1, y3);

  y3 = y2 - 196;
  fl_line( x2, y3, x1, y3);

  y3 = y2 - YAIPS_LUT_N_POINTS + 1;
  fl_line( x2, y3, x1, y3);

  fl_line_style( 0);   // Reset to default

  // Check that the 3 LUTs are different

  LUTs_are_different = false;

  for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {

    if( pDisplayColMod->LookupR[ i] !=  pDisplayColMod->LookupG[ i] ||
        pDisplayColMod->LookupR[ i] !=  pDisplayColMod->LookupB[ i]) {

      LUTs_are_different = true;
      break;
    }
  }

  // Draw LUTs

  LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

  fl_line_style( 0, LineWidth);   // Set line width

  // Draw green curve or LUTs are not different

  pLUTdata = pDisplayColMod->LookupR;

  fl_color( LUTs_are_different ? ColR : FL_YELLOW);

  fl_begin_line();
  for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {

    y3 = y2 - pLUTdata[ i];

    fl_vertex( x1 + i, y3);
  }
  fl_end_line();

  if( LUTs_are_different) {

    // Draw green curve

    pLUTdata = pDisplayColMod->LookupG;

    fl_color( ColG);

    fl_begin_line();
    for( i = 0; i < YAIPS_LUT_N_POINTS; i++) {

      y3 = y2 - pLUTdata[ i];

      fl_vertex( x1 + i, y3);
    }
    fl_end_line();

    // Draw blue curve

    pLUTdata = pDisplayColMod->LookupB;

    fl_color( ColB);

    fl_begin_line();
    for( i = 0; i < YAIPS_HISTO_N_POINTS; i++) {

      y3 = y2 - pLUTdata[ i];

      fl_vertex( x1 + i, y3);
    }
    fl_end_line();
  }

//x/ExitPoint:

  fl_line_style( 0);   // Reset to default
  fl_pop_clip();
}

/****************************** End Of File ******************************/
