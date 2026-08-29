/****************************************************************************

  YaIPS_GUI_Objects.cpp

  Runlength coding of images and object processing.

  06.04.2025 RR: First edition of this file.

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
#define MY_WIN_ID     YAIPS_WIN_ID_OBJECTS      // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_OBJECTS       // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_Objects_Title=Objects"            // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinObjects"            // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_Objects_Tool  // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       YAIPS_WIN_SIZE_S1_X_MIN
#define MYWIN_SIZE_X_MAX       YAIPS_WIN_SIZE_S1_X_MAX
#define MYWIN_SIZE_X_DEFAULT   YAIPS_WIN_SIZE_S1_X_DEFAULT

#define MYWIN_SIZE_Y_MIN       YAIPS_WIN_SIZE_S1_Y_MIN
#define MYWIN_SIZE_Y_MAX       YAIPS_WIN_SIZE_S1_Y_MAX
#define MYWIN_SIZE_Y_DEFAULT   YAIPS_WIN_SIZE_S1_Y_DEFAULT


#define YAIPS_FILTER_FORMAT_MULT      "%.1f"     // Format string for multiplier

/***************************************************************************
* extern
* Last histogram and threshold results leftover by YaIPS_RGB_RLC_Code()
* with a bimodal threshold mode.
****************************************************************************
*/
extern Fl_YaIPS_Histo_RGB_t YaIPS_RGB_RLC_BM_Histo;
extern int YaIPS_RGB_RLC_BM_Thres, YaIPS_RGB_RLC_BM_Max1, YaIPS_RGB_RLC_BM_Max2;

/************************************************************************************
* forwards
*/

static void close_cb( Fl_Widget *w, long int iToolData);
static void YaIPS_ToolWin_GUI_Callback( Fl_Widget *w, long int iToolData);
static void MyWinUpdate( int iToolData, int DoEnable);
static void IqeB_GUI_ToolsMyIdleAction( void *);
static void YaIPS_GUI_MyDrawAfter_cb( Fl_Widget *pW, void *pArg1, void *pAr2);

/************************************************************************************
* Global variables for this window
*/

#define MAX_OBJ_TO_LABEL    10000  // max number of objects in the image to label

//-----------------------------------------------------------------------------------
// Manage multiple tool windows
//-----------------------------------------------------------------------------------

#define YAIPS_OBJ_POSTOP_N_ACTIONS   4  // Number of post-processing actions

typedef struct {
  int Post_Operator;                    // Post-processing operator, 0 = off.
  int Post_Arg;                         // Argument for Post-processing operator.
} YaIPS_PostAction_t;

typedef struct {

  // Parameters
  int IsOpen;                           // True if this window is open.
  void *pMyToolWin;                     // Pointer to window data ( is pointer to CLASS_WIN_TOOL)

  int MyWinPosX, MyWinPosY;             // last window position
  int MyWinSizeX, MyWinSizeY;           // last window size

  int Input1_WinIdNr;                   // Window ID nr of 1. input

  // Used for intern data management

  Fl_YaIPS_ImageDisp_t YaIPS_ImageDisp;   // Info image output

  // Used to catch a change of the input image
  int Input1_Change;                    // Last processed 'ImageChanged' from input image

  // Intermediate processing resources
  Tvector *pObjects;                    // If != NULL, objects in scene
  Timages *iRLC1;                       // If != NULL, RLC image.
  Timages *iRLC2;                       // If != NULL, RLC image.

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int Tab_Group_Selected;               // Number of last selected tab group.

  // Tab group 'Binarization'

  int Obj_ColorSpace;                   // Color space for color images BW, R, G or B
  int Obj_BinThres1;                    // Binarization 1. threshold
  int Obj_BinThres2;                    // Binarization 2. threshold
  int Obj_BinThresPc;                   // Binarization percent threshold
  int Obj_BinMode;                      // Binarization mode, 0: objects >= Thres, 1:  < Thres, 2: >= thres1 && < thres2 3: < thres1 || >= thres2
  int Obj_ThresInvert;                  // Invert threshold
  int Obj_ChromaR;                      // Chroma key red
  int Obj_ChromaG;                      // Chroma key green
  int Obj_ChromaB;                      // Chroma key blue
  int Obj_ChromaThres;                  // Chroma key threshold color distance

  // Tab group 'Post-processing'

  YaIPS_PostAction_t PostAction[ YAIPS_OBJ_POSTOP_N_ACTIONS];  // A number of post-processing actions

  // Tab group 'objects'

  int Obj_Label;                        // If true, label objects and show them
  int Obj_ShowText;                     // What text to show. 0 = nr, 1 = size in pixel, 2 = size in mm,
                                        //                    3 = ara in pixel², 4 = area in mm²
  int Obj_AreaMinSide;                  // Minimum area of ​​an object to be processed, side length of square
  int Obj_AreaMaxSide;                  // Maximum area of ​​an object to be processed, side length of square

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

  //
  // Parameter Dialog
  //

  { PREF_T_INT,    "MyParPosX",  IQE_GUI_NO_WINPOS_X_STRING, &YaIPS_ToolData_info[0].MyParPosX }, // NOTE: values will be clipped against MYWIN_SIZE_X_MIN / MYWIN_SIZE_Y_MIN
  { PREF_T_INT,    "MyParPosY",  IQE_GUI_NO_WINPOS_Y_STRING, &YaIPS_ToolData_info[0].MyParPosY },

  // Hold last selected tab

  { PREF_T_INT,    "Group_Selected",  "0", &YaIPS_ToolData_info[0].Tab_Group_Selected},

  // Tab group 'Binarization'

  { PREF_T_INT,   "Obj_ColorSpace",        "1", &YaIPS_ToolData_info[0].Obj_ColorSpace},
  { PREF_T_INT,    "Obj_BinThres1",      "128", &YaIPS_ToolData_info[0].Obj_BinThres1},
  { PREF_T_INT,    "Obj_BinThres2",      "128", &YaIPS_ToolData_info[0].Obj_BinThres2},
  { PREF_T_INT,   "Obj_BinThresPc",       "50", &YaIPS_ToolData_info[0].Obj_BinThresPc},
  { PREF_T_INT,      "Obj_BinMode",        "0", &YaIPS_ToolData_info[0].Obj_BinMode},
  { PREF_T_INT,  "Obj_ThresInvert",        "0", &YaIPS_ToolData_info[0].Obj_ThresInvert},
  { PREF_T_INT,      "Obj_ChromaR",        "0", &YaIPS_ToolData_info[0].Obj_ChromaR},
  { PREF_T_INT,      "Obj_ChromaG",      "200", &YaIPS_ToolData_info[0].Obj_ChromaG},
  { PREF_T_INT,      "Obj_ChromaB",        "0", &YaIPS_ToolData_info[0].Obj_ChromaB},
  { PREF_T_INT,  "Obj_ChromaThres",       "25", &YaIPS_ToolData_info[0].Obj_ChromaThres},

  // Tab group 'Post-processing'

  // NOTE: See define YAIPS_OBJ_POSTOP_N_ACTIONS for the number of entries
  { PREF_T_INT,  "Post_Operator_0",        "0", &YaIPS_ToolData_info[0].PostAction[ 0].Post_Operator},
  { PREF_T_INT,       "Post_Arg_0",        "1", &YaIPS_ToolData_info[0].PostAction[ 0].Post_Arg},
  { PREF_T_INT,  "Post_Operator_1",        "0", &YaIPS_ToolData_info[0].PostAction[ 1].Post_Operator},
  { PREF_T_INT,       "Post_Arg_1",        "1", &YaIPS_ToolData_info[0].PostAction[ 1].Post_Arg},
  { PREF_T_INT,  "Post_Operator_2",        "0", &YaIPS_ToolData_info[0].PostAction[ 2].Post_Operator},
  { PREF_T_INT,       "Post_Arg_2",        "1", &YaIPS_ToolData_info[0].PostAction[ 2].Post_Arg},
  { PREF_T_INT,  "Post_Operator_3",        "0", &YaIPS_ToolData_info[0].PostAction[ 3].Post_Operator},
  { PREF_T_INT,       "Post_Arg_3",        "1", &YaIPS_ToolData_info[0].PostAction[ 3].Post_Arg},

  // ...

  { PREF_T_INT,        "Obj_Label",        "0", &YaIPS_ToolData_info[0].Obj_Label},
  { PREF_T_INT,     "Obj_ShowText",        "0", &YaIPS_ToolData_info[0].Obj_ShowText},
  { PREF_T_INT,  "Obj_AreaMinSide",        "0", &YaIPS_ToolData_info[0].Obj_AreaMinSide},
  { PREF_T_INT,  "Obj_AreaMaxSide",        "0", &YaIPS_ToolData_info[0].Obj_AreaMaxSide},

};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_ObjectsWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp);

//-----------------------------------------------------------------------------------
// Parameter dialog
//
// This is a modal dialog. Therefore we can use global variables to hold
// info about the data.
//-----------------------------------------------------------------------------------

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolData;     // NOTE: Is used by all parameter dialog functions

static IqeFl_Tabs      *pTab_Groups;          // Point to tabulator GUI element
static IqeFl_Int_Input *pInt_Obj_BinThres1;   // Pointer to GUI input element
static IqeFl_Int_Input *pInt_Obj_BinThres2;   // Pointer to GUI input element
static IqeFl_Int_Input *pInt_Obj_BinThresPc;  // Pointer to GUI input element
static IqeFl_Int_Input *pInt_Obj_ChromaThres; // Pointer to GUI input element
static Fl_Button       *pBut_Obj_ChromaKey;   // Pointer to color button

// The pop up menu for post-processing operators

#define YAIPS_OBJ_POSTOP_NONE               0
#define YAIPS_OBJ_POSTOP_EROSION            1
#define YAIPS_OBJ_POSTOP_EROSION_2          2      // Border handling variant
#define YAIPS_OBJ_POSTOP_DILATION           3
#define YAIPS_OBJ_POSTOP_DILATION_2         4      // Border handling variant
#define YAIPS_OBJ_POSTOP_CLOSING            5
#define YAIPS_OBJ_POSTOP_CLOSING_2          6      // Border handling variant
#define YAIPS_OBJ_POSTOP_OPENING            7
#define YAIPS_OBJ_POSTOP_OPENING_2          8      // Border handling variant
#define YAIPS_OBJ_POSTOP_INVERSION          9
#define YAIPS_OBJ_POSTOP_REMOVE_BIGGEST    10
#define YAIPS_OBJ_POSTOP_KEEP_BIGGEST      11
#define YAIPS_OBJ_POSTOP_REMOVE_SIZE_THRES 12

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{
  int EnableThres1, EnableThres2, EnableThresPc, EnableChroma;

  // Get last selected tab group

  pToolData->Tab_Group_Selected = pTab_Groups->GetTabGroup();

  // Enables for binarization

  EnableThres1 = EnableThres2 = EnableThresPc = EnableChroma = false;   // Preset all disabled

  switch( pToolData->Obj_BinMode) {
  case YAIPS_RLC_BINMODE_GE_T1:          // >= BinThres1
  case YAIPS_RLC_BINMODE_L_T1:           // < BinThres1
    EnableThres1 = true;   // Set to enable
    break;
  case YAIPS_RLC_BINMODE_GE_T1_AND_L_T2: // >= BinThres1 && <  BinThres2
  case YAIPS_RLC_BINMODE_L_T1_OR_GE_T2:  // < BinThres1 || >=  BinThres2
    EnableThres1 = EnableThres2 = true;  // Set to enable
    break;
  case YAIPS_RLC_BINMODE_L_CHROMA:       // < chroma threshold
  case YAIPS_RLC_BINMODE_GE_CHROMA:      // >= chroma threshold
    EnableChroma = true;   // Set to enable
    break;
  case YAIPS_RLC_BINMODE_GE_BM_AUTO:     // < bimodal threshold
  case YAIPS_RLC_BINMODE_L_BM_AUTO:      // >= bimodal threshold
    EnableThresPc = true;   // Set to enable
    break;
  default:
    EnableThres1 = EnableThres2 = EnableChroma = true;   // Set to enable
    break;
  }

  IqeB_GUI_WidgetActivate( pInt_Obj_BinThres1, EnableThres1);     // Enable GUI elements
  IqeB_GUI_WidgetActivate( pInt_Obj_BinThres2, EnableThres2);     // Enable GUI elements
  IqeB_GUI_WidgetActivate( pInt_Obj_BinThresPc, EnableThresPc);   // Enable GUI elements
  IqeB_GUI_WidgetActivate( pBut_Obj_ChromaKey, EnableChroma);     // Enable GUI elements
  IqeB_GUI_WidgetActivate( pInt_Obj_ChromaThres, EnableChroma);   // Enable GUI elements
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
 * YaIPS_ColorChannel_Callback
 *
 * Output image setting will change
 */

static void YaIPS_ColorChannel_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                     // get value to set

  if( pToolData->Obj_ColorSpace == Value) {      // Value will not change

    return;                                      // Exit, nothing to do
  }

  pToolData->Obj_ColorSpace = Value;             // Set new value

  pToolData->Input1_Change = 0;                  // Force recalculation output
}

/************************************************************************************
 * YaIPS_BinMode_Callback
 *
 * Output Obj_BinMode setting will change
 */

static void YaIPS_BinMode_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                     // get value to set

  if( pToolData->Obj_BinMode == Value) {         // Value will not change

    return;                                      // Exit, nothing to do
  }

  pToolData->Obj_BinMode = Value;                // Set new value

  // Force recalculation of processing

  pToolData->Input1_Change = 0;                // Force recalculation output
}

/************************************************************************************
 * YaIPS_ShowText_Callback
 *
 * Output Obj_ShowText setting will change
 */

static void YaIPS_ShowText_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                     // get value to set

  if( pToolData->Obj_ShowText == Value) {         // Value will not change

    return;                                      // Exit, nothing to do
  }

  pToolData->Obj_ShowText = Value;                // Set new value

  // Force recalculation of processing

  pToolData->Input1_Change = 0;                // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_But_Color_SetValue_Callback
 */

static void IqeB_GUI_But_Color_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  Fl_Color ColorBefore, ColorAfter;
  Fl_Button *pThis;
  uchar r,g,b;

  // ...

  pThis  = (Fl_Button *)w;

  if( pThis == pBut_Obj_ChromaKey) {  // Chroma key color button

    ColorBefore = fl_rgb_color( pToolData->Obj_ChromaR, pToolData->Obj_ChromaG, pToolData->Obj_ChromaB);

    ColorAfter = IqeB_GUI_ColorChooser( ColorBefore);

    if( ColorBefore != ColorAfter) {

      Fl::get_color( ColorAfter, r, g, b);

      pToolData->Obj_ChromaR = r;
      pToolData->Obj_ChromaG = g;
      pToolData->Obj_ChromaB = b;

      pThis->color( ColorAfter);
      pThis->parent()->redraw();

      pToolData->Input1_Change = 0;                    // Force recalculation output
    }
  }
}

/************************************************************************************
 * IqeB_GUI_Float_SetValue_Callback
 *
 * Callback, set a float or double value
 */

#ifdef use_again
static void IqeB_GUI_Float_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  void *pValue;
  double Value;
  IqeFl_Float_Input *pThis;
  char TempBuffer[ 256];

  pThis  = (IqeFl_Float_Input *)w;
  pValue = (float *)pValueArg;             // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pValue == NULL) {

    return;
  }

  // Set value

  Value  = atof( pThis->value());         // Get the value

  if( pThis->Min < pThis->Max) {           // Range is set

    if( Value < pThis->Min) {              // Clip minimum value

      Value = pThis->Min;
    }

    if( Value > pThis->Max) {              // Clip maximum value

      Value = pThis->Max;
    }
  }

  // Reformat number on GUI
  sprintf( TempBuffer, YAIPS_FILTER_FORMAT_MULT, Value);   // Format number

  if( strcmp( TempBuffer, pThis->value()) != 0) {         // Is different

    pThis->value( TempBuffer);                            // Is different on GUI

    Value  = atof( TempBuffer);                           // Get proper value
  }

  *(float *)pValue = Value;

  pToolData->Input1_Change = 0;                    // Force recalculation output
}
#endif

/************************************************************************************
 * IqeB_GUI_Int_SetValue_Callback
 */

static void IqeB_GUI_Int_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  int Value, *pValue, WasClipped;
  IqeFl_Int_Input *pThis;
  char TempBuffer[ 256];

  // ...

  pThis  = (IqeFl_Int_Input *)w;
  pValue = (int *)pValueArg;               // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pValue == NULL) {

    return;
  }

  Value = atoi( pThis->value());           // get the value

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

      sprintf( TempBuffer, "%d", Value);   // Update on GUI
      pThis->value( TempBuffer);
    }
  }

  *pValue = Value;         // update the variable

  pToolData->Input1_Change = 0;                    // Force recalculation output
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

  pToolData->Input1_Change = 0;          // Force recalculation output
}

/************************************************************************************
 * YaIPS_PostOp_SetValue_Callback
 *
 * Set value for post-processeing operator
 */

static void YaIPS_PostOp_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  Fl_Choice *pThis;
  int *pValue;

  pThis  = (Fl_Choice *)w;
  pValue = (int *)pValueArg;              // get pointer to associated variable

  if( *pValue == pThis->value()) {        // Value is equal

    return;
  }

  *pValue = pThis->value();               // Set new value

  pToolData->Input1_Change = 0;           // Force recalculation output
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

  pMyParWin = new Fl_Window( xPos, yPos, 297 /*IQE_GUI_TOOLS_STD_WITDH*/, 162 + 24, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  //
  //  GUI things
  //

  int x1, x2, y, yy, xx1, xx2;
  //x/int xc, xx2;
  int yGroup;
  //x/char TempBuffer[ 256];

  Fl_Check_Button *pCheckTemp;
  Fl_Box          *pTempBox;
  IqeFl_Int_Input    *pTemp_Int;
  //x/IqeFl_Float_Input  *pFloatTemp;
  IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group, *pTemp_Group2;
  Fl_Button       *pTemp_Button;
  Fl_Choice       *pTempChoice;
  Fl_Radio_Round_Button *pRadioButTemp;
  char TempBuffer[ 256];

  //x/xx1 = pMyParWin->w() - 16;
  //x/xx2 = xx1 / 2;
  //x/xc  = pMyToolWin->w() / 2;          // x center
  yy  = 20;

  x1  = 4;
  y = 4;

  //
  // Tabs
  //

  pTemp_Tabs = new IqeFl_Tabs( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4);
  pTemp_Tabs->selection_color( YAIPS_COLOR_SELECTION);
  pTab_Groups = pTemp_Tabs;

  y += 26;

  //
  // Group 'Binarization'
  //

  yGroup = y;

  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Objects_TabA1=Binarization"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Objects_TabA1a="
                        "Binarization converts an image into\n"
                        "RLCs (Run Length Codes).\n"
                        "These can be processed more efficiently\n"
                        "than color or black and white images."));

    y += 8;

    x1 = 4;
    xx1 = pMyParWin->w() - x1 - 4;

    pTemp_Group2 = new Fl_Group( x1, y, xx1, yy, LANGDEF_COL_CHANNEL_DPOINT);  // Group around the radio buttons
    pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

    // ...

    x1 += 100;

    xx1 = 50;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LANGDEF_COLOR_BW);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabA3a="
                            "The intensity is used for a color image"));
    pRadioButTemp->callback( YaIPS_ColorChannel_Callback, (void *)YAIPS_DISP_COLMOD_BW);
    pRadioButTemp->value( pToolData->Obj_ColorSpace == YAIPS_DISP_COLMOD_BW);   // Set value

    x1 += xx1;
    x1 += 2;

    xx1 = 40;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LANGDEF_COLOR_R);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabA4a="
                            "The red color component is used for a color image"));
    pRadioButTemp->callback( YaIPS_ColorChannel_Callback, (void *)YAIPS_DISP_COLMOD_R);
    pRadioButTemp->value( pToolData->Obj_ColorSpace == YAIPS_DISP_COLMOD_R);   // Set value

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LANGDEF_COLOR_G);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabA5a="
                            "The green color component is used for a color image"));
    pRadioButTemp->callback( YaIPS_ColorChannel_Callback, (void *)YAIPS_DISP_COLMOD_G);
    pRadioButTemp->value( pToolData->Obj_ColorSpace == YAIPS_DISP_COLMOD_G);   // Set value

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LANGDEF_COLOR_B);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabA6a="
                            "The blue color component is used for a color image"));
    pRadioButTemp->callback( YaIPS_ColorChannel_Callback, (void *)YAIPS_DISP_COLMOD_B);
    pRadioButTemp->value( pToolData->Obj_ColorSpace == YAIPS_DISP_COLMOD_B);   // Set value

    pTemp_Group2->end();

    // Next line

    y += yy + 4;

    x1 = 4;
    xx1 = pMyParWin->w() - x1 - 4;

    pTemp_Group2 = new Fl_Group( x1, y, xx1, yy + yy + 4, LangStringLookup( "&GUI_Objects_TabA7=Binarization:"));  // Group around the radio buttons
    pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT);

    // ...

    x1 += 100;

    xx1 = 60;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Objects_TabA8=>= T1"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabA8a=Objects are brighter than the 1. threshold."));
    pRadioButTemp->callback( YaIPS_BinMode_Callback, (void *)YAIPS_RLC_BINMODE_GE_T1);
    pRadioButTemp->value( pToolData->Obj_BinMode == YAIPS_RLC_BINMODE_GE_T1);   // Set value

    x1 += xx1;
    x1 += 2;

    xx1 = 126;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Objects_TabA9=>= T1 and < T2"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabA9a="
                            "Objects are brighter than the 1. threshold.\n"
                            "and darker than the 2. threshold."));
    pRadioButTemp->callback( YaIPS_BinMode_Callback, (void *)YAIPS_RLC_BINMODE_GE_T1_AND_L_T2);
    pRadioButTemp->value( pToolData->Obj_BinMode == YAIPS_RLC_BINMODE_GE_T1_AND_L_T2);   // Set value

    x1 += xx1;
    x1 += 2;

    // Next line

    y += yy + 4;

    x1 = 4;

    xx1 = 98;

    pCheckTemp = new Fl_Check_Button( x1 + 4, y, xx1, yy, LANGDEF_INVERTED);
    pCheckTemp->tooltip( LangStringLookup( "&GUI_Objects_TabA20a=Inverts the binarization."));
    pCheckTemp->value( pToolData->Obj_ThresInvert);
    pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->Obj_ThresInvert);

    x1 += xx1;
    x1 += 2;

    xx1 = 80;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Objects_TabA10=Chroma"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabA10a="
                            "Chroma keying or green screen.\n"
                            "In a color image, the color\n"
                            "value is used for binarization."));
    pRadioButTemp->callback( YaIPS_BinMode_Callback, (void *)YAIPS_RLC_BINMODE_L_CHROMA);
    pRadioButTemp->value( pToolData->Obj_BinMode == YAIPS_RLC_BINMODE_L_CHROMA);   // Set value

    x1 += xx1;
    x1 += 2;

    xx1 = 80;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Objects_TabA11=Auto"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabA11a="
                            "Calculation of a binarization threshold through\n"
                            "histogram analysis. The brightness histogram of\n"
                            "the image must contain two distinct maxima. The\n"
                            "threshold is a % value between these maxima."));
    pRadioButTemp->callback( YaIPS_BinMode_Callback, (void *)YAIPS_RLC_BINMODE_GE_BM_AUTO);
    pRadioButTemp->value( pToolData->Obj_BinMode == YAIPS_RLC_BINMODE_GE_BM_AUTO);   // Set value

    x1 += xx1;
    x1 += 2;

    pTemp_Group2->end();

    // Next line

    y += yy + 4;

    xx1 = 34;

    x1 = 145 - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Objects_TabA21=Threshold 1"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Objects_TabA21a="
                        "1. Brightness threshold for binarization.\n"
                        "Separates objects from the background."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Obj_BinThres1);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Obj_BinThres1);
    pTemp_Int->SetModifyData( 0, 255, 10, 1);
    pInt_Obj_BinThres1 = pTemp_Int;

    x1 = pMyParWin->w() - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Objects_TabA22=Threshold 2"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Objects_TabA22a="
                        "2. Brightness threshold for binarization.\n"
                        "Separates objects from the background."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Obj_BinThres2);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Obj_BinThres2);
    pTemp_Int->SetModifyData( 0, 255, 10, 1);
    pInt_Obj_BinThres2 = pTemp_Int;

    // Next line

    y += yy + 4;

    x1 = 4;
    xx1 = 97;

    pTempBox = new Fl_Box( x1, y - 3, xx1, yy, LangStringLookup( "&GUI_Objects_TabA23=Chroma key"));
    pTempBox->box( FL_NO_BOX);
    pTempBox->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    xx1 = 34;

    x1 = 145 - xx1 - 8;

    pTemp_Button = new Fl_Button( x1, y, xx1, yy);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Objects_TabA24a="
                                             "Press to change the color"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Button->color( fl_rgb_color( pToolData->Obj_ChromaR, pToolData->Obj_ChromaG, pToolData->Obj_ChromaB));
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, NULL);

    pBut_Obj_ChromaKey = pTemp_Button;


    x1 = pMyParWin->w() - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Objects_TabA25=Threshold"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Objects_TabA25a=Chroma key threshold color difference"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Obj_ChromaThres);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Obj_ChromaThres);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_Obj_ChromaThres= pTemp_Int;

    // Next line

    y += yy + 4;

    xx1 = 34;

    x1 = 145 - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Objects_TabA26=Threshold %"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Objects_TabA26a="
                        "Percent threshold for binarization.\n"
                        "% value between maxima of a bimodal histogram."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Obj_BinThresPc);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Obj_BinThresPc);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_Obj_BinThresPc = pTemp_Int;

    // Finish things for this group
    pTemp_Group->end();

  //
  // Group 'Post-processing'
  //

  y = yGroup;

  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Objects_TabB1=Actions"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Objects_TabB1a="
                        "After binarization, additional\n"
                        "actions can be applied to the RLCs."));

    y += 8;

    x1 = 14;
    xx1 = 188;

    x2 = 222;
    xx2 = 34;

    // Legend
    yy = 14;

    pTempBox = new Fl_Box( x1, y - 3, xx1, yy, LangStringLookup( "&GUI_Objects_TabB2=Action"));
    pTempBox->box( FL_NO_BOX);
    pTempBox->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    pTempBox = new Fl_Box( x2, y - 3, pMyParWin->w() - x2 - 4, yy, LangStringLookup( "&GUI_Objects_TabB3=Argument"));
    pTempBox->box( FL_NO_BOX);
    pTempBox->align( FL_ALIGN_TOP_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    y += yy;

    yy = 24;

    for( int i = 0; i < YAIPS_OBJ_POSTOP_N_ACTIONS; i++) {

      pTempChoice = new Fl_Choice( x1, y, xx1, yy);
      pTempChoice->tooltip( LangStringLookup( "&GUI_Objects_TabB3a=Auswahl einer Aktion"));
      pTempChoice->callback( YaIPS_PostOp_SetValue_Callback, &pToolData->PostAction[ i].Post_Operator);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp1=---"),                       0, NULL, (void *)YAIPS_OBJ_POSTOP_NONE);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp2=Erosion"),                   0, NULL, (void *)YAIPS_OBJ_POSTOP_EROSION);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp2b=Erosion-2"),                0, NULL, (void *)YAIPS_OBJ_POSTOP_EROSION_2);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp3=Dilation"),                  0, NULL, (void *)YAIPS_OBJ_POSTOP_DILATION);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp3b=Dilation-2"),               0, NULL, (void *)YAIPS_OBJ_POSTOP_DILATION_2);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp4=Closing"),                   0, NULL, (void *)YAIPS_OBJ_POSTOP_CLOSING);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp4b=Closing-2"),                0, NULL, (void *)YAIPS_OBJ_POSTOP_CLOSING_2);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp5=Opening"),                   0, NULL, (void *)YAIPS_OBJ_POSTOP_OPENING);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp5b=Opening-2"),                0, NULL, (void *)YAIPS_OBJ_POSTOP_OPENING_2);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp6=Inversion"),                 0, NULL, (void *)YAIPS_OBJ_POSTOP_INVERSION);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp7=Remove largest object"),     0, NULL, (void *)YAIPS_OBJ_POSTOP_REMOVE_BIGGEST);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp8=Keep largest object"),       0, NULL, (void *)YAIPS_OBJ_POSTOP_KEEP_BIGGEST);
      pTempChoice->add( LangStringLookup( "&GUI_Objects_PostOp9=Remove size threshold"),     0, NULL, (void *)YAIPS_OBJ_POSTOP_REMOVE_SIZE_THRES);

      pTempChoice->selection_color( Fl::get_color( FL_SELECTION_COLOR));
      pTempChoice->value( pToolData->PostAction[ i].Post_Operator);

      pTemp_Int = new IqeFl_Int_Input( x2, y, xx2, yy);
      pTemp_Int->tooltip( LangStringLookup( "&GUI_Objects_TabB4a=Argument for selected action"));
      pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
      sprintf( TempBuffer, "%d", pToolData->PostAction[ i].Post_Arg);
      pTemp_Int->value( TempBuffer);
      pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PostAction[ i].Post_Arg);
      pTemp_Int->SetModifyData( 0, 8, 1);

      y += yy + 2;
    }

    // Finish things for this group
    pTemp_Group->end();

  //
  // Group 'Post-processing'
  //

  y = yGroup;

  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Objects_TabC1=Objects"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Objects_TabC1a="
                        "RLCs are grouped into objects.\n"
                        "These contain information about an object.\n"
                        "For example, the position and size of an\n"
                        "enclosing rectangle, the number of binarized\n"
                        "pixels/area, the center of gravity, ..."));

    y += 8;

    xx1 = pMyParWin->w() - 16;
    x1  = 4;

    pCheckTemp = new Fl_Check_Button( x1 + 4, y, xx1, yy, LangStringLookup( "&GUI_Objects_TabC2=Label and display objects"));
    pCheckTemp->tooltip( LangStringLookup( "&GUI_Objects_TabC2a=Identification (labeling) of connected image parts"));
    pCheckTemp->value( pToolData->Obj_Label);
    pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->Obj_Label);

    // Next line

    y += yy + 4;

    x1 = 4;
    xx1 = pMyParWin->w() - x1 - 4;

    pTemp_Group2 = new Fl_Group( x1, y, xx1, yy, LangStringLookup( "&GUI_Objects_TabC3=Thresh. Size:"));  // Group around the radio buttons
    pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

    // ...

    xx1 = 34;

    x1 = 180 - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Objects_TabC4=T small"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Objects_TabC4a="
                        "Smallest side length of a square in pixels.\n"
                        "The area of the square is the minimum\n"
                        "area of an object to be processed."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Obj_AreaMinSide);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Obj_AreaMinSide);
    pTemp_Int->SetModifyData( 0, 200, 10, 1);

    x1 = 274 - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Objects_TabC5=T large"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Objects_TabC5a="
                        "Largest side length of a square in pixels.\n"
                        "The area of the square is the maximum\n"
                        "area of an object to be processed.\n"
                        "NOTE: must be greater than 'T small'."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Obj_AreaMaxSide);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Obj_AreaMaxSide);
    pTemp_Int->SetModifyData( 0, 500, 10, 1);

    pTemp_Group2->end();

    // Next line

    y += yy + 4;

    x1 = 4;
    xx1 = pMyParWin->w() - x1 - 4;

    pTemp_Group2 = new Fl_Group( x1, y, xx1, yy * 2, LangStringLookup( "&GUI_Objects_TabC6=Label:"));  // Group around the radio buttons
    pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT);
    pTemp_Group2->vertical_label_margin( 3);

    // ...

    x1 += 86;
    x2 = x1;        // Save position

    xx1 = 38;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Objects_TabC7=Nr"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_Tab7a=Label with consecutive numbering."));
    pRadioButTemp->callback( YaIPS_ShowText_Callback, (void *)0);
    pRadioButTemp->value( pToolData->Obj_ShowText == 0);   // Set value

    x1 += xx1;
    x1 += 2;

    xx1 = 66;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Objects_TabC8=Si. Pix"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabC8a=Label with size in pixels."));
    pRadioButTemp->callback( YaIPS_ShowText_Callback, (void *)1);
    pRadioButTemp->value( pToolData->Obj_ShowText == 1);   // Set value

    x1 += xx1;
    x1 += 2;

    xx1 = 70;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Objects_TabC9=Si. mm"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabC9a=Label with size in mm."));
    pRadioButTemp->callback( YaIPS_ShowText_Callback, (void *)2);
    pRadioButTemp->value( pToolData->Obj_ShowText == 2);   // Set value

    // Next line

    y += yy - 2;

#ifdef use_again
    x1 = x2;        // Restore position
#else
    x1 = 10;
#endif

    xx1 = 66;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Objects_TabC10=Ar. Pix²"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabC10a=Label with area in pixels²."));
    pRadioButTemp->callback( YaIPS_ShowText_Callback, (void *)3);
    pRadioButTemp->value( pToolData->Obj_ShowText == 3);   // Set value

    x1 += xx1;
    x1 += 2;

    xx1 = 70;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Objects_TabC11=Ar. mm²"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabC11a=BLabel with area in mm²."));
    pRadioButTemp->callback( YaIPS_ShowText_Callback, (void *)4);
    pRadioButTemp->value( pToolData->Obj_ShowText == 4);   // Set value

    x1 += xx1;
    x1 += 2;

    xx1 = 80;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Objects_TabC12=Center"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabC12a="
                            "Marks the center of gravity\n"
                            "of the binarized area."));
    pRadioButTemp->callback( YaIPS_ShowText_Callback, (void *)5);
    pRadioButTemp->value( pToolData->Obj_ShowText == 5);   // Set value
    x1 += xx1;
    x1 += 2;

    xx1 = 47;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Objects_TabC13=Bright."));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Objects_TabC13a="
                            "Labeling with average brightness/color."));
    pRadioButTemp->callback( YaIPS_ShowText_Callback, (void *)6);
    pRadioButTemp->value( pToolData->Obj_ShowText == 6);   // Set value

    pTemp_Group2->end();

    // Next line

    y += yy + 4;

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

    pToolData->Input1_Change = 0;                // Reset image change check

    // ...

    Fl_Group *pGUI_GroupTopSide;                 // Top side of window
    Fl_Box   *pBoxTemp;
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

    xx = yy * 2 + yy / 2;

    // Input element
    pBox_Input1 = new Fl_Box( x1, y + 10, xx, yy - 10);
    pBox_Input1->box( FL_BORDER_BOX);
    pBox_Input1->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pBox_Input1->labelsize( 18);
    pBox_Input1->copy_label( "---");

    // Legend
    pBoxTemp = new Fl_Box( x1, y - 3, xx, 12, LANGDEF_INPUT);
    pBoxTemp->box( FL_NO_BOX);
    pBoxTemp->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pBoxTemp->labelsize( 10);

    x1 += xx - 1;

    xx = yy - 10;

    pBut_Input1 = new Fl_Button( x1, y + 10, xx, yy - 10, "@2>");
    pBut_Input1->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pBut_Input1->tooltip( LANGDEF_SELECT_INPUT_IMAGE);
    pBut_Input1->labelcolor( YAIPS_BCOL_BUTTON);
    pBut_Input1->box( FL_BORDER_BOX);

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
    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawAfterCallback  = YaIPS_GUI_MyDrawAfter_cb;     // Draw after callback
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

  if( pToolData->pObjects != NULL) {       // Object vector was used

    ve_remove( pToolData->pObjects);       // Release the vector
    pToolData->pObjects = NULL;
  }

  if( pToolData->iRLC1 != IMNULL) {        // RLC image was used

    im_remove( pToolData->iRLC1);
    pToolData->iRLC1 = IMNULL;
  }

  if( pToolData->iRLC2 != IMNULL) {        // RLC image was used

    im_remove( pToolData->iRLC2);
    pToolData->iRLC2 = IMNULL;
  }

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
        YaIPS_ImageDisp.pImage_Img != NULL) {                   // Got an image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                      MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName);   // Load the image to the display
    }

  } else if( w == pMyToolWin->pBut_Input1) {                   // Select 1. input

    int WinIdNr_Before;

    WinIdNr_Before = pToolData->Input1_WinIdNr;

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + iToolData, &pToolData->Input1_WinIdNr, pMyToolWin->pBut_Input1, pMyToolWin->pBox_Input1);

    if( WinIdNr_Before != pToolData->Input1_WinIdNr) {         // Image source selection as changed

      pToolData->Input1_Change = 0;                    // Force recalculation output
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

    int Input1_Check, Input1_ImageChanged, ForceUpdate, BinThres1, BinThres2, BinMode;
    int UsedSrcXX, iPostAction, WasLabeled, MaxLength, ThisLength;
    Fl_RGB_Image *pImgIn1;

    ForceUpdate = false;                         // Preset: NO Force update of output image

    // Check input image and visualize state
    Input1_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr,
                                           NULL, &pImgIn1, &Input1_ImageChanged);

    if( Input1_Check != 0) {                                 // Input image is not valid

      // Empty a display image
      YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);

    } else
      if( Input1_ImageChanged != pToolData->Input1_Change) { // Image count is different

      pToolData->Input1_Change = Input1_ImageChanged;        // Image is processed

      // Do the image processing

      ierr = 0;                                              // Reset error
      errstring = NULL;                                      // Reset error string

      YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp);  // Empty info string

      // --------------------------------------
      // Preallocate object vector
      // --------------------------------------

      if( pToolData->pObjects == NULL) {                   // NO object vector preallocated

        pToolData->pObjects = ve_ucreate( DV_HOST);

        if( pToolData->pObjects != VENULL) {               // Have the vector

          // preallocate the max number of objects

          ierr = ve_alloc( pToolData->pObjects , MAX_OBJ_TO_LABEL * (sizeof(Trl2objdst) / sizeof(int32)), sizeof(int32), TY_INT32);

          if( ierr != 0) {                                 // ERROR allocating data

#ifdef use_again
            sprintf( errbuffer, "error %d allocating vector", ierr);
#else
            errstring = ERR_IPS_VEC_ALLOC;
#endif
          }

        } else {                                           // Error creating the vector

          errstring = ERR_IPS_VEC_CREATE;

          ierr = -200;
        }
      }

      // --------------------------------------
      // Binaries image to run length codes
      // --------------------------------------

      if( pToolData->Obj_BinMode == YAIPS_RLC_BINMODE_L_CHROMA ||     // Any of the chroma binarization modes
          pToolData->Obj_BinMode == YAIPS_RLC_BINMODE_GE_CHROMA) {

        // BinThres1 holds threshold color distance
        BinThres1 = pToolData->Obj_ChromaThres;

        // BinThres2 holds color key
        BinThres2 = (pToolData->Obj_ChromaR & 0xff) | ((pToolData->Obj_ChromaG & 0xff) << 8) | ((pToolData->Obj_ChromaB & 0xff) << 16);

      } else if( pToolData->Obj_BinMode == YAIPS_RLC_BINMODE_GE_BM_AUTO ||     // Any of the bimodal threshold binarization modes
                 pToolData->Obj_BinMode == YAIPS_RLC_BINMODE_L_BM_AUTO) {

        BinThres1 = pToolData->Obj_BinThresPc;
        BinThres2 = 0;

      } else {                                   // Other binarization mode

        BinThres1 = pToolData->Obj_BinThres1;
        BinThres2 = pToolData->Obj_BinThres2;
      }

      BinMode = pToolData->Obj_BinMode;
      if( pToolData->Obj_ThresInvert) {             // Invert threshold use of binarization mode

        BinMode ^= 1;                               // Invert lower bit
      }

      ierr = YaIPS_RGB_RLC_Code( &pToolData->iRLC1, pImgIn1,
                                 pToolData->Obj_ColorSpace,       // In: Binarization threshold
                                 BinThres1,                       // In: Binarization threshold
                                 BinThres2,                       // In: Binarization threshold
                                 BinMode,                         // In: Binarization mode, 0: objects >= Thres, 1: code if < Thres
                                 &UsedSrcXX);

      // --------------------------------------
      // Post-processing actions
      // NOTE: On entry and on exit iRLC1 holds the usable RLCs.
      //       iRLC2 is used for temporary processing.
      // --------------------------------------

      WasLabeled = 0;                         // RLCs have NOT been labeled

      for( iPostAction = 0; iPostAction < YAIPS_OBJ_POSTOP_N_ACTIONS; iPostAction++) {

        int Post_Operator, Post_Arg;
        Timages *iRLC_tmp;

        if( ierr != 0) {      // There was an error before
          break;
        }

        Post_Operator = pToolData->PostAction[ iPostAction].Post_Operator;
        Post_Arg      = pToolData->PostAction[ iPostAction].Post_Arg;

        // Check for proper iRLC2 image

        switch( Post_Operator) {

        case YAIPS_OBJ_POSTOP_EROSION:
        case YAIPS_OBJ_POSTOP_EROSION_2:
        case YAIPS_OBJ_POSTOP_DILATION:
        case YAIPS_OBJ_POSTOP_DILATION_2:
        case YAIPS_OBJ_POSTOP_CLOSING:
        case YAIPS_OBJ_POSTOP_CLOSING_2:
        case YAIPS_OBJ_POSTOP_OPENING:
        case YAIPS_OBJ_POSTOP_OPENING_2:

          if( pToolData->iRLC2 == NULL ||              // Have no image
              getxm( pToolData->iRLC1) != getxm( pToolData->iRLC2) ||
              getym( pToolData->iRLC1) != getym( pToolData->iRLC2)) {

            if( pToolData->iRLC2 != IMNULL) {         // There is an image width different sizes

              im_remove( pToolData->iRLC2);
              pToolData->iRLC2 = IMNULL;
            }

            // Get sizes and type form iRLC1
            pToolData->iRLC2 = im_ucreateMem( NULL, getyp( pToolData->iRLC1), DV_HOST, getxm( pToolData->iRLC1), getym( pToolData->iRLC1), NULL);

            if( pToolData->iRLC2 == IMNULL) {

#ifdef use_again
              sprintf( errbuffer, "internal error: error %d creating image", ierr);
              errstring = errbuffer;
#else
              errstring = ERR_IPS_IMG_CREATE;
#endif

              ierr = -100;
              break;
            }
          }

          break;
        }

        if( ierr != 0) {      // There was an error before
          break;
        }

        // ...

        switch( Post_Operator) {

        case YAIPS_OBJ_POSTOP_EROSION:

          ierr = rl2t_Erode( &pToolData->iRLC1, &pToolData->iRLC2, Post_Arg);
          break;

        case YAIPS_OBJ_POSTOP_EROSION_2:

          ierr = rl2t_Erode2( &pToolData->iRLC1, &pToolData->iRLC2, Post_Arg, UsedSrcXX);
          break;

        case YAIPS_OBJ_POSTOP_DILATION:

          ierr = rl2t_Dilate( &pToolData->iRLC1, &pToolData->iRLC2, Post_Arg, 0, UsedSrcXX);
          break;

        case YAIPS_OBJ_POSTOP_DILATION_2:

          ierr = rl2t_Dilate2( &pToolData->iRLC1, &pToolData->iRLC2, Post_Arg, 0, UsedSrcXX);
          break;

        case YAIPS_OBJ_POSTOP_CLOSING:

          ierr = rl2t_Dilate( &pToolData->iRLC1, &pToolData->iRLC2, Post_Arg, 0, UsedSrcXX);

          if( ierr != 0) {      // There was an error before
            break;
          }

          ierr = rl2t_Erode( &pToolData->iRLC1, &pToolData->iRLC2, Post_Arg);
          break;

        case YAIPS_OBJ_POSTOP_CLOSING_2:

          ierr = rl2t_Dilate( &pToolData->iRLC1, &pToolData->iRLC2, Post_Arg, 0, UsedSrcXX);

          if( ierr != 0) {      // There was an error before
            break;
          }

          ierr = rl2t_Erode2( &pToolData->iRLC1, &pToolData->iRLC2, Post_Arg, UsedSrcXX);
          break;

        case YAIPS_OBJ_POSTOP_OPENING:

          ierr = rl2t_Erode( &pToolData->iRLC1, &pToolData->iRLC2, Post_Arg);

          if( ierr != 0) {      // There was an error before
            break;
          }

          ierr = rl2t_Dilate( &pToolData->iRLC1, &pToolData->iRLC2, Post_Arg, 0, UsedSrcXX);
          break;

        case YAIPS_OBJ_POSTOP_OPENING_2:

          ierr = rl2t_Erode( &pToolData->iRLC1, &pToolData->iRLC2, Post_Arg);

          if( ierr != 0) {      // There was an error before
            break;
          }

          ierr = rl2t_Dilate2( &pToolData->iRLC1, &pToolData->iRLC2, Post_Arg, 0, UsedSrcXX);
          break;

        case YAIPS_OBJ_POSTOP_INVERSION:

          ierr = rl2t_invert( pToolData->iRLC1, pToolData->iRLC2, 0, UsedSrcXX);

          // exchange source and destination
          iRLC_tmp = pToolData->iRLC1; pToolData->iRLC1 = pToolData->iRLC2; pToolData->iRLC2 = iRLC_tmp;
          break;

        case YAIPS_OBJ_POSTOP_REMOVE_BIGGEST:   // Remove biggest object in scene
        case YAIPS_OBJ_POSTOP_KEEP_BIGGEST:     // Keep biggest object in scene

          {
            int iObj, nObj;
            Trl2objdst *pObjBest, *pObjBase, *pObjEnd, *pObj;

            if( WasLabeled > 0)  {           // Was labeled before

              // Need to reset the labels in the RLCs before a new labeling
              rl2t_ResetLabelRLCs( pToolData->iRLC1);
            }

            ierr = YaIPS_RGB_RLC_LabelMeas( pToolData->pObjects, pToolData->iRLC1, 0, 0);

            if( ierr != 0) {         // There was an error before
              break;
            }

            WasLabeled += 1;    // RLCS have been labeled

            // get biggest object.
            // We use the city block length to measure 'how big'.

            MaxLength = 0;
            pObjBest  = NULL;

            pObjBase = (Trl2objdst *)vgetpm( pToolData->pObjects);

            nObj = (int)(vgetnm( pToolData->pObjects) / (sizeof(Trl2objdst) / sizeof(int32)));
            pObj = pObjBase;

            for( iObj = 0; iObj < nObj; iObj++, pObj++) {

              ThisLength = (pObj->xmax - pObj->xmin + 1) + (pObj->ymax - pObj->ymin + 1);

              if( ThisLength > MaxLength) {
                MaxLength = ThisLength;
                pObjBest  = pObj;
              }
            }

            if( pObjBest != NULL) {         // there is a biggest object

              if( Post_Operator == YAIPS_OBJ_POSTOP_REMOVE_BIGGEST) { // Remove biggest object in scene

                // Remove RLC from the biggest object

                rl2t_RemoveLabelRLCs( pToolData->iRLC1, pObjBest);

                // Remove it from the object vector

                pObjEnd = pObjBase + nObj;   // Point after end of all objects

                pObj = pObjBest + 1;         // Point after the best object

                iObj = pObjEnd - pObj;       // # objects after the best object

                if( iObj > 0) {              // Make sense to copy down

                  // Copy down in memory
                  memcpy( pObjBest, pObj, iObj);
                }

                // Decrease object count by one
                vputnm( pToolData->pObjects, (nObj - 1) * (sizeof(Trl2objdst) / sizeof(int32)));

              } else { // Keep biggest object in scene

                // Keep RLCs from biggest object, remove RLCs from other objects

                rl2t_IsolateLabelRLC( pToolData->iRLC1, pObjBest);

                // Copy object data to begin of vector
                memcpy( pObjBase, pObjBest, sizeof(Trl2objdst));

                // Set object count to one
                vputnm( pToolData->pObjects, 1 * (sizeof(Trl2objdst) / sizeof(int32)));

              }
            }
          }
          break;

        case YAIPS_OBJ_POSTOP_REMOVE_SIZE_THRES:     // Remove objects according size thresholds

          {
            int iObj, iDst, nObj, ThresMinSize, ThresMaxSize;
            Trl2objdst *pObjBase, *pObjTemp, *pObj;

            if( WasLabeled > 0)  {           // Was labeled before

              // Need to reset the labels in the RLCs before a new labeling
              rl2t_ResetLabelRLCs( pToolData->iRLC1);
            }

            ierr = YaIPS_RGB_RLC_LabelMeas( pToolData->pObjects, pToolData->iRLC1, 0, 0);

            if( ierr != 0) {         // There was an error before
              break;
            }

            WasLabeled += 1;    // RLCS have been labeled

            // get biggest object.
            // We use the city block length to measure 'how big'.

            MaxLength = 0;

            pObjBase = (Trl2objdst *)vgetpm( pToolData->pObjects);

            nObj = (int)(vgetnm( pToolData->pObjects) / (sizeof(Trl2objdst) / sizeof(int32)));
            pObj = pObjBase;

            // Size thresholds as used by YaIPS_RGB_RLC_LabelMeas()

            ThresMinSize = pToolData->Obj_AreaMinSide * pToolData->Obj_AreaMinSide;  // Minimum area of an object to be labeled
            ThresMaxSize = pToolData->Obj_AreaMaxSide * pToolData->Obj_AreaMaxSide;  // Maximum area of an object to be labeled
            if( ThresMaxSize < ThresMinSize) {    // Max size is only used if bigger then min sizhe
              ThresMaxSize = 0;                   // Mark as not used
            }

            iDst = 0;

            for( iObj = 0; iObj < nObj; iObj++, pObj++) {

              if( pObj->area < ThresMinSize ||     // Below minimum area
                  (ThresMaxSize > 0 && pObj->area > ThresMaxSize)) {     // or above maximum area

                // Remove RLC for this object

                rl2t_RemoveLabelRLCs( pToolData->iRLC1, pObj);

                continue;
              }

              // Copy down objects which are not removed

              if( iObj == iDst) {            // no remove until now

                iDst += 1;

                continue;
              }

              // Copy down this object

              pObjTemp = pObjBase + iDst;    // Where to copy

              // Copy down in memory
              memcpy( pObjTemp, pObj, sizeof(Trl2objdst));

              iDst += 1;                     // Next destination
            }

            nObj = iDst;                     // Updated number of objects

            vputnm( pToolData->pObjects, nObj * (sizeof(Trl2objdst) / sizeof(int32)));

          }
          break;

        default:

          break;
        }
      }

      // --------------------------------------
      // Process objects
      // --------------------------------------

      vputnm( pToolData->pObjects, 0);   // Reset number of labeld objects

      if( ierr == 0 &&                   // No error until now
          pToolData->Obj_Label) {        // and labeling is on

        if( WasLabeled > 0)  {           // Was labeled before

          // Need to reset the labels in the RLCs before a new labeling
          rl2t_ResetLabelRLCs( pToolData->iRLC1);
        }

        ierr = YaIPS_RGB_RLC_LabelMeas( pToolData->pObjects, pToolData->iRLC1,
                                       pToolData->Obj_AreaMinSide * pToolData->Obj_AreaMinSide,  // In: Minimum area of an object to be labeled
                                       pToolData->Obj_AreaMaxSide * pToolData->Obj_AreaMaxSide); // In: Maximum area of an object to be labeled

        if( ierr >= 0) {     // No error

          int nObj;

          if( pToolData->Obj_ShowText == 6) {    // show average brightness

            // Get average brightness of the objects

            YaIPS_RGB_RLC_GetBright( pToolData->pObjects, pToolData->iRLC1, pImgIn1);
          }

          // Display number of objects
          nObj = (int)(vgetnm( pToolData->pObjects) / (sizeof(Trl2objdst) / sizeof(int32)));

          if( nObj <= 0) {            // No objects

            YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, LangStringLookup( "&GUI_Objects_Insp4a=--- objects"));

          } else if( nObj == 1) {     // one object

            YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, LangStringLookup( "&GUI_Objects_Insp4b=1 object"));

          } else {                         // more objects

            YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, LangStringLookup( "&GUI_Objects_Insp4c=%d objects"), nObj);
          }
        }
      }

      // --------------------------------------
      // Decode runlength code image and display it
      // --------------------------------------

      if( ierr == 0) {    // No error until now

        // Converts run length coded labeled image to grey level image.
        ierr = YaIPS_RGB_RLC_Decode( &pToolData->YaIPS_ImageDisp.pImage_Img, pToolData->iRLC1, 0, 0, 255);
      }

      // Has a valid output image

      if( ierr == 0) {                                  // Have a result image

        YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, (char *)MY_WIN_GUI_NAME);

        YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp); // Reset error message

        ForceUpdate = true;                        // Force update of output image

      } else {                                          // Processing error

        pToolData->Input1_Change = 0;                   // Force recalculation output

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
 * IqeB_GUI_FilterWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_ObjectsWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
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

  pToolData->pObjects = NULL;                              // Have no object vector

  pToolData->IsOpen = true;                                // Flag image as open

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
 * YaIPS_GUI_MyDrawAfter_Func
 *
 * Additional drawings after the image was drawn.
 *
 * Common draw after function for this tool data.
 *
 * NOTE: This is only called it the tool window is open
 * NOTE: A call to YaIPS_ImageDispCalcSizes() must be done before calling
 *       this function.
 *
 * pYaIPS_ImageDisp: point to image display data
 *                   NOTE: Is image display for this tool window or an other image display
 * pToolData:        point to tool data
 * pMyToolWin:       point to window data
 * DoClip:           if true (> 0) handle clipping of draw region
 *                   else this must be done in the calling function
 *
 */
static void YaIPS_GUI_MyDrawAfter_Func( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // point to image display data
                                        YaIPS_ToolData_info_t *pToolData,        // point to tool data
                                        int DoClip)                              // if true (> 0) handle clipping of draw region else caller must do it
{
  int x1, y1, x, y, xx, yy, xxo, yyo, OffX, OffY;
  int LineWidth, DrawObjText, TempFontSize, mdx, mdy, mw, mh, GravitySize;
  char TempString[ 256];

  if( pYaIPS_ImageDisp->pImage_Img == NULL) {                // Have no output image

    return;
  }

  // Draw Histogram first. Use by bimodal threshold modes

  if( pToolData->Obj_BinMode == YAIPS_RLC_BINMODE_GE_BM_AUTO ||     // Any of the bimodal threshold binarization modes
      pToolData->Obj_BinMode == YAIPS_RLC_BINMODE_L_BM_AUTO) {

    if( YaIPS_RGB_RLC_BM_Histo.nHistos >= 1)  {  // Expect one or two histograms

      // Set vertical marker lines
      YaIPS_RGB_RLC_BM_Histo.MarkPos1 = YaIPS_RGB_RLC_BM_Thres;
      YaIPS_RGB_RLC_BM_Histo.MarkCol1 = 219;

      YaIPS_RGB_RLC_BM_Histo.MarkPos2 = YaIPS_RGB_RLC_BM_Max1;
      YaIPS_RGB_RLC_BM_Histo.MarkCol2 = 4;

      YaIPS_RGB_RLC_BM_Histo.MarkPos3 = YaIPS_RGB_RLC_BM_Max2;
      YaIPS_RGB_RLC_BM_Histo.MarkCol3 = 4;

      // Draw the histogram
      YaIPS_Histo_Draw( pYaIPS_ImageDisp->pImage_Box,  // Draw into this box widget
                        &YaIPS_RGB_RLC_BM_Histo,       // Pointer to RGB histogram
                        8, 8,                          // Left upper reference point for drawing
                        85 /*YAIPS_HISTO_N_POINTS / 2*/,  // Height of curves
                        NULL, NULL,                    // Optional header text
                        9, 13, 10);                    // Optional color overwrites
    }
  }

  // If we have no objects we have nothing to draw

  if( pToolData->pObjects == NULL ||
      vgetnm( pToolData->pObjects) <= 0) {

    return;
  }

  // Colors
  //x/Fl_Color ColFrameErr = fl_rgb_color( 255, 64, 64);
  Fl_Color ColFrameOK = fl_rgb_color( 0, 188, 255); //x/fl_rgb_color( 255, 255, 0);
  //x/Fl_Color ColTemp;

  // Preparations

  x1 = pYaIPS_ImageDisp->BigImage_sx;
  y1 = pYaIPS_ImageDisp->BigImage_sy;
  xx = pYaIPS_ImageDisp->BigImage_sw;
  yy = pYaIPS_ImageDisp->BigImage_sh;

  // Points relative to image

  OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
  OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

  // Clipping ?

  if( DoClip > 0) {      // The the draw clipping

    DoClip = -1;         // Need to pop clipping

    fl_push_clip( x1, y1, xx, yy);
  }

  LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

  // ...

  // Prepare font size

  DrawObjText = false;                                             // Preset, do not draw object text
  TempFontSize = 10;
  GravitySize = 4;                                                 // Is used do draw the center of gravity

  if( pYaIPS_ImageDisp->PixelImageToScreen >= 0.66) {              // Is NOT to tiny

    DrawObjText = true;                                            // Draw object text

    TempFontSize = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 12.0 + 0.5);

    if( TempFontSize < 10) {
      TempFontSize = 10;
    }

    fl_font( FL_HELVETICA, TempFontSize);

    GravitySize = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 4.0 + 0.5);
    if( TempFontSize < 4) {
      TempFontSize = 4;
    }
  }

  // ...

  fl_line_style( 0, LineWidth);   // Set line width

  // Draw objects in scene
  if( pToolData->pObjects != NULL) {

    int nSceneObj, iSceneObj;
    Trl2objdst *pSceneObj;

    nSceneObj = (int)(vgetnm( pToolData->pObjects) / (sizeof(Trl2objdst) / sizeof(int32)));

    pSceneObj  = (Trl2objdst *)vgetpm( pToolData->pObjects);

    for( iSceneObj = 0; iSceneObj < nSceneObj; iSceneObj++, pSceneObj++) {

      xxo = pSceneObj->xmax - pSceneObj->xmin + 1;
      yyo = pSceneObj->ymax - pSceneObj->ymin + 1;

      xxo = (int)( xxo * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
      yyo = (int)( yyo * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

      // Relative to AOI
      x = (int)( (pSceneObj->xmin - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
      y = (int)( (pSceneObj->ymin - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

      fl_color( ColFrameOK);
      fl_rect( x1 + x, y1 + y, xxo, yyo);

      // Draw label and quality of correlation to screen

      if( DrawObjText) {                                               // Draw object text

        switch( pToolData->Obj_ShowText) {                  // Show area

        default:  // off
          sprintf( TempString, "%d", iSceneObj + 1);
          break;

        case 1: // show size in pixel²
          sprintf( TempString, "%dx%d P", pSceneObj->xmax - pSceneObj->xmin + 1, pSceneObj->ymax - pSceneObj->ymin + 1);
          break;

        case 2: // show size in mm²
          sprintf( TempString, "%.1fx%.1f mm",
                                (pSceneObj->xmax - pSceneObj->xmin + 1) * YaIPS_Calib_UPP_X,
                                (pSceneObj->ymax - pSceneObj->ymin + 1) * YaIPS_Calib_UPP_Y);
          break;

        case 3: // show area in pixel²
          sprintf( TempString, "%ld P²", pSceneObj->area);
          break;

        case 4: // show area in mm²
          sprintf( TempString, "%.1f mm²", pSceneObj->area * YaIPS_Calib_UPP_X * YaIPS_Calib_UPP_Y);
          break;

        case 5: // draw center of gravity

          TempString[ 0] = '\0';     // No text

          // Relative to AOI
          xxo = (int)( (((pSceneObj->xgrav + 8) >> 4) - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
          yyo = (int)( (((pSceneObj->ygrav + 8) >> 4) - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

          xxo += x1;
          yyo += y1;

          fl_line( xxo + GravitySize, yyo + GravitySize, xxo - GravitySize, yyo - GravitySize);
          fl_line( xxo - GravitySize, yyo + GravitySize, xxo + GravitySize, yyo - GravitySize);
          break;

        case 6: // show average brightness

          if( (pSceneObj->ExData1 & 0x01000000) != 0) {  // Have color

            sprintf( TempString, "%d/%d/%d", (int)pSceneObj->ExData1 & 0xff, ((int)pSceneObj->ExData1 >> 8) & 0xff,
                                             ((int)pSceneObj->ExData1 >> 16) & 0xff);

          } else {                                       // Have a black white value

            sprintf( TempString, "%d", (int)pSceneObj->ExData1 & 0xff);
          }
          break;
        }

        if( TempString[ 0] != '\0') {       // Have something to draw

          fl_text_extents( TempString, mdx, mdy, mw, mh);

          if( y + mdy < 4 /*||         // To near to upper border
              (mw + 6 < xxo && mh + 8 < yyo)*/) { // or fits into rectangle


            y += mh + 5;             // Show below upper frame
            x += 4;

          } else {                   // Fits above upper frame

            y -= 4;
          }

          fl_draw( TempString, x1 + x, y1 + y);
        }
      }
    }
  }

  // Finish up

  fl_line_style( 0);   // Reset to default

  if( DoClip < 0) {      // Need to pop clipping

    fl_pop_clip();
  }
}

/************************************************************************************
 * YaIPS_GUI_MyDrawAfter_Other
 *
 * Additional drawings after the image was drawn.
 *
 * Draw after function for image display of other window.
 *
 * NOTE: A call to YaIPS_ImageDispCalcSizes() must be done before calling
 *       this function.
 *
 * pYaIPS_ImageDisp: point to image display data
 *                   NOTE: Is image display for this tool window or an other image display
 * SubWinIDx:        What sub window to use
 */
static void YaIPS_GUI_MyDrawAfter_Other( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // point to image display data
                                         int SubWinIDx)                           // What sub window to use

{
  YaIPS_ToolData_info_t *pToolData;

  pToolData = YaIPS_ToolData_info + SubWinIDx;               // Point to info data

  YaIPS_GUI_MyDrawAfter_Func( pYaIPS_ImageDisp, pToolData, false);
}

/************************************************************************************
 * YaIPS_GUI_MyDrawAfter_cb
 *
 * Additional drawings after the image was drawn.
 *
 * Draw after function for image display of this tool window.
 *
 */
static void YaIPS_GUI_MyDrawAfter_cb( Fl_Widget *pW,
                                      void *pArg1,        // Pointer to Fl_YaIPS_ImageDisp_t
                                      void *pArg2)        // Optional pointer to ToolData
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  YaIPS_ToolData_info_t *pToolData;
  int ierr;

  pYaIPS_ImageDisp = (Fl_YaIPS_ImageDisp_t *)pArg1;

  ierr = YaIPS_ImageDispCalcSizes( pYaIPS_ImageDisp);   // Check sizes

  if( ierr < 0) {                          // No image box (no drawing area)

    return;                                // Return to caller
  }

  // Get pointer to tool data

  pToolData = (YaIPS_ToolData_info_t *)pArg2;          // Need pointer to tool data
  if( pToolData == NULL) {                             // Security test, have a pointer

    return;
  }

  YaIPS_GUI_MyDrawAfter_Func( pYaIPS_ImageDisp, pToolData, true);  // Additional drawings after the image was drawn

  YaIPS_ImageDispDrawAfter_Common( pYaIPS_ImageDisp, 0, true);                 // Draw additional common drawings
}

/************************************************************************************
 * IqeB_GUI_ObjectsWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_ObjectsWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

    // Register draw after function for big image display
    YaIPS_ToolWinDrawAfterSet( MY_WIN_ID + SubWinIDx, YaIPS_GUI_MyDrawAfter_Other);

    IqeB_GUI_ObjectsWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  // Register draw after function for big image display
  YaIPS_ToolWinDrawAfterSet( MY_WIN_ID + iUnused, YaIPS_GUI_MyDrawAfter_Other);

  IqeB_GUI_ObjectsWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


