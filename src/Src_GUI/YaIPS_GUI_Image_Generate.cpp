/****************************************************************************

  YaIPS_GUI_Image_Generate.cpp

  Read and display videos.

  25.05.2025 RR: First edition of this file.

*****************************************************************************
*/

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sysinfoapi.h>

// Other includes
#include "YaIPS.h"

/************************************************************************************
* Defines for this source file.
*/

// Defines for windows ID
#define MY_WIN_ID     YAIPS_WIN_ID_GEN_IMAGE          // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_GEN_IMAGE         // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_GenImage_Title=New image"    // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinNewImage"              // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_GenImage_Tool    // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       YAIPS_WIN_SIZE_S1_X_MIN
#define MYWIN_SIZE_X_MAX       YAIPS_WIN_SIZE_S1_X_MAX
#define MYWIN_SIZE_X_DEFAULT   YAIPS_WIN_SIZE_S1_X_DEFAULT

#define MYWIN_SIZE_Y_MIN       YAIPS_WIN_SIZE_S1_Y_MIN
#define MYWIN_SIZE_Y_MAX       YAIPS_WIN_SIZE_S1_Y_MAX
#define MYWIN_SIZE_Y_DEFAULT   YAIPS_WIN_SIZE_S1_Y_DEFAULT

/************************************************************************************
* forwards
*/

static void close_cb( Fl_Widget *w, long int iToolData);
static void YaIPS_ToolWin_GUI_Callback( Fl_Widget *w, long int iToolData);
static void PasteImg_cb( Fl_Widget *w, long int iToolData);
static void MyWinUpdate( int iToolData, int DoEnable);
static void IqeB_GUI_ToolsMyIdleAction( void *);

//-----------------------------------------------------------------------------------
// Manage multiple tool windows
//-----------------------------------------------------------------------------------

typedef struct {

  // Parameters
  int IsOpen;                           // True if this window is open.
  void *pMyToolWin;                     // Pointer to window data ( is pointer to CLASS_WIN_TOOL)

  int MyWinPosX, MyWinPosY;             // last window position
  int MyWinSizeX, MyWinSizeY;           // last window size

  // Used for intern data management

  Fl_YaIPS_ImageDisp_t YaIPS_ImageDisp;   // Info image output

  Fl_RGB_Image *pBGndFile;              // Loaded background image file

  // Used to catch a change of the input image
  int Settings_Changed;                  // != 0 if setting has changed

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int Tab_Group_Selected;               // Number of last selected tab group.

  // Group Size

  int ImgWidth;                         // Image width in pixel
  int ImgHeight;                        // Image height in pixel
  int PixelDepth;                       // # Bytes per pixel, 1 ... 4

  int   NewUnit;                        // Change size: Calibration unit 0 = pixel, 1 = mm, 2 = cm ...
                                        // Used for NewWidth, NewHeight. Used to check of global calibration unit.
  float NewWidth;                       // Change size: width
  float NewHeight;                      // Change size: height
  int   NewPixelDepth;                  // Change size: # Bytes per pixel, 1 ... 4
  int   NewSRatioLocked;                // Change size: True if size ratio is locked
  int   NewWidthLocked;                 // Change size: width locked
  int   NewHeightLocked;                // Change size: height locked

  // Group Background

  int BGndType;           // Background type, see #defines YAIPS_SHAPE_GEN_BGND_XXX

  // YAIPS_SHAPE_GEN_BGND_COLOR
  int BGND_LT_Col_A;                    // Background Color left top corner active
  int BGND_RT_Col_A;                    // Background Color right top corner active
  int BGND_LB_Col_A;                    // Background Color left bottom corner active
  int BGND_RB_Col_A;                    // Background Color right bottom corner active

  unsigned int BGND_LT_Color;           // Background Color left top corner
  unsigned int BGND_RT_Color;           // Background Color right top corner
  unsigned int BGND_LB_Color;           // Background Color left bottom corner
  unsigned int BGND_RB_Color;           // Background Color right bottom corner

  // YAIPS_SHAPE_GEN_BGND_IMAGE
  char BGndFileName[ FILENAME_MAX]; // File name of last loaded image file. This is inclusive path and file extension.

  // Group Alpha

  // Shape drawing
  int ShapeType;                        // Shape type, see #defines YAIPS_SHAPE_GEN_TYPE_XXX
  float ShapeArg;                       // Argument for shape

  float AlphaMult;                      // Alpha multiplier: 0.0 ... 100.0.

} YaIPS_ToolData_info_t;

static int nYaIPS_ToolData_info;       // Number of image files windows open

static YaIPS_ToolData_info_t YaIPS_ToolData_info[ MY_WIN_MAX];

//-----------------------------------------------------------------------------------
// Presets for this tools window
//-----------------------------------------------------------------------------------

static T_GUI_PreferenceEntry MyPreferences[] =

{
  //
  // Tool window
  //

  // File is open
  { PREF_T_INT,    "IsOpen",          "0", &YaIPS_ToolData_info[0].IsOpen},

  // Window size

  { PREF_T_INT,    "WinSizeX",      "100", &YaIPS_ToolData_info[0].MyWinSizeX }, // NOTE: values will be clipped against MYWIN_SIZE_X_MIN / MYWIN_SIZE_Y_MIN
  { PREF_T_INT,    "WinSizeY",      "100", &YaIPS_ToolData_info[0].MyWinSizeY },

  //
  // Parameter Dialog
  //

  { PREF_T_INT,    "MyParPosX",  IQE_GUI_NO_WINPOS_X_STRING, &YaIPS_ToolData_info[0].MyParPosX }, // NOTE: values will be clipped against MYWIN_SIZE_X_MIN / MYWIN_SIZE_Y_MIN
  { PREF_T_INT,    "MyParPosY",  IQE_GUI_NO_WINPOS_Y_STRING, &YaIPS_ToolData_info[0].MyParPosY },

  // Hold last selected tab

  { PREF_T_INT,    "Group_Selected",     "0", &YaIPS_ToolData_info[0].Tab_Group_Selected},

  // Group Size

  { PREF_T_INT,          "ImgWidth",   "256", &YaIPS_ToolData_info[0].ImgWidth},
  { PREF_T_INT,         "ImgHeight",   "256", &YaIPS_ToolData_info[0].ImgHeight},
  { PREF_T_INT,         "PixelDepth",    "3", &YaIPS_ToolData_info[0].PixelDepth},

  { PREF_T_INT,           "NewUnit",     "0", &YaIPS_ToolData_info[0].NewUnit},
  { PREF_T_FLOAT,        "NewWidth",   "100", &YaIPS_ToolData_info[0].NewWidth},
  { PREF_T_FLOAT,       "NewHeight",   "100", &YaIPS_ToolData_info[0].NewHeight},
  { PREF_T_INT,     "NewPixelDepth",     "3", &YaIPS_ToolData_info[0].NewPixelDepth},
  { PREF_T_INT,   "NewSRatioLocked",     "0", &YaIPS_ToolData_info[0].NewSRatioLocked},
  { PREF_T_INT,    "NewWidthLocked",   "100", &YaIPS_ToolData_info[0].NewWidthLocked},
  { PREF_T_INT,   "NewHeightLocked",   "100", &YaIPS_ToolData_info[0].NewHeightLocked},

  // Group Background

  { PREF_T_INT,          "BGndType",     "0", &YaIPS_ToolData_info[0].BGndType},

  { PREF_T_INT,     "BGND_LT_Col_A",     "1", &YaIPS_ToolData_info[0].BGND_LT_Col_A},
  { PREF_T_INT,     "BGND_RT_Col_A",     "0", &YaIPS_ToolData_info[0].BGND_RT_Col_A},
  { PREF_T_INT,     "BGND_LB_Col_A",     "0", &YaIPS_ToolData_info[0].BGND_LB_Col_A},
  { PREF_T_INT,     "BGND_RB_Col_A",     "0", &YaIPS_ToolData_info[0].BGND_RB_Col_A},

  { PREF_T_INT,     "BGND_LT_Color",     "2", &YaIPS_ToolData_info[0].BGND_LT_Color},
  { PREF_T_INT,     "BGND_RT_Color",     "0", &YaIPS_ToolData_info[0].BGND_RT_Color},
  { PREF_T_INT,     "BGND_LB_Color",     "0", &YaIPS_ToolData_info[0].BGND_LB_Color},
  { PREF_T_INT,     "BGND_RB_Color",     "0", &YaIPS_ToolData_info[0].BGND_RB_Color},

  { PREF_T_STRING,   "BGndFileName",      "", &YaIPS_ToolData_info[0].BGndFileName, sizeof( YaIPS_ToolData_info[0].BGndFileName) - 1 },

  // Group Alpha

  { PREF_T_INT,         "ShapeType",     "0", &YaIPS_ToolData_info[0].ShapeType},
  { PREF_T_FLOAT,        "ShapeArg",     "0", &YaIPS_ToolData_info[0].ShapeArg},
  { PREF_T_FLOAT,       "AlphaMult", "100.0", &YaIPS_ToolData_info[0].AlphaMult},
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_GenImageWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp);

//-----------------------------------------------------------------------------------
// Parameter dialog
//
// This is a modal dialog. Therefore we can use global variables to hold
// info about the data.
//-----------------------------------------------------------------------------------

// ...

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolData;     // NOTE: Is used by all parameter dialog functions

static IqeFl_Tabs      *pTab_Groups;          // Point to tabulator GUI element

// Current size
static Fl_Output *pShowCurrSize;

// Change size
static Fl_Box    *pNew_Unit;
static IqeFl_Float_Input *pFloatNewWidth, *pFloatNewHeight;
static Fl_Output *pShowNewSize;
static Fl_Choice *pChoice_NewPixelDepth;
static Fl_Menu_Button  *pMButNewSize;
static Fl_Button *pNew_SizeRatio;

static Fl_Button *pButNewSizeMul2, *pButNewSizeDiv2, *pButNewSizeSwap, *pButNewApply;

// Background type
static Fl_Radio_Round_Button *pBGndType_0,  *pBGndType_1;

// Background corner colors
static Fl_Button *pButCol_LT, *pButCol_RT, *pButCol_LB, *pButCol_RB;
static IqeFl_Int_Input *pIntColR_LT, *pIntColR_RT, *pIntColR_LB, *pIntColR_RB;
static IqeFl_Int_Input *pIntColG_LT, *pIntColG_RT, *pIntColG_LB, *pIntColG_RB;
static IqeFl_Int_Input *pIntColB_LT, *pIntColB_RT, *pIntColB_LB, *pIntColB_RB;
static Fl_Check_Button *pCheckBut_LT, *pCheckBut_RT, *pCheckBut_LB, *pCheckBut_RB;

// Background image
static Fl_Input   *pBGndFileName;
static Fl_Button  *pBGndFileLoad;

// Use alpha
static Fl_Choice *pChoiceShape;
static Fl_Value_Slider *pSlider_3D_ShapeArg, *pSliderAlphaMult;
static int iCurrShape;                              // Index of current selected shape

/************************************************************************************
 * MyParWin_NewSizes_SetCurrentSize
 *
 * Update current size in pixel
 *
 */

static void MyParWin_NewSizes_SetCurrentSize()
{
  int g;
  char TempString[ 256], *pPixelDepth;

  pPixelDepth = pYaIPS_PixelDepth_to_string( pToolData->PixelDepth);

  g = GreatestcommonDivisor( pToolData->ImgWidth, pToolData->ImgHeight);

  if( g > 1 && pToolData->ImgWidth / g <= 24 && pToolData->ImgWidth / g <= 24) {

    sprintf( TempString, "%d x %d  (%d : %d), %s", pToolData->ImgWidth, pToolData->ImgHeight,
                         pToolData->ImgWidth / g, pToolData->ImgHeight / g, pPixelDepth);
  } else {

    sprintf( TempString, "%d x %d, %s", pToolData->ImgWidth, pToolData->ImgHeight, pPixelDepth);
  }

  if( strcmp( pShowCurrSize->value(), TempString) != 0) {   // New string is different

    pShowCurrSize->value( TempString);
  }
}

/************************************************************************************
 * MyParWin_NewSizes_SetResultingSize
 *
 * Set resulting size in pixel
 *
 */

static void MyParWin_NewSizes_SetResultingSize( int *pXX,  int *pYY)
{
  int NewWidthPix, NewHeightPix, g;
  char TempString[ 256];

  // New size settings

  NewWidthPix  = round( pToolData->NewWidth / YaIPS_Calib_UPP_X);
  NewHeightPix = round( pToolData->NewHeight / YaIPS_Calib_UPP_Y);

  g = GreatestcommonDivisor( NewWidthPix, NewHeightPix);

  if( g > 1 && NewWidthPix / g <= 24 && NewHeightPix / g <= 24) {

    sprintf( TempString, "%d x %d  (%d : %d)", NewWidthPix, NewHeightPix, NewWidthPix / g, NewHeightPix / g);
  } else {

    sprintf( TempString, "%d x %d", NewWidthPix, NewHeightPix);
  }

  if( strcmp( pShowNewSize->value(), TempString) != 0) {   // New string is different

    pShowNewSize->value( TempString);
  }

  if( pXX != NULL) {

    *pXX = NewWidthPix;
  }

  if( pYY != NULL) {

    *pYY = NewHeightPix;
  }
}

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{
  unsigned int TempColor;
  int TempEnable, UseShapeArg, HaveAlpha, CurrWidth, CurrHeight;
  char TempString1[ FILENAME_MAX + 128], TempString2[ 128];

  // Get last selected tab group

  pToolData->Tab_Group_Selected = pTab_Groups->GetTabGroup();

  // Check for change of global unit to last used unit
  if( pToolData->NewUnit != YaIPS_Calib_Unit) {

    // Set modify data for with, height

    strcpy( TempString1, pYaIPS_Calib_Unit2String( YaIPS_Calib_Unit));
    if( YaIPS_Calib_Unit == YAIPS_CALIB_UNIT_PIXEL) {      // Pixel unit need special attention
      strcat( TempString1, " !");
    }

    pNew_Unit->copy_label( TempString1);
    pNew_Unit->labelcolor( YaIPS_Calib_Unit == YAIPS_CALIB_UNIT_PIXEL ? FL_RED : FL_BLACK);  // Color red for pixel format

    // Modify float values by change of the unit

    YaIPS_Calib_Change_Unit( &pToolData->NewWidth, &pToolData->NewHeight, pToolData->NewUnit, YaIPS_Calib_Unit);

    pToolData->NewUnit = YaIPS_Calib_Unit;

    // Set modify data and values for with, height
    YaIPS_Calib_SetModifyDataAndValue( pFloatNewWidth, pFloatNewHeight,              // GUI input elements for X and y value
                                       pToolData->NewWidth, pToolData->NewHeight,    // Set this X and Y values to the X GUI elements
                                       false);                                       // Have size values

    if( pToolData->NewSRatioLocked) {   // Switch lock is on

      // Latch size in pixel
      MyParWin_NewSizes_SetResultingSize( &pToolData->NewWidthLocked, &pToolData->NewHeightLocked);
    }
  }

  // Update resulting new size in pixel
  MyParWin_NewSizes_SetResultingSize( &CurrWidth, &CurrHeight);

  // Update current size in pixel
  MyParWin_NewSizes_SetCurrentSize();

  // Enables for new size modify

  switch( YaIPS_Calib_Unit) {
  default:
  case YAIPS_CALIB_UNIT_PIXEL:         // Pixel
    IqeB_GUI_WidgetActivate( pButNewSizeMul2, pToolData->NewWidth *   2 <= YAIPS_CALIB_MAX_VAL_PIXEL && pToolData->NewHeight   * 2 <= YAIPS_CALIB_MAX_VAL_PIXEL);
    IqeB_GUI_WidgetActivate( pButNewSizeDiv2, pToolData->NewWidth * 0.5 >= YAIPS_CALIB_MIN_VAL_PIXEL && pToolData->NewHeight * 0.5 >= YAIPS_CALIB_MIN_VAL_PIXEL);
    break;
  case YAIPS_CALIB_UNIT_MM:            // mm
    IqeB_GUI_WidgetActivate( pButNewSizeMul2, pToolData->NewWidth *   2 <= YAIPS_CALIB_MAX_VAL_MM && pToolData->NewHeight   * 2 <= YAIPS_CALIB_MAX_VAL_MM);
    IqeB_GUI_WidgetActivate( pButNewSizeDiv2, pToolData->NewWidth * 0.5 >= YAIPS_CALIB_MIN_VAL_MM && pToolData->NewHeight * 0.5 >= YAIPS_CALIB_MIN_VAL_MM);
    break;
  case YAIPS_CALIB_UNIT_CM:            // cm
    IqeB_GUI_WidgetActivate( pButNewSizeMul2, pToolData->NewWidth *   2 <= YAIPS_CALIB_MAX_VAL_CM && pToolData->NewHeight   * 2 <= YAIPS_CALIB_MAX_VAL_CM);
    IqeB_GUI_WidgetActivate( pButNewSizeDiv2, pToolData->NewWidth * 0.5 >= YAIPS_CALIB_MIN_VAL_CM && pToolData->NewHeight * 0.5 >= YAIPS_CALIB_MIN_VAL_CM);
    break;
  case YAIPS_CALIB_UNIT_INCH:          // inch
    IqeB_GUI_WidgetActivate( pButNewSizeMul2, pToolData->NewWidth *   2 <= YAIPS_CALIB_MAX_VAL_INCH && pToolData->NewHeight   * 2 <= YAIPS_CALIB_MAX_VAL_INCH);
    IqeB_GUI_WidgetActivate( pButNewSizeDiv2, pToolData->NewWidth * 0.5 >= YAIPS_CALIB_MIN_VAL_INCH && pToolData->NewHeight * 0.5 >= YAIPS_CALIB_MIN_VAL_INCH);
    break;
  }

  // Update size ratio

  if( strcmp( pNew_SizeRatio->label(), pToolData->NewSRatioLocked ? "@+1PadlockC" : "@+1PadlockO") != 0) {
    pNew_SizeRatio->label( pToolData->NewSRatioLocked ? "@+1PadlockC" : "@+1PadlockO");
    pNew_SizeRatio->redraw();
  }

  // Update apply button

  TempEnable = pToolData->ImgWidth  != CurrWidth ||                // NOT equal realized size and pixel depth
               pToolData->ImgHeight != CurrHeight ||
               pToolData->PixelDepth != pToolData->NewPixelDepth;

  IqeB_GUI_WidgetActivate( pButNewApply, TempEnable);

  // Color of color buttons

  TempColor = pToolData->PixelDepth >= 3 ? pToolData->BGND_LT_Color : fl_rgb_color( uchar( pToolData->BGND_LT_Color >> 24));
  if( pButCol_LT->color() != TempColor) {
    pButCol_LT->color( TempColor);
    pButCol_LT->redraw();
  }
  TempColor = pToolData->PixelDepth >= 3 ? pToolData->BGND_RT_Color : fl_rgb_color( uchar( pToolData->BGND_RT_Color >> 24));
  if( pButCol_RT->color() != TempColor) {
    pButCol_RT->color( TempColor);
    pButCol_RT->redraw();
  }
  TempColor = pToolData->PixelDepth >= 3 ? pToolData->BGND_LB_Color : fl_rgb_color( uchar( pToolData->BGND_LB_Color >> 24));
  if( pButCol_LB->color() != TempColor) {
    pButCol_LB->color( TempColor);
    pButCol_LB->redraw();
  }
  TempColor = pToolData->PixelDepth >= 3 ? pToolData->BGND_RB_Color : fl_rgb_color( uchar( pToolData->BGND_RB_Color >> 24));
  if( pButCol_RB->color() != TempColor) {
    pButCol_RB->color( TempColor);
    pButCol_RB->redraw();
  }

  // Enables for color widgets

  TempEnable = pToolData->BGndType == YAIPS_SHAPE_GEN_BGND_COLOR;

  IqeB_GUI_WidgetActivate( pCheckBut_LT, TempEnable);
  IqeB_GUI_WidgetActivate( pCheckBut_RT, TempEnable);
  IqeB_GUI_WidgetActivate( pCheckBut_LB, TempEnable);
  IqeB_GUI_WidgetActivate( pCheckBut_RB, TempEnable);

  TempEnable = pToolData->BGndType == YAIPS_SHAPE_GEN_BGND_COLOR && pToolData->BGND_LT_Col_A;

  IqeB_GUI_WidgetActivate( pButCol_LT, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColR_LT, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColG_LT, TempEnable && pToolData->PixelDepth >= 3);
  IqeB_GUI_WidgetActivate( pIntColB_LT, TempEnable && pToolData->PixelDepth >= 3);

  TempEnable = pToolData->BGndType == YAIPS_SHAPE_GEN_BGND_COLOR && pToolData->BGND_RT_Col_A;

  IqeB_GUI_WidgetActivate( pButCol_RT, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColR_RT, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColG_RT, TempEnable && pToolData->PixelDepth >= 3);
  IqeB_GUI_WidgetActivate( pIntColB_RT, TempEnable && pToolData->PixelDepth >= 3);

  TempEnable = pToolData->BGndType == YAIPS_SHAPE_GEN_BGND_COLOR && pToolData->BGND_LB_Col_A;

  IqeB_GUI_WidgetActivate( pButCol_LB, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColR_LB, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColG_LB, TempEnable && pToolData->PixelDepth >= 3);
  IqeB_GUI_WidgetActivate( pIntColB_LB, TempEnable && pToolData->PixelDepth >= 3);

  TempEnable = pToolData->BGndType == YAIPS_SHAPE_GEN_BGND_COLOR && pToolData->BGND_RB_Col_A;

  IqeB_GUI_WidgetActivate( pButCol_RB, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColR_RB, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColG_RB, TempEnable && pToolData->PixelDepth >= 3);
  IqeB_GUI_WidgetActivate( pIntColB_RB, TempEnable && pToolData->PixelDepth >= 3);

  TempEnable = pToolData->BGndType == YAIPS_SHAPE_GEN_BGND_IMAGE;

  IqeB_GUI_WidgetActivate( pBGndFileName, TempEnable);
  IqeB_GUI_WidgetActivate( pBGndFileLoad, TempEnable);

  // Background image
  IqeB_FileGetFileName( pToolData->BGndFileName, TempString1, sizeof( TempString1));   // Get filename without path
  if( pToolData->pBGndFile != NULL) {                      // Have the file loaded
    sprintf( TempString2, " %d x %d, %s", pToolData->pBGndFile->data_w(), pToolData->pBGndFile->data_h(),
                                          pYaIPS_PixelDepth_to_string( pToolData->pBGndFile->d()));
    strcat( TempString1, TempString2);
  }
  if( strcmp( TempString1, pBGndFileName->value()) != 0) { // Is different form displayed file name
    pBGndFileName->value( TempString1);                    // Update displayed file name
    pBGndFileName->insert_position( 0);                    // Position to begin of text
  }

  // Enables for alpha

  HaveAlpha = pToolData->PixelDepth == 2 || pToolData->PixelDepth == 4;

  IqeB_GUI_WidgetActivate( pSliderAlphaMult, HaveAlpha && pToolData->ShapeType != YAIPS_SHAPE_GEN_TYPE_NONE); // Set item activated/inactive

  // Update shape generation ...

  IqeB_GUI_WidgetActivate( pChoiceShape, HaveAlpha);

  iCurrShape = 0;

  const Fl_Menu_Item *m;

  m = pChoiceShape->find_item_with_argument( pToolData->ShapeType);

  if( m != NULL) {                                 // Found it

    iCurrShape = pChoiceShape->find_index( m);

  } else {                                         // NOT found

    pToolData->ShapeType = 0;                      // Invalidate shape type
  }

  pChoiceShape->value( iCurrShape);

  TempEnable = HaveAlpha && pToolData->ShapeType != YAIPS_SHAPE_GEN_TYPE_NONE;   // Generate shape

  // Locate shape generation data

  int iShapeGen_List;

  iShapeGen_List = 0;

  for( int i = 0; i < nShapeGen_List; i++) {

    if( pToolData->ShapeType == ShapeGen_List[ i].Type) {

      iShapeGen_List = i;
      break;
    }
  }

  // ...

  UseShapeArg = TempEnable && ( ShapeGen_List[ iShapeGen_List].ShapeArgMin != 0 || ShapeGen_List[ iShapeGen_List].ShapeArgMax != 0);

  IqeB_GUI_WidgetActivate( pSlider_3D_ShapeArg, TempEnable && UseShapeArg);    // Set item activated/inactive

  if( UseShapeArg) {

    pSlider_3D_ShapeArg->bounds( ShapeGen_List[ iShapeGen_List].ShapeArgMin, ShapeGen_List[ iShapeGen_List].ShapeArgMax);

    if( pToolData->ShapeArg < ShapeGen_List[ iShapeGen_List].ShapeArgMin) {    // Clip minimum

      pToolData->ShapeArg = ShapeGen_List[ iShapeGen_List].ShapeArgMin;
    }

    if( pToolData->ShapeArg > ShapeGen_List[ iShapeGen_List].ShapeArgMax) {    // Clip maximum

      pToolData->ShapeArg = ShapeGen_List[ iShapeGen_List].ShapeArgMax;
    }

    pSlider_3D_ShapeArg->value( pToolData->ShapeArg);
  }

}

/************************************************************************************
 * IqeB_GUI_ToolsMyIdleAction
 */

static void IqeB_GUI_ParIdleAction( void *)
{
  unsigned int TimeTemp;
  static unsigned int TimeLastCalled_100 = 0;

  TimeTemp = GetTickCount();           // Get current time

  //

  if( pMyParWin == NULL) {     // Security test, window must exist

    return;
  }

  //
  // some timed actions (not each call)
  //

  if( TimeTemp - TimeLastCalled_100 >= 100) {  // 100 ms gone since last call

    TimeLastCalled_100 = TimeTemp;             // Remember last time called

    // some time gone, do ...

    // update window size

    //x/MyWinSizeX = pMyToolWin->w();  // update window size
    //x/MyWinSizeY = pMyToolWin->h();  // update window size

    // Periodically updates on GUI
    MyParWinUpdate();
  }

  return;
}

/************************************************************************************
 * close_Par_cb, close the parameter dialog
 *
 * pValueArg is a pointer to the widget. Set this pointer to NULL on deletion.
 */

static void close_Par_cb( Fl_Widget *w, void *pValueArg)
{

  pToolData->MyParPosX = pMyParWin->x();
  pToolData->MyParPosY = pMyParWin->y();

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::remove_idle( IqeB_GUI_ParIdleAction);      // Redraw window during idle
#endif
  Fl::remove_check( IqeB_GUI_ParIdleAction);     // Check small image size change

  // ...

  IqeB_GUI_CloseToolWindow( (void **)&pMyParWin);
}

/************************************************************************************
 * YaIPS_AlphaOp_Callback
 *
 * Alpha operator will change
 */

static void YaIPS_BGndType_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);             // get value to set

  pToolData->BGndType = Value;           // Set new value

  MyParWinUpdate();                      // update all enables

  pToolData->Settings_Changed = 1;       // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_BGndFileLoad_Callback
 *
 * Handle a load background image file request from the button
 */

static void IqeB_GUI_BGndFileLoad_Callback( Fl_Widget *w)
{
  Fl_Native_File_Chooser fc;
  Fl_RGB_Image *pTempImage;
  char FileFilter[ 1024];
  char *pFileName;
  int ierr;

  // Initialize the file chooser

  strcpy( FileFilter, LANGDEF_IMAGES);
  strcat( FileFilter, "\t*.{");
  strcat( FileFilter, YAIPS_IMAGE_FILES_READ_KNOWN);
  strcat( FileFilter, "}\n");

  fc.filter( FileFilter);

  fc.title( LANGDEF_FILE_LOAD_IMAGE);
  fc.type( Fl_Native_File_Chooser::BROWSE_FILE);  // only picks files that exist
  fc.directory( YaIPS_BrowserDirectory);          // Set browser directory
  ierr = fc.show();                               // Open file chooser dialog

  if( ierr != 0) {      // User cancelled or error

    return;
  }

  // Have a filename here

  pFileName = (char *)fc.filename();

  // load and show file

  pTempImage = YaIPS_Image_Read( pFileName);   // Try to load an image

  if( pTempImage != NULL) {                   // Got an image

    // Remember last used directory
    IqeB_FileGetPath( pFileName, YaIPS_BrowserDirectory, sizeof( YaIPS_BrowserDirectory));

    // Remember last loaded file name
    memset( pToolData->BGndFileName, 0, sizeof( pToolData->BGndFileName));
    strncpy( pToolData->BGndFileName, pFileName, sizeof( pToolData->BGndFileName) - 1);

    // Set new loaded file

    if( pToolData->pBGndFile != NULL) {   // Was a file loaded before ?

      pToolData->pBGndFile->release();    // Release data of this file

      pToolData->pBGndFile = NULL;
    }

    pToolData->pBGndFile = pTempImage;    // Set loaded file
  }

  pToolData->Settings_Changed = 1;       // Force recalculation output
}

/************************************************************************************
 * IqeB_Sizes_PresetButton_Callback
 *
 * Callback, Set one of preset sizes
 */

static void IqeB_Sizes_PresetButton_Callback( Fl_Widget *w)
{
  int xx, yy, unit;
  Fl_Menu_Button *pMenu_Button;
  float NewWidth, NewHeight;

  pMenu_Button = (Fl_Menu_Button *)w;

  // Return a pointer to the last menu item that was picked
  const Fl_Menu_Item *m = pMenu_Button->mvalue();

  if( m != NULL) {       // A menu line was selected

    xx   = ((int)(uintptr_t)(m->user_data_) >> 17) & 0x7fff;
    yy   = ((int)(uintptr_t)(m->user_data_) >>  2) & 0x7fff;
    unit = ((int)(uintptr_t)(m->user_data_)) & 0x0003;

    switch( unit) {
    default:
    case YAIPS_CALIB_UNIT_PIXEL:         // Pixel
      NewWidth  = xx;
      NewHeight = yy;
      break;
    case YAIPS_CALIB_UNIT_MM:            // mm
      NewWidth  = xx * 0.1;
      NewHeight = yy * 0.1;
      break;
    case YAIPS_CALIB_UNIT_CM:            // cm
    case YAIPS_CALIB_UNIT_INCH:          // inch
      NewWidth  = xx * 0.01;
      NewHeight = yy * 0.01;
      break;
    }

    // Modify float values by change of the unit
    YaIPS_Calib_Change_Unit( &NewWidth, &NewHeight, unit, YaIPS_Calib_Unit);

    pToolData->NewWidth  = NewWidth;
    pToolData->NewHeight = NewHeight;

    pFloatNewWidth->SetValue( pToolData->NewWidth);
    pFloatNewHeight->SetValue( pToolData->NewHeight);

    // Update resulting new size in pixel
    MyParWin_NewSizes_SetResultingSize( NULL, NULL);

    if( pToolData->NewSRatioLocked) {   // Switch lock is on

      // Latch size in pixel
      MyParWin_NewSizes_SetResultingSize( &pToolData->NewWidthLocked, &pToolData->NewHeightLocked);
    }
  }
}

/************************************************************************************
 * IqeB_NewSizes_Buttons_Callback
 *
 * Callback, one of the new size buttons was pressed
 */

static void IqeB_NewSizes_Buttons_Callback( Fl_Widget *w)
{
  int xx, yy;
  float OldWidth, NewWidth, OldHeight, NewHeight;
  Fl_Button *pButton;

  pButton = (Fl_Button *)w;

  OldWidth  = pToolData->NewWidth;
  NewWidth  = pToolData->NewWidth;
  OldHeight = pToolData->NewHeight;
  NewHeight = pToolData->NewHeight;

  if( pButton == pButNewSizeMul2)  {         // Double image sizes

    switch( YaIPS_Calib_Unit) {
    default:
    case YAIPS_CALIB_UNIT_PIXEL:         // Pixel
      if( OldWidth * 2 <= YAIPS_CALIB_MAX_VAL_PIXEL && OldHeight * 2 <= YAIPS_CALIB_MAX_VAL_PIXEL) {

        NewWidth  = OldWidth  * 2.0;
        NewHeight = OldHeight * 2.0;
      }
      break;
    case YAIPS_CALIB_UNIT_MM:            // mm
      if( OldWidth * 2 <= YAIPS_CALIB_MAX_VAL_MM && OldHeight * 2 <= YAIPS_CALIB_MAX_VAL_MM) {

        NewWidth  = OldWidth  * 2.0;
        NewHeight = OldHeight * 2.0;
      }
      break;
    case YAIPS_CALIB_UNIT_CM:            // cm
      if( OldWidth * 2 <= YAIPS_CALIB_MAX_VAL_CM && OldHeight * 2 <= YAIPS_CALIB_MAX_VAL_CM) {

        NewWidth  = OldWidth  * 2.0;
        NewHeight = OldHeight * 2.0;
      }
      break;
    case YAIPS_CALIB_UNIT_INCH:          // inch
      if( OldWidth * 2 <= YAIPS_CALIB_MAX_VAL_INCH && OldHeight * 2 <= YAIPS_CALIB_MAX_VAL_INCH) {

        NewWidth  = OldWidth  * 2.0;
        NewHeight = OldHeight * 2.0;
      }
      break;
    }

  } else if( pButton == pButNewSizeDiv2)  {  // Half image sizes

    switch( YaIPS_Calib_Unit) {
    default:
    case YAIPS_CALIB_UNIT_PIXEL:         // Pixel
      if( OldWidth * 0.5 >= YAIPS_CALIB_MIN_VAL_PIXEL && OldHeight * 0.5 >= YAIPS_CALIB_MIN_VAL_PIXEL) {

        NewWidth  = floor( OldWidth  * 0.5);
        NewHeight = floor( OldHeight * 0.5);
      }
      break;
    case YAIPS_CALIB_UNIT_MM:            // mm
      if( OldWidth * 2 >= YAIPS_CALIB_MIN_VAL_MM && OldHeight * 0.5 >= YAIPS_CALIB_MIN_VAL_MM) {

        NewWidth  = floor( OldWidth  * 0.5 * 10.0) * 0.1;  // Ensure one afterpoint digits
        NewHeight = floor( OldHeight * 0.5 * 10.0) * 0.1;
      }
      break;
    case YAIPS_CALIB_UNIT_CM:            // cm
      if( OldWidth * 2 >= YAIPS_CALIB_MIN_VAL_CM && OldHeight * 0.5 >= YAIPS_CALIB_MIN_VAL_CM) {

        NewWidth  = floor( OldWidth  * 0.5 * 100.0) * 0.01;  // Ensure two afterpoint digits
        NewHeight = floor( OldHeight * 0.5 * 100.0) * 0.01;
      }
      break;
    case YAIPS_CALIB_UNIT_INCH:          // inch
      if( OldWidth * 2 >= YAIPS_CALIB_MIN_VAL_INCH && OldHeight * 0.5 >= YAIPS_CALIB_MIN_VAL_INCH) {

        NewWidth  = floor( OldWidth  * 0.5 * 100.0) * 0.01;  // Ensure two afterpoint digits
        NewHeight = floor( OldHeight * 0.5 * 100.0) * 0.01;
      }
      break;
    }

  } else if( pButton == pButNewSizeSwap)  {     // Swap image sizes

    NewWidth  = OldHeight;
    NewHeight = OldWidth;

  } else if( pButton == pButNewApply)  {     // Apply new size

    // Get new size in pixels

    MyParWin_NewSizes_SetResultingSize( &xx, &yy);

    pToolData->ImgWidth  = xx;
    pToolData->ImgHeight = yy;

    pToolData->PixelDepth = pToolData->NewPixelDepth;

    pToolData->Settings_Changed = 1;     // Force recalculation output
  }

  // Update size changes

  if( NewWidth != OldWidth) {

    pToolData->NewWidth = NewWidth;
    pFloatNewWidth->SetValue( NewWidth);
  }

  if( NewHeight != OldHeight) {

    pToolData->NewHeight = NewHeight;
    pFloatNewHeight->SetValue( NewHeight);
  }
}

/************************************************************************************
 * IqeB_GUI_Float_SetValue_Callback
 *
 * Callback, set a float or double value
 */

static void IqeB_GUI_Float_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  void *pValue;
  float Value;
  IqeFl_Float_Input *pThis;

  pThis  = (IqeFl_Float_Input *)w;
  pValue = (float *)pValueArg;             // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pValue == NULL) {

    return;
  }

  // Set value

  Value  = pThis->GetValue();              // Get the value

  if( pThis->Min < pThis->Max) {           // Range is set

    if( Value < pThis->Min) {              // Clip minimum value

      Value = pThis->Min;
    }

    if( Value > pThis->Max) {              // Clip maximum value

      Value = pThis->Max;
    }
  }

  // Reformat number on GUI

  pThis->SetValue( Value);

  *(float *)pValue = Value;

  // Check for locked ratio

  if( pToolData->NewSRatioLocked) {        // Size ratio is locked

    int CurrWidthPix, CurrHeightPix;
    float CurrWidthFloat, CurrHeightFloat;

    // Get sizes in pixel
    MyParWin_NewSizes_SetResultingSize( &CurrWidthPix, &CurrHeightPix);

    if( pValue == &pToolData->NewWidth) {          // Changed X Size

      // update Y Size depending from radio
      CurrHeightPix = (CurrWidthPix * pToolData->NewHeightLocked + pToolData->NewWidthLocked / 2) / pToolData->NewWidthLocked;

      switch( YaIPS_Calib_Unit) {
      default:
      case YAIPS_CALIB_UNIT_PIXEL:          // Pixel

        CurrHeightFloat = CurrHeightPix;
        break;

      case YAIPS_CALIB_UNIT_MM:          // mm

        CurrHeightFloat = floor( (float)CurrHeightPix / YaIPS_Calib_UPP_Y * 10.0) * 0.1;
        break;

      case YAIPS_CALIB_UNIT_CM:          // cm

        CurrHeightFloat = floor( (float)CurrHeightPix / YaIPS_Calib_UPP_Y * 100.0) * 0.01;
        break;

      case YAIPS_CALIB_UNIT_INCH:          // inch

        CurrHeightFloat = floor( (float)CurrHeightPix / YaIPS_Calib_UPP_Y * 100.0) * 0.01;
        break;
      }

      pFloatNewHeight->SetValue( CurrHeightFloat);

      pToolData->NewHeight = CurrHeightFloat;

    } else if( pValue == &pToolData->NewHeight) {   // Changed Y Size

      // update X Size depending from radio
      CurrWidthPix = (CurrHeightPix * pToolData->NewWidthLocked + pToolData->NewHeightLocked / 2) / pToolData->NewHeightLocked;

      switch( YaIPS_Calib_Unit) {
      default:
      case YAIPS_CALIB_UNIT_PIXEL:          // Pixel

        CurrWidthFloat  = CurrWidthPix;
        break;

      case YAIPS_CALIB_UNIT_MM:          // mm

        CurrWidthFloat  = floor( (float)CurrWidthPix / YaIPS_Calib_UPP_X * 10.0) * 0.1;  // Ensure one afterpoint digits
        break;

      case YAIPS_CALIB_UNIT_CM:          // cm

        CurrWidthFloat  = floor( (float)CurrWidthPix / YaIPS_Calib_UPP_X * 100.0) * 0.01;  // Ensure two afterpoint digits
        break;

      case YAIPS_CALIB_UNIT_INCH:          // inch

        CurrWidthFloat  = floor( (float)CurrWidthPix / YaIPS_Calib_UPP_X * 100.0) * 0.01;  // Ensure two afterpoint digits
        break;
      }

      pFloatNewWidth->SetValue( CurrWidthFloat);

      pToolData->NewWidth = CurrWidthFloat;

    }
  }

  //x/pToolData->Settings_Changed = 1;                    // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_Int_SetValue_Callback
 */

static void IqeB_GUI_Int_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  int Value, *pValue, WasClipped;
  IqeFl_Int_Input *pThis;

  // ...

  pThis  = (IqeFl_Int_Input *)w;
  pValue = (int *)pValueArg;               // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pValue == NULL) {

    return;
  }

  Value = pThis->GetValue();               // get the value

  WasClipped = false;                      // Preset, value was NOT clipped

  if( pThis->Min < pThis->Max) {           // Range is set

    if( Value < pThis->Min) {              // Clip minimum value

      Value = pThis->Min;
      WasClipped = true;                   // Value was clipped
    }

    if( Value > pThis->Max) {              // Clip maximum value

      Value = pThis->Max;
      WasClipped = true;                   // Value was clipped
    }

    if( WasClipped) {                      // Value was clipped

      pThis->SetValue( Value);
    }
  }

  if( *pValue != Value) {                  // Value is different

    *pValue = Value;                       // update the variable

    pToolData->Settings_Changed = 1;     // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_Int_SetValue_ColR_Callback
 *
 * Callback change only red color component.
 */

static void IqeB_GUI_Int_SetValue_ColR_Callback( Fl_Widget *w, void *pValueArg)
{
  unsigned int Value, *pValue;
  IqeFl_Int_Input *pThis;
  uchar r,g,b;

  // ...

  pThis  = (IqeFl_Int_Input *)w;
  pValue = (unsigned int *)pValueArg;               // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pValue == NULL) {

    return;
  }

  Fl::get_color( *pValue, r, g, b);

  Value = pThis->GetValue();               // get the value

  IqeB_GUI_Int_SetValue_Callback( w, &Value);

  if( r != Value) {                        // Value has changed

    // Set new changed value
    r = Value;

    *pValue = fl_rgb_color( r, g, b);

    pToolData->Settings_Changed = 1;                    // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_Int_SetValue_ColG_Callback
 *
 * Callback change only red color component.
 */

static void IqeB_GUI_Int_SetValue_ColG_Callback( Fl_Widget *w, void *pValueArg)
{
  unsigned int Value, *pValue;
  IqeFl_Int_Input *pThis;
  uchar r,g,b;

  // ...

  pThis  = (IqeFl_Int_Input *)w;
  pValue = (unsigned int *)pValueArg;               // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pValue == NULL) {

    return;
  }

  Fl::get_color( *pValue, r, g, b);

  Value = pThis->GetValue();               // get the value

  IqeB_GUI_Int_SetValue_Callback( w, &Value);

  if( g != Value) {                        // Value has changed

    // Set new changed value
    g = Value;

    *pValue = fl_rgb_color( r, g, b);

    pToolData->Settings_Changed = 1;                    // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_Int_SetValue_ColB_Callback
 *
 * Callback change only red color component.
 */

static void IqeB_GUI_Int_SetValue_ColB_Callback( Fl_Widget *w, void *pValueArg)
{
  unsigned int Value, *pValue;
  IqeFl_Int_Input *pThis;
  uchar r,g,b;

  // ...

  pThis  = (IqeFl_Int_Input *)w;
  pValue = (unsigned int *)pValueArg;               // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pValue == NULL) {

    return;
  }

  Fl::get_color( *pValue, r, g, b);

  Value = pThis->GetValue();               // get the value

  IqeB_GUI_Int_SetValue_Callback( w, &Value);

  if( b != Value) {                        // Value has changed

    // Set new changed value
    b = Value;

    *pValue = fl_rgb_color( r, g, b);

    pToolData->Settings_Changed = 1;                // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_CBox_SetValue_Callback
 */

static void IqeB_GUI_CBox_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  int *pValue;
  Fl_Check_Button *pThis;

  pThis  = (Fl_Check_Button *)w;
  pValue = (int *)pValueArg;             // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pValue == NULL) {

    return;
  }

  // Set value

  *pValue = pThis->value();              // update the variable

  pToolData->Settings_Changed = 1;          // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_But_Color_SetValue_Callback
 */

static void IqeB_GUI_But_Color_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  unsigned int *pColor, ColorBefore;
  Fl_Button *pThis;
  uchar r,g,b;

  // Allow chooser only for color images

  if( pToolData->PixelDepth < 3) {       // No color image

    return;
  }

  // ...

  pThis  = (Fl_Button *)w;
  pColor = (unsigned int *)pValueArg;    // get pointer to associated variable
  ColorBefore = *pColor;

  *pColor = IqeB_GUI_ColorChooser( *pColor);

  if( ColorBefore != *pColor) {

    pThis->color( *pColor);
    pThis->parent()->redraw();

    // Update color RGB components
    Fl::get_color( *pColor, r, g, b);

    if( pColor == &pToolData->BGND_LT_Color) {
      pIntColR_LT->SetValue( r);
      pIntColG_LT->SetValue( g);
      pIntColB_LT->SetValue( b);
    } else if( pColor == &pToolData->BGND_RT_Color) {
      pIntColR_RT->SetValue( r);
      pIntColG_RT->SetValue( g);
      pIntColB_RT->SetValue( b);
    } else if( pColor == &pToolData->BGND_LB_Color) {
      pIntColR_LB->SetValue( r);
      pIntColG_LB->SetValue( g);
      pIntColB_LB->SetValue( b);
    } else if( pColor == &pToolData->BGND_RB_Color) {
      pIntColR_RB->SetValue( r);
      pIntColG_RB->SetValue( g);
      pIntColB_RB->SetValue( b);
    }

    pToolData->Settings_Changed = 1;          // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_Misc_SetValue_Callback
 *
 * This is usable for Fl_Valuator, Fl_Choice, Fl_Check_Button
 */

static void IqeB_GUI_Misc_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{

  if( w == NULL ||                       // security test
      pValueArg == NULL) {

    return;
  }

  if( pValueArg == &iCurrShape) {    // Changed alpha shape

    // Fl_Choice

    Fl_Choice *pThis;
    int *pValue, NewValue;
    const Fl_Menu_Item *m;

    pThis  = (Fl_Choice *)w;
    pValue = (int *)pValueArg;               // get pointer to associated variable

    NewValue = pThis->value();             // Get new value

    if( *pValue != NewValue) {             // Value is different

      *pValue = NewValue;                  // Update the variable
      if( *pValue >= 0 && *pValue < nShapeGen_List) { // Security test

        pToolData->ShapeType = ShapeGen_List[ NewValue].Type;

        // On change of shape change default argument
        pToolData->ShapeArg = ShapeGen_List[ NewValue].ShapeArgDef;
      }

      m = pThis->mvalue();
      if( m != NULL) {

        pToolData->ShapeType = (int)m->argument();
      }
    }

  } else if( pValueArg == &pToolData->NewSRatioLocked) {    // Changed size ratio

    pToolData->NewSRatioLocked = ! pToolData->NewSRatioLocked;   // Toggle flag

    pNew_SizeRatio->label( pToolData->NewSRatioLocked ? "@+1PadlockC" : "@+1PadlockO");
    pNew_SizeRatio->redraw();

    if( pToolData->NewSRatioLocked) {   // Switch lock to on

      // Latch size in pixel if just switched to on
      MyParWin_NewSizes_SetResultingSize( &pToolData->NewWidthLocked, &pToolData->NewHeightLocked);
    }
  } else if( pValueArg == &pToolData->NewPixelDepth) {    // Changed pixel depth

    // Fl_Choice

    Fl_Choice *pThis;
    int *pValue, NewValue;

    pThis  = (Fl_Choice *)w;
    pValue = (int *)pValueArg;             // get pointer to associated variable

#ifdef use_again
    NewValue = pThis->value();             // Get new value
#else

    NewValue = *pValue;                    // Preset sane value

    // Return a pointer to the last menu item that was picked
    const Fl_Menu_Item *m = pThis->mvalue();

    if( m != NULL) {                       // A menu line was selected

      NewValue = (int)(uintptr_t)(m->user_data_);
    }
#endif

    if( *pValue != NewValue) {             // Value is different

      *pValue = NewValue;                  // Update the variable

      // Update resulting new size in pixel
      MyParWin_NewSizes_SetResultingSize( NULL, NULL);
    }

  } else if( w == pSlider_3D_ShapeArg) {            // Shape deformation

    // Fl_Valuator, Fl_Slider or Fl_Value_Slider

    Fl_Valuator *pThis;
    float *pValue;

    pValue = (float *)pValueArg;             // get pointer to associated variable

    pThis  = (Fl_Valuator *)w;
    *pValue = pThis->value();               // update the variable

  } else if( pValueArg == (void *)&pToolData->AlphaMult) {

    // Fl_Valuator, Fl_Slider or Fl_Value_Slider

    Fl_Valuator *pThis;

    pThis  = (Fl_Valuator *)w;
    *(float *)pValueArg = pThis->value();               // update the variable
  }

  pToolData->Settings_Changed = 1;          // Force recalculation output
}

/************************************************************************************
 * YaIPS_GUI_ParameterWin
 *
 * Open parameter dialog
 *
 */

static void YaIPS_GUI_ParameterWin( int xLeft, int yTop, int iToolData)
{
  int xPos, yPos;

  // Get pointer to tool data

  pToolData = YaIPS_ToolData_info + iToolData;             // Point to info data

  //
  // creation of window on first call
  //

  xPos = xLeft;
  yPos = yTop;

  if( pToolData->MyParPosX != IQE_GUI_NO_WINPOS_X && pToolData->MyParPosY != IQE_GUI_NO_WINPOS_Y) { // have last window position

    xPos = pToolData->MyParPosX;
    yPos = pToolData->MyParPosY;
  }

  pMyParWin = new Fl_Window( xPos, yPos, 340 /*IQE_GUI_TOOLS_STD_WITDH*/, 221, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  //
  //  GUI things
  //

  int x1, y, yy, xx2;
  //x/int xx1, xc;
  int yGroup;
  char TempBuffer[ 256];
  uchar r,g,b;

  Fl_Check_Button *pCheckTemp;
  Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  IqeFl_Float_Input  *pFloatTemp;
  IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group;
  Fl_Button       *pTemp_Button;
  Fl_Choice       *pTemp_Choice;
  Fl_Radio_Round_Button *pRadioButTemp;
  //x/Fl_Hor_Nice_Slider    *pTemp_Slider;
  Fl_Value_Slider *pTemp_ValSlider;

  x1  = 4;
  //x/xx1 = pMyParWin->w() - 16;
  //x/xx2 = xx1 / 2;
  //x/xc  = pMyToolWin->w() / 2;          // x center
  yy  = 20;

  y = 4;

  //
  // Tabs
  //

  pTemp_Tabs = new IqeFl_Tabs( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4);
  pTemp_Tabs->selection_color( YAIPS_COLOR_SELECTION);
  pTab_Groups = pTemp_Tabs;

  y += 26;

  //
  // Group 'Size'
  //

  yGroup = y;
  x1 = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_GenImage_TabA1=Size"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_GenImage_TabA1a=Image size"));

    y += 22;
    x1  = 8;

    xx2 = pMyParWin->w() - x1 - 8;

    pShowCurrSize = new Fl_Output( x1, y, xx2, yy, LangStringLookup( "&GUI_GenImage_TabA2=Current size and pixel depth"));
    pShowCurrSize->align( FL_ALIGN_TOP_LEFT /*FL_ALIGN_LEFT*/);     // align for label
    pShowCurrSize->color( YAIPS_COLOR_RONLY_BGND);

    // Update current size in pixel
    MyParWin_NewSizes_SetCurrentSize();

    // Next line

    x1  = 4;
    y += yy + 8;

    // a separator line
    pTemp_Box = new Fl_Box( x1 + 4, y, pMyParWin->w() - x1 - 12, 2, LangStringLookup( "&GUI_GenImage_TabA10=Change size and pixel depth"));
    pTemp_Box->align( FL_ALIGN_BOTTOM | FL_ALIGN_LEFT);
    pTemp_Box->box( FL_BORDER_BOX);    //  FL_BORDER_BOX FL_DOWN_FRAME

    // Next line

    x1  = 8;
    y += 32;

    xx2 = 78;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2 - 6, yy, LangStringLookup( "&GUI_GenImage_TabA11=Width"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GenImage_TabA11a="
                                           "Width for size change."));
    pFloatTemp->align( FL_ALIGN_TOP_LEFT);     // align for label
    pFloatTemp->labelsize( 10);
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->NewWidth);
    pFloatTemp->SetModifyData( 1, 1234.0, 10.0, 1.0);  // Is modified later form MyParWinUpdateNewSizes()
    pFloatNewWidth = pFloatTemp;

    x1 += xx2 - 6;
    x1 += 2;

    pTemp_Button = new Fl_Button( x1, y, yy - 6, yy, "@+1PadlockO");
    pTemp_Button->tooltip( LangStringLookup( "&GUI_GenImage_TabA11b="
                                             "Lock size ratio."));
    pTemp_Button->labelcolor( FL_BLUE);
    pTemp_Button->box( FL_NO_BOX);
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->NewSRatioLocked);
    pNew_SizeRatio = pTemp_Button;

    x1 += yy - 6;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2 - 6, yy, LangStringLookup( "&GUI_GenImage_TabA12=Height"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GenImage_TabA12a="
                                           "Height for size change."));
    pFloatTemp->align( FL_ALIGN_TOP_LEFT);     // align for label
    pFloatTemp->labelsize( 10);
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->NewHeight);
    pFloatTemp->SetModifyData( 1, 1234.0, 10.0, 1.0);  // Is modified later form MyParWinUpdateNewSizes()
    pFloatNewHeight = pFloatTemp;

    // Set modify data and values for with, height
    YaIPS_Calib_SetModifyDataAndValue( pFloatNewWidth, pFloatNewHeight,              // GUI input elements for X and y value
                                       pToolData->NewWidth, pToolData->NewHeight,    // Set this X and Y values to the X GUI elements
                                       false,                                        // Have size values
                                       pToolData->NewUnit);                          // Use this calibration unit

    x1 += xx2 - 6;
    x1 += 4;

    xx2 = 78;

    pTemp_Box = new Fl_Box( x1, y, xx2, yy);

    strcpy( TempBuffer, pYaIPS_Calib_Unit2String( pToolData->NewUnit));
    if( pToolData->NewUnit == YAIPS_CALIB_UNIT_PIXEL) {      // Pixel unit need special attention
      strcat( TempBuffer, " !");
    }

    pTemp_Box->copy_label( TempBuffer);
    pTemp_Box->labelcolor( pToolData->NewUnit == YAIPS_CALIB_UNIT_PIXEL ? FL_RED : FL_BLACK);  // Color red for pixel format

    pTemp_Box->tooltip( LangStringLookup( "&GUI_GenImage_TabA13a="
                                          "Unit selected in calibration dialog.\n"
                                          "For preset sizes that are not in\n"
                                          "pixels, no pixels should be set\n"
                                          "in the calibration dialog!"));
    pTemp_Box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label
    pTemp_Box->box( FL_NO_BOX);
    pNew_Unit = pTemp_Box;

    // Next line

    x1  = 8;
    y += yy + 16;

    xx2 = 160;

    pShowNewSize = new Fl_Output( x1, y, xx2, yy, LangStringLookup( "&GUI_GenImage_TabA16=Resulting size [Pixel]"));
    pShowNewSize->tooltip( LangStringLookup( "&GUI_GenImage_TabA16a="
                                             "Shows the resulting size of\n"
                                             "the above settings in pixels.\n"
                                             "NOTE: is read only."));
    pShowNewSize->align( FL_ALIGN_TOP_LEFT /*FL_ALIGN_LEFT*/);     // align for label
    pShowNewSize->labelsize( 10);
    pShowNewSize->color( YAIPS_COLOR_RONLY_BGND);

    // Update resulting new size in pixel
    MyParWin_NewSizes_SetResultingSize( NULL, NULL);

    x1 += xx2;
    x1 += 4;

    xx2 = 78;

    pTemp_Choice = new Fl_Choice( x1, y, xx2, yy, LangStringLookup( "&GUI_GenImage_TabA17=Pixel depth"));
    pTemp_Choice->tooltip( LangStringLookup( "&GUI_GenImage_TabA17a="
                                             "Number of bytes per pixel:\n"
                                             " 1 black & white\n"
                                             " 2 black & white + alpha mask\n"
                                             " 3 RGB color\n"
                                             " 4 RGB color + alpha mask"));
    pTemp_Choice->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Choice->labelsize( 10);
    pTemp_Choice->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->NewPixelDepth);
    pTemp_Choice->menu_box( FL_BORDER_BOX);

    pChoice_NewPixelDepth = pTemp_Choice;
    pChoice_NewPixelDepth->add( LANGDEF_PIXDEPTH_1_TXT, 0, NULL, (void *)(fl_intptr_t)1);
    pChoice_NewPixelDepth->add( LANGDEF_PIXDEPTH_2_TXT, 0, NULL, (void *)(fl_intptr_t)2);
    pChoice_NewPixelDepth->add( LANGDEF_PIXDEPTH_3_TXT, 0, NULL, (void *)(fl_intptr_t)3);
    pChoice_NewPixelDepth->add( LANGDEF_PIXDEPTH_4_TXT, 0, NULL, (void *)(fl_intptr_t)4);
    pChoice_NewPixelDepth->value( pToolData->NewPixelDepth - 1);

    x1 += xx2;
    x1 += 4;

    pMButNewSize = new Fl_Menu_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_GenImage_TabA18=Sizes ..."));
    pMButNewSize->tooltip( LangStringLookup( "&GUI_GenImage_TabA18a="
                                              "Some typical Sizes"));
    pMButNewSize->align( FL_ALIGN_TOP_LEFT /*FL_ALIGN_LEFT*/);     // align for label
    pMButNewSize->labelsize( 10);
    pMButNewSize->callback( IqeB_Sizes_PresetButton_Callback);

#define PR_SIZE( w, h, u) ((w << 17) | (h << 2) | u)

    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18b=640 x 480"),         0, NULL, (void *)(fl_intptr_t)( PR_SIZE(  640,  480, YAIPS_CALIB_UNIT_PIXEL)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18c=800 x 600"),         0, NULL, (void *)(fl_intptr_t)( PR_SIZE(  800,  600, YAIPS_CALIB_UNIT_PIXEL)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18d=1024 x 768"),        0, NULL, (void *)(fl_intptr_t)( PR_SIZE( 1024,  768, YAIPS_CALIB_UNIT_PIXEL)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18e=1280 x 720 (HD)"),   0, NULL, (void *)(fl_intptr_t)( PR_SIZE( 1280,  720, YAIPS_CALIB_UNIT_PIXEL)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18f=1600 x 1200"),       0, NULL, (void *)(fl_intptr_t)( PR_SIZE( 1600, 1200, YAIPS_CALIB_UNIT_PIXEL)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18g=1920 x 1080 (FHD)"), 0, NULL, (void *)(fl_intptr_t)( PR_SIZE( 1920, 1080, YAIPS_CALIB_UNIT_PIXEL)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18h=3840 x 2160 (4K)"),  0, NULL, (void *)(fl_intptr_t)( PR_SIZE( 3840, 2160, YAIPS_CALIB_UNIT_PIXEL)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18DinA0=DIN/DIN A0"),    0, NULL, (void *)(fl_intptr_t)( PR_SIZE( 8410, 11890, YAIPS_CALIB_UNIT_CM)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18DinA1=DIN/DIN A1"),    0, NULL, (void *)(fl_intptr_t)( PR_SIZE( 5940, 8410, YAIPS_CALIB_UNIT_CM)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18DinA2=DIN/DIN A2"),    0, NULL, (void *)(fl_intptr_t)( PR_SIZE( 4200, 5940, YAIPS_CALIB_UNIT_CM)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18DinA3=DIN/DIN A3"),    0, NULL, (void *)(fl_intptr_t)( PR_SIZE( 2970, 4200, YAIPS_CALIB_UNIT_CM)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18DinA4=DIN/DIN A4"),    0, NULL, (void *)(fl_intptr_t)( PR_SIZE( 2100, 2970, YAIPS_CALIB_UNIT_CM)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18DinA5=DIN/DIN A5"),    0, NULL, (void *)(fl_intptr_t)( PR_SIZE( 1480, 2100, YAIPS_CALIB_UNIT_CM)));
    pMButNewSize->add( LangStringLookup( "&GUI_GenImage_TabA18DinA6=DIN/DIN A6"),    0, NULL, (void *)(fl_intptr_t)( PR_SIZE( 1050, 1480, YAIPS_CALIB_UNIT_CM)));

    // Next line

    x1  = 8;
    y += yy + 6;

    xx2 = 78;

    pButNewSizeMul2 = new Fl_Button( x1, y, xx2, yy + 4, LangStringLookup( "&GUI_GenImage_TabA20=Size * 2"));
    pButNewSizeMul2->tooltip( LangStringLookup( "&GUI_GenImage_TabA20a="
                                             "Double the image sizes."));
    pButNewSizeMul2->callback( IqeB_NewSizes_Buttons_Callback);

    x1 += xx2;
    x1 += 4;

    pButNewSizeDiv2 = new Fl_Button( x1, y, xx2, yy + 4, LangStringLookup( "&GUI_GenImage_TabA21=Size / 2"));
    pButNewSizeDiv2->tooltip( LangStringLookup( "&GUI_GenImage_TabA21a="
                                             "Halves the image sizes."));
    pButNewSizeDiv2->callback( IqeB_NewSizes_Buttons_Callback);

    x1 += xx2;
    x1 += 4;

    pButNewSizeSwap = new Fl_Button( x1, y, xx2, yy + 4, LangStringLookup( "&GUI_GenImage_TabA22=Size <->"));
    pButNewSizeSwap->tooltip( LangStringLookup( "&GUI_GenImage_TabA22a="
                                             "Swaps height and width."));
    pButNewSizeSwap->callback( IqeB_NewSizes_Buttons_Callback);

    x1 += xx2;
    x1 += 4;

    pButNewApply = new Fl_Button( x1, y, xx2, yy + 4, LANGDEF_BUTTON_APPLY);
    pButNewApply->tooltip( LangStringLookup( "&GUI_GenImage_TabA23a="
                                             "Apply the setting for a new size."));
    pButNewApply->callback( IqeB_NewSizes_Buttons_Callback);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'Background'
  //

  y = yGroup;
  x1 = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_GenImage_TabB1=Backgr."));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_GenImage_TabBa=Background of new image"));

    y += 8;

    x1  = 8;

    xx2 = pMyParWin->w() - x1 - 12;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2, yy, LANGDEF_COLOR);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GenImage_TabB2a_0="
                            "Background is color.\n"
                            "A separate color can be specified\n"
                            "for each corner of the background."));
    pRadioButTemp->callback( YaIPS_BGndType_Callback, (void *)YAIPS_SHAPE_GEN_BGND_COLOR);
    pRadioButTemp->value( pToolData->BGndType == YAIPS_SHAPE_GEN_BGND_COLOR);
    pBGndType_0 = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 2;

    //x/x1 = pMyParWin->w() / 2;
    x1 += 100;

    pTemp_Box = new Fl_Box( 4, y, x1, yy, LangStringLookup( "&GUI_GenImage_TabB10=Left upper"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    pTemp_Button = new Fl_Button( x1 + 4, y, yy * 2, yy, "");
    pTemp_Button->color( pToolData->BGND_LT_Color);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &pToolData->BGND_LT_Color);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_GenImage_TabB10a="
                           "Color for image background of left upper corner"));
    pButCol_LT = pTemp_Button;

    x1 += yy * 2 + 8;

    xx2 = 30;
    x1 += 20;

    Fl::get_color( pToolData->BGND_LT_Color, r, g, b);

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_R);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_R);
    pTemp_Int->SetValue( r);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColR_Callback, &pToolData->BGND_LT_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColR_LT = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_G);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_G);
    pTemp_Int->SetValue( g);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColG_Callback, &pToolData->BGND_LT_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColG_LT = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_B);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_B);
    pTemp_Int->SetValue( b);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColB_Callback, &pToolData->BGND_LT_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColB_LT = pTemp_Int;

    x1 += xx2;
    x1 += 6;

    pCheckTemp = new Fl_Check_Button( x1, y, 30, yy, LANGDEF_ACTIVE_SHORT);
    pCheckTemp->tooltip( LangStringLookup( "&GUI_GenImage_TabB11a="
                         "Check this to use the color for the named corner."));
    pCheckTemp->value( pToolData->BGND_LT_Col_A);
    pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->BGND_LT_Col_A);
    pCheckBut_LT = pCheckTemp;

    // Next line

    x1  = 4;
    y += yy + 6;

    //x/x1 = pMyParWin->w() / 2;
    x1 += 100;

    pTemp_Box = new Fl_Box( 4, y, x1, yy, LangStringLookup( "&GUI_GenImage_TabB12=Right upper"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    pTemp_Button = new Fl_Button( x1 + 4, y, yy * 2, yy, "");
    pTemp_Button->color( pToolData->BGND_RT_Color);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &pToolData->BGND_RT_Color);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_GenImage_TabB12a="
                           "Color for image background of right upper corner"));
    pButCol_RT = pTemp_Button;

    x1 += yy * 2 + 8;

    xx2 = 30;
    x1 += 20;

    Fl::get_color( pToolData->BGND_RT_Color, r, g, b);

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_R);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_R);
    pTemp_Int->SetValue( r);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColR_Callback, &pToolData->BGND_RT_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColR_RT = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_G);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_G);
    pTemp_Int->SetValue( g);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColG_Callback, &pToolData->BGND_RT_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColG_RT = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_B);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_B);
    pTemp_Int->SetValue( b);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColB_Callback, &pToolData->BGND_RT_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColB_RT = pTemp_Int;

    x1 += xx2;
    x1 += 6;

    pCheckTemp = new Fl_Check_Button( x1, y, 30, yy, LANGDEF_ACTIVE_SHORT);
    pCheckTemp->tooltip( LangStringLookup( "&GUI_GenImage_TabB11a="
                         "Check this to use the color for the named corner."));
    pCheckTemp->value( pToolData->BGND_RT_Col_A);
    pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->BGND_RT_Col_A);
    pCheckBut_RT = pCheckTemp;

    // Next line

    x1  = 4;
    y += yy + 6;

    //x/x1 = pMyParWin->w() / 2;
    x1 += 100;

    pTemp_Box = new Fl_Box( 4, y, x1, yy, LangStringLookup( "&GUI_GenImage_TabB13=Left bottom"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    pTemp_Button = new Fl_Button( x1 + 4, y, yy * 2, yy, "");
    pTemp_Button->color( pToolData->BGND_LB_Color);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &pToolData->BGND_LB_Color);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_GenImage_TabB13a="
                           "Color for image background of left bottom corner"));
    pButCol_LB = pTemp_Button;

    x1 += yy * 2 + 8;

    xx2 = 30;
    x1 += 20;

    Fl::get_color( pToolData->BGND_LB_Color, r, g, b);

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_R);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_R);
    pTemp_Int->SetValue( r);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColR_Callback, &pToolData->BGND_LB_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColR_LB = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_G);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_G);
    pTemp_Int->SetValue( g);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColG_Callback, &pToolData->BGND_LB_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColG_LB = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_B);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_B);
    pTemp_Int->SetValue( b);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColB_Callback, &pToolData->BGND_LB_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColB_LB = pTemp_Int;

    x1 += xx2;
    x1 += 6;

    pCheckTemp = new Fl_Check_Button( x1, y, 30, yy, LANGDEF_ACTIVE_SHORT);
    pCheckTemp->tooltip( LangStringLookup( "&GUI_GenImage_TabB11a="
                         "Check this to use the color for the named corner."));
    pCheckTemp->value( pToolData->BGND_LB_Col_A);
    pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->BGND_LB_Col_A);
    pCheckBut_LB = pCheckTemp;

    // Next line

    x1  = 4;
    y += yy + 6;

    //x/x1 = pMyParWin->w() / 2;
    x1 += 100;

    pTemp_Box = new Fl_Box( 4, y, x1, yy, LangStringLookup( "&GUI_GenImage_TabB14=Right bottom"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    pTemp_Button = new Fl_Button( x1 + 4, y, yy * 2, yy, "");
    pTemp_Button->color( pToolData->BGND_RB_Color);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &pToolData->BGND_RB_Color);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_GenImage_TabB14a="
                           "Color for image background of right bottom corner"));
    pButCol_RB = pTemp_Button;

    x1 += yy * 2 + 8;

    xx2 = 30;
    x1 += 20;

    Fl::get_color( pToolData->BGND_RB_Color, r, g, b);

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_R);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_R);
    pTemp_Int->SetValue( r);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColR_Callback, &pToolData->BGND_RB_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColR_RB = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_G);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_G);
    pTemp_Int->SetValue( g);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColG_Callback, &pToolData->BGND_RB_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColG_RB = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_B);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_B);
    pTemp_Int->SetValue( b);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColB_Callback, &pToolData->BGND_RB_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColB_RB = pTemp_Int;

    x1 += xx2;
    x1 += 6;

    pCheckTemp = new Fl_Check_Button( x1, y, 30, yy, LANGDEF_ACTIVE_SHORT);
    pCheckTemp->tooltip( LangStringLookup( "&GUI_GenImage_TabB11a="
                         "Check this to use the color for the named corner."));
    pCheckTemp->value( pToolData->BGND_RB_Col_A);
    pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->BGND_RB_Col_A);
    pCheckBut_RB = pCheckTemp;

    // Next line

    y += yy + 6;
    x1  = 8;

    xx2 = pMyParWin->w() - x1 - 12;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabA2_1=Image file"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GenImage_TabB2a_1="
                            "Background is an image file."));
    pRadioButTemp->callback( YaIPS_BGndType_Callback, (void *)YAIPS_SHAPE_GEN_BGND_IMAGE);
    pRadioButTemp->value( pToolData->BGndType == YAIPS_SHAPE_GEN_BGND_IMAGE);
    pBGndType_1 = pRadioButTemp;

    // Next line

    y += yy + 6;
    x1  = 8;

    yy = 22;

    xx2 = pMyParWin->w() - yy - 17;

    pBGndFileName = new Fl_Input( x1, y, xx2, yy, NULL);
    pBGndFileName->tooltip( LangStringLookup( "&GUI_Overlay_TabB20a="
                                              "Name of loaded image file.\n"
                                              "NOTE: is read only."));
    pBGndFileName->readonly( 1);    // Set read only. String is better visible then deactivate()
    pBGndFileName->color( YAIPS_COLOR_RONLY_BGND);

    x1 += xx2;

    pTemp_Button = new Fl_Button( x1, y, yy, yy, "...");
    pTemp_Button->callback( IqeB_GUI_BGndFileLoad_Callback);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabB21a="
                           "Load an image file."));
    pBGndFileLoad = pTemp_Button;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'Alpha'
  //

  y = yGroup;
  x1 = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_GenImage_TabC1=Alpha"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_GenImage_TabC1a=Alpha channel"));

    y += 8;
    x1  = 8;

    x1 += 70;
    xx2 = pMyParWin->w() - x1 - 8;

    pTemp_Choice = new Fl_Choice( x1, y, xx2, yy + 6, LangStringLookup( "&GUI_GenImage_TabC3=Shape:"));
    pTemp_Choice->tooltip( LangStringLookup( "&GUI_GenImage_TabC3a="
                                             "Select Shape for alpha channel."));
    pTemp_Choice->callback( IqeB_GUI_Misc_SetValue_Callback, &iCurrShape);
    pTemp_Choice->menu_box( FL_BORDER_BOX);
    pChoiceShape = pTemp_Choice;

    iCurrShape = 0;

    pTemp_Choice->add( "---");

    // Add shapes
    for( int i = 0; i < nShapeGen_List; i++) {

      pTemp_Choice->add( LangStringLookup( ShapeGen_List[ i].pName), 0, NULL, (void *)(fl_intptr_t)(ShapeGen_List[ i].Type));
    }

    x1 += xx2;

    // Next line

    x1  = 4 + 4;
    y += yy + 6 + 22;

    xx2 = pMyParWin->w() - x1 - 8;

    pTemp_ValSlider = new Fl_Value_Slider( x1, y, xx2, yy, LangStringLookup( "&GUI_GenImage_TabC4=Shape deformation"));
    pTemp_ValSlider->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_ValSlider->tooltip( LangStringLookup( "&GUI_GenImage_TabC4a=Deformation of shape."));
    pTemp_ValSlider->type( FL_HOR_SLIDER);
    pTemp_ValSlider->color( FL_LIGHT2 + 1);  // Background color
    pTemp_ValSlider->bounds( 0.0, 100.0);
    pTemp_ValSlider->step( 0.1);
    pTemp_ValSlider->value( pToolData->ShapeArg);
    pTemp_ValSlider->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->ShapeArg);
    pSlider_3D_ShapeArg = pTemp_ValSlider;

    // Next line

    x1  = 4 + 4;
    y += yy + 6 + 14;

    xx2 = pMyParWin->w() - x1 - 8;

    pTemp_ValSlider = new Fl_Value_Slider( x1, y, xx2, yy, LangStringLookup( "&GUI_GenImage_TabC10=Alpha multiplier shape"));
    pTemp_ValSlider->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_ValSlider->tooltip( LangStringLookup( "&GUI_GenImage_TabC10a="
                                             " Left side = invisible"
                                             "Right side = no alpha modification"));
    pTemp_ValSlider->type( FL_HOR_SLIDER);
    pTemp_ValSlider->color( FL_LIGHT2 + 1);  // Background color
    pTemp_ValSlider->bounds( 0.0, 100.0);
    pTemp_ValSlider->step( 0.1);
    pTemp_ValSlider->value( pToolData->AlphaMult);
    pTemp_ValSlider->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->AlphaMult);
    pSliderAlphaMult = pTemp_ValSlider;

    // Finish things for this group

    pTemp_Group->end();

  // finish up

  pTemp_Tabs->end();

  pTemp_Tabs->SetTabGroup( pToolData->Tab_Group_Selected);    // Select tab group from last session

    // finish up

  pMyParWin->end();
  pMyParWin->set_modal();
  pMyParWin->callback( close_Par_cb, &pMyParWin);
  pMyParWin->show();

  // Hack: Add close button to window caption
  YaIPS_DialogAddCloseButton( pMyParWin);

  // Add idle action for this window

  #ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::add_idle( IqeB_GUI_ParIdleAction);      // Redraw window during idle
  #endif
  Fl::add_check( IqeB_GUI_ParIdleAction);     // Check small image size change
}

//-----------------------------------------------------------------------------------
// Create a specialized window class for image load and display
//-----------------------------------------------------------------------------------

class CLASS_WIN_TOOL : public Fl_Double_Window {

public:

  int iToolData;                         // Index of info data element, see YaIPS_ToolData_info

  Fl_Button *pGUI_Img_ShowOnBig;         // Show this image on big display
  Fl_Button *pGUI_Parameter;             // Open the parameter dialog

  // Create the window

  CLASS_WIN_TOOL( int X, int Y, int W, int H, const char *l, int iToolDataArg) : Fl_Double_Window( X, Y, W, H, l)
  {
    YaIPS_ToolData_info_t *pToolData;
    Fl_Button *pTemp_Button;

    iToolData = iToolDataArg;                    // Index of info data element, see YaIPS_ToolData_info
    pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

    // Initialize some data

    memset( &pToolData->YaIPS_ImageDisp, 0, sizeof( Fl_YaIPS_ImageDisp_t)); // Zero data

    pToolData->Settings_Changed = 1;             // Force changed settings

    // ...

    Fl_Group *pGUI_GroupTopSide;                 // Top side of window
    int x, x1, y, xx, xx0, yy, hWin, wWin;

    hWin = this->h();
    wWin = this->w();

    //
    // Group, top side
    //

    x = 4;
    y = 2;

    yy = 36;                  // Top side group height

    pGUI_GroupTopSide = new Fl_Group( x, y, wWin - 8, yy);

    y += 4;

    xx0 = yy;
    //x/xx1 = yy + yy / 3;
    //x/xx2 = yy + yy / 2;
    x1 = x;

    // Show image on big display

    yy = pGUI_GroupTopSide->h();

    xx = xx0;

    pGUI_Img_ShowOnBig = new Fl_Button( x1, y, xx, yy, "@+3circle");
    pGUI_Img_ShowOnBig->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_Img_ShowOnBig->tooltip( LANGDEF_SHOW_ON_BIG_IMAGE);
    pGUI_Img_ShowOnBig->labelcolor( YAIPS_BCOL_SHOW_OTHER);

    x1 += xx + 5;

    // Open setting dialog

    xx = xx0;

    pGUI_Parameter = new Fl_Button( x1, y, xx, yy, "@+4menu2");
    pGUI_Parameter->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_Parameter->tooltip( LANGDEF_SETTINGS_POINTS);
    pGUI_Parameter->labelcolor( YAIPS_BCOL_BUTTON);
    pGUI_Parameter->shortcut( FL_COMMAND+'p');       // Short cut key

    // ...

    x1 += xx + 5;

    // Paste image from clipboard button
    // NOTE: we place the button outside the window.
    //       This makes the button invisible.
    //       The shortcut still can be used.

    pTemp_Button = new Fl_Button( x1, y - 100, xx, yy, "Paste");
    pTemp_Button->callback( PasteImg_cb, (long int)iToolData);
    pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
    pTemp_Button->shortcut( FL_COMMAND+'v');       // Short cut key

    x1 += xx + 5;

    // Copy image to clipboard button
    // NOTE: we place the button outside the window.
    //       This makes the button invisible.
    //       The shortcut still can be used.

    pTemp_Button = new Fl_Button( x1, y - 100, xx, yy, "Copy");
    pTemp_Button->callback( YaIPS_ImageDispCopyImage_cb, &pToolData->YaIPS_ImageDisp);
    pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
    pTemp_Button->shortcut( FL_COMMAND+'c');       // Short cut key
    //
    // ...
    //

    pGUI_GroupTopSide->end();             // end this group
    pGUI_GroupTopSide->resizable( 0);     // nothing in this group is resizable

    //
    // Image display box
    //

    int ImageDisp_x, ImageDisp_y, ImageDisp_xx, ImageDisp_yy;

    ImageDisp_x  = 4;
    ImageDisp_y  = pGUI_GroupTopSide->y() + pGUI_GroupTopSide->h() + 8;
    ImageDisp_xx = wWin - 8;
    ImageDisp_yy = hWin - ImageDisp_y - 4;

    // Big image display is created now

    pToolData->YaIPS_ImageDisp.DisplayResolution_Last = YAIPS_DISP_RESOLUTION_INVALID;  // Setup resolution change check

    pToolData->YaIPS_ImageDisp.pImage_Box = new YaIPS_Fl_Box( ImageDisp_x, ImageDisp_y, ImageDisp_xx, ImageDisp_yy);
    pToolData->YaIPS_ImageDisp.pImage_Box->box(FL_DOWN_BOX);
    pToolData->YaIPS_ImageDisp.pImage_Box->align( FL_ALIGN_CLIP);
    pToolData->YaIPS_ImageDisp.pImage_Box->color( YaIPS_Color_IMG_BGND);  // Background color

    pToolData->YaIPS_ImageDisp.pImage_Box->pDropCallback = NULL;        // Accept file drops
    // Use the YaIPS_GUI_MyChangeOutput() function to display a pasted image.
    // This also saves the pasted image so it is reloaded on next open of this dialog.
    pToolData->YaIPS_ImageDisp.pImage_Box->pPasteImgCallback    = YaIPS_ImageDispPasteToOutFunc_cb; // Common paste image callback
    pToolData->YaIPS_ImageDisp.pImage_Box->PasteImgCallbackArg1 = &pToolData->YaIPS_ImageDisp;  // Pointer to Fl_YaIPS_ImageDisp_t

    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawBeforeCallback = YaIPS_ImageDispDrawBefore_cb; // Draw before callback
    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawAfterCallback  = YaIPS_ImageDispDrawAfter_cb;  // Draw after callback
    pToolData->YaIPS_ImageDisp.pImage_Box->DrawCallbackArg1    = &pToolData->YaIPS_ImageDisp;  /// Pointer to Fl_YaIPS_ImageDisp_t
    pToolData->YaIPS_ImageDisp.pImage_Box->DrawCallbackArg2    = pToolData;                    // Optional pointer to ToolData

    pToolData->YaIPS_ImageDisp.MyWinID = MY_WIN_ID + iToolData;  // Remember my image ID. Is needed for paste image.

    // Layout end work

    this->end();

    this->resizable( pToolData->YaIPS_ImageDisp.pImage_Box);  // This is resizable

    this->size_range( MYWIN_SIZE_X_MIN, MYWIN_SIZE_Y_MIN, MYWIN_SIZE_X_MAX, MYWIN_SIZE_Y_MAX); // minimum window size

    // finish up

    //x/IsNotNeeded/this->end();
    this->set_non_modal();
    this->callback( close_cb, (long int)iToolData);
    this->show();

    // Hack: Remove minimize and maximize buttons from the window caption
    YaIPS_DialogRemoveMinMaxButton( this);
  }
};

/************************************************************************************
 * close_cb, close this window
 *
 * pValueArg is a pointer to the widget. Set this pointer to NULL on deletion.
 */

static void close_cb( Fl_Widget *w, long int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  int DeleteFile;
  char TempFileName[ 256 + 16];

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

  if( pToolData->pMyToolWin == NULL) {  // security test, has main window

    return;
  }

  // Release intermediate images

  if( pToolData->pBGndFile != NULL) {       // Have an intermediate image

    (pToolData->pBGndFile)->release();      // Release image data

    pToolData->pBGndFile = NULL;
  }

  // Ensure not used clipboard images are deleted

  DeleteFile = false;         // Preset no delete

  // Construct a file name for the clipboard image

  sprintf( TempFileName, "%s/Images/YaIPS/Clipboard-NewImage-%d.png", YaIPS_WorkingDirectory, (int)iToolData + 1);
  IqeB_FileNormalizePathChars( TempFileName);

  if( pToolData->BGndType != YAIPS_SHAPE_GEN_BGND_IMAGE) {

    DeleteFile = true;         // Ensure clipboard image is deleted

  } else {

    // Background is image

    // Check for clipboard image
    if( strcmp( pToolData->BGndFileName, TempFileName) != 0) {

      DeleteFile = true;        // Ensure clipboard image is deleted
    }
  }

  if( DeleteFile) {             // Check delete of clipboard image

    // If existing, delete the file
    IqeB_FileDelete( TempFileName);
  }

  // ...

  YaIPS_ImageDispReleaseBeforeClose( &pToolData->YaIPS_ImageDisp);

  pToolData->IsOpen = false;                 // Flag info data is not in use

  IqeB_GUI_CloseToolWindow( (void **)&pToolData->pMyToolWin);

  // Decrease count for closed elements in info data

  while( nYaIPS_ToolData_info > 0 &&                                         // Have any element in info data
         YaIPS_ToolData_info[ nYaIPS_ToolData_info - 1].IsOpen == false) {   // end last element is closed

    nYaIPS_ToolData_info -= 1;         // Decrease table size by one
  }

  if( nYaIPS_ToolData_info == 0) {     // No more image file window open

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
    Fl::remove_idle( IqeB_GUI_ToolsMyIdleAction);      // Redraw window during idle
#endif
    Fl::remove_check( IqeB_GUI_ToolsMyIdleAction);     // Check small image size change
  }
}

/************************************************************************************
 * ShowOnBig_cb
 *
 * Show loaded image to the big display
 */

//
static void YaIPS_ToolWin_GUI_Callback( Fl_Widget *w, long int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  CLASS_WIN_TOOL *pMyToolWin;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  // Show output image on the big display

  if( w == pMyToolWin->pGUI_Img_ShowOnBig) {                   // Show on big image

    if( pToolData->
        YaIPS_ImageDisp.pImage_Img != NULL) {                   // Got an image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                       MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName);   // Load the image to the display
    }

  } else if( w == pMyToolWin->pGUI_Parameter) {                // Open parameter dialog

    YaIPS_GUI_ParameterWin( pMyToolWin->x() + 16, pMyToolWin->y() + 16, iToolData);
  }

}

/************************************************************************************
 * PasteImg_cb
 *
 * Control + V was pressed on keyboard. Check clip board.
 */

static void PasteImg_cb( Fl_Widget *w, long int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

  if( Fl::clipboard_contains(Fl::clipboard_image) == 0) {  // NO image in the clipboard

    return;
  }

  if( pToolData->YaIPS_ImageDisp.pImage_Box == NULL) {     // Security test

    return;
  }

  // Background type must be image to accept the paste

  if( pToolData->BGndType != YAIPS_SHAPE_GEN_BGND_IMAGE) {

    return;
  }

  // ...

  Fl::paste( *pToolData->YaIPS_ImageDisp.pImage_Box, 1, Fl::clipboard_image); // try to find image in the clipboard

#ifdef use_again  // NOT needed if pPasteImgCallback = YaIPS_ImageDispPasteToOutFunc_cb
  // Set windows title

  char TempString2[ FILENAME_MAX];

#ifdef use_again
  sprintf( TempString2, "%d %s: %s", (int)iToolData + 1, MY_WIN_GUI_NAME, LANGDEF_CLIPBOARD);
  pMyToolWin->copy_label( TempString2);
#else
  sprintf( TempString2, "%s", LANGDEF_CLIPBOARD);
  YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, TempString2);
#endif
#endif

  return;
}

/************************************************************************************
 * update GUI of this tool window
 *
 * DoEnable: true   do enable GUI element if OK
 *           false  do disable all GUI elements
 *               2  do periodically updates only
 */

static void MyWinUpdate( int iToolData, int DoEnable)
{
  YaIPS_ToolData_info_t *pToolData;
  CLASS_WIN_TOOL *pMyToolWin;
  unsigned int BigImageSourceCol;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  if( pToolData->IsOpen == false) {        // Security test, must be open

    return;
  }

  if( pMyToolWin == NULL) {               // Security test

    return;
  }

  // Dis-/Enable GUI elements

  // To periodically updates first

  if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {          // Expected label color for the button

    BigImageSourceCol = YAIPS_BCOL_SHOW_THIS;
  } else {

    BigImageSourceCol = YAIPS_BCOL_SHOW_OTHER;
  }

  if( pMyToolWin->pGUI_Img_ShowOnBig->labelcolor() != BigImageSourceCol) {  // Color is different

    pMyToolWin->pGUI_Img_ShowOnBig->labelcolor( BigImageSourceCol);         // Set color

    pMyToolWin->pGUI_Img_ShowOnBig->redraw();                               // Redraw GUI element
  }

  // Keep image background color up to date. May be changed by load of a preset.

  if( YaIPS_Color_IMG_BGND != pToolData->YaIPS_ImageDisp.pImage_Box->color()) {     // Changed color for main window image backgroudn

    pToolData->YaIPS_ImageDisp.pImage_Box->color( YaIPS_Color_IMG_BGND);            // Background color
    pToolData->YaIPS_ImageDisp.pImage_Box->redraw();
  }

  if( DoEnable == 2) {                                          // Was periodically updates only

    return;
  }

  // ...

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_ShowOnBig,
                             DoEnable &&                                    // Enable GUI elements
                             pToolData->YaIPS_ImageDisp.pImage_Img != NULL); // and have an image loaded

  // ...

}

/************************************************************************************
 * IqeB_GUI_ToolsMyIdleAction
 */

static void IqeB_GUI_ToolsMyIdleAction( void *)
{
  YaIPS_ToolData_info_t *pToolData;
  CLASS_WIN_TOOL *pMyToolWin;
  int ierr, iToolData;
  unsigned int TimeTemp, TimedAction;
  static unsigned int TimeLastCalled_100 = 0;

  // Check for any elements in data info table

  if( nYaIPS_ToolData_info <= 0) {          // table is empty

    return;                                 // can return
  }

  // Wait for all tool windows to started up
  if( YaIPS_GUI_Main_Do_Startup) {  // Startup phase of tool windows

    return;
  }

  // Check for timed actions

  TimeTemp = GetTickCount();           // Get current time

  TimedAction = false;
  if( TimeTemp - TimeLastCalled_100 >= 100) {  // 100 ms gone since last call

    TimedAction = true;
    TimeLastCalled_100 = TimeTemp;             // Remember last time called
  }

  // check all open windows

  for( iToolData = 0; iToolData < nYaIPS_ToolData_info; iToolData++) {

    if( YaIPS_ToolData_info[ iToolData].IsOpen == false) {             // This window is not open

      continue;                             // Skip this element
    }

    // This window is open

    pToolData = YaIPS_ToolData_info + iToolData;               // Point to info data
    pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

    if( pMyToolWin == NULL) {                                    // Security test

      continue;
    }

    // some timed actions (not each call)

    if( TimedAction) {

      // some time gone, do ...

      // update window size

      pToolData->MyWinSizeX = pMyToolWin->w();  // update window size
      pToolData->MyWinSizeY = pMyToolWin->h();  // update window size

      // Periodically updates on GUI
      MyWinUpdate( iToolData, 2);
    }

    //
    // Do the image processing
    //

    int ForceUpdate, HaveAlpha, AlphaOp;

    ForceUpdate = false;                         // Preset: NO Force update of output image

    if( pToolData->Settings_Changed != 0) {      // Any setting has changed

      pToolData->Settings_Changed = 0;           // Change is processed

      // Do the image processing

      ierr = 0;                                              // Reset error
      errstring = NULL;                                      // Reset error string

      // Set image size and bytes per pixel

      if( pToolData->ImgWidth < 8) {                         // Security test, ensure minimum value

        pToolData->ImgWidth = 8;
      }

      if( pToolData->ImgHeight < 8) {                        // Security test, ensure minimum value

        pToolData->ImgHeight = 8;
      }

      // Ensure pixel depth is in range

      if( pToolData->PixelDepth < 1) {

        pToolData->PixelDepth = 1;
      }

      if( pToolData->PixelDepth > 4) {

        pToolData->PixelDepth = 4;
      }

      // Create image

      ierr = YaIPS_RGB_ImageSetSize( &pToolData->YaIPS_ImageDisp.pImage_Img,
                                     pToolData->ImgWidth, pToolData->ImgHeight, pToolData->PixelDepth);

      AlphaOp = YAIPS_ALPHA_REPLEACE_OP_SET;          // Preset alpha operator replace

      switch( pToolData->BGndType) {

      default:
      case YAIPS_SHAPE_GEN_BGND_COLOR:  // Background: color

        if( pToolData->pBGndFile != NULL) {        // Have a background file loaded

          (pToolData->pBGndFile)->release();       // Can release the image

          pToolData->pBGndFile = NULL;             // Flag image is released
        }

        // Set background color

        if( ierr == 0) {      // Was OK until now

          int CornerBits;

          CornerBits = 0;
          if( pToolData->BGND_LT_Col_A) CornerBits |= 0x08;
          if( pToolData->BGND_RT_Col_A) CornerBits |= 0x04;
          if( pToolData->BGND_LB_Col_A) CornerBits |= 0x02;
          if( pToolData->BGND_RB_Col_A) CornerBits |= 0x01;

          ierr = YaIPS_RGB_SetColor( pToolData->YaIPS_ImageDisp.pImage_Img,
                                     pToolData->BGND_LT_Color, pToolData->BGND_RT_Color,
                                     pToolData->BGND_LB_Color, pToolData->BGND_RB_Color,
                                     CornerBits);
        }
        break;

      case YAIPS_SHAPE_GEN_BGND_IMAGE:

        if( pToolData->pBGndFile == NULL &&         // Have no file image loaded
            pToolData->BGndFileName[ 0] != '\0') {  // and have a file name

          // load the image file file

          pToolData->pBGndFile = YaIPS_Image_Read( pToolData->BGndFileName);   // Try to load an image
        }

        if( pToolData->pBGndFile != NULL) {         // Have an image file loaded

          Fl_RGB_Image *pImgTmp;

          pImgTmp = NULL;

          ierr = YaIPS_RGB_Geo_Transform( &pImgTmp, pToolData->pBGndFile,
                                          0, 0,
                                          0.0,
                                          (float)pToolData->ImgWidth / (float)pToolData->pBGndFile->data_w(),
                                          (float)pToolData->ImgHeight / (float)pToolData->pBGndFile->data_h(),
                                          0.0, 0.0, pToolData->ImgWidth, pToolData->ImgHeight);

          if( ierr == 0 && pImgTmp != NULL) {

            YaIPS_RGB_ImgD_t iDst, iSrc;
            int x, y;
            uchar *s8, *d8;

            YaIPS_RGB_to_ImgD( pToolData->YaIPS_ImageDisp.pImage_Img, &iDst);
            YaIPS_RGB_to_ImgD( pImgTmp, &iSrc);

            if( iSrc.d == 2 || iSrc.d == 4) {          // Image file has alpha,

              AlphaOp = YAIPS_ALPHA_REPLEACE_OP_MIN;   // Set minimum alpha operator
            }

            if( iDst.xx == iSrc.xx && iDst.yy == iSrc.yy) {     // Security test, images must have the same size

              for( y = 0; y < iDst.yy; y++) {

                d8 = RGB_pixad( 0,  y, &iDst);
                s8 = RGB_pixad( 0,  y, &iSrc);

                switch( iDst.d) {

                default:                     // Pixel depth not known
                  ierr = -300;
                  break;

                case 1:                      // BW

                  // Destination is BW

                  switch( iSrc.d) {

                  default:                     // Pixel depth not known
                    ierr = -301;
                    break;

                  case 1:                      // BW
                  case 2:                      // BW + Alpha

                    for( x = 0; x < iDst.xx; x++) {

                      *d8 = *s8;

                      d8 += iDst.d;
                      s8 += iSrc.d;
                    }
                    break;

                  case 3:                      // RGB
                  case 4:                      // RGB + Alpha

                    for( x = 0; x < iDst.xx; x++) {

                      // A fast black white conversion. Intensity part of an IHS conversion.
                      *d8 = (s8[ 0] * 76 + s8[ 1] * 150 + s8[ 2] * 30) >> 8;

                      d8 += iDst.d;
                      s8 += iSrc.d;
                    }
                    break;

                  } // end switch( iSrc.d)

                  break;

                case 2:                      // BW + Alpha

                  // Destination is BW + Alpha

                  switch( iSrc.d) {

                  default:                     // Pixel depth not known
                    ierr = -301;
                    break;

                  case 1:                      // BW

                    for( x = 0; x < iDst.xx; x++) {

                      d8[ 0] = s8[ 0];
                      d8[ 1] = 255;

                      d8 += iDst.d;
                      s8 += iSrc.d;
                    }
                    break;

                  case 2:                      // BW + Alpha

                    for( x = 0; x < iDst.xx; x++) {

                      d8[ 0] = s8[ 0];
                      d8[ 1] = s8[ 1];

                      d8 += iDst.d;
                      s8 += iSrc.d;
                    }
                    break;

                  case 3:                      // RGB

                    for( x = 0; x < iDst.xx; x++) {

                      // A fast black white conversion. Intensity part of an IHS conversion.
                      d8[ 0] = (s8[ 0] * 76 + s8[ 1] * 150 + s8[ 2] * 30) >> 8;
                      d8[ 1] = 255;

                      d8 += iDst.d;
                      s8 += iSrc.d;
                    }
                    break;

                  case 4:                      // RGB + Alpha

                    for( x = 0; x < iDst.xx; x++) {

                      // A fast black white conversion. Intensity part of an IHS conversion.
                      d8[ 0] = (s8[ 0] * 76 + s8[ 1] * 150 + s8[ 2] * 30) >> 8;
                      d8[ 1] = s8[ 3];

                      d8 += iDst.d;
                      s8 += iSrc.d;
                    }
                    break;

                  } // end switch( iSrc.d)

                  break;

                case 3:                      // RGB

                  // Destination is RGB

                  switch( iSrc.d) {

                  default:                     // Pixel depth not known
                    ierr = -301;
                    break;

                  case 1:                      // BW
                  case 2:                      // BW + Alpha

                    for( x = 0; x < iDst.xx; x++) {

                      d8[ 0] = *s8;
                      d8[ 1] = *s8;
                      d8[ 2] = *s8;

                      d8 += iDst.d;
                      s8 += iSrc.d;
                    }
                    break;

                  case 3:                      // RGB
                  case 4:                      // RGB + Alpha

                    for( x = 0; x < iDst.xx; x++) {

                      d8[ 0] = s8[ 0];
                      d8[ 1] = s8[ 1];
                      d8[ 2] = s8[ 2];

                      d8 += iDst.d;
                      s8 += iSrc.d;
                    }
                    break;

                  } // end switch( iSrc.d)

                  break;

                case 4:                      // RGB + Alpha

                  // Destination is RGB + Alpha

                  switch( iSrc.d) {

                  default:                     // Pixel depth not known
                    ierr = -301;
                    break;

                  case 1:                      // BW

                    for( x = 0; x < iDst.xx; x++) {

                      d8[ 0] = s8[ 0];
                      d8[ 1] = s8[ 0];
                      d8[ 2] = s8[ 0];
                      d8[ 3] = 255;

                      d8 += iDst.d;
                      s8 += iSrc.d;
                    }
                    break;

                  case 2:                      // BW + Alpha

                    for( x = 0; x < iDst.xx; x++) {

                      d8[ 0] = s8[ 0];
                      d8[ 1] = s8[ 0];
                      d8[ 2] = s8[ 0];
                      d8[ 3] = s8[ 1];

                      d8 += iDst.d;
                      s8 += iSrc.d;
                    }
                    break;

                  case 3:                      // RGB

                    for( x = 0; x < iDst.xx; x++) {

                      d8[ 0] = s8[ 0];
                      d8[ 1] = s8[ 1];
                      d8[ 2] = s8[ 2];
                      d8[ 3] = 255;

                      d8 += iDst.d;
                      s8 += iSrc.d;
                    }
                    break;

                  case 4:                      // RGB + Alpha

                    for( x = 0; x < iDst.xx; x++) {

                      d8[ 0] = s8[ 0];
                      d8[ 1] = s8[ 1];
                      d8[ 2] = s8[ 2];
                      d8[ 3] = s8[ 3];

                      d8 += iDst.d;
                      s8 += iSrc.d;
                    }
                    break;

                  } // end switch( iSrc.d)

                  break;
                }

              }
            }
          }

          if( pImgTmp != NULL) {

            pImgTmp->release();                        // Release the temporary image
          }

        } else {

          // Preset overlay with black

          ierr = YaIPS_RGB_SetColor( pToolData->YaIPS_ImageDisp.pImage_Img, 0, 0, 0, 0, 0);                  // Color black
        }

        break;
      }

      HaveAlpha = pToolData->PixelDepth == 2 || pToolData->PixelDepth == 4;

      if( ierr == 0 &&                                            // No error until now
          HaveAlpha &&                                            // and with alpha channel
          pToolData->ShapeType != YAIPS_SHAPE_GEN_TYPE_NONE) {    // and have a shape

        ierr = YaIPS_RGB_Alpha_Replace( pToolData->YaIPS_ImageDisp.pImage_Img, pToolData->ShapeType, pToolData->ShapeArg,
                                        pToolData->AlphaMult, AlphaOp);
      }

      // Has a valid output image

      if( ierr == 0) {                                  // Have a result image

        YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, (char *)MY_WIN_GUI_NAME);

        YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, "%d x %d, %s",          // Info
                                pToolData->ImgWidth, pToolData->ImgHeight,
                                pYaIPS_PixelDepth_to_string( pToolData->PixelDepth));
        YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp); // Reset error message

        ForceUpdate = true;                        // Force update of output image

      } else {                                          // Processing error

        pToolData->Settings_Changed = 1;                // Force recalculation output

        // Empty display image
        YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, " --- "); // Info
        YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);

        if( errstring != NULL) {                           // Have a error message
          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp, errstring);
        } else {
          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp, LANGDEF_ERROR_CODE, ierr);
        }
      }
    }

    // ...

    MyWinUpdate( iToolData, true);               // Update the GUI

    // Check an image display for size change and redisplay if size has changed.
    YaIPS_ImageDispDrawUpdate( &pToolData->YaIPS_ImageDisp, ForceUpdate);

    if( ForceUpdate &&                                                // New image
        YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {   // and display this on the big image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                      MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName);   // Load the image to the display
    }
  }  // end: for( iToolData ...
}

/************************************************************************************
 * YaIPS_GUI_MyChangeOutput
 *
 * Called from outside this module.
 * Replace output image by an other image.
 *
 * SubWinIDx:        What sub window to use
 * pNewimage:        Replace output image with this window
 */

static void YaIPS_GUI_MyChangeOutput( int iToolData,                           // What sub window to use
                                      Fl_RGB_Image *pNewimage)                 // Replace output image with this window
{
  YaIPS_ToolData_info_t *pToolData;
  char TempFileName[ FILENAME_MAX + 16];

  if( pNewimage == NULL) {                                   // Security test

    return;
  }

  pToolData = YaIPS_ToolData_info + iToolData;               // Point to info data

  if( pToolData->YaIPS_ImageDisp.pImage_Box == NULL) {       // Security test

    return;
  }

  // Background type must be image to accept the paste

  if( pToolData->BGndType != YAIPS_SHAPE_GEN_BGND_IMAGE) {

    return;
  }

  // ...

  // Construct a file name for the image

  sprintf( TempFileName, "%s/Images/YaIPS/Clipboard-NewImage-%d.png", YaIPS_WorkingDirectory, iToolData + 1);

  IqeB_FileNormalizePathChars( TempFileName);

  // Save the image
  YaIPS_Image_Write_PNG( TempFileName, pNewimage);

  // Remember last loaded file name
  memset( pToolData->BGndFileName, 0, sizeof( pToolData->BGndFileName));
  strncpy( pToolData->BGndFileName, TempFileName, sizeof( pToolData->BGndFileName) - 1);

  // Set new loaded file

  if( pToolData->pBGndFile != NULL) {   // Was a file loaded before ?

    pToolData->pBGndFile->release();    // Release data of this file

    pToolData->pBGndFile = NULL;
  }

  YaIPS_RGB_CopyImg( &pToolData->pBGndFile, pNewimage);

  pToolData->Settings_Changed = 1;       // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_GenImageWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_GenImageWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  CLASS_WIN_TOOL *pMyToolWin;
  int xPos, yPos, nTabelOnEntry;
  char TempString[ 256];

  //
  // Get a free info data element
  //

  nTabelOnEntry = nYaIPS_ToolData_info;                      // Remember count of elements in info data

  if( nYaIPS_ToolData_info < iToolData + 1) {               // Catch maximum value

    nYaIPS_ToolData_info = iToolData + 1;
  }

  pToolData = YaIPS_ToolData_info + iToolData;             // Point to info data

  //
  // Prepare info data element
  //

  pToolData->IsOpen = true;           // Flag image as open

  // clip window sizes

  if( pToolData->MyWinSizeX < MYWIN_SIZE_X_MIN) pToolData->MyWinSizeX = MYWIN_SIZE_X_MIN;
  if( pToolData->MyWinSizeX > MYWIN_SIZE_X_MAX) pToolData->MyWinSizeX = MYWIN_SIZE_X_MAX;

  if( pToolData->MyWinSizeY < MYWIN_SIZE_Y_MIN) pToolData->MyWinSizeY = MYWIN_SIZE_Y_MIN;
  if( pToolData->MyWinSizeY > MYWIN_SIZE_Y_MAX) pToolData->MyWinSizeY = MYWIN_SIZE_Y_MAX;

  // Position of window

  xPos = xRight;
  yPos = yTop;

  if( pToolData->MyWinPosX != IQE_GUI_NO_WINPOS_X && pToolData->MyWinPosY != IQE_GUI_NO_WINPOS_Y) { // have last window position

    xPos = pToolData->MyWinPosX;
    yPos = pToolData->MyWinPosY;
  }

  YAIPS_BEFORE_TOOL_WIN_CREATE();  // Execute this before creation of a tool windows

  pToolData->pMyToolWin = new CLASS_WIN_TOOL( xPos, yPos, pToolData->MyWinSizeX, pToolData->MyWinSizeY, NULL, iToolData);

  if( pToolData->pMyToolWin == NULL) {   // security test

    // Window creation has failed

    pToolData->IsOpen = false;           // Flag info data is not in use

    if( iToolData == nYaIPS_ToolData_info - 1) {  // Was the last table element

      nYaIPS_ToolData_info -= 1;          // Decrease table size by one
    }

    return;
  }

  // Set window title

  pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  sprintf( TempString, "%d %s", iToolData + 1, MY_WIN_GUI_NAME);
  pMyToolWin->copy_label( TempString);

  // ...

  if( nTabelOnEntry == 0) {                         // Table was empty before

    // Add idle action for this window

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
    Fl::add_idle( IqeB_GUI_ToolsMyIdleAction);      // Redraw window during idle
#endif
    Fl::add_check( IqeB_GUI_ToolsMyIdleAction);     // Check small image size change
  }
}

/************************************************************************************
 * IqeB_GUI_GenImageWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_GenImageWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

    // Register change output function for big image display
    YaIPS_ToolChangeOutputSet( MY_WIN_ID + SubWinIDx, YaIPS_GUI_MyChangeOutput);

	  IqeB_GUI_GenImageWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

	  return;
  }

  //
  // Get a free info data element
  //

  iUnused = -1;               // No unused until now

  // Try to recycle closed info data
  for( iToolData = 0; iToolData < nYaIPS_ToolData_info; iToolData++) {

    if( YaIPS_ToolData_info[ iToolData].IsOpen == false) {

      iUnused = iToolData;                      // This one is unused
      break;
    }
  }

  if( iUnused < 0) {                             // No unused until now

    // Test window table is full

    if( nYaIPS_ToolData_info >= MY_WIN_MAX) {     // Table overflow ?

      return;
    }

    iUnused = nYaIPS_ToolData_info;
  }

  //
  // Prepare info data element
  //

  // Register change output function for big image display
  YaIPS_ToolChangeOutputSet( MY_WIN_ID + iUnused, YaIPS_GUI_MyChangeOutput);

  IqeB_GUI_GenImageWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


