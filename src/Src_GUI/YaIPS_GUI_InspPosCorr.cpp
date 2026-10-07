/****************************************************************************

  YaIPS_GUI_InspPosCorr.cpp

  Position corrections.

  24.08.2025 RR: First edition of this file.

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

//x/#ifdef _DEBUG
#define USE_DEBUG_TRAFO 1 // use this to debug some transformation system things
//x/#endif

/************************************************************************************
* Defines for this source file.
*/

// Defines for windows ID
#define MY_WIN_ID     YAIPS_WIN_ID_POSCORR          // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_POSCORR         // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_PosCorr_Title=Position correction"  // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)     // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinPosCorr"              // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_PosCorr_Tool    // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       262
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
static void YaIPS_GUI_MyDrawAfter_cb( Fl_Widget *pW, void *pArg1, void *pArg2);
static int YaIPS_GUI_MyMouse_cb( Fl_Widget *pW, int event, void *pArg1, void *pArg2);

/************************************************************************************
* Global variables for this window
*/

#define MAX_OBJ_TO_LABEL    10000  // max number of objects in the image to label

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

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int Tab_Group_Selected;               // Number of last selected tab group.
  int SelectionType;                    // What selection to use

  int Teach_mode;                       // 0 = inspection mode, 1 = teach mode

  // Tab Group 'Base'

  int Base_InspCorrect;                 // Perform position correction in inspection mode
  int Base_OutsiteColor;                // Color used for areas outside an image
  int Base_OutsiteBlend;                // If set, outside area is alpha blended

  // Tab group 'Pos Edges'

  YaIPS_EdgePM_t Edge_PM;               // Edge position measurement data

  // Tab group 'Pattern search'

  YaIPS_PSearchXY_t PSearch_XY;      // Position search measurement

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
  // and other common settings

  { PREF_T_INT,   "Group_Selected",     "0", &YaIPS_ToolData_info[0].Tab_Group_Selected},
  { PREF_T_INT,    "SelectionType",     "0", &YaIPS_ToolData_info[0].SelectionType},
  { PREF_T_INT,       "Teach_mode",     "0", &YaIPS_ToolData_info[0].Teach_mode},

  // Tab group 'Base'

  { PREF_T_INT,  "Base_InspCorrect" ,   "0",  &YaIPS_ToolData_info[0].Base_InspCorrect},
  { PREF_T_INT, "Base_OutsiteColor" ,  "43",  &YaIPS_ToolData_info[0].Base_OutsiteColor},   // Default: Gray 128
  { PREF_T_INT, "Base_OutsiteBlend" ,   "0",  &YaIPS_ToolData_info[0].Base_OutsiteBlend},

  // Tab group 'Pos Edges'

  { PREF_T_INT,       "MeasureMode_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.MeasureMode},
  { PREF_T_FLOAT,         "MaxDevX_ES", "1.0", &YaIPS_ToolData_info[0].Edge_PM.MaxDevX},
  { PREF_T_FLOAT,         "MaxDevY_ES", "1.0", &YaIPS_ToolData_info[0].Edge_PM.MaxDevY},

  // 1. sub window
  // GUI Parameter
  { PREF_T_INT,           "AOI_X_1_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 0].AOI.XPos},
  { PREF_T_INT,           "AOI_Y_1_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 0].AOI.YPos},
  { PREF_T_INT,          "AOI_XX_1_ES",  "30", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 0].AOI.XSize},
  { PREF_T_INT,          "AOI_YY_1_ES",  "30", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 0].AOI.YSize},
  { PREF_T_INT,         "r2l_b2t_1_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 0].r2l_b2t},
  { PREF_T_INT,  "minPeakConPerc_1_ES",  "30", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 0].minPeakConPerc},
#ifdef use_again  // 09.07.2025 RR: Replaced nominal position by 1/2 search length
  { PREF_T_INT,          "nomPos_1_ES",  "15", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 0].nomPos},
#endif
  // teach results
  { PREF_T_INT,      "CheckError_1_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 0].CheckError},
  { PREF_T_INT,       "colorUsed_1_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 0].colorUsed},
  { PREF_T_INT,          "minMax_1_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 0].minMax},
  { PREF_T_INT,      "refPeakCon_1_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 0].refPeakCon},
  { PREF_T_FLOAT,          "peak_1_ES","-1.0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 0].refPeak},      // preset illegal

  // 2. sub window
  // GUI Parameter
  { PREF_T_INT,           "AOI_X_2_ES", "100", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 1].AOI.XPos},
  { PREF_T_INT,           "AOI_Y_2_ES", "100", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 1].AOI.YPos},
  { PREF_T_INT,          "AOI_XX_2_ES",  "30", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 1].AOI.XSize},
  { PREF_T_INT,          "AOI_YY_2_ES",  "30", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 1].AOI.YSize},
  { PREF_T_INT,         "r2l_b2t_2_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 1].r2l_b2t},
  { PREF_T_INT,  "minPeakConPerc_2_ES",  "30", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 1].minPeakConPerc},
#ifdef use_again  // 09.07.2025 RR: Replaced nominal position by 1/2 search length
  { PREF_T_INT,          "nomPos_2_ES",  "15", &YaIPS_ToolData_info[0].Edges[ 1].nomPos},
#endif
  // teach results
  { PREF_T_INT,      "CheckError_2_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 1].CheckError},
  { PREF_T_INT,       "colorUsed_2_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 1].colorUsed},
  { PREF_T_INT,          "minMax_2_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 1].minMax},
  { PREF_T_INT,      "refPeakCon_2_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 1].refPeakCon},
  { PREF_T_FLOAT,          "peak_2_ES","-1.0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 1].refPeak},      // preset illegal

  // 3. sub window
  // GUI Parameter
  { PREF_T_INT,           "AOI_X_3_ES", "200", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 2].AOI.XPos},
  { PREF_T_INT,           "AOI_Y_3_ES", "200", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 2].AOI.YPos},
  { PREF_T_INT,          "AOI_XX_3_ES",  "30", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 2].AOI.XSize},
  { PREF_T_INT,          "AOI_YY_3_ES",  "30", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 2].AOI.YSize},
  { PREF_T_INT,         "r2l_b2t_3_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 2].r2l_b2t},
  { PREF_T_INT,  "minPeakConPerc_3_ES",  "30", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 2].minPeakConPerc},
#ifdef use_again  // 09.07.2025 RR: Replaced nominal position by 1/2 search length
  { PREF_T_INT,          "nomPos_3_ES",  "15", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 2].nomPos},
#endif
  // teach results
  { PREF_T_INT,      "CheckError_3_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 2].CheckError},
  { PREF_T_INT,       "colorUsed_3_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 2].colorUsed},
  { PREF_T_INT,          "minMax_3_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 2].minMax},
  { PREF_T_INT,      "refPeakCon_3_ES",   "0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 2].refPeakCon},
  { PREF_T_FLOAT,          "peak_3_ES","-1.0", &YaIPS_ToolData_info[0].Edge_PM.Edges[ 2].refPeak},      // preset illegal

  // Tab group 'Pattern search'

  { PREF_T_INT,       "MeasureMode_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.MeasureMode},
  { PREF_T_FLOAT,         "MaxDevX_PS", "1.0", &YaIPS_ToolData_info[0].PSearch_XY.MaxDevX},
  { PREF_T_FLOAT,         "MaxDevY_PS", "1.0", &YaIPS_ToolData_info[0].PSearch_XY.MaxDevY},

  // 1. sub window
  // GUI Parameter
  { PREF_T_INT,           "AOI_X_1_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 0].AOI.XPos},
  { PREF_T_INT,           "AOI_Y_1_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 0].AOI.YPos},
  { PREF_T_INT,          "AOI_XX_1_PS",  "30", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 0].AOI.XSize},
  { PREF_T_INT,          "AOI_YY_1_PS",  "30", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 0].AOI.YSize},
  { PREF_T_FLOAT,       "PosTolX_1_PS", "1.0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 0].PosToleranceX},
  { PREF_T_FLOAT,       "PosTolY_1_PS", "1.0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 0].PosToleranceY},
  // teach results
  { PREF_T_INT,           "Flags_1_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 0].Flags},
  { PREF_T_INT,        "CorrNorm_1_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 0].CorrNorm},
  { PREF_T_INT,       "colorUsed_1_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 0].colorUsed},
  { PREF_T_INT,      "CheckError_1_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 0].CheckError},

  // 2. sub window
  // GUI Parameter
  { PREF_T_INT,           "AOI_X_2_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 1].AOI.XPos},
  { PREF_T_INT,           "AOI_Y_2_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 1].AOI.YPos},
  { PREF_T_INT,          "AOI_XX_2_PS",  "30", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 1].AOI.XSize},
  { PREF_T_INT,          "AOI_YY_2_PS",  "30", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 1].AOI.YSize},
  { PREF_T_FLOAT,       "PosTolX_2_PS", "1.0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 1].PosToleranceX},
  { PREF_T_FLOAT,       "PosTolY_2_PS", "1.0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 1].PosToleranceY},
  // teach results
  { PREF_T_INT,           "Flags_2_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 1].Flags},
  { PREF_T_INT,        "CorrNorm_2_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 1].CorrNorm},
  { PREF_T_INT,       "colorUsed_2_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 1].colorUsed},
  { PREF_T_INT,      "CheckError_2_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 1].CheckError},

  // 3. sub window
  // GUI Parameter
  { PREF_T_INT,           "AOI_X_3_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 2].AOI.XPos},
  { PREF_T_INT,           "AOI_Y_3_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 2].AOI.YPos},
  { PREF_T_INT,          "AOI_XX_3_PS",  "30", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 2].AOI.XSize},
  { PREF_T_INT,          "AOI_YY_3_PS",  "30", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 2].AOI.YSize},
  { PREF_T_FLOAT,       "PosTolX_3_PS", "1.0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 2].PosToleranceX},
  { PREF_T_FLOAT,       "PosTolY_3_PS", "1.0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 2].PosToleranceY},
  // teach results
  { PREF_T_INT,           "Flags_3_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 2].Flags},
  { PREF_T_INT,        "CorrNorm_3_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 2].CorrNorm},
  { PREF_T_INT,       "colorUsed_3_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 2].colorUsed},
  { PREF_T_INT,      "CheckError_3_PS",   "0", &YaIPS_ToolData_info[0].PSearch_XY.AOI[ 2].CheckError},
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_PosCorrWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp);

//-----------------------------------------------------------------------------------
// Parameter dialog
//
// This is a modal dialog. Therefore we can use global variables to hold
// info about the data.
//-----------------------------------------------------------------------------------

// defines for function selection

#define YAIPS_SELECTION_EDGES_FIRST     0                       // First position correction edges
// Used YAIPS_EDGEPM_MODE_xxx defines as values for position correction with edges
#define YAIPS_SELECTION_EDGES_LAST  (YAIPS_EDGEPM_MODE_MAX - 1)  // Last position correction edges

#define YAIPS_SELECTION_PSEAR_FIRST  (YAIPS_SELECTION_EDGES_LAST + 1)  // First position correction crcdf
#define YAIPS_SELECTION_PSEAR_1XY    (YAIPS_SELECTION_PSEAR_FIRST + YAIPS_PSEARCH_MODE_1XY)  // Position correction with search pattern, 1 XY sub-window
#define YAIPS_SELECTION_PSEAR_3XY    (YAIPS_SELECTION_PSEAR_FIRST + YAIPS_PSEARCH_MODE_3XY)  // Position correction with search pattern, 3 XY sub-windows
#define YAIPS_SELECTION_PSEAR_LAST   (YAIPS_SELECTION_PSEAR_FIRST + YAIPS_PSEARCH_MODE_MAX - 1)  // First position correction crcdf

#define YAIPS_SELECTION_BUTTON_MAX   (YAIPS_SELECTION_PSEAR_LAST + 1)  // Number of selection radio buttons

// ...

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolData;     // NOTE: Is used by all parameter dialog functions

static IqeFl_Tabs      *pTab_Groups;         // Point to tabulator GUI element
static Fl_Radio_Round_Button *SelectionButtons[ YAIPS_SELECTION_BUTTON_MAX]; // Table of filter buttons
static int SelectionType_Last;                  // Catch filter change

// Tab group 'Base'

static Fl_Button *pBaseColor;
static Fl_Check_Button *pBaseBlend;

// Tab group 'Pos Edges'

static IqeFl_Int_Input  *pES_AOI_X[ YAIPS_EDGEPM_MAX_AOIS],  *pES_AOI_Y[ YAIPS_EDGEPM_MAX_AOIS];
static IqeFl_Int_Input *pES_AOI_XX[ YAIPS_EDGEPM_MAX_AOIS], *pES_AOI_YY[ YAIPS_EDGEPM_MAX_AOIS];
static Fl_Button       *pES_TeachBut;
static Fl_Check_Button *pR2l_b2t_1, *pR2l_b2t_2, *pR2l_b2t_3;
static IqeFl_Int_Input *pminPeak_1, *pminPeak_2, *pminPeak_3;

// Tab group 'Pattern search'

static IqeFl_Int_Input   *pPS_AOI_X[ YAIPS_EDGEPM_MAX_AOIS],   *pPS_AOI_Y[ YAIPS_EDGEPM_MAX_AOIS];
static IqeFl_Int_Input   *pPS_AOI_XX[ YAIPS_EDGEPM_MAX_AOIS],  *pPS_AOI_YY[ YAIPS_EDGEPM_MAX_AOIS];
static IqeFl_Float_Input *pPS_SearchX[ YAIPS_EDGEPM_MAX_AOIS], *pPS_SearchY[ YAIPS_EDGEPM_MAX_AOIS];
static Fl_Button       *pPS_TeachBut;

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{
  Fl_RGB_Image *pImgIn1;
  int ValThis, RedrawOnExit;
  int i, TempEnable, TempEnable2, nAOIs;
  int ImgXX, ImgYY;
  Fl_Widget *pCurrFocus;

  RedrawOnExit = false;

  pCurrFocus = Fl::focus();

  // Get last selected tab group

  pToolData->Tab_Group_Selected = pTab_Groups->GetTabGroup();

  // Update filter button
  // Current selection must be set

  // Update all buttons
  if( SelectionType_Last != pToolData->SelectionType) {                // Filter type has change

    SelectionType_Last = pToolData->SelectionType;

    for( i = 0; i < YAIPS_SELECTION_BUTTON_MAX; i++) {

      ValThis = SelectionButtons[ i]->value();

      if( ValThis != (i == pToolData->SelectionType)) {             // Not what we expected

        SelectionButtons[ i]->value( i == pToolData->SelectionType);   // Set value
        SelectionButtons[ i]->redraw();
      }
    }
  }

  // Ensure section type dependent settings are up to data

  if( pToolData->SelectionType >= YAIPS_SELECTION_EDGES_FIRST &&  // One of the edge search modes
      pToolData->SelectionType <= YAIPS_SELECTION_EDGES_LAST) {

    pToolData->Edge_PM.MeasureMode = pToolData->SelectionType - YAIPS_SELECTION_EDGES_FIRST;  // Also update

  } else if( pToolData->SelectionType >= YAIPS_SELECTION_PSEAR_FIRST &&  // One of the pattern search modes
      pToolData->SelectionType <= YAIPS_SELECTION_PSEAR_LAST) {

    pToolData->PSearch_XY.MeasureMode = pToolData->SelectionType - YAIPS_SELECTION_PSEAR_FIRST;  // Also update
  }

  //
  // Enable Tab group 'Base'
  //

  TempEnable = pToolData->Base_InspCorrect;    // Perform position correction

  IqeB_GUI_WidgetActivate( pBaseColor, TempEnable);
  IqeB_GUI_WidgetActivate( pBaseBlend, TempEnable);

  //
  // Enable group 'Pos Edges'
  //

  TempEnable = pToolData->SelectionType >= YAIPS_SELECTION_EDGES_FIRST &&  // One of the edge search modes
               pToolData->SelectionType <= YAIPS_SELECTION_EDGES_LAST;

  IqeB_GUI_WidgetActivate( pES_TeachBut, TempEnable);
  IqeB_GUI_WidgetLabelColor( pES_TeachBut, pToolData->Teach_mode ? FL_GREEN : YAIPS_BCOL_BUTTON);

  TempEnable = TempEnable && pToolData->Teach_mode;    // Teach mode

  IqeB_GUI_WidgetActivate( pES_AOI_X[ 0], TempEnable);
  IqeB_GUI_WidgetActivate( pES_AOI_Y[ 0], TempEnable);
  IqeB_GUI_WidgetActivate( pES_AOI_XX[ 0], TempEnable);
  IqeB_GUI_WidgetActivate( pES_AOI_YY[ 0], TempEnable);
  IqeB_GUI_WidgetActivate( pR2l_b2t_1, TempEnable);
  IqeB_GUI_WidgetActivate( pminPeak_1, TempEnable);

  TempEnable2 = TempEnable && pToolData->Edge_PM.MeasureMode >= YAIPS_EDGEPM_MODE_XY;  // Use two ore more AOIs

  IqeB_GUI_WidgetActivate( pES_AOI_X[ 1], TempEnable2);
  IqeB_GUI_WidgetActivate( pES_AOI_Y[ 1], TempEnable2);
  IqeB_GUI_WidgetActivate( pES_AOI_XX[ 1], TempEnable2);
  IqeB_GUI_WidgetActivate( pES_AOI_YY[ 1], TempEnable2);
  IqeB_GUI_WidgetActivate( pR2l_b2t_2, TempEnable2);
  IqeB_GUI_WidgetActivate( pminPeak_2, TempEnable2);

  TempEnable2 = TempEnable && pToolData->Edge_PM.MeasureMode >= YAIPS_EDGEPM_MODE_XYY;  // Use three ore more AOIs

  IqeB_GUI_WidgetActivate( pES_AOI_X[ 2], TempEnable2);
  IqeB_GUI_WidgetActivate( pES_AOI_Y[ 2], TempEnable2);
  IqeB_GUI_WidgetActivate( pES_AOI_XX[ 2], TempEnable2);
  IqeB_GUI_WidgetActivate( pES_AOI_YY[ 2], TempEnable2);
  IqeB_GUI_WidgetActivate( pR2l_b2t_3, TempEnable2);
  IqeB_GUI_WidgetActivate( pminPeak_3, TempEnable2);

  //
  // Enable Tab group 'Pattern search'
  //

  TempEnable = pToolData->SelectionType >= YAIPS_SELECTION_PSEAR_FIRST &&  // One of the pattern search modes
               pToolData->SelectionType <= YAIPS_SELECTION_PSEAR_LAST;

  IqeB_GUI_WidgetActivate( pPS_TeachBut, TempEnable);
  IqeB_GUI_WidgetLabelColor( pPS_TeachBut, pToolData->Teach_mode ? FL_GREEN : YAIPS_BCOL_BUTTON);

  TempEnable = TempEnable && pToolData->Teach_mode;    // Teach mode

  IqeB_GUI_WidgetActivate( pPS_AOI_X[ 0], TempEnable);
  IqeB_GUI_WidgetActivate( pPS_AOI_Y[ 0], TempEnable);
  IqeB_GUI_WidgetActivate( pPS_AOI_XX[ 0], TempEnable);
  IqeB_GUI_WidgetActivate( pPS_AOI_YY[ 0], TempEnable);
  IqeB_GUI_WidgetActivate( pPS_SearchX[ 0], TempEnable);
  IqeB_GUI_WidgetActivate( pPS_SearchY[ 0], TempEnable);

  TempEnable2 = TempEnable && pToolData->PSearch_XY.MeasureMode >= YAIPS_PSEARCH_MODE_3XY;  // Use two ore more AOIs

  IqeB_GUI_WidgetActivate( pPS_AOI_X[ 1], TempEnable2);
  IqeB_GUI_WidgetActivate( pPS_AOI_Y[ 1], TempEnable2);
  IqeB_GUI_WidgetActivate( pPS_AOI_XX[ 1], TempEnable2);
  IqeB_GUI_WidgetActivate( pPS_AOI_YY[ 1], TempEnable2);
  IqeB_GUI_WidgetActivate( pPS_SearchX[ 1], TempEnable2);
  IqeB_GUI_WidgetActivate( pPS_SearchY[ 1], TempEnable2);

  IqeB_GUI_WidgetActivate( pPS_AOI_X[ 2], TempEnable2);
  IqeB_GUI_WidgetActivate( pPS_AOI_Y[ 2], TempEnable2);
  IqeB_GUI_WidgetActivate( pPS_AOI_XX[ 2], TempEnable2);
  IqeB_GUI_WidgetActivate( pPS_AOI_YY[ 2], TempEnable2);
  IqeB_GUI_WidgetActivate( pPS_SearchX[ 2], TempEnable2);
  IqeB_GUI_WidgetActivate( pPS_SearchY[ 2], TempEnable2);

  //
  // Check Scene input
  //

  pImgIn1 = NULL;       // Will be set if there is a valid scene image

  YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Input1_WinIdNr,
                           NULL, &pImgIn1, NULL);

  if( pImgIn1 != NULL) {

    ImgXX = pImgIn1->w();
    ImgYY = pImgIn1->h();

  } else {

    ImgXX = 1024;
    ImgYY = 1024;
  }

  if( pToolData->SelectionType >= YAIPS_SELECTION_EDGES_FIRST &&  // One of the edge search modes
      pToolData->SelectionType <= YAIPS_SELECTION_EDGES_LAST) {

    nAOIs = (pToolData->Edge_PM.MeasureMode / 2) + 1;

    for( i = 0; i < nAOIs; i++) {

      if( pCurrFocus == pES_AOI_X[ i]  || pCurrFocus == pES_AOI_Y[ i] ||      // Input element has keyboard focus ?
          pCurrFocus == pES_AOI_XX[ i] || pCurrFocus == pES_AOI_YY[ i]) {

        continue;  // Skip test for this input elements
      }

      if( YaIPS_ImageDispAoiRectIGuiUpdate( &pToolData->Edge_PM.Edges[ i].AOI, ImgXX, ImgYY,
                                            pES_AOI_X[ i], pES_AOI_Y[ i], pES_AOI_XX[ i], pES_AOI_YY[ i]) > 0) {

        RedrawOnExit = true;                                 // Redraw on exit
      }
    }

  } else if( pToolData->SelectionType >= YAIPS_SELECTION_PSEAR_FIRST &&  // One of the pattern search modes
      pToolData->SelectionType <= YAIPS_SELECTION_PSEAR_LAST) {

    nAOIs = (pToolData->PSearch_XY.MeasureMode / 2) + 1;

    for( i = 0; i < nAOIs; i++) {

      if( pCurrFocus == pPS_AOI_X[ i]  || pCurrFocus == pPS_AOI_Y[ i] ||      // Input element has keyboard focus ?
          pCurrFocus == pPS_AOI_XX[ i] || pCurrFocus == pPS_AOI_YY[ i]) {

        continue;  // Skip test for this input elements
      }

      if( YaIPS_ImageDispAoiRectIGuiUpdate( &pToolData->PSearch_XY.AOI[ i].AOI, ImgXX, ImgYY,
                                            pPS_AOI_X[ i], pPS_AOI_Y[ i], pPS_AOI_XX[ i], pPS_AOI_YY[ i]) > 0) {

        RedrawOnExit = true;                                 // Redraw on exit
      }
    }
  }

  // Redraw

  if( RedrawOnExit) {                                           // Redraw on exit

    pToolData->Input1_Change = 0;                // Reset image change check
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
 * YaIPS_OutputImage_Callback
 *
 * Output image setting will change
 */

static void YaIPS_SelectionType_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                       // get value to set

  if( pToolData->SelectionType == Value) {         // Value will not change

    return;                                        // Exit, nothing to do
  }

  pToolData->SelectionType = Value;                // Set new value

  if( pToolData->SelectionType >= YAIPS_SELECTION_EDGES_FIRST &&  // and one of the edge search modes
      pToolData->SelectionType <= YAIPS_SELECTION_EDGES_LAST) {

    pToolData->Edge_PM.MeasureMode = Value - YAIPS_SELECTION_EDGES_FIRST;  // Also update

  } else if( pToolData->SelectionType >= YAIPS_SELECTION_PSEAR_FIRST &&  // and one of the pattern search modes
      pToolData->SelectionType <= YAIPS_SELECTION_PSEAR_LAST) {

    pToolData->PSearch_XY.MeasureMode = Value - YAIPS_SELECTION_PSEAR_FIRST;  // Also update
  }

  // Try to redisplay changed output without new image correlation

  pToolData->Input1_Change = 0;                    // Force recalculation output
  pToolData->Input2_Change = 0;                    // Force recalculation output
}

/************************************************************************************
 * YaIPS_BigImageOverlay_Callbac
 *
 * Image overlay radiobutton was pressed
 */

static void IqeB_GUI_But_Color_Callback( Fl_Widget *w, void *pValueArg)
{
  Fl_Button *b = (Fl_Button*)w;
  int *pColor = (int *)pValueArg;
  int ColorBefore;

  // ...

  ColorBefore = b->color();

  *pColor = IqeB_GUI_ColorChooser( *pColor);
  b->color( *pColor);
  b->parent()->redraw();

  if( ColorBefore != *pColor)  {   // Color has changed

    // Update display

    pToolData->Input1_Change = 0;                    // Force recalculation output
    pToolData->Input2_Change = 0;                    // Force recalculation output
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

      pThis->SetValue( Value);             // Update on GUI
    }
  }

  *pValue = Value;                         // update the variable

  // Force recalculation of processing

  pToolData->Input1_Change = 0;          // Force recalculation output
  pToolData->Input2_Change = 0;          // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_Misc_SetValue_Callback
 *
 * This is usable for Fl_Valuator, Fl_Choice, Fl_Check_Button
 */

static void IqeB_GUI_Misc_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  int *pValue;

  pValue = (int *)pValueArg;             // get pointer to associated variable

  if( w == NULL ||                       // security test
      pValue == NULL) {

    return;
  }

  if( pValueArg == &pToolData->Teach_mode) {  // Toggle Teach / Inspection button

    pToolData->Teach_mode = ! pToolData->Teach_mode;

  } else {

    Fl_Button *pThis;

    pThis  = (Fl_Check_Button *)w;
    *pValue = pThis->value();                // update the variable
  }

  pToolData->Input1_Change = 0;          // Force recalculation output
  pToolData->Input2_Change = 0;                    // Force recalculation output
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

  pMyParWin = new Fl_Window( xPos, yPos, 405 /*IQE_GUI_TOOLS_STD_WITDH*/, 194, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  SelectionType_Last = -1;         // Reset last filter type

  //
  //  GUI things
  //

  int x1, y, yy, xx1, xx2;
  //x/int xx2, xc;
  int yGroup;
  //x/char TempBuffer[ 256];

  Fl_Check_Button *pCheckTemp;
  Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  IqeFl_Float_Input  *pFloatTemp;
  IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group, *pTemp_Group2;
  Fl_Button       *pTemp_Button;
  //x/Fl_Choice       *pTemp_Choice;
  Fl_Radio_Round_Button *pRadioButTemp;
  //x/char TempString[ 256];

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
  // Group 'Base'
  //

  yGroup = y;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LANGDEF_ALL);
  pTemp_Group->tooltip( LangStringLookup( "&GUI_PosCorr_TabA1a=General settings"));

    y += 8;
    y += 4;

    x1 = 8;

    xx2 = 220;

    pCheckTemp = new Fl_Check_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_PosCorr_TabA2=Perform position correction"));
    pCheckTemp->tooltip( LangStringLookup( "&GUI_PosCorr_TabA2a="
                         "The position correction is performed in the inspection mode.\n"
                         "The input image is shifted and rotated accordingly."));
    pCheckTemp->value( pToolData->Base_InspCorrect);
    pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->Base_InspCorrect);

    // Next line

    x1  = 8;
    y += yy + 4;
    y += 4;

    xx2 = 32;

    pTemp_Button = new Fl_Button( x1, y - 2, xx2 - 2, yy + 4, LangStringLookup( "&GUI_PosCorr_TabA3=Color outside"));
    pTemp_Button->align( FL_ALIGN_RIGHT);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_PosCorr_TabA3a="
                           "Color for the area outside an image.\n"
                           "Rotating an image or performing other transformations\n"
                           "creates areas outside the image. These are colored\n"
                           "accordingly."));
    pTemp_Button->color( pToolData->Base_OutsiteColor);
    pTemp_Button->callback( IqeB_GUI_But_Color_Callback, &pToolData->Base_OutsiteColor);
    pBaseColor = pTemp_Button;

    // Next line

    x1  = 8;
    y += yy + 4;

    xx2 = 220;

    pCheckTemp = new Fl_Check_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_PosCorr_TabA4=Hide outside area"));
    pCheckTemp->tooltip( LangStringLookup( "&GUI_PosCorr_TabA4a="
                         "Hide outside area.\n"
                         "If enabled, the area outside the image is hidden.\n"
                         "Otherwise, the area outside is visible."));
    pCheckTemp->value( pToolData->Base_OutsiteBlend);
    pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->Base_OutsiteBlend);
    pBaseBlend = pCheckTemp;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'edges'
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_PosCorr_TabB1=Edges"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_PosCorr_TabB1a=Position correction with edge finding"));

    y += 8;

    x1 = 4;
    xx1 = pMyParWin->w() - x1 - 4;

    pTemp_Group2 = new Fl_Group( x1, y, xx1, yy, LangStringLookup( "&GUI_PosCorr_TabB2=AOIs:"));  // Group around the radio buttons
    pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

    // ...

    x1 += 50;

    xx1 = 48;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LANGDEF_X);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_PosCorr_TabB3a="
                           "Uses 1 X AOI.\n"
                           "Correct only X position."));
    pRadioButTemp->callback( YaIPS_SelectionType_Callback, (void *)YAIPS_EDGEPM_MODE_X);
    SelectionButtons[ YAIPS_EDGEPM_MODE_X] = pRadioButTemp;

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LANGDEF_Y);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_PosCorr_TabB4a="
                           "Uses 1 Y AOI.\n"
                           "Correct only Y position."));
    pRadioButTemp->callback( YaIPS_SelectionType_Callback, (void *)YAIPS_EDGEPM_MODE_Y);
    SelectionButtons[ YAIPS_EDGEPM_MODE_Y] = pRadioButTemp;

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LangStringLookup( "&GUI_PosCorr_TabB5=XY"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_PosCorr_TabB5a="
                           "Uses 1 X and 1 Y AOI.\n"
                           "Correct the X position first and then the Y position."));
    pRadioButTemp->callback( YaIPS_SelectionType_Callback, (void *)YAIPS_EDGEPM_MODE_XY);
    SelectionButtons[ YAIPS_EDGEPM_MODE_XY] = pRadioButTemp;

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LangStringLookup( "&GUI_PosCorr_TabB6=YX"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_PosCorr_TabB6a="
                           "Uses 1 Y and 1 X AOI.\n"
                           "Correct the Y position first and then the X position."));
    pRadioButTemp->callback( YaIPS_SelectionType_Callback, (void *)YAIPS_EDGEPM_MODE_YX);
    SelectionButtons[ YAIPS_EDGEPM_MODE_YX] = pRadioButTemp;

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LangStringLookup( "&GUI_PosCorr_TabB7=XYY"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_PosCorr_TabB7a="
                           "Uses 1 X and 2 Y AOI.\n"
                           "Position and rotation correction."));
    pRadioButTemp->callback( YaIPS_SelectionType_Callback, (void *)YAIPS_EDGEPM_MODE_XYY);
    SelectionButtons[ YAIPS_EDGEPM_MODE_XYY] = pRadioButTemp;

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LangStringLookup( "&GUI_PosCorr_TabB8=XXY"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_PosCorr_TabB8a="
                           "Uses 2 X and 1 Y AOI.\n"
                           "Position and rotation correction."));
    pRadioButTemp->callback( YaIPS_SelectionType_Callback, (void *)YAIPS_EDGEPM_MODE_XXY);
    SelectionButtons[ YAIPS_EDGEPM_MODE_XXY] = pRadioButTemp;

    pTemp_Group2->end();

    x1 = pMyParWin->w() - 28 - 9;

    pTemp_Button = new Fl_Button( x1, y, 28, 28, "@+1pencil");
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Teach_mode);
    pTemp_Button->tooltip( LANGDEF_AOI_TEACH_TOOLTIP);
    pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
    pTemp_Button->shortcut( FL_COMMAND+'t');       // Short cut key
    pES_TeachBut = pTemp_Button;

    // Next line

    x1  = 4;
    y += yy;

    xx2 = 46;                                              // With of position/size input element

    x1 += 80;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupB9a=1. AOI"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += xx2 + 4 + xx2 + 12;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupB9b=2. AOI"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += xx2 + 4 + xx2 + 12;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupB9c=3. AOI"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    // Next line

    x1  = 4;
    y += yy;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupB10=AOI pos."));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += 80;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB11a=Position left edge 1. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 0].AOI.XPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 0].AOI.XPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pES_AOI_X[ 0] = pTemp_Int;

    x1 += xx2;
    x1 += 4;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB12a=Position upper edge 1. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 0].AOI.YPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 0].AOI.YPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pES_AOI_Y[ 0] = pTemp_Int;

    x1 += xx2;
    x1 += 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB13a=Position left edge 2. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 1].AOI.XPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 1].AOI.XPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pES_AOI_X[ 1] = pTemp_Int;

    x1 += xx2;
    x1 += 4;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB14a=Position upper edge 2. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 1].AOI.YPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 1].AOI.YPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pES_AOI_Y[ 1] = pTemp_Int;

    x1 += xx2;
    x1 += 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB15a=Position left edge 3. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 2].AOI.XPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 2].AOI.XPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pES_AOI_X[ 2] = pTemp_Int;

    x1 += xx2;
    x1 += 4;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB16a=Position upper edge 3. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 2].AOI.YPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 2].AOI.YPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pES_AOI_Y[ 2] = pTemp_Int;

    // Next line

    x1  = 4;
    y += yy + 2;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupB17=AOI size"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += 80;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB18a=Width of the 1. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 0].AOI.XSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 0].AOI.XSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pES_AOI_XX[ 0] = pTemp_Int;

    x1 += xx2;
    x1 += 4;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB19a=Height of the 1. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 0].AOI.YSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 0].AOI.YSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pES_AOI_YY[ 0] = pTemp_Int;

    x1 += xx2;
    x1 += 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB20a=Width of the 2. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 1].AOI.XSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 1].AOI.XSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pES_AOI_XX[ 1] = pTemp_Int;

    x1 += xx2;
    x1 += 4;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB21a=Height of the 2. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 1].AOI.YSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 1].AOI.YSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pES_AOI_YY[ 1] = pTemp_Int;

    x1 += xx2;
    x1 += 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB22a=Width of the 3. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 1].AOI.XSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 2].AOI.XSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pES_AOI_XX[ 2] = pTemp_Int;

    x1 += xx2;
    x1 += 4;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB23a=Height of the 3. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 1].AOI.YSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 2].AOI.YSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pES_AOI_YY[ 2] = pTemp_Int;

    // Next line

    x1  = 4;
    y += yy + 2;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupB30=Direction"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += 80;

    pCheckTemp = new Fl_Check_Button( x1, y, 86, yy, LANGDEF_REVERSE);
    pCheckTemp->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB31a=Reverse direction of edge detection 1. AOI."));
    pCheckTemp->value( pToolData->Edge_PM.Edges[ 0].r2l_b2t);
    pCheckTemp->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Edge_PM.Edges[ 0].r2l_b2t);
    pR2l_b2t_1 = pCheckTemp;

    x1 += xx2 + 4 + xx2 + 12;

    pCheckTemp = new Fl_Check_Button( x1, y, 86, yy, LANGDEF_REVERSE);
    pCheckTemp->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB32a=Reverse direction of edge detection 2. AOI."));
    pCheckTemp->value( pToolData->Edge_PM.Edges[ 1].r2l_b2t);
    pCheckTemp->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Edge_PM.Edges[ 1].r2l_b2t);
    pR2l_b2t_2 = pCheckTemp;

    x1 += xx2 + 4 + xx2 + 12;

    pCheckTemp = new Fl_Check_Button( x1, y, 86, yy, LANGDEF_REVERSE);
    pCheckTemp->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB33a=Reverse direction of edge detection 3. AOI."));
    pCheckTemp->value( pToolData->Edge_PM.Edges[ 2].r2l_b2t);
    pCheckTemp->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Edge_PM.Edges[ 2].r2l_b2t);
    pR2l_b2t_3 = pCheckTemp;

    // Next line

    x1  = 4;
    y += yy + 2;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupB34=Min. cont. %"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += 80;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB35a=Minimum contrast 1. AOI in %"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 0].minPeakConPerc);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 0].minPeakConPerc);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pminPeak_1 = pTemp_Int;

    x1 += xx2 + 4 + xx2 + 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB36a=Minimum contrast 2. AOI in %"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 1].minPeakConPerc);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 1].minPeakConPerc);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pminPeak_2 = pTemp_Int;

    x1 += xx2 + 4 + xx2 + 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB37a=Minimum contrast 3. AOI in %"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Edge_PM.Edges[ 2].minPeakConPerc);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Edge_PM.Edges[ 2].minPeakConPerc);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pminPeak_3 = pTemp_Int;

    // Next line

    x1  = 4;
    y += yy + 2;

    pTemp_Box = new Fl_Box( x1, y, 100, yy, LangStringLookup( "&GUI_PosCorr_GroupB40=Max. position deviation"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += 160;

    pTemp_Box = new Fl_Box( x1, y, 20, yy, LANGDEF_X);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label

    x1 += 20;

    xx2 = 46;                                               // With of input element

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, pYaIPS_Calib_Unit2String());
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB42a="
                         "Error if the position deviation\n"
                         "in X is greater than this."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->Edge_PM.MaxDevX);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->Edge_PM.MaxDevX);
    pFloatTemp->SetModifyData( 0.0, 100.0, 1.0, 0.1);

    x1 += xx2;

    x1 += 24;

    pTemp_Box = new Fl_Box( x1, y, 20, yy, LANGDEF_Y);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label

    x1 += 20;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, pYaIPS_Calib_Unit2String());
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_PosCorr_GroupB44a="
                         "Error if the position deviation\n"
                         "in Y is greater than this."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->Edge_PM.MaxDevY);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->Edge_PM.MaxDevY);
    pFloatTemp->SetModifyData( 0.0, 100.0, 1.0, 0.1);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'Pattern search'
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_PosCorr_TabC1=XY"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_PosCorr_TabC1a=Position correction with search pattern"));

    y += 8;

    x1 = 4;
    xx1 = pMyParWin->w() - x1 - 4;

    pTemp_Group2 = new Fl_Group( x1, y, xx1, yy, LangStringLookup( "&GUI_PosCorr_TabC2=AOIs:"));  // Group around the radio buttons
    pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

    // ...


    x1 += 50;

    xx1 = 48;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LangStringLookup( "&GUI_PosCorr_TabC3=1 XY"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_PosCorr_TabC3a="
                           "Uses 1 XY AOI.\n"
                           "Correct only X and Y position."));
    pRadioButTemp->callback( YaIPS_SelectionType_Callback, (void *)YAIPS_SELECTION_PSEAR_1XY);
    SelectionButtons[ YAIPS_SELECTION_PSEAR_1XY] = pRadioButTemp;

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LangStringLookup( "&GUI_PosCorr_TabC4=3 XY"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_PosCorr_TabC4a="
                           "Uses 3 XY AOIs.\n"
                           "Correct position, rotation and size."));
    pRadioButTemp->callback( YaIPS_SelectionType_Callback, (void *)YAIPS_SELECTION_PSEAR_3XY);
    SelectionButtons[ YAIPS_SELECTION_PSEAR_3XY] = pRadioButTemp;

    pTemp_Group2->end();

    x1 = pMyParWin->w() - 28 - 9;

    pTemp_Button = new Fl_Button( x1, y, 28, 28, "@+1pencil");
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Teach_mode);
    pTemp_Button->tooltip( LANGDEF_AOI_TEACH_TOOLTIP);
    pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
    pTemp_Button->shortcut( FL_COMMAND+'t');       // Short cut key
    pPS_TeachBut = pTemp_Button;

    // Next line

    x1  = 4;
    y += yy;

    xx2 = 46;                                              // With of position/size input element

    x1 += 80;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupC9a=1. AOI"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += xx2 + 4 + xx2 + 12;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupC9b=2. AOI"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += xx2 + 4 + xx2 + 12;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupC9c=3. AOI"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    // Next line

    x1  = 4;
    y += yy;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupC10=AOI pos."));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += 80;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC11a=Position left edge 1. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->PSearch_XY.AOI[ 0].AOI.XPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 0].AOI.XPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pPS_AOI_X[ 0] = pTemp_Int;

    x1 += xx2;
    x1 += 4;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC12a=Position upper edge 1. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->PSearch_XY.AOI[ 0].AOI.YPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 0].AOI.YPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pPS_AOI_Y[ 0] = pTemp_Int;

    x1 += xx2;
    x1 += 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC13a=Position left edge 2. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->PSearch_XY.AOI[ 1].AOI.XPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 1].AOI.XPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pPS_AOI_X[ 1]= pTemp_Int;

    x1 += xx2;
    x1 += 4;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC14a=Position upper edge 2. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->PSearch_XY.AOI[ 1].AOI.YPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 1].AOI.YPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pPS_AOI_Y[ 1] = pTemp_Int;

    x1 += xx2;
    x1 += 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC15a=Position left edge 3. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->PSearch_XY.AOI[ 2].AOI.XPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 2].AOI.XPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pPS_AOI_X[ 2] = pTemp_Int;

    x1 += xx2;
    x1 += 4;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC16a=Position upper edge 3. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->PSearch_XY.AOI[ 2].AOI.YPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 2].AOI.YPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pPS_AOI_Y[ 2] = pTemp_Int;

    // Next line

    x1  = 4;
    y += yy + 2;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupC17=AOI size"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += 80;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC18a=Width of the 1. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->PSearch_XY.AOI[ 0].AOI.XSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 0].AOI.XSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pPS_AOI_XX[ 0] = pTemp_Int;

    x1 += xx2;
    x1 += 4;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC19a=Height of the 1. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->PSearch_XY.AOI[ 0].AOI.YSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 0].AOI.YSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pPS_AOI_YY[ 0] = pTemp_Int;

    x1 += xx2;
    x1 += 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC20a=Width of the 2. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->PSearch_XY.AOI[ 1].AOI.XSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 1].AOI.XSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pPS_AOI_XX[ 1] = pTemp_Int;

    x1 += xx2;
    x1 += 4;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC21a=Height of the 2. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->PSearch_XY.AOI[ 1].AOI.YSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 1].AOI.YSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pPS_AOI_YY[ 1] = pTemp_Int;

    x1 += xx2;
    x1 += 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC22a=Width of the 3. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->PSearch_XY.AOI[ 2].AOI.XSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 2].AOI.XSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pPS_AOI_XX[ 2] = pTemp_Int;

    x1 += xx2;
    x1 += 4;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "");
    pTemp_Int->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC23a=Height of the 3. AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->PSearch_XY.AOI[ 2].AOI.YSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 2].AOI.YSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pPS_AOI_YY[ 2] = pTemp_Int;

    // Next line

    x1  = 4;
    y += yy;

    pTemp_Box = new Fl_Box( x1, y, 80, yy, LangStringLookup( "&GUI_PosCorr_GroupC24=Search"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += 80;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, "");
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Edges_GroupC25a=Search range X."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->PSearch_XY.AOI[ 0].PosToleranceX);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 0].PosToleranceX);
    pFloatTemp->SetModifyData( 0.1, 10.0, 1.0, 0.1);
    pPS_SearchX[ 0] = pFloatTemp;

    x1 += xx2;
    x1 += 4;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, "");
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Edges_GroupC26a=Search range Y."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->PSearch_XY.AOI[ 0].PosToleranceY);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 0].PosToleranceY);
    pFloatTemp->SetModifyData( 0.1, 10.0, 1.0, 0.1);
    pPS_SearchY[ 0] = pFloatTemp;

    x1 += xx2;
    x1 += 12;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, "");
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Edges_GroupC25a=Search range X."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->PSearch_XY.AOI[ 1].PosToleranceX);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 1].PosToleranceX);
    pFloatTemp->SetModifyData( 0.1, 10.0, 1.0, 0.1);
    pPS_SearchX[ 1] = pFloatTemp;

    x1 += xx2;
    x1 += 4;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, "");
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Edges_GroupC26a=Search range Y."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->PSearch_XY.AOI[ 1].PosToleranceY);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 1].PosToleranceY);
    pFloatTemp->SetModifyData( 0.1, 10.0, 1.0, 0.1);
    pPS_SearchY[ 1] = pFloatTemp;

    x1 += xx2;
    x1 += 12;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, "");
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Edges_GroupC25a=Search range X."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->PSearch_XY.AOI[ 2].PosToleranceX);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 2].PosToleranceX);
    pFloatTemp->SetModifyData( 0.1, 10.0, 1.0, 0.1);
    pPS_SearchX[ 2] = pFloatTemp;

    x1 += xx2;
    x1 += 4;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, "");
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Edges_GroupC26a=Search range Y."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->PSearch_XY.AOI[ 2].PosToleranceY);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->PSearch_XY.AOI[ 2].PosToleranceY);
    pFloatTemp->SetModifyData( 0.1, 10.0, 1.0, 0.1);
    pPS_SearchY[ 2] = pFloatTemp;

    // Next line

    x1  = 4;
    y += yy + 2;

    pTemp_Box = new Fl_Box( x1, y, 100, yy, LangStringLookup( "&GUI_PosCorr_GroupC40=Max. position deviation"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += 160;

    pTemp_Box = new Fl_Box( x1, y, 20, yy, LANGDEF_X);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label

    x1 += 20;

    xx2 = 46;                                               // With of input element

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, pYaIPS_Calib_Unit2String());
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_PosCorr_GroupBC2a="
                         "Error if the position deviation\n"
                         "in X is greater than this."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->PSearch_XY.MaxDevX);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->PSearch_XY.MaxDevX);
    pFloatTemp->SetModifyData( 0.0, 100.0, 1.0, 0.1);

    x1 += xx2;

    x1 += 24;

    pTemp_Box = new Fl_Box( x1, y, 20, yy, LANGDEF_Y);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label

    x1 += 20;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, pYaIPS_Calib_Unit2String());
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_PosCorr_GroupC44a="
                         "Error if the position deviation\n"
                         "in Y is greater than this."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->PSearch_XY.MaxDevY);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->PSearch_XY.MaxDevY);
    pFloatTemp->SetModifyData( 0.0, 100.0, 1.0, 0.1);

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
  Fl_Button *pGUI_TeachToggle;           // Toggle Teach / Inspection

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
    pTemp_Box = new Fl_Box( x1, y + yy - yy2 - yy2 + 1, xx, yy2, LANGDEF_IMGSEL_PDS_INPUT);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label

    // 2. Input element
    pTemp_Box = new Fl_Box( x1, y + yy - yy2, xx, yy2, LANGDEF_IMGSEL_PDS_REF);
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
    pBut_Input1->tooltip( LANGDEF_INSP_WIN_INP);
    pBut_Input1->labelcolor( YAIPS_BCOL_BUTTON);
    pBut_Input1->box( FL_BORDER_BOX);

    // 2. Input element
    pBut_Input2 = new Fl_Button( x1, y + yy - yy2, xx, yy2, "@2>");
    pBut_Input2->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pBut_Input2->tooltip( LANGDEF_INSP_WIN_REF);
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

    // Toggle Teach / Inspection

    xx = xx0;

    pGUI_TeachToggle = new Fl_Button( x1, y, xx, yy, "@+3pencil");
    pGUI_TeachToggle->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_TeachToggle->tooltip( LANGDEF_SWITCH_TEACH_INSPECT);
    pGUI_TeachToggle->labelcolor( YAIPS_BCOL_BUTTON);
    pGUI_TeachToggle->shortcut( FL_COMMAND+'t');       // Short cut key

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

    pToolData->YaIPS_ImageDisp.pImage_Box->pMouseCallback      = YaIPS_GUI_MyMouse_cb;         // Mouse event callback
    pToolData->YaIPS_ImageDisp.pImage_Box->MouseCallbackArg1   = &pToolData->YaIPS_ImageDisp;  // Pointer to Fl_YaIPS_ImageDisp_t
    pToolData->YaIPS_ImageDisp.pImage_Box->MouseCallbackArg2   = pToolData;                    // Optional pointer to ToolData

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

  // Ensure AOI from this tool window on big image display is removed if this tool window is closed.
  if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

    if( YaIPS_BigImageDisp.pImage_Box != NULL) {
      YaIPS_BigImageDisp.pImage_Box->redraw();
    }
  }

  // Free allocated work data at close of window
  YaIPS_PSearchAOI_WorkDataFree( &pToolData->PSearch_XY);
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

  } else if( w == pMyToolWin->pGUI_TeachToggle) {              // Toggle Teach / Inspection

    pToolData->Teach_mode = ! pToolData->Teach_mode;

    pToolData->Input1_Change = 0;          // Force recalculation output
    pToolData->Input2_Change = 0;                    // Force recalculation output
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

  // Update color of Toggle Teach / Inspection button

  int MouseTeachState;

  MouseTeachState = 1;                                            // We have a mouse callback state

  // Is teach mode available
  if( DoEnable) {                                                 // Enable GUI elements

    MouseTeachState = pToolData->Teach_mode ? 3 : 2;
  }

  pToolData->YaIPS_ImageDisp.pImage_Box->MouseTeachState = MouseTeachState;  // Shadow setting of teach

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_TeachToggle, MouseTeachState >= 2);   // Teach mode available

  IqeB_GUI_WidgetLabelColor( pMyToolWin->pGUI_TeachToggle, MouseTeachState == 3 ? FL_GREEN : YAIPS_BCOL_BUTTON);
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

    int Input1_Check, Input1_ImageChanged, Input2_Check, Input2_ImageChanged, ForceUpdate;
    Fl_RGB_Image *pImgIn1, *pImgIn2, *pImgUsed;
    char *pInspErrText;
    float Rotation, xshift, yshift;

    ForceUpdate = false;                         // Preset: NO Force update of output image

    Rotation = 0.0;
    xshift = 0.0;
    yshift = 0.0;

    // Check input image and visualize state
    Input1_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr,
                                           NULL, &pImgIn1, &Input1_ImageChanged);

    Input2_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input2_WinIdNr,
                                            NULL, &pImgIn2, &Input2_ImageChanged);

    pImgUsed = NULL;
    pInspErrText = NULL;

    {

      ierr = 0;                                            // Reset error
      errstring = NULL;                                    // Reset error string

      // Check inputs

      if( Input1_Check != 0 || Input2_Check != 0) {            // Any input image is not valid

        if( Input1_Check == 0 && Input2_Check != 0) {          // Input 1 is valid, input 2 is not valid

          errstring = LANGDEF_IMGSEL_ERR_NO_REF;

          ierr = -100;

          if( Input1_ImageChanged != pToolData->Input1_Change) { // if image counts are different

            // Show scene image to adjust AOI parameter

            pToolData->Input1_Change = Input1_ImageChanged;        // Image is processed
          }

        } else if( Input1_Check != 0 && Input2_Check == 0) {   // Input 1 is not valid, input 2 is valid

          errstring = LANGDEF_IMGSEL_ERR_NO_INP;

          ierr = -101;

          if( Input2_ImageChanged != pToolData->Input2_Change) { // if image counts are different

            // Show scene image to adjust AOI parameter

            pToolData->Input2_Change = Input2_ImageChanged;        // Image is processed
          }

        } else {                                               // Both inputs are not valid

          errstring = LANGDEF_IMGSEL_ERR_NONE;

          ierr = -103;

          pToolData->Input1_Change = 0;                    // Force recalculation output
          pToolData->Input2_Change = 0;                    // Force recalculation output

        }

      } else {

        // Have both images

        // Check image size to be the same

        if( pImgIn1->data_w() != pImgIn2->data_w()) {         // Image size is different

          errstring = LANGDEF_IMGSEL_ERR_SIZES;

          ierr = -104;

        } else if( (pImgIn1->d() >= 3) != (pImgIn2->d() >= 3)) {  // Image color/BW is different

          errstring = LANGDEF_IMGSEL_ERR_BPP;

          ierr = -105;
        }
      }

      if( ierr == 0 &&                                        // No error until now
          Input1_ImageChanged == pToolData->Input1_Change &&  // Both image counts are same
          Input2_ImageChanged == pToolData->Input2_Change) {

        // Skip inspection because no image has changed
        goto SkipInspection;
      } else {

        if( ierr != 0 &&           // Have an error
            errstring != NULL) {   // and an error text

          pInspErrText = errstring;   // Copy to Info string

          errstring = NULL;
        }
      }

      pToolData->Input1_Change = Input1_ImageChanged;        // Image is processed
      pToolData->Input2_Change = Input2_ImageChanged;        // Image is processed

      if( ierr == 0) {                // No error until now

        // Depending from teach mode process one of the input images

        if( pToolData->Teach_mode) {

          pImgUsed = pImgIn2;   // Use reference image

        } else {

          pImgUsed = pImgIn1;   // Use input image
        }

        if( pToolData->SelectionType >= YAIPS_SELECTION_EDGES_FIRST &&   // Is one of the edge position types
            pToolData->SelectionType <= YAIPS_SELECTION_EDGES_LAST) {

          // Do the edge position measurement

          ierr = YaIPS_EdgePM_Inspect( pImgUsed,               // Input image
                                       &pToolData->Edge_PM,    // Point to edge position data
                                       pToolData->Teach_mode); // 0 = inspection mode, 1 = teach mode

          pInspErrText = pToolData->Edge_PM.CheckText;

          Rotation = pToolData->Edge_PM.DeltaA;
          xshift = pToolData->Edge_PM.T_Matrix[ B1];
          yshift = pToolData->Edge_PM.T_Matrix[ B2];

        } else if( pToolData->SelectionType >= YAIPS_SELECTION_PSEAR_FIRST &&   // Is one of the pattern search types
                   pToolData->SelectionType <= YAIPS_SELECTION_PSEAR_LAST) {

          if( pToolData->Teach_mode) {

            ierr = YaIPS_PSearchAOI_Teach( pImgIn2, &pToolData->PSearch_XY);

          } else {

            ierr = YaIPS_PSearchAOI_Inspect( pImgIn1, pImgIn2, &pToolData->PSearch_XY);
          }

          pInspErrText = pToolData->PSearch_XY.CheckText;

          Rotation = pToolData->PSearch_XY.DeltaA;
          xshift = pToolData->PSearch_XY.T_Matrix[ B1];
          yshift = pToolData->PSearch_XY.T_Matrix[ B2];
        }
      }

      // End processing

      // Has a valid output image

      if( ierr == 0 ||                                  // Have a result image
          ierr == YAIPS_EDGEDM_ERR_INSP_TOL) {          // or an out of tolerance error

        if( pToolData->Teach_mode ||                    // Teach mode
            pToolData->Base_InspCorrect == false) {     // Don't perform position correction in inspection mode

          // Copy input image to output image
          YaIPS_RGB_MixChannels( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgUsed, NULL, NULL, NULL);   // Remove alpha channel from input image

        } else {

          // Perform position correction

          YaIPS_RGB_Geo_Transform( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                   pToolData->Base_OutsiteColor, pToolData->Base_OutsiteBlend,
                                   Rotation,
                                   1.0, 1.0, - xshift, - yshift, 0, 0,
                                   0, 0, 0, 0);
        }

        YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, (char *)MY_WIN_GUI_NAME);

        if( ierr == 0) {

          YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, FL_GREEN, FL_BLACK, pInspErrText);  // Display info message
        } else {

          YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, FL_RED, FL_WHITE, pInspErrText);  // Display info message
        }

#ifdef _DEBUG
        if( errstring != NULL) {                           // Have a error message
          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp, errstring);
        } else {
          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp);
        }
#else
        YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp);
#endif

        ForceUpdate = true;                        // Force update of output image

      } else {                                          // Processing error

        //x/pToolData->Input1_Change = 0;                   // Force recalculation output
        //x/pToolData->Input2_Change = 0;                   // Force recalculation output

        // Copy input image to output image
        if( pImgUsed != NULL) {

          YaIPS_RGB_MixChannels( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgUsed, NULL, NULL, NULL);   // Remove alpha channel from input image

        } else {

          // No image to output

          YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);
        }

        YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, (char *)MY_WIN_GUI_NAME);

        ForceUpdate = true;                        // Force update of output image

        // Info / error text display

        if( pInspErrText != NULL && pInspErrText[ 0] != '\0') {             // Have a check text

          YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, FL_RED, FL_WHITE, pInspErrText);  // Display error message

        } else {

          YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp);  // Reset info message
        }

#ifdef _DEBUG
        if( errstring != NULL) {                           // Have a error message

          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp, errstring);

        } else {

#ifdef use_again
          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp, LANGDEF_ERROR_CODE, ierr);
#else
          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp);
#endif
        }
#else
        YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp);
#endif
      }
    }

    // ...

SkipInspection:

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
 * IqeB_GUI_PosCorrWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_PosCorrWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  CLASS_WIN_TOOL *pMyToolWin;
  int xPos, yPos, nTabelOnEntry;
  char TempString[ 256];

  //
  // Get a free info data element
  //

  nTabelOnEntry = nYaIPS_ToolData_info;                    // Remember count of elements in info data

  if( nYaIPS_ToolData_info < iToolData + 1) {              // Catch maximum value

    nYaIPS_ToolData_info = iToolData + 1;
  }

  pToolData = YaIPS_ToolData_info + iToolData;             // Point to info data

  //
  // Prepare info data element
  //

  pToolData->iToolData = iToolData;                        // Set sub window number

  pToolData->IsOpen = true;                                // Flag image as open

  pToolData->Edge_PM.CheckText[ 0] = '\0';                 // Reset check text at entry

  pToolData->PSearch_XY.CheckText[ 0] = '\0';              // Reset temporary work data
  pToolData->PSearch_XY.pWorkData = NULL;

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
 * DoClip:           if true (> 0) handle clipping of draw region
 *                   else this must be done in the calling function
 *
 */
static void YaIPS_GUI_MyDrawAfter_Func( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // point to image display data
                                        YaIPS_ToolData_info_t *pToolData,        // point to tool data
                                        int DoClip)                              // if true (> 0) handle clipping of draw region else caller must do it
{
  int x1, y1, xx, yy;
  Fl_Color DrawColor;

  // Check for valid output image displayed

  if( pToolData->YaIPS_ImageDisp.pImage_Img == NULL) {

    return;
  }

  // Preparations

  x1 = pYaIPS_ImageDisp->BigImage_sx;
  y1 = pYaIPS_ImageDisp->BigImage_sy;
  xx = pYaIPS_ImageDisp->BigImage_sw;
  yy = pYaIPS_ImageDisp->BigImage_sh;

#ifdef USE_DEBUG_TRAFO // debug some transformation system things

  double OffX, OffY;

  OffX = pYaIPS_ImageDisp->SubImage_x + 0.5;
  OffY = pYaIPS_ImageDisp->SubImage_y + 0.5;
#endif

  //
  // Draw AOIs for position correction with edges
  //

  if( pToolData->SelectionType >= YAIPS_SELECTION_EDGES_FIRST &&  // One of the edge search modes
      pToolData->SelectionType <= YAIPS_SELECTION_EDGES_LAST) {

    int orientation, iAOI, nAOIs, ET;
    YaIPS_EdgeAOI_t *pEdgeAOI;
    char TempString[ 128];

    // Clipping ?

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( x1, y1, xx, yy);
    }

    nAOIs = (pToolData->Edge_PM.MeasureMode / 2) + 1;

    ET = pToolData->Edge_PM.MeasureMode;

    pEdgeAOI = pToolData->Edge_PM.Edges;

    for( iAOI = 0; iAOI < nAOIs; iAOI++, pEdgeAOI++) {

      int IsSelected;

      IsSelected = pToolData->Teach_mode &&
                   (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0 &&  // Mouse is over any AOI
                   pYaIPS_ImageDisp->AoiIdNr == iAOI;                                  // and mouse is over this AOI

      if( pToolData->Teach_mode) {                // Teach mode

        if( IsSelected) {     // Teach mode and mouse is over this AOI

          DrawColor = FL_RED;
        } else {

          DrawColor = FL_GREEN - 2;
        }

        if( pEdgeAOI->CheckError != 0) {

          if( ! IsSelected) {

            DrawColor = FL_MAGENTA;
          }

          sprintf( TempString, "%d", iAOI + 1);

        } else {

          sprintf( TempString, "%d", iAOI + 1);
        }

      } else {                                // Inspection mode

        if( pEdgeAOI->CheckError != 0) {

          sprintf( TempString, "%d", iAOI + 1);

          DrawColor = FL_RED;

        } else {

          sprintf( TempString, "%d", iAOI + 1);

          DrawColor = FL_GREEN - 2;
        }
      }

      if( iAOI == 0) {

        orientation = ET == YAIPS_EDGEPM_MODE_Y || ET == YAIPS_EDGEPM_MODE_YX ? 1 : 0; // 0: x-distance, 1: y-distance

      } else if( iAOI == 1) {

        orientation = ET == YAIPS_EDGEPM_MODE_XY || ET == YAIPS_EDGEPM_MODE_XYY ? 1 : 0; // 0: x-distance, 1: y-distance

      } else {

        orientation =  1; // 0: x-distance, 1: y-distance
      }

      sprintf( TempString, "%d", iAOI + 1);

      YaIPS_EdgeAOI_Draw( pYaIPS_ImageDisp, pEdgeAOI, pToolData->Teach_mode, DrawColor, TempString,
                          orientation, 0);

#ifdef USE_DEBUG_TRAFO // debug some transformation system things

      if( pToolData->Teach_mode == 0 &&             // inspection mode
          pToolData->Edge_PM.CheckError == 0 ) {    // and last inspection was OK

        double P1X, P1Y, P2X, P2Y;

        P1X = (double)pEdgeAOI->AOI.XPos + (double)pEdgeAOI->AOI.XSize * 0.5;
        P1Y = (double)pEdgeAOI->AOI.YPos + (double)pEdgeAOI->AOI.YSize * 0.5;

        P2X = MATRIX_TRANSFORM_X( pToolData->Edge_PM.T_Matrix, P1X, P1Y);
        P2Y = MATRIX_TRANSFORM_Y( pToolData->Edge_PM.T_Matrix, P1X, P1Y);

        // Draw ...
        P1X = ((P1X - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
        P1Y = ((P1Y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

        P2X = ((P2X - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
        P2Y = ((P2Y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

        fl_color( FL_YELLOW);           // Color

        fl_line( dto32( P1X), dto32( P1Y), dto32( P2X), dto32( P2Y));

        fl_line( dto32( P1X) + 5, dto32( P1Y) + 5, dto32( P1X) - 5, dto32( P1Y) - 5);
        fl_line( dto32( P1X) - 5, dto32( P1Y) + 5, dto32( P1X) + 5, dto32( P1Y) - 5);

        fl_line( dto32( P2X) + 5, dto32( P2Y) + 5, dto32( P2X) - 5, dto32( P2Y) - 5);
        fl_line( dto32( P2X) - 5, dto32( P2Y) + 5, dto32( P2X) + 5, dto32( P2Y) - 5);
      }
#endif
    }

#ifdef USE_DEBUG_TRAFO // debug some transformation system things

    // Estimated center of transform system

    if( pToolData->Teach_mode == 0 &&           // inspection mode
        pToolData->Edge_PM.CheckError == 0 &&   // and last inspection was OK
        nAOIs >= 3 &&                           // and minimum 3 AOIs
        pToolData->Edge_PM.MeasureMode >= YAIPS_EDGEPM_MODE_XYY) { // and one of the 3 AOI modes

      double P1X, P1Y, P2X, P2Y;
      float DeltaX, DeltaY;

      pEdgeAOI = pToolData->Edge_PM.Edges;

      if( pToolData->Edge_PM.MeasureMode == YAIPS_EDGEPM_MODE_XYY) {

        // X is average of center of the 2 y AOIs
        P1X = ((double)pEdgeAOI[ 1].AOI.XPos + (double)pEdgeAOI[ 1].AOI.XSize * 0.5 + (double)pEdgeAOI[ 2].AOI.XPos + (double)pEdgeAOI[ 2].AOI.XSize * 0.5) * 0.5;

        // Y is  center of the x AOI
        P1Y = (double)pEdgeAOI[ 0].AOI.YPos + (double)pEdgeAOI[ 0].AOI.YSize * 0.5;

      } else {   // is YAIPS_EDGEPM_MODE_XXY

        // X is center of the y AOI
        P1X = (double)pEdgeAOI[ 2].AOI.XPos + (double)pEdgeAOI[ 2].AOI.XSize * 0.5;

        // Y is average of center of the 2 x AOIs
        P1Y = ((double)pEdgeAOI[ 0].AOI.YPos + (double)pEdgeAOI[ 0].AOI.YSize * 0.5 + (double)pEdgeAOI[ 1].AOI.YPos + (double)pEdgeAOI[ 1].AOI.YSize * 0.5) * 0.5;
      }

      P2X = MATRIX_TRANSFORM_X( pToolData->Edge_PM.T_Matrix, P1X, P1Y);
      P2Y = MATRIX_TRANSFORM_Y( pToolData->Edge_PM.T_Matrix, P1X, P1Y);

      // Delta after transformation is the offset

      DeltaX = P2X - P1X;
      DeltaY = P2Y - P1Y;

      // Draw ...
      P1X = ((P1X - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
      P1Y = ((P1Y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

      P2X = ((P2X - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
      P2Y = ((P2Y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

      fl_color( FL_YELLOW);           // Color

      fl_line( dto32( P1X), dto32( P1Y), dto32( P2X), dto32( P2Y));

      fl_line( dto32( P1X) + 5, dto32( P1Y) + 5, dto32( P1X) - 5, dto32( P1Y) - 5);
      fl_line( dto32( P1X) - 5, dto32( P1Y) + 5, dto32( P1X) + 5, dto32( P1Y) - 5);

      fl_line( dto32( P2X) + 5, dto32( P2Y) + 5, dto32( P2X) - 5, dto32( P2Y) - 5);
      fl_line( dto32( P2X) - 5, dto32( P2Y) + 5, dto32( P2X) + 5, dto32( P2Y) - 5);

      fl_font( FL_HELVETICA, 14);

      sprintf( TempString, "%+.2f/%+.2f %s", DeltaX * YaIPS_Calib_UPP_X,DeltaY * YaIPS_Calib_UPP_Y, pYaIPS_Calib_Unit2String());

      fl_draw( TempString, dto32( P2X), dto32( P2Y));
    }
#endif
    goto ExitPoint;

  } else if( pToolData->SelectionType >= YAIPS_SELECTION_PSEAR_FIRST &&  // One of the pattern search modes
      pToolData->SelectionType <= YAIPS_SELECTION_PSEAR_LAST) {

    int iAOI, nAOIs;
    YaIPS_PSearchAOI_t *pPSearchAOI;
    char TempString[ 128];

    // Clipping ?

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( x1, y1, xx, yy);
    }

    nAOIs = pToolData->PSearch_XY.MeasureMode == YAIPS_PSEARCH_MODE_1XY ? 1 : 3;

    pPSearchAOI = pToolData->PSearch_XY.AOI;

    for( iAOI = 0; iAOI < nAOIs; iAOI++, pPSearchAOI++) {

      int IsSelected;

      IsSelected = pToolData->Teach_mode &&
                   (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0 &&  // Mouse is over any AOI
                   pYaIPS_ImageDisp->AoiIdNr == iAOI;                                  // and mouse is over this AOI

      YaIPS_PSearchAOI_Draw( pYaIPS_ImageDisp, pPSearchAOI, pToolData->Teach_mode, IsSelected, iAOI);

#ifdef USE_DEBUG_TRAFO // debug some transformation system things

      if( pToolData->Teach_mode == 0 &&             // inspection mode
          pToolData->PSearch_XY.CheckError == 0 ) {    // and last inspection was OK

        double P1X, P1Y, P2X, P2Y;

        P1X = (double)pPSearchAOI->AOI.XPos + (double)pPSearchAOI->AOI.XSize * 0.5;
        P1Y = (double)pPSearchAOI->AOI.YPos + (double)pPSearchAOI->AOI.YSize * 0.5;

        P2X = MATRIX_TRANSFORM_X( pToolData->PSearch_XY.T_Matrix, P1X, P1Y);
        P2Y = MATRIX_TRANSFORM_Y( pToolData->PSearch_XY.T_Matrix, P1X, P1Y);

        // Draw ...
        P1X = ((P1X - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
        P1Y = ((P1Y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

        P2X = ((P2X - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
        P2Y = ((P2Y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

        fl_color( FL_YELLOW);           // Color

        fl_line( dto32( P1X), dto32( P1Y), dto32( P2X), dto32( P2Y));

        fl_line( dto32( P1X) + 5, dto32( P1Y) + 5, dto32( P1X) - 5, dto32( P1Y) - 5);
        fl_line( dto32( P1X) - 5, dto32( P1Y) + 5, dto32( P1X) + 5, dto32( P1Y) - 5);

        fl_line( dto32( P2X) + 5, dto32( P2Y) + 5, dto32( P2X) - 5, dto32( P2Y) - 5);
        fl_line( dto32( P2X) - 5, dto32( P2Y) + 5, dto32( P2X) + 5, dto32( P2Y) - 5);
      }
#endif
    }

#ifdef USE_DEBUG_TRAFO // debug some transformation system things

    // Estimated center of transform system

    if( pToolData->Teach_mode == 0 &&           // inspection mode
        pToolData->PSearch_XY.CheckError == 0 &&   // and last inspection was OK
        nAOIs >= 3 &&                           // and minimum 3 AOIs
        pToolData->PSearch_XY.MeasureMode >= YAIPS_PSEARCH_MODE_3XY) { // and one of the 3 AOI modes

      double P1X, P1Y, P2X, P2Y, PTX, PTY;
      float DeltaX, DeltaY;

      pPSearchAOI = pToolData->PSearch_XY.AOI;

      // Use min/max center of the three AOIs

      P1X = (double)pPSearchAOI[ 0].AOI.XPos + (double)pPSearchAOI[ 0].AOI.XSize * 0.5;
      P1Y = (double)pPSearchAOI[ 0].AOI.YPos + (double)pPSearchAOI[ 0].AOI.YSize * 0.5;

      P2X = P1X;
      P2Y = P1Y;

      PTX = (double)pPSearchAOI[ 1].AOI.XPos + (double)pPSearchAOI[ 1].AOI.XSize * 0.5;
      PTY = (double)pPSearchAOI[ 1].AOI.YPos + (double)pPSearchAOI[ 1].AOI.YSize * 0.5;

      if( PTX < P1X) P1X = PTX;
      if( PTY < P1Y) P1Y = PTY;
      if( PTX > P2X) P2X = PTX;
      if( PTY > P2Y) P2Y = PTY;

      PTX = (double)pPSearchAOI[ 2].AOI.XPos + (double)pPSearchAOI[ 2].AOI.XSize * 0.5;
      PTY = (double)pPSearchAOI[ 2].AOI.YPos + (double)pPSearchAOI[ 2].AOI.YSize * 0.5;

      if( PTX < P1X) P1X = PTX;
      if( PTY < P1Y) P1Y = PTY;
      if( PTX > P2X) P2X = PTX;
      if( PTY > P2Y) P2Y = PTY;

      P1X = (P1X + P2X) * 0.5;    // Average min/max
      P1Y = (P1Y + P2Y) * 0.5;

      P2X = MATRIX_TRANSFORM_X( pToolData->PSearch_XY.T_Matrix, P1X, P1Y);
      P2Y = MATRIX_TRANSFORM_Y( pToolData->PSearch_XY.T_Matrix, P1X, P1Y);

      // Delta after transformation is the offset

      DeltaX = P2X - P1X;
      DeltaY = P2Y - P1Y;

      // Draw ...
      P1X = ((P1X - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
      P1Y = ((P1Y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

      P2X = ((P2X - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + x1;
      P2Y = ((P2Y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + y1;

      fl_color( FL_YELLOW);           // Color

      fl_line( dto32( P1X), dto32( P1Y), dto32( P2X), dto32( P2Y));

      fl_line( dto32( P1X) + 5, dto32( P1Y) + 5, dto32( P1X) - 5, dto32( P1Y) - 5);
      fl_line( dto32( P1X) - 5, dto32( P1Y) + 5, dto32( P1X) + 5, dto32( P1Y) - 5);

      fl_line( dto32( P2X) + 5, dto32( P2Y) + 5, dto32( P2X) - 5, dto32( P2Y) - 5);
      fl_line( dto32( P2X) - 5, dto32( P2Y) + 5, dto32( P2X) + 5, dto32( P2Y) - 5);

      fl_font( FL_HELVETICA, 14);

      sprintf( TempString, "%+.2f/%+.2f %s", DeltaX * YaIPS_Calib_UPP_X,DeltaY * YaIPS_Calib_UPP_Y, pYaIPS_Calib_Unit2String());

      fl_draw( TempString, dto32( P2X), dto32( P2Y));
    }
#endif
    goto ExitPoint;

  }

ExitPoint:

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

  YaIPS_GUI_MyDrawAfter_Func( pYaIPS_ImageDisp, pToolData, true);
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
 * YaIPS_GUI_MyMouse_cb
 *
 * Mouse event callback for image displays
 *
 * pArg: Depending on callback function this points to a 'Fl_YaIPS_ImageDisp_t'
 *       structure or to tool window data.
 *
 */
static int YaIPS_GUI_MyMouse_cb( Fl_Widget *pW, int event,
                                  void *pArg1,        // Pointer to Fl_YaIPS_ImageDisp_t
                                  void *pArg2)        // Optional pointer to ToolData
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  int ierr, x, y, i, nAOIs, AoiIdNrEntry;
  int minAoiDist, CursorShapeTest, AoiDeltaAddTest, IsBigImageDisp;
  YaIPS_ToolData_info_t *pToolData;
  Fl_YaIPS_AOI_t *pAOI_Best, *pAOI_This;
  static int Last_x = -9999, Last_y = -9999;           // Must be static
  static int Pressed_x, Pressed_y;                     // Used for move with pressed mouse button
  static Fl_YaIPS_AOI_t Pressed_AOI;                   // AOI on press of mouse button
  static Fl_YaIPS_AOI_t *pPressed_AOI_Best;            // What AOI to modify

  pYaIPS_ImageDisp = (Fl_YaIPS_ImageDisp_t *)pArg1;    // Get pointer to image display data

  // Get pointer to tool data

  pToolData = (YaIPS_ToolData_info_t *)pArg2;          // Need pointer to tool data
  if( pToolData == NULL) {                             // Security test, have a pointer

    return( -1);
  }

  // Mouse callback common entry work

  ierr = YaIPS_ImageDispMouse_CommonEntry( pW, event, pYaIPS_ImageDisp, &x, &y, &Last_x, &Last_y);

  if( ierr < 0) {            // Error or problem

    return( ierr);                  // Return to caller

  } else if( ierr == 0) {    // After return to exit work

    goto ExitPoint;
  }

  //
  // Module specific presets and actions
  //

  IsBigImageDisp = pYaIPS_ImageDisp == &YaIPS_BigImageDisp;   // Called for big image display

  pYaIPS_ImageDisp->CursorShape = IsBigImageDisp ? 0 : -1;       // Overwrite: Preset default cursor shape
  pYaIPS_ImageDisp->MyWinID = MY_WIN_ID + pToolData->iToolData;  // Overwrite: Update big image, set my tool window ID
  minAoiDist     = -1;                                // Needed for section of nearest AOI
  pAOI_Best      = NULL;                              // Modify this AOI
  AoiIdNrEntry   = -1;                                // Not set

  // Process mouse events

  switch( event) {

  case FL_MOUSEWHEEL:      // The user has moved the mouse wheel.

    if( IsBigImageDisp) {  // Called for big image display

      // Manage display resolution change by mouse wheel event
      YaIPS_ImageDispMouse_CommonMouseWheel( pYaIPS_ImageDisp, x, y);
    }

    break;

  case FL_PUSH:          // A mouse button has gone down
  case FL_RELEASE:       // A mouse button has been released.
  case FL_MOVE:          // The mouse has moved without any mouse buttons held down.
  case FL_DRAG:          // The mouse has moved with a button held down.

#ifdef use_again
#ifdef _DEBUG
    YaIPS_ImageDispStrDebug( pYaIPS_ImageDisp, "FL_MOVE: %d/%d %d/%d %d/%d", Delta_x, Delta_y, x, y, mouseleft, mouseright);
    pYaIPS_ImageDisp->pImage_Box->redraw();
#endif
#endif

    // Left mouse button pressed and NOT 3D plot active

    if( pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_AUTO) {  // Any resolution

      if( pToolData->Teach_mode &&                   // Adjust AOI
          pYaIPS_ImageDisp->pImage_Img != NULL) {    // and have an output image

        if( pToolData->SelectionType >= YAIPS_SELECTION_EDGES_FIRST &&  // One of the edge search modes
            pToolData->SelectionType <= YAIPS_SELECTION_EDGES_LAST) {

          nAOIs = (pToolData->Edge_PM.MeasureMode / 2) + 1;

          AoiIdNrEntry = pYaIPS_ImageDisp->AoiIdNr;

          for( i = 0; i < nAOIs; i++) {

            pAOI_This = &pToolData->Edge_PM.Edges[ i].AOI;

            ierr = YaIPS_ImageDispAoiRectCC( pYaIPS_ImageDisp, pAOI_This, &minAoiDist, &CursorShapeTest, &AoiDeltaAddTest);

            if( ierr == true) {     // Got one (or a better one)

              // nearer aoi found
              pYaIPS_ImageDisp->CursorShape = CursorShapeTest;
              pYaIPS_ImageDisp->AoiDeltaAdd = AoiDeltaAddTest;
              pYaIPS_ImageDisp->AoiIdNr = i;           // AOI selected
              pAOI_Best   = pAOI_This;
            }
          }

        } else if( pToolData->SelectionType >= YAIPS_SELECTION_PSEAR_FIRST &&  // One of the pattern search modes
            pToolData->SelectionType <= YAIPS_SELECTION_PSEAR_LAST) {

          nAOIs = pToolData->PSearch_XY.MeasureMode == YAIPS_PSEARCH_MODE_1XY ? 1 : 3;

          AoiIdNrEntry = pYaIPS_ImageDisp->AoiIdNr;

          for( i = 0; i < nAOIs; i++) {

            pAOI_This = &pToolData->PSearch_XY.AOI[ i].AOI;

            ierr = YaIPS_ImageDispAoiRectCC( pYaIPS_ImageDisp, pAOI_This, &minAoiDist, &CursorShapeTest, &AoiDeltaAddTest);

            if( ierr == true) {     // Got one (or a better one)

              // nearer aoi found
              pYaIPS_ImageDisp->CursorShape = CursorShapeTest;
              pYaIPS_ImageDisp->AoiDeltaAdd = AoiDeltaAddTest;
              pYaIPS_ImageDisp->AoiIdNr = i;           // AOI selected
              pAOI_Best   = pAOI_This;
            }
          }
        }
      }

      if( pYaIPS_ImageDisp->mouseleft) {                    // Left mouse button pressed

        if( pYaIPS_ImageDisp->AoiDeltaAdd != 0 && pYaIPS_ImageDisp->Latched_AoiDeltaAdd == 0) {   // Latch AOI mouse modification

          pYaIPS_ImageDisp->Latched_AoiDeltaAdd = pYaIPS_ImageDisp->AoiDeltaAdd;
          pYaIPS_ImageDisp->Latched_CursorShape = pYaIPS_ImageDisp->CursorShape;

          pPressed_AOI_Best = pAOI_Best;                    // Modify this AOI
          Pressed_x = Last_x + pYaIPS_ImageDisp->Delta_x;   // Latch position at button press
          Pressed_y = Last_y + pYaIPS_ImageDisp->Delta_y;

          memcpy( &Pressed_AOI, pAOI_Best, sizeof( Fl_YaIPS_AOI_t)); // Remember AOI data a button press
        }

        if( pYaIPS_ImageDisp->Latched_AoiDeltaAdd != 0) {        // Have latched AOI mouse modification

          pYaIPS_ImageDisp->AoiDeltaAdd = pYaIPS_ImageDisp->Latched_AoiDeltaAdd;   // Use it
          pYaIPS_ImageDisp->CursorShape = pYaIPS_ImageDisp->Latched_CursorShape;
          pAOI_Best   = pPressed_AOI_Best;
        }

      } else {                                                   // Left mouse button is NOT pressed

        pYaIPS_ImageDisp->Latched_AoiDeltaAdd = 0;               // Reset latched data
        pYaIPS_ImageDisp->Latched_CursorShape = 0;

        if( AoiIdNrEntry >= 0 && AoiIdNrEntry != pYaIPS_ImageDisp->AoiIdNr) {  // Window selection has changed

          pYaIPS_ImageDisp->RedrawOnExit   = true;                      // Set redraw on exit
          pYaIPS_ImageDisp->BigImageUpdate = pYaIPS_ImageDisp->MyWinID; // Update big image

          pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;    // Set AOI changed flag bit
        }
      }

      if( pYaIPS_ImageDisp->mouseleft &&                                        // and left button pressed
          pYaIPS_ImageDisp->AoiDeltaAdd != 0 &&                                 // and add deltas
          (pYaIPS_ImageDisp->Delta_x != 0 || pYaIPS_ImageDisp->Delta_y != 0)) { // and mouse has moved

        memcpy( pAOI_Best, &Pressed_AOI, sizeof( Fl_YaIPS_AOI_t)); // Restore AOI data from button press

        pYaIPS_ImageDisp->Delta_x = dto32( (Last_x - Pressed_x) / pYaIPS_ImageDisp->PixelImageToScreen);
        pYaIPS_ImageDisp->Delta_y = dto32( (Last_y - Pressed_y) / pYaIPS_ImageDisp->PixelImageToScreen);

        YaIPS_ImageDispAoiRectDeltaAdd( pYaIPS_ImageDisp, pAOI_Best,
                                        pYaIPS_ImageDisp->AoiDeltaAdd, pYaIPS_ImageDisp->Delta_x, pYaIPS_ImageDisp->Delta_y);

        pToolData->Input1_Change = 0;                                 // Force recalculation output
        pToolData->Input2_Change = 0;                                 // Force recalculation output

        pYaIPS_ImageDisp->RedrawOnExit   = true;                      // Set redraw on exit
        pYaIPS_ImageDisp->BigImageUpdate = pYaIPS_ImageDisp->MyWinID; // Update big image

        pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;    // Set AOI changed flag bit

        break;
      }
    }

    if( IsBigImageDisp) {  // Called for big image display

      // Left/right mouse button pressed on image background
      YaIPS_ImageDispMouse_CommonMouseBackGnd( pYaIPS_ImageDisp, &Last_x, &Last_y);
    }

    break;
  }

ExitPoint:

  // Mouse callback common exit work. Manage AOI selection and setting cursor shape

  YaIPS_ImageDispMouse_CommonExit( pYaIPS_ImageDisp);

  return( 0);        // Mouse events are processed
}

/************************************************************************************
 * IqeB_GUI_PosCorrWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_PosCorrWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  // ...

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

    // Register draw after function for big image display
    YaIPS_ToolWinDrawAfterSet( MY_WIN_ID + SubWinIDx, YaIPS_GUI_MyDrawAfter_Other);

    IqeB_GUI_PosCorrWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  IqeB_GUI_PosCorrWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


