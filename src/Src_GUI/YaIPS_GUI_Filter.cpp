/****************************************************************************

  YaIPS_GUI_Filter.cpp

  Filter an image.

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
#define MY_WIN_ID     YAIPS_WIN_ID_FILTER       // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_FILTER        // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_Filter_Title=Filter"              // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinFilter"            // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_Filter_Tool   // Use this as class name for the window class

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
  int IsOpen;                           // True if this window is open.
  void *pMyToolWin;                     // Pointer to window data ( is pointer to CLASS_WIN_TOOL)

  int MyWinPosX, MyWinPosY;             // last window position
  int MyWinSizeX, MyWinSizeY;           // last window size

  int Input1_WinIdNr;                   // Window ID nr of 1. input

  // Used for intern data management

  Fl_YaIPS_ImageDisp_t YaIPS_ImageDisp;   // Info image output

  // Used to catch a change of the input image
  int Input1_Change;                    // Last processed 'ImageChanged' from input image

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int Tab_Group_Selected;               // Number of last selected tab group.
  int FilterType;                       // What filter to use
  int LowpassKSize;                     // Kernel size for lowpass filters
  float EdgeResMult;                    // Result multiplier
  int EdgeOffset;                       // EgeFilter: Add Offset to edge filter output. Range is 0 ... 255.
  int EdgeAbsRes;                       // EgeFilter: If true, edge filter have an absolute result
  float CannyResMult;                   // Canny: Result multiplier
  float CannySigma;                     // Canny: Sigma for gauss filter
  int CannyMode;                        // Canny: Processing mode for canny filter
  int MorphRuns;                        // Morphology: number of runs
  int ContourPlaneThres;                // Contour: Plane threshold
  int SharpnessRadius;                  // Sharpness: The number of pixels that are taken into account around each edge.
  int SharpnessStrengthPercent;         // Sharpness: The amount by which the contrast of the pixels in the image is increased [%].
  int SharpnessDifference;              // Sharpness: The brightness difference between neighboring pixels so that they are adjusted.
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

  { PREF_T_INT,    "FilterType",      "0", &YaIPS_ToolData_info[0].FilterType},
  { PREF_T_INT,  "LowpassKSize",      "3", &YaIPS_ToolData_info[0].LowpassKSize},
  { PREF_T_FLOAT, "EdgeResMult",    "1.0", &YaIPS_ToolData_info[0].EdgeResMult},
  { PREF_T_INT,    "EdgeOffset",      "0", &YaIPS_ToolData_info[0].EdgeOffset},
  { PREF_T_INT,    "EdgeAbsRes",      "0", &YaIPS_ToolData_info[0].EdgeAbsRes},
  { PREF_T_FLOAT,"CannyResMult",    "1.0", &YaIPS_ToolData_info[0].CannyResMult},
  { PREF_T_FLOAT,  "CannySigma",    "0.0", &YaIPS_ToolData_info[0].CannySigma},
  { PREF_T_INT,     "CannyMode",      "0", &YaIPS_ToolData_info[0].CannyMode},
  { PREF_T_INT,               "MorphRuns",      "1", &YaIPS_ToolData_info[0].MorphRuns},
  { PREF_T_INT,        "ContourPlaneThres",    "16", &YaIPS_ToolData_info[0].ContourPlaneThres},
  { PREF_T_INT,          "SharpnessRadius",     "3", &YaIPS_ToolData_info[0].SharpnessRadius},
  { PREF_T_INT, "SharpnessStrengthPercent",   "100", &YaIPS_ToolData_info[0].SharpnessStrengthPercent},
  { PREF_T_INT,      "SharpnessDifference",    "10", &YaIPS_ToolData_info[0].SharpnessDifference},

};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_FilterWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp);

//-----------------------------------------------------------------------------------
// Parameter dialog
//
// This is a modal dialog. Therefore we can use global variables to hold
// info about the data.
//-----------------------------------------------------------------------------------

// defines for filter

#define YAIPS_FILTER_LPASS_GAUSS_X       0    // Lowpass gauss horizontal
#define YAIPS_FILTER_LPASS_GAUSS_Y       1    // Lowpass gauss vertical
#define YAIPS_FILTER_LPASS_GAUSS_XY      2    // Lowpass gauss
#define YAIPS_FILTER_LPASS_BOX_X         3    // Lowpass box horizontal
#define YAIPS_FILTER_LPASS_BOX_Y         4    // Lowpass box vertical
#define YAIPS_FILTER_LPASS_BOX_XY        5    // Lowpass box
#define YAIPS_FILTER_EDGE_DIR_HOR        6    // Edge difference
#define YAIPS_FILTER_EDGE_DIR_VER        7    // Edge difference
#define YAIPS_FILTER_EDGE_DIR_DIAG1      8    // Edge difference
#define YAIPS_FILTER_EDGE_DIR_DIAG2      9    // Edge difference
#define YAIPS_FILTER_EDGE_LAPLACE       10    // Edge filter
#define YAIPS_FILTER_EDGE_SOBEL         11    // Edge filter
#define YAIPS_FILTER_EDGE_CANNY         12    // Edge filter
#define YAIPS_FILTER_MORPH_EROSION      13    // Morphology filter
#define YAIPS_FILTER_MORPH_DILATION     14    // Morphology filter
#define YAIPS_FILTER_MORPH_MEDIAN       15    // Morphology filter
#define YAIPS_FILTER_MORPH_CLOSING      16    // Morphology filter
#define YAIPS_FILTER_MORPH_OPENING      17    // Morphology filter
#define YAIPS_FILTER_MORPH_GRADIENT     18    // Morphology filter
#define YAIPS_FILTER_CONTOUR_CONTOUR    19    // Set 255 for contour, 0 for plane
#define YAIPS_FILTER_CONTOUR_PLANE      20    // Set 0 for contour, 255 for plane
#define YAIPS_FILTER_CONTOUR_LOWPASS    21    // Lowpass for plane pixels
#define YAIPS_FILTER_SHARPNESS          22    // Image sharpness

#define YAIPS_FILTER_BUTTON_MAX    (YAIPS_FILTER_SHARPNESS + 1)  // Number of filter radio buttons

// ...

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolData;     // NOTE: Is used by all parameter dialog functions

static IqeFl_Tabs      *pTab_Groups;         // Point to tabulator GUI element
static Fl_Radio_Round_Button *FilterButtons[ YAIPS_FILTER_BUTTON_MAX]; // Table of filter buttons
static int FilterType_Last;                  // Catch filter change

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

  // Update all buttons
  if( FilterType_Last != pToolData->FilterType) {                // Filter type has change

    FilterType_Last = pToolData->FilterType;

    for( i = 0; i < YAIPS_FILTER_BUTTON_MAX; i++) {

      ValThis = FilterButtons[ i]->value();

      if( ValThis != (i == pToolData->FilterType)) {             // Not what we expected

        FilterButtons[ i]->value( i == pToolData->FilterType);   // Set value
        FilterButtons[ i]->redraw();
      }
    }
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
 * YaIPS_Filter_Callback
 *
 * Filter will change
 */

static void YaIPS_Filter_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                       // get value to set

  if( pToolData->FilterType == Value) {            // Value will not change

    return;                                        // Exit, nothing to do
  }

  pToolData->FilterType = Value;                   // Set new value

  pToolData->Input1_Change = 0;                    // Force recalculation output
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

  pToolData->Input1_Change = 0;                    // Force recalculation output
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

  pMyParWin = new Fl_Window( xPos, yPos, 297 /*IQE_GUI_TOOLS_STD_WITDH*/, 142 /* 162 */, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  FilterType_Last = -1;         // Reset last filter type

  //
  //  GUI things
  //

  int x1, y, yy, xx2;
  //x/int xx1, xc;
  int yGroup;
  //x/char TempBuffer[ 256];

  Fl_Check_Button *pCheckTemp;
  Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  IqeFl_Float_Input  *pFloatTemp;
  IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group;
  //x/Fl_Button       *pTemp_Button;
  //x/Fl_Choice       *pTemp_Choice;
  Fl_Radio_Round_Button *pRadioButTemp;

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
  // Group 'Tiefpass'
  //

  yGroup = y;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Filter_TabA1=Low-pass"));

    y += 8;

    xx2 = 80;

    pTemp_Box = new Fl_Box( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabA2=Gaussian:"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    x1 += xx2;

    xx2 = 42;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "@+1line2");
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabA2a=X/horizontal Gaussian filter"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_LPASS_GAUSS_X);
    FilterButtons[ YAIPS_FILTER_LPASS_GAUSS_X] = pRadioButTemp;

    x1 += xx2;
    x1 += 16;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "@+12line2");
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabA3a=Y/vertical Gaussian filter"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_LPASS_GAUSS_Y);
    FilterButtons[ YAIPS_FILTER_LPASS_GAUSS_Y] = pRadioButTemp;

    x1 += xx2;
    x1 += 16;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "@+1cross2");
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabA4a=XY Gaussian filter"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_LPASS_GAUSS_XY);
    FilterButtons[ YAIPS_FILTER_LPASS_GAUSS_XY] = pRadioButTemp;

    x1 += xx2;
    x1 += 16;

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 80;

    pTemp_Box = new Fl_Box( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabA5=Box:"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    x1 += xx2;

    xx2 = 42;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "@+1line2");
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabA5a=X/horizontal Box filter"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_LPASS_BOX_X);
    FilterButtons[ YAIPS_FILTER_LPASS_BOX_X] = pRadioButTemp;

    x1 += xx2;
    x1 += 16;

    xx2 = 42;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "@+12line2");
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabA6a=Y/vertical Box filter"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_LPASS_BOX_Y);
    FilterButtons[ YAIPS_FILTER_LPASS_BOX_Y] = pRadioButTemp;

    x1 += xx2;
    x1 += 16;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "@+1cross2");
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabA7a=XY Box filter"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_LPASS_BOX_XY);
    FilterButtons[ YAIPS_FILTER_LPASS_BOX_XY] = pRadioButTemp;

    x1 += xx2;
    x1 += 16;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 76;
    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Filter_TabA8=Filter size"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Filter_TabA8a=Filter size must be odd and >= 3."));
    pTemp_Int->SetValue( pToolData->LowpassKSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->LowpassKSize);
    pTemp_Int->SetModifyData( 3, 511, 2, 10);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group Kanten
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Filter_TabB1=Edges"));

    y += 8;

    xx2 = 80;

    pTemp_Box = new Fl_Box( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabB2=Difference:"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    x1 += xx2;

    xx2 = 42;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "@+1line2");
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabB2a="
                            "Difference filter horizontal\n"
                            " 1  2  1\n"
                            " 0  0  0   * G + O\n"
                            "-1 -2 -1"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_EDGE_DIR_HOR);
    FilterButtons[ YAIPS_FILTER_EDGE_DIR_HOR] = pRadioButTemp;

    x1 += xx2;

    xx2 = 50;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "@+12line2");
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabB3a=Difference filter vertical"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_EDGE_DIR_VER);
    FilterButtons[ YAIPS_FILTER_EDGE_DIR_VER] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "@+11line2");
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabB4a=Difference filter oblique"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_EDGE_DIR_DIAG1);
    FilterButtons[ YAIPS_FILTER_EDGE_DIR_DIAG1] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "@+13line2");
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabB5a=Difference filter oblique"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_EDGE_DIR_DIAG2);
    FilterButtons[ YAIPS_FILTER_EDGE_DIR_DIAG2] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 80;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabB6=Laplace"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabB6a="
                            "Laplace filter\n"
                            " 0 -1  0\n"
                            "-1  4 -1   * G + O\n"
                            " 0 -1  0"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_EDGE_LAPLACE);
    FilterButtons[ YAIPS_FILTER_EDGE_LAPLACE] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabB7=Sobel"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabB7a="
                            "Sobel filter\n"
                            " a  b  c\n"
                            " d  e  f     (  | (a+2b+c)-(g+2h+i) |\n"
                            " g  h  i      + | (a+2d+g)-(c+2f+i) | ) * G"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_EDGE_SOBEL);
    FilterButtons[ YAIPS_FILTER_EDGE_SOBEL] = pRadioButTemp;

    x1 += xx2;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 48;
    xx2 = 40;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Filter_TabB8=Gain"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Filter_TabB8a=Gain\nMultiplier of the filter result"));
    pFloatTemp->SetValue( pToolData->EdgeResMult);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->EdgeResMult);
    pFloatTemp->SetModifyData( 0.5, 8.0, 0.5, 0.1);

    x1 += xx2;
    x1 += 48;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Filter_TabB9=Offs."));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Filter_TabB9a=Offset\nAdd to the filter result"));
    pTemp_Int->SetValue( pToolData->EdgeOffset);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->EdgeOffset);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);

    x1 += xx2;
    x1 += 16;

    pCheckTemp = new Fl_Check_Button( x1, y, 50, yy, LangStringLookup( "&GUI_Filter_TabB10=Abs."));
    pCheckTemp->tooltip( LangStringLookup( "&GUI_Filter_TabB10a="
                         "When set, the edge filter\n"
                         "has an absolute result."));
    pCheckTemp->value( pToolData->EdgeAbsRes);
    pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->EdgeAbsRes);

    // Next line

    x1 = 4;
    y += yy + 3;

    // a separator line
    pTemp_Box = new Fl_Box( x1 + 4, y, pMyParWin->w() - x1 - 12, 1);
    pTemp_Box->align( FL_ALIGN_TOP | FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    pTemp_Box->box( FL_BORDER_BOX);    //  FL_BORDER_BOX FL_DOWN_FRAME

    y += 4;

    xx2 = 64;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabB11=Canny"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabB11a=Canny edge filter"));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_EDGE_CANNY);
    FilterButtons[ YAIPS_FILTER_EDGE_CANNY] = pRadioButTemp;

    x1 += xx2;

    x1 += 40;
    xx2 = 40;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Filter_TabB12=Gain"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Filter_TabB12a=Gain\nMultiplier of the filter result"));
    pFloatTemp->SetValue( pToolData->CannyResMult);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->CannyResMult);
    pFloatTemp->SetModifyData( 0.5, 16.0, 0.5, 0.1);

    x1 += xx2;
    x1 += 32;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Filter_TabB13=Sig."));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Filter_TabB13a=Sigma for gaussian filter\nRange 0.0 ... 8.0, 0 is off"));
    pFloatTemp->SetValue( pToolData->CannySigma);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->CannySigma);
    pFloatTemp->SetModifyData( 0.0, 8.0, 0.5, 0.1);

    x1 += xx2;
    x1 += 43;

    xx2 = 24;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Filter_TabB14=Mode"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Filter_TabB14a="
                        "Result mode\n"
                        "0 = Slope/inclination of the edges\n"
                        "1 = 0 and clean up maxima\n"
                        "2 = Direction of the edges\n"));
    pTemp_Int->SetValue( pToolData->CannyMode);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->CannyMode);
    pTemp_Int->SetModifyData( 0, 2, 1);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group morphology
  //

  y = yGroup;
  x1  = 4;

    pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Filter_TabC1=Morphology"));

    y += 8;

    xx2 = 95;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabC2=Erosion"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabC2a="
                            "Erosion.\n"
                            "Darkest pixel in a 3x3 neighborhood."));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_MORPH_EROSION);
    FilterButtons[ YAIPS_FILTER_MORPH_EROSION] = pRadioButTemp;

    x1 += xx2;

    xx2 = 95;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabC3=Dilation"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabC3a="
                            "Dilation.\n"
                            "Brightest pixel in a 3x3 neighborhood."));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_MORPH_DILATION);
    FilterButtons[ YAIPS_FILTER_MORPH_DILATION] = pRadioButTemp;

    x1 += xx2;

    xx2 = 95;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabC4=Median"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabC4a="
                            "Median.\n"
                            "Middle pixel of a 3x3 neighborhood."));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_MORPH_MEDIAN);
    FilterButtons[ YAIPS_FILTER_MORPH_MEDIAN] = pRadioButTemp;

    x1 += xx2;

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 95;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabC5=Closing"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabC5a="
                            "Closing.\n"
                            "Dilation followed by erosion."));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_MORPH_CLOSING);
    FilterButtons[ YAIPS_FILTER_MORPH_CLOSING] = pRadioButTemp;

    x1 += xx2;

    xx2 = 95;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabC6=Opening"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabC6a="
                            "Opening.\n"
                            "Erosion followed by dilation."));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_MORPH_OPENING);
    FilterButtons[ YAIPS_FILTER_MORPH_OPENING] = pRadioButTemp;

    x1 += xx2;

    xx2 = 95;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabC7=Gradient"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabC7a="
                            "Gradient.\n"
                            "Difference between the brightest and\n"
                            "darkest pixels in a 3x3 neighborhood."));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_MORPH_GRADIENT);
    FilterButtons[ YAIPS_FILTER_MORPH_GRADIENT] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 60;
    xx2 = 34;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Filter_TabC8=Number"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Filter_TabC8a="
                        "Number of filter passes.\n"
                        "If 0, no filtering is performed."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->MorphRuns);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->MorphRuns);
    pTemp_Int->SetModifyData( 0, 8, 1);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group contour
  //

  y = yGroup;
  x1  = 4;

    pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Filter_TabD1=Contour"));

    y += 8;

    xx2 = 95;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabD2=Contour"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabD2a="
                            "Contour, set 255 for contour, 0 for plane.\n"
                            "Set if one (|neighbour - central| > plane threshold)."));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_CONTOUR_CONTOUR);
    FilterButtons[ YAIPS_FILTER_CONTOUR_CONTOUR] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabD3=Plane"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabD3a="
                            "Plane, set 0 for contour, 255 for plane.\n"
                            "Set if all (|neighbour - central| <= plane threshold)."));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_CONTOUR_PLANE);
    FilterButtons[ YAIPS_FILTER_CONTOUR_PLANE] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabD4=LP Plane"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabD4a="
                            "Low-pass plane. Average 3x3 neighborhood\n"
                            "if all (|neighbour - central| <= plane threshold)."));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_CONTOUR_LOWPASS);
    FilterButtons[ YAIPS_FILTER_CONTOUR_LOWPASS] = pRadioButTemp;

    x1 += xx2;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 120;
    xx2 = 34;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Filter_TabD5=Plane threshold"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Filter_TabD5a="
                        "Plane threshold.\n"
                        "Separates contours from plane."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->ContourPlaneThres);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->ContourPlaneThres);
    pTemp_Int->SetModifyData( 8, 255, 8, 1);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group sharpness
  //

  y = yGroup;
  x1  = 4;

    pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Filter_TabE1=Sharp"));

    y += 8;

    xx2 = pMyParWin->w() - x1 - 8;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Filter_TabE2=Image sharpness"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Filter_TabE2a="
                                    "Image sharpen gives your images a clear, focused look,\n"
                                    "with objects standing out clearly. This adds a sense\n"
                                    "of depth and sharpness to the image. Blurry, unclear,\n"
                                    "or slightly out-of-focus images can look clearer."));
    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_FILTER_SHARPNESS);
    FilterButtons[ YAIPS_FILTER_SHARPNESS] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 40;
    x1 = pMyParWin->w() - xx2 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Filter_TabE3=Radius"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Filter_TabE3a="
                        "The number of pixels that are taken\n"
                        "into account around each edge."));
    pTemp_Int->SetValue( pToolData->SharpnessRadius);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->SharpnessRadius);
    pTemp_Int->SetModifyData( 1, 15, 1, 1);

    // Next line

    y += yy;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Filter_TabE4=Strength [%]"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Filter_TabE4a="
                        "The amount by which the contrast of the\n"
                        "pixels in the image is increased [%]."));
    pTemp_Int->SetValue( pToolData->SharpnessStrengthPercent);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->SharpnessStrengthPercent);
    pTemp_Int->SetModifyData( 0, 300, 10, 1);

    // Next line

    y += yy;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Filter_TabE5=Difference"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Filter_TabE5a="
                        "The brightness difference between neighboring\n"
                        "pixels so that they are adjusted."));
    pTemp_Int->SetValue( pToolData->SharpnessDifference);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->SharpnessDifference);
    pTemp_Int->SetModifyData( 0, 250, 10, 1);

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
    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawAfterCallback  = YaIPS_ImageDispDrawAfter_cb;  // Draw after callback
    pToolData->YaIPS_ImageDisp.pImage_Box->DrawCallbackArg1    = &pToolData->YaIPS_ImageDisp;  /// Pointer to Fl_YaIPS_ImageDisp_t
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

    int Input1_Check, Input1_ImageChanged, ForceUpdate;
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

      switch( pToolData->FilterType) {

      case YAIPS_FILTER_LPASS_GAUSS_X:
      default:

        ierr = YaIPS_RGB_GaussXY( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pToolData->LowpassKSize, YAIPS_GAUSXY_MODE_X);
        break;

      case YAIPS_FILTER_LPASS_GAUSS_Y:

        ierr = YaIPS_RGB_GaussXY( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pToolData->LowpassKSize, YAIPS_GAUSXY_MODE_Y);
        break;

      case YAIPS_FILTER_LPASS_GAUSS_XY:

        ierr = YaIPS_RGB_GaussXY( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pToolData->LowpassKSize, YAIPS_GAUSXY_MODE_XY);
        break;

      case YAIPS_FILTER_LPASS_BOX_X:

        ierr = YaIPS_RGB_BoxNxN( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pToolData->LowpassKSize, 1);
        break;

      case YAIPS_FILTER_LPASS_BOX_Y:

        ierr = YaIPS_RGB_BoxNxN( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, 1, pToolData->LowpassKSize);
        break;

      case YAIPS_FILTER_LPASS_BOX_XY:

        ierr = YaIPS_RGB_BoxNxN( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pToolData->LowpassKSize, pToolData->LowpassKSize);
        break;

      case YAIPS_FILTER_EDGE_DIR_HOR:

        ierr = YaIPS_RGB_Filt_EdgeDiff( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, YAIPS_FILTER_EDGE_DIFF_HOR,
                                pToolData->EdgeResMult, pToolData->EdgeOffset, pToolData->EdgeAbsRes);
        break;

      case YAIPS_FILTER_EDGE_DIR_VER:

        ierr = YaIPS_RGB_Filt_EdgeDiff( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, YAIPS_FILTER_EDGE_DIFF_VER,
                                pToolData->EdgeResMult, pToolData->EdgeOffset, pToolData->EdgeAbsRes);
        break;

      case YAIPS_FILTER_EDGE_DIR_DIAG1:

        ierr = YaIPS_RGB_Filt_EdgeDiff( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, YAIPS_FILTER_EDGE_DIFF_DIAG1,
                                pToolData->EdgeResMult, pToolData->EdgeOffset, pToolData->EdgeAbsRes);
        break;

      case YAIPS_FILTER_EDGE_DIR_DIAG2:

        ierr = YaIPS_RGB_Filt_EdgeDiff( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, YAIPS_FILTER_EDGE_DIFF_DIAG2,
                                pToolData->EdgeResMult, pToolData->EdgeOffset, pToolData->EdgeAbsRes);
        break;

      case YAIPS_FILTER_EDGE_LAPLACE:

        ierr = YaIPS_RGB_Laplace3x3( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                             pToolData->EdgeResMult, pToolData->EdgeOffset, pToolData->EdgeAbsRes);
        break;

      case YAIPS_FILTER_EDGE_SOBEL:

        ierr = YaIPS_RGB_Sobel3x3( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pToolData->EdgeResMult);
        break;

      case YAIPS_FILTER_EDGE_CANNY:

        switch( pToolData->CannyMode) {

        case 0:   // Slope of edges
        default:
          ierr = YaIPS_RGB_Canny( &pToolData->YaIPS_ImageDisp.pImage_Img, NULL, pImgIn1, pToolData->CannySigma, pToolData->CannyResMult, 0);
          break;

        case 1:   // Cleaned slop of edges
          ierr = YaIPS_RGB_Canny( &pToolData->YaIPS_ImageDisp.pImage_Img, NULL, pImgIn1, pToolData->CannySigma, pToolData->CannyResMult, 1);
          break;

        case 2:   // Direction of edges
          ierr = YaIPS_RGB_Canny( NULL, &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pToolData->CannySigma, pToolData->CannyResMult, 0);
          break;
        }

        break;

      case YAIPS_FILTER_MORPH_EROSION:

        ierr = YaIPS_RGB_Morphology( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, YAIPS_RGB_MORPH_EROSION, pToolData->MorphRuns);
        break;

      case YAIPS_FILTER_MORPH_DILATION:

        ierr = YaIPS_RGB_Morphology( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, YAIPS_RGB_MORPH_DILATION, pToolData->MorphRuns);
        break;

      case YAIPS_FILTER_MORPH_MEDIAN:

        ierr = YaIPS_RGB_Morphology( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, YAIPS_RGB_MORPH_MEDIAN, pToolData->MorphRuns);
        break;

      case YAIPS_FILTER_MORPH_CLOSING:

        ierr = YaIPS_RGB_Morphology( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, YAIPS_RGB_MORPH_CLOSING, pToolData->MorphRuns);
        break;

      case YAIPS_FILTER_MORPH_OPENING:

        ierr = YaIPS_RGB_Morphology( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, YAIPS_RGB_MORPH_OPENING, pToolData->MorphRuns);
        break;

      case YAIPS_FILTER_MORPH_GRADIENT:

        ierr = YaIPS_RGB_Morphology( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, YAIPS_RGB_MORPH_GRADIENT, pToolData->MorphRuns);
        break;

      case YAIPS_FILTER_CONTOUR_CONTOUR:
      case YAIPS_FILTER_CONTOUR_PLANE:
      case YAIPS_FILTER_CONTOUR_LOWPASS:

        ierr = YaIPS_RGB_Contour( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pToolData->ContourPlaneThres, pToolData->FilterType - YAIPS_FILTER_CONTOUR_CONTOUR);
        break;

      case YAIPS_FILTER_SHARPNESS:
        ierr = YaIPS_RGB_Sharpness( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pToolData->SharpnessRadius, pToolData->SharpnessStrengthPercent, pToolData->SharpnessDifference);
        break;

      } // end switch

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

static void IqeB_GUI_FilterWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
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
 * IqeB_GUI_FilterWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_FilterWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

	  IqeB_GUI_FilterWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  IqeB_GUI_FilterWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


