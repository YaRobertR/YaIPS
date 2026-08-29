/****************************************************************************

  YaIPS_GUI_VideoWrite.cpp

  Create a video.

  03.06.2026 RR: First edition of this file.

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
#include <ccap_writer_c.h>

// Other includes
#include "YaIPS.h"

/************************************************************************************
* Defines for this source file.
*/

// Defines for windows ID
#define MY_WIN_ID     YAIPS_WIN_ID_VIDEO_WRITE        // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_VIDEO_WRITE       // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_VideoW_Title=Video writer"     // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME) // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinVideoWrite"             // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_VideoWrite_Tool   // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       287  /* YAIPS_WIN_SIZE_S1_X_MIN */
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

#define VIDEO_STATE_NONE             0   // None of the bits is set
#define VIDEO_STATE_SIZE_OK     0x0001   // Bit set: Size of output buffer is OK
#define VIDEO_STATE_BGND_BLACK  0x0002   // Bit set: Image is set to black
#define VIDEO_STATE_RECORD_ON   0x0004   // Bit set: Recording a video
#define VIDEO_STATE_PAUSED      0x0008   // Bit set: Recording is paused


#define VIDEO_ACTION_NONE           0   // Nothing to do
#define VIDEO_ACTION_UPDATE_DISP    1   // Update display
#define VIDEO_ACTION_PAUSE_OFF      2   // Pause switched to off
#define VIDEO_ACTION_STOP_VIDEO     3   // Stop the video

typedef struct {

  // Parameters
  int IsOpen;                           // True if this window is open.
  void *pMyToolWin;                     // Pointer to window data ( is pointer to CLASS_WIN_TOOL)

  int MyWinPosX, MyWinPosY;             // last window position
  int MyWinSizeX, MyWinSizeY;           // last window size

  char LastFileName[ FILENAME_MAX];     // File name of last loaded video file. This is inclusive path and file extension.

  int Input1_WinIdNr;                   // Window ID nr of 1. input

  // Used for intern data management

  Fl_YaIPS_ImageDisp_t YaIPS_ImageDisp;   // Info image output

  // Used to catch a change of the input image
  int Input1_Change;                    // Last processed 'ImageChanged' from input image
  int Input1_Recorded;                  // Last processed input image recored to video

  CcapVideoWriter *pCcapVideoWriter;    // CCap: write video instance

  int VideoState;                       // State of video

  Fl_RGB_Image *pImgTmpColConv;         // Temporary image used for color conversion
  Fl_RGB_Image *pImgTmpScale;           // Temporary image used for resize of image
  Fl_RGB_Image *pImgTmpTrans;           // Temporary image used for transition between images
  Fl_RGB_Image *pImgTmpRGB2BGR;         // Temporary image used for RGB to BGR transformation

  int FramesRecorted;                   // Number of frames recorded until now
  int FrameCntDownImage;                // Count down frames adding for an image
  int FramesTransition;                 // Number of frames for a transition
  int FrameCntDownTrans;                // Count down frames for a transition

  int VideoAction;                      // != 0 Something to do in video recording loope

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int VideoWidth;                       // With of video
  int VideoHeight;                      // Height of video
  int VideoFPS;                         // Video FPS
  int VideoCodec;                       // Codec used for video
  float VideoQuality;                   // Video quality improvement

  unsigned int BGND_Color;              // Background Color

  float TimeAddImages;                  // Image add options: Add image seconds
  float TimeTransition;                 // Image add options: Transition time from one image to an other
  int RecordMode;                       // 0: Record 'image' using time parameter 1: record 'video'

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

  { PREF_T_STRING,   "LastFileName",   "", &YaIPS_ToolData_info[0].LastFileName, sizeof( YaIPS_ToolData_info[0].LastFileName) - 1 },

  { PREF_T_INT,    "Input1_WinIdNr", "-1", &YaIPS_ToolData_info[0].Input1_WinIdNr },

  //
  // Parameter Dialog
  //

  { PREF_T_INT,    "MyParPosX",  IQE_GUI_NO_WINPOS_X_STRING, &YaIPS_ToolData_info[0].MyParPosX }, // NOTE: values will be clipped against MYWIN_SIZE_X_MIN / MYWIN_SIZE_Y_MIN
  { PREF_T_INT,    "MyParPosY",  IQE_GUI_NO_WINPOS_Y_STRING, &YaIPS_ToolData_info[0].MyParPosY },

  // Hold last selected tab

  { PREF_T_INT,       "VideoWidth", "1280", &YaIPS_ToolData_info[0].VideoWidth},
  { PREF_T_INT,      "VideoHeight",  "720", &YaIPS_ToolData_info[0].VideoHeight},
  { PREF_T_INT,         "VideoFPS",   "25", &YaIPS_ToolData_info[0].VideoFPS},
  { PREF_T_INT,       "VideoCodec",    "0", &YaIPS_ToolData_info[0].VideoCodec},
  { PREF_T_FLOAT,   "VideoQuality",  "1.0", &YaIPS_ToolData_info[0].VideoQuality},
  { PREF_T_INT,       "BGND_Color",    "0", &YaIPS_ToolData_info[0].BGND_Color},

  { PREF_T_FLOAT,  "TimeAddImages",    "0", &YaIPS_ToolData_info[0].TimeAddImages},
  { PREF_T_FLOAT, "TimeTransition",    "0", &YaIPS_ToolData_info[0].TimeTransition},
  { PREF_T_INT,       "RecordMode",    "0", &YaIPS_ToolData_info[0].RecordMode},
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_VideoWriteWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME /*, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp*/);
                                               // NOTE: No output image.

//-----------------------------------------------------------------------------------
// Parameter dialog
//
// This is a modal dialog. Therefore we can use global variables to hold
// info about the data.
//-----------------------------------------------------------------------------------

// ...

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolData;     // NOTE: Is used by all parameter dialog functions
static IqeFl_Int_Input *pInt_VideoWidth, *pInt_VideoHeight, *pInt_VideoFPS;
static Fl_Menu_Button  *pMBut_Sizes;
static Fl_Choice *pChoice_VideoCodec;
static IqeFl_Float_Input *pFloat_VideoQuality;
static Fl_Output *pShowNewSize;

//-----------------------------------------------------------------------------------
// Ccap error handling functions
//
//-----------------------------------------------------------------------------------

static int My_ccap_last_error = CCAP_ERROR_NONE;  // Last error code
#ifdef _DEBUG
static char My_ccap_last_err_string[ 1024];
#endif

// Error callback function

static void My_ccap_error_callback( CcapErrorCode errorCode, const char* errorDescription, void* userData)
{

  My_ccap_last_error = errorCode;         // Latch last error occurred

#ifdef _DEBUG
  sprintf( My_ccap_last_err_string, "Error - Code: %d, Description: %s\n", (int)errorCode, errorDescription);
#endif
}

// Code picked from: ccap_writer_c.h
/// Compute auto bit rate based on resolution and frame rate.
///
/// Reference: YouTube official recommended bitrates for H.264 encoding.
/// https://support.google.com/youtube/answer/2853702
///
/// YouTube H.264 reference points (Mbps):
///   720p  @30fps â†’ 7.5    720p  @60fps â†’ 9.0
///   1080p @30fps â†’ 10     1080p @60fps â†’ 12
///   1440p @30fps â†’ 15     1440p @60fps â†’ 24
///   2160p @30fps â†’ 30     2160p @60fps â†’ 35
///
/// For resolutions below 720p, the 720p rate is used as floor.
/// For resolutions between reference points, bit rate is linearly interpolated by pixel count.
/// For resolutions above 4K, bit rate is extrapolated by pixel count.
/// For HEVC, bit rate is scaled down to ~60% of H.264 (HEVC achieves similar quality at lower bit rate).
static uint64_t computeAutoBitRate( uint32_t width, uint32_t height, double frameRate, CcapVideoCodec codec, float QualityFaktor) {
    const double kMbps = 1'000'000.0;
    const double fps = (frameRate > 0.0) ? frameRate : 30.0;
    const bool is60fps = fps > 45.0;

    // YouTube H.264 reference data points: (pixelCount, bitrateInMbps)
    struct RefPoint {
        double pixels;
        double bitrateMbps;
    };
    static const RefPoint refs30[] = {
        { 1280 * 720, 7.5 },
        { 1920 * 1080, 10.0 },
        { 2560 * 1440, 15.0 },
        { 3840 * 2160, 30.0 },
    };
    static const RefPoint refs60[] = {
        { 1280 * 720, 9.0 },
        { 1920 * 1080, 12.0 },
        { 2560 * 1440, 24.0 },
        { 3840 * 2160, 35.0 },
    };

    const RefPoint* refs = is60fps ? refs60 : refs30;
    const int refCount = 4;
    const double pixels = static_cast<double>(width) * height;
    double bitrateMbps;

    if (pixels <= refs[0].pixels) {
        // Below 720p: use 720p floor
        bitrateMbps = refs[0].bitrateMbps;
    } else if (pixels >= refs[refCount - 1].pixels) {
        // Above 4K: extrapolate using slope of the last two reference points
        const auto& a = refs[refCount - 2];
        const auto& b = refs[refCount - 1];
        double slope = (b.bitrateMbps - a.bitrateMbps) / (b.pixels - a.pixels);
        bitrateMbps = b.bitrateMbps + slope * (pixels - b.pixels);
    } else {
        // Between reference points: linear interpolation by pixel count
        int i = 0;
        while (i < refCount - 1 && pixels > refs[i + 1].pixels)
            i++;
        const auto& lo = refs[i];
        const auto& hi = refs[i + 1];
        double t = (pixels - lo.pixels) / (hi.pixels - lo.pixels);
        bitrateMbps = lo.bitrateMbps + t * (hi.bitrateMbps - lo.bitrateMbps);
    }

    // Scale for non-standard frame rates between 30 and 60
    if (!is60fps && fps > 30.0) {
        const auto& r30 = refs30;
        const auto& r60 = refs60;
        // Average ratio across reference points
        double ratio = 0;
        for (int i = 0; i < refCount; i++)
            ratio += r60[i].bitrateMbps / r30[i].bitrateMbps;
        ratio /= refCount; // ~1.27
        double t = (fps - 30.0) / 30.0;
        bitrateMbps *= (1.0 + t * (ratio - 1.0));
    }

    // HEVC achieves similar visual quality at ~60% of H.264 bit rate
    if (codec == CCAP_VIDEO_CODEC_HEVC) {
        bitrateMbps *= 0.6;
    }

    return( bitrateMbps * kMbps * QualityFaktor);
}

/************************************************************************************
 * VideoWriteFileOpen
 *
 * Ask for and open a video file for writing.
 *
 * pFileName   Try to load this video vile
 *             If NULL reset all video information to no video
 *
 * Return:    0   OK
 *         else  Error
 */

static int VideoWriteFileOpen( YaIPS_ToolData_info_t *pToolData)
{
  Fl_Native_File_Chooser fc;
  //x/CLASS_WIN_TOOL *pMyToolWin;
  char FileFilter[ 1024];
  char *pFileName;
  char TempFileName[ FILENAME_MAX + 16];
  int ierr;
  //x/ int iFileType;
  CcapWriterConfig WriterConfig;

  //x/pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  // Need an open video write object

  if( pToolData->pCcapVideoWriter == NULL) {  // Have NO video writer object

    return( -1);
  }

  // Ensure open video file is closed

  if( ccap_video_writer_is_opened( pToolData->pCcapVideoWriter)) { // Device is open

    ccap_video_writer_close( pToolData->pCcapVideoWriter);
  }

  // Initialize the file chooser. Only can save png images

  strcpy( FileFilter, LANGDEF_VIDEOS);
  strcat( FileFilter, "\t*.{");
  strcat( FileFilter, YAIPS_VIDEO_FILES_WRITE_KNOWN);
  strcat( FileFilter, "}\n");

  fc.filter( FileFilter);
  fc.options( Fl_Native_File_Chooser::SAVEAS_CONFIRM | Fl_Native_File_Chooser::USE_FILTER_EXT);

  pFileName = NULL;      // NO preset file name

  pFileName = LangStringLookup( "&GUI_VideoW_Save1=Video");

  if( pFileName != NULL) {

    // Remove the file extension of the preset file name.
    IqeB_FileEnsureExtension( pFileName, NULL /*FileTypes[ iFileType]*/, TempFileName, sizeof( TempFileName));
    fc.preset_file( TempFileName);
  }

  fc.title( LangStringLookup( "&GUI_VideoW_Save2=Save video"));
  fc.type( Fl_Native_File_Chooser::BROWSE_SAVE_FILE);  // need this if file doesn't exist yet
  fc.directory( YaIPS_BrowserDirVideos);               // Set browser directory
  ierr = fc.show();                                    // Open file chooser dialog

  if( ierr != 0) {      // User cancelled or error

    return( -2);
  }

  // Have a filename here

  // Have a filename here. Ensure a proper file extension.

  pFileName = (char *)fc.filename();
  //x/iFileType = fc.filter_value();        // What file to save

#ifdef use_again
  if( iFileType >= 0 && iFileType < YAIPS_IMAGE_FILES_WRITE_TAB_N) {

    IqeB_FileEnsureExtension( pFileName, FileTypes[ iFileType], TempFileName, sizeof( TempFileName));

    pToolData->WriteFileType_Last = iFileType;
  } else {

    IqeB_FileEnsureExtension( pFileName, (char *)"mp4", TempFileName, sizeof( TempFileName));
    pToolData->WriteFileType_Last = 0;
  }
#else
  IqeB_FileEnsureExtension( pFileName, (char *)"mp4", TempFileName, sizeof( TempFileName));
#endif

  pFileName = TempFileName;

  // Try to open a video file for write

  memset( &WriterConfig, 0, sizeof( WriterConfig));   // Zero all

  WriterConfig.codec     = (CcapVideoCodec)pToolData->VideoCodec;   ///< Preferred codec
  WriterConfig.container = CCAP_VIDEO_FORMAT_MP4;   ///< Container format
  WriterConfig.width     = pToolData->VideoWidth;   ///< Frame width
  WriterConfig.height    = pToolData->VideoHeight;  ///< Frame height
  WriterConfig.frameRate = pToolData->VideoFPS;     ///< Target frame rate; 0 lets open() normalize to 30fps

  // With and height must be even

  if( (WriterConfig.width & 1) != 0) {    // Is odd

    WriterConfig.width -= 1;              // Make even
  }

  if( (WriterConfig.height & 1) != 0) {   // Is odd

    WriterConfig.height -= 1;             // Make even
  }

  // Bit rate
  if( pToolData->VideoQuality < 0.5) {    // Clip to sane default
    pToolData->VideoQuality = 0.5;
  }

  WriterConfig.bitRate   = computeAutoBitRate( WriterConfig.width, WriterConfig.height, WriterConfig.frameRate, WriterConfig.codec, pToolData->VideoQuality);


  if( ccap_video_writer_open( pToolData->pCcapVideoWriter, pFileName, &WriterConfig) == false) {

    // Open video for writing failed

    return( -10);
  }

  // Remember last loaded file name
  memset( pToolData->LastFileName, 0, sizeof( pToolData->LastFileName));
  strncpy( pToolData->LastFileName, pFileName, sizeof( pToolData->LastFileName) - 1);

  // Remember last used directory
  IqeB_FileGetPath( pFileName, YaIPS_BrowserDirVideos, sizeof( YaIPS_BrowserDirectory));

  return( 0);   // Return OK
}

/************************************************************************************
 * VideoWriteToFile
 *
 * Write a frame to the video file
 *
 * pFrame     Pointer to image to be written into the video file
 *
 * Return:    0   OK
 *         else  Error
 */

static int VideoWriteToFile( YaIPS_ToolData_info_t *pToolData, Fl_RGB_Image *pFrame)
{
  CcapVideoFrameInfo info;

  // Check prerequisites

  if( pToolData->pCcapVideoWriter == NULL) {                                // Have NO video writer object

    return( -1);
  }

  if( ccap_video_writer_is_opened( pToolData->pCcapVideoWriter) == false) { // File to write is NOT open

    return( -2);
  }

  if( pToolData->pImgTmpRGB2BGR == NULL) {                                  // Have NO temporary image buffer

    return( -3);
  }

  if( pFrame == NULL) {                                                    // Frame pointer is NULL

    return( -4);
  }

  // Must convert input image RGB to BGR. ccap_video_writer_write_frame() can't work with RGB images.

  YaIPS_RGB_Color_ConvSimple( &pToolData->pImgTmpRGB2BGR, pFrame, YAIPS_COLOR_RGB_2_BGR);

  // Give image to video writer

  memset( &info, 0, sizeof( info));      // Reset all to 0

  info.data[ 0] = (uchar *)pToolData->pImgTmpRGB2BGR->data()[ 0];               /**< Pointers to frame data planes */
  info.stride[ 0] = pToolData->VideoWidth * 3;                                  /**< Stride (bytes per row) for each plane */
  info.pixelFormat = CCAP_PIXEL_FORMAT_BGR24;                                   /**< Pixel format of the frame */
  info.width = pToolData->VideoWidth;                                           /**< Frame width in pixels */
  info.height = pToolData->VideoHeight;                                         /**< Frame height in pixels */
  info.sizeInBytes = pToolData->VideoWidth * pToolData->VideoHeight * 3;        /**< Total size of frame data in bytes */
  //x/info.timestamp;                                                           /**< Frame timestamp in nanoseconds */
  info.frameIndex = pToolData->FramesRecorted;                                  /**< Unique, incremental frame index */
  info.orientation = CCAP_FRAME_ORIENTATION_TOP_TO_BOTTOM;                      /**< Frame orientation */

  // timestampNs == 0 means auto time stamp generation from cfg.frameRate.
  if( ccap_video_writer_write_frame( pToolData->pCcapVideoWriter, &info, 0) == false) {

    // Error writing frame
    return( -10);
  }

  return( 0);   // Return OK
}

/************************************************************************************
 * MyParWin_NewSizes_SetResultingSize
 *
 * Set resulting size in megabytes per hour.
 *
 */

static void MyParWin_NewSizes_SetResultingSize()
{
  uint64_t bitRate;              ///< Target bit rate in bits/s (0 = auto, YouTube recommended bitrates)
  double MegabytesPerHour;
  char TempString[ 256];

  // New size settings

  bitRate   = computeAutoBitRate( pToolData->VideoWidth, pToolData->VideoHeight, pToolData->VideoFPS,
                                  (CcapVideoCodec)pToolData->VideoCodec, pToolData->VideoQuality);

  // Convert to bytes per hour.

  MegabytesPerHour = ((bitRate / (double)8.0) * 60.0 * 60.0) / (1024.0 * 1024.0);


  sprintf( TempString, LangStringLookup( "&GUI_VideoW_ResFileSize= %ld [Mbytes / Hour]"), lround( MegabytesPerHour));

  if( strcmp( pShowNewSize->value(), TempString) != 0) {   // New string is different

    pShowNewSize->value( TempString);
  }
}

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{
  int TempEnable;

  TempEnable = (pToolData->VideoState & VIDEO_STATE_RECORD_ON) == 0;    // Enabled if NOT recording

  IqeB_GUI_WidgetActivate( pInt_VideoWidth, TempEnable); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pInt_VideoHeight, TempEnable); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pMBut_Sizes, TempEnable); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pInt_VideoFPS, TempEnable); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pChoice_VideoCodec, TempEnable); // Set item activated/inactive
  IqeB_GUI_WidgetActivate( pFloat_VideoQuality, TempEnable); // Set item activated/inactive

  if( TempEnable) {       // Recording is off

    // Update resulting file size
    MyParWin_NewSizes_SetResultingSize();
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
  }

  if( pValue == &pToolData->VideoWidth ||  // Video with or height
      pValue == &pToolData->VideoHeight) {

    if( (Value & 1) != 0) {                // Have an odd value

      Value -= 1;                          // Make even
      WasClipped = true;                   // Value was clipped
    }
  }

  if( WasClipped) {                        // Value was clipped

    pThis->SetValue( Value);
  }

  *pValue = Value;         // update the variable
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
}
#endif

/************************************************************************************
 * IqeB_GUI_But_Color_SetValue_Callback
 */

static void IqeB_GUI_But_Color_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  unsigned int *pColor, ColorBefore;
  Fl_Button *pThis;

  pThis  = (Fl_Button *)w;
  pColor = (unsigned int *)pValueArg;    // get pointer to associated variable
  ColorBefore = *pColor;

  *pColor = IqeB_GUI_ColorChooser( *pColor);

  if( ColorBefore != *pColor) {

    pThis->color( *pColor);
    pThis->parent()->redraw();

    // Hack to change colored background image
    if( (pToolData->VideoState & VIDEO_STATE_BGND_BLACK) != 0) { // Image is NOT set to black

      pToolData->VideoState &= ~VIDEO_STATE_BGND_BLACK;      // will set new output image so can reset black
    }

    //x/pToolData->Input1_Change = 0;                // Reset image change check
  }
}

/************************************************************************************
 * IqeB_Sizes_PresetButton_Callback
 *
 * Callback, Set one of preset sizes
 */

static void IqeB_Sizes_PresetButton_Callback( Fl_Widget *w)
{
  int xx, yy;
  Fl_Menu_Button *pMenu_Button;

  pMenu_Button = (Fl_Menu_Button *)w;

  // Return a pointer to the last menu item that was picked
  const Fl_Menu_Item *m = pMenu_Button->mvalue();

  if( m != NULL) {       // A menu line was selected

    xx   = ((int)(uintptr_t)(m->user_data_) >> 16) & 0x7fff;
    yy   = ((int)(uintptr_t)(m->user_data_) >>  0) & 0x7fff;

    pToolData->VideoWidth  = xx;
    pToolData->VideoHeight = yy;

    pInt_VideoWidth->SetValue( pToolData->VideoWidth);
    pInt_VideoHeight->SetValue( pToolData->VideoHeight);

    // Update resulting new size in pixel
    MyParWin_NewSizes_SetResultingSize();
  }
}

/************************************************************************************
 * IqeB_GUI_Misc_SetValue_Callback
 *
 * This is usable for Fl_Valuator, Fl_Choice, Fl_Check_Button
 */

static void IqeB_GUI_Misc_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{

  if( w == NULL ||                       // security test
      pValueArg == NULL) {

    return;
  }

  if( pValueArg == &pToolData->VideoCodec) {    // Changed video codec

    // Fl_Choice

    Fl_Choice *pThis;
    int *pValue, NewValue;

    pThis  = (Fl_Choice *)w;
    pValue = (int *)pValueArg;             // get pointer to associated variable

    NewValue = *pValue;                    // Preset sane value

    // Return a pointer to the last menu item that was picked
    const Fl_Menu_Item *m = pThis->mvalue();

    if( m != NULL) {                       // A menu line was selected

      NewValue = (int)(uintptr_t)(m->user_data_);
    }

    if( *pValue != NewValue) {             // Value is different

      *pValue = NewValue;                  // Update the variable

      // Update resulting new size in pixel
      MyParWin_NewSizes_SetResultingSize();
    }
  }
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

  pMyParWin = new Fl_Window( xPos, yPos, 178 /*IQE_GUI_TOOLS_STD_WITDH*/, 218, LANGDEF_SETTINGS);

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
  Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  IqeFl_Float_Input  *pFloatTemp;
  //x/IqeFl_Tabs      *pTemp_Tabs;
  //x/Fl_Group        *pTemp_Group;
  Fl_Button       *pTemp_Button;
  //x/Fl_Choice       *pTemp_Choice;
  //x/Fl_Radio_Round_Button *pRadioButTemp;

  x1  = 4;
  //x/xx1 = pMyParWin->w() - 8;
  //x/xx2 = xx1 / 2;
  //x/xc  = pMyToolWin->w() / 2;          // x center
  yy  = 20;

  y = 2;

  xx2 = 42;

  // Use a separator line to make a kind of group

  pTemp_Box = new Fl_Box( x1, y, pMyParWin->w() - 8, 0, LangStringLookup( "&GUI_VideoW_Par_Group1=Settings for new video"));
  pTemp_Box->align( FL_ALIGN_BOTTOM | FL_ALIGN_LEFT);
  pTemp_Box->box( FL_NO_BOX);    //  FL_BORDER_BOX FL_DOWN_FRAME

  y += yy + 10;

  pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_AOI_WIDTH);
  pTemp_Int->tooltip( LangStringLookup( "&GUI_VideoW_Par_VideoWidtha="
                                        "Width for new videos.\n"
                                        "Range: 240 to 2160 pixel."));
  pTemp_Int->align( FL_ALIGN_TOP_LEFT);     // align for label
  pTemp_Int->labelsize( 10);
  pTemp_Int->SetValue( pToolData->VideoWidth);
  pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->VideoWidth);
  pTemp_Int->SetModifyData( 240, 2160, 100, 10);
  pInt_VideoWidth = pTemp_Int;

  x1 += xx2 + 8;

  pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_AOI_HEIGHT);
  pTemp_Int->tooltip( LangStringLookup( "&GUI_VideoW_Par_VideoHeighta="
                                        "Height for new videos.\n"
                                        "Range: 240 to 2160 pixel."));
  pTemp_Int->align( FL_ALIGN_TOP_LEFT);     // align for label
  pTemp_Int->labelsize( 10);
  pTemp_Int->SetValue( pToolData->VideoHeight);
  pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->VideoHeight);
  pTemp_Int->SetModifyData( 240, 2160, 100, 10);
  pInt_VideoHeight = pTemp_Int;

  x1 += xx2 + 8;

  // Presets for sizes

  pMBut_Sizes = new Fl_Menu_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_VideoW_Par_VideoSizes=Sizes ..."));
  pMBut_Sizes->tooltip( LangStringLookup( "&GUI_VideoW_Par_VideoSizesa="
                                            "Some typical Sizes"));
  pMBut_Sizes->align( FL_ALIGN_TOP_LEFT /*FL_ALIGN_LEFT*/);     // align for label
  pMBut_Sizes->labelsize( 10);
  pMBut_Sizes->callback( IqeB_Sizes_PresetButton_Callback);

#define PR_SIZE( w, h) ((w << 16) | h)

  pMBut_Sizes->add( LangStringLookup( "&GUI_VideoW_VideoSizes11=16:9/640 x 360"),         0, NULL, (void *)(fl_intptr_t)( PR_SIZE(  640,  360)));
  pMBut_Sizes->add( LangStringLookup( "&GUI_VideoW_VideoSizes12=16:9/854 x 480"),         0, NULL, (void *)(fl_intptr_t)( PR_SIZE(  854,  480)));
  pMBut_Sizes->add( LangStringLookup( "&GUI_VideoW_VideoSizes13=16:9/1024 x 576"),        0, NULL, (void *)(fl_intptr_t)( PR_SIZE(  1024,  576)));
  pMBut_Sizes->add( LangStringLookup( "&GUI_VideoW_VideoSizes14=16:9/1280 x 720 (HD)"),   0, NULL, (void *)(fl_intptr_t)( PR_SIZE(  1280,  720)));
  pMBut_Sizes->add( LangStringLookup( "&GUI_VideoW_VideoSizes15=16:9/1920 x 1080 (FHD)"), 0, NULL, (void *)(fl_intptr_t)( PR_SIZE(  1920,  1080)));
  pMBut_Sizes->add( LangStringLookup( "&GUI_VideoW_VideoSizes21=9:16/720 x 1280"),        0, NULL, (void *)(fl_intptr_t)( PR_SIZE(  720,  1280)));
  pMBut_Sizes->add( LangStringLookup( "&GUI_VideoW_VideoSizes22=9:16/1080 x 1920"),       0, NULL, (void *)(fl_intptr_t)( PR_SIZE(  1080,  1920)));
  pMBut_Sizes->add( LangStringLookup( "&GUI_VideoW_VideoSizes31=1:1/1080 x 1080"),        0, NULL, (void *)(fl_intptr_t)( PR_SIZE(  1080,  1080)));

  // New line

  y += yy + 14;

  x1 = 4;

  pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_VideoW_Par_VideoFPS=Frame rate"));
  pTemp_Int->tooltip( LangStringLookup( "&GUI_VideoW_Par_VideoFPSa="
                                        "Frames per seconds for new videos.\n"
                                        "Range: 24 to 60."));
  pTemp_Int->align( FL_ALIGN_TOP_LEFT);     // align for label
  pTemp_Int->labelsize( 10);
  pTemp_Int->SetValue( pToolData->VideoFPS);
  pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->VideoFPS);
  pTemp_Int->SetModifyData( 24, 60, 10, 1);
  pInt_VideoFPS = pTemp_Int;

  x1 += xx2 + 8;

  // Video quality

  pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_VideoW_Par_VideoQuality=Quality"));
  pFloatTemp->type( FL_FLOAT_INPUT);
  pFloatTemp->tooltip( LangStringLookup( "&GUI_VideoW_Par_VideoQualitya="
                                         "Video quality improvement.\n"
                                         "Higher values ​​result in better detail\n"
                                         "rendering and fewer artifacts.\n"
                                         "Higher values ​​--> larger file sizes.\n"
                                         "Range: 0.5 to 2.0\n"
                                         "Recommended value: 1.0"));
  pFloatTemp->align( FL_ALIGN_TOP_LEFT);     // align for label
  pFloatTemp->labelsize( 10);
  pFloatTemp->SetFormat( "%.1f");
  pFloatTemp->SetValue( pToolData->VideoQuality);
  pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->VideoQuality);
  pFloatTemp->SetModifyData( 0.5, 2.0, 0.5, 0.1);  // Is modified later form MyParWinUpdateNewSizes()
  pFloat_VideoQuality = pFloatTemp;

  x1 += xx2 + 8;

  // Video codec

  pChoice_VideoCodec = new Fl_Choice( x1, y, xx2 + 26, yy, LangStringLookup( "&GUI_VideoW_Par_VideoCodec=Codec"));
  pChoice_VideoCodec->tooltip( LangStringLookup( "&GUI_VideoW_Par_VideoCodeca="
                                           "Codec used for data compression:\n"
                                           " H.264 / AVC (best compatibility)\n"
                                           " H.265 / HEVC (better compression, less compatible)"));
  pChoice_VideoCodec->align( FL_ALIGN_TOP_LEFT);     // align for label
  pChoice_VideoCodec->labelsize( 10);
  pChoice_VideoCodec->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->VideoCodec);
  pChoice_VideoCodec->menu_box( FL_BORDER_BOX);

  pChoice_VideoCodec->add( LangStringLookup( "&GUI_VideoW_VideoCodec1=H.264"), 0, NULL, (void *)(fl_intptr_t)CCAP_VIDEO_CODEC_H264);
  pChoice_VideoCodec->add( LangStringLookup( "&GUI_VideoW_VideoCodec2=H.265"), 0, NULL, (void *)(fl_intptr_t)CCAP_VIDEO_CODEC_HEVC);
  pChoice_VideoCodec->value( pToolData->VideoCodec);

  x1 += xx2 + 26 + 8;

  // New line

  y += yy + 14;

  x1 = 4;

  // Show resulting file size

  pShowNewSize = new Fl_Output( x1, y, pMyParWin->w() - 8, yy, LangStringLookup( "&GUI_VideoW_Par_FileSize=Resulting file size"));
  pShowNewSize->tooltip( LangStringLookup( "&GUI_VideoW_Par_FileSizea="
                                           "Shows the estimated file size\n"
                                           "in megabytes per hour.\n"
                                           "NOTE: is read only."));
  pShowNewSize->align( FL_ALIGN_TOP_LEFT /*FL_ALIGN_LEFT*/);     // align for label
  pShowNewSize->labelsize( 10);
  pShowNewSize->color( YAIPS_COLOR_RONLY_BGND);

  // Update resulting new size in pixel
  MyParWin_NewSizes_SetResultingSize();

  // New line

  y += yy + 4;

  x1 = 4;

  pTemp_Box = new Fl_Box( x1, y, pMyParWin->w() - 8, 1, LangStringLookup( "&GUI_VideoW_Par_Group2=Timings [Seconds]"));
  pTemp_Box->align( FL_ALIGN_BOTTOM | FL_ALIGN_LEFT);
  pTemp_Box->box( FL_BORDER_BOX);    //  FL_BORDER_BOX FL_DOWN_FRAME

  // New line

  y += yy + 10;

  pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_VideoW_Par_TimeTransition=Transition"));
  pFloatTemp->type( FL_FLOAT_INPUT);
  pFloatTemp->tooltip( LangStringLookup( "&GUI_VideoW_Par_TimeTransitiona="
                                         "When an image changes,\n"
                                         "a transition is created.\n"
                                         "Range: 0 to 15 seconds."));
  pFloatTemp->align( FL_ALIGN_TOP_LEFT);     // align for label
  pFloatTemp->labelsize( 10);
  pFloatTemp->SetFormat( "%.1f");
  pFloatTemp->SetValue( pToolData->TimeTransition);
  pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->TimeTransition);
  pFloatTemp->SetModifyData( 0.0, 15.0, 1.0, 0.1);

  x1 += xx2 + 8;

  pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_VideoW_Par_TimeAddImage=Add image"));
  pFloatTemp->type( FL_FLOAT_INPUT);
  pFloatTemp->tooltip( LangStringLookup( "&GUI_VideoW_Par_TimeAddImagea="
                                         "Adding an image appends a video\n"
                                         "clip of the specified length.\n"
                                         "Range: 0 to 60 seconds."));
  pFloatTemp->align( FL_ALIGN_TOP_LEFT);     // align for label
  pFloatTemp->labelsize( 10);
  pFloatTemp->SetFormat( "%.1f");
  pFloatTemp->SetValue( pToolData->TimeAddImages);
  pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->TimeAddImages);
  pFloatTemp->SetModifyData( 0.0, 60.0, 10.0, 1.0);

  // New line

  y += yy + 4;

  x1 = 4;

  pTemp_Box = new Fl_Box( x1, y, pMyParWin->w() - 8, 1, LangStringLookup( "&GUI_VideoW_Par_Group3=Other"));
  pTemp_Box->align( FL_ALIGN_BOTTOM | FL_ALIGN_LEFT);
  pTemp_Box->box( FL_BORDER_BOX);    //  FL_BORDER_BOX FL_DOWN_FRAME

  // New line

  y += yy - 4;

  x1 = pMyParWin->w() - xx2 - 8;

  pTemp_Box = new Fl_Box( 4, y, x1 - 4, yy, LangStringLookup( "&GUI_VideoW_Par_BGND_Color=Background"));
  pTemp_Box->box( FL_NO_BOX);
  pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

  pTemp_Button = new Fl_Button( x1, y, xx2, yy, "");
  pTemp_Button->color( pToolData->BGND_Color);
  pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &pToolData->BGND_Color);
  pTemp_Button->tooltip( LangStringLookup( "&GUI_VideoW_Par_BGND_Colora="
                                           "Color for background.\n"
                                           "The background is visible when images with an aspect\n"
                                           "ratio different from that of the video are used.\n"
                                           "In this case, borders of this color appear at the top\n"
                                           "and bottom, and on the left and right, respectively."));
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
  Fl_Button *pGUI_Video_StartStop;       // Toggle record start / stop
  Fl_Button *pGUI_Video_RecordPause;     // Toggle between record and pause
  Fl_Button *pGUI_Video_RecordMode;      // Toggle record mode
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

    pGUI_Video_StartStop = new Fl_Button( x1, y, xx, yy, "@+2circle");
    pGUI_Video_StartStop->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_Video_StartStop->tooltip( LangStringLookup( "&GUI_VideoW_StartStop="
                                   "Toggle between start and stop record.\n"
                                   "Shortcut: Enter"));
    pGUI_Video_StartStop->labelcolor( FL_RED);

    x1 += xx + 5;

    pGUI_Video_RecordMode = new Fl_Button( x1, y, xx, yy / 2 - 1);
    pGUI_Video_RecordMode->labelsize( 10);
    pGUI_Video_RecordMode->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_Video_RecordMode->tooltip( LangStringLookup( "&GUI_VideoW_RecordMode="
                                     "Toggle record mode between 'image' and 'video'.\n"
                                     "For 'image' a new image is added for 'Add image' seconds.\n"
                                     "For 'video' only the new image is added.\n"
                                     "Shortcut: M, left or right"));

    pGUI_Video_RecordPause = new Fl_Button( x1, y + yy / 2 + 1, xx, yy / 2 - 1, "@-1>");
    pGUI_Video_RecordPause->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_Video_RecordPause->tooltip( LangStringLookup( "&GUI_VideoW_RecordPause="
                                     "Toggle between 'Resume Recording' and 'pause'.\n"
                                     "Shortcut: Spacebar"));
    pGUI_Video_RecordPause->labelcolor( YAIPS_BCOL_BUTTON);

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

      case FL_Enter:

        if( pGUI_Video_StartStop->active()) {

          YaIPS_ToolWin_GUI_Callback( pGUI_Video_StartStop, (long int)iToolData);
        }

        return( true);
        break;

      case ' ': // Pause on/off

        if( pGUI_Video_RecordPause->active()) {

          YaIPS_ToolWin_GUI_Callback( pGUI_Video_RecordPause, (long int)iToolData);
        }

        return( true);
        break;

      case 'm': // Toggle record mode
      case FL_Left:
      case FL_Right:

        if( pGUI_Video_RecordMode->active()) {

          YaIPS_ToolWin_GUI_Callback( pGUI_Video_RecordMode, (long int)iToolData);
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

  if( pToolData->pImgTmpColConv != NULL) {   // Temporary image was used

    pToolData->pImgTmpColConv->release();    // Release temporary image
  }

  if( pToolData->pImgTmpScale != NULL) {     // Temporary image was used

    pToolData->pImgTmpScale->release();      // Release temporary image
  }

  if( pToolData->pImgTmpTrans != NULL) {     // Temporary image was used

    pToolData->pImgTmpTrans->release();      // Release temporary image
  }

  if( pToolData->pImgTmpRGB2BGR != NULL) {   // Temporary image was used

    pToolData->pImgTmpRGB2BGR->release();    // Release temporary image
  }

  YaIPS_ImageDispReleaseBeforeClose( &pToolData->YaIPS_ImageDisp);

  pToolData->IsOpen = false;               // Flag info data is not in use

  IqeB_GUI_CloseToolWindow( (void **)&pToolData->pMyToolWin);

  // Close ccap

  if( pToolData->pCcapVideoWriter != NULL) {  // Have a video writer object from before

    if( ccap_video_writer_is_opened( pToolData->pCcapVideoWriter)) { // Device is open

      ccap_video_writer_close( pToolData->pCcapVideoWriter);
    }

    ccap_video_writer_destroy( pToolData->pCcapVideoWriter);       // Delete it

    pToolData->pCcapVideoWriter = NULL;                        // Mark closed
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
 * ShowOnBig_cb
 *
 * Show loaded image to the big display
 */

//
static void YaIPS_ToolWin_GUI_Callback( Fl_Widget *w, long int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  CLASS_WIN_TOOL *pMyToolWin;
  int VideoStateOnEntry, ierr;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  VideoStateOnEntry = pToolData->VideoState;             // Video state on entry. Used to detect state changes.

  // Show output image on the big display

  if( w == pMyToolWin->pGUI_Img_ShowOnBig) {                   // Show on big image

    if( pToolData->
        YaIPS_ImageDisp.pImage_Img != NULL) {                   // Got an image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                      MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName);   // Load the image to the display
    }

  } else if( w == pMyToolWin->pGUI_Video_StartStop) {          // Toggle record start / stop

    if( (pToolData->VideoState & VIDEO_STATE_SIZE_OK) == 0 ||     // Input size is not OK
        (pToolData->VideoState & VIDEO_STATE_BGND_BLACK) != 0) {  // Image is blacked

      pToolData->VideoState &= ~VIDEO_STATE_RECORD_ON;         // Stop recording

    } else {

      pToolData->VideoState ^= VIDEO_STATE_RECORD_ON;          // Toggle start / stop
    }

    if( ((VideoStateOnEntry ^ pToolData->VideoState) & VIDEO_STATE_RECORD_ON) != 0) {  // A state bit has changed

      if( (pToolData->VideoState & VIDEO_STATE_RECORD_ON) != 0) {  // Recording on

        // Get file to open

        VideoWriteFileOpen( pToolData);

        // Check for error

        if( pToolData->pCcapVideoWriter == NULL ||                                 // No video writer object
            ccap_video_writer_is_opened( pToolData->pCcapVideoWriter) == false) {  // or no video file open


          pToolData->VideoState &= ~VIDEO_STATE_RECORD_ON;         // Stop recording

          // ToDo ... Show error on GUI

        } else {

          // We need the transition buffer image during recording

          ierr = YaIPS_RGB_ImageSetSize( &pToolData->pImgTmpTrans, pToolData->VideoWidth, pToolData->VideoHeight, 3);

          if( ierr != 0) {                                       // Security test

            pToolData->VideoState &= ~VIDEO_STATE_RECORD_ON;     // Stop recording

            return;                                              // Return to caller
          }

          // We need the RGB 2 BGR image during recording

          ierr = YaIPS_RGB_ImageSetSize( &pToolData->pImgTmpRGB2BGR, pToolData->VideoWidth, pToolData->VideoHeight, 3);

          if( ierr != 0) {                                       // Security test

            pToolData->VideoState &= ~VIDEO_STATE_RECORD_ON;     // Stop recording

            return;                                              // Return to caller
          }

          // Reset variables for recording management

          pToolData->FramesRecorted    = 0;
          pToolData->FrameCntDownImage = 0;
          pToolData->FramesTransition  = 0;
          pToolData->FrameCntDownTrans = 0;

          pToolData->VideoAction = VIDEO_ACTION_UPDATE_DISP;       // Update display
        }

      } else {                                                     // Stopped recording

        // Ensure open video file is closed

        if( ccap_video_writer_is_opened( pToolData->pCcapVideoWriter)) { // Device is open

          pToolData->VideoAction = VIDEO_ACTION_STOP_VIDEO;        // Stop the video

        } else {

          pToolData->VideoAction = VIDEO_ACTION_UPDATE_DISP;       // Update display
        }
      }
    }

  } else if( w == pMyToolWin->pGUI_Video_RecordPause) {        // Toggle between record and pause

    if( (pToolData->VideoState & VIDEO_STATE_SIZE_OK) == 0 ||     // Input size is not OK
        (pToolData->VideoState & VIDEO_STATE_BGND_BLACK) != 0) {  // Image is blacked

      pToolData->VideoState &= ~VIDEO_STATE_PAUSED;            // Play to record

    } else {

      pToolData->VideoState ^= VIDEO_STATE_PAUSED;             // Toggle play/pause
    }

    if( ((VideoStateOnEntry ^ pToolData->VideoState) & VIDEO_STATE_PAUSED) != 0) {  // A state bit has changed

      if( (pToolData->VideoState & VIDEO_STATE_PAUSED) != 0) {     // Paused on

        pToolData->VideoAction = VIDEO_ACTION_UPDATE_DISP;         // Update display

      } else {                                                     // Stopped recording

        pToolData->VideoAction = VIDEO_ACTION_PAUSE_OFF;           // Pause switched to off
      }
    }

  } else if( w == pMyToolWin->pGUI_Video_RecordMode) {         // Toggle record mode

    pToolData->RecordMode = ! pToolData->RecordMode;           // Toggle record mode

  } else if( w == pMyToolWin->pBut_Input1) {                   // Select 1. input

    int WinIdNr_Before;

    WinIdNr_Before = pToolData->Input1_WinIdNr;

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + iToolData, &pToolData->Input1_WinIdNr, pMyToolWin->pBut_Input1, pMyToolWin->pBox_Input1);

    if( WinIdNr_Before != pToolData->Input1_WinIdNr) {         // Image source selection as changed

      pToolData->Input1_Change = 0;                            // Force recalculation output
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
  int TempEnable, TempEnable2;
  char TempString1[ 256], TempString2[ 256];
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

  // Info about video

  pToolData->YaIPS_ImageDisp.ColInfoTextSize = 12;                // Use reduced font size

  if( pToolData->pCcapVideoWriter != NULL) {                      // Have video writer

    pToolData->YaIPS_ImageDisp.ColInfoBgnd = 0;                   // Default background color
    pToolData->YaIPS_ImageDisp.ColInfoText = 0;                   // Default text color

    // Info about video

    sprintf( TempString1, "%d x %d  FPS %d",
                          pToolData->VideoWidth, pToolData->VideoHeight, pToolData->VideoFPS);

    // Add info about recording time
    if( (pToolData->VideoState & VIDEO_STATE_SIZE_OK) != 0 &&     // Input size is not OK
        (pToolData->VideoState & VIDEO_STATE_BGND_BLACK) == 0 &&  // Image is blacked
        (pToolData->VideoState & VIDEO_STATE_RECORD_ON) != 0) {   // Recording is on

      int CurHour, CurMinutes, CurSeconds, Seconds_100;

      CurSeconds = (int)(pToolData->FramesRecorted / pToolData->VideoFPS);
      CurMinutes = CurSeconds / 60;
      CurHour    = CurMinutes / 60;
      CurSeconds -= CurMinutes * 60;
      CurMinutes -= CurHour * 60;

      // 1/100 seconds
      Seconds_100 = (pToolData->FramesRecorted - CurSeconds * pToolData->VideoFPS) * 100  / pToolData->VideoFPS;

      if( CurHour <= 0) {          // Video duration is below one hour

        sprintf( TempString2, "\n%02d:%02d.%02d",
                              CurMinutes, CurSeconds, Seconds_100);
      } else {

        sprintf( TempString2, "\n%d:%02d:%02d.%02d",
                              CurHour, CurMinutes, CurSeconds, Seconds_100);
      }

      if( (pToolData->VideoState & VIDEO_STATE_PAUSED) != 0) {   // Is paused

        strcat( TempString2, LangStringLookup( "&GUI_VideoW_VideoPaused=  Pause"));

      } else if( pToolData->FrameCntDownImage > 0 || pToolData->FrameCntDownTrans) {  // Timed image processing

        strcat( TempString2, "  ...");
      }

      strcat( TempString1, TempString2);
    }

  } else {    // EROR have no video writer

    pToolData->YaIPS_ImageDisp.ColInfoBgnd = FL_RED;             // Background color
    pToolData->YaIPS_ImageDisp.ColInfoText = FL_BLACK;           // Text color

    strcpy( TempString1, LangStringLookup( "&GUI_VideoW_ErrNoVideWrite=ERROR: NO CCAP Video writer"));
  }

  YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, TempString1);

  // ...

  if( DoEnable == 2) {                                          // Was periodically updates only

    return;
  }

  // ...

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_ShowOnBig,
                             DoEnable &&                                    // Enable GUI elements
                             pToolData->YaIPS_ImageDisp.pImage_Img != NULL); // and have an image loaded

  // Button: Toggle record start / stop

  TempEnable = DoEnable &&
               pToolData->pCcapVideoWriter != NULL &&                 // Video write OK ?
               (pToolData->VideoState & VIDEO_STATE_SIZE_OK) != 0 &&
               (pToolData->VideoState & VIDEO_STATE_BGND_BLACK) == 0;

  TempEnable2 = pToolData->FrameCntDownImage == 0 &&
                pToolData->FrameCntDownTrans == 0;

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Video_StartStop,
                           TempEnable && TempEnable2);                        // Enable GUI elements

  pTempString = TempEnable && (pToolData->VideoState & VIDEO_STATE_RECORD_ON) != 0 ? (char *)"@+2square" : (char *)"@+2circle";
  if( strcmp( pTempString, pMyToolWin->pGUI_Video_StartStop->label())  != 0) {

    pMyToolWin->pGUI_Video_StartStop->label( pTempString);
  }

  BigImageSourceCol = (pToolData->VideoState & VIDEO_STATE_RECORD_ON) != 0 ? FL_BLUE : FL_RED;

  if( pMyToolWin->pGUI_Video_StartStop->labelcolor() != BigImageSourceCol) {  // Color is different

    pMyToolWin->pGUI_Video_StartStop->labelcolor( BigImageSourceCol);         // Set color
    pMyToolWin->pGUI_Video_StartStop->redraw();                               // Redraw GUI element
  }

  // Button: Toggle between record and pause

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Video_RecordPause,
                           TempEnable && TempEnable2);                        // Enable GUI elements

  pTempString = TempEnable && (pToolData->VideoState & VIDEO_STATE_PAUSED) != 0 ? (char *)"@-1Dbar2" : (char *)"@-1>";
  if( strcmp( pTempString, pMyToolWin->pGUI_Video_RecordPause->label())  != 0) {

    pMyToolWin->pGUI_Video_RecordPause->label( pTempString);
  }

  BigImageSourceCol = (pToolData->VideoState & VIDEO_STATE_PAUSED) != 0 ? FL_DARK3 : YAIPS_BCOL_BUTTON;

  if( pMyToolWin->pGUI_Video_RecordPause->labelcolor() != BigImageSourceCol) {  // Color is different

    pMyToolWin->pGUI_Video_RecordPause->labelcolor( BigImageSourceCol);         // Set color
    pMyToolWin->pGUI_Video_RecordPause->redraw();                               // Redraw GUI element
  }

  // Button: Toggle record mode

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Video_RecordMode,
                           DoEnable && TempEnable2);                            // Enable GUI elements

  if( pToolData->RecordMode == 0) {
    pTempString = LangStringLookup( "&GUI_VideoW_RecordModeImg=Img");
  } else {
    pTempString = LangStringLookup( "&GUI_VideoW_RecordModeVideo=Video");
  }
  if( strcmp( pTempString, pMyToolWin->pGUI_Video_RecordPause->label())  != 0) {

    pMyToolWin->pGUI_Video_RecordMode->copy_label( pTempString);
  }

  BigImageSourceCol = pToolData->RecordMode == 0 ? FL_DARK_RED : FL_DARK_BLUE;

  if( pMyToolWin->pGUI_Video_RecordMode->labelcolor() != BigImageSourceCol) {  // Color is different

    pMyToolWin->pGUI_Video_RecordMode->labelcolor( BigImageSourceCol);         // Set color
    pMyToolWin->pGUI_Video_RecordMode->redraw();                               // Redraw GUI element
  }

  // Check input image and visualize state
  IqeB_GUI_WidgetActivate( pMyToolWin->pBox_Input1,
                           TempEnable2);                                       // Enable GUI elements
  IqeB_GUI_WidgetActivate( pMyToolWin->pBut_Input1,
                           TempEnable2);                                       // Enable GUI elements

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

    ierr = 0;                                              // Reset error
    errstring = NULL;                                      // Reset error string

    // Ensure RGB image with same size as video

    if( pToolData->YaIPS_ImageDisp.pImage_Img == NULL ||
        pToolData->YaIPS_ImageDisp.pImage_Img->w() != pToolData->VideoWidth ||
        pToolData->YaIPS_ImageDisp.pImage_Img->h() != pToolData->VideoHeight ||
        pToolData->YaIPS_ImageDisp.pImage_Img->d() != 3) {

      if( (pToolData->VideoState & VIDEO_STATE_RECORD_ON) != 0) {  // Recording on

        // During we need an output image and the size may not be changed.

        ierr = 1;    // Set error

        pToolData->VideoState &= ~VIDEO_STATE_RECORD_ON;         // Stop recording

      } else {                                                     // Recording off

        ierr = YaIPS_RGB_ImageSetSize( &pToolData->YaIPS_ImageDisp.pImage_Img, pToolData->VideoWidth, pToolData->VideoHeight, 3);

      }

      if( ierr != 0) {

        if( (pToolData->VideoState & VIDEO_STATE_SIZE_OK) != 0) { // Size was OK before

          // Empty output image
          YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);

          pToolData->VideoState &= ~VIDEO_STATE_SIZE_OK;         // Reset size is OK

          ForceUpdate = true;                        // Force update
        }

        pToolData->Input1_Change = 0;                // Reset image change check

        goto DoneProcessing;
      }
    }

    pToolData->VideoState |= VIDEO_STATE_SIZE_OK;   // Size is OK

    // Check for valid input image

    Input1_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr,
                                            NULL, &pImgIn1, &Input1_ImageChanged);

    if( Input1_Check != 0) {                                 // Input image is not valid

      if( (pToolData->VideoState & VIDEO_STATE_BGND_BLACK) == 0) { // Image is NOT set to black

        // Color background of output image

        YaIPS_RGB_SetVal( pToolData->YaIPS_ImageDisp.pImage_Img, YaIPS_RGB_Color2Val( 3, pToolData->BGND_Color));   // Color image

        pToolData->VideoState |= VIDEO_STATE_BGND_BLACK;     // Image is set to black

        ForceUpdate = true;                        // Force update
      }

      pToolData->Input1_Change = 0;                // Reset image change check

      goto DoneProcessing;
    }

    // Check for changed input image

    if( (pToolData->VideoState & VIDEO_STATE_RECORD_ON) != 0 &&  // Recording on
        (pToolData->VideoState & VIDEO_STATE_PAUSED) == 0) {     // and not paused

      if( pToolData->FramesRecorted == 0 ||                      // No image recored until now
          (pToolData->VideoAction == VIDEO_ACTION_PAUSE_OFF &&   // or pause switched off
            pToolData->Input1_Recorded != Input1_ImageChanged)) {  // and input image has changed

        // First image of an video

        if( pToolData->RecordMode == 0) {                         // Record mode 'image'

          // Frames to record for an image
          pToolData->FrameCntDownImage = lround( pToolData->TimeAddImages * pToolData->VideoFPS);
        }

        // Frames for transition
        pToolData->FramesTransition = lround( pToolData->TimeTransition * pToolData->VideoFPS);

        if( pToolData->RecordMode == 0 &&                         // Record mode 'image'
            pToolData->FramesTransition > pToolData->FrameCntDownImage) {  // clip frames transition to frames for an image

          pToolData->FramesTransition = pToolData->FrameCntDownImage;
        }

        pToolData->FrameCntDownTrans = pToolData->FramesTransition;

        if( pToolData->FrameCntDownTrans > 0 &&     // Start of a transition ?
            pToolData->pImgTmpTrans != NULL) {      // and have image buffer

          if( pToolData->FramesRecorted == 0) {     // Begin of video ?

            // Color background of transition image

            YaIPS_RGB_SetVal( pToolData->pImgTmpTrans, YaIPS_RGB_Color2Val( 3, pToolData->BGND_Color));   // Color image

          } else {

            // Copy current output image to transfer buffer

            YaIPS_RGB_CopyImg( &pToolData->pImgTmpTrans, pToolData->YaIPS_ImageDisp.pImage_Img);
          }
        }

        goto ProcessInputImage;
      }
    }

    if( (pToolData->VideoState & VIDEO_STATE_RECORD_ON) != 0 &&  // Recording on
        pToolData->FrameCntDownImage > 0) {                      // Have to count down image

      goto ProcessInputImage;
    }

    if( Input1_ImageChanged == pToolData->Input1_Change) {       // Image count is NOT different

      if( pToolData->VideoAction == VIDEO_ACTION_UPDATE_DISP ||  // Update display to show updated status
          pToolData->VideoAction == VIDEO_ACTION_PAUSE_OFF ||    // or an unprocessed 'pause switched off' from above
          pToolData->VideoAction == VIDEO_ACTION_STOP_VIDEO) {   // and not stop video

        ForceUpdate = true;                        // Force update
      }

      goto DoneProcessing;
    }

    // Have a new input image

    // Frames to record for an image

    if( (pToolData->VideoState & VIDEO_STATE_RECORD_ON) != 0 &&  // Recording on
        (pToolData->VideoState & VIDEO_STATE_PAUSED) == 0) {     // and not paused

      if(pToolData->RecordMode == 0) {                           // Record mode 'image'

        pToolData->FrameCntDownImage = lround(pToolData->TimeAddImages * pToolData->VideoFPS);
      }

      // Frames for transition
      pToolData->FramesTransition = lround(pToolData->TimeTransition * pToolData->VideoFPS);

      if( pToolData->RecordMode == 0 &&                           // Record mode 'image'
          pToolData->FramesTransition > pToolData->FrameCntDownImage) {  // clip frames transition to frames for an image

        pToolData->FramesTransition = pToolData->FrameCntDownImage;
      }

      pToolData->FrameCntDownTrans = pToolData->FramesTransition;

      if( pToolData->FrameCntDownTrans > 0 &&     // Start of a transition ?
          pToolData->pImgTmpTrans != NULL) {      // and have image buffer

        if( pToolData->FramesRecorted == 0) {     // Begin of video ?

          // Color background of transition image

          YaIPS_RGB_SetVal( pToolData->pImgTmpTrans, YaIPS_RGB_Color2Val( 3, pToolData->BGND_Color));   // Color image

        } else {

          // Copy current output image to transfer buffer

          YaIPS_RGB_CopyImg( &pToolData->pImgTmpTrans, pToolData->YaIPS_ImageDisp.pImage_Img);
        }
      }
    }

    // Process input image

ProcessInputImage:

    pToolData->Input1_Change = Input1_ImageChanged;        // Image is processed

    pToolData->VideoState &= ~VIDEO_STATE_BGND_BLACK;      // will set new output image so can reset black

    // Do the image processing

    if( pImgIn1->d() != 3) {                               // Source image has NOT 3 bytes per pixel

      ierr = YaIPS_RGB_Color_DestD( &pToolData->pImgTmpColConv, pImgIn1, 3);   // Convert to 3 bytes per pixel

      if( ierr != 0) {

        goto DoneProcessing;
      }

      pImgIn1 = pToolData->pImgTmpColConv;                              // Continue with converted image
    }

    if( pToolData->YaIPS_ImageDisp.pImage_Img->w() == pImgIn1->w() &&   // Check same size
        pToolData->YaIPS_ImageDisp.pImage_Img->h() == pImgIn1->h() &&
        pToolData->YaIPS_ImageDisp.pImage_Img->d() == pImgIn1->d()) {

      // Copy image

      YaIPS_RGB_CopyImg( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1);

    } else {                                                            // Has not the same size

      double Scale, ScaleX, ScaleY;
      int xxDst, yyDst, xxDstX, yyDstX, xxDstY, yyDstY;

      // Try to fit one side of the source image into the destination image

      ScaleX = (double)pToolData->YaIPS_ImageDisp.pImage_Img->w() / (double)pImgIn1->w();
      ScaleY = (double)pToolData->YaIPS_ImageDisp.pImage_Img->h() / (double)pImgIn1->h();

      // Get resulting sizes of both possibilities
      xxDstX = pToolData->YaIPS_ImageDisp.pImage_Img->w();
      yyDstX = (int)(pImgIn1->h() * ScaleX);

      yyDstY = pToolData->YaIPS_ImageDisp.pImage_Img->h();
      xxDstY = (int)(pImgIn1->w() * ScaleY);

      // Only one of them fits

      if( yyDstX <= pToolData->YaIPS_ImageDisp.pImage_Img->h()) {   // This one fits

        Scale = ScaleX;
        xxDst = xxDstX;
        yyDst = yyDstX;

      } else if( yyDstY <= pToolData->YaIPS_ImageDisp.pImage_Img->w()) {   // This one fits

        Scale = ScaleY;
        xxDst = xxDstY;
        yyDst = yyDstY;

      } else {       // Should not happen, none of them fits

        Scale = 0.0;
        xxDst = 0;   // Invalidate
        yyDst = 0;

      }

      if( xxDst > 0 && yyDst > 0)   {    // Source fits into destination

        // Preset output image
        YaIPS_RGB_SetVal( pToolData->YaIPS_ImageDisp.pImage_Img, YaIPS_RGB_Color2Val( 3, pToolData->BGND_Color));   // Black output image

        // Scale to temporary image
        ierr = YaIPS_RGB_Geo_Transform( &pToolData->pImgTmpScale, pImgIn1,
                                        pToolData->BGND_Color, false,
                                        0.0,
                                        Scale, Scale, 0.0, 0.0, xxDst, yyDst);

        // Center to output image
        YaIPS_RGB_CopyInImg( pToolData->YaIPS_ImageDisp.pImage_Img, pToolData->pImgTmpScale,
                             (pToolData->YaIPS_ImageDisp.pImage_Img->w() - xxDst) / 2,
                             (pToolData->YaIPS_ImageDisp.pImage_Img->h() - yyDst) / 2);

      } else {                       // Should not happen, none of them fits

        // Until know it better color the output image
        YaIPS_RGB_SetColor( pToolData->YaIPS_ImageDisp.pImage_Img, FL_RED, FL_GREEN, FL_BLUE, FL_YELLOW, YAIPS_SETVAL_CORNER_BIT_MASK);   // Black output image
      }
    }

    // Process Transition

    if( pToolData->FrameCntDownTrans > 0 &&
        pToolData->pImgTmpTrans != NULL) {      // and have image buffer

      float FadeFaktor;

      // Fade in or fade between images

      FadeFaktor = (float)pToolData->FrameCntDownTrans / (float)pToolData->FramesTransition;

      // Inplace mix the images together.
      YaIPS_RGB_Combine( &pToolData->YaIPS_ImageDisp.pImage_Img,                         // Destination image
                         pToolData->YaIPS_ImageDisp.pImage_Img, pToolData->pImgTmpTrans, // source images
                         YAIPS_COMBINE_OP_FADE, YAIPS_COMBINE_ALPHA_NO,
                         FadeFaktor, 0);
    }

    YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp); // Reset error message

    ForceUpdate = true;                        // Force update of output image

    // Output frame

    if( (pToolData->VideoState & VIDEO_STATE_RECORD_ON) != 0 &&  // Recording on
        (pToolData->VideoState & VIDEO_STATE_PAUSED) == 0) {     // and not paused

      // Write to video file

      VideoWriteToFile( pToolData, pToolData->YaIPS_ImageDisp.pImage_Img);

      pToolData->FramesRecorted += 1;   // One more frame recorded

      pToolData->Input1_Recorded = pToolData->Input1_Change;
    }

    if( pToolData->FrameCntDownImage > 0) {

      pToolData->FrameCntDownImage -= 1;
    }

    if( pToolData->FrameCntDownTrans > 0) {

      pToolData->FrameCntDownTrans -= 1;
    }

    // Has a valid output image

DoneProcessing:

   if( pToolData->VideoAction == VIDEO_ACTION_STOP_VIDEO) {   // 'stop video' processing

     if( pToolData->TimeTransition > 0.0) {                   // To a fade out ?

       // Frames for transition
       pToolData->FramesTransition = lround(pToolData->TimeTransition * pToolData->VideoFPS);

       pToolData->FrameCntDownImage = lround( pToolData->TimeAddImages * pToolData->VideoFPS);

       if( pToolData->RecordMode == 0 &&                           // Record mode 'image'
           pToolData->FramesTransition > pToolData->FrameCntDownImage) {  // clip frames transition to frames for an image

         pToolData->FramesTransition = pToolData->FrameCntDownImage;
       }

       pToolData->FrameCntDownTrans = pToolData->FramesTransition;

       // Color background of transition image

       YaIPS_RGB_SetVal( pToolData->pImgTmpTrans, YaIPS_RGB_Color2Val( 3, pToolData->BGND_Color));   // Color image

       while( pToolData->FrameCntDownTrans > 0) {

         float FadeFaktor;

         // Fade out

         FadeFaktor = (float)(pToolData->FramesTransition + 1 - pToolData->FrameCntDownTrans) / (float)pToolData->FramesTransition;

         if( FadeFaktor < 0.0) {

           FadeFaktor = 0.0;
         }

         // Inplace mix the images together.
         // NOTE: Use 'pImgTmpRGB2BGR' as mixing output image. So the original output image is not changed.

         YaIPS_RGB_Combine( &pToolData->pImgTmpRGB2BGR,                                     // Destination image
                            pToolData->YaIPS_ImageDisp.pImage_Img, pToolData->pImgTmpTrans, // source images
                            YAIPS_COMBINE_OP_FADE, YAIPS_COMBINE_ALPHA_NO,
                            FadeFaktor, 0);

         // Write to video file

         VideoWriteToFile( pToolData, pToolData->pImgTmpRGB2BGR);

         pToolData->FramesRecorted += 1;   // One more frame recorded

         if( pToolData->FrameCntDownTrans > 0) {

           pToolData->FrameCntDownTrans -= 1;
         }
       }
     }

     // Close video file

     if( ccap_video_writer_is_opened( pToolData->pCcapVideoWriter)) { // Device is open

       ccap_video_writer_close( pToolData->pCcapVideoWriter);
     }

     // Reset variables for recording management

     pToolData->FramesRecorted    = 0;
     pToolData->FrameCntDownImage = 0;
     pToolData->FramesTransition  = 0;
     pToolData->FrameCntDownTrans = 0;
   }

#ifdef USE_DEBUG_OUTPUTS

    YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp, "F %2d/%2d",
                             pToolData->FramesRecorted % pToolData->VideoFPS, pToolData->VideoFPS); // Info
#endif

    pToolData->VideoAction = VIDEO_ACTION_NONE;        // All video actions are processed now

    MyWinUpdate( iToolData, true);               // Update the GUI

    // ...

    if( ForceUpdate) {             // Got a frame

      YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, (char *)MY_WIN_GUI_NAME);
    }

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
 * IqeB_GUI_VideoWriteWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_VideoWriteWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
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

  pToolData->Input1_Change  = 0;                 // Reset image change check
  pToolData->VideoState     = VIDEO_STATE_NONE;  // None of the bits is set
  pToolData->pImgTmpColConv = NULL;              // No memory for temporary image
  pToolData->pImgTmpScale   = NULL;              // No memory for temporary image
  pToolData->pImgTmpTrans   = NULL;              // No memory for temporary image
  pToolData->pImgTmpRGB2BGR = NULL;              // No memory for temporary image

  pToolData->FramesRecorted    = 0;
  pToolData->FrameCntDownImage = 0;
  pToolData->FramesTransition  = 0;
  pToolData->FrameCntDownTrans = 0;

  pToolData->VideoAction = VIDEO_ACTION_NONE;    // Nothing to do after startup

  // Set ccap error callback

  My_ccap_last_error = CCAP_ERROR_NONE;                   // Reset last error

  ccap_set_error_callback( My_ccap_error_callback, NULL);

  // Create a ccap video writer object

  pToolData->pCcapVideoWriter = ccap_video_writer_create();      // Create a capture object

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

  if( MY_WIN_MAX > 1) {

    sprintf( TempString, "%d %s", iToolData + 1, MY_WIN_GUI_NAME);
  } else {

    sprintf( TempString, "%s", MY_WIN_GUI_NAME);
  }
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
 * IqeB_GUI_VideoWriteWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_VideoWriteWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

	  IqeB_GUI_VideoWriteWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  IqeB_GUI_VideoWriteWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


