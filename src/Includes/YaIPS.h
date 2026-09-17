/****************************************************************************

  YaIPS.h

  03.01.2025 RR: First edition of this file.
  03.09.2026 RR: * Added reference to IqeB_FileNormPathCharsAndCWD().
                 * Added reference to IqeB_DirExsits().
                 * Reworked clipboard handling
                   Added defines YaIPS_CLIPBOARD_DIR and YaIPS_CLIPBOARD_PATH.
  08.09.2026 RR: * Added reference to
                   * IqeB_FileCopyFilesInDir()
                   * IqeB_FileDelFilesInDir()
                   * IqeB_PresetCleanClipboard()

*****************************************************************************
*/

#ifndef YAIPS_H_
#define YAIPS_H_

/************************************************************************************
 * Some version defines
 */

#define WIN_PROG_NAME         "&VersionInfo_PName=YaIPS"
#define WIN_PROG_VERSION_NR   "V1.01"
#define WIN_PROG_VERSION_DATE "&VersionInfo_Date=September 16, 2026"  // Date like: April 9  2017
#define WIN_DEFAULT_TITLE     "&VersionInfo_Title=Yet another Image Processing Software"
#define WIN_ENGLISH_TITLE     "Yet another Image Processing Software"

#define IQE_CREDITS_TEXT                       \
   "&VersionInfo_Credits="                     \
   "FLTK, Fast Light Toolkit\n"                \
   "  https://www.fltk.org/\n"                 \
   "ccap, Camera Capture Library\n"            \
   "  https://ccap.work\n"                     \
   "stb, public domain libraries for C/C++\n"  \
   "  https://github.com/nothings\n"           \


/************************************************************************************
 * FLTK includes
 */

// Fltk specific includes
#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Spinner.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Hor_Nice_Slider.H>
#include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/fl_draw.H>
#include <FL/fl_box.H>
#include <FL/filename.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_show_colormap.H>
#include <FL/Fl_Image_Surface.H>
#include <FL/Fl_Text_Buffer.h>
//#include <FL/FL_types.H>                // Must be the last FL include

extern FL_EXPORT HINSTANCE fl_display;    // Is needed for LoadIcon()

/************************************************************************************
 * NOTE:
 * There are other includes which depends on declarations in this file
 * are at the end of this Files.
 *
 * Here are some of this includes:
 *   #include "YaIPS_Utils_FLTK.h"
 *   #include "YaIPS_IPS_Interface.h"
 *   #include "YaIPS_RGB_Interface.h"
 *
 */

/************************************************************************************
 * Some compile options
 */

//x/#define YAIPS_IDLE_CALLBACK_USE    1    // define this to use the idle callbacks in tool windows

#define USE_STB_IMAGE_FILES  1   // Define this to read/write images with stb functions else use FLTK functions

/************************************************************************************
 * Some common used things
 */

#ifdef NULL
#undef NULL
#endif
#define NULL 0

// Transformation matrix support

#define A11 0
#define A12 1
#define A21 2
#define A22 3
#define B1  4
#define B2  5

typedef float YaIPS_dMatrix_t[ 6];    // Double Matrix

// Transform points with a matrix.
// pMatrix is a float double or int vecgtor with 6 elements.
#define MATRIX_TRANSFORM_X( pMatrix, X, Y)  ((pMatrix[ A11]) * (X) + (pMatrix[ A12]) * (Y) + (pMatrix[ B1]))
#define MATRIX_TRANSFORM_Y( pMatrix, X, Y)  ((pMatrix[ A21]) * (X) + (pMatrix[ A22]) * (Y) + (pMatrix[ B2]))

/************************************************************************************
 * Some common used window sizes
 */

#define WIN_ID_NAME_SIZE        128   // Size of string holding a window id name.

// Colors
#define YAIPS_COLOR_MAIN_BGND    220, 220, 220                  // Color for window background
#define YAIPS_COLOR_SELECTION    fl_rgb_color(  97, 255, 140)   // Color for FL_SELECTION_COLOR
#define YAIPS_COLOR_RONLY_BGND   fl_rgb_color(  255, 255, 200)  // Color for the background of read only input elements

// Colors for buttons: Show this image on big display
#define YAIPS_BCOL_BUTTON           (FL_BLUE + 5) //(FL_DARK3 - 2)       // 39 - 2  Dark gray
#define YAIPS_BCOL_SHOW_THIS        FL_GREEN             // 63  Light green
#define YAIPS_BCOL_SHOW_OTHER       (FL_DARK_GREEN - 1)  // 59  Dark green

// Line width for some graphics. Used line width depends from setting 'YaIPS_Setting_Wide_Graphic_Lines'.

#define YAIPS_LINE_WIDTH_SMALL      0        // Small line width in pixel for histograms, AOIs
#define YAIPS_LINE_WIDTH_WIDE       3        // Wide line width in pixel for histograms, AOIs

// Small 1  window sizes
#define YAIPS_WIN_SIZE_S1_X_MIN       (193 + 12)
#define YAIPS_WIN_SIZE_S1_X_MAX       (512 + 12)
#define YAIPS_WIN_SIZE_S1_X_DEFAULT   MYWIN_SIZE_X_MIN

#define YAIPS_WIN_SIZE_S1_Y_MIN       (136 + 54)
#define YAIPS_WIN_SIZE_S1_Y_MAX       (512 + 54)
#define YAIPS_WIN_SIZE_S1_Y_DEFAULT   MYWIN_SIZE_Y_MIN

/************************************************************************************
 * Some global defines
 */

#define MAX_FILENAME_LEN 2048                       // length of filenames with path

#ifdef USE_STB_IMAGE_FILES                          // Use stb functions to read/write images

  // Image file types which can be read
  #define YAIPS_IMAGE_FILES_READ_KNOWN  "png,jpeg,jpg,tga,bmp,gif,psd,hdr,pic,ppm,pgm"    // List of known read image file extensions

  // Image file types which can be written
  #define YAIPS_IMAGE_FILES_WRITE_BROWSER  "*.png\n*.jpg\n*.tga\n"     // Use this for the file browser

  #define YAIPS_IMAGE_FILES_WRITE_TAB_N     3                          // Number of file type table elements
  #define YAIPS_IMAGE_FILES_WRITE_TAB_DATA (char *)"png", (char *)"jpg", (char *)"tga"         // Data for a file type table. Must be same order as used for the browser.

#else                                              // Use FLTK functions to read/write images

#define YAIPS_IMAGE_FILES_READ_KNOWN "png,jpeg,jpg,gif,bmp"    // List of known image file extensions
#endif

// 27.05.2026 RR: Video file formats picked from ccap source 'ccap_imp.h' v1.7.4
#define YAIPS_VIDEO_FILES_READ_KNOWN "mp4,mov,avi,mkv,wmv,webm,m4v,flv,3gp"    // List of known video file extensions
#define YAIPS_VIDEO_FILES_WRITE_KNOWN "mp4"    // 6.6.2026 RR: Also .mov is known but not used

// Display of images in image box

// Image enlargement
#define YAIPS_DISP_RESOLUTION_INVALID  -1          // Display resolution is invalid.
#define YAIPS_DISP_RESOLUTION_AUTO      0          // Display resolution automatic. This is the default.
#define YAIPS_DISP_RESOLUTION_1_1       1          // Display resolution 1:1
#define YAIPS_DISP_RESOLUTION_X_2       2          // Display resolution enlarged 2
#define YAIPS_DISP_RESOLUTION_X_4       3          // Display resolution enlarged 4
#define YAIPS_DISP_RESOLUTION_X_8       4          // Display resolution enlarged 8
#define YAIPS_DISP_RESOLUTION_X_16      5          // Display resolution enlarged 16
#define YAIPS_DISP_RESOLUTION_MAX   YAIPS_DISP_RESOLUTION_X_16  // Display resolution max value

// Colorize image, color modification
#define YAIPS_DISP_COLMOD_INVALID      -1          // Colorize image is invalid.
#define YAIPS_DISP_COLMOD_NORMAL        0          // No color modification
#define YAIPS_DISP_COLMOD_BW            1          // BW image
#define YAIPS_DISP_COLMOD_R             2          // Red component
#define YAIPS_DISP_COLMOD_G             3          // Green component
#define YAIPS_DISP_COLMOD_B             4          // Blue component
#define YAIPS_DISP_COLMOD_A             5          // Alpha component
#define YAIPS_DISP_COLMOD_MAX       YAIPS_DISP_COLMOD_A   // Max value

#define YAIPS_LUT_N_POINTS   256                      // Number of entries in a LUT

typedef struct {                                     // Color modification style. See YAIPS_DISP_COLMOD_xxx
  int Style;                                         // Histogram table style
  int FalseColor;                                    // > 0 False color table type
  int Invert;                                        // Flag: Invert color
  int Darken;                                        // Flag: Darken color
  uchar LookupR[ YAIPS_LUT_N_POINTS], LookupG[ YAIPS_LUT_N_POINTS], LookupB[ YAIPS_LUT_N_POINTS]; // OUT: Calculated color table
} Fl_YaIPS_ColMod_t;

// Image overlay
#define YAIPS_OVERLAY_OFF               0          // No overlay
#define YAIPS_OVERLAY_CROSSHAIR         1          // Draw a crosshair
#define YAIPS_OVERLAY_GRID_1            2          // Grid 1, few lines
#define YAIPS_OVERLAY_GRID_2            3          // Grid 2, many lines
#define YAIPS_OVERLAY_MAX       YAIPS_OVERLAY_GRID_2   // Max value

// Show measurement/info modes

#define YAIPS_SHOW_INFO_NOTHING        -1          // Show nothing
#define YAIPS_SHOW_INFO_OFF             0          // Off. Show histogram big image
#define YAIPS_SHOW_INFO_CU_LUT          1          // Show current lookup table
#define YAIPS_SHOW_INFO_CU_VAL          2          // At cursor show pixel value
#define YAIPS_SHOW_INFO_RE_HISTO_ALL    3          // In show histogram in complete image
#define YAIPS_SHOW_INFO_RE_HISTO_AOI    4          // In show histogram in AOI
#define YAIPS_SHOW_INFO_RE_COLSUM       5          // In rectangle show column sum
#define YAIPS_SHOW_INFO_RE_ROWSUM       6          // In rectangle show row sum
#define YAIPS_SHOW_INFO_2P_DIST_VER     7          // Show distance of two points vertical
#define YAIPS_SHOW_INFO_2P_DIST_HOR     8          // Show distance of two points horizontal
#define YAIPS_SHOW_INFO_MAX       YAIPS_SHOW_INFO_2P_DIST_HOR   // Max value

// May # of windows for multiple window types

#define YAIPS_WIN_MAX_ENTRIES      8     // Possible maximum # of windows for multiple window types
                                         // This allows the change of # for the defines below
                                         // because it keeps the windows ID numbers.

#define YAIPS_WIN_MAX_IMG_FILES    6     // Max number of file windows
#define YAIPS_WIN_MAX_FILTER       4     // Max number of filter windows
#define YAIPS_WIN_MAX_COMBINE      4     // Max number of combine windows
#define YAIPS_WIN_MAX_CRCDF        4     // Max number of crcdf windows
#define YAIPS_WIN_MAX_OBJECTS      4     // Max number of objects windows
#define YAIPS_WIN_MAX_GEOTRAN      4     // Max number of geometric transformation windows
#define YAIPS_WIN_MAX_COLOR        6     // Max number of color windows
#define YAIPS_WIN_MAX_EDGES        4     // Max number of edges windows
#define YAIPS_WIN_MAX_POSCORR      4     // Max number of position correction windows
#define YAIPS_WIN_MAX_OTHER        4     // Max number of other windows
#define YAIPS_WIN_MAX_GEN_IMAGE    4     // Max number of generate image windows
#define YAIPS_WIN_MAX_INSP_COLOR   4     // Max number of inspection color windows
#define YAIPS_WIN_MAX_INSP_COMPARE 4     // Max number of inspection compare windows
#define YAIPS_WIN_MAX_INSP_REF_IMG 4     // Max number of inspection reference image windows
#define YAIPS_WIN_MAX_OVERLAY      4     // Max number of overlay windows
#define YAIPS_WIN_MAX_CAM_PR_SAVE  YAIPS_WIN_MAX_ENTRIES  // Max number of saved camera property settings
#define YAIPS_WIN_MAX_VIDEO_READ   4     // Max number of video read windows
#define YAIPS_WIN_MAX_VIDEO_WRITE  1     // Max number of video write windows

// Big image source defines
// See: YaIPS_BigImageSourceID
// See:
#define YAIPS_WIN_ID_KEEP             -1   // Don't change current source define
#define YAIPS_WIN_ID_NONE              0   // No image source set
#define YAIPS_WIN_ID_IS_VALID          1   // Is any valid window ID
#define YAIPS_WIN_ID_DROP_MAIN         2   // Image was dropped to main window
#define YAIPS_WIN_ID_CAMERA            3   // Image from camera
#define YAIPS_WIN_ID_PROPERTIES        4   // Camera properties dialog
#define YAIPS_WIN_ID_CALIBRATE         5   // Calibrate dialog
#define YAIPS_WIN_ID_CUSTOM_COLORS     6   // Custom colors dialog
#define YAIPS_WIN_ID_IMG_WIN          10   // Image is loaded from file from an image window
#define YAIPS_WIN_ID_IMG_WIN_END      (YAIPS_WIN_ID_IMG_WIN + YAIPS_WIN_MAX_ENTRIES - 1)      // 18 last one
#define YAIPS_WIN_ID_FILTER           (YAIPS_WIN_ID_IMG_WIN_END + 1)                          // Image is from a filter window
#define YAIPS_WIN_ID_FILTER_END       (YAIPS_WIN_ID_FILTER + YAIPS_WIN_MAX_ENTRIES  - 1)      // 26 last one
#define YAIPS_WIN_ID_COMBINE          (YAIPS_WIN_ID_FILTER_END + 1)                           // Image is from a combine window
#define YAIPS_WIN_ID_COMBINE_END      (YAIPS_WIN_ID_COMBINE + YAIPS_WIN_MAX_ENTRIES - 1)      // 34 last one
#define YAIPS_WIN_ID_CRCDF            (YAIPS_WIN_ID_COMBINE_END + 1)                          // Image is from a crcdf window
#define YAIPS_WIN_ID_CRCDF_END        (YAIPS_WIN_ID_CRCDF + YAIPS_WIN_MAX_ENTRIES - 1)        // 42 last one
#define YAIPS_WIN_ID_OBJECTS          (YAIPS_WIN_ID_CRCDF_END + 1)                            // Image is from a objects window
#define YAIPS_WIN_ID_OBJECTS_END      (YAIPS_WIN_ID_OBJECTS + YAIPS_WIN_MAX_ENTRIES - 1)      // 50 last one
#define YAIPS_WIN_ID_GEOTRAN          (YAIPS_WIN_ID_OBJECTS_END + 1)                          // Image is from a geometric transformation window
#define YAIPS_WIN_ID_GEOTRAN_END      (YAIPS_WIN_ID_GEOTRAN + YAIPS_WIN_MAX_ENTRIES - 1)      // 58 last one
#define YAIPS_WIN_ID_COLOR            (YAIPS_WIN_ID_GEOTRAN_END + 1)                          // Image is from a color window
#define YAIPS_WIN_ID_COLOR_END        (YAIPS_WIN_ID_COLOR + YAIPS_WIN_MAX_ENTRIES - 1)        // 66 last one
#define YAIPS_WIN_ID_EDGES            (YAIPS_WIN_ID_COLOR_END + 1)                            // Image is from a edges window
#define YAIPS_WIN_ID_EDGES_END        (YAIPS_WIN_ID_EDGES + YAIPS_WIN_MAX_ENTRIES - 1)        // 74 last one
#define YAIPS_WIN_ID_POSCORR          (YAIPS_WIN_ID_EDGES_END + 1)                            // Image is from a position correction window
#define YAIPS_WIN_ID_POSCORR_END      (YAIPS_WIN_ID_POSCORR + YAIPS_WIN_MAX_ENTRIES - 1)      // 82 last one
#define YAIPS_WIN_ID_OTHER            (YAIPS_WIN_ID_POSCORR_END + 1)                          // Image is from a other window
#define YAIPS_WIN_ID_OTHER_END        (YAIPS_WIN_ID_OTHER + YAIPS_WIN_MAX_ENTRIES - 1)        // 90 last one
#define YAIPS_WIN_ID_GEN_IMAGE        (YAIPS_WIN_ID_OTHER_END + 1)                            // Image is from a generate image window
#define YAIPS_WIN_ID_GEN_IMAGE_END    (YAIPS_WIN_ID_GEN_IMAGE + YAIPS_WIN_MAX_ENTRIES - 1)    // 98 last one
#define YAIPS_WIN_ID_INSP_COLOR       (YAIPS_WIN_ID_GEN_IMAGE_END + 1)                        // Image is from a color inspection window
#define YAIPS_WIN_ID_INSP_COLOR_END   (YAIPS_WIN_ID_INSP_COLOR + YAIPS_WIN_MAX_ENTRIES - 1)   // 106 last one
#define YAIPS_WIN_ID_INSP_COMPARE     (YAIPS_WIN_ID_INSP_COLOR_END + 1)                       // Image is from a compare inspection window
#define YAIPS_WIN_ID_INSP_COMPARE_END (YAIPS_WIN_ID_INSP_COMPARE + YAIPS_WIN_MAX_ENTRIES - 1) // 114 last one
#define YAIPS_WIN_ID_INSP_REF_IMG     (YAIPS_WIN_ID_INSP_COMPARE_END + 1)                     // Image is from a reference image inspection window
#define YAIPS_WIN_ID_INSP_REF_IMG_END (YAIPS_WIN_ID_INSP_REF_IMG + YAIPS_WIN_MAX_ENTRIES - 1) // 122 last one
#define YAIPS_WIN_ID_OVERLAY          (YAIPS_WIN_ID_INSP_REF_IMG_END + 1)                     // Image is from an overlay window
#define YAIPS_WIN_ID_OVERLAY_END      (YAIPS_WIN_ID_OVERLAY + YAIPS_WIN_MAX_ENTRIES - 1)      // 130 last one
#define YAIPS_WIN_ID_CAM_PR_SAVE      (YAIPS_WIN_ID_OVERLAY_END + 1)                          // No image output for this
#define YAIPS_WIN_ID_CAM_PR_SAVE_END  (YAIPS_WIN_ID_CAM_PR_SAVE + YAIPS_WIN_MAX_ENTRIES - 1)  // 138 last one
#define YAIPS_WIN_ID_VIDEO_READ       (YAIPS_WIN_ID_CAM_PR_SAVE_END + 1)                      // Image is from a video read window
#define YAIPS_WIN_ID_VIDEO_READ_END   (YAIPS_WIN_ID_VIDEO_READ + YAIPS_WIN_MAX_ENTRIES - 1)   // 146 last one
#define YAIPS_WIN_ID_VIDEO_WRITE      (YAIPS_WIN_ID_VIDEO_READ_END + 1)                       // Image is from a video write window
#define YAIPS_WIN_ID_VIDEO_WRITE_END  (YAIPS_WIN_ID_VIDEO_WRITE + YAIPS_WIN_MAX_ENTRIES - 1)  // 154 last one

#define YAIPS_WIN_MANAGER_MAX       (YAIPS_WIN_ID_VIDEO_WRITE_END + 1)  // Max number of windows managed

//
// YaIPS_GUI_Calibrate.cpp
//

// functions

void IqeB_GUI_CalibrationWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx);

//
// YaIPS_GUI_Camera.cpp
//

#define HMODE_ACQ_REGION_MIN_HEIGHT   64    // Minimum height of height mode region

extern int YaIPS_Camera_XX;                     // If > 0, Width of camera image
extern int YaIPS_Camera_YY;                     // If > 0, Height of camera image

// Open camera window
void IqeB_GUI_CameraWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

// Redraw camera image
void IqeB_GUI_CameraImageRedraw();

// Snap a single camera image after a camera property has changed
void IqeB_GUI_CameraSnapAfterPropertyChange();

//
// YaIPS_GUI_CamProperties.cpp
//

void YaIPS_GUI_CamPropertiesWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx);

// Load properties for specific camera if property dialog is not open.
void YaIPS_GUI_CamPropertiesLoad( char *pCamName);

//
// YaIPS_GUI_CamSettings.cpp
//

// Camera acquisition mode

#define YAIPS_CAM_ACQ_MODE_AREA    0  // Acquisition mode area camera
#define YAIPS_CAM_ACQ_MODE_LINE    1  // Acquisition mode line camera
#define YAIPS_CAM_ACQ_MODE_HEIGHT  2  // Acquisition mode height camera

extern int YaIPS_CamPar_Acq_Mode;           // Acquisition mode. Is also number of last selected tab group.

// Area camera settings
extern int YaIPS_CamPar_AMode_Square;       // Area mode: If true, make a square image output
extern int YaIPS_CamPar_AMode_ColMod;       // All cameras: Color space of output: RGB, BW, R, G or B
extern int YaIPS_CamPar_AMode_SkipFirst;    // All cameras: Skip first camera.
extern int YaIPS_CamPar_AMode_StartAcqOn;   // All cameras: Start with 'continuous acquire on' after open of dialog.

// Line scan camera simulation
extern int YaIPS_CamPar_LMode_Adjust;       // Line mode: If true, show lines to average
extern int YaIPS_CamPar_LMode_LinesAcq;     // Line mode: height of acquired image, # of lines to acquire
extern int YaIPS_CamPar_LMode_nAvgLines;    // Line mode: # area lines to average
extern int YaIPS_CamPar_LMode_nRefresh;     // Line mode: Refresh image after # lines acquired

// Height camera simulation
extern int YaIPS_CamPar_HMode_Adjust;       // Height mode: If true, adjust top/lower line in area camera
extern int YaIPS_CamPar_HMode_LinesAcq;     // Height mode: height of acquired image, # of lines to acquire
extern int YaIPS_CamPar_HMode_Top_Line;     // Height mode: Top line in area camera
extern int YaIPS_CamPar_HMode_Base_Line;    // Height mode: Base/lower line in area camera
extern int YaIPS_CamPar_HMode_Val_Thres;    // Height mode: Brightness threshold

void YaIPS_GUI_CamSettingsWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx);

//
// YaIPS_GUI_Color.cpp
//

// Open filter window
void IqeB_GUI_ColorWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_Combine.cpp
//

// Open combine window
void IqeB_GUI_CombineWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_Crcdf.cpp
//

// Open crcdf window
void IqeB_GUI_CrcdfWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_CustomColors.cpp
//

// Open Custom color manager
void IqeB_GUI_CustomColorWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_Filter.cpp
//

// Open filter window
void IqeB_GUI_FilterWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_Image_Generate.cpp
//

// Open generate image window
void IqeB_GUI_GenImageWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_GeoTransform
//

// Open geometry transformation window
void IqeB_GUI_GeoTranWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_Image_File.cpp
//

// Open a window to show images loaded from files
void IqeB_GUI_ImageFileWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx);

//
// YaIPS_GUI_InspColor.cpp
//

// Open the inspection color window
void IqeB_GUI_InspColorWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx);

//
// YaIPS_GUI_InspCompare.cpp
//

// Open the inspection compare window
void IqeB_GUI_InspCompareWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx);

//
// YaIPS_GUI_InspEdges.cpp
//

// Open filter window
void IqeB_GUI_EdgesWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_InspPosCorr.cpp
//

// Open position correction window
void IqeB_GUI_PosCorrWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_InspRefImage.cpp
//

#define YAIPS_REFMASK_CONTOUR      0x80        // Use this bit to set contour in alpha channel.

// Open inspection reference image window
void IqeB_GUI_InspRefImgWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_Main.cpp
//

// define for main  window sizes
#define YAIPS_MAIN_SIZE_X_MIN       964
#define YAIPS_MAIN_SIZE_X_MAX      2048
#define YAIPS_MAIN_SIZE_X_DEFAULT   YAIPS_MAIN_SIZE_X_MIN

#define YAIPS_MAIN_SIZE_Y_MIN       715
#define YAIPS_MAIN_SIZE_Y_MAX      2048
#define YAIPS_MAIN_SIZE_Y_DEFAULT   YAIPS_MAIN_SIZE_Y_MIN

extern int YaIPS_GUI_Main_Do_Startup;    // Used during startup of tool windows. True during startup phase.
extern char YaIPS_WorkingDirectory[ MAX_FILENAME_LEN]; // Working directory. Is set at startup of application.
extern char YaIPS_BrowserDirectory[ MAX_FILENAME_LEN]; // Path of current directory for file browsers
extern char YaIPS_BrowserDirVideos[ MAX_FILENAME_LEN]; // Path of current directory for video file browsers

// Predefined window sizes

#define IQE_GUI_TOOLS_STD_WITDH   240   // Std with of tools windows
#define IQE_GUI_BUTTON_STD_HEIGHT  22   // Std button height

#define IQE_GUI_NO_WINPOS_X     -4711   // if this value, no preference window position set
#define IQE_GUI_NO_WINPOS_Y     -4711   //
#define IQE_GUI_NO_WINPOS_X_STRING   "-4711"   // if this value, no preference window position set
#define IQE_GUI_NO_WINPOS_Y_STRING   "-4711"   //

#define YAIPS_IMAGE_DISP_BORDER     16   // Smaller inside of big/small image box border
#define YAIPS_IMAGE_DISP_BORDER2     8   // Indent from each side big/small image box border

// Used for menu definitions for opening a window.
void IqeB_GUI_But_Tool_OpenWin_Callback( Fl_Widget *w, void *pValueArg);

//
// YaIPS_GUI_Objects.cpp
//

// Open objects window
void IqeB_GUI_ObjectsWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_Other.cpp
//

// Open other window
void IqeB_GUI_OtherWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_Overlay.cpp
//

// Open overlay window
void IqeB_GUI_OverlayWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

///
// YaIPS_GUI_Settings.cpp
//

// Common settings

extern char YaIPS_Setting_Language[];          // Language to load at startup
extern int YaIPS_Setting_Startup_WinRestore;   // If true, open windows from last session
extern int YaIPS_Setting_Wide_Graphic_Lines;   // If true, use wide lines for graphics
extern int YaIPS_Setting_PasteImgCol2BW;       // If true, support conversion from pseudo BW color images to a BW images
extern int  YaIPS_Setting_File_JPEG_Quality;   // JPEG quality used for writing IPEG files

// Colors

extern unsigned int YaIPS_Color_IMG_BGND;      // Color for background of image boxes. 215: a light yellow background
extern unsigned int YaIPS_Color_GRA_FRAME;     // For some graphics (row sum, ..) color for the frame
extern unsigned int YaIPS_Color_RIGHT_BGND;    // Right side box background (used to place tool windows)

#define YAIPS_CUSTOM_COLORS_N 16               // Number of custom colors

extern unsigned int YaIPS_Setting_ChooseColorPredef[ YAIPS_CUSTOM_COLORS_N]; //Array of custom colors for choose color dialog

extern unsigned int YaIPS_Setting_CopyPasteColor;    // Use this variable to copy/paste color values for the color button

// ...

void YaIPS_GUI_SettingsWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_Utils_Misc.cpp
//

// Math utilities

// Get the greatest common divisor of two numbers.
int GreatestcommonDivisor( int a, int b);

// File system utilities

void IqeB_FileNormalizePathChars( char *pPath);
void IqeB_FileNormPathCharsAndCWD( char *pPath);
void IqeB_FileGetBaseName( char *pFilename, char *pBasename, int SizeBasename);
void IqeB_FileGetFileName( char *pFilename, char *pOut, int SizeOut);
void IqeB_FileGetPath( char *pFilename, char *pOut, int SizeOut);
void IqeB_FileEnsureExtension( char *pFilename, char *pExtension, char *pOut, int SizeOut);
int  IqeB_FileCheckExtension( char *pFilename, const char *pExtensionList);
int  IqeB_FileExsits( char *pFilename);
int  IqeB_DirExsits( char *pPath);
void IqeB_FileDelete( char *pFilename);
void IqeB_FileMakePath( char *pPath);
void IqeB_FileCopyFilesInDir( char *pDirDst, char *pDirSrc);
void IqeB_FileDelFilesInDir( char *pDir);

// string utilities

int IqeB_strcasecmp( const char *s1, const char *s2);
int IqeB_strncasecmp( const char *s1, const char *s2, int n);
char *IqeB_strcasestr( char *s1, char *s2);
char *IqeB_strdup( const char *s);

// Add additional symbols (for buttons, ...)

void YaIPS_Utils_AddSympols();

// Enumerate known fonts on this PC

#define YAIPS_FONT_NAME_SIZE 128  // Size in characters for font name

typedef struct {
  char FontName[ YAIPS_FONT_NAME_SIZE];  // Base name of font
  int FontNr_regular;             // Font nr for regular face
  int FontNr_bold;                // Font nr for bold face
  int FontNr_italic;              // Font nr for italic face
  int FontNr_bold_italic;         // Font nr for bold italic face
} YaIPS_FontBase_t;

extern YaIPS_FontBase_t *pYaIPS_FontBase; // Point to font face pool
extern int YaIPS_nFontBase;       // Number of base fonts

extern int YaIPS_FontsCount;      // Number of enumerated fonts

// Enumerate known fonts on this PC
void YaIPS_Utils_FontsEnum();

// Lookup a known base font
YaIPS_FontBase_t *pYaIPS_Utils_FontsLookup( char *pFontName);

// Convert Pixel depth to string
char *pYaIPS_PixelDepth_to_string( int PixelDepth);

// -----------------------------------------------------------
// A specialized Fl_Box usable for file drops
// and for additional drawing code.
// -----------------------------------------------------------

// Mouse callback function
typedef int (YaIPS_Fl_Drop_Callback )( Fl_Widget *, void *, void *pImageDispArg, int SubWinIDx);  // Sub-window index. Starts with 0.
typedef int (YaIPS_Fl_Mouse_Callback )( Fl_Widget *, int, void *, void *);
typedef void (YaIPS_Fl_DrawAfter_Callback )( Fl_Widget *, void *, void *);
typedef void (YaIPS_Fl_PasteImgCallback )( void *, Fl_RGB_Image *pRGB);

class YaIPS_Fl_Box : public Fl_Box {

public:

  // Event handler callbacks
  YaIPS_Fl_Drop_Callback *pDropCallback        = NULL; // Call on file drop. NOTE: Return true when handled event else false.
  YaIPS_Fl_PasteImgCallback *pPasteImgCallback = NULL; // Call on past of an image
  void *PasteImgCallbackArg1                   = NULL; // Argument for pPasteImgCallback. Must be pointer to Fl_YaIPS_ImageDisp_t.
  YaIPS_Fl_Mouse_Callback *pMouseCallback      = NULL; // Call on mouse events.    NOTE: returns are ignored until now.
  YaIPS_Fl_Mouse_Callback *pKeyboardCallback   = NULL; // Call on keyboard events. NOTE: Return true when handled event else false.
  void *MouseCallbackArg1 = NULL;                 // Argument for pMouseCallback. Must be pointer to Fl_YaIPS_ImageDisp_t.
  void *MouseCallbackArg2 = NULL;                 // Argument for pMouseCallback. Optional, could be pointer to ToolData.
  int MouseTeachState = 0;                        // Used to color teach pencil symbol on main GUI
                                                  // 0: no mouse callback set, 1: Teach mode not available, 2: disabled: 3: enabled

  // Draw callbacks
  Fl_Callback *pDrawBeforeCallback = NULL;     // Call before box drawing
  YaIPS_Fl_DrawAfter_Callback *pDrawAfterCallback  = NULL;     // Call after box drawing
  void *DrawCallbackArg1 = NULL;               // Argument for pDrawBeforeCallback / pDrawAfterCallback. Must be pointer to Fl_YaIPS_ImageDisp_t.
  void *DrawCallbackArg2 = NULL;               // Argument for pDrawBeforeCallback / pDrawAfterCallback. Optional, could be pointer to ToolData.

  // Create the window

  YaIPS_Fl_Box( int X, int Y, int W, int H, const char *l=0) : Fl_Box( X, Y, W, H, l) {

    // Event handler callbacks
    pDropCallback        = NULL;
    pPasteImgCallback    = NULL;
    PasteImgCallbackArg1 = NULL;
    pMouseCallback       = NULL;
    pKeyboardCallback    = NULL;
    MouseCallbackArg1    = NULL;
    MouseCallbackArg2    = NULL;
    MouseTeachState      = 0;

    // Draw callbacks
    pDrawBeforeCallback  = NULL;
    pDrawAfterCallback   = NULL;
    DrawCallbackArg1     = NULL;
    DrawCallbackArg2     = NULL;
  }

  // Overwrite draw handler
  void draw() {

    if( pDrawBeforeCallback != NULL) {          // Have an optional draw after callback ?

      pDrawBeforeCallback( this, DrawCallbackArg1);
    }

    Fl_Box::draw();                             // Call parent draw

    if( pDrawAfterCallback != NULL) {           // Have an optional draw after callback ?

      pDrawAfterCallback( this, DrawCallbackArg1, DrawCallbackArg2);
    }
  }

private:

  // Overwrite receiver event handler to handle drop of files
  int handle( int event) {
    int ret;

    ret = 0;

    // A simple drag and drop receiver class

    if( pDropCallback != NULL ||       // A drop callback is set
        pPasteImgCallback != NULL) {   // or a paste image is set

      switch( event) {
      case FL_DND_DRAG:
        if( pDropCallback != NULL) {   // A drop callback is set

          if( pMouseCallback != NULL) {  // A drop callback is set

            pMouseCallback( this, event, MouseCallbackArg1, MouseCallbackArg2);
          }

          return 1;                    // Processed event, return to caller
        }
        break;
      case FL_DND_ENTER:               // return(1) for these events to 'accept' dnd
      case FL_DND_RELEASE:
        if( pDropCallback != NULL) {   // A drop callback is set
          return 1;                    // Processed event, return to caller
        }
        break;
      case FL_PASTE:              // handle actual drop (paste) operation

        if (strcmp(Fl::event_clipboard_type(), Fl::clipboard_image) == 0) { // an image is being pasted

          Fl_RGB_Image *cl_img;

          cl_img = (Fl_RGB_Image *)Fl::event_clipboard(); // get it as an Fl_RGB_Image object

          if( cl_img != NULL &&                           // Security test
              pPasteImgCallback != NULL) {                // and callback is set

            pPasteImgCallback( PasteImgCallbackArg1, cl_img);
          }

        } else {      // text is being pasted

          const char *pEventText;

          pEventText = Fl::event_text();
          if( pEventText != NULL && strlen( pEventText) > 0 &&  // This must be a file name
              pDropCallback != NULL &&                          // and callback is set
              PasteImgCallbackArg1 != NULL) {                   // and this argument is set

            pDropCallback( this, (void *)pEventText, PasteImgCallbackArg1, -1);
          }
        }

        return 1;                 // Processed event, return to caller
        break;
      }
    }

    // Mouse handler callbacks

    if( pMouseCallback != NULL) {  // A drop callback is set

      switch( event) {
      case FL_ENTER:      // The mouse has been moved to point at this widget.
      case FL_LEAVE:      // The mouse has moved out of the widget.
      case FL_MOVE:       // The mouse has moved without any mouse buttons held down.
      case FL_DRAG:       // The mouse has moved with a button held down.
      case FL_MOUSEWHEEL: // The user has moved the mouse wheel.
      case FL_PUSH:       // A mouse button has gone down
      case FL_RELEASE:    // A mouse button has been released.

        pMouseCallback( this, event, MouseCallbackArg1, MouseCallbackArg2);

        return( 1);
      }
    }

    // Keyboard handler callbacks

    if( pKeyboardCallback != NULL) {  // A drop callback is set

      switch( event) {
      case FL_FOCUS:      // This indicates an attempt to give a widget the keyboard focus.
      case FL_UNFOCUS:    // This event is sent to the previous Fl::focus() widget when another widget gets the focus or the window loses focus.
      case FL_KEYDOWN:    // A key was pressed.
      //x/case FL_KEYBOARD:   // Equivalent to FL_KEYDOWN.
      case FL_KEYUP:      // Key release event.
      case FL_SHORTCUT:   // Process keyboard shortcuts

        ret = pKeyboardCallback( this, event, MouseCallbackArg1, MouseCallbackArg2);

        break;
      }
    }

    if( ret == 0) {                   // Event not processed until now

      ret = Fl_Box::handle(event);
    }

    return(ret);
  }
};

 // Display image utilities

#define YAIPS_IDISP_FLAG_FORCE_UPDATE      0x0001     // Force update of image as soon as possible
#define YAIPS_IDISP_FLAG_MOUSE_IN_WIDGET   0x0002     // If set, mouse is inside/over box widget
#define YAIPS_IDISP_FLAG_MOUSE_IN_IMAGE    0x0004     // If set, mouse is inside/over box image in box
#define YAIPS_IDISP_FLAG_MOUSE_BUTT_ANY    0x0008     // Any mouse button pressed
#define YAIPS_IDISP_FLAG_MOUSE_AOI_SEL     0x0010     // AOI is selected
#define YAIPS_IDISP_FLAG_MOUSE_AOI_CHA     0x0020     // AOI has changed
#define YAIPS_IDISP_FLAG_DO_DISP_MODIFY    0x0040     // Have a valid image. Do display modification.
                                                      // Set by adding a new image. See YaIPS_ImageDispUpdateByNewImage()
#define YAIPS_IDISP_FLAG_LUT_CHANGED       0x0080     // LUT has changed and must be recalculated
#define YAIPS_IDISP_FLAG_LUT_ACTIVE        0x0100     // LUT is active, no 1:1 LUT used

// Reset this flag bits if out of image
#define YAIPS_IDISP_FLAG_MASK_OUT_OF_WIDGET    (~(YAIPS_IDISP_FLAG_MOUSE_IN_WIDGET | YAIPS_IDISP_FLAG_MOUSE_IN_IMAGE | \
                                                 YAIPS_IDISP_FLAG_MOUSE_BUTT_ANY | YAIPS_IDISP_FLAG_MOUSE_AOI_SEL))

#define YAIPS_3DPLOT_AZIMUT_MIN              -180     // Azimuth min angle
#define YAIPS_3DPLOT_AZIMUT_MAX               180     // Azimuth max angle
#define YAIPS_3DPLOT_ELEVATION_MIN             10     // Elevation min angle
#define YAIPS_3DPLOT_ELEVATION_MAX             90     // Elevation max angle

#define YAIPS_IDISP_AOI_MIN_SIZE                8     // Minimum AOI size
#define YAIPS_AOI_FRAME_DIST                    8     // Be near the frame to select AOI


typedef struct {
  Fl_RGB_Image    *pImage_Img;    // Last loaded image do display
  YaIPS_Fl_Box    *pImage_Box;    // Display the image in this box
  int Flags;                      // Same flag bits. See defines YAIPS_IDISP_FLAG_xxx
  int ImageSourceID;              // Image source. See defines YAIPS_WIN_ID_xxx
  char ImageName[ 128];           // Name of image. Is empty if there is no name.
  char FileName[ MAX_FILENAME_LEN]; // File name with path
  int ImageChanged;               // If >= 1, there is an image loaded. This changes any time the image has changed.
  // --- Display resolution
  int DisplayResolution;          // Display image box resolution. See YAIPS_DISP_RESOLUTION_xxx
  int DisplayResolution_Last;     // Used to catch resolution changes
  // --- Display color modification
  Fl_YaIPS_ColMod_t DisplayColMod; // Display image color modification. See YAIPS_DISP_COLMOD_xxx
  int NoAlphaDisplay;             // Don't use alpha to blend image display on screen
  // --- Precalculated things
  int BigImage_Calc_OK;           // True if size calculations are OK
  int BigImage_bx, BigImage_by, BigImage_bw, BigImage_bh;  // Box outside
  int BigImage_iw, BigImage_ih;                            // Big image size
  int BigImage_sx, BigImage_sy, BigImage_sw, BigImage_sh;  // Image part displayed on screen
  // --- Variables for 1:1 or enlarged display resolutions
  int SubImage_w, SubImage_h;    // Size of displayed image part
  float SubImage_x, SubImage_y;  // Offset of displayed image part. NOTE: must be float
  int SubImage_x_max, SubImage_y_max; // Max value for offset of displayed image part
  float PixelImageToScreen;      // One image pixel is how many screen pixels.
  // --- 3D-Visualisierung
  int Plot3D_Active;             // Flag: Plot 3D is active
  int Plot3D_DrawGrid;           // Flag: Draw grid lines
  int Plot3D_Azimuth;            // Azimuth (-180 .. 180)
  int Plot3D_Elevation;          // Elevation (0-90)
  int Plot3D_Inverted;           // Inverted grey value surface plot
  // --- Show measurement/info
  int ShowInfoMode;              // Selected mode
  // Mouse feedback
  int MouseX, MouseY;            // Mouse position relative to image box if flag YAIPS_IDISP_FLAG_MOUSE_IN_IMAGE is set
  // AOI area of interest. Is used for histogram, rowsum, colsum, ...
  int AoiP1x, AoiP1y;            // 1. point of AOI rectangle. This is the left top point.
  int AoiP2x, AoiP2y;            // 2. point of AOI rectangle. This is the right bottom point.
  int Latched_AoiDeltaAdd;       // Latched AOI mouse modification. Is != 0 during a button press.
  int Latched_CursorShape;       // Latched AOI mouse cursor shape. Is != 0 during a button press.
  // --- Display some additional things
  char StrInfo[ 256];           // If != 0 display user info about current settings or so
  Fl_Color ColInfoBgnd;         // Color for background of info string. 0 is used for default color. 0xffffffff is used for no background.
  Fl_Color ColInfoText;         // Color for text of info string. 0 is used for default color.
  int      ColInfoTextSize;     // Size for text field. Default is 18.
  char StrDebug[ 256];          // If != 0 display debug info
  // --- Variables used for CommonEntry/CommonExit and work functions
  Fl_Window *pParentWindow;
  int CursorShape, AoiDeltaAdd; // Data used for mouse pressed over an AOI
  int AoiIdNr;                  // If >= 0 identify one of multiple AOIs for mouse pressed over an AOI
  int RedrawOnExit, MyWinID, BigImageUpdate, Delta_x, Delta_y, mouseleft, mouseright;
} Fl_YaIPS_ImageDisp_t;

// Empty a display image
void YaIPS_ImageDispEmpty( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, int KeepInfoStrings = false);

// Before close of an window release all allocated data in an image display structure and reset data.
void YaIPS_ImageDispReleaseBeforeClose( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp);

// Calculate sizes for drawing image in box
// return:     -1: No image box (no drawing area)
//          false: No big image to display or no image source set
//                 NOTE: The BigImage_bX variables are set
//           true: OK, sizes are calculated
int YaIPS_ImageDispCalcSizes( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, int NewFlagBits = 0, int MouseX = -99999, int MouseY = -99999);

// Update display image. Adapt image in box.
void YaIPS_ImageDispDrawUpdate( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, int ForceUpdate);

// Additional actions before the image is drawn.
// NOTE: This hack updates the image in the box during resize of window.
void YaIPS_ImageDispDrawBefore_cb( Fl_Widget *pW, void *pArg);

// Additional actions after the image was drawn.
// Can be called from other DrawAfterCallback functions.
void YaIPS_ImageDispDrawAfter_Common( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // point to image display data
                                      int DrawFlags = 0,                       // 0 draws all. Else drawing parts must be flagged
                                      int DoClip = 0);                         // if true (> 0) handle clipping of draw region else caller must do it

// Additional actions after the image was drawn.
// NOTE: This hack displays the 'StrDebug' string and maybe other things
void YaIPS_ImageDispDrawAfter_cb( Fl_Widget *pW,
                                  void *pArg1,        // Pointer to Fl_YaIPS_ImageDisp_t
                                  void *pArg2);       // Optional pointer to ToolData

// An image display has changed.
void YaIPS_ImageDispUpdateByChangedImage( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  int ImageSourceID, char *pImageName);

// Update an image display by a new image.
void YaIPS_ImageDispUpdateByNewImage( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, Fl_RGB_Image *pNewimage,
                                      int ImageSourceID, char *pFileName,
                                      int SetDoDispModify = true, int SetScaleAuto = false, int NoAlphaDisplay = false);

// Update an image display by an image file
int YaIPS_ImageDispUpdateByFileName( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, char *pFileName,
                                    int ImageSourceID, int SetDoDispModify = true, int SetScaleAuto = false, int NoAlphaDisplay = false);

// Common paste image callback. Paste image from clipboard direct to output image.
void YaIPS_ImageDispPasteToOut_cb( void *pYaIPS_ImageDispArg, Fl_RGB_Image *pRGB);

// Common paste image callback. Paste image from clipboard using 'ToolChangeOutput' function.
void YaIPS_ImageDispPasteToOutFunc_cb( void *pYaIPS_ImageDispArg, Fl_RGB_Image *pRGB_Arg);

// Copy image to clipboard
void YaIPS_ImageDispCopyImage_cb( Fl_Widget *pWidget, void *pYaIPS_ImageDispArg);

// Mouse event callback for image displays, common entry work
int YaIPS_ImageDispMouse_CommonEntry( Fl_Widget *pW,                           // Widget calling the event
                                      int event,                               // Event code
                                      Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Image display data
                                      int *pMouseX, int *pMouseY,              // Point to unprocessed mouse coordinates
                                      int *pLast_x, int *pLast_y);             // Point to latched mouse position

// Mouse event callback for image displays, common mouse wheel work.
// Manage display resolution change by mouse wheel event.
void YaIPS_ImageDispMouse_CommonMouseWheel( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Image display data
                                            int x, int y);                           // unprocessed mouse coordinates

// Manage Left/right mouse button pressed on image background
void YaIPS_ImageDispMouse_CommonMouseBackGnd( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Image display data
                                              int *pLast_x, int *pLast_y);             // Point to latched mouse position

// Mouse event callback for image displays, common exit work
void YaIPS_ImageDispMouse_CommonExit( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp);  // Image display data

// Mouse event callback for image displays
int YaIPS_ImageDispMouse_cb( Fl_Widget *pW, int event,
                             void *pArg1,        // Pointer to Fl_YaIPS_ImageDisp_t
                             void *pArg2);       // Optional pointer to ToolData

// Clip AOI points and check for mouse selection.
int YaIPS_ImageDispAoiClipAndCheck( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                                   int *pAoiXX = NULL, int *pAoiYY = NULL,
                                   int *pCursor = NULL, int *pDeltaAdd = NULL);

// AOI on screen handling

typedef struct {
  int XPos, YPos;             // Left upper corner of AOI
  int XSize, YSize;           // Width and height of AOI
} Fl_YaIPS_AOI_t;

// Draw an AOI
void ImageDispAoiRectDraw( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                           Fl_YaIPS_AOI_t *pAOI,                    // Pointer to AOI data
                           int Teach_mode,                          // 0 = inspection mode, 1 = teach mode
                           int IsSelected,                          // True if window is selected
                           int InspError,                           // != 0 if error in inspection. Color AOI red if Teach_mode is zero.
                           char *pText);                            // If != NULL, draw this text

// AOI rectangle clip against image size
int YaIPS_ImageDispAoiRectClip( int ImgXX, int ImgYY,       // Size of image
                                Fl_YaIPS_AOI_t *pAOI);      // Point to AOI to test

// AOI rectangle clip to image displayed on the screen
int YaIPS_ImageDispAoiRectClip( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                                Fl_YaIPS_AOI_t *pAOI);       // Point to AOI to test
// Add position change to AOI
int YaIPS_ImageDispAoiRectDeltaAdd( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                                    Fl_YaIPS_AOI_t *pAOI,       // Point to AOI to test
                                    int AoiDeltaAdd,            // Where to add mouse delta
                                    int Delta_x,                // Delta in X direction
                                    int Delta_y);               // Delta in y direction

// AOI rectangle clip and check for mouse selection.
int YaIPS_ImageDispAoiRectCC( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                              Fl_YaIPS_AOI_t *pAOI,       // Point to AOI to test
                              int *distanceToBeat,        // In Out: Distance to beat
                              int *pCursor,               // Out: Cursor shape
                              int *pDeltaAdd);            // Out: Where to add mouse delta

// AOI rectangle clip against image size and update GUI input elements of the AOI.
int YaIPS_ImageDispAoiRectIGuiUpdate( Fl_YaIPS_AOI_t *pAOI,       // Point to AOI to test
                                      int ImgXX, int ImgYY,       // Size of image
                                      void *pAOI_X,               // GUI input elements, must be a IqeFl_Int_Input pointer
                                      void *pAOI_Y,
                                      void *pAOI_XX,
                                      void *pAOI_YY);

// Empty 'StrInfo' string
void YaIPS_ImageDispStrInfo( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp); // Point to image display data

// Empty 'StrInfo' string and set colors
void YaIPS_ImageDispStrInfo( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                             Fl_Color ColInfoBgnd,                    // Color for background of info string. 0 is used for default
                             Fl_Color ColInfoText);                   // Color for text of info string. 0 is used for default color.

// Copy to 'StrInfo' string
void YaIPS_ImageDispStrInfo( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                             char *pString);                          // Pointer in string to set

// Copy to 'StrInfo' string and set colors
void YaIPS_ImageDispStrInfo( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                             Fl_Color ColInfoBgnd,                    // Color for background of info string. 0 is used for default
                             Fl_Color ColInfoText,                    // Color for text of info string. 0 is used for default color.
                             char *pString);                          // Pointer in string to set

// Print to 'StrInfo' string.
void YaIPS_ImageDispStrInfo( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                             const char * format,                     // Point to format string
                             ...);                                    // Additional arguments

// Print to 'StrInfo' string and set colors
void YaIPS_ImageDispStrInfo( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                             Fl_Color ColInfoBgnd,                    // Color for background of info string. 0 is used for default
                             Fl_Color ColInfoText,                    // Color for text of info string. 0 is used for default color.
                             const char * format,                     // Point to format string
                             ...);                                    // Additional arguments

// Empty 'StrDebug' string
void YaIPS_ImageDispStrDebug( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp); // Point to image display data

// Copy to 'StrDebug' string
void YaIPS_ImageDispStrDebug( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                              char *pString);                          // Pointer in string to set

// Print to 'StrDebug' string.
void YaIPS_ImageDispStrDebug( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                              const char * format,                     // Point to format string
                              ...);                                    // Additional arguments

// Hack: Add close button to window caption
void YaIPS_DialogAddCloseButton( Fl_Window *pFlWin);

// Hack: Remove minimize and maximize buttons from the window caption
void YaIPS_DialogRemoveMinMaxButton( Fl_Window *pFlWin);

//
// YaIPS_Utils_Pref_Win_Manag.cpp
//

// Execute this before creation of a tool windows
#define YAIPS_BEFORE_TOOL_WIN_CREATE() pGUI_Main->make_current()  // Make the main window the parent of the new window
//x/#define YAIPS_BEFORE_TOOL_WIN_CREATE() Fl_Group::current( NULL)  // Make the main window the parent of the new window

// Clip board files

#define YaIPS_CLIPBOARD_DIR   "Clipboard"                  // Name of clipboard directory
#define YaIPS_CLIPBOARD_PATH  "./Images/YaIPS/Clipboard"   // Path to clip board directory

// Preset director

#define YaIPS_PRESET_DIR_NAME   "Presets"                  // Name of preset directory
#define YaIPS_PRESET_CLIPB_DIR  ".Clipboard"               // Name of clipboard directories below preset directory

// preferences handling

extern unsigned int IqeB_PreferencesUpdateChanges_TimeLastCalled;  // remember time last called

#define PREFERENCE_MAX_GROUPS 512       // max preference groups managed

// Type of preference settign

#define PREF_T_INT       1     // Settings is of type int
#define PREF_T_FLOAT     2     // Settings is of type float
#define PREF_T_DOUBLE    3     // Settings is of type double
#define PREF_T_STRING    4     // Settings is a string

//x/#define PREF_TO_STR( a)  #a    // enclose arguments in strings

// Define a preference setting

typedef struct {
  int  Type;                  // Type of setting, see defines PREF_T_XXX
  const char *pName;          // Point to name of setting
  const char *pDefaultValue;  // String with preset value
  void *pValue;               // Pointer to variable holding value
  int   SizeOfString;         // Length string for PREF_T_STRING

  // intern use
  union {                     // Default value
    int    DVal_Int;
    float  DVal_Float;
    double DVal_Double;
  };
  union {                     // last value
    int    LVal_Int;
    float  LVal_Float;
    double LVal_Double;
   };
} T_GUI_PreferenceEntry;

/************************************************************************************
* Helper class IqeB_PreferencesGroup
* Automatic add preference settings at startup of the program.

* Add a group of preferences. Call this only once per group.
*
* Support for multiple windows
* ----------------------------
* Multiple windows look the same but hold different data.
* Requirements for using this feature are:
* - The data from the windows is stored in a table
* - References to data in the 'pSettings' Table point to the
*   first element of this table
* 'nTableElements' groups of preferences are created.
* A sequential number is added to the group name, starting with 1.
* Starting with the second group, a copy of the 'pSettings'
* table is created and the pointers to the data are adjusted.
*
*  WinSrcID               Windows source ID.
*                         One of the YAIPS_WIN_ID_XXX defines.
*  nWindows               Number of multiple windows/data table
*                         Must be > 0 for multiple windows
*  SizeOfTableElement     Size of data table elements
*                         Must be > 0 for multiple windows
*/

/** Default callback type definition for all fltk widgets (by far the most used) */
typedef void (YaIPS_WinStartup_Callback )( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx);

class IqeB_PreferencesGroup {

public:

  IqeB_PreferencesGroup( const char *pPrefName, T_GUI_PreferenceEntry *pSettings, int nSettings,
                         void **ppMyToolWin, int *pWinPosX, int *pWinPosY,
                         int WinSrcID = 0, int nWindows = 0, int SizeOfTableElement = 0,
                         int *pIsOpen = 0, YaIPS_WinStartup_Callback *pStartup = NULL, Fl_Callback *pClose = NULL,
                         const char *pGuiName = NULL, Fl_YaIPS_ImageDisp_t *pOutImage = NULL);
  ~IqeB_PreferencesGroup();
};

// ...

void IqeB_PreferencesGetFromFile();
void IqeB_PreferencesUpdateChanges();
void IqeB_PreferencesRestoreDefaults( T_GUI_PreferenceEntry *pSettingTable = NULL);

// Save the current references to a preset file
void IqeB_PresetSave_cb( Fl_Widget *pWidget, void *pValueArg);

// Load a preset file from file or reset presets
void IqeB_PresetLoad_cb( Fl_Widget *pWidget, void *pValueArg);

// Clean not used clipboard subdirectories
void IqeB_PresetCleanClipboard();

// Startup the windows from last session
void YaIPS_WindowsStartup( int DoStartup, int xLeft, int xRight, int yTop, int yBotton);

// Called on shut down of the application. Close the current open windows.
void YaIPS_WindowsShutDown( int DoStartup);

// Add offset to tool windows inside right side if right side position has changed
void YaIPS_WindowsToolWinAddPosDelta( int RightSide_X, int RightSide_Y,  // Position main window right side
                                     int RightSide_W, int RightSide_H,  // Size main window right side
                                     int DeltaX, int DeltaY);           // Move tool windows inside right side

// Used to register tool window draw after callback functions
typedef void (YaIPS_ToolWinDrawAfterCb )( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, int SubWinIDx);

// Used to register tool window change output callback functions
typedef void (YaIPS_ToolWinChangeOutputCb )( int SubWinIDx, Fl_RGB_Image *pNewimage);

// Check for a specific dialog to be open
int YaIPS_ToolWinIsOpen( int WinIdNr);                     // Tool window number

// Check the output image connected to this image input
int YaIPS_ToolWinInputCheck( int DstWinIdNr,                 // Window nr of calling tool window
                             int SrcWinIdNr,                 // IN: selected WinIdNr
                             Fl_Box *pInp_Input = NULL,      // Optional: Place for output image name
                             Fl_RGB_Image **ppImgOut = NULL, // Optional: For a valid output image return pointer to output image
                             int *pImageChanged = NULL,      // Optional: For a valid output image return 'ImageChanged'
                             char *pFileName = NULL,         // Optional: Place file name (if any) here
                             int SizeOfFileName = 0);        // Size of string for file name. Must be > 1.

// Select an input image. Also sets variables to latch image and color the window name field.
int YaIPS_ToolWinInputSelect( int DstWinIdNr,            // Window nr of calling tool window
                              int *pWinIdNr,             // OUT: selected WinIdNr
                              Fl_Button *pBut_Input,     // Button to popup the menu
                              Fl_Box *pInp_Input);       // Place for output image name

// Set a draw after function for the big image display
int YaIPS_ToolWinDrawAfterSet( int WinIdNr,                              // Tool window number
                               YaIPS_ToolWinDrawAfterCb *pBigDrawAfter); // Pointer to draw after function

// Call a draw after function from a tool window to draw into the big image display
void YaIPS_ToolWinDrawAfterCall( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                                 int WinIdNr);                            // Tool window number

// Set a change output function for the big image display
int YaIPS_ToolChangeOutputSet( int WinIdNr,                                     // Tool window number
                                   YaIPS_ToolWinChangeOutputCb *pChangeOutput); // Pointer to change output function

// Test for a change output function is set
int YaIPS_ToolChangeOutputTest( int WinIdNr);                           // Tool window number

// Call the change output callback. This sets a new output image
void YaIPS_ToolChangeOutputCall( int WinIdNr,                             // Tool window number
                                 Fl_RGB_Image *pNewimage);                // Replace output image with this window

// Refresh other than big image if linked to big image
void YaIPS_ToolWinDrawAfterRedraw( int WinIdNr);                            // Tool window number

// Call a mouse callback function from a tool window
int YaIPS_ToolWinMouseCallbackCall( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                                    int WinIdNr,                             // Tool window number
                                    Fl_Widget *pW,                           // Calling widget
                                    int event);                              // Mouse event

// Call a drop callback function from a tool window
int YaIPS_ToolWinDropCallbackCall( int WinIdNr,                             // Tool window number
                                   Fl_Widget *pW,                           // Calling widget
                                   void *pArg,                              // File name argument
                                   void *pImageDispArg);

// Check for a mouse callback / teach mode
int YaIPS_ToolWinMouseCallbackState( int WinIdNr);                           // Tool window number

// Count the children widgets
int YaIPS_ToolWinCountChildren( Fl_Double_Window *pWin);

// Dump the window table to the console
void YaIPS_ToolWinDumpWindows();

// Do test actions with the windows in the window table

#define YAIPS_TWIN_ACTION_SHOW_ALL      0 // show() all tool windows
#define YAIPS_TWIN_ACTION_HIDE_ALL      1 // hide() all tool windows
#define YAIPS_TWIN_ACTION_ICONIZE_ALL   2 // iconize() all tool windows
#define YAIPS_TWIN_ACTION_HIDE_SHOW_ALL 3 // hide() + show() all tool windows
#define YAIPS_TWIN_ACTION_REDRAW_ALL    4 // redraw() all tool windows

#ifdef _DEBUG
void YaIPS_ToolWinTestAction_cb( Fl_Widget *pWidget, void *pValueArg);
#endif
void YaIPS_ToolWinTestAction( int Action, int MainMakeTopWindow);

//
// YaIPS_Utils_Calibration.cpp
//

// Values for calibration mode
#define YAIPS_CALIB_MODE_IMAGE           0     // Use a file or camera image for calibration
#define YAIPS_CALIB_MODE_DPI             1     // Use a DPI value for calibration

// Values for calibration unit
#define YAIPS_CALIB_UNIT_PIXEL           0     // Units are pixel
#define YAIPS_CALIB_UNIT_MM              1     // Units are mm
#define YAIPS_CALIB_UNIT_CM              2     // Units are cm
#define YAIPS_CALIB_UNIT_INCH            3     // Units are inch


// Minimum maximum sizes for GUI elements.
// Maximum sizes are oriented after max size of DIN A0

#define YAIPS_CALIB_MIN_VAL_PIXEL        8     // Minimum value for size in pixel
#define YAIPS_CALIB_MAX_VAL_PIXEL    15000     // Maximum value for size in pixel

#define YAIPS_CALIB_MIN_VAL_MM        10.0     // Minimum value for size in pixel
#define YAIPS_CALIB_MAX_VAL_MM      1200.0     // Maximum value for size in pixel

#define YAIPS_CALIB_MIN_VAL_CM         1.0     // Minimum value for size in pixel
#define YAIPS_CALIB_MAX_VAL_CM       120.0     // Maximum value for size in pixel

#define YAIPS_CALIB_MIN_VAL_INCH       1.0     // Minimum value for size in pixel
#define YAIPS_CALIB_MAX_VAL_INCH      50.0     // Maximum value for size in pixel

// Other defines

#define YAIPS_CALIB_MIN_VAL_DPI         50     // Minimum value for DPI input field
#define YAIPS_CALIB_MAX_VAL_DPI        300     // Maximum value for DPI input field

#define YAIPS_CALIB_MM_PER_INCH  25.4          // One inch has 25.4 mm
#define YAIPS_CALIB_CM_PER_INCH  2.54          // One inch has 2.54 cm

// Global calibration variables

extern double YaIPS_Calib_UPP_X;      // Calibration factor in X [Units/Pixel]
extern double YaIPS_Calib_UPP_Y;      // Calibration factor in Y [Units/Pixel]

extern double YaIPS_Calib_Image_X;    // Calibration factor in X for image mode [Units/Pixel]
extern double YaIPS_Calib_Image_Y;    // Calibration factor in Y for image mode [Units/Pixel]

extern int YaIPS_Calib_Mode;          // Calibration mode
extern int YaIPS_Calib_Unit;          // Calibration unit 0 = pixel, 1 = mm, 2 = cm ...
extern int YaIPS_Calib_DPI;           // Calibration DPI (dots per inch)

//  Ensure proper afterpoint digits for a unit value using a specific calibration unit
double YaIPS_Calib_FixUnitsAfterPoint( double ValUnit, int Calib_Unit);

// Ensure proper afterpoint digits for a unit value using the global calibration unit
double YaIPS_Calib_FixUnitsAfterPoint( double ValUnit);

// Convert pixel value to units
double YaIPS_Calib_PixVal2Units( int ValPixel, int Calib_Unit, double UnitsPerPixel, int RoundToUnits);

// Convert pixel X value to units using the global calibration variables
double YaIPS_Calib_PixXVal2Units( int ValPixel, int RoundToUnits);

// Convert pixel Y value to units using the global calibration variables
double YaIPS_Calib_PixYVal2Units( int ValPixel, int RoundToUnits);

// Convert unit value to pixels
int YaIPS_Calib_UnitVal2Pixel( double ValUnit, double UnitsPerPixel);

// Convert unit X value to pixel using the global calibration variables.
int YaIPS_Calib_UnitXVal2Pixel( double ValUnit);

// Convert unit X value to pixel using the global calibration variables.
int YaIPS_Calib_UnitYVal2Pixel( double ValUnit);

// Recalculate calibration factors YaIPS_Calib_UPP_X and YaIPS_Calib_UPP_Y
int YaIPS_Calib_Recaclc_Factors();

// Return unit string for given calibration unit
char *pYaIPS_Calib_Unit2String( int Calib_Unit);

// Return unit string for global calibration unit
char *pYaIPS_Calib_Unit2String();

// Convert pixel value to units and append the unit
char *pYaIPS_Calib_PixelVal2UnitStr( int ValPixel, int Calib_Unit, double UnitsPerPixel);

// Convert pixel X value to units and append the unit
char *pYaIPS_Calib_PixelXVal2UnitStr( int ValPixel);

// Convert pixel Y value to units and append the unit
char *pYaIPS_Calib_PixelYVal2UnitStr( int ValPixel);

// Modify float values by change of the unit
void YaIPS_Calib_Change_Unit( float *pX, float *pY, int Source_Unit, int Target_Unit);

// Setup two GUI input elements for use with a specific calibration unit.
void YaIPS_Calib_SetModifyDataAndValue( void *pGuiXValue,     // IqeFl_Float_Input GUI input element for X value
                                        void *pGuiYValue,     // IqeFl_Float_Input GUI input element for Y value
                                        float XValue,         // Set this X value to the X GUI element
                                        float YValue,         // Set this Y value to the Y GUI element
                                        int IsPosition,       // If != 0: is an position value, minimum is set to 0
                                        int Calib_Unit);      // Use this calibration unit

// Setup two GUI input elements for use with the global calibration unit.
void YaIPS_Calib_SetModifyDataAndValue( void *pGuiXValue,     // IqeFl_Float_Input GUI input element for X value
                                        void *pGuiYValue,     // IqeFl_Float_Input GUI input element for Y value
                                        float XValue,         // Set this X value to the X GUI element
                                        float YValue,         // Set this Y value to the Y GUI element
                                        int IsPosition);      // If != 0: is an position value, minimum is set to 0

//
// YaIPS_Utils_ColorMod.cpp
//

extern char * YaIPS_ColMod_LUT_table[];      // Table of names of predefined LUTs
extern int YaIPS_ColMod_nLUT_tables;         // Number of predefined LUTs

// Calculate lookup table
int YaIPS_ColModCalcLUT( Fl_YaIPS_ColMod_t *pDisplayColMod, int ForceStdLut);

// Recalculate LUT if an update is needed.
void YaIPS_ColModCheckLUT( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, int ForceUpdate);

// Draw LUT
void YaIPS_ColMod_LUT_Draw( YaIPS_Fl_Box *pYaIPS_Box,          // Draw into this box widget
                           Fl_YaIPS_ColMod_t *pDisplayColMod,  // Pointer to LUT data
                           int RefX, int RefY,                 // Left upper reference point for drawing
                           int DrawBackground = false);        // If true, draw a background

//
// YaIPS_Utils_ColRowSum.cpp
//

#define YAIPS_CRSUM_FLAG_COLSUM        0x00001 // Make a column sum profile
#define YAIPS_CRSUM_FLAG_ROWSUM        0x00002 // Make a row sum profile
#define YAIPS_CRSUM_FLAG_NORMALIZE     0x00004 // Normalize summed up values by height/width
#define YAIPS_CRSUM_FLAG_RESET         0x00008 // Reset the data envelope to zero. Else must be zeroed before first use.

// Data structure for column/row summation profiles
typedef struct {                                     // Data element for a single component histogram
  int nElements;                                     // number of elements in data vector
  int *pData;                                        // If != NULL, allocated vector of data
  int ValueMax;                                      // Maximum value in vector
  int ValueMin;                                      // Maximum value in vector
} Fl_YaIPS_ColRowSum_t;

typedef struct {                                     // Data element for RGB profiles
  int nProfiles;                                     // 0 = no histogram, 1 = gray 3 = RGB profiles
  int Flags;                                         // Flag bits, see YAIPS_CRSUM_FLAG_XXX defines
  Fl_YaIPS_ColRowSum_t R;                             // Profile for red component color or gray
  Fl_YaIPS_ColRowSum_t G;                             // Profile for green component color
  Fl_YaIPS_ColRowSum_t B;                             // Profile for blue component color
} Fl_YaIPS_ColRowSum_RGB_t;

// Make column/row sum profiles from an image

int YaIPS_ColRowSum_Measure( Fl_RGB_Image *pImage_Img,                  // Pointer to image envelop
                            Fl_YaIPS_ColRowSum_RGB_t *pColRowSum_RGB,   // Pointer to RGB profiles
                            int Flags,                                 // Flag bits, see YAIPS_CRSUM_FLAG_XXX defines
                            int AoiX, int AoiY,                        // AOI rectangle start point
                            int AoiW, int AoiH);                       // AOI rectangle size

// Draw a column / row sum profile
void YaIPS_ColRowSum_Draw( Fl_YaIPS_ColRowSum_RGB_t *pColRowSum_RGB,   // Pointer to RGB profiles
                           int x1, int y1,                            // Left upper drawing reference point
                           int w, int h,                              // With / height of drawing box
                           int LenghtSample, int LengthFirst);        // Length of all/first sample to draw

//
// YaIPS_Utils_EdgeAOI.cpp
//

// Single AOI for edge measurement

typedef struct {
  // GUI Parameter
  Fl_YaIPS_AOI_t AOI;             // The measurement AOIs
  int r2l_b2t;                    // 0: left->right or top->bottom, 1: right->left or bottom->top
  int minPeakConPerc;             // minimum peak contrast [%] of ref
#ifdef use_again  // 09.07.2025 RR: Replaced nominal position by 1/2 search length
  int nomPos;                     // DM subwin relative nominal position of ref. peak (for graphics and peak search)
#endif
  // teach results
  int CheckError;                 // Result of last teach, 0 == OK.
  int colorUsed;                  // color channel used for color processing (IHS)
  int minMax;                     // 0: search minimum peak, 1: search maximum peak
  int refPeakCon;                 // peak contrast in reference
  float refPeak;                  // DM subwin relative position of ref. peak (for graphics and nominal value out of ref)
  // result of last inspection
  int LastPeakCon;                // peak contrast of last edge found
  float LastPeak;                 // DM subwin relative position of ref. peak (for graphics and nominal value out of ref)
} YaIPS_EdgeAOI_t;

// Two edge AOIs to measure distance

typedef struct {
  // GUI Parameter
  int   nomOutOfRef;            // take nominal value out of measured value in reference
  int   orientation;            // 0: x-distance, 1: y-distance, 2: orientation is given parameter
  int   OrientationAngle;       // orientation angle for mode (180 .. - 180 degree)
  int   DifferenceMeas;         // != 0 to measure difference
  float nomVal;                 // nominal value
  float pTol;                   // positive tolerance
  float nTol;                   // negative tolerance
  // teach results
  float refDist;                // Distance of last inspection if CheckError is 0.
  // result of last inspection
  int   CheckError;             // Result of last inspection code, 0 == OK.
  char  CheckText[ 64];         // Last check text

  YaIPS_EdgeAOI_t SW_1, SW_2;  // Two sub windows

} YaIPS_EdgeDM_t;

// One to three edge AOIs to measure position deviation

#define YAIPS_EDGEPM_MAX_AOIS    3    // Max number of AOIs used

// Defines for measurement mode

#define YAIPS_EDGEPM_MODE_X       0    // Position correction with edges, 1 X, only in x direction
#define YAIPS_EDGEPM_MODE_Y       1    // Position correction with edges, 1 Y, only in y direction
#define YAIPS_EDGEPM_MODE_XY      2    // Position correction with edges, 1 X 1 Y, first x than y correction
#define YAIPS_EDGEPM_MODE_YX      3    // Position correction with edges, 1 Y 1 X, first y than x correction
#define YAIPS_EDGEPM_MODE_XYY     4    // Position correction with edges, 1 X 2 Y, position and rotation correction
#define YAIPS_EDGEPM_MODE_XXY     5    // Position correction with edges, 2 X 1 Y, position and rotation correction
#define YAIPS_EDGEPM_MODE_MAX     6    // Number of  modes

typedef struct {
  // GUI Parameter
  int   MeasureMode;            // Measurement mode
  float MaxDevX;                // Max position deviation in X
  float MaxDevY;                // Max position deviation in X
  // teach results
  float DeltaX, DeltaY, DeltaA; // Position and angle deltas
  YaIPS_dMatrix_t T_Matrix;     // Transformation matrix
  // result of last inspection
  int   CheckError;             // Result of last inspection code, 0 == OK.
  char  CheckText[ 64];         // Last check text

  YaIPS_EdgeAOI_t Edges[ YAIPS_EDGEPM_MAX_AOIS];    // Three sub windows

} YaIPS_EdgePM_t;

// Inspection error codes returned from YaIPS_EdgeDM_Inspect()

#define YAIPS_EDGEDM_ERR_INSP_TOL   1   // Tolerance error. Must be value 1.
#define YAIPS_EDGEDM_ERR_NO_TEACH   2   // No teach
#define YAIPS_EDGEDM_ERR_IMBORDER   3   // any window touches image border
#define YAIPS_EDGEDM_ERR_1_IMBORDER 4   // .1 window touches image border
#define YAIPS_EDGEDM_ERR_2_IMBORDER 5   // .2 window touches image border
#define YAIPS_EDGEDM_ERR_3_IMBORDER 6   // .3 window touches image border
#define YAIPS_EDGEDM_ERR_ENF        7   // any edge not found
#define YAIPS_EDGEDM_ERR_1_ENF      8   // .1 edge not found
#define YAIPS_EDGEDM_ERR_2_ENF      9   // .2 edge not found
#define YAIPS_EDGEDM_ERR_3_ENF     10   // .3 edge not found
#define YAIPS_EDGEDM_ERR_BOTH_ENF  11   // no edge found
#define YAIPS_EDGEDM_ERR_ILLTRAFO  12   // illegal trafo system

// Get edges for distance measurement
int YaIPS_EdgeDM_Inspect( Fl_RGB_Image *pSrc,          // Input image
                          YaIPS_EdgeDM_t *pEdgeDM,     // Point to edge DM data
                          int Teach_mode);             // 0 = inspection mode, 1 = teach mode

// Get edges for position measurement
int YaIPS_EdgePM_Inspect( Fl_RGB_Image *pSrc,          // Input image
                          YaIPS_EdgePM_t *pEdgePM,     // Point to edge PM data
                          int Teach_mode);             // 0 = inspection mode, 1 = teach mode

// Draw an edge AOI
void YaIPS_EdgeAOI_Draw( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                         YaIPS_EdgeAOI_t *pEdgeAOI,    // Pointer to EdgeAOI data
                         int Teach_mode,               // 0 = inspection mode, 1 = teach mode
                         Fl_Color DrawColor,           // Color for drawing
                         char *pText,                  // Text for caption
                         int   orientation,            // 0: x-distance, 1: y-distance, 2: orientation is given parameter
                         int   OrientationAngle = 0);  // orientation angle for mode (180 .. - 180 Grad)

// EdgeAOI rectangle clip and check for mouse selection.
int YaIPS_EdgeAOI_MouseCC( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                           YaIPS_EdgeAOI_t *pEdgeAOI,  // Pointer to EdgeAOI data
                           int orientation,            // 0: x-distance, 1: y-distance, 2: orientation is given parameter
                           int OrientationAngle,       // orientation angle for mode (180 .. - 180 Grad)
                           int *distanceToBeat,        // In Out: Distance to beat
                           int *pCursor,               // Out: Cursor shape
                           int *pDeltaAdd);            // Out: Where to add mouse delta

// Add position change to EdgeAOI
int YaIPS_EdgeAOI_DeltaAdd( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                            YaIPS_EdgeAOI_t *pEdgeAOI,  // Pointer to EdgeAOI data
                            int orientation,            // 0: x-distance, 1: y-distance, 2: orientation is given parameter
                            int OrientationAngle,       // orientation angle for mode (180 .. - 180 Grad)
                            int AoiDeltaAdd,            // Where to add mouse delta
                            int Delta_x,                // Delta in X direction
                            int Delta_y);               // Delta in y direction

//
// YaIPS_Utils_Histo.cpp
//

#define YAIPS_HISTO_N_POINTS   256                    // Number of entries in a histogram

typedef struct {                                      // Data element for a single component histogram
  int HistoTable[ YAIPS_HISTO_N_POINTS];              // Histogram table
  int HistMax;                                        // Maximum value in table
  int nPoints;                                        // number of measured pixels (is sum of table elements)
  float Average;                                      // Average value
  float StdDev;                                       // Standard deviation
} Fl_YaIPS_Histo_t;

typedef struct {                                      // Data element for a RGB histogram
  int nHistos;                                        // 0 = no histogram, 1 = gray 3 = RGB histogram
  Fl_YaIPS_Histo_t R;                                 // Histogram for red component color or gray
  Fl_YaIPS_Histo_t G;                                 // Histogram for green component color
  Fl_YaIPS_Histo_t B;                                 // Histogram for blue component color
  int MarkPos1, MarkPos2, MarkPos3;                   // Position and color for up to three vertical lines.
  Fl_Color MarkCol1, MarkCol2, MarkCol3;              // The lines are drawn only if the colors are != 0.
} Fl_YaIPS_Histo_RGB_t;

// Measure histogram data from an image
void YaIPS_Histo_Measure( Fl_RGB_Image *pImage_Img,          // Pointer to image envelop
                          Fl_YaIPS_Histo_RGB_t *pHisto_RGB,  // Pointer to RGB histogram
                          int AoiX = 0, int AoiY = 0,        // AOI rectangle start point
                          int AoiW = 0, int AoiH = 0);       // AOI rectangle size

// Measure histogram data from an image
void YaIPS_Histo_Measure2( Fl_RGB_Image *pImage_Img,          // Pointer to image envelop
                           Fl_YaIPS_Histo_RGB_t *pHisto_RGB,  // Pointer to RGB histogram
                           int ColorSpace,                    // In: Color space for color images NORMAL, BW, R, G or B
                           int StepFast = 0,                  // In: If > 1 speed up measurement by using less pixels
                           int AoiX = 0, int AoiY = 0,        // AOI rectangle start point
                           int AoiW = 0, int AoiH = 0);       // AOI rectangle size

// Draw histogram
void YaIPS_Histo_Draw( YaIPS_Fl_Box *pYaIPS_Box,          // Draw into this box widget
                       Fl_YaIPS_Histo_RGB_t *pHisto_RGB,  // Pointer to RGB histogram
                       int RefX, int RefY,                // Left upper reference point for drawing
                       int CurvesYY,                      // Height of curves
                       char *pHeader1 = NULL,             // Optional header text
                       char *pHeader2 = NULL,             // Optional header text
                       Fl_Color OW_Line = 0,              // If != 0 use this as line color
                       Fl_Color OW_Text = 0,              // If != 0 use this as text color
                       Fl_Color OW_CurveBW = 0);          // If != 0 use this as color for a BW histogram

#define YAIPS_HISTO_MODE_MEAN     0  // mean value
#define YAIPS_HISTO_MODE_MIN      1  // n% min value
#define YAIPS_HISTO_MODE_MAX      2  // n% max value

int YaIPS_Histo_GetVal( Fl_RGB_Image *pImage_Img,         // Pointer to image envelop
                        Fl_YaIPS_AOI_t *pAOI,             // The measurement AOI
                        int Mode,                         // Average/Min/Max
                        int Percent,                      // Min/Max percent value
                        int *pValR,                       // Place red value here
                        int *pValG,                       // Place green value here
                        int *pValB);                      // Place blue value here

//
// YaIPS_Utils_ImageCompare.cpp
//

typedef struct {
  // GUI Parameter
  Fl_YaIPS_AOI_t AOI;             // The measurement AOI
  int   GWToleranceHi;            // in gray values
  int   GWToleranceLo;            // in gray values
  float MinBlobArea;              // in units minimum blob area
  float MinLBlobArea;             // in units minimum large blob area
  float SumBlobArea;              // in units maximum sum area of all blobs
  int   FlagDarken;               // If set, darken the image. This makes error markings more visible.
  int   FlagShowBlobArea;         // If set, show area of small and large blobs
  // teach results

  //x/ Nothing is teached

  // result of last inspection
  int   InspError;                // Result of last inspection, 0 == OK.
  char  InspText[ 128];           // Last inspection text
  int   numLightErrPix;           // number of light error pixels PASS 1
  int   numDarkErrPix;            // number of dark error pixels PASS 1
  int   numBlobs;                 // number of large blobs
  int   numLargeBlobs;            // number of large blobs
  float sumBErrArea;              // error sum area of blobs in units * units
  float sumLBErrArea;             // error sum area of large blobs in units * units
  float maxBErrArea;              // maximum blob error area in units * units
  float maxLBErrArea;             // maximum large blob error area in units * units
  float maxSErrArea;              // maximum small error area in units * units

  // Temporary work data

  void *pWorkData;

} YaIPS_CompareAOI_t;

// Free allocated work data at close of window
int YaIPS_ImageCompare_WorkDataFree( YaIPS_CompareAOI_t *pCompareAOI);

// Get edges for distance measurement
int YaIPS_ImageCompare_Inspect( Fl_RGB_Image **ppOut,            // Output image
                                Fl_RGB_Image *pSrc,              // Input image
                                Fl_RGB_Image *pRef,              // Reference image
                                YaIPS_CompareAOI_t *pCompareAOI, // Point to image compare data
                                int Teach_mode);                 // 0 = inspection mode, 1 = teach mode

// Show area of small and large blobs
int YaIPS_ImageCompare_DrawArea( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                                 YaIPS_CompareAOI_t *pCompareAOI,         // Point to image compare data
                                 int Teach_mode);                         // 0 = inspection mode, 1 = teach mode

//
// YaIPS_Utils_ImageFile.cpp
//

// Read image from file
Fl_RGB_Image * YaIPS_Image_Read( char *pFileName);

// Write image to file
int YaIPS_Image_Write( char *pFileName, Fl_RGB_Image *pImg);

// Write image to file
int YaIPS_Image_Write( char *pFileName, const unsigned char *pixels, int w, int h, int d);

// Write PNG image to file
int YaIPS_Image_Write_PNG( char *pFileName, Fl_RGB_Image *pImg);

// Write PNG image to file
int YaIPS_Image_Write_PNG( char *pFileName, const unsigned char *pixels, int w, int h, int d, int ld = 0);

// Get all image files in a directory
int YaIPS_Image_GetFilesInDir( char *pDirName, struct dirent ***pFileList, int *pNumFiles);

//
// YaIPS_Utils_Language.cpp
//

// Free language data
void Lang_FreeData();

// Load translations for a specific language
int LangLoadTranslations( char *pLanguage, int NewLineCharForMultipleStringLines = false);

// Load translations for last selected language
void Lang_Init( int NewLineCharForMultipleStringLines = false);

// Lookup a language string
char * LangStringLookup( char *pString);
char * LangStringLookup( const char *pString);

//
// YaIPS_Utils_PSearchAOI.cpp
//

// Single AOI for pattern search

typedef struct {
  // GUI Parameter
  Fl_YaIPS_AOI_t AOI;             // The measurement AOIs
  float PosToleranceX, PosToleranceY; // Search range in x and y [Units]
  // teach results
  int Flags;                      // Intern used
  int CorrNorm;                   // correlation norm factor
  int colorUsed;                  // color channel used for color processing (IHS)
  // result of last check/teach
  int  CheckError;                // Result of last teach, 0 == OK.
  int  CheckCorrQual;             // Teach correlation quality
  char CheckText[ 64];            // Last check text
  // result of last inspection
  int  InspError;                 // Result of last inspection, 0 == OK.
  char InspText[ 64];             // Last inspection text
  int  InspQual;                  // Inspection quality
  float LastDX, LastDY;           // Last position deviation (for graphics and nominal value out of ref) [Pixel]
} YaIPS_PSearchAOI_t;

// One to three edge AOIs to measure position deviation

#define YAIPS_PSEARCH_MAX_AOIS    3   // Max number of AOIs used

// Defines for pattern search mode

#define YAIPS_PSEARCH_MODE_1XY   0    // Position correction with search pattern, 1 XY sub-window
#define YAIPS_PSEARCH_MODE_3XY   1    // Position correction with search pattern, 3 XY sub-windows
#define YAIPS_PSEARCH_MODE_MAX   2    // Number of modes

typedef struct {
  // GUI Parameter
  int   MeasureMode;            // Measurement mode
  float MaxDevX;                // Max position deviation in X
  float MaxDevY;                // Max position deviation in X
  // teach results
  float DeltaX, DeltaY, DeltaA; // Position and angle deltas
  YaIPS_dMatrix_t T_Matrix;     // Transformation matrix
  // result of last inspection
  int   CheckError;             // Result of last inspection code, 0 == OK.
  char  CheckText[ 64];         // Last check text

  YaIPS_PSearchAOI_t AOI[ YAIPS_PSEARCH_MAX_AOIS];    // Three sub windows

  // Temporary work data

  void *pWorkData;

} YaIPS_PSearchXY_t;

// Free allocated work data at close of window
int YaIPS_PSearchAOI_WorkDataFree( YaIPS_PSearchXY_t *pWin);

// Teach all AOIs
int YaIPS_PSearchAOI_Teach( Fl_RGB_Image *piref, YaIPS_PSearchXY_t *pPSearchXY);

// Inspect
int YaIPS_PSearchAOI_Inspect( Fl_RGB_Image *pImgIn,            // Input image
                              Fl_RGB_Image *piref,             // Reference image
                              YaIPS_PSearchXY_t *pWin);        // Search parameter

// Draw a position search AOI
void YaIPS_PSearchAOI_Draw( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                            YaIPS_PSearchAOI_t *pPSearchAOI, // Pointer to PSearchAOI data
                            int Teach_mode,               // 0 = inspection mode, 1 = teach mode
                            int IsSelected,               // True if window is selected
                            int iAOI);                    // What AOI, is zero based

//
// YaIPS_Utils_RotatableAOI.cpp
//

#define ACTION_FA_MASK_NR     0x0003    // Bit Mask for Line Number
#define ACTION_FA_CHANGE_XX   0x0004
#define ACTION_FA_CHANGE_YY   0x0008

typedef struct {

  // set from getWinCoordinates ...

  double CenterX, CenterY;             // Center of AOI, is also the rotation center
  double OrientationAngle;             // orientation angle

  // set from getWinMoreData ...

  int TextXPosHigh, TextYPosHigh;      // Top edge position for header text
  int TextXPosLow, TextYPosLow;        // Bottom edge position for header text
  Fl_YaIPS_AOI_t SurroundingRectangle; // surrounding rectangle
  double xP[ 4], yP[ 4];               // the rectangle points, succeding points are on a line

  int CursorPointShape[ 4];            // cursor shape for the corner points
  int CursorPointAction[ 4];           // cursor action for the corner points

  int CursorLineShape[ 4];             // cursor shape for the rectangle lines
  int CursorLineAction[ 4];            // cursor action for the rectangle lines

} T_FreeAngleInfo;

int YaIPS_RotatableAOI_Coordinates( T_FreeAngleInfo * pFreeAngleInfo, Fl_YaIPS_AOI_t *pWinDesc,
                              double OrientationAngleArg,
                              double *retx0 = NULL, double *rety0 = NULL, double *retx1 = NULL, double *rety1 = NULL,
                              double *retxA = NULL, double *retxB = NULL, double *retxC = NULL, double *retxD = NULL,
                              double *retyA = NULL, double *retyB = NULL, double *retyC = NULL, double *retyD = NULL,
                              int *retnSample = NULL, int *retnQuer = NULL,
                              double *pdxScanDir = NULL, double *pdyScanDir = NULL,
                              double *pdxSideDir = NULL, double *pdySideDir = NULL);

void YaIPS_RotatableAOI_Draw( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                              Fl_YaIPS_AOI_t *pWinDesc,     // Pointer to AOI data
                              Fl_Color DrawColor,           // Color for drawing
                              char *pText,                  // Text for caption
                              float OrientationAngle);      // orientation angle

// Rotatable AOI rectangle clip and check for mouse selection
int YaIPS_RotatableAOI_MouseCC( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                           Fl_YaIPS_AOI_t *pWinDesc,   // Pointer to AOI data
                           float OrientationAngle,     // orientation angle
                           int *distanceToBeat,        // In Out: Distance to beat
                           int *pCursor,               // Out: Cursor shape
                           int *pDeltaAdd);            // Out: Where to add mouse delta

// Add position change to rotatable AOI
int YaIPS_RotatableAOI_DeltaAdd( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                                 Fl_YaIPS_AOI_t *pAOI,       // Pointer to AOI data
                                 float OrientationAngle,     // orientation angle
                                 int AoiDeltaAdd,            // Where to add mouse delta
                                 int Delta_x,                // Delta in X direction
                                 int Delta_y,                // Delta in y direction
                                 int RatioLocked = 0,        // True for locked size ratio
                                 int RatioXX = 0,            // Used as size ratio reference
                                 int RatioYY = 0);

// Clip rotatable AOI against image boundaries.
//The center of the AOI is clipped to stay inside the image.
int YaIPS_RotatableAOI_Clip( int ImgXX, int ImgYY,       // Size of image
                             Fl_YaIPS_AOI_t *pAOI);      // Point to AOI to test

// AOI rectangle clip against image boundaries and update GUI input elements of the AOI.
// The center of the AOI is clipped to stay inside the image.
int YaIPS_RotatableAOI_ClipGuiUpdate( Fl_YaIPS_AOI_t *pAOI,       // Point to AOI to test
                                      int ImgXX, int ImgYY,       // Size of image
                                      void *pAOI_X_Arg,           // GUI input elements, must be a IqeFl_Int_Input pointer
                                      void *pAOI_Y_Arg,
                                      void *pAOI_XX_Arg,
                                      void *pAOI_YY_Arg);

//
// YaIPS_GUI_VideoRead.cpp
//

// Open video read window
void IqeB_GUI_VideoReadWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_VideoRead.cpp
//

// Open video write window
void IqeB_GUI_VideoWriteWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx); // Open dialog

//
// YaIPS_GUI_Main.cpp
//

extern Fl_Double_Window *pGUI_Main;                   // Pointer to main window

extern int YaIPS_Main_WinPosX, YaIPS_Main_WinPosY;    // Last main window position
extern int YaIPS_Main_WinSizeX, YaIPS_Main_WinSizeY;  // Last main window size

extern Fl_YaIPS_ImageDisp_t YaIPS_BigImageDisp;       // Info big image display on main window

extern int YaIPS_Main_ImageSourceID_Last;             // Last used image source ID. Is used for startup from last session

extern int YaIPS_Main_Measured_Distance;              // > 0 for a measured distance YAIPS_SHOW_INFO_2P_DIST_XXX [Pixel]

extern Fl_YaIPS_Histo_RGB_t YaIPS_Main_Histo;         // Histogram of current big image for YAIPS_SHOW_INFO_RE_HISTO_ALL / YAIPS_SHOW_INFO_RE_HISTO_AOI

extern Fl_Box *pGUI_Main_RightSide;                   // Box for right side (used to place tool windows)

// Make the main window the top window
void IqeB_GUI_MainMakeTopWindow();

//x/void IqeB_GUI_MainOpensToolWindow( void *pToolWinFunc, void *pCallerWin);
//x/void IqeB_GUI_OpenGLUpdateGUISettingVariable( int *pValue, int NewValue);  // Update variable and GUI element
void IqeB_GUI_SetWindowTitle( char *pTitle);

// Change activation state of a widget
int IqeB_GUI_WidgetActivate( void *wArg, int ActivateIt);    // Set widget activated/inactive

// Change  color of widget
int IqeB_GUI_WidgetColor( void *wArg, Fl_Color NewColor);

// Change label color of widget
int IqeB_GUI_WidgetLabelColor( void *wArg, Fl_Color NewColor);

void IqeB_GUI_CloseToolWindow( void **ppMyToolWin);

// Popup a dialog to choose a color
Fl_Color IqeB_GUI_ColorChooser( Fl_Color OldColor);

/************************************************************************************
 * Other includes which includes which depends on declarations in this file
 */

#include "../Src_Utils/YaIPS_Utils_FLTK.h"
#include "../Src_IPS/YaIPS_IPS_Interface.h"
#include "../Src_RGB/YaIPS_RGB_Interface.h"
#include "YaIPS_LanguageStrings.h"  // Language string definitions

#endif /* YAIPS_H_ */

/****************************** End Of File ******************************/
