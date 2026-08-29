/****************************************************************************

  YaIPS_GUI_CamProperties.cpp

  Camera properties pages.

  22.04.2026 RR: First edition of this file.

*****************************************************************************
*/
//x/#define CAMERA_INIT_DEBUG _PRINTS   1   // define this to have camera init debug prints during _DEBUG

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
#define MY_WIN_ID     YAIPS_WIN_ID_PROPERTIES   // Source specific windows ID
#define MY_WIN_MAX    1                         // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_CamProperties_Title=Camera Properties"      // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "CamProperties"              // Name used for the preference data

/************************************************************************************
 * Externals
 *
 */

 // From camera dialog: If >= 0, number of cameras found, < 0: camera info table not initialized
extern int YaIPS_Camera_n;

// From camera dialog: If >= 0, selected camera, index in camera info table
extern int YaIPS_Cam_Info_Idx;

// From camera dialog: current selected camera.
extern char CamIdLastSelected[ CCAP_MAX_DEVICE_NAME_LENGTH];

/************************************************************************************
 * Globals
 *
 */

//
// Global setting
//

// Last selected property page
static int YaIPS_CamPropertPage;      // Is also number of last selected tab group.

// Data for one property
typedef struct {
  char *pName;                       // GUI Name for property
  long Property;                     // Property ID value
} YaIPS_Cam_Property_Element_t;

// Strings for amplifier properties

#ifndef VideoProcAmp_Powerline_Frequency        // Powerline frequency setting
#define VideoProcAmp_Powerline_Frequency 13

// Values for this setting:
// 0   The power-line frequency control is disabled.
// 1   The power-line frequency is 50 Hz.
// 2   The power-line frequency is 60 Hz.
#endif

#define CAM_PROP_AMP_N  11               // Number of amplifier properties

static YaIPS_Cam_Property_Element_t ProcAmpPropertyTable[] = {
    (char*)"&GUI_CamPropAmp1=Brightness",       VideoProcAmp_Brightness,             // 0
    (char*)"&GUI_CamPropAmp2=Contrast",         VideoProcAmp_Contrast,               // 1
    (char*)"&GUI_CamPropAmp3=Hue",              VideoProcAmp_Hue,                    // 2
    (char*)"&GUI_CamPropAmp4=Saturation",       VideoProcAmp_Saturation,             // 3
    (char*)"&GUI_CamPropAmp5=Sharpness",        VideoProcAmp_Sharpness,              // 4
    (char*)"&GUI_CamPropAmp6=Gamma",            VideoProcAmp_Gamma,                  // 5
    (char*)"&GUI_CamPropAmp7=WhiteBalance",     VideoProcAmp_WhiteBalance,           // 6
    (char*)"&GUI_CamPropAmp8=BacklightComp.",   VideoProcAmp_BacklightCompensation,  // 7
    (char*)"&GUI_CamPropAmp9=Gain",             VideoProcAmp_Gain,                   // 8
    (char*)"&GUI_CamPropAmp10=ColorEnable",     VideoProcAmp_ColorEnable,            // 9
    (char*)"&GUI_CamPropAmp11=Power Frq. [Hz]", VideoProcAmp_Powerline_Frequency,    // 10
};

// Strings for camera control properties

#define CAM_PROP_CTR_N   7               // Number of camera control properties

static YaIPS_Cam_Property_Element_t CameraControlPropertyTable[] = {
    (char*)"&GUI_CamPropCtr1=Zoom",     CameraControl_Zoom,      // 0
    (char*)"&GUI_CamPropCtr2=Focus",    CameraControl_Focus,     // 1
    (char*)"&GUI_CamPropCtr3=Exposure", CameraControl_Exposure,  // 2
    (char*)"&GUI_CamPropCtr4=Iris",     CameraControl_Iris,      // 3
    (char*)"&GUI_CamPropCtr5=Pan",      CameraControl_Pan,       // 4
    (char*)"&GUI_CamPropCtr6=Tilt",     CameraControl_Tilt,      // 5
    (char*)"&GUI_CamPropCtr7=Roll",     CameraControl_Roll,      // 6
};

// Data for one property
typedef struct {
  int  Valid;                                          // True if entry is valid
  long min, max, SteppingDelta, defaultValue;          // setup values
  long FlagsGetRang, FlagsGet;                         // Flag values
  long ValueOpen;                                      // Value at open of property dialog
  long ValueCurr;                                      // current value
  Fl_Box          *pBoxName;                           // Box used to display name of property
  Fl_Value_Slider *pValueSlider;                       // GUI value slider
  Fl_Check_Button *pCheckBox;                          // GUI check box
} YaIPS_Cam_Prop_Data_t;

// Properties for current open camera
static char Cam_Prop_CamName[ CCAP_MAX_DEVICE_NAME_LENGTH];            // Name of camera for this dialog
static YaIPS_Cam_Prop_Data_t ProcAmpPropertyies[ CAM_PROP_AMP_N];
static YaIPS_Cam_Prop_Data_t ProcCtrPropertyies[ CAM_PROP_CTR_N];

// Handles for camera properties
static IAMVideoProcAmp *pAMVideoProcAmp = NULL;
static IAMVideoProcAmp *pIAMCameraControl = NULL;

// Default settings button
static Fl_Button *pBut_AmpDefault;
static Fl_Button *pBut_CtrDefault;

/************************************************************************************
 * Statics
 */

static  Fl_Window *pMyToolWin;
static int IsOpen;                           // True if this window is open.
static int MyWinPosX = IQE_GUI_NO_WINPOS_X, MyWinPosY = IQE_GUI_NO_WINPOS_Y; // last window position

static IqeFl_Tabs      *pTab_Groups;         // Point to tabulator GUI element

/************************************************************************************
 * Presets for this tools window
 *
 */

static T_GUI_PreferenceEntry MyPreferences[] =
{

  // Dialog is open
  { PREF_T_INT,    "IsOpen",      "0", &IsOpen},

  // Hold last selected tab

  { PREF_T_INT, "LastPropertPage",      "0", &YaIPS_CamPropertPage},

};


// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&pMyToolWin), &MyWinPosX, &MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, 0,
                                               &IsOpen, YaIPS_GUI_CamPropertiesWin);

/************************************************************************************
 * Save camera settings for cameras
 *
 */

typedef struct {

  char CamName[ CCAP_MAX_DEVICE_NAME_LENGTH];  // name of camera

  unsigned int TimeStamp;              // Time stamp multiple of 7,15 minutes since 1.1.1601

  int AmpProperty[ CAM_PROP_AMP_N];    // Saved video amp property values
  int AmpCheckBox[ CAM_PROP_AMP_N];    // Saved video amp auto on values

  int CtrProperty[ CAM_PROP_CTR_N];    // Saved camera control property values
  int CtrCheckBox[ CAM_PROP_CTR_N];    // Saved camera control auto on values

} YaIPS_CamPropertiesSaved_t;

static YaIPS_CamPropertiesSaved_t YaIPS_CamPropertiesSaved[ YAIPS_WIN_MAX_CAM_PR_SAVE];

static T_GUI_PreferenceEntry CamPropertiesSaved[] =
{

  { PREF_T_STRING,       "CamName",   "", &YaIPS_CamPropertiesSaved[0].CamName, sizeof( YaIPS_CamPropertiesSaved[0].CamName) - 1 },

  { PREF_T_INT,         "TimeStamp",  "0", &YaIPS_CamPropertiesSaved[0].TimeStamp},

  // Saved video amp property values
  { PREF_T_INT,    "AmpProperty_00",  "0", YaIPS_CamPropertiesSaved[0].AmpProperty + 0},
  { PREF_T_INT,    "AmpProperty_01",  "0", YaIPS_CamPropertiesSaved[0].AmpProperty + 1},
  { PREF_T_INT,    "AmpProperty_02",  "0", YaIPS_CamPropertiesSaved[0].AmpProperty + 2},
  { PREF_T_INT,    "AmpProperty_03",  "0", YaIPS_CamPropertiesSaved[0].AmpProperty + 3},
  { PREF_T_INT,    "AmpProperty_04",  "0", YaIPS_CamPropertiesSaved[0].AmpProperty + 4},
  { PREF_T_INT,    "AmpProperty_05",  "0", YaIPS_CamPropertiesSaved[0].AmpProperty + 5},
  { PREF_T_INT,    "AmpProperty_06",  "0", YaIPS_CamPropertiesSaved[0].AmpProperty + 6},
  { PREF_T_INT,    "AmpProperty_07",  "0", YaIPS_CamPropertiesSaved[0].AmpProperty + 7},
  { PREF_T_INT,    "AmpProperty_08",  "0", YaIPS_CamPropertiesSaved[0].AmpProperty + 8},
  { PREF_T_INT,    "AmpProperty_09",  "0", YaIPS_CamPropertiesSaved[0].AmpProperty + 9},
  { PREF_T_INT,    "AmpProperty_10",  "0", YaIPS_CamPropertiesSaved[0].AmpProperty + 10},

  // Saved video amp auto on values
  { PREF_T_INT,    "AmpCheckBox_00",  "0", YaIPS_CamPropertiesSaved[0].AmpCheckBox + 0},
  { PREF_T_INT,    "AmpCheckBox_01",  "0", YaIPS_CamPropertiesSaved[0].AmpCheckBox + 1},
  { PREF_T_INT,    "AmpCheckBox_02",  "0", YaIPS_CamPropertiesSaved[0].AmpCheckBox + 2},
  { PREF_T_INT,    "AmpCheckBox_03",  "0", YaIPS_CamPropertiesSaved[0].AmpCheckBox + 3},
  { PREF_T_INT,    "AmpCheckBox_04",  "0", YaIPS_CamPropertiesSaved[0].AmpCheckBox + 4},
  { PREF_T_INT,    "AmpCheckBox_05",  "0", YaIPS_CamPropertiesSaved[0].AmpCheckBox + 5},
  { PREF_T_INT,    "AmpCheckBox_06",  "0", YaIPS_CamPropertiesSaved[0].AmpCheckBox + 6},
  { PREF_T_INT,    "AmpCheckBox_07",  "0", YaIPS_CamPropertiesSaved[0].AmpCheckBox + 7},
  { PREF_T_INT,    "AmpCheckBox_08",  "0", YaIPS_CamPropertiesSaved[0].AmpCheckBox + 8},
  { PREF_T_INT,    "AmpCheckBox_09",  "0", YaIPS_CamPropertiesSaved[0].AmpCheckBox + 9},
  { PREF_T_INT,    "AmpCheckBox_10",  "0", YaIPS_CamPropertiesSaved[0].AmpCheckBox + 10},

  // Saved camera control property values
  { PREF_T_INT,    "CtrProperty_00",  "0", YaIPS_CamPropertiesSaved[0].CtrProperty + 0},
  { PREF_T_INT,    "CtrProperty_01",  "0", YaIPS_CamPropertiesSaved[0].CtrProperty + 1},
  { PREF_T_INT,    "CtrProperty_02",  "0", YaIPS_CamPropertiesSaved[0].CtrProperty + 2},
  { PREF_T_INT,    "CtrProperty_03",  "0", YaIPS_CamPropertiesSaved[0].CtrProperty + 3},
  { PREF_T_INT,    "CtrProperty_04",  "0", YaIPS_CamPropertiesSaved[0].CtrProperty + 4},
  { PREF_T_INT,    "CtrProperty_05",  "0", YaIPS_CamPropertiesSaved[0].CtrProperty + 5},
  { PREF_T_INT,    "CtrProperty_06",  "0", YaIPS_CamPropertiesSaved[0].CtrProperty + 6},

  //Saved camera control auto on values
  { PREF_T_INT,    "CtrCheckBox_00",  "0", YaIPS_CamPropertiesSaved[0].CtrCheckBox + 0},
  { PREF_T_INT,    "CtrCheckBox_01",  "0", YaIPS_CamPropertiesSaved[0].CtrCheckBox + 1},
  { PREF_T_INT,    "CtrCheckBox_02",  "0", YaIPS_CamPropertiesSaved[0].CtrCheckBox + 2},
  { PREF_T_INT,    "CtrCheckBox_03",  "0", YaIPS_CamPropertiesSaved[0].CtrCheckBox + 3},
  { PREF_T_INT,    "CtrCheckBox_04",  "0", YaIPS_CamPropertiesSaved[0].CtrCheckBox + 4},
  { PREF_T_INT,    "CtrCheckBox_05",  "0", YaIPS_CamPropertiesSaved[0].CtrCheckBox + 5},
  { PREF_T_INT,    "CtrCheckBox_06",  "0", YaIPS_CamPropertiesSaved[0].CtrCheckBox + 6},
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyCamPropertiesAdd( "CamPropertiesSaved", CamPropertiesSaved, sizeof( CamPropertiesSaved) / sizeof( T_GUI_PreferenceEntry),
                                               NULL, NULL, NULL,
                                               YAIPS_WIN_ID_CAM_PR_SAVE, YAIPS_WIN_MAX_CAM_PR_SAVE, sizeof( YaIPS_CamPropertiesSaved_t));

/************************************************************************************
* YaIPS_CamPropSaved_EntryGet
* For a specific camera look up saved data.
*
* Return: != NULL  pointer to saved data
*         NULL     no entry found
*/

static YaIPS_CamPropertiesSaved_t *pYaIPS_CamPropSaved_EntryGet( char *pCamName)
{
  int i;
  YaIPS_CamPropertiesSaved_t *pCamPropSaved;

  pCamPropSaved = YaIPS_CamPropertiesSaved;

  for( i = 0; i < YAIPS_WIN_MAX_CAM_PR_SAVE; i++, pCamPropSaved++) {

    if( strcmp( pCamName, pCamPropSaved->CamName) == 0) {   // Got it

      return( pCamPropSaved);
    }
  }

  // Not found
  return( NULL);
}

/************************************************************************************
* YaIPS_CamPropSaved_EntryPut
* For a specific camera get an entry to save data
*
* Return: != NULL  pointer to saved data
*/

static YaIPS_CamPropertiesSaved_t *pYaIPS_CamPropSaved_EntryPut( char *pCamName)
{
  int i;
  unsigned int TimeOldest;
  YaIPS_CamPropertiesSaved_t *pCamPropSaved, *pCamPropOldest;

  // Check for existing entry

  pCamPropSaved = pYaIPS_CamPropSaved_EntryGet( pCamName);   // Look up existing entry

  if( pCamPropSaved != NULL) {                               // Got one

    return( pCamPropSaved);
  }

  // Check for empty entry

  pCamPropSaved = YaIPS_CamPropertiesSaved;

  for( i = 0; i < YAIPS_WIN_MAX_CAM_PR_SAVE; i++, pCamPropSaved++) {

    if( pCamPropSaved->CamName[ 0] == '\0') {   // Entry is empty

      return( pCamPropSaved);
    }
  }

  // All entries are used, locate oldest entry

  TimeOldest     = 0;
  pCamPropOldest = NULL;

  pCamPropSaved = YaIPS_CamPropertiesSaved;

  for( i = 0; i < YAIPS_WIN_MAX_CAM_PR_SAVE; i++, pCamPropSaved++) {

    if( i == 0) {                                          // First entry

      TimeOldest     = pCamPropSaved->TimeStamp;
      pCamPropOldest = pCamPropSaved;

    } else if( pCamPropSaved->TimeStamp < TimeOldest) {    // Other entry is older

      TimeOldest     = pCamPropSaved->TimeStamp;
      pCamPropOldest = pCamPropSaved;
    }
  }

  // Always we have an oldest entry
  return( pCamPropOldest);
}

/************************************************************************************
* YaIPS_CamPropSaved_Save
* Save the property data of current camera.
* This is always called after a camera property was changed.
*
*/

static void YaIPS_CamPropSaved_Save()
{
  YaIPS_CamPropertiesSaved_t *pCamPropSaved;

  // Check for existing entry

  pCamPropSaved = pYaIPS_CamPropSaved_EntryPut( Cam_Prop_CamName);   // Look up existing entry

  if( pCamPropSaved == NULL) {     // Error ?

    return;
  }

  // Clear all data

  memset( pCamPropSaved, 0, sizeof( YaIPS_CamPropertiesSaved_t));

  // Save camera name

  strncpy( pCamPropSaved->CamName, Cam_Prop_CamName, sizeof( pCamPropSaved->CamName) - 1);

  // time stamp

  FILETIME ft;
  GetSystemTimeAsFileTime( &ft);

  pCamPropSaved->TimeStamp = ft.dwHighDateTime;  // High part is multiply of 7,15 minutes

  // Store video amp data

  for( int iProperty = 0; iProperty < CAM_PROP_AMP_N; iProperty++) {

    pCamPropSaved->AmpProperty[ iProperty] = ProcAmpPropertyies[ iProperty].ValueCurr;

    if( ProcAmpPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual) &&  // Can switch auto ?
        ProcAmpPropertyies[ iProperty].FlagsGet == CameraControl_Flags_Auto) {                                     // and auto is on

      pCamPropSaved->AmpCheckBox[ iProperty] = true;
    }
  }

  for( int iProperty = 0; iProperty < CAM_PROP_CTR_N; iProperty++) {

    pCamPropSaved->CtrProperty[ iProperty] = ProcCtrPropertyies[ iProperty].ValueCurr;

    if( ProcCtrPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual) &&  // Can switch auto ?
        ProcCtrPropertyies[ iProperty].FlagsGet == CameraControl_Flags_Auto) {                                     // and auto is on

      pCamPropSaved->CtrCheckBox[ iProperty] = true;
    }
  }
}

/************************************************************************************
* Get data for current open camera
*/

static int YaIPS_CamProp_Init( char *pCamName)
{
  ICreateDevEnum *pDevEnum = NULL;
  IEnumMoniker *pEnum = NULL;
  IMoniker *pMoniker = NULL;
  IBaseFilter *pFilter = NULL;
  int ierr;
  int GotCurrentCamera;
  HRESULT hr;
  //x/HWND hwndParent;
  int nCameras;
  static int CoInitializeDone = false;

  // Initializes the COM library on the current thread and identifies
  // the concurrency model as single-thread apartment (STA).
  // ==> Is needed for use of direct show functions.
  // ==> Is never undone (call of CoUninitialize()).
  //     Is undone at end of application.

  if( ! CoInitializeDone) {                // If not called until now

    CoInitializeDone = true;               // Initalize now

    CoInitialize( NULL);
  }

  // Ensure handles for camera properties are released

  if( pAMVideoProcAmp != NULL) {

    pAMVideoProcAmp->Release();
    pAMVideoProcAmp = NULL;
  }

  if( pIAMCameraControl != NULL) {

    pIAMCameraControl->Release();
    pIAMCameraControl = NULL;
  }

  // Reset name of last displayed camera
  memset( Cam_Prop_CamName, 0, sizeof( Cam_Prop_CamName));

  // Camera dialog must be open
  ierr = YaIPS_ToolWinIsOpen( YAIPS_WIN_ID_CAMERA);

  if( ierr <= 0) {               // Camera is not open

    ierr = -1;
    goto ErrorExit;
  }

  // Security test
  if( YaIPS_Camera_n < 1 ||      // Have no camera
      YaIPS_Cam_Info_Idx < 0) {  // or no camera selected

    ierr = -2;
    goto ErrorExit;
  }

  if( pCamName == NULL || pCamName[ 0] == '\0') {   // No camera name given

    ierr = -3;
    goto ErrorExit;
  }

  // Try to find direct draw camera

  // Enumerate video input devices

  hr = CoCreateInstance( CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                         IID_ICreateDevEnum, (void**)&pDevEnum);
  if (FAILED(hr)) {

    ierr = -10;
    goto ErrorExit;
  }

  hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);
  if (FAILED(hr) || !pEnum) {

    ierr = -11;
    goto ErrorExit;
  }

  // Enumerate devices and check for current selected camera

  GotCurrentCamera = false;

  while( pEnum->Next(1, &pMoniker, nullptr) == S_OK) {

    IPropertyBag* pPropBag;
    hr = pMoniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void**)&pPropBag);

    if (SUCCEEDED(hr)) {

      VARIANT varName;
      VariantInit(&varName);
      char TempString[ CCAP_MAX_DEVICE_NAME_LENGTH];

      nCameras += 1;

      // Get friendlyName
      hr = pPropBag->Read(L"FriendlyName", &varName, nullptr);
      if(SUCCEEDED(hr)) {

        int size_needed = WideCharToMultiByte(CP_UTF8, 0, varName.bstrVal, -1,
                                              nullptr, 0, nullptr, nullptr);

        WideCharToMultiByte( CP_UTF8, 0, varName.bstrVal, -1,
                             TempString, size_needed, nullptr, nullptr);

        // Is this the current selected camera =
        if( stricmp( TempString, pCamName) == 0) {

          GotCurrentCamera = true;
        }
      }

      VariantClear(&varName);
      pPropBag->Release();

      if( GotCurrentCamera) {
        break;
      }
    }
  }

  // Current selected camera not found
  if( ! GotCurrentCamera) {

    ierr = -20;
    goto ErrorExit;
  }

  hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pFilter);
  if (FAILED(hr)) {

    ierr = -21;
    goto ErrorExit;
  }

  int Valid;
  long min, max, SteppingDelta, defaultValue, FlagsGetRang, FlagsGet, currentValue;

  hr = pFilter->QueryInterface( IID_IAMVideoProcAmp, (void**)&pAMVideoProcAmp);
  if( FAILED(hr) || !pAMVideoProcAmp) {

    ierr = -22;
    goto ErrorExit;
  }

#ifdef CAMERA_INIT_DEBUG
#ifdef _DEBUG
  printf( "--------------------------------------------\n");
  printf( "VideoProcAmp: %s\n", pCamName);
#endif
#endif
  for( int iProperty = 0; iProperty < CAM_PROP_AMP_N; iProperty++) {

    Valid = true;             // Preset entry is valid
    min = 0;
    max = 100;
    SteppingDelta = 1;
    defaultValue = 1;
    FlagsGetRang = 0x04;
    FlagsGet = 0x04;
    currentValue = 0;

    hr = pAMVideoProcAmp->GetRange( ProcAmpPropertyTable[ iProperty].Property, &min, &max, &SteppingDelta, &defaultValue, &FlagsGetRang);
    if( FAILED( hr)) {
#ifdef CAMERA_INIT_DEBUG
#ifdef _DEBUG
      printf( "%2d: %14s = ERROR GetRange()\n", iProperty, LangStringLookup( ProcAmpPropertyTable[ iProperty].pName));
#endif
#endif
      Valid = false;             // Entry is not valid
    } else {

      if( ProcAmpPropertyTable[ iProperty].Property == VideoProcAmp_Powerline_Frequency) { // Hack for power line frequency

        min = 50;              // 1 = 50 Hz
        max = 60;             // 2 = 60 Hz
        SteppingDelta = 10;
        if( defaultValue < 1) {
          defaultValue = 1;
        }
        if( defaultValue > 2) {
          defaultValue = 2;
        }
      }
    }

    if( Valid) {

      hr = pAMVideoProcAmp->Get( ProcAmpPropertyTable[ iProperty].Property, &currentValue, &FlagsGet);
      if( FAILED( hr)) {
#ifdef CAMERA_INIT_DEBUG
#ifdef _DEBUG
        printf( "%2d: %14s = ERROR Get()\n", iProperty, LangStringLookup( ProcAmpPropertyTable[ iProperty].pName));
#endif
#endif
        Valid = false;             // Entry is not valid

      } else {

        // If set to auto current value is max value --> set to default value
        if( FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual) &&  // Can switch auto ?
            FlagsGet == CameraControl_Flags_Auto) {                                     // and auto is on

          currentValue = defaultValue;
        }
      }

      if( ProcAmpPropertyTable[ iProperty].Property == VideoProcAmp_Powerline_Frequency) { // Hack for power line frequency

        if( currentValue < 1) {
          currentValue = 1;
        }
        if( currentValue > 2) {
          currentValue = 2;
        }
      }
    }

#ifdef CAMERA_INIT_DEBUG
#ifdef _DEBUG
    if( Valid) {

      printf( "%2d: %14s = %5ld, %5ld .. %5ld, Stepping: %3ld Def:%5ld Flags:0x%2lx 0x%2lx\n",
                iProperty, LangStringLookup( ProcAmpPropertyTable[ iProperty].pName), currentValue, min, max, SteppingDelta, defaultValue, FlagsGetRang, FlagsGet);
    }
#endif
#endif

    ProcAmpPropertyies[ iProperty].Valid = Valid;
    ProcAmpPropertyies[ iProperty].min = min;
    ProcAmpPropertyies[ iProperty].max = max;
    ProcAmpPropertyies[ iProperty].SteppingDelta = SteppingDelta;
    ProcAmpPropertyies[ iProperty].defaultValue = defaultValue;
    ProcAmpPropertyies[ iProperty].FlagsGetRang = FlagsGetRang;
    ProcAmpPropertyies[ iProperty].FlagsGet  = FlagsGet;
    ProcAmpPropertyies[ iProperty].ValueOpen = currentValue;
    ProcAmpPropertyies[ iProperty].ValueCurr = currentValue;
  }

  if( ierr < 0) {            // Error ?

    goto ErrorExit;
  }

  hr = pFilter->QueryInterface( IID_IAMCameraControl, (void**)&pIAMCameraControl);
  if( FAILED(hr) || !pIAMCameraControl) {

    ierr = -23;
    goto ErrorExit;
  }

#ifdef CAMERA_INIT_DEBUG
#ifdef _DEBUG
  printf( "--------------------------------------------\n");
  printf( "CameraControl: %s\n", pCamName);
#endif
#endif

  for( int iProperty = 0; iProperty < CAM_PROP_CTR_N; iProperty++) {

    Valid = true;             // Preset entry is valid
    min = 0;
    max = 100;
    SteppingDelta = 1;
    defaultValue = 1;
    FlagsGetRang = 0x04;
    FlagsGet = 0x04;
    currentValue = 0;

    hr = pIAMCameraControl->GetRange( CameraControlPropertyTable[ iProperty].Property, &min, &max, &SteppingDelta, &defaultValue, &FlagsGetRang);
    if( FAILED( hr)) {
#ifdef CAMERA_INIT_DEBUG
#ifdef _DEBUG
      printf( "%2d: %14s = ERROR GetRange()\n", iProperty, LangStringLookup( CameraControlPropertyTable[ iProperty].pName));
#endif
#endif
      Valid = false;             // Entry is not valid
    }

    if( Valid) {

      hr = pIAMCameraControl->Get( CameraControlPropertyTable[ iProperty].Property, &currentValue, &FlagsGet);
      if( FAILED( hr)) {
#ifdef CAMERA_INIT_DEBUG
#ifdef _DEBUG
        printf( "%2d: %14s = ERROR Get()\n", iProperty, LangStringLookup( CameraControlPropertyTable[ iProperty].pName));
#endif
#endif
        Valid = false;             // Entry is not valid

      } else {

        // If set to auto current value is max value --> set to default value
        if( FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual) &&  // Can switch auto ?
            FlagsGet == CameraControl_Flags_Auto) {                                     // and auto is on

          currentValue = defaultValue;
        }
      }
    }

#ifdef CAMERA_INIT_DEBUG
#ifdef _DEBUG
    if( Valid) {

      printf( "%2d: %14s = %5ld, %5ld .. %5ld, Stepping: %3ld Def:%5ld Flags:0x%2lx 0x%2lx\n",
                iProperty, LangStringLookup( CameraControlPropertyTable[ iProperty].pName), currentValue, min, max, SteppingDelta, defaultValue, FlagsGetRang, FlagsGet);
    }
#endif
#endif

    ProcCtrPropertyies[ iProperty].Valid = Valid;
    ProcCtrPropertyies[ iProperty].min = min;
    ProcCtrPropertyies[ iProperty].max = max;
    ProcCtrPropertyies[ iProperty].SteppingDelta = SteppingDelta;
    ProcCtrPropertyies[ iProperty].defaultValue = defaultValue;
    ProcCtrPropertyies[ iProperty].FlagsGetRang = FlagsGetRang;
    ProcCtrPropertyies[ iProperty].FlagsGet  = FlagsGet;
    ProcCtrPropertyies[ iProperty].ValueOpen = currentValue;
    ProcCtrPropertyies[ iProperty].ValueCurr = currentValue;
  }

  if( ierr < 0) {            // Error =

    goto ErrorExit;
  }

  // Remember Name of camera

  strncpy( Cam_Prop_CamName, pCamName, sizeof( Cam_Prop_CamName) - 1);

  ierr = 0;         // Return OK

  // Error exit

ErrorExit:

  if( ierr != 0) {              // Have any error

    // Release handles for camera properties

    if( pAMVideoProcAmp != NULL) {

      pAMVideoProcAmp->Release();
      pAMVideoProcAmp = NULL;
    }

    if( pIAMCameraControl != NULL) {

      pIAMCameraControl->Release();
      pIAMCameraControl = NULL;
    }
  }

  if( pFilter != NULL) {

    pFilter->Release();
    pFilter = NULL;
  }

  if( pMoniker != NULL) {

    pMoniker->Release();
    pMoniker = NULL;
  }

  if( pEnum != NULL) {

    pEnum->Release();
    pEnum = NULL;
  }

  if( pDevEnum != NULL) {

    pDevEnum->Release();
    pDevEnum= NULL;
  }

  // GUI elements

  YaIPS_CamPropertiesSaved_t *pCamPropSaved;

  pCamPropSaved = pYaIPS_CamPropSaved_EntryGet( pCamName);   // Look up saved camera properties

  for( int iProperty = 0; iProperty < CAM_PROP_AMP_N; iProperty++) {

    if( ierr == 0 && ProcAmpPropertyies[ iProperty].Valid) {     // Return OK

      if( pCamPropSaved != NULL) {                               // Have saved data

        ProcAmpPropertyies[ iProperty].ValueCurr = pCamPropSaved->AmpProperty[ iProperty];

        if( ProcAmpPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual)) {  // Can switch auto ?

          if( pCamPropSaved->AmpCheckBox[ iProperty] != 0) {

            ProcAmpPropertyies[ iProperty].FlagsGet = CameraControl_Flags_Auto;
          } else {

            ProcAmpPropertyies[ iProperty].FlagsGet = CameraControl_Flags_Manual;
          }
        }

        pAMVideoProcAmp->Set( ProcAmpPropertyTable[ iProperty].Property, ProcAmpPropertyies[ iProperty].ValueCurr, ProcAmpPropertyies[ iProperty].FlagsGet);
      }

      if( pMyToolWin == NULL) {      // GUI is not open

        continue;
      }

      ProcAmpPropertyies[ iProperty].pValueSlider->bounds( ProcAmpPropertyies[ iProperty].min, ProcAmpPropertyies[ iProperty].max);
      ProcAmpPropertyies[ iProperty].pValueSlider->step( (int)ProcAmpPropertyies[ iProperty].SteppingDelta);

      if( ProcAmpPropertyTable[ iProperty].Property == VideoProcAmp_Powerline_Frequency) { // Hack for power line frequency

        ProcAmpPropertyies[ iProperty].pValueSlider->value( ProcAmpPropertyies[ iProperty].ValueCurr <= 1 ? 50 : 60);

      } else {

        ProcAmpPropertyies[ iProperty].pValueSlider->value( ProcAmpPropertyies[ iProperty].ValueCurr);
      }

      IqeB_GUI_WidgetActivate( ProcAmpPropertyies[ iProperty].pValueSlider, true);

      if( ProcAmpPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual)) {  // Can switch auto ?

        ProcAmpPropertyies[ iProperty].pCheckBox->value( ProcAmpPropertyies[ iProperty].FlagsGet == CameraControl_Flags_Auto);
        IqeB_GUI_WidgetActivate( ProcAmpPropertyies[ iProperty].pCheckBox, true);
      } else {
        ProcAmpPropertyies[ iProperty].pCheckBox->value( 0);
        IqeB_GUI_WidgetActivate( ProcAmpPropertyies[ iProperty].pCheckBox, false);
      }

      if( ProcAmpPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual) &&  // Can switch auto
          ProcAmpPropertyies[ iProperty].FlagsGet == CameraControl_Flags_Auto) {                                     // ... and is set to auto

        // Deactivate slider
        ProcAmpPropertyies[ iProperty].pValueSlider->deactivate();
      }

      IqeB_GUI_WidgetActivate( ProcAmpPropertyies[ iProperty].pBoxName, true);

    } else {             // Error exit

      if( pMyToolWin == NULL) {      // GUI is not open

        continue;
      }

      ProcAmpPropertyies[ iProperty].pValueSlider->bounds( 0, 100);
      ProcAmpPropertyies[ iProperty].pValueSlider->step( 1);
      ProcAmpPropertyies[ iProperty].pValueSlider->value( 0);
      IqeB_GUI_WidgetActivate( ProcAmpPropertyies[ iProperty].pValueSlider, false);

      ProcAmpPropertyies[ iProperty].pCheckBox->value( 0);
      IqeB_GUI_WidgetActivate( ProcAmpPropertyies[ iProperty].pCheckBox, false);

      IqeB_GUI_WidgetActivate( ProcAmpPropertyies[ iProperty].pBoxName, false);
    }
  }

  for( int iProperty = 0; iProperty < CAM_PROP_CTR_N; iProperty++) {

    if( ierr == 0 && ProcCtrPropertyies[ iProperty].Valid) {     // Return OK

      if( pCamPropSaved != NULL) {                               // Have saved data

        ProcCtrPropertyies[ iProperty].ValueCurr = pCamPropSaved->CtrProperty[ iProperty];

        if( ProcCtrPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual)) {  // Can switch auto ?

          if( pCamPropSaved->CtrCheckBox[ iProperty] != 0) {

            ProcCtrPropertyies[ iProperty].FlagsGet = CameraControl_Flags_Auto;
          } else {

            ProcCtrPropertyies[ iProperty].FlagsGet = CameraControl_Flags_Manual;
          }
        }

        pIAMCameraControl->Set( CameraControlPropertyTable[ iProperty].Property, ProcCtrPropertyies[ iProperty].ValueCurr, ProcCtrPropertyies[ iProperty].FlagsGet);
      }

      if( pMyToolWin == NULL) {      // GUI is not open

        continue;
      }

      ProcCtrPropertyies[ iProperty].pValueSlider->bounds( ProcCtrPropertyies[ iProperty].min, ProcCtrPropertyies[ iProperty].max);
      ProcCtrPropertyies[ iProperty].pValueSlider->step( (int)ProcCtrPropertyies[ iProperty].SteppingDelta);
      ProcCtrPropertyies[ iProperty].pValueSlider->value( ProcCtrPropertyies[ iProperty].ValueCurr);
      IqeB_GUI_WidgetActivate( ProcCtrPropertyies[ iProperty].pValueSlider, true);

      if( ProcCtrPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual)) {  // Can switch auto ?

        ProcCtrPropertyies[ iProperty].pCheckBox->value( ProcCtrPropertyies[ iProperty].FlagsGet == CameraControl_Flags_Auto);
        IqeB_GUI_WidgetActivate( ProcCtrPropertyies[ iProperty].pCheckBox, true);
      } else {
        ProcCtrPropertyies[ iProperty].pCheckBox->value( 0);
        IqeB_GUI_WidgetActivate( ProcCtrPropertyies[ iProperty].pCheckBox, false);
      }

      if( ProcCtrPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual) &&  // Can switch auto
          ProcCtrPropertyies[ iProperty].FlagsGet == CameraControl_Flags_Auto) {                                     // ... and is set to auto

        // Deactivate slider
        ProcCtrPropertyies[ iProperty].pValueSlider->deactivate();
      }

      IqeB_GUI_WidgetActivate( ProcCtrPropertyies[ iProperty].pBoxName, true);

    } else {             // Error exit

      if( pMyToolWin == NULL) {      // GUI is not open

        continue;
      }

      ProcCtrPropertyies[ iProperty].pValueSlider->bounds( 0, 100);
      ProcCtrPropertyies[ iProperty].pValueSlider->step( 1);
      ProcCtrPropertyies[ iProperty].pValueSlider->value( 0);
      IqeB_GUI_WidgetActivate( ProcCtrPropertyies[ iProperty].pValueSlider, false);

      ProcCtrPropertyies[ iProperty].pCheckBox->value( 0);
      IqeB_GUI_WidgetActivate( ProcCtrPropertyies[ iProperty].pCheckBox, false);

      IqeB_GUI_WidgetActivate( ProcCtrPropertyies[ iProperty].pBoxName, false);
    }
  }

  return( ierr);
}

/************************************************************************************
 * YaIPS_CamProp_SnapAfterPropertyChange
 *
 * Snap a single camera image after a camera property has changed.
 * The snap is done only for area camera mode and continuous acquisition is off.
 */

static void YaIPS_CamProp_SnapAfterPropertyChange()
{
  int ierr;

  // Camera dialog must be open
  ierr = YaIPS_ToolWinIsOpen( YAIPS_WIN_ID_CAMERA);

  if( ierr <= 0) {               // Camera is not open

    return;
  }

  IqeB_GUI_CameraSnapAfterPropertyChange();
}

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyWinUpdate()
{
  int iProperty, Enable;
  long Value, Flags;

  // Remember last selected tabulator group

  YaIPS_CamPropertPage = pTab_Groups->GetTabGroup();

  // Check for video amp default button enable

  Enable = false;                                    // Disable default button

  if( pAMVideoProcAmp != NULL) {                     // Video amp is usable

    for( iProperty = 0; iProperty < CAM_PROP_AMP_N; iProperty++) {

      if( ProcAmpPropertyies[ iProperty].Valid == false) {            // Entry is not valid

        continue;
      }

      Value = ProcAmpPropertyies[ iProperty].defaultValue;

      Flags = ProcAmpPropertyies[ iProperty].FlagsGet;

      if( ProcAmpPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual)) {  // Can switch auto ?

        Flags = CameraControl_Flags_Auto;    // Force auto
      }

      // Is not the default setting ?

      if( ProcAmpPropertyies[ iProperty].ValueCurr != Value ||    // Is NOT the default setting
          ProcAmpPropertyies[ iProperty].FlagsGet  != Flags) {    // or flags are different

        Enable = true;                                    // Enable default button
        break;
      }
    }
  }

  IqeB_GUI_WidgetActivate( pBut_AmpDefault, Enable);     // Enable GUI elements

  // Check for camera control default button enable

  Enable = false;                                    // Disable default button

  if( pIAMCameraControl != NULL) {                     // Video amp is usable

    for( iProperty = 0; iProperty < CAM_PROP_CTR_N; iProperty++) {

      if( ProcCtrPropertyies[ iProperty].Valid == false) {            // Entry is not valid

        continue;
      }

      Value = ProcCtrPropertyies[ iProperty].defaultValue;

      Flags = ProcCtrPropertyies[ iProperty].FlagsGet;

      if( ProcCtrPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual)) {  // Can switch auto ?

        Flags = CameraControl_Flags_Auto;    // Force auto
      }

      // Is not the default setting ?

      if( ProcCtrPropertyies[ iProperty].ValueCurr != Value ||    // Is NOT the default setting
          ProcCtrPropertyies[ iProperty].FlagsGet  != Flags) {    // or flags are different

        Enable = true;                                    // Enable default button
        break;
      }
    }
  }

  IqeB_GUI_WidgetActivate( pBut_CtrDefault, Enable);     // Enable GUI elements
}


/************************************************************************************
 * YaIPS_CamProp_AmpSlider_Callback
 *
 * Callback for video amp property slider
 */

static void YaIPS_CamProp_AmpSlider_Callback( Fl_Widget *w, void *pValueArg)
{
  Fl_Value_Slider *pTemp_ValSlider;
  int iProperty;
  long Value;

  if( pAMVideoProcAmp == NULL) {  // Security test

    return;
  }

  pTemp_ValSlider = (Fl_Value_Slider *)w;

  Value = lround( pTemp_ValSlider->value());

  iProperty = (int)(long long)pValueArg;                // What property

  if( ProcAmpPropertyies[ iProperty].Valid == false) {  // Security test, entry is not valid

    return;
  }

  if( ProcAmpPropertyTable[ iProperty].Property == VideoProcAmp_Powerline_Frequency) { // Hack for power line frequency

    if( Value <= 50) {
      Value = 1;            // Convert to 50 Hz
    } else {
      Value = 2;            // Convert to 68 Hz
    }
  }

  ProcAmpPropertyies[ iProperty].ValueCurr = Value;

  pAMVideoProcAmp->Set( ProcAmpPropertyTable[ iProperty].Property, Value, ProcAmpPropertyies[ iProperty].FlagsGet);

  // Save properties of camera
  YaIPS_CamPropSaved_Save();

  // Snap a single camera image after a camera property has changed.
  YaIPS_CamProp_SnapAfterPropertyChange();
}

/************************************************************************************
 * YaIPS_CamProp_CtrSlider_Callback
 *
 * Callback for camera control property slider
 */

static void YaIPS_CamProp_CtrSlider_Callback( Fl_Widget *w, void *pValueArg)
{
  Fl_Value_Slider *pTemp_ValSlider;
  int iProperty;
  long Value;

  if( pIAMCameraControl == NULL) {  // Security test

    return;
  }

  pTemp_ValSlider = (Fl_Value_Slider *)w;

  Value = lround( pTemp_ValSlider->value());

  iProperty = (int)(long long)pValueArg;            // What property

  if( ProcCtrPropertyies[ iProperty].Valid == false) {  // Security test, entry is not valid

    return;
  }

  ProcCtrPropertyies[ iProperty].ValueCurr = Value;

  pIAMCameraControl->Set( CameraControlPropertyTable[ iProperty].Property, Value, ProcCtrPropertyies[ iProperty].FlagsGet);

  // Save properties of camera
  YaIPS_CamPropSaved_Save();

  // Snap a single camera image after a camera property has changed.
  YaIPS_CamProp_SnapAfterPropertyChange();
}

/************************************************************************************
 * YaIPS_CamProp_AmpCheck_Callback
 *
 * Callback for video amp property check box
 */

static void YaIPS_CamProp_AmpCheck_Callback( Fl_Widget *w, void *pValueArg)
{
  Fl_Check_Button *pTemp_Check_Button;
  int iProperty;
  long Value;

  if( pAMVideoProcAmp == NULL) {  // Security test

    return;
  }

  pTemp_Check_Button = (Fl_Check_Button *)w;

  Value = lround( pTemp_Check_Button->value());

  iProperty = (int)(long long)pValueArg;            // What property

  if( ProcAmpPropertyies[ iProperty].Valid == false) {  // Security test, entry is not valid

    return;
  }

  if( ProcAmpPropertyies[ iProperty].FlagsGetRang != (CameraControl_Flags_Auto | CameraControl_Flags_Manual)) {  // Can not switch auto ?

    return;
  }

  // If auto is on, disable value slider
  if( Value) {
    ProcAmpPropertyies[ iProperty].pValueSlider->deactivate();
  } else {
    ProcAmpPropertyies[ iProperty].pValueSlider->activate();
  }

  ProcAmpPropertyies[ iProperty].FlagsGet = Value ? CameraControl_Flags_Auto : CameraControl_Flags_Manual;

  pAMVideoProcAmp->Set( ProcAmpPropertyTable[ iProperty].Property, ProcAmpPropertyies[ iProperty].ValueCurr, ProcAmpPropertyies[ iProperty].FlagsGet);

  // Save properties of camera
  YaIPS_CamPropSaved_Save();

  // Snap a single camera image after a camera property has changed.
  YaIPS_CamProp_SnapAfterPropertyChange();
}

/************************************************************************************
 * YaIPS_CamProp_CtrCheck_Callback
 *
 * Callback for camera control property check box
 */

static void YaIPS_CamProp_CtrCheck_Callback( Fl_Widget *w, void *pValueArg)
{
  Fl_Check_Button *pTemp_Check_Button;
  int iProperty;
  long Value;

  if( pIAMCameraControl == NULL) {  // Security test

    return;
  }

  pTemp_Check_Button = (Fl_Check_Button *)w;

  Value = lround( pTemp_Check_Button->value());

  iProperty = (int)(long long)pValueArg;            // What property

  if( ProcCtrPropertyies[ iProperty].Valid == false) {  // Security test, entry is not valid

    return;
  }

  if( ProcCtrPropertyies[ iProperty].FlagsGetRang != (CameraControl_Flags_Auto | CameraControl_Flags_Manual)) {  // Can not switch auto ?

    return;
  }

  // If auto is on, disable value slider
  if( Value) {
    ProcCtrPropertyies[ iProperty].pValueSlider->deactivate();
  } else {
    ProcCtrPropertyies[ iProperty].pValueSlider->activate();
  }

  ProcCtrPropertyies[ iProperty].FlagsGet = Value ? CameraControl_Flags_Auto : CameraControl_Flags_Manual;

  pIAMCameraControl->Set( CameraControlPropertyTable[ iProperty].Property, ProcCtrPropertyies[ iProperty].ValueCurr, ProcCtrPropertyies[ iProperty].FlagsGet);

  // Save properties of camera
  YaIPS_CamPropSaved_Save();

  // Snap a single camera image after a camera property has changed.
  YaIPS_CamProp_SnapAfterPropertyChange();
}

/************************************************************************************
 * YaIPS_CamProp_AmpDefault_Callback
 *
 * Set default values for the video amp properties
 */

static void YaIPS_CamProp_AmpDefault_Callback( Fl_Widget *w, void *pValueArg)
{
  int iProperty;
  long Value, Flags;

  if( pAMVideoProcAmp == NULL) {  // Security test

    return;
  }

  for( iProperty = 0; iProperty < CAM_PROP_AMP_N; iProperty++) {

    if( ProcAmpPropertyies[ iProperty].Valid == false) {            // Entry is not valid

      continue;
    }

    Value = ProcAmpPropertyies[ iProperty].defaultValue;

    Flags = ProcAmpPropertyies[ iProperty].FlagsGet;

    if( ProcAmpPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual)) {  // Can switch auto ?

      Flags = CameraControl_Flags_Auto;    // Force auto
    }

    // Is not the default setting ?

    if( ProcAmpPropertyies[ iProperty].ValueCurr != Value ||    // Is NOT the default setting
        ProcAmpPropertyies[ iProperty].FlagsGet  != Flags) {    // or flags are different

      // Apply default value to camera

      ProcAmpPropertyies[ iProperty].ValueCurr = Value;
      ProcAmpPropertyies[ iProperty].FlagsGet  = Flags;

      pAMVideoProcAmp->Set( ProcAmpPropertyTable[ iProperty].Property, Value, Flags);

      // Reflect default settings on the GUI

      if( ProcAmpPropertyTable[ iProperty].Property == VideoProcAmp_Powerline_Frequency) { // Hack for power line frequency

        ProcAmpPropertyies[ iProperty].pValueSlider->value( ProcAmpPropertyies[ iProperty].ValueCurr <= 1 ? 50 : 60);

      } else {

        ProcAmpPropertyies[ iProperty].pValueSlider->value( ProcAmpPropertyies[ iProperty].ValueCurr);
      }

      if( ProcAmpPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual)) {  // Can switch auto ?

        ProcAmpPropertyies[ iProperty].pCheckBox->value( ProcAmpPropertyies[ iProperty].FlagsGet == CameraControl_Flags_Auto);

        // If auto is on, disable value slider
        if( ProcAmpPropertyies[ iProperty].FlagsGet == CameraControl_Flags_Auto) {
          ProcAmpPropertyies[ iProperty].pValueSlider->deactivate();
        } else {
          ProcAmpPropertyies[ iProperty].pValueSlider->activate();
        }
      }
    }
  }

  // Save properties of camera
  YaIPS_CamPropSaved_Save();

  // Snap a single camera image after a camera property has changed.
  YaIPS_CamProp_SnapAfterPropertyChange();
}

/************************************************************************************
 * YaIPS_CamProp_CtrDefault_Callback
 *
 * Set default values for the camera control properties
 */

static void YaIPS_CamProp_CtrDefault_Callback( Fl_Widget *w, void *pValueArg)
{
  int iProperty;
  long Value, Flags;

  if( pIAMCameraControl == NULL) {  // Security test

    return;
  }

  for( iProperty = 0; iProperty < CAM_PROP_CTR_N; iProperty++) {

    if( ProcCtrPropertyies[ iProperty].Valid == false) {            // Entry is not valid

      continue;
    }

    Value = ProcCtrPropertyies[ iProperty].defaultValue;

    Flags = ProcCtrPropertyies[ iProperty].FlagsGet;

    if( ProcCtrPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual)) {  // Can switch auto ?

      Flags = CameraControl_Flags_Auto;    // Force auto
    }

    // Is not the default setting ?

    if( ProcCtrPropertyies[ iProperty].ValueCurr != Value ||    // Is NOT the default setting
        ProcCtrPropertyies[ iProperty].FlagsGet  != Flags) {    // or flags are different

      // Apply default value to camera

      ProcCtrPropertyies[ iProperty].ValueCurr = Value;
      ProcCtrPropertyies[ iProperty].FlagsGet  = Flags;

      pIAMCameraControl->Set( CameraControlPropertyTable[ iProperty].Property, Value, Flags);

      // Reflect default settings on the GUI

      ProcCtrPropertyies[ iProperty].pValueSlider->value( ProcCtrPropertyies[ iProperty].ValueCurr);
      if( ProcCtrPropertyies[ iProperty].FlagsGetRang == (CameraControl_Flags_Auto | CameraControl_Flags_Manual)) {  // Can switch auto ?

        ProcCtrPropertyies[ iProperty].pCheckBox->value( ProcCtrPropertyies[ iProperty].FlagsGet == CameraControl_Flags_Auto);

        // If auto is on, disable value slider
        if( ProcCtrPropertyies[ iProperty].FlagsGet == CameraControl_Flags_Auto) {
          ProcCtrPropertyies[ iProperty].pValueSlider->deactivate();
        } else {
          ProcCtrPropertyies[ iProperty].pValueSlider->activate();
        }
      }
    }
  }

  // Save properties of camera
  YaIPS_CamPropSaved_Save();

  // Snap a single camera image after a camera property has changed.
  YaIPS_CamProp_SnapAfterPropertyChange();
}

/************************************************************************************
 * IqeB_GUI_ToolsMyIdleAction
 */

static void IqeB_GUI_ToolsMyIdleAction( void *)
{
  unsigned int TimeTemp;
  static unsigned int TimeLastCalled_100 = 0;

  TimeTemp = GetTickCount();           // Get current time

  //

  if( pMyToolWin == NULL) {     // Security test, window must exist

    return;
  }

  //
  // some timed actions (not each call)
  //

  if( TimeTemp - TimeLastCalled_100 >= 100) {  // 100 ms gone since last call

    TimeLastCalled_100 = TimeTemp;             // Remember last time called

    // some time gone, do ...

    // Check for camera change

    if( YaIPS_Camera_n >= 1 &&                 // Have any camera
        YaIPS_Cam_Info_Idx >= 0) {             // Camera is selected

      if( strcmp( Cam_Prop_CamName, CamIdLastSelected) != 0) {   // Has the camera changed

        // Change to new camera
        YaIPS_CamProp_Init( CamIdLastSelected);
      }

    } else {                                   // Have no camera

      if( Cam_Prop_CamName[ 0] != 0) {         // But GUI shows camea

        // Have no camera
        YaIPS_CamProp_Init( (char *)"");
      }
    }

    // Periodically updates on GUI

    MyWinUpdate();
  }

  return;
}

/************************************************************************************
 * close_cb, close this window
 *
 * pValueArg is a pointer to the widget. Set this pointer to NULL on deletion.
 */

static void close_cb( Fl_Widget *w, void *pValueArg)
{

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::remove_idle( IqeB_GUI_ToolsMyIdleAction);      // Redraw window during idle
#endif
  Fl::remove_check( IqeB_GUI_ToolsMyIdleAction);     // Check small image size change

  // Ensure handles for camera properties are released

  if( pAMVideoProcAmp != NULL) {

    pAMVideoProcAmp->Release();
    pAMVideoProcAmp = NULL;
  }

  if( pIAMCameraControl != NULL) {

    pIAMCameraControl->Release();
    pIAMCameraControl = NULL;
  }

  IsOpen = false;                                    // Flag info data is not in use

  IqeB_GUI_CloseToolWindow( (void **)&pMyToolWin);
}

/************************************************************************************
 * YaIPS_GUI_CamPropertiesWin
 *
 * Open dialog
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void YaIPS_GUI_CamPropertiesWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{

  //
  // window already created --> show it
  //

  if( pMyToolWin != NULL) {         // already have tool window

    // Show invisible window or bring visible window to foreground

    pMyToolWin->show();          // show it

    return;
  }

  IsOpen = true;                // Flag dialog as open

  //
  // creation of window on first call
  //

  if( pMyToolWin == NULL) {  // no tool window until now

    int xPos, yPos;

    xPos = xRight;
    yPos = yTop;

    if( MyWinPosX != IQE_GUI_NO_WINPOS_X && MyWinPosY != IQE_GUI_NO_WINPOS_Y) { // have last window position

      xPos = MyWinPosX;
      yPos = MyWinPosY;
    }

    pMyToolWin = new Fl_Window( xPos, yPos, 352, 339, MY_WIN_GUI_NAME);

    if( pMyToolWin == NULL) {  // security test

      IsOpen = false;           // Flag info data is not in use

      return;
    }
  }

  //
  //  GUI things
  //

  int x1, x2, xx1, y, yy;
  //x/int xx2, xc;
  int yGroup;

  Fl_Check_Button *pTemp_Check_Button;
  IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group;
  Fl_Button       *pTemp_Button;
  //x/Fl_Choice       *pTemp_Choice;
  Fl_Value_Slider *pTemp_ValSlider;
  Fl_Box          *pTemp_Box;

  x1  = 4;
  xx1 = pMyToolWin->w() - 16;
  //x/xx2 = xx1 / 2;
  //x/xc  = pMyToolWin->w() / 2;          // x center

  yy  = 20;

  y = 2;

  //
  // Tabs
  //

  pTemp_Tabs = new IqeFl_Tabs( x1, y, pMyToolWin->w() - x1 - 4, pMyToolWin->h() - y - 4);
  pTemp_Tabs->selection_color( YAIPS_COLOR_SELECTION);
  pTab_Groups = pTemp_Tabs;

  y += 22;

  //
  // Group 'Video Proc Amp'
  //

  yGroup = y;

  //
  // Group 'XXX'
  //

  y = yGroup;
  x1  = 4;
  pTemp_Group = new Fl_Group( x1, y, pMyToolWin->w() - x1 - 4, pMyToolWin->h() - y - 4, LangStringLookup( "&GUI_CamProperties_TabA1=Video Proc Amp"));

    y += 22;

    for( int iProperty = 0; iProperty < CAM_PROP_AMP_N; iProperty++) {

      x1 = 4;

      xx1 = 86;

      pTemp_Box = new Fl_Box( x1, y, xx1, yy, LangStringLookup( ProcAmpPropertyTable[ iProperty].pName));
      pTemp_Box->labelsize( 11);
      pTemp_Box->box( FL_NO_BOX);
      pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
      ProcAmpPropertyies[ iProperty].pBoxName = pTemp_Box;

      x1 += xx1 + 4;

      xx1 = 220;

      x2 = x1 + xx1 / 2;                  // Remember center of slider

      pTemp_ValSlider = new Fl_Value_Slider( x1, y, xx1, yy);
      pTemp_ValSlider->align( FL_ALIGN_LEFT);     // align for label
      pTemp_ValSlider->type( FL_HOR_NICE_SLIDER);
      pTemp_ValSlider->box( FL_FLAT_BOX);
      pTemp_ValSlider->color( FL_BACKGROUND_COLOR);  // Background color
      pTemp_ValSlider->bounds( 0, 100);
      pTemp_ValSlider->step( 1);
      pTemp_ValSlider->slider_size( 0.01);
      pTemp_ValSlider->value( 0);
      pTemp_ValSlider->callback( YaIPS_CamProp_AmpSlider_Callback, (void *)(long long)iProperty);
      ProcAmpPropertyies[ iProperty].pValueSlider = pTemp_ValSlider;

      x1 += xx1 + 4;

      pTemp_Check_Button = new Fl_Check_Button( x1, y, yy, yy);
      pTemp_Check_Button->value( 0);
      pTemp_Check_Button->callback( YaIPS_CamProp_AmpCheck_Callback, (void *)(long long)iProperty);
      ProcAmpPropertyies[ iProperty].pCheckBox = pTemp_Check_Button;
      if( iProperty == 0) {  // First checkbox

        pTemp_Box = new Fl_Box( x1, y - 1, yy, 1, LangStringLookup( "&GUI_CamProperties_Auto=Autom."));
        pTemp_Box->labelsize( 11);
        pTemp_Box->box( FL_NO_BOX);
        pTemp_Box->align( FL_ALIGN_TOP);
      }

      // Next line

      y += yy + 4;
    }

    // Standard settings button

    xx1 = 80;
    x1 = x2 - xx1 / 2;         // Center button to slider

    pTemp_Button = new Fl_Button( x1, y, xx1, yy, LangStringLookup( "&GUI_CamProperties_Default=Default"));
    pTemp_Button->tooltip( LangStringLookup( "&GUI_CamProperties_Defaulta=Set default values."));
    pTemp_Button->callback( YaIPS_CamProp_AmpDefault_Callback, NULL);
    pBut_AmpDefault = pTemp_Button;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'Camera Control'
  //

  y = yGroup;
  x1  = 4;
  pTemp_Group = new Fl_Group( x1, y, pMyToolWin->w() - x1 - 4, pMyToolWin->h() - y - 4, LangStringLookup( "&GUI_CamProperties_TabB1=Camera Control"));

    y += 22;

    for( int iProperty = 0; iProperty < CAM_PROP_CTR_N; iProperty++) {

      x1 = 4;

      xx1 = 86;

      pTemp_Box = new Fl_Box( x1, y, xx1, yy, LangStringLookup( CameraControlPropertyTable[ iProperty].pName));
      pTemp_Box->labelsize( 11);
      pTemp_Box->box( FL_NO_BOX);
      pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
      ProcCtrPropertyies[ iProperty].pBoxName = pTemp_Box;

      x1 += xx1 + 4;

      xx1 = 220;

      x2 = x1 + xx1 / 2;                  // Remember center of slider

      pTemp_ValSlider = new Fl_Value_Slider( x1, y, xx1, yy);
      pTemp_ValSlider->align( FL_ALIGN_LEFT);     // align for label
      pTemp_ValSlider->type( FL_HOR_NICE_SLIDER);
      pTemp_ValSlider->box( FL_FLAT_BOX);
      pTemp_ValSlider->color( FL_BACKGROUND_COLOR);  // Background color
      pTemp_ValSlider->bounds( 0, 100);
      pTemp_ValSlider->step( 1);
      pTemp_ValSlider->slider_size( 0.01);
      pTemp_ValSlider->value( 0);
      pTemp_ValSlider->callback( YaIPS_CamProp_CtrSlider_Callback, (void *)(long long)iProperty);
      ProcCtrPropertyies[ iProperty].pValueSlider = pTemp_ValSlider;

      x1 += xx1 + 4;

      pTemp_Check_Button = new Fl_Check_Button( x1, y, yy, yy);
      pTemp_Check_Button->value( 0);
      pTemp_Check_Button->callback( YaIPS_CamProp_CtrCheck_Callback, (void *)(long long)iProperty);
      ProcCtrPropertyies[ iProperty].pCheckBox = pTemp_Check_Button;
      if( iProperty == 0) {  // First checkbox

        pTemp_Box = new Fl_Box( x1, y - 1, yy, 1, LangStringLookup( "&GUI_CamProperties_Auto=Autom."));
        pTemp_Box->labelsize( 11);
        pTemp_Box->box( FL_NO_BOX);
        pTemp_Box->align( FL_ALIGN_TOP);
      }

     // Next line

      y += yy + 4;
    }

    // Standard settings button

    xx1 = 80;
    x1 = x2 - xx1 / 2;         // Center button to slider

    pTemp_Button = new Fl_Button( x1, y, xx1, yy, LangStringLookup( "&GUI_CamProperties_Default=Default"));
    pTemp_Button->tooltip( LangStringLookup( "&GUI_CamProperties_Defaulta=Set default values."));
    pTemp_Button->callback( YaIPS_CamProp_CtrDefault_Callback, NULL);
    pBut_CtrDefault = pTemp_Button;

    // Finish things for this group

    pTemp_Group->end();

  // finish up

  pTemp_Tabs->end();

  pTemp_Tabs->SetTabGroup( YaIPS_CamPropertPage);    // Select tab group from last session

  //
  // Layout end work
  //

  pMyToolWin->end();

  // finish up

  pMyToolWin->set_non_modal();
  pMyToolWin->callback( close_cb, &pMyToolWin);
  pMyToolWin->show();

  // Get data of current camera

  YaIPS_CamProp_Init( CamIdLastSelected);

  // Hack: Remove minimize and maximize buttons from the window caption
  YaIPS_DialogRemoveMinMaxButton( pMyToolWin);

  // Add idle action for this window

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::add_idle( IqeB_GUI_ToolsMyIdleAction);      // Redraw window during idle
#endif
  Fl::add_check( IqeB_GUI_ToolsMyIdleAction);     // Check small image size change
}

/************************************************************************************
* YaIPS_GUI_CamPropertiesLoad()
*
* Load saved properties for specific camera if property dialog is not open.
* If the property dialog loading of saved properties are done by this
* dialog on change of the camera.
*/

void YaIPS_GUI_CamPropertiesLoad( char *pCamName)
{

  if( IsOpen) {               // GUI is open

    return;
  }

  if( pMyToolWin != NULL) {   // Must be NULL

    return;
  }

  YaIPS_CamProp_Init( pCamName);   // Setup for this camera

  // Ensure handles for camera properties are released

  if( pAMVideoProcAmp != NULL) {

    pAMVideoProcAmp->Release();
    pAMVideoProcAmp = NULL;
  }

  if( pIAMCameraControl != NULL) {

    pIAMCameraControl->Release();
    pIAMCameraControl = NULL;
  }

}

/********************************** End Of File **********************************/
