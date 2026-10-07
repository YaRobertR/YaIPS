/****************************************************************************

  YaIPS_GUI_Camera.cpp

  Direct Show camera input using OpenCV interface.

  20.01.2025 RR: First edition of this file.
  15.04.2026 RR: Change camera interface from Direct Show to ccap.

*****************************************************************************
*/
//x/#define CAMERA_INIT_DEBUG _PRINTS   1   // define this to have camera init debug prints during _DEBUG

// Needed for director show camera settings dialog
#include <windows.h>
#include <dshow.h>
#include <initguid.h>
#include <math.h>

// Ccap specific includes
#include <ccap_c.h>

// Other includes
#include "YaIPS.h"

/************************************************************************************
* Defines for this source file.
*/

// Defines for windows ID
#define MY_WIN_ID     YAIPS_WIN_ID_CAMERA       // Source specific windows ID
#define MY_WIN_MAX    1                          // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_Camera_Title=Camera"              // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinCamera"            // Name used for the preference data

// define for window sizes
#define MYWIN_SIZE_X_MIN       322
#define MYWIN_SIZE_X_MAX       (MYWIN_SIZE_X_MIN + 256)
#define MYWIN_SIZE_X_DEFAULT   MYWIN_SIZE_X_MIN

#define MYWIN_SIZE_Y_MIN       (234 + 32)
#define MYWIN_SIZE_Y_MAX       (512 + 32)
#define MYWIN_SIZE_Y_DEFAULT   MYWIN_SIZE_Y_MIN

/************************************************************************************
* forwards
*/

static void ShutDown_close( Fl_Widget *w, void *pValueArg);

/************************************************************************************
* Global variables for camera management
*/

#define CAM_MAX_CAMERAS    CCAP_MAX_DEVICES   // Max number of cameras, use same value as ccap API

static int StartupDone;                       // Intern use, delayed startup is done

static CcapProvider *pCcapProvider;           // CCap: camera provider
       int YaIPS_Camera_n = -1;               // If >= 0, number of cameras found, < 0: camera info table not initialized
       int YaIPS_Cam_Info_Idx = -1;           // If >= 0, selected camera, index in camera info table
static int YaIPS_Cam_Info_Res = -1;           // If >= 0, selected resolution (of selected camera)
static int YaIPS_CameraID = -1;               // If >= 0, selected camera (index used for ccap interface)
int YaIPS_Camera_XX = -1;                     // If > 0, Width of camera image
int YaIPS_Camera_YY = -1;                     // If > 0, Height of camera image
char CamIdLastSelected[ CCAP_MAX_DEVICE_NAME_LENGTH];  // Use this to remember last used camera. This is restored at next startup
static char CamResLastSelected;               // Last selected camera resolution

//x/static Mat YaIPS_Camera_CV_Img;               // Last acquired OpenCV image
static Fl_RGB_Image *pYaIPS_Camera_Fl_Img;    // Last acquired image in FLTK format

// Info about cameras

typedef struct {                               // Info about camera
  char CamName[ CCAP_MAX_DEVICE_NAME_LENGTH];  // Name for camera. Is used for the camera section GUI.
  int CameraID;                                // Idx of the camera
  CcapResolution Resolutions[CCAP_MAX_RESOLUTIONS];   // Array of supported resolutions
  int resolutionCount;                         // Number of supported resolutions
} YaIPS_Cam_info_t;

static YaIPS_Cam_info_t YaIPS_Cam_Info[ CAM_MAX_CAMERAS];

/************************************************************************************
* Global variables for camera window
*/

static  Fl_Double_Window *pMyToolWin;
static int IsOpen;                           // True if this window is open.
static int MyWinPosX  = IQE_GUI_NO_WINPOS_X, MyWinPosY = IQE_GUI_NO_WINPOS_Y; // last window position
static int MyWinSizeX = MYWIN_SIZE_X_DEFAULT, MyWinSizeY = MYWIN_SIZE_Y_DEFAULT; // last window size

// GUI elements
static Fl_Button *pGUI_Img_ShowOnBig;         // Show this image on big display
static Fl_Button *pGUI_But_Snap;              // Acquire a single image from the camera
static Fl_Button *pGUI_But_Grab;              // Grab images from the camera on/off
static Fl_Choice *pGUI_Choice_SelCam;         // Select camera
static int Last_Choice_SelCam_n = -2;         // Last size of pGUI_Choice_SelCam
static Fl_Choice *pGUI_Choice_SelRes;         // Select resolution
static int Last_Choice_SelRes_n = -2;         // Last size of pGUI_Choice_SelRes
static Fl_Box *pGUI_Box_FPS;                  // Frames per second
static Fl_Button *pGUI_CamRefresh;            // Refresh cameras
static Fl_Button *pGUI_CamSetting;            // Open camera settings dialog
static Fl_Button *pGUI_Parameter;             // Open the parameter dialog
static Fl_Button *pGUI_TeachToggle;           // Toggle Teach / Inspection

static Fl_YaIPS_ImageDisp_t YaIPS_ImageDisp;    // Info image display

// Acquire management

static int Camera_DoGrab = false;             // Do continues grab of images if on
static float Camera_FPS = 0.0;                // FPS value shown on GUI
static float Camera_FPS_2 = 0.0;              // Hidden FPS value used to smooth output
static int Camera_FPS_n = 0;                  // Frame count used for average of FPS value
static unsigned int Camera_FPS_TimeStamp = 0;

// Line camera simulation

static int CamLine_NextLine = -1;             // < 0 if no line camera mode, else next line to write

//...

#ifndef CV_FOURCC_MACRO
#define CV_FOURCC_MACRO(c1, c2, c3, c4) (((c1) & 255) + (((c2) & 255) << 8) + (((c3) & 255) << 16) + (((c4) & 255) << 24))
#endif

// Initialize cameras
static int YaIPS_Cameras_Init()
{
  int ierr, CameraID, nCamerasFound;
  char TempString[ 256];

  // Is already initialized ?

  if( YaIPS_Camera_n >= 0) {       // Is already initialized ?

    return( 0);
  }

  if( pCcapProvider != NULL) {    // Have a capture object from before

    ccap_provider_destroy( pCcapProvider);    // Delete it
  }

  pCcapProvider = ccap_provider_create();     // Create a capture object

  if( pCcapProvider == NULL) {                // Test for problem

    YaIPS_Camera_n = 0;                       // No new scan

    return( -1);
  }

  // Lower latency for real-time applications
  ccap_provider_set_max_available_frame_size( pCcapProvider, 2);

  // Enumerate camera devices

  CcapDeviceNamesList deviceList;

  ierr = ccap_provider_find_device_names_list( pCcapProvider, &deviceList);

  if( ierr != true) {                           // Problem with find devices

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
    printf( "YaIPS_Cameras_Init: Failed to find any video capture device.\n");
#endif
#endif

    YaIPS_Camera_n = 0;                         // No new scan

    return( -1);
  }

  memset( YaIPS_Cam_Info, 0, sizeof( YaIPS_Cam_Info));     // Zero all memory for new camera info

  nCamerasFound = deviceList.deviceCount;       // Number of cameras found from enumerator

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
  printf( "YaIPS_Cameras_Init: Found %d video capture device\n", nCamerasFound);
#endif
#endif

  // Walk the cameras found

  YaIPS_Camera_n = 0;

  for( CameraID = 0; CameraID < nCamerasFound; CameraID++) {

    // Close device from before

    if( ccap_provider_is_opened( pCcapProvider)) {             // Other device is open

      ccap_provider_close( pCcapProvider);
    }

    if( YaIPS_CamPar_AMode_SkipFirst &&  // Skip first camera
        CameraID == 0) {                 // and test fist camera

      continue;                          // skip it ...
    }

    if( YaIPS_Camera_n < 0) {           // No camera until now

      YaIPS_Camera_n = 0;               // Prepare for add below
    }

    if( YaIPS_ImageDisp.pImage_Box) {   // Security test

      // Update camera
      sprintf( TempString, LangStringLookup( "&GUI_Camera_Search1=Search cameras %d ..."), YaIPS_Camera_n + 1);
      YaIPS_ImageDisp.pImage_Box->copy_label( TempString);
      Fl::flush();      // Update the screen
    }

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
    printf( "YaIPS_Cameras_Init: Get info from camera %d ...\n", CameraID);
#endif
#endif

    ierr = ccap_provider_open_by_index( pCcapProvider, CameraID, false);
    if( ierr != true) {                              // Problem with open

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
      printf( "YaIPS_Cameras_Init: open failed !\n");
#endif
#endif
      continue;
    }

    // Get device info

    CcapDeviceInfo deviceInfo;

    ierr = ccap_provider_get_device_info( pCcapProvider, &deviceInfo);
    if( ierr != true) {                              // Problem with device info
#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
      printf( "YaIPS_Cameras_Init: Failed to get device info !\n");
#endif
#endif
      continue;
    }

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
    printf("===== Info for device: %s =======\n", deviceInfo.deviceName);

    printf("  Supported resolutions:\n");
    for( int i = 0; i < (int)deviceInfo.resolutionCount; i++) {
      printf("    %dx%d\n", deviceInfo.supportedResolutions[ i].width, deviceInfo.supportedResolutions[ i].height);
    }

    printf("  Supported pixel formats:\n");
    for( int i = 0; i < (int)deviceInfo.pixelFormatCount; i++) {
      printf("    %08X\n", deviceInfo.supportedPixelFormats[ i]);
    }

    puts("===== Info end =======\n");
#endif
#endif

    strncpy( YaIPS_Cam_Info[ YaIPS_Camera_n].CamName, deviceInfo.deviceName, sizeof( YaIPS_Cam_Info[ YaIPS_Camera_n].CamName) - 1);

    YaIPS_Cam_Info[ YaIPS_Camera_n].CameraID = CameraID;

	  // enter camera resolutions

    YaIPS_Cam_Info[ YaIPS_Camera_n].resolutionCount = 0;

    for( int i = 0; i < (int)deviceInfo.resolutionCount; i++) {

      if( YaIPS_Cam_Info[ YaIPS_Camera_n].resolutionCount >= CCAP_MAX_RESOLUTIONS) {     // Overflow number of cameras

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
        printf( "!!! Not more than %d cameras supported !!!\n", CAM_MAX_CAMERAS);
#endif
#endif
        break;   // Break probe loop
      }

      YaIPS_Cam_Info[ YaIPS_Camera_n].Resolutions[ i].width  = deviceInfo.supportedResolutions[ i].width;
      YaIPS_Cam_Info[ YaIPS_Camera_n].Resolutions[ i].height = deviceInfo.supportedResolutions[ i].height;

      YaIPS_Cam_Info[ YaIPS_Camera_n].resolutionCount += 1;    // Have on more
	  }

    YaIPS_Camera_n += 1;                           // Have one more camera

    if( YaIPS_Camera_n >= CAM_MAX_CAMERAS) {       // Overflow number of cameras

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
      printf( "!!! Not more than %d cameras supported !!!\n", CAM_MAX_CAMERAS);
#endif
#endif
      break;   // Break camera loop
    }
  }

  // Close device from before

  if( ccap_provider_is_opened( pCcapProvider)) {             // Other device is open

    ccap_provider_close( pCcapProvider);
  }

  return( YaIPS_Camera_n);
}

/************************************************************************************
* Open one of the cameras
*
* return:   0  OK
*         < 0  Error
*/

static int YaIPS_Cameras_Open( int Cam_Info_Idx, int Cam_Info_Res)
{
  int ierr;
  YaIPS_Cam_info_t *pYaIPS_Cam_Info;

	// Invalidate camera nr. first

	YaIPS_Cam_Info_Idx = -1;
	YaIPS_Cam_Info_Res = -1;
	YaIPS_CameraID     = -1;

	// Test argument

	if( Cam_Info_Idx < 0 && Cam_Info_Idx >= YaIPS_Camera_n) {  // Security test camera nr.

	  return( -1);                                             // Argument out of range.
	}

	// Check for cap provider

  if( pCcapProvider == NULL) {                // Have NO capture object from before

    pCcapProvider = ccap_provider_create();   // Create a capture object

    if( pCcapProvider == NULL) {              // Test for problem

      YaIPS_Camera_n = 0;                       // No new scan

      return( -1);
    }
  }

  // Close device from before

  if( ccap_provider_is_opened( pCcapProvider)) {           // Other device is open

    ccap_provider_close( pCcapProvider);
  }

	// Try to open camera

  pYaIPS_Cam_Info = YaIPS_Cam_Info + Cam_Info_Idx;           // use this camera info entry

  // use this camera

	YaIPS_Cam_Info_Idx = Cam_Info_Idx;
  YaIPS_Cam_Info_Res = Cam_Info_Res;
  if( YaIPS_Cam_Info_Res < 0 || YaIPS_Cam_Info_Res >= pYaIPS_Cam_Info->resolutionCount) {  // If default resolution
    YaIPS_Cam_Info_Res = pYaIPS_Cam_Info->resolutionCount / 2;   // selected center entry
  }
	YaIPS_CameraID     = pYaIPS_Cam_Info->CameraID;

	// Camera settings before open

  YaIPS_Camera_XX = pYaIPS_Cam_Info->Resolutions[ YaIPS_Cam_Info_Res].width;     // Remember used image size
  YaIPS_Camera_YY = pYaIPS_Cam_Info->Resolutions[ YaIPS_Cam_Info_Res].height;

	ccap_provider_set_property( pCcapProvider, CCAP_PROPERTY_WIDTH, YaIPS_Camera_XX);
	ccap_provider_set_property( pCcapProvider, CCAP_PROPERTY_HEIGHT, YaIPS_Camera_YY);
	ccap_provider_set_property( pCcapProvider, CCAP_PROPERTY_PIXEL_FORMAT_OUTPUT, CCAP_PIXEL_FORMAT_RGB24);
  ccap_provider_set_property( pCcapProvider, CCAP_PROPERTY_FRAME_RATE, 60);

	// Try to open camera

  ierr = ccap_provider_open_by_index( pCcapProvider, YaIPS_CameraID, false);   // Open this camera, no capture auto start
  if( ierr != true) {                              // Problem with open

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
    printf( "YaIPS_Cameras_Open: open failed !\n");
#endif
#endif


    YaIPS_Cam_Info_Idx = -1;                        // No camera is open
    YaIPS_Cam_Info_Res = -1;
    YaIPS_CameraID     = -1;

    return( -2);                                    // Error
  }

	if( ! ccap_provider_is_opened( pCcapProvider)) {

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
	  printf( "YaIPS_Cameras_Open: Open Webcam %d. of %d failed\n", YaIPS_CameraID, YaIPS_Camera_n);
#endif
#endif

	  YaIPS_Cam_Info_Idx = -1;                         // No camera is open
    YaIPS_Cam_Info_Res = -1;
	  YaIPS_CameraID     = -1;

	  return( -3);                                     // Error
	}

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
  printf( "YaIPS_Cameras_Open: Open Webcam %d. of %d successful\n", YaIPS_CameraID, YaIPS_Camera_n);
#endif
#endif

  // Remember last selected camera
  strncpy( CamIdLastSelected, pYaIPS_Cam_Info->CamName, sizeof( CamIdLastSelected) - 1);
  CamResLastSelected = YaIPS_Cam_Info_Res;

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
  printf( "YaIPS_Cameras_Open: cam %d: %dx%d\n", YaIPS_Cam_Info_Idx, YaIPS_Camera_XX, YaIPS_Camera_YY);
#endif
#endif

  // Load properties for specific camera if property dialog is not open.

  YaIPS_GUI_CamPropertiesLoad( CamIdLastSelected);

	return( 0);    // Return OK
}

/************************************************************************************
* Close an open camera
*
* return:   0  OK
*         < 0  Error
*/

static int YaIPS_Cameras_Close()
{

  // Close device from before

  if( pCcapProvider != NULL) {  // Have a capture object from before

    if( ccap_provider_is_opened( pCcapProvider)) {             // Other device is open

      ccap_provider_close( pCcapProvider);
    }

    ccap_provider_destroy( pCcapProvider);       // Delete it

    pCcapProvider = NULL;                        // Mark closed
  }

  return( 0);    // Return OK
}

/************************************************************************************
* Snap image from a camera
*
* CamLine_NextLine:  < 0 area camera mode
*                   >= 0 line scan camera mode, next line to aquire too
*
* return:   0  OK
*         < 0  Error
*/

static int YaIPS_Cameras_Snap()
{
  int ierr, SizeOfLineSrc, SizeOfLineDst, SizeInBytesDst, ImageHeightSrc, ImageWidthDst, ImageHeightDst, OutXX, OutYY, OffX, OffY;
  //x/int ImageWidthSrc;
  int BytesPpSrc, BytesPpDst, BytesPpConvert;
  uchar *pDataImg, *pSrc, *pDst;
#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
  //x/int w, h;
#endif
#endif

  if( pCcapProvider == NULL) {    // Security test

  	return( -2);
  }

  // Ensure grab is started

  ierr = ccap_provider_is_started( pCcapProvider);
  if( ierr == false) {    // Grab not started

    ccap_provider_start( pCcapProvider);    // Start grab
  }

  // Grab a frame

  CcapVideoFrame *frame;
  CcapVideoFrameInfo frameInfo;

  frame = ccap_provider_grab( pCcapProvider, 1000);
  if( frame == NULL) {

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
    printf( "YaIPS_Cameras_Snap: Read Webcam failed\n");
#endif
#endif

    return( -1);
  }

  ierr = ccap_video_frame_get_info(frame, &frameInfo);
  if( ierr != true) {                              // Problem with find devces

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
    printf( "YaIPS_Cameras_Snap: Failed to get frame info\n");
#endif
#endif

    // Release frame
    ccap_video_frame_release( frame);

    return( -2);
  }

  // Convert color space

  uint8_t   *data = frameInfo.data[0];    // Data of the first plane
  int   StrideSrc = (int)frameInfo.stride[0];  // Size of one data line. May be bigger then 'with in pixel' * 'bytes per pixel'

#ifdef CAMERA_INIT_DEBUG   // Camera init debug prints during _DEBUG
#ifdef _DEBUG
  //x/w = YaIPS_Camera_CV_Img.cols;
  //x/h = YaIPS_Camera_CV_Img.rows;

  printf( "Frame: %dx%d, format=%08x, size=%u bytes\n",
            frameInfo.width, frameInfo.height,
            frameInfo.pixelFormat, frameInfo.sizeInBytes);
#endif
#endif

  // Size for output image

  OutXX = frameInfo.width;        // Preset use of full image
  OutYY = frameInfo.height;
  OffX  = 0;                              // Preset offset
  OffY  = 0;

#ifdef use_again
  if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_AREA ||  // Area mode active
      (YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_LINE && YaIPS_CamPar_LMode_Adjust) ||
      (YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT && YaIPS_CamPar_HMode_Adjust)) {
#else
    if( CamLine_NextLine < 0) {                  // Area camera mode
#endif

    if( YaIPS_CamPar_AMode_Square) {       // Square cut out active ?

      // Get square size
      if( OutXX > OutYY) {

        OffX  = (OutXX - OutYY) / 2;
        OutXX = OutYY;

      } else {

        OffY  = (OutYY - OutXX) / 2;
        OutYY = OutXX;
      }
    }

  } else {                 // Line or height mode active

    if( YaIPS_CamPar_AMode_Square) {       // Square cut out active ?

      // Ensure same with as used in area mode
      if( OutXX > OutYY) {

        OffX  = (OutXX - OutYY) / 2;
        OutXX = OutYY;
      }
    }

    if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_LINE) {            // Line mode active

      OutYY = YaIPS_CamPar_LMode_LinesAcq;                            // Image height

    } else if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT) {   // Height mode

      OutYY = YaIPS_CamPar_HMode_LinesAcq;                            // Image height
    }
  }

  // Color space:

  if( frameInfo.pixelFormat == CCAP_PIXEL_FORMAT_RGB24) {    // Expect this pixel format

    BytesPpSrc = 3;                  // Bytes per pixel source
  } else {

    BytesPpSrc = 0;                  // Bytes per pixel not known
  }

  BytesPpDst = BytesPpSrc;                                           // Default: bytes per pixel destination
  BytesPpConvert = false;

  if( BytesPpSrc == 3 &&                                             // Source has 3 bytes, is a color image
      (YaIPS_CamPar_AMode_ColMod > YAIPS_DISP_COLMOD_NORMAL ||       // and convert to 1 byte
       YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT)) {        // or have height mode

    BytesPpConvert = true;
    BytesPpDst = 1;
  }

  // Convert to FLTK image

  if( pYaIPS_Camera_Fl_Img != NULL &&
      ( pYaIPS_Camera_Fl_Img->data_w() != OutXX ||   // If image size is different
        pYaIPS_Camera_Fl_Img->data_h() != OutYY ||
        pYaIPS_Camera_Fl_Img->d() != BytesPpDst ||
        pYaIPS_Camera_Fl_Img->alloc_array == 0)) {                     // or no data allocated

    pYaIPS_Camera_Fl_Img->release();                                   // Release date
    pYaIPS_Camera_Fl_Img = NULL;
  }

  ImageWidthDst  = OutXX;
  SizeOfLineDst  = ImageWidthDst * BytesPpDst;
  ImageHeightDst = OutYY;
  SizeInBytesDst = SizeOfLineDst * ImageHeightDst;

  //x/ImageWidthSrc  = frameInfo.width;
  SizeOfLineSrc  = frameInfo.width * BytesPpSrc;
  ImageHeightSrc = frameInfo.height;

  OffX *= BytesPpSrc;

  if( pYaIPS_Camera_Fl_Img == NULL) {     // Have no image

    // Allocate memory for image

    pDataImg = new uchar[ SizeInBytesDst];

    // Create image envelope
    pYaIPS_Camera_Fl_Img = new Fl_RGB_Image( pDataImg, OutXX, OutYY, BytesPpDst /*, int LD=0*/);

    pYaIPS_Camera_Fl_Img->alloc_array = 1;      // Flag, data is allocated

  } else {

    // Get pointer to allocated data

    pDataImg = (uchar *)pYaIPS_Camera_Fl_Img->array;
  }

  // Copy data from camera to image buffer

  if( CamLine_NextLine < 0) {                  // Area camera mode

    int x, y;
    uchar *pSrc2;
    uchar *pDst2;

    pSrc = (uchar *)data;
#ifdef use_again
    pSrc += SizeOfLineSrc * OffY + OffX;
#else
    // ccap has image reversed so start with last line
    pSrc = (uchar *)data;
    pSrc += (ImageHeightSrc - 1 - OffY) * StrideSrc + OffX;
#endif
    pDst = pDataImg;

    for( y = 0; y < OutYY; y++) {

      pSrc2 = pSrc;
#ifdef use_again
      pSrc += SizeOfLineSrc;                   // Point to next line
#else
      // ccap has image reversed so decrement
      pSrc -= StrideSrc;                       // Point to previous line
#endif

      pDst2 = pDst;
      pDst += SizeOfLineDst;                   // Point to next line

      if( BytesPpConvert) {                    // Convert colors to BW
        switch( YaIPS_CamPar_AMode_ColMod) {
        case YAIPS_DISP_COLMOD_BW:
        default:
          for( x = 0; x < SizeOfLineDst; x++) {                // Walk through all columns
            *pDst2++ = (pSrc2[ 0] * 76 + pSrc2[ 1] * 150 + pSrc2[ 2] * 30) >> 8;
            pSrc2 += BytesPpSrc;
          }
          break;
        case YAIPS_DISP_COLMOD_R:
          for( x = 0; x < SizeOfLineDst; x++) {                // Walk through all columns
            *pDst2++ = pSrc2[ 0];
            pSrc2 += BytesPpSrc;
          }
          break;
        case YAIPS_DISP_COLMOD_G:
          for( x = 0; x < SizeOfLineDst; x++) {                // Walk through all columns
            *pDst2++ = pSrc2[ 1];
            pSrc2 += BytesPpSrc;
          }
          break;
        case YAIPS_DISP_COLMOD_B:
          for( x = 0; x < SizeOfLineDst; x++) {                // Walk through all columns
            *pDst2++ = pSrc2[ 2];
            pSrc2 += BytesPpSrc;
          }
          break;
        }
      } else {

        memcpy( pDst2, pSrc2, SizeOfLineDst);
      }
    }

  } else {                                     // Line camera simulation active

    if( CamLine_NextLine == 0) {               // Is the first line

      memset( pDataImg, 0, SizeInBytesDst);    // Black background
    }

    if( CamLine_NextLine < ImageHeightDst) {   // Not at top of image

      pSrc = (uchar *)data;
      pSrc += OffX;                            // Add X Offset

      pDst = pDataImg + SizeOfLineDst * CamLine_NextLine;

      if( YaIPS_CamPar_Acq_Mode != YAIPS_CAM_ACQ_MODE_HEIGHT) {  // NOT height mode. Must be line mode

        if( YaIPS_CamPar_LMode_nAvgLines <= 1) {                // Do NO line average ?

          int x;

          pSrc += StrideSrc * (ImageHeightSrc / 2);

          if( BytesPpConvert) {                                // Convert colors to BW
            switch( YaIPS_CamPar_AMode_ColMod) {
            case YAIPS_DISP_COLMOD_BW:
            default:
              for( x = 0; x < SizeOfLineDst; x++) {                // Walk through all columns
                *pDst++ = (pSrc[ 0] * 76 + pSrc[ 1] * 150 + pSrc[ 2] * 30) >> 8;
                pSrc += BytesPpSrc;
              }
              break;
            case YAIPS_DISP_COLMOD_R:
              for( x = 0; x < SizeOfLineDst; x++) {                // Walk through all columns
                *pDst++ = pSrc[ 0];
                pSrc += BytesPpSrc;
              }
              break;
            case YAIPS_DISP_COLMOD_G:
              for( x = 0; x < SizeOfLineDst; x++) {                // Walk through all columns
                *pDst++ = pSrc[ 1];
                pSrc += BytesPpSrc;
              }
              break;
            case YAIPS_DISP_COLMOD_B:
              for( x = 0; x < SizeOfLineDst; x++) {                // Walk through all columns
                *pDst++ = pSrc[ 2];
                pSrc += BytesPpSrc;
              }
              break;
            }
          } else {

            memcpy( pDst, pSrc, SizeOfLineDst);
          }

        } else {                                               // Do line average

          int i, n, n2, x, TempVal;
          uchar *pSrc2;

          n = YaIPS_CamPar_LMode_nAvgLines;                    // Get # average

          if( n > (ImageHeightSrc / 2)) {                      // Clip to 1/2 image height

            n = (ImageHeightSrc / 2);
          }

          if( n > 128) {                                       // Clip to max value

            n = 128;
          }

          n2 = n / 2;

          pSrc += StrideSrc * ((ImageHeightSrc - n) / 2);

          for( x = 0; x < SizeOfLineDst; x++) {                // Walk through all columns

            pSrc2 = pSrc;                                      // Begin of current element line
            if( BytesPpConvert) {
              pSrc += BytesPpSrc;                              // Point to next column
            } else {
              pSrc += 1;                                       // Point to next column
            }

            TempVal = 0;

            if( BytesPpConvert) {                              // Convert colors to BW
              switch( YaIPS_CamPar_AMode_ColMod) {
              case YAIPS_DISP_COLMOD_BW:
              default:
                for( i = 0; i < n; i++) {
                  TempVal += (pSrc2[ 0] * 76 + pSrc2[ 1] * 150 + pSrc2[ 2] * 30) >> 8;
                  pSrc2 += StrideSrc;                          // Point to next line
                }
                break;
              case YAIPS_DISP_COLMOD_R:
                for( i = 0; i < n; i++) {
                  TempVal += pSrc2[ 0];
                  pSrc2 += StrideSrc;                          // Point to next line
                }
                break;
              case YAIPS_DISP_COLMOD_G:
                for( i = 0; i < n; i++) {
                  TempVal += pSrc2[ 1];
                  pSrc2 += StrideSrc;                          // Point to next line
                }
                break;
              case YAIPS_DISP_COLMOD_B:
                for( i = 0; i < n; i++) {
                  TempVal += pSrc2[ 2];
                  pSrc2 += StrideSrc;                          // Point to next line
                }
                break;
              }
            } else {
              for( i = 0; i < n; i++) {
                TempVal += *pSrc2;                               // Sum up the pixel values
                pSrc2 += StrideSrc;                          // Point to next line
              }
            }

            *pDst++ = (TempVal + n2) / n;                      // Store average value
          }
        }

      } else {                                                 // Must be height mode

        int x, y, BestVal, BestY1, BestY2, TempVal, RegionHeight;
        uchar *pSrc2, *pSrc3, *pSrcBestY1;

        // Gauss filter the image. Is better for the laser maximum localization.
        // Try to extent filter region by one line up and down.

        Fl_RGB_Image *pTempAOI;
        int y1, y2;

#ifdef use_again
        y1 = YaIPS_CamPar_HMode_Top_Line;
        if( y1 > 0) {
          y1 -= 1;
        }

        y2 = YaIPS_CamPar_HMode_Base_Line;
        if( y2 < ImageHeightSrc - 1) {

          y2 += 1;
        }
#else
        // ccap has image reversed so reverse
        y1 = ImageHeightSrc - 1 - YaIPS_CamPar_HMode_Base_Line; // Replacement for top line
        if( y1 > 0) {
          y1 -= 1;
        }

        y2 = ImageHeightSrc - 1 - YaIPS_CamPar_HMode_Top_Line; // Replacement for bottom line
        if( y2 < ImageHeightSrc - 1) {

          y2 += 1;
        }
#endif

        pTempAOI = new Fl_RGB_Image( pSrc + (StrideSrc * y1), OutXX, y2 - y1 + 1, BytesPpSrc, SizeOfLineSrc);

        if( pTempAOI != NULL) {

          // Filter in place

          YaIPS_RGB_Filt_Gauss3x3( &pTempAOI, pTempAOI);

          pTempAOI->release();
        }

        // Get height

#ifdef use_again
        pSrc2 = pSrc + (SizeOfLineSrc * YaIPS_CamPar_HMode_Top_Line); // Point to top line
#else
        // ccap has image reversed so reverse
        pSrc2 = pSrc + (StrideSrc * (ImageHeightSrc - 1 - YaIPS_CamPar_HMode_Top_Line)); // Point to top line
#endif
        RegionHeight = YaIPS_CamPar_HMode_Base_Line - YaIPS_CamPar_HMode_Top_Line + 1;  // Height of acquire region

        for( x = 0; x < SizeOfLineDst; x++) {                  // Walk through all columns

          pSrc3 = pSrc2;                                       // Top of column
          if( BytesPpConvert) {
            pSrc2 += BytesPpSrc;                               // Point to next column
          } else {
            pSrc2 += 1;                                        // Point to next column
          }

          BestVal = -1;                                        // No best found
          BestY1  = -1;
          BestY2  = -1;
          pSrcBestY1 = NULL;

          // Get highest value in the line. y is relative to image height.

          for( y = 0; y < RegionHeight; y++) {                // Walk lines of acquisition area

            if( BytesPpConvert) {                             // Convert colors to BW
              switch( YaIPS_CamPar_AMode_ColMod) {
              case YAIPS_DISP_COLMOD_BW:
              default:
                TempVal = (pSrc3[ 0] * 76 + pSrc3[ 1] * 150 + pSrc3[ 2] * 30) >> 8;
                break;
              case YAIPS_DISP_COLMOD_R:
                TempVal = pSrc3[ 0];
                break;
              case YAIPS_DISP_COLMOD_G:
                TempVal = pSrc3[ 1];
                break;
              case YAIPS_DISP_COLMOD_B:
                TempVal = pSrc3[ 2];
                break;
              }
            } else {

              TempVal = (pSrc3[ 0] * 76 + pSrc3[ 1] * 150 + pSrc3[ 2] * 30) >> 8;
            }

            if( TempVal > BestVal) {                           // Have a new best

              BestVal = TempVal;                               // Catch new best value
              BestY1  = y;                                     // and its position
              BestY2  = -1;
              pSrcBestY1 = pSrc3;

            } else if( TempVal == BestVal && (BestY2 < 0 || BestY2 == y - 1)) {    // Have a plateau

              BestY2  = y;                                     // End of plateau
            }

#ifdef use_again
            pSrc3 += SizeOfLineSrc;                            // Point to next line
#else
            // ccap has image reversed so reverse
            pSrc3 -= StrideSrc;                            // Point to next line
#endif
          }

          //

          if( BestVal >= YaIPS_CamPar_HMode_Val_Thres) {       // Threshold best value


            // NOTE: Temporary calculations are done wit 8 bit precision

            if( BestY2 >= 0) {                                 // Was a plateau

              TempVal = (BestY1 + BestY2) << 7;                // Center of plateau (with 8 bit precision)

            } else {                                           // Single line height

              if( pSrcBestY1 != NULL &&                        // Have pointer to max value
                  BestY1 > 0 &&  BestY1 < RegionHeight - 1) {  // was above top line and below bottom line

                int v1, v2, v3;
                double divisor;

                // make a parabola interpolation

                v1 = pSrcBestY1[ - StrideSrc];        // Value before
                v2 = BestVal;
                v3 = pSrcBestY1[ StrideSrc];          // Value after

                divisor = 2 * (v1 - 2 * v2 + v3);

                if (divisor != 0) {

                  TempVal = (int)(256.0 * ((BestY1) + (v1 - v3) / divisor) + 0.5);

                } else {

                  TempVal = BestY1 << 8;
                }

              } else {

                TempVal = BestY1 << 8;
              }
            }

            // NOTE: By using the 8 bit precision, TempVal is multiplied with 256.
            // Range after calculation is 0 .. 255.

            TempVal = (int)((TempVal + 0.5) / RegionHeight);   // Max height is on top of region

            TempVal = 255 - TempVal;                           // Invert

            if( TempVal < 1) {                                 // Lowest measurable height
              TempVal = 1;
            } else if( TempVal > 255) {                        // Clip to maximum
              TempVal = 255;
            }

          } else {                                             // Best value was below threshold

            TempVal = 0;                                       // Mark not measurable column
          }

          *pDst++ = TempVal ;                                 // Store height value
        }
      }
    }

    CamLine_NextLine += 1;                     // One more line acquired

    if( CamLine_NextLine >= ImageHeightDst) {  // Was the last line

      CamLine_NextLine = -1;                   // Line scan image is acquired
    }
  }

  // Release frame
  ccap_video_frame_release( frame);

  return( 0);    // Return OK
}

/************************************************************************************
 * Presets for this tools window
 *
 */

static T_GUI_PreferenceEntry MyPreferences[] =
 {

  // File is open
  { PREF_T_INT,    "IsOpen",      "0", &IsOpen},

  // Window size

  { PREF_T_INT, "WinSizeX", "100", &MyWinSizeX},  // NOTE: values will be clipped against MYWIN_SIZE_X_MIN / MYWIN_SIZE_Y_MIN
  { PREF_T_INT, "WinSizeY", "100", &MyWinSizeY},

  // ...

  { PREF_T_STRING, "CamIdLastSelected", "-not-known-", &CamIdLastSelected, sizeof( CamIdLastSelected) - 1 },
  { PREF_T_INT,   "CamResLastSelected",          "-1", &CamResLastSelected},
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&pMyToolWin), &MyWinPosX, &MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, 0,
                                               &IsOpen, IqeB_GUI_CameraWin, ShutDown_close,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ImageDisp);

/************************************************************************************
 * IqeB_GUI_SnapAndDisplay
 *
 * Snap an image from the camera and display it
 *
 */
static void IqeB_GUI_SnapAndDisplay( int ForceAeraacquisition)
{

  if( ForceAeraacquisition) {                 // Force area camera acquisition

    CamLine_NextLine = -1;                    // Flag area camera mode

  } else {

    if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_AREA ||  // Area mode active
        (YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_LINE && YaIPS_CamPar_LMode_Adjust) ||
        (YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT && YaIPS_CamPar_HMode_Adjust)) {

      CamLine_NextLine = -1;                // Flag area camera mode

    } else {                                // Line or height mode active

      if( CamLine_NextLine < 0) {           // Was no acquisition before

        CamLine_NextLine = 0;               // Next line to acquire to
      }

    }
  }

  if( YaIPS_Camera_n >= 1 &&                // Does we have any camera
      YaIPS_Cam_Info_Idx >= 0 &&            // Check camera selected
      YaIPS_Cameras_Snap() >= 0 &&
      pYaIPS_Camera_Fl_Img != NULL) {

    // Snap was OK

    if( CamLine_NextLine <= 1 ||             // Area acquisition done or first or end of line scan acquisition
        YaIPS_CamPar_LMode_nRefresh <= 1 ||   // or line scan mode and refresh each line
        (CamLine_NextLine % YaIPS_CamPar_LMode_nRefresh) == 0) {  // or line scan mode and refresh after xxx lines

      YaIPS_ImageDispUpdateByNewImage(    &YaIPS_ImageDisp, pYaIPS_Camera_Fl_Img,
                                         MY_WIN_ID, (char *)MY_WIN_GUI_NAME);   // Load the image to the display

      if( CamLine_NextLine < 0 &&           // Area acquisition done or end of line scan acquisition
          YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID) {   // and display this on the

        YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pYaIPS_Camera_Fl_Img,
                                           MY_WIN_ID, (char *)MY_WIN_GUI_NAME);   // Load the image to the display
      }
    }
  }

  return;
}

/************************************************************************************
 * update GUI of this tool window
 *
 * DoEnable: true   do enable GUI element if OK
 *           false  do disable all GUI elements
 *               2  do periodically updates only
 */

static void MyWinUpdate( int DoEnable)
{
  char TempString[ 256];
  char *pTempString;
  unsigned int BigImageSourceCol;
  int Camera_DoGrab2, Camera_DoGrab3;
  Fl_Color TempCol;

  // To periodically updates first

  if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID) {           // Expected label color for the button

    BigImageSourceCol = YAIPS_BCOL_SHOW_THIS;
  } else {

    BigImageSourceCol = YAIPS_BCOL_SHOW_OTHER;
  }

  if( pGUI_Img_ShowOnBig->labelcolor() != BigImageSourceCol) {  // Color is different

    pGUI_Img_ShowOnBig->labelcolor( BigImageSourceCol);         // Set color

    pGUI_Img_ShowOnBig->redraw();                               // Redraw GUI element
  }

  // Keep image background color up to date. May be changed by load of a preset.

  if( YaIPS_Color_IMG_BGND != YaIPS_ImageDisp.pImage_Box->color()) {     // Changed color for main window image backgroudn

    YaIPS_ImageDisp.pImage_Box->color( YaIPS_Color_IMG_BGND);            // Background color
    YaIPS_ImageDisp.pImage_Box->redraw();
  }

  if( DoEnable == 2) {                                          // Was periodically updates only

    return;
  }

  // ...

  // Enable for grab/snap button
  Camera_DoGrab3 = ((! Camera_DoGrab) && CamLine_NextLine < 0) || Camera_DoGrab;

  // disable for other GUI elements
  Camera_DoGrab2 = Camera_DoGrab || CamLine_NextLine >= 0;      // Continues grab or line scan active

  IqeB_GUI_WidgetActivate( pGUI_Img_ShowOnBig, DoEnable &&                     // Enable GUI elements
                                               pYaIPS_Camera_Fl_Img != NULL);   // and have a big image

  // ...

  IqeB_GUI_WidgetActivate( pGUI_But_Snap, DoEnable &&                 // Enable GUI elements
                                          YaIPS_Camera_n > 0 &&       // Have minimum one camera
                                          YaIPS_Cam_Info_Idx >= 0 &&  // and any camera selected
                                          Camera_DoGrab2 == false);   // and no grab active

  // ...

  IqeB_GUI_WidgetActivate( pGUI_But_Grab, DoEnable &&                 // Enable GUI elements
                                          YaIPS_Camera_n > 0 &&       // Have minimum one camera
                                          YaIPS_Cam_Info_Idx >= 0 &&  // and any camera selected
                                          Camera_DoGrab3 == true);

  // Update camera selection

  IqeB_GUI_WidgetActivate( pGUI_Choice_SelCam, DoEnable &&                 // Enable GUI elements
                                               YaIPS_Camera_n >= 1 &&      // Have more than one camera
                                               YaIPS_Cam_Info_Idx >= 0 &&  // and any camera selected
                                               Camera_DoGrab2 == false);   // and no grab active

  if( YaIPS_Camera_n != Last_Choice_SelCam_n) {    // Test for changed

    int i;

    Last_Choice_SelCam_n = YaIPS_Camera_n;
    Last_Choice_SelRes_n = -2;

    // Rebuild the menu

    pGUI_Choice_SelCam->clear();

    for( i = 0; i < YaIPS_Camera_n; i++) {

      memset( TempString, 0, sizeof( TempString));

      sprintf( TempString, "%s", YaIPS_Cam_Info[ i].CamName);

      pGUI_Choice_SelCam->add( TempString);
    }

    pGUI_Choice_SelCam->value( YaIPS_Cam_Info_Idx);
  }

  if( YaIPS_Camera_n <= 0) {

    sprintf( TempString, LangStringLookup( "&GUI_Camera_Num1=- cameras"));

  } else if( YaIPS_Camera_n == 1) {

    sprintf( TempString,  LangStringLookup( "&GUI_Camera_Num2=1 camera"));

  } else {

    sprintf( TempString,  LangStringLookup( "&GUI_Camera_Num3=%d cameras"), YaIPS_Camera_n);
  }

  TempCol = fl_darker( fl_darker( YAIPS_BCOL_BUTTON));

  if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_LINE) {   // Line mode active

    strcat( TempString,  LangStringLookup( "&GUI_Camera_Mode1= - Line mode   "));
    TempCol = FL_DARK_RED;

  } else if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT) {   // Height mode active

    strcat( TempString, LangStringLookup( "&GUI_Camera_Mode2=  - Height mode   "));
    TempCol = FL_DARK_RED;

  } else {

    strcat( TempString, "                       ");
  }

  pTempString = (char *)pGUI_Choice_SelCam->label();     // Get label

  if( pTempString == NULL ||                     // String is different
      strcmp( pTempString, TempString) != 0) {

    pGUI_Choice_SelCam->label( NULL);            // Free old text
    pGUI_Choice_SelCam->copy_label( TempString); // Set new text
    pGUI_Choice_SelCam->labelcolor( TempCol);
    pGUI_Choice_SelCam->redraw();
  }

  // Update camera resolution

  IqeB_GUI_WidgetActivate( pGUI_Choice_SelRes, DoEnable &&                 // Enable GUI elements
                                               YaIPS_Camera_n >= 1 &&      // Have more than one camera
                                               YaIPS_Cam_Info_Idx >= 0 &&  // and any camera selected
                                               Camera_DoGrab2 == false);   // and no grab active

  if( YaIPS_Camera_n >= 0 && YaIPS_Cam_Info_Idx >= 0 &&  // Have a camera selected
      YaIPS_Cam_Info[ YaIPS_Cam_Info_Idx].resolutionCount != Last_Choice_SelRes_n) {    // Test for changed

    int i, xx, yy, g;

    Last_Choice_SelRes_n = YaIPS_Cam_Info[ YaIPS_Cam_Info_Idx].resolutionCount;

    // Rebuild the menu

    pGUI_Choice_SelRes->clear();

    for( i = 0; i < YaIPS_Cam_Info[ YaIPS_Cam_Info_Idx].resolutionCount; i++) {

      memset( TempString, 0, sizeof( TempString));

      xx = YaIPS_Cam_Info[ YaIPS_Cam_Info_Idx].Resolutions[ i].width;     // Remember used image size
      yy = YaIPS_Cam_Info[ YaIPS_Cam_Info_Idx].Resolutions[ i].height;

      g = GreatestcommonDivisor( xx, yy);

      if( g > 1 && xx / g <= 24 && yy / g <= 24) {

        sprintf( TempString, "%4d x %4d  %d : %d", xx, yy, xx / g, yy / g);
      } else {

        sprintf( TempString, "%4d x %4d", xx, yy);
      }

      pGUI_Choice_SelRes->add( TempString);
    }

    pGUI_Choice_SelRes->value( YaIPS_Cam_Info_Res);
  }

  // ...

  IqeB_GUI_WidgetActivate( pGUI_CamRefresh, DoEnable &&                 // Enable GUI elements
                                            Camera_DoGrab2 == false);   // and no grab active

  IqeB_GUI_WidgetActivate( pGUI_CamSetting, DoEnable &&                 // Enable GUI elements
                                            YaIPS_Camera_n > 0 /*&&       // Have any camera
                                            Camera_DoGrab2 == false*/);   // and no grab active

  IqeB_GUI_WidgetActivate( pGUI_Parameter, DoEnable &&                 // Enable GUI elements
                                           YaIPS_Camera_n > 0 &&       // Have any camera
                                           Camera_DoGrab2 == false);   // and no grab active

  // Update color of Toggle Teach / Inspection button

  int MouseTeachState;

  MouseTeachState = 1;                                            // We have a mouse callback state

  // Is teach mode available
  if( DoEnable &&                 // Enable GUI elements
      YaIPS_CamPar_Acq_Mode > YAIPS_CAM_ACQ_MODE_AREA && // Only for line and height mode
      YaIPS_Camera_n > 0 &&       // Have any camera
      Camera_DoGrab2 == false) {  // and no grab active

    MouseTeachState = YaIPS_CamPar_HMode_Adjust ? 3 : 2;
  }

  YaIPS_ImageDisp.pImage_Box->MouseTeachState = MouseTeachState;      // Shadow setting of teach

  IqeB_GUI_WidgetActivate( pGUI_TeachToggle, MouseTeachState >= 2);   // Teach mode available

  IqeB_GUI_WidgetLabelColor( pGUI_TeachToggle, MouseTeachState == 3 ? FL_GREEN : YAIPS_BCOL_BUTTON);

  IqeB_GUI_WidgetActivate( pGUI_TeachToggle, DoEnable &&                 // Enable GUI elements
                                             YaIPS_CamPar_Acq_Mode > YAIPS_CAM_ACQ_MODE_AREA && // Only for line and height mode
                                             YaIPS_Camera_n > 0 &&       // Have any camera
                                             Camera_DoGrab2 == false);   // and no grab active

  IqeB_GUI_WidgetLabelColor( pGUI_TeachToggle,
                             (YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_LINE && YaIPS_CamPar_LMode_Adjust) ||
                             (YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT && YaIPS_CamPar_HMode_Adjust) ? FL_GREEN : YAIPS_BCOL_BUTTON);

  // ...

  // Update FPS
  if( (Camera_DoGrab || CamLine_NextLine >= 0) &&  // Continues grab is on or line scan camera mode
      Camera_FPS > 0.0) {                          // and have a FPS value

    sprintf( TempString, LangStringLookup( "&GUI_Camera_FPS1=FPS\n%.1f"), Camera_FPS);
  } else {

    sprintf( TempString, LangStringLookup( "&GUI_Camera_FPS2=FPS\n---"));
  }

  pTempString = (char *)pGUI_Box_FPS->label();     // Get label

  if( pTempString == NULL ||                       // String is different
      strcmp( pTempString, TempString) != 0) {

    pGUI_Box_FPS->label( NULL);                    // Free old text
    pGUI_Box_FPS->copy_label( TempString);         // Set new text
  }
}

/************************************************************************************
 * IqeB_GUI_ToolsMyIdleAction
 */

static void IqeB_GUI_ToolsMyIdleAction( void *)
{
  unsigned int TimeTemp;
  static unsigned int TimeLastCalled_100 = 0;
  static unsigned int TimeLastCalled_FPS = 0;

  TimeTemp = GetTickCount();           // Get current time

  //

  if( pMyToolWin == NULL) {     // Security test, window must exist

    return;
  }

  //
  // Delayed startup of camera initialization.
  //

  if( ! StartupDone) {          // Startup is not done until now

    // Wait for all tool windows to started up
    if( YaIPS_GUI_Main_Do_Startup) {  // Startup phase of tool windows

      return;
    }

    StartupDone = true;        // Flag start up as done

    // Setup cameras

    YaIPS_ImageDisp.pImage_Box->labelsize( 20);     // Busy ...

    if( YaIPS_Camera_n < 0) {                       // Is NOT initialized until now ?

      YaIPS_ImageDisp.pImage_Box->label( LangStringLookup( "&GUI_Camera_Search2=Search cameras ..."));
      Fl::check();                                  // give fltk some cpu to update the screen

      YaIPS_Cameras_Init();

    } else {

      if( YaIPS_ImageDisp.pImage_Img != NULL) {      // Have a big image

        YaIPS_ImageDisp.pImage_Img->release();       // Release the image

        YaIPS_ImageDisp.pImage_Img = NULL;           // Set pointer to NULL
      }

      YaIPS_ImageDisp.pImage_Box->label( "...");
      Fl::check();                                   // give fltk some cpu to update the screen
    }

    if( YaIPS_Camera_n >= 1) {                       // Have more minimum one camera

      YaIPS_Cam_Info_Idx = -2;                       // No camera selected until now
      YaIPS_Cam_Info_Res = -2;                       // Preset default resolution

      // Try to find the last used camera

      int i;

      for( i = 0; i < YaIPS_Camera_n; i++) {

        if( strcmp( CamIdLastSelected, YaIPS_Cam_Info[ i].CamName) == 0) {   // Got it

          YaIPS_Cam_Info_Idx = i;
          YaIPS_Cam_Info_Res = CamResLastSelected;
          break;
        }
      }

      // ...

      if( YaIPS_Cam_Info_Idx >= 0) {                // Have a camera selected

        YaIPS_Cameras_Open( YaIPS_Cam_Info_Idx, YaIPS_Cam_Info_Res);   // Open last selected camera

      } else {

        YaIPS_Cameras_Open( 0, -1);                   // Open the first camera
      }
    }

    YaIPS_ImageDisp.pImage_Box->label( NULL);       // Remove busy ...

    // ...

    // Snap a single image from camera

    IqeB_GUI_SnapAndDisplay( true);                // Snap
    Sleep( 30);                                    // Some cameras need more time
    IqeB_GUI_SnapAndDisplay( true);                // Snap a second time (because of camera image buffer)

    MyWinUpdate( true);               // Update the GUI

    return;
  }

  //
  // some timed actions (not each call)
  //

  if( TimeTemp - TimeLastCalled_100 >= 100) {  // 100 ms gone since last call

    TimeLastCalled_100 = TimeTemp;             // Remember last time called

    // some time gone, do ...

    // update window size

    MyWinSizeX = pMyToolWin->w();  // update window size
    MyWinSizeY = pMyToolWin->h();  // update window size

    // Periodically updates on GUI
    MyWinUpdate( 2);
  }

  //
  // Check for image display GUI elements
  //

  // Check an image display for size change and redisplay if size has changed.
  YaIPS_ImageDispDrawUpdate( &YaIPS_ImageDisp, false);

  //
  // Grab next image ?
  //

  if( YaIPS_Camera_n >= 1 &&                // Does we have any camera
      YaIPS_Cam_Info_Idx >= 0 &&            // Check camera selected
      (Camera_DoGrab != 0 ||                // Continues grab is on
         CamLine_NextLine >= 0)) {          // Line scan mode active

    unsigned int TimeStamp, TimeDelta;

    IqeB_GUI_SnapAndDisplay( false);       // Snap and display image

    // Calculate FPS

    TimeStamp = GetTickCount();

    if( TimeStamp == 0) TimeStamp = 1;      // Skip zero

    if( Camera_FPS_TimeStamp != 0) {

      TimeDelta = TimeStamp - Camera_FPS_TimeStamp;   // Time delta

      if( TimeDelta > 11.0) {                 // Minimum time

        if( Camera_FPS == 0.0) {              // No frame until now

          Camera_FPS_n = 1;

          Camera_FPS_2 = 1000.0 / (float)TimeDelta;
          Camera_FPS = Camera_FPS_2;

        } else if( Camera_FPS_n < 10) {         // Less then 10 frames

          Camera_FPS_n += 1;

          // Sliding average
          Camera_FPS_2 =  (Camera_FPS_2 * (Camera_FPS_n - 1) + 1000.0 / (float)TimeDelta) / (float)Camera_FPS_n;
        } else {

          // Sliding average
          Camera_FPS_2 =  (Camera_FPS_2 * (Camera_FPS_n - 1) + 1000.0 / (float)TimeDelta) / (float)Camera_FPS_n;
        }

        // Smooth FPS output

        if( TimeTemp - TimeLastCalled_FPS >= 500) {  // 300 ms gone since last call

          TimeLastCalled_FPS = TimeTemp;             // Remember last time called

          Camera_FPS = Camera_FPS_2;
        }

      } else {

        Camera_FPS = 0.0;
      }
    }

    Camera_FPS_TimeStamp = TimeStamp;
  }

  MyWinUpdate( true);               // Update the GUI

  return;
}

/************************************************************************************
 * pop up menu
 *
 * pValueArg is a pointer to the widget. Set this pointer to NULL on deletion.
 */

// Open camera settings dialog
static void IqeB_Camera_Settings_Callback( Fl_Widget *pWidget, void *pValueArg)
{

  // Open camera properties dialog

  IqeB_GUI_But_Tool_OpenWin_Callback( NULL, (void *)YaIPS_GUI_CamPropertiesWin);
}

// Rescan cameras
static void IqeB_Camera_Rescan_Callback( Fl_Widget *pWidget, void *pValueArg)
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  Fl_RGB_Image *oldimage;

  pYaIPS_ImageDisp = &YaIPS_ImageDisp;

  // Release previous camera image

  if( pYaIPS_ImageDisp->pImage_Img != NULL) {      // Have a big image

	  pYaIPS_ImageDisp->pImage_Img->release();       // Release the image

	  pYaIPS_ImageDisp->pImage_Img = NULL;           // Set pointer to NULL
  }

  // Release previous box image
  oldimage = (Fl_RGB_Image *)pYaIPS_ImageDisp->pImage_Box->image();

  if( oldimage != NULL) {

    oldimage->release();                         // Release date
  }

  pYaIPS_ImageDisp->pImage_Box->image( NULL);     // Set the image to the display

  pYaIPS_ImageDisp->ImageSourceID = YAIPS_WIN_ID_NONE;   // Invalidate image source

  // Search cameras ...

  YaIPS_ImageDisp.pImage_Box->labelsize( 20);     // Busy ...
  YaIPS_ImageDisp.pImage_Box->label( LangStringLookup( "&GUI_Camera_Search2=Search cameras ..."));

  YaIPS_Camera_n = -1;                           // Force rescan of cameras
  YaIPS_Cam_Info_Idx = -1;
  YaIPS_Cam_Info_Res = -1;
  Last_Choice_SelCam_n = -2;                     // Last size of pGUI_Choice_SelCam
  Last_Choice_SelRes_n = -2;

  pGUI_Choice_SelCam->clear();                   // Empty camera selection
  pGUI_Choice_SelRes->clear();                   // Empty resolution selection

  MyWinUpdate( true);               // Update the GUI

  Fl::check();                                   // give fltk some cpu to update the screen

  if( ccap_provider_is_opened( pCcapProvider)) {             // Other device is open

    ccap_provider_close( pCcapProvider);
  }

  YaIPS_Cameras_Init();

  if( YaIPS_Camera_n >= 1) {                   // Have more than one camera

    YaIPS_Cam_Info_Idx = -1;                       // No camera selected until now
    YaIPS_Cam_Info_Res = -1;                       // Preset default resolution

    // Try to find the last used camera

    int i;

    for( i = 0; i < YaIPS_Camera_n; i++) {

      if( strcmp( CamIdLastSelected, YaIPS_Cam_Info[ i].CamName) == 0) {   // Got it

        YaIPS_Cam_Info_Idx = i;
        YaIPS_Cam_Info_Res = CamResLastSelected;
        break;
      }
    }

    // ...

    if( YaIPS_Cam_Info_Idx >= 0) {                // Have a camera selected

      YaIPS_Cameras_Open( YaIPS_Cam_Info_Idx, YaIPS_Cam_Info_Res);   // Open last selected camera

    } else {

      YaIPS_Cameras_Open( 0, -1);                   // Open the first camera
    }
  }

  YaIPS_ImageDisp.pImage_Box->label( NULL);       // Remove busy ...

  // ...

  // Snap a single image from camera

  IqeB_GUI_SnapAndDisplay( true);                // Snap
  Sleep( 30);                                    // Some cameras need more time
  IqeB_GUI_SnapAndDisplay( true);                // Snap a second time (because of camera image buffer)

  MyWinUpdate( true);               // Update the GUI
}

/************************************************************************************
 * IqeB_Camera_GUI_Callback
 *
 * Callback for GUI elements
 */

static void IqeB_Camera_GUI_Callback( Fl_Widget *w, void *data)
{

  // Does we have any camera

  if( YaIPS_Camera_n < 1) {                 // Does we have no camera

    return;                                 // Exit to caller
  }

  // Ensure a camera is selected

  if( YaIPS_Cam_Info_Idx < 0) {             // No camera selected until now

    YaIPS_Cameras_Open( 0, -1);             // Try to open first camera
  }

  if( YaIPS_Cam_Info_Idx < 0) {             // Check for a selected camera

    return;                                 // Exit to caller
  }

  // Show loaded image on the big display

  if( w == pGUI_Img_ShowOnBig) {         // Show on big image

    if( pYaIPS_Camera_Fl_Img != NULL) {

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pYaIPS_Camera_Fl_Img,
                                       MY_WIN_ID, (char *)MY_WIN_GUI_NAME);   // Load the image to the display
    }

    MyWinUpdate( true);                 // Update the GUI
  }

  // Snap button

  else if( w == pGUI_But_Snap) {        // Snap button

    // Snap image from camera

    Camera_DoGrab = false;              // Continues grab off

    MyWinUpdate( false);                // Disable all GUI elements
    Fl::check();                        // give fltk some cpu to update the screen

    IqeB_GUI_SnapAndDisplay( false);    // Snap and display image

    if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_AREA ||  // Area mode active
        (YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_LINE && YaIPS_CamPar_LMode_Adjust) ||
        (YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT && YaIPS_CamPar_HMode_Adjust)) {

      Sleep( 30);                       // Some cameras need more time
      IqeB_GUI_SnapAndDisplay( false);  // Snap a second time (because of camera image buffer)
    }

    MyWinUpdate( true);                 // Update the GUI
  }

  // Continues grab on/off

  else if( w == pGUI_But_Grab) {          // Grab button

    Camera_DoGrab = ! Camera_DoGrab;      // Toggle continues grab

    CamLine_NextLine = -1;                // Flag area camera mode

    w->label( Camera_DoGrab ? "@+3||" : "@+3>");

    Camera_FPS = 0.0;                       // Reset FPS things
    Camera_FPS_TimeStamp = 0;

    MyWinUpdate(true);                    // Update the GUI
  }

  // Select other camera

  else if( w == pGUI_Choice_SelCam &&       // Camera selection
           YaIPS_Camera_n >= 0) {           // and have any cameras

    int TempInt;

    TempInt = pGUI_Choice_SelCam->value();  // Menu nr. starting with 0

    if( TempInt >= YaIPS_Camera_n) TempInt = YaIPS_Camera_n - 1;   // Check range
    if( TempInt < 0) TempInt = 0;

    if( TempInt != YaIPS_Cam_Info_Idx) {    // Camera selection will change

      MyWinUpdate( false);                  // Disable all GUI elements
      Fl::flush();                          // give fltk some cpu to update the screen

      // Change camera

      YaIPS_Cameras_Open( TempInt, -1);
      Last_Choice_SelRes_n = -2;            // Reset resolution selection

      Camera_DoGrab = false;                // Continues grab off

      // Snap image from camera

      IqeB_GUI_SnapAndDisplay( true);   // Snap
      Sleep( 30);                       // Some cameras need more time
      IqeB_GUI_SnapAndDisplay( true);   // Snap a second time (because of camera image buffer)

      MyWinUpdate( true);               // Update the GUI
    }
  }

  // Select other resolution

  else if( w == pGUI_Choice_SelRes &&       // Resolution selection
           YaIPS_Camera_n >= 0 &&           // and have any cameras
           YaIPS_Cam_Info_Idx >= 0) {       // any any camera is selected

    YaIPS_Cam_info_t *pYaIPS_Cam_Info;
    int TempInt;

    pYaIPS_Cam_Info = YaIPS_Cam_Info + YaIPS_Cam_Info_Idx;   // use this camera info entry

    TempInt = pGUI_Choice_SelRes->value();  // Menu nr. starting with 0

    if( TempInt >= pYaIPS_Cam_Info->resolutionCount) {
      TempInt = pYaIPS_Cam_Info->resolutionCount - 1;   // Check range
    }
    if( TempInt < 0) TempInt = 0;

    if( TempInt != YaIPS_Cam_Info_Res) {    // Resolution selection will change

      MyWinUpdate( false);                  // Disable all GUI elements
      Fl::flush();                          // give fltk some cpu to update the screen

      // Change camera

      YaIPS_Cameras_Open( YaIPS_Cam_Info_Idx, TempInt);

      Camera_DoGrab = false;                // Continues grab off

      // Snap image from camera

      IqeB_GUI_SnapAndDisplay( true);   // Snap
      Sleep( 30);                       // Some cameras need more time
      IqeB_GUI_SnapAndDisplay( true);   // Snap a second time (because of camera image buffer)

      MyWinUpdate( true);               // Update the GUI
    }
  }

  // Open parameter dialog

  else if( w == pGUI_Parameter) {    // Parameter button

    YaIPS_GUI_CamSettingsWin( pMyToolWin->x() + 16, pMyToolWin->y() + 16, 0, 0, 0);

  }

  // Toggle Teach / Inspection

  else if( w == pGUI_TeachToggle) {

    if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_LINE) {

      YaIPS_CamPar_LMode_Adjust = ! YaIPS_CamPar_LMode_Adjust;

    } else if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT) {

      YaIPS_CamPar_HMode_Adjust = ! YaIPS_CamPar_HMode_Adjust;
    }

    IqeB_GUI_CameraImageRedraw();               // Redraw camera image to show changes of lines
  }

}

/************************************************************************************
 * close_cb, close this window
 *
 * pValueArg is a pointer to the widget. Set this pointer to NULL on deletion.
 */

static void close_cb( Fl_Widget *w, void *pValueArg)
{
  // Close any open camera

  YaIPS_Cameras_Close();

  // ...

  YaIPS_ImageDispReleaseBeforeClose( &YaIPS_ImageDisp);

  IsOpen = false;                                    // Flag info data is not in use

  IqeB_GUI_CloseToolWindow( (void **)&pMyToolWin);

  // ...

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::remove_idle( IqeB_GUI_ToolsMyIdleAction);      // Redraw window during idle
#endif
  Fl::remove_check( IqeB_GUI_ToolsMyIdleAction);     // Check small image size change
}

/************************************************************************************
 * ShutDown_close
 *
 * Called on shutdown of application
 *
 * pValueArg is a pointer to the widget. Set this pointer to NULL on deletion.
 */

static void ShutDown_close( Fl_Widget *w, void *pValueArg)
{

  close_cb( w, pValueArg);
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
                                        int DoClip)                              // if true (> 0) handle clipping of draw region else caller must do it
{
  int x1, y1, xx, yy, y, LineWidth, OutXX, OutYY, OffY;
  //x/ int OffX;                          // Unused variables

  if( YaIPS_Camera_n <= 0) {              // No camera

    return;
  }

  // Check for valid output image displayed

  if( YaIPS_ImageDisp.pImage_Img == NULL) {

    return;
  }

  // Size for area image

  OutXX = YaIPS_Camera_XX;               // Preset use of full image
  OutYY = YaIPS_Camera_YY;

  if( YaIPS_CamPar_AMode_Square) {       // Square cut out active ?

    // Get square size
    if( OutXX > OutYY) {

      //x/OffX  = (OutXX - OutYY) / 2;
      OutXX = OutYY;

    } else {

      //x/OffY  = (OutYY - OutXX) / 2;
      OutYY = OutXX;
    }
  }

  // Preparations

  LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

  x1 = pYaIPS_ImageDisp->BigImage_sx;
  y1 = pYaIPS_ImageDisp->BigImage_sy;
  xx = pYaIPS_ImageDisp->BigImage_sw;
  yy = pYaIPS_ImageDisp->BigImage_sh;

  //x/OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
  OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

  // ...

  if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_LINE && YaIPS_CamPar_LMode_Adjust) {     // Adjust for line mode

    // Have to clip ?

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( x1, y1, xx, yy);
    }

    // ...

    fl_line_style( 0);   // Reset to default
    fl_color( FL_GREEN - 2);

    if( YaIPS_CamPar_LMode_nAvgLines <= 1) { // Do NO line average ?

      y = (int)( ((OutYY / 2) - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
      fl_xyline( x1, y1 + y, x1 + xx - 1);
      if( LineWidth == 3) {                // Hack: line width sometime is 1. To it explicit.
        fl_xyline( x1, y1 + y - 1, x1 + xx - 1);
        fl_xyline( x1, y1 + y + 1, x1 + xx - 1);
      }

    } else {                                // Do line average

      int n;

      n = YaIPS_CamPar_LMode_nAvgLines;      // Get # average

      if( n > (OutYY / 2)) {          // Clip to 1/2 image height

        n = (OutYY / 2);
      }

      if( n > 128) {                        // Clip to max value

        n = 128;
      }

      y = (int)( (((OutYY - n) / 2) - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
      fl_xyline( x1, y1 + y, x1 + xx - 1);
      if( LineWidth == 3) {                // Hack: line width sometime is 1. To it explicit.
        fl_xyline( x1, y1 + y - 1, x1 + xx - 1);
        fl_xyline( x1, y1 + y + 1, x1 + xx - 1);
      }

      y = (int)( (((OutYY - n) / 2 + n) - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
      fl_xyline( x1, y1 + y, x1 + xx - 1);
      if( LineWidth == 3) {                // Hack: line width sometime is 1. To it explicit.
        fl_xyline( x1, y1 + y - 1, x1 + xx - 1);
        fl_xyline( x1, y1 + y + 1, x1 + xx - 1);
      }
    }

    fl_line_style( 0);   // Reset to default

  } else
  if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT && YaIPS_CamPar_HMode_Adjust) {   // Adjust for height mode

    // Have to clip ?

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( pYaIPS_ImageDisp->BigImage_bx, pYaIPS_ImageDisp->BigImage_by, pYaIPS_ImageDisp->BigImage_bw, pYaIPS_ImageDisp->BigImage_bh);
    }

    // ...

    fl_line_style( 0);   // Reset to default

    // Draw AOI lines

    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0 &&  // Mouse is over any AOI
        pYaIPS_ImageDisp->AoiIdNr == 0) {                                   // and mouse is over first AOI

      fl_color( FL_RED);
    } else {

      fl_color( FL_GREEN - 2);
    }

    y = (int)( (YaIPS_CamPar_HMode_Base_Line - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    fl_xyline( x1, y1 + y, x1 + xx - 1);
    if( LineWidth == 3) {                // Hack: line width sometime is 1. To it explicit.
      fl_xyline( x1, y1 + y - 1, x1 + xx - 1);
      fl_xyline( x1, y1 + y + 1, x1 + xx - 1);
    }

    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0 &&  // Mouse is over any AOI
        pYaIPS_ImageDisp->AoiIdNr == 1) {                                   // and mouse is over first AOI

      fl_color( FL_RED);
    } else {

      fl_color( FL_GREEN - 2);
    }

    y = (int)( (YaIPS_CamPar_HMode_Top_Line - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    fl_xyline( x1, y1 + y, x1 + xx - 1);
    if( LineWidth == 3) {                // Hack: line width sometime is 1. To it explicit.
      fl_xyline( x1, y1 + y - 1, x1 + xx - 1);
      fl_xyline( x1, y1 + y + 1, x1 + xx - 1);
    }

    fl_line_style( 0);   // Reset to default
  }

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
static void YaIPS_GUI_MyDrawAfter_Other( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // point to image display data
                                         int SubWinIDx)                           // What sub window to use

{

  if( pMyToolWin == NULL) {                                  // Security test

    return;
  }

  YaIPS_GUI_MyDrawAfter_Func( pYaIPS_ImageDisp, true);     // Additional drawings after the image was drawn
}

/************************************************************************************
 * YaIPS_GUI_MyDrawAfter_cb
 *
 * Additional drawings after the image was drawn.
 *
 */
static void YaIPS_GUI_MyDrawAfter_cb( Fl_Widget *pW,
                                      void *pArg1,        // Pointer to Fl_YaIPS_ImageDisp_t
                                      void *pArg2)        // Optional pointer to ToolData
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  int ierr;

  pYaIPS_ImageDisp = (Fl_YaIPS_ImageDisp_t *)pArg1;

  ierr = YaIPS_ImageDispCalcSizes( pYaIPS_ImageDisp);   // Check sizes

  if( ierr < 0) {                          // No image box (no drawing area)

    return;                                // Return to caller
  }

  YaIPS_GUI_MyDrawAfter_Func( pYaIPS_ImageDisp, true);          // Additional drawings after the image was drawn

  YaIPS_ImageDispDrawAfter_Common( pYaIPS_ImageDisp, 0, true);  // Draw additional common drawings

  // Finish up

  fl_line_style( 0);   // Reset to default
  fl_pop_clip();
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
  int ierr, x, y;
  int minAoiDist, IsBigImageDisp;
  int *pLineNr_Best, *pLineNr_This;
  static int Last_x = -9999, Last_y = -9999;           // Must be static
  static int Pressed_x, Pressed_y;                     // Used for move with pressed mouse button
  static int Pressed_LineNr, *pPressed_LineNr_Best;    // What line on press of mouse button

  pYaIPS_ImageDisp = (Fl_YaIPS_ImageDisp_t *)pArg1;    // Get pointer to image display data

  // Only used for height mode and adjust of lines

  if( YaIPS_CamPar_Acq_Mode != YAIPS_CAM_ACQ_MODE_HEIGHT ||    // Is NOT height mode
      YaIPS_CamPar_HMode_Adjust == false) {                    // and NO height mode adjust

    return( -1);
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

  pYaIPS_ImageDisp->CursorShape = IsBigImageDisp ? 0 : -1;    // Overwrite: Preset default cursor shape
  pYaIPS_ImageDisp->MyWinID = MY_WIN_ID;                      // Overwrite: Update big image, set my tool window ID
  minAoiDist     = -1;                                // Needed for section of nearest AOI
  pLineNr_Best   = NULL;                              // Modify this line nr

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

      if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT &&               // Height mode
          YaIPS_CamPar_HMode_Adjust) {                                        // and can be changed with the mouse

        int yMouse, dist, AoiIdNr;

        yMouse = (int)( pYaIPS_ImageDisp->MouseY / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
        yMouse += (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

        AoiIdNr = -1;

        // Check base line

        pLineNr_This = &YaIPS_CamPar_HMode_Base_Line;

        // Get nearest distance to frame

        dist = abs( yMouse - *pLineNr_This);

        if( dist < YAIPS_AOI_FRAME_DIST &&                // Near border and better than previous minimum distance
            (minAoiDist == -1 || dist < minAoiDist)) {

          pLineNr_Best = pLineNr_This;

          AoiIdNr = 0;
        }

        // Check top line

        pLineNr_This = &YaIPS_CamPar_HMode_Top_Line;

        // Get nearest distance to frame

        dist = abs( yMouse - *pLineNr_This);

        if( dist < YAIPS_AOI_FRAME_DIST &&                // Near border and better than previous minimum distance
            (minAoiDist == -1 || dist < minAoiDist)) {

          pLineNr_Best = pLineNr_This;

          AoiIdNr = 1;
        }

        if( pLineNr_Best != NULL) {               // Got one

          pYaIPS_ImageDisp->CursorShape = FL_CURSOR_NS;
          pYaIPS_ImageDisp->AoiDeltaAdd = 0x02;
          pYaIPS_ImageDisp->AoiIdNr     = AoiIdNr;
        }
      }

      if( pYaIPS_ImageDisp->mouseleft) {                    // Left mouse button pressed

        if( pYaIPS_ImageDisp->AoiDeltaAdd != 0 && pYaIPS_ImageDisp->Latched_AoiDeltaAdd == 0) {   // Latch AOI mouse modification

          pYaIPS_ImageDisp->Latched_AoiDeltaAdd = pYaIPS_ImageDisp->AoiDeltaAdd;
          pYaIPS_ImageDisp->Latched_CursorShape = pYaIPS_ImageDisp->CursorShape;

          pPressed_LineNr_Best = pLineNr_Best;                    // Modify this AOI
          Pressed_x = Last_x + pYaIPS_ImageDisp->Delta_x;         // Latch position at button press
          Pressed_y = Last_y + pYaIPS_ImageDisp->Delta_y;

          Pressed_LineNr = *pLineNr_Best;                        // Value at button press
        }

        if( pYaIPS_ImageDisp->Latched_AoiDeltaAdd != 0) {        // Have latched AOI mouse modification

          pYaIPS_ImageDisp->AoiDeltaAdd = pYaIPS_ImageDisp->Latched_AoiDeltaAdd;   // Use it
          pYaIPS_ImageDisp->CursorShape = pYaIPS_ImageDisp->Latched_CursorShape;
          pLineNr_Best = pPressed_LineNr_Best;
        }

      } else {                                                   // Left mouse button is NOT pressed

        pYaIPS_ImageDisp->Latched_AoiDeltaAdd = 0;               // Reset latched data
        pYaIPS_ImageDisp->Latched_CursorShape = 0;
      }

      if( pYaIPS_ImageDisp->mouseleft &&                                        // and left button pressed
          pYaIPS_ImageDisp->AoiDeltaAdd != 0 &&                                 // and add deltas
          (pYaIPS_ImageDisp->Delta_x != 0 || pYaIPS_ImageDisp->Delta_y != 0)) { // and mouse has moved

        int CamYY;

        // Get camera height
        if( YaIPS_Camera_YY > 0) {    // Camera height is known

          CamYY = YaIPS_Camera_YY;
        } else {

          CamYY = 2024;
        }

        *pLineNr_Best = Pressed_LineNr;                            // Restore value from button press

        pYaIPS_ImageDisp->Delta_x = dto32( (Last_x - Pressed_x) / pYaIPS_ImageDisp->PixelImageToScreen);
        pYaIPS_ImageDisp->Delta_y = dto32( (Last_y - Pressed_y) / pYaIPS_ImageDisp->PixelImageToScreen);

        *pLineNr_Best += pYaIPS_ImageDisp->Delta_y;

        if( *pLineNr_Best < 0) {

          *pLineNr_Best = 0;
        }

        if( *pLineNr_Best >= CamYY) {

          *pLineNr_Best = CamYY;
        }

        if( pLineNr_Best == &YaIPS_CamPar_HMode_Top_Line) {      // Has changed top line

          int ValueBot, ValueTop;

          ValueTop = YaIPS_CamPar_HMode_Top_Line;
          ValueBot = YaIPS_CamPar_HMode_Base_Line;

          if( ValueBot - ValueTop < HMODE_ACQ_REGION_MIN_HEIGHT - 1) {  // Below minimum distance

            ValueBot = ValueTop + HMODE_ACQ_REGION_MIN_HEIGHT - 1;

            if( ValueBot >= CamYY) {

              ValueBot = CamYY - 1;
              ValueTop = ValueBot - HMODE_ACQ_REGION_MIN_HEIGHT + 1;
            }
          }

          YaIPS_CamPar_HMode_Top_Line  = ValueTop;
          YaIPS_CamPar_HMode_Base_Line = ValueBot;

        } else
        if( pLineNr_Best == &YaIPS_CamPar_HMode_Base_Line) {     // Has changed bottom line

          int ValueBot, ValueTop;

          ValueTop = YaIPS_CamPar_HMode_Top_Line;
          ValueBot = YaIPS_CamPar_HMode_Base_Line;

          if( ValueBot - ValueTop < HMODE_ACQ_REGION_MIN_HEIGHT - 1) {  // Below minimum distance

            ValueTop = ValueBot - HMODE_ACQ_REGION_MIN_HEIGHT + 1;

            if( ValueTop < 0) {

              ValueTop = 0;
              ValueBot = HMODE_ACQ_REGION_MIN_HEIGHT - 1;
            }
          }

          YaIPS_CamPar_HMode_Top_Line = ValueTop;
          YaIPS_CamPar_HMode_Base_Line = ValueBot;
        }

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
 * IqeB_GUI_CameraWin
 *
 * Open camera window
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_CameraWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  Fl_Button       *pTemp_Button;

  //
  // window already created --> show it
  //

  if( pMyToolWin != NULL) {         // already have tool window

    // Show invisible window or bring visible window to foreground

    pMyToolWin->show();          // show it

    return;
  }

  //
  // Setup some variables
  //

  if( YaIPS_CamPar_AMode_StartAcqOn) {          // Start with 'continuous acquire on' after open of dialog

    Camera_DoGrab = true;                       // Continues grab on
  } else {
    Camera_DoGrab = false;                      // Continues grab off
  }
  Camera_FPS = 0.0;                             // Reset FPS value
  Camera_FPS_TimeStamp = 0;                     // and FPS time stamp
  CamLine_NextLine = -1;                        // No line scan active

  //
  // creation of window on first call
  //

  IsOpen = true;           // Flag image as open

  // clip window sizes

  if( MyWinSizeX < MYWIN_SIZE_X_MIN) MyWinSizeX = MYWIN_SIZE_X_MIN;
  if( MyWinSizeX > MYWIN_SIZE_X_MAX) MyWinSizeX = MYWIN_SIZE_X_MAX;

  if( MyWinSizeY < MYWIN_SIZE_Y_MIN) MyWinSizeY = MYWIN_SIZE_Y_MIN;
  if( MyWinSizeY > MYWIN_SIZE_Y_MAX) MyWinSizeY = MYWIN_SIZE_Y_MAX;

  if( pMyToolWin == NULL) {  // no tool window until now

    int xPos, yPos;

    xPos = xRight;
    yPos = yTop;

    if( MyWinPosX != IQE_GUI_NO_WINPOS_X && MyWinPosY != IQE_GUI_NO_WINPOS_Y) { // have last window position

      xPos = MyWinPosX;
      yPos = MyWinPosY;
    }

    YAIPS_BEFORE_TOOL_WIN_CREATE();  // Execute this before creation of a tool windows

    pMyToolWin = new Fl_Double_Window( xPos, yPos, MyWinSizeX, MyWinSizeY, MY_WIN_GUI_NAME);

    if( pMyToolWin == NULL) {   // security test

      IsOpen = false;           // Flag info data is not in use

      return;
    }
  }

  //
  //  GUI things
  //

  Fl_Group *pGUI_GroupTopSide;           // Top side of window
  int x, x1, y, xx, xx0, yy, hWin, wWin;

  hWin = pMyToolWin->h();
  wWin = pMyToolWin->w();

  //
  // Group, top side
  //

  x = 4;
  y = 2;

  yy = 36;                  // Top side group height

  pGUI_GroupTopSide = new Fl_Group( x, y, wWin - 8, yy * 2 + 4);

  y += 4;

  xx0 = yy;
  x1 = x;

  //
  // First line of top side
  //

  // Button acquire

  xx = xx0;

  pGUI_Img_ShowOnBig = new Fl_Button( x1, y, xx, yy, "@+3circle");
  pGUI_Img_ShowOnBig->callback( IqeB_Camera_GUI_Callback, NULL);
  pGUI_Img_ShowOnBig->tooltip( LANGDEF_SHOW_ON_BIG_IMAGE);
  pGUI_Img_ShowOnBig->labelcolor( YAIPS_BCOL_SHOW_OTHER);

  x1 += xx + 5;

  // Snap camera image

  xx = xx0;

  pGUI_But_Snap = new Fl_Button( x1, y, xx, yy, "@+32>");
  pGUI_But_Snap->callback( IqeB_Camera_GUI_Callback, NULL);
  pGUI_But_Snap->tooltip( LangStringLookup( "&GUI_Camera_Tools2a="
                          "Acquire a picture.\n"
                          "Shortcut: Ctrl+A"));
  pGUI_But_Snap->labelcolor( YAIPS_BCOL_BUTTON);
  pGUI_But_Snap->shortcut( FL_COMMAND + 'a');       // Short cut key

  x1 += xx + 5;

  // Continues grab on/off

  xx = xx0;

  pGUI_But_Grab = new Fl_Button( x1, y, xx, yy, Camera_DoGrab ? "@+3||" : "@+3>");
  pGUI_But_Grab->callback( IqeB_Camera_GUI_Callback, NULL);
  pGUI_But_Grab->tooltip( LangStringLookup( "&GUI_Camera_Tools3a="
                          "Continuous acquire on/off.\n"
                          "Shortcut: Ctrl+' '"));
  pGUI_But_Grab->labelcolor( YAIPS_BCOL_BUTTON);
  pGUI_But_Grab->shortcut( FL_COMMAND + ' ');       // Short cut key


  x1 += xx + 5;

  // Select camera resolution

  xx = 150;

  pGUI_Choice_SelRes = new Fl_Choice( x1, y + 10, xx, yy - 10, LangStringLookup( "&GUI_Camera_Tools5=Resolution"));
  pGUI_Choice_SelRes->callback( IqeB_Camera_GUI_Callback, NULL);
  pGUI_Choice_SelRes->tooltip( LangStringLookup( "&GUI_Camera_Tools5a=Change camera resolution"));
  pGUI_Choice_SelRes->align( FL_ALIGN_TOP | FL_ALIGN_LEFT);
  pGUI_Choice_SelRes->labelsize( 10);
  Last_Choice_SelRes_n = -2;      // Reset last size of pGUI_Choice_SelRes

  x1 += xx + 5;

  // Output FPS

  xx = xx0;

  pGUI_Box_FPS = new Fl_Box( x1, y, xx, yy);
  pGUI_Box_FPS->box(FL_BORDER_BOX);
  pGUI_Box_FPS->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

  x1 += xx + 5;

  //
  // Second line of top side
  //

  y += yy + 4;
  x1 = x;

  // Select camera

  xx = 273;

  pGUI_Choice_SelCam = new Fl_Choice( x1, y + 10, xx, yy - 10, NULL);
  pGUI_Choice_SelCam->callback( IqeB_Camera_GUI_Callback, NULL);
  pGUI_Choice_SelCam->tooltip( LangStringLookup( "&GUI_Camera_Tools4a=Change camera selection"));
  pGUI_Choice_SelCam->align( FL_ALIGN_TOP | FL_ALIGN_LEFT);
  pGUI_Choice_SelCam->labelsize( 10);
  Last_Choice_SelCam_n = -2;      // Reset last size of pGUI_Choice_SelCam

  x1 += xx + 5;

  // Open camera Setting dialog

  xx = xx0 / 2 - 1;

  pGUI_CamRefresh = new Fl_Button( x1, y, xx, xx, "@-2refresh");
  pGUI_CamRefresh->callback( IqeB_Camera_Rescan_Callback, NULL);
  pGUI_CamRefresh->tooltip( LangStringLookup( "&GUI_Camera_Tools6a=Search for cameras"));
  pGUI_CamRefresh->labelcolor( YAIPS_BCOL_BUTTON);

  pGUI_CamSetting = new Fl_Button( x1 + xx + 2, y, xx, xx, "@-2camera");
  pGUI_CamSetting->callback( IqeB_Camera_Settings_Callback, NULL);
  pGUI_CamSetting->tooltip( LangStringLookup( "&GUI_Camera_Tools7b=Camera properties"));
  pGUI_CamSetting->labelcolor( YAIPS_BCOL_BUTTON);

  pGUI_Parameter = new Fl_Button( x1, y + xx + 2, xx, xx, "@-4menu2");
  pGUI_Parameter->callback( IqeB_Camera_GUI_Callback, NULL);
  pGUI_Parameter->tooltip( LANGDEF_SETTINGS_POINTS);
  pGUI_Parameter->labelcolor( YAIPS_BCOL_BUTTON);
  pGUI_Parameter->shortcut( FL_COMMAND+'p');       // Short cut key

  pGUI_TeachToggle = new Fl_Button( x1 + xx + 2, y + xx + 2, xx, xx, "@-2pencil");
  pGUI_TeachToggle->callback( IqeB_Camera_GUI_Callback, NULL);
  pGUI_TeachToggle->tooltip( LANGDEF_SWITCH_TEACH_INSPECT);
  pGUI_TeachToggle->labelcolor( YAIPS_BCOL_BUTTON);
  pGUI_TeachToggle->shortcut( FL_COMMAND+'t');       // Short cut key

  // Copy image to clipboard button
  // NOTE: we place the button outside the window.
  //       This makes the button invisible.
  //       The shortcut still can be used.

  x1 += xx + 5;

  pTemp_Button = new Fl_Button( x1, y - 100, xx, yy, "Copy");
  pTemp_Button->callback( YaIPS_ImageDispCopyImage_cb, &YaIPS_ImageDisp);
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

  YaIPS_ImageDisp.DisplayResolution_Last = YAIPS_DISP_RESOLUTION_INVALID;  // Setup resolution change check

  YaIPS_ImageDisp.pImage_Box = new YaIPS_Fl_Box( ImageDisp_x, ImageDisp_y, ImageDisp_xx, ImageDisp_yy);
  YaIPS_ImageDisp.pImage_Box->box(FL_DOWN_BOX);
  YaIPS_ImageDisp.pImage_Box->align( FL_ALIGN_CLIP);
  YaIPS_ImageDisp.pImage_Box->color( YaIPS_Color_IMG_BGND);  // Background color

  YaIPS_ImageDisp.pImage_Box->pDrawBeforeCallback = YaIPS_ImageDispDrawBefore_cb;  // Draw before callback
  YaIPS_ImageDisp.pImage_Box->pDrawAfterCallback  = YaIPS_GUI_MyDrawAfter_cb;      // Draw after callback
  YaIPS_ImageDisp.pImage_Box->DrawCallbackArg1    = &YaIPS_ImageDisp;              // Argument for draw before/draw after callbacks

  YaIPS_ImageDisp.pImage_Box->pMouseCallback      = YaIPS_GUI_MyMouse_cb;         // Mouse event callback
  YaIPS_ImageDisp.pImage_Box->MouseCallbackArg1   = &YaIPS_ImageDisp;             // Pointer to Fl_YaIPS_ImageDisp_t


  //
  // Layout end work
  //

  pMyToolWin->end();

  pMyToolWin->resizable( YaIPS_ImageDisp.pImage_Box);  // This window is resizable

  pMyToolWin->size_range( MYWIN_SIZE_X_MIN, MYWIN_SIZE_Y_MIN, MYWIN_SIZE_X_MAX, MYWIN_SIZE_Y_MAX); // minimum window size

  // finish up

  //x/IsNotNeeded/pMyToolWin->end();
  pMyToolWin->set_non_modal();
  pMyToolWin->callback( close_cb, &pMyToolWin);
  pMyToolWin->show();

  // Hack: Remove minimize and maximize buttons from the window caption
  YaIPS_DialogRemoveMinMaxButton( pMyToolWin);

  // Add idle action for this window

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::add_idle( IqeB_GUI_ToolsMyIdleAction);      // Redraw window during idle
#endif
  Fl::add_check( IqeB_GUI_ToolsMyIdleAction);     // Check small image size change

  // Setup cameras is delayed until all tool windows are started up

  StartupDone = false;                           // Flag start needed

  MyWinUpdate( false);                           // Update the GUI

  // Register draw after function for big image display
  YaIPS_ToolWinDrawAfterSet( MY_WIN_ID, YaIPS_GUI_MyDrawAfter_Other);
}

/************************************************************************************
 * IqeB_GUI_CameraImageRedraw
 *
 * Redraw camera image after any parameter modification
 */

// Redraw camera image
void IqeB_GUI_CameraImageRedraw()
{

  if( pMyToolWin == NULL) {                     // Security test

    return;
  }

  if( YaIPS_ImageDisp.pImage_Box == NULL) {      // Security test

    return;
  }

  YaIPS_ImageDisp.pImage_Box->redraw();          // Redraw image

  if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID &&  // Display also on big image
      YaIPS_BigImageDisp.pImage_Box != NULL) {          // and there is a display box definition

    YaIPS_BigImageDisp.pImage_Box->redraw();            // Redraw also
  }
}

/************************************************************************************
 * IqeB_GUI_CameraSnapAfterProperyChange
 *
 * Snap a single camera image after a camera property has changed.
 * The snap is done only for area camera mode and continuous acquisition is off.
 */

void IqeB_GUI_CameraSnapAfterPropertyChange()
{
  int i;
  unsigned long TimeStamp;

  if( pMyToolWin == NULL) {                     // Security test

    return;
  }

  if( YaIPS_ImageDisp.pImage_Box == NULL) {      // Security test

    return;
  }

  // Snap image from camera

  if( YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_AREA ||  // Area mode active
      (YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_LINE && YaIPS_CamPar_LMode_Adjust) ||
      (YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT && YaIPS_CamPar_HMode_Adjust)) {

    // OK, can make an acquisition
  } else {

    return;
  }

  if( Camera_DoGrab) {                // Continues grab is on

    return;
  }

  MyWinUpdate( false);                // Disable all GUI elements
  Fl::check();                        // give fltk some cpu to update the screen

  // Some cameras need more time until property change is active

  TimeStamp = GetTickCount();

  for( i = 0; i < 5; i++) {

    IqeB_GUI_SnapAndDisplay( false);   // Snap and display image

    if( GetTickCount() - TimeStamp >= 120) {   // Enough time gone
      break;
    }
  }

  MyWinUpdate( true);                 // Update the GUI
}

/************************* End Of File *************************/
