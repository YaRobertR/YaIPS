/****************************************************************************

  YaIPS_GUI_VideoRead.cpp

  Read and display a video.

  25.05.2026 RR: First edition of this file.
  03.09.2026 RR: * Function VideoLoadFile()
                   Ensure normalized path characters and working directory
                   for last loaded file.

*****************************************************************************
*/

//x/#define USE_DEBUG_OUTPUTS                  1 // Set this to make debug outputs

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sysinfoapi.h>

// Ccap specific includes
#include <ccap_c.h>

// Other includes
#include "YaIPS.h"

/************************************************************************************
* Defines for this source file.
*/

// Defines for windows ID
#define MY_WIN_ID     YAIPS_WIN_ID_VIDEO_READ         // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_VIDEO_READ        // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_VideoR_Title=Video viewer"    // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinVideoRead"             // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_VideoRead_Tool    // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       249 // YAIPS_WIN_SIZE_S1_X_MIN
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
static  int DropFile_cb( Fl_Widget *w, void *pFileNameArg, void *pImageDispArg, int SubWinIDx);
static void YaIPS_ToolWin_GUI_Callback( Fl_Widget *w, long int iToolData);
static void MyWinUpdate( int iToolData, int DoEnable);
static void IqeB_GUI_ToolsMyIdleAction( void *);
static void YaIPS_GUI_MyDrawAfter_cb( Fl_Widget *pW, void *pArg1, void *pArg2);

/************************************************************************************
* Global variables for camera management
*/

#define CAM_MAX_CAMERAS    CCAP_MAX_DEVICES  // Max number of cameras, use same value as ccap API

//-----------------------------------------------------------------------------------
// Manage multiple tool windows
//-----------------------------------------------------------------------------------

#define VIDEO_BUFFER_ALLOC_MIN     50   // Min size of video buffer for backspace [MByte]
#define VIDEO_BUFFER_ALLOC_MAX    500   // Max size of video buffer for backspace [MByte]

#define VIDEO_STATE_NONE            0   // No video loaded
#define VIDEO_STATE_LOADED     0x0001   // Bit set: video is loaded
#define VIDEO_STATE_PLAY_ON    0x0002   // Bit set: Play the video, else video is paused

#define VIDEO_ACTION_NONE           0   // Nothing to do
#define VIDEO_ACTION_BLACK_DISP     1   // Black display image (after an error or so)
#define VIDEO_ACTION_PAR_CHECK      2   // Parameter window has closed. Check for change of parameters.
#define VIDEO_ACTION_SKIP_BACK      3   // Skip back one frame
#define VIDEO_ACTION_SKIP_FORW      4   // Skip forward one frame
#define VIDEO_ACTION_POS_BEGIN      5   // Position to begin of video

typedef struct {

  // Parameters
  int IsOpen;                           // True if this window is open.
  void *pMyToolWin;                     // Pointer to window data ( is pointer to CLASS_WIN_TOOL)

  int MyWinPosX, MyWinPosY;             // last window position
  int MyWinSizeX, MyWinSizeY;           // last window size

  char LastFileName[ FILENAME_MAX];     // File name of last loaded video file. This is inclusive path and file extension.

  // Used for intern data management

  Fl_YaIPS_ImageDisp_t YaIPS_ImageDisp;   // Info image output

  CcapProvider *pCcapProvider;            // CCap: camera provider

  int VideoState;                         // State of video
  int PlaybackSpeedIndex;                 // Current playback speed index

  double PlaybackSpeedTarget;             // Playback speed to set during play
  double PlaybackSpeedCurr;               // Current playback speed set

  // Buffer for video backspace possibility

  uchar *pBuffer;                         // Pointer to video buffer
  int   BufferWidth;                      // Used buffer frame width
  int   BufferHeight;                     // Used buffer frame height
  int   BufferSize1Frame;                 // Buffer size for 1 one frame.
  int   BufferFramesAlloc;                // Number of frame buffers fit into the buffer
  int   BufferMBytesAlloc;                // MBytes used for video buffer allocation. Used to check for buffer size change.
  int   BufferFramesN;                    // Number of used frames in buffer
  int   BufferFramePut;                   // But next frame here in the buffer
  int   BufferFrameBack;                  // If > 0 displayed buffered image

  // Actions to do in video playback code

  int VideoAction;                        // != 0 Something to do
  int VideoActionArg;                     // If != 0 event state for some actions.

  int WaitFrameCaptured;                  // Wait for frame is captured

  // Info about open video. If video is closed all is set to 0.

  float FrameRate;                        // CCap: Frame rate of open video
  int   FrameWidth;                       // CCap: Frame width
  int   FrameHeight;                      // CCap: Frame height
  int   FrameCount;                       // CCap: Total number of frames
  int   FrameIndex;                       // CCap: Unique, incremental frame index, -1 if no frame displayed until now
#ifdef USE_DEBUG_OUTPUTS
  int   FrameIndex2;                      // CCap: Other method to get frame index
  double FrameTimeCurr2;                  // CCap: Other method to get current position in seconds
#endif
  double FrameTimeVideo;                  // CCap: Video duration in seconds
  double FrameTimeCurr;                   // CCap: Current position in seconds

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int Video_Autoplay;                   // If true, a video automatically begins playing after loading.
  int Video_Repeat;                     // If true, restart video if the video ends.
  int Video_UseBuffer;                  // If true, use a back step buffer.
  int Video_BufferSize;                 // Size of video buffer in megabytes.

} YaIPS_ToolData_info_t;

static int nYaIPS_ToolData_info;       // Number of image files windows open

static YaIPS_ToolData_info_t YaIPS_ToolData_info[ MY_WIN_MAX];

// Playback speed

#ifdef use_again
#define PLAYBACL_SPEED_N       11      // Number of playback speeds
#define PLAYBACL_SPEED_DEF      4      // Index of default playback speed

// NOTE: negative values are changed to 1/x values for slower playback speeds
static int PlaybackSpeeds[ PLAYBACL_SPEED_N] = { -20, -10, -5, -2, 1, 2, 5, 10, 20, 50, 100};
#else

// NOTE: Maximum play back speed is 1.

#define PLAYBACL_SPEED_N        5      // Number of playback speeds
#define PLAYBACL_SPEED_DEF      4      // Index of default playback speed

// NOTE: negative values are changed to 1/x values for slower playback speeds
static int PlaybackSpeeds[ PLAYBACL_SPEED_N] = { -20, -10, -5, -2, 1};
#endif

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

  { PREF_T_STRING,       "LastFileName",   "", &YaIPS_ToolData_info[0].LastFileName, sizeof( YaIPS_ToolData_info[0].LastFileName) - 1 },

  //
  // Parameter Dialog
  //

  { PREF_T_INT,      "MyParPosX",  IQE_GUI_NO_WINPOS_X_STRING, &YaIPS_ToolData_info[0].MyParPosX }, // NOTE: values will be clipped against MYWIN_SIZE_X_MIN / MYWIN_SIZE_Y_MIN
  { PREF_T_INT,      "MyParPosY",  IQE_GUI_NO_WINPOS_Y_STRING, &YaIPS_ToolData_info[0].MyParPosY },

  { PREF_T_INT,   "Video_Autoplay",   "0", &YaIPS_ToolData_info[0].Video_Autoplay },
  { PREF_T_INT,     "Video_Repeat",   "0", &YaIPS_ToolData_info[0].Video_Repeat },
  { PREF_T_INT,  "Video_UseBuffer",   "0", &YaIPS_ToolData_info[0].Video_UseBuffer },
  { PREF_T_INT, "Video_BufferSize", "100", &YaIPS_ToolData_info[0].Video_BufferSize },
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_VideoReadWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp);

//-----------------------------------------------------------------------------------
// Ccap error handling functions
//
//-----------------------------------------------------------------------------------

static int My_ccap_last_error = CCAP_ERROR_NONE;  // Last error code
#ifdef USE_DEBUG_OUTPUTS
static int My_ccap_last_error2 = 0;               // Keep last error code for display
#endif

// Error callback function

static void My_ccap_error_callback( CcapErrorCode errorCode, const char* errorDescription, void* userData)
{

  My_ccap_last_error = errorCode;         // Latch last error occurred

#ifdef USE_DEBUG_OUTPUTS
  My_ccap_last_error2 = errorCode;         // Keep last error code for display
#endif

  //x/ printf("Camera Error - Code: %d, Description: %s\n", (int)errorCode, errorDescription);
}

// Get last latched error code.
//
// Also latched error code.
//
// Usage hint:
//   * Call ccap_error_test() to reset latched error before test function for error
//   * Call the function to test
//   * Call ccap_error_test() and test the error code or test 'My_ccap_last_error'
// return:    0  No error
//         else  Last occurred error,

static int ccap_error_test()
{
  int TempErrorCode;

  TempErrorCode = My_ccap_last_error;     // Latch last error code

  My_ccap_last_error = CCAP_ERROR_NONE;   // Rest last error

  return( TempErrorCode);                 // Return last error code picked
}

//-----------------------------------------------------------------------------------
// Parameter dialog
//
// This is a modal dialog. Therefore we can use global variables to hold
// info about the data.
//-----------------------------------------------------------------------------------

// ...

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolData;     // NOTE: Is used by all parameter dialog functions

static IqeFl_Int_Input *pInt_Video_BufferSize;    // Video buffer size

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{

  IqeB_GUI_WidgetActivate( pInt_Video_BufferSize, pToolData->Video_UseBuffer); // Set item activated/inactive
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

  // Parameter window has close. Check for change of parameters.
  pToolData->VideoAction = VIDEO_ACTION_PAR_CHECK;
  pToolData->VideoActionArg = 0;
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

  if( *pValue != Value) {                  // Value is different

    *pValue = Value;                       // update the variable

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

  pMyParWin = new Fl_Window( xPos, yPos, IQE_GUI_TOOLS_STD_WITDH, 58, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  //
  //  GUI things
  //

  int x1, y, yy, xx1, xx2;
  //x/int xx2, xc;
  //x/char TempBuffer[ 256];

  Fl_Check_Button *pCheckTemp;
  //x/Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  //x/IqeFl_Float_Input  *pFloatTemp;
  //x/IqeFl_Tabs      *pTemp_Tabs;
  //x/Fl_Group        *pTemp_Group;
  //x/Fl_Button       *pTemp_Button;
  //x/Fl_Choice       *pTemp_Choice;
  //x/Fl_Radio_Round_Button *pRadioButTemp;
  //x/Fl_Hor_Nice_Slider    *pTemp_Slider;
  //x/Fl_Value_Slider *pTemp_ValSlider;

  x1  = 4;
  xx1 = pMyParWin->w() - 8;
  //x/xx2 = xx1 / 2;
  //x/xc  = pMyToolWin->w() / 2;          // x center
  yy  = 20;

  y = 4;

  y += 4;

  pCheckTemp = new Fl_Check_Button( x1, y, xx1, yy, LangStringLookup( "&GUI_VideoR_Par_Autoplay=Video autoplay"));
  pCheckTemp->tooltip( LangStringLookup( "&GUI_VideoR_Par_Autoplaya="
                                         "A video automatically begins\n"
                                         "playing after loading."));
  pCheckTemp->value( pToolData->Video_Autoplay);
  pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->Video_Autoplay);

  y += yy + 4;

  xx2 = (xx1 * 3) / 5;

  pCheckTemp = new Fl_Check_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_VideoR_Par_VideoBufOn=Video buffer"));
  pCheckTemp->tooltip( LangStringLookup( "&GUI_VideoR_Par_VideoBufOna="
                                         "If set, a backtracking buffer is used.\n"
                                         "When playing forward, video frames are buffered.\n"
                                         "When paused, these past frames can be viewed."));
  pCheckTemp->value( pToolData->Video_UseBuffer);
  pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &pToolData->Video_UseBuffer);

  xx2 = 48;
  x1 += xx1 - xx2;

  pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_VideoR_Par_VideoBufSize=Size"));
  pTemp_Int->tooltip( LangStringLookup( "&GUI_VideoR_Par_VideoBufSizea="
                                        "Backtracking buffer size.\n"
                                        "Range: 50 to 500 megabytes.\n"
                                        "Recommended value: 100"));
  pTemp_Int->SetValue( pToolData->Video_BufferSize);
  pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Video_BufferSize);
  pTemp_Int->SetModifyData( VIDEO_BUFFER_ALLOC_MIN, VIDEO_BUFFER_ALLOC_MAX, 50);
  pInt_Video_BufferSize = pTemp_Int;

  y += yy + 4;

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
  Fl_Button *pGUI_Video_PlayPause;       // Toggle between play and pause
  Fl_Button *pGUI_Video_PosBegin;        // Back to the beginning of the video
  Fl_Button *pGUI_Video_SkipBack;        // Skip back
  Fl_Button *pGUI_Video_SkipForw;        // Skip forward
  Fl_Button *pGUI_Video_SpeedDec;        // Decrease play speed
  Fl_Button *pGUI_Video_SpeedInc;        // Increase play speed
  Fl_Button *pGUI_Video_Load;            // Load a Video
  Fl_Button *pGUI_Parameter;             // Open the parameter dialog
  Fl_Button *pGUI_Repeat;                // Switch repeat mode
  Fl_Box    *pGUI_Box_PBS;               // Show playback speed

  // Create the window

  CLASS_WIN_TOOL( int X, int Y, int W, int H, const char *l, int iToolDataArg) : Fl_Double_Window( X, Y, W, H, l)
  {
    YaIPS_ToolData_info_t *pToolData;
    Fl_Button *pTemp_Button;

    iToolData = iToolDataArg;                    // Index of info data element, see YaIPS_ToolData_info
    pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

    // Initialize some data

    memset( &pToolData->YaIPS_ImageDisp, 0, sizeof( Fl_YaIPS_ImageDisp_t)); // Zero data

    // ...

    Fl_Group *pGUI_GroupTopSide;                 // Top side of window
    int x, x1, x2, y, xx, xx0, yy, hWin, wWin;

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

    // Video play/pause

    pGUI_Video_PlayPause = new Fl_Button( x1, y, xx, yy, "@+3>");
    pGUI_Video_PlayPause->callback( YaIPS_ToolWin_GUI_Callback, NULL);
    pGUI_Video_PlayPause->tooltip( LangStringLookup( "&GUI_VideoR_PlayPause="
                            "Toggle between play and pause.\n"
                            "Shortcut: Spacebar"));
    pGUI_Video_PlayPause->labelcolor( YAIPS_BCOL_BUTTON);
    pGUI_Video_PlayPause->shortcut( ' ');                   // Short cut key

    x1 += xx + 5;

    // Video back to begin

    x2 = x1;        // Remember this position

    pGUI_Video_PosBegin = new Fl_Button( x1, y + yy / 2 + 1, xx, yy / 2 - 1, "@-1|<");
    pGUI_Video_PosBegin->callback( YaIPS_ToolWin_GUI_Callback, NULL);
    pGUI_Video_PosBegin->tooltip( LangStringLookup( "&GUI_VideoR_PosBegin="
                            "Back to the beginning of the video.\n"
                            "Shortcut: B or Home"));
    pGUI_Video_PosBegin->labelcolor( YAIPS_BCOL_BUTTON);
    // NOTE: Shortcut is handled by keyboard callback of this window.

    x1 += xx + 5;

    // Video skip back

    pGUI_Video_SkipBack = new Fl_Button( x1, y + yy / 2 + 1, xx, yy / 2 - 1, "@-1<-");
    pGUI_Video_SkipBack->callback( YaIPS_ToolWin_GUI_Callback, NULL);
    pGUI_Video_SkipBack->tooltip( LangStringLookup( "&GUI_VideoR_ButSkipBack="
                            "Skip back.\n"
                            "Shortcut: N or Left\n"
                            "     back 10 Seconds\n"
                            "  + Ctrl:  1 Minute\n"
                            " + Shift: 10 Minutes"));
    pGUI_Video_SkipBack->labelcolor( YAIPS_BCOL_BUTTON);
    // NOTE: Shortcut is handled by keyboard callback of this window.

    x1 += xx + 5;

    // Video skip forward

    pGUI_Video_SkipForw = new Fl_Button( x1, y + yy / 2 + 1, xx, yy / 2 - 1, "@-1->");
    pGUI_Video_SkipForw->callback( YaIPS_ToolWin_GUI_Callback, NULL);
    pGUI_Video_SkipForw->tooltip( LangStringLookup( "&GUI_VideoR_ButSkipForward="
                            "Skip forward.\n"
                            "Shortcut: M or right\n"
                            "  forward 10 Seconds\n"
                            "  + Ctrl:  1 Minute\n"
                            " + Shift: 10 Minutes"));
    pGUI_Video_SkipForw->labelcolor( YAIPS_BCOL_BUTTON);
    // NOTE: Shortcut is handled by keyboard callback of this window.

    x1 += xx + 5;

    // Decrease play speed

    x1 = x2;        // Restore  position

    pGUI_Video_SpeedDec = new Fl_Button( x1, y, xx, yy / 2 - 1, "@-1<<");
    pGUI_Video_SpeedDec->callback( YaIPS_ToolWin_GUI_Callback, NULL);
    pGUI_Video_SpeedDec->tooltip( LangStringLookup( "&GUI_VideoR_ButSpeedDec="
                            "Decrease playback speed.\n"
                            "Shortcut: J or Down"));
    pGUI_Video_SpeedDec->labelcolor( YAIPS_BCOL_BUTTON);
    // NOTE: Shortcut is handled by keyboard callback of this window.

    x1 += xx + 5;

    // Output FPS

    pGUI_Box_PBS = new Fl_Box( x1, y, xx, yy / 2 - 1);
    pGUI_Box_PBS->box(FL_BORDER_BOX);
    pGUI_Box_PBS->align( FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    x1 += xx + 5;

    // Increase play speed

    pGUI_Video_SpeedInc = new Fl_Button( x1, y, xx, yy / 2 - 1, "@-1>>");
    pGUI_Video_SpeedInc->callback( YaIPS_ToolWin_GUI_Callback, NULL);
    pGUI_Video_SpeedInc->tooltip( LangStringLookup( "&GUI_VideoR_ButSpeedInc="
                            "Increase playback speed.\n"
                            "Shortcut: K or Up"));
    pGUI_Video_SpeedInc->labelcolor( YAIPS_BCOL_BUTTON);
    // NOTE: Shortcut is handled by keyboard callback of this window.

    x1 += xx + 5;

    // 4 smaller buttons

    xx = xx0 / 2 - 1;

    // Button load image

    pGUI_Video_Load = new Fl_Button( x1, y, xx, xx, "@-2fileopen");
    pGUI_Video_Load->callback( Load_cb, (long int)iToolData);
    pGUI_Video_Load->tooltip( LangStringLookup( "&GUI_VideoR_ButOpen="
                              "Load video from file.\n"
                              "Shortcut: Ctrl+L"));
    pGUI_Video_Load->labelcolor( YAIPS_BCOL_BUTTON);
    pGUI_Video_Load->shortcut( FL_COMMAND + 'l');                   // Short cut key

    // Open setting dialog

    pGUI_Parameter = new Fl_Button( x1 + xx + 2, y, xx, xx, "@-2menu2");
    pGUI_Parameter->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_Parameter->tooltip( LANGDEF_SETTINGS_POINTS);
    pGUI_Parameter->labelcolor( YAIPS_BCOL_BUTTON);
    pGUI_Parameter->shortcut( FL_COMMAND+'p');       // Short cut key

    // Button switch repeat mode

    pGUI_Repeat = new Fl_Button( x1, y + xx + 2, xx, xx, "@-2$reload");
    pGUI_Repeat->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_Repeat->tooltip( LangStringLookup( "&GUI_VideoR_ButRepeat="
                          "Toggle repeat video.\n"
                          "Shortcut: R"));
    pGUI_Repeat->labelcolor( YAIPS_BCOL_BUTTON);

    // ...

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
    pToolData->YaIPS_ImageDisp.pImage_Box->PasteImgCallbackArg1 = &pToolData->YaIPS_ImageDisp;  // Pointer to Fl_YaIPS_ImageDisp_t

    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawBeforeCallback = YaIPS_ImageDispDrawBefore_cb; // Draw before callback
    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawAfterCallback  = YaIPS_GUI_MyDrawAfter_cb;     // Draw after callback
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

private:

  // Overwrite receiver event handler to catch keyboard events
  int handle( int event) {

    int ret, KeyCode, EventState;
    YaIPS_ToolData_info_t *pToolData;
    CLASS_WIN_TOOL *pMyToolWin;

    ret = 0;

    pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
    pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

    KeyCode    = Fl::event_key();
    EventState = Fl::event_state();

    // Keyboard handler callbacks

    switch( event) {
    case FL_FOCUS:      // This indicates an attempt to give a widget the keyboard focus.
    case FL_UNFOCUS:    // This event is sent to the previous Fl::focus() widget when another widget gets the focus or the window loses focus.
    case FL_KEYUP:      // Key release event.
    case FL_SHORTCUT:   // Process keyboard shortcuts

      return( true);

      break;

    case FL_KEYDOWN:    // A key was pressed.
    //x/case FL_KEYBOARD:   // Equivalent to FL_KEYDOWN.

      switch( KeyCode) {

      case 'p': // Parameter

        if( EventState & FL_CTRL) {    // Only if control is pressed

          YaIPS_GUI_ParameterWin( pMyToolWin->x() + 16, pMyToolWin->y() + 16, iToolData);
        }

        return( true);
        break;

      case 'l': // Load video from file

        if( EventState & FL_CTRL) {    // Only if control is pressed

          Load_cb( pGUI_Video_Load, iToolData);
        }

        return( true);
        break;

      case 'r': // Switch repeat video

        pToolData->Video_Repeat = ! pToolData->Video_Repeat;

        return( true);
        break;

      case ' ': // Play video

        if( (pToolData->VideoState & VIDEO_STATE_LOADED) == 0) {   // No video loaded

          pToolData->VideoState &= ~VIDEO_STATE_PLAY_ON;           // Play to off

        } else {

          pToolData->VideoState ^= VIDEO_STATE_PLAY_ON;            // Toggle play/pause
        }

        return( true);
        break;

      case 'b': // Back to the beginning of the video
      case FL_Home:

        if( (pToolData->VideoState & VIDEO_STATE_LOADED) != 0) {   // Video loaded

          pToolData->VideoAction = VIDEO_ACTION_POS_BEGIN;
        }

        return( true);
        break;

      case 'n': // Skip back
      case FL_Left:

        if( (pToolData->VideoState & VIDEO_STATE_LOADED) != 0) {   // Video loaded

          pToolData->VideoAction = VIDEO_ACTION_SKIP_BACK;
          pToolData->VideoActionArg = EventState;
        }

        return( true);
        break;

      case 'm': // Skip forward
      case FL_Right:

        if( (pToolData->VideoState & VIDEO_STATE_LOADED) != 0) {   // Video loaded

          pToolData->VideoAction = VIDEO_ACTION_SKIP_FORW;
          pToolData->VideoActionArg = EventState;
        }
        return( true);
        break;

      case 'j': // Decrease playback speed
      case FL_Down:

        if( (pToolData->VideoState & VIDEO_STATE_LOADED) == 0) {   // No video loaded

          pToolData->PlaybackSpeedIndex = PLAYBACL_SPEED_DEF;      // Set to default

        } else {

          double TempDouble;

          pToolData->PlaybackSpeedIndex -= 1;                      // Decrement value

          if( pToolData->PlaybackSpeedIndex < 0) {                 // Clip to minimum

            pToolData->PlaybackSpeedIndex = 0;
          }

          // Set playback speed

          TempDouble = PlaybackSpeeds[ pToolData->PlaybackSpeedIndex];
          if( TempDouble < 0) {  // Slower playback

            TempDouble = -1.0 / TempDouble;
          }

          pToolData->PlaybackSpeedTarget = TempDouble;
        }

        return( true);
        break;

      case 'k': // Increase playback speed
      case FL_Up:

        if( (pToolData->VideoState & VIDEO_STATE_LOADED) == 0) {   // No video loaded

          pToolData->PlaybackSpeedIndex = PLAYBACL_SPEED_DEF;      // Set to default

        } else {

          double TempDouble;

          pToolData->PlaybackSpeedIndex += 1;                      // Increment value

          if( pToolData->PlaybackSpeedIndex >= PLAYBACL_SPEED_N) {  // Clip to maximum

            pToolData->PlaybackSpeedIndex = PLAYBACL_SPEED_N - 1;
          }

          // Set playback speed

          TempDouble = PlaybackSpeeds[ pToolData->PlaybackSpeedIndex];
          if( TempDouble < 0) {  // Slower playback

            TempDouble = -1.0 / TempDouble;
          }

          pToolData->PlaybackSpeedTarget = TempDouble;
        }

        return( true);
        break;
      }

      //x/return( true);

      break;
    }

    if( ret == 0) {                   // Event not processed until now

      // Pass to parent class

      ret = Fl_Double_Window::handle(event);
    }

    return( ret);
  }

};

/************************************************************************************
 * VideoBufferSetup
 *
 * Initial video buffer setup.
 * Reset video buffer data to defaults.
 *
 */

static void VideoBufferSetup( YaIPS_ToolData_info_t *pToolData)
{

  // Reset buffer for video backspace

  pToolData->pBuffer = NULL;         // Pointer to video buffer
  pToolData->BufferWidth = 0;        // Used buffer frame width
  pToolData->BufferHeight = 0;       // Used buffer frame height
  pToolData->BufferSize1Frame = 0;   // Buffer size for 1 one frame. May be rounded up. [Bytes]
  pToolData->BufferFramesAlloc = 0;  // Number of frame buffers fit into the buffer
  pToolData->BufferMBytesAlloc = 0;  // MBytes used for video buffer allocation. Used to check for buffer size change.
  pToolData->BufferFramesN = 0;      // Number of used frames in buffer
  pToolData->BufferFramePut = 0;     // But next frame here in the buffer
  pToolData->BufferFrameBack = 0;    // If > 0 displayed buffered image
}

/************************************************************************************
 * VideoBufferClose
 *
 * Close video buffer on exit of window. Frees all data.
 *
 */

static void VideoBufferClose( YaIPS_ToolData_info_t *pToolData)
{

  if( pToolData->pBuffer != NULL) {

    free( pToolData->pBuffer);

    pToolData->pBuffer = NULL;
  }

  VideoBufferSetup( pToolData);
}

/************************************************************************************
 * VideoBufferAlloc
 *
 * Call this after open of a video file.
 * Try to allocate a buffer.
 *
 * If any problems or video buffer is disabled,
 * pBuffer will be NULL on exit.
 *
 */

static void VideoBufferAlloc( YaIPS_ToolData_info_t *pToolData)
{
  int BufferSize1Frame, BufferSizeBytes, BufferFramesAlloc;

  // Check parameters

  if( pToolData->FrameWidth <= 0 ||
      pToolData->FrameHeight <= 0 ||
      pToolData->Video_BufferSize <= 0) {

    // Problem with parameters

    VideoBufferClose( pToolData);    // Release video buffer

    return;
  }

  // Security test. Ensure the video buffer size is in range.

  if( pToolData->Video_BufferSize < VIDEO_BUFFER_ALLOC_MIN) {

    pToolData->Video_BufferSize = VIDEO_BUFFER_ALLOC_MIN;
  }

  if( pToolData->Video_BufferSize > VIDEO_BUFFER_ALLOC_MAX) {

    pToolData->Video_BufferSize = VIDEO_BUFFER_ALLOC_MAX;
  }

  // Calculated sizes for 1 frame

  BufferSize1Frame = pToolData->FrameWidth * pToolData->FrameHeight * 3;

  if( BufferSize1Frame & 0x03) {   // Not multiple of 4 bytes

    BufferSize1Frame &= ~0x03;     // Reset lower 2 bits
    BufferSize1Frame += 0x4;       // Round up
  }

  // Calculate number of frames fitting into the video buffer

  BufferFramesAlloc = pToolData->Video_BufferSize * 1024 * 1024 / BufferSize1Frame; // Number of frame buffers fit into give size

  if( BufferFramesAlloc < 2) {       // Need minimum 2 buffers

    // No frame buffer fit into given buffer size

    VideoBufferClose( pToolData);    // Release video buffer

    return;
  }

  // Calculate size of video buffer

  BufferSizeBytes = BufferSize1Frame * BufferFramesAlloc;

  // Do we already have a buffer with the same size

  if( pToolData->pBuffer != NULL &&
      pToolData->BufferWidth == pToolData->FrameWidth &&
      pToolData->BufferHeight == pToolData->FrameHeight &&
      pToolData->BufferSize1Frame == BufferSize1Frame &&
      pToolData->BufferFramesAlloc == BufferFramesAlloc ) {

    // Need no new buffer allocation

    // Reset video buffer put/get pointer

    pToolData->BufferFramesN = 0;      // Number of used frames in buffer
    pToolData->BufferFramePut = 0;     // But next frame here in the buffer
    pToolData->BufferFrameBack = 0;    // If > 0 displayed buffered image

    // Done

    return;
  }

  // Have to allocate a new buffer

  // Release video buffer

  VideoBufferClose( pToolData);       // Release video buffer

  // Try to allocate the new

  pToolData->pBuffer = (uchar *)malloc( BufferSizeBytes);

  if( pToolData->pBuffer == NULL) {   // Allocation failed

    // Return after memory allocation error

    return;
  }

  // If we come to here all buffer variables are 0.
  // Only the pointer to the buffer memory is set.

  // Save info about buffer configuration

  pToolData->BufferWidth       = pToolData->FrameWidth;   // Used buffer frame width
  pToolData->BufferHeight      = pToolData->FrameHeight;  // Used buffer frame height
  pToolData->BufferSize1Frame  = BufferSize1Frame;        // Buffer size for 1 one frame. May be rounded up. [Bytes]
  pToolData->BufferFramesAlloc = BufferFramesAlloc;       // Number of frame buffers fit into the buffer
  pToolData->BufferMBytesAlloc = pToolData->Video_BufferSize;  // MBytes used for video buffer allocation

  // Done

  return;
}

/************************************************************************************
 * VideoBufferFramePut
 *
 * Put a new frame into the video buffer
 *
 */

static void VideoBufferFramePut( YaIPS_ToolData_info_t *pToolData, uchar *pFrameData, int StrideSrc)
{
  uchar *pSrc, *pDst;
  int y, SizeOfLineDst;

  if( pToolData->pBuffer == NULL) {      // Security test, a buffer is allocated

    return;
  }

  pSrc = pFrameData;

  SizeOfLineDst = pToolData->FrameWidth * 3;

  if( pToolData->BufferFramesN < pToolData->BufferFramesAlloc) {      // Buffer not filled until now

    pToolData->BufferFramesN += 1;                                    // One frame more
  }

  pDst = pToolData->pBuffer + pToolData->BufferSize1Frame * pToolData->BufferFramePut;

  pToolData->BufferFramePut += 1;                                  // One frame more

  if( pToolData->BufferFramePut >= pToolData->BufferFramesAlloc) {  // Handle buffer wrap around

    pToolData->BufferFramePut = 0;
  }

  for( y = 0; y < pToolData->FrameHeight; y++) {

    memcpy( pDst, pSrc, SizeOfLineDst);

    pDst += SizeOfLineDst;
    pSrc += StrideSrc;
  }

  return;
}

/************************************************************************************
 * VideoBufferFrameGet
 *
 * Get a frame from the video buffer and store it to the display image.
 *
 */

static void VideoBufferFrameGet( YaIPS_ToolData_info_t *pToolData, Fl_RGB_Image *pDstImage)
{
  uchar *pSrc, *pDst;
  int y, SizeOfLineDst, SizeOfLineSrc;

  if( pToolData->pBuffer == NULL) {      // Security test, a buffer is allocated

    return;
  }

  // Test for correct buffer configuration

  if( pToolData->BufferFramesN <= 0) {   // There must be any frame in the buffer

    return;
  }

  if( pToolData->BufferFrameBack > pToolData->BufferFramesN - 1) {  // The back step is to high

    return;
  }

  // ...

  SizeOfLineDst = pToolData->FrameWidth * 3;

  pDst = (uchar *)pDstImage->data()[ 0];

  SizeOfLineSrc = pToolData->FrameWidth * 3;

  y = pToolData->BufferFramePut - 1 - pToolData->BufferFrameBack;     // Step back

  while( y < 0) {

    y += pToolData->BufferFramesAlloc;
  }

  pSrc = pToolData->pBuffer + pToolData->BufferSize1Frame * y;

  for( y = 0; y < pToolData->FrameHeight; y++) {

    memcpy( pDst, pSrc, SizeOfLineDst);

    pDst += SizeOfLineDst;
    pSrc += SizeOfLineSrc;
  }

  return;
}

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

  VideoBufferClose( pToolData);  // Close video buffer on exit of window. Frees all data

  YaIPS_ImageDispReleaseBeforeClose( &pToolData->YaIPS_ImageDisp);

  pToolData->IsOpen = false;                 // Flag info data is not in use

  IqeB_GUI_CloseToolWindow( (void **)&pToolData->pMyToolWin);

  // Close ccap

  if( pToolData->pCcapProvider != NULL) {  // Have a capture object from before

    if( ccap_provider_is_opened( pToolData->pCcapProvider)) { // Device is open

      ccap_provider_close( pToolData->pCcapProvider);
    }

    ccap_provider_destroy( pToolData->pCcapProvider);       // Delete it

    pToolData->pCcapProvider = NULL;                        // Mark closed
  }

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
 * VideoReset
 *
 * Reset video data to defaults.
 *
 * Return:    0   OK
 *         else  Error
 */

static void VideoReset( YaIPS_ToolData_info_t *pToolData)
{
  // Zero all data about device

  memset( pToolData->LastFileName, 0, sizeof( pToolData->LastFileName));

  pToolData->FrameRate      = 0;
  pToolData->FrameWidth     = 0;
  pToolData->FrameHeight    = 0;
  pToolData->FrameCount     = 0;
  pToolData->FrameIndex     = -1;   // no frame displayed until now
#ifdef USE_DEBUG_OUTPUTS
  pToolData->FrameIndex2    = 0;
  pToolData->FrameTimeCurr2 = 0;
#endif
  pToolData->FrameTimeVideo = 0;
  pToolData->FrameTimeCurr  = 0;

  pToolData->VideoState = VIDEO_STATE_NONE;
  pToolData->PlaybackSpeedIndex = PLAYBACL_SPEED_DEF;

  pToolData->VideoAction = VIDEO_ACTION_BLACK_DISP;   // Preset black display. Errors on loading ensure a black display.
}

/************************************************************************************
 * VideoLoadFile
 *
 * Load of a video file
 *
 * pFileName   Try to load this video vile
 *             If NULL reset all video information to no video
 *
 * Return:    0   OK
 *         else  Error
 */

static int VideoLoadFile( YaIPS_ToolData_info_t *pToolData, char *pFileName)
{
  int ierr;
  double TempDouble;

  // Close video from before

  if( pFileName != NULL) {                     // Have a file to load

    if( pToolData->pCcapProvider == NULL) {    // Security test

      return( -1);    // Error: no ccap provider
    }

    if( ccap_provider_is_opened( pToolData->pCcapProvider)) {            // Other device is open

      if( ccap_provider_is_started(  pToolData->pCcapProvider)) {        // Video was started

        ccap_provider_stop(  pToolData->pCcapProvider);                  // Stop video
      }

      ccap_provider_close( pToolData->pCcapProvider);
    }
  }

  // Reset video data to defaults

  VideoReset( pToolData);

  if( pFileName == NULL) {   // Was reset all video information to no video

    // Done

    return( 0);      // Return OK
  }

  if( ! pToolData->Video_UseBuffer) {                 // NOT use the video buffer

    // Close video buffer --> free video buffer memory

    VideoBufferClose( pToolData);
  }

  // Try to open video file

  ccap_provider_set_property( pToolData->pCcapProvider, CCAP_PROPERTY_PIXEL_FORMAT_OUTPUT, CCAP_PIXEL_FORMAT_RGB24);

  ccap_provider_set_property( pToolData->pCcapProvider, CCAP_PROPERTY_FRAME_ORIENTATION, CCAP_FRAME_ORIENTATION_TOP_TO_BOTTOM);

  // Small buffer for minimal latency
  ccap_provider_set_max_available_frame_size( pToolData->pCcapProvider, 1);

  ierr = ccap_provider_open( pToolData->pCcapProvider, pFileName, true);
  if( ierr != true) {                              // Problem with open

    return( -2);   // Error: can't open video
  }

  // Check if we are in file mode

  if( ccap_provider_is_file_mode( pToolData->pCcapProvider) != true) {   // NO file mode =

    ccap_provider_close( pToolData->pCcapProvider);    // ensure closed

    return( -3);   // Error: no video file
  }

  // Set default playback speed

  TempDouble = PlaybackSpeeds[ pToolData->PlaybackSpeedIndex];
  if( TempDouble < 0) {  // Slower playback

    TempDouble = -1.0 / TempDouble;
  }

  pToolData->PlaybackSpeedTarget = TempDouble;
  pToolData->PlaybackSpeedCurr   = -1.0;              // Force change on playback speed

  // Get info about video

  TempDouble = ccap_provider_get_property( pToolData->pCcapProvider, CCAP_PROPERTY_FRAME_RATE);  // Frame rate
  pToolData->FrameRate = (float)TempDouble;

  TempDouble = ccap_provider_get_property( pToolData->pCcapProvider, CCAP_PROPERTY_WIDTH);       // Frame width
  pToolData->FrameWidth = (int)TempDouble;

  TempDouble = ccap_provider_get_property( pToolData->pCcapProvider, CCAP_PROPERTY_HEIGHT);      // Frame height
  pToolData->FrameHeight = (int)TempDouble;

  TempDouble = ccap_provider_get_property( pToolData->pCcapProvider, CCAP_PROPERTY_FRAME_COUNT); // Total number of frames
  pToolData->FrameCount = (int)TempDouble;

  TempDouble = ccap_provider_get_property( pToolData->pCcapProvider, CCAP_PROPERTY_DURATION);    // Duration of video in seconds
  pToolData->FrameTimeVideo = TempDouble;

  // Remember last loaded file name
  memset( pToolData->LastFileName, 0, sizeof( pToolData->LastFileName));
  strncpy( pToolData->LastFileName, pFileName, sizeof( pToolData->LastFileName) - 1);

  IqeB_FileNormPathCharsAndCWD( pToolData->LastFileName);  // Ensure normalized path characters and working directory

  // Remember last used directory
  IqeB_FileGetPath( pFileName, YaIPS_BrowserDirVideos, sizeof( YaIPS_BrowserDirVideos));

  IqeB_FileNormPathCharsAndCWD( YaIPS_BrowserDirVideos);  // Ensure normalized path characters and working directory

  pToolData->VideoState |= VIDEO_STATE_LOADED;        // Video is loaded

  if( pToolData->Video_Autoplay) {                    // Video auto play is set

    pToolData->VideoState |= VIDEO_STATE_PLAY_ON;     // Play video to on
  }

  pToolData->VideoAction = VIDEO_ACTION_NONE;         // Everything OK here. Need no video action.

  if( pToolData->Video_UseBuffer) {                   // Use the video buffer

    VideoBufferAlloc( pToolData);                     // Handle video buffer allocation
  }

  return( 0);      // Return OK
}

/************************************************************************************
 * Load_cb
 *
 * Handle a load file request from the button
 */

static void Load_cb( Fl_Widget *w, long int iToolData)
{
  Fl_Native_File_Chooser fc;
  YaIPS_ToolData_info_t *pToolData;
  //x/CLASS_WIN_TOOL *pMyToolWin;
  char FileFilter[ 1024];
  char *pFileName;
  int ierr;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  //x/pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  if( pToolData->pCcapProvider == NULL) {    // Security test

    return;
  }

  // Initialize the file chooser

  strcpy( FileFilter, LANGDEF_VIDEOS);
  strcat( FileFilter, "\t*.{");
  strcat( FileFilter, YAIPS_VIDEO_FILES_READ_KNOWN);
  strcat( FileFilter, "}\n");

  fc.filter( FileFilter);

  fc.title( LANGDEF_FILE_LOAD_VIDEO);
  fc.type( Fl_Native_File_Chooser::BROWSE_FILE);  // only picks files that exist
  fc.directory( YaIPS_BrowserDirVideos);          // Set browser directory
  ierr = fc.show();                               // Open file chooser dialog

  if( ierr != 0) {      // User cancelled or error

    return;
  }

  // Close video from before

  if( ccap_provider_is_opened( pToolData->pCcapProvider)) {             // Other device is open

    ccap_provider_close( pToolData->pCcapProvider);
  }

  // Have a filename here

  pFileName = (char *)fc.filename();

  // Try to load video file

  VideoLoadFile( pToolData, pFileName);

  MyWinUpdate( iToolData, true);                 // Update the GUI
}

/************************************************************************************
 * DropFile_cb
 *
 * A file was dropped to the image box
 *
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
  CLASS_WIN_TOOL *pMyToolWin;
  char *pFileName;
  int iToolData;

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
    pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

    if( pMyToolWin == NULL) {                                    // Security test

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

    pFileName = (char *)pFileNameArg;                      // Filename

    // Check for a known video file extension

    if( IqeB_FileCheckExtension( pFileName, YAIPS_VIDEO_FILES_READ_KNOWN) != true) {

      // Extension is not known
      goto ErrorExit;
    }

    // Try to load video file

    VideoLoadFile( pToolData, pFileName);

    // Done, can exit here

    MyWinUpdate( iToolData, true);                 // Update the GUI

    return( 0);    // OK Processed drop
  }

ErrorExit:

  return( -1);   // Error on processing drop
}

/************************************************************************************
 * YaIPS_ToolWin_GUI_Callback
 *
 * Main window button callbacks
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

  } else if( w == pMyToolWin->pGUI_Video_PlayPause) {          // Play video

    if( (pToolData->VideoState & VIDEO_STATE_LOADED) == 0) {   // No video loaded

      pToolData->VideoState &= ~VIDEO_STATE_PLAY_ON;           // Play to off

    } else {

      pToolData->VideoState ^= VIDEO_STATE_PLAY_ON;            // Toggle play/pause
    }

  } else if( w == pMyToolWin->pGUI_Video_PosBegin) {           // Back to the beginning of the video

    if( (pToolData->VideoState & VIDEO_STATE_LOADED) != 0) {   // Video loaded

      pToolData->VideoAction = VIDEO_ACTION_POS_BEGIN;
    }

  } else if( w == pMyToolWin->pGUI_Video_SkipBack) {           // Skip back

    if( (pToolData->VideoState & VIDEO_STATE_LOADED) != 0) {   // Video loaded

      pToolData->VideoAction = VIDEO_ACTION_SKIP_BACK;
      pToolData->VideoActionArg = Fl::event_state();
    }

  } else if( w == pMyToolWin->pGUI_Video_SkipForw) {           // Skip forward

    if( (pToolData->VideoState & VIDEO_STATE_LOADED) != 0) {   // Video loaded

      pToolData->VideoAction = VIDEO_ACTION_SKIP_FORW;
      pToolData->VideoActionArg = Fl::event_state();
    }

  } else if( w == pMyToolWin->pGUI_Video_SpeedDec) {           // Decrement playback speed

    if( (pToolData->VideoState & VIDEO_STATE_LOADED) == 0) {   // No video loaded

      pToolData->PlaybackSpeedIndex = PLAYBACL_SPEED_DEF;      // Set to default

    } else {

      double TempDouble;

      pToolData->PlaybackSpeedIndex -= 1;                      // Decrement value

      if( pToolData->PlaybackSpeedIndex < 0) {                 // Clip to minimum

        pToolData->PlaybackSpeedIndex = 0;
      }

      // Set playback speed

      TempDouble = PlaybackSpeeds[ pToolData->PlaybackSpeedIndex];
      if( TempDouble < 0) {  // Slower playback

        TempDouble = -1.0 / TempDouble;
      }

      pToolData->PlaybackSpeedTarget = TempDouble;
    }

  } else if( w == pMyToolWin->pGUI_Video_SpeedInc) {           // Increment playback speed

    if( (pToolData->VideoState & VIDEO_STATE_LOADED) == 0) {   // No video loaded

      pToolData->PlaybackSpeedIndex = PLAYBACL_SPEED_DEF;      // Set to default

    } else {

      double TempDouble;

      pToolData->PlaybackSpeedIndex += 1;                      // Increment value

      if( pToolData->PlaybackSpeedIndex >= PLAYBACL_SPEED_N) {  // Clip to maximum

        pToolData->PlaybackSpeedIndex = PLAYBACL_SPEED_N - 1;
      }

      // Set playback speed

      TempDouble = PlaybackSpeeds[ pToolData->PlaybackSpeedIndex];
      if( TempDouble < 0) {  // Slower playback

        TempDouble = -1.0 / TempDouble;
      }

      pToolData->PlaybackSpeedTarget = TempDouble;
    }

  } else if( w == pMyToolWin->pGUI_Parameter) {                // Open parameter dialog

    YaIPS_GUI_ParameterWin( pMyToolWin->x() + 16, pMyToolWin->y() + 16, iToolData);

  } else if( w == pMyToolWin->pGUI_Repeat) {                   // Toggle video repeat mode

    pToolData->Video_Repeat = ! pToolData->Video_Repeat;
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
  int TempEnable, EventState;
  char TempString[ 256];
  char *pTempString;

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

  // Info about loaded file

  pToolData->YaIPS_ImageDisp.ColInfoTextSize = 12;                // Use reduced font size

  if( pToolData->VideoState == VIDEO_STATE_NONE ||                // No video loaded
      pToolData->LastFileName[ 0] == '\0') {                      // Or no filename loaded

    YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, " --- "); // Info

  } else {

    char TempString1[ FILENAME_MAX], TempString2[ FILENAME_MAX], TempString3[ FILENAME_MAX];
    int MaxHour, MaxMinutes, MaxSeconds, CurHour, CurMinutes, CurSeconds;

    IqeB_FileGetFileName( pToolData->LastFileName, TempString1, sizeof( TempString1));   // Get filename without path

    MaxSeconds = (int)pToolData->FrameTimeVideo;
    MaxMinutes = MaxSeconds / 60;
    MaxHour    = MaxMinutes / 60;
    MaxSeconds -= MaxMinutes * 60;
    MaxMinutes -= MaxHour * 60;

    CurSeconds = (int)(pToolData->FrameTimeCurr - pToolData->BufferFrameBack / pToolData->FrameRate);
    CurMinutes = CurSeconds / 60;
    CurHour    = CurMinutes / 60;
    CurSeconds -= CurMinutes * 60;
    CurMinutes -= CurHour * 60;

    if( MaxHour <= 0) {          // Video duration is below one hour

      sprintf( TempString2, "%s\n%02d:%02d / %02d:%02d  %d x %d  FPS %d",
                            TempString1,
                            CurMinutes, CurSeconds, MaxMinutes, MaxSeconds,
                            pToolData->FrameWidth, pToolData->FrameHeight, (int)lround( pToolData->FrameRate));
    } else {

      sprintf( TempString2, "%s\n%d:%02d:%02d / %d:%02d:%02d  %d x %d  FPS %d",
                            TempString1,
                            CurHour, CurMinutes, CurSeconds, MaxHour, MaxMinutes, MaxSeconds,
                            pToolData->FrameWidth, pToolData->FrameHeight, (int)lround( pToolData->FrameRate));
    }

    if( pToolData->pBuffer &&            // Have a video buffer allocated
        pToolData->BufferFramesN > 1) {  // Any data buffered

      // How many step back is possible
      sprintf( TempString3, "\n<= %d / %d", pToolData->BufferFrameBack, pToolData->BufferFramesN - 1);

      strcat( TempString2, TempString3);
    }

    YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, TempString2);
  }

  // Update playback speed

  if( pToolData->VideoState == VIDEO_STATE_NONE) {               // No video loaded

    pToolData->PlaybackSpeedIndex = PLAYBACL_SPEED_DEF;          // Ensure default value
  }

  if( PlaybackSpeeds[ pToolData->PlaybackSpeedIndex] == 1) {     // Normal

    sprintf( TempString, "%d", PlaybackSpeeds[ pToolData->PlaybackSpeedIndex]);

  } else if( PlaybackSpeeds[ pToolData->PlaybackSpeedIndex] >= 0) {     // Faster

    sprintf( TempString, "x %d", PlaybackSpeeds[ pToolData->PlaybackSpeedIndex]);

  } else {                                                        // Slower

    sprintf( TempString, "1/%d", -PlaybackSpeeds[ pToolData->PlaybackSpeedIndex]);
  }

  pTempString = (char *)pMyToolWin->pGUI_Box_PBS->label();     // Get label

  if( pTempString == NULL ||                                   // String is different
      strcmp( pTempString, TempString) != 0) {

    pMyToolWin->pGUI_Box_PBS->label( NULL);                    // Free old text
    pMyToolWin->pGUI_Box_PBS->copy_label( TempString);         // Set new text
  }

  // Update repeat button

#ifdef use_again
  Fl_Color TempColor;

  TempColor = pToolData->Video_Repeat ? FL_GREEN /*(FL_BLUE + 7)*/ : FL_BACKGROUND_COLOR;

  if( pMyToolWin->pGUI_Repeat->color() != TempColor) {

    pMyToolWin->pGUI_Repeat->color( TempColor);
    pMyToolWin->pGUI_Repeat->redraw();
  }
#else
  IqeB_GUI_WidgetLabelColor( pMyToolWin->pGUI_Repeat, pToolData->Video_Repeat ? FL_GREEN : YAIPS_BCOL_BUTTON);
#endif

  // ...

  if( DoEnable == 2) {                                          // Was periodically updates only

    return;
  }

  // ...

  EventState = Fl::event_state();

  TempEnable = DoEnable && (pToolData->VideoState & VIDEO_STATE_LOADED) != 0;

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_ShowOnBig,
                           TempEnable &&                                     // Enable GUI elements
                           pToolData->YaIPS_ImageDisp.pImage_Img != NULL);   // and have an image loaded

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Video_PlayPause,
                           TempEnable);                                      // Enable GUI elements

  pTempString = TempEnable && (pToolData->VideoState & VIDEO_STATE_PLAY_ON) != 0 ? (char *)"@+3||" : (char *)"@+3>";
  if( strcmp( pTempString, pMyToolWin->pGUI_Video_PlayPause->label())  != 0) {

    pMyToolWin->pGUI_Video_PlayPause->label( pTempString);
  }

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Video_PosBegin,
                           TempEnable && pToolData->FrameIndex > 0);         // Enable GUI elements

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Video_SkipBack,
                           TempEnable &&
                           pToolData->FrameIndex > 0 &&                            // Can step back
                           ( (pToolData->VideoState & VIDEO_STATE_PLAY_ON) != 0 || // or play is on
                             (EventState & (FL_CTRL | FL_SHIFT)) != 0 ||           // or shift or ctrl key is pressed
                             pToolData->pBuffer == NULL ||                         // or no buffer allocated
                             pToolData->BufferFramesN <= 1 ||                      // or not more than one image in the buffer
                             pToolData->BufferFrameBack < pToolData->BufferFramesN - 1)); // can go back

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Video_SkipForw,
                           TempEnable &&
                           pToolData->FrameIndex - pToolData->BufferFrameBack < pToolData->FrameCount - 1); // Can step forward

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Video_SpeedDec,
                           TempEnable && pToolData->PlaybackSpeedIndex > 0); // Enable GUI elements

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Video_SpeedInc,
                           TempEnable && pToolData->PlaybackSpeedIndex < PLAYBACL_SPEED_N - 1); // Enable GUI elements

  TempEnable = DoEnable && pToolData->pCcapProvider != NULL;

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Video_Load,                      // Enable GUI elements
                           TempEnable);

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Parameter,                       // Enable GUI elements
                           TempEnable);

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Repeat,                          // Enable GUI elements
                           TempEnable);
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
  if( TimeTemp - TimeLastCalled_100 >= 100) {   // 100 ms gone since last call

    TimedAction = true;
    TimeLastCalled_100 = TimeTemp;              // Remember last time called
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

    CcapVideoFrame *frame;
    CcapVideoFrameInfo frameInfo;
    int ForceUpdate, ierr, FramesSkip;
    char *pImageName;
#ifdef USE_DEBUG_OUTPUTS
    static unsigned int TimeStampLast = 0;
    static unsigned int TimeDeltaLast = 0;
    unsigned int TimeStampThis;
#endif
    double TempFrameTime;

    ForceUpdate = false;                         // Preset: NO Force update of output image
    frame = NULL;                                // Preset: NO video frame
    pImageName = NULL;                           // Preset: NO image name set

    // Have to black display ?

    if( pToolData->VideoAction == VIDEO_ACTION_BLACK_DISP) {       // Black display image (after an error or so)

      pToolData->WaitFrameCaptured = false;    // Reset frame is captured

      if( pToolData->YaIPS_ImageDisp.pImage_Img  != NULL) {

        YaIPS_RGB_SetVal( pToolData->YaIPS_ImageDisp.pImage_Img, 0); // Black the image

        ForceUpdate = true;                   // Ensure display of black image
      }
    } else if( pToolData->VideoAction == VIDEO_ACTION_PAR_CHECK) { // Parameter window has closed. Check for change of parameters.

      if( pToolData->YaIPS_ImageDisp.pImage_Img  != NULL) {

        ForceUpdate = true;                        // Ensure refresh of output image
      }

      if( pToolData->pBuffer == NULL) {            // No buffer until now

        if( pToolData->Video_UseBuffer) {          // Use the video buffer

          VideoBufferAlloc( pToolData);            // Try video buffer allocation
        }

      } else {                                     // There is a video buffer allocated

        if( ! pToolData->Video_UseBuffer) {                 // NOT use the video buffer

          // Close video buffer --> free video buffer memory

          VideoBufferClose( pToolData);

        } else if( pToolData->BufferMBytesAlloc != pToolData->Video_BufferSize) {  // Buffer size has changed.

          VideoBufferAlloc( pToolData);            // Try video buffer allocation
        }
      }
    }

    // Grab a video frame

    if( (pToolData->VideoState & VIDEO_STATE_LOADED) == 0) {  // Have NO video loaded

      pToolData->WaitFrameCaptured = false;         // Reset frame is captured

      pToolData->VideoAction = VIDEO_ACTION_NONE;   // All actions must be processed now

      goto SkipAcquire;
    }

    // Ensure RGB image with same size as video
    ierr = YaIPS_RGB_ImageSetSize( &pToolData->YaIPS_ImageDisp.pImage_Img, pToolData->FrameWidth, pToolData->FrameHeight, 3);

    if( ierr != 0) {                  // Problem with image

      pToolData->WaitFrameCaptured = false;         // Reset frame is captured

      pToolData->VideoAction = VIDEO_ACTION_NONE;   // All actions must be processed now

      goto SkipAcquire;
    }

    // Handle playback speed change

    if( (pToolData->VideoState & VIDEO_STATE_PLAY_ON) == 0) {   // and video is paused

      if( pToolData->PlaybackSpeedCurr != 0.0) {                // NOT in backpressure mode

        pToolData->PlaybackSpeedCurr = 0.0;                     // In pause switch to backpressure mode

        ccap_provider_set_property( pToolData->pCcapProvider, CCAP_PROPERTY_PLAYBACK_SPEED, pToolData->PlaybackSpeedCurr);
      }

    } else {

      if( pToolData->PlaybackSpeedCurr != pToolData->PlaybackSpeedTarget) { // NOT current target speed

        pToolData->PlaybackSpeedCurr = pToolData->PlaybackSpeedTarget;      // Change target speed

        ccap_provider_set_property( pToolData->pCcapProvider, CCAP_PROPERTY_PLAYBACK_SPEED, pToolData->PlaybackSpeedCurr);
      }
    }

    // Have frames to skip

    FramesSkip = 0;

    switch( pToolData->VideoAction) {

    case VIDEO_ACTION_POS_BEGIN:  // Position to begin of video

      if( (pToolData->VideoState & VIDEO_STATE_PLAY_ON) == 0) {   // Video is paused

        if( pToolData->FrameIndex == 0) {                         // Already show first frame

          pToolData->WaitFrameCaptured = false;    // Reset frame is captured

          pToolData->VideoAction = VIDEO_ACTION_NONE;   // All actions must be processed now

          goto SkipAcquire;
        }
      }

      // Restart video, should begin at first frame

      ccap_provider_stop( pToolData->pCcapProvider);    // Stop video playing

      // Try consume frame from the video buffer
      frame = ccap_provider_grab( pToolData->pCcapProvider, 2000 /*1*/);      // Poll for next frame

      if( frame != NULL) {                   // Got a frame

        // Release video frame
        ccap_video_frame_release( frame);
      }

      ccap_provider_set_property( pToolData->pCcapProvider, CCAP_PROPERTY_CURRENT_FRAME_INDEX, 0);

      ccap_provider_start( pToolData->pCcapProvider);   // Start video playing

      FramesSkip = 1;     // Flag frame skip to acquire next frame

      // Reset video buffer put/get pointer

      pToolData->BufferFramesN = 0;      // Number of used frames in buffer
      pToolData->BufferFramePut = 0;     // But next frame here in the buffer
      pToolData->BufferFrameBack = 0;    // If > 0 displayed buffered image

      break;

    case VIDEO_ACTION_SKIP_BACK:  // Skip back one frame


      if( (pToolData->VideoState & VIDEO_STATE_PLAY_ON) != 0 ||       // Video is playing
          (pToolData->VideoActionArg & (FL_CTRL | FL_SHIFT)) != 0 ||  // or shift or ctrl key is pressed
          pToolData->pBuffer == NULL ||                               // or have no video buffer
          pToolData->BufferFramesN <= 1) {                            // or not more than one image in the buffer

        // Stop video playing

        ccap_provider_stop( pToolData->pCcapProvider);            // Stop video playing

        // Try consume frame from the video buffer
        frame = ccap_provider_grab( pToolData->pCcapProvider, 2000 /*1*/);      // Poll for next frame

        if( frame != NULL) {                   // Got a frame

          // Release video frame
          ccap_video_frame_release( frame);
        }

        TempFrameTime = ccap_provider_get_property( pToolData->pCcapProvider, CCAP_PROPERTY_CURRENT_TIME);

        if( pToolData->VideoActionArg & FL_CTRL) {

          TempFrameTime -= 60.0;                                // Back for 1 minute

        } else if( pToolData->VideoActionArg & FL_SHIFT) {

          TempFrameTime -= 600.0;                               // Back for 10 minutes

        } else {

          TempFrameTime -= 10.0;                                // Back for 10 seconds
        }

        if( TempFrameTime < 0) {           // Clip

          TempFrameTime = 0;
        }

        ccap_provider_set_property( pToolData->pCcapProvider, CCAP_PROPERTY_CURRENT_TIME, TempFrameTime);

        ccap_provider_start( pToolData->pCcapProvider);   // Start video playing

        FramesSkip = 1;     // Flag frame skip to acquire next frame

        // Reset video buffer put/get pointer

        pToolData->BufferFramesN = 0;      // Number of used frames in buffer
        pToolData->BufferFramePut = 0;     // But next frame here in the buffer
        pToolData->BufferFrameBack = 0;    // If > 0 displayed buffered image

        break;
      }

      if( pToolData->pBuffer == NULL) {  // Security test, have no video buffer

        break;
      }

      if( pToolData->BufferFrameBack < pToolData->BufferFramesN - 1) {  // can go back

        pToolData->BufferFrameBack += 1;

        //  Get a frame from the video buffer and store it to the display image
        VideoBufferFrameGet( pToolData, pToolData->YaIPS_ImageDisp.pImage_Img);

        pToolData->WaitFrameCaptured = false;    // Reset frame is captured

        ForceUpdate = true;

        pImageName = pToolData->LastFileName;     // Set to current file name

        pToolData->VideoAction = VIDEO_ACTION_NONE;   // All actions must be processed now

        goto SkipAcquire;
      }

      // Noting to do here

     break;

    case VIDEO_ACTION_SKIP_FORW:  // Skip forward one frame

      if( (pToolData->VideoState & VIDEO_STATE_PLAY_ON) != 0 ||       // Video is playing
          (pToolData->VideoActionArg & (FL_CTRL | FL_SHIFT)) != 0) {  // Or shift or ctrl key is pressed

        // Stop video playing

        ccap_provider_stop( pToolData->pCcapProvider);    // Stop video playing

        // Try consume frame from the video buffer
        frame = ccap_provider_grab( pToolData->pCcapProvider, 2000 /*1*/);      // Poll for next frame

        if( frame != NULL) {                   // Got a frame

          // Release video frame
          ccap_video_frame_release( frame);
        }

        TempFrameTime = ccap_provider_get_property( pToolData->pCcapProvider, CCAP_PROPERTY_CURRENT_TIME);

        if( pToolData->VideoActionArg & FL_CTRL) {

          TempFrameTime += 60.0;                               // Forward for 1 minute

        } else if( pToolData->VideoActionArg & FL_SHIFT) {

          TempFrameTime += 600.0;                              // Forward for 10 minutes

        } else {

          TempFrameTime += 10.0;                               // Forward for 10 seconds
        }

        if( TempFrameTime >= pToolData->FrameTimeVideo - 1.5 / pToolData->FrameRate) {    // Clip

          TempFrameTime = pToolData->FrameTimeVideo - 2.0 / pToolData->FrameRate;
        }

        ccap_provider_set_property( pToolData->pCcapProvider, CCAP_PROPERTY_CURRENT_TIME, TempFrameTime);

        ccap_provider_start( pToolData->pCcapProvider);   // Start video playing

        FramesSkip = 1;     // Flag frame skip to acquire next frame

        // Reset video buffer put/get pointer

        pToolData->BufferFramesN = 0;      // Number of used frames in buffer
        pToolData->BufferFramePut = 0;     // But next frame here in the buffer
        pToolData->BufferFrameBack = 0;    // If > 0 displayed buffered image

        break;
      }

      if( pToolData->BufferFrameBack > 0) {               // Have frames buffered

        pToolData->BufferFrameBack -= 1;

        //  Get a frame from the video buffer and store it to the display image
        VideoBufferFrameGet( pToolData, pToolData->YaIPS_ImageDisp.pImage_Img);

        pToolData->WaitFrameCaptured = false;    // Reset frame is captured

        ForceUpdate = true;

        pImageName = pToolData->LastFileName;     // Set to current file name

        pToolData->VideoAction = VIDEO_ACTION_NONE;   // All actions must be processed now

        goto SkipAcquire;
      }

      // Nothing to do, simply read next frame from the video

      FramesSkip = 1;

      break;
    }  // End switch()

    pToolData->VideoAction = VIDEO_ACTION_NONE;   // All actions must be processed now
    pToolData->VideoActionArg = 0;

    // Need to do acquire

    if( FramesSkip == 0 &&                                        // and no frame skipping
        pToolData->WaitFrameCaptured == false) {                  // and not wait for frame capture done

      if( pToolData->FrameIndex >= 0 &&                           // Already have a first image
          (pToolData->VideoState & VIDEO_STATE_PLAY_ON) == 0) {   // and video is paused

        pToolData->WaitFrameCaptured = false;    // Reset frame is captured

        goto SkipAcquire;
      }

      // Play on and back in video buffer --> go forward in video buffer

      if( (pToolData->VideoState & VIDEO_STATE_PLAY_ON) != 0 &&   // Video is playing
          pToolData->BufferFrameBack > 0) {                       // Have frames buffered

        pToolData->BufferFrameBack -= 1;

        //  Get a frame from the video buffer and store it to the display image
        VideoBufferFrameGet( pToolData, pToolData->YaIPS_ImageDisp.pImage_Img);

        pToolData->WaitFrameCaptured = false;    // Reset frame is captured

        ForceUpdate = true;

        pImageName = pToolData->LastFileName;     // Set to current file name

        Sleep( 30);                               // Sleep some time to slow video display

        goto SkipAcquire;
      }
    }

    // Acquire a frame

    ccap_error_test();                       // Reset last ccap error

    pToolData->WaitFrameCaptured = false;    // Reset frame is captured

    frame = ccap_provider_grab( pToolData->pCcapProvider, 1);      // Poll for next frame

    ierr = ccap_error_test();              // Reset last ccap error

    if( frame == NULL) {                   // Acquire failed

#ifdef USE_DEBUG_OUTPUTS
      // Force update of screen for to display YaIPS_ImageDispStrDebug() output
      ForceUpdate = true;
#endif

      if( ierr == CCAP_ERROR_FRAME_CAPTURE_TIMEOUT) {             // Capture timeout, still waiting to get next frame

        pToolData->WaitFrameCaptured = true;    // Wait for next frame is captured

        goto SkipAcquire;
      }

      // Assume end of frame

      if( pToolData->FrameIndex == pToolData->FrameCount - 1) {    // Last frame streamed was the last frame of the vidoe

        pToolData->FrameTimeCurr = pToolData->FrameTimeVideo;      // Set proper end of frame time
      }

      if( (pToolData->VideoState & VIDEO_STATE_PLAY_ON) != 0) {    // Video is playing

        if( pToolData->Video_Repeat) {

          pToolData->VideoAction = VIDEO_ACTION_POS_BEGIN;         // Restart the video

        } else {

          pToolData->VideoState &= ~VIDEO_STATE_PLAY_ON;           // Play to off
        }
      }

      goto SkipAcquire;
    }

    // Got a Frame

#ifdef USE_DEBUG_OUTPUTS

    // Measure time between frames

    TimeStampThis = timeGetTime();

    TimeDeltaLast = TimeStampThis - TimeStampLast;
    if( TimeDeltaLast > 999) {   // Clip to reasonable value
      TimeDeltaLast = 999;
    }

    TimeStampLast = TimeStampThis;
#endif

    // Prepare frame from video for display

    int y, SizeOfLineDst, StrideSrc;
    uchar *pSrc, *pDst;
    uint8_t *data;

    ccap_video_frame_get_info( frame, &frameInfo);

    data = frameInfo.data[0];    // Data of the first plane
    StrideSrc = (int)frameInfo.stride[0];  // Size of one data line. May be bigger then 'with in pixel' * 'bytes per pixel'

    SizeOfLineDst = pToolData->FrameWidth * 3;

    pToolData->FrameTimeCurr = frameInfo.timestamp * 0.000000001;   // Frame time, convert nanoseconds to seconds

#ifdef use_again
    pToolData->FrameIndex = frameInfo.frameIndex;                // Unique, incremental frame index
#else
    pToolData->FrameIndex = lround( pToolData->FrameTimeCurr * pToolData->FrameRate);
#endif

#ifdef USE_DEBUG_OUTPUTS
    pToolData->FrameIndex2 = lround( ccap_provider_get_property( pToolData->pCcapProvider, CCAP_PROPERTY_CURRENT_FRAME_INDEX));
    pToolData->FrameTimeCurr2 = ccap_provider_get_property( pToolData->pCcapProvider, CCAP_PROPERTY_CURRENT_TIME);
#endif

    // Copy frame data to frame buffer

    pSrc = (uchar *)data;

    pDst = (uchar *)pToolData->YaIPS_ImageDisp.pImage_Img->data()[ 0];

    for( y = 0; y < pToolData->FrameHeight; y++) {

      memcpy( pDst, pSrc, SizeOfLineDst);

      pDst += SizeOfLineDst;
      pSrc += StrideSrc;
    }

    // Put a new frame into the video buffer
    VideoBufferFramePut( pToolData, (uchar *)data, StrideSrc);

    // Release video frame
    ccap_video_frame_release( frame);

    ForceUpdate = true;

    pImageName = pToolData->LastFileName;     // Set to current file name

    // If this was the last frame switch to pause

    if( pToolData->FrameIndex == pToolData->FrameCount - 1) {    // Last frame streamed was the last frame of the vidoe

      pToolData->FrameTimeCurr = pToolData->FrameTimeVideo;      // Set proper end of frame time

      if( (pToolData->VideoState & VIDEO_STATE_PLAY_ON) != 0) {  // Video is playing

        if( pToolData->Video_Repeat) {

          pToolData->VideoAction = VIDEO_ACTION_POS_BEGIN;       // Restart the video

        } else {

          pToolData->VideoState &= ~VIDEO_STATE_PLAY_ON;         // Play to off
        }
      }
    }

SkipAcquire:

#ifdef USE_DEBUG_OUTPUTS
    YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp, "F %5d(%d-%d)/%5d T %.2lf  %.2lf S %4.02lf D %3ld O%d S%d err 0x%04x",
                                    pToolData->FrameIndex - pToolData->BufferFrameBack, pToolData->FrameIndex, pToolData->BufferFrameBack,
                                    pToolData->FrameIndex2, pToolData->FrameTimeCurr, pToolData->FrameTimeCurr2,
                                    pToolData->PlaybackSpeedCurr, TimeDeltaLast,
                                    ccap_provider_is_opened( pToolData->pCcapProvider),
                                    ccap_provider_is_started( pToolData->pCcapProvider),
                                    My_ccap_last_error2); // Info
#endif

    if( pImageName == NULL) {                     // No image name set

      if( pToolData->WaitFrameCaptured) {         // Wait for next frame is captured

        pImageName = pToolData->LastFileName;     // Set to current file name
      }

      if( pImageName == NULL ||                   // No image name set
         pImageName[ 0] == '\0') {

        pImageName = (char *)"---";               // No image name set
      }
    }

    if( ForceUpdate) {             // Got a frame

      YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, pImageName);
    }

    // Check an image display for size change and redisplay if size has changed.
    YaIPS_ImageDispDrawUpdate( &pToolData->YaIPS_ImageDisp, ForceUpdate);

    if( ForceUpdate &&                                                 // New image
        YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {   // and display this on the big image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                         MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName);   // Load the image to the display
    }

    // ...

    MyWinUpdate( iToolData, true);               // Update the GUI

  }  // end: for( iToolData ...
}

/************************************************************************************
 * IqeB_GUI_VideoReadWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_VideoReadWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
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

  // Setup ccap

  // Set ccap error callback

  My_ccap_last_error = CCAP_ERROR_NONE;                   // Reset last error

  ccap_set_error_callback( My_ccap_error_callback, NULL);

  pToolData->pCcapProvider = ccap_provider_create();      // Create a capture object

  // Setup buffer for video backspace

  VideoBufferSetup( pToolData);

  // NOTE: no error check for ccap_provider_create();

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

  // Restore last loaded video

  char LastFileName[ FILENAME_MAX];     // File name of last loaded video file. This is inclusive path and file extension.

  // Need a copy of the last loaded file name

  memset( LastFileName, 0, sizeof( LastFileName));
  strncpy( LastFileName, pToolData->LastFileName, sizeof( LastFileName) - 1);

  if( LastFileName[ 0] != '\0') {               // There was a file loaded

    VideoLoadFile( pToolData, LastFileName);    // Load last used video file

  } else {

    VideoReset( pToolData);                     // Reset video data to defaults
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

  // Don't have a video loaded
  if( pToolData->VideoState == VIDEO_STATE_NONE ||                // No video loaded
      pToolData->LastFileName[ 0] == '\0') {                      // Or no filename loaded

    return;
  }

  // Draw progress bar

  if(pYaIPS_ImageDisp->pImage_Img != NULL) {                  // Have a source image

    int x1, x2, y1, xx, xx2, yy;
    Fl_Color ColBackground, ColForeground, ColFIFO, ColCursor;

    // Preparations

    x1 = pYaIPS_ImageDisp->BigImage_bx;
    y1 = pYaIPS_ImageDisp->BigImage_by;
    xx = pYaIPS_ImageDisp->BigImage_bw;
    yy = pYaIPS_ImageDisp->BigImage_bh;

    // Clipping ?

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( x1, y1, xx, yy);
    }

    // Prepare drawing of size bars

#ifndef use_again
    ColBackground = FL_DARK_GREEN - 2;
    ColForeground = FL_GREEN - 2;
    ColFIFO       = FL_CYAN;
    ColCursor     = FL_MAGENTA;
#else
    ColBackground = FL_BLACK;
    ColForeground = FL_WHITE;
    ColFIFO       = FL_MAGENTA;
    ColCursor     = FL_RED;
#endif

    // Draw horizontal size marker at top side

    x1 = pYaIPS_ImageDisp->BigImage_bx + 8;
    y1 = pYaIPS_ImageDisp->BigImage_by + 4;
    xx = pYaIPS_ImageDisp->BigImage_bw - 16;
    yy = 2;

    // Background line

    fl_color( ColBackground);
    fl_rectf( x1, y1, xx, yy);

    // Video played

    xx2 = (xx * (pToolData->FrameIndex)) / (pToolData->FrameCount - 1);

    if( xx2 > 0) {

      fl_color( ColForeground);
      fl_rectf( x1, y1 - 1, xx2, yy + 2);
    }

    // Buffer filled

    if( pToolData->BufferFramesN > 1) {

      fl_color( ColFIFO);

      x2 = x1 + xx2;

      xx2 = (xx * pToolData->BufferFramesN + pToolData->FrameCount / 2) / (pToolData->FrameCount - 1);

      if( xx2 <= 1) {

        xx2 = 1;
      }

      x2 -= xx2;

      fl_rectf( x2, y1 - 1, xx2, yy + 2);

      // Play cursor

      if( pToolData->BufferFrameBack > 0) {

        fl_color( ColCursor);

        x2 = x2 + (xx * (pToolData->BufferFramesN - 1 - pToolData->BufferFrameBack) + pToolData->FrameCount / 2) / (pToolData->FrameCount - 1);

        fl_rectf( x2, y1 - 2, 2, yy + 4);
      }

    }

    // ...

    fl_line_style( 0);   // Reset to default
  }

//x/ExitPoint:

  // Finish up

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
#ifdef use_again    // Is needed for display on big image --> don't want this
static void YaIPS_GUI_MyDrawAfter_Other( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // point to image display data
                                         int SubWinIDx)                           // What sub window to use

{
  YaIPS_ToolData_info_t *pToolData;

  pToolData = YaIPS_ToolData_info + SubWinIDx;               // Point to info data

  YaIPS_GUI_MyDrawAfter_Func( pYaIPS_ImageDisp, pToolData, false);
}
#endif

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
 * IqeB_GUI_VideoReadWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_VideoReadWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

	  IqeB_GUI_VideoReadWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  IqeB_GUI_VideoReadWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


