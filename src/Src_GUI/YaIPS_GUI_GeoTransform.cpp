/****************************************************************************

  YaIPS_GUI_GeoTransform.cpp

  Geometric transformations of Images

 04.06.2025 RR: First edition of this file.
 05.10.2026 RR: * Finished coding for tools
                  * Trapezoidal distortion
                  * Warp, applies a perspective transformation to an image.

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
#define MY_WIN_ID     YAIPS_WIN_ID_GEOTRAN      // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_GEOTRAN       // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_GeoTrans_Title=Geometry transformation"  // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinGeoTran"            // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_GeoTran_Tool  // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       246
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
static void YaIPS_GUI_MyDrawAfter_cb( Fl_Widget *pW, void *pArg1, void *pArg2);
static int YaIPS_GUI_MyMouse_cb( Fl_Widget *pW, int event, void *pArg1, void *pArg2);

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

  // Used for intern data management

  Fl_YaIPS_ImageDisp_t YaIPS_ImageDisp;   // Info image output

  // Used to catch a change of the input image
  int Input1_Change;                    // Last processed 'ImageChanged' from input image

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int Tab_Group_Selected;               // Number of last selected tab group.
  int GeoTranType;                      // What geometry transformation to use

  int Teach_mode;                       // 0 = inspection mode, 1 = teach mode

  // Group 'Base'

  int Base_OutsiteColor;                // Color used for areas outside an image
  int Base_OutsiteBlend;                // If set, outside area is alpha blended

  // Group 'Simple'

  int Resize2SizeShift;                 // Resize with power of 2
  float Base_Rotate;                    // Rotate clockwise
  float Base_ScalePer;                  // Size correction percent [%]

  // Group 'Lens'

  int Lens_CorrFac;                     // Lens correction factor [in units of 1.0E-10]
  float Lens_Rotation;                  // Rotation correction [Degree]
  float Lens_SizeCorrPer;               // Size correction percent [%]
  int Lens_xDelta; int Lens_yDelta;     // Position correction [pixel]

  // Group 'Trapezoid 1' Trapezoidal distortion with parameters

  float Trapezoid_Fac;                  // Trapezoid distortion factor
  float Trapezoid_Rotation;             // Rotation correction [Degree]
  float Trapezoid_SizeCorrPer;          // Size correction percent [%]
  int Trapezoid_xDelta; int Trapezoid_yDelta;  // Position correction [pixel]

  // Group 'Trapezoid 2' Trapezoidal distortion with 4 points

  YaIPS_XY_int Trapezoid_SrcPoint[ 4];  // 4 Source points
  YaIPS_XY_int Trapezoid_DstPoint[ 4];  // 4 Destination points

  // Group 'Parallel projection'

  float ParPro_Rotation;                       // Rotation clockwise [Degree]
  float ParPro_xscal; float  ParPro_yscal;     // Scale image horizontal and vertical
  float ParPro_xshift; float  ParPro_yshift;   // Shift image horizontal and vertical

  // Tab group 'Cut out'

  Fl_YaIPS_AOI_t CutOut_AOI;                  // Cut out AOI

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

  { PREF_T_INT,      "Group_Selected",  "0", &YaIPS_ToolData_info[0].Tab_Group_Selected},
  { PREF_T_INT,        "GeoTranType",   "0", &YaIPS_ToolData_info[0].GeoTranType},

  { PREF_T_INT,         "Teach_mode",   "0", &YaIPS_ToolData_info[0].Teach_mode},

  // Group 'Base'

  { PREF_T_INT,  "Base_OutsiteColor",  "43", &YaIPS_ToolData_info[0].Base_OutsiteColor},   // Default: Gray 128
  { PREF_T_INT,  "Base_OutsiteBlend",   "0", &YaIPS_ToolData_info[0].Base_OutsiteBlend},

  // Group 'Size'

  { PREF_T_INT,   "Resize2SizeShift",     "-1", &YaIPS_ToolData_info[0].Resize2SizeShift},
  { PREF_T_FLOAT,      "Base_Rotate",    "0.0", &YaIPS_ToolData_info[0].Base_Rotate},
  { PREF_T_FLOAT,    "Base_ScalePer",  "100.0", &YaIPS_ToolData_info[0].Base_ScalePer},

  // Group 'Lens'

  { PREF_T_INT,       "Lens_CorrFac",      "0", &YaIPS_ToolData_info[0].Lens_CorrFac},
  { PREF_T_FLOAT,    "Lens_Rotation",    "0.0", &YaIPS_ToolData_info[0].Lens_Rotation},
  { PREF_T_FLOAT, "Lens_SizeCorrPer",    "0.0", &YaIPS_ToolData_info[0].Lens_SizeCorrPer},
  { PREF_T_INT,        "Lens_xDelta",      "0", &YaIPS_ToolData_info[0].Lens_xDelta},
  { PREF_T_INT,        "Lens_yDelta",      "0", &YaIPS_ToolData_info[0].Lens_yDelta},

  // Group 'Trapezoid 1' Trapezoidal distortion with parameters

  { PREF_T_FLOAT,    "Trapezoid_Fac",    "0.0", &YaIPS_ToolData_info[0].Trapezoid_Fac},
  { PREF_T_FLOAT, "Trapezoid_Rotation",  "0.0", &YaIPS_ToolData_info[0].Trapezoid_Rotation},
  { PREF_T_FLOAT, "Trapezoid_SizeCorrPer",  "0.0", &YaIPS_ToolData_info[0].Trapezoid_SizeCorrPer},
  { PREF_T_INT,   "Trapezoid_xDelta",      "0", &YaIPS_ToolData_info[0].Trapezoid_xDelta},
  { PREF_T_INT,   "Trapezoid_yDelta",      "0", &YaIPS_ToolData_info[0].Trapezoid_yDelta},

  // Group 'Trapezoid 2' Trapezoidal distortion with 4 points

  { PREF_T_INT,    "Trapezoid_SP0_x",      "0", &YaIPS_ToolData_info[0].Trapezoid_SrcPoint[ 0].x},
  { PREF_T_INT,    "Trapezoid_SP0_y",      "0", &YaIPS_ToolData_info[0].Trapezoid_SrcPoint[ 0].y},
  { PREF_T_INT,    "Trapezoid_SP1_x",    "100", &YaIPS_ToolData_info[0].Trapezoid_SrcPoint[ 1].x},
  { PREF_T_INT,    "Trapezoid_SP1_y",      "0", &YaIPS_ToolData_info[0].Trapezoid_SrcPoint[ 1].y},
  { PREF_T_INT,    "Trapezoid_SP2_x",    "100", &YaIPS_ToolData_info[0].Trapezoid_SrcPoint[ 2].x},
  { PREF_T_INT,    "Trapezoid_SP2_y",    "100", &YaIPS_ToolData_info[0].Trapezoid_SrcPoint[ 2].y},
  { PREF_T_INT,    "Trapezoid_SP3_x",      "0", &YaIPS_ToolData_info[0].Trapezoid_SrcPoint[ 3].x},
  { PREF_T_INT,    "Trapezoid_SP3_y",    "100", &YaIPS_ToolData_info[0].Trapezoid_SrcPoint[ 3].y},

  { PREF_T_INT,    "Trapezoid_DP0_x",      "0", &YaIPS_ToolData_info[0].Trapezoid_DstPoint[ 0].x},
  { PREF_T_INT,    "Trapezoid_DP0_y",      "0", &YaIPS_ToolData_info[0].Trapezoid_DstPoint[ 0].y},
  { PREF_T_INT,    "Trapezoid_DP1_x",    "100", &YaIPS_ToolData_info[0].Trapezoid_DstPoint[ 1].x},
  { PREF_T_INT,    "Trapezoid_DP1_y",      "0", &YaIPS_ToolData_info[0].Trapezoid_DstPoint[ 1].y},
  { PREF_T_INT,    "Trapezoid_DP2_x",    "100", &YaIPS_ToolData_info[0].Trapezoid_DstPoint[ 2].x},
  { PREF_T_INT,    "Trapezoid_DP2_y",    "100", &YaIPS_ToolData_info[0].Trapezoid_DstPoint[ 2].y},
  { PREF_T_INT,    "Trapezoid_DP3_x",      "0", &YaIPS_ToolData_info[0].Trapezoid_DstPoint[ 3].x},
  { PREF_T_INT,    "Trapezoid_DP3_y",    "100", &YaIPS_ToolData_info[0].Trapezoid_DstPoint[ 3].y},

  // Group 'Parallel projection'

  { PREF_T_FLOAT,  "ParPro_Rotation",    "0.0", &YaIPS_ToolData_info[0].ParPro_Rotation},
  { PREF_T_FLOAT,     "ParPro_xscal",    "1.0", &YaIPS_ToolData_info[0].ParPro_xscal},
  { PREF_T_FLOAT,     "ParPro_yscal",    "1.0", &YaIPS_ToolData_info[0].ParPro_yscal},
  { PREF_T_FLOAT,    "ParPro_xshift",    "0.0", &YaIPS_ToolData_info[0].ParPro_xshift},
  { PREF_T_FLOAT,    "ParPro_yshift",    "0.0", &YaIPS_ToolData_info[0].ParPro_yshift},

  // Group 'Cut out'
  { PREF_T_INT,       "CutOut_AOI_X",      "0", &YaIPS_ToolData_info[0].CutOut_AOI.XPos},
  { PREF_T_INT,       "CutOut_AOI_Y",      "0", &YaIPS_ToolData_info[0].CutOut_AOI.YPos},
  { PREF_T_INT,      "CutOut_AOI_XX",     "30", &YaIPS_ToolData_info[0].CutOut_AOI.XSize},
  { PREF_T_INT,      "CutOut_AOI_YY",     "30", &YaIPS_ToolData_info[0].CutOut_AOI.YSize},
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_GeoTranWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp);

//-----------------------------------------------------------------------------------
// Parameter dialog
//
// This is a modal dialog. Therefore we can use global variables to hold
// info about the data.
//-----------------------------------------------------------------------------------

// defines for geometry transformation

#define YAIPS_GEOTRAN_RESIZE2            0    // Resize by power of 2
#define YAIPS_GEOTRAN_MIRROR_X           1    // Mirror X axis
#define YAIPS_GEOTRAN_MIRROR_Y           2    // Mirror Y axis
#define YAIPS_GEOTRAN_ROTATE_90P         3    // Rotate 90 degrees clockwise
#define YAIPS_GEOTRAN_ROTATE_180         4    // Rotate 180 degrees
#define YAIPS_GEOTRAN_ROTATE_90M         5    // Rotate 90 degrees counterclockwise
#define YAIPS_GEOTRAN_ROTATE             6    // Rotate counterclockwise
#define YAIPS_GEOTRAN_SCALE              7    // Scale image
#define YAIPS_GEOTRAN_LENS               8    // Lens corrections
#define YAIPS_GEOTRAN_PAR_PRO            9    // Parallel projection
#define YAIPS_GEOTRAN_PAR_CUT_OUT       10    // Cut out an image part
#define YAIPS_GEOTRAN_TRAPEZOID_1       11    // Trapezoidal distortion with parameters
#define YAIPS_GEOTRAN_TRAPEZOID_2       12    // Trapezoidal distortion with 4 points

#define YAIPS_GEOTRAN_BUTTON_MAX    (YAIPS_GEOTRAN_TRAPEZOID_2 + 1)  // Number of radio buttons

// ...

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolData;     // NOTE: Is used by all parameter dialog functions

static IqeFl_Tabs      *pTab_Groups;         // Point to tabulator GUI element
static Fl_Radio_Round_Button *GeoTranButtons[ YAIPS_GEOTRAN_BUTTON_MAX]; // Table of buttons
static int GeoTranType_Last;                  // Catch change

// Group 'Trapezoid 2'

static Fl_Button *pT2_TeachToggle;
static IqeFl_Int_Input *pInt_T2_SrcX[ 4], *pInt_T2_SrcY[ 4], *pInt_T2_DstX[ 4], *pInt_T2_DstY[ 4];

// Cut out GUI Elements

static IqeFl_Int_Input *pCO_AOI_X, *pCO_AOI_Y, *pCO_AOI_XX, *pCO_AOI_YY;
static Fl_Button *pCO_TeachToggle;

/************************************************************************************
 * Support for trapezoid points
 *
 */

// Reset trapezoid points
static void TrapezoidPointsReset( YaIPS_XY_int *pPoints, int xx, int yy, int DeltaX, int DeltaY)
{

  pPoints[ 0].x = DeltaX;
  pPoints[ 0].y = DeltaY;

  pPoints[ 1].x = xx - 1 - DeltaX;
  pPoints[ 1].y = DeltaY;

  pPoints[ 2].x = xx - 1 - DeltaX;
  pPoints[ 2].y = yy - 1 - DeltaY;

  pPoints[ 3].x = DeltaX;
  pPoints[ 3].y = yy - 1 - DeltaY;
}

// Check trapezoid points to be inside image
static void TrapezoidPointsCheck( YaIPS_XY_int *pPoints, int xx, int yy)
{
  int i, DoReset;

  DoReset = false;

  for( i = 0; i < 4; i++) {

    if( pPoints[ i].x < 0 || pPoints[ i].x >= xx) {
      DoReset = true;
      break;
    }

    if( pPoints[ i].y < 0 || pPoints[ i].y >= yy) {
      DoReset = true;
      break;
    }
  }

  if( DoReset) {

    TrapezoidPointsReset( pPoints, xx, yy, xx / 3, yy / 3);
  }
}

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{
  int i, ValThis;
  Fl_RGB_Image *pImgIn1;
  int ImgXX, ImgYY, RedrawOnExit;
  Fl_Widget *pCurrFocus;
  Fl_Color TeachColor;

  RedrawOnExit = false;

  pCurrFocus = Fl::focus();

  // Get last selected tab group

  pToolData->Tab_Group_Selected = pTab_Groups->GetTabGroup();

  // Update button
  // Current selection must be set

  // Update all buttons
  if( GeoTranType_Last != pToolData->GeoTranType) {                // Type has change

    GeoTranType_Last = pToolData->GeoTranType;

    for( i = 0; i < YAIPS_GEOTRAN_BUTTON_MAX; i++) {

      ValThis = GeoTranButtons[ i]->value();

      if( ValThis != (i == pToolData->GeoTranType)) {             // Not what we expected

        GeoTranButtons[ i]->value( i == pToolData->GeoTranType);   // Set value
        GeoTranButtons[ i]->redraw();
      }
    }
  }

  // Teach button color

  TeachColor = pToolData->Teach_mode ? FL_GREEN : YAIPS_BCOL_BUTTON;

  // Trapezoid 2: Color teach toggle button

  IqeB_GUI_WidgetActivate( pT2_TeachToggle, pToolData->GeoTranType == YAIPS_GEOTRAN_TRAPEZOID_2);// Only usable for brightness correction

  IqeB_GUI_WidgetLabelColor( pT2_TeachToggle, pToolData->GeoTranType == YAIPS_GEOTRAN_TRAPEZOID_2 ? TeachColor : FL_GRAY);

  // Cut out: Enable AOI pos/size input buttons

  IqeB_GUI_WidgetActivate( pCO_AOI_X, pToolData->GeoTranType == YAIPS_GEOTRAN_PAR_CUT_OUT && pToolData->Teach_mode);
  IqeB_GUI_WidgetActivate( pCO_AOI_Y, pToolData->GeoTranType == YAIPS_GEOTRAN_PAR_CUT_OUT && pToolData->Teach_mode);
  IqeB_GUI_WidgetActivate( pCO_AOI_XX, pToolData->GeoTranType == YAIPS_GEOTRAN_PAR_CUT_OUT && pToolData->Teach_mode);
  IqeB_GUI_WidgetActivate( pCO_AOI_YY, pToolData->GeoTranType == YAIPS_GEOTRAN_PAR_CUT_OUT && pToolData->Teach_mode);

  // Cut out: Color teach toggle button

  IqeB_GUI_WidgetActivate( pCO_TeachToggle, pToolData->GeoTranType == YAIPS_GEOTRAN_PAR_CUT_OUT);// Only usable for brightness correction

  IqeB_GUI_WidgetLabelColor( pCO_TeachToggle, pToolData->GeoTranType == YAIPS_GEOTRAN_PAR_CUT_OUT ? TeachColor : FL_GRAY);

  // Check Scene input

  pImgIn1 = NULL;       // Will be set if there is a valid scene image

  YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Input1_WinIdNr,
                           NULL, &pImgIn1, NULL);

  if( pImgIn1 != NULL) {

    ImgXX = pImgIn1->w();
    ImgYY = pImgIn1->h();

    // Check points to fit in image

    TrapezoidPointsCheck( pToolData->Trapezoid_SrcPoint, ImgXX, ImgYY);
    TrapezoidPointsCheck( pToolData->Trapezoid_DstPoint, ImgXX, ImgYY);

  } else {

    ImgXX = 1024;
    ImgYY = 1024;
  }

  // Check limits for warp points

  for( i = 0; i < 4; i++) {

    IqeFl_Int_Input *pInt_T2;
    YaIPS_XY_int    *pPoint;

    pInt_T2 = pInt_T2_SrcX[ i];
    pPoint  = pToolData->Trapezoid_SrcPoint + i;

    if( pInt_T2->Max != ImgXX - 1) {
      pInt_T2->ChangeMinMax( 0, ImgXX - 1);
    }
    if( pPoint->x != pInt_T2->GetValue()) {
      pInt_T2->SetValue( pPoint->x);
    }

    pInt_T2 = pInt_T2_SrcY[ i];

    if( pInt_T2->Max != ImgYY - 1) {
      pInt_T2->ChangeMinMax( 0, ImgYY - 1);
    }
    if( pPoint->y != pInt_T2->GetValue()) {
      pInt_T2->SetValue( pPoint->y);
    }

    pInt_T2 = pInt_T2_DstX[ i];
    pPoint  = pToolData->Trapezoid_DstPoint + i;

    if( pInt_T2->Max != ImgXX - 1) {
      pInt_T2->ChangeMinMax( 0, ImgXX - 1);
    }
    if( pPoint->x != pInt_T2->GetValue()) {
      pInt_T2->SetValue( pPoint->x);
    }

    pInt_T2 = pInt_T2_DstY[ i];

    if( pInt_T2->Max != ImgYY - 1) {
      pInt_T2->ChangeMinMax( 0, ImgYY - 1);
    }
    if( pPoint->y != pInt_T2->GetValue()) {
      pInt_T2->SetValue( pPoint->y);
    }
  }

  // Check cut out AOI

  if( pCurrFocus != pCO_AOI_X  && pCurrFocus != pCO_AOI_Y &&               // Input element has NO keyboard focus ?
      pCurrFocus != pCO_AOI_XX && pCurrFocus != pCO_AOI_YY) {

    if( YaIPS_ImageDispAoiRectIGuiUpdate( &pToolData->CutOut_AOI, ImgXX, ImgYY,
                                          pCO_AOI_X, pCO_AOI_Y, pCO_AOI_XX, pCO_AOI_YY) > 0) {

      RedrawOnExit = true;                                 // Redraw on exit
    }
  }

  // Redraw

  if( RedrawOnExit) {                                           // Redraw on exit

    if( pToolData->GeoTranType == YAIPS_GEOTRAN_PAR_CUT_OUT) {  // AOI is displayed on image

      if( pToolData->YaIPS_ImageDisp.pImage_Box != NULL) {
        pToolData->YaIPS_ImageDisp.pImage_Box->redraw();
      }

      if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

        if( YaIPS_BigImageDisp.pImage_Box != NULL) {
          YaIPS_BigImageDisp.pImage_Box->redraw();
        }
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
 * YaIPS_GeoTran_Callback
 *
 * Filter will change
 */

static void YaIPS_GeoTran_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                       // get value to set

  if( pToolData->GeoTranType == Value) {            // Value will not change

    return;                                        // Exit, nothing to do
  }

  pToolData->GeoTranType = Value;                   // Set new value

  pToolData->Input1_Change = 0;                    // Force recalculation output
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

  if( w == pCO_TeachToggle || w == pT2_TeachToggle) {   // Toggle Teach / Inspection button

    pToolData->Teach_mode = ! pToolData->Teach_mode;

  } else if( pValueArg == pToolData->Trapezoid_SrcPoint ||      // Reset warp source points
             pValueArg == pToolData->Trapezoid_SrcPoint + 1 ||
             pValueArg == pToolData->Trapezoid_DstPoint ||      // Reset warp destination points
             pValueArg == pToolData->Trapezoid_DstPoint + 1) {

    int ImgXX, ImgYY, DeltaX, DeltaY;
    Fl_RGB_Image *pImgIn1;

    // Check Scene input

    pImgIn1 = NULL;       // Will be set if there is a valid scene image

    YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Input1_WinIdNr,
                             NULL, &pImgIn1, NULL);

    if( pImgIn1 != NULL) {                  // Have an input image

      ImgXX = pImgIn1->w();
      ImgYY = pImgIn1->h();

      if( pValueArg == pToolData->Trapezoid_SrcPoint ||      // Reset warp source points
          pValueArg == pToolData->Trapezoid_DstPoint) {

        DeltaX = 0;
        DeltaY = 0;

      } else {

        DeltaX = ImgXX / 3;
        DeltaY = ImgYY / 3;
      }


      if( pValueArg == pToolData->Trapezoid_SrcPoint ||   // Reset warp source points
          pValueArg == pToolData->Trapezoid_SrcPoint + 1) {


        TrapezoidPointsReset( pToolData->Trapezoid_SrcPoint, ImgXX, ImgYY, DeltaX, DeltaY);

      } else {                                            // Reset warp destination points

        TrapezoidPointsReset( pToolData->Trapezoid_DstPoint, ImgXX, ImgYY, DeltaX, DeltaY);
      }
    }

  } else {

    Fl_Button *pThis;

    pThis  = (Fl_Check_Button *)w;
    *pValue = pThis->value();                // update the variable
  }

  pToolData->Input1_Change = 0;          // Force recalculation output
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

  if( pThis == NULL ||                   // security test
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

  pMyParWin = new Fl_Window( xPos, yPos, 297 /*IQE_GUI_TOOLS_STD_WITDH*/, 166 /* 162 */, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  GeoTranType_Last = -1;         // Reset last filter type

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
  Fl_Button       *pTemp_Button;
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
  // Group 'Base'
  //

  yGroup = y;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LANGDEF_ALL);
  pTemp_Group->tooltip( LangStringLookup( "&GUI_GeoTrans_TabM1a=Settings for all others."));

    y += 8;
    y += 4;

    x1 = 8;
    xx2 = 32;

    pTemp_Button = new Fl_Button( x1, y - 2, xx2 - 2, yy + 4, LangStringLookup( "&GUI_GeoTrans_TabM2=Color outside"));
    pTemp_Button->align( FL_ALIGN_RIGHT);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_GeoTrans_TabM2h="
                           "Color for the area outside an image.\n"
                           "Rotating an image or performing other transformations\n"
                           "creates areas outside the image. These are colored\n"
                           "accordingly."));
    pTemp_Button->color( pToolData->Base_OutsiteColor);
    pTemp_Button->callback( IqeB_GUI_But_Color_Callback, &pToolData->Base_OutsiteColor);

    // Next line

    x1  = 8;
    y += yy + 4;

    xx2 = 220;

    pCheckTemp = new Fl_Check_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabM3=Hide outside area"));
    pCheckTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabM3h="
                         "Hide outside area.\n"
                         "If enabled, the area outside the image is hidden.\n"
                         "Otherwise, the area outside is visible."));
    pCheckTemp->value( pToolData->Base_OutsiteBlend);
    pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->Base_OutsiteBlend);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'Simple'
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_GeoTrans_TabA1=Simple"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_GeoTrans_TabA1a=Simple corrections."));

    y += 8;

    xx2 = 105;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_GeoTrans_TabA2=Power of 2"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabA2a="
                            "Enlargement/reduction by powers of two\n"
                            " > 0: 1 = * 2, 2 = * 4, 3 = * 8, ...\n"
                            "   0: 1:1 copy\n"
                            " < 0: 1 = / 2, 2 = / 4, 3 = / 8, ...\n"
                            "Pixels are enlarged without interpolation."));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_RESIZE2);
    GeoTranButtons[ YAIPS_GEOTRAN_RESIZE2] = pRadioButTemp;

    x1 += xx2;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabA3a=Power of 2"));
    pTemp_Int->SetValue( pToolData->Resize2SizeShift);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Resize2SizeShift);
    pTemp_Int->SetModifyData( -8, 8, 1);

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 95;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_GeoTrans_TabA4=Mirroring"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabA4a=Mirror image."));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_MIRROR_X);
    GeoTranButtons[ YAIPS_GEOTRAN_MIRROR_X] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_GeoTrans_TabA5=Flip"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabA5a=Flip image."));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_MIRROR_Y);
    GeoTranButtons[ YAIPS_GEOTRAN_MIRROR_Y] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_GeoTrans_TabA6=Rot. +90°"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabA6a=Rotate image by +90."));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_ROTATE_90P);
    GeoTranButtons[ YAIPS_GEOTRAN_ROTATE_90P] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_GeoTrans_TabA7=Rot. 180°"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabA7a=Rotate image by 180°."));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_ROTATE_180);
    GeoTranButtons[ YAIPS_GEOTRAN_ROTATE_180] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_GeoTrans_TabA8=Rot. -90°"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabA8a=Rotate image by -90°."));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_ROTATE_90M);
    GeoTranButtons[ YAIPS_GEOTRAN_ROTATE_90M] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 95;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LANGDEF_ROTATION);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabA9a=Rotates the image around its center."));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_ROTATE);
    GeoTranButtons[ YAIPS_GEOTRAN_ROTATE] = pRadioButTemp;

    x1 += xx2;

    xx2 = 50;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabA10a=Rotation angle [°]"));
    pFloatTemp->SetValue( pToolData->Base_Rotate);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->Base_Rotate);
    pFloatTemp->SetModifyData( -360.0, 360.0, 5.0, 0.5, true);

    x1 += xx2;

    x1 += 4;

    xx2 = 86;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_GeoTrans_TabA11=Size [%]"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabA11a=Enlarges the image"));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_SCALE);
    GeoTranButtons[ YAIPS_GEOTRAN_SCALE] = pRadioButTemp;

    x1 += xx2;

    xx2 = 50;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabA12a=Size [%]"));
    pFloatTemp->SetValue( pToolData->Base_ScalePer);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->Base_ScalePer);
    pFloatTemp->SetModifyData( 1.0, 1000.0, 10.0, 1.0);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group lens correction
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_GeoTrans_TabB1=Lens"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB1a=Lens correction."));

    y += 8;

    xx2 = 132;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_GeoTrans_TabB2=Lens correction"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB2a="
                            "Correction of lens distortion and minor\n"
                            "rotation, size, or offset corrections."));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_LENS);
    GeoTranButtons[ YAIPS_GEOTRAN_LENS] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 100;
    xx2 = 50;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabB3=Lens factor"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB3a=Correction of lens distortion [units of 1.0E-10]"));
    pTemp_Int->SetValue(  pToolData->Lens_CorrFac);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Lens_CorrFac);
    pTemp_Int->SetModifyData( -50000, 50000, 100, 10);

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 100;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabB4=Rotation"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB4a=Correction of rotational position [°]"));
    pFloatTemp->SetValue( pToolData->Lens_Rotation);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->Lens_Rotation);
    pFloatTemp->SetModifyData( -180.0, 180.0, 1.0, 0.1, true);

    x1 += 136;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabB5=Size"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB5a=Size correction [%]"));
    pFloatTemp->SetValue( pToolData->Lens_SizeCorrPer);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->Lens_SizeCorrPer);
    pFloatTemp->SetModifyData( -50.0, 20.0, 0.5, 0.1);

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 100;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabB6=Offset X"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Lens_xDelta);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Lens_xDelta);
    pTemp_Int->SetModifyData( -500, 500, 10, 1);

    x1 += 136;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabB7=Offset Y"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB7a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Lens_yDelta);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Lens_yDelta);
    pTemp_Int->SetModifyData( -500, 500, 10, 1);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'Trapezoid 1' Trapezoidal distortion with parameters
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_GeoTrans_TabE1=Tra."));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_GeoTrans_TabE1a=Trapezoidal distortion."));

    y += 8;

    xx2 = 160;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_GeoTrans_TabE2=Trapezoid distortion"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabE2a="
                            "Trapezoid distortion."));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_TRAPEZOID_1);
    GeoTranButtons[ YAIPS_GEOTRAN_TRAPEZOID_1] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 100;
    xx2 = 50;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabE3=Factor"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->SetFormat( "%.3f");
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabE3a=Trapezoid distortion factor"));
    pFloatTemp->SetValue( pToolData->Trapezoid_Fac);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->Trapezoid_Fac);
    pFloatTemp->SetModifyData( -3.0, 3.0, 0.1, 0.01);

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 100;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabE4=Rotation"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabE4a=Correction of rotational position [°]"));
    pFloatTemp->SetValue( pToolData->Trapezoid_Rotation);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->Trapezoid_Rotation);
    pFloatTemp->SetModifyData( -180.0, 180.0, 15.0, 1.0, true);

    x1 += 136;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabE5=Size"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabE5a=Size correction [%]"));
    pFloatTemp->SetValue( pToolData->Trapezoid_SizeCorrPer);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->Trapezoid_SizeCorrPer);
    pFloatTemp->SetModifyData( -10.0, 5.0, 1.0, 0.1);

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 100;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabB6=Offset X"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_xDelta);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_xDelta);
    pTemp_Int->SetModifyData( -500, 500, 10, 1);

    x1 += 136;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabB7=Offset Y"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB7a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_yDelta);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_yDelta);
    pTemp_Int->SetModifyData( -500, 500, 10, 1);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'Trapezoid 2' Trapezoidal distortion with 4 points
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_GeoTrans_TabF1=Warp"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_GeoTrans_TabF1a=Applies a perspective transformation to an image."));

    y += 8;

    xx2 = 220;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_GeoTrans_TabF2=Perspective Transformation"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabF2a="
                            "Applies a perspective transformation to an image.\n"
                            "Edit mode on: Shows input image.\n"
                            "move 4 Points to the corners of an object.\n"
                            "Edit mode off: Shows output image.\n"
                            "arrange the 4 points with perspective\n"
                            "correction applied.\n"
                            "NOTE: The point order must be clockwise."));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_TRAPEZOID_2);
    GeoTranButtons[ YAIPS_GEOTRAN_TRAPEZOID_2] = pRadioButTemp;

    x1 = pMyParWin->w() - 7 - yy;

    pTemp_Button = new Fl_Button( x1, y, yy, yy, "@-2pencil");
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Teach_mode);
    pTemp_Button->tooltip(  LangStringLookup( "&GUI_GeoTrans_TabF3a="
                                              "Toggle edit mode.\n"
                                              "On: place points on input image.\n"
                                              "Off: place points on output image."));
    pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
    pTemp_Button->shortcut( FL_COMMAND+'t');       // Short cut key
    pT2_TeachToggle = pTemp_Button;

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 34;

    pTemp_Box = new Fl_Box( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabF4=Inp."));
    pTemp_Box->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
    pTemp_Box->box( FL_NO_BOX);    //  FL_BORDER_BOX FL_DOWN_FRAME

    x1 += xx2 + 12;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "1");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_SrcPoint[0].x);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_SrcPoint[0].x);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_SrcX[ 0] = pTemp_Int;

    x1 += xx2 + 10;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "/");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_SrcPoint[0].y);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_SrcPoint[0].y);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_SrcY[ 0] = pTemp_Int;

    x1 += xx2 + 20;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "2");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_SrcPoint[1].x);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_SrcPoint[1].x);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_SrcX[ 1] = pTemp_Int;

    x1 += xx2 + 10;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "/");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_SrcPoint[1].y);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_SrcPoint[1].y);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_SrcY[ 1] = pTemp_Int;

    x1 += xx2 + 2;

    xx2 = yy - 2;

    pTemp_Button = new Fl_Button( x1, y + (yy - xx2) / 2, xx2, xx2, "@-1squaref1");
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, pToolData->Trapezoid_SrcPoint);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_GeoTrans_TabF5a=Reset points"));

    x1 += xx2 + 2;

    pTemp_Button = new Fl_Button( x1, y + (yy - xx2) / 2, xx2, xx2, "@-1squaref2");
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, pToolData->Trapezoid_SrcPoint + 1);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_GeoTrans_TabF5a=Reset points"));

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 46;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "4");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_SrcPoint[3].x);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_SrcPoint[3].x);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_SrcX[ 3] = pTemp_Int;

    x1 += xx2 + 10;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "/");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_SrcPoint[3].y);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_SrcPoint[3].y);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_SrcY[ 3] = pTemp_Int;

    x1 += xx2 + 20;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "3");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_SrcPoint[2].x);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_SrcPoint[2].x);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_SrcX[ 2] = pTemp_Int;

    x1 += xx2 + 10;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "/");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_SrcPoint[2].y);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_SrcPoint[2].y);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_SrcY[ 2] = pTemp_Int;

    // Next line

    x1  = 4;
    y += yy + 4;

    y += 4;

    xx2 = 34;

    pTemp_Box = new Fl_Box( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabF6=Out"));
    pTemp_Box->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
    pTemp_Box->box( FL_NO_BOX);    //  FL_BORDER_BOX FL_DOWN_FRAME

    x1 += xx2 + 12;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "1");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_DstPoint[0].x);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_DstPoint[0].x);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_DstX[ 0] = pTemp_Int;

    x1 += xx2 + 10;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "/");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_DstPoint[0].y);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_DstPoint[0].y);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_DstY[ 0] = pTemp_Int;

    x1 += xx2 + 20;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "2");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_DstPoint[1].x);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_DstPoint[1].x);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_DstX[ 1] = pTemp_Int;

    x1 += xx2 + 10;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "/");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_DstPoint[1].y);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_DstPoint[1].y);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_DstY[ 1] = pTemp_Int;

    x1 += xx2 + 2;

    xx2 = yy - 2;

    pTemp_Button = new Fl_Button( x1, y + (yy - xx2) / 2, xx2, xx2, "@-1squaref1");
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, pToolData->Trapezoid_DstPoint);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_GeoTrans_TabF5a=Reset points"));

    x1 += xx2 + 2;

    pTemp_Button = new Fl_Button( x1, y + (yy - xx2) / 2, xx2, xx2, "@-1squaref2");
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, pToolData->Trapezoid_DstPoint + 1);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_GeoTrans_TabF5a=Reset points"));

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 46;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "4");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_DstPoint[3].x);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_DstPoint[3].x);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_DstX[ 3] = pTemp_Int;

    x1 += xx2 + 10;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "/");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_DstPoint[3].y);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_DstPoint[3].y);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_DstY[ 3] = pTemp_Int;

    x1 += xx2 + 20;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "3");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_DstPoint[2].x);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_DstPoint[2].x);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_DstX[ 2] = pTemp_Int;

    x1 += xx2 + 10;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, "/");
    //pTemp_Int->tooltip( LangStringLookup( "&GUI_GeoTrans_TabB6a=Offset correction [pixels]"));
    pTemp_Int->SetValue( pToolData->Trapezoid_DstPoint[2].y);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Trapezoid_DstPoint[2].y);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pInt_T2_DstY[ 2] = pTemp_Int;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group Parallel projection
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_GeoTrans_TabC1=ParPro"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_GeoTrans_TabC1a=Distortions by parallel projection"));

    y += 8;

    xx2 = 132;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_GeoTrans_TabC2=Parallel projection"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabC2a="
                            "Correction of lens distortion and minor\n"
                            "rotation, size, or offset corrections."));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_PAR_PRO);
    GeoTranButtons[ YAIPS_GEOTRAN_PAR_PRO] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 70;

    x1 += 100;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_ROTATION);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabC3a=Rotation [°]"));
    pFloatTemp->SetValue( pToolData->ParPro_Rotation);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->ParPro_Rotation);
    pFloatTemp->SetModifyData( -180.0, 180.0, 5.0, 0.5, true);

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 100;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabC4=Size X"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabC4a=Enlargement X"));
    pFloatTemp->SetFormat( (char *)"%.2f");
    pFloatTemp->SetValue( pToolData->ParPro_xscal);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->ParPro_xscal);
    pFloatTemp->SetModifyData( 0.1, 10.0, 0.1, 0.01);

    x1 += xx2;

    x1 += 20;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_Y);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabC5a=Enlargement Y"));
    pFloatTemp->SetFormat( (char *)"%.2f");
    pFloatTemp->SetValue( pToolData->ParPro_yscal);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->ParPro_yscal);
    pFloatTemp->SetModifyData( 0.1, 10.0, 0.1, 0.01);

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 100;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_GeoTrans_TabC6=Offset X"));
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabC6a=Offset X"));
    pFloatTemp->SetValue( pToolData->ParPro_xshift);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->ParPro_xshift);
    pFloatTemp->SetModifyData( -10000.0, 10000.0, 10.0, 1.0);

    x1 += xx2;

    x1 += 20;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_Y);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabC7a=Offset Y"));
    pFloatTemp->SetValue( pToolData->ParPro_yshift);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->ParPro_yshift);
    pFloatTemp->SetModifyData( -10000.0, 10000.0, 10.0, 1.0);

    x1 += xx2;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group cut out
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_GeoTrans_TabD1=Cut"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_GeoTrans_TabD1a=Cut out an image part"));

    y += 8;

    xx2 = 180;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_GeoTrans_TabD2=Cut out image part"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_GeoTrans_TabD2a="
                                              "Cut out an image part.\n"
                                              "The selected AOI is cut out."));
    pRadioButTemp->callback( YaIPS_GeoTran_Callback, (void *)YAIPS_GEOTRAN_PAR_CUT_OUT);
    GeoTranButtons[ YAIPS_GEOTRAN_PAR_CUT_OUT] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 69;

    xx2 = 44;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_AOI_LEFT);
    pTemp_Int->tooltip( LANGDEF_AOI_LEFT_TOOLTIP);
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->CutOut_AOI.XPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->CutOut_AOI.XPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pCO_AOI_X = pTemp_Int;

    x1 += xx2;
    x1 += 50;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_AOI_TOP);
    pTemp_Int->tooltip( LANGDEF_AOI_TOP_TOOLTIP);
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->CutOut_AOI.YPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->CutOut_AOI.YPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pCO_AOI_Y = pTemp_Int;

    x1 += xx2;
    x1 += 8;

    pTemp_Button = new Fl_Button( x1, y, 28, 28, "@+1pencil");
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Teach_mode);
    pTemp_Button->tooltip( LANGDEF_AOI_TEACH_TOOLTIP);
    pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
    pTemp_Button->shortcut( FL_COMMAND+'t');       // Short cut key
    pCO_TeachToggle = pTemp_Button;

    // Next line

    x1  = 4;
    y += yy;

    x1 += 69;

    xx2 = 44;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_AOI_WIDTH);
    pTemp_Int->tooltip( LANGDEF_AOI_WIDTH_TOOLTIP);
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->CutOut_AOI.XSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->CutOut_AOI.XSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pCO_AOI_XX = pTemp_Int;

    x1 += xx2;
    x1 += 50;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_AOI_HEIGHT);
    pTemp_Int->tooltip( LANGDEF_AOI_HEIGHT_TOOLTIP);
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->CutOut_AOI.YSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->CutOut_AOI.YSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pCO_AOI_YY = pTemp_Int;

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
  Fl_Button *pGUI_TeachToggle;           // Toggle Teach / Inspection

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
    pToolData->YaIPS_ImageDisp.pImage_Box->MouseCallbackFlag   = YaIPS_MOUSE_CB_FLAG_ALSO_DISABLED; // Also call mouse callback function for disabled 'MouseTeachState'.
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

    if( pToolData->GeoTranType == YAIPS_GEOTRAN_PAR_CUT_OUT || // Only usable for cut out
        pToolData->GeoTranType == YAIPS_GEOTRAN_TRAPEZOID_2) { // or trapezoid 2

      pToolData->Teach_mode = ! pToolData->Teach_mode;

      pToolData->Input1_Change = 0;          // Force recalculation output
    }
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

  // Update color of Toggle Teach / Inspection button

  int MouseTeachState;

  MouseTeachState = 1;                                            // We have a mouse callback state

  // Is teach mode available
  if( DoEnable &&                                                 // Enable GUI elements
      (pToolData->GeoTranType == YAIPS_GEOTRAN_PAR_CUT_OUT ||     // Only usable for cut out
       pToolData->GeoTranType == YAIPS_GEOTRAN_TRAPEZOID_2)) {    // or trapezoid 2

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

      switch( pToolData->GeoTranType) {

      default:

        // Should not happen, simply copy image
        ierr = YaIPS_RGB_CopyImg( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1);
        break;

      case YAIPS_GEOTRAN_RESIZE2:

        ierr = YaIPS_RGB_Geo_Resize2( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pToolData->Resize2SizeShift);
        break;

      case YAIPS_GEOTRAN_MIRROR_X:

        ierr = YaIPS_RGB_Geo_Mirror( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, 0);
        break;

      case YAIPS_GEOTRAN_MIRROR_Y:

        ierr = YaIPS_RGB_Geo_Mirror( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, 1);
        break;

      case YAIPS_GEOTRAN_ROTATE_90P:

        ierr = YaIPS_RGB_Geo_Rotate90( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, 1);
        break;

      case YAIPS_GEOTRAN_ROTATE_180:

        ierr = YaIPS_RGB_Geo_Rotate90( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, 2);
        break;

      case YAIPS_GEOTRAN_ROTATE_90M:

        ierr = YaIPS_RGB_Geo_Rotate90( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, 3);
        break;

      case YAIPS_GEOTRAN_ROTATE:

        ierr = YaIPS_RGB_Geo_Rotate( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                     pToolData->Base_OutsiteColor, pToolData->Base_OutsiteBlend, pToolData->Base_Rotate);
        break;

      case YAIPS_GEOTRAN_SCALE:
        {
          int xxDst, yyDst;

          // Calculate size of image
          xxDst = (int)(pImgIn1->w() * pToolData->Base_ScalePer * 0.01);
          yyDst = (int)(pImgIn1->h() * pToolData->Base_ScalePer * 0.01);

          ierr = YaIPS_RGB_Geo_Transform( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                          pToolData->Base_OutsiteColor, pToolData->Base_OutsiteBlend,
                                          0.0,
                                          pToolData->Base_ScalePer * 0.01, pToolData->Base_ScalePer * 0.01, 0.0, 0.0, xxDst, yyDst);

        }
        break;

      case YAIPS_GEOTRAN_LENS:

        ierr = YaIPS_RGB_Geo_Lens( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                   pToolData->Base_OutsiteColor, pToolData->Base_OutsiteBlend,
                                   pToolData->Lens_CorrFac, pToolData->Lens_Rotation,
                                   pToolData->Lens_SizeCorrPer, pToolData->Lens_xDelta, pToolData->Lens_yDelta);
        break;

      case YAIPS_GEOTRAN_TRAPEZOID_1:
        {
          YaIPS_XY_float SrcPoints[ 4], DstPoints[ 4];
          int xx, yy, iPoint;
          double PX1, PY1, PX2, PY2, PXD, PYD, Trapezoid_Fac, Angle, SinAngle, CosAngle;

          xx = pImgIn1->data_w();
          yy = pImgIn1->data_h();

          // Source points

          // Prepare trapezoidal distortion

          Trapezoid_Fac = pToolData->Trapezoid_Fac;

          if( Trapezoid_Fac > 3.0) {            // Clip

            Trapezoid_Fac = 3.0;

          } else if( Trapezoid_Fac < -3.0) {

            Trapezoid_Fac = -3.0;
          }

          // Prepare rotation

          Angle = pToolData->Trapezoid_Rotation * M_PI / 180.0; // convert rotation from degree to radiant
          Angle = -1.0 * Angle;                                 // change direction, we work backwards

          SinAngle = sin( Angle);
          CosAngle = cos( Angle);

          for( iPoint = 0; iPoint < 4; iPoint++) {

            // Corner points

            switch( iPoint) {

            default:
            case 0:
              PX1 = xx * -0.5;
              PY1 = yy * -0.5;
              break;

            case 1:
              PX1 = xx * 0.5;
              PY1 = yy * -0.5;
              break;

            case 2:
              PX1 = xx * 0.5;
              PY1 = yy * 0.5;
              break;

            case 3:
              PX1 = xx * -0.5;
              PY1 = yy * 0.5;
              break;
            }

            // Remember for destination point

            PXD = PX1;
            PYD = PY1;

            // Trapezoid for source point

            if( Trapezoid_Fac >= 0.0) {

              if( iPoint == 0 || iPoint == 1)

                PX1 = PX1 * (1.0 + Trapezoid_Fac);
            }  else {

              if( iPoint == 2 || iPoint == 3) {

                PX1 = PX1 * (1.0 - Trapezoid_Fac);
              }
            }

            // Rotation

            PX2 = PX1 * CosAngle - PY1 * SinAngle;
            PY2 = PY1 * CosAngle + PX1 * SinAngle;

            // Size correction

            PX2 *= 1.0 - pToolData->Trapezoid_SizeCorrPer * 0.1;
            PY2 *= 1.0 - pToolData->Trapezoid_SizeCorrPer * 0.1;

            // Store point

            SrcPoints[ iPoint].x = PX2 + xx * 0.5 + pToolData->Trapezoid_xDelta;
            SrcPoints[ iPoint].y = PY2 + yy * 0.5 + pToolData->Trapezoid_yDelta;

            // Continue with destination point

            PX1 = PXD;
            PY1 = PYD;

            // Rotation

            PX2 = PX1 * CosAngle - PY1 * SinAngle;
            PY2 = PY1 * CosAngle + PX1 * SinAngle;

            // Store point

            DstPoints[ iPoint].x = PX2 + xx * 0.5 + pToolData->Trapezoid_xDelta;
            DstPoints[ iPoint].y = PY2 + yy * 0.5 + pToolData->Trapezoid_yDelta;
          }

          // Warp 4 points in source to 4 points in destination.
          ierr = YaIPS_RGB_Geo_Warp_4_Points( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                              pToolData->Base_OutsiteColor, pToolData->Base_OutsiteBlend,
                                              SrcPoints,       // Point to 4 source points x + y
                                              DstPoints,       // Point to 4 destination points x + y
                                              xx, yy);         // Destination height if > 0 else compute from pDstPoints

        }
        break;

      case YAIPS_GEOTRAN_TRAPEZOID_2:

        if( pToolData->Teach_mode)  {    // Teach mode

          // Copy image. Also shows points on source image

          ierr = YaIPS_RGB_CopyImg( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1);

        } else {

          YaIPS_XY_float SrcPoints[ 4], DstPoints[ 4];
          int xx, yy, iPoint, x, y;

          xx = pImgIn1->data_w();
          yy = pImgIn1->data_h();

          TrapezoidPointsCheck( pToolData->Trapezoid_SrcPoint, xx, yy);
          TrapezoidPointsCheck( pToolData->Trapezoid_DstPoint, xx, yy);

          for( iPoint = 0; iPoint < 4; iPoint++) {

            // Convert source points

            x = pToolData->Trapezoid_SrcPoint[ iPoint].x;
            y = pToolData->Trapezoid_SrcPoint[ iPoint].y;

            if( x >= xx) {
              x = xx - 1;
            }
            if( x < 0) {
              x = 0;
            }

            if( y >= yy) {
              y = yy - 1;
            }
            if( y < 0) {
              y = 0;
            }

            SrcPoints[ iPoint].x = x + 0.5;
            SrcPoints[ iPoint].y = y + 0.5;

            // Convert destination points

            x = pToolData->Trapezoid_DstPoint[ iPoint].x;
            y = pToolData->Trapezoid_DstPoint[ iPoint].y;

            if( x >= xx) {
              x = xx - 1;
            }
            if( x < 0) {
              x = 0;
            }

            if( y >= yy) {
              y = yy - 1;
            }
            if( y < 0) {
              y = 0;
            }

            DstPoints[ iPoint].x = x + 0.5;
            DstPoints[ iPoint].y = y + 0.5;
          }

          // Warp 4 points in source to 4 points in destination.
          ierr = YaIPS_RGB_Geo_Warp_4_Points( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                              pToolData->Base_OutsiteColor, pToolData->Base_OutsiteBlend,
                                              SrcPoints,       // Point to 4 source points x + y
                                              DstPoints,       // Point to 4 destination points x + y
                                              xx, yy);         // Destination height if > 0 else compute from pDstPoints

        }
        break;

      case YAIPS_GEOTRAN_PAR_PRO:

        ierr = YaIPS_RGB_Geo_Transform( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                        pToolData->Base_OutsiteColor, pToolData->Base_OutsiteBlend,
                                        pToolData->ParPro_Rotation,
                                        pToolData->ParPro_xscal, pToolData->ParPro_yscal,
                                        pToolData->ParPro_xshift, pToolData->ParPro_yshift);
        break;

      case YAIPS_GEOTRAN_PAR_CUT_OUT:

        if( pToolData->Teach_mode)  {    // Teach mode

          // Copy image. Also show AOI rectangle.
          ierr = YaIPS_RGB_CopyImg( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1);

        } else {

          // Cut out a rectangular image part
          ierr = YaIPS_RGB_CutOut( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, &pToolData->CutOut_AOI);
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
 * IqeB_GUI_GeoTranWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_GeoTranWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
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

  pToolData->iToolData = iToolData;                        // Set sub window number

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

  // Draw AOI for trapezoid 2

  if( pToolData->GeoTranType == YAIPS_GEOTRAN_TRAPEZOID_2 &&   // Trapezoid selected ?
      pYaIPS_ImageDisp->pImage_Img != NULL) {                  // Have a source image

    int SrcXX, SrcYY, Radius, LineWidth, iPoint;
    int x1, y1, x, y, xx, yy, OffX, OffY, DrawText;
    YaIPS_XY_int Point[ 4];
    char TempString[ 256];


    // Preparations

    x1 = pYaIPS_ImageDisp->BigImage_sx;
    y1 = pYaIPS_ImageDisp->BigImage_sy;
    xx = pYaIPS_ImageDisp->BigImage_sw;
    yy = pYaIPS_ImageDisp->BigImage_sh;

    // Points relative to image

    OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
    OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

    LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

    // Clipping ?

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( x1, y1, xx, yy);
    }

    // Prepare font size

    DrawText = false;                                            // Preset, do not draw text

    if( pYaIPS_ImageDisp->PixelImageToScreen >= 0.33) {              // Is NOT to tiny

      int TempFontSize;

      DrawText = true;                                           // Draw text

      if( pYaIPS_ImageDisp->PixelImageToScreen > 1.0) {
        TempFontSize = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 16.0 + 0.5);
      } else {
        TempFontSize = (int)(16.0 + 0.5);
      }

      if( TempFontSize < 10) {
        TempFontSize = 10;
      }

      fl_font( FL_HELVETICA, TempFontSize);
    }

    SrcXX  = pYaIPS_ImageDisp->pImage_Img->w();
    SrcYY  = pYaIPS_ImageDisp->pImage_Img->h();
    Radius = YAIPS_AOI_FRAME_DIST;
    if( pYaIPS_ImageDisp->PixelImageToScreen > 1.0) {
      Radius = (int)( Radius * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    }

    // Get points

    for( iPoint = 0; iPoint < 4; iPoint++) {

      // Get points

      if( pToolData->Teach_mode) {

        x = pToolData->Trapezoid_SrcPoint[ iPoint].x;
        y = pToolData->Trapezoid_SrcPoint[ iPoint].y;

      } else {

        x = pToolData->Trapezoid_DstPoint[ iPoint].x;
        y = pToolData->Trapezoid_DstPoint[ iPoint].y;
      }

      // Clip Points to image

      if( x >= SrcXX) {
        x = SrcXX - 1;
      }
      if( x < 0) {
        x = 0;
      }

      if( y >= SrcYY) {
        y = SrcYY - 1;
      }
      if( y < 0) {
        y = 0;
      }

      // Convert to image space

      x = (int)( (x - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
      y = (int)( (y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

      Point[ iPoint].x = x;
      Point[ iPoint].y = y;
    }

    // Connect the 4 points by lines

    fl_line_style( 0, LineWidth);   // Set line width

    fl_color( FL_CYAN);

    for( iPoint = 0; iPoint < 4; iPoint++) {

      fl_line( x1 + Point[ iPoint].x, y1 + Point[ iPoint].y,
               x1 + Point[ (iPoint + 1) & 0x03].x, y1 + Point[ (iPoint + 1) & 0x03].y);
    }

    // Draw circles around points

    for( iPoint = 0; iPoint < 4; iPoint++) {

      x = Point[ iPoint].x;
      y = Point[ iPoint].y;

      // Draw Points

      fl_line_style( 0, LineWidth);   // Set line width

      int IsSelected;

      IsSelected = (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0 &&  // Mouse is over any AOI
                   pYaIPS_ImageDisp->AoiIdNr == iPoint;                                // and mouse is over this AOI

      // Draw color for AOI
      if( IsSelected) {  // Mouse is over point

        fl_color( FL_RED);
      } else {

        fl_color( FL_GREEN - 2);
      }

      fl_begin_loop();

      fl_circle( x1 + x, y1 + y, Radius);

      fl_end_loop();

      // Draw label and quality of correlation to screen

      if( DrawText) {                                                 // Draw text

        int mdx, mdy, mw, mh;

        sprintf( TempString, "%d", iPoint + 1);

        fl_text_extents( TempString, mdx, mdy, mw, mh);

#ifdef use_again

        x += Radius;

        if( y + mdy < 4) {         // To near to upper border

          y += mh + 5;             // Show below upper frame
          x += 4;

        } else {                   // Fits above upper frame

          y -= 4;
        }
#else
        if( x + mw + Radius >= xx) {

          x = x - Radius - mw - 2;

        } else {

          x = x + Radius;
        }

        if( y + mdy < 4) {         // To near to upper border

          y = y + mh + Radius + 2;

        } else {

          y = y - Radius - 2;
        }
#endif

        fl_draw( TempString, x1 + x, y1 + y);
      }
    }

    goto ExitPoint;
  }

  // Draw AOI for cut out ?

  if( pToolData->GeoTranType == YAIPS_GEOTRAN_PAR_CUT_OUT &&   // Cut out selected ?
      pToolData->Teach_mode &&                                 // Change AOI active ?
      pYaIPS_ImageDisp->pImage_Img != NULL) {                  // Have a source image

    int AOI_XX, AOI_YY, LineWidth;
    int x1, y1, x, y, xx, yy, xxo, yyo, OffX, OffY, DrawText;
    char TempString[ 256];

    // Preparations

    x1 = pYaIPS_ImageDisp->BigImage_sx;
    y1 = pYaIPS_ImageDisp->BigImage_sy;
    xx = pYaIPS_ImageDisp->BigImage_sw;
    yy = pYaIPS_ImageDisp->BigImage_sh;

    // Points relative to image

    OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
    OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

    LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

    // Ensure AOI is in image.
    // This helps if the input image is resized smaller than the AOI.
    YaIPS_ImageDispAoiRectClip( pYaIPS_ImageDisp, &pToolData->CutOut_AOI);

    // Get AOI

    if( pToolData->CutOut_AOI.XSize >= pYaIPS_ImageDisp->pImage_Img->w()) {

      AOI_XX = pYaIPS_ImageDisp->pImage_Img->w();
    } else {
      AOI_XX = pToolData->CutOut_AOI.XSize;
    }

    if( pToolData->CutOut_AOI.YSize >= pYaIPS_ImageDisp->pImage_Img->h()) {

      AOI_YY = pYaIPS_ImageDisp->pImage_Img->h();
    } else {
      AOI_YY = pToolData->CutOut_AOI.YSize;
    }

    // Clipping ?

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( x1, y1, xx, yy);
    }

    // Prepare font size

    DrawText = false;                                            // Preset, do not draw text

    if( pYaIPS_ImageDisp->PixelImageToScreen >= 0.33) {              // Is NOT to tiny

      int TempFontSize;

      DrawText = true;                                           // Draw text

      TempFontSize = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 16.0 + 0.5);

      if( TempFontSize < 10) {
        TempFontSize = 10;
      }

      fl_font( FL_HELVETICA, TempFontSize);
    }

    // Draw AOI

    fl_line_style( 0, LineWidth);   // Set line width

    // Draw color for AOI
    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0) {  // Mouse is over the AOI

      fl_color( FL_RED);
    } else {

      fl_color( FL_GREEN - 2);
    }

    x = (int)( (pToolData->CutOut_AOI.XPos - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    y = (int)( (pToolData->CutOut_AOI.YPos - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

    xxo = (int)( AOI_XX * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    yyo = (int)( AOI_YY * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

    fl_rect( x1 + x, y1 + y, xxo, yyo);

    // Draw label and quality of correlation to screen

    if( DrawText) {                                                 // Draw text

      int mdx, mdy, mw, mh;

      sprintf( TempString, "%d/%d %dx%d", pToolData->CutOut_AOI.XPos, pToolData->CutOut_AOI.YPos, pToolData->CutOut_AOI.XSize, pToolData->CutOut_AOI.YSize);

      fl_text_extents( TempString, mdx, mdy, mw, mh);

      if( y + mdy < 4) {         // To near to upper border

        y += mh + 5;             // Show below upper frame
        x += 4;

      } else {                   // Fits above upper frame

        y -= 4;
      }

      fl_draw( TempString, x1 + x, y1 + y);
    }

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
 * Return: < 0  Error, don't processed mouse callback
 *           0 OK, processed mouse callback
 *
 */

static int YaIPS_GUI_MyMouse_cb( Fl_Widget *pW, int event,
                                 void *pArg1,        // Pointer to Fl_YaIPS_ImageDisp_t
                                 void *pArg2)        // Optional pointer to ToolData
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  int ierr, x, y, AoiIdNrEntry;
  int minAoiDist, CursorShapeTest, AoiDeltaAddTest, IsBigImageDisp;
  YaIPS_ToolData_info_t *pToolData;
  static int Last_x = -9999, Last_y = -9999;           // Must be static
  static int Pressed_x, Pressed_y;                     // Used for move with pressed mouse button
  Fl_YaIPS_AOI_t *pAOI_Best, *pAOI_This;
  static Fl_YaIPS_AOI_t Pressed_AOI;                   // AOI on press of mouse button
  static Fl_YaIPS_AOI_t *pPressed_AOI_Best;            // What AOI to modify
  YaIPS_XY_int *pPoint_Best, *pPoint_This;
  static YaIPS_XY_int Pressed_Point;                   // Point on press of mouse button
  static YaIPS_XY_int *pPressed_Point_Best;            // What Point to modify

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
  pPoint_Best    = NULL;                              // Modify this point
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

      // Support for trapezoid 2
      if( pToolData->GeoTranType == YAIPS_GEOTRAN_TRAPEZOID_2) {              // Need an AOI

        int iPoint, Radius;

        Radius = YAIPS_AOI_FRAME_DIST;
        if( pYaIPS_ImageDisp->PixelImageToScreen < 1.0) {
          Radius = (int)( Radius / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
        }

        AoiIdNrEntry = pYaIPS_ImageDisp->AoiIdNr;

        for( iPoint = 0; iPoint < 4; iPoint++) {

          // Get points

          if( pToolData->Teach_mode) {

            pPoint_This = &pToolData->Trapezoid_SrcPoint[ iPoint];

          } else {

            pPoint_This = &pToolData->Trapezoid_DstPoint[ iPoint];

          }

          ierr = YaIPS_ImageDispAoiPointCC( pYaIPS_ImageDisp, &pPoint_This->x, &pPoint_This->y, Radius,
                                            pYaIPS_ImageDisp == &YaIPS_BigImageDisp ? FL_CURSOR_ARROW : FL_CURSOR_CROSS,
                                            &minAoiDist, &CursorShapeTest, &AoiDeltaAddTest);

          if( ierr == true) {     // Got one (or a better one)

            // nearer aoi found
            pYaIPS_ImageDisp->CursorShape = CursorShapeTest;
            pYaIPS_ImageDisp->AoiDeltaAdd = AoiDeltaAddTest;
            pYaIPS_ImageDisp->AoiIdNr = iPoint;           // AOI selected
            pPoint_Best   = pPoint_This;
          }
        }

        if( pYaIPS_ImageDisp->mouseleft) {                    // Left mouse button pressed

          if( pYaIPS_ImageDisp->AoiDeltaAdd != 0 && pYaIPS_ImageDisp->Latched_AoiDeltaAdd == 0) {   // Latch AOI mouse modification

            pYaIPS_ImageDisp->Latched_AoiDeltaAdd = pYaIPS_ImageDisp->AoiDeltaAdd;
            pYaIPS_ImageDisp->Latched_CursorShape = pYaIPS_ImageDisp->CursorShape;

            pPressed_Point_Best = pPoint_Best;                // Modify this point
            Pressed_x = Last_x + pYaIPS_ImageDisp->Delta_x;   // Latch position at button press
            Pressed_y = Last_y + pYaIPS_ImageDisp->Delta_y;

            memcpy( &Pressed_Point, pPoint_Best, sizeof( YaIPS_XY_int)); // Remember AOI data a button press
          }

          if( pYaIPS_ImageDisp->Latched_AoiDeltaAdd != 0) {        // Have latched AOI mouse modification

            pYaIPS_ImageDisp->AoiDeltaAdd = pYaIPS_ImageDisp->Latched_AoiDeltaAdd;   // Use it
            pYaIPS_ImageDisp->CursorShape = pYaIPS_ImageDisp->Latched_CursorShape;
            pPoint_Best   = pPressed_Point_Best;
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

          memcpy( pPoint_Best, &Pressed_Point, sizeof( YaIPS_XY_int)); // Restore AOI data from button press

          pYaIPS_ImageDisp->Delta_x = dto32( (Last_x - Pressed_x) / pYaIPS_ImageDisp->PixelImageToScreen);
          pYaIPS_ImageDisp->Delta_y = dto32( (Last_y - Pressed_y) / pYaIPS_ImageDisp->PixelImageToScreen);

          YaIPS_ImageDispAoiPointDeltaAdd( pYaIPS_ImageDisp, &pPoint_Best->x, &pPoint_Best->y,
                                           pYaIPS_ImageDisp->Delta_x, pYaIPS_ImageDisp->Delta_y);

          pToolData->Input1_Change = 0;                                 // Force recalculation output

          pYaIPS_ImageDisp->RedrawOnExit   = true;                      // Set redraw on exit
          pYaIPS_ImageDisp->BigImageUpdate = pYaIPS_ImageDisp->MyWinID; // Update big image

          pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;    // Set AOI changed flag bit

          break;
        }
      }

      // Support for cut out
      if( pToolData->GeoTranType == YAIPS_GEOTRAN_PAR_CUT_OUT &&              // Need an AOI
          pToolData->Teach_mode != 0) {                                       // and can be changed with the mouse

        pAOI_This = &pToolData->CutOut_AOI;

        ierr = YaIPS_ImageDispAoiRectCC( pYaIPS_ImageDisp, pAOI_This, &minAoiDist, &CursorShapeTest, &AoiDeltaAddTest);

        if( ierr == true) {     // Got one (or a better one)

          // nearer aoi found
          pYaIPS_ImageDisp->CursorShape = CursorShapeTest;
          pYaIPS_ImageDisp->AoiDeltaAdd = AoiDeltaAddTest;
          pAOI_Best   = pAOI_This;
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

          pYaIPS_ImageDisp->RedrawOnExit   = true;                      // Set redraw on exit
          pYaIPS_ImageDisp->BigImageUpdate = pYaIPS_ImageDisp->MyWinID; // Update big image

          pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;    // Set AOI changed flag bit

          break;
        }
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
 * IqeB_GUI_GeoTranWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_GeoTranWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

    // Register draw after function for big image display
    YaIPS_ToolWinDrawAfterSet( MY_WIN_ID + SubWinIDx, YaIPS_GUI_MyDrawAfter_Other);

	  IqeB_GUI_GeoTranWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  IqeB_GUI_GeoTranWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


