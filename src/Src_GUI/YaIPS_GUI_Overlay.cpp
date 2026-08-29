/****************************************************************************

  YaIPS_GUI_Overlay.cpp

  Overlay images, shapes, text to an image.

  04.06.2025 RR: First edition of this file.

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

#include <FL/Fl_Text_Editor.h>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Menu_Window.H>

/************************************************************************************
* Defines for this source file.
*/

// Defines for windows ID
#define MY_WIN_ID     YAIPS_WIN_ID_OVERLAY        // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_OVERLAY       // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_Overlay_Title=Overlay"  // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinOvelay"            // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_Overlay_Tool  // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       246
#define MYWIN_SIZE_X_MAX       YAIPS_WIN_SIZE_S1_X_MAX
#define MYWIN_SIZE_X_DEFAULT   YAIPS_WIN_SIZE_S1_X_DEFAULT

#define MYWIN_SIZE_Y_MIN       YAIPS_WIN_SIZE_S1_Y_MIN
#define MYWIN_SIZE_Y_MAX       YAIPS_WIN_SIZE_S1_Y_MAX
#define MYWIN_SIZE_Y_DEFAULT   YAIPS_WIN_SIZE_S1_Y_DEFAULT

// Other defines

#define VALUE_INDENT_MAX     250     // Maximal value for horizontal or vertical indent
#define VALUE_SPACING_MAX    250     // Maximal value for line or character spacing

/************************************************************************************
* forwards
*/

static void close_cb( Fl_Widget *w, long int iToolData);
static  int DropFile_cb( Fl_Widget *w, void *pFileNameArg, void *pImageDispArg, int SubWinIDx);
static void YaIPS_ToolWin_GUI_Callback( Fl_Widget *w, long int iToolData);
static void PasteImg_cb( Fl_Widget *w, long int iToolData);
static void MyWinUpdate( int iToolData, int DoEnable);
static void IqeB_GUI_ToolsMyIdleAction( void *);
static void YaIPS_GUI_MyDrawAfter_cb( Fl_Widget *pW, void *pArg1, void *pArg2);
static int YaIPS_GUI_MyMouse_cb( Fl_Widget *pW, int event, void *pArg1, void *pArg2);
static void ButtonNewCallback( Fl_Widget *w, void *data);

/************************************************************************************
* Global variables for this window
*/

static Fl_Text_Buffer *pEmptyTextBuffer = NULL;

//-----------------------------------------------------------------------------------
// Manage multiple tool windows
//-----------------------------------------------------------------------------------

#define OVERLAY_NUM_MAX             16  // Max number of overlays

typedef struct {

  // Parameters
  int iToolData;                        // Sub window number
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

  int TeachMode;                        // If != 0, teach mode is on
  int Tab_Group_Selected;               // Number of last selected tab group.

  // Overlay definitions

  int nOverlays;                        // Number of overlays used
  YaIPS_OverlayData_t Overlay[ OVERLAY_NUM_MAX];  // Max number of overlays

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

  { PREF_T_INT,          "TeachMode",   "0", &YaIPS_ToolData_info[0].TeachMode },
  { PREF_T_INT,     "Group_Selected",   "0", &YaIPS_ToolData_info[0].Tab_Group_Selected},

  // Overlay definitions

  { PREF_T_INT,          "nOverlays",  "0", &YaIPS_ToolData_info[0].nOverlays},

  { PREF_T_STRING,      "OV_Name_01",   "", &YaIPS_ToolData_info[0].Overlay[ 0].Name, sizeof( YaIPS_ToolData_info[0].Overlay[ 0].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_02",   "", &YaIPS_ToolData_info[0].Overlay[ 1].Name, sizeof( YaIPS_ToolData_info[0].Overlay[ 1].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_03",   "", &YaIPS_ToolData_info[0].Overlay[ 2].Name, sizeof( YaIPS_ToolData_info[0].Overlay[ 2].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_04",   "", &YaIPS_ToolData_info[0].Overlay[ 3].Name, sizeof( YaIPS_ToolData_info[0].Overlay[ 3].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_05",   "", &YaIPS_ToolData_info[0].Overlay[ 4].Name, sizeof( YaIPS_ToolData_info[0].Overlay[ 4].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_06",   "", &YaIPS_ToolData_info[0].Overlay[ 5].Name, sizeof( YaIPS_ToolData_info[0].Overlay[ 5].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_07",   "", &YaIPS_ToolData_info[0].Overlay[ 6].Name, sizeof( YaIPS_ToolData_info[0].Overlay[ 6].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_08",   "", &YaIPS_ToolData_info[0].Overlay[ 7].Name, sizeof( YaIPS_ToolData_info[0].Overlay[ 7].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_09",   "", &YaIPS_ToolData_info[0].Overlay[ 8].Name, sizeof( YaIPS_ToolData_info[0].Overlay[ 8].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_10",   "", &YaIPS_ToolData_info[0].Overlay[ 9].Name, sizeof( YaIPS_ToolData_info[0].Overlay[ 9].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_11",   "", &YaIPS_ToolData_info[0].Overlay[10].Name, sizeof( YaIPS_ToolData_info[0].Overlay[10].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_12",   "", &YaIPS_ToolData_info[0].Overlay[11].Name, sizeof( YaIPS_ToolData_info[0].Overlay[11].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_13",   "", &YaIPS_ToolData_info[0].Overlay[12].Name, sizeof( YaIPS_ToolData_info[0].Overlay[12].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_14",   "", &YaIPS_ToolData_info[0].Overlay[13].Name, sizeof( YaIPS_ToolData_info[0].Overlay[13].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_15",   "", &YaIPS_ToolData_info[0].Overlay[14].Name, sizeof( YaIPS_ToolData_info[0].Overlay[14].Name) - 1 },
  { PREF_T_STRING,      "OV_Name_16",   "", &YaIPS_ToolData_info[0].Overlay[15].Name, sizeof( YaIPS_ToolData_info[0].Overlay[15].Name) - 1 },

  { PREF_T_INT,         "OV_Type_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].Type},
  { PREF_T_INT,         "OV_Type_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].Type},
  { PREF_T_INT,         "OV_Type_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].Type},
  { PREF_T_INT,         "OV_Type_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].Type},
  { PREF_T_INT,         "OV_Type_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].Type},
  { PREF_T_INT,         "OV_Type_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].Type},
  { PREF_T_INT,         "OV_Type_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].Type},
  { PREF_T_INT,         "OV_Type_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].Type},
  { PREF_T_INT,         "OV_Type_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].Type},
  { PREF_T_INT,         "OV_Type_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].Type},
  { PREF_T_INT,         "OV_Type_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].Type},
  { PREF_T_INT,         "OV_Type_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].Type},
  { PREF_T_INT,         "OV_Type_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].Type},
  { PREF_T_INT,         "OV_Type_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].Type},
  { PREF_T_INT,         "OV_Type_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].Type},
  { PREF_T_INT,         "OV_Type_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].Type},

  { PREF_T_INT,        "OV_AOI_X_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].AOI.XPos},
  { PREF_T_INT,        "OV_AOI_X_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].AOI.XPos},

  { PREF_T_INT,        "OV_AOI_Y_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].AOI.YPos},
  { PREF_T_INT,        "OV_AOI_Y_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].AOI.YPos},

  { PREF_T_INT,       "OV_AOI_XX_01", "50",  &YaIPS_ToolData_info[0].Overlay[ 0].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_02", "50",  &YaIPS_ToolData_info[0].Overlay[ 1].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_03", "50",  &YaIPS_ToolData_info[0].Overlay[ 2].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_04", "50",  &YaIPS_ToolData_info[0].Overlay[ 3].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_05", "50",  &YaIPS_ToolData_info[0].Overlay[ 4].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_06", "50",  &YaIPS_ToolData_info[0].Overlay[ 5].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_07", "50",  &YaIPS_ToolData_info[0].Overlay[ 6].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_08", "50",  &YaIPS_ToolData_info[0].Overlay[ 7].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_09", "50",  &YaIPS_ToolData_info[0].Overlay[ 8].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_10", "50",  &YaIPS_ToolData_info[0].Overlay[ 9].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_11", "50",  &YaIPS_ToolData_info[0].Overlay[10].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_12", "50",  &YaIPS_ToolData_info[0].Overlay[11].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_13", "50",  &YaIPS_ToolData_info[0].Overlay[12].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_14", "50",  &YaIPS_ToolData_info[0].Overlay[13].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_15", "50",  &YaIPS_ToolData_info[0].Overlay[14].AOI.XSize},
  { PREF_T_INT,       "OV_AOI_XX_16", "50",  &YaIPS_ToolData_info[0].Overlay[15].AOI.XSize},

  { PREF_T_INT,       "OV_AOI_YY_01", "50",  &YaIPS_ToolData_info[0].Overlay[ 0].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_02", "50",  &YaIPS_ToolData_info[0].Overlay[ 1].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_03", "50",  &YaIPS_ToolData_info[0].Overlay[ 2].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_04", "50",  &YaIPS_ToolData_info[0].Overlay[ 3].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_05", "50",  &YaIPS_ToolData_info[0].Overlay[ 4].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_06", "50",  &YaIPS_ToolData_info[0].Overlay[ 5].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_07", "50",  &YaIPS_ToolData_info[0].Overlay[ 6].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_08", "50",  &YaIPS_ToolData_info[0].Overlay[ 7].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_09", "50",  &YaIPS_ToolData_info[0].Overlay[ 8].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_10", "50",  &YaIPS_ToolData_info[0].Overlay[ 9].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_11", "50",  &YaIPS_ToolData_info[0].Overlay[10].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_12", "50",  &YaIPS_ToolData_info[0].Overlay[11].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_13", "50",  &YaIPS_ToolData_info[0].Overlay[12].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_14", "50",  &YaIPS_ToolData_info[0].Overlay[13].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_15", "50",  &YaIPS_ToolData_info[0].Overlay[14].AOI.YSize},
  { PREF_T_INT,       "OV_AOI_YY_16", "50",  &YaIPS_ToolData_info[0].Overlay[15].AOI.YSize},

  { PREF_T_FLOAT,       "OV_PosX_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].PosX},
  { PREF_T_FLOAT,       "OV_PosX_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].PosX},
  { PREF_T_FLOAT,       "OV_PosX_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].PosX},
  { PREF_T_FLOAT,       "OV_PosX_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].PosX},
  { PREF_T_FLOAT,       "OV_PosX_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].PosX},
  { PREF_T_FLOAT,       "OV_PosX_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].PosX},
  { PREF_T_FLOAT,       "OV_PosX_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].PosX},
  { PREF_T_FLOAT,       "OV_PosX_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].PosX},
  { PREF_T_FLOAT,       "OV_PosX_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].PosX},
  { PREF_T_FLOAT,       "OV_PosX_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].PosX},
  { PREF_T_FLOAT,       "OV_PosX_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].PosX},
  { PREF_T_FLOAT,       "OV_PosX_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].PosX},
  { PREF_T_FLOAT,       "OV_PosX_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].PosX},
  { PREF_T_FLOAT,       "OV_PosX_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].PosX},
  { PREF_T_FLOAT,       "OV_PosX_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].PosX},
  { PREF_T_FLOAT,       "OV_PosX_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].PosX},

  { PREF_T_FLOAT,       "OV_PosY_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].PosY},
  { PREF_T_FLOAT,       "OV_PosY_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].PosY},
  { PREF_T_FLOAT,       "OV_PosY_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].PosY},
  { PREF_T_FLOAT,       "OV_PosY_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].PosY},
  { PREF_T_FLOAT,       "OV_PosY_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].PosY},
  { PREF_T_FLOAT,       "OV_PosY_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].PosY},
  { PREF_T_FLOAT,       "OV_PosY_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].PosY},
  { PREF_T_FLOAT,       "OV_PosY_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].PosY},
  { PREF_T_FLOAT,       "OV_PosY_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].PosY},
  { PREF_T_FLOAT,       "OV_PosY_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].PosY},
  { PREF_T_FLOAT,       "OV_PosY_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].PosY},
  { PREF_T_FLOAT,       "OV_PosY_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].PosY},
  { PREF_T_FLOAT,       "OV_PosY_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].PosY},
  { PREF_T_FLOAT,       "OV_PosY_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].PosY},
  { PREF_T_FLOAT,       "OV_PosY_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].PosY},
  { PREF_T_FLOAT,       "OV_PosY_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].PosY},

  { PREF_T_FLOAT,      "OV_SizeX_01", "100", &YaIPS_ToolData_info[0].Overlay[ 0].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_02", "100", &YaIPS_ToolData_info[0].Overlay[ 1].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_03", "100", &YaIPS_ToolData_info[0].Overlay[ 2].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_04", "100", &YaIPS_ToolData_info[0].Overlay[ 3].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_05", "100", &YaIPS_ToolData_info[0].Overlay[ 4].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_06", "100", &YaIPS_ToolData_info[0].Overlay[ 5].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_07", "100", &YaIPS_ToolData_info[0].Overlay[ 6].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_08", "100", &YaIPS_ToolData_info[0].Overlay[ 7].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_09", "100", &YaIPS_ToolData_info[0].Overlay[ 8].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_10", "100", &YaIPS_ToolData_info[0].Overlay[ 9].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_11", "100", &YaIPS_ToolData_info[0].Overlay[10].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_12", "100", &YaIPS_ToolData_info[0].Overlay[11].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_13", "100", &YaIPS_ToolData_info[0].Overlay[12].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_14", "100", &YaIPS_ToolData_info[0].Overlay[13].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_15", "100", &YaIPS_ToolData_info[0].Overlay[14].SizeX},
  { PREF_T_FLOAT,      "OV_SizeX_16", "100", &YaIPS_ToolData_info[0].Overlay[15].SizeX},

  { PREF_T_FLOAT,      "OV_SizeY_01", "100", &YaIPS_ToolData_info[0].Overlay[ 0].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_02", "100", &YaIPS_ToolData_info[0].Overlay[ 1].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_03", "100", &YaIPS_ToolData_info[0].Overlay[ 2].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_04", "100", &YaIPS_ToolData_info[0].Overlay[ 3].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_05", "100", &YaIPS_ToolData_info[0].Overlay[ 4].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_06", "100", &YaIPS_ToolData_info[0].Overlay[ 5].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_07", "100", &YaIPS_ToolData_info[0].Overlay[ 6].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_08", "100", &YaIPS_ToolData_info[0].Overlay[ 7].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_09", "100", &YaIPS_ToolData_info[0].Overlay[ 8].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_10", "100", &YaIPS_ToolData_info[0].Overlay[ 9].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_11", "100", &YaIPS_ToolData_info[0].Overlay[10].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_12", "100", &YaIPS_ToolData_info[0].Overlay[11].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_13", "100", &YaIPS_ToolData_info[0].Overlay[12].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_14", "100", &YaIPS_ToolData_info[0].Overlay[13].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_15", "100", &YaIPS_ToolData_info[0].Overlay[14].SizeY},
  { PREF_T_FLOAT,      "OV_SizeY_16", "100", &YaIPS_ToolData_info[0].Overlay[15].SizeY},

  { PREF_T_INT,      "OV_AoiUnit_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].AoiUnit},
  { PREF_T_INT,      "OV_AoiUnit_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].AoiUnit},

  { PREF_T_INT,  "OV_AOI_XX_Lock_01", "50",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_02", "50",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_03", "50",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_04", "50",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_05", "50",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_06", "50",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_07", "50",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_08", "50",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_09", "50",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_10", "50",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_11", "50",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_12", "50",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_13", "50",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_14", "50",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_15", "50",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.AOI_XX_Locked},
  { PREF_T_INT,  "OV_AOI_XX_Lock_16", "50",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.AOI_XX_Locked},

  { PREF_T_INT,  "OV_AOI_YY_Lock_01", "50",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_02", "50",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_03", "50",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_04", "50",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_05", "50",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_06", "50",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_07", "50",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_08", "50",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_09", "50",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_10", "50",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_11", "50",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_12", "50",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_13", "50",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_14", "50",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_15", "50",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.AOI_YY_Locked},
  { PREF_T_INT,  "OV_AOI_YY_Lock_16", "50",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.AOI_YY_Locked},

  { PREF_T_FLOAT,     "OV_Rotate_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.RotAngle},
  { PREF_T_FLOAT,     "OV_Rotate_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.RotAngle},

  { PREF_T_FLOAT,  "OV_AlphaMult_01","100",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_02","100",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_03","100",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_04","100",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_05","100",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_06","100",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_07","100",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_08","100",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_09","100",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_10","100",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_11","100",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_12","100",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_13","100",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_14","100",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_15","100",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.AlphaMult},
  { PREF_T_FLOAT,  "OV_AlphaMult_16","100",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.AlphaMult},

  { PREF_T_INT,   "OV_ShapeFlags_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.ShapeFlags},
  { PREF_T_INT,   "OV_ShapeFlags_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.ShapeFlags},

  // Background

  { PREF_T_INT,     "OV_BGndType_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.BGndType},
  { PREF_T_INT,     "OV_BGndType_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.BGndType},

  // YAIPS_SHAPE_GEN_BGND_COLOR

  { PREF_T_INT,   "OV_BGndCol_LT_01",  "2",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_02",  "2",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_03",  "2",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_04",  "2",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_05",  "2",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_06",  "2",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_07",  "2",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_08",  "2",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_09",  "2",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_10",  "2",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_11",  "2",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_12",  "2",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_13",  "2",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_14",  "2",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_15",  "2",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.BGndCol_LT},
  { PREF_T_INT,   "OV_BGndCol_LT_16",  "2",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.BGndCol_LT},

  { PREF_T_INT,   "OV_BGndCol_RT_01",  "3",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_02",  "3",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_03",  "3",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_04",  "3",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_05",  "3",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_06",  "3",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_07",  "3",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_08",  "3",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_09",  "3",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_10",  "3",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_11",  "3",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_12",  "3",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_13",  "3",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_14",  "3",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_15",  "3",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.BGndCol_RT},
  { PREF_T_INT,   "OV_BGndCol_RT_16",  "3",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.BGndCol_RT},

  { PREF_T_INT,   "OV_BGndCol_LB_01",  "1",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_02",  "1",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_03",  "1",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_04",  "1",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_05",  "1",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_06",  "1",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_07",  "1",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_08",  "1",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_09",  "1",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_10",  "1",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_11",  "1",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_12",  "1",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_13",  "1",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_14",  "1",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_15",  "1",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.BGndCol_LB},
  { PREF_T_INT,   "OV_BGndCol_LB_16",  "1",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.BGndCol_LB},

  { PREF_T_INT,   "OV_BGndCol_RB_01",  "6",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_02",  "6",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_03",  "6",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_04",  "6",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_05",  "6",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_06",  "6",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_07",  "6",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_08",  "6",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_09",  "6",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_10",  "6",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_11",  "6",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_12",  "6",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_13",  "6",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_14",  "6",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_15",  "6",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.BGndCol_RB},
  { PREF_T_INT,   "OV_BGndCol_RB_16",  "6",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.BGndCol_RB},


  // YAIPS_SHAPE_GEN_BGND_IMAGE

  { PREF_T_STRING, "OV_BGndFileName_01", "", &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_02", "", &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_03", "", &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_04", "", &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_05", "", &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_06", "", &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_07", "", &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_08", "", &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_09", "", &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_10", "", &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_11", "", &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[10].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_12", "", &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[11].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_13", "", &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[12].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_14", "", &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[13].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_15", "", &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[14].ShapeGen.BGndFileName) - 1 },
  { PREF_T_STRING, "OV_BGndFileName_16", "", &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.BGndFileName, sizeof( YaIPS_ToolData_info[0].Overlay[15].ShapeGen.BGndFileName) - 1 },

  // YAIPS_SHAPE_GEN_BGND_WINDOW

  { PREF_T_INT, "OV_BGnd_WinIdNr_01", "-1",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_02", "-1",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_03", "-1",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_04", "-1",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_05", "-1",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_06", "-1",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_07", "-1",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_08", "-1",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_09", "-1",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_10", "-1",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_11", "-1",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_12", "-1",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_13", "-1",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_14", "-1",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_15", "-1",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.BGnd_WinIdNr},
  { PREF_T_INT, "OV_BGnd_WinIdNr_16", "-1",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.BGnd_WinIdNr},

  // Shape drawing

  { PREF_T_INT,    "OV_ShapeType_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.ShapeType},
  { PREF_T_INT,    "OV_ShapeType_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.ShapeType},

  { PREF_T_FLOAT,   "OV_ShapeArg_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.ShapeArg},
  { PREF_T_FLOAT,   "OV_ShapeArg_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.ShapeArg},

  // Line drawing

  { PREF_T_INT,     "OV_LineType_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.LineType},
  { PREF_T_INT,     "OV_LineType_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.LineType},

  { PREF_T_INT,    "OV_LineColor_01",  "4",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_02",  "4",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_03",  "4",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_04",  "4",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_05",  "4",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_06",  "4",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_07",  "4",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_08",  "4",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_09",  "4",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_10",  "4",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_11",  "4",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_12",  "4",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_13",  "4",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_14",  "4",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_15",  "4",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.LineColor},
  { PREF_T_INT,    "OV_LineColor_16",  "4",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.LineColor},

  { PREF_T_INT,    "OV_LineWidth_01",  "1",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_02",  "1",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_03",  "1",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_04",  "1",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_05",  "1",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_06",  "1",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_07",  "1",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_08",  "1",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_09",  "1",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_10",  "1",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_11",  "1",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_12",  "1",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_13",  "1",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_14",  "1",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_15",  "1",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.LineWidth},
  { PREF_T_INT,    "OV_LineWidth_16",  "1",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.LineWidth},

  // Text drawing

  { PREF_T_STRING,  "OV_FontName_01", "", &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_02", "", &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_03", "", &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_04", "", &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_05", "", &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_06", "", &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_07", "", &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_08", "", &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_09", "", &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_10", "", &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_11", "", &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[10].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_12", "", &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[11].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_13", "", &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[12].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_14", "", &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[13].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_15", "", &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[14].ShapeGen.FontName) - 1 },
  { PREF_T_STRING,  "OV_FontName_16", "", &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.FontName, sizeof( YaIPS_ToolData_info[0].Overlay[15].ShapeGen.FontName) - 1 },

  { PREF_T_INT,    "OV_FontStyle_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.FontStyle},
  { PREF_T_INT,    "OV_FontStyle_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.FontStyle},

  { PREF_T_INT,    "OV_FontSize_01",  "16",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_02",  "16",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_03",  "16",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_04",  "16",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_05",  "16",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_06",  "16",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_07",  "16",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_08",  "16",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_09",  "16",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_10",  "16",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_11",  "16",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_12",  "16",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_13",  "16",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_14",  "16",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_15",  "16",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.FontSize},
  { PREF_T_INT,    "OV_FontSize_16",  "16",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.FontSize},

  { PREF_T_INT,    "OV_FontColor_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.FontColor},
  { PREF_T_INT,    "OV_FontColor_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.FontColor},

  { PREF_T_INT,    "OV_IndentHor_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.IndentHor},
  { PREF_T_INT,    "OV_IndentHor_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.IndentHor},

  { PREF_T_INT,    "OV_IndentVer_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.IndentVer},
  { PREF_T_INT,    "OV_IndentVer_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.IndentVer},

  { PREF_T_INT,  "OV_SpacingLine_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.SpacingLine},
  { PREF_T_INT,  "OV_SpacingLine_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.SpacingLine},

  { PREF_T_INT,  "OV_SpacingChar_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.SpacingChar},
  { PREF_T_INT,  "OV_SpacingChar_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.SpacingChar},

  // Shadow

  { PREF_T_FLOAT,  "OV_ShadowAngle_01",  "315",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_02",  "315",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_03",  "315",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_04",  "315",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_05",  "315",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_06",  "315",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_07",  "315",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_08",  "315",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_09",  "315",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_10",  "315",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_11",  "315",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_12",  "315",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_13",  "315",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_14",  "315",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_15",  "315",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.ShadowAngle},
  { PREF_T_FLOAT,  "OV_ShadowAngle_16",  "315",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.ShadowAngle},

  { PREF_T_INT,  "OV_ShadowDist_01",  "8",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_02",  "8",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_03",  "8",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_04",  "8",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_05",  "8",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_06",  "8",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_07",  "8",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_08",  "8",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_09",  "8",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_10",  "8",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_11",  "8",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_12",  "8",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_13",  "8",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_14",  "8",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_15",  "8",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.ShadowDist},
  { PREF_T_INT,  "OV_ShadowDist_16",  "8",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.ShadowDist},

  { PREF_T_INT,  "OV_ShadowColor_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.ShadowColor},
  { PREF_T_INT,  "OV_ShadowColor_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.ShadowColor},

  { PREF_T_INT,  "OV_ShadowBlur_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.ShadowBlur},
  { PREF_T_INT,  "OV_ShadowBlur_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.ShadowBlur},

  { PREF_T_INT,  "OV_ShadowTrans_01",  "0",  &YaIPS_ToolData_info[0].Overlay[ 0].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_02",  "0",  &YaIPS_ToolData_info[0].Overlay[ 1].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_03",  "0",  &YaIPS_ToolData_info[0].Overlay[ 2].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_04",  "0",  &YaIPS_ToolData_info[0].Overlay[ 3].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_05",  "0",  &YaIPS_ToolData_info[0].Overlay[ 4].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_06",  "0",  &YaIPS_ToolData_info[0].Overlay[ 5].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_07",  "0",  &YaIPS_ToolData_info[0].Overlay[ 6].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_08",  "0",  &YaIPS_ToolData_info[0].Overlay[ 7].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_09",  "0",  &YaIPS_ToolData_info[0].Overlay[ 8].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_10",  "0",  &YaIPS_ToolData_info[0].Overlay[ 9].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_11",  "0",  &YaIPS_ToolData_info[0].Overlay[10].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_12",  "0",  &YaIPS_ToolData_info[0].Overlay[11].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_13",  "0",  &YaIPS_ToolData_info[0].Overlay[12].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_14",  "0",  &YaIPS_ToolData_info[0].Overlay[13].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_15",  "0",  &YaIPS_ToolData_info[0].Overlay[14].ShapeGen.ShadowTrans},
  { PREF_T_INT,  "OV_ShadowTrans_16",  "0",  &YaIPS_ToolData_info[0].Overlay[15].ShapeGen.ShadowTrans},
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_OverlayWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp);

//-----------------------------------------------------------------------------------
// Parameter dialog
//
// This is a modal dialog. Therefore we can use global variables to hold
// info about the data.
//-----------------------------------------------------------------------------------

// defines for Overlay

#define YAIPS_OVERLAY_TYPE_NONE          0    // Until know it better

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolDataParam; // NOTE: Is used by all parameter dialog functions

static Fl_Browser *pOverlayBrowser;           // Overlay browser
static int BrowserSelected = -1;              // Last selected browser line, 0 ... nLines - 1
static Fl_Input   *pInputName;

static Fl_Button *pGUI_ButDelete;    // Delete selected overlay
static Fl_Button *pGUI_ButDelAll;    // Delete all overlays
static Fl_Button *pGUI_ButShiftUp;   // Shift up selected overlay
static Fl_Button *pGUI_ButShiftDown; // Shift down selected overlay
static Fl_Button *pGUI_ButInsert;    // Insert new overlay
static Fl_Button *pGUI_ButAppend;    // Append new overlay

// AOI ...
#ifdef use_again
static Fl_YaIPS_AOI_t CurrentAOI;               // Current AOI shown on Dialog
static IqeFl_Int_Input *pAOI_X, *pAOI_Y, *pAOI_XX, *pAOI_YY;
#else
static float CurrentPosX, CurrentPosY, CurrentSizeX, CurrentSizeY;
static IqeFl_Float_Input *pFloat_AOI_X, *pFloat_AOI_Y, *pFloat_AOI_XX, *pFloat_AOI_YY;
#endif
static Fl_Button *pAOI_SizeRatio, *pTeachToggle;
static Fl_Input  *pShowSizeRatio;
static Fl_Value_Slider *pSliderAlphaMult;
static Fl_Menu_Button  *pMButAlign;

static IqeFl_Tabs      *pTab_Groups;         // Point to tabulator GUI element

// Background type
static Fl_Radio_Round_Button *pBGndType_0,  *pBGndType_1, *pBGndType_2;

// Background color
static Fl_Button *pButCol_LT, *pButCol_RT, *pButCol_LB, *pButCol_RB;
static IqeFl_Check_Bit *pCheck_Bit_LT, *pCheck_Bit_RT, *pCheck_Bit_LB, *pCheck_Bit_RB;

// Background image
static Fl_Input   *pBGndFileName;
static Fl_Button  *pBGndFileLoad;

// Background tool window
static Fl_Box *pBGnd_Win_Box;
static Fl_Button *pBGnd_Win_But;

// Background image/tool window info
static Fl_Input   *pBGndFileInfo;
static Fl_Button  *pBGndFileCpSize;

// Shape generation ...
static YaIPS_RGB_ShapeGen_Par_t CurrentShapeGen;    // Current shape generation parameters shown on Dialog
static IqeFl_Float_Input *pRotAngle;
static Fl_Choice *pChoiceShape;                     // Pointer to shape selection
static int iCurrShape;                              // Index of current selected shape

static Fl_Value_Slider *pSlider_ShapeArg;
static IqeFl_Check_Bit *pCheck_Bit_ShapeFill, *pCheck_Bit_ShapeOutline, *pCheck_Bit_LineColUse;
static Fl_Button *pButCol_LineColor;
static IqeFl_Int_Input *pInt_LineWidth;

// Text ...
static Fl_Input   *pFontName;
static Fl_Button  *pFontChoose;
static int iCurrFont;                              // Index of current selected font
static IqeFl_Int_Input *pInt_FontSize;
static Fl_Menu_Button *pSizeChoose;
static Fl_Button *pButCol_FontColor;
static IqeFl_Check_Bit *pCheck_Bit_FontColUse;
static IqeFl_Int_Input *pInt_SpacingLine, *pInt_SpacingChar;

static Fl_Button *pButFontBold, *pButFontItalic, *pButFontUnterl;

static Fl_Button *pButAlignHor[ 3], *pButAlignVer[ 3];
static IqeFl_Int_Input *pInt_IndentHor, *pInt_IndentVer;

static Fl_Text_Editor *app_editor = NULL;

// Shadow

static IqeFl_Check_Bit *pCheck_Bit_ShadowUse;
static IqeFl_Float_Input *pShadowAngle;
static IqeFl_Int_Input *pInt_ShadowDist, *pInt_ShadowBlur, *pInt_ShadowTrans;
static Fl_Button *pButCol_ShadowColor;
static IqeFl_Check_Bit *pCheck_Bit_ShadowColUse;

/************************************************************************************
 * IqeB_GUI_Param_RecalcAOI
 *
 * Recalculate the AOI of an overlay.
 *
 */
static void IqeB_GUI_Param_RecalcAOI( YaIPS_OverlayData_t *pOverlay)
{

  pOverlay->AOI.XPos  = YaIPS_Calib_UnitXVal2Pixel( pOverlay->PosX);
  pOverlay->AOI.YPos  = YaIPS_Calib_UnitYVal2Pixel( pOverlay->PosY);
  pOverlay->AOI.XSize = YaIPS_Calib_UnitXVal2Pixel( pOverlay->SizeX);
  pOverlay->AOI.YSize = YaIPS_Calib_UnitYVal2Pixel( pOverlay->SizeY);
}

/************************************************************************************
 * IqeB_GUI_Param_Update_SizeRatio
 *
 * Update the size radio gui element.
 */
static void IqeB_GUI_Param_Update_SizeRatio()
{
  char TempString[ 256];
  int g, xx, yy;

  if( (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_AOI_SIZE_RATIO) != 0 && // Locked sized radio
      CurrentShapeGen.AOI_XX_Locked > 0 && CurrentShapeGen.AOI_YY_Locked) {      // and latched sizes are set

    xx = CurrentShapeGen.AOI_XX_Locked;
    yy = CurrentShapeGen.AOI_YY_Locked;

  } else {

    xx = YaIPS_Calib_UnitXVal2Pixel( CurrentSizeX);
    yy = YaIPS_Calib_UnitYVal2Pixel( CurrentSizeY);
  }

  g = GreatestcommonDivisor( xx, yy);

  // Have some common divisors
  if( g > 1 && xx / g <= 24 && yy / g <= 24) {

    sprintf( TempString, "%d : %d", xx / g, yy / g);
  } else {

    TempString[ 0] = '\0';
  }

  if( strcmp( TempString, pShowSizeRatio->value()) != 0) { // Is different form displayed file name
    pShowSizeRatio->value( TempString);                    // Update displayed file name
    pShowSizeRatio->insert_position( 0);                   // Position to begin of text
  }
}

/************************************************************************************
 * YaIPS_RotatableAOI_ClipGuiUpdate
 *
 * AOI rectangle clip against image boundaries and update GUI input elements of the AOI.
 * The center of the AOI is clipped to stay inside the image.
 *
 * The AOI rectangle is relative to image displayed on the screen 'BigImage_iw/-ih'.
 *
 *   pAOI               Point to AOI to test
 *   mgXX, ImgYY        Size of image
 *   pAOI_X, pAOI_Y     GUI input elements
 *   pAOI_XX, pAOI_YY
 *
 * return:    0  OK
 *            1  One of the GUI elements have been changed.
 */

static int YaIPS_RotatableAOI_ClipGuiUpdate( int ImgXX, int ImgYY) // Size of output image
{
  IqeFl_Float_Input *pAOI_X, *pAOI_Y, *pAOI_XX, *pAOI_YY;
  int RedrawOnExit;
  float ImgXX_Unit, ImgYY_Unit;

  pAOI_X  = pFloat_AOI_X;
  pAOI_Y  = pFloat_AOI_Y;
  pAOI_XX = pFloat_AOI_XX;
  pAOI_YY = pFloat_AOI_YY;

  ImgXX_Unit = YaIPS_Calib_PixXVal2Units( ImgXX, true);
  ImgYY_Unit = YaIPS_Calib_PixYVal2Units( ImgYY, true);

  RedrawOnExit = false;

#ifdef use_again
  // Maximums/minimums AOI Inputs

  if( pAOI_X->Max != ImgXX - 1) {       // Maximum is not correct
    pAOI_X->Max = ImgXX - 1;
  }

  if( pAOI_X->Min != ImgXX / -2) {      // Minimum is not correct
    pAOI_X->Min = ImgXX / -2;
  }

  if( pAOI_Y->Max != ImgYY - 1) {       // Maximum is not correct
    pAOI_Y->Max = ImgYY - 1;
  }

  if( pAOI_Y->Min != ImgYY / -2) {      // Minimum is not correct
    pAOI_Y->Min = ImgYY / -2;
  }

  if( pAOI_XX->Max != ImgXX) {          // Maximum is not correct
    pAOI_XX->Max = ImgXX;
  }

  if( pAOI_YY->Max != ImgYY) {          // Maximum is not correct
    pAOI_YY->Max = ImgYY;
  }
#endif

  // Clip size x
  if( CurrentSizeX < pAOI_XX->Min){

    RedrawOnExit = true;                              // Something has changed

    CurrentSizeX = pAOI_XX->Min;
  }

  if( CurrentSizeX > ImgXX_Unit){

    RedrawOnExit = true;                              // Something has changed

    CurrentSizeX = ImgXX_Unit;
  }

  // Clip position x
  if( CurrentPosX < CurrentSizeX / -2) {

    RedrawOnExit = true;                              // Something has changed

    CurrentPosX = CurrentSizeX / -2;
  }

  if( CurrentPosX > ImgXX_Unit - CurrentSizeX / 2) {

    RedrawOnExit = true;                              // Something has changed

    CurrentPosX = ImgXX_Unit - CurrentSizeX / 2 - 1;
  }

  // Clip size y
  if( CurrentSizeY < pAOI_YY->Min){

    RedrawOnExit = true;                              // Something has changed

    CurrentSizeY = pAOI_YY->Min;
  }

  if( CurrentSizeY > ImgYY_Unit){

    RedrawOnExit = true;                              // Something has changed

    CurrentSizeY = ImgYY_Unit;
  }

  // Clip position y
  if( CurrentPosY < CurrentSizeY / -2) {

    RedrawOnExit = true;                              // Something has changed

    CurrentPosY = CurrentSizeY / -2;
  }

  if( CurrentPosY > ImgYY_Unit - CurrentSizeY / 2) {

    RedrawOnExit = true;                              // Something has changed

    CurrentPosY = ImgYY_Unit - CurrentSizeY / 2 - 1;
  }

  // Update changed values

  if( CurrentPosX != pAOI_X->GetValue()) {

    RedrawOnExit = true;                              // Something has changed

    pAOI_X->SetValue( CurrentPosX);                    // Update on GUI
    pAOI_X->redraw();
  }

  if( CurrentPosY != pAOI_Y->GetValue()) {

    RedrawOnExit = true;                              // Something has changed

    pAOI_Y->SetValue( CurrentPosY);                    // Update on GUI
    pAOI_Y->redraw();
  }

  if( CurrentSizeX != pAOI_XX->GetValue()) {

    RedrawOnExit = true;                              // Something has changed

    pAOI_XX->SetValue( CurrentSizeX);                  // Update on GUI
    pAOI_XX->redraw();
  }

  if( CurrentSizeY != pAOI_YY->GetValue()) {

    RedrawOnExit = true;                              // Something has changed

    pAOI_YY->SetValue( CurrentSizeY);                  // Update on GUI
    pAOI_YY->redraw();
  }

  return( RedrawOnExit);
}

/************************************************************************************
 * YaIPS_RotatableAOI_Clip
 *
 * Clip rotatable AOI against image boundaries.
 * The center of the AOI is clipped to stay inside the image.
 *
 *   ImgXX, ImgYY       Size of image
 *   pAOI               Point to AOI to test
 *
 * return:    0  OK
 *            1  Something clipped
 */

static int YaIPS_RotatableAOI_Clip( int ImgXX, int ImgYY,       // Size of image
                                    YaIPS_OverlayData_t *pOverlay)       // Point to AOI to test
{
  int RedrawOnExit;
  Fl_YaIPS_AOI_t *pAOI;

  IqeB_GUI_Param_RecalcAOI( pOverlay);   // Ensure proper AOI values

  RedrawOnExit = false;

  pAOI = &pOverlay->AOI;

  // Clip size x
  if( pAOI->XSize < YAIPS_IDISP_AOI_MIN_SIZE) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->XSize = YAIPS_IDISP_AOI_MIN_SIZE;
  }

  if( pAOI->XSize > ImgXX){
    RedrawOnExit = true;                                 // Something clipped
    pAOI->XSize = ImgXX;
  }

  // Clip position x
  if( pAOI->XPos < pAOI->XSize / -2) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->XPos = pAOI->XSize / -2;
  }

  if( pAOI->XPos > ImgXX - pAOI->XSize / 2) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->XPos = ImgXX - pAOI->XSize / 2 - 1;
  }

  // Clip size y
  if( pAOI->YSize < YAIPS_IDISP_AOI_MIN_SIZE){
    RedrawOnExit = true;                                 // Something clipped
    pAOI->YSize = YAIPS_IDISP_AOI_MIN_SIZE;
  }

  if( pAOI->YSize > ImgYY){
    RedrawOnExit = true;                                 // Something clipped
    pAOI->YSize = ImgYY;
  }

  // Clip position y
  if( pAOI->YPos < pAOI->YSize / -2) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->YPos = pAOI->YSize / -2;
  }

  if( pAOI->YPos > ImgYY - pAOI->YSize / 2) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->YPos = ImgYY - pAOI->YSize / 2 - 1;
  }

  if( RedrawOnExit) {

    pOverlay->PosX  = YaIPS_Calib_PixXVal2Units( pAOI->XPos, true);
    pOverlay->PosY  = YaIPS_Calib_PixYVal2Units( pAOI->YPos, true);
    pOverlay->SizeX = YaIPS_Calib_PixXVal2Units( pAOI->XSize, true);
    pOverlay->SizeY = YaIPS_Calib_PixYVal2Units( pAOI->YSize, true);
  }

  return( RedrawOnExit);
}

/************************************************************************************
 * IqeB_GUI_ToolsAnimManagerUpdateSelected
 *
 * Updates the GUI things for single animation
 *
 * iOverlay: >= 0  Update for this animation
 *             < 0   No animation to update
 */

static void IqeB_GUI_ToolsAnimManagerUpdateSelected( int iOverlay)
{
  int BrowserSize, BrowserValue, TempEnable, UseShapeArg, iButton, ButtonCurVal;
  char TempString1[ FILENAME_MAX];
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  //x/char TempString[ 256];

  if( iOverlay < 0 || iOverlay >= pToolData->nOverlays) {  // NOT in range

    if( BrowserSelected != -1) {                   // Selection will change

      BrowserSelected = -1;                        // No line is selected

      // Update frame color
      pToolData->YaIPS_ImageDisp.pImage_Box->redraw();

      if(YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

        YaIPS_BigImageDisp.pImage_Box->redraw();
      }
    }

    pInputName->value( "");
    IqeB_GUI_WidgetActivate( pInputName, false); // Set item activated/inactive

    // AOI ...
    IqeB_GUI_WidgetActivate( pFloat_AOI_X, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pFloat_AOI_Y, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pFloat_AOI_XX, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pFloat_AOI_YY, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pAOI_SizeRatio, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pMButAlign, false); // Set item activated/inactive

    pShowSizeRatio->value( "");
    IqeB_GUI_WidgetActivate( pShowSizeRatio, false); // Set item activated/inactive

    // Shape rotation
    IqeB_GUI_WidgetActivate( pRotAngle, false); // Set item activated/inactive

    // Alpha mulitpier
    IqeB_GUI_WidgetActivate( pSliderAlphaMult, false); // Set item activated/inactive

    // Background type
    IqeB_GUI_WidgetActivate( pBGndType_0, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pBGndType_1, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pBGndType_2, false); // Set item activated/inactive
    pBGndType_0->value( 0);
    pBGndType_1->value( 0);
    pBGndType_2->value( 0);

    // Background color
    IqeB_GUI_WidgetActivate( pButCol_LT, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pButCol_RT, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pButCol_LB, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pButCol_RB, false); // Set item activated/inactive

    pButCol_LT->color( FL_BLACK); pButCol_LT->parent()->redraw();
    pButCol_RT->color( FL_BLACK); pButCol_RT->parent()->redraw();
    pButCol_LB->color( FL_BLACK); pButCol_LB->parent()->redraw();
    pButCol_RB->color( FL_BLACK); pButCol_RB->parent()->redraw();

    IqeB_GUI_WidgetActivate( pCheck_Bit_LT, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pCheck_Bit_RT, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pCheck_Bit_LB, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pCheck_Bit_RB, false); // Set item activated/inactive

    // Background image
    pBGndFileName->value( "");   // NOTE: This is always read only, so no need for IqeB_GUI_WidgetActivate().
    IqeB_GUI_WidgetActivate( pBGndFileName, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pBGndFileLoad, false); // Set item activated/inactive

    // Background window
    YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, -1, pBGnd_Win_Box);
    IqeB_GUI_WidgetActivate( pBGnd_Win_Box, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pBGnd_Win_But, false); // Set item activated/inactive

    // Background image/tool window info
    pBGndFileInfo->value( "");   // NOTE: This is always read only, so no need for IqeB_GUI_WidgetActivate().
    IqeB_GUI_WidgetActivate( pBGndFileInfo, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pBGndFileCpSize, false); // Set item activated/inactive

    // Shape generation ...
    IqeB_GUI_WidgetActivate( pChoiceShape, false); // Set item activated/inactive
    pChoiceShape->value( 0);

    IqeB_GUI_WidgetActivate( pSlider_ShapeArg, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pCheck_Bit_ShapeFill, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pCheck_Bit_ShapeOutline, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pCheck_Bit_LineColUse, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pButCol_LineColor, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pInt_LineWidth, false); // Set item activated/inactive

    // Text display
    pFontName->value( "");   // NOTE: This is always read only, so no need for IqeB_GUI_WidgetActivate().
    IqeB_GUI_WidgetActivate( pFontChoose, false); // Set item activated/inactive

    IqeB_GUI_WidgetActivate( pInt_FontSize, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pSizeChoose, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pButCol_FontColor, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pCheck_Bit_FontColUse, false); // Set item activated/inactive

    IqeB_GUI_WidgetActivate( pInt_SpacingLine, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pInt_SpacingChar, false); // Set item activated/inactive

    IqeB_GUI_WidgetActivate( pButFontBold, false); // Set item activated/inactive
    pButFontBold->color( FL_BACKGROUND_COLOR);
    pButFontBold->redraw();

    IqeB_GUI_WidgetActivate( pButFontItalic, false); // Set item activated/inactive
    pButFontItalic->color( FL_BACKGROUND_COLOR);
    pButFontItalic->redraw();

    IqeB_GUI_WidgetActivate( pButFontUnterl, false); // Set item activated/inactive
    pButFontUnterl->color( FL_BACKGROUND_COLOR);
    pButFontUnterl->redraw();

    for( iButton = 0; iButton < 3; iButton++) {

      IqeB_GUI_WidgetActivate( pButAlignHor[ iButton], false); // Set item activated/inactive
      pButAlignHor[ iButton]->color( FL_BACKGROUND_COLOR);
      pButAlignHor[ iButton]->redraw();
    }

    for( iButton = 0; iButton < 3; iButton++) {

      IqeB_GUI_WidgetActivate( pButAlignVer[ iButton], false); // Set item activated/inactive
      pButAlignVer[ iButton]->color( FL_BACKGROUND_COLOR);
      pButAlignVer[ iButton]->redraw();
    }

    IqeB_GUI_WidgetActivate( pInt_IndentHor, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pInt_IndentVer, false); // Set item activated/inactive

    IqeB_GUI_WidgetActivate( app_editor, false);     // Set item activated/inactive
    app_editor->buffer( pEmptyTextBuffer);           // Detach text buffer

    // Shadow
    IqeB_GUI_WidgetActivate( pCheck_Bit_ShadowUse, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pShadowAngle, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pInt_ShadowDist, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pInt_ShadowBlur, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pInt_ShadowTrans, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pButCol_ShadowColor, false); // Set item activated/inactive
    IqeB_GUI_WidgetActivate( pCheck_Bit_ShadowColUse, false); // Set item activated/inactive

    // Update the buttons
    goto UpdateButtonEnables;
  }

  if( BrowserSelected != iOverlay) {             // Selection will change

    BrowserSelected = iOverlay;                    // This line is selected

    // Update frame color
    pToolData->YaIPS_ImageDisp.pImage_Box->redraw();

    if(YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

      YaIPS_BigImageDisp.pImage_Box->redraw();
    }
  }

  // update input fields

  pInputName->value( pToolData->Overlay[ iOverlay].Name);
  IqeB_GUI_WidgetActivate( pInputName, true); // Set item activated/inactive

  // AOI ...
#ifdef use_again
  memcpy( &CurrentAOI, &pToolData->Overlay[ iOverlay].AOI, sizeof( Fl_YaIPS_AOI_t));
#else
  CurrentPosX  = pToolData->Overlay[ iOverlay].PosX;
  CurrentPosY  = pToolData->Overlay[ iOverlay].PosY;
  CurrentSizeX = pToolData->Overlay[ iOverlay].SizeX;
  CurrentSizeY = pToolData->Overlay[ iOverlay].SizeY;
#endif

  IqeB_GUI_WidgetActivate( pFloat_AOI_X, pToolData->TeachMode); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pFloat_AOI_Y, pToolData->TeachMode); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pFloat_AOI_XX, pToolData->TeachMode); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pFloat_AOI_YY, pToolData->TeachMode); // Set item activated/inactive

  // Set AOI values

  // Ensure min/maximus of AOI data is correct
  Fl_RGB_Image *pImgIn1;
  int ImgXX, ImgYY;

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

  YaIPS_RotatableAOI_ClipGuiUpdate( ImgXX, ImgYY);

  IqeB_GUI_WidgetActivate( pAOI_SizeRatio, pToolData->TeachMode); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pMButAlign, pToolData->TeachMode); // Set item activated/inactive

  // Shape generation ...
  memcpy( &CurrentShapeGen, &pToolData->Overlay[ iOverlay].ShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));

  pAOI_SizeRatio->label( (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_AOI_SIZE_RATIO) ? "@+1PadlockC" : "@+1PadlockO");
  pAOI_SizeRatio->redraw();

  // Shape rotation
  IqeB_GUI_WidgetActivate( pRotAngle, pToolData->TeachMode); // Set item activated/inactive
  pRotAngle->SetValue( CurrentShapeGen.RotAngle);

  // Alpha mulitpier
  IqeB_GUI_WidgetActivate( pSliderAlphaMult, true /* pToolData->TeachMode*/); // Set item activated/inactive
  pSliderAlphaMult->value( CurrentShapeGen.AlphaMult);

  // Update size ratio

  IqeB_GUI_Param_Update_SizeRatio();              // Update size ration
  IqeB_GUI_WidgetActivate( pShowSizeRatio, true); // Set item

  // Background type
  IqeB_GUI_WidgetActivate( pBGndType_0, true); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pBGndType_1, true); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pBGndType_2, true); // Set item activated/inactive
  pBGndType_0->value( CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_COLOR);
  pBGndType_1->value( CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_IMAGE);
  pBGndType_2->value( CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_WINDOW);

  // Background color
  IqeB_GUI_WidgetActivate( pCheck_Bit_LT, CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_COLOR); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pCheck_Bit_RT, CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_COLOR); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pCheck_Bit_LB, CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_COLOR); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pCheck_Bit_RB, CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_COLOR); // Set item activated/inactive

  pCheck_Bit_LT->UpdateValue();
  pCheck_Bit_RT->UpdateValue();
  pCheck_Bit_LB->UpdateValue();
  pCheck_Bit_RB->UpdateValue();

  IqeB_GUI_WidgetActivate( pButCol_LT, CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_COLOR &&
                                       (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_CBIT_LT)); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pButCol_RT, CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_COLOR &&
                                       (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_CBIT_RT)); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pButCol_LB, CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_COLOR &&
                                       (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_CBIT_LB)); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pButCol_RB, CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_COLOR &&
                                       (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_CBIT_RB)); // Set item activated/inactive

  pButCol_LT->color( CurrentShapeGen.BGndCol_LT); pButCol_LT->parent()->redraw();
  pButCol_RT->color( CurrentShapeGen.BGndCol_RT); pButCol_RT->parent()->redraw();
  pButCol_LB->color( CurrentShapeGen.BGndCol_LB); pButCol_LB->parent()->redraw();
  pButCol_RB->color( CurrentShapeGen.BGndCol_RB); pButCol_RB->parent()->redraw();

  // Background image
  IqeB_FileGetFileName( CurrentShapeGen.BGndFileName, TempString1, sizeof( TempString1));   // Get filename without path
  if( strcmp( TempString1, pBGndFileName->value()) != 0) { // Is different form displayed file name
    pBGndFileName->value( TempString1);                    // Update displayed file name
    pBGndFileName->insert_position( 0);                    // Position to begin of text
  }
  IqeB_GUI_WidgetActivate( pBGndFileName, CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_IMAGE); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pBGndFileLoad, CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_IMAGE); // Set item activated/inactive

  // Background window
  YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, CurrentShapeGen.BGnd_WinIdNr, pBGnd_Win_Box);

  IqeB_GUI_WidgetActivate( pBGnd_Win_Box, CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_WINDOW); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pBGnd_Win_But, CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_WINDOW); // Set item activated/inactive

  // Background image/tool window info
  TempEnable = CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_IMAGE || CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_WINDOW;
  IqeB_GUI_WidgetActivate( pBGndFileInfo, TempEnable); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pBGndFileCpSize, TempEnable); // Set item activated/inactive

  // Update shape generation ...

  IqeB_GUI_WidgetActivate( pChoiceShape, true); // Set item activated/inactive
  iCurrShape = 0;

  const Fl_Menu_Item *m;

  m = pChoiceShape->find_item_with_argument( CurrentShapeGen.ShapeType);

  if( m != NULL) {                                 // Found it

    iCurrShape = pChoiceShape->find_index( m);

  } else {                                         // NOT found

    CurrentShapeGen.ShapeType = 0;                 // Invalidate shape type
  }

  pChoiceShape->value( iCurrShape);

  TempEnable = CurrentShapeGen.ShapeType != YAIPS_SHAPE_GEN_TYPE_NONE;   // Generate shape

  // Locate shape generation data

  int iShapeGen_List;

  iShapeGen_List = 0;

  for( int i = 0; i < nShapeGen_List; i++) {

    if( CurrentShapeGen.ShapeType == ShapeGen_List[ i].Type) {

      iShapeGen_List = i;
      break;
    }
  }

  // ...

  UseShapeArg = TempEnable && ( ShapeGen_List[ iShapeGen_List].ShapeArgMin != 0 || ShapeGen_List[ iShapeGen_List].ShapeArgMax != 0);

  IqeB_GUI_WidgetActivate( pSlider_ShapeArg, TempEnable && UseShapeArg);    // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pCheck_Bit_ShapeFill, TempEnable);    // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pCheck_Bit_ShapeOutline, TempEnable); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pCheck_Bit_LineColUse, TempEnable &&
                                                  (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_DRAW_LINE)); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pButCol_LineColor, TempEnable &&
                                              (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_DRAW_LINE) && // Set item activated/inactive
                                              (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_LINE_COL_USE));  // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pInt_LineWidth, TempEnable &&
                           (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_DRAW_LINE)); // Set item activated/inactive

  if( UseShapeArg) {

    pSlider_ShapeArg->bounds( ShapeGen_List[ iShapeGen_List].ShapeArgMin, ShapeGen_List[ iShapeGen_List].ShapeArgMax);

    if( CurrentShapeGen.ShapeArg < ShapeGen_List[ iShapeGen_List].ShapeArgMin) {    // Clip minimum

      CurrentShapeGen.ShapeArg = ShapeGen_List[ iShapeGen_List].ShapeArgMin;
    }

    if( CurrentShapeGen.ShapeArg > ShapeGen_List[ iShapeGen_List].ShapeArgMax) {    // Clip maximum

      CurrentShapeGen.ShapeArg = ShapeGen_List[ iShapeGen_List].ShapeArgMax;
    }

    pSlider_ShapeArg->value( CurrentShapeGen.ShapeArg);
  }

  pCheck_Bit_ShapeFill->UpdateValue();
  pCheck_Bit_ShapeOutline->UpdateValue();
  pCheck_Bit_LineColUse->UpdateValue();
  pButCol_LineColor->color( CurrentShapeGen.LineColor); pButCol_LineColor->parent()->redraw();
  pInt_LineWidth->SetValue( CurrentShapeGen.LineWidth);

  // Text display

  if( strcmp( CurrentShapeGen.FontName, pFontName->value()) != 0) { // Is different form displayed file name
    pFontName->value( CurrentShapeGen.FontName);       // Update displayed file name
    pFontName->insert_position( 0);                    // Position to begin of text
  }
  IqeB_GUI_WidgetActivate( pFontChoose, true); // Set item activated/inactive

  IqeB_GUI_WidgetActivate( pInt_FontSize, true); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pSizeChoose, true); // Set item activated/inactive

  // Fill size pull down button

  YaIPS_FontBase_t *pFontBase;
  int FontHeight, FontFace, size_count, BestMatchIdx, BestMatchDelta, DeltaThis;
  int *size_array;

  pFontBase = pYaIPS_Utils_FontsLookup( CurrentShapeGen.FontName);

  FontHeight = CurrentShapeGen.FontSize;

  if( pFontBase != NULL) {             // Got a font

    FontFace = pFontBase->FontNr_regular;

  } else {                             // No font

    FontFace = FL_HELVETICA;           // Fall back
  }

  size_count = Fl::get_font_sizes( FontFace, size_array);

  BestMatchIdx   = -1;
  BestMatchDelta = 0;

  for( int Pass = 0; Pass < 2; Pass ++) {

    if( Pass != 0) {

      pSizeChoose->clear();
    }

    if( size_count <= 0 ||                            // No counts
        (size_count == 1 && size_array[ 0] == 0)) {   // Or only one zero

      static int DefaultSizes[] = { 6, 7, 8, 9, 10, 12, 14, 16, 18, 20, 24, 28, 32, 38, 44, 50, 56, 64, 72, 80, 88, 96};
      static int nDefaultSizes = sizeof( DefaultSizes) / sizeof( int);

      for( int i = 0; i < nDefaultSizes; i++) {

        if( Pass == 0) {

          DeltaThis = FontHeight - DefaultSizes[ i];
          if( DeltaThis < 0) {

            DeltaThis = - DeltaThis;
          }

          if( BestMatchIdx < 0 || DeltaThis < BestMatchDelta) {

            BestMatchDelta = DeltaThis;
            BestMatchIdx = i;
          }

        } else {

          if( i == BestMatchIdx) {

            if( BestMatchDelta == 0) {

              sprintf( TempString1, "%4d=", DefaultSizes[ i]);

            } else {

              sprintf( TempString1, "%4d~", DefaultSizes[ i]);
            }

          } else {

            sprintf( TempString1, "%4d ", DefaultSizes[ i]);
          }

          pSizeChoose->add( TempString1, 0, NULL, (void *)(fl_intptr_t)( DefaultSizes[ i]));
        }
      }

    } else {

      if( size_array[ 0] == 0) {          // First number is a zero

        size_count = 1;                   // Skip first entry
        size_array -= 1;
      }

      for( int i = 0; i < size_count; i++) {

        if( Pass == 0) {

          DeltaThis = FontHeight - size_array[ i];
          if( DeltaThis < 0) {

            DeltaThis = - DeltaThis;
          }

          if( BestMatchIdx < 0 || DeltaThis < BestMatchDelta) {

            BestMatchDelta = DeltaThis;
            BestMatchIdx = i;
          }

        } else {

          if( i == BestMatchIdx) {

            if( BestMatchDelta == 0) {

              sprintf( TempString1, "%4d=", size_array[ i]);

            } else {

              sprintf( TempString1, "%4d~", size_array[ i]);
            }

          } else {

            sprintf( TempString1, "%4d ", size_array[ i]);
          }

          pSizeChoose->add( TempString1, 0, NULL, (void *)(fl_intptr_t)( size_array[ i]));
        }
      }
    }

    if( Pass != 0) {

      pSizeChoose->menu_end();
    }
  }

  // ...

  IqeB_GUI_WidgetActivate( pButCol_FontColor, true); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pCheck_Bit_FontColUse, true); // Set item activated/inactive

  IqeB_GUI_WidgetActivate( pInt_SpacingLine, true); // Set item activated/inactive
  pInt_SpacingLine->SetValue( CurrentShapeGen.SpacingLine);
  IqeB_GUI_WidgetActivate( pInt_SpacingChar, true); // Set item activated/inactive
  pInt_SpacingChar->SetValue( CurrentShapeGen.SpacingChar);

  ButtonCurVal = (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_FONT_BOLD_ON) != 0;
  IqeB_GUI_WidgetActivate( pButFontBold, true); // Set item activated/inactive
  pButFontBold->color( ButtonCurVal ? (FL_BLUE + 7) : FL_BACKGROUND_COLOR);
  pButFontBold->redraw();

  ButtonCurVal = (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_FONT_ITALIC_ON) != 0;
  IqeB_GUI_WidgetActivate( pButFontItalic, true); // Set item activated/inactive
  pButFontItalic->color( ButtonCurVal ? (FL_BLUE + 7) : FL_BACKGROUND_COLOR);
  pButFontItalic->redraw();

  ButtonCurVal = (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_FONT_UNDERL_ON) != 0;
  IqeB_GUI_WidgetActivate( pButFontUnterl, true); // Set item activated/inactive
  pButFontUnterl->color( ButtonCurVal ? (FL_BLUE + 7) : FL_BACKGROUND_COLOR);
  pButFontUnterl->redraw();

  ButtonCurVal = (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_MASK) >> YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_SHIFT;
  for( iButton = 0; iButton < 3; iButton++) {

    IqeB_GUI_WidgetActivate( pButAlignHor[ iButton], true); // Set item activated/inactive
    pButAlignHor[ iButton]->color( iButton == ButtonCurVal ? (FL_BLUE + 7) : FL_BACKGROUND_COLOR);
    pButAlignHor[ iButton]->redraw();
  }

  ButtonCurVal = (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_MASK) >> YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_SHIFT;
  for( iButton = 0; iButton < 3; iButton++) {

    IqeB_GUI_WidgetActivate( pButAlignVer[ iButton], true); // Set item activated/inactive
    pButAlignVer[ iButton]->color( iButton == ButtonCurVal ? (FL_BLUE + 7) : FL_BACKGROUND_COLOR);
    pButAlignVer[ iButton]->redraw();
  }

  pInt_FontSize->SetValue( CurrentShapeGen.FontSize);
  pCheck_Bit_FontColUse->UpdateValue();
  pButCol_FontColor->color( CurrentShapeGen.FontColor); pButCol_FontColor->parent()->redraw();

  IqeB_GUI_WidgetActivate( pInt_IndentHor, true); // Set item activated/inactive
  if( (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_MASK) == YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_CENTER) {               // New align mode is center

    pInt_IndentHor->ChangeMinMax( -VALUE_INDENT_MAX, VALUE_INDENT_MAX);  // Allow negative values

  } else {                                                               // Align mode left or right

    pInt_IndentHor->ChangeMinMax( 0, VALUE_INDENT_MAX);                  // Only positive values allowed
  }
  pInt_IndentHor->SetValue( CurrentShapeGen.IndentHor);

  IqeB_GUI_WidgetActivate( pInt_IndentVer, true); // Set item activated/inactive
  if( (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_MASK) == YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_CENTER) {               // New align mode is center

    pInt_IndentVer->ChangeMinMax( -VALUE_INDENT_MAX, VALUE_INDENT_MAX);  // Allow negative values

  } else {                                                               // Align mode left or right

    pInt_IndentVer->ChangeMinMax( 0, VALUE_INDENT_MAX);                  // Only positive values allowed
  }
  pInt_IndentVer->SetValue( CurrentShapeGen.IndentVer);

  IqeB_GUI_WidgetActivate( app_editor, true);                            // Set item activated/inactive
  if( pToolData->Overlay[ iOverlay].pTextBuffer != NULL) {               // Have a text buffer to set

    if( app_editor->buffer() != pToolData->Overlay[ iOverlay].pTextBuffer) {  // Buffer is not set

      if( app_editor->buffer() != pEmptyTextBuffer) {                   // Have not the empty text buffer

        app_editor->buffer( pEmptyTextBuffer);                          // Detach text buffer
      }

      app_editor->buffer( pToolData->Overlay[ iOverlay].pTextBuffer);   // Attach text buffer
    }
  } else {                                                              // No text buffer to attach

    if( app_editor->buffer() != pEmptyTextBuffer) {                     // Have not the empty text buffer

      app_editor->buffer( pEmptyTextBuffer);                            // Detach text buffer
    }
  }

  // Shadow
  IqeB_GUI_WidgetActivate( pCheck_Bit_ShadowUse, true); // Set item activated/inactive
  pCheck_Bit_ShadowUse->UpdateValue();

  TempEnable = (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_SHADOW_USE) != 0;   // Generate shape

  IqeB_GUI_WidgetActivate( pShadowAngle, TempEnable);     // Set item activated/inactive
  pShadowAngle->SetValue( CurrentShapeGen.ShadowAngle);

  IqeB_GUI_WidgetActivate( pInt_ShadowDist, TempEnable); // Set item activated/inactive
  pInt_ShadowDist->SetValue( CurrentShapeGen.ShadowDist);

  IqeB_GUI_WidgetActivate( pInt_ShadowBlur, TempEnable); // Set item activated/inactive
  pInt_ShadowBlur->SetValue( CurrentShapeGen.ShadowBlur);

  IqeB_GUI_WidgetActivate( pInt_ShadowTrans, TempEnable); // Set item activated/inactive
  pInt_ShadowTrans->SetValue( CurrentShapeGen.ShadowTrans);

  IqeB_GUI_WidgetActivate( pButCol_ShadowColor, TempEnable); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pCheck_Bit_ShadowColUse, TempEnable); // Set item activated/inactive
  pCheck_Bit_ShadowColUse->UpdateValue();
  pButCol_ShadowColor->color( CurrentShapeGen.ShadowColor); pButCol_ShadowColor->parent()->redraw();

  // Update the buttons

UpdateButtonEnables:

  BrowserSize  = pOverlayBrowser->size();
  BrowserValue = pOverlayBrowser->value();

  IqeB_GUI_WidgetActivate( pGUI_ButInsert , pToolData->nOverlays < OVERLAY_NUM_MAX);   // Have space for one more overlay

  IqeB_GUI_WidgetActivate( pGUI_ButAppend , pToolData->nOverlays < OVERLAY_NUM_MAX);   // Have space for one more overlay

  IqeB_GUI_WidgetActivate( pGUI_ButDelete , pToolData->nOverlays > 0 &&    // Have overlays
                                            BrowserSize > 0 &&             // The browser shows something
                                            BrowserValue > 0);             // An item is selected

  IqeB_GUI_WidgetActivate( pGUI_ButDelAll , pToolData->nOverlays > 0);     // Have overlays


  IqeB_GUI_WidgetActivate( pGUI_ButShiftUp , pToolData->nOverlays > 0 &&    // Have overlays
                                             BrowserSize > 1 &&             // The browser shows tow or more lines
                                             BrowserValue > 1);             // An item other then first is selected

  IqeB_GUI_WidgetActivate( pGUI_ButShiftDown , pToolData->nOverlays > 0 &&  // Have overlays
                                               BrowserSize > 1 &&           // The browser shows tow or more lines
                                               BrowserValue > 0 &&          // An item is selected
                                               BrowserValue < pToolData->nOverlays); // Not the last is selected
}

/************************************************************************************
 * IqeB_GUI_ToolsAnimManagerUpdate
 *
 * Updates the animation view
 */

static void BrowserLineText( char *pTextOut, int iLine)
{
  YaIPS_OverlayData_t *pOverlay;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  pOverlay = pToolData->Overlay + iLine;

  if( pOverlay->Name[ 0] != '\0') {

    sprintf( pTextOut, "%2d: %s", iLine + 1, pOverlay->Name);

  } else {

    char *pShapeName, *p;
    int ShapeType, iShapeGen_List;

    // If we have a text, prefer display of the text

    if( pOverlay->pTextBuffer != NULL &&
        pOverlay->pTextBuffer->length() > 0) {

      int LenText, iText, nChars, nBytesPerChar;
      char *pText;
      char TempString2[ 512];

      sprintf( pTextOut, "@C152@.%2d: ", iLine + 1);      // Prefix for browser line

      LenText = pOverlay->pTextBuffer->length();

      pText = pOverlay->pTextBuffer->address( 0);

      iText = 0;
      nChars = 0;

      while( iText < LenText) {

        if( *pText == '\n') {     // End of line

          memcpy( TempString2 + iText, (char *)" ...", 4);
          iText += 4;

          break;
        }

        nBytesPerChar = utf8_decode( pText);
        if( nBytesPerChar <= 0) {

          break;
        }

        memcpy( TempString2 + iText, pText, nBytesPerChar);

        nChars += 1;

        if( nChars > 18) {

          memcpy( TempString2 + iText, (char *)" ...", 4);
          iText += 4;

          break;
        }

        iText += nBytesPerChar;
        pText += nBytesPerChar;

      }

      TempString2[ iText] = '\0';

      strcat( pTextOut, TempString2);

    } else {

      // If there is no text, display background and shape

      switch( pOverlay->ShapeGen.BGndType) {

      case YAIPS_SHAPE_GEN_BGND_COLOR:

        sprintf( pTextOut, "@C59@.%2d: ", iLine + 1);             // Prefix for browser line

        strcat( pTextOut, LangStringLookup( "&GUI_Overlay_TB_0=Color"));

        break;

      case YAIPS_SHAPE_GEN_BGND_IMAGE:

        sprintf( pTextOut, "@C176@.%2d: ", iLine + 1);             // Prefix for browser line

        strcat( pTextOut, LangStringLookup( "&GUI_Overlay_TB_1=Image"));

        break;

      case YAIPS_SHAPE_GEN_BGND_WINDOW:

        sprintf( pTextOut, "@C72@.%2d: ", iLine + 1);             // Prefix for browser line

        strcat( pTextOut, LangStringLookup( "&GUI_Overlay_TB_2=Tool"));

        break;

      default:

        sprintf( pTextOut, "%2d: ", iLine + 1);             // Prefix for browser line

        strcat( pTextOut, "???");

        break;
      }

      strcat( pTextOut, " - ");

      ShapeType = pOverlay->ShapeGen.ShapeType;

      iShapeGen_List = 0;

      for( int iShape = 0; iShape < nShapeGen_List; iShape++) {

        if( ShapeType == ShapeGen_List[ iShape].Type) {

          iShapeGen_List = iShape;
          break;
        }
      }

      pShapeName = LangStringLookup( ShapeGen_List[ iShapeGen_List].pName);

      p = strrchr( pShapeName, '/');

      if( p != NULL) {

        pShapeName = p + 1;
      }

      strcat( pTextOut, pShapeName);
    }
  }
}

static void IqeB_GUI_ToolsAnimManagerUpdate( int KeepSelection)
{
  int i;
  int LastSelection;
  char TempString[ 512];
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  if( pOverlayBrowser == NULL) {   // security test, need this pointer

    return;
  }

  LastSelection = pOverlayBrowser->value();  // get index of selected item

  pOverlayBrowser->clear();                  // empty the list box

  // test for overlays to display

  if( pToolData->nOverlays <= 0) {

    IqeB_GUI_ToolsAnimManagerUpdateSelected( -1);  // deselect
    pOverlayBrowser->redraw();                     // Redraw it

    return;
  }

  // Add overlays

  for( i = 0; i < pToolData->nOverlays; i++) {

    BrowserLineText( TempString, i);      // Generate a line for the overlay

    pOverlayBrowser->add( TempString);
  }

  if( KeepSelection && LastSelection > 0 && LastSelection <= pOverlayBrowser->size()) {

    pOverlayBrowser->select( LastSelection);         // set to last selected

    pOverlayBrowser->middleline( LastSelection);
    IqeB_GUI_ToolsAnimManagerUpdateSelected( LastSelection - 1);

  } else if( pToolData->nOverlays > 0) {              // have any

    if( BrowserSelected >= 0) {                       // Any selected

      if( BrowserSelected >= pToolData->nOverlays) {  // Security test

        BrowserSelected = pToolData->nOverlays - 1;
      }

      LastSelection = BrowserSelected + 1;

      pOverlayBrowser->select( LastSelection);  // Select previous
      pOverlayBrowser->middleline( LastSelection);
      IqeB_GUI_ToolsAnimManagerUpdateSelected( LastSelection - 1);

    } else {

      pOverlayBrowser->topline( 1);                     // position to top line
      IqeB_GUI_ToolsAnimManagerUpdateSelected( -1);     // deselect
    }
  }

  pOverlayBrowser->redraw();     // Redraw it
}

/************************************************************************************
 * IqeB_AnimManagerBrowser_Callback
 */

static void IqeB_OverlayBrowser_Callback( Fl_Widget *w, void *data)
{
  int this_item;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  if( pToolData->nOverlays <= 0) {

    IqeB_GUI_ToolsAnimManagerUpdateSelected( -1);   // deselect

    return;
  }

  // Select item

  this_item = pOverlayBrowser->value();  // get index of selected item

  if( this_item > 0 && this_item <= pToolData->nOverlays) { // index is in range

    // Have a selection
    this_item -= 1;   // adapt index from 1 based to 0 based table access

    IqeB_GUI_ToolsAnimManagerUpdateSelected( this_item);

  } else {

    // selected the background

    IqeB_GUI_ToolsAnimManagerUpdateSelected( -1);   // deselect
  }
}

/************************************************************************************
 * IqeB_GUI_Input_Name_SetValue_Callback
 */

static void IqeB_GUI_Input_Name_SetValue_Callback( Fl_Widget *w, void *data)
{
  Fl_Input *pInput;
  YaIPS_OverlayData_t *pOverlay;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  pInput = (Fl_Input *)w;

  if( pInput == NULL) {   // security test

    return;
  }

  // security tests

  if( pToolData->nOverlays <= 0 ||
      BrowserSelected < 0 || BrowserSelected >= pToolData->nOverlays) {  // NOT in range

    return;
  }

  pOverlay = pToolData->Overlay + BrowserSelected;

  if( strcmp( pOverlay->Name, pInput->value()) != 0) { // string is different

    memset( pOverlay->Name, 0, sizeof( pOverlay->Name));

    strncpy( pOverlay->Name, pInput->value(), sizeof( pOverlay->Name) - 1);

    // Also update browser

    IqeB_GUI_ToolsAnimManagerUpdate( true);   // Update overlay browser and enables

    pToolData->YaIPS_ImageDisp.Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;    // Set AOI changed flag bit
  }

  // Update graphic
  pToolData->YaIPS_ImageDisp.pImage_Box->redraw();

  if(YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

    YaIPS_BigImageDisp.pImage_Box->redraw();
  }
}

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{
  int iOverlay;
  Fl_RGB_Image *pImgIn1;
  int ImgXX, ImgYY, RedrawOnExit, TempEnable;
  Fl_Widget *pCurrFocus;
  Fl_RGB_Image *pBGndFile;
  char TempString1[ 256];
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  RedrawOnExit = false;

  // Get last selected tab group

  pToolData->Tab_Group_Selected = pTab_Groups->GetTabGroup();

  pCurrFocus = Fl::focus();

  // Color teach toggle button

  IqeB_GUI_WidgetActivate( pTeachToggle, true);   // Always selected

  IqeB_GUI_WidgetLabelColor( pTeachToggle, pToolData->TeachMode ? FL_GREEN : YAIPS_BCOL_BUTTON);

  // Check Scene input

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

  // Check AOI

  for( iOverlay = 0; iOverlay < pToolData->nOverlays && iOverlay < OVERLAY_NUM_MAX; iOverlay++) {

    if( iOverlay == BrowserSelected) {   // This AOI is currently selected

      if( pCurrFocus == pFloat_AOI_X  || pCurrFocus == pFloat_AOI_Y ||               // Input element has keyboard focus ?
          pCurrFocus == pFloat_AOI_XX || pCurrFocus == pFloat_AOI_YY) {

        continue;   // Skip, otherwise inputs below minium values will be skipped
      }

      if( YaIPS_RotatableAOI_ClipGuiUpdate( ImgXX, ImgYY) > 0) {

        // Copy back modified AOI
#ifdef use_again
        memcpy( &pToolData->Overlay[ iOverlay].AOI, &CurrentAOI, sizeof( Fl_YaIPS_AOI_t));
#else
        pToolData->Overlay[ iOverlay].PosX  = CurrentPosX;
        pToolData->Overlay[ iOverlay].PosY  = CurrentPosY;
        pToolData->Overlay[ iOverlay].SizeX = CurrentSizeX;
        pToolData->Overlay[ iOverlay].SizeY = CurrentSizeY;
#endif

        RedrawOnExit = true;                                 // Redraw on exit
        pToolData->Overlay[ iOverlay].ParChanged = 1;        // Recreate intermediate image
      }

      // update background file info

      strcpy( TempString1, "");           // Preset empty
      TempEnable = false;                 // Preset no enable for copy size button
      pBGndFile  = NULL;

      if( CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_IMAGE) {

        pBGndFile = pToolData->Overlay[ BrowserSelected].pBGndFile;

      } else if( CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_WINDOW) {

        pBGndFile = NULL;       // Will be set if there is a valid image

        // NOTE: Argument 'DstWinIdNr' is not needed to check.
        YaIPS_ToolWinInputCheck( -1, pToolData->Overlay[ BrowserSelected].ShapeGen.BGnd_WinIdNr, NULL, &pBGndFile, NULL);
      }

      if( pBGndFile != NULL) {

        sprintf( TempString1, "%d x %d, %s", pBGndFile->data_w(), pBGndFile->data_h(),
                               pYaIPS_PixelDepth_to_string( pBGndFile->d()));

        TempEnable = true;
      }

      if( strcmp( TempString1, pBGndFileInfo->value()) != 0) {

        pBGndFileInfo->value( TempString1);
      }

      IqeB_GUI_WidgetActivate( pBGndFileCpSize, TempEnable); // Set item activated/inactive

    } else {

      if( YaIPS_RotatableAOI_Clip( ImgXX, ImgYY,
                                   pToolData->Overlay + iOverlay) > 0) {

        RedrawOnExit = true;                                 // Redraw on exit
        pToolData->Overlay[ iOverlay].ParChanged = 1;        // Recreate intermediate image
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
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  pToolData->MyParPosX = pMyParWin->x();
  pToolData->MyParPosY = pMyParWin->y();

  #ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::remove_idle( IqeB_GUI_ParIdleAction);      // Redraw window during idle
#endif
  Fl::remove_check( IqeB_GUI_ParIdleAction);     // Check small image size change

  IqeB_GUI_CloseToolWindow( (void **)&pMyParWin);

  pOverlayBrowser = NULL;                        // Must be reseted at exit
  pToolDataParam  = NULL;                        // Must be reseted at exit
  app_editor      = NULL;                        // Must be reseted at exit
}

/************************************************************************************
 * YaIPS_AlphaOp_Callback
 *
 * Alpha operator will change
 */

static void YaIPS_BGndType_Callback( Fl_Widget *w, void *data)
{
  int Value;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  // ...

  Value = (long long)(data);                       // get value to set

  CurrentShapeGen.BGndType = Value;                // Set new value

  if( BrowserSelected >= 0) {                    // Any overlay selected in browser

    if( memcmp( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t)) != 0) { // is different

      pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

      // Copy back modified data
      memcpy( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));
    }

    // update all enables
#ifdef use_again
    IqeB_GUI_ToolsAnimManagerUpdateSelected( BrowserSelected);  // Update enables
#else
    IqeB_GUI_ToolsAnimManagerUpdate( true);   // Update overlay browser and enables
#endif
  }

  pToolData->Input1_Change = 0;                    // Force recalculation output
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
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  if( BrowserSelected < 0) {              // Security test, no overlay selected

    return;
  }

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

    if( BrowserSelected >= 0) {                       // Any selected

      // Remember last loaded file name
      memset( CurrentShapeGen.BGndFileName, 0, sizeof( CurrentShapeGen.BGndFileName));
      strncpy( CurrentShapeGen.BGndFileName, pFileName, sizeof( CurrentShapeGen.BGndFileName) - 1);

      if( memcmp( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t)) != 0) { // is different

        pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

        // Copy back modified data
        memcpy( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));
      }

      // Set new loaded file

      if( pToolData->Overlay[ BrowserSelected].pBGndFile != NULL) {   // Was a file loaded before ?

        pToolData->Overlay[ BrowserSelected].pBGndFile->release();    // Release data of this file
      }

      pToolData->Overlay[ BrowserSelected].pBGndFile = pTempImage;    // Set loaded file

      pTempImage = NULL;                                                       // Invalidate

      // update all enables
      IqeB_GUI_ToolsAnimManagerUpdateSelected( BrowserSelected);  // Update enables
    }

    if( pTempImage != NULL) {                       // Still have remporary image

      pTempImage->release();                        // Release temporary image
    }
  }

  pToolData->Input1_Change = 0;                    // Force recalculation output
}

/************************************************************************************
 * DropFile_cb
 *
 * A file was dropped to the image box
 *
 *  pFileNameArg: Pointer to name of dropped file.
 * pImageDispArg: Pointer to image display of original file drop
 *     SubWinIDx: < 0 = Use w to check for sub window else is sub window index
 *
 * Return: < 0  Error, don't processed mouse callback
 *           0 OK, processed mouse callback
 *
 */

static int DropFile_cb( Fl_Widget *w, void *pFileNameArg, void *pImageDispArg, int SubWinIDx)
{
  YaIPS_ToolData_info_t *pToolData;
  Fl_RGB_Image *pTempImage = NULL;
  char *pFileName;
  int iToolData, iOverlay;
  YaIPS_OverlayData_t *pOverlay;
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;

  // NOTE: If we come to here, no paramter dialog is open

  pYaIPS_ImageDisp = (Fl_YaIPS_ImageDisp_t *)pImageDispArg;

  // Must locate the associated window so search ...

  // check all open windows

  for( iToolData = 0; iToolData < nYaIPS_ToolData_info; iToolData++) {

    if( SubWinIDx >= 0 &&            // Only check for specific sub window
        iToolData != SubWinIDx) {

      continue;
    }

    if( YaIPS_ToolData_info[ iToolData].IsOpen == false) {             // This window is not open

      continue;                             // Skip this element
    }

    // This window is open

    pToolData = YaIPS_ToolData_info + iToolData;               // Point to info data

    if( pToolData->TeachMode == 0) {        // Teach mode is off

      continue;
    }

    // Check for drop callback set

    if( pToolData->YaIPS_ImageDisp.pImage_Box->pDropCallback == NULL) {   // Not for this

      continue;
    }

    // Check for correct widget

    if( SubWinIDx < 0) {           // Check for sub window

      if( pToolData->YaIPS_ImageDisp.pImage_Box != w) {   // Not for this box

        continue;
      }
    }

    // Got it

    pFileName = (char *)pFileNameArg;                   // Filename

    // load and show file

    pTempImage = YaIPS_Image_Read( pFileName);          // Try to load an image

    if( pTempImage == NULL) {           // Got NO image

      goto ErrorExit;
    }

    // Check for mouse over AOI

    iOverlay = -1;    // Invalidate

    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0 &&  // Mouse is over any AOI
        pYaIPS_ImageDisp->AoiIdNr >= 0 ) {                                  // and any AOI selected

      iOverlay = pYaIPS_ImageDisp->AoiIdNr;

    } else {              // Drop outside any existing window

      if( pToolData->nOverlays < OVERLAY_NUM_MAX) {    // have space for a new ovelay

        int xMouse, yMouse, xSize, ySize;

        // Append overlay

        pToolDataParam = pToolData;        // Set data pointer for buttons or other callback functions

        // Outside any window, append after last window

        iOverlay = pToolData->nOverlays;

        ButtonNewCallback( NULL, (void *)(long long)(pToolData->nOverlays - 1 + OVERLAY_NUM_MAX + OVERLAY_NUM_MAX));

        pToolDataParam = NULL;            // Must reset after use

        // Set background type to image

        pOverlay = pToolData->Overlay + iOverlay;

        pOverlay->ShapeGen.BGndType = YAIPS_SHAPE_GEN_BGND_IMAGE;

        // Last mouse coordinate to ...

        xMouse = (int) (pYaIPS_ImageDisp->SubImage_x + 0.5) + (int) (pYaIPS_ImageDisp->MouseX / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
        yMouse = (int) (pYaIPS_ImageDisp->SubImage_y + 0.5) + (int) (pYaIPS_ImageDisp->MouseY / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

        // Size of image

        xSize = pTempImage->w();
        ySize = pTempImage->h();

        // Try to size of overlay into image

        if( pToolData->YaIPS_ImageDisp.pImage_Img != NULL) {

          while( xSize > pToolData->YaIPS_ImageDisp.pImage_Img->w() / 2 ||
                 ySize > pToolData->YaIPS_ImageDisp.pImage_Img->h() / 2) {

            xSize = xSize / 2;
            ySize = ySize / 2;
          }
        }

        xMouse -= xSize / 2;        // Left top corner center of top side

        // Clip to keep center of image inside

        if( xMouse < - xSize / 2) {

          xMouse = - xSize / 2;
        }

        if( xMouse > pToolData->YaIPS_ImageDisp.pImage_Img->w() - xSize / 2) {

          xMouse = pToolData->YaIPS_ImageDisp.pImage_Img->w() - xSize / 2;
        }

        if( yMouse < - ySize / 2) {

          yMouse = - ySize / 2;
        }

        if( yMouse > pToolData->YaIPS_ImageDisp.pImage_Img->h() - ySize / 2) {

          yMouse = pToolData->YaIPS_ImageDisp.pImage_Img->h() - ySize / 2;
        }

        // ...

        pOverlay->AoiUnit = YaIPS_Calib_Unit;                       // Latch unit used on creation of AOI

        pOverlay->PosX  = YaIPS_Calib_PixXVal2Units( xMouse, true);
        pOverlay->PosY  = YaIPS_Calib_PixYVal2Units( yMouse, true);

        pOverlay->SizeX = YaIPS_Calib_PixXVal2Units( xSize, true);
        pOverlay->SizeY = YaIPS_Calib_PixYVal2Units( ySize, true);

        // Lock size of image

        pOverlay->ShapeGen.ShapeFlags |= YAIPS_SHAPE_GEN_FLAG_AOI_SIZE_RATIO;

        // Latch size if just switched to on
        pOverlay->ShapeGen.AOI_XX_Locked = pTempImage->w();
        pOverlay->ShapeGen.AOI_YY_Locked = pTempImage->h();

        IqeB_GUI_Param_RecalcAOI( pOverlay);   // Ensure proper AOI values

      }

    }

    if( iOverlay < 0) {       // no overlay selected

      goto ExitPoint;
    }

    pOverlay = pToolData->Overlay + iOverlay;

    if( pOverlay->ShapeGen.BGndType != YAIPS_SHAPE_GEN_BGND_IMAGE) {  // Not using a background image

      goto ExitPoint;
    }

    pOverlay->ParChanged = 1;         // Recreate intermediate image

    pToolData->Input1_Change = 0;     // Force recalculation output

    // Set new loaded file

    if( pOverlay->pBGndFile != NULL) {   // Was a file loaded before ?

      pOverlay->pBGndFile->release();    // Release data of this file
    }

    pOverlay->pBGndFile = pTempImage;    // Set loaded file

    pTempImage = NULL;                                                       // Invalidate

    // Remember last loaded file name
    memset( pOverlay->ShapeGen.BGndFileName, 0, sizeof( pOverlay->ShapeGen.BGndFileName));
    strncpy( pOverlay->ShapeGen.BGndFileName, pFileName, sizeof( pOverlay->ShapeGen.BGndFileName) - 1);

    // Done, can exit here

ExitPoint:

    if( pTempImage != NULL) {                       // Still have remporary image

      pTempImage->release();                        // Release temporary image
    }

    return( 0);    // OK Processed drop
  }

ErrorExit:

  if( pTempImage != NULL) {                       // Still have remporary image

    pTempImage->release();                        // Release temporary image
  }

  return( -1);   // Error on processing drop
}

/************************************************************************************
 * IqeB_GUI_BGndFileCpSize_Callback
 *
 * Copy size of background image do AOI with/height.
 */

static void IqeB_GUI_BGndFileCpSize_Callback( Fl_Widget *w)
{
  Fl_RGB_Image *pBGndFile;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  if( BrowserSelected < 0) {              // Security test, no overlay selected

    return;
  }

  // Set AOI width/height from background image

  pBGndFile  = NULL;

  if( CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_IMAGE) {

    pBGndFile = pToolData->Overlay[ BrowserSelected].pBGndFile;

  } else if( CurrentShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_WINDOW) {

    pBGndFile = NULL;       // Will be set if there is a valid image

    // NOTE: Argument 'DstWinIdNr' is not needed to check.
    YaIPS_ToolWinInputCheck( -1, pToolData->Overlay[ BrowserSelected].ShapeGen.BGnd_WinIdNr, NULL, &pBGndFile, NULL);
  }

  if( pBGndFile != NULL) {  // Securit test, have a background file

    int XSize, YSize;

    XSize = pBGndFile->data_w();
    YSize = pBGndFile->data_h();

    CurrentSizeX = YaIPS_Calib_PixXVal2Units( XSize, true);
    CurrentSizeY = YaIPS_Calib_PixYVal2Units( YSize, true);

    if( (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_AOI_SIZE_RATIO) != 0) { // Locked sized radio

      CurrentShapeGen.AOI_XX_Locked = XSize;
      CurrentShapeGen.AOI_YY_Locked = YSize;
    }
  }

  // AOI has changed

#ifdef use_again
  if( memcmp( &pToolData->Overlay[ BrowserSelected].AOI, &CurrentAOI, sizeof( Fl_YaIPS_AOI_t)) != 0) { // is different
#else
  if( pToolData->Overlay[ BrowserSelected].PosX  != CurrentPosX  ||
      pToolData->Overlay[ BrowserSelected].PosY  != CurrentPosY  ||
      pToolData->Overlay[ BrowserSelected].SizeX != CurrentSizeX ||
      pToolData->Overlay[ BrowserSelected].SizeY != CurrentSizeY) {
#endif

    pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

    // Copy back modified data
#ifdef use_again
    memcpy( &pToolData->Overlay[ BrowserSelected].AOI, &CurrentAOI, sizeof( Fl_YaIPS_AOI_t));
#else
    pToolData->Overlay[ BrowserSelected].PosX  = CurrentPosX;
    pToolData->Overlay[ BrowserSelected].PosY  = CurrentPosY;
    pToolData->Overlay[ BrowserSelected].SizeX = CurrentSizeX;
    pToolData->Overlay[ BrowserSelected].SizeY = CurrentSizeY;
#endif

    memcpy( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));

    pFloat_AOI_XX->SetValue( CurrentSizeX);
    pFloat_AOI_YY->SetValue( CurrentSizeY);
    IqeB_GUI_Param_Update_SizeRatio();               // Update size ration

    pToolData->Input1_Change = 0;                    // Force recalculation output
  }

}

/************************************************************************************
 * IqeB_GUI_FontChoose_Callback
 *
 * Open an window to choose a font.
 */
static int Font_Catch_OutsideMouseClick_handler( int event);    // Forward

static Fl_Menu_Window  *pFontPopup = nullptr;
static Fl_Hold_Browser *pFontBrowser = nullptr;
static int Font_Catch_ButtonLast = 1;

static void Font_close_popup() {

  if( pFontPopup) {

    Fl::remove_handler( Font_Catch_OutsideMouseClick_handler);

    pFontPopup->hide();
    delete pFontPopup;
    pFontPopup   = nullptr;
    pFontBrowser = nullptr;

  }
}

static int Font_Catch_OutsideMouseClick_handler( int event)
{
  if( 1 /*event == FL_NO_EVENT*/) {

    int sx, sy, ButtonThis;

    // Hack to catch button down in windows systems
    ButtonThis = (GetAsyncKeyState( VK_LBUTTON) & 0x01) || (GetAsyncKeyState( VK_RBUTTON) & 0x01);

    if( ButtonThis != 0 && Font_Catch_ButtonLast == 0) {   // Any mouse buttons down

      if( pFontPopup != NULL) {

        Fl::get_mouse( sx, sy);       // screen coordinates

        int x0 = pFontPopup->x_root();
        int y0 = pFontPopup->y_root();
        int x1 = x0 + pFontPopup->w();
        int y1 = y0 + pFontPopup->h();

        // Check if click is outside modal window
        if( sx < x0 || sx >= x1 || sy < y0 || sy >= y1) {

          // Close the font popup
          Font_close_popup();

          return 1; // event handled
        }
      }
    }

    Font_Catch_ButtonLast = ButtonThis;
  }

  return 0; // let FLTK continue normal processing
}

static void Font_close_cb( Fl_Widget* w, void* data) {

  Font_close_popup();
}

static void Font_select_cb( Fl_Widget* w, void* data) {

  Fl_Hold_Browser* b = (Fl_Hold_Browser*)w;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  char *pFontText, *p2;
  int sel = b->value();

  if( sel > 0) {

    pFontText = (char *)b->text( sel);

    // Find formating character '@.' and set pointer after this
    p2 = pFontText;
    if( p2[ 0] != '\0')  {

      for( ; p2[ 1] != '\0'; p2++) {

        if( p2[ 0] == '@' && p2[ 1] == '.') {  // Got '@.'

          pFontText = p2 + 2;
          break;
        }
      }
    }

    memset( CurrentShapeGen.FontName, 0, sizeof( CurrentShapeGen.FontName));
    strncpy( CurrentShapeGen.FontName, pFontText, sizeof( CurrentShapeGen.FontName));

    if( BrowserSelected >= 0) {                       // Any selected

      memset( pToolData->Overlay[ BrowserSelected].ShapeGen.FontName, 0, sizeof( pToolData->Overlay[ BrowserSelected].ShapeGen.FontName));
      strncpy( pToolData->Overlay[ BrowserSelected].ShapeGen.FontName, pFontText, sizeof( pToolData->Overlay[ BrowserSelected].ShapeGen.FontName));
    }

    if( strcmp( CurrentShapeGen.FontName, pFontName->value()) != 0) { // Is different form displayed file name

      pFontName->value( CurrentShapeGen.FontName);                    // Update displayed file name
    }

    // Rebuild image
    if( BrowserSelected >= 0) {         // Any overlay selected in browser

      pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

      pToolData->Input1_Change = 0;          // Force recalculation output
    }
  }

  Font_close_popup();
}

static void Font_open_popup( int xPos, int yPos, int Width, int Height)
{
  int LineSelect;

  if( pFontBrowser) {
    return; // already open
  }

  pFontPopup = new Fl_Menu_Window( xPos, yPos, Width, Height, LangStringLookup( "&GUI_Overlay_FontSel=Font selection"));
  pFontPopup->box(  FL_UP_BOX /*FL_BORDER_BOX*/);
  pFontPopup->color( YAIPS_COLOR_SELECTION);     // Color background
  pFontPopup->callback( Font_close_cb, NULL);
  pFontPopup->set_menu_window();
  pFontPopup->clear_border();     // Don't show the window frame

  pFontBrowser = new Fl_Hold_Browser( 3, 3, Width - 6, Height - 6);
  pFontBrowser->callback( Font_select_cb, NULL);

  // Add fonts

  LineSelect = -1;             // Preset, no line select

  for( int i = 0; i < YaIPS_nFontBase; i++) {

    char TempBuffer[ 512];

    if( pYaIPS_FontBase[ i].FontNr_regular >= 0) {

      sprintf( TempBuffer, "@F%d@S16@.%s", pYaIPS_FontBase[ i].FontNr_regular, pYaIPS_FontBase[ i].FontName);

#ifdef _DEBUG
#ifdef use_again
      char TempBuffer2[ 512];

      int *s; int n = Fl::get_font_sizes((Fl_Font)pYaIPS_FontBase[ i].FontNr_regular, s);

      if( n > 1) {
        sprintf( TempBuffer2, " - %d, %d/%d/%d", n, s[ 0], s[ 1], s[ n - 1]);

      } else if( n > 0) {

        sprintf( TempBuffer2, " - %d, %d", n, s[ 0]);

      } else {

        sprintf( TempBuffer2, " - XXX");
      }

      strcat( TempBuffer, TempBuffer2);
#endif
#endif

    } else {
      sprintf( TempBuffer, "%s", pYaIPS_FontBase[ i].FontName);
    }

    pFontBrowser->add( TempBuffer, (void *)(fl_intptr_t)i);

    if( BrowserSelected >= 0) {        // Any overlay selected in browser

      if( strcmp( pYaIPS_FontBase[ i].FontName, CurrentShapeGen.FontName) == 0) {  // Added this font

        LineSelect = i;   // Select on pop up
      }
    }
  }

  pFontPopup->end();

  if( LineSelect >= 0) {

    pFontBrowser->select( LineSelect + 1);
  }

  pFontPopup->set_modal();
  pFontPopup->show();

  Fl::wait();

  Font_Catch_ButtonLast = 1;
  Fl::add_handler( Font_Catch_OutsideMouseClick_handler);
}

static void IqeB_GUI_FontChoose_Callback( Fl_Widget *w)
{
  int xPos, yPos, Width, Height;

  if( BrowserSelected < 0) {              // Security test, no overlay selected

    return;
  }

  // Open pop up

  Width  = 196; // With of drop down
  Height = 236; // Height of drop down

  xPos = w->x() + w->window()->x_root() - Width + w->w();
  yPos = w->y() + w->window()->y_root() + w->h() + 0;

  Font_open_popup( xPos, yPos, Width, Height);
}

/************************************************************************************
 * IqeB_GUI_FontSizeButton_Callback
 *
 * Callback, font size selection button has changed.
 */

static void IqeB_GUI_FontSizeButton_Callback( Fl_Widget *w)
{
  int FontSizeNew;
  Fl_Menu_Button *pMenu_Button;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  pMenu_Button = (Fl_Menu_Button *)w;

  if( BrowserSelected < 0) {              // Security test, no overlay selected

    return;
  }

  // Return a pointer to the last menu item that was picked
  const Fl_Menu_Item *m = pMenu_Button->mvalue();

  if( m != NULL) {       // A menu line was selected

    FontSizeNew = (int)(uintptr_t)(m->user_data_);

    if( FontSizeNew != CurrentShapeGen.FontSize) {    // Font size will change

      CurrentShapeGen.FontSize = FontSizeNew;

      pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

      // Copy back modified data
      memcpy( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));

      // update all enables/GUI
      IqeB_GUI_ToolsAnimManagerUpdateSelected( BrowserSelected);  // Update enables

      pToolData->Input1_Change = 0;                    // Force recalculation output
    }
  }
}

/************************************************************************************
 * IqeB_AOI_Align_Callback
 *
 * Called on press of the aling menu button.
 */

static void IqeB_AOI_Align_Callback( Fl_Widget *w)
{
  int AlignType;
  Fl_Menu_Button *pMenu_Button;
  YaIPS_OverlayData_t *pOverlay;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;
  int XPos, YPos;

  pMenu_Button = (Fl_Menu_Button *)w;

  if( BrowserSelected < 0) {              // Security test, no overlay selected

    return;
  }

  if( pToolData->YaIPS_ImageDisp.pImage_Img == NULL) { // Security test, need this image

    return;
  }

  pOverlay = pToolData->Overlay + BrowserSelected;

  // Return a pointer to the last menu item that was picked
  const Fl_Menu_Item *m = pMenu_Button->mvalue();

  if( m != NULL) {       // A menu line was selected

    AlignType = (int)(uintptr_t)(m->user_data_);  // Get type of align

    XPos = pOverlay->AOI.XPos;  // Latch current AOI position
    YPos = pOverlay->AOI.YPos;


    switch( AlignType) {

    case 1: // Left
      XPos = 0;
      break;

    case 2: // Centered
      XPos = (pToolData->YaIPS_ImageDisp.pImage_Img->w() - pOverlay->AOI.XSize) / 2;
      break;

    case 3: // Right
      XPos = pToolData->YaIPS_ImageDisp.pImage_Img->w() - pOverlay->AOI.XSize - 1;
      break;

    case 4: // Top
      YPos = 0;
      break;

    case 5: // Middle
      YPos = (pToolData->YaIPS_ImageDisp.pImage_Img->h() - pOverlay->AOI.YSize) / 2;
      break;

    case 6: // Bottom
      YPos = pToolData->YaIPS_ImageDisp.pImage_Img->h() - pOverlay->AOI.YSize - 1;
      break;
    }

    if( XPos != pOverlay->AOI.XPos ||     // Any position has changed
        YPos != pOverlay->AOI.XPos) {

      pOverlay->AOI.XPos = XPos;
      pOverlay->AOI.YPos = YPos;

      // Update AOI input fields

      pOverlay->PosX  = pOverlay->AOI.XPos  * YaIPS_Calib_UPP_X;
      pOverlay->PosY  = pOverlay->AOI.YPos  * YaIPS_Calib_UPP_Y;

      pOverlay->ParChanged = 1;        // Recreate intermediate image

      // Copy back modified data
      memcpy( &pOverlay->ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));

      // update all enables/GUI
      IqeB_GUI_ToolsAnimManagerUpdateSelected( BrowserSelected);  // Update enables

      pToolData->Input1_Change = 0;                    // Force recalculation output
    }
  }
}

/************************************************************************************
 * IqeB_GUI_Float_SetValue_Callback
 *
 * Callback, set a float or double value
 */

static void IqeB_GUI_Float_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  float *pValue;
  float Value;
  int WasClipped, UpdateGUI;
  IqeFl_Float_Input *pThis;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

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

  // Reformat number on GUI

  pThis->SetValue( Value);

  *pValue = Value;

  UpdateGUI = false;

  if( BrowserSelected >= 0) {                     // Any overlay selected in browser

    if( (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_AOI_SIZE_RATIO) != 0 && // Locked sized radio
        CurrentShapeGen.AOI_XX_Locked > 0 && CurrentShapeGen.AOI_YY_Locked) {      // and latched sizes are set

      if( pValue == &CurrentSizeX) {              // Changed AOI X Size

        // update Y Size depending from radio
        CurrentSizeY = YaIPS_Calib_FixUnitsAfterPoint( (CurrentSizeX * CurrentShapeGen.AOI_YY_Locked) / CurrentShapeGen.AOI_XX_Locked);

      } else if( pValue == &CurrentSizeY) {       // Changed AOI Y Size

        // update X Size depending from radio
        CurrentSizeX = YaIPS_Calib_FixUnitsAfterPoint( (CurrentSizeY * CurrentShapeGen.AOI_XX_Locked) / CurrentShapeGen.AOI_YY_Locked);
      }
    }

    if( pValue == &CurrentSizeX ||          // Changed AOI X Size
        pValue == &CurrentSizeY) {          // Changed AOI Y Size

      IqeB_GUI_Param_Update_SizeRatio();    // Update size ration
    }

#ifdef use_again
    if( memcmp( &pToolData->Overlay[ BrowserSelected].AOI, &CurrentAOI, sizeof( Fl_YaIPS_AOI_t)) != 0) { // is different
#else
    if( pToolData->Overlay[ BrowserSelected].PosX  != CurrentPosX  ||
        pToolData->Overlay[ BrowserSelected].PosY  != CurrentPosY  ||
        pToolData->Overlay[ BrowserSelected].SizeX != CurrentSizeX ||
        pToolData->Overlay[ BrowserSelected].SizeY != CurrentSizeY) {
#endif

      pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

      // Copy back modified data
#ifdef use_again
      memcpy( &pToolData->Overlay[ BrowserSelected].AOI, &CurrentAOI, sizeof( Fl_YaIPS_AOI_t));
#else
      pToolData->Overlay[ BrowserSelected].PosX  = CurrentPosX;
      pToolData->Overlay[ BrowserSelected].PosY  = CurrentPosY;
      pToolData->Overlay[ BrowserSelected].SizeX = CurrentSizeX;
      pToolData->Overlay[ BrowserSelected].SizeY = CurrentSizeY;
#endif
    }

    if( memcmp( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t)) != 0) { // is different

      if( pValue != &CurrentShapeGen.RotAngle) {          // If not angle

        pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image
      }

      // Copy back modified data
      memcpy( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));
    }
  }

  if( UpdateGUI) {

    // update all enables
    IqeB_GUI_ToolsAnimManagerUpdateSelected( BrowserSelected);  // Update enables
  }

  pToolData->Input1_Change = 0;                    // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_Int_SetValue_Callback
 */

static void IqeB_GUI_Int_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  int Value, *pValue, WasClipped, UpdateGUI;
  IqeFl_Int_Input *pThis;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

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

  UpdateGUI = false;

  if( BrowserSelected >= 0) {              // Any overlay selected in browser

    if( memcmp( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t)) != 0) { // is different

      pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

      // Copy back modified data
      memcpy( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));

      if( pValue == &CurrentShapeGen.FontSize) {   // Font size has changed

        UpdateGUI= true;
      }
    }
  }

  if( UpdateGUI) {

    // update all enables
    IqeB_GUI_ToolsAnimManagerUpdateSelected( BrowserSelected);  // Update enables
  }

  pToolData->Input1_Change = 0;                    // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_But_Color_SetValue_Callback
 */

static void IqeB_GUI_But_Color_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  unsigned int *pColor, ColorBefore;
  Fl_Button *pThis;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  // ...

  pThis  = (Fl_Button *)w;
  pColor = (unsigned int *)pValueArg;    // get pointer to associated variable
  ColorBefore = *pColor;

  *pColor = IqeB_GUI_ColorChooser( *pColor);

  if( ColorBefore != *pColor) {

    pThis->color( *pColor);
    pThis->parent()->redraw();

    if( BrowserSelected >= 0) {                    // Any overlay selected in browser

      if( memcmp( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t)) != 0) { // is different

        pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

        // Copy back modified data
        memcpy( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));
      }
    }
  }

  pToolData->Input1_Change = 0;                    // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_FontStyle_Callback
 *
 * Is used for bold, italic and underline font style toggle
 */

static void IqeB_GUI_FontStyle_Callback( Fl_Widget *w, long int ValueArg)
{
  int StyleBit, ButtonCurVal;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  // ...

  StyleBit = (int)ValueArg;      // Is on of the style flag bit values

  CurrentShapeGen.ShapeFlags ^= StyleBit;    // Toggle style bit

  if( BrowserSelected >= 0) {                         // Any overlay selected in browser

    // always a value will change

    ButtonCurVal = (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_FONT_BOLD_ON) != 0;
    //x/IqeB_GUI_WidgetActivate( pButFontBold, true); // Set item activated/inactive
    pButFontBold->color( ButtonCurVal ? (FL_BLUE + 7) : FL_BACKGROUND_COLOR);
    pButFontBold->redraw();

    ButtonCurVal = (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_FONT_ITALIC_ON) != 0;
    //x/IqeB_GUI_WidgetActivate( pButFontItalic, true); // Set item activated/inactive
    pButFontItalic->color( ButtonCurVal ? (FL_BLUE + 7) : FL_BACKGROUND_COLOR);
    pButFontItalic->redraw();

    ButtonCurVal = (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_FONT_UNDERL_ON) != 0;
    //x/IqeB_GUI_WidgetActivate( pButFontUnterl, true); // Set item activated/inactive
    pButFontUnterl->color( ButtonCurVal ? (FL_BLUE + 7) : FL_BACKGROUND_COLOR);
    pButFontUnterl->redraw();

    if( memcmp( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t)) != 0) { // is different

      pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

      // Copy back modified data
      memcpy( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));
    }

    pToolData->Input1_Change = 0;                    // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_FontAlingHor_Callback
 */

static void IqeB_GUI_FontAlingHor_Callback( Fl_Widget *w, long int ValueArg)
{
  int ValueNew, ValueBefore, iButton;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  // ...

  ValueNew = (int)ValueArg >> YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_SHIFT;

  for( iButton = 0; iButton < 3; iButton++) {

    pButAlignHor[ iButton]->color( iButton == ValueNew ? (FL_BLUE + 7) : FL_BACKGROUND_COLOR);

    pButAlignHor[ iButton]->redraw();
  }

  ValueBefore = CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_MASK;  // Get current align setting

  ValueNew = ValueArg & YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_MASK;        // Security: isolate align horizontal bits

  if( BrowserSelected >= 0 &&                    // Any overlay selected in browser
      ValueNew != ValueBefore) {                 // and value will change

    CurrentShapeGen.ShapeFlags &= ~ YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_MASK;
    CurrentShapeGen.ShapeFlags |= ValueNew;

    if( ValueNew == YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_CENTER) {               // New align mode is center

      pInt_IndentHor->ChangeMinMax( -VALUE_INDENT_MAX, VALUE_INDENT_MAX);  // Allow negative values

    } else {                                                               // Align mode left or right

      pInt_IndentHor->ChangeMinMax( 0, VALUE_INDENT_MAX);                  // Only positive values allowed
    }

    if( memcmp( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t)) != 0) { // is different

      pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

      // Copy back modified data
      memcpy( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));
    }

    pToolData->Input1_Change = 0;                    // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_FontAlingVer_Callback
 */

static void IqeB_GUI_FontAlingVer_Callback( Fl_Widget *w, long int ValueArg)
{
  int ValueNew, ValueBefore, iButton;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  // ...

  ValueNew = (int)ValueArg >> YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_SHIFT;

  for( iButton = 0; iButton < 3; iButton++) {

    pButAlignVer[ iButton]->color( iButton == ValueNew ? (FL_BLUE + 7) : FL_BACKGROUND_COLOR);

    pButAlignVer[ iButton]->redraw();
  }

  ValueBefore = CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_MASK;  // Get current align setting

  ValueNew = ValueArg & YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_MASK;        // Security: isolate align vertical bits

  if( BrowserSelected >= 0 &&                    // Any overlay selected in browser
      ValueNew != ValueBefore) {                 // and value will change

    CurrentShapeGen.ShapeFlags &= ~ YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_MASK;
    CurrentShapeGen.ShapeFlags |= ValueNew;

    if( ValueNew == YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_CENTER) {               // New align mode is center

      pInt_IndentVer->ChangeMinMax( -VALUE_INDENT_MAX, VALUE_INDENT_MAX);  // Allow negative values

    } else {                                                               // Align mode left or right

      pInt_IndentVer->ChangeMinMax( 0, VALUE_INDENT_MAX);                  // Only positive values allowed
    }

    if( memcmp( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t)) != 0) { // is different

      pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

      // Copy back modified data
      memcpy( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));
    }

    pToolData->Input1_Change = 0;                    // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_Misc_SetValue_Callback
 *
 * This is usable for Fl_Valuator, Fl_Choice, Fl_Check_Button
 */

static void IqeB_GUI_Misc_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  if( w == NULL ||                       // security test
      pValueArg == NULL) {

    return;
  }

  if( pValueArg == &iCurrShape) {

    // Fl_Choice

    Fl_Choice *pThis;
    int *pValue, NewValue;

    pThis  = (Fl_Choice *)w;
    pValue = (int *)pValueArg;               // get pointer to associated variable

    if( BrowserSelected >= 0) {              // Any overlay selected in browser
      const Fl_Menu_Item *m;

      NewValue = pThis->value();             // Get new value

      if( *pValue != NewValue) {             // Value is different

        *pValue = NewValue;                  // Update the variable
        if( *pValue >= 0 && *pValue < nShapeGen_List) { // Security test

          CurrentShapeGen.ShapeType = ShapeGen_List[ NewValue].Type;

          // On change of shape change default argument
          CurrentShapeGen.ShapeArg = ShapeGen_List[ NewValue].ShapeArgDef;
        }

        m = pThis->mvalue();
        if( m != NULL) {

          CurrentShapeGen.ShapeType = (int)m->argument();
        }

      }
    }

  } else if( pValueArg == &CurrentShapeGen.BGnd_WinIdNr) {

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + pToolData->iToolData, &CurrentShapeGen.BGnd_WinIdNr, pBGnd_Win_But, pBGnd_Win_Box);

  } else if( w == pSlider_ShapeArg) {            // Shape deformation

    // Fl_Valuator, Fl_Slider or Fl_Value_Slider

    Fl_Valuator *pThis;
    float *pValue;

    pValue = (float *)pValueArg;             // get pointer to associated variable

    pThis  = (Fl_Valuator *)w;
    *pValue = pThis->value();               // update the variable

  } else if( w == pTeachToggle) {                   // Toggle Teach / Inspection button

    pToolData->TeachMode = ! pToolData->TeachMode;

    IqeB_GUI_ToolsAnimManagerUpdateSelected( BrowserSelected);  // Update enables

  } else if( w == pAOI_SizeRatio) {                                      // Lock AOI size ratio

    if( BrowserSelected >= 0) {          // Any overlay selected in browser

      CurrentShapeGen.ShapeFlags ^= YAIPS_SHAPE_GEN_FLAG_AOI_SIZE_RATIO;   // Flip bit

      pAOI_SizeRatio->label( (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_AOI_SIZE_RATIO) ? "@+1PadlockC" : "@+1PadlockO");
      pAOI_SizeRatio->redraw();

      if( (CurrentShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_AOI_SIZE_RATIO) != 0) {   // Switch lock on

        // Latch size if just switched to on
        CurrentShapeGen.AOI_XX_Locked = YaIPS_Calib_UnitXVal2Pixel( pToolData->Overlay[ BrowserSelected].SizeX);
        CurrentShapeGen.AOI_YY_Locked = YaIPS_Calib_UnitYVal2Pixel( pToolData->Overlay[ BrowserSelected].SizeY);
      }
    }

  } else if( w == pSliderAlphaMult) {       // Alpha multiplier

    // Fl_Valuator, Fl_Slider or Fl_Value_Slider

    Fl_Valuator *pThis;
    float *pValue;

    pValue = (float *)pValueArg;             // get pointer to associated variable

    pThis  = (Fl_Valuator *)w;
    *pValue = pThis->value();               // update the variable

  } else {

    Fl_Button *pThis;
    int *pValue;

    pValue = (int *)pValueArg;             // get pointer to associated variable

    pThis  = (Fl_Check_Button *)w;
    *pValue = pThis->value();                // update the variable
  }

  if( BrowserSelected >= 0) {          // Any overlay selected in browser

    if( memcmp( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t)) != 0) { // is different

      // Copy back modified data
      memcpy( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));

      if( pValueArg != &CurrentShapeGen.AlphaMult) {            // If not alpha multiplier

        pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

        // update all enables
        if( pValueArg == &iCurrShape) {

          IqeB_GUI_ToolsAnimManagerUpdate( true);   // Update overlay browser and enables

        } else if( w == pSlider_ShapeArg) {          // Shape deformation

          // Nothing to update here

        } else {

          IqeB_GUI_ToolsAnimManagerUpdateSelected( BrowserSelected);  // Update enables
        }
      }
    }
  }

  pToolData->Input1_Change = 0;          // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_CBox_SetValue_Callback
 */

static void IqeB_GUI_Check_Bit_Callback( Fl_Widget *w, void *pValueArg)
{
  int *pValue, HasChanged;
  IqeFl_Check_Bit *pThis;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  pThis  = (IqeFl_Check_Bit *)w;
  pValue = (int *)pValueArg;             // get pointer to associated variable

  if( pThis == NULL ||                   // security test
      pValue == NULL) {

    return;
  }

  // Set value

  HasChanged = pThis->GetValue();      // Update the variable

  if( BrowserSelected >= 0 &&          // Any overlay selected in browser
      HasChanged) {                    // and value has changed

    if( memcmp( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t)) != 0) { // is different

      pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

      // Copy back modified data
      memcpy( &pToolData->Overlay[ BrowserSelected].ShapeGen, &CurrentShapeGen, sizeof( YaIPS_RGB_ShapeGen_Par_t));
    }

    // update all enables
    IqeB_GUI_ToolsAnimManagerUpdateSelected( BrowserSelected);  // Update enables
  }

  pToolData->Input1_Change = 0;          // Force recalculation output
}

/************************************************************************************
 * text_changed_callback
 *
 * Text to draw has change
 */


static void text_changed_callback( int, int n_inserted, int n_deleted, int, const char*, void*)
{
  YaIPS_OverlayData_t *pOverlay;
  char TempString[ 512];
  const char *pText;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  if( pToolData == NULL) {        // This can be NULL during startup on loaded of a textfile

    return;
  }

  if( n_inserted || n_deleted) {   // Something changed

    // Rebuild image
    if( BrowserSelected >= 0) {         // Any overlay selected in browser

      pOverlay = pToolData->Overlay + BrowserSelected;

      pOverlay->ParChanged = 1;        // Recreate intermediate image

      pToolData->Input1_Change = 0;     // Force recalculation output

      // Check for change of proser line text

      BrowserLineText( TempString, BrowserSelected);        // Generate a line for the overlay

      pText = pOverlayBrowser->text( BrowserSelected + 1);  // Get text for current line

      if( pText != NULL &&                                  // Have a text
          strcmp( pText, TempString) != 0) {                // Text has changed

        pOverlayBrowser->text( BrowserSelected + 1, TempString);    // Replace text for current line
      }
    }
  }
}

/************************************************************************************
 * Callback, button 'New' pressed
 */

static void ButtonNewCallback( Fl_Widget *w, void *data)
{
  int i, iOverlay, iSource, DoInsert, DoClone, State;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  if( pToolData->nOverlays >= OVERLAY_NUM_MAX) {    // Have NO space for one more

    return;
  }

  if( pOverlayBrowser != NULL) {              // Called from button

    iOverlay = pOverlayBrowser->value() - 1;  // get index of selected item

    DoInsert = w == pGUI_ButInsert;           // Insert or append

  } else {                                    // Called from mouse handling

    iOverlay = (int)(long long)data;

    // Hack to get insert/append
    if( iOverlay < OVERLAY_NUM_MAX + OVERLAY_NUM_MAX - 1) {

      DoInsert = true;
    } else {

      iOverlay -= OVERLAY_NUM_MAX + OVERLAY_NUM_MAX;
      DoInsert = false;
    }
  }

  State = Fl::event_state();
  DoClone = (State & FL_CTRL) != 0;                 // Control key pressed

  iSource  = -1;                                    // No clone source

  if( DoInsert) {                                   // Do insert

    if( iOverlay < 0) {                             // None was selected

      iOverlay = 0;                                 // Insert at begin of list

    } else {

      iSource  = iOverlay + 1;                      // Remember clone source
    }

    // Shift up

    for( i = pToolData->nOverlays; i > iOverlay; i--) {

      memcpy( pToolData->Overlay + i, pToolData->Overlay + (i - 1), sizeof( YaIPS_OverlayData_t));
    }

  } else {

    if( iOverlay < 0) {                             // None was selected

      iOverlay = pToolData->nOverlays;              // Append at end

    } else {                                        // Add before selected

      // Shift up

      iOverlay += 1;
      iSource  = iOverlay - 1;                      // Remember clone source

      for( i = pToolData->nOverlays; i > iOverlay; i--) {

        memcpy( pToolData->Overlay + i, pToolData->Overlay + (i - 1), sizeof( YaIPS_OverlayData_t));
      }
    }
  }

  memset( pToolData->Overlay + iOverlay, 0, sizeof( YaIPS_OverlayData_t));   // Zero new one

  if( DoClone && iSource >= 0) {   // Clone

    Fl_Text_Buffer *pTextBufferOld;

    memcpy( pToolData->Overlay + iOverlay, pToolData->Overlay + iSource, sizeof( YaIPS_OverlayData_t));

    // Reset intermedia data

    pToolData->Overlay[ iOverlay].pImgOverlay = NULL;    // Will be recreated if used
    pToolData->Overlay[ iOverlay].pImgShadow  = NULL;    // Will be recreated if used
    pToolData->Overlay[ iOverlay].pBGndFile   = NULL;    // Will be recreated if used

    pTextBufferOld = pToolData->Overlay[ iOverlay].pTextBuffer;  // Textbuffer from before

    pToolData->Overlay[ iOverlay].pTextBuffer = new Fl_Text_Buffer(); // Create new text buffer
    pToolData->Overlay[ iOverlay].pTextBuffer->add_modify_callback( text_changed_callback, NULL);

    if( pTextBufferOld != NULL && pTextBufferOld->length() > 0) {     // Textbuffer with text from before

      pToolData->Overlay[ iOverlay].pTextBuffer->copy( pTextBufferOld, 0, pTextBufferOld->length(), 0);
    }

  } else {         // New

    // Set in some reasonable defaults.

    strcpy( pToolData->Overlay[ iOverlay].Name, "");

    switch( YaIPS_Calib_Unit) {
    default:                         // Units is not know
    case YAIPS_CALIB_UNIT_PIXEL:     // Units are pixel
      pToolData->Overlay[ iOverlay].PosX  = pToolData->nOverlays * 10;
      pToolData->Overlay[ iOverlay].PosY  = pToolData->nOverlays * 10;
      pToolData->Overlay[ iOverlay].SizeX = 50;
      pToolData->Overlay[ iOverlay].SizeY = 50;
      break;

    case YAIPS_CALIB_UNIT_MM:        // Units are mm
      pToolData->Overlay[ iOverlay].PosX  = pToolData->nOverlays * 5;
      pToolData->Overlay[ iOverlay].PosY  = pToolData->nOverlays * 5;
      pToolData->Overlay[ iOverlay].SizeX = 20;
      pToolData->Overlay[ iOverlay].SizeY = 20;
      break;

    case YAIPS_CALIB_UNIT_CM:        // Units are cm
    case YAIPS_CALIB_UNIT_INCH:      // Units are inch
      pToolData->Overlay[ iOverlay].PosX  = pToolData->nOverlays * 2;
      pToolData->Overlay[ iOverlay].PosY  = pToolData->nOverlays * 2;
      pToolData->Overlay[ iOverlay].SizeX = 5;
      pToolData->Overlay[ iOverlay].SizeY = 5;
      break;
    }

    pToolData->Overlay[ iOverlay].AoiUnit = YaIPS_Calib_Unit;                       // Latch unit used on creation of AOI

    IqeB_GUI_Param_RecalcAOI( pToolData->Overlay + iOverlay);   // Ensure proper AOI values

    pToolData->Overlay[ iOverlay].ShapeGen.AlphaMult = 100.0;

    pToolData->Overlay[ iOverlay].ShapeGen.ShapeFlags = YAIPS_SHAPE_GEN_FLAG_CBIT_LT | YAIPS_SHAPE_GEN_FLAG_DRAW_SHAPE |
                                                        YAIPS_SHAPE_GEN_FLAG_LINE_COL_USE | YAIPS_SHAPE_GEN_FLAG_FONT_COL_USE |
                                                        YAIPS_SHAPE_GEN_FLAG_SHADOW_COL_USE;
    pToolData->Overlay[ iOverlay].ShapeGen.BGndCol_LT = 2;
    pToolData->Overlay[ iOverlay].ShapeGen.BGndCol_RT = 3;
    pToolData->Overlay[ iOverlay].ShapeGen.BGndCol_LB = 1;
    pToolData->Overlay[ iOverlay].ShapeGen.BGndCol_RB = 6;
    pToolData->Overlay[ iOverlay].ShapeGen.BGnd_WinIdNr = -1;

    pToolData->Overlay[ iOverlay].ShapeGen.LineColor = 1;
    pToolData->Overlay[ iOverlay].ShapeGen.LineWidth = 1;

    pToolData->Overlay[ iOverlay].ShapeGen.FontSize = 16;
    pToolData->Overlay[ iOverlay].ShapeGen.FontColor = 0;

    pToolData->Overlay[ iOverlay].ShapeGen.ShadowAngle = 315;
    pToolData->Overlay[ iOverlay].ShapeGen.ShadowDist = 8;
    pToolData->Overlay[ iOverlay].ShapeGen.ShadowColor = 0;

    pToolData->Overlay[ iOverlay].pTextBuffer = new Fl_Text_Buffer(); // Create new text buffer
    pToolData->Overlay[ iOverlay].pTextBuffer->add_modify_callback( text_changed_callback, NULL);

  }

  pToolData->nOverlays += 1;                    // Have one more

  // and update ...

  IqeB_GUI_ToolsAnimManagerUpdate( false);     // Update browser list

  if( pOverlayBrowser != NULL) {              // Called from button
    pOverlayBrowser->select( iOverlay + 1);      // Select new one
  }

  IqeB_GUI_ToolsAnimManagerUpdate( true);      // update Browser

  pToolData->Input1_Change = 0;                // Force recalculation output
}

/************************************************************************************
 * ReleaseOverlayData
 *
 * Release intermedia data of a specific ovlera
 */

static void ReleaseOverlayData( int iOverlay)
{
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  if( pToolData->Overlay[ iOverlay].pImgOverlay != NULL) {       // Have an intermediate image

    (pToolData->Overlay[ iOverlay].pImgOverlay)->release();      // Release image data

    pToolData->Overlay[ iOverlay].pImgOverlay = NULL;
  }

  if( pToolData->Overlay[ iOverlay].pImgShadow != NULL) {        // Have an intermediate image

    (pToolData->Overlay[ iOverlay].pImgShadow)->release();       // Release image data

    pToolData->Overlay[ iOverlay].pImgShadow = NULL;
  }

  if( pToolData->Overlay[ iOverlay].pBGndFile != NULL) {       // Have an intermediate image

    (pToolData->Overlay[ iOverlay].pBGndFile)->release();      // Release image data

    pToolData->Overlay[ iOverlay].pBGndFile = NULL;
  }

  if( pToolData->Overlay[ iOverlay].pTextBuffer != NULL) {     // Have a text buffer allocated

    if( app_editor != NULL) {

      if( app_editor->buffer() == pToolData->Overlay[ iOverlay].pTextBuffer) {  // Buffer is set to the editor display

        app_editor->buffer( pEmptyTextBuffer);                       // Detach text buffer
      }
    }

    delete (pToolData->Overlay[ iOverlay].pTextBuffer);              // Release text buffer

    pToolData->Overlay[ iOverlay].pTextBuffer = NULL;
  }
}

/************************************************************************************
 * Callback, button 'Delete' pressed
 */

static void ButtonDeleteCallback(Fl_Widget *w, void *data)
{
  int i, iOverlay;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  if( pToolData->nOverlays <= 0) {                 // Browser is empty

    return;
  }

  // ...

  if( pOverlayBrowser != NULL) {              // Called from button

    iOverlay = pOverlayBrowser->value() - 1;  // get index of selected item

  } else {                                    // Called from mouse handling

    iOverlay = (int)(long long)data;          // get index of selected item
  }

  if( iOverlay <  0) {                      // None selected

    return;
  }

  // Release intermediate images

  ReleaseOverlayData( iOverlay);

  // Shift down

  for( i = iOverlay; i < pToolData->nOverlays + 1; i++) {

    memcpy( pToolData->Overlay + i, pToolData->Overlay + (i + 1), sizeof( YaIPS_OverlayData_t));
  }

  pToolData->nOverlays -= 1;                    // Have one less

  memset( pToolData->Overlay + pToolData->nOverlays, 0, sizeof( YaIPS_OverlayData_t));   // Zero old top one

  if( pOverlayBrowser != NULL &&              // Called from button
      pToolData->nOverlays > 0 &&             // Any in browser
      iOverlay >= pToolData->nOverlays) {     // last one was deletet

    pOverlayBrowser->select( iOverlay);       // Select last one in list
  }

  // and update ...

  IqeB_GUI_ToolsAnimManagerUpdate( true);            // update Browser

  pToolData->Input1_Change = 0;          // Force recalculation output
}

/************************************************************************************
 * Callback, button 'Delete all' pressed
 */

static void ButtonDelAllCallback(Fl_Widget *w, void *data)
{
  int iOverlay;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  if( pToolData->nOverlays <= 0) {  // Browser is empty

    return;
  }

  //

  fl_message_title( LangStringLookup( "&GUI_Overlay_DelAll_1=Delete all overlays"));
  if( fl_choice( LangStringLookup( "&GUI_Overlay_DelAll_2=Delete all overlays?"),
                 LANGDEF_BUTTON_CANCEL, LANGDEF_BUTTON_YES, NULL) == 0) {
    return;
  }

  // Release intermediate images

  for( iOverlay = 0; iOverlay < pToolData->nOverlays && iOverlay < OVERLAY_NUM_MAX; iOverlay++) {

    ReleaseOverlayData( iOverlay);
  }

  // ...

  pToolData->nOverlays = 0;                      // Reset number of overlays

  memset( pToolData->Overlay, 0, sizeof( pToolData->Overlay));

  // and update ...

  IqeB_GUI_ToolsAnimManagerUpdate( true);            // update Browser

  pToolData->Input1_Change = 0;          // Force recalculation output
}

/************************************************************************************
 * Callback, button 'Shift up' pressed
 */

static void ButtonShiftUpCallback(Fl_Widget *w, void *data)
{
  int iOverlay, State;
  YaIPS_OverlayData_t TempOverlay;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  if( pToolData->nOverlays <= 1) {  // Browser is empty or only one entry

    return;
  }

  // ...

  iOverlay = pOverlayBrowser->value() - 1;  // get index of selected item

  if( iOverlay < 1) {                       // None or first

    return;
  }

  State = Fl::event_state();

  if( (State & FL_CTRL) == 0) {    // NO control key pressed

    // Shift named animation down by swapping with previous one

    memcpy( &TempOverlay, pToolData->Overlay + iOverlay, sizeof( YaIPS_OverlayData_t));
    memcpy( pToolData->Overlay + iOverlay, pToolData->Overlay + (iOverlay - 1), sizeof( YaIPS_OverlayData_t));
    memcpy( pToolData->Overlay + (iOverlay - 1), &TempOverlay, sizeof( YaIPS_OverlayData_t));

    pOverlayBrowser->select( iOverlay);  // Select previous

  } else {                         // Control key pressed

    // Shift to lowest position

    int i;

    // Save selected
    memcpy( &TempOverlay, pToolData->Overlay + iOverlay, sizeof( YaIPS_OverlayData_t));

    // Shift up other

    for( i = iOverlay; i >= 1; i--) {

      memcpy( pToolData->Overlay + i, pToolData->Overlay + (i - 1), sizeof( YaIPS_OverlayData_t));
    }

    // Restore selected to lowest position
    memcpy( pToolData->Overlay + 0, &TempOverlay, sizeof( YaIPS_OverlayData_t));

    pOverlayBrowser->select( 1);             // top
  }

  // and update ...

  IqeB_GUI_ToolsAnimManagerUpdate( true);    // update Browser

  pToolData->Input1_Change = 0;              // Force recalculation output
}

/************************************************************************************
 * Callback, button 'Shift down' pressed
 */

static void ButtonShiftDownCallback(Fl_Widget *w, void *data)
{
  int iOverlay, State;
  YaIPS_OverlayData_t TempOverlay;
  YaIPS_ToolData_info_t *pToolData = pToolDataParam;

  if( pToolData->nOverlays <= 1) {  // Browser is empty or only one entry

    return;
  }

  // ...

  iOverlay = pOverlayBrowser->value() - 1;  // get index of selected item

  if( iOverlay < 0 ||                       // None or last selected
      iOverlay >= pToolData->nOverlays) {

    return;
  }

  State = Fl::event_state();

  if( (State & FL_CTRL) == 0) {    // NO control key pressed

    // Shift named animation up by swapping with previous one

    memcpy( &TempOverlay, pToolData->Overlay + iOverlay, sizeof( YaIPS_OverlayData_t));
    memcpy( pToolData->Overlay + iOverlay, pToolData->Overlay + (iOverlay + 1), sizeof( YaIPS_OverlayData_t));
    memcpy( pToolData->Overlay + (iOverlay + 1), &TempOverlay, sizeof( YaIPS_OverlayData_t));

    pOverlayBrowser->select( iOverlay + 2);  // Select next

  } else {                         // Control key pressed

    // Shift to top position

    int i;

    // Save selected
    memcpy( &TempOverlay, pToolData->Overlay + iOverlay, sizeof( YaIPS_OverlayData_t));

    // Shift down other

    for( i = iOverlay; i < pToolData->nOverlays; i++) {

      memcpy( pToolData->Overlay + i, pToolData->Overlay + (i + 1), sizeof( YaIPS_OverlayData_t));
    }

    // Restore selected to top position
    memcpy( pToolData->Overlay + (pToolData->nOverlays - 1), &TempOverlay, sizeof( YaIPS_OverlayData_t));

    pOverlayBrowser->select( pToolData->nOverlays);             // top
  }

  // and update ...

  IqeB_GUI_ToolsAnimManagerUpdate( true);            // update Browser

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
  YaIPS_ToolData_info_t *pToolData;

  // Get pointer to tool data

  pToolData = YaIPS_ToolData_info + iToolData;             // Point to info data

  pToolDataParam = pToolData;   // Is used by buttons or other callback functions

  //
  // creation of window on first call
  //

  xPos = xLeft;
  yPos = yTop;

  if( pToolData->MyParPosX != IQE_GUI_NO_WINPOS_X && pToolData->MyParPosY != IQE_GUI_NO_WINPOS_Y) { // have last window position

    xPos = pToolData->MyParPosX;
    yPos = pToolData->MyParPosY;
  }

  pMyParWin = new Fl_Window( xPos, yPos, 424, 432, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  //
  //  GUI things
  //

  int x1, x, y, yy, xx2, xxColumn, yy2;
  int xGroup, yGroup;
  //x/char TempBuffer[ 256];

  //x/Fl_Check_Button *pCheckTemp;
  Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  IqeFl_Float_Input  *pFloatTemp;
  IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group;
  Fl_Button       *pTemp_Button;
  Fl_Choice       *pTemp_Choice;
  Fl_Radio_Round_Button *pRadioButTemp;
  IqeFl_Check_Bit *pTemp_Check_Bit;
  Fl_Value_Slider *pTemp_ValSlider;

  x1  = 4;
  //x/xx1 = pMyParWin->w() - 16;
  //x/xx2 = xx1 / 2;
  //x/xc  = pMyToolWin->w() / 2;          // x center
  yy  = 20;

  y = 4;

  //
  // Parameter dialog
  //

  // Overlay browser

  xxColumn = 200;

  //
  // Left side of parameters
  //


  yy2 = 334;

  y += 12;     // Space for label at top

  pOverlayBrowser = new Fl_Browser( x1, y, xxColumn, yy2, LangStringLookup( "&GUI_Overlay_1=Overlays"));
  pOverlayBrowser->align( FL_ALIGN_TOP_LEFT);     // align for label
  pOverlayBrowser->labelsize( 10);
  pOverlayBrowser->type(FL_HOLD_BROWSER);        // use for single selection
  pOverlayBrowser->callback( IqeB_OverlayBrowser_Callback, NULL);
  pOverlayBrowser->tooltip( LangStringLookup( "&GUI_Overlay_1a=Select overlay"));

  y += yy2;

  // Buttons

  y += 4;
  yy = 22;

  x = x1;
  xx2 = (xxColumn - 4) / 2;

  pGUI_ButInsert = new Fl_Button( x, y, xx2, yy, LangStringLookup( "&GUI_Overlay_20=&Insert"));
  pGUI_ButInsert->callback( ButtonNewCallback, NULL);
  pGUI_ButInsert->tooltip( LangStringLookup( "&GUI_Overlay_20a="
                           "Create a new overlay.\n"
                           "If an overlay is selected, the new one is\n"
                           "inserted before this position.\n"
                           "Otherwise it is inserted at top of the list.\n"
                           "Press key Ctrl to clone the selected."));

  x += xx2 + 4;


  pGUI_ButAppend = new Fl_Button( x, y, xx2, yy, LangStringLookup( "&GUI_Overlay_21=&Append"));
  pGUI_ButAppend->callback( ButtonNewCallback, NULL);
  pGUI_ButAppend->tooltip( LangStringLookup( "&GUI_Overlay_21a="
                           "Create a new overlay.\n"
                           "If an overlay is selected, the new one is\n"
                           "inserted after this position.\n"
                           "Otherwise it is inserted after the end\n"
                           "of the list.\n"
                           "Press key Ctrl to clone the selected."));
  y += yy + 4;
  x = x1;

  pGUI_ButDelete = new Fl_Button( x, y, xx2, yy, LangStringLookup( "&GUI_Overlay_22=&Delete"));
  pGUI_ButDelete->callback( ButtonDeleteCallback, NULL);
  pGUI_ButDelete->tooltip( LangStringLookup( "&GUI_Overlay_22a=Delete selected overlay"));

  x += xx2 + 4;
  pGUI_ButDelAll = new Fl_Button( x, y, xx2, yy, LangStringLookup( "&GUI_Overlay_23=Del. &all"));
  pGUI_ButDelAll->callback( ButtonDelAllCallback, NULL);
  pGUI_ButDelAll->tooltip( LangStringLookup( "&GUI_Overlay_23a=Delete all overlays"));

  y += yy + 4;
  x = x1;

  pGUI_ButShiftUp = new Fl_Button( x, y, xx2, yy, "@#+32<");
  pGUI_ButShiftUp->callback( ButtonShiftUpCallback, NULL);
  pGUI_ButShiftUp->tooltip( LangStringLookup( "&GUI_Overlay_24a="
                            "Shift up selected overlay.\n"
                            "Press key Ctrl key to move\n"
                            "to the lowest position."));

  x += xx2 + 4;

  pGUI_ButShiftDown = new Fl_Button( x, y, xx2, yy, "@#+32>");
  pGUI_ButShiftDown->callback( ButtonShiftDownCallback, NULL);
  pGUI_ButShiftDown->tooltip( LangStringLookup( "&GUI_Overlay_25a="
                              "Shift down selected overlay.\n"
                              "Press key Ctrl key to move\n"
                              "to the top position."));

  //
  // Right side of parameters
  //

  y = 4;

  x1  = xxColumn + 8;

  // Name

  y += 12;

  xx2 = xxColumn - 19;

  pInputName = new Fl_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_2=Name"));
  pInputName->align( FL_ALIGN_TOP_LEFT);     // align for label
  pInputName->labelsize( 10);
  pInputName->tooltip( LangStringLookup( "&GUI_Overlay_2a=Name for this overlay"));
  pInputName->callback( IqeB_GUI_Input_Name_SetValue_Callback);

  x1 += xx2;
  x1 += 4;

  pTemp_Button = new Fl_Button( x1, y - 6, 28, 28, "@+1pencil");
  pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->TeachMode);
  pTemp_Button->tooltip( LANGDEF_AOI_TEACH_TOOLTIP);
  pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
  pTemp_Button->shortcut( FL_COMMAND+'t');       // Short cut key
  pTeachToggle = pTemp_Button;

  // Next line

  x1  = xxColumn + 8;
  y += yy + 16;

  xx2 = 52;

  pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_AOI_LEFT);
  pFloatTemp->tooltip( LANGDEF_AOI_LEFT_TOOLTIP);
  pFloatTemp->align( FL_ALIGN_TOP_LEFT);     // align for label
  pFloatTemp->labelsize( 10);
  pFloatTemp->SetValue( 0);
  pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &CurrentPosX);
  pFloatTemp->SetModifyData( 0, 4096, 10, 1);
  pFloatTemp->SetModifyData( 1, 1234.0, 10.0, 1.0);  // Is modified later form MyParWinUpdateNewSizes()
  pFloat_AOI_X = pFloatTemp;

  x1 += xx2;
  x1 += 1;

  pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_AOI_TOP);
  pFloatTemp->tooltip( LANGDEF_AOI_TOP_TOOLTIP);
  pFloatTemp->align( FL_ALIGN_TOP_LEFT);     // align for label
  pFloatTemp->labelsize( 10);
  pFloatTemp->SetValue( 0);
  pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &CurrentPosY);
  pFloatTemp->SetModifyData( 0, 4096, 10, 1);
  pFloatTemp->SetModifyData( 1, 1234.0, 10.0, 1.0);  // Is modified later form MyParWinUpdateNewSizes()
  pFloat_AOI_Y = pFloatTemp;

  x1 += xx2;
  x1 += 3;

  pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_AOI_WIDTH);
  pFloatTemp->tooltip( LANGDEF_AOI_WIDTH_TOOLTIP);
  pFloatTemp->align( FL_ALIGN_TOP_LEFT);     // align for label
  pFloatTemp->labelsize( 10);
  pFloatTemp->SetValue( 0);
  pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &CurrentSizeX);
  pFloatTemp->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 4096, 10, 1);
  pFloatTemp->SetModifyData( 1, 1234.0, 10.0, 1.0);  // Is modified later form MyParWinUpdateNewSizes()
  pFloat_AOI_XX = pFloatTemp;

  x1 += xx2;
  x1 += 1;

  pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_AOI_HEIGHT);
  pFloatTemp->tooltip( LANGDEF_AOI_HEIGHT_TOOLTIP);
  pFloatTemp->align( FL_ALIGN_TOP_LEFT);     // align for label
  pFloatTemp->labelsize( 10);
  pFloatTemp->SetValue( 0);
  pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &CurrentSizeY);
  pFloatTemp->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 4096, 10, 1);
  pFloatTemp->SetModifyData( 1, 1234.0, 10.0, 1.0);  // Is modified later form MyParWinUpdateNewSizes()
  pFloat_AOI_YY = pFloatTemp;

  // Set modify data for positin, with, and height depenting from global calibration unit

  YaIPS_Calib_SetModifyDataAndValue( pFloat_AOI_X, pFloat_AOI_Y,    // GUI input elements for X and y value
                                     CurrentPosX, CurrentPosY,      // Set this X and Y values to the X GUI elements
                                     true);                         // Have postion values

  YaIPS_Calib_SetModifyDataAndValue( pFloat_AOI_XX, pFloat_AOI_YY,  // GUI input elements for X and y value
                                     CurrentSizeX, CurrentSizeY,    // Set this X and Y values to the X GUI elements
                                     false);                        // Have size values

  // Next line

  x1  = xxColumn + 8;
  y += yy + 16;

  xx2 = 52;

  // Unit of position and size
  pTemp_Box = new Fl_Box( x1, y, xx2, 0, LangStringLookup( "&GUI_Overlay_3=Unit"));
  pTemp_Box->align( FL_ALIGN_TOP_LEFT);     // align for label
  pTemp_Box->labelsize( 10);
  pTemp_Box->box( FL_NO_BOX);


  pTemp_Box = new Fl_Box( x1, y, xx2, yy);
  pTemp_Box->copy_label( pYaIPS_Calib_Unit2String());

  pTemp_Box->tooltip( LangStringLookup( "&GUI_Overlay_3a="
                                        "Unit of position and size"));
  pTemp_Box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label
  pTemp_Box->box( FL_DOWN_BOX);

  x1 += xx2;
  x1 += 1;

  pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_4=Rotation"));
  pFloatTemp->type( FL_FLOAT_INPUT);
  pFloatTemp->tooltip( LangStringLookup( "&GUI_Overlay_4a="
                                         "Rotation angle of the overlay [Degree].\n"
                                         "NOTE: When the mouse wheel is pressed while\n"
                                         "holding down the shift key, a selected\n"
                                         "overlay is rotated in 15-degree increments.\n"
                                         "Holding down the control button rotates in\n"
                                         "1-degree increments."));
  pFloatTemp->align( FL_ALIGN_TOP_LEFT);     // align for label
  pFloatTemp->labelsize( 10);
  pFloatTemp->SetFormat( "%.2f");
  pFloatTemp->SetValue( 0);
  pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &CurrentShapeGen.RotAngle);
  pFloatTemp->SetModifyData( 0.0, 360.0, 15.0, 1.0, true);
  pRotAngle = pFloatTemp;

  x1 += xx2 + 3;

  pShowSizeRatio = new Fl_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_5=Ratio"));
  pShowSizeRatio->tooltip( LangStringLookup( "&GUI_Overlay_5a="
                                             "Shows the ratio of the width\n"
                                             "to the height of the image.\n"
                                             "NOTE: is read only."));
  pShowSizeRatio->align( FL_ALIGN_TOP_LEFT);     // align for label
  pShowSizeRatio->labelsize( 10);
  pShowSizeRatio->readonly( 1);    // Set read only. String is better visible then deactivate()
  pShowSizeRatio->color( YAIPS_COLOR_RONLY_BGND);

  x1 += xx2 + 3;

  xx2 = 31;

  pMButAlign = new Fl_Menu_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_6=Align"));
  pMButAlign->tooltip( LangStringLookup( "&GUI_Overlay_6a="
                                         "Align overlay to the\n"
                                         "edges of the background."));
  pMButAlign->align( FL_ALIGN_TOP_LEFT /*FL_ALIGN_LEFT*/);     // align for label
  pMButAlign->labelsize( 10);
  pMButAlign->callback( IqeB_AOI_Align_Callback);

  pMButAlign->add( LangStringLookup( "&GUI_Overlay_6_1=Left"),     0, NULL, (void *)(fl_intptr_t)( 1));
  pMButAlign->add( LangStringLookup( "&GUI_Overlay_6_2=Centered"), 0, NULL, (void *)(fl_intptr_t)( 2));
  pMButAlign->add( LangStringLookup( "&GUI_Overlay_6_3=Right"),    0, NULL, (void *)(fl_intptr_t)( 3), FL_MENU_DIVIDER);
  pMButAlign->add( LangStringLookup( "&GUI_Overlay_6_4=Top"),      0, NULL, (void *)(fl_intptr_t)( 4));
  pMButAlign->add( LangStringLookup( "&GUI_Overlay_6_5=Middle"),   0, NULL, (void *)(fl_intptr_t)( 5));
  pMButAlign->add( LangStringLookup( "&GUI_Overlay_6_6=Bottom"),   0, NULL, (void *)(fl_intptr_t)( 6));

  x1 += xx2 + 3;

  pTemp_Button = new Fl_Button( x1, y, yy - 6, yy, "@+1PadlockO");
  pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_7a="
                                           "Lock size ratio."));
  pTemp_Button->labelcolor( FL_BLUE);
  pTemp_Button->box( FL_NO_BOX);
  pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &CurrentShapeGen.ShapeFlags);
  pAOI_SizeRatio = pTemp_Button;

  x1 += yy - 6;
  x1 += 3;

  // Next line

  x1  = xxColumn + 8;
  y += yy + 16;

  xx2 = xxColumn + 13;

  pTemp_ValSlider = new Fl_Value_Slider( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_8=Alpha multiplier"));
  pTemp_ValSlider->align( FL_ALIGN_TOP_LEFT);     // align for label
  pTemp_ValSlider->labelsize( 10);
  pTemp_ValSlider->tooltip( LangStringLookup( "&GUI_Overlay_8a="
                                           " Left side = invisible"
                                           "Right side = no alpha modification"));
  pTemp_ValSlider->type( FL_HOR_SLIDER);
  pTemp_ValSlider->color( FL_LIGHT2 + 1);  // Background color
  pTemp_ValSlider->bounds( 0.0, 100.0);
  pTemp_ValSlider->step( 0.1);
  pTemp_ValSlider->value( CurrentShapeGen.AlphaMult);
  pTemp_ValSlider->callback( IqeB_GUI_Misc_SetValue_Callback, &CurrentShapeGen.AlphaMult);
  pSliderAlphaMult = pTemp_ValSlider;

  // Tabs begin -------------------------------------------

  x1  = xxColumn + 8;
  y += yy + 6;

  pTemp_Tabs = new IqeFl_Tabs( x1, y, xxColumn + 12, pMyParWin->h() - y - 4);
  pTemp_Tabs->selection_color( YAIPS_COLOR_SELECTION);
  pTab_Groups = pTemp_Tabs;

  y += 26;

  //
  // Group 'Base'
  //

  yGroup = y;
  xGroup  = x1 + 4;

  //
  // Group 'Background'
  //

  yGroup = y;
  x1  = xGroup;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Overlay_TabA1=Backgr."));

    y += 8;

    x1 = xGroup + 0;
    xx2 = xxColumn;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2, yy, LANGDEF_COLOR);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Overlay_TabA2_0a="
                            "Overlay background is color.\n"
                            "A separate color can be specified\n"
                            "for each corner of the overlay."));
    pRadioButTemp->callback( YaIPS_BGndType_Callback, (void *)YAIPS_SHAPE_GEN_BGND_COLOR);
    pBGndType_0 = pRadioButTemp;

    // Next line

    x1 = xGroup + 4;
    y += yy + 2;

    pTemp_Button = new Fl_Button( x1, y, yy * 2, yy, "");
    pTemp_Button->color( CurrentShapeGen.BGndCol_LT);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &CurrentShapeGen.BGndCol_LT);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabA3a="
                           "Color for image background of left upper corner"));
    pButCol_LT = pTemp_Button;

    x1 += yy * 2;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, 30, yy,
                                           &CurrentShapeGen.ShapeFlags, YAIPS_SHAPE_GEN_FLAG_CBIT_LT,
                                           LANGDEF_ACTIVE_SHORT);

    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Overlay_TabA4a="
                         "Check this to use the color for this corner corner."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &CurrentShapeGen.ShapeFlags);

    pCheck_Bit_LT = pTemp_Check_Bit;

    x1 += 30 + 4;

    pTemp_Button = new Fl_Button( x1, y, yy * 2, yy, "");
    pTemp_Button->color( CurrentShapeGen.BGndCol_RT);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &CurrentShapeGen.BGndCol_RT);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabA5a="
                           "Color for image background of right upper corner"));
    pButCol_RT = pTemp_Button;

    x1 += yy * 2;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, 30, yy,
                                           &CurrentShapeGen.ShapeFlags, YAIPS_SHAPE_GEN_FLAG_CBIT_RT,
                                           LANGDEF_ACTIVE_SHORT);
    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Overlay_TabA4a="
                         "Check this to use the color for this corner corner."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &CurrentShapeGen.ShapeFlags);

    pCheck_Bit_RT = pTemp_Check_Bit;

    // Next line

    x1 = xGroup + 4;
    y += yy + 6;

    pTemp_Button = new Fl_Button( x1, y, yy * 2, yy, "");
    pTemp_Button->color( CurrentShapeGen.BGndCol_LB);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &CurrentShapeGen.BGndCol_LB);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabA6a="
                           "Color for image background of left bottom corner"));
    pButCol_LB = pTemp_Button;

    x1 += yy * 2;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, 30, yy,
                                           &CurrentShapeGen.ShapeFlags, YAIPS_SHAPE_GEN_FLAG_CBIT_LB,
                                           LANGDEF_ACTIVE_SHORT);
    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Overlay_TabA4a="
                         "Check this to use the color for this corner corner."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &CurrentShapeGen.ShapeFlags);

    pCheck_Bit_LB = pTemp_Check_Bit;

    x1 += 30 + 4;

    pTemp_Button = new Fl_Button( x1, y, yy * 2, yy, "");
    pTemp_Button->color( CurrentShapeGen.BGndCol_RB);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &CurrentShapeGen.BGndCol_RB);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabA7a="
                           "Color for image background of right bottom corner"));
    pButCol_RB = pTemp_Button;

    x1 += yy * 2;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, 30, yy,
                                           &CurrentShapeGen.ShapeFlags, YAIPS_SHAPE_GEN_FLAG_CBIT_RB,
                                           LANGDEF_ACTIVE_SHORT);
    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Overlay_TabA4a="
                         "Check this to use the color for this corner corner."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &CurrentShapeGen.ShapeFlags);

    pCheck_Bit_RB = pTemp_Check_Bit;

    // Next line

    x1 = xGroup + 0;
    y += yy + 6;
    xx2 = xxColumn;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabA2_1=Image file"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Overlay_TabA2_1a="
                            "Overlay background is an image file."));
    pRadioButTemp->callback( YaIPS_BGndType_Callback, (void *)YAIPS_SHAPE_GEN_BGND_IMAGE);
    pBGndType_1 = pRadioButTemp;

    // Next line

    x1 = xGroup + 4;
    y += yy + 4;
    xx2 = xxColumn - yy;

    pBGndFileName = new Fl_Input( x1, y, xx2, yy, NULL);
    pBGndFileName->tooltip( LangStringLookup( "&GUI_Overlay_TabA10a="
                                              "Name of loaded image file.\n"
                                              "NOTE: is read only."));
    pBGndFileName->readonly( 1);    // Set read only. String is better visible then deactivate()
    pBGndFileName->color( YAIPS_COLOR_RONLY_BGND);

    x1 += xx2;

    pTemp_Button = new Fl_Button( x1, y, yy, yy, "...");
    pTemp_Button->callback( IqeB_GUI_BGndFileLoad_Callback);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabA11a="
                           "Load an image file."));
    pBGndFileLoad = pTemp_Button;

    // Next line

    x1 = xGroup + 0;
    y += yy + 6;
    xx2 = xxColumn;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabA2_2=Tool window"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Overlay_TabA2_2a="
                            "Overlay background from tool window.\n"
                            "The overlay background is the\n"
                            "output image of a tool window."));
    pRadioButTemp->callback( YaIPS_BGndType_Callback, (void *)YAIPS_SHAPE_GEN_BGND_WINDOW);
    pBGndType_2 = pRadioButTemp;

    // Next line

    x1 = xGroup + 4;
    y += yy + 4;

    xx2 = xxColumn - yy + 1;

    pBGnd_Win_Box = new Fl_Box( x1, y, xx2, yy + 2);
    pBGnd_Win_Box->box( FL_BORDER_BOX);
    pBGnd_Win_Box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pBGnd_Win_Box->labelsize( 18);
    pBGnd_Win_Box->copy_label( "---");

    x1 += xx2 - 1;

    xx2 = yy;

    pBGnd_Win_But = new Fl_Button( x1, y, xx2, yy + 2, "@2>");
    pBGnd_Win_But->callback( IqeB_GUI_Misc_SetValue_Callback, &CurrentShapeGen.BGnd_WinIdNr);
    pBGnd_Win_But->tooltip( LangStringLookup( "&GUI_Overlay_TabA20a=Select window as background image."));
    pBGnd_Win_But->labelcolor( YAIPS_BCOL_BUTTON);
    pBGnd_Win_But->box( FL_BORDER_BOX);

    // Next line

    x1 = xGroup + 4;
    y += yy + 16;
    xx2 = xxColumn - yy;

    pBGndFileInfo = new Fl_Input( x1, y, xx2, yy, NULL);
    pBGndFileInfo->tooltip( LangStringLookup( "&GUI_Overlay_TabA12a="
                                              "Information about the image.\n"
                                              "NOTE: is read only."));
    pBGndFileInfo->readonly( 1);    // Set read only. String is better visible then deactivate()
    pBGndFileInfo->color( YAIPS_COLOR_RONLY_BGND);

    x1 += xx2;

    pTemp_Button = new Fl_Button( x1, y, yy, yy, "@2[]<");
    pTemp_Button->labelcolor( FL_BLUE);
    pTemp_Button->callback( IqeB_GUI_BGndFileCpSize_Callback);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabA13a="
                           "Copy image size to\n"
                           "AOI width and height."));
    pBGndFileCpSize = pTemp_Button;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'Shape'
  //

  y = yGroup;
  x1  = xGroup;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Overlay_TabB1=Shape"));

    y += 8;

    // Shape selector

    y += 12;

    xx2 = xxColumn + 4;

    pTemp_Choice = new Fl_Choice( x1, y, xx2, yy + 6, LangStringLookup( "&GUI_Overlay_TabB2=Shape"));
    pTemp_Choice->tooltip( LangStringLookup( "&GUI_Overlay_TabB2a="
                                             "Shape selection."));
    pTemp_Choice->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Choice->labelsize( 10);
    pTemp_Choice->callback( IqeB_GUI_Misc_SetValue_Callback, &iCurrShape);
    pTemp_Choice->menu_box( FL_BORDER_BOX);
    pChoiceShape = pTemp_Choice;
    iCurrShape = 0;

    // Add shapes
    for( int i = 0; i < nShapeGen_List; i++) {

      pTemp_Choice->add( LangStringLookup( ShapeGen_List[ i].pName), 0, NULL, (void *)(fl_intptr_t)(ShapeGen_List[ i].Type));
    }

    y += 6;               // Extra add for higher height of GUI element above

    // Shape deformation

    x1  = xGroup;
    y += yy + 16;

    xx2 = xxColumn + 4;

    pTemp_ValSlider = new Fl_Value_Slider( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabB3=Shape deformation"));
    pTemp_ValSlider->labelsize( 10);
    pTemp_ValSlider->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_ValSlider->tooltip( LangStringLookup( "&GUI_Overlay_TabB3a=Deformation of shape."));
    pTemp_ValSlider->type( FL_HOR_SLIDER);
    pTemp_ValSlider->color( FL_LIGHT2 + 1);  // Background color
    pTemp_ValSlider->bounds( 0.0, 100.0);
    pTemp_ValSlider->step( 0.1);
    pTemp_ValSlider->value( CurrentShapeGen.ShapeArg);
    pTemp_ValSlider->callback( IqeB_GUI_Misc_SetValue_Callback, &CurrentShapeGen.ShapeArg);
    pSlider_ShapeArg = pTemp_ValSlider;

   // Next line

    x1  = xGroup;
    y += yy + 4;

    xx2 = 80;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, xx2, yy,
                                           &CurrentShapeGen.ShapeFlags, YAIPS_SHAPE_GEN_FLAG_DRAW_SHAPE,
                                           LangStringLookup( "&GUI_Overlay_TabB4=Fill"));
    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Overlay_TabB4a="
                         "Check this to use to draw a filled shape."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &CurrentShapeGen.ShapeFlags);

    pCheck_Bit_ShapeFill = pTemp_Check_Bit;

    x1 += xx2 + 8;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, xx2, yy,
                                           &CurrentShapeGen.ShapeFlags, YAIPS_SHAPE_GEN_FLAG_DRAW_LINE,
                                           LangStringLookup( "&GUI_Overlay_TabB5=Outline"));
    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Overlay_TabB5a="
                         "Check this to use to trace the outline."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &CurrentShapeGen.ShapeFlags);

    pCheck_Bit_ShapeOutline = pTemp_Check_Bit;

    // Next line

    x1  = xGroup + 100;
    y += yy + 6;

    pTemp_Button = new Fl_Button( x1, y, yy * 2, yy, LangStringLookup( "&GUI_Overlay_TabB10=Outline Color"));
    pTemp_Button->align( FL_ALIGN_LEFT);                  // align for label
    pTemp_Button->color( CurrentShapeGen.LineColor);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &CurrentShapeGen.LineColor);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabB10a="
                                             "Color for the shape outline."));
    pButCol_LineColor = pTemp_Button;

    x1 += yy * 2;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, 30, yy,
                                           &CurrentShapeGen.ShapeFlags, YAIPS_SHAPE_GEN_FLAG_LINE_COL_USE,
                                           LANGDEF_ACTIVE_SHORT);
    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Overlay_TabB11a="
                         "If set use outline color else use\n"
                         "the background for the outline."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &CurrentShapeGen.ShapeFlags);

    pCheck_Bit_LineColUse = pTemp_Check_Bit;

    // Next line

    x1  = xGroup + 100;
    y += yy + 6;

    xx2 = 40;
    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabB12=Outline width"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Overlay_TabB12a="
                        "The thickness of the lines in pixels.\n"
                        "Zero is the Windows defined default, which\n"
                        "is somewhat different and nicer than 1."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( CurrentShapeGen.LineWidth);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &CurrentShapeGen.LineWidth);
    pTemp_Int->SetModifyData( 1, 65, 1, 5);
    pInt_LineWidth = pTemp_Int;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'Text'
  //

  y = yGroup;
  x1  = xGroup;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Overlay_TabC1=Text"));

    y += 8;

    // Font selector

    y += 12;

    xx2 = xxColumn - yy + 8;

    iCurrFont = 0;

    pFontName = new Fl_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabC2=Font"));
    pFontName->align( FL_ALIGN_TOP_LEFT);     // align for label
    pFontName->labelsize( 10);
    pFontName->tooltip( LangStringLookup( "&GUI_Overlay_TabC2a="
                                          "Selected font"));

    pFontName->readonly( 1);    // Set read only. String is better visible then deactivate()
    pFontName->color( YAIPS_COLOR_RONLY_BGND);

    x1 += xx2;

    pFontChoose = new Fl_Button( x1, y, yy - 2, yy, "@-12>");
    pFontChoose->callback( IqeB_GUI_FontChoose_Callback);
    pFontChoose->tooltip( LangStringLookup( "&GUI_Overlay_TabC3a="
                                             "Select an other font"));

    // Next line

    y += yy + 14;
    x1  = xGroup;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabC4=Size"));
    pTemp_Int->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Int->labelsize( 10);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Overlay_TabC4a="
                                             "Font size"));
    pTemp_Int->SetValue( CurrentShapeGen.FontSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &CurrentShapeGen.FontSize);
    pTemp_Int->SetModifyData( 6, 600, 1, 5);
    pInt_FontSize = pTemp_Int;

    x1 += xx2;

    pSizeChoose = new Fl_Menu_Button( x1, y, yy - 2, yy);
    pSizeChoose->callback( IqeB_GUI_FontSizeButton_Callback);
    pSizeChoose->tooltip( LangStringLookup( "&GUI_Overlay_TabC4b="
                                             "Select font size"));
    x1 += yy - 2 + 8;

    xx2 = 34;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabC7=@-3SpacingH AV"));
    pTemp_Int->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Int->labelsize( 10);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Overlay_TabC7a="
                                             "Additional space between characters [pixel]"));
    pTemp_Int->SetValue( CurrentShapeGen.SpacingChar);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &CurrentShapeGen.SpacingChar);
    pTemp_Int->SetModifyData( 0, VALUE_SPACING_MAX, 1, 10);
    pInt_SpacingChar = pTemp_Int;

    x1 += xx2 + 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabC8=@-3SpacingV"));
    pTemp_Int->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Int->labelsize( 10);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Overlay_TabC8a="
                                             "Additional space between lines [pixel]"));
    pTemp_Int->SetValue( CurrentShapeGen.SpacingLine);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &CurrentShapeGen.SpacingLine);
    pTemp_Int->SetModifyData( 0, VALUE_SPACING_MAX, 1, 10);
    pInt_SpacingLine = pTemp_Int;

    // Next line

    y += yy + 14;
    x1  = xGroup;

    xx2 = yy - 2;

    pTemp_Box = new Fl_Box(  x1, y, 0, 0, LangStringLookup( "&GUI_Overlay_TabC10=Format"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Box->labelsize( 10);

    pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabC11=B"));
    pTemp_Button->labelfont( FL_HELVETICA_BOLD);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabC11a="
                                             "Toggle bold font style."));
    pTemp_Button->box( FL_BORDER_BOX);
    pTemp_Button->callback( IqeB_GUI_FontStyle_Callback, YAIPS_SHAPE_GEN_FLAG_FONT_BOLD_ON);
    pButFontBold = pTemp_Button;
    x1 += xx2 - 1;

    x1 += 4;

    pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabC12=I"));
    pTemp_Button->labelfont( FL_HELVETICA_ITALIC);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabC12a="
                                             "Toggle italic font style."));
    pTemp_Button->box( FL_BORDER_BOX);
    pTemp_Button->callback( IqeB_GUI_FontStyle_Callback, YAIPS_SHAPE_GEN_FLAG_FONT_ITALIC_ON);
    pButFontItalic = pTemp_Button;
    x1 += xx2 - 1;

    x1 += 4;

    pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabC13=U"));
    pTemp_Button->labelfont( FL_HELVETICA);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabC13a="
                                             "Toggle underline font style."));
    pTemp_Button->box( FL_BORDER_BOX);
    pTemp_Button->callback( IqeB_GUI_FontStyle_Callback, YAIPS_SHAPE_GEN_FLAG_FONT_UNDERL_ON);
    pButFontUnterl = pTemp_Button;
    x1 += xx2 - 1;

    x1 += 4;
    x1 += 4;

    pTemp_Box = new Fl_Box(  x1, y, 0, 0, LANGDEF_COLOR);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Box->labelsize( 10);

    pTemp_Button = new Fl_Button( x1, y, yy * 2, yy);
    pTemp_Button->color( CurrentShapeGen.FontColor);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &CurrentShapeGen.FontColor);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabC17a="
                                             "Text color"));
    pButCol_FontColor = pTemp_Button;

    x1 += yy * 2;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, 30, yy,
                                           &CurrentShapeGen.ShapeFlags, YAIPS_SHAPE_GEN_FLAG_FONT_COL_USE,
                                           LANGDEF_ACTIVE_SHORT);
    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Overlay_TabC18a="
                                                "If set use text color else use\n"
                                                "the background to color the text."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &CurrentShapeGen.ShapeFlags);
    pCheck_Bit_FontColUse = pTemp_Check_Bit;

    x1 += 32;

    // Next line

    y += yy + 14;
    x1  = xGroup;

    pTemp_Box = new Fl_Box( x1, y, 0, 0, LangStringLookup( "&GUI_Overlay_TabC20=Hor. align"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Box->labelsize( 10);

    xx2 = yy - 2;

    pTemp_Button = new Fl_Button( x1, y, xx2, yy, "@-1AlignHL");
    pTemp_Button->labelcolor( FL_BLUE);
    pTemp_Button->box( FL_BORDER_BOX);
    pTemp_Button->callback( IqeB_GUI_FontAlingHor_Callback, YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_LEFT);
    pButAlignHor[ 0] = pTemp_Button;
    x1 += xx2 - 1;

    pTemp_Button = new Fl_Button( x1, y, xx2, yy, "@-1AlignHC");
    pTemp_Button->labelcolor( FL_BLUE);
    pTemp_Button->box( FL_BORDER_BOX);
    pTemp_Button->callback( IqeB_GUI_FontAlingHor_Callback, YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_CENTER);
    pButAlignHor[ 1] = pTemp_Button;
    x1 += xx2 - 1;

    pTemp_Button = new Fl_Button( x1, y, xx2, yy, "@-1AlignHR");
    pTemp_Button->labelcolor( FL_BLUE);
    pTemp_Button->box( FL_BORDER_BOX);
    pTemp_Button->callback( IqeB_GUI_FontAlingHor_Callback, YAIPS_SHAPE_GEN_FLAG_ALIGN_HOR_RIGHT);
    pButAlignHor[ 2] = pTemp_Button;
    x1 += xx2;

    x1 += 2;

    xx2 = 38;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabC21=Indent"));
    pTemp_Int->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Int->labelsize( 10);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Overlay_TabC21a="
                                             "Horizontal indent"));
    pTemp_Int->SetValue( CurrentShapeGen.IndentHor);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &CurrentShapeGen.IndentHor);
    pTemp_Int->SetModifyData( 0, VALUE_INDENT_MAX, 1, 10);
    pInt_IndentHor = pTemp_Int;

    x1 += xx2 + 8;

    pTemp_Box = new Fl_Box( x1, y, 0, 0, LangStringLookup( "&GUI_Overlay_TabC22=Ver. align"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Box->labelsize( 10);

    xx2 = yy - 2;

    pTemp_Button = new Fl_Button( x1, y, xx2, yy, "@-1AlignVT");
    pTemp_Button->labelcolor( FL_BLUE);
    pTemp_Button->box( FL_BORDER_BOX);
    pTemp_Button->callback( IqeB_GUI_FontAlingVer_Callback, YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_TOP);
    pButAlignVer[ 0] = pTemp_Button;
    x1 += xx2 - 1;

    pTemp_Button = new Fl_Button( x1, y, xx2, yy, "@-1AlignVC");
    pTemp_Button->labelcolor( FL_BLUE);
    pTemp_Button->box( FL_BORDER_BOX);
    pTemp_Button->callback( IqeB_GUI_FontAlingVer_Callback, YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_CENTER);
    pButAlignVer[ 1] = pTemp_Button;
    x1 += xx2 - 1;

    pTemp_Button = new Fl_Button( x1, y, xx2, yy, "@-1AlignVB");
    pTemp_Button->labelcolor( FL_BLUE);
    pTemp_Button->box( FL_BORDER_BOX);
    pTemp_Button->callback( IqeB_GUI_FontAlingVer_Callback, YAIPS_SHAPE_GEN_FLAG_ALIGN_VER_BOTTOM);
    pButAlignVer[ 2] = pTemp_Button;
    x1 += xx2;

    x1 += 2;

    xx2 = 38;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabC23=Indent"));
    pTemp_Int->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Int->labelsize( 10);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Overlay_TabC23a="
                                             "Vertical indent"));
    pTemp_Int->SetValue( CurrentShapeGen.IndentVer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &CurrentShapeGen.IndentVer);
    pTemp_Int->SetModifyData( 0, VALUE_INDENT_MAX, 1, 10);
    pInt_IndentVer = pTemp_Int;

    // Next line

    y += yy + 14;
    x1  = xGroup;

    xx2 = xxColumn + 4;

    app_editor = new Fl_Text_Editor( x1, y, xx2, 77, LangStringLookup( "&GUI_Overlay_TabC30=Text"));
    app_editor->align( FL_ALIGN_TOP_LEFT);     // align for label
    app_editor->labelsize( 10);
    app_editor->tooltip( LangStringLookup( "&GUI_Overlay_TabC30a="
                                           "Text to display"));
    app_editor->textfont( FL_COURIER);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'Text'
  //

  y = yGroup;
  x1  = xGroup;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Overlay_TabD1=Shadow"));

    y += 8;

    xx2 = xxColumn;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, xx2, yy,
                                           &CurrentShapeGen.ShapeFlags, YAIPS_SHAPE_GEN_FLAG_SHADOW_USE,
                                           LangStringLookup( "&GUI_Overlay_TabD2=Active"));
    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Overlay_TabD2a="
                                                "Enable shadow."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &CurrentShapeGen.ShapeFlags);

    pCheck_Bit_ShadowUse = pTemp_Check_Bit;

    x1 += xx2 + 8;

    // Next line

    x1  = xGroup;
    y += yy + 16;

    xx2 = 52;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabD3=Angle"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Overlay_TabD3a=Angle of shadow casting [Degree]."));
    pFloatTemp->align( FL_ALIGN_TOP_LEFT);     // align for label
    pFloatTemp->labelsize( 10);
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( 0);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &CurrentShapeGen.ShadowAngle);
    pFloatTemp->SetModifyData( 0.0, 360.0, 15.0, 1.0, true);
    pShadowAngle = pFloatTemp;

    x1 += xx2 + 8;

    xx2 = 52;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabD4=Dist."));
    pTemp_Int->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Int->labelsize( 10);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Overlay_TabD4a="
                                             "Distance of shadow"));
    pTemp_Int->SetValue( CurrentShapeGen.ShadowDist);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &CurrentShapeGen.ShadowDist);
    pTemp_Int->SetModifyData( 0, 300, 10, 1);
    pInt_ShadowDist = pTemp_Int;

    x1 += xx2 + 8;

    pTemp_Box = new Fl_Box(  x1, y, 0, 0, LANGDEF_COLOR);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Box->labelsize( 10);

    pTemp_Button = new Fl_Button( x1, y, yy * 2, yy);
    pTemp_Button->color( CurrentShapeGen.ShadowColor);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &CurrentShapeGen.ShadowColor);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Overlay_TabD5a="
                                             "Shadow color"));
    pButCol_ShadowColor = pTemp_Button;

    x1 += yy * 2;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, 30, yy,
                                           &CurrentShapeGen.ShapeFlags, YAIPS_SHAPE_GEN_FLAG_SHADOW_COL_USE,
                                           LANGDEF_ACTIVE_SHORT);
    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Overlay_TabD6a="
                                                "If set use shadow color else use\n"
                                                "the oveleray image as shadow."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &CurrentShapeGen.ShapeFlags);
    pCheck_Bit_ShadowColUse = pTemp_Check_Bit;

    // Next line

    x1  = xGroup;
    y += yy + 16;

    xx2 = 52;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabD7=Blur"));
    pTemp_Int->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Int->labelsize( 10);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Overlay_TabD7a="
                                             "Shadow blur"));
    pTemp_Int->SetValue( CurrentShapeGen.ShadowDist);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &CurrentShapeGen.ShadowBlur);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_ShadowBlur = pTemp_Int;

    x1 += xx2 + 8;

    xx2 = 52;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Overlay_TabD8=Trans."));
    pTemp_Int->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Int->labelsize( 10);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Overlay_TabD8a="
                                             "Shadow transparency [%]"));
    pTemp_Int->SetValue( CurrentShapeGen.ShadowDist);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &CurrentShapeGen.ShadowTrans);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_ShadowTrans = pTemp_Int;

    x1 += xx2 + 8;

    // Finish things for this group

    pTemp_Group->end();

  // Tabs finish up -------------------------------------------

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

  IqeB_GUI_ToolsAnimManagerUpdate( false);    // update Browser
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
  Fl_Button *pGUI_TeachToggle;           // Toggle Teach / Inspection

  // Create the window

  CLASS_WIN_TOOL( int X, int Y, int W, int H, const char *l, int iToolDataArg) : Fl_Double_Window( X, Y, W, H, l)
  {
    YaIPS_ToolData_info_t *pToolData;
    Fl_Button *pTemp_Button;
    int iOverlay;

    iToolData = iToolDataArg;                    // Index of info data element, see YaIPS_ToolData_info
    pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

    // Initialize some data

    memset( &pToolData->YaIPS_ImageDisp, 0, sizeof( Fl_YaIPS_ImageDisp_t)); // Zero data

    pToolData->Input1_Change = 0;                // Reset image change check

    for( iOverlay = 0; iOverlay < pToolData->nOverlays && iOverlay < OVERLAY_NUM_MAX; iOverlay++) {

      pToolData->Overlay[ iOverlay].ShapeGen.BGnd_Win_Change = 0;                // Reset image change check
    }

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

    // Toggle Teach / Inspection

    xx = xx0;

    pGUI_TeachToggle = new Fl_Button( x1, y, xx, yy, "@+3pencil");
    pGUI_TeachToggle->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_TeachToggle->tooltip( LANGDEF_SWITCH_TEACH_INSPECT);
    pGUI_TeachToggle->labelcolor( YAIPS_BCOL_BUTTON);
    pGUI_TeachToggle->shortcut( FL_COMMAND+'t');       // Short cut key

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

    pToolData->YaIPS_ImageDisp.pImage_Box->pDropCallback = DropFile_cb;   // Accept file drops
    // Use the YaIPS_GUI_MyChangeOutput() function to display a pasted image.
    // This also saves the pasted image so it is reloaded on next open of this dialog.
    pToolData->YaIPS_ImageDisp.pImage_Box->pPasteImgCallback    = YaIPS_ImageDispPasteToOutFunc_cb; // Common paste image callback
    pToolData->YaIPS_ImageDisp.pImage_Box->PasteImgCallbackArg1 = &pToolData->YaIPS_ImageDisp;  // Pointer to Fl_YaIPS_ImageDisp_t

    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawBeforeCallback = YaIPS_ImageDispDrawBefore_cb; // Draw before callback
    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawAfterCallback  = YaIPS_GUI_MyDrawAfter_cb;     // Draw after callback
    pToolData->YaIPS_ImageDisp.pImage_Box->DrawCallbackArg1    = &pToolData->YaIPS_ImageDisp;  // Pointer to Fl_YaIPS_ImageDisp_t
    pToolData->YaIPS_ImageDisp.pImage_Box->DrawCallbackArg2    = pToolData;                    // Optional pointer to ToolData

    pToolData->YaIPS_ImageDisp.pImage_Box->pMouseCallback      = YaIPS_GUI_MyMouse_cb;         // Mouse event callback
    pToolData->YaIPS_ImageDisp.pImage_Box->MouseCallbackArg1   = &pToolData->YaIPS_ImageDisp;  // Pointer to Fl_YaIPS_ImageDisp_t
    pToolData->YaIPS_ImageDisp.pImage_Box->MouseCallbackArg2   = pToolData;                    // Optional pointer to ToolData

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
  int iOverlay, DeleteFile;
  char TempFileName[ 256 + 16];

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

  if( pToolData->pMyToolWin == NULL) {  // security test, has main window

    return;
  }

  // Release intermediate images

  for( iOverlay = 0; iOverlay < OVERLAY_NUM_MAX; iOverlay++) {

    if( pToolData->Overlay[ iOverlay].pImgOverlay != NULL) {       // Have an intermediate image

      (pToolData->Overlay[ iOverlay].pImgOverlay)->release();      // Release image data

      pToolData->Overlay[ iOverlay].pImgOverlay = NULL;
    }

    if( pToolData->Overlay[ iOverlay].pImgShadow != NULL) {        // Have an intermediate image

      (pToolData->Overlay[ iOverlay].pImgShadow)->release();       // Release image data

      pToolData->Overlay[ iOverlay].pImgShadow = NULL;
    }

    if( pToolData->Overlay[ iOverlay].pBGndFile != NULL) {       // Have an intermediate image

      (pToolData->Overlay[ iOverlay].pBGndFile)->release();      // Release image data

      pToolData->Overlay[ iOverlay].pBGndFile = NULL;
    }

    // Save text buffer if used or ensure deleted if not used

    sprintf( TempFileName, "%s/Images/YaIPS/Overlay-Text-%02d-%02d.txt", YaIPS_WorkingDirectory, (int)iToolData + 1, iOverlay + 1);
    IqeB_FileNormalizePathChars( TempFileName);

    DeleteFile = false;         // Preset no delete

    if( pToolData->Overlay[ iOverlay].pTextBuffer == NULL) {     // Have NO text buffer allocated

      DeleteFile = true;         // Ensure text buffer is deleted

    } else {         // Have a text buffer allocated

      if( (pToolData->Overlay[ iOverlay].pTextBuffer)->length() > 0) {         // Does we have any data in the text bufer

        (pToolData->Overlay[ iOverlay].pTextBuffer)->savefile( TempFileName);  // Save the text buffer

      } else {

        DeleteFile = true;         // Ensure text buffer is deleted
      }

      delete (pToolData->Overlay[ iOverlay].pTextBuffer);              // Release text buffer

      pToolData->Overlay[ iOverlay].pTextBuffer = NULL;
    }

    if( DeleteFile) {             // Check delete of text buffer

      // If existing, delete the file
      IqeB_FileDelete( TempFileName);
    }

    // Ensure not used clipboard images are deleted

    DeleteFile = false;         // Preset no delete

    // Construct a file name for the clipboard image

    sprintf( TempFileName, "%s/Images/YaIPS/Clipboard-Overlay-%d-%d.png", YaIPS_WorkingDirectory, (int)iToolData + 1, iOverlay + 1);
    IqeB_FileNormalizePathChars( TempFileName);

    if( pToolData->Overlay[ iOverlay].ShapeGen.BGndType != YAIPS_SHAPE_GEN_BGND_IMAGE) {

      DeleteFile = true;         // Ensure clipboard image is deleted

    } else {

      // Background is image

      // Check for clipboard image
      if( strcmp( pToolData->Overlay[ iOverlay].ShapeGen.BGndFileName, TempFileName) != 0) {

        DeleteFile = true;        // Ensure clipboard image is deleted
      }
    }

    if( DeleteFile) {             // Check delete of clipboard image

      // If existing, delete the file
      IqeB_FileDelete( TempFileName);
    }
  }

  YaIPS_ImageDispReleaseBeforeClose( &pToolData->YaIPS_ImageDisp);

  pToolData->IsOpen = false;               // Flag info data is not in use

  IqeB_GUI_CloseToolWindow( (void **)&pToolData->pMyToolWin);

  pOverlayBrowser = NULL;                        // Must be reseted at exit

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

  } else if( w == pMyToolWin->pGUI_TeachToggle) {              // Toggle Teach / Inspection

    pToolData->TeachMode = ! pToolData->TeachMode;

    pToolData->Input1_Change = 0;          // Force recalculation output
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
  YaIPS_RGB_ShapeGen_Par_t *pShapeGen;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

  if( Fl::clipboard_contains(Fl::clipboard_image) == 0) {  // NO image in the clipboard

    return;
  }

  if( pToolData->YaIPS_ImageDisp.pImage_Box == NULL) {     // Security test

    return;
  }

  if( BrowserSelected < 0) {              // Security test, no overlay selected

    return;
  }

  pShapeGen = &pToolData->Overlay[ BrowserSelected].ShapeGen;  // Point to current shape

  // Background type must be image to accept the paste

  if( pShapeGen->BGndType != YAIPS_SHAPE_GEN_BGND_IMAGE) {

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


  // Check input image and visualize state
  YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr, pMyToolWin->pBox_Input1);

  // Update color of Toggle Teach / Inspection button

  int MouseTeachState;

  MouseTeachState = 1;                                            // We have a mouse callback state

  // Is teach mode available
  if( DoEnable) {                                                 // Enable GUI elements

    MouseTeachState = pToolData->TeachMode ? 3 : 2;
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

    } else {

      int iOverlay;
      YaIPS_OverlayData_t *pOverlay;

      pOverlay = pToolData->Overlay;

      for( iOverlay = 0; iOverlay < pToolData->nOverlays && iOverlay < OVERLAY_NUM_MAX; iOverlay++, pOverlay++) {

        if( pOverlay->ShapeGen.BGndType == YAIPS_SHAPE_GEN_BGND_WINDOW) {   // Check for background window

          int BGnd_Win_ImageChanged;

          YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pOverlay->ShapeGen.BGnd_WinIdNr, NULL, NULL, &BGnd_Win_ImageChanged);

          if( BGnd_Win_ImageChanged != pOverlay->ShapeGen.BGnd_Win_Change) {  // Any of the images has changed since the last call

            pOverlay->ShapeGen.BGnd_Win_Change = BGnd_Win_ImageChanged;       // Copy new image count

            pOverlay->ParChanged = 1;                                         // Recreate intermediate image

            pToolData->Input1_Change = 0;                                     // Fore new compute
          }
        }
      }

      if( Input1_ImageChanged != pToolData->Input1_Change) { // Image count is different

        pToolData->Input1_Change = Input1_ImageChanged;        // Image is processed

        // Do the image processing

        ierr = 0;                                              // Reset error
        errstring = NULL;                                      // Reset error string

        //
        // Image processing begin
        //

        // Copy background image

        ierr = YaIPS_RGB_CopyImg( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1);

        if( ierr != 0) goto ProcessingError;    // Security test, check for error

        pOverlay = pToolData->Overlay;

        for( iOverlay = 0; iOverlay < pToolData->nOverlays && iOverlay < OVERLAY_NUM_MAX; iOverlay++, pOverlay++) {

          IqeB_GUI_Param_RecalcAOI( pOverlay);   // Ensure proper AOI values

          if( pOverlay->pImgOverlay == NULL ||   // Have no intermediate image until now
              pOverlay->ParChanged != NULL) {    // or parameter for this image has changed

            // Until know it better, overwrite some parameters

            ierr = YaIPS_RGB_ShapeGen( &pOverlay->pImgOverlay,   // Out: Pointer to pointer to RGB color image
                                       pOverlay->AOI.XSize, pOverlay->AOI.YSize,      // In: size of image.
                                       &pOverlay->ShapeGen,
                                       &pOverlay->pImgShadow,
                                       &pOverlay->pBGndFile,
                                       pOverlay->pTextBuffer);

            if( ierr != 0) goto ProcessingError;    // Security test, check for error

            pOverlay->ParChanged = 0;               // Reset: parameter has changed
          }

          if( pOverlay->pImgOverlay == NULL) {  // Security test
            goto ProcessingError;               // Security test, check for error
          }

          ierr = YaIPS_RGB_Overlay( pToolData->YaIPS_ImageDisp.pImage_Img,    // Pointer to RGB color image. Must exist.
                                    pOverlay);                                // Point to overlay data

          if( ierr < 0) goto ProcessingError;    // Security test, check for error
        }

        //
        // Image processing end
        //

        // Has a valid output image

ProcessingError:      // On image processing error jump to here

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
  YaIPS_RGB_ShapeGen_Par_t *pShapeGen;

  if( pNewimage == NULL) {                                   // Security test

    return;
  }

  pToolData = YaIPS_ToolData_info + iToolData;               // Point to info data

  if( pToolData->YaIPS_ImageDisp.pImage_Box == NULL) {       // Security test

    return;
  }

  if( BrowserSelected < 0) {              // Security test, no overlay selected

    return;
  }

  pShapeGen = &pToolData->Overlay[ BrowserSelected].ShapeGen;  // Point to current shape

  // Background type must be image to accept the paste

  if( pShapeGen->BGndType != YAIPS_SHAPE_GEN_BGND_IMAGE) {

    return;
  }

  // ...

  // Construct a file name for the image

  sprintf( TempFileName, "%s/Images/YaIPS/Clipboard-Overlay-%d-%d.png", YaIPS_WorkingDirectory, iToolData + 1, BrowserSelected + 1);

  IqeB_FileNormalizePathChars( TempFileName);

  // Save the image
  YaIPS_Image_Write_PNG( TempFileName, pNewimage);

  // Remember last loaded file name
  memset( pShapeGen->BGndFileName, 0, sizeof( pShapeGen->BGndFileName));
  strncpy( pShapeGen->BGndFileName, TempFileName, sizeof( pShapeGen->BGndFileName) - 1);

  pToolData->Overlay[ BrowserSelected].ParChanged = 1;        // Recreate intermediate image

  // Set new loaded file

  if( pToolData->Overlay[ BrowserSelected].pBGndFile != NULL) {   // Was a file loaded before ?

    pToolData->Overlay[ BrowserSelected].pBGndFile->release();    // Release data of this file

    pToolData->Overlay[ BrowserSelected].pBGndFile = NULL;
  }

  YaIPS_RGB_CopyImg( &pToolData->Overlay[ BrowserSelected].pBGndFile, pNewimage);

  pToolData->Input1_Change = 0;                    // Force recalculation output

  if( pOverlayBrowser != NULL) {         // Parameter dialog is open

    // update all enables
    IqeB_GUI_ToolsAnimManagerUpdateSelected( BrowserSelected);  // Update enables
  }
}

/************************************************************************************
 * IqeB_GUI_OverlayWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_OverlayWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  CLASS_WIN_TOOL *pMyToolWin;
  int xPos, yPos, nTabelOnEntry, iOverlay;
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

  pToolData->iToolData = iToolData;                        // Set sub window number

  pToolData->IsOpen = true;           // Flag image as open

  pOverlayBrowser = NULL;                        // Must be reseted at exit
  pToolDataParam  = NULL;                        // Must be reseted at exit
  app_editor      = NULL;                        // Must be reseted at exit

  // Ensure all intermediate images will be recreated
  for( iOverlay = 0; iOverlay < pToolData->nOverlays && iOverlay < OVERLAY_NUM_MAX; iOverlay++) {

    YaIPS_OverlayData_t *pOverlay;
    char TempFileName[ 256 + 16];

    pOverlay = pToolData->Overlay + iOverlay;

    pOverlay->ParChanged = 1;     // Recreate intermediate image
    pOverlay->pImgOverlay = NULL; // Has no image object until now
    pOverlay->pImgShadow = NULL;  // Has no image object until now
    pOverlay->pBGndFile = NULL;   // Has no image object until now

    pOverlay->pTextBuffer = NULL; // Has no text buffer until now

    // Load text buffers (if exisiting)
    sprintf( TempFileName, "%s/Images/YaIPS/Overlay-Text-%02d-%02d.txt", YaIPS_WorkingDirectory, (int)iToolData + 1, iOverlay + 1);

    IqeB_FileNormalizePathChars( TempFileName);

    pOverlay->pTextBuffer = new Fl_Text_Buffer(); // Create new text buffer
    pOverlay->pTextBuffer->add_modify_callback( text_changed_callback, NULL);

    if( IqeB_FileExsits( TempFileName)) {                             // Does the file exist

      (pOverlay->pTextBuffer)->loadfile( TempFileName);
    }

    // Check for change of global unit to last used unit
    if( pOverlay->AoiUnit != YaIPS_Calib_Unit) {

      // Modify float values by change of the unit

      YaIPS_Calib_Change_Unit( &pOverlay->PosX, &pOverlay->PosY, pOverlay->AoiUnit, YaIPS_Calib_Unit);

      YaIPS_Calib_Change_Unit( &pOverlay->SizeX, &pOverlay->SizeY, pOverlay->AoiUnit, YaIPS_Calib_Unit);

      pOverlay->AoiUnit = YaIPS_Calib_Unit;
    }

    IqeB_GUI_Param_RecalcAOI( pOverlay);   // Ensure proper AOI values
  }

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
  int iOverlay, iDraw;
  int x1, y1, xx, yy;
  float OrientationAngle;
  char TempString[ 256];
  Fl_Color DrawColor;
  YaIPS_OverlayData_t *pOverlay;
  Fl_YaIPS_AOI_t *pAOI;       // Point to AOI to test

  if( pYaIPS_ImageDisp->pImage_Img == NULL ||      // Have no image to display
      pToolData->TeachMode == 0) {                 // Teach mode is off

    return;
  }

  // Preparations

  x1 = pYaIPS_ImageDisp->BigImage_sx;
  y1 = pYaIPS_ImageDisp->BigImage_sy;
  xx = pYaIPS_ImageDisp->BigImage_sw;
  yy = pYaIPS_ImageDisp->BigImage_sh;

  // Clipping ?

  if( DoClip > 0) {      // The the draw clipping

    DoClip = -1;         // Need to pop clipping

    fl_push_clip( x1, y1, xx, yy);
  }

  for( iOverlay = 0; iOverlay < pToolData->nOverlays && iOverlay < OVERLAY_NUM_MAX; iOverlay++) {

    // Draw color for AOI

    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0 &&  // Mouse is over any AOI
        pYaIPS_ImageDisp->AoiIdNr >= 0) {                                   // and any AOI selected

      // HACK: draw the selected AOI always as the last one. Red colored frame is over other green colored frames.

      if( iOverlay == pToolData->nOverlays - 1) {           // Draw last AOI

        iDraw = pYaIPS_ImageDisp->AoiIdNr;                  // --> Draw selected AOI

      } else if( iOverlay < pYaIPS_ImageDisp->AoiIdNr) {    // Draw before selected

        iDraw = iOverlay;                                   // --> just draw it

      }  else {                                             // Draw after selected

        iDraw = iOverlay + 1;                               // --> draw one up
      }

    } else {

      iDraw = iOverlay;
    }

    pOverlay = pToolData->Overlay + iDraw;

    IqeB_GUI_Param_RecalcAOI( pOverlay);   // Ensure proper AOI values

    pAOI = &pOverlay->AOI;
    OrientationAngle = pOverlay->ShapeGen.RotAngle;

    // Ensure AOI is in image.
    // This helps if the input image is resized smaller than the AOI.
    //x/YaIPS_ImageDispAoiRectClip( pYaIPS_ImageDisp, pAOI);

    // Draw AOI

    // Draw color for AOI
    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0 &&  // Mouse is over any AOI
        pYaIPS_ImageDisp->AoiIdNr >= 0 &&                                   // and any AOI selected
        iOverlay == pToolData->nOverlays - 1) {                             // Draw last AOI

      DrawColor = FL_RED;

    } else if( iDraw == BrowserSelected) {                                  // Is selected in browser

      DrawColor = FL_MAGENTA;

    } else {

      DrawColor = FL_GREEN;
    }

    sprintf( TempString, "%d: %s", iDraw + 1, pOverlay->Name);

    YaIPS_RotatableAOI_Draw( pYaIPS_ImageDisp, pAOI, DrawColor, TempString, OrientationAngle);
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
 * Return: < 0  Error, don't processed mouse callback
 *           0 OK, processed mouse callback
 *
 */

static int YaIPS_GUI_MyMouse_cb( Fl_Widget *pW, int event,
                                 void *pArg1,        // Pointer to Fl_YaIPS_ImageDisp_t
                                 void *pArg2)        // Optional pointer to ToolData
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  int ierr, x, y, iOverlay, AoiIdNrEntry, XSizeBefore, YSizeBefore;
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

    return( -1);                                       // Return error
  }

  // Mouse callback common entry work

  ierr = YaIPS_ImageDispMouse_CommonEntry( pW, event, pYaIPS_ImageDisp, &x, &y, &Last_x, &Last_y);

  if( ierr < 0) {            // Error or problem

    return( ierr);           // Return to caller

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

    // NOTE: if we come to here, no overlay parameter dialog is up

    int Delta_MouseWheel_dy, Delta_MouseWheel_dx, EventState;
    float DeltaAngle;

    Delta_MouseWheel_dx = Fl::event_dx();              // Get direction of wheel
    Delta_MouseWheel_dy = Fl::event_dy();              // Get direction of wheel

    EventState       = Fl::event_state();

    if( pToolData->TeachMode != 0 &&                   // Can be changed with the mouse
        pYaIPS_ImageDisp->AoiIdNr >= 0 &&              // One of the overlays is selected
        (EventState & (FL_SHIFT | FL_CTRL)) != 0) {    // and shift or control key is pressed

      if( (EventState & FL_SHIFT) != 0) {              // Shift key pressed

        DeltaAngle = 15.0;

      } else {                                         // Control key pressed

        DeltaAngle = 1.0;
      }

      if( Delta_MouseWheel_dx + Delta_MouseWheel_dy >= 0) {

        DeltaAngle = 0.0 - DeltaAngle;
      }

#ifdef use_again
#ifdef _DEBUG
      YaIPS_ImageDispStrInfo( pYaIPS_ImageDisp, "FL_MOUSEWHEEL: Dm %d/%d Shift %d Ctrl %d Da %d",
                               Delta_MouseWheel_dx, Delta_MouseWheel_dy,
                               (EventState & FL_SHIFT) != 0, (EventState & FL_CTRL) != 0, (int)DeltaAngle);
      pYaIPS_ImageDisp->pImage_Box->redraw();
#endif
#endif

      // Change angle

      pToolData->Overlay[ pYaIPS_ImageDisp->AoiIdNr].ShapeGen.RotAngle += DeltaAngle;

      // Handle angle wrap around

      if( pToolData->Overlay[ pYaIPS_ImageDisp->AoiIdNr].ShapeGen.RotAngle < 0.0) {

        pToolData->Overlay[ pYaIPS_ImageDisp->AoiIdNr].ShapeGen.RotAngle += 360.0;
      }

      if( pToolData->Overlay[ pYaIPS_ImageDisp->AoiIdNr].ShapeGen.RotAngle >= 360.0) {

        pToolData->Overlay[ pYaIPS_ImageDisp->AoiIdNr].ShapeGen.RotAngle -= 360.0;
      }

      pToolData->Input1_Change = 0;                    // Force recalculation output

      pYaIPS_ImageDisp->RedrawOnExit   = true;                      // Set redraw on exit
      pYaIPS_ImageDisp->BigImageUpdate = pYaIPS_ImageDisp->MyWinID; // Update big image

      pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;    // Set AOI changed flag bit

    } else if( IsBigImageDisp) {                        // Called for big image display

      // Manage display resolution change by mouse wheel event
      YaIPS_ImageDispMouse_CommonMouseWheel( pYaIPS_ImageDisp, x, y);
    }

    // HACK: keep AOI selected
    if( pToolData->TeachMode != 0 &&                   // Can be changed with the mouse
        pYaIPS_ImageDisp->AoiIdNr >= 0) {              // One of the overlays is selected

      pYaIPS_ImageDisp->AoiDeltaAdd = 1;                // Hack to keep slected AOI selected
    }

    break;

  case FL_PUSH:          // A mouse button has gone down
  case FL_RELEASE:       // A mouse button has been released.
  case FL_MOVE:          // The mouse has moved without any mouse buttons held down.
  case FL_DRAG:          // The mouse has moved with a button held down.
  case FL_DND_DRAG:      // Mouse has moved while dragging a file

#ifdef use_again
#ifdef _DEBUG
    YaIPS_ImageDispStrDebug( pYaIPS_ImageDisp, "FL_MOVE: %d/%d %d/%d", pYaIPS_ImageDisp->Delta_x, pYaIPS_ImageDisp->Delta_y, x, y);
    pYaIPS_ImageDisp->pImage_Box->redraw();
#endif
#endif

    // Left mouse button pressed and NOT 3D plot active

    if( pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_AUTO) {  // Any resolution

      if( pToolData->TeachMode != 0) {                                        // and can be changed with the mouse

        AoiIdNrEntry = pYaIPS_ImageDisp->AoiIdNr;

        if( pYaIPS_ImageDisp->mouseleft &&                    // Left mouse button pressed
            pYaIPS_ImageDisp->Latched_AoiDeltaAdd != 0 &&     // Have latched AOI mouse modification
            pYaIPS_ImageDisp->AoiIdNr >= 0) {                 // Have any AOI selected

          // Get latched data
          CursorShapeTest = pYaIPS_ImageDisp->CursorShape;
          AoiDeltaAddTest = pYaIPS_ImageDisp->AoiDeltaAdd;
          iOverlay = pYaIPS_ImageDisp->AoiIdNr;           // This AOI selected
          pAOI_Best   = &pToolData->Overlay[ iOverlay].AOI;

        } else {

          pYaIPS_ImageDisp->AoiIdNr = -1;           // No AOI selected until now

          for( iOverlay = 0; iOverlay < pToolData->nOverlays && iOverlay < OVERLAY_NUM_MAX; iOverlay++) {

            pAOI_This = &pToolData->Overlay[ iOverlay].AOI;

            ierr = YaIPS_RotatableAOI_MouseCC( pYaIPS_ImageDisp, pAOI_This, pToolData->Overlay[ iOverlay].ShapeGen.RotAngle,
                                               &minAoiDist, &CursorShapeTest, &AoiDeltaAddTest);

            if( ierr == true) {     // Got one (or a better one)

              // nearer aoi found
              pYaIPS_ImageDisp->CursorShape = CursorShapeTest;
              pYaIPS_ImageDisp->AoiDeltaAdd = AoiDeltaAddTest;
              pYaIPS_ImageDisp->AoiIdNr = iOverlay;           // This AOI selected
              pAOI_Best   = pAOI_This;
            }
          }

          iOverlay = pYaIPS_ImageDisp->AoiIdNr;               //
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

          // Mouse click over window, also select in browser
          if( BrowserSelected != pYaIPS_ImageDisp->AoiIdNr) {  // Selection will change

            BrowserSelected = pYaIPS_ImageDisp->AoiIdNr;       // This line is selected

            // Update frame color
            pToolData->YaIPS_ImageDisp.pImage_Box->redraw();

            if(YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

              YaIPS_BigImageDisp.pImage_Box->redraw();
            }
          }

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

      if( pToolData->TeachMode != 0 &&                                          // Teach mode on
          pYaIPS_ImageDisp->mouseleft == 0 &&                                   // NO left button pressed
          pYaIPS_ImageDisp->mouseright) {                                       // right button pressed

        // Mouse click over window, also select in browser
        if( BrowserSelected != pYaIPS_ImageDisp->AoiIdNr) {  // Selection will change

          BrowserSelected = pYaIPS_ImageDisp->AoiIdNr;       // This line is selected

          // Update frame color
          pToolData->YaIPS_ImageDisp.pImage_Box->redraw();

          if(YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

            YaIPS_BigImageDisp.pImage_Box->redraw();
          }
        }

        if( pMyParWin == NULL) {          // Parameter windw is not open

          // Open a popup menu

          Fl_Menu_Button popup( Fl::event_x(), Fl::event_y(), 80, 1);
          const Fl_Menu_Item *m;

          popup.add( LangStringLookup( "&GUI_Overlay_Popup_Param=Parameter"), 0, NULL, (void*)1);

          if( BrowserSelected >= 0) {                                // Any 0verlay selected

            popup.add( LangStringLookup( "&GUI_Overlay_Popup_Del=Delete overlay"), 0, NULL, (void*)2);

            if( pToolData->nOverlays < OVERLAY_NUM_MAX) {
              popup.add( LangStringLookup( "&GUI_Overlay_Popup_Ins=Insert new overlay"), 0, NULL, (void*)3);
            }

          } else {                                                   // No overlay selected

            if( pToolData->nOverlays < OVERLAY_NUM_MAX) {
              popup.add( LangStringLookup( "&GUI_Overlay_Popup_App=Append new overlay"), 0, NULL, (void*)4);
            }

            if( pToolData->nOverlays > 0) {    // Have any overlay
              popup.add( LangStringLookup( "&GUI_Overlay_Popup_DelAll=Delete all overlays"), 0, NULL, (void*)5);
            }
          }

          m = popup.popup();
          if( m!= NULL) {

            CLASS_WIN_TOOL *pMyToolWin;

            pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

            switch( (long long)m->user_data()) {

            case 1: // Parameter

              // Open Parameter windows

              YaIPS_GUI_ParameterWin( pMyToolWin->x() + 16, pMyToolWin->y() + 16, pToolData->iToolData);
              break;

            case 2: // Delete overlay

              pToolDataParam = pToolData;        // Set data pointer for buttons or other callback functions

              ButtonDeleteCallback( NULL, (void *)(long long)BrowserSelected);

              pToolDataParam = NULL;            // Must reset after use
              break;

            case 3: // Insert overlay

              pToolDataParam = pToolData;        // Set data pointer for buttons or other callback functions

              ButtonNewCallback( NULL, (void *)(long long)BrowserSelected);

              pToolDataParam = NULL;            // Must reset after use

              // Refresh GUI

              // Update frame color
              pToolData->YaIPS_ImageDisp.pImage_Box->redraw();

              if(YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

                YaIPS_BigImageDisp.pImage_Box->redraw();
              }

              // Open Parameter windows

              YaIPS_GUI_ParameterWin( pMyToolWin->x() + 16, pMyToolWin->y() + 16, pToolData->iToolData);
              break;

            case 4: // Append overlay

              pToolDataParam = pToolData;        // Set data pointer for buttons or other callback functions

              // Outside any window, append after last window

              ButtonNewCallback( NULL, (void *)(long long)(pToolData->nOverlays - 1 + OVERLAY_NUM_MAX + OVERLAY_NUM_MAX));

              pToolDataParam = NULL;            // Must reset after use

              // Refresh GUI

              BrowserSelected = pToolData->nOverlays - 1;      // This line is selected
              pYaIPS_ImageDisp->AoiIdNr = BrowserSelected;

              // Update frame color
              pToolData->YaIPS_ImageDisp.pImage_Box->redraw();

              if(YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

                YaIPS_BigImageDisp.pImage_Box->redraw();
              }

              // Open Parameter windows

              YaIPS_GUI_ParameterWin( pMyToolWin->x() + 16, pMyToolWin->y() + 16, pToolData->iToolData);
              break;

            case 5: // Delete all overlays

              pToolDataParam = pToolData;        // Set data pointer for buttons or other callback functions

              ButtonDelAllCallback( NULL, NULL);

              pToolDataParam = NULL;            // Must reset after use
              break;
            } // End switch
          }
        }
      }

      if( pYaIPS_ImageDisp->mouseleft &&                                        // and left button pressed
          pYaIPS_ImageDisp->AoiDeltaAdd != 0 &&                                 // and add deltas
          (pYaIPS_ImageDisp->Delta_x != 0 || pYaIPS_ImageDisp->Delta_y != 0)) { // and mouse has moved

        YaIPS_OverlayData_t *pOverlay;

        pOverlay = pToolData->Overlay + pYaIPS_ImageDisp->AoiIdNr;

        memcpy( pAOI_Best, &Pressed_AOI, sizeof( Fl_YaIPS_AOI_t)); // Restore AOI data from button press

        pYaIPS_ImageDisp->Delta_x = dto32( (Last_x - Pressed_x) / pYaIPS_ImageDisp->PixelImageToScreen);
        pYaIPS_ImageDisp->Delta_y = dto32( (Last_y - Pressed_y) / pYaIPS_ImageDisp->PixelImageToScreen);

        XSizeBefore = pAOI_Best->XSize;
        YSizeBefore = pAOI_Best->YSize;

        YaIPS_RotatableAOI_DeltaAdd( pYaIPS_ImageDisp, pAOI_Best, pOverlay->ShapeGen.RotAngle,
                                     pYaIPS_ImageDisp->AoiDeltaAdd, pYaIPS_ImageDisp->Delta_x, pYaIPS_ImageDisp->Delta_y,
                                     (pOverlay->ShapeGen.ShapeFlags & YAIPS_SHAPE_GEN_FLAG_AOI_SIZE_RATIO) != 0,
                                     pOverlay->ShapeGen.AOI_XX_Locked,
                                     pOverlay->ShapeGen.AOI_YY_Locked);

        // Update AOI input fields

        pOverlay->PosX  = pOverlay->AOI.XPos  * YaIPS_Calib_UPP_X;
        pOverlay->PosY  = pOverlay->AOI.YPos  * YaIPS_Calib_UPP_Y;
        pOverlay->SizeX = pOverlay->AOI.XSize * YaIPS_Calib_UPP_X;
        pOverlay->SizeY = pOverlay->AOI.YSize * YaIPS_Calib_UPP_Y;

        // ...

#ifdef use_again
#ifdef _DEBUG
        YaIPS_ImageDispStrDebug( pYaIPS_ImageDisp, "Delta: %d/%d Size: %dx%d -> %dx%d",
                                 pYaIPS_ImageDisp->Delta_x, pYaIPS_ImageDisp->Delta_y,
                                 XSizeBefore, YSizeBefore, pAOI_Best->XSize, pAOI_Best->YSize);
        pYaIPS_ImageDisp->pImage_Box->redraw();
#endif
#endif

        if( XSizeBefore != pAOI_Best->XSize ||     // Size has changed
            YSizeBefore != pAOI_Best->YSize) {

          pToolData->Overlay[ pYaIPS_ImageDisp->AoiIdNr].ParChanged = 1;  // Recreate intermediate image
        }

        pToolData->Input1_Change = 0;                                 // Force recalculation output

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
 * IqeB_GUI_OverlayWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_OverlayWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( pEmptyTextBuffer == NULL) {              // Have no empty textbuffer

    pEmptyTextBuffer = new Fl_Text_Buffer();   // Set empt textbuffer
  }

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

    // Register draw after function for big image display
    YaIPS_ToolWinDrawAfterSet( MY_WIN_ID + SubWinIDx, YaIPS_GUI_MyDrawAfter_Other);

    // Register change output function for big image display
    YaIPS_ToolChangeOutputSet( MY_WIN_ID + SubWinIDx, YaIPS_GUI_MyChangeOutput);

	  IqeB_GUI_OverlayWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  // Register change output function for big image display
  YaIPS_ToolChangeOutputSet( MY_WIN_ID + iUnused, YaIPS_GUI_MyChangeOutput);

  IqeB_GUI_OverlayWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


