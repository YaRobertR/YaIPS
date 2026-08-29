/****************************************************************************

  YaIPS_Utils_Calibration.cpp

  Calibration utilities

 25.03.2026 RR: First edition of this file.

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
* Data managed by this module
*/

// Global calibration variables
// NOTE: See YaIPS_GUI_Calibrate.cpp for GUI and backup for this variables

double YaIPS_Calib_UPP_X = 0.1;             // Calibration factor in X [Units/Pixel]
double YaIPS_Calib_UPP_Y = 0.1;             // Calibration factor in Y [Units/Pixel]

double YaIPS_Calib_Image_X = 0.1;           // Calibration factor in X for image mode [Units/Pixel]
double YaIPS_Calib_Image_Y = 0.1;           // Calibration factor in Y for image mode [Units/Pixel]

int YaIPS_Calib_Mode = 0;                   // Calibration mode
int YaIPS_Calib_Unit = 0;                   // Calibration unit 0 = pixel, 1 = mm, 2 = cm ...
int YaIPS_Calib_DPI = 96;                   // Calibration DPI (dots per inch)

/************************************************************************************
 * YaIPS_Calib_FixUnitsAfterPoint
 *
 * Ensure proper afterpoint digits for a unit value using a specific calibration unit.
 *
 * ValUnit         Unit value as double
 * Calib_Unit      Use this calibration unit
 *
 */

double YaIPS_Calib_FixUnitsAfterPoint( double ValUnit, int Calib_Unit)
{
  double TempDouble;

  TempDouble = ValUnit;

  switch( Calib_Unit) {

  default:                         // Units is not know
  case YAIPS_CALIB_UNIT_PIXEL:     // Units are pixel
    TempDouble = round( TempDouble);
    break;

  case YAIPS_CALIB_UNIT_MM:        // Units are mm
    TempDouble = round( TempDouble * 10.0) * 0.1;
    break;

  case YAIPS_CALIB_UNIT_CM:        // Units are cm
  case YAIPS_CALIB_UNIT_INCH:      // Units are inch
    TempDouble = round( TempDouble * 100.0) * 0.01;
    break;
  }

  return( TempDouble);
}

/************************************************************************************
 * YaIPS_Calib_FixUnitsAfterPoint
 *
 * Ensure proper afterpoint digits for a unit value using the global calibration unit.
 *
 * ValUnit         Unit value as double
 *
 */

double YaIPS_Calib_FixUnitsAfterPoint( double ValUnit)
{
  double TempDouble;

  TempDouble = YaIPS_Calib_FixUnitsAfterPoint( ValUnit, YaIPS_Calib_Unit);

  return( TempDouble);
}

/************************************************************************************
 * YaIPS_Calib_PixVal2Units
 *
 * Convert pixel value to units.
 *
 * ValPixel        Pixel value as integer
 * Calib_Unit      Use this calibration unit
 * UnitsPerPixel   Factor units per pixel
 * RoundToUnits    If true round to the after point digits. This depends from the unit.
 *
 */

double YaIPS_Calib_PixVal2Units( int ValPixel, int Calib_Unit, double UnitsPerPixel, int RoundToUnits)
{
  double TempDouble;

  TempDouble = ValPixel * UnitsPerPixel;

  if( RoundToUnits) {

    TempDouble = YaIPS_Calib_FixUnitsAfterPoint( TempDouble, Calib_Unit);
  }

  return( TempDouble);
}

/************************************************************************************
 * YaIPS_Calib_PixXVal2Units
 *
 * Convert pixel X value to units using the global calibration variables.
 *
 * ValPixel        Pixel value as integer
 * RoundToUnits    If true round to the after point digits. This depends from the unit.
 *
 */

double YaIPS_Calib_PixXVal2Units( int ValPixel, int RoundToUnits)
{
  double TempDouble;

  TempDouble = YaIPS_Calib_PixVal2Units( ValPixel, YaIPS_Calib_Unit, YaIPS_Calib_UPP_X, RoundToUnits);

  return( TempDouble);
}

/************************************************************************************
 * YaIPS_Calib_PixYVal2Units
 *
 * Convert pixel Y value to units using the global calibration variables.
 *
 * ValPixel        Pixel value as integer
 * RoundToUnits    If true round to the after point digits. This depends from the unit.
 *
 */

double YaIPS_Calib_PixYVal2Units( int ValPixel, int RoundToUnits)
{
  double TempDouble;

  TempDouble = YaIPS_Calib_PixVal2Units( ValPixel, YaIPS_Calib_Unit, YaIPS_Calib_UPP_Y, RoundToUnits);

  return( TempDouble);
}

/************************************************************************************
 * YaIPS_Calib_UnitVal2Pixel
 *
 * Convert unit value to pixels.
 *
 * ValUnit         Unit value as double
 * UnitsPerPixel   Factor units per pixel
 *
 */

int YaIPS_Calib_UnitVal2Pixel( double ValUnit, double UnitsPerPixel)
{
  int TempInt;

  TempInt = lround( ValUnit / UnitsPerPixel);

  return( TempInt);
}

/************************************************************************************
 * YaIPS_Calib_UnitXVal2Pixel
 *
 * Convert unit X value to pixel using the global calibration variables.
 *
 * ValUnit         Unit value as double
 *
 */

int YaIPS_Calib_UnitXVal2Pixel( double ValUnit)
{
  int TempInt;

  TempInt = YaIPS_Calib_UnitVal2Pixel( ValUnit, YaIPS_Calib_UPP_X);

  return( TempInt);
}

/************************************************************************************
 * YaIPS_Calib_UnitYVal2Pixel
 *
 * Convert unit X value to pixel using the global calibration variables.
 *
 * ValUnit         Unit value as double
 *
 */

int YaIPS_Calib_UnitYVal2Pixel( double ValUnit)
{
  int TempInt;

  TempInt = YaIPS_Calib_UnitVal2Pixel( ValUnit, YaIPS_Calib_UPP_Y);

  return( TempInt);
}

/************************************************************************************
 * YaIPS_Calib_Recaclc_Factors
 *
 * Recalculate calibration factors YaIPS_Calib_UPP_X and YaIPS_Calib_UPP_Y.
 * Call this after any of the calculation parameters has changed.
 *
 * Return:    1  Any of the calibration factors has changed.
 *            0  None of the factors has changed.
 */

int YaIPS_Calib_Recaclc_Factors()
{
  double UPP_X, UPP_Y;
  int RetVal;

  if( YaIPS_Calib_Unit <= YAIPS_CALIB_UNIT_PIXEL) {         // Work with pixel only

    UPP_X = 1.0;                                            // no change
    UPP_Y = 1.0;

  } else if( YaIPS_Calib_Mode == YAIPS_CALIB_MODE_IMAGE) {  // Calibration mode image

    UPP_X = YaIPS_Calib_Image_X;                            // Simply copy from last image calibration
    UPP_Y = YaIPS_Calib_Image_Y;

  } else {                                                  // Calibration mode DPI

    switch( YaIPS_Calib_Unit) {

    default:                         // Units is not know
    case YAIPS_CALIB_UNIT_PIXEL:     // Units are pixel
      // We never should come to here.
      UPP_X = 1.0;
      UPP_Y = 1.0;
      break;

    case YAIPS_CALIB_UNIT_MM:        // Units are mm
      UPP_X = (double)YAIPS_CALIB_MM_PER_INCH / YaIPS_Calib_DPI;
      UPP_Y = UPP_X;
      break;

    case YAIPS_CALIB_UNIT_CM:        // Units are cm
      UPP_X = (double)YAIPS_CALIB_CM_PER_INCH / YaIPS_Calib_DPI;
      UPP_Y = UPP_X;
      break;

    case YAIPS_CALIB_UNIT_INCH:      // Units are inch
      UPP_X = (double)1.0 / YaIPS_Calib_DPI;
      UPP_Y = UPP_X;
      break;
    }
  }

  // Check for change of settings

  RetVal = 0;                          // Preset: None of the factors has changed
  if( YaIPS_Calib_UPP_X != UPP_X ||
      YaIPS_Calib_UPP_Y != UPP_Y) {

    RetVal = 1;                        // Any of the calibration factors has changed

    YaIPS_Calib_UPP_X = UPP_X;
    YaIPS_Calib_UPP_Y = UPP_Y;
  }

  return( RetVal);
}

/************************************************************************************
 * pYaIPS_Calib_Unit2String
 *
 * Return unit string for given calibration unit.
 *
 */

char *pYaIPS_Calib_Unit2String( int Calib_Unit)
{
  char *pReturn;

  switch( Calib_Unit) {

  default:                         // Units is not know
    pReturn = LANGDEF_CALIB_UNIT_UNLNOWN;
    break;

  case YAIPS_CALIB_UNIT_PIXEL:     // Units are pixel
    pReturn = LANGDEF_CALIB_UNIT_PIXEL;
    break;

  case YAIPS_CALIB_UNIT_MM:        // Units are mm
    pReturn = LANGDEF_CALIB_UNIT_MM;
    break;

  case YAIPS_CALIB_UNIT_CM:        // Units are cm
    pReturn = LANGDEF_CALIB_UNIT_CM;
    break;

  case YAIPS_CALIB_UNIT_INCH:      // Units are inch
    pReturn = LANGDEF_CALIB_UNIT_INCH;
    break;
  }

  return( pReturn);
}

/************************************************************************************
 * pYaIPS_Calib_Unit2String
 *
 * Return unit string for global calibration unit.
 *
 */

char *pYaIPS_Calib_Unit2String()
{
  char *pReturn;

  pReturn = pYaIPS_Calib_Unit2String( YaIPS_Calib_Unit);

  return( pReturn);
}

/************************************************************************************
 * pYaIPS_Calib_PixelVal2UnitStr
 *
 * Convert pixel value to units and append the unit
 *
 */

char *pYaIPS_Calib_PixelVal2UnitStr( int ValPixel, int Calib_Unit, double UnitsPerPixel)
{
  static char TempString1[ 128];
  static char TempString2[ 128];
  static int SwitchTempString = 0;

  char *pTempString;
  double TempDouble;

  // Return pointer to one of two static temporary string buffer.
  // This enables use in sprintf() functions as argument.
  if( SwitchTempString == 0) {
    pTempString = TempString1;

    SwitchTempString = 1;
  } else {

   pTempString = TempString2;
   SwitchTempString = 0;
  }

  TempDouble = ValPixel * UnitsPerPixel;

  switch( Calib_Unit) {

  default:                         // Units is not know
  case YAIPS_CALIB_UNIT_PIXEL:     // Units are pixel
    TempDouble = round( TempDouble);
    sprintf( pTempString, "%d %s", (int)TempDouble, pYaIPS_Calib_Unit2String( Calib_Unit));
    break;

  case YAIPS_CALIB_UNIT_MM:        // Units are mm
    TempDouble = round( TempDouble * 10.0) * 0.1;
    sprintf( pTempString, "%.1f %s", TempDouble, pYaIPS_Calib_Unit2String( Calib_Unit));
    break;

  case YAIPS_CALIB_UNIT_CM:        // Units are cm
  case YAIPS_CALIB_UNIT_INCH:      // Units are inch
    TempDouble = round( TempDouble * 100.0) * 0.01;
    sprintf( pTempString, "%.2lf %s", TempDouble, pYaIPS_Calib_Unit2String( Calib_Unit));
    break;
  }

  return( pTempString);
}

/************************************************************************************
 * pYaIPS_Calib_PixelXVal2UnitStr
 *
 * Convert pixel X value to units and append the unit
 *
 */

char *pYaIPS_Calib_PixelXVal2UnitStr( int ValPixel)
{
  char *pReturn;

  pReturn = pYaIPS_Calib_PixelVal2UnitStr( ValPixel, YaIPS_Calib_Unit, YaIPS_Calib_UPP_X);

  return( pReturn);
}

/************************************************************************************
 * pYaIPS_Calib_PixelYVal2UnitStr
 *
 * Convert pixel Y value to units and append the unit
 *
 */

char *pYaIPS_Calib_PixelYVal2UnitStr( int ValPixel)
{
  char *pReturn;

  pReturn = pYaIPS_Calib_PixelVal2UnitStr( ValPixel, YaIPS_Calib_Unit, YaIPS_Calib_UPP_Y);

  return( pReturn);
}

/************************************************************************************
 * YaIPS_Calib_Change_Unit
 *
 * Modify float values by change of the unit.
 *
 * pX, pY         Float values in x and y
 * Source_Unit    Until now, this unit was used
 * Target_Unit    Convert float values to this unit
 */

void YaIPS_Calib_Change_Unit( float *pX, float *pY, int Source_Unit, int Target_Unit)
{

  if( Source_Unit == Target_Unit)  {        // Unit will not change

    return;                                 // Have nothing to recalculate
  }

  if( Target_Unit == YAIPS_CALIB_UNIT_PIXEL) {

    // Target unit is pixel and source unit is no pixel

    *pX = *pX / YaIPS_Calib_UPP_X;      // Convert unit to pixel
    *pY = *pY / YaIPS_Calib_UPP_Y;

  } else if( Source_Unit == YAIPS_CALIB_UNIT_PIXEL) {

    // Target unit is no pixel and source unit is pixel

    *pX = *pX * YaIPS_Calib_UPP_X;      // Convert pixel to unit
    *pY = *pY * YaIPS_Calib_UPP_Y;

  } else {

    // Target is no pixel and source unit is an other no pixel unit

    switch( Target_Unit) {

    default:                                    // Units is not know
    case YAIPS_CALIB_UNIT_PIXEL:                // Units are pixel

      // This should no happen

      *pX = *pX / YaIPS_Calib_UPP_X;            // Convert unit to pixel
      *pY = *pY / YaIPS_Calib_UPP_Y;
      break;

    case YAIPS_CALIB_UNIT_MM:                   // Target unit is mm

      if( Source_Unit == YAIPS_CALIB_UNIT_CM) { // Source is cm

        *pX = *pX * 10.0;                       // Convert cm to mm
        *pY = *pY * 10.0;

      } else {                                  // Source is inch

        *pX = *pX * YAIPS_CALIB_MM_PER_INCH;    // Convert inch to mm
        *pY = *pY * YAIPS_CALIB_MM_PER_INCH;
      }
      break;

    case YAIPS_CALIB_UNIT_CM:                   // Target unit is cm

      if( Source_Unit == YAIPS_CALIB_UNIT_MM) { // Source is mm

        *pX = *pX * 0.1;                        // Convert mm to cm
        *pY = *pY * 0.1;

      } else {                                  // Source is inch

        *pX = *pX * YAIPS_CALIB_CM_PER_INCH;    // Convert inch to cm
        *pY = *pY * YAIPS_CALIB_CM_PER_INCH;
      }
      break;

    case YAIPS_CALIB_UNIT_INCH:                 // Target unit is inch

      if( Source_Unit == YAIPS_CALIB_UNIT_MM) { // Source is mm

        *pX = *pX / YAIPS_CALIB_MM_PER_INCH;    // Convert inch to mm
        *pY = *pY / YAIPS_CALIB_MM_PER_INCH;

      } else {                                  // Source is cm

        *pX = *pX / YAIPS_CALIB_CM_PER_INCH;    // Convert cm to inch
        *pY = *pY / YAIPS_CALIB_CM_PER_INCH;
      }
      break;
    }
  }

  // Ensure proper target unit rounding

  switch( Target_Unit) {
  default:
  case YAIPS_CALIB_UNIT_PIXEL:         // Pixel
    *pX = round( *pX);
    *pY = round( *pY);
    break;
  case YAIPS_CALIB_UNIT_MM:            // mm
    *pX = round( *pX * 10.0) * 0.1;
    *pY = round( *pY * 10.0) * 0.1;
    break;
  case YAIPS_CALIB_UNIT_CM:            // cm
  case YAIPS_CALIB_UNIT_INCH:          // inch
    *pX = round( *pX * 100.0) * 0.01;
    *pY = round( *pY * 100.0) * 0.01;
    break;
  }
}

/************************************************************************************
 * YaIPS_Calib_SetModifyDataAndValue
 *
 * Setup two GUI input elements for use with a specific calibration unit.
 * For an X and Y pair of GUI elements the lower and higher input limits
 * and the increments for changes with the mouse wheel are set.
 * After set, the given values are set for to the GUI elements.
 *
 * pFloatXValue, pFloatYValue    Must point to a IqeFl_Float_Input input element
 * XValue, YValue                Set this values
 * IsPosition                    False: use for with and height values, set some reasonable minimum values
 *                               True: use for position values, set o as minimum values
 * Calib_Unit                    Use this calibration unit for setup
 */

void YaIPS_Calib_SetModifyDataAndValue( void *pGuiXValue,     // IqeFl_Float_Input GUI input element for X value
                                        void *pGuiYValue,     // IqeFl_Float_Input GUI input element for Y value
                                        float XValue,         // Set this X value to the X GUI element
                                        float YValue,         // Set this Y value to the Y GUI element
                                        int IsPosition,       // If != 0: is an position value, minimum is set to 0
                                        int Calib_Unit)       // Use this calibration unit
{
  IqeFl_Float_Input *pFloatXValue, *pFloatYValue;

  pFloatXValue = (IqeFl_Float_Input *)pGuiXValue;
  pFloatYValue = (IqeFl_Float_Input *)pGuiYValue;

  // New size settings

  switch( Calib_Unit) {
  default:
  case YAIPS_CALIB_UNIT_PIXEL:          // Pixel

    if( pFloatXValue->Max != YAIPS_CALIB_MAX_VAL_PIXEL) {    // max for 50 inch and 300 dpi

      pFloatXValue->SetFormat( "%.0f");
      pFloatXValue->SetModifyData( IsPosition ? 0.0 : YAIPS_CALIB_MIN_VAL_PIXEL, YAIPS_CALIB_MAX_VAL_PIXEL, 10.0, 1.0);

      pFloatYValue->SetFormat( "%.0f");
      pFloatYValue->SetModifyData( IsPosition ? 0.0 : YAIPS_CALIB_MIN_VAL_PIXEL, YAIPS_CALIB_MAX_VAL_PIXEL, 10.0, 1.0);
    }
    break;

  case YAIPS_CALIB_UNIT_MM:          // mm

    if( pFloatXValue->Max != YAIPS_CALIB_MAX_VAL_MM) {    // Not 1.2 meter

      pFloatXValue->SetFormat( "%.1f");
      pFloatXValue->SetModifyData( IsPosition ? 0.0 : YAIPS_CALIB_MIN_VAL_CM, YAIPS_CALIB_MAX_VAL_MM, 1.0, 0.1);

      pFloatYValue->SetFormat( "%.1f");
      pFloatYValue->SetModifyData( IsPosition ? 0.0 : YAIPS_CALIB_MIN_VAL_CM, YAIPS_CALIB_MAX_VAL_MM, 1.0, 0.1);
    }
    break;

  case YAIPS_CALIB_UNIT_CM:          // cm

    if( pFloatXValue->Max != YAIPS_CALIB_MAX_VAL_CM) {    // Not 1.2 meter

      pFloatXValue->SetFormat( "%.2f");
      pFloatXValue->SetModifyData( IsPosition ? 0.0 : YAIPS_CALIB_MIN_VAL_CM, YAIPS_CALIB_MAX_VAL_CM, 1.0, 0.1);

      pFloatYValue->SetFormat( "%.2f");
      pFloatYValue->SetModifyData( IsPosition ? 0.0 : YAIPS_CALIB_MIN_VAL_CM, YAIPS_CALIB_MAX_VAL_CM, 1.0, 0.1);
    }
    break;

  case YAIPS_CALIB_UNIT_INCH:          // inch

    if( pFloatXValue->Max != YAIPS_CALIB_MAX_VAL_INCH) {     // 50 inch are 1,27 cm

      pFloatXValue->SetFormat( "%.2f");
      pFloatXValue->SetModifyData( IsPosition ? 0.0 : YAIPS_CALIB_MIN_VAL_INCH, YAIPS_CALIB_MAX_VAL_INCH, 1.0, 0.1);

      pFloatYValue->SetFormat( "%.2f");
      pFloatYValue->SetModifyData( IsPosition ? 0.0 : YAIPS_CALIB_MIN_VAL_INCH, YAIPS_CALIB_MAX_VAL_INCH, 1.0, 0.1);
    }
    break;
  }

  // Set value to GUI element

  pFloatXValue->SetValue( XValue);
  pFloatYValue->SetValue( YValue);
}

/************************************************************************************
 * YaIPS_Calib_SetModifyDataAndValue
 *
 * Setup two GUI input elements for use with the global calibration unit.
 * For an X and Y pair of GUI elements the lower and higher input limits
 * and the increments for changes with the mouse wheel are set.
 * After set, the given values are set for to the GUI elements.
 *
 * pFloatXValue, pFloatYValue    Must point to a IqeFl_Float_Input input element
 * XValue, YValue                Set this values
 * IsPosition                    False: use for with and height values, set some reasonable minimum values
 *                               True: use for position values, set o as minimum values
 *
 * NOTE: Use the global calibration unit for setup.
 */

void YaIPS_Calib_SetModifyDataAndValue( void *pGuiXValue,     // IqeFl_Float_Input GUI input element for X value
                                        void *pGuiYValue,     // IqeFl_Float_Input GUI input element for Y value
                                        float XValue,         // Set this X value to the X GUI element
                                        float YValue,         // Set this Y value to the Y GUI element
                                        int IsPosition)       // If != 0: is an position value, minimum is set to 0
{

  YaIPS_Calib_SetModifyDataAndValue( pGuiXValue, pGuiYValue, XValue, YValue, IsPosition, YaIPS_Calib_Unit);
}

/****************************** End Of File ******************************/
