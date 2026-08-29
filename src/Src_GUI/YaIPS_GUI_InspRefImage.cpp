/****************************************************************************

  YaIPS_GUI_InspRefImage.cpp

  Inspection reference image windows.

  25.09.2025 RR: First edition of this file.

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

  int ContourPlaneThres;                // Contour: Plane threshold

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

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

  // Parameter settings ...

  { PREF_T_INT,     "ContourPlaneThres",    "16", &YaIPS_ToolData_info[0].ContourPlaneThres},
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

// ...

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolData;     // NOTE: Is used by all parameter dialog functions

/************************************************************************************
 * Process reference image and display it
 *
 * Check for contours in the reference image and set contour bit in alpha mask.
 *
 * return:    0  OK
 *         else  Error
 */

static int ProcessRefImgAndDisp( int iToolData, Fl_RGB_Image *pImgInp, int NoAlphaDisplay)
{
  int ierr;
  YaIPS_ToolData_info_t *pToolData;
  char TempFileName[ 256 + 16];
  Fl_RGB_Image *pIntermediateImage = NULL;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

  // Copy input image as reference image, also sets contour bit in alpha

  ierr = YaIPS_RGB_Contour( &pIntermediateImage, pImgInp, pToolData->ContourPlaneThres, YAIPS_RGB_CONTOUR_LP_ALPHA);

  if( pIntermediateImage == NULL) {    // Security test, have no image

    return( -1);
  }

  if( ierr != 0)    {                  // YaIPS_RGB_Contour() has an error

    pIntermediateImage->release();    // Release image data

    return( ierr);
  }

  // Load the image to the display
  YaIPS_ImageDispUpdateByNewImage( &pToolData->YaIPS_ImageDisp, pIntermediateImage,
                                   MY_WIN_ID + iToolData, TempFileName, true, true, NoAlphaDisplay);    // No alpha display

  if( pToolData->YaIPS_ImageDisp.pImage_Img != NULL) {   // Have a latched image

    // Check an image display for size change and redisplay if size has changed.
    YaIPS_ImageDispDrawUpdate( &pToolData->YaIPS_ImageDisp, true);

    if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {   // and display this on the big image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                       MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName,    // Load the image to the display
                                       true, true, NoAlphaDisplay);    // No alpha display
    }
  }

  pIntermediateImage->release();      // Release image data

  return( 0);    // Return OK
}

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{

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
  int iToolData;

  pToolData->MyParPosX = pMyParWin->x();
  pToolData->MyParPosY = pMyParWin->y();

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::remove_idle( IqeB_GUI_ParIdleAction);      // Redraw window during idle
#endif
  Fl::remove_check( IqeB_GUI_ParIdleAction);     // Check small image size change

  iToolData = YaIPS_ToolData_info - pToolData;

  // On exit refresh display without 'hack to visualize contour'

  pToolData->YaIPS_ImageDisp.NoAlphaDisplay  = true;                      // Set no alpha display

  YaIPS_ImageDispDrawUpdate( &pToolData->YaIPS_ImageDisp, true);

  // Copy to display images
  if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {        // and this tool window is selected

    // Show also on big image
    YaIPS_BigImageDisp.NoAlphaDisplay  = true;                            // Set no alpha display

    YaIPS_ImageDispDrawUpdate( &YaIPS_BigImageDisp, true);
  }

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

    pToolData->Input1_Change = 0;         // Force recalculation output

    // ...

    if( pValue == &pToolData->ContourPlaneThres) {    // Was the plane threshold

      Fl_RGB_Image *pTempImage = NULL;
      char TempFileName[ 256 + 16];
      int iToolData;

      iToolData = YaIPS_ToolData_info - pToolData;

      // Construct a file name for the last reference image

      sprintf( TempFileName, "%s/Images/YaIPS/Reference-%d.png", YaIPS_WorkingDirectory, (int)iToolData + 1);

      IqeB_FileNormalizePathChars( TempFileName);

      // load file

      pTempImage = YaIPS_Image_Read( TempFileName);   // Try to load an image

      if( pTempImage == NULL) {           // Got NO image

        goto ExitPoint;                   // panic exit
      }

      // Process the reference image and display it

      ProcessRefImgAndDisp( (int)iToolData, pTempImage, 2);   // 2 = hack to visualize contour

  ExitPoint:

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

  pToolData->Input1_Change = 0;          // Force recalculation output
}
#endif

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

  pToolData = YaIPS_ToolData_info + iToolData;               // Point to info data

  //
  // creation of window on first call
  //

  xPos = xLeft;
  yPos = yTop;

  if( pToolData->MyParPosX != IQE_GUI_NO_WINPOS_X && pToolData->MyParPosY != IQE_GUI_NO_WINPOS_Y) { // have last window position

    xPos = pToolData->MyParPosX;
    yPos = pToolData->MyParPosY;
  }

  pMyParWin = new Fl_Window( xPos, yPos, 297 /*IQE_GUI_TOOLS_STD_WITDH*/, 36 /* 162 */, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  //
  //  GUI things
  //

  int x1, y, yy, xx2;
  //x/int xx1, xc;
  //x/char TempBuffer[ 256];

  //x/Fl_Check_Button *pCheckTemp;
  //x/Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  //x/IqeFl_Float_Input  *pFloatTemp;
  //x/IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group;
  //x/Fl_Button       *pTemp_Button;
  //x/Fl_Choice       *pTemp_Choice;
  //x/Fl_Radio_Round_Button *pRadioButTemp;

  //x/x1  = 4;
  //x/xx1 = pMyParWin->w() - 16;
  //x/xx2 = xx1 / 2;
  //x/xc  = pMyToolWin->w() / 2;          // x center

  yy  = 20;

  xx2 = 54;      // With of input element

  y = 4;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - 8, pMyParWin->h() - 8);
  pTemp_Group->box( FL_UP_BOX);

  // Next line

  y += 4;
  x1 = 4;

  x1 += 120;
  xx2 = 34;

  pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_InspRefImg_GroupA1=Plane threshold"));
  pTemp_Int->tooltip( LangStringLookup( "&GUI_InspRefImg_GroupA1a="
                                        "Plane threshold.\n"
                                        "Used to prepare the reference image.\n"
                                        "Pixel values below this threshold are\n"
                                        "lowpass filtered. The contour pixels are\n"
                                        "not changed. Also sets a contour bit in\n"
                                        "the alpha part of the reference image."
                                        "This contour bit is used by the image"
                                        "compare window."));
  pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
  pTemp_Int->SetValue( pToolData->ContourPlaneThres);
  pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->ContourPlaneThres);
  pTemp_Int->SetModifyData( 8, 255, 8, 1);

  // Finish things for this group

  pTemp_Group->end();

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

  // On entry refresh display with 'hack to visualize contour'

  pToolData->YaIPS_ImageDisp.NoAlphaDisplay  = 2;                         // 2 = hack to visualize contour

  YaIPS_ImageDispDrawUpdate( &pToolData->YaIPS_ImageDisp, true);

  // Copy to display images
  if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {        // and this tool window is selected

    // Show also on big image
    YaIPS_BigImageDisp.NoAlphaDisplay  = 2;                               // 2 = hack to visualize contour

    YaIPS_ImageDispDrawUpdate( &YaIPS_BigImageDisp, true);
  }

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

  sprintf( TempFileName, "%s/Images/YaIPS/Reference-%d.png", YaIPS_WorkingDirectory, (int)iToolData + 1);

  IqeB_FileNormalizePathChars( TempFileName);

  YaIPS_Image_Write_PNG( TempFileName, pImgIn1);

  // Process the reference image and display it

  ProcessRefImgAndDisp( (int)iToolData, pImgIn1, true);

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

  sprintf( TempFileName, "%s/Images/YaIPS/Reference-%d.png", YaIPS_WorkingDirectory, (int)iToolData + 1);

  IqeB_FileNormalizePathChars( TempFileName);

  YaIPS_Image_Write_PNG( TempFileName, pTempImage);

  // Process the reference image and display it

  ierr = ProcessRefImgAndDisp( (int)iToolData, pTempImage, true);

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

    if( pToolData->
        YaIPS_ImageDisp.pImage_Img != NULL) {                   // Got an image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                      MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName,   // Load the image to the display
                                      true, false, true);    // No alpha display
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
                             Input1_Check == 0);                        // Have a valid input image

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_Load,
                             DoEnable);                             // Enable GUI elements
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

    // No image processing on input change for this modul,


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

  pToolData->IsOpen = true;               // Flag image as open

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

    // Place of saved reference image

    sprintf( TempFileName, "%s/Images/YaIPS/Reference-%d.png", YaIPS_WorkingDirectory, (int)iToolData + 1);

    IqeB_FileNormalizePathChars( TempFileName);

    // load file

    pTempImage = YaIPS_Image_Read( TempFileName);   // Try to load an image

    if( pTempImage == NULL) {           // Got NO image

      goto ExitPoint;                   // panic exit
    }

    // Process the reference image and display it

    ProcessRefImgAndDisp( (int)iToolData, pTempImage, true);

ExitPoint:

    if( pTempImage != NULL) {

      pTempImage->release();                       // Release temporary image
    }

  }
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

  IqeB_GUI_InspRefImgWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


