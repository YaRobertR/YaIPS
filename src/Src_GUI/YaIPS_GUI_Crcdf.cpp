/****************************************************************************

  YaIPS_GUI_Crcdf.cpp

  Cross difference function of images

  26.05.2025 RR: First edition of this file.

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
#define MY_WIN_ID     YAIPS_WIN_ID_CRCDF        // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_CRCDF         // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_Crcdf_Title=Correlation"          // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinCrcdf"              // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_Crcdf_Tool    // Use this as class name for the window class

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

  // Intermediate processing resources
  Fl_RGB_Image *pCorrImage;             // Temporary correlation image
  T_YaIPS_Res_Crcdf Res_Crcdf;          // Temporary best correlation result
  Tvector *pObjects;                    // If != NULL, objects in scene
  T_YaIPS_Crcdf_ObjOpt ObjOpt;          // Object options

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int Tab_Group_Selected;               // Number of last selected tab group.

  // Tab group 'General'

  int OutputImage;                      // Show output image. 0 = show scene image, 1 = show correlation image
  int ColorChannel;                     // Use this color channel for color images (0 = R, 1 = G, 2 = B);

  int AOI_Teach;                        // If set, show AOI on scene image
  Fl_YaIPS_AOI_t M_AOI;                 // Measurement AOI

  // Tab group 'Objects in scene'

  int Obj_processing;                   // Isolate objects in scene image and correlate each of this objects.
  int Obj_BinThres;                     // Binarization threshold
  int Obj_BinMode;                      // Binarization mode, 0: objects >= Thres, 1: code if < Thres
  int Obj_AreaMinSide;                  // Minimum area of ​​an object to be processed, side length of square
  int Obj_AreaMaxSide;                  // Maximum area of ​​an object to be processed, side length of square
  int Obj_QualityThres;                 // Quality threshold

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

  { PREF_T_INT,   "Group_Selected",     "0", &YaIPS_ToolData_info[0].Tab_Group_Selected},

  // Tab group 'General'

  { PREF_T_INT,      "OutputImage",        "0", &YaIPS_ToolData_info[0].OutputImage},
  { PREF_T_INT,     "ColorChannel",        "0", &YaIPS_ToolData_info[0].ColorChannel},
  { PREF_T_INT,        "AOI_Teach",        "0", &YaIPS_ToolData_info[0].AOI_Teach},
  { PREF_T_INT,            "AOI_X",        "0", &YaIPS_ToolData_info[0].M_AOI.XPos},
  { PREF_T_INT,            "AOI_Y",        "0", &YaIPS_ToolData_info[0].M_AOI.YPos},
  { PREF_T_INT,           "AOI_XX",      "512", &YaIPS_ToolData_info[0].M_AOI.XSize},
  { PREF_T_INT,           "AOI_YY",      "512", &YaIPS_ToolData_info[0].M_AOI.YSize},
  { PREF_T_INT,   "Obj_processing",        "0", &YaIPS_ToolData_info[0].Obj_processing},
  { PREF_T_INT,     "Obj_BinThres",      "128", &YaIPS_ToolData_info[0].Obj_BinThres},
  { PREF_T_INT,      "Obj_BinMode",        "0", &YaIPS_ToolData_info[0].Obj_BinMode},
  { PREF_T_INT,  "Obj_AreaMinSide",        "5", &YaIPS_ToolData_info[0].Obj_AreaMinSide},
  { PREF_T_INT,  "Obj_AreaMaxSide",       "50", &YaIPS_ToolData_info[0].Obj_AreaMaxSide},
  { PREF_T_INT, "Obj_QualityThres",       "70", &YaIPS_ToolData_info[0].Obj_QualityThres},
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_CrcdfWin, (Fl_Callback *)close_cb,
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

static IqeFl_Tabs      *pTab_Groups;         // Point to tabulator GUI element
static IqeFl_Int_Input *pAOI_X, *pAOI_Y, *pAOI_XX, *pAOI_YY;
static Fl_Button       *pTeachToggle;

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{
  int ImgXX, ImgYY, RedrawOnExit;
  Fl_RGB_Image *pImgIn1;
  Fl_Widget *pCurrFocus;

  RedrawOnExit = false;

  pCurrFocus = Fl::focus();

  // Get last selected tab group

  pToolData->Tab_Group_Selected = pTab_Groups->GetTabGroup();

  // Enables for AOI

  IqeB_GUI_WidgetActivate( pAOI_X, pToolData->AOI_Teach);         // Enable GUI elements
  IqeB_GUI_WidgetActivate( pAOI_Y, pToolData->AOI_Teach);         // Enable GUI elements
  IqeB_GUI_WidgetActivate( pAOI_XX, pToolData->AOI_Teach);        // Enable GUI elements
  IqeB_GUI_WidgetActivate( pAOI_YY, pToolData->AOI_Teach);        // Enable GUI elements

  // Color teach toggle button

  IqeB_GUI_WidgetLabelColor( pTeachToggle, pToolData->AOI_Teach ? FL_GREEN : YAIPS_BCOL_BUTTON);

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

  if( pCurrFocus != pAOI_X  && pCurrFocus != pAOI_Y &&               // Input element has NO keyboard focus ?
      pCurrFocus != pAOI_XX && pCurrFocus != pAOI_YY) {

    if( YaIPS_ImageDispAoiRectIGuiUpdate( &pToolData->M_AOI, ImgXX, ImgYY,
                                          pAOI_X, pAOI_Y, pAOI_XX, pAOI_YY) > 0) {

      RedrawOnExit = true;                                 // Redraw on exit
    }
  }

  // Redraw

  if( RedrawOnExit) {                                          // Redraw on exit

    if( pToolData->AOI_Teach) {                                // AOI is displayed on image

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
 * YaIPS_OutputImage_Callback
 *
 * Output image setting will change
 */

static void YaIPS_OutputImage_Callback( Fl_Widget *w, void *data)
{
  int Value, Input1_Check, iToolData;
  Fl_RGB_Image *pImgIn1;

  // ...

  Value = (long long)(data);                       // get value to set

  if( pToolData->OutputImage == Value) {           // Value will not change

    return;                                        // Exit, nothing to do
  }

  pToolData->OutputImage = Value;                  // Set new value

  // Try to redisplay changed output without new image correlation

  iToolData = pToolData->iToolData;                // Get sub window number

  // Check input image and visualize state
  Input1_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr,
                                         NULL, &pImgIn1, NULL);


  if( pToolData->Res_Crcdf.Valid &&                // Test for a valid correlation result
      Input1_Check == 0) {                         // and a valid scene image

    // Redisplay output image

    if( pToolData->OutputImage == 1) {             // Show correlation image

      // Convert the correlation image to a displayable format
      YaIPS_RGB_Crcdf_Conv2BW( &pToolData->YaIPS_ImageDisp.pImage_Img, pToolData->pCorrImage, &pToolData->Res_Crcdf);

    } else {                                       // Show scene input image

      // Copy scene image to output image
      YaIPS_RGB_CopyImg( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1);
    }

    // Check an image display for size change and redisplay if size has changed.
    YaIPS_ImageDispDrawUpdate( &pToolData->YaIPS_ImageDisp, true);

    if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {   // and display this on the big image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                       MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName);   // Load the image to the display
    }
  } else {                                         // else for recalculation

    pToolData->Input1_Change = 0;                  // Force recalculation output
    pToolData->Input2_Change = 0;                  // Force recalculation output
    pToolData->Res_Crcdf.Valid = false;            // Correlation results are NOT valid
  }
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

  if( pToolData->ColorChannel == Value) {        // Value will not change

    return;                                      // Exit, nothing to do
  }

  pToolData->ColorChannel = Value;               // Set new value

  pToolData->Input1_Change = 0;                  // Force recalculation output
  pToolData->Input2_Change = 0;                  // Force recalculation output
  pToolData->Res_Crcdf.Valid = false;            // Correlation results are NOT valid
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

  if( pToolData->Obj_processing) {               // Object processing is on

    // Force recalculation of processing

    pToolData->Input1_Change = 0;                // Force recalculation output
    pToolData->Input2_Change = 0;                // Force recalculation output
    pToolData->Res_Crcdf.Valid = false;          // Correlation results are NOT valid
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
  pToolData->Input2_Change = 0;                    // Force recalculation output
  pToolData->Res_Crcdf.Valid = false;              // Correlation results are NOT valid
#endif

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


  if( ! pToolData->Obj_processing ||       // Object processing is off
      pValue == &pToolData->M_AOI.XPos ||       // or any none object processing variable
      pValue == &pToolData->M_AOI.YPos ||
      pValue == &pToolData->M_AOI.XSize ||
      pValue == &pToolData->M_AOI.YSize) {

    // Force recalculation of processing

    pToolData->Input1_Change = 0;          // Force recalculation output
    pToolData->Input2_Change = 0;          // Force recalculation output
    pToolData->Res_Crcdf.Valid = false;    // Correlation results are NOT valid

  } if( pToolData->Obj_processing) {      // Object processing is on

    if( pValue == &pToolData->Obj_QualityThres) {   // Quality threshold

      // Redraw last results

      if( pToolData->YaIPS_ImageDisp.pImage_Box != NULL) {
        pToolData->YaIPS_ImageDisp.pImage_Box->redraw();
      }

      if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

        if( YaIPS_BigImageDisp.pImage_Box != NULL) {
          YaIPS_BigImageDisp.pImage_Box->redraw();
        }
      }

    } else if( pValue == &pToolData->Obj_BinThres ||      // Binarization threshold
               pValue == &pToolData->Obj_AreaMinSide ||   // Minimum area
               pValue == &pToolData->Obj_AreaMaxSide) {   // Maximum area

      // Force recalculation of processing

      pToolData->Input1_Change = 0;          // Force recalculation output
      pToolData->Input2_Change = 0;          // Force recalculation output
      pToolData->Res_Crcdf.Valid = false;    // Correlation results are NOT valid
    }
  }
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

  if( w == pTeachToggle) {                   // Toggle Teach / Inspection button

    pToolData->AOI_Teach = ! pToolData->AOI_Teach;

  }

  pToolData->Input1_Change = 0;          // Force recalculation output
  pToolData->Input2_Change = 0;                    // Force recalculation output
  pToolData->Res_Crcdf.Valid = false;              // Correlation results are NOT valid
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
  pToolData->Res_Crcdf.Valid = false;              // Correlation results are NOT valid
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

  pMyParWin = new Fl_Window( xPos, yPos, 297 /*IQE_GUI_TOOLS_STD_WITDH*/, 138, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  //
  //  GUI things
  //

  int x1, y, yy, xx1;
  //x/int xx2, xc;
  int yGroup;
  //x/char TempBuffer[ 256];

  Fl_Check_Button *pCheckTemp;
  //x/Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  //x/IqeFl_Float_Input  *pFloatTemp;
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
  // Group 'Combine'
  //

  yGroup = y;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Crcdf_TabA1=General"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Crcdf_TabA1a=General settings"));

    y += 8;

    x1 = 4;
    xx1 = pMyParWin->w() - x1 - 4;

    pTemp_Group2 = new Fl_Group( x1, y, xx1, yy, LangStringLookup( "&GUI_Crcdf_TabA2=Output image:"));  // Group around the radio buttons
    pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

    // ...

    x1 += 94;

    xx1 = 62;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LangStringLookup( "&GUI_Crcdf_TabA3=Scene"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Crcdf_TabA3a="
                           "The input image ‘Scene’ is\n"
                           "displayed as the output image."));
    pRadioButTemp->callback( YaIPS_OutputImage_Callback, (void *)0);
    pRadioButTemp->value( pToolData->OutputImage == 0);   // Set value

    x1 += xx1;
    x1 += 2;

    xx1 = 92;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LangStringLookup( "&GUI_Crcdf_TabA4=Correlation"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Crcdf_TabA4a=The correlation field is displayed as the output image."));
    pRadioButTemp->callback( YaIPS_OutputImage_Callback, (void *)1);
    pRadioButTemp->value( pToolData->OutputImage == 1);   // Set value

    pTemp_Group2->end();

    // Next line

    y += yy + 4;

    x1 = 4;
    xx1 = pMyParWin->w() - x1 - 4;

    pTemp_Group2 = new Fl_Group( x1, y, xx1, yy, LANGDEF_COL_CHANNEL_DPOINT);  // Group around the radio buttons
    pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

    // ...

    x1 += 94;

    xx1 = 40;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LANGDEF_COLOR_R);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Crcdf_TabA6a="
                            "For a color image, the red color\n"
                            "channel is used for correlation."));
    pRadioButTemp->callback( YaIPS_ColorChannel_Callback, (void *)0);
    pRadioButTemp->value( pToolData->ColorChannel == 0);   // Set value

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LANGDEF_COLOR_G);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Crcdf_TabA7a="
                            "For a color image, the green color\n"
                            "channel is used for correlation."));
    pRadioButTemp->callback( YaIPS_ColorChannel_Callback, (void *)1);
    pRadioButTemp->value( pToolData->ColorChannel == 1);   // Set value

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LANGDEF_COLOR_B);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Crcdf_TabA8a="
                            "For a color image, the blue color\n"
                            "channel is used for correlation."));
    pRadioButTemp->callback( YaIPS_ColorChannel_Callback, (void *)2);
    pRadioButTemp->value( pToolData->ColorChannel == 2);   // Set value

    pTemp_Group2->end();

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 69;

    xx1 = 44;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LANGDEF_AOI_LEFT);
    pTemp_Int->tooltip( LANGDEF_AOI_LEFT_TOOLTIP);
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->M_AOI.XPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pTemp_Int->SetValue( pToolData->M_AOI.XPos);
    pAOI_X = pTemp_Int;

    x1 += xx1;
    x1 += 50;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LANGDEF_AOI_TOP);
    pTemp_Int->tooltip( LANGDEF_AOI_TOP_TOOLTIP);
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->M_AOI.YPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pTemp_Int->SetValue( pToolData->M_AOI.YPos);
    pAOI_Y = pTemp_Int;

    x1 += xx1;
    x1 += 8;

    pTemp_Button = new Fl_Button( x1, y, 28, 28, "@+1pencil");
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->AOI_Teach);
    pTemp_Button->tooltip( LANGDEF_AOI_TEACH_TOOLTIP);
    pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
    pTemp_Button->shortcut( FL_COMMAND+'t');       // Short cut key
    pTeachToggle = pTemp_Button;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 69;

    xx1 = 44;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LANGDEF_AOI_WIDTH);
    pTemp_Int->tooltip( LANGDEF_AOI_WIDTH_TOOLTIP);
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->M_AOI.XSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pTemp_Int->SetValue( pToolData->M_AOI.XSize);
    pAOI_XX = pTemp_Int;

    x1 += xx1;
    x1 += 50;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LANGDEF_AOI_HEIGHT);
    pTemp_Int->tooltip( LANGDEF_AOI_HEIGHT_TOOLTIP);
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->M_AOI.YSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pTemp_Int->SetValue( pToolData->M_AOI.YSize);
    pAOI_YY = pTemp_Int;

    // Finish things for this group

  pTemp_Group->end();

  //
  // Group weighted add
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Crcdf_TabB1=Objects in scene"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Crcdf_TabB1a=Separate objects in the scene"));

    y += 8;

    x1 = 4;
    xx1 = pMyParWin->w() - 16;

    pCheckTemp = new Fl_Check_Button( x1 + 4, y, xx1, yy, LangStringLookup( "&GUI_Crcdf_TabB2=Separate objects"));
    pCheckTemp->tooltip( LangStringLookup( "&GUI_Crcdf_TabB2a="
                         "Objects are isolated in the scene.\n"
                         "Each object is correlated separately."));
    pCheckTemp->value( pToolData->Obj_processing);
    pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->Obj_processing);

    // Next line

    y += yy + 4;

    x1 = 4;
    xx1 = pMyParWin->w() - x1 - 4;

    pTemp_Group2 = new Fl_Group( x1, y, xx1, yy, LangStringLookup( "&GUI_Crcdf_TabB3=Objects are:"));  // Group around the radio buttons
    pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

    // ...

    x1 += 94;

    xx1 = 62;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LangStringLookup( "&GUI_Crcdf_TabB4=bright"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Crcdf_TabB4a=Objects are brighter than the threshold."));
    pRadioButTemp->callback( YaIPS_BinMode_Callback, (void *)0);
    pRadioButTemp->value( pToolData->Obj_BinMode == 0);   // Set value

    x1 += xx1;
    x1 += 2;

    xx1 = 92;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LangStringLookup( "&GUI_Crcdf_TabB5=dark"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Crcdf_TabB5a=Objects are darker than the threshold."));
    pRadioButTemp->callback( YaIPS_BinMode_Callback, (void *)1);
    pRadioButTemp->value( pToolData->Obj_BinMode == 1);   // Set value

    pTemp_Group2->end();

    // Next line

    y += yy + 4;

    xx1 = 34;

    x1 = 148 - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Crcdf_TabB6=Thres. bright."));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Crcdf_TabB6a="
                        "Brightness threshold for binarization.\n"
                        "Separates objects from the background."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Obj_BinThres);
    pTemp_Int->SetModifyData( 1, 254, 10, 1);
    pTemp_Int->SetValue( pToolData->Obj_BinThres);

    x1 = 296 - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Crcdf_TabB9=Thres. Quality"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Crcdf_TabB9a="
                        "Results with better quality are colored green,\n"
                        "results with poor quality are colored yellow to pink."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Obj_QualityThres);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pTemp_Int->SetValue( pToolData->Obj_QualityThres);

    // Next line

    y += yy + 4;

    xx1 = 34;

    x1 = 148 - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Crcdf_TabB7=T small"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Crcdf_TabB7a="
                        "Smallest side length of a square in pixels.\n"
                        "The area of the square is the minimum\n"
                        "area of an object to be processed."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Obj_AreaMinSide);
    pTemp_Int->SetModifyData( 0, 200, 10, 1);
    pTemp_Int->SetValue( pToolData->Obj_AreaMinSide);

    x1 = 296 - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Crcdf_TabB8=T large"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Crcdf_TabB8a="
                        "Largest side length of a square in pixels.\n"
                        "The area of the square is the maximum\n"
                        "area of an object to be processed.\n"
                        "NOTE: must be greater than 'T small'."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Obj_AreaMaxSide);
    pTemp_Int->SetModifyData( 0, 500, 10, 1);
    pTemp_Int->SetValue( pToolData->Obj_AreaMaxSide);

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
    pToolData->Res_Crcdf.Valid = false;          // Correlation results are NOT valid

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
    pTemp_Box = new Fl_Box( x1, y + yy - yy2 - yy2 + 1, xx, yy2, LANGDEF_IMGSEL_PDS_SCENE);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label

    // 2. Input element
    pTemp_Box = new Fl_Box( x1, y + yy - yy2, xx, yy2, LANGDEF_IMGSEL_PDS_OBJECT);
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
    pBut_Input1->tooltip( LangStringLookup( "&GUI_Crcdf_Tools2a="
                          "Scene. This image is searched\n"
                          "for occurrences of the object."));
    pBut_Input1->labelcolor( YAIPS_BCOL_BUTTON);
    pBut_Input1->box( FL_BORDER_BOX);

    // 2. Input element
    pBut_Input2 = new Fl_Button( x1, y + yy - yy2, xx, yy2, "@2>");
    pBut_Input2->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pBut_Input2->tooltip( LangStringLookup( "&GUI_Crcdf_Tools3a="
                          "Object.\n"
                          "Must be a black and white image."));
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

  if( pToolData->pCorrImage != NULL) {     // This image was used

    pToolData->pCorrImage->release();      // Release the image
    pToolData->pCorrImage = NULL;
  }

  if( pToolData->pObjects != NULL) {       // Object vector was used

    ve_remove( pToolData->pObjects);       // Release the vector
    pToolData->pObjects = NULL;
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

  // Ensure AOI from this tool window on big image display is removed if this tool window is closed.
  if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

    if( YaIPS_BigImageDisp.pImage_Box != NULL) {
      YaIPS_BigImageDisp.pImage_Box->redraw();
    }
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

      pToolData->Input1_Change = 0;                            // Force recalculation output
      pToolData->Res_Crcdf.Valid = false;                      // Correlation results are NOT valid
    }

  } else if( w == pMyToolWin->pBut_Input2) {                   // Select 2. input

    int WinIdNr_Before;

    WinIdNr_Before = pToolData->Input2_WinIdNr;

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + iToolData, &pToolData->Input2_WinIdNr, pMyToolWin->pBut_Input2, pMyToolWin->pBox_Input2);

    if( WinIdNr_Before != pToolData->Input2_WinIdNr) {         // Image source selection as changed

      pToolData->Input2_Change = 0;                            // Force recalculation output
      pToolData->Res_Crcdf.Valid = false;                      // Correlation results are NOT valid
    }

  } else if( w == pMyToolWin->pGUI_Parameter) {                // Open parameter dialog

    YaIPS_GUI_ParameterWin( pMyToolWin->x() + 16, pMyToolWin->y() + 16, iToolData);

  } else if( w == pMyToolWin->pGUI_TeachToggle) {              // Toggle Teach / Inspection

    pToolData->AOI_Teach = ! pToolData->AOI_Teach;

    pToolData->Input1_Change = 0;          // Force recalculation output
    pToolData->Input2_Change = 0;                    // Force recalculation output
    pToolData->Res_Crcdf.Valid = false;              // Correlation results are NOT valid
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

    MouseTeachState = pToolData->AOI_Teach ? 3 : 2;
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

    int Input1_Check, Input1_ImageChanged, Input2_Check, Input2_ImageChanged, ForceUpdate, ScnByte;
    Fl_RGB_Image *pImgIn1, *pImgIn2;
    char FileName2[ MAX_FILENAME_LEN];    // File name with path

    ForceUpdate = false;                         // Preset: NO Force update of output image

    // Check input image and visualize state
    Input1_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr,
                                           NULL, &pImgIn1, &Input1_ImageChanged);

    Input2_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input2_WinIdNr,
                                            NULL, &pImgIn2, &Input2_ImageChanged, FileName2, sizeof( FileName2));

    if( Input1_Check == 0 &&                                // First input image is OK
        pToolData->AOI_Teach) {                             // and show AOI on scene image

      if( Input1_ImageChanged != pToolData->Input1_Change) { // if image counts are different

        // Show scene image to adjust AOI parameter

        pToolData->Input1_Change = Input1_ImageChanged;        // Image is processed
        errstring = NULL;                                      // Reset error string

        // Copy scene image to output image
        ierr = YaIPS_RGB_CopyImg( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1);

        YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, (char *)MY_WIN_GUI_NAME);

        YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp); // Reset error message

        ForceUpdate = true;                        // Force update of output image
      }

      if( pToolData->pObjects != NULL) {           // Object vector is preallocated

        vputnm( pToolData->pObjects, 0);           // Reset number of labeled objects
      }

      pToolData->Input2_Change = 0;                    // Force recalculation output
      pToolData->Res_Crcdf.Valid = false;              // Correlation results are NOT valid

    } else
      if( Input1_Check != 0 || Input2_Check != 0) {            // Any input image is not valid

      // Empty display image
      YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);

      if( YaIPS_BigImageDisp.pImage_Img != NULL &&                       // Big display has an image
          YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {   // and display this on the big image

        YaIPS_ImageDispEmpty( &YaIPS_BigImageDisp);     // Empty the big display

        YaIPS_BigImageDisp.ImageSourceID = MY_WIN_ID + iToolData;        // Restore display selection of this tool window
      }

      if( pToolData->pObjects != NULL) {           // Object vector is preallocated

        vputnm( pToolData->pObjects, 0);           // Reset number of labeled objects
      }

      pToolData->Input1_Change = 0;                    // Force recalculation output
      pToolData->Input2_Change = 0;                    // Force recalculation output
      pToolData->Res_Crcdf.Valid = false;              // Correlation results are NOT valid

    } else
      if( Input1_ImageChanged != pToolData->Input1_Change ||  // Image counts are different
          Input2_ImageChanged != pToolData->Input2_Change) {

      pToolData->Input1_Change = Input1_ImageChanged;        // Image is processed
      pToolData->Input2_Change = Input2_ImageChanged;        // Image is processed

      // Reset errors

      ierr = 0;                                              // Reset error
      errstring = NULL;                                      // Reset error string

      // Preallocate object vector

      if( pToolData->Obj_processing) {                       // Object processing on

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

      } else {                                               // No object processing

        if( pToolData->pObjects != NULL) {                   // Have an object vector

          ve_remove( pToolData->pObjects);                   // Remove it

          pToolData->pObjects = NULL;
        }
      }

      // Prepare color channel selection

      if( pImgIn1->d() >= 3) {                               // Image has three or more color channels
        ScnByte = pToolData->ColorChannel;                   // Use color channel selection
      } else {
        ScnByte = 0;                                         // Use first channel
      }

      // Get objects in scene

      if( pToolData->pObjects != NULL) {                     // Object vector preallocated

        ierr = YaIPS_RGB_RLC_CodeMeas( pToolData->pObjects, pImgIn1,
                                       ScnByte + YAIPS_DISP_COLMOD_R,   // In: Color space for color images R, G or B
                                       pToolData->Obj_BinThres,         // In: Binarization 1. threshold
                                       0,                               // In: Binarization 2. threshold
                                       pToolData->Obj_BinMode,          // In: Binarization mode, 0: objects >= Thres, 1: code if < Thres
                                       pToolData->Obj_AreaMinSide * pToolData->Obj_AreaMinSide,  // In: Minimum area of an object to be labeled
                                       pToolData->Obj_AreaMaxSide * pToolData->Obj_AreaMaxSide,  // In: Maximum area of an object to be labeled
                                       pToolData->M_AOI.XPos, pToolData->M_AOI.YPos, pToolData->M_AOI.XSize, pToolData->M_AOI.YSize);

        // Check for multiple objects

        pToolData->ObjOpt.NumObjects = 0;
        ierr = YaIPS_RGB_Crcdf_ObjOptions( &pToolData->ObjOpt, pImgIn2, FileName2,
                                           pToolData->Obj_BinThres,         // In: Binarization threshold
                                           pToolData->Obj_BinMode,          // In: Binarization mode, 0: objects >= Thres, 1: code if < Thres
                                           pToolData->Obj_AreaMinSide * pToolData->Obj_AreaMinSide,  // In: Minimum area of an object to be labeled
                                           pToolData->Obj_AreaMaxSide * pToolData->Obj_AreaMaxSide); // In: Maximum area of an object to be labeled

      } else {

        // Check for multiple objects

        pToolData->ObjOpt.NumObjects = 0;
        ierr = YaIPS_RGB_Crcdf_ObjOptions( &pToolData->ObjOpt, pImgIn2, FileName2);
      }

      // Do the image processing

      if( ierr == 0 && pToolData->ObjOpt.NumObjects > 0) {                  // Have multiple objects

        // Correlation of multiple objects

        ierr = YaIPS_RGB_Crcdf_Calc( &pToolData->pCorrImage, pImgIn1, ScnByte, pToolData->pObjects, pImgIn2, &pToolData->ObjOpt, &pToolData->Res_Crcdf,
                                     pToolData->M_AOI.XPos, pToolData->M_AOI.YPos, pToolData->M_AOI.XSize, pToolData->M_AOI.YSize);

      } else {                                               // Have a single object

        pToolData->ObjOpt.NumObjects = 0;                    // Ensure reseted

        // Correlation of a single object

        ierr = YaIPS_RGB_Crcdf_Calc( &pToolData->pCorrImage, pImgIn1, ScnByte, pToolData->pObjects, pImgIn2, &pToolData->Res_Crcdf,
                                     pToolData->M_AOI.XPos, pToolData->M_AOI.YPos, pToolData->M_AOI.XSize, pToolData->M_AOI.YSize);
      }

      if( ierr == 0) {                                       // Correlation was OK, has a correlation image

        if( pToolData->OutputImage == 1 &&                   // Show correlation image
            pToolData->pCorrImage != NULL) {                 // and have a correlation image

          // Convert the correlation image to a displayable format
          ierr = YaIPS_RGB_Crcdf_Conv2BW( &pToolData->YaIPS_ImageDisp.pImage_Img, pToolData->pCorrImage, &pToolData->Res_Crcdf);

        } else {                                             // Show scene input image

          // Copy scene image to output image
          ierr = YaIPS_RGB_CopyImg( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1);
        }
      }

      // Has a valid output image

      if( ierr == 0) {                                  // Have a result image

        YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, (char *)MY_WIN_GUI_NAME);

        YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp); // Reset error message

        ForceUpdate = true;                        // Force update of output image

      } else {                                          // Processing error

        pToolData->Input1_Change = 0;                   // Force recalculation output
        pToolData->Input2_Change = 0;                   // Force recalculation output
        pToolData->Res_Crcdf.Valid = false;              // Correlation results are NOT valid

        // Empty display image
        YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);

        if( pToolData->pObjects != NULL) {           // Object vector is preallocated

          vputnm( pToolData->pObjects, 0);           // Reset number of labeled objects
        }

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
 * IqeB_GUI_CrcdfWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_CrcdfWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
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
  pToolData->pCorrImage = NULL;                            // Have no correlation image until now
  pToolData->pObjects = NULL;                              // Have no object vector
  pToolData->ObjOpt.NumObjects = 0;                        // Ensure reseted

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
 * DoClip:           if true (> 0) handle clipping of draw region
 *                   else this must be done in the calling function
 *
 */
static void YaIPS_GUI_MyDrawAfter_Func( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // point to image display data
                                        YaIPS_ToolData_info_t *pToolData,        // point to tool data
                                        int DoClip)                              // if true (> 0) handle clipping of draw region else caller must do it
{
  int x1, y1, x, y, xx, yy, xxo, yyo, OffX, OffY, DrawCrdfText;
  char TempString[ 256];
  // Colors
  Fl_Color ColFrameErr = fl_rgb_color( 255, 64, 64);
  uchar TextGood1r = 128, TextGood1g = 255,TextGood1b =  0, TextGood2r = 0, TextGood2g = 196, TextGood2b = 0;
  uchar TextBad1r = 255, TextBad1g = 255,TextBad1b =  0, TextBad2r = 255, TextBad2g = 0, TextBad2b = 255;
  uchar r, g, b;
  int TempMul1, TempMul2;
  Fl_Color ColTemp;
  float ThresGood, FakGood, FakBad, Quality;
  static char MyLineDashes[] = { 4, 4, 0};


  // Check for valid output image displayed

  if( pToolData->YaIPS_ImageDisp.pImage_Img == NULL) {

    return;
  }

  // ...

  ThresGood = pToolData->Obj_QualityThres;

  if( pToolData->Obj_QualityThres >= 100.0) {

    FakGood = 255.0 / 100.0;
    FakBad  = 255.0 / 100.0;

  } else if( pToolData->Obj_QualityThres <= 0.0) {

    FakGood = 255.0 / 100.0;
    FakBad  = 0.0;

  } else {

    FakGood = 255.0 / (100.0 - ThresGood);
    FakBad  = 255.0 / ThresGood;
  }

  int LineWidth;                          // Unused variables

  // Preparations

  x1 = pYaIPS_ImageDisp->BigImage_sx;
  y1 = pYaIPS_ImageDisp->BigImage_sy;
  xx = pYaIPS_ImageDisp->BigImage_sw;
  yy = pYaIPS_ImageDisp->BigImage_sh;

  // Points relative to image

  OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
  OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

  LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

  // Draw AOI

  if( pYaIPS_ImageDisp->pImage_Img != NULL &&   // Have an output image
      ( pToolData->AOI_Teach ||                 // Adjust AOI
        pToolData->OutputImage != 1 ||          // Show scene image
        pToolData->pCorrImage == NULL)) {       // Or have no correlation image

    int AOI_XX, AOI_YY;

    // Get AOI

    if( pToolData->M_AOI.XSize >= pYaIPS_ImageDisp->pImage_Img->w()) {

      AOI_XX = pYaIPS_ImageDisp->pImage_Img->w();
    } else {
      AOI_XX = pToolData->M_AOI.XSize;
    }

    if( pToolData->M_AOI.YSize >= pYaIPS_ImageDisp->pImage_Img->h()) {

      AOI_YY = pYaIPS_ImageDisp->pImage_Img->h();
    } else {
      AOI_YY = pToolData->M_AOI.YSize;
    }

    // Clipping ?

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( x1, y1, xx, yy);
    }

    if( pToolData->AOI_Teach) {                // Adjust AOI

      if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0) {  // Mouse is over the AOI

        fl_color( FL_RED);
      } else {

        fl_color( FL_GREEN - 2);
      }

      fl_line_style( 0, LineWidth);   // Set line width

    } else {

      fl_color( FL_GREEN - 2);

      fl_line_style( 0, 0, MyLineDashes);   // Set line width
    }

    x = (int)( (pToolData->M_AOI.XPos - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    y = (int)( (pToolData->M_AOI.YPos - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

    xxo = (int)( AOI_XX * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    yyo = (int)( AOI_YY * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

    fl_rect( x1 + x, y1 + y, xxo, yyo);
  }

  //
  // Draw best correlation result
  //

  if( pToolData->pObjects == NULL &&                         // No objects in scene
    ! pToolData->Res_Crcdf.Valid) {                          // Test for a valid correlation result

    goto ExitPoint;
  }

  // Clipping ?

  if( DoClip > 0) {      // The the draw clipping

    DoClip = -1;         // Need to pop clipping

    fl_push_clip( x1, y1, xx, yy);
  }

  // ...

  // Prepare font size

  DrawCrdfText = false;                                            // Preset, do not draw crdcf text

  if( pYaIPS_ImageDisp->PixelImageToScreen >= 0.33) {              // Is NOT to tiny

    int TempFontSize;

    DrawCrdfText = true;                                           // Draw crcdf text

    TempFontSize = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 6.0 + 0.5);

    if( TempFontSize < 10) {

      TempFontSize = 10;
    }

    fl_font( FL_HELVETICA, TempFontSize);
  }

  // ...

  fl_line_style( 0);   // Reset to default

  if( pToolData->pObjects == NULL) {                    // No objects in scene

    xxo = (int)( pToolData->Res_Crcdf.ObjSizeXX * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    yyo = (int)( pToolData->Res_Crcdf.ObjSizeYY * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

    if( pToolData->OutputImage == 1) {                   // Show correlation image

      x = (int)( (pToolData->Res_Crcdf.xpos - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
      y = (int)( (pToolData->Res_Crcdf.ypos - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

      // Show rectangle in center of peak
      x -= xxo / 2;
      y -= yyo / 2;

    } else {                                             // Show scene input image

      // Relative to AOI
      x = (int)( (pToolData->Res_Crcdf.xpos + pToolData->M_AOI.XPos - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
      y = (int)( (pToolData->Res_Crcdf.ypos + pToolData->M_AOI.YPos - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    }

    Quality = pToolData->Res_Crcdf.Quality;

    if( Quality >= 100.0) {

      ColTemp = fl_rgb_color( TextGood1r, TextGood1g, TextGood1b);

    } else if( Quality <= 0.0) {

      ColTemp = fl_rgb_color( TextBad2r, TextBad2g, TextBad2b);

    } else if( Quality >= ThresGood) {

      TempMul1 = (int)((Quality - ThresGood) * FakGood);
      TempMul2 = 255 - TempMul1;

      r = (TextGood1r * TempMul1 + TextGood2r * TempMul2) >> 8;
      g = (TextGood1g * TempMul1 + TextGood2g * TempMul2) >> 8;
      b = (TextGood1b * TempMul1 + TextGood2b * TempMul2) >> 8;

      ColTemp = fl_rgb_color( r, g, b);

    } else {

      TempMul1 = (int)(Quality * FakBad);
      TempMul2 = 255 - TempMul1;

      r = (TextBad1r * TempMul1 + TextBad2r * TempMul2) >> 8;
      g = (TextBad1g * TempMul1 + TextBad2g * TempMul2) >> 8;
      b = (TextBad1b * TempMul1 + TextBad2b * TempMul2) >> 8;

      ColTemp = fl_rgb_color( r, g, b);
    }

    fl_color( ColTemp);
    fl_rect( x1 + x, y1 + y, xxo, yyo);

    // Draw label and quality of correlation to screen

    if( DrawCrdfText) {                                                 // Draw crcdf text

      if( pToolData->Res_Crcdf.Label[ 0] != '\0') {

        sprintf( TempString, "%s: %.1f", pToolData->Res_Crcdf.Label, Quality);
      } else {

        sprintf( TempString, LangStringLookup( "&GUI_Crcdf_Draw1=Q %.1f"), Quality);
      }

      fl_draw( TempString,  x1 + x, y1 + y - 2);
    }

  } else {                                 // Have objects in scene
    int nSceneObj, iSceneObj;
    Trl2objdst *pSceneObj;
    char *pLabel;

    nSceneObj = (int)(vgetnm( pToolData->pObjects) / (sizeof(Trl2objdst) / sizeof(int32)));

    pSceneObj  = (Trl2objdst *)vgetpm( pToolData->pObjects);

    for( iSceneObj = 0; iSceneObj < nSceneObj; iSceneObj++, pSceneObj++) {

      xxo = pSceneObj->xmax - pSceneObj->xmin + 1;
      yyo = pSceneObj->ymax - pSceneObj->ymin + 1;

      xxo = (int)( xxo * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
      yyo = (int)( yyo * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

      // Relative to AOI
      x = (int)( (pSceneObj->xmin + pToolData->M_AOI.XPos - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
      y = (int)( (pSceneObj->ymin + pToolData->M_AOI.YPos - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

      Quality = 0.0;                    // Need this to avoid compiler warning

      if( pSceneObj->ExData1 < 0) {     // Error during correlation

        fl_color( ColFrameErr);

      } else {

        Quality = pSceneObj->ExData2 * 0.1;

        if( Quality >= 100.0) {

          ColTemp = fl_rgb_color( TextGood1r, TextGood1g, TextGood1b);

        } else if( Quality <= 0.0) {

          ColTemp = fl_rgb_color( TextBad2r, TextBad2g, TextBad2b);

        } else if( Quality >= ThresGood) {

          TempMul1 = (int)((Quality - ThresGood) * FakGood);
          TempMul2 = 255 - TempMul1;

          r = (TextGood1r * TempMul1 + TextGood2r * TempMul2) >> 8;
          g = (TextGood1g * TempMul1 + TextGood2g * TempMul2) >> 8;
          b = (TextGood1b * TempMul1 + TextGood2b * TempMul2) >> 8;

          ColTemp = fl_rgb_color( r, g, b);

        } else {

          TempMul1 = (int)(Quality * FakBad);
          TempMul2 = 255 - TempMul1;

          r = (TextBad1r * TempMul1 + TextBad2r * TempMul2) >> 8;
          g = (TextBad1g * TempMul1 + TextBad2g * TempMul2) >> 8;
          b = (TextBad1b * TempMul1 + TextBad2b * TempMul2) >> 8;

          ColTemp = fl_rgb_color( r, g, b);
        }

        fl_color( ColTemp);
      }

      fl_rect( x1 + x, y1 + y, xxo, yyo);

      // Draw label and quality of correlation to screen

      if( DrawCrdfText &&                                                // Draw crcdf text
          pSceneObj->ExData1 >= 0) {                                     // and is Valid pointer to object

        if( pSceneObj->ExData1 < pToolData->ObjOpt.NumObjects) {               // Is one of multiple objects

          pLabel = pToolData->ObjOpt.Object[ pSceneObj->ExData1].Label;
        } else {

          pLabel = NULL;                                                       // Is a single object
        }

        if( pLabel != NULL && pLabel[ 0] != '\0') {

          sprintf( TempString, "%s: %.1f", pLabel, Quality);
        } else {

          sprintf( TempString, LangStringLookup( "&GUI_Crcdf_Draw1=Q %.1f"), Quality);
        }

        fl_draw( TempString,  x1 + x, y1 + y - 2);
      }
    }
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
  int ierr, x, y;
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

      if( pToolData->AOI_Teach &&                   // Adjust AOI
          pYaIPS_ImageDisp->pImage_Img != NULL) {   // and have an output image

        pAOI_This = &pToolData->M_AOI;

        ierr = YaIPS_ImageDispAoiRectCC( pYaIPS_ImageDisp, pAOI_This, &minAoiDist, &CursorShapeTest, &AoiDeltaAddTest);

        if( ierr == true) {     // Got one (or a better one)

          // nearer aoi found
          pYaIPS_ImageDisp->CursorShape = CursorShapeTest;
          pYaIPS_ImageDisp->AoiDeltaAdd = AoiDeltaAddTest;
          pAOI_Best   = pAOI_This;
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
 * IqeB_GUI_CrcdfWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_CrcdfWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  // ...

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

    // Register draw after function for big image display
    YaIPS_ToolWinDrawAfterSet( MY_WIN_ID + SubWinIDx, YaIPS_GUI_MyDrawAfter_Other);

    IqeB_GUI_CrcdfWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  IqeB_GUI_CrcdfWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


