/****************************************************************************

  YaIPS_GUI_Combine.cpp

  Combine two images

  25.04.2025 RR: First edition of this file.

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

#define SHADING_MAX_AVERAGE_COUNT           16   // Maximum shading average count
#define SHADING_GAUSS_FILER_KERNEL          15   // Kernel size of gauss filter

// Defines for windows ID
#define MY_WIN_ID     YAIPS_WIN_ID_COMBINE        // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_COMBINE       // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_Combine_Title=Calculation"        // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinCombine"           // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_Combine_Tool  // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       221 //x/ YAIPS_WIN_SIZE_S1_X_MIN
#define MYWIN_SIZE_X_MAX       YAIPS_WIN_SIZE_S1_X_MAX
#define MYWIN_SIZE_X_DEFAULT   YAIPS_WIN_SIZE_S1_X_DEFAULT

#define MYWIN_SIZE_Y_MIN       YAIPS_WIN_SIZE_S1_Y_MIN
#define MYWIN_SIZE_Y_MAX       YAIPS_WIN_SIZE_S1_Y_MAX
#define MYWIN_SIZE_Y_DEFAULT   YAIPS_WIN_SIZE_S1_Y_DEFAULT


#define YAIPS_FILTER_FORMAT_MULT      "%.1f"     // Format string for multiplier

/************************************************************************************
* forwards
*/

static void close_cb( Fl_Widget *w, long int iToolData);
static void YaIPS_ToolWin_GUI_Callback( Fl_Widget *w, long int iToolData);
static void MyWinUpdate( int iToolData, int DoEnable);
static void IqeB_GUI_ToolsMyIdleAction( void *);

/************************************************************************************
* Global variables for this window
*/

//-----------------------------------------------------------------------------------
// Manage multiple tool windows
//-----------------------------------------------------------------------------------

typedef struct {

  // Parameters
  int iToolData;                        // Sub window number
  int IsOpen;                           // True if this window is open.
  void *pMyToolWin;                     // Pointer to window data ( is pointer to CLASS_WIN_TOOL)

  int MyWinPosX, MyWinPosY;             // last window position
  int MyWinSizeX, MyWinSizeY;           // last window size

  int Input1_WinIdNr;                   // Window ID nr of 1. input
  int Input2_WinIdNr;                   // Window ID nr of 2. input

  // Used for intern data management

  Fl_YaIPS_ImageDisp_t YaIPS_ImageDisp;   // Info image output

  // Used to catch a change of the input image
  int Input1_Change;                    // Last processed 'ImageChanged' from 1. input image
  int Input2_Change;                    // Last processed 'ImageChanged' from 2. input image
  int FadeImg_Change;                   // Last processed 'ImageChanged' from fade input image

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int Tab_Group_Selected;               // Number of last selected tab group.
  int CombineType;                      // What combine operation to use
  int Alpha_Op;                         // Alpha operator
  float CombineResMult;                 // Combine: Result multiplier
  int CombineOffset;                    // Combine: Add Offset to output. Range is -256 ... 256.
  float WAddMult1;                      // Weighted add: 1. multiplier
  float WAddMult2;                      // Weighted add: 2. multiplier
  int  WAddOffset;                      // Weighted add: Add Offset to output. Range is -256 ... 256.
  int  ShadingRefBright;                // Shading: Reference brightness. 128 .. 255.
  int  ShadingAverageCount;             // Shading: Number of averaged images
  float Fade;                           // Fade: 0.0 ... 1.0. Fade between two images
  int  FadeImg_WinIdNr;                 // Window ID nr used for fading with image
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

  // ...

  { PREF_T_INT,    "Input1_WinIdNr", "-1", &YaIPS_ToolData_info[0].Input1_WinIdNr },

  { PREF_T_INT,    "Input2_WinIdNr", "-1", &YaIPS_ToolData_info[0].Input2_WinIdNr },

  //
  // Parameter Dialog
  //

  { PREF_T_INT,    "MyParPosX",  IQE_GUI_NO_WINPOS_X_STRING, &YaIPS_ToolData_info[0].MyParPosX }, // NOTE: values will be clipped against MYWIN_SIZE_X_MIN / MYWIN_SIZE_Y_MIN
  { PREF_T_INT,    "MyParPosY",  IQE_GUI_NO_WINPOS_Y_STRING, &YaIPS_ToolData_info[0].MyParPosY },

  // Hold last selected tab

  { PREF_T_INT,      "Group_Selected",     "0", &YaIPS_ToolData_info[0].Tab_Group_Selected},

  { PREF_T_INT,         "CombineType",     "0", &YaIPS_ToolData_info[0].CombineType},
  { PREF_T_INT,            "Alpha_Op",     "4", &YaIPS_ToolData_info[0].Alpha_Op},
  { PREF_T_FLOAT,    "CombineResMult",   "1.0", &YaIPS_ToolData_info[0].CombineResMult},
  { PREF_T_INT,       "CombineOffset",     "0", &YaIPS_ToolData_info[0].CombineOffset},
  { PREF_T_FLOAT,         "WAddMult1",   "0.5", &YaIPS_ToolData_info[0].WAddMult1},
  { PREF_T_FLOAT,         "WAddMult2",   "0.5", &YaIPS_ToolData_info[0].WAddMult2},
  { PREF_T_INT,          "WAddOffset",     "0", &YaIPS_ToolData_info[0].WAddOffset},
  { PREF_T_INT,    "ShadingRefBright",   "200", &YaIPS_ToolData_info[0].ShadingRefBright},
  { PREF_T_INT, "ShadingAverageCount",     "0", &YaIPS_ToolData_info[0].ShadingAverageCount},
  { PREF_T_FLOAT,              "Fade",   "0.5", &YaIPS_ToolData_info[0].Fade},
  { PREF_T_INT,     "FadeImg_WinIdNr",    "-1", &YaIPS_ToolData_info[0].FadeImg_WinIdNr },
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_CombineWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp);

//-----------------------------------------------------------------------------------
// Parameter dialog
//
// This is a modal dialog. Therefore we can use global variables to hold
// info about the data.
//-----------------------------------------------------------------------------------

// defines for operators
// NOTE:  First operators have same values as YaIPS_RGB_Combine()
//        operator values.

#define YAIPS_COMBINE_GUI_OP_ADD       YAIPS_COMBINE_OP_ADD     // Addition
#define YAIPS_COMBINE_GUI_OP_W_ADD     YAIPS_COMBINE_OP_W_ADD   // Weighted addition
#define YAIPS_COMBINE_GUI_OP_SUB_1_2   YAIPS_COMBINE_OP_SUB_1_2 // Subtraction 1. source - 2. source
#define YAIPS_COMBINE_GUI_OP_SUB_2_1   YAIPS_COMBINE_OP_SUB_2_1 // Subtraction 2. source - 1. source
#define YAIPS_COMBINE_GUI_OP_SUB_ABS   YAIPS_COMBINE_OP_SUB_ABS // Subtraction with absolute value
#define YAIPS_COMBINE_GUI_OP_MULT      YAIPS_COMBINE_OP_MULT    // Multiplication
#define YAIPS_COMBINE_GUI_OP_MIN       YAIPS_COMBINE_OP_MIN     // Minimum value
#define YAIPS_COMBINE_GUI_OP_MAX       YAIPS_COMBINE_OP_MAX     // Maximum value
#define YAIPS_COMBINE_GUI_OP_AVG       YAIPS_COMBINE_OP_AVG     // Average images
#define YAIPS_COMBINE_GUI_OP_AND       YAIPS_COMBINE_OP_AND     // AND images
#define YAIPS_COMBINE_GUI_OP_OR        YAIPS_COMBINE_OP_OR      // OR images
#define YAIPS_COMBINE_GUI_OP_XOR       YAIPS_COMBINE_OP_XOR     // XOR images
#define YAIPS_COMBINE_GUI_OP_CMP_EQ    YAIPS_COMBINE_OP_CMP_EQ  // Compare images ==
#define YAIPS_COMBINE_GUI_OP_CMP_NE    YAIPS_COMBINE_OP_CMP_NE  // Compare images !=
#define YAIPS_COMBINE_GUI_OP_CMP_GT    YAIPS_COMBINE_OP_CMP_GT  // Compare images >
#define YAIPS_COMBINE_GUI_OP_CMP_LE    YAIPS_COMBINE_OP_CMP_LE  // Compare images <=
#define YAIPS_COMBINE_GUI_OP_CMP_GE    YAIPS_COMBINE_OP_CMP_GE  // Compare images >=
#define YAIPS_COMBINE_GUI_OP_CMP_LT    YAIPS_COMBINE_OP_CMP_LT  // Compare images <
#define YAIPS_COMBINE_GUI_OP_SHADING   YAIPS_COMBINE_OP_SHADING // Image shading
#define YAIPS_COMBINE_GUI_OP_FADE      YAIPS_COMBINE_OP_FADE    // Fade between two images
#define YAIPS_COMBINE_GUI_OP_FADEIMG1  YAIPS_COMBINE_OP_FADEIMG1 // Fade between two images by third image. First is overlaid.
#define YAIPS_COMBINE_GUI_OP_FADEIMG2  YAIPS_COMBINE_OP_FADEIMG2 // Fade between two images by third image. Second is overlaid.
#define YAIPS_COMBINE_GUI_BUTTON_MAX   (YAIPS_COMBINE_GUI_OP_FADEIMG2 + 1)  // Number of operator radio buttons

// ...

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolData;     // NOTE: Is used by all parameter dialog functions

static IqeFl_Tabs      *pTab_Groups;         // Point to tabulator GUI element
static Fl_Radio_Round_Button *OperatorButtons[ YAIPS_COMBINE_GUI_BUTTON_MAX]; // Table of filter buttons
static int OperatorType_Last;                // Catch operator change

static Fl_Radio_Round_Button *AlphaButtons[ YAIPS_COMBINE_ALPHA_BUTTON_MAX]; // Table of filter buttons
static int AlphaType_Last;                // Catch operator change

static Fl_Button *pBut_ShadReset, *pBut_ShadAverage;  // Buttons for shading
static IqeFl_Int_Input *pInt_ShadAvgCount;            // Shading average count

static Fl_Box *pMergeImg_Box;    // Merge two input images by third image
static Fl_Button *pMergeImg_But; // Merge two input images by third image

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{
  int i, ValThis;

  // Get last selected tab group

  pToolData->Tab_Group_Selected = pTab_Groups->GetTabGroup();

  // Update filter button
  // Current selection must be set

  // Update all operator type buttons

  if( OperatorType_Last != pToolData->CombineType) {              // Operator type has change

    OperatorType_Last = pToolData->CombineType;

    for( i = 0; i < YAIPS_COMBINE_GUI_BUTTON_MAX; i++) {

      ValThis = OperatorButtons[ i]->value();

      if( ValThis != (i == pToolData->CombineType)) {             // Not what we expected

        OperatorButtons[ i]->value( i == pToolData->CombineType);   // Set value
        OperatorButtons[ i]->redraw();
      }
    }
  }

  // Update all alpha operator type buttons

  if( AlphaType_Last != pToolData->Alpha_Op) {              // Alopha operator type has change

    AlphaType_Last = pToolData->Alpha_Op;

    for( i = 0; i < YAIPS_COMBINE_ALPHA_BUTTON_MAX; i++) {

      ValThis = AlphaButtons[ i]->value();

      if( ValThis != (i == pToolData->Alpha_Op)) {             // Not what we expected

        AlphaButtons[ i]->value( i == pToolData->Alpha_Op);   // Set value
        AlphaButtons[ i]->redraw();
      }
    }
  }

  // Support for shading

  if( pInt_ShadAvgCount->GetValue() != pToolData->ShadingAverageCount) {  // If average count is different

    pInt_ShadAvgCount->SetValue( pToolData->ShadingAverageCount);
  }

  // Background of average count
  Fl_Color NewColor;

  if( pToolData->ShadingAverageCount <= 0) {               // Not valid

    NewColor = FL_LIGHT1 + 1;

  } else if( pToolData->ShadingAverageCount >= SHADING_MAX_AVERAGE_COUNT) {  // Maximum reached

    NewColor = FL_YELLOW;

  } else {

    NewColor = FL_GREEN;
  }

  IqeB_GUI_WidgetColor( pInt_ShadAvgCount, NewColor);

  // Shading buttons enable

  Fl_RGB_Image *pImgIn1 /*, *pImgIn2*/;
  int Enable;

  pImgIn1 = NULL;       // Will be set if there is a image
  //pImgIn2 = NULL;       // Will be set if there is a shading image

  // Get first input image
  YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Input1_WinIdNr, NULL, &pImgIn1, NULL);

  // Second input image is only needed if we have the first.
  //if( pImgIn1 != NULL) {

  //  YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Input2_WinIdNr, NULL, &pImgIn2, NULL);
  //}

  // Enable, if there is a first input image and the output of the second can be set.
  Enable = pImgIn1 != NULL && YaIPS_ToolChangeOutputTest( pToolData->Input2_WinIdNr) > 0;  // Enable shading buttons

  IqeB_GUI_WidgetActivate( pBut_ShadReset, Enable);
  IqeB_GUI_WidgetActivate( pBut_ShadAverage, Enable && pToolData->ShadingAverageCount < SHADING_MAX_AVERAGE_COUNT);

  // Fading with image

  if( pToolData->CombineType == YAIPS_COMBINE_GUI_OP_FADEIMG1 ||    // Fade with image selected
      pToolData->CombineType == YAIPS_COMBINE_GUI_OP_FADEIMG2) {

    YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->FadeImg_WinIdNr, pMergeImg_Box);
    IqeB_GUI_WidgetActivate( pMergeImg_Box, true); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pMergeImg_But, true); // Set item activated/inactive

  } else {

    YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, -1, pMergeImg_Box);
    IqeB_GUI_WidgetActivate( pMergeImg_Box, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pMergeImg_But, false); // Set item activated/inactive
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

  IqeB_GUI_CloseToolWindow( (void **)&pMyParWin);
}

/************************************************************************************
 * YaIPS_Operator_Callback
 *
 * Operator will change
 */

static void YaIPS_Operator_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                       // get value to set

  if( pToolData->CombineType == Value) {            // Value will not change

    return;                                        // Exit, nothing to do
  }

  pToolData->CombineType = Value;                   // Set new value

  pToolData->Input1_Change = 0;                    // Force recalculation output
  pToolData->Input2_Change = 0;                    // Force recalculation output
}

/************************************************************************************
 * YaIPS_AlphaOp_Callback
 *
 * Alpha operator will change
 */

static void YaIPS_AlphaOp_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                       // get value to set

  if( pToolData->Alpha_Op == Value) {              // Value will not change

    return;                                        // Exit, nothing to do
  }

  pToolData->Alpha_Op = Value;                     // Set new value

  pToolData->Input1_Change = 0;                    // Force recalculation output
  pToolData->Input2_Change = 0;                    // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_Float_SetValue_Callback
 *
 * Callback, set a float or double value
 */

static void IqeB_GUI_Float_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  float *pValue;
  double Value;
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

  *pValue = Value;                        // update the variable

  pToolData->Input1_Change = 0;                    // Force recalculation output
  pToolData->Input2_Change = 0;                    // Force recalculation output
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

      pThis->SetValue( Value);            // Update on GUI
    }
  }

  *pValue = Value;                        // update the variable

  pToolData->Input1_Change = 0;                    // Force recalculation output
  pToolData->Input2_Change = 0;                    // Force recalculation output
}

/************************************************************************************
 * ShadingButtons
 *
 * One of the shading buttons was pressed.
 */

static void ShadingButtons( int Average)
{
  Fl_RGB_Image *pImgIn1, *pImgIn2, *pImgInTemp;

  pImgIn1 = NULL;       // Will be set if there is a image
  pImgIn2 = NULL;       // Will be set if there is a shading image
  pImgInTemp = NULL;

  // Get first input image
  YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Input1_WinIdNr, NULL, &pImgIn1, NULL);

  if( pImgIn1 == NULL ||         // No first image
      YaIPS_ToolChangeOutputTest( pToolData->Input2_WinIdNr) <= 0) {  // Or second image can not set an output image

    // Same requests are not satisfied
    return;
  }

  if( Average <= 0) {                              // Reset shading

    pToolData->ShadingAverageCount = 0;

    // Black second image

    // HACK: Multiply with 0 sets the pixel values to 0.
    //       pImgIn2 gets same size and number of pixels as pImgIn1.

    YaIPS_RGB_Combine( &pImgInTemp, pImgIn1, pImgIn1, YAIPS_COMBINE_OP_ADD,
                       YAIPS_COMBINE_ALPHA_NO, 0.0, 0);

    if( pImgInTemp != NULL) {  // Security test

      YaIPS_ToolChangeOutputCall( pToolData->Input2_WinIdNr, pImgInTemp);

      pImgInTemp->release();
    }

    return;    // Done
  }

  if( pToolData->ShadingAverageCount >= SHADING_MAX_AVERAGE_COUNT) { // Over max average count

    // nothing to do

    return;
  }

  if( pToolData->ShadingAverageCount < 0) {     // Clipping below sane value

    pToolData->ShadingAverageCount = 0;
  }

  // Gauss filter the first input image

  YaIPS_RGB_GaussXY( &pImgInTemp, pImgIn1, SHADING_GAUSS_FILER_KERNEL, YAIPS_GAUSXY_MODE_XY);

  if( pImgInTemp != NULL) {  // Security test

    if( pToolData->ShadingAverageCount <= 0) {       // No Average until now

      // Copy first shading image

      pToolData->ShadingAverageCount += 1;

      YaIPS_ToolChangeOutputCall( pToolData->Input2_WinIdNr, pImgInTemp);

      return;   // Done
    }

    // Check second input image
    YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Input2_WinIdNr, NULL, &pImgIn2, NULL);

    if( pImgIn2 == NULL) {  // Security test

      return;
    }

    // Weighted add

    pToolData->ShadingAverageCount += 1;

    YaIPS_RGB_Combine( &pImgInTemp, pImgInTemp, pImgIn2, YAIPS_COMBINE_OP_W_ADD,
                       YAIPS_COMBINE_ALPHA_NO, 1.0 / pToolData->ShadingAverageCount, 0,
                       (float)(pToolData->ShadingAverageCount - 1) / pToolData->ShadingAverageCount);

    YaIPS_ToolChangeOutputCall( pToolData->Input2_WinIdNr, pImgInTemp);
  }

}

/************************************************************************************
 * IqeB_GUI_Misc_SetValue_Callback
 *
 * This is usable for Fl_Valuator, Fl_Choice, Fl_Check_Button
 */

static void IqeB_GUI_Misc_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  void *pValue;

  pValue = (void *)pValueArg;            // get pointer to associated variable

  if( w == NULL ||                       // security test
      pValue == NULL) {

    return;
  }

  // Button actions

  if( w == pBut_ShadReset) {                   // Shading reset button

    ShadingButtons( 0);

  } else if( w == pBut_ShadAverage) {         // Shading average button

    ShadingButtons( 1);

  // Set value ...

  } else if( pValue == (void *)&pToolData->Fade) {

    // Fl_Valuator, Fl_Slider or Fl_Value_Slider

    Fl_Valuator *pThis;

    pThis  = (Fl_Valuator *)w;
    *(float *)pValue = pThis->value();               // update the variable

  } else if( pValueArg == &pToolData->FadeImg_WinIdNr) {

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + pToolData->iToolData, &pToolData->FadeImg_WinIdNr, pMergeImg_But, pMergeImg_Box);

  } else {

    Fl_Button *pThis;

    pThis  = (Fl_Check_Button *)w;
    *(int *)pValue = pThis->value();               // update the variable
  }

  pToolData->Input1_Change = 0;          // Force recalculation output
  pToolData->Input2_Change = 0;                    // Force recalculation output
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

  pMyParWin = new Fl_Window( xPos, yPos, 297 /*IQE_GUI_TOOLS_STD_WITDH*/, 190 /* 162 */, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  OperatorType_Last = -1;         // Reset last operator type
  AlphaType_Last    = -1;         // Reset last alpha operator type

  //
  //  GUI things
  //

  int x1, y, yy, xx2, yyDefault;
  //x/int xx1, xc;
  int yGroup;
  //x/char TempBuffer[ 256];

  //x/Fl_Check_Button *pCheckTemp;
  Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  IqeFl_Float_Input  *pFloatTemp;
  IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group;
  Fl_Button       *pTemp_Button;
  //x/Fl_Choice       *pTemp_Choice;
  Fl_Radio_Round_Button *pRadioButTemp;
  Fl_Hor_Nice_Slider    *pTemp_Slider;
  //x/char TempString[ 256];

  x1  = 4;
  //x/xx1 = pMyParWin->w() - 16;
  //x/xx2 = xx1 / 2;
  //x/xc  = pMyToolWin->w() / 2;          // x center
  yyDefault = 20;
  yy = yyDefault;

  y = 4;

  //
  // Tabs
  //

  pTemp_Tabs = new IqeFl_Tabs( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4);
  pTemp_Tabs->selection_color( YAIPS_COLOR_SELECTION);
  pTab_Groups = pTemp_Tabs;

  y += 26;

  //
  // Group 'Base'
  //

  yGroup = y;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LANGDEF_ALL);

    y += 8;

    xx2 = pMyParWin->w() - x1 - 8;

    pTemp_Box = new Fl_Box( x1, y, xx2, yy, LangStringLookup( "&GUI_Combine_TabM2=Alpha output:"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    x1 += xx2;

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 85;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabM3=NO"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabM3a="
                            "NO alpha output.\n"
                            "Strips alpha from the input images."));
    pRadioButTemp->callback( YaIPS_AlphaOp_Callback, (void *)YAIPS_COMBINE_ALPHA_NO);
    AlphaButtons[ YAIPS_COMBINE_ALPHA_NO] = pRadioButTemp;

    x1 += xx2;
    x1 += 16;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabM4=Keep 1"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabM4a="
                            "Keep 1 image.\n"
                            "Keep alpha from first input image."));
    pRadioButTemp->callback( YaIPS_AlphaOp_Callback, (void *)YAIPS_COMBINE_ALPHA_KEEP_1);
    AlphaButtons[ YAIPS_COMBINE_ALPHA_KEEP_1] = pRadioButTemp;

    x1 += xx2;
    x1 += 16;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabM5=Keep 2"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabM5a="
                            "Keep 2 image.\n"
                            "Keep alpha from second input image."));
    pRadioButTemp->callback( YaIPS_AlphaOp_Callback, (void *)YAIPS_COMBINE_ALPHA_KEEP_2);
    AlphaButtons[ YAIPS_COMBINE_ALPHA_KEEP_2] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabM6=Min"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabM6a="
                            "Minimum alpha.\n"
                            "Minimum alpha value from both images."));
    pRadioButTemp->callback( YaIPS_AlphaOp_Callback, (void *)YAIPS_COMBINE_ALPHA_MIN);
    AlphaButtons[ YAIPS_COMBINE_ALPHA_MIN] = pRadioButTemp;

    x1 += xx2;
    x1 += 16;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabM7=Max"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabM7a="
                            "Maximum alpha.\n"
                            "Maximum alpha value from both images."));
    pRadioButTemp->callback( YaIPS_AlphaOp_Callback, (void *)YAIPS_COMBINE_ALPHA_MAX);
    AlphaButtons[ YAIPS_COMBINE_ALPHA_MAX] = pRadioButTemp;


    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'Combine'
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Combine_TabA1=Simple"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Combine_TabA1a=Simple calculations"));

    y += 8;

    xx2 = 48;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA2=+"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA2a=Addition"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_ADD);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_ADD] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA3=1-2"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA3a=Subtraktion\n1. input - 2. input"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_SUB_1_2);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_SUB_1_2] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA4=2-1"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA4a=Subtraktion\n2. input - 1. input"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_SUB_2_1);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_SUB_2_1] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA5=|-|"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA5a=Absolute value of subtraction"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_SUB_ABS);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_SUB_ABS] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA6=*"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA6a=Multiplication"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_MULT);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_MULT] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA7=Avg"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA7a=Average of the pixel values"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_AVG);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_AVG] = pRadioButTemp;

    x1 += xx2;

    // Next line

    x1  = 4;
    y += yy + 4;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA8=Min"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA8a=Lower value"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_MIN);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_MIN] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA9=Max"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA9a=Greater value"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_MAX);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_MAX] = pRadioButTemp;

    x1 += xx2;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA10==="));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA10a=Image compare: equal"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_CMP_EQ);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_CMP_EQ] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA11=>"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA11a=Image compare: larger"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_CMP_GT);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_CMP_GT] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA12=<"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA12a=Image compare: smaller"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_CMP_LT);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_CMP_LT] = pRadioButTemp;

    x1 += xx2;

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 48;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA13=AND"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA13a=Bitwise AND"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_AND);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_AND] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA14=OR"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA14a=Bitwise OR"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_OR);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_OR] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA15=XOR"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA15a=Bitwise XOR"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_XOR);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_XOR] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA16=!="));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA16a=Image compare: unequal"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_CMP_NE);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_CMP_NE] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA17=<="));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA17a=Image compare: less than or equal"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_CMP_LE);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_CMP_LE] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabA18=>="));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA18a=Image compare: greater than or equal"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_CMP_GE);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_CMP_GE] = pRadioButTemp;

    x1 += xx2;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 48;
    xx2 = 40;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Combine_TabA19=Gain"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Combine_TabA19a=Gain\nMultiplier"));
    pFloatTemp->SetFormat( YAIPS_FILTER_FORMAT_MULT);
    pFloatTemp->SetValue( pToolData->CombineResMult);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->CombineResMult);
    pFloatTemp->SetModifyData( 0.1, 8.0, 0.5, 0.1);

    x1 += xx2;
    x1 += 48;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Combine_TabA20=Offs."));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Combine_TabA20a=Offset\nAddition"));
    pTemp_Int->SetValue( pToolData->CombineOffset);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->CombineOffset);
    pTemp_Int->SetModifyData( -256, 256, 16, 1);

    x1 += xx2;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group weighted add
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Combine_TabB1=Merge"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Combine_TabB1a=Weighted addition"));

    y += 8;

    xx2 = 148;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabB2=Weighted addition"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabB2a=Merge input images"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_W_ADD);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_W_ADD] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 48;
    xx2 = 40;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Combine_TabB3=Gain 1"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Combine_TabB3a=1. Gain Multiplier\nfor 1. input image"));
    pFloatTemp->SetFormat( YAIPS_FILTER_FORMAT_MULT);
    pFloatTemp->SetValue( pToolData->WAddMult1);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->WAddMult1);
    pFloatTemp->SetModifyData( -4.0, 4.0, 0.5, 0.1);

    x1 += xx2;
    x1 += 64;
    xx2 = 40;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Combine_TabB4=Gain 2"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Combine_TabB4a=2. Gain Multiplier\nfor 2. input image"));
    pFloatTemp->SetFormat( YAIPS_FILTER_FORMAT_MULT);
    pFloatTemp->SetValue( pToolData->WAddMult2);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->WAddMult2);
    pFloatTemp->SetModifyData( -4.0, 4.0, 0.5, 0.1);

    x1 += xx2;
    x1 += 48;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Combine_TabB5=Offs."));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Combine_TabB5a=Offset\nAddition"));
    pTemp_Int->SetValue( pToolData->WAddOffset);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->WAddOffset);
    pTemp_Int->SetModifyData( -256, 256, 16, 1);

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 106;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabB6=Crossfade"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabB6a=Crossfade between two images"));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_FADE);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_FADE] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 4;
    xx2 = pMyParWin->w() - x1 - 8;

    pTemp_Slider = new Fl_Hor_Nice_Slider( x1, y, xx2, yy);
    pTemp_Slider->tooltip( LangStringLookup( "&GUI_Combine_TabB6a=Crossfade between two images"));
    pTemp_Slider->type( FL_HOR_NICE_SLIDER);
    pTemp_Slider->bounds( 0.0, 1.0);
    pTemp_Slider->value( pToolData->Fade);
    pTemp_Slider->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Fade);
    pTemp_Slider->color( 40);

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 106;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabB7=Merge 1"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabB7a="
                                              "Merge two images by pixel values of third image.\n"
                                              "High pixel values output first input image,\n"
                                              "low pixel values output second input image.\n"
                                              "If present, an alpha channel is used for blending."));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_FADEIMG1);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_FADEIMG1] = pRadioButTemp;

    x1 += xx2 + 4;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabB8=Merge 2"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabB8a="
                                              "Merge two images by pixel values of third image.\n"
                                              "High pixel values output second input image,\n"
                                              "low pixel values output first input image.\n"
                                              "If present, an alpha channel is used for blending."));
    pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_FADEIMG2);
    OperatorButtons[ YAIPS_COMBINE_GUI_OP_FADEIMG2] = pRadioButTemp;

    // Next line

    x1  = 4 + 4;
    y += yy + 4;

    xx2= 16;

    // 3. Input element
    pTemp_Box = new Fl_Box( x1, y, xx2, yy, LANGDEF_IMGSEL_PDS_3);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label

    x1 += xx2;

    xx2 = 165;

    pMergeImg_Box = new Fl_Box( x1, y, xx2, yy + 2);
    pMergeImg_Box->box( FL_BORDER_BOX);
    pMergeImg_Box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pMergeImg_Box->labelsize( 18);
    pMergeImg_Box->copy_label( "---");

    // Legend
    //x/pTemp_Box = new Fl_Box( x1 - 80, y, 80, yy + 2, LangStringLookup( "&GUI_Color_TabF3=Alpha"));
    //x/pTemp_Box->box( FL_NO_BOX);
    //x/pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    //x/pTemp_Box->labelsize( 10);

    x1 += xx2 - 1;

    xx2 = yy;

    pMergeImg_But = new Fl_Button( x1, y, xx2, yy + 2, "@2>");
    pMergeImg_But->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->FadeImg_WinIdNr);
    pMergeImg_But->tooltip( LangStringLookup( "&GUI_Combine_TabB9a=Select merge input image"));
    pMergeImg_But->labelcolor( YAIPS_BCOL_BUTTON);
    pMergeImg_But->box( FL_BORDER_BOX);

    // Finish things for this group

    pTemp_Group->end();


    //
    // Group shading
    //

    y = yGroup;
    x1  = 4;

    pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Combine_TabC1=Shading"));
    pTemp_Group->tooltip( LangStringLookup( "&GUI_Combine_TabC1a=Compensation of shading/brightness differences in the image"));

      y += 8;

      xx2 = 114;

      pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Combine_TabC2=Shading"));
      pRadioButTemp->tooltip( LangStringLookup( "&GUI_Combine_TabC2a="
                             "Compensation for shading/brightness differences in the image\n"
                             "Input 1 is adjusted to the target brightness.\n"
                             "Input 2 holds the brightness reference image."));
      pRadioButTemp->callback( YaIPS_Operator_Callback, (void *)YAIPS_COMBINE_GUI_OP_SHADING);
      OperatorButtons[ YAIPS_COMBINE_GUI_OP_SHADING] = pRadioButTemp;

      xx2 = 40;
      x1 = pMyParWin->w() - xx2 - 8;

      pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Combine_TabC3=Target brightness"));
      pTemp_Int->tooltip( LangStringLookup( "&GUI_Combine_TabC3a="
                          "Target brightness.\n"
                          "Roughly brightest spot in the shading image."));
      pTemp_Int->SetValue( pToolData->ShadingRefBright);
      pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->ShadingRefBright);
      pTemp_Int->SetModifyData( 128, 256, 5, 1);

      // Next line

      x1 = 4;
      y += yy + 6;

      // a separator line
      pTemp_Box = new Fl_Box( x1 + 4, y, pMyParWin->w() - x1 - 12, 1, LangStringLookup( "&GUI_Combine_TabC4=Shading image"));
      pTemp_Box->align( FL_ALIGN_BOTTOM | FL_ALIGN_LEFT);
      pTemp_Box->box( FL_BORDER_BOX);    //  FL_BORDER_BOX FL_DOWN_FRAME

      y += 4;

      // Next line
      // A line with two buttons

      x1  = 4;
      y += yy - 4;

      x1 += 4;
      xx2 = (pMyParWin->w() - 12) / 2 - 8;

      yy = yyDefault + 4;     // Make buttons higher

      pTemp_Button = new Fl_Button( x1, y, xx2, yy, LANGDEF_BUTTON_RESET);
      pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->ShadingAverageCount);
      pTemp_Button->tooltip( LangStringLookup( "&GUI_Combine_TabC5a="
                                               "Image 'Input 2' is reset.\n"
                                               "The average count is set to 0."));
      pBut_ShadReset = pTemp_Button;

      x1 = pMyParWin->w() - xx2 - 8;

      pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_Combine_TabC6=Average"));
      pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->ShadingAverageCount);
      pTemp_Button->tooltip( LangStringLookup( "&GUI_Combine_TabC6a="
                                               "Image 'Input 1' is added to the image 'Input 2'\n"
                                               "weighting. The average count is increased by 1."));
      pBut_ShadAverage = pTemp_Button;

      // Next line

      y += yy + 4;

      yy = yyDefault;         // Restore default height

      xx2 = 40;
      x1 = pMyParWin->w() - xx2 - 8;

      pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Combine_TabC7=Average count"));
      pTemp_Int->tooltip( LangStringLookup( "&GUI_Combine_TabC7a="
                          "Number of averaged images."));
      pTemp_Int->SetValue( pToolData->ShadingAverageCount);
      pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->ShadingAverageCount);
      pTemp_Int->color( FL_LIGHT1 + 1);
      pTemp_Int->set_output();        // Only show value
      pInt_ShadAvgCount = pTemp_Int;

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
  Fl_Box *pBox_Input1;                   // Text for selected 1. input
  Fl_Button *pBut_Input1;                // Select 1. input
  Fl_Box *pBox_Input2;                   // Text for selected 2. input
  Fl_Button *pBut_Input2;                // Select 2. input
  Fl_Button *pGUI_Parameter;             // Open the parameter dialog

  // Create the window

  CLASS_WIN_TOOL( int X, int Y, int W, int H, const char *l, int iToolDataArg) : Fl_Double_Window( X, Y, W, H, l)
  {
    YaIPS_ToolData_info_t *pToolData;
    Fl_Button *pTemp_Button;
    Fl_Box    *pTemp_Box;

    iToolData = iToolDataArg;                    // Index of info data element, see YaIPS_ToolData_info
    pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

    // Initialize some data

    memset( &pToolData->YaIPS_ImageDisp, 0, sizeof( Fl_YaIPS_ImageDisp_t)); // Zero data

    pToolData->Input1_Change = 0;                // Reset image change check
    pToolData->Input2_Change = 0;                // Reset image change check

    // ...

    Fl_Group *pGUI_GroupTopSide;                 // Top side of window
    //x/Fl_Box   *pBoxTemp;
    int x, x1, y, xx, xx0, yy, yy2, hWin, wWin;

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

    yy2 = yy / 2 + 2;            // Height for input element

    // Legend of image inputs

    xx = 16;

    // 1. Input element
    pTemp_Box = new Fl_Box( x1, y + yy - yy2 - yy2 + 1, xx, yy2, LANGDEF_IMGSEL_PDS_1);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label

    // 2. Input element
    pTemp_Box = new Fl_Box( x1, y + yy - yy2, xx, yy2, LANGDEF_IMGSEL_PDS_2);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label

    x1 += xx;

    // Selected image input
    xx = yy * 2 + yy / 2;

    // 1. Input element
    pBox_Input1 = new Fl_Box( x1, y + yy - yy2 - yy2 + 1, xx, yy2);
    pBox_Input1->box( FL_BORDER_BOX);
    pBox_Input1->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pBox_Input1->labelsize( 18);
    pBox_Input1->copy_label( "---");

    // 2. Input element
    pBox_Input2 = new Fl_Box( x1, y + yy - yy2, xx, yy2);
    pBox_Input2->box( FL_BORDER_BOX);
    pBox_Input2->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pBox_Input2->labelsize( 18);
    pBox_Input2->copy_label( "---");

    x1 += xx - 1;

    // Pull down buttons

    xx = yy - 10;

    // 1. Input element
    pBut_Input1 = new Fl_Button( x1, y + yy - yy2 - yy2 + 1, xx, yy2, "@2>");
    pBut_Input1->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pBut_Input1->tooltip( LangStringLookup( "&GUI_Combine_Tools2a=Select 1. input image"));
    pBut_Input1->labelcolor( YAIPS_BCOL_BUTTON);
    pBut_Input1->box( FL_BORDER_BOX);

    // 2. Input element
    pBut_Input2 = new Fl_Button( x1, y + yy - yy2, xx, yy2, "@2>");
    pBut_Input2->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pBut_Input2->tooltip( LangStringLookup( "&GUI_Combine_Tools3a=Select 2. input image"));
    pBut_Input2->labelcolor( YAIPS_BCOL_BUTTON);
    pBut_Input2->box( FL_BORDER_BOX);

    x1 += xx + 5;

    // Open setting dialog

    xx = xx0;

    pGUI_Parameter = new Fl_Button( x1, y, xx, yy, "@+4menu2");
    pGUI_Parameter->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_Parameter->tooltip( LANGDEF_SETTINGS_POINTS);
    pGUI_Parameter->labelcolor( YAIPS_BCOL_BUTTON);
    pGUI_Parameter->shortcut( FL_COMMAND+'p');       // Short cut key

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

    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawBeforeCallback = YaIPS_ImageDispDrawBefore_cb; // Draw before callback
    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawAfterCallback  = YaIPS_ImageDispDrawAfter_cb;  // Draw after callback
    pToolData->YaIPS_ImageDisp.pImage_Box->DrawCallbackArg1    = &pToolData->YaIPS_ImageDisp;  // Pointer to Fl_YaIPS_ImageDisp_t
    pToolData->YaIPS_ImageDisp.pImage_Box->DrawCallbackArg2    = pToolData;                    // Optional pointer to ToolData

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

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

  if( pToolData->pMyToolWin == NULL) {  // security test, has main window

    return;
  }

  YaIPS_ImageDispReleaseBeforeClose( &pToolData->YaIPS_ImageDisp);

  pToolData->IsOpen = false;               // Flag info data is not in use

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
        YaIPS_ImageDisp.pImage_Img != NULL) {                 // Got an image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                       MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName);   // Load the image to the display
    }

  } else if( w == pMyToolWin->pBut_Input1) {                   // Select 1. input

    int WinIdNr_Before;

    WinIdNr_Before = pToolData->Input1_WinIdNr;

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + iToolData, &pToolData->Input1_WinIdNr, pMyToolWin->pBut_Input1, pMyToolWin->pBox_Input1);

    if( WinIdNr_Before != pToolData->Input1_WinIdNr) {         // Image source selection as changed

      pToolData->Input1_Change = 0;                            // Force recalculation output
    }

  } else if( w == pMyToolWin->pBut_Input2) {                   // Select 2. input

    int WinIdNr_Before;

    WinIdNr_Before = pToolData->Input2_WinIdNr;

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + iToolData, &pToolData->Input2_WinIdNr, pMyToolWin->pBut_Input2, pMyToolWin->pBox_Input2);

    if( WinIdNr_Before != pToolData->Input2_WinIdNr) {         // Image source selection as changed

      pToolData->Input2_Change = 0;                            // Force recalculation output
    }

  } else if( w == pMyToolWin->pGUI_Parameter) {                // Open parameter dialog

    YaIPS_GUI_ParameterWin( pMyToolWin->x() + 16, pMyToolWin->y() + 16, iToolData);
  }

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


  // Check input image and visualize state
  YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr, pMyToolWin->pBox_Input1);

  YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input2_WinIdNr, pMyToolWin->pBox_Input2);

  // ...

}

/************************************************************************************
 * IqeB_GUI_ToolsMyIdleAction
 */

static void IqeB_GUI_ToolsMyIdleAction( void *)
{
  YaIPS_ToolData_info_t *pToolData;
  CLASS_WIN_TOOL *pMyToolWin;
  int iToolData, ierr;
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

    int Input1_Check, Input1_ImageChanged, Input2_Check, Input2_ImageChanged, Input3_Check, Input3_ImageChanged, ForceUpdate;
    Fl_RGB_Image *pImgIn1, *pImgIn2, *pImgIn3;

    ForceUpdate = false;                         // Preset: NO Force update of output image

    // Check input image and visualize state
    Input1_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr,
                                           NULL, &pImgIn1, &Input1_ImageChanged);

    Input2_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input2_WinIdNr,
                                           NULL, &pImgIn2, &Input2_ImageChanged);

    if( pToolData->CombineType == YAIPS_COMBINE_GUI_OP_FADEIMG1 ||   // Need fade input image
        pToolData->CombineType == YAIPS_COMBINE_GUI_OP_FADEIMG2) {

      Input3_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->FadeImg_WinIdNr,
                                              NULL, &pImgIn3, &Input3_ImageChanged);
    } else {


      Input3_Check = 0;
      Input3_ImageChanged = 0;
      pToolData->FadeImg_Change = 0;
      pImgIn3 = NULL;
    }

    if( Input1_Check != 0 || Input2_Check != 0 || Input3_Check != 0) {    // Any input image is not valid

      // Empty display image
      YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);

      pToolData->Input1_Change = 0;                    // Force recalculation output
      pToolData->Input2_Change = 0;                    // Force recalculation output
      pToolData->FadeImg_Change = 0;                   // Force recalculation output

    } else
      if( Input1_ImageChanged != pToolData->Input1_Change ||  // Image counts are different
          Input2_ImageChanged != pToolData->Input2_Change ||
          Input3_ImageChanged != pToolData->FadeImg_Change) {

      pToolData->Input1_Change = Input1_ImageChanged;        // Image is processed
      pToolData->Input2_Change = Input2_ImageChanged;        // Image is processed
      pToolData->FadeImg_Change = Input3_ImageChanged;       // Image is processed

      // Do the image processing

      ierr = 0;                                              // Reset error
      errstring = NULL;                                      // Reset error string

      switch( pToolData->CombineType) {

      case YAIPS_COMBINE_GUI_OP_ADD:
      default:

        ierr = YaIPS_RGB_Combine( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pImgIn2, YAIPS_COMBINE_OP_ADD,
                                  pToolData->Alpha_Op, pToolData->CombineResMult, pToolData->CombineOffset);
        break;

      // NOTE:  First operators have same values as YaIPS_RGB_Combine()
      //        operator values.
      case YAIPS_COMBINE_GUI_OP_SUB_1_2:
      case YAIPS_COMBINE_GUI_OP_SUB_2_1:
      case YAIPS_COMBINE_GUI_OP_SUB_ABS:
      case YAIPS_COMBINE_GUI_OP_MULT:
      case YAIPS_COMBINE_GUI_OP_MIN:
      case YAIPS_COMBINE_GUI_OP_MAX:
      case YAIPS_COMBINE_GUI_OP_AVG:
      case YAIPS_COMBINE_GUI_OP_AND:
      case YAIPS_COMBINE_GUI_OP_OR:
      case YAIPS_COMBINE_GUI_OP_XOR:
      case YAIPS_COMBINE_GUI_OP_CMP_EQ:
      case YAIPS_COMBINE_GUI_OP_CMP_NE:
      case YAIPS_COMBINE_GUI_OP_CMP_GT:
      case YAIPS_COMBINE_GUI_OP_CMP_LE:
      case YAIPS_COMBINE_GUI_OP_CMP_GE:
      case YAIPS_COMBINE_GUI_OP_CMP_LT:

        ierr = YaIPS_RGB_Combine( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pImgIn2, pToolData->CombineType,
                                  pToolData->Alpha_Op, pToolData->CombineResMult, pToolData->CombineOffset);
        break;

      case YAIPS_COMBINE_GUI_OP_W_ADD:   // Weighted addition

        ierr = YaIPS_RGB_Combine( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pImgIn2, pToolData->CombineType,
                                  pToolData->Alpha_Op, pToolData->WAddMult1, pToolData->WAddOffset, pToolData->WAddMult2);
        break;

      case YAIPS_COMBINE_GUI_OP_SHADING:   // Image shading

        ierr = YaIPS_RGB_Combine( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pImgIn2, pToolData->CombineType,
                                  pToolData->Alpha_Op, 0.0, pToolData->ShadingRefBright);
        break;

      case YAIPS_COMBINE_GUI_OP_FADE:

        ierr = YaIPS_RGB_Combine( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pImgIn2, pToolData->CombineType,
                                  pToolData->Alpha_Op, pToolData->Fade, 0);
        break;

      case YAIPS_COMBINE_GUI_OP_FADEIMG1:
      case YAIPS_COMBINE_GUI_OP_FADEIMG2:

        {

          // NOTE: We have ensured before that the fade image 'pImgIn3' is valid

          ierr = YaIPS_RGB_Combine( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pImgIn2, pToolData->CombineType,
                                    pToolData->Alpha_Op, pToolData->Fade, 0, 0.0, pImgIn3);
        }
        break;
      } // end switch

      // Has a valid output image

      if( ierr == 0) {                                  // Have a result image

        YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, (char *)MY_WIN_GUI_NAME);

        YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp); // Reset error message

        ForceUpdate = true;                        // Force update of output image

      } else {                                          // Processing error

        pToolData->Input1_Change = 0;                   // Force recalculation output
        pToolData->Input2_Change = 0;                   // Force recalculation output

        // Empty display image
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
 * IqeB_GUI_CombineWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_CombineWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
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

  pToolData = YaIPS_ToolData_info + iToolData;            // Point to info data

  //
  // Prepare info data element
  //

  pToolData->iToolData = iToolData;                       // Set sub window number

  pToolData->IsOpen = true;                               // Flag image as open

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
 * IqeB_GUI_CombineWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_CombineWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

    IqeB_GUI_CombineWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  IqeB_GUI_CombineWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


