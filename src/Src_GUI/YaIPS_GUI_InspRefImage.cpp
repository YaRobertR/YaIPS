/****************************************************************************

  YaIPS_GUI_InspRefImage.cpp

  Inspection reference image windows.

  25.09.2025 RR: First edition of this file.
  03.09.2026 RR: * Replace use of the working directory string
                   'YaIPS_WorkingDirectory' by '.'.
                 * Reworked clipboard handling
                   Replaced patch './Images/YaIPS' with YaIPS_CLIPBOARD_PATH.
  18.10.2026 RR: * Reworked behavior of tool window parameter dialogs.
                   See functions IqeB_GUI_ToolParCheckOpen(),
                   IqeB_GUI_ToolParCheckReset() and IqeB_GUI_ToolParCheckClose().
  24.10.2026 RR: * Begin with support for check masks in image comparison.
                   The plan is to use the alpha channel of the reference image
                   for check masks. There will be three check
                   masks. Each will be stored in its own bit plane in the alpha
                   channel. These masks will be made with an editor.
  09.11.2026 RR: * Added support for check masks in image comparison.
                   See: one of the defines YAIPS_INSP_COMP_FLAG_MASK,
                        YAIPS_REFMASK_CHECK_MASK_ or YAIPS_NOALPHADISP_MASK_
  11.11.2026 RR: * MyWinUpdate()
                   Parameter dialog button is disabled if the parameter dialog
                   is open.

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
#define MY_WIN_ID     YAIPS_WIN_ID_INSP_REF_IMG          // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_INSP_REF_IMG         // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_InspRefImg_Title=Reference image" // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinInspRefImg"            // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_InspRefImg_Tool  // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       227  // YAIPS_WIN_SIZE_S1_X_MIN
#define MYWIN_SIZE_X_MAX       YAIPS_WIN_SIZE_S1_X_MAX
#define MYWIN_SIZE_X_DEFAULT   YAIPS_WIN_SIZE_S1_X_DEFAULT

#define MYWIN_SIZE_Y_MIN       YAIPS_WIN_SIZE_S1_Y_MIN
#define MYWIN_SIZE_Y_MAX       YAIPS_WIN_SIZE_S1_Y_MAX
#define MYWIN_SIZE_Y_DEFAULT   YAIPS_WIN_SIZE_S1_Y_DEFAULT

/************************************************************************************
* forwards
*/

static void close_cb( Fl_Widget *w, long int iToolData);
static void Load_cb( Fl_Widget *w, long int iToolData);
static void SnapFromInput_cb( Fl_Widget *w, long int iToolData);
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

#define MASK_GROUPS_N               3    // Number of mask groups

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
  int MaskDrawMouseActive;              // If != 0 if mask draw action is active

  // Used to catch a change of the input image
  int Input1_Change;                    // Last processed 'ImageChanged' from input image

  int ImageHasChanged;                  // Set to != 0 if image needs an backup after an image change

  YaIPS_G2D_UnDo_Buffer_t UnDoFIFO;     // Data for undo FIFO

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int Tab_Group_Selected;               // Number of last selected tab group.

  // Tab group 'Edges'

  int ContourPlaneThres;                // Contour: Plane threshold

  // Tab group 'Mask'

  int MaskDrawMode[ MASK_GROUPS_N];     // Mask: Mask draw mode
  int MaskBrush[ MASK_GROUPS_N];        // Mask: Brush for free hand mode
  int MaskColor[ MASK_GROUPS_N];        // Mask: If set draw mask else erase mask
  int MaskAutoThres[ MASK_GROUPS_N];    // Mask: Brightness threshold for auto mask function
  int MaskAutoMode[ MASK_GROUPS_N];     // Mask: Mode for auto mask: 0 is bright, 1 is dark

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
  // and other common settings

  { PREF_T_INT,   "Group_Selected",     "0", &YaIPS_ToolData_info[0].Tab_Group_Selected},

  // Tab group 'Edges'

  { PREF_T_INT,     "ContourPlaneThres",    "16", &YaIPS_ToolData_info[0].ContourPlaneThres},

  // Tab group 'Mask'

  { PREF_T_INT,        "MaskDrawMode_1",     "0", YaIPS_ToolData_info[0].MaskDrawMode + 0},
  { PREF_T_INT,        "MaskDrawMode_2",     "0", YaIPS_ToolData_info[0].MaskDrawMode + 1},
  { PREF_T_INT,        "MaskDrawMode_3",     "0", YaIPS_ToolData_info[0].MaskDrawMode + 2},

  { PREF_T_INT,           "MaskBrush_1",     "1", YaIPS_ToolData_info[0].MaskBrush + 0},
  { PREF_T_INT,           "MaskBrush_2",     "1", YaIPS_ToolData_info[0].MaskBrush + 1},
  { PREF_T_INT,           "MaskBrush_3",     "1", YaIPS_ToolData_info[0].MaskBrush + 2},

  { PREF_T_INT,           "MaskColor_1",     "1", YaIPS_ToolData_info[0].MaskColor + 0},
  { PREF_T_INT,           "MaskColor_2",     "1", YaIPS_ToolData_info[0].MaskColor + 1},
  { PREF_T_INT,           "MaskColor_3",     "1", YaIPS_ToolData_info[0].MaskColor + 2},

  { PREF_T_INT,       "MaskAutoThres_1",    "64", YaIPS_ToolData_info[0].MaskAutoThres + 0},
  { PREF_T_INT,       "MaskAutoThres_2",    "64", YaIPS_ToolData_info[0].MaskAutoThres + 1},
  { PREF_T_INT,       "MaskAutoThres_3",    "64", YaIPS_ToolData_info[0].MaskAutoThres + 2},

  { PREF_T_INT,        "MaskAutoMode_1",     "0", YaIPS_ToolData_info[0].MaskAutoMode + 0},
  { PREF_T_INT,        "MaskAutoMode_2",     "0", YaIPS_ToolData_info[0].MaskAutoMode + 1},
  { PREF_T_INT,        "MaskAutoMode_3",     "0", YaIPS_ToolData_info[0].MaskAutoMode + 2},

};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_InspRefImgWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp);

//-----------------------------------------------------------------------------------
// Parameter dialog
//
// This is a modal dialog. Therefore we can use global variables to hold
// info about the data.
//-----------------------------------------------------------------------------------

// defines for function selection
// NOTE: Tab group selection is used for function

#define YAIPS_SELECTION_EDGES       0    // Tab group 'Edges'
#define YAIPS_SELECTION_MASK_1      1    // First tab group 'Mask'
#define YAIPS_SELECTION_MASK_3      (1 + MASK_GROUPS_N - 1)  // Last tab group 'Mask'

// ...

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolDataParam;         // NOTE: Is used by all parameter dialog functions

static IqeFl_Tabs      *pTab_Groups;              // Point to tabulator GUI element

// Tab group 'Mask'

#define MASK_DRAW_MOVE_IMAGE        0    // Move image with mouse
#define MASK_DRAW_FREEHAND          1    // Draw free hand
#define MASK_DRAW_RECTGANGLE        2    // Draw a rectangle
#define MASK_DRAW_ELLIPSE           3    // Draw an ellipse
#define MASK_DRAW_POLYGON           4    // Draw a polygon
#define MASK_DRAW_FILL              5    // Fill an area
#define MASK_DRAW_N_BUTTONS         6    // Number of draw buttons

static Fl_Button *pBut_MaskDrawMode[ MASK_GROUPS_N][ MASK_DRAW_N_BUTTONS];
static Fl_Button *pBut_Undo[ MASK_GROUPS_N], *pBut_Redo[ MASK_GROUPS_N];

/************************************************************************************
 * Process reference image and display it
 *
 * Check for contours in the reference image and set contour bit in alpha mask.
 *
 * return:    0  OK
 *         else  Error
 */

static int RefImgProcessAndDisp( int iToolData, Fl_RGB_Image *pImgInp,
                                 int NoAlphaDisplay,            // Type of alpha channel usage
                                 int DisplayOnly,               // If this is set do no contour processing
                                 int ClearCheckMasks)           // If this is set clear check mask bits
{
  int ierr;
  YaIPS_ToolData_info_t *pToolData;
  char TempFileName[ 256 + 16];
  Fl_RGB_Image *pIntermediateImage = NULL;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

  // Check parameter

  if( pImgInp == NULL) {          // Must have an input image

    return( -1);
  }

  // Copy input image as reference image, also sets contour bit in alpha

  if( DisplayOnly == false)  {       // Preprocess reference image

    ierr = YaIPS_RGB_Contour( &pIntermediateImage, pImgInp, pToolData->ContourPlaneThres,
                              YAIPS_RGB_CONTOUR_LP_ALPHA, ClearCheckMasks ? NULL : pToolData->YaIPS_ImageDisp.pImage_Img);

    if( pIntermediateImage == NULL) {    // Security test, have no image

      return( -2);
    }

    if( ierr != 0)    {                  // YaIPS_RGB_Contour() has an error

      pIntermediateImage->release();     // Release image data

      return( ierr);
    }

    // Have a preprocessed image

    pImgInp = pIntermediateImage;       // Set for later use
  }

  // Load the image to the display
  YaIPS_ImageDispUpdateByNewImage( &pToolData->YaIPS_ImageDisp, pImgInp,
                                   MY_WIN_ID + iToolData, TempFileName, true, true, NoAlphaDisplay);    // No alpha display

  if( pToolData->YaIPS_ImageDisp.pImage_Img != NULL) {   // Have a latched image

    // Check an image display for size change and redisplay if size has changed.
    YaIPS_ImageDispDrawUpdate( &pToolData->YaIPS_ImageDisp, true);

    if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {   // and display this on the big image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                       MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName,    // Load the image to the display
                                       true, false, NoAlphaDisplay);    // No alpha display
    }
  }

  if( pIntermediateImage != NULL) {     // Have the intermediate image used

    pIntermediateImage->release();      // Release image data
  }

  return( 0);    // Return OK
}

/************************************************************************************
 * Process reference image and display it
 *
 * Check for contours in the reference image and set contour bit in alpha mask.
 *
 * return:    0  OK
 *         else  Error
 */

static int RefImgBackupToFile( int iToolData)
{
  char TempFileName[ 256 + 16];
  YaIPS_ToolData_info_t *pToolData;

  pToolData = YaIPS_ToolData_info + iToolData;

  // Check for image

  if( pToolData->YaIPS_ImageDisp.pImage_Img == NULL) {   // Have no image

    return( -1);
  }

  // Construct a file name for the raw reference image

  sprintf( TempFileName, "%s/Reference+Mask-%d.png", YaIPS_CLIPBOARD_PATH, (int)iToolData + 1);

  IqeB_FileNormalizePathChars( TempFileName);

  YaIPS_Image_Write_PNG( TempFileName, pToolData->YaIPS_ImageDisp.pImage_Img);

  pToolData->ImageHasChanged = false;               // Reset image has changed

  return( 0);            // Return OK
}

/************************************************************************************
 * My_G2D_init
 *
 * Setup G2D graphic before each call
 *
 * return:    0  OK
 *         else  Error
 */

static int My_G2D_init( YaIPS_ToolData_info_t *pToolData, int iMask)
{
  int ierr, Mask;

  Mask = -1;             // Enable all

  if( pToolData->YaIPS_ImageDisp.pImage_Img != NULL) {

    if( pToolData->YaIPS_ImageDisp.pImage_Img->d() == 2) {

      Mask = YAIPS_REFMASK_CHECK_MASK_1 << (8 + iMask);

    } else if( pToolData->YaIPS_ImageDisp.pImage_Img->d() == 4) {

      Mask = YAIPS_REFMASK_CHECK_MASK_1 << (24 + iMask);
    }
  }

  ierr = YaIPS_G2D_init( pToolData->YaIPS_ImageDisp.pImage_Img, Mask, &pToolData->UnDoFIFO, 16);

  return( ierr);
}

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{
  int i, RedrawOnExit, New_Tab_Group, iMask, NoAlphaDisplay;

  RedrawOnExit = false;

  // Get last selected tab group

  New_Tab_Group = pTab_Groups->GetTabGroup();

  if( pToolDataParam->Tab_Group_Selected != New_Tab_Group) {   // Tab group has changed

    // Release undo buffer

    My_G2D_init( pToolDataParam, 0);      // Must set current 2D graphic environment. Can use any iMask argument.

    YaIPS_G2D_end();                 // Frees optional undo buffers

    // Change selected tab group

    pToolDataParam->Tab_Group_Selected = New_Tab_Group;

    // Initial setup of undo FIFO

    if( pToolDataParam->Tab_Group_Selected >= YAIPS_SELECTION_MASK_1 &&   // Tab group 'Mask'
        pToolDataParam->Tab_Group_Selected <= YAIPS_SELECTION_MASK_3) {

      iMask = pToolDataParam->Tab_Group_Selected - YAIPS_SELECTION_MASK_1;

      memset( &pToolDataParam->UnDoFIFO, 0, sizeof( YaIPS_G2D_UnDo_Buffer_t)); // Zero undo FIFO data

      My_G2D_init( pToolDataParam, iMask);    // Must set current 2D graphic environment

      YaIPS_G2D_UnDo_Save();

      // hack to visualize contour
      NoAlphaDisplay = YAIPS_NOALPHADISP_MASK_1 + pToolDataParam->Tab_Group_Selected  - YAIPS_SELECTION_MASK_1;

    } else {   // Must be contour

      NoAlphaDisplay = YAIPS_NOALPHADISP_CONTOUR;          // hack to visualize contour
    }

    if( pToolDataParam->YaIPS_ImageDisp.NoAlphaDisplay != NoAlphaDisplay ||
        (NoAlphaDisplay >= YAIPS_NOALPHADISP_MASK_1 && NoAlphaDisplay <= YAIPS_NOALPHADISP_MASK_3)) { // 'Mask' want update on each update request

      pToolDataParam->YaIPS_ImageDisp.NoAlphaDisplay = NoAlphaDisplay;

      YaIPS_ImageDispDrawUpdate( &pToolDataParam->YaIPS_ImageDisp, true);
    }


    // Copy to display images
    if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolDataParam->iToolData) {        // and this tool window is selected

      // Show also on big image
      if( YaIPS_BigImageDisp.NoAlphaDisplay != NoAlphaDisplay ||
          (NoAlphaDisplay >= YAIPS_NOALPHADISP_MASK_1 && NoAlphaDisplay <= YAIPS_NOALPHADISP_MASK_3)) {  // 'Mask' want update on each update request

        YaIPS_BigImageDisp.NoAlphaDisplay = NoAlphaDisplay;                               // 2 = hack to visualize contour

        YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolDataParam->YaIPS_ImageDisp.pImage_Img,
                                         MY_WIN_ID + pToolDataParam->iToolData, pToolDataParam->YaIPS_ImageDisp.FileName,   // Load the image to the display
                                         true, false, NoAlphaDisplay);   // Alpha display depends from open parameter dialog
      }
    }

    RedrawOnExit = true;                                 // Redraw on exit
  }

  // One of the mask tab groups selected

  if( New_Tab_Group >= YAIPS_SELECTION_MASK_1 && New_Tab_Group <= YAIPS_SELECTION_MASK_3) {

    iMask = New_Tab_Group - YAIPS_SELECTION_MASK_1;

    // Check for color of the mask draw buttons

    for( i = 0; i < MASK_DRAW_N_BUTTONS; i++) {

      Fl_Color TempCol, SetCol;

      TempCol = pBut_MaskDrawMode[ iMask][ i]->color();

      if( pToolDataParam->MaskDrawMode[ iMask] == i) {

        SetCol = YAIPS_COLOR_SELECTION;
      } else {

        SetCol = fl_rgb_color( YAIPS_COLOR_MAIN_BGND);
      }

      if( TempCol != SetCol) {

        pBut_MaskDrawMode[ iMask][ i]->color( SetCol);
        pBut_MaskDrawMode[ iMask][ i]->redraw();
      }
    }

    // Undo and redo enables

    My_G2D_init( pToolDataParam, iMask);

    i = YaIPS_G2D_UnDo_Enables();

    IqeB_GUI_WidgetActivate( pBut_Redo[ iMask], i >= 0 && (i & 0x01) != 0);

    IqeB_GUI_WidgetActivate( pBut_Undo[ iMask], i >= 0 && (i & 0x02) != 0);
  }

  // Redraw

  if( RedrawOnExit) {                                    // Redraw on exit

    pToolDataParam->Input1_Change = 0;                        // Reset image change check
  }
}

/************************************************************************************
 * IqeB_GUI_ToolsMyIdleAction
 */

static void IqeB_GUI_ParIdleAction( void *)
{
  unsigned int TimeTemp;
  int iToolData;
  static unsigned int TimeLastCalled_5000 = 0;
  static unsigned int TimeLastCalled_10 = 0;

  TimeTemp = GetTickCount();           // Get current time

  //

  if( pMyParWin == NULL) {     // Security test, parameter window must exist

    return;
  }

  //
  // some timed actions (not each call)
  //

  iToolData = YaIPS_ToolData_info - pToolDataParam;

  if( TimeTemp - TimeLastCalled_10 >= 10) {     // 10 ms gone since last call

    TimeLastCalled_10 = TimeTemp;              // Remember last time called

    // some time gone, do ...
    // Periodically updates on GUI
    MyParWinUpdate();
  }

  if( TimeTemp - TimeLastCalled_5000 >= 5000) {  // 5000 ms gone since last call

    TimeLastCalled_5000 = TimeTemp;             // Remember last time called

    // some time gone, do ...

    // update window size

    //x/MyWinSizeX = pMyToolWin->w();  // update window size
    //x/MyWinSizeY = pMyToolWin->h();  // update window size

    // Backup change reference image

    if( pToolDataParam->ImageHasChanged) {

      RefImgBackupToFile( iToolData);
    }

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
  int iToolData;

#ifdef USE_NON_MODAL_PAR_DIALOGS // Have none modal parameter dialogs
  IqeB_GUI_ToolParCheckReset();   // The one and only open tool window parameter dialog is closed
#endif

  pToolDataParam->MyParPosX = pMyParWin->x();
  pToolDataParam->MyParPosY = pMyParWin->y();

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::remove_idle( IqeB_GUI_ParIdleAction);      // Redraw window during idle
#endif
  Fl::remove_check( IqeB_GUI_ParIdleAction);     // Check small image size change

  iToolData = YaIPS_ToolData_info - pToolDataParam;

  // Backup change reference image

  if( pToolDataParam->ImageHasChanged) {

    RefImgBackupToFile( iToolData);
  }

  // Release undo buffer

  My_G2D_init( pToolDataParam, 0);      // Must set current 2D graphic environment. Can use any iMask argument.

  YaIPS_G2D_end();                 // Frees optional undo buffers

  // On exit refresh display without 'hack to visualize contour'

  pToolDataParam->YaIPS_ImageDisp.NoAlphaDisplay  = true;                      // Set no alpha display

  YaIPS_ImageDispDrawUpdate( &pToolDataParam->YaIPS_ImageDisp, true);

  // Copy to display images
  if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {        // and this tool window is selected

    // Show also on big image
    YaIPS_BigImageDisp.NoAlphaDisplay  = true;                            // Set no alpha display

    YaIPS_ImageDispDrawUpdate( &YaIPS_BigImageDisp, true);
  }

  pToolDataParam->YaIPS_ImageDisp.pImage_Box->MouseTeachState = 0;  // Teach mode NOT available for current settings.

  // ...

  IqeB_GUI_CloseToolWindow( (void **)&pMyParWin);
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

  pToolDataParam->Input1_Change = 0;                    // Force recalculation output
}
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

      pThis->SetValue( Value);
    }
  }

  if( *pValue != Value) {                  // Value has changed

    *pValue = Value;                      // update the variable

    // ...

    if( pValue == &pToolDataParam->ContourPlaneThres) {    // Was the plane threshold

      Fl_RGB_Image *pTempImage = NULL;
      char TempFileName[ 256 + 16];
      int iToolData;

      iToolData = YaIPS_ToolData_info - pToolDataParam;

      // Construct a file name for the last reference image

      sprintf( TempFileName, "%s/Reference-%d.png", YaIPS_CLIPBOARD_PATH, iToolData + 1);

      IqeB_FileNormalizePathChars( TempFileName);

      // load file

      pTempImage = YaIPS_Image_Read( TempFileName);   // Try to load an image

      if( pTempImage == NULL) {           // Got NO image

        goto ExitPoint;                   // panic exit
      }

      // Process the reference image and display it

      RefImgProcessAndDisp( iToolData, pTempImage, YAIPS_NOALPHADISP_CONTOUR, false, false);  // Preprocess new image and display

      RefImgBackupToFile( iToolData);     // Backup reference image + mask to file

 ExitPoint:

      pToolDataParam->Input1_Change = 0;         // Force recalculation output

      if( pTempImage != NULL) {

        pTempImage->release();                       // Release temporary image
      }
    }
  }
}

/************************************************************************************
 * IqeB_GUI_CBox_SetValue_Callback
 */

#ifdef use_again
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

  pToolDataParam->Input1_Change = 0;          // Force recalculation output
}
#endif

/************************************************************************************
 * IqeB_GUI_Misc_SetValue_Callback
 *
 * This is usable for Fl_Valuator, Fl_Choice, Fl_Check_Button
 */

static void IqeB_GUI_Misc_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  int i, RedrawOnExit, New_Tab_Group, iMask;
  //x/int *pValue;

  //x/pValue = (int *)pValueArg;             // get pointer to associated variable

  if( w == NULL) {                       // security test

    return;
  }

  RedrawOnExit = false;

  // One of the mask tab groups selected

  New_Tab_Group = pTab_Groups->GetTabGroup();

  if( New_Tab_Group >= YAIPS_SELECTION_MASK_1 && New_Tab_Group <= YAIPS_SELECTION_MASK_3) {

    iMask = New_Tab_Group - YAIPS_SELECTION_MASK_1;

    if( pValueArg == &pToolDataParam->MaskAutoMode[ iMask]) {

      // Fl_Choice

      Fl_Choice *pThis;
      int *pValue, NewValue;

      pThis  = (Fl_Choice *)w;
      pValue = (int *)pValueArg;               // get pointer to associated variable

      NewValue = pThis->value();               // Get new value

      *pValue = NewValue;
    }

    // Check for one of the mask draw buttons

    for( i = 0; i < MASK_DRAW_N_BUTTONS; i++) {

      if( w == pBut_MaskDrawMode[ iMask][ i]) {

        if( pToolDataParam->MaskDrawMode[ iMask] != i) {

          pToolDataParam->MaskDrawMode[ iMask] = i;

          pToolDataParam->MaskDrawMouseActive = 0;        // No active mouse action

          RedrawOnExit = true;
        }

        goto ExitPoint;
      }
    }
  }

  // Redraw

ExitPoint:

  if( RedrawOnExit) {                                    // Redraw on exit

    pToolDataParam->Input1_Change = 0;                        // Reset image change check
  }
}

/************************************************************************************
 * IqeB_GUI_MaskAction_Callback
 *
 * One of the other mask action buttons is pressed
 */

#define MASK_ACTION_UNDO      0    // Undo former draw
#define MASK_ACTION_REDO      1    // Redo former draw
#define MASK_ACTION_CLEAR     2    // Clears the mask
#define MASK_ACTION_DILAT     3    // The mask grows 1 pixel in all directions
#define MASK_ACTION_EROS      4    // The mask shrinks 1 pixel in all directions
#define MASK_ACTION_BINARIZE  5    // Binarize image to mask

static void IqeB_GUI_MaskActtion_Callback( Fl_Widget *w, void *pValueArg)
{
  int ierr, RedrawOnExit, New_Tab_Group, iMask;
  int Value;

  Value = (long long)pValueArg;          // Expected argument is a number

  if( w == NULL) {                       // security test

    return;
  }

  RedrawOnExit = false;

  // One of the mask tab groups selected

  New_Tab_Group = pTab_Groups->GetTabGroup();

  if( New_Tab_Group < YAIPS_SELECTION_MASK_1 || New_Tab_Group > YAIPS_SELECTION_MASK_3) {

    // No mask tab group selected
    return;
  }

  iMask = New_Tab_Group - YAIPS_SELECTION_MASK_1;

  // Init 2D graphic

  ierr = My_G2D_init( pToolDataParam, iMask);

  if(ierr < 0) {    // Error on initialize

    return;         // Return
  }

  // What action to do ?

  switch( Value) {

  case MASK_ACTION_UNDO:    // Undo former draw

    YaIPS_G2D_UnDo_NextPrev( -1);

    RedrawOnExit = true;

    break;

  case MASK_ACTION_REDO:    // Redo former draw

    YaIPS_G2D_UnDo_NextPrev( +1);

    RedrawOnExit = true;

    break;

  case MASK_ACTION_CLEAR:   // Clears the mask

    YaIPS_G2D_Rectangle( 0, 0, pToolDataParam->YaIPS_ImageDisp.BigImage_iw, pToolDataParam->YaIPS_ImageDisp.BigImage_iw, 0, true);

    YaIPS_G2D_UnDo_Save();

    RedrawOnExit = true;

    break;

  case MASK_ACTION_DILAT:   // The mask grows 1 pixel in all directions

    YaIPS_G2D_Morphology( YAIPS_RGB_MORPH_DILATION);

    YaIPS_G2D_UnDo_Save();

    RedrawOnExit = true;

    break;

  case MASK_ACTION_EROS:    // The mask shrinks 1 pixel in all directions.

    YaIPS_G2D_Morphology( YAIPS_RGB_MORPH_EROSION);

    YaIPS_G2D_UnDo_Save();

    RedrawOnExit = true;

    break;

  case MASK_ACTION_BINARIZE: // Binarize image to mask

    if( pToolDataParam->YaIPS_ImageDisp.pImage_Img != NULL &&
        ( pToolDataParam->YaIPS_ImageDisp.pImage_Img->d() == 2 ||
          pToolDataParam->YaIPS_ImageDisp.pImage_Img->d() == 4)) {

      int x, y, Tmp, MaskAutoMode, MaskAutoThres, SkipMask;;
      uchar *pPix;
      YaIPS_RGB_ImgD_t ImgD;

      // Binarize the image to mask

      MaskAutoMode  = pToolDataParam->MaskAutoMode[ iMask];
      MaskAutoThres = pToolDataParam->MaskAutoThres[ iMask];

      SkipMask = YAIPS_REFMASK_CHECK_MASK_1 << iMask;

      ierr = YaIPS_RGB_to_ImgD( pToolDataParam->YaIPS_ImageDisp.pImage_Img, &ImgD);

      for( y = 0; y < ImgD.yy; y++) {

        pPix = RGB_pixad( 0, y, &ImgD);

        if( pToolDataParam->YaIPS_ImageDisp.pImage_Img->d() == 2) {    // Black white image

          if( MaskAutoMode == 0) {                                // Mode bright

            for( x = 0; x < ImgD.xx; x++) {

              if( (int)pPix[ 0] >= MaskAutoThres) {

                pPix[ 1] |= SkipMask;
              } else {

                pPix[ 1] &= ~ SkipMask;
              }

              pPix += ImgD.d;
            }

          } else {                                                // Mode dark

            for( x = 0; x < ImgD.xx; x++) {

              if( (int)pPix[ 0] < MaskAutoThres) {

                pPix[ 1] |= SkipMask;
              } else {

                pPix[ 1] &= ~ SkipMask;
              }

              pPix += ImgD.d;
            }
          }

        } else {                                                   // Color image

          if( MaskAutoMode == 0) {                                 // Mode bright

            for( x = 0; x < ImgD.xx; x++) {

              // A fast black white conversion. Intensity part of an IHS conversion.
              Tmp = (pPix[ 0] * 76 + pPix[ 1] * 150 + pPix[ 2] * 30) >> 8;

              if( Tmp >= MaskAutoThres) {

                pPix[ 3] |= SkipMask;
              } else {

                pPix[ 3] &= ~ SkipMask;
              }

              pPix += ImgD.d;
            }

          } else {                                                // Mode dark

            for( x = 0; x < ImgD.xx; x++) {

              // A fast black white conversion. Intensity part of an IHS conversion.
              Tmp = (pPix[ 0] * 76 + pPix[ 1] * 150 + pPix[ 2] * 30) >> 8;

              if( Tmp < MaskAutoThres) {

                pPix[ 3] |= SkipMask;
              } else {

                pPix[ 3] &= ~ SkipMask;
              }

              pPix += ImgD.d;
            }
          }
        }
      }

      YaIPS_G2D_UnDo_Save();

      RedrawOnExit = true;
    }
  }


  // Redraw

//x/ExitPoint:

  if( RedrawOnExit) {                                    // Redraw on exit

    pToolDataParam->ImageHasChanged = true;                   // Reference image has changed

    pToolDataParam->Input1_Change = 0;                        // Reset image change check
  }
}

/************************************************************************************
 * YaIPS_Brusch_Callback
 *
 * Filter will change
 */

static void YaIPS_Brusch_Callback( Fl_Widget *w, void *data)
{
  int Value, New_Tab_Group, iMask;

  // One of the mask tab groups selected

  New_Tab_Group = pTab_Groups->GetTabGroup();

  if( New_Tab_Group < YAIPS_SELECTION_MASK_1 || New_Tab_Group > YAIPS_SELECTION_MASK_3) {

    // No mask tab group selected
    return;
  }

  iMask = New_Tab_Group - YAIPS_SELECTION_MASK_1;

  // ...

  Value = (long long)(data);                       // get value to set

  if( pToolDataParam->MaskBrush[ iMask] == Value) {     // Value will not change

    return;                                        // Exit, nothing to do
  }

  pToolDataParam->MaskBrush[ iMask] = Value;            // Set new value

  //x/ToolData->Input1_Change = 0;                // Force recalculation output
}

/************************************************************************************
 * YaIPS_MaskColor_Callback
 *
 * Filter will change
 */

static void YaIPS_MaskColor_Callback( Fl_Widget *w, void *data)
{
  int Value, New_Tab_Group, iMask;

  // One of the mask tab groups selected

  New_Tab_Group = pTab_Groups->GetTabGroup();

  if( New_Tab_Group < YAIPS_SELECTION_MASK_1 || New_Tab_Group > YAIPS_SELECTION_MASK_3) {

    // No mask tab group selected
    return;
  }

  iMask = New_Tab_Group - YAIPS_SELECTION_MASK_1;

  // ...

  Value = (long long)(data);                       // get value to set

  if( pToolDataParam->MaskColor[ iMask] == Value) {             // Value will not change

    return;                                        // Exit, nothing to do
  }

  pToolDataParam->MaskColor[ iMask] = Value;                    // Set new value

  //x/ToolData->Input1_Change = 0;                    // Force recalculation output
}

/************************************************************************************
 * YaIPS_GUI_ParameterWin
 *
 * Open parameter dialog
 *
 */

static void YaIPS_GUI_ParameterWin( int xLeft, int yTop, int iToolData)
{
  int xPos, yPos, iMask;
  Fl_Double_Window *pMyToolWin;
  char TempString[ 256];

  // Check parameter dialog is allowed to open

#ifdef USE_NON_MODAL_PAR_DIALOGS // Have none modal parameter dialogs
  if( IqeB_GUI_ToolParCheckOpen( close_Par_cb, &pMyParWin, iToolData) < 0) {    // Allowed to open parameter dialog

    return;
  }
#endif

  // Get pointer to tool data

  pToolDataParam = YaIPS_ToolData_info + iToolData;               // Point to info data

  //
  // creation of window on first call
  //

  xPos = xLeft;
  yPos = yTop;

  if( pToolDataParam->MyParPosX != IQE_GUI_NO_WINPOS_X && pToolDataParam->MyParPosY != IQE_GUI_NO_WINPOS_Y) { // have last window position

    xPos = pToolDataParam->MyParPosX;
    yPos = pToolDataParam->MyParPosY;
  }

  pMyParWin = new Fl_Window( xPos, yPos, 297 /*IQE_GUI_TOOLS_STD_WITDH*/, 182);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  // Set window title
#ifdef USE_NON_MODAL_PAR_DIALOGS // Have none modal parameter dialogs

   pMyToolWin = (Fl_Double_Window *)pToolDataParam->pMyToolWin;  // Convert type of pointer

   pMyParWin->copy_label( pMyToolWin->label());
#else
   pMyParWin->label( LANGDEF_SETTINGS);
#endif

  // Initial setup of undo FIFO

  if( pToolDataParam->Tab_Group_Selected >= YAIPS_SELECTION_MASK_1 &&   // Tab group 'Mask'
      pToolDataParam->Tab_Group_Selected <= YAIPS_SELECTION_MASK_3) {

    iMask = pToolDataParam->Tab_Group_Selected - YAIPS_SELECTION_MASK_1;

    memset( &pToolDataParam->UnDoFIFO, 0, sizeof( YaIPS_G2D_UnDo_Buffer_t)); // Zero undo FIFO data

    My_G2D_init( pToolDataParam, iMask);    // Must set current 2D graphic environment

    YaIPS_G2D_UnDo_Save();
  }

  //
  //  GUI things
  //

  int x1, y, yy, xx2;
  //x/int xx1, xc;
  int yGroup, xSave1, xSave2, ySave;
  //x/char TempBuffer[ 256];

  //x/Fl_Check_Button *pCheckTemp;
  //x/Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  //x/IqeFl_Float_Input  *pFloatTemp;
  IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group, *pTemp_Group2;
  Fl_Button       *pTemp_Button;
  Fl_Choice       *pTemp_Choice;
  Fl_Radio_Round_Button *pRadioButTemp;

  //x/x1  = 4;
  //x/xx1 = pMyParWin->w() - 16;
  //x/xx2 = xx1 / 2;
  //x/xc  = pMyToolWin->w() / 2;          // x center

  yy  = 20;
  y = 4;
  x1  = 4;

  //
  // Tabs
  //

  pTemp_Tabs = new IqeFl_Tabs( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4);
  pTemp_Tabs->selection_color( YAIPS_COLOR_SELECTION);
  pTab_Groups = pTemp_Tabs;

  y += 26;

  //
  // Group 'Edges'
  //

  yGroup = y;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LANGDEF_EDGES);
  pTemp_Group->tooltip( LangStringLookup( "&GUI_InspRefImg_TabA1a="
                                          "Edge mask.\n"
                                          "Used to mark edges. These are processed\n"
                                          "differently during image comparison."));

    y += 8;

    x1 = 4;

    x1 += 120;
    xx2 = 32;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_InspRefImg_TabA2=Plane threshold"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_InspRefImg_TabA2a="
                                          "Plane threshold.\n"
                                          "Used to prepare the reference image.\n"
                                          "Pixel values below this threshold are\n"
                                          "lowpass filtered. The contour pixels are\n"
                                          "not changed. Also sets a contour bit in\n"
                                          "the alpha part of the reference image."
                                          "This contour bit is used by the image"
                                          "compare window."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolDataParam->ContourPlaneThres);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolDataParam->ContourPlaneThres);
    pTemp_Int->SetModifyData( 8, 255, 8, 1);

    // Finish things for this group

    pTemp_Group->end();

    //
    // Group 'masks'
    //

    for( iMask = 0; iMask < MASK_GROUPS_N; iMask++) {

      y = yGroup;
      x1  = 4;

      sprintf( TempString, "%s %d", LangStringLookup( "&GUI_InspRefImg_TabB1=Mask"), iMask + 1);

      pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4);
      pTemp_Group->copy_label( TempString);
      pTemp_Group->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB1a=Editor to define check regions."));

      y += 8;

      x1 = 4 + 4;

      ySave = y;

      xx2 = 80;

      pTemp_Group2 = new Fl_Group( x1, y, xx2 + 8, 16 + (yy + 4) * 5, LangStringLookup( "&GUI_InspRefImg_TabB2=Draw functions"));
      pTemp_Group2->box( FL_THIN_DOWN_BOX);
      pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT);
      pTemp_Group2->labelsize( 11);

        x1 += 4;
        y += 16;

        pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_InspRefImg_TabB10=Move"));
        pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, NULL);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB10a="
                                                 "Move image with mouse.\n"
                                                 "Left mouse button pressed: Move image"));
        pBut_MaskDrawMode[ iMask][ MASK_DRAW_MOVE_IMAGE] = pTemp_Button;

        y += yy + 4;

        pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_InspRefImg_TabB12=Rectangle"));
        pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, NULL);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB12a="
                                               "Draw a rectangle.\n"
                                               "Left mouse button pressed: Change size\n"
                                               "Left + right Mouse button pressed: Move\n"
                                               "Right mouse button pressed: Move image"));
        pBut_MaskDrawMode[ iMask][ MASK_DRAW_RECTGANGLE] = pTemp_Button;

        y += yy + 4;

        pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_InspRefImg_TabB13=Ellipse"));
        pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, NULL);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB13a="
                                               "Draw an ellipse.\n"
                                               "Left mouse button pressed: Change size\n"
                                               "Left + right Mouse button pressed: Move\n"
                                               "Right mouse button pressed: Move image"));
        pBut_MaskDrawMode[ iMask][ MASK_DRAW_ELLIPSE] = pTemp_Button;

        y += yy + 4;

        pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_InspRefImg_TabB14=Polygon"));
        pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, NULL);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB14a="
                                                 "Draw a polygon.\n"
                                                 "Left mouse button click: Set point\n"
                                                 "Right Mouse button click: Abort\n"
                                                 "Right mouse button pressed: Move image"));
        pBut_MaskDrawMode[ iMask][ MASK_DRAW_POLYGON] = pTemp_Button;

        y += yy + 4;

        pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_InspRefImg_TabB15=Fill"));
        pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, NULL);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB15a="
                                                 "Fill area.\n"
                                                 "Click left mouse button in the area to be filled.\n"
                                                 "Right mouse button pressed: Move image"));
        pBut_MaskDrawMode[ iMask][ MASK_DRAW_FILL] = pTemp_Button;

      // Finish things for brush group

      pTemp_Group2->end();

      y = ySave;
      x1 += xx2 + 7;

      xSave1 = x1;

      // Group around brushes

      pTemp_Group2 = new Fl_Group( x1, y, pMyParWin->w() - x1 - 8, yy + yy + 9, LangStringLookup( "&GUI_InspRefImg_TabB3=Brushes"));
      pTemp_Group2->box( FL_THIN_DOWN_BOX);
      pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT);
      pTemp_Group2->labelsize( 11);

        x1 += 3;
        y += 4;

        pTemp_Button = new Fl_Button( x1, y + yy, xx2, yy, LangStringLookup( "&GUI_InspRefImg_TabB11=Freehand"));
        pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, NULL);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB11a="
                                                 "Draw free hand."));
        pBut_MaskDrawMode[ iMask][ MASK_DRAW_FREEHAND] = pTemp_Button;

        x1 += xx2 + 4;

        xSave2 = x1;

        xx2 = 34;

        pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "1");
        pRadioButTemp->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB20a=Freehand brush width"));
        pRadioButTemp->value( pToolDataParam->MaskBrush[ iMask] == 1);
        pRadioButTemp->callback( YaIPS_Brusch_Callback, (void *)1);

        x1 += xx2 - 2;

        pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "3");
        pRadioButTemp->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB20a=Freehand brush width"));
        pRadioButTemp->value( pToolDataParam->MaskBrush[ iMask] == 3);
        pRadioButTemp->callback( YaIPS_Brusch_Callback, (void *)3);

        x1 += xx2 - 2;

        pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "5");
        pRadioButTemp->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB20a=Freehand brush width"));
        pRadioButTemp->value( pToolDataParam->MaskBrush[ iMask] == 5);
        pRadioButTemp->callback( YaIPS_Brusch_Callback, (void *)5);

        x1 = xSave2;
        y += yy + 4;

        pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "7");
        pRadioButTemp->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB20a=Freehand brush width"));
        pRadioButTemp->value( pToolDataParam->MaskBrush[ iMask] == 7);
        pRadioButTemp->callback( YaIPS_Brusch_Callback, (void *)7);

        x1 += xx2 - 2;

        pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, "9");
        pRadioButTemp->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB20a=Freehand brush width"));
        pRadioButTemp->value( pToolDataParam->MaskBrush[ iMask] == 9);
        pRadioButTemp->callback( YaIPS_Brusch_Callback, (void *)9);

        x1 += xx2 - 2;

        pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 + 3, yy, "11");
        pRadioButTemp->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB20a=Freehand brush width"));
        pRadioButTemp->value( pToolDataParam->MaskBrush[ iMask] == 11);
        pRadioButTemp->callback( YaIPS_Brusch_Callback, (void *)11);

      // Finish things for brush group

      pTemp_Group2->end();

      y += yy + 4;
      x1 = xSave1;

      // Group around mask color

      pTemp_Group2 = new Fl_Group( x1, y, pMyParWin->w() - x1 - 8, yy + 6, LangStringLookup( "&GUI_InspRefImg_TabB4=Mask"));
      pTemp_Group2->box( FL_THIN_DOWN_BOX);
      pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT);
      pTemp_Group2->labelsize( 11);

        y += 3;
        x1 += 37;

        xx2 = 77;

        pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_InspRefImg_TabB30=Draw"));
        pRadioButTemp->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB30a=Set mask bits"));
        pRadioButTemp->value( pToolDataParam->MaskColor[ iMask] == 1);
        pRadioButTemp->callback( YaIPS_MaskColor_Callback, (void *)1);

        x1 += xx2 - 2;

        pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_InspRefImg_TabB31=Erase"));
        pRadioButTemp->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB31a=Reset mask bits"));
        pRadioButTemp->value( pToolDataParam->MaskColor[ iMask] == 0);
        pRadioButTemp->callback( YaIPS_MaskColor_Callback, (void *)0);

      // Finish things for mask color group

      pTemp_Group2->end();

      y += pTemp_Group2->h();
      x1 = xSave1;

      // Group around other actions

      pTemp_Group2 = new Fl_Group( x1, y, pMyParWin->w() - x1 - 8, yy + 6 /*, LangStringLookup( "&GUI_InspRefImg_TabB5=Other") */);
      pTemp_Group2->box( FL_THIN_DOWN_BOX);
      pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT);
      pTemp_Group2->labelsize( 11);

        y += 3;
        x1 += 3;

        xx2 = 23;

        pTemp_Button = new Fl_Button( x1, y, xx2, yy, "<");
        pTemp_Button->callback( IqeB_GUI_MaskActtion_Callback, (void *)MASK_ACTION_UNDO);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB40a="
                                                 "Undo former draw."));
        pBut_Undo[ iMask] = pTemp_Button;

        x1 += xx2 + 2;

        pTemp_Button = new Fl_Button( x1, y, xx2, yy, ">");
        pTemp_Button->callback( IqeB_GUI_MaskActtion_Callback, (void *)MASK_ACTION_REDO);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB41a="
                                                 "Redo former draw."));
        pBut_Redo[ iMask] = pTemp_Button;

        x1 += xx2 + 2;

        xx2 = 43;

        pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_InspRefImg_TabB42=Clear"));
        pTemp_Button->callback( IqeB_GUI_MaskActtion_Callback, (void *)MASK_ACTION_CLEAR);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB42a="
                                                 "Clears the mask."));
        x1 += xx2 + 2;

        pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_InspRefImg_TabB43=Dilat."));
        pTemp_Button->callback( IqeB_GUI_MaskActtion_Callback, (void *)MASK_ACTION_DILAT);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB43a="
                                                 "The mask grows 1 pixel in all directions."));
        x1 += xx2 + 2;

        pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_InspRefImg_TabB44=Eros."));
        pTemp_Button->callback( IqeB_GUI_MaskActtion_Callback, (void *)MASK_ACTION_EROS);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB44a="
                                                 "The mask shrinks 1 pixel in all directions."));

      // Finish things for other actions group

      pTemp_Group2->end();

      y += pTemp_Group2->h();
      x1 = xSave1;

      // Group around other actions

      pTemp_Group2 = new Fl_Group( x1, y, pMyParWin->w() - x1 - 8, yy + 6 /*, LangStringLookup( "&GUI_InspRefImg_TabB6=Bin.")*/);
      pTemp_Int->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB6a=Binarize image to mask."));
      pTemp_Group2->box( FL_THIN_DOWN_BOX);
      pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT);
      pTemp_Group2->labelsize( 11);

        y += 3;
        x1 += 3;

        x1 += 36;

        xx2 = 32;

        pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_InspRefImg_TabB50=Thr."));
        pTemp_Int->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB50a="
                                              "Brightness threshold.\n"
                                              "Binarize image to mask.\n"
                                              "If 'Bright' is selected, all pixels brighter\n"
                                              "then threshold are set as mask pixels.\n"
                                              "If 'Dark' is selected, all pixels below\n"
                                              "threshold are taken, respectively.\n"));
        pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
        pTemp_Int->SetValue( pToolDataParam->MaskAutoThres[ iMask]);
        pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, pToolDataParam->MaskAutoThres + iMask);
        pTemp_Int->SetModifyData( 0, 255, 8, 1);

        x1 += xx2 + 2;

        xx2 = 70;

        pTemp_Choice = new Fl_Choice( x1, y, xx2, yy);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB51a="
                                                 "Selection of the binarization mode."));
        pTemp_Choice->callback( IqeB_GUI_Misc_SetValue_Callback, pToolDataParam->MaskAutoMode + iMask);
        pTemp_Choice->menu_box( FL_BORDER_BOX);

        pTemp_Choice->add( LangStringLookup( "&GUI_InspRefImg_TabB51b=Bright"), 0, NULL, (void *)(fl_intptr_t)0);
        pTemp_Choice->add( LangStringLookup( "&GUI_InspRefImg_TabB51C=Dark"), 0, NULL, (void *)(fl_intptr_t)1);

        pTemp_Choice->value( pToolDataParam->MaskAutoMode[ iMask]);

        x1 += xx2 + 2;

        xx2 = 41;

        pTemp_Button = new Fl_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_InspRefImg_TabB52=Apply"));
        pTemp_Button->callback( IqeB_GUI_MaskActtion_Callback, (void *)MASK_ACTION_BINARIZE);
        pTemp_Button->tooltip( LangStringLookup( "&GUI_InspRefImg_TabB52a="
                                                 "Apply binarization."));

      // Finish things for other actions group

      pTemp_Group2->end();

      // Finish things for this group

      pTemp_Group->end();

    } // end for( iMask = 0; ...

    // finish up

    pTemp_Tabs->end();

    pTemp_Tabs->SetTabGroup( pToolDataParam->Tab_Group_Selected);    // Select tab group from last session

  // finish up

  pMyParWin->end();
#ifdef USE_NON_MODAL_PAR_DIALOGS // Have none modal parameter dialogs
  pMyParWin->set_non_modal();
#else
  pMyParWin->set_modal();
#endif
  pMyParWin->callback( close_Par_cb, &pMyParWin);
  pMyParWin->show();

  // Hack: Add close button to window caption
  YaIPS_DialogAddCloseButton( pMyParWin);

  // Add idle action for this window

  #ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::add_idle( IqeB_GUI_ParIdleAction);      // Redraw window during idle
  #endif
  Fl::add_check( IqeB_GUI_ParIdleAction);     // Check small image size change

  // On entry refresh display with 'hack to visualize contour'

  pToolDataParam->Input1_Change = 0;                        // Reset image change check

}

//-----------------------------------------------------------------------------------
// Create a specialized window class for image load and display
//-----------------------------------------------------------------------------------

class CLASS_WIN_TOOL : public Fl_Double_Window {

public:

  int iToolData;                         // Index of info data element, see YaIPS_ToolData_info

  Fl_Button *pGUI_Img_ShowOnBig;         // Show this image on big display
  Fl_Button *pGUI_But_Snap;              // Acquire a reference image from the input
  Fl_Button *pGUI_Img_Load;              // Load an image
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

    // Snap image

    xx = xx0;

    pGUI_But_Snap = new Fl_Button( x1, y, xx, yy, "@+32>");
    pGUI_But_Snap->callback( SnapFromInput_cb, (long int)iToolData);
    pGUI_But_Snap->tooltip( LangStringLookup( "&GUI_InspRefImg_Tools2a="
                            "Apply the reference image from input.\n"
                            "Shortcut: Ctrl+A"));
    pGUI_But_Snap->labelcolor( YAIPS_BCOL_BUTTON);
    pGUI_But_Snap->shortcut( FL_COMMAND + 'a');       // Short cut key

    x1 += xx + 5;


    // Open Setting dialog

     xx = xx0 / 2 - 1;

     pGUI_Img_Load = new Fl_Button( x1, y, xx, xx, "@-2fileopen");
     pGUI_Img_Load->callback( Load_cb, (long int)iToolData);
     pGUI_Img_Load->tooltip( LangStringLookup( "&GUI_InspRefImg_Tools3a=Load reference image from file."));
     pGUI_Img_Load->labelcolor( YAIPS_BCOL_BUTTON);

     pGUI_Parameter = new Fl_Button( x1, y + xx + 2, xx, xx, "@-4menu2");
     pGUI_Parameter->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
     pGUI_Parameter->tooltip( LANGDEF_SETTINGS_POINTS);
     pGUI_Parameter->labelcolor( YAIPS_BCOL_BUTTON);
     pGUI_Parameter->shortcut( FL_COMMAND+'p');       // Short cut key

#ifdef use_again
     pGUI_TeachToggle = new Fl_Button( x1 + xx + 2, y + xx + 2, xx, xx, "@-2pencil");
     pGUI_TeachToggle->callback( IqeB_Camera_GUI_Callback, NULL);
     pGUI_TeachToggle->tooltip( LANGDEF_SWITCH_TEACH_INSPECT);
     pGUI_TeachToggle->labelcolor( YAIPS_BCOL_BUTTON);
     pGUI_TeachToggle->shortcut( FL_COMMAND+'t');       // Short cut key
#endif

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

    pToolData->YaIPS_ImageDisp.NoAlphaDisplay = true;                                   // No alpha display ! Alpha is used for contour mask bit

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

#ifdef USE_NON_MODAL_PAR_DIALOGS // Have none modal parameter dialogs
  IqeB_GUI_ToolParCheckClose( close_Par_cb, &pMyParWin, iToolData);   // Check to close specific associated parameter dialog
#endif

  // ...

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
}

/************************************************************************************
 * SnapFromInput_cb
 *
 * Snap image from input
 */

//
static void SnapFromInput_cb( Fl_Widget *w, long int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  //x/CLASS_WIN_TOOL *pMyToolWin;
  int Input1_Check;
  Fl_RGB_Image *pImgIn1;
  char TempFileName[ 256 + 16];

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  //x/pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  // Check input

  Input1_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr, NULL, &pImgIn1);

  if( Input1_Check != 0 ||               // Have no valid input image
      pImgIn1 == NULL) {                 // or no image pointer

    return;
  }

  if( pToolData->YaIPS_ImageDisp.pImage_Box == NULL) {       // Security test

    return;
  }

  // Construct a file name for the raw reference image

  sprintf( TempFileName, "%s/Reference-%d.png", YaIPS_CLIPBOARD_PATH, (int)iToolData + 1);

  IqeB_FileNormalizePathChars( TempFileName);

  YaIPS_Image_Write_PNG( TempFileName, pImgIn1);

  // Process the reference image and display it

  RefImgProcessAndDisp( (int)iToolData, pImgIn1, true, false, true);  // Preprocess new image and display

  RefImgBackupToFile( iToolData);     // Backup reference image + mask to file

  MyWinUpdate( iToolData, true);      // Update the GUI
}

/************************************************************************************
 * Load_cb
 *
 * Handle a load file request from the button
 */

//
static void Load_cb( Fl_Widget *w, long int iToolData)
{
  Fl_Native_File_Chooser fc;
  YaIPS_ToolData_info_t *pToolData;
  //x/CLASS_WIN_TOOL *pMyToolWin;
  Fl_RGB_Image *pTempImage = NULL;
  char FileFilter[ 1024];
  char TempFileName[ 256 + 16];
  char *pFileName;
  int ierr;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  //x/pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

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

  if( pTempImage == NULL) {           // Got NO image

    goto ExitPoint;                   // panic exit
  }

  // Construct a file name for the raw reference image

  sprintf( TempFileName, "%s/Reference-%d.png", YaIPS_CLIPBOARD_PATH, (int)iToolData + 1);

  IqeB_FileNormalizePathChars( TempFileName);

  YaIPS_Image_Write_PNG( TempFileName, pTempImage);

  // Process the reference image and display it

  ierr = RefImgProcessAndDisp( (int)iToolData, pTempImage, true, false, true);  // Preprocess new image and display

  RefImgBackupToFile( iToolData);     // Backup reference image + mask to file

  pToolData->Input1_WinIdNr = -1;                // Invalidate input window ID to show, that we have loaded from file.

ExitPoint:

  if( pTempImage != NULL) {

    pTempImage->release();                       // Release temporary image
  }

  MyWinUpdate( iToolData, true);                 // Update the GUI
}

/************************************************************************************
 * YaIPS_ToolWin_GUI_Callback
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

    if( pToolData-> YaIPS_ImageDisp.pImage_Img != NULL) {      // Got an image

      int NoAlphaDisplay;

      NoAlphaDisplay = true;                                   // Preset no alpha display

      if( pMyParWin != NULL) {                                 // Parameter dialog is open

        if( pToolData->Tab_Group_Selected == YAIPS_SELECTION_EDGES) {   // Is edges tab

          NoAlphaDisplay = YAIPS_NOALPHADISP_CONTOUR;          // hack to visualize contour

        } else {                                               // Is mask tab

          // hack to visualize contour
          NoAlphaDisplay = YAIPS_NOALPHADISP_MASK_1 + pToolData->Tab_Group_Selected  - YAIPS_SELECTION_MASK_1;
        }
      }

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                      MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName,   // Load the image to the display
                                      true, false, NoAlphaDisplay);   // Alpha display depends from open parameter dialog
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
  int Input1_Check;

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

#ifdef use_again
  if( DoEnable == 2) {                                          // Was periodically updates only

    return;
  }
#endif

  // ...

  // Check input image and visualize state
  Input1_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr, pMyToolWin->pBox_Input1);

  // ...

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_ShowOnBig,
                             DoEnable &&                                    // Enable GUI elements
                             pToolData->YaIPS_ImageDisp.pImage_Img != NULL); // and have an image loaded


  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_But_Snap,
                             DoEnable &&                                // Enable GUI elements
                             (pMyParWin == NULL || pToolDataParam->iToolData != iToolData) && // Parameter dialog is closed
                             Input1_Check == 0);                        // Have a valid input image

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_Load,
                            (pMyParWin == NULL || pToolDataParam->iToolData != iToolData) && // Parameter dialog is closed
                             DoEnable);                             // Enable GUI elements

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Parameter,
                            (pMyParWin == NULL || pToolDataParam->iToolData != iToolData) && // Parameter dialog is closed
                             DoEnable);                             // Enable GUI elements

  // Set mouse teach state to enable mouse callback from main window for mask editing

  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;

  pYaIPS_ImageDisp =  &pToolData->YaIPS_ImageDisp;

  if( pYaIPS_ImageDisp != NULL &&
      pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_AUTO &&  // Any resolution
      ! pYaIPS_ImageDisp->Plot3D_Active &&                                  // and NO 3D plot active
      pMyParWin != NULL &&                                                  // and parameter dialog is open
      pToolData->Tab_Group_Selected >= YAIPS_SELECTION_MASK_1 &&            // and tab group 'Mask'
      pToolData->Tab_Group_Selected <= YAIPS_SELECTION_MASK_3) {

    pToolData->YaIPS_ImageDisp.pImage_Box->MouseTeachState = 3;  // Teach mode enabled

  } else {

    pToolData->YaIPS_ImageDisp.pImage_Box->MouseTeachState = 0;  // Teach mode NOT available for current settings.

  }
}

/************************************************************************************
 * IqeB_GUI_ToolsMyIdleAction
 */

static void IqeB_GUI_ToolsMyIdleAction( void *)
{
  YaIPS_ToolData_info_t *pToolData;
  CLASS_WIN_TOOL *pMyToolWin;
  int iToolData;
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

    if( pToolData->YaIPS_ImageDisp.pImage_Img != NULL) {   // Have a latched image

      if( pToolData->Input1_Change == 0)  {                // Want a update of the displayed image

        int NoAlphaDisplay;

       NoAlphaDisplay = true;                                   // Preset no alpha display

        if( pMyParWin != NULL) {                                 // Parameter dialog is open

          if( pToolData->Tab_Group_Selected == YAIPS_SELECTION_EDGES) {   // Is edges tab

            NoAlphaDisplay = YAIPS_NOALPHADISP_CONTOUR;          // hack to visualize contour

          } else {                                               // Is mask tab

            // hack to visualize contour
            NoAlphaDisplay = YAIPS_NOALPHADISP_MASK_1 + pToolData->Tab_Group_Selected  - YAIPS_SELECTION_MASK_1;
          }
        }

        if( pToolData->YaIPS_ImageDisp.NoAlphaDisplay != NoAlphaDisplay ||
            (NoAlphaDisplay >= YAIPS_NOALPHADISP_MASK_1 && NoAlphaDisplay <= YAIPS_NOALPHADISP_MASK_3)) { // 'Mask' want update on each update request

          pToolData->YaIPS_ImageDisp.NoAlphaDisplay = NoAlphaDisplay;

          YaIPS_ImageDispDrawUpdate( &pToolData->YaIPS_ImageDisp, true);
        }


        // Copy to display images
        if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {        // and this tool window is selected

          // Show also on big image
          if( YaIPS_BigImageDisp.NoAlphaDisplay != NoAlphaDisplay ||
              (NoAlphaDisplay >= YAIPS_NOALPHADISP_MASK_1 && NoAlphaDisplay <= YAIPS_NOALPHADISP_MASK_3)) {  // 'Mask' want update on each update request

            YaIPS_BigImageDisp.NoAlphaDisplay = NoAlphaDisplay;                               // 2 = hack to visualize contour

            YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                             MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName,   // Load the image to the display
                                             true, false, NoAlphaDisplay);   // Alpha display depends from open parameter dialog
          }
        }

        pToolData->Input1_Change = 1;                        // Have updated
      }

    } else {

      // Empty display image
      if( pToolData->YaIPS_ImageDisp.ImageSourceID != YAIPS_WIN_ID_NONE) {  // Not empty until now

        YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);
      }

      pToolData->Input1_Change = 0;                        // Reset image change check
    }


  }  // end: for( iToolData ...
}

/************************************************************************************
 * IqeB_GUI_InspRefImgWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_InspRefImgWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
{
  CLASS_WIN_TOOL *pMyToolWin;
  YaIPS_ToolData_info_t *pToolData;
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

  pToolData->IsOpen = true;                                // Flag image as open

  pToolData->MaskDrawMouseActive = 0;                      // Mask draw is not active

  pToolData->ImageHasChanged = false;                      // Reset image has changed

  memset( &pToolData->UnDoFIFO, 0, sizeof( YaIPS_G2D_UnDo_Buffer_t)); // Zero undo FIFO data

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

  // Restore last reference image

  if( true) {

    Fl_RGB_Image *pTempImage = NULL;
    char TempFileName[ 256 + 16];

    // Check for reference image and mask

    sprintf( TempFileName, "%s/Reference+Mask-%d.png", YaIPS_CLIPBOARD_PATH, (int)iToolData + 1);

    IqeB_FileNormalizePathChars( TempFileName);

    // load file

    pTempImage = YaIPS_Image_Read( TempFileName);   // Try to load an image

    if( pTempImage != NULL) {                       // Got reference image with mask image

      RefImgProcessAndDisp( (int)iToolData, pTempImage, true, true, false);  // Display new image

      goto ExitPoint;                   // Done
    }

    // NO 'reference image and mask', try it with saved reference image

    sprintf( TempFileName, "%s/Reference-%d.png", YaIPS_CLIPBOARD_PATH, (int)iToolData + 1);

    IqeB_FileNormalizePathChars( TempFileName);

    // load file

    pTempImage = YaIPS_Image_Read( TempFileName);   // Try to load an image

    if( pTempImage == NULL) {           // Got NO image

      goto ExitPoint;                   // panic exit
    }

    // Process the reference image and display it

    RefImgProcessAndDisp( (int)iToolData, pTempImage, true, false, true);  // Preprocess new image and display

    RefImgBackupToFile( iToolData);     // Backup reference image + mask to file

ExitPoint:

    if( pTempImage != NULL) {

      pTempImage->release();                       // Release temporary image
    }

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
  Fl_YaIPS_AOI_t TempAOI;
  int New_Tab_Group, iMask;

  // Check for valid output image displayed

  if( pToolData->YaIPS_ImageDisp.pImage_Img == NULL) {

    return;
  }

  if( pMyParWin == NULL) {     // Security test, parameter window must exist

    return;
  }

  // One of the mask tab groups selected

  New_Tab_Group = pTab_Groups->GetTabGroup();

  if( New_Tab_Group < YAIPS_SELECTION_MASK_1 || New_Tab_Group > YAIPS_SELECTION_MASK_3) {

    // No mask tab group selected
    return;
  }

  iMask = New_Tab_Group - YAIPS_SELECTION_MASK_1;

  //
  // Draw AOI
  //

  if( pToolData->MaskDrawMouseActive == 0) {   // Left mouse button is not pressed

    goto ExitPoint;
  }

  switch( pToolData->MaskDrawMode[ iMask]) {

  default:
  case MASK_DRAW_MOVE_IMAGE:    // Move image with mouse

    break;

  case MASK_DRAW_FREEHAND:      // Draw free hand

    break;

  case MASK_DRAW_RECTGANGLE:    // Draw a rectangle

    if( pYaIPS_ImageDisp->AoiP1x < pYaIPS_ImageDisp->AoiP2x) {

      TempAOI.XPos  = pYaIPS_ImageDisp->AoiP1x;
      TempAOI.XSize = pYaIPS_ImageDisp->AoiP2x - pYaIPS_ImageDisp->AoiP1x + 1;

    } else {

      TempAOI.XPos  = pYaIPS_ImageDisp->AoiP2x;
      TempAOI.XSize = pYaIPS_ImageDisp->AoiP1x - pYaIPS_ImageDisp->AoiP2x + 1;
    }

    if( pYaIPS_ImageDisp->AoiP1y < pYaIPS_ImageDisp->AoiP2y) {

      TempAOI.YPos  = pYaIPS_ImageDisp->AoiP1y;
      TempAOI.YSize = pYaIPS_ImageDisp->AoiP2y - pYaIPS_ImageDisp->AoiP1y + 1;

    } else {

      TempAOI.YPos  = pYaIPS_ImageDisp->AoiP2y;
      TempAOI.YSize = pYaIPS_ImageDisp->AoiP1y - pYaIPS_ImageDisp->AoiP2y + 1;
    }

    // Clipping ?

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( pYaIPS_ImageDisp->BigImage_sx,
                    pYaIPS_ImageDisp->BigImage_sy,
                    pYaIPS_ImageDisp->BigImage_sw,
                    pYaIPS_ImageDisp->BigImage_sh);
    }

    ImageDispAoiRectDraw( pYaIPS_ImageDisp, &TempAOI, false, false, false, NULL);

    break;

  case MASK_DRAW_ELLIPSE:       // Draw an ellipse

    if( pYaIPS_ImageDisp->AoiP1x < pYaIPS_ImageDisp->AoiP2x) {

      TempAOI.XPos  = pYaIPS_ImageDisp->AoiP1x;
      TempAOI.XSize = pYaIPS_ImageDisp->AoiP2x - pYaIPS_ImageDisp->AoiP1x + 1;

    } else {

      TempAOI.XPos  = pYaIPS_ImageDisp->AoiP2x;
      TempAOI.XSize = pYaIPS_ImageDisp->AoiP1x - pYaIPS_ImageDisp->AoiP2x + 1;
    }

    if( pYaIPS_ImageDisp->AoiP1y < pYaIPS_ImageDisp->AoiP2y) {

      TempAOI.YPos  = pYaIPS_ImageDisp->AoiP1y;
      TempAOI.YSize = pYaIPS_ImageDisp->AoiP2y - pYaIPS_ImageDisp->AoiP1y + 1;

    } else {

      TempAOI.YPos  = pYaIPS_ImageDisp->AoiP2y;
      TempAOI.YSize = pYaIPS_ImageDisp->AoiP1y - pYaIPS_ImageDisp->AoiP2y + 1;
    }

    // Clipping ?

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( pYaIPS_ImageDisp->BigImage_sx,
                    pYaIPS_ImageDisp->BigImage_sy,
                    pYaIPS_ImageDisp->BigImage_sw,
                    pYaIPS_ImageDisp->BigImage_sh);
    }

    ImageDispAoiEllipseDraw( pYaIPS_ImageDisp, &TempAOI, false, false, false, NULL);

    break;

  case MASK_DRAW_POLYGON:       // Draw a polygon

    {
      int InImageX, InImageY, P1X, P1Y, P2X, P2Y, x1, y1, OffX, OffY, LineWidth, Radius;

      // Preparations

      x1 = pYaIPS_ImageDisp->BigImage_sx;
      y1 = pYaIPS_ImageDisp->BigImage_sy;

      // Points relative to image

      OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
      OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

      LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

      // Get position of mouse in image

      InImageX =
          (int) (pYaIPS_ImageDisp->SubImage_x + 0.5)
              + (int) (pYaIPS_ImageDisp->MouseX
                  / pYaIPS_ImageDisp->PixelImageToScreen);
      InImageY =
          (int) (pYaIPS_ImageDisp->SubImage_y + 0.5)
              + (int) (pYaIPS_ImageDisp->MouseY
                  / pYaIPS_ImageDisp->PixelImageToScreen);

      if (InImageX >= pYaIPS_ImageDisp->BigImage_iw) {     // Clip image source start
        InImageX = pYaIPS_ImageDisp->BigImage_iw - 1;
      }
      if (InImageX < 0) {
        InImageX = 0;
      }

      if (InImageY >= pYaIPS_ImageDisp->BigImage_ih) {     // Clip image source start
        InImageY = pYaIPS_ImageDisp->BigImage_ih - 1;
      }
      if (InImageY < 0) {
        InImageY = 0;
      }

      // Clipping ?

      if( DoClip > 0) {      // The the draw clipping

        DoClip = -1;         // Need to pop clipping

        fl_push_clip( pYaIPS_ImageDisp->BigImage_sx,
                      pYaIPS_ImageDisp->BigImage_sy,
                      pYaIPS_ImageDisp->BigImage_sw,
                      pYaIPS_ImageDisp->BigImage_sh);
      }

      // Draw first point of polygon

      Radius = YAIPS_AOI_FRAME_DIST;
      if( pYaIPS_ImageDisp->PixelImageToScreen > 1.0) {
        Radius = (int)( Radius * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
      }

      P1X = (int)( (pYaIPS_ImageDisp->AoiP1x - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + (int)(pYaIPS_ImageDisp->PixelImageToScreen * 0.5);
      P1Y = (int)( (pYaIPS_ImageDisp->AoiP1y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + (int)(pYaIPS_ImageDisp->PixelImageToScreen * 0.5);

      fl_line_style( 0, LineWidth);   // Set line width

      if( pYaIPS_ImageDisp->MouseX > P1X - Radius && pYaIPS_ImageDisp->MouseX < P1X + Radius &&  // Is mouse near first point
          pYaIPS_ImageDisp->MouseY > P1Y - Radius && pYaIPS_ImageDisp->MouseY < P1Y + Radius) {

        fl_color( FL_RED);           // Color

      } else {

        fl_color( FL_GREEN - 2);           // Color
      }

      fl_begin_loop();

      fl_circle( x1 + P1X, y1 + P1Y, Radius);

      fl_end_loop();

      // Draw line

      P1X = (int)( (InImageX - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + (int)(pYaIPS_ImageDisp->PixelImageToScreen * 0.5);
      P1Y = (int)( (InImageY - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + (int)(pYaIPS_ImageDisp->PixelImageToScreen * 0.5);

      P2X = (int)( (pYaIPS_ImageDisp->AoiP2x - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + (int)(pYaIPS_ImageDisp->PixelImageToScreen * 0.5);
      P2Y = (int)( (pYaIPS_ImageDisp->AoiP2y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + (int)(pYaIPS_ImageDisp->PixelImageToScreen * 0.5);

      fl_line_style( 0, LineWidth);   // Set line width
      fl_color( FL_GREEN - 2);           // Color

      fl_line( x1 + P1X, y1 + P1Y, x1 + P2X, y1 + P2Y);

    }

    break;

  case MASK_DRAW_FILL:          // Fill an area

    break;
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
  int ierr, x, y, iMask;
  int IsBigImageDisp;
  YaIPS_ToolData_info_t *pToolData;
  int InImageX, InImageY, DrawValue;
  static int Last_InImageX, Last_InImageY;
  static int Last_x = -9999, Last_y = -9999;           // Must be static

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

    if( pMyParWin != NULL &&                                                  // Parameter dialog is open
        pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_AUTO &&  // Any resolution
        ! pYaIPS_ImageDisp->Plot3D_Active &&                                  // and NO 3D plot active
        pMyParWin != NULL &&                                                  // and parameter dialog is open
        pToolData->Tab_Group_Selected >= YAIPS_SELECTION_MASK_1 &&            // and tab group 'Mask'
        pToolData->Tab_Group_Selected <= YAIPS_SELECTION_MASK_3) {

      iMask = pToolData->Tab_Group_Selected - YAIPS_SELECTION_MASK_1;

      // For test set an cursor shape

      pYaIPS_ImageDisp->CursorShape = FL_CURSOR_HAND;

      // Get position in image

      InImageX =
          (int) (pYaIPS_ImageDisp->SubImage_x + 0.5)
              + (int) (pYaIPS_ImageDisp->MouseX
                  / pYaIPS_ImageDisp->PixelImageToScreen);
      InImageY =
          (int) (pYaIPS_ImageDisp->SubImage_y + 0.5)
              + (int) (pYaIPS_ImageDisp->MouseY
                  / pYaIPS_ImageDisp->PixelImageToScreen);

      if (InImageX >= pYaIPS_ImageDisp->BigImage_iw) {     // Clip image source start
        InImageX = pYaIPS_ImageDisp->BigImage_iw - 1;
      }
      if (InImageX < 0) {
        InImageX = 0;
      }

      if (InImageY >= pYaIPS_ImageDisp->BigImage_ih) {     // Clip image source start
        InImageY = pYaIPS_ImageDisp->BigImage_ih - 1;
      }
      if (InImageY < 0) {
        InImageY = 0;
      }

      DrawValue = pToolData->MaskColor[ iMask] ? -1 : 0;            // Mask draw color

      if( pYaIPS_ImageDisp->mouseleft) {                    // Left mouse button pressed

        // Set value

        switch( pToolData->MaskDrawMode[ iMask]) {

        default:
        case MASK_DRAW_MOVE_IMAGE:    // Move image with mouse

          if( IsBigImageDisp) {  // Called for big image display

            // Left/right mouse button pressed on image background
            YaIPS_ImageDispMouse_CommonMouseBackGnd( pYaIPS_ImageDisp, &Last_x, &Last_y);
          }

          break;

        case MASK_DRAW_FREEHAND:      // Draw free hand

          if( pToolData->MaskDrawMouseActive == 0) {   // No mouse button pressed until now

            pToolData->MaskDrawMouseActive = 1;        // Set button pressed

            pYaIPS_ImageDisp->AoiP1x = InImageX;
            pYaIPS_ImageDisp->AoiP1y = InImageY;
          }

          My_G2D_init( pToolData, iMask);

          YaIPS_G2D_Line( pYaIPS_ImageDisp->AoiP1x, pYaIPS_ImageDisp->AoiP1y, InImageX, InImageY, pToolData->MaskBrush[ iMask], DrawValue);

          pYaIPS_ImageDisp->AoiP1x = InImageX;
          pYaIPS_ImageDisp->AoiP1y = InImageY;

          pToolData->Input1_Change = 0;                          // Force recalculation output

          break;

        case MASK_DRAW_RECTGANGLE:    // Draw a rectangle
        case MASK_DRAW_ELLIPSE:       // Draw an ellipse

          if( pToolData->MaskDrawMouseActive == 0) {   // No mouse button pressed until now

            pToolData->MaskDrawMouseActive = 1;        // Set button pressed

            pYaIPS_ImageDisp->AoiP1x = InImageX;
            pYaIPS_ImageDisp->AoiP1y = InImageY;

            pYaIPS_ImageDisp->AoiP2x = InImageX;
            pYaIPS_ImageDisp->AoiP2y = InImageY;
          }

          if( pYaIPS_ImageDisp->mouseright) {           // Right mouse button pressed too

            // Move rectangle

            pYaIPS_ImageDisp->AoiP1x += InImageX - Last_InImageX;
            pYaIPS_ImageDisp->AoiP1y += InImageY - Last_InImageY;

            pYaIPS_ImageDisp->AoiP2x += InImageX - Last_InImageX;
            pYaIPS_ImageDisp->AoiP2y += InImageY - Last_InImageY;

          } else {                                      // Right mouse button is not pressed

            // Change second point of rectangle

            pYaIPS_ImageDisp->AoiP2x = InImageX;
            pYaIPS_ImageDisp->AoiP2y = InImageY;
          }

          pYaIPS_ImageDisp->pImage_Box->redraw();

          break;

        case MASK_DRAW_POLYGON:       // Draw a polygon

          if( pToolData->MaskDrawMouseActive == 0) {   // No mouse button pressed first time

            pToolData->MaskDrawMouseActive = 1;        // Set button pressed

            pYaIPS_ImageDisp->AoiP1x = InImageX;       // Remember first press
            pYaIPS_ImageDisp->AoiP1y = InImageY;

            pYaIPS_ImageDisp->AoiP2x = InImageX;       // Remember last point
            pYaIPS_ImageDisp->AoiP2y = InImageY;

          } else if( pToolData->MaskDrawMouseActive == 2) { // Mouse button pressed next time

            int P1X, P1Y, OffX, OffY, Radius;

            // Check mouse near first image

            OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
            OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

            Radius = YAIPS_AOI_FRAME_DIST;
            if( pYaIPS_ImageDisp->PixelImageToScreen > 1.0) {
              Radius = (int)( Radius * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
            }

            P1X = (int)( (pYaIPS_ImageDisp->AoiP1x - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + (int)(pYaIPS_ImageDisp->PixelImageToScreen * 0.5);
            P1Y = (int)( (pYaIPS_ImageDisp->AoiP1y - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5) + (int)(pYaIPS_ImageDisp->PixelImageToScreen * 0.5);

            if( pYaIPS_ImageDisp->MouseX > P1X - Radius && pYaIPS_ImageDisp->MouseX < P1X + Radius &&  // Is mouse near first point
                pYaIPS_ImageDisp->MouseY > P1Y - Radius && pYaIPS_ImageDisp->MouseY < P1Y + Radius) {

              pToolData->MaskDrawMouseActive = 4;        // Set done

              My_G2D_init( pToolData, iMask);

              YaIPS_G2D_Line( pYaIPS_ImageDisp->AoiP2x, pYaIPS_ImageDisp->AoiP2y, pYaIPS_ImageDisp->AoiP1x, pYaIPS_ImageDisp->AoiP1y, DrawValue);

            } else {

              pToolData->MaskDrawMouseActive = 1;        // Set button pressed

              My_G2D_init( pToolData, iMask);

              YaIPS_G2D_Line( pYaIPS_ImageDisp->AoiP2x, pYaIPS_ImageDisp->AoiP2y, InImageX, InImageY, DrawValue);

              pYaIPS_ImageDisp->AoiP2x = InImageX;       // Remember last point
              pYaIPS_ImageDisp->AoiP2y = InImageY;
            }

            pToolData->ImageHasChanged = true;                   // Reference image has changed
          }

          pToolData->Input1_Change = 0;                          // Force recalculation output

          break;

        case MASK_DRAW_FILL:          // Fill an area

          pToolData->MaskDrawMouseActive = 1;        // Set button pressed

          My_G2D_init( pToolData, iMask);

          YaIPS_G2D_Fill( InImageX, InImageY, DrawValue, true, 0);

          pToolData->ImageHasChanged = true;                   // Reference image has changed

          pToolData->Input1_Change = 0;                          // Force recalculation output

          break;
        }

      } else {                        // Left mouse button is NOT pressed

        // Mouse button is not pressed

        if( pToolData->MaskDrawMouseActive != 0) {   // Mouse button was pressed before

          switch( pToolData->MaskDrawMode[ iMask]) {

          default:
          case MASK_DRAW_MOVE_IMAGE:    // Move image with mouse

            break;

          case MASK_DRAW_FREEHAND:      // Draw free hand

            pToolData->ImageHasChanged = true;                   // Reference image has changed

            break;

          case MASK_DRAW_RECTGANGLE:    // Draw a rectangle

            My_G2D_init( pToolData, iMask);

            YaIPS_G2D_Rectangle( pYaIPS_ImageDisp->AoiP1x, pYaIPS_ImageDisp->AoiP1y, pYaIPS_ImageDisp->AoiP2x, pYaIPS_ImageDisp->AoiP2y, DrawValue, false);

            pToolData->ImageHasChanged = true;                   // Reference image has changed

            pToolData->Input1_Change = 0;                          // Force recalculation output

            break;

          case MASK_DRAW_ELLIPSE:       // Draw an ellipse

            {
              int x1, y1, x2, y2, tmp, rx, ry;

              x1 = pYaIPS_ImageDisp->AoiP1x;
              y1 = pYaIPS_ImageDisp->AoiP1y;
              x2 = pYaIPS_ImageDisp->AoiP2x;
              y2 = pYaIPS_ImageDisp->AoiP2y;

              if(x2 < x1) { tmp = x1; x1 = x2; x2 = tmp; }  // Ensure 1. point is top right
              if(y2 < y1) { tmp = y1; y1 = y2; y2 = tmp; }

              My_G2D_init( pToolData, iMask);

              rx = (x2 - x1 + 1) / 2;
              ry = (y2 - y1 + 1) / 2;

              YaIPS_G2D_Ellipse( x1 + rx, y1 + ry, rx, ry, DrawValue, false);

              pToolData->ImageHasChanged = true;                   // Reference image has changed
            }

            pToolData->Input1_Change = 0;                          // Force recalculation output

            break;

          case MASK_DRAW_POLYGON:       // Draw a polygon

            if( pYaIPS_ImageDisp->mouseright) {                 // Right mouse button pressed

              if( pToolData->MaskDrawMouseActive != 0) {        // Active mouse action

                YaIPS_G2D_UnDo_Save();

                pToolData->MaskDrawMouseActive = 0;             // No active mouse action
              }
            }

            if( pToolData->MaskDrawMouseActive == 1) {          // Mouse button was pressed

              pToolData->MaskDrawMouseActive = 2;               // Set released

            } else if( pToolData->MaskDrawMouseActive == 4) {   // Polygon was closed

              YaIPS_G2D_UnDo_Save();

              pToolData->MaskDrawMouseActive = 0;               // Set released
            }

            break;

          case MASK_DRAW_FILL:          // Fill an area

            if( pToolData->MaskDrawMouseActive != 0) {          // Mouse button was pressed

              YaIPS_G2D_UnDo_Save();

              pToolData->MaskDrawMouseActive = 0;               // Set released
            }

            break;
          }
        }

        if( pToolData->MaskDrawMode[ iMask] == MASK_DRAW_POLYGON) {   // Drawing a polygon

          if( pToolData->MaskDrawMouseActive != 0) {      // Drawing a line

            pYaIPS_ImageDisp->pImage_Box->redraw();       // Force refresh after line move
          }

        } else {

          if( pToolData->MaskDrawMouseActive != 0) {      // Mouse button was pressed

            YaIPS_G2D_UnDo_Save();

            pToolData->MaskDrawMouseActive = 0;           // Reset latched data
          }
        }
      }

      Last_InImageX = InImageX;
      Last_InImageY = InImageY;


      if( pYaIPS_ImageDisp->mouseleft == false &&                    // Left mouse button pressed
          pYaIPS_ImageDisp->mouseright) {                            // and right mouse button is pressed

        if( IsBigImageDisp) {  // Called for big image display

          pYaIPS_ImageDisp->mouseleft  = true;                       // Simulate left mouse button press
          pYaIPS_ImageDisp->mouseright = false;

          // Left/right mouse button pressed on image background
          YaIPS_ImageDispMouse_CommonMouseBackGnd( pYaIPS_ImageDisp, &Last_x, &Last_y);

          pYaIPS_ImageDisp->mouseleft  = false;                     // Restore
          pYaIPS_ImageDisp->mouseright = true;
        }

      }

    } else {

      if( pToolData->MaskDrawMouseActive != 0) {      // Mouse button was pressed

        YaIPS_G2D_UnDo_Save();

        pToolData->MaskDrawMouseActive = 0;
      }

      pYaIPS_ImageDisp->Latched_AoiDeltaAdd = 0;               // Reset latched data
      pYaIPS_ImageDisp->Latched_CursorShape = 0;
    }

    break;
  }

ExitPoint:

  // Mouse callback common exit work. Manage AOI selection and setting cursor shape

  YaIPS_ImageDispMouse_CommonExit( pYaIPS_ImageDisp);

  return( 0);        // Mouse events are processed
}

/************************************************************************************
 * IqeB_GUI_InspRefImgWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_InspRefImgWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

    // Register draw after function for big image display
    YaIPS_ToolWinDrawAfterSet( MY_WIN_ID + SubWinIDx, YaIPS_GUI_MyDrawAfter_Other);

	  IqeB_GUI_InspRefImgWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  IqeB_GUI_InspRefImgWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


