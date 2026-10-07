/****************************************************************************

  YaIPS_GUI_Main.cpp

  03.01.2025 RR: First edition of this file.
  03.09.2026 RR: * Function main()
                   * Ensure that 'YaIPS_WorkingDirectory' has normalized
                     path characters.
                   * Ensure clip board directory is created
                 * IqeB_Main_SaveCopy_Callback()
                   Ensure normalized path characters and working directory
                   for files to save.
  08.09.2026 RR: * Added menu entry 'File/Reset presets'.
                   Call the call back IqeB_Main_PresetLoad() with the
                   argument 'pValueArg' set to 1.
                 * IqeB_Main_PresetLoad()
                   Handle 'pValueArg' over to call of function IqeB_PresetLoad_cb().
                 * Function main()
                   Added call to IqeB_PresetCleanClipboard().
                   Clean not used clipboard subdirectories.
  23.09.2026 RR: * IqeB_MainWindow_GUI_Setup().
                   Added 'Reset' button into right upper corner of
                   the windows left area.
                   Pressing the button resets the display settings.

*****************************************************************************
*/

//x/#define USE_LOGFILE   1          // Define this to use a log file for startup testing

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sysinfoapi.h>

// Other includes
#include "YaIPS.h"

#include <FL/Fl_Copy_Surface.H>
#include <FL/Fl_Image_Surface.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/platform.H>

/************************************************************************************
 * Some forwards
 */

static void IqeB_GUI_ToolsMyIdleAction( void *);    // Manage display if big image in image box and GUI update.
static void IqeB_GUI_OpenGLIdleAction( void *);     // Periodically save presets and ...
static void MyWinUpdate();                          // Update GUI of this tool window
static int YaIPS_GUI_DiffUsesCalcThings();           // Calculate things to draw by YaIPS_GUI_DiffUsesDrawAfter_cb or YaIPS_GUI_BigDrawAfter_cb

/************************************************************************************
 * Variables for GUI (widget pointers, state variables, ...)
 * Part 1.
 */

int  YaIPS_GUI_Main_Do_Startup = false;                // Used during startup of tool windows. True during startup phase.
char YaIPS_WorkingDirectory[ MAX_FILENAME_LEN];        // Working directory. Is set at startup of application.
char YaIPS_BrowserDirectory[ MAX_FILENAME_LEN];        // Path of current directory for file browsers
char YaIPS_BrowserDirVideos[ MAX_FILENAME_LEN]; // Path of current directory for video file browsers

// Global variables ...

int IeqB_GUI_ResourceBits = 0;                        // Hold locked resources

// Module local variables ...

static char GUI_StartupFile[ MAX_FILENAME_LEN];       // file to load after startup
static unsigned int GUI_BackgroundColor;              // Remember for GUI call

/************************************************************************************
 * Log-file support.
 */

#ifdef USE_LOGFILE          // Use a log file

static FILE *pLogFile = NULL;   /* test-log on logfile */

#define FOPEN()                        Logfile_Open()
#define FCLOSE()                       if( logf != NULL) { fclose( pLogFile); pLogFile = NULL;}

#define FPRINTF0( p0)                  if( pLogFile != NULL) { fprintf( pLogFile,p0); fflush( pLogFile); }
#define FPRINTF1( p0,p1)               if( pLogFile != NULL) { fprintf( pLogFile,p0,p1); fflush( pLogFile); }
#define FPRINTF2( p0,p1,p2)            if( pLogFile != NULL) { fprintf( pLogFile,p0,p1,p2); fflush( pLogFile); }
#define FPRINTF3( p0,p1,p2,p3)         if( pLogFile != NULL) { fprintf( pLogFile,p0,p1,p2,p3); fflush( pLogFile); }
#define FPRINTF4( p0,p1,p2,p3,p4)      if( pLogFile != NULL) { fprintf( pLogFile,p0,p1,p2,p3,p4); fflush( pLogFile); }
#define FPRINTF5( p0,p1,p2,p3,p4,p5)   if( pLogFile != NULL) { fprintf( pLogFile,p0,p1,p2,p3,p4,p5); fflush( pLogFile); }

static void Logfile_Open()
{

  if( pLogFile == NULL) {

    pLogFile = fopen( "Logilfe-YaIPS.txt", "wt");
  }

#ifdef use_again
  if( pLogFile == NULL) {

    fprintf(stderr, "Logfile_Open: can't open logfile\n"); exit(1);
  }
#endif
}

#else

#define FOPEN()
#define FCLOSE()

#define FPRINTF0( p0)
#define FPRINTF1( p0,p1)
#define FPRINTF2( p0,p1,p2)
#define FPRINTF3( p0,p1,p2,p3)
#define FPRINTF4( p0,p1,p2,p3,p4)
#define FPRINTF5( p0,p1,p2,p3,p4,p5)

#endif

/************************************************************************************
 * IqeB_GUI_CloseToolWindow
 *
 * * Updates tool windows positions to preferences cache
 * * Close tool window dialog
 * * Resets pointer to tool window dialog to NULL
 * * Unregister callbacks
 */

void IqeB_GUI_CloseToolWindow( void **ppMyToolWin)
{
  Fl_Double_Window *pThisWin;
  int WasModal;

  // Update preferences, this also stores window positions
  // to the preferences cache

  IqeB_PreferencesUpdateChanges();  // update preferences database and save to to file

  // Close tool window dialog

  pThisWin = (Fl_Double_Window *)*ppMyToolWin; // get pointer to tool

  WasModal = pThisWin->modal();                // Was this window modal

  delete pThisWin;

  // * Resets pointer to tool window dialog to NULL

  *ppMyToolWin = NULL;

#ifdef use_again
  pGUI_Main->show();
#else

  // 15.04.2025 RR: Hack to restore hidden windows if one of the tool windows is closed

  if( ! WasModal) {

    //x/YaIPS_ToolWinTestAction( YAIPS_TWIN_ACTION_HIDE_SHOW_ALL, true);
  }
#endif
}

/************************************************************************************
 * IqeB_GUI_ColorChooser
 *
 * Popup a dialog to choose a color
 *
 * In:     old color
 * Return: new color
 */

#ifdef not_used
static bool Mouse_PrimaryPressed() {
    bool swapped = GetSystemMetrics(SM_SWAPBUTTON);

    if (!swapped) {
        return (GetAsyncKeyState(VK_LBUTTON) & 0x01) != 0;
    } else {
        return (GetAsyncKeyState(VK_RBUTTON) & 0x01) != 0;
    }
}
#endif

static bool Mouse_SecondaryPressed() {
    bool swapped = GetSystemMetrics(SM_SWAPBUTTON);

    if (!swapped) {
        return (GetAsyncKeyState(VK_RBUTTON) & 0x01) != 0;
    } else {
        return (GetAsyncKeyState(VK_LBUTTON) & 0x01) != 0;
    }
}

Fl_Color IqeB_GUI_ColorChooser( Fl_Color OldColor)
{
  Fl_Color NewColor;
  CHOOSECOLOR cc;                 // common dialog box structure
  uchar r,g,b;
  Fl_Window *pWin;

  NewColor = OldColor;

  Fl::get_color( OldColor, r, g, b);

  if( Mouse_SecondaryPressed()) {   // Right mouse button pressed

    Fl_Menu_Button popup( Fl::event_x(), Fl::event_y(), 80, 1);
    const Fl_Menu_Item *m;
    int HaveValidMainHisto, TempInt;

    HaveValidMainHisto =
                 (YaIPS_BigImageDisp.Flags & YAIPS_IDISP_FLAG_DO_DISP_MODIFY) != 0 &&   // Have a valid image
                 ! YaIPS_BigImageDisp.Plot3D_Active &&                                  // NO 3D plot active
                 YaIPS_Main_Histo.nHistos > 0 &&                                        // and have a measured histogramm
                 ( YaIPS_BigImageDisp.ShowInfoMode == YAIPS_SHOW_INFO_RE_HISTO_ALL ||   // and any of the histogram measurements
                   YaIPS_BigImageDisp.ShowInfoMode == YAIPS_SHOW_INFO_RE_HISTO_AOI);


    popup.add( LangStringLookup( "&GUI_Main_Menu_Color_Copy=Copy color to clipboard"), 0, NULL, (void*)1);
    popup.add( LangStringLookup( "&GUI_Main_Menu_Color_Paste=Get color from clipboard"), 0, NULL, (void*)2);

    if( HaveValidMainHisto) {                     // Main window shows a histogram with a Color

      popup.add( LangStringLookup( "&GUI_Main_Menu_Color_Histo=Get color from histogram"), 0, NULL, (void*)3);
    }

    m = popup.popup();
    if( m ) {

      if( (long long)m->user_data() == 2) { // Paste color

        NewColor = YaIPS_Setting_CopyPasteColor;

      } else if( (long long)m->user_data() == 3) { // Get color from histogram

        // Get red color component or gray value

        TempInt = lround( YaIPS_Main_Histo.R.Average);
        if( TempInt <   0) TempInt =   0;
        if( TempInt > 255) TempInt = 255;

        r = (uchar)TempInt;

        if( YaIPS_Main_Histo.nHistos >= 3) {       // Have a RGB color

          TempInt = lround( YaIPS_Main_Histo.G.Average);
          if( TempInt <   0) TempInt =   0;
          if( TempInt > 255) TempInt = 255;

          g = (uchar)TempInt;

          TempInt = lround( YaIPS_Main_Histo.B.Average);
          if( TempInt <   0) TempInt =   0;
          if( TempInt > 255) TempInt = 255;

          b = (uchar)TempInt;

        } else {   // Gray value

          g = r;
          b = r;
        }

        NewColor = fl_rgb_color( r, g, b);

      } else {                              // Copy color

        YaIPS_Setting_CopyPasteColor = NewColor;
      }
    }

  } else {                                        // Must be left mouse button pressed

    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    pWin = Fl::first_window();
    if( pWin != NULL) {
      cc.hwndOwner = fl_win32_xid( pWin);
    }
    cc.lpCustColors = (LPDWORD)YaIPS_Setting_ChooseColorPredef;
    cc.rgbResult = RGB( r, g, b);
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if( ChooseColor( &cc)==TRUE)  {

      NewColor = fl_rgb_color( GetRValue( cc.rgbResult), GetGValue( cc.rgbResult), GetBValue( cc.rgbResult));
    }
  }

  return( NewColor);
}

/************************************************************************************
 * Variables for GUI (widget pointers, state variables, ...)
 * Part 2.
 */

// define for window sizes
#define YAIPS_MAIN_SIZE_X_MIN       964
#define YAIPS_MAIN_SIZE_X_MAX      2048
#define YAIPS_MAIN_SIZE_X_DEFAULT   YAIPS_MAIN_SIZE_X_MIN

#define YAIPS_MAIN_SIZE_Y_MIN       715
#define YAIPS_MAIN_SIZE_Y_MAX      2048
#define YAIPS_MAIN_SIZE_Y_DEFAULT   YAIPS_MAIN_SIZE_Y_MIN

//

#define WITH_MIN_IMAGE_BOX   330              // minimum with of image box
#define WITH_MIN_TOOL_BOX     10              // minimum with of box for placing tool windows

// Main window

Fl_Double_Window *pGUI_Main;

int YaIPS_Main_WinPosX = IQE_GUI_NO_WINPOS_X, YaIPS_Main_WinPosY = IQE_GUI_NO_WINPOS_Y; // last window position
int YaIPS_Main_WinSizeX = YAIPS_MAIN_SIZE_X_DEFAULT, YaIPS_Main_WinSizeY = YAIPS_MAIN_SIZE_Y_DEFAULT; // last window size

#ifdef WIN32
// A memory mapped file.
// Is used to avoid mutilple running instances of IqeBrowser.

static HANDLE MultipleRunning_hMapFile = NULL;  // File handle
static int  MultipleRunning_SizeData = MAX_FILENAME_LEN;  // size of data
static char *MultipleRunning_pData = NULL;      // Point to data
#endif

// GUI elements
static Fl_Group *pGUI_GroupLeftSide;                 // left side of OpenGL window

// Image enlargement
static Fl_Radio_Round_Button * ResolutionButtons[ YAIPS_DISP_RESOLUTION_MAX + 1]; // Table of resolution buttons

// Image color modification
static Fl_Radio_Round_Button * ColModButtons[ YAIPS_DISP_COLMOD_MAX + 1]; // Table of resolution buttons

// Image overlay
static Fl_Radio_Round_Button * OverlayButtons[ YAIPS_OVERLAY_MAX + 1]; // Table of resolution buttons
static Fl_Button *pBut_OverlayColor;                 // Button to select ovelay color
// --- Image overlay
static int OverlayStyle = 0;                         // Current selected image overlay
static int OverlayColor = FL_YELLOW;                 // Color used for overlay drawing

// Color palette
static Fl_Choice       *pChoice_Col_Palette;         // Color palette
static Fl_Check_Button *pCBox_Col_Inverted;          // Color inversion
static Fl_Check_Button *pCBox_Col_Darken;            // Color darken

// 3D plot things
static Fl_Value_Slider *pSlider_3D_Azimuth;          // Slider for 3D azimuth
static Fl_Value_Slider *pSlider_3D_Elevation;        // Slider for 3D elevation
static Fl_Check_Button *pCBox_3D_Active;             // Check box 3D active
static Fl_Check_Button *pCBox_3D_DrawGrid;           // Check box 3D draw grid lines
static Fl_Check_Button *pCBox_3D_Inverted;           // Check box 3D inverted

// Show measurement/info
static Fl_Radio_Round_Button * ShowButtons[ YAIPS_SHOW_INFO_MAX + 1];  // Table of show buttons

// Different uses. Histogram, ..
static YaIPS_Fl_Box *pGUI_Box_DiffUses;               // Display used for different uses
static int DUseDataChange = -1;                       // Monitor source image change
Fl_YaIPS_Histo_RGB_t YaIPS_Main_Histo;                // Histogram of current big image for YAIPS_SHOW_INFO_RE_HISTO_ALL / YAIPS_SHOW_INFO_RE_HISTO_AOI
static Fl_YaIPS_ColRowSum_RGB_t ShowColRowSumInfo;    // Hold info about column / row profiles
int    YaIPS_Main_Measured_Distance = 0;              // > 0 for a measured distance YAIPS_SHOW_INFO_2P_DIST_XXX [Pixel]

// Display a big image
Fl_YaIPS_ImageDisp_t YaIPS_BigImageDisp;              // Info big image display

int YaIPS_Main_ImageSourceID_Last;                    // Last used image source ID. Is used for startup from last session

// Others

static Fl_Tile *pTile_RightSide;                      // Groups the image box and the box for placing tool windows
Fl_Box *pGUI_Main_RightSide;                          // Box for right side (used to place tool windows)
static int RightSideWidth;                            // Width of right side box
static int RightSideWidthFS;                          // Width of right side box if window full screen

static int FullScreenActive = 0;                      // Full screen was active on last exit

static int YaIPS_Main_WriteFileType_Last = 0;         // Last used file type used to write an image file

static int RightSide_LastSkip = 1;                    // If > 0 skip latch of right side position
static int RightSide_LastX, RightSide_LastY;

//-----------------------------------------------------------------------------------
//
// Functions working with big image display
//
//-----------------------------------------------------------------------------------

/************************************************************************************
 * YaIPS_GUI_BigDrawAfter_cb
 *
 * Additional drawings after the image was drawn.
 *
 */
static void YaIPS_GUI_BigDrawAfter_cb( Fl_Widget *pW,
                                       void *pArg1,        // Pointer to Fl_YaIPS_ImageDisp_t
                                       void *pArg2)        // Optional pointer to ToolData
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  int ierr, x1, y1, xx, yy;
  int Plot3D_Active;
  Fl_Color ColBackground, ColForeground;

  pYaIPS_ImageDisp = &YaIPS_BigImageDisp;

  ierr = YaIPS_ImageDispCalcSizes( pYaIPS_ImageDisp);   // Check sizes

  if( ierr < 0) {                          // No image box (no drawing area)

    return;                                // Return to caller
  }

  // Preparations

  Plot3D_Active = YaIPS_BigImageDisp.Plot3D_Active;

  // ...

  // Clipping for box outside. Draw here size bars or color LUT.

  fl_push_clip( pYaIPS_ImageDisp->BigImage_bx, pYaIPS_ImageDisp->BigImage_by, pYaIPS_ImageDisp->BigImage_bw, pYaIPS_ImageDisp->BigImage_bh);

  if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_DO_DISP_MODIFY) == 0) {

    // No image display until now

    // Display version information

    int mw, mh;
    char TempString[ 256];

    if( ierr != true) {                      // Something not OK

      // Use info from image box

      xx = pYaIPS_ImageDisp->BigImage_bw;
      x1 = pYaIPS_ImageDisp->BigImage_bx;
      y1 = pYaIPS_ImageDisp->BigImage_by + 16;

    } else {                                // Have an image loaded

      xx = pYaIPS_ImageDisp->BigImage_sw;
      x1 = pYaIPS_ImageDisp->BigImage_sx;
      y1 = pYaIPS_ImageDisp->BigImage_sy - 80;

      if( y1 < pYaIPS_ImageDisp->BigImage_by + 16) {

        y1 = pYaIPS_ImageDisp->BigImage_by + 16;
      }
    }

#ifdef _DEBUG

    fl_color( FL_BLUE);

    fl_font( FL_HELVETICA, 18);
    sprintf( TempString, "Debug build");

    mw = mh = 0;
    fl_measure( TempString, mw, mh);

    y1 -= mh + 0;

    fl_draw( TempString, x1 + (xx - mw) / 2, y1 + mh);

    y1 += mh + 0;

 #endif

    fl_color( FL_DARK_RED);

    fl_font( FL_HELVETICA, 28);
    sprintf( TempString, "%s", LangStringLookup( WIN_PROG_NAME));

    mw = mh = 0;
    fl_measure( TempString, mw, mh);
    fl_draw( TempString, x1 + (xx - mw) / 2, y1 + mh);

    y1 += mh + 6;

    fl_font( FL_HELVETICA, 18);
    sprintf( TempString, "%s", LangStringLookup( WIN_DEFAULT_TITLE));

    mw = mh = 0;
    fl_measure( TempString, mw, mh);
    fl_draw( TempString, x1 + (xx - mw) / 2, y1 + mh);

    y1 += mh + 4;

    fl_font( FL_HELVETICA, 18);
    sprintf( TempString, "%s  %s", WIN_PROG_VERSION_NR, LangStringLookup( WIN_PROG_VERSION_DATE));

    mw = mh = 0;
    fl_measure( TempString, mw, mh);
    fl_draw( TempString, x1 + (xx - mw) / 2, y1 + mh);

    y1 += mh + 6;

    goto ExitPoint;
  }

  if( ierr != true) {                      // Something not OK

    goto ExitPoint;
  }

  if( pYaIPS_ImageDisp->DisplayResolution > YAIPS_DISP_RESOLUTION_AUTO) {  // no auto resolution

    // Prepare drawing of size bars

    ColBackground = FL_DARK_GREEN;
    ColForeground = FL_CYAN;   //x/ FL_GREEN - 1; FL_DARK_BLUE;

    // Draw horizontal size marker at bottom side

    x1 = pYaIPS_ImageDisp->BigImage_sx;
    y1 = pYaIPS_ImageDisp->BigImage_by + pYaIPS_ImageDisp->BigImage_bh - YAIPS_IMAGE_DISP_BORDER2 + 2;
    xx = pYaIPS_ImageDisp->BigImage_sw;
    yy = 2;

    fl_color( ColBackground);
    fl_rectf( x1, y1, xx, yy);

    x1 = pYaIPS_ImageDisp->BigImage_sx + ((int)(pYaIPS_ImageDisp->SubImage_x + 0.5) * pYaIPS_ImageDisp->BigImage_sw) / pYaIPS_ImageDisp->BigImage_iw;
    xx = (pYaIPS_ImageDisp->SubImage_w * pYaIPS_ImageDisp->BigImage_sw) / pYaIPS_ImageDisp->BigImage_iw;

    fl_color( ColForeground);
    fl_rectf( x1, y1 - 1, xx, yy + 2);

    // Draw vertical size marker at right side

    y1 = pYaIPS_ImageDisp->BigImage_sy;
    x1 = pYaIPS_ImageDisp->BigImage_bx + pYaIPS_ImageDisp->BigImage_bw - YAIPS_IMAGE_DISP_BORDER2 + 2;
    xx = 2;
    yy = pYaIPS_ImageDisp->BigImage_sh;

    fl_color( ColBackground);
    fl_rectf( x1, y1, xx, yy);

    y1 = pYaIPS_ImageDisp->BigImage_sy + ((int)(pYaIPS_ImageDisp->SubImage_y + 0.5) * pYaIPS_ImageDisp->BigImage_sh) / pYaIPS_ImageDisp->BigImage_ih;
    yy = (pYaIPS_ImageDisp->SubImage_h * pYaIPS_ImageDisp->BigImage_sh) / pYaIPS_ImageDisp->BigImage_ih;

    fl_color( ColForeground);
    fl_rectf( x1 - 1, y1, xx + 2, yy);
  }

  // Draw false color bar

  if( pYaIPS_ImageDisp->DisplayColMod.FalseColor ||                         // Have false color table
      (pYaIPS_ImageDisp->DisplayColMod.Style > YAIPS_DISP_COLMOD_NORMAL &&   // or any B/W style
          (pYaIPS_ImageDisp->DisplayColMod.Invert ||                        //   and inverted color
              pYaIPS_ImageDisp->DisplayColMod.Darken))) {                   //   or darken color

    int i;

    x1 = pYaIPS_ImageDisp->BigImage_bx + 2;
    y1 = pYaIPS_ImageDisp->BigImage_by + 8;

    for( i = 0; i < 256; i++) {

      fl_color( fl_rgb_color( pYaIPS_ImageDisp->DisplayColMod.LookupR[ i],
                              pYaIPS_ImageDisp->DisplayColMod.LookupG[ i],
                              pYaIPS_ImageDisp->DisplayColMod.LookupB[ i]));

      fl_xyline( x1, y1 + 255 - i, x1 + 4);
    }

    fl_color( FL_BLACK);

    fl_xyline( x1, y1 - 1, x1 + 5);
    fl_xyline( x1, y1 + 256, x1 + 5);

    fl_yxline( x1 + 5, y1, y1 + 255);
  }

  fl_pop_clip();

  // Make drawings from a tool window

  if( Plot3D_Active == false /* &&                                // NO plot3d active
      pYaIPS_ImageDisp->ShowInfoMode == YAIPS_SHOW_INFO_OFF*/) { // and no measurement/info modes

    YaIPS_ToolWinDrawAfterCall( pYaIPS_ImageDisp, YaIPS_BigImageDisp.ImageSourceID);
  }

  // Clip to image part displayed on screen
  fl_push_clip( pYaIPS_ImageDisp->BigImage_sx, pYaIPS_ImageDisp->BigImage_sy, pYaIPS_ImageDisp->BigImage_sw, pYaIPS_ImageDisp->BigImage_sh);

  // Show measurement/info things

  if( ! Plot3D_Active) {                                                 // NO plot3d active

    int ShowInfoMode, LineWidth;

    // Ensure histogram is measured

    ShowInfoMode = YaIPS_GUI_DiffUsesCalcThings();          // Check for changed image

    // Used line width in pixel for histograms, AOIs

    LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

    switch( ShowInfoMode) {

    case YAIPS_SHOW_INFO_CU_LUT:           // Show current lookup table

      // Nothing to do here

      break;

    case YAIPS_SHOW_INFO_CU_VAL:           // At cursor show pixel value
      if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_IN_IMAGE) != 0 &&  // and mouse is over image
          (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_BUTT_ANY) == 0) {  // and NOT any mouse button pressed

        int SrcX, SrcY, OffX, OffY, Src_d, Src_ld;
        uchar *pDataSrc;
        int mw, mh;
        char TempString[256];

        // Get position in image

        SrcX =
            (int) (pYaIPS_ImageDisp->SubImage_x + 0.5)
                + (int) (pYaIPS_ImageDisp->MouseX
                    / pYaIPS_ImageDisp->PixelImageToScreen);
        SrcY =
            (int) (pYaIPS_ImageDisp->SubImage_y + 0.5)
                + (int) (pYaIPS_ImageDisp->MouseY
                    / pYaIPS_ImageDisp->PixelImageToScreen);

        if (SrcX >= pYaIPS_ImageDisp->BigImage_iw) {     // Clip image source start
          SrcX = pYaIPS_ImageDisp->BigImage_iw - 1;
        }
        if (SrcX < 0) {
          SrcX = 0;
        }

        if (SrcY >= pYaIPS_ImageDisp->BigImage_ih) {     // Clip image source start
          SrcY = pYaIPS_ImageDisp->BigImage_ih - 1;
        }
        if (SrcY < 0) {
          SrcY = 0;
        }

        // Get value

        Src_d = pYaIPS_ImageDisp->pImage_Img->d();
        Src_ld =
            pYaIPS_ImageDisp->pImage_Img->ld() ?
                pYaIPS_ImageDisp->pImage_Img->ld() :
                pYaIPS_ImageDisp->pImage_Img->data_w() * Src_d;

        pDataSrc = (uchar*) pYaIPS_ImageDisp->pImage_Img->data()[0];
        pDataSrc = pDataSrc + SrcY * Src_ld + SrcX * Src_d;

        // Get position on screen

        OffX = pYaIPS_ImageDisp->BigImage_sx;
        OffY = pYaIPS_ImageDisp->BigImage_sy;

        x1 = OffX + pYaIPS_ImageDisp->MouseX;
        y1 = OffY + pYaIPS_ImageDisp->MouseY;

        fl_font( FL_SCREEN, 16);

        mh = 22;

        if (Src_d >= 3) {     // Have an color image

          mw = 40 * 3 + 10;

        } else {               // Have a BW image

          mw = 45;
        }

        if( Src_d == 2) {            // Has an alpha channel

          mw += 51;

        } else if(  Src_d >= 4) {   // Has an alpha channel

          mw += 47;
        }

        if (x1 - mw - 6 > pYaIPS_ImageDisp->BigImage_sx) {

          x1 -= mw + 6;

        } else {

          x1 += 6;
        }

        if (y1 + mh + 6
            >= pYaIPS_ImageDisp->BigImage_sy + pYaIPS_ImageDisp->BigImage_sh) {

          y1 -= mh + 6;

        } else {

          y1 += 6;
        }

        fl_color(FL_WHITE);
        fl_rectf(x1, y1, mw, mh);
        fl_color(FL_BLACK);
        fl_rect(x1, y1, mw, mh);

        if (Src_d >= 3) {     // Have an color image

          sprintf(TempString, "%3d", pDataSrc[0]);
          fl_color(FL_RED);
          fl_draw(TempString, x1 + 4, y1 + mh - 4);

          x1 += 40;
          sprintf(TempString, "%3d", pDataSrc[1]);
          fl_color(FL_GREEN - 2);
          fl_draw(TempString, x1 + 4, y1 + mh - 4);

          x1 += 40;
          sprintf(TempString, "%3d", pDataSrc[2]);
          fl_color(FL_BLUE);
          fl_draw(TempString, x1 + 4, y1 + mh - 4);

        } else {               // Have a BW image

          sprintf(TempString, "%3d", pDataSrc[0]);
          fl_color(FL_BLACK);
          fl_draw(TempString, x1 + 4, y1 + mh - 4);
        }

        if( Src_d == 2 || Src_d >= 4) {   // Has an alpha channel

          x1 += 40;
          sprintf(TempString, "+%3d", Src_d == 2 ? pDataSrc[1] : pDataSrc[3]);
          fl_color(FL_DARK3 - 2);
          fl_draw(TempString, x1 + 4, y1 + mh - 4);
        }
       }

      YaIPS_BigImageDisp.Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;     // Set AOI changed flag bit
      break;

    case YAIPS_SHOW_INFO_RE_HISTO_AOI:         // In rectangle show histogram
    case YAIPS_SHOW_INFO_RE_COLSUM:            // In rectangle show column sum
    case YAIPS_SHOW_INFO_RE_ROWSUM:            // In rectangle show row sum
    case YAIPS_SHOW_INFO_2P_DIST_VER:          // Show distance of two points vertical
    case YAIPS_SHOW_INFO_2P_DIST_HOR:          // Show distance of two points horizontal
      {
        int OffX, OffY, AoiXX, AoiYY, x1, y1, x2, y2, p1x, p1y, p2x, p2y;
        int DistInPixel, mw, mh, SizeShift, SizeFactor, LengthFirst;
        Fl_Color ColAOI;
        char TempString[ 256];

        // Clip AOI

        YaIPS_ImageDispAoiClipAndCheck( pYaIPS_ImageDisp, &AoiXX, &AoiYY, NULL, NULL);

        // Draw color for AOI
        if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0) {

          ColAOI = FL_RED;
        } else {

          ColAOI = FL_GREEN - 2;
        }

        // AOI is relative to this

        OffX = pYaIPS_ImageDisp->BigImage_sx;
        OffY = pYaIPS_ImageDisp->BigImage_sy;

        // Points relative do display box
        x1 = OffX + pYaIPS_ImageDisp->AoiP1x;
        y1 = OffY + pYaIPS_ImageDisp->AoiP1y;
        x2 = OffX + pYaIPS_ImageDisp->AoiP2x;
        y2 = OffY + pYaIPS_ImageDisp->AoiP2y;

        // Points relative to image
        p1x = (int) (pYaIPS_ImageDisp->SubImage_x + 0.5) + (int) (pYaIPS_ImageDisp->AoiP1x / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
        p1y = (int) (pYaIPS_ImageDisp->SubImage_y + 0.5) + (int) (pYaIPS_ImageDisp->AoiP1y / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
        p2x = (int) (pYaIPS_ImageDisp->SubImage_x + 0.5) + (int) (pYaIPS_ImageDisp->AoiP2x / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
        p2y = (int) (pYaIPS_ImageDisp->SubImage_y + 0.5) + (int) (pYaIPS_ImageDisp->AoiP2y / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

        // Display resolution. Is needed for profiles.

        if( pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_1_1) {  // 1:1 or enlarged

          SizeShift  = pYaIPS_ImageDisp->DisplayResolution - 1;       // Shift is enlargement by power of 2
          SizeFactor = 1 << SizeShift;                               // Enlargement factor

        } else {    // Must be YAIPS_DISP_RESOLUTION_AUTO

          SizeShift  = 0;
          SizeFactor = 0;
        }

        // ...

        TempString[ 0] = '\0';                  // Preset, no text do display
        fl_font( FL_HELVETICA, 16);

        switch( pYaIPS_ImageDisp->ShowInfoMode) {

        default:
        case YAIPS_SHOW_INFO_RE_HISTO_AOI:     // In rectangle show histogram

          fl_color( ColAOI);

          fl_line_style( 0, LineWidth);   // Set line width

          fl_rect( x1, y1, AoiXX, AoiYY);

          fl_line_style( 0);   // Reset to default

          break;

        case YAIPS_SHOW_INFO_RE_COLSUM:          // In rectangle show column sum

          if (y1 + AoiYY + 256 + 4 <= pYaIPS_ImageDisp->BigImage_sy + pYaIPS_ImageDisp->BigImage_sh - 4) {

            y2 = y1 + AoiYY + 4;

          } else if (y1 - 256 - 4 >= pYaIPS_ImageDisp->BigImage_sy + 4) {

            y2 = y1 - 256 - 4;

          } else {

#ifdef use_again
            y2 = y1 + AoiYY - 256 - 4;
#else
            y2 = pYaIPS_ImageDisp->BigImage_sy + pYaIPS_ImageDisp->BigImage_sh - 256 - 4;
#endif
          }

          if( SizeFactor > 1) {

            LengthFirst = SizeFactor - (pYaIPS_ImageDisp->AoiP1x % SizeFactor) - 1;
          } else {

            LengthFirst = 0;
          }

#ifdef use_again
#ifdef _DEBUG
          YaIPS_ImageDispStrDebug( pYaIPS_ImageDisp, "s%2d, r%02d, f%02d, Elements %3d",
                      SizeFactor, SizeFactor > 0 ? pYaIPS_ImageDisp->AoiP1x % SizeFactor : 0, LengthFirst,
                      ShowColRowSumInfo.R.nElements);

#endif
#endif

          YaIPS_ColRowSum_Draw( &ShowColRowSumInfo,   // Pointer to RGB profiles
                               x1, y2,               // Left upper drawing reference point
                               AoiXX, 256,           // With / height of drawing box
                               SizeFactor, LengthFirst);

          fl_color( ColAOI);

          fl_line_style( 0, LineWidth);   // Set line width

          fl_rect( x1, y1, AoiXX, AoiYY);

          fl_line_style( 0);   // Reset to default

          break;

        case YAIPS_SHOW_INFO_RE_ROWSUM:          // In rectangle show row sum

          if (x1 + AoiXX + 256 + 4 <= pYaIPS_ImageDisp->BigImage_sx + pYaIPS_ImageDisp->BigImage_sw) {

            x2 = x1 + AoiXX + 4;

          } else if (x1 - 256 - 4 >= pYaIPS_ImageDisp->BigImage_sx + 4) {

            x2 = x1 - 256 - 4;

          } else {

#ifdef use_again
             x2 = x1 + 4;
#else
             x2 = pYaIPS_ImageDisp->BigImage_sx + 4;
#endif
          }

          if( SizeFactor > 1) {

            LengthFirst = SizeFactor - (pYaIPS_ImageDisp->AoiP1y % SizeFactor) - 1;
          } else {

            LengthFirst = 0;
          }

#ifdef use_again
#ifdef _DEBUG
          YaIPS_ImageDispStrDebug( pYaIPS_ImageDisp, "s%2d, r%02d, f%02d, Elements %3d",
                      SizeFactor, SizeFactor > 0 ? pYaIPS_ImageDisp->AoiP1y % SizeFactor : 0, LengthFirst,
                      ShowColRowSumInfo.R.nElements);

#endif
#endif

          YaIPS_ColRowSum_Draw( &ShowColRowSumInfo,   // Pointer to RGB profiles
                               x2, y1,               // Left upper drawing reference point
                               256, AoiYY,           // With / height of drawing box
                               SizeFactor, LengthFirst);

          fl_color( ColAOI);

          fl_line_style( 0, LineWidth);   // Set line width

          fl_rect( x1, y1, AoiXX, AoiYY);

          fl_line_style( 0);   // Reset to default

          break;

        case YAIPS_SHOW_INFO_2P_DIST_VER:          // Show distance of two points horizontal

          DistInPixel = p2y - p1y;                             // Distance in pixel
          if( DistInPixel < 0)  DistInPixel = - DistInPixel;   // Use absolute value
          YaIPS_Main_Measured_Distance = DistInPixel;           // Set measured distance

          if( YaIPS_Calib_Unit <= YAIPS_CALIB_UNIT_PIXEL) {

            sprintf( TempString, LangStringLookup( "&GUI_Main_Disp_Dist_Pix=%d Pixel"),
                     DistInPixel);
          } else {

            sprintf( TempString, LangStringLookup( "&GUI_Main_Disp_Dist=%.3f %s/%d Pixel"),
                     DistInPixel * YaIPS_Calib_UPP_Y, pYaIPS_Calib_Unit2String(), DistInPixel);
          }

          // Draw ruler
          fl_color( ColAOI);

          fl_line_style( 0, LineWidth);   // Set line width

          fl_line( x1, y1, x2, y1);
          fl_line( x1, y2, x2, y2);
          fl_line( (x1 + x2 + 1) / 2, y1, (x1 + x2 + 1) / 2, y2);

          fl_line_style( 0);   // Reset to default
          break;

        case YAIPS_SHOW_INFO_2P_DIST_HOR:          // Show distance of two points horizontal

          DistInPixel = p2x - p1x;                             // Distance in pixel
          if( DistInPixel < 0)  DistInPixel = - DistInPixel;   // Use absolute value
          YaIPS_Main_Measured_Distance = DistInPixel;           // Set measured distance

          if( YaIPS_Calib_Unit <= YAIPS_CALIB_UNIT_PIXEL) {

            sprintf( TempString, LangStringLookup( "&GUI_Main_Disp_Dist_Pix=%d Pixel"),
                     DistInPixel);
          } else {

            sprintf( TempString, LangStringLookup( "&GUI_Main_Disp_Dist=%.3f %s/%d Pixel"),
                     DistInPixel * YaIPS_Calib_UPP_X, pYaIPS_Calib_Unit2String(), DistInPixel);
          }

          // Draw ruler
          fl_color( ColAOI);

          fl_line_style( 0, LineWidth);   // Set line width

          fl_line( x1, y1, x1, y2);
          fl_line( x2, y1, x2, y2);
          fl_line( x1, (y1 + y2 + 1) / 2, x2, (y1 + y2 + 1) / 2);

          fl_line_style( 0);   // Reset to default
          break;

        }  // switch( pYaIPS_ImageDisp->ShowInfoMode)

        // Has to display a text

        if( TempString[ 0] != '\0') {

          mw = mh = 0;
          fl_measure( TempString, mw, mh);

          // Adjust x coordinate for out of screen

          if( x1 + mw + 4 > pYaIPS_ImageDisp->BigImage_sx + pYaIPS_ImageDisp->BigImage_sw) {

            x1 = pYaIPS_ImageDisp->BigImage_sx + pYaIPS_ImageDisp->BigImage_sw - mw - 4;
          }

          // Adjust y coordinate for out of screen

          if( y1 - 6 - mh > pYaIPS_ImageDisp->BigImage_sy) {  // Fits above frame

            fl_draw( TempString, x1 + 2, y1 - 4);

          } else if( y2 + 6 + mh < pYaIPS_ImageDisp->BigImage_sy + pYaIPS_ImageDisp->BigImage_sh - 1) {  // Fits above frame

            fl_draw( TempString, x1 + 2, y2 + 4 + mh);

          } else {

            fl_draw( TempString, x1 + 2, y2 - 6);
          }
        }
      }
      break;

    } // end switch( pYaIPS_ImageDisp->ShowInfoMode)
  }

  // Image overlay

  if( ! Plot3D_Active &&                                                // NO plot3d active
      OverlayStyle > 0) {                                               // Draw overlay

    int OffX, OffY, p1x, p1y, x2, y2, dw, dh, i;
    static char MyLineDashes1[] = { 8, 8, 0};
    static char MyLineDashes2[] = { 3, 13, 0};

    OffX = pYaIPS_ImageDisp->BigImage_sx;
    OffY = pYaIPS_ImageDisp->BigImage_sy;

    // Center relative to image
    p1x = pYaIPS_ImageDisp->BigImage_iw / 2;      // Image center in pixel
    p1y = pYaIPS_ImageDisp->BigImage_ih / 2;

    fl_color( OverlayColor);

    switch( OverlayStyle) {

    case YAIPS_OVERLAY_CROSSHAIR:

      // Center cross

      x1 = (int)(((p1x - (int)(pYaIPS_ImageDisp->SubImage_x + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffX;
      y1 = (int)(((p1y - (int)(pYaIPS_ImageDisp->SubImage_y + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffY;

      fl_line( x1, pYaIPS_ImageDisp->BigImage_sy, x1, pYaIPS_ImageDisp->BigImage_sy + pYaIPS_ImageDisp->BigImage_sh - 1);
      fl_line( pYaIPS_ImageDisp->BigImage_sx, y1, pYaIPS_ImageDisp->BigImage_sx + pYaIPS_ImageDisp->BigImage_sw - 1, y1);

      break;

    case YAIPS_OVERLAY_GRID_1:

      // Center cross

      x1 = (int)(((p1x - (int)(pYaIPS_ImageDisp->SubImage_x + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffX;
      y1 = (int)(((p1y - (int)(pYaIPS_ImageDisp->SubImage_y + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffY;

      fl_line( x1, pYaIPS_ImageDisp->BigImage_sy, x1, pYaIPS_ImageDisp->BigImage_sy + pYaIPS_ImageDisp->BigImage_sh - 1);
      fl_line( pYaIPS_ImageDisp->BigImage_sx, y1, pYaIPS_ImageDisp->BigImage_sx + pYaIPS_ImageDisp->BigImage_sw - 1, y1);

      // Points relative do display box

      fl_line_style( FL_DASH, 0, MyLineDashes1);

      dw = (pYaIPS_ImageDisp->BigImage_iw + 2) / 4;
      dh = (pYaIPS_ImageDisp->BigImage_ih + 2) / 4;

      x1 = (int)(((p1x - dw - (int)(pYaIPS_ImageDisp->SubImage_x + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffX;
      y1 = (int)(((p1y - dh - (int)(pYaIPS_ImageDisp->SubImage_y + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffY;

      x2 = (int)(((p1x + dw - (int)(pYaIPS_ImageDisp->SubImage_x + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffX;
      y2 = (int)(((p1y + dh - (int)(pYaIPS_ImageDisp->SubImage_y + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffY;

      fl_rect( x1, y1, x2 - x1 + 1, y2 - y1 + 1);

      fl_line_style( FL_DOT, 0, MyLineDashes2);

      dw = (pYaIPS_ImageDisp->BigImage_iw + 4) / 8;
      dh = (pYaIPS_ImageDisp->BigImage_ih + 4) / 8;

      x1 = (int)(((p1x - dw - (int)(pYaIPS_ImageDisp->SubImage_x + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffX;
      y1 = (int)(((p1y - dh - (int)(pYaIPS_ImageDisp->SubImage_y + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffY;

      x2 = (int)(((p1x + dw - (int)(pYaIPS_ImageDisp->SubImage_x + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffX;
      y2 = (int)(((p1y + dh - (int)(pYaIPS_ImageDisp->SubImage_y + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffY;

      fl_rect( x1, y1, x2 - x1 + 1, y2 - y1 + 1);

      dw = (3 * pYaIPS_ImageDisp->BigImage_iw + 4) / 8;
      dh = (3 * pYaIPS_ImageDisp->BigImage_ih + 4) / 8;

      x1 = (int)(((p1x - dw - (int)(pYaIPS_ImageDisp->SubImage_x + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffX;
      y1 = (int)(((p1y - dh - (int)(pYaIPS_ImageDisp->SubImage_y + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffY;

      x2 = (int)(((p1x + dw - (int)(pYaIPS_ImageDisp->SubImage_x + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffX;
      y2 = (int)(((p1y + dh - (int)(pYaIPS_ImageDisp->SubImage_y + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffY;

      fl_rect( x1, y1, x2 - x1 + 1, y2 - y1 + 1);

      fl_line_style( 0);   // Reset to default

      break;

    case YAIPS_OVERLAY_GRID_2:

      // Center cross

      x1 = (int)(((p1x - (int)(pYaIPS_ImageDisp->SubImage_x + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffX;
      y1 = (int)(((p1y - (int)(pYaIPS_ImageDisp->SubImage_y + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffY;

      fl_line( x1, pYaIPS_ImageDisp->BigImage_sy, x1, pYaIPS_ImageDisp->BigImage_sy + pYaIPS_ImageDisp->BigImage_sh - 1);
      fl_line( pYaIPS_ImageDisp->BigImage_sx, y1, pYaIPS_ImageDisp->BigImage_sx + pYaIPS_ImageDisp->BigImage_sw - 1, y1);

      // Points relative do display box

      for( i = 1; ; i++) {

        dw = 50 * i;
        dh = 50 * i;

        if( dw > pYaIPS_ImageDisp->BigImage_iw / 2 &&
            dh > pYaIPS_ImageDisp->BigImage_ih / 2) {

          break;
        }

        fl_line_style( FL_DASH, 0, (i % 5) == 0 ? MyLineDashes1 : MyLineDashes2);

        x1 = (int)(((p1x - dw - (int)(pYaIPS_ImageDisp->SubImage_x + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffX;
        y1 = (int)(((p1y - dh - (int)(pYaIPS_ImageDisp->SubImage_y + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffY;

        x2 = (int)(((p1x + dw - (int)(pYaIPS_ImageDisp->SubImage_x + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffX;
        y2 = (int)(((p1y + dh - (int)(pYaIPS_ImageDisp->SubImage_y + 0.5)) * pYaIPS_ImageDisp->PixelImageToScreen) + 0.5) + OffY;

        fl_line( x1, pYaIPS_ImageDisp->BigImage_sy, x1, pYaIPS_ImageDisp->BigImage_sy + pYaIPS_ImageDisp->BigImage_sh - 1);
        fl_line( pYaIPS_ImageDisp->BigImage_sx, y1, pYaIPS_ImageDisp->BigImage_sx + pYaIPS_ImageDisp->BigImage_sw - 1, y1);

        fl_line( x2, pYaIPS_ImageDisp->BigImage_sy, x2, pYaIPS_ImageDisp->BigImage_sy + pYaIPS_ImageDisp->BigImage_sh - 1);
        fl_line( pYaIPS_ImageDisp->BigImage_sx, y2, pYaIPS_ImageDisp->BigImage_sx + pYaIPS_ImageDisp->BigImage_sw - 1, y2);

      }

      fl_line_style( 0);   // Reset to default

      break;
    } // end switch()
  }

ExitPoint:

// Finish up

  fl_line_style( 0);   // Reset to default
  fl_pop_clip();                 // Pop clip frome before

  // Draw additional common drawings

  YaIPS_ImageDispDrawAfter_Common( pYaIPS_ImageDisp, 0, true);  // handle clipping of draw region
}

//-----------------------------------------------------------------------------------
//
// Functions working with big display for different uses
//
//-----------------------------------------------------------------------------------

/************************************************************************************
 * YaIPS_GUI_DiffUsesCalcThings
 *
 * Calculate things to draw by YaIPS_GUI_DiffUsesDrawAfter_cb or YaIPS_GUI_BigDrawAfter_cb
 *
 * Return: One of the YAIPS_SHOW_INFO_XXX defines
 *
 */
static int YaIPS_GUI_DiffUsesCalcThings()
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  int ShowInfoMode, AoiChanged;

  pYaIPS_ImageDisp = &YaIPS_BigImageDisp;

  // Do we have any image
  if( pYaIPS_ImageDisp->pImage_Img == NULL ||           // Security test, no image
      (YaIPS_BigImageDisp.Flags & YAIPS_IDISP_FLAG_DO_DISP_MODIFY) == 0) { // or have the startup image

    YaIPS_Main_Histo.nHistos = 0;                       // No Histograms

    return( YAIPS_SHOW_INFO_NOTHING);                   // Nothing to show
  }

  // Get current info mode

  ShowInfoMode = YaIPS_BigImageDisp.ShowInfoMode;       // Get the current show info mode
  AoiChanged   = (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_CHA) != 0;  // The aoi changed since last call

  if( pYaIPS_ImageDisp->Plot3D_Active) {               // 3D plot is active

    ShowInfoMode = YAIPS_SHOW_INFO_OFF;
  }

  switch( ShowInfoMode) {

  case YAIPS_SHOW_INFO_OFF:                     // Off. Show histogram big image
  default:

    if( ShowInfoMode != YAIPS_SHOW_INFO_OFF) {  // Other than YAIPS_SHOW_INFO_OFF

      ShowInfoMode = YAIPS_SHOW_INFO_OFF;       // do the same as YAIPS_SHOW_INFO_OFF
    }

    break;

  case YAIPS_SHOW_INFO_CU_LUT:           // Show current lookup table

    // Nothing to do here

    break;

  case YAIPS_SHOW_INFO_CU_VAL:                  // At cursor show pixel value

    // Has the AOI changed

    if( ! AoiChanged) {                     // AOI has NOT changed
      break;                                // If not: nothing to do
    }

    break;

  case YAIPS_SHOW_INFO_RE_HISTO_ALL:        // Show histogram big image

    // Is the source image modified ?

    if( pYaIPS_ImageDisp->ImageChanged == DUseDataChange) {  // No forced update and the image has NOT changed

      break;
    }

    DUseDataChange = pYaIPS_ImageDisp->ImageChanged;  // Got current changed stamp

    AoiChanged = true;                               // Set this to get info area refreshed

    // Get histograms and statistical data

    YaIPS_Histo_Measure( pYaIPS_ImageDisp->pImage_Img, &YaIPS_Main_Histo, 0, 0, 0, 0);

    break;

  case YAIPS_SHOW_INFO_RE_HISTO_AOI:        // In rectangle show histogram

    // Has the AOI changed

    if( ! AoiChanged) {                     // AOI has NOT changed
      break;                                // If not: nothing to do
    }

    // Get histograms and statistical data

    {
      int AoiXX, AoiYY;
      int p1x, p1y;                      // Aoi rectangle start point in image pixel coordinates

      YaIPS_ImageDispAoiClipAndCheck( &YaIPS_BigImageDisp, &AoiXX, &AoiYY, NULL, NULL);

      p1x = (int) (pYaIPS_ImageDisp->SubImage_x + 0.5) + (int) (pYaIPS_ImageDisp->AoiP1x / pYaIPS_ImageDisp->PixelImageToScreen);
      p1y = (int) (pYaIPS_ImageDisp->SubImage_y + 0.5) + (int) (pYaIPS_ImageDisp->AoiP1y / pYaIPS_ImageDisp->PixelImageToScreen);

      AoiXX = (int) ((AoiXX / pYaIPS_ImageDisp->PixelImageToScreen) + 0.5);
      AoiYY = (int) ((AoiYY / pYaIPS_ImageDisp->PixelImageToScreen) + 0.5);

      YaIPS_Histo_Measure( pYaIPS_ImageDisp->pImage_Img, &YaIPS_Main_Histo, p1x, p1y, AoiXX, AoiYY);
    }
    break;

  case YAIPS_SHOW_INFO_RE_COLSUM:            // In rectangle show column sum

    // Has the AOI changed

    if( ! AoiChanged) {                     // AOI has NOT changed
      break;                                // If not: nothing to do
    }

    // Get column profile

    {
      int AoiXX, AoiYY, SizeShift, SizeFactor, LengthFirst;
      int p1x, p1y;                      // Aoi rectangle start point in image pixel coordinates

      YaIPS_ImageDispAoiClipAndCheck( &YaIPS_BigImageDisp, &AoiXX, &AoiYY, NULL, NULL);

      p1x = (int) (pYaIPS_ImageDisp->SubImage_x + 0.5) + (int) (pYaIPS_ImageDisp->AoiP1x / pYaIPS_ImageDisp->PixelImageToScreen);
      p1y = (int) (pYaIPS_ImageDisp->SubImage_y + 0.5) + (int) (pYaIPS_ImageDisp->AoiP1y / pYaIPS_ImageDisp->PixelImageToScreen);

      if( pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_1_1) {  // 1:1 or enlarged

        SizeShift  = pYaIPS_ImageDisp->DisplayResolution - 1;       // Shift is enlargement by power of 2
        SizeFactor = 1 << SizeShift;                               // Enlargement factor

      } else {    // Must be YAIPS_DISP_RESOLUTION_AUTO

        SizeShift  = 0;
        SizeFactor = 0;
      }

      if( SizeFactor > 1) {

        LengthFirst = SizeFactor - (pYaIPS_ImageDisp->AoiP1x % SizeFactor) - 1;
      } else {

        LengthFirst = 0;
      }

      AoiXX = (int) ( ceil((AoiXX + LengthFirst) / pYaIPS_ImageDisp->PixelImageToScreen));
      AoiYY = (int) ((AoiYY / pYaIPS_ImageDisp->PixelImageToScreen) + 0.5);

      YaIPS_ColRowSum_Measure( pYaIPS_ImageDisp->pImage_Img, &ShowColRowSumInfo, YAIPS_CRSUM_FLAG_COLSUM | YAIPS_CRSUM_FLAG_NORMALIZE, p1x, p1y, AoiXX, AoiYY);
    }
    break;

  case YAIPS_SHOW_INFO_RE_ROWSUM:            // In rectangle show row sum

    // Has the AOI changed

    if( ! AoiChanged) {                     // AOI has NOT changed
      break;                                // If not: nothing to do
    }

    // Get row profile

    {
      int AoiXX, AoiYY, SizeShift, SizeFactor, LengthFirst;
      int p1x, p1y;                      // Aoi rectangle start point in image pixel coordinates

      YaIPS_ImageDispAoiClipAndCheck( &YaIPS_BigImageDisp, &AoiXX, &AoiYY, NULL, NULL);

      p1x = (int) (pYaIPS_ImageDisp->SubImage_x + 0.5) + (int) (pYaIPS_ImageDisp->AoiP1x / pYaIPS_ImageDisp->PixelImageToScreen);
      p1y = (int) (pYaIPS_ImageDisp->SubImage_y + 0.5) + (int) (pYaIPS_ImageDisp->AoiP1y / pYaIPS_ImageDisp->PixelImageToScreen);

      if( pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_1_1) {  // 1:1 or enlarged

        SizeShift  = pYaIPS_ImageDisp->DisplayResolution - 1;       // Shift is enlargement by power of 2
        SizeFactor = 1 << SizeShift;                               // Enlargement factor

      } else {    // Must be YAIPS_DISP_RESOLUTION_AUTO

        SizeShift  = 0;
        SizeFactor = 0;
      }

      if( SizeFactor > 1) {

        LengthFirst = SizeFactor - (pYaIPS_ImageDisp->AoiP1y % SizeFactor) - 1;
      } else {

        LengthFirst = 0;
      }

      AoiXX = (int) ((AoiXX / pYaIPS_ImageDisp->PixelImageToScreen) + 0.5);
      AoiYY = (int) ( ceil((AoiYY + LengthFirst) / pYaIPS_ImageDisp->PixelImageToScreen));

      YaIPS_ColRowSum_Measure( pYaIPS_ImageDisp->pImage_Img, &ShowColRowSumInfo, YAIPS_CRSUM_FLAG_ROWSUM | YAIPS_CRSUM_FLAG_NORMALIZE, p1x, p1y, AoiXX, AoiYY);
    }
    break;

  case YAIPS_SHOW_INFO_2P_DIST_VER:          // Show distance of two points vertical
  case YAIPS_SHOW_INFO_2P_DIST_HOR:          // Show distance of two points horizontal

    // Has the AOI changed

    if( ! AoiChanged) {                     // AOI has NOT changed
      break;                                // If not: nothing to do
    }

    // Get ....

    break;
  } // end switch( ShowInfoMode)


  if( AoiChanged) {

    // Reset AOI changed flag bit. NOTE: this is the only place resetting this bit.
    pYaIPS_ImageDisp->Flags &= ~YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;
  }

  return( ShowInfoMode);
}

/************************************************************************************
 * YaIPS_GUI_DiffUsesDrawAfter_cb
 *
 * Additional drawings after the box inside was drawn
 *
 */
static void YaIPS_GUI_DiffUsesDrawAfter_cb( Fl_Widget *pW,
                                            void *pArg1,        // Pointer to Fl_YaIPS_ImageDisp_t
                                            void *pArg2)        // Optional pointer to ToolData
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  YaIPS_Fl_Box *pYaIPS_Box;
  int ShowInfoMode, p1x, p1y, p2x, p2y, DistInPixelX, DistInPixelY;
  int GapString2_3;
  char TempString1[ 256], TempString2[ 256], TempString3[ 256], TempString4[ 256];

  pYaIPS_ImageDisp = &YaIPS_BigImageDisp;
  pYaIPS_Box = pGUI_Box_DiffUses;           // Draw into this box widget

  if( pYaIPS_Box == NULL) {                 // Security test

    return;
  }

  // Ensure histogram is measured

  ShowInfoMode = YaIPS_GUI_DiffUsesCalcThings();          // Check for changed image

  // Points relative to image

  p1x = (int) (pYaIPS_ImageDisp->SubImage_x + 0.5) + (int) (pYaIPS_ImageDisp->AoiP1x / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
  p1y = (int) (pYaIPS_ImageDisp->SubImage_y + 0.5) + (int) (pYaIPS_ImageDisp->AoiP1y / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
  p2x = (int) (pYaIPS_ImageDisp->SubImage_x + 0.5) + (int) (pYaIPS_ImageDisp->AoiP2x / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
  p2y = (int) (pYaIPS_ImageDisp->SubImage_y + 0.5) + (int) (pYaIPS_ImageDisp->AoiP2y / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

  // Draw the histogram

  TempString1[ 0] = '\0';                    // Empty string
  TempString2[ 0] = '\0';
  GapString2_3 = 0;                          // Extra space between these strings
  TempString3[ 0] = '\0';
  TempString4[ 0] = '\0';

  switch( ShowInfoMode) {

  case YAIPS_SHOW_INFO_OFF:                  // Off. Show histogram big image

    strcpy( TempString1, pYaIPS_ImageDisp->ImageName);

    if( pYaIPS_ImageDisp->pImage_Img != NULL) {

      switch( pYaIPS_ImageDisp->pImage_Img->d()) {

      default:

        sprintf( TempString2, LangStringLookup( "&GUI_Main_Info_ISize1a=%d x %d %s, %d bytes/pixel"),
                 pYaIPS_ImageDisp->pImage_Img->data_w(), pYaIPS_ImageDisp->pImage_Img->data_h(),
                 LANGDEF_PIXEL_LC, pYaIPS_ImageDisp->pImage_Img->d());
        break;

      case 1:
      case 2:
      case 3:
      case 4:

        sprintf( TempString2, LangStringLookup( "&GUI_Main_Info_ISize1b=%d x %d %s, %s"),
                 pYaIPS_ImageDisp->pImage_Img->data_w(), pYaIPS_ImageDisp->pImage_Img->data_h(),
                 LANGDEF_PIXEL_LC, pYaIPS_PixelDepth_to_string( pYaIPS_ImageDisp->pImage_Img->d()));
        break;
      }

      if( YaIPS_Calib_Unit > YAIPS_CALIB_UNIT_PIXEL) {    // Any calibration with units

        sprintf( TempString3, "%s x %s",
            pYaIPS_Calib_PixelXVal2UnitStr( pYaIPS_ImageDisp->pImage_Img->data_w()),
            pYaIPS_Calib_PixelYVal2UnitStr( pYaIPS_ImageDisp->pImage_Img->data_h()));

      }
    }

#ifdef use_again
#ifdef _DEBUG
    int StrLen;

    StrLen = strlen( YaIPS_WorkingDirectory);

    if( StrLen > 20) {

      sprintf( TempString4, "WorkingDir: ..%s", YaIPS_WorkingDirectory + StrLen - 20);
    } else {
      sprintf( TempString4, "WorkingDir: %s", YaIPS_WorkingDirectory);
    }
#endif
#endif

    break;

  case YAIPS_SHOW_INFO_CU_LUT:           // Show current lookup table

    // Show lut

    YaIPS_ColMod_LUT_Draw( pYaIPS_Box,                           // Draw into this box widget
                   &pYaIPS_ImageDisp->DisplayColMod,            // Pointer to LUT data
                   (pYaIPS_Box->w() - YAIPS_LUT_N_POINTS) / 2,   // Left upper reference point for drawing
                   6 /*pYaIPS_Box->h() - YAIPS_LUT_N_POINTS - 4*/);

    break;

  case YAIPS_SHOW_INFO_CU_VAL:                  // At cursor show pixel value

    sprintf( TempString1, LangStringLookup( "&GUI_Main_Info_CuVal1=Pixel value under the mouse pointer."));
    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_IN_IMAGE) != 0 &&  // and mouse is over image
        (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_BUTT_ANY) == 0) {  // and NOT any mouse button pressed

      p1x = (int) (pYaIPS_ImageDisp->SubImage_x + 0.5) + (int) (pYaIPS_ImageDisp->MouseX / pYaIPS_ImageDisp->PixelImageToScreen);
      p1y = (int) (pYaIPS_ImageDisp->SubImage_y + 0.5) + (int) (pYaIPS_ImageDisp->MouseY / pYaIPS_ImageDisp->PixelImageToScreen);

      sprintf( TempString2, LangStringLookup( "&GUI_Main_Info_CuVal2=Position: %d/%d"), p1x, p1y);

    } else {

      sprintf( TempString2, LangStringLookup( "&GUI_Main_Info_CuVal3=Position:"));
    }

    break;

  case YAIPS_SHOW_INFO_RE_HISTO_ALL:        // Show histogram big image

    TempString2[ 0] = '\0';                 // Empty string

    YaIPS_Histo_Draw( pYaIPS_Box,                                  // Draw into this box widget
                     &YaIPS_Main_Histo,                            // Pointer to RGB histogram
                     (pYaIPS_Box->w() - YAIPS_HISTO_N_POINTS) / 2, // Left upper reference point for drawing
                     pYaIPS_Box->h() - YAIPS_HISTO_N_POINTS / 2 - 91,
                     YAIPS_HISTO_N_POINTS / 2 + 70,               // Height of curves
                     (char *)LangStringLookup( "&GUI_Main_Info_Histo2=Histogramm"),
                     TempString2);

    // Strings are written to screen above
    TempString1[ 0] = '\0';                    // Empty string
    TempString2[ 0] = '\0';                    // Empty string
    break;

  case YAIPS_SHOW_INFO_RE_HISTO_AOI:         // In rectangle show histogram

    sprintf( TempString2, LangStringLookup( "&GUI_Main_Info_AOI=AOI: %d-%d/%d-%d  %d*%d"),
                          p1x, p2x, p1y, p2y, p2x - p1x + 1, p2y - p1y + 1);

    YaIPS_Histo_Draw( pYaIPS_Box,                                  // Draw into this box widget
                     &YaIPS_Main_Histo,                            // Pointer to RGB histogram
                     (pYaIPS_Box->w() - YAIPS_HISTO_N_POINTS) / 2, // Left upper reference point for drawing
                     pYaIPS_Box->h() - YAIPS_HISTO_N_POINTS / 2 - 91,
                     YAIPS_HISTO_N_POINTS / 2 + 70,                // Height of curves
                     (char *)LangStringLookup( "&GUI_Main_Info_Histo2=Histogramm"),
                     TempString2);

    // Strings are written to screen above
    TempString1[ 0] = '\0';                    // Empty string
    TempString2[ 0] = '\0';                    // Empty string
    break;

  case YAIPS_SHOW_INFO_RE_COLSUM:          // In rectangle show column sum

    sprintf( TempString1, LangStringLookup( "&GUI_Main_Info_ColSum1=Column sum"));
    sprintf( TempString2, LangStringLookup( "&GUI_Main_Info_AOI=AOI: %d-%d/%d-%d  %d*%d"),
                          p1x, p2x, p1y, p2y, p2x - p1x + 1, p2y - p1y + 1);
    break;

  case YAIPS_SHOW_INFO_RE_ROWSUM:          // In rectangle show row sum

    sprintf( TempString1, LangStringLookup( "&GUI_Main_Info_RowSum1=Row sum"));
    sprintf( TempString2, "AOI: %d-%d/%d-%d  %d*%d", p1x, p2x, p1y, p2y, p2x - p1x + 1, p2y - p1y + 1);
    break;

  case YAIPS_SHOW_INFO_2P_DIST_VER:          // Show distance of two points vertical

    sprintf( TempString1, LangStringLookup( "&GUI_Main_Info_DiVer1=Y / vertical distance"));
    sprintf( TempString2, LangStringLookup( "&GUI_Main_Info_AOI=AOI: %d-%d/%d-%d  %d*%d"),
                          p1x, p2x, p1y, p2y, p2x - p1x + 1, p2y - p1y + 1);

    DistInPixelX = p2x - p1x;                             // Distance in pixel
    DistInPixelY = p2y - p1y;                             // Distance in pixel

    GapString2_3 = 6;                                     // Extra space between these strings
    sprintf( TempString3, LangStringLookup( "&GUI_Main_Info_Dist1=Ruler: %.02f * %.02f %s"),
                          DistInPixelX * YaIPS_Calib_UPP_X, DistInPixelY * YaIPS_Calib_UPP_Y, pYaIPS_Calib_Unit2String());
    sprintf( TempString4, LangStringLookup( "&GUI_Main_Info_Dist2=Area: %.02f %s²"),
                          DistInPixelX * YaIPS_Calib_UPP_X * DistInPixelY * YaIPS_Calib_UPP_Y, pYaIPS_Calib_Unit2String());

    break;

  case YAIPS_SHOW_INFO_2P_DIST_HOR:          // Show distance of two points horizontal

    sprintf( TempString1, LangStringLookup( "&GUI_Main_Info_DiHor1=X / horizontal distance"));
    sprintf( TempString2, LangStringLookup( "&GUI_Main_Info_AOI=AOI: %d-%d/%d-%d  %d*%d"),
                          p1x, p2x, p1y, p2y, p2x - p1x + 1, p2y - p1y + 1);

    DistInPixelX = p2x - p1x;                             // Distance in pixel
    DistInPixelY = p2y - p1y;                             // Distance in pixel

    GapString2_3 = 6;                                     // Extra space between these strings
    sprintf( TempString3, LangStringLookup( "&GUI_Main_Info_Dist1=Ruler: %.02f * %.02f %s"),
                          DistInPixelX * YaIPS_Calib_UPP_X, DistInPixelY * YaIPS_Calib_UPP_Y, pYaIPS_Calib_Unit2String());

    sprintf( TempString4, LangStringLookup( "&GUI_Main_Info_Dist2=Area: %.02f %s²"),
                          DistInPixelX * YaIPS_Calib_UPP_X * DistInPixelY * YaIPS_Calib_UPP_Y, pYaIPS_Calib_Unit2String());

    break;
  } // end switch( ShowInfoMode)

  // Write info strings to screen

  if( TempString1[ 0] != '\0' || TempString2[ 0] != '\0') {

    int x1, y1;

    fl_push_clip( pYaIPS_Box->x(), pYaIPS_Box->y(), pYaIPS_Box->w(), pYaIPS_Box->h());

    x1 = pYaIPS_Box->x() + 4;
    y1 = pYaIPS_Box->y() + 20;

    fl_font( FL_HELVETICA, 14);
    fl_color( FL_WHITE);

    if( TempString1[ 0] != '\0') {

      fl_draw( TempString1, x1, y1);

      y1 += 20;
    }

    if( TempString2[ 0] != '\0') {

      fl_draw( TempString2, x1, y1);

      y1 += 20;
    }

    y1 += GapString2_3;

    if( TempString3[ 0] != '\0') {

      fl_draw( TempString3, x1, y1);

      y1 += 20;
    }

    if( TempString4[ 0] != '\0') {

      fl_draw( TempString4, x1, y1);

      y1 += 20;
    }

    fl_line_style( 0);   // Reset to default
    fl_pop_clip();
  }

}

/************************************************************************************
 * IqeB_GUI_MainEndWorks
 *
 * Do work on shutting down the program.
 */

static void IqeB_GUI_MainEndWorks()
{

  // Close the current open windows.
  YaIPS_WindowsShutDown( YaIPS_Setting_Startup_WinRestore);

  IqeB_PreferencesUpdateChanges();  // update preferences database and save to to file

  Lang_FreeData();                  // Free language data
}

/************************************************************************************
 * Callback exit program
 */

static void MainExitCallback( Fl_Widget *,void *)
{

  // Skip if closing event is escape key

  if( Fl::event() == FL_SHORTCUT &&       // Keyboard shortcut event
      Fl::event_key() == FL_Escape) {     // and escape key

    return;
  }

  // ...

  IqeB_GUI_MainEndWorks();      // Do work on shutting down the program.

#ifdef WIN32
  // Protect against multiple running of this application

  if( MultipleRunning_pData != NULL) {

    UnmapViewOfFile( MultipleRunning_pData);
  }

  if( MultipleRunning_hMapFile != NULL) {

    CloseHandle( MultipleRunning_hMapFile);
  }
#endif

  exit(0);
}

/************************************************************************************
 * MainMenuPasteCallback
 *
 * Control + V was pressed on keyboard. Check clip board.
 */

static void MainMenuPasteCallback( Fl_Widget *,void *)
{

  if( Fl::clipboard_contains(Fl::clipboard_image) == 0) {  // NO image in the clipboard

    return;
  }

  if( YaIPS_BigImageDisp.pImage_Box == NULL) {             // Security test

    return;
  }

  Fl::paste( *YaIPS_BigImageDisp.pImage_Box, 1, Fl::clipboard_image); // try to find image in the clipboard

  return;
}

/************************************************************************************
 * Callback, hide window
 */

static void MainHideWinCallback( Fl_Return_Button* o, void*)
{

  ((Fl_Window*)(o->parent()))->hide();
}

/************************************************************************************
 * Callback, open about dialog
 */

static void MainAboutCallback( Fl_Widget *w, void *)
{
  Fl_Double_Window *about_panel;
  char TempStringTitle[ 256];
  char TempStringVersion[ 256];
  int x, y, TimeToBreakLoop, yy;
  char *pCreditText = NULL;

  sprintf( TempStringTitle, LangStringLookup( "&GUI_Main_About_1=About %s"), LangStringLookup( WIN_PROG_NAME));

#ifdef use_again
  about_panel = new Fl_Double_Window( 340, 100, TempStringTitle);
#else

  x = w->x();
  y = w->y();

  for( Fl_Widget* p = w->parent(); p != nullptr; p = p->parent()) {
    x += p->x();
    y += p->y();
  }

  about_panel = new Fl_Double_Window( x + w->w(), y, 340, 100, TempStringTitle);
#endif

  // version

  y = 10;

  yy = 32;
  strcpy( TempStringVersion, LangStringLookup( WIN_PROG_NAME));

  { Fl_Box* o = new Fl_Box( 16, y, 300, yy);
    o->copy_label(TempStringVersion);
    o->labelfont(1);
    o->labelsize(24);
    //o->labelcolor( fl_rgb_color( 103, 255, 142));
    o->labelcolor( fl_rgb_color( 104, 72, 35));
    o->align( Fl_Align( FL_ALIGN_TOP | FL_ALIGN_CENTER | FL_ALIGN_INSIDE));
  } // Fl_Box* o

  y += yy;

  yy = 46;
  sprintf( TempStringVersion, LangStringLookup( "&GUI_Main_About_2=%s\nVersion %s  %s"),
                              LangStringLookup( WIN_DEFAULT_TITLE),
                              WIN_PROG_VERSION_NR, LangStringLookup( WIN_PROG_VERSION_DATE));
  { Fl_Box* o = new Fl_Box( 16, y, 300, yy);
    o->copy_label(TempStringVersion);
    o->labelfont(1);
    o->labelsize(16);
    //o->labelcolor( fl_rgb_color( 103, 255, 142));
    o->labelcolor( fl_rgb_color( 104, 72, 35));
    o->align( Fl_Align( FL_ALIGN_TOP | FL_ALIGN_CENTER | FL_ALIGN_INSIDE));
  } // Fl_Box* o

  y += yy;

  { Fl_Box* o = new Fl_Box( 16, y, 300, 16, LangStringLookup( "&GUI_Main_About_3=Credits:"));
    o->align(Fl_Align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE));
    o->labelsize(16);
    o->labelcolor( fl_rgb_color( 128, 0, 0));
  } // Fl_Box* o

  y += 24;

  // prepare credits text

  pCreditText = (char *)malloc( strlen( LangStringLookup( IQE_CREDITS_TEXT)) + 1);   // Allocate temp buffer

  if( pCreditText != NULL) {  // Allocation OK

    strcpy( pCreditText, LangStringLookup( IQE_CREDITS_TEXT));

    char *pThis, *pFirst;

    pFirst = pCreditText;
    pThis  = pFirst;

    TimeToBreakLoop = false;
    for( ; ;) {

      if( *pThis == '\0' || *pThis == '\n') {  // Is end of line

        if( *pThis == '\n') {       // Is line feed

          *pThis = '\0';            // set end of string
        } else {                    // was end of string

          TimeToBreakLoop = true;   // break loop after this line is outputed
        }

        { Fl_Box* o = new Fl_Box( 16, y, 300, 16, pFirst);
          o->align(Fl_Align(FL_ALIGN_TOP_LEFT|FL_ALIGN_INSIDE));

          if( *pFirst == ' ') {   // line starts with blanks
            // A line starting with a blank is a internet address
            // (color it blue) and is the last line of a credits block.

            o->labelsize( 12);
            o->labelcolor( fl_rgb_color( 0, 0, 255));

            y += 20;
          } else {

            o->labelsize( 14);
            y += 16;
          }
        } // Fl_Box* o

        pFirst = pThis + 1;    // Begin of next line (if any)
      }

      if( TimeToBreakLoop) {  // Was end of string

        break;
      }

      pThis++;
    }
  }

  // button

  y += 32;

  { Fl_Return_Button* o = new Fl_Return_Button( about_panel->w() - 102, y - 32, 93, 25, LANGDEF_BUTTON_CLOSE);
    o->callback((Fl_Callback*)MainHideWinCallback);
  } // Fl_Return_Button* o

  about_panel->size( about_panel->w(), y);  // final size of window

  about_panel->set_modal();
  about_panel->end();

  // show dialog

  about_panel->show();

  // Hack: Add close button to window caption
  YaIPS_DialogAddCloseButton( about_panel);

  while( about_panel->shown()) {

    Fl::wait();
  }

  delete about_panel;

  if( pCreditText != NULL) {

    free( pCreditText);
  }
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

//
static int DropFile_cb( Fl_Widget *w, void *pFileNameArg, void *pImageDispArg, int SubWinIDx)
{
  char *pFileName;
  int ierr;

  // Support for big image display to modify AOIs of tool windows

  if( YaIPS_BigImageDisp.Plot3D_Active == false &&              // Main window NO 3D plot active
      YaIPS_BigImageDisp.ShowInfoMode == YAIPS_SHOW_INFO_OFF) { // and main window no measurement/info modes

    // Check for tool window shown on big image to support drop of files.

    ierr = YaIPS_ToolWinDropCallbackCall( YaIPS_BigImageDisp.ImageSourceID, w, pFileNameArg, pImageDispArg);

    if( ierr == 0) {     // Mouse callback of tool window was called

      return( 0);        // Mouse events are processed, return to caller
    }

    // continue ...
  }

  pFileName = (char *)pFileNameArg;                      // Filename

  ierr = YaIPS_ImageDispUpdateByFileName( &YaIPS_BigImageDisp, pFileName, YAIPS_WIN_ID_DROP_MAIN, true, true);

  return( ierr);
}

/************************************************************************************
 * IqeB_GUI_MainOpensToolWindowIntern
 *
 * Calls this if you want to open a tool window dialog explicit from a function.
 * Works together with IqeB_GUI_OpenGLIdleAction() to call the tool window create
 * function. Needed this stupid hack to get also tool windows opened from other
 * tool windows as brother and not as child.
 *
 */

static void IqeB_GUI_MainOpensToolWindowIntern( void *pToolWinFunc)
{
  static int PosOffset = 0;    // Position Offset for window creates

  if( pGUI_Main == NULL) {     // security test, no main window

    return;
  }

  Fl::first_window( pGUI_Main);   // Select the main window as top window

  if( pToolWinFunc != NULL) {     // have a function

    void (*pFunc)( int, int, int, int, int);

    pFunc = (void (*)( int, int, int, int, int))pToolWinFunc;   // convert to pointer to function

    // call the function
    pFunc(
#ifdef use_again
           pGUI_Main->x_root(),
           pGUI_Main->x_root() + pGUI_Main->decorated_w(),
           pGUI_Main->y_root() + PosOffset,
           pGUI_Main->y_root() + pGUI_Main->decorated_h() + PosOffset,
#else
           // Default position is in main window right side
           pGUI_Main->x_root() + pGUI_Main_RightSide->x(),
           pGUI_Main->x_root() + pGUI_Main_RightSide->x() + 16,
           pGUI_Main->y_root() + pGUI_Main_RightSide->y() + 36 + PosOffset,
           pGUI_Main->y_root() + pGUI_Main_RightSide->y() + pGUI_Main_RightSide->h() + 36 + PosOffset,
#endif
		      -1);                                                                  // SubWinIDx is not valid

    PosOffset += 32;   // Other offset for next create

    if( PosOffset >= 256) {

      PosOffset = 0;
    }
  }
}

/************************************************************************************
 * IqeB_GUI_But_Tool_OpenWin_Callback
 *
 * Used for menu definitions for opening a window.
 */

void IqeB_GUI_But_Tool_OpenWin_Callback( Fl_Widget *w, void *pValueArg)
{

  if( pGUI_Main == NULL) {     // security test, no main window

    return;
  }

  //x/IqeB_DispMessage( DRAW_MSG_COL_OK, NULL, NULL, NULL);  // Clear message area

  IqeB_GUI_MainOpensToolWindowIntern( pValueArg);              // Go call it.

}

/************************************************************************************
 * IqeB_Main_Load_Callback
 */

static void IqeB_Main_Load_Callback( Fl_Widget *w, void *pValueArg)
{
  Fl_Native_File_Chooser fc;
  char FileFilter[ 1024];
  char *pFileName;
  int ierr;

  if( pGUI_Main == NULL) {     // security test, no main window

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

  YaIPS_ImageDispUpdateByFileName( &YaIPS_BigImageDisp, pFileName,
                                  YAIPS_WIN_ID_DROP_MAIN, true, true);

  // Remember last used directory
  IqeB_FileGetPath( pFileName, YaIPS_BrowserDirectory, sizeof( YaIPS_BrowserDirectory));

  IqeB_FileNormPathCharsAndCWD( YaIPS_BrowserDirectory);  // Ensure normalized path characters and working directory
}

/************************************************************************************
 * IqeB_Main_SaveCopy_Callback
 */

static void IqeB_Main_SaveCopy_Callback( Fl_Widget *pWidget, void *pValueArg, int DoFileSave)
{
  Fl_Native_File_Chooser fc;
  char FileFilter[ 1024];
  char *pFileName;
  int ierr, ArgInt, x, y, w, h, iFileType;
  char TempFileName[ MAX_FILENAME_LEN];
  uchar *p;
  static char *FileTypes[ YAIPS_IMAGE_FILES_WRITE_TAB_N] = { YAIPS_IMAGE_FILES_WRITE_TAB_DATA };

  if( pGUI_Main == NULL) {     // security test, no main window

    return;
  }

  ArgInt = (long long)pValueArg;

  // Initialize the file chooser. Only can save png images

  iFileType = 0;        // Set this so compiler don't warn for uninitialized variable

  if( DoFileSave) {     // Save to file ?

    strcpy( FileFilter, YAIPS_IMAGE_FILES_WRITE_BROWSER);

    fc.filter( FileFilter);
    fc.options( Fl_Native_File_Chooser::SAVEAS_CONFIRM | Fl_Native_File_Chooser::USE_FILTER_EXT);

    iFileType = YaIPS_Main_WriteFileType_Last;

    if( iFileType < 0 || iFileType >= YAIPS_IMAGE_FILES_WRITE_TAB_N) {    // Security test out of range

      iFileType = 0;
      YaIPS_Main_WriteFileType_Last = 0;
    }

    fc.filter_value( YaIPS_Main_WriteFileType_Last);  // What file type to use

    pFileName = NULL;      // NO preset file name

    switch( ArgInt) {

    case 0:                // Save latched display image
    default:

      if( YaIPS_BigImageDisp.ImageName[ 0] != 0) {

        pFileName = YaIPS_BigImageDisp.ImageName;

      } else {

        pFileName = LangStringLookup( "&GUI_Main_Save5=Image.png");
      }

      break;

    case 1:              // Save display area

      pFileName = LangStringLookup( "&GUI_Main_Save6=Display.png");

      break;

    case 2:              // Save info area

      pFileName = LangStringLookup( "&GUI_Main_Save7=Info.png");

      break;

    case 3:              // Save application screenshot

      pFileName = LangStringLookup( "&GUI_Main_Save8=Screenshot.png");

      break;
    }  // end switch( ArgInt)

    if( pFileName != NULL) {

      // Remove the file extension of the preset file name.
      IqeB_FileEnsureExtension( pFileName, NULL /*FileTypes[ iFileType]*/, TempFileName, sizeof( TempFileName));
      fc.preset_file( TempFileName);
    }

    fc.title( LangStringLookup( "&GUI_Main_Save2=Save image"));
    fc.type( Fl_Native_File_Chooser::BROWSE_SAVE_FILE);  // need this if file doesn't exist yet
    fc.directory( YaIPS_BrowserDirectory);               // Set browser directory
    ierr = fc.show();                                    // Open file chooser dialog

    if( ierr != 0) {      // User cancelled or error

      return;
    }

    // Have a filename here. Ensure a png file extension.

    pFileName = (char *)fc.filename();    // Name of file
    iFileType = fc.filter_value();        // What file to save

    if( iFileType >= 0 && iFileType < YAIPS_IMAGE_FILES_WRITE_TAB_N) {

      IqeB_FileEnsureExtension( pFileName, FileTypes[ iFileType], TempFileName, sizeof( TempFileName));

      YaIPS_Main_WriteFileType_Last = iFileType;
    } else {

      IqeB_FileEnsureExtension( pFileName, (char *)"png", TempFileName, sizeof( TempFileName));
      YaIPS_Main_WriteFileType_Last = 0;
    }

    IqeB_FileNormPathCharsAndCWD( TempFileName);  // Ensure normalized path characters and working directory

    pFileName = TempFileName;

    // Remember last used directory
    IqeB_FileGetPath( pFileName, YaIPS_BrowserDirectory, sizeof( YaIPS_BrowserDirectory));
  }

  // ...

  x = y = w = h = 0;         // Flag no screenshot
  p = NULL;

  switch( ArgInt) {

  case 0:                // Save latched display image
  default:

    if( YaIPS_BigImageDisp.pImage_Img != NULL) {   // Have a latched image

      if( DoFileSave) {     // Save to file ?

        if( iFileType >= 0 && iFileType < YAIPS_IMAGE_FILES_WRITE_TAB_N) {

          YaIPS_Image_Write( pFileName, YaIPS_BigImageDisp.pImage_Img);
        } else {

          YaIPS_Image_Write_PNG( pFileName, YaIPS_BigImageDisp.pImage_Img);
        }

      } else {              // Copy to clipboard

        YaIPS_ImageDispCopyImage_cb( NULL, &YaIPS_BigImageDisp);
      }
    }

    break;

  case 1:              // Save display area

    x = YaIPS_BigImageDisp.BigImage_bx;
    y = YaIPS_BigImageDisp.BigImage_by;
    w = YaIPS_BigImageDisp.BigImage_bw;
    h = YaIPS_BigImageDisp.BigImage_bh;

    break;

  case 2:              // Save info area

    if( pGUI_Box_DiffUses != NULL) {

      x = pGUI_Box_DiffUses->x();
      y = pGUI_Box_DiffUses->y();
      w = pGUI_Box_DiffUses->w();
      h = pGUI_Box_DiffUses->h();
    }

    break;

  case 3:              // Save application screenshot

    x = 0;
    y = 0;
    w = pGUI_Main->w();
    h = pGUI_Main->h();

    break;
  }  // end switch( ArgInt)


  if( w != 0 && h != 0)      {    // Have a window size set

    if( DoFileSave) {     // Save to file ?

      p = fl_read_image( 0, x, y, w, h);

      if( p != NULL) {

        if( iFileType >= 0 && iFileType < YAIPS_IMAGE_FILES_WRITE_TAB_N) {

          YaIPS_Image_Write( pFileName, p, w, h, 3);
        } else {

          YaIPS_Image_Write_PNG( pFileName, p, w, h, 3, 0);
        }
      }

    } else {              // Copy to clipboard

      Fl_Copy_Surface *copy_surf;

#ifdef use_again
      if( x == 0 && y == 0 && w == pGUI_Main->w() && h == pGUI_Main->h() &&  // Is complete window
          pGUI_Main->as_window() && !pGUI_Main->parent()) {                  // and is window

        copy_surf = new Fl_Copy_Surface( pGUI_Main->as_window()->decorated_w(), pGUI_Main->as_window()->decorated_h());
        Fl_Surface_Device::push_current( copy_surf);
        copy_surf->draw_decorated_window( pGUI_Main->as_window(), 0, 0);

      } else {

        copy_surf = new Fl_Copy_Surface( w, h);
        Fl_Surface_Device::push_current( copy_surf);
        copy_surf->print_window_part( pGUI_Main, x, y, w, h);
      }
#else
      // NOTE 21.08.2025 RR: Paste full window with caption don't work.
      // I don't know why. So paste it without caption.

      copy_surf = new Fl_Copy_Surface( w, h);
      Fl_Surface_Device::push_current( copy_surf);
      copy_surf->print_window_part( pGUI_Main, x, y, w, h);
#endif

      delete copy_surf;
      Fl_Surface_Device::pop_current();
    }
  }

  if( p != NULL) {

    delete[]p;
  }
}

/************************************************************************************
 * IqeB_Main_Save_Callback
 */

static void IqeB_Main_Save_Callback( Fl_Widget *pWidget, void *pValueArg)
{

  IqeB_Main_SaveCopy_Callback( pWidget, pValueArg, true);
}

/************************************************************************************
 * IqeB_Main_Copy_Callback
 */

static void IqeB_Main_Copy_Callback( Fl_Widget *pWidget, void *pValueArg)
{

  IqeB_Main_SaveCopy_Callback( pWidget, pValueArg, false);
}

/************************************************************************************
 * IqeB_Main_Update_Display_Settings
 *
 * Update display settings of the main menu after changes.
 */

static void IqeB_Main_Update_Display_Settings()
{

  DUseDataChange                 = -1;                          // Reset monitor source image change
  YaIPS_BigImageDisp.Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;     // Set AOI changed flag bit

  // Update some buttons.
  // Doing this in MyWinUpdate() does no work

  if( pCBox_Col_Inverted->value() != YaIPS_BigImageDisp.DisplayColMod.Invert) {

    pCBox_Col_Inverted->value( YaIPS_BigImageDisp.DisplayColMod.Invert);
  }

  if( pCBox_Col_Darken->value() != YaIPS_BigImageDisp.DisplayColMod.Darken) {

    pCBox_Col_Darken->value( YaIPS_BigImageDisp.DisplayColMod.Darken);
  }

  if( pCBox_3D_Active->value() != YaIPS_BigImageDisp.Plot3D_Active) {

    pCBox_3D_Active->value( YaIPS_BigImageDisp.Plot3D_Active);
  }

  if( pCBox_3D_DrawGrid->value() != YaIPS_BigImageDisp.Plot3D_DrawGrid) {

    pCBox_3D_DrawGrid->value( YaIPS_BigImageDisp.Plot3D_DrawGrid);
  }

  if( pCBox_3D_Inverted->value() != YaIPS_BigImageDisp.Plot3D_Inverted) {

    pCBox_3D_Inverted->value( YaIPS_BigImageDisp.Plot3D_Inverted);
  }

  // Update display

  YaIPS_ImageDispCalcSizes( &YaIPS_BigImageDisp, YAIPS_IDISP_FLAG_FORCE_UPDATE,               // Check sizes
                           YaIPS_BigImageDisp.BigImage_sx + YaIPS_BigImageDisp.BigImage_sw / 2,  // position is center of window
                           YaIPS_BigImageDisp.BigImage_sy + YaIPS_BigImageDisp.BigImage_sh / 2);

  YaIPS_ImageDispDrawUpdate( &YaIPS_BigImageDisp, true);
}

/************************************************************************************
 * IqeB_Main_Reset_Display
 *
 * Reset the display settings
 */

static void IqeB_Main_Reset_Display( Fl_Widget *pWidget, void *pValueArg)
{

  YaIPS_BigImageDisp.DisplayResolution  = 0;

  YaIPS_BigImageDisp.DisplayColMod.Style = 0;

  YaIPS_BigImageDisp.DisplayColMod.FalseColor = 0;
  YaIPS_BigImageDisp.DisplayColMod.Invert = 0;
  YaIPS_BigImageDisp.DisplayColMod.Darken = 0;

  YaIPS_BigImageDisp.Plot3D_Active = 0;
  YaIPS_BigImageDisp.Plot3D_DrawGrid = 0;
  YaIPS_BigImageDisp.Plot3D_Inverted = 0;
  YaIPS_BigImageDisp.Plot3D_Azimuth = 0;
  YaIPS_BigImageDisp.Plot3D_Elevation = 45;

  OverlayStyle = 0;
  OverlayColor = FL_YELLOW;

  YaIPS_BigImageDisp.ShowInfoMode = 0;

  //  Update display settings of the main menu after changes.

  IqeB_Main_Update_Display_Settings();

  // Save changed preference settings

  IqeB_PreferencesUpdateChanges();           // update preferences database and save to to file

}

/************************************************************************************
 * IqeB_Main_Toogle_Fullscreen
 *
 * Full screen on/off
 */

static void IqeB_Main_Toogle_Fullscreen( Fl_Widget *pWidget, void *pValueArg)
{
  int TempRightSideWidth;

  if( pGUI_Main->fullscreen_active())  {      // Full screen is active

    pGUI_Main->fullscreen_off();

    TempRightSideWidth = RightSideWidth;

  } else {                                    // Full screen is NOT active

    pGUI_Main->fullscreen();

    TempRightSideWidth = RightSideWidthFS;
  }

  // Reconstruct right side with

  if( TempRightSideWidth != pGUI_Main_RightSide->w()) {   // If width is different

    int BigImage_bx, BigImage_by, BigImage_bw, BigImage_bh;

    BigImage_bx = pTile_RightSide->x();
    BigImage_by = pTile_RightSide->y();
    BigImage_bw = pTile_RightSide->w();
    BigImage_bh = pTile_RightSide->h();

    // Check size of right side

    if( BigImage_bw - RightSideWidthFS < WITH_MIN_IMAGE_BOX) {

      TempRightSideWidth = BigImage_bw - WITH_MIN_IMAGE_BOX;
    }

    if( TempRightSideWidth < WITH_MIN_TOOL_BOX) {

      TempRightSideWidth = WITH_MIN_TOOL_BOX;
    }

    YaIPS_BigImageDisp.pImage_Box->resize( BigImage_bx, BigImage_by, BigImage_bw - TempRightSideWidth, BigImage_bh);

    pGUI_Main_RightSide->resize( BigImage_bx + BigImage_bw - TempRightSideWidth, BigImage_by, TempRightSideWidth, BigImage_bh);

    pTile_RightSide->redraw();
  }
}

/************************************************************************************
 * IqeB_Main_PresetLoad
 *
 * Load presets from file
 *
 * *pValueArg:  0  Load presets from file
 *              1  Reset presets
 *
 * NOTE: Language settings is not changed.
 */

static void IqeB_Main_PresetLoad( Fl_Widget *pWidget, void *pValueArg)
{
  char Language_Save[ 512];  // Save language name
  int TempRightSideWidth, TempFullScreenActive;

  strcpy( Language_Save, YaIPS_Setting_Language);          // Save current selected GUI language

  RightSide_LastSkip = 5;                                  // Reset check for right side position change

  // This resets all presets without question
  IqeB_PresetLoad_cb( NULL, pValueArg);

  strcpy( YaIPS_Setting_Language, Language_Save);          // Restore last selected GUI language

  // Manage full screen toggle

  TempFullScreenActive = pGUI_Main->fullscreen_active();

  if( TempFullScreenActive != FullScreenActive) {     // Must change full screen mode

    if( FullScreenActive == 0) {                      // full screen is not set

      pGUI_Main->fullscreen_off();

    } else {                                          // Full screen is NOT active

      pGUI_Main->fullscreen();
    }
  }

  // Have to adapt screen position and size

  if( FullScreenActive == 0) {    // full screen is not set

    TempRightSideWidth = RightSideWidth;

    if( pGUI_Main->x_root() != YaIPS_Main_WinPosX ||
        pGUI_Main->y_root() != YaIPS_Main_WinPosY ||
        pGUI_Main->w() != YaIPS_Main_WinSizeX ||
        pGUI_Main->h() != YaIPS_Main_WinSizeY) {

      pGUI_Main->resize( YaIPS_Main_WinPosX, YaIPS_Main_WinPosY, YaIPS_Main_WinSizeX, YaIPS_Main_WinSizeY);
    }

  } else {                        // full screen is set

    TempRightSideWidth = RightSideWidthFS;
  }

  // Reconstruct right side with

  if( TempRightSideWidth != pGUI_Main_RightSide->w()) {   // If width is different

    int BigImage_bx, BigImage_by, BigImage_bw, BigImage_bh;

    BigImage_bx = pTile_RightSide->x();
    BigImage_by = pTile_RightSide->y();
    BigImage_bw = pTile_RightSide->w();
    BigImage_bh = pTile_RightSide->h();

    // Check size of right side

    if( BigImage_bw - RightSideWidthFS < WITH_MIN_IMAGE_BOX) {

      TempRightSideWidth = BigImage_bw - WITH_MIN_IMAGE_BOX;
    }

    if( TempRightSideWidth < WITH_MIN_TOOL_BOX) {

      TempRightSideWidth = WITH_MIN_TOOL_BOX;
    }

    YaIPS_BigImageDisp.pImage_Box->resize( BigImage_bx, BigImage_by, BigImage_bw - TempRightSideWidth, BigImage_bh);

    pGUI_Main_RightSide->resize( BigImage_bx + BigImage_bw - TempRightSideWidth, BigImage_by, TempRightSideWidth, BigImage_bh);

    pTile_RightSide->redraw();
  }

  //  Update display settings of the main menu after changes.

  IqeB_Main_Update_Display_Settings();

}

/************************************************************************************
 * YaIPS_BigImageReso_Callback
 *
 * Image resolution radiobutton was pressed
 */

static void YaIPS_BigImageReso_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                             // get value to set

  if( YaIPS_BigImageDisp.DisplayResolution == Value) {    // Value will not change

    return;                                              // Exit, nothing to do
  }

  YaIPS_BigImageDisp.DisplayResolution = Value;           // Set new value

  YaIPS_BigImageDisp.Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;     // Set AOI changed flag bit

  // Save changed preference settings

  IqeB_PreferencesUpdateChanges();           // update preferences database and save to to file

  // Update display

  YaIPS_ImageDispCalcSizes( &YaIPS_BigImageDisp, YAIPS_IDISP_FLAG_FORCE_UPDATE,               // Check sizes
                           YaIPS_BigImageDisp.BigImage_sx + YaIPS_BigImageDisp.BigImage_sw / 2,  // position is center of window
                           YaIPS_BigImageDisp.BigImage_sy + YaIPS_BigImageDisp.BigImage_sh / 2);

  YaIPS_ImageDispDrawUpdate( &YaIPS_BigImageDisp, true);
}

/************************************************************************************
 * YaIPS_BigImageOverlay_Callbac
 *
 * Image overlay radiobutton was pressed
 */

static void YaIPS_BigImageOverlay_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                               // get value to set

  if( OverlayStyle == Value) {                             // Value will not change

    return;                                                // Exit, nothing to do
  }

  OverlayStyle = Value;                                    // Set new value

  // Save changed preference settings

  IqeB_PreferencesUpdateChanges();                         // update preferences database and save to to file

  // Update display

  YaIPS_ImageDispDrawUpdate( &YaIPS_BigImageDisp, true);
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

    // Save changed preference settings

    IqeB_PreferencesUpdateChanges();           // update preferences database and save to to file

    // Update display

    YaIPS_ImageDispDrawUpdate( &YaIPS_BigImageDisp, true);
  }
}

/************************************************************************************
 * YaIPS_BigImageColMod_Callback
 *
 * Color modification radiobutton was pressed
 */

static void YaIPS_BigImageColMod_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                               // get value to set

  if( YaIPS_BigImageDisp.DisplayColMod.Style == Value) {    // Value will not change

    return;                                                // Exit, nothing to do
  }

  YaIPS_BigImageDisp.DisplayColMod.Style = Value;           // Set new value

  // Save changed preference settings

  IqeB_PreferencesUpdateChanges();           // update preferences database and save to to file

  // Update display

  YaIPS_ImageDispDrawUpdate( &YaIPS_BigImageDisp, true);
}

/************************************************************************************
 * YaIPS_BigImageSowInfo_Callback
 *
 * Info mode radio button has changed.
 */

static void YaIPS_BigImageSowInfo_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                       // get value to set

  if( YaIPS_BigImageDisp.ShowInfoMode == Value) {   // Value will not change

    return;                                        // Exit, nothing to do
  }

  DUseDataChange                 = -1;                          // Reset monitor source image change
  YaIPS_BigImageDisp.Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;     // Set AOI changed flag bit
  if( pGUI_Box_DiffUses != NULL) {                 // Not zero until now

    pGUI_Box_DiffUses->redraw();                   // Force redraw
  }

  YaIPS_BigImageDisp.ShowInfoMode = Value;          // Set new value

  // Save changed preference settings

  IqeB_PreferencesUpdateChanges();                 // update preferences database and save to to file

  // Update display

  YaIPS_ImageDispDrawUpdate( &YaIPS_BigImageDisp, true);
}

/************************************************************************************
 * YaIPS_BImg_CBox_SetValue_Callback
 *
 * This is usable for Fl_Valuator, Fl_Choice, Fl_Check_Button
 */

static void YaIPS_BImg_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  int *pValue;

  pYaIPS_ImageDisp = &YaIPS_BigImageDisp;

  pValue = (int *)pValueArg;              // get pointer to associated variable

  if( pValue == &pYaIPS_ImageDisp->Plot3D_Azimuth ||
      pValue == &pYaIPS_ImageDisp->Plot3D_Elevation) {

    // Fl_Valuator, Fl_Slider or Fl_Value_Slider

    Fl_Valuator *pThis;

    pThis  = (Fl_Valuator *)w;
    *pValue = pThis->value();               // update the variable

  } else if( pValue == &pYaIPS_ImageDisp->DisplayColMod.FalseColor) {

      // Fl_Choice

      Fl_Choice *pThis;

      pThis  = (Fl_Choice *)w;
      *pValue = pThis->value();               // update the variable

   } else {

     Fl_Button *pThis;

     pThis  = (Fl_Check_Button *)w;
     *pValue = pThis->value();               // update the variable
   }

  // If any of the LUT settings are changed
  if( pValue == &pYaIPS_ImageDisp->DisplayColMod.FalseColor ||
      pValue == &pYaIPS_ImageDisp->DisplayColMod.Invert ||
      pValue == &pYaIPS_ImageDisp->DisplayColMod.Darken) {

    pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_LUT_CHANGED;    // Need LUT recalculation
  }

  // Info area needs to be redrawn
  if( pValue == &YaIPS_BigImageDisp.Plot3D_Active) {

    pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;  // Set AOI changed flag bit
  }

  // Save changed preference settings

  IqeB_PreferencesUpdateChanges();        // update preferences database and save to to file

  // Update display

  YaIPS_ImageDispDrawUpdate( &YaIPS_BigImageDisp, true);
}

/************************************************************************************
 * Main dialog menus
 */

#define GUI_MENUHEIGHT       24          // Height of menu bar
#define GUI_LEFT_SIDE_WIDTH (256 + 18)   // Left side group with


/************************************************************************************
 * Main_Menu_AddItems
 *
 * Add item to maim menu bar.
 * Adding the items explicit allows addition of language translations.
 */

static Fl_Menu_Bar *main_menubar;    // Pointer to main menu bar data element

static void Main_Menu_AddItems( Fl_Menu_Bar *pM)
{

  // Sub menu File

  pM->add( LangStringLookup( "&GUI_Main_Menu1a=File/&Open image"),            FL_COMMAND+'o', IqeB_Main_Load_Callback, 0, FL_MENU_DIVIDER);
  pM->add( LangStringLookup( "&GUI_Main_Menu1b=File/&Save image"),            FL_COMMAND+'s', IqeB_Main_Save_Callback, 0);
  pM->add( LangStringLookup( "&GUI_Main_Menu1c=File/Save &display"),          FL_COMMAND+'d', IqeB_Main_Save_Callback, (void *)1);
  pM->add( LangStringLookup( "&GUI_Main_Menu1d=File/Save &Info area"),        FL_COMMAND+'a', IqeB_Main_Save_Callback, (void *)2);
  pM->add( LangStringLookup( "&GUI_Main_Menu1e=File/Save screens&hot"),       FL_COMMAND+'h', IqeB_Main_Save_Callback, (void *)3, FL_MENU_DIVIDER);
  pM->add( LangStringLookup( "&GUI_Main_Menu1f=File/Load presets"),                        0, IqeB_Main_PresetLoad, 0);
  pM->add( LangStringLookup( "&GUI_Main_Menu1g=File/Save presets"),                        0, IqeB_PresetSave_cb, 0);
  pM->add( LangStringLookup( "&GUI_Main_Menu1h=File/Reset presets"),                       0, IqeB_Main_PresetLoad, (void *)1, FL_MENU_DIVIDER);
#ifdef use_again
#ifdef _DEBUG
  pM->add( LangStringLookup( "&GUI_Main_Menu1m=File/TEST show all"),           FL_COMMAND+'0', YaIPS_ToolWinTestAction_cb, (void *)YAIPS_TWIN_ACTION_SHOW_ALL);
  pM->add( LangStringLookup( "&GUI_Main_Menu1n=File/TEST hide all"),           FL_COMMAND+'1', YaIPS_ToolWinTestAction_cb, (void *)YAIPS_TWIN_ACTION_HIDE_ALL);
  pM->add( LangStringLookup( "&GUI_Main_Menu1o=File/TEST iconize all"),        FL_COMMAND+'2', YaIPS_ToolWinTestAction_cb, (void *)YAIPS_TWIN_ACTION_ICONIZE_ALL);
  pM->add( LangStringLookup( "&GUI_Main_Menu1p=File/TEST hide/show all"),      FL_COMMAND+'3', YaIPS_ToolWinTestAction_cb, (void *)YAIPS_TWIN_ACTION_HIDE_SHOW_ALL);
  pM->add( LangStringLookup( "&GUI_Main_Menu1q=File/TEST redraw all"),         FL_COMMAND+'4', YaIPS_ToolWinTestAction_cb, (void *)YAIPS_TWIN_ACTION_REDRAW_ALL, FL_MENU_DIVIDER);
#endif
#endif
  pM->add( LangStringLookup( "&GUI_Main_Menu1u=File/S&ettings ..."),           FL_COMMAND+'p', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)YaIPS_GUI_SettingsWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu1v=File/&Calibration ..."),                     0, IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_CalibrationWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu1w=File/Custom colors ..."),                    0, IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_CustomColorWin, FL_MENU_DIVIDER);
  pM->add( LangStringLookup( "&GUI_Main_Menu1z=File/&Quit"),                   FL_COMMAND+'q', MainExitCallback);

  // Invisible menu entries. Used for some control key catching

  // Sub menu Edit

  pM->add( LangStringLookup( "&GUI_Main_Menu2a=Edit/&Paste image"),            FL_COMMAND+'v', MainMenuPasteCallback, 0, FL_MENU_DIVIDER);
  pM->add( LangStringLookup( "&GUI_Main_Menu2b=Edit/&Copy image"),             FL_COMMAND+'c', IqeB_Main_Copy_Callback, 0);
  pM->add( LangStringLookup( "&GUI_Main_Menu2c=Edit/Copy &display"),     FL_SHIFT + FL_COMMAND+'d', IqeB_Main_Copy_Callback, (void *)1);
  pM->add( LangStringLookup( "&GUI_Main_Menu2d=Edit/Copy &Info area"),   FL_SHIFT + FL_COMMAND+'a', IqeB_Main_Copy_Callback, (void *)2);
  pM->add( LangStringLookup( "&GUI_Main_Menu2e=Edit/Copy screens&hot"),  FL_SHIFT + FL_COMMAND+'h', IqeB_Main_Copy_Callback, (void *)3, FL_MENU_DIVIDER);
  pM->add( LangStringLookup( "&GUI_Main_Menu2h=Edit/&Reset display"),          FL_COMMAND+'r', IqeB_Main_Reset_Display, 0);
  pM->add( LangStringLookup( "&GUI_Main_Menu2i=Edit/Toggle &full screen"),     FL_COMMAND+'f', IqeB_Main_Toogle_Fullscreen, 0);


  // Sub menu image

  // Sub menu input
  pM->add( LangStringLookup( "&GUI_Main_Menu3In1=Image/Source/&Camera"),           FL_COMMAND+'k', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_CameraWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3In2=Image/Source/&New image"),        FL_COMMAND+'n', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_GenImageWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3In3=Image/Source/&Image viewer"),     FL_COMMAND+'i', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_ImageFileWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3In4=Image/Source/Video viewer"),                   0, IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_VideoReadWin);

  // Sub menu processing
  pM->add( LangStringLookup( "&GUI_Main_Menu3Pr1=Image/Processing/Filter"),       FL_COMMAND+'1', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_FilterWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3Pr2=Image/Processing/Color"),        FL_COMMAND+'2', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_ColorWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3Pr3=Image/Processing/Geometry"),     FL_COMMAND+'3', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_GeoTranWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3Pr4=Image/Processing/Calculation"),  FL_COMMAND+'4', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_CombineWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3Pr5=Image/Processing/Correlation"),  FL_COMMAND+'5', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_CrcdfWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3Pr6=Image/Processing/Objects"),      FL_COMMAND+'6', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_ObjectsWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3Pr7=Image/Processing/Other"),        FL_COMMAND+'7', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_OtherWin);

  // Sub menu output
  pM->add( LangStringLookup( "&GUI_Main_Menu3Ou1=Image/Output/Video writer"),                  0, IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_VideoWriteWin);

  // Sub menu inspection
  pM->add( LangStringLookup( "&GUI_Main_Menu3Ip1=Image/Inspection/Reference image"),     FL_ALT+'1', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_InspRefImgWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3Ip2=Image/Inspection/Color testing"),       FL_ALT+'2', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_InspColorWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3Ip3=Image/Inspection/Position correction"), FL_ALT+'3', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_PosCorrWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3Ip4=Image/Inspection/Edges"),               FL_ALT+'4', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_EdgesWin);
  pM->add( LangStringLookup( "&GUI_Main_Menu3Ip5=Image/Inspection/Compare"),             FL_ALT+'5', IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_InspCompareWin);

  // Sub menu other
  pM->add( LangStringLookup( "&GUI_Main_Menu3Ot1=Image/Other/Overlay"),           0, IqeB_GUI_But_Tool_OpenWin_Callback, (void *)IqeB_GUI_OverlayWin);

  // Sub menu Help

  pM->add( LangStringLookup( "&GUI_Main_Menu4a=Help/&About YaIPS ..."), 0, MainAboutCallback);
}

/************************************************************************************
 * Presets for the Main_GUI
 *
 * Main Window GUI
 */

 static T_GUI_PreferenceEntry MyPreferences[] =
 {

  // Window size

  { PREF_T_INT, "WinSizeX", "100", &YaIPS_Main_WinSizeX},  // NOTE: values will be clipped against YAIPS_MAIN_SIZE_X_MIN / YAIPS_MAIN_SIZE_Y_MIN
  { PREF_T_INT, "WinSizeY", "100", &YaIPS_Main_WinSizeY},

  // File browser group

  { PREF_T_STRING, "FileBrowserLastDir",     "",  YaIPS_BrowserDirectory, sizeof( YaIPS_BrowserDirectory) - 1},
  { PREF_T_STRING, "FileBrowserLastVideo",   "",  YaIPS_BrowserDirVideos, sizeof( YaIPS_BrowserDirVideos) - 1},

  // Other

  { PREF_T_INT, "ImageSourceID_Last",          "0",  &YaIPS_Main_ImageSourceID_Last},                   // Default:
  { PREF_T_INT,     "RightSideWidth",        "160",  &RightSideWidth},                                  // Default:
  { PREF_T_INT,   "RightSideWidthFS",        "256",  &RightSideWidthFS},                                // Default:
  { PREF_T_INT,   "FullScreenActive",          "0",  &FullScreenActive},                                // Default:
  { PREF_T_INT, "WriteFileType_Last",          "0",  &YaIPS_Main_WriteFileType_Last},                   // Default:

  // Display style settings

  { PREF_T_INT, "YaIPS_BigImageResolution",      "0",  &YaIPS_BigImageDisp.DisplayResolution},         // Default: YAIPS_DISP_RESOLUTION_AUTO
  { PREF_T_INT, "YaIPS_BigImageColModStyle",     "0",  &YaIPS_BigImageDisp.DisplayColMod.Style},       // Default: YAIPS_DISP_COLMOD_NORMAL
  { PREF_T_INT, "YaIPS_BigImageColModFalseC",    "0",  &YaIPS_BigImageDisp.DisplayColMod.FalseColor},  // Default: false color off
  { PREF_T_INT, "YaIPS_BigImageColModInvert",    "0",  &YaIPS_BigImageDisp.DisplayColMod.Invert},      // Default: invert off
  { PREF_T_INT, "YaIPS_BigImageColModDarken",    "0",  &YaIPS_BigImageDisp.DisplayColMod.Darken},      // Default: darken off
  { PREF_T_INT, "YaIPS_BigImagePlot3D_Active",   "0",  &YaIPS_BigImageDisp.Plot3D_Active},             // Default: false plot 3d active
  { PREF_T_INT, "YaIPS_BigImagePlot3D_DrawGrid", "0",  &YaIPS_BigImageDisp.Plot3D_DrawGrid},           // Default: false no grid lines
  { PREF_T_INT, "YaIPS_BigImagePlot3D_Inverted", "0",  &YaIPS_BigImageDisp.Plot3D_Inverted},           // Default: false gray value inversion
  { PREF_T_INT, "YaIPS_BigImagePlot3D_Azim",     "0",  &YaIPS_BigImageDisp.Plot3D_Azimuth},            // Default:
  { PREF_T_INT, "YaIPS_BigImagePlot3D_Elev",    "45",  &YaIPS_BigImageDisp.Plot3D_Elevation},          // Default:
  { PREF_T_INT, "YaIPS_BigImageOverlayStyle" ,   "0",  &OverlayStyle},                                // Default: Off
  { PREF_T_INT, "YaIPS_BigImageOverlayColor" ,  "95",  &OverlayColor},                                // Default: Yellow
  { PREF_T_INT, "YaIPS_BigImageShowInfoMode",    "0",  &YaIPS_BigImageDisp.ShowInfoMode},              // Default: Off
  { PREF_T_INT, "YaIPS_BigImageAoiP1x",         "10",  &YaIPS_BigImageDisp.AoiP1x},                    // Default:
  { PREF_T_INT, "YaIPS_BigImageAoiP1y",         "10",  &YaIPS_BigImageDisp.AoiP1y},                    // Default:
  { PREF_T_INT, "YaIPS_BigImageAoiP2x",        "110",  &YaIPS_BigImageDisp.AoiP2x},                    // Default:
  { PREF_T_INT, "YaIPS_BigImageAoiP2y",        "110",  &YaIPS_BigImageDisp.AoiP2y},                    // Default:
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( "GUI_Main", MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry), // @suppress("Ambiguous problem")
                                               (void **)(&pGUI_Main), &YaIPS_Main_WinPosX, &YaIPS_Main_WinPosY);

/************************************************************************************
 * IqeB_MainWindow_GUI_Setup
 *
 * Find the menu item for the given callback and user data pointer.
 *
 * This method finds a menu item in a menu array, also traversing submenus, but
 * not submenu pointers. This is useful if an application uses
 * internationalisation and a menu item can not be found using its label. This
 * search is also much faster.
 *
 * paramer cb: Find the first item with this callback
 *         ud: and this user data pointer.
 * returns:   The item found, or NULL if not found
 */
const Fl_Menu_Item * My_find_item_user_data( Fl_Menu_Bar *menubar, Fl_Callback *cb, void *ud) {
  for ( int t=0; t < menubar->size(); t++ ) {
    const Fl_Menu_Item *m = menubar->menu() + t;
    if (m->callback_==cb && m->user_data_==ud) {
      return m;
    }
  }
  return (const Fl_Menu_Item *)0;
}

/************************************************************************************
 * IqeB_MainWindow_GUI_Setup
 *
 * Main Window GUI
 */

static void IqeB_MainWindow_GUI_Setup( Fl_Double_Window *pWin)
{
  int x, x1, y, yy, xx, xx2, yBox, hWin, wWin;
  int BigImage_bx, BigImage_by, BigImage_bw, BigImage_bh;
  Fl_Box *pBoxTemp;
  Fl_Group *pGroupTemp;
  Fl_Check_Button *pCBoxTemp;
  Fl_Radio_Round_Button *pRadioButTemp;
  Fl_Choice *pChoiceTemp;
  //x/Fl_Slider *pSliderTemp;
  Fl_Value_Slider *pSliderTemp;
  Fl_Button *pTempButton;

  //

  hWin = pWin->h();
  wWin = pWin->w();

  //
  // main menu
  //

  main_menubar = new Fl_Menu_Bar( 4, 4, GUI_LEFT_SIDE_WIDTH, GUI_MENUHEIGHT);
  Main_Menu_AddItems( main_menubar);
  main_menubar->selection_color( Fl::get_color( FL_SELECTION_COLOR));
  main_menubar->global();   // Make the key shortcuts global known

  //
  // Group, left side OpenGL window
  //

  x = 4;

  xx = GUI_LEFT_SIDE_WIDTH;

  pGUI_GroupLeftSide = new Fl_Group( x, GUI_MENUHEIGHT, xx + 2, hWin);

  //
  // file browser
  //

#ifdef use_again
  yy = (hWin - GUI_MENUHEIGHT) / 2;
#else
  yy = 366;
#endif

  //
  // Display render options
  //

  y = GUI_MENUHEIGHT + 27;

  yy = 16;

  // Reset button

  xx2 = 54;

  pTempButton = new Fl_Button( x + xx - xx2, y - yy - 4, xx2, yy + 2, LANGDEF_BUTTON_RESET);
  pTempButton->tooltip( LangStringLookup( "&GUI_Main_Reseth=Reset display settings."));
  pTempButton->callback( IqeB_Main_Reset_Display, NULL);

  //
  // Image enlargement
  //

  yBox = y;

  pBoxTemp = new Fl_Box( x, y, xx, yy + 12, LangStringLookup( "&GUI_Main_Size1=Magnification"));
  pBoxTemp->box( FL_DOWN_FRAME);
  pBoxTemp->align(FL_ALIGN_TOP_LEFT);     // align for label
  pBoxTemp->vertical_label_margin( 2);    // gap distance

  pGroupTemp = new Fl_Group( x, y, xx, yy + 12);  // Group around this radio buttons

  xx2 = (xx - 12) / 4 - 4;
  x1 = x + 4;
  y += 6;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Main_Size2=Auto"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_Size2h=Automatically adjust magnification"));
  pRadioButTemp->callback( YaIPS_BigImageReso_Callback, (void *)YAIPS_DISP_RESOLUTION_AUTO);
  ResolutionButtons[ YAIPS_DISP_RESOLUTION_AUTO] = pRadioButTemp;

  x1 += xx2;

  xx2 = xx / 7 + 1;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Main_Size3=1"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_Size3h=Magnification 1:1"));
  pRadioButTemp->callback( YaIPS_BigImageReso_Callback, (void *)YAIPS_DISP_RESOLUTION_1_1);
  ResolutionButtons[ YAIPS_DISP_RESOLUTION_1_1] = pRadioButTemp;

  x1 += xx2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Main_Size4=2"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_Size4h=Image 2 * enlarged"));
  pRadioButTemp->callback( YaIPS_BigImageReso_Callback, (void *)YAIPS_DISP_RESOLUTION_X_2);
  ResolutionButtons[ YAIPS_DISP_RESOLUTION_X_2] = pRadioButTemp;

  x1 += xx2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Main_Size5=4"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_Size5h=Image 4 * enlarged"));
  pRadioButTemp->callback( YaIPS_BigImageReso_Callback, (void *)YAIPS_DISP_RESOLUTION_X_4);
  ResolutionButtons[ YAIPS_DISP_RESOLUTION_X_4] = pRadioButTemp;

  x1 += xx2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Main_Size6=8"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_Size6h=Image 8 * enlarged"));
  pRadioButTemp->callback( YaIPS_BigImageReso_Callback, (void *)YAIPS_DISP_RESOLUTION_X_8);
  ResolutionButtons[ YAIPS_DISP_RESOLUTION_X_8] = pRadioButTemp;

  x1 += xx2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 + 4, yy, LangStringLookup( "&GUI_Main_Size7=16"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_Size7h=Image 16 * enlarged"));
  pRadioButTemp->callback( YaIPS_BigImageReso_Callback, (void *)YAIPS_DISP_RESOLUTION_X_16);
  ResolutionButtons[ YAIPS_DISP_RESOLUTION_X_16] = pRadioButTemp;

  pGroupTemp->end();

  y = yBox + pBoxTemp->h();

  //
  // Overlay on the screen
  //

  y += 24;
  yBox = y;

  pBoxTemp = new Fl_Box( x, y, xx, yy + 12, LangStringLookup( "&GUI_Main_Over1=Overlay"));
  pBoxTemp->box( FL_DOWN_FRAME);
  pBoxTemp->align(FL_ALIGN_TOP_LEFT);     // align for label
  pBoxTemp->vertical_label_margin( 2);    // gap distance

  pGroupTemp = new Fl_Group( x, y, xx, yy + 12);  // Group around this radio buttons

  x1 = x + 4;
  y += 6;

  // ...

  xx2 = 48;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Main_Over2=Off"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_Over2h=No overlay"));
  pRadioButTemp->callback( YaIPS_BigImageOverlay_Callback, (void *)YAIPS_OVERLAY_OFF);
  OverlayButtons[ YAIPS_OVERLAY_OFF] = pRadioButTemp;

  x1 += xx2;

  xx2 = 100;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Main_Over3=Crosshair"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_Over3h=Overlay a crosshair on the image"));
  pRadioButTemp->callback( YaIPS_BigImageOverlay_Callback, (void *)YAIPS_OVERLAY_CROSSHAIR);
  OverlayButtons[ YAIPS_OVERLAY_CROSSHAIR] = pRadioButTemp;

  x1 += xx2;

  xx2 = 44;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Main_Over4=# 1"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_Over4h=Overlay 3 rectangles on the image"));
  pRadioButTemp->callback( YaIPS_BigImageOverlay_Callback, (void *)YAIPS_OVERLAY_GRID_1);
  OverlayButtons[ YAIPS_OVERLAY_GRID_1] = pRadioButTemp;

  x1 += xx2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Main_Over5=# 2"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_Over5h="
                                            "Overlay the image with a grid\n"
                                            "with a spacing of 50 pixels"));
  pRadioButTemp->callback( YaIPS_BigImageOverlay_Callback, (void *)YAIPS_OVERLAY_GRID_2);
  OverlayButtons[ YAIPS_OVERLAY_GRID_2] = pRadioButTemp;

  pGroupTemp->end();

  x1 += xx2;

  xx2 = 32;

  pTempButton = new Fl_Button( x1, y - 2, xx2 - 2, yy + 4, "");
  pTempButton->tooltip( LangStringLookup( "&GUI_Main_Over6h=Changes the color of the overlay"));
  pTempButton->color( OverlayColor);
  pTempButton->callback( IqeB_GUI_But_Color_Callback, &OverlayColor);
  pBut_OverlayColor = pTempButton;

  y = yBox + pBoxTemp->h();

  //
  // Color channel selection
  //

  y += 24;
  yBox = y;

  pBoxTemp = new Fl_Box( x, y, xx, yy + 12, LANGDEF_COL_CHANNEL);
  pBoxTemp->box( FL_DOWN_FRAME);
  pBoxTemp->align(FL_ALIGN_TOP_LEFT);     // align for label
  pBoxTemp->vertical_label_margin( 2);    // gap distance

  pGroupTemp = new Fl_Group( x, y, xx, yy + 12);  // Group around this radio buttons

  x1 = x + 4;
  y += 6;

  // ...

  xx2 = 70;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LANGDEF_COLOR);
  pRadioButTemp->tooltip( LANGDEF_IMAGE_NOT_CHANGED);
  pRadioButTemp->callback( YaIPS_BigImageColMod_Callback, (void *)YAIPS_DISP_COLMOD_NORMAL);
  ColModButtons[ YAIPS_DISP_COLMOD_NORMAL] = pRadioButTemp;

  x1 += xx2;

  xx2 = 46;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LANGDEF_COLOR_BW);
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_ColChan3h=Black and white"));
  pRadioButTemp->callback( YaIPS_BigImageColMod_Callback, (void *)YAIPS_DISP_COLMOD_BW);
  ColModButtons[ YAIPS_DISP_COLMOD_BW] = pRadioButTemp;

  x1 += xx2;

  xx2 = 36;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LANGDEF_COLOR_R);
  pRadioButTemp->tooltip( LANGDEF_COL_CHANNEL_R);
  pRadioButTemp->callback( YaIPS_BigImageColMod_Callback, (void *)YAIPS_DISP_COLMOD_R);
  ColModButtons[ YAIPS_DISP_COLMOD_R] = pRadioButTemp;

  x1 += xx2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LANGDEF_COLOR_G);
  pRadioButTemp->tooltip( LANGDEF_COL_CHANNEL_G);
  pRadioButTemp->callback( YaIPS_BigImageColMod_Callback, (void *)YAIPS_DISP_COLMOD_G);
  ColModButtons[ YAIPS_DISP_COLMOD_G] = pRadioButTemp;

  x1 += xx2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LANGDEF_COLOR_B);
  pRadioButTemp->tooltip( LANGDEF_COL_CHANNEL_B);
  pRadioButTemp->callback( YaIPS_BigImageColMod_Callback, (void *)YAIPS_DISP_COLMOD_B);
  ColModButtons[ YAIPS_DISP_COLMOD_B] = pRadioButTemp;

  x1 += xx2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LANGDEF_COLOR_A);
  pRadioButTemp->tooltip( LANGDEF_COL_CHANNEL_A);
  pRadioButTemp->callback( YaIPS_BigImageColMod_Callback, (void *)YAIPS_DISP_COLMOD_A);
  ColModButtons[ YAIPS_DISP_COLMOD_A] = pRadioButTemp;

  pGroupTemp->end();

  y = yBox + pBoxTemp->h();

  //
  // Color palette selection
  //

  y += 24;
  yBox = y;

  pBoxTemp = new Fl_Box( x, y, xx, yy + 18, LangStringLookup( "&GUI_Main_ColPal1=Color palette"));
  pBoxTemp->box( FL_DOWN_FRAME);
  pBoxTemp->align(FL_ALIGN_TOP_LEFT);     // align for label
  pBoxTemp->vertical_label_margin( 2);    // gap distance

  x1 = x + 4;
  y += 6;

  // ...

  xx2 = xx / 2;

  pChoiceTemp = new Fl_Choice( x1, y, xx2 - 10, yy + 6);
  pChoiceTemp->tooltip( LANGDEF_LUT_TOOLTIP);

  // Add predefined LUT tables
  for( int i = 0; i < YaIPS_ColMod_nLUT_tables; i++) {

    pChoiceTemp->add( LangStringLookup( YaIPS_ColMod_LUT_table[ i]));
  }

  pChoiceTemp->value( YaIPS_BigImageDisp.DisplayColMod.FalseColor);
  pChoiceTemp->callback( YaIPS_BImg_SetValue_Callback, &YaIPS_BigImageDisp.DisplayColMod.FalseColor);
  pChoice_Col_Palette = pChoiceTemp;

  x1 += xx2 - 4;

  xx2 = 92;

  pCBoxTemp = new Fl_Check_Button( x1, y, xx2 - 16, yy, LANGDEF_INVERTED);
  pCBoxTemp->tooltip( LangStringLookup( "&GUI_Main_ColPal3h=Invert color, black and white, or color palette image"));
  pCBoxTemp->value( YaIPS_BigImageDisp.DisplayColMod.Invert);
  pCBoxTemp->callback( YaIPS_BImg_SetValue_Callback, &YaIPS_BigImageDisp.DisplayColMod.Invert);
  pCBox_Col_Inverted = pCBoxTemp;

  x1 += xx2 - 14;

  xx2 = 58;

  pCBoxTemp = new Fl_Check_Button( x1, y, xx2 - 16, yy, LangStringLookup( "&GUI_Main_ColPal4=1/2"));
  pCBoxTemp->tooltip( LangStringLookup( "&GUI_Main_ColPal4h="
                                        "Reduces the brightness of the image by half.\n"
                                        "Overlaid graphics are then more visible."));
  pCBoxTemp->value( YaIPS_BigImageDisp.DisplayColMod.Darken);
  pCBoxTemp->callback( YaIPS_BImg_SetValue_Callback, &YaIPS_BigImageDisp.DisplayColMod.Darken);
  pCBox_Col_Darken = pCBoxTemp;

  y = yBox + pBoxTemp->h();

  //
  // 3D-Visualisierung
  //

  y += 24;
  yBox = y;

  pBoxTemp = new Fl_Box( x, y, xx, yy * 4 + 4, LangStringLookup( "&GUI_Main_3D1=3D visualization"));
  pBoxTemp->box( FL_DOWN_FRAME);
  pBoxTemp->align(FL_ALIGN_TOP_LEFT);     // align for label
  pBoxTemp->vertical_label_margin( 2);    // gap distance

  x1 = x + 4;

  y += 6;

  xx2 = 58;

  pCBoxTemp = new Fl_Check_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_Main_3D2=Active"));
  pCBoxTemp->tooltip( LangStringLookup( "&GUI_Main_3D2h=Display brightness as a height map"));
  pCBoxTemp->value( YaIPS_BigImageDisp.Plot3D_Active);
  pCBoxTemp->callback( YaIPS_BImg_SetValue_Callback, &YaIPS_BigImageDisp.Plot3D_Active);
  pCBox_3D_Active = pCBoxTemp;

  x1 += xx2 + 2;

  xx2 = 59;

  pCBoxTemp = new Fl_Check_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_Main_3D3=Lines"));
  pCBoxTemp->tooltip( LangStringLookup( "&GUI_Main_3D3h=Draw grid lines"));
  pCBoxTemp->value( YaIPS_BigImageDisp.Plot3D_DrawGrid);
  pCBoxTemp->callback( YaIPS_BImg_SetValue_Callback, &YaIPS_BigImageDisp.Plot3D_DrawGrid);
  pCBox_3D_DrawGrid = pCBoxTemp;

  x1 += xx2 + 2;

  xx2 = 46;

  pCBoxTemp = new Fl_Check_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_Main_3D4=Inv."));
  pCBoxTemp->tooltip( LangStringLookup( "&GUI_Main_3D4h=Inverted gray/height values."));
  pCBoxTemp->value( YaIPS_BigImageDisp.Plot3D_Inverted);
  pCBoxTemp->callback( YaIPS_BImg_SetValue_Callback, &YaIPS_BigImageDisp.Plot3D_Inverted);
  pCBox_3D_Inverted = pCBoxTemp;

  y += yy - 5;

  x1 = x + 4;

  xx2 = xx / 3 + 4;

  pSliderTemp = new Fl_Value_Slider( x1 + xx - xx2 - 8, y, xx2, yy + 4, LangStringLookup( "&GUI_Main_3D5=Slope"));
  pSliderTemp->labelsize( 10);
  pSliderTemp->align( FL_ALIGN_TOP);     // align for label
  pSliderTemp->tooltip( LangStringLookup( "&GUI_Main_3D5h=Slope of the height map"));
  pSliderTemp->type( FL_HOR_SLIDER);
  pSliderTemp->color( FL_LIGHT2 + 1);  // Background color
  pSliderTemp->bounds( YAIPS_3DPLOT_ELEVATION_MIN, YAIPS_3DPLOT_ELEVATION_MAX);
  pSliderTemp->step( 1);
  pSliderTemp->value( YaIPS_BigImageDisp.Plot3D_Elevation);
  pSliderTemp->callback( YaIPS_BImg_SetValue_Callback, &YaIPS_BigImageDisp.Plot3D_Elevation);
  pSlider_3D_Elevation = pSliderTemp;

  y += yy + 10;

  xx2 = xx - 8;

  pSliderTemp = new Fl_Value_Slider( x1, y, xx - 8, yy + 4, LANGDEF_ROTATION);
  pSliderTemp->labelsize( 10);
  pSliderTemp->align( FL_ALIGN_TOP);     // align for label
  pSliderTemp->tooltip( LangStringLookup( "&GUI_Main_3D6h=Viewing direction toward the height map"));
  pSliderTemp->type( FL_HOR_SLIDER);
  pSliderTemp->color( FL_LIGHT2 + 1);  // Background color
  pSliderTemp->bounds( YAIPS_3DPLOT_AZIMUT_MIN, YAIPS_3DPLOT_AZIMUT_MAX);
  pSliderTemp->step( 1);
  pSliderTemp->value( YaIPS_BigImageDisp.Plot3D_Azimuth);
  pSliderTemp->callback( YaIPS_BImg_SetValue_Callback, &YaIPS_BigImageDisp.Plot3D_Azimuth);
  pSlider_3D_Azimuth = pSliderTemp;

  // ...

  y = yBox + pBoxTemp->h();

  //
  // Show measurement/info modes
  //

  y += 24;
  yBox = y;

  pBoxTemp = new Fl_Box( x, y, xx, yy * 3, LangStringLookup( "&GUI_Main_InfMeas1=Info/Measurement"));
  pBoxTemp->box( FL_DOWN_FRAME);
  pBoxTemp->align(FL_ALIGN_TOP_LEFT);     // align for label
  pBoxTemp->vertical_label_margin( 2);    // gap distance

  pGroupTemp = new Fl_Group( x, y, xx, yy * 3);  // Group around this radio buttons

  x1 = x + 4;
  y += 6;

  // ...

  xx2 = xx / 5 + 1;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 0, yy, "@-1pencil");
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_InfMeas2h="
                          "Support for teach mode.\n"
                          "In some tool windows, AOIs can be moved/changed with the\n"
                          "mouse in teach mode.\n"
                          "These windows have a button with a pencil icon.\n"
                          "If such a tool window is selected for display in the big window,\n"
                          "these changes can also be made with the mouse in\n"
                          "the big window.\n"
                          "This function is available when this radio button is selected\n"
                          "and the pencil icon is green."));
  pRadioButTemp->callback( YaIPS_BigImageSowInfo_Callback, (void *)YAIPS_SHOW_INFO_OFF);
  ShowButtons[ YAIPS_SHOW_INFO_OFF] = pRadioButTemp;

  x1 += xx2 - 2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 0, yy, LANGDEF_LUT_SHORT);
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_InfMeas3h=Visualization of the color palette\nLUT = Lookup table"));
  pRadioButTemp->callback( YaIPS_BigImageSowInfo_Callback, (void *)YAIPS_SHOW_INFO_CU_LUT);
  ShowButtons[ YAIPS_SHOW_INFO_CU_LUT] = pRadioButTemp;

  x1 += xx2 - 2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 0, yy, LANGDEF_PIXEL_UCF);
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_InfMeas4h=Display pixel value under mouse pointer"));
  pRadioButTemp->callback( YaIPS_BigImageSowInfo_Callback, (void *)YAIPS_SHOW_INFO_CU_VAL);
  ShowButtons[ YAIPS_SHOW_INFO_CU_VAL] = pRadioButTemp;

  x1 += xx2 - 2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 0, yy, LangStringLookup( "&GUI_Main_InfMeas5a=H All"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_InfMeas5ah=Histogram of pixel values in the image"));
  pRadioButTemp->callback( YaIPS_BigImageSowInfo_Callback, (void *)YAIPS_SHOW_INFO_RE_HISTO_ALL);
  ShowButtons[ YAIPS_SHOW_INFO_RE_HISTO_ALL] = pRadioButTemp;

  x1 += xx2 - 2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 0, yy, LangStringLookup( "&GUI_Main_InfMeas5b=H AOI"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_InfMeas5bh=Histogram of pixel values in a rectangle/AOI"));
  pRadioButTemp->callback( YaIPS_BigImageSowInfo_Callback, (void *)YAIPS_SHOW_INFO_RE_HISTO_AOI);
  ShowButtons[ YAIPS_SHOW_INFO_RE_HISTO_AOI] = pRadioButTemp;

  x1 = x + 4;
  y += yy + 4;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 0, yy, LangStringLookup( "&GUI_Main_InfMeas6=Su |"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_InfMeas6h=Profile of column sum in a rectangle"));
  pRadioButTemp->callback( YaIPS_BigImageSowInfo_Callback, (void *)YAIPS_SHOW_INFO_RE_COLSUM);
  ShowButtons[ YAIPS_SHOW_INFO_RE_COLSUM] = pRadioButTemp;

  x1 += xx2 - 2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 0, yy, LangStringLookup( "&GUI_Main_InfMeas7=Su -"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_InfMeas7h=Profile of row sum in a rectangle"));
  pRadioButTemp->callback( YaIPS_BigImageSowInfo_Callback, (void *)YAIPS_SHOW_INFO_RE_ROWSUM);
  ShowButtons[ YAIPS_SHOW_INFO_RE_ROWSUM] = pRadioButTemp;

  x1 += xx2 - 2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 0, yy, LangStringLookup( "&GUI_Main_InfMeas8=Di |"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_InfMeas8h=Measures vertical distance"));
  pRadioButTemp->callback( YaIPS_BigImageSowInfo_Callback, (void *)YAIPS_SHOW_INFO_2P_DIST_VER);
  ShowButtons[ YAIPS_SHOW_INFO_2P_DIST_VER] = pRadioButTemp;

  x1 += xx2 - 2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 0, yy, LangStringLookup( "&GUI_Main_InfMeas9=Di -"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Main_InfMeas9h=Measures horizontal distance"));
  pRadioButTemp->callback( YaIPS_BigImageSowInfo_Callback, (void *)YAIPS_SHOW_INFO_2P_DIST_HOR);
  ShowButtons[ YAIPS_SHOW_INFO_2P_DIST_HOR] = pRadioButTemp;

  pGroupTemp->end();

  y = yBox + pBoxTemp->h();


  //
  // Histogram drawing area
  //

  y += 24;

  yy = YAIPS_HISTO_N_POINTS / 2 + 88;
  if( YAIPS_HISTO_N_POINTS + 27 > yy) {

    yy = YAIPS_HISTO_N_POINTS + 27;
  }

  pGUI_Box_DiffUses = new YaIPS_Fl_Box( x, y, xx, yy, LangStringLookup( "&GUI_Main_InfBox1=Info"));
  pGUI_Box_DiffUses->box( FL_DOWN_BOX);
  pGUI_Box_DiffUses->align(FL_ALIGN_TOP_LEFT);     // align for label
  pGUI_Box_DiffUses->color( FL_BLACK);             // Background color
  pGUI_Box_DiffUses->vertical_label_margin( 2);    // gap distance

  //x/pGUI_Box_DiffUses->pDrawBeforeCallback = YaIPS_ImageDispDrawBefore_cb; // Draw before callback
  pGUI_Box_DiffUses->pDrawAfterCallback  = YaIPS_GUI_DiffUsesDrawAfter_cb;   // Draw after callback
  //x/pGUI_Box_DiffUses->DrawCallbackArg     = &YaIPS_BigImageDisp;          // Argument for draw before/draw after callbacks


  //
  // ...
  //

  y = pBoxTemp->y() + pBoxTemp->h();

  y += 20;

  yy = 16;

  //
  // ...
  //

  pGUI_GroupLeftSide->end();             // end this group
  pGUI_GroupLeftSide->resizable( 0);     // nothing in this group is resizable

  //
  // Big image display and right side box
  //

  // Size left for big image display and right side box

  x = pGUI_GroupLeftSide->x() + pGUI_GroupLeftSide->w();

  BigImage_bx = x + 1;
  BigImage_by = 2;
  BigImage_bw = wWin - x - 4;
  BigImage_bh = hWin - 5;

  // Check size of right side

  if( BigImage_bw - RightSideWidth < WITH_MIN_IMAGE_BOX) {

    RightSideWidth = BigImage_bw - WITH_MIN_IMAGE_BOX;
  }

  if( RightSideWidth < WITH_MIN_TOOL_BOX) {

    RightSideWidth = WITH_MIN_TOOL_BOX;
  }

  // Create tile to group big image display and right side box together

  pTile_RightSide = new Fl_Tile( BigImage_bx, BigImage_by, BigImage_bw, BigImage_bh);

  // Big image display is created now

  YaIPS_BigImageDisp.DisplayResolution_Last = YAIPS_DISP_RESOLUTION_INVALID;  // Setup resolution change check

  YaIPS_BigImageDisp.pImage_Box = new YaIPS_Fl_Box( BigImage_bx, BigImage_by, BigImage_bw - RightSideWidth, BigImage_bh);
  YaIPS_BigImageDisp.pImage_Box->box( FL_DOWN_BOX);
  YaIPS_BigImageDisp.pImage_Box->labelsize(100);
  YaIPS_BigImageDisp.pImage_Box->align( FL_ALIGN_CLIP);
  YaIPS_BigImageDisp.pImage_Box->color( YaIPS_Color_IMG_BGND);  // Background color

  YaIPS_BigImageDisp.pImage_Box->pDrawBeforeCallback  = YaIPS_ImageDispDrawBefore_cb; // Draw before callback
  YaIPS_BigImageDisp.pImage_Box->pDrawAfterCallback   = YaIPS_GUI_BigDrawAfter_cb;    // Draw after callback
  YaIPS_BigImageDisp.pImage_Box->DrawCallbackArg1     = &YaIPS_BigImageDisp;          // Pointer to Fl_YaIPS_ImageDisp_t

  YaIPS_BigImageDisp.pImage_Box->pDropCallback        = DropFile_cb;                  // Accept file drops
  YaIPS_BigImageDisp.pImage_Box->pPasteImgCallback    = YaIPS_ImageDispPasteToOut_cb; // Common paste image callback
  YaIPS_BigImageDisp.pImage_Box->PasteImgCallbackArg1 = &YaIPS_BigImageDisp;          // Pointer to Fl_YaIPS_ImageDisp_t

  YaIPS_BigImageDisp.pImage_Box->pMouseCallback       = YaIPS_ImageDispMouse_cb;      // Common mouse event callback
  YaIPS_BigImageDisp.pImage_Box->MouseCallbackArg1    = &YaIPS_BigImageDisp;          // Pointer to Fl_YaIPS_ImageDisp_t

  pTile_RightSide->size_range( YaIPS_BigImageDisp.pImage_Box, WITH_MIN_IMAGE_BOX, 256);

  // Area to place tool windows

  pGUI_Main_RightSide = new Fl_Box( BigImage_bx + BigImage_bw - RightSideWidth, BigImage_by, RightSideWidth, BigImage_bh);
  pGUI_Main_RightSide->box( FL_DOWN_BOX);
  pGUI_Main_RightSide->color( FL_DARK_CYAN);  // Background color

  pTile_RightSide->size_range( pGUI_Main_RightSide, WITH_MIN_TOOL_BOX, 10);
  pTile_RightSide->resizable( YaIPS_BigImageDisp.pImage_Box);

  pTile_RightSide->end();

  //
  // Layout end work
  //

  pWin->end();

  pWin->resizable( pTile_RightSide);  // This window is resizable

  pWin->size_range( YAIPS_MAIN_SIZE_X_MIN, YAIPS_MAIN_SIZE_Y_MIN, YAIPS_MAIN_SIZE_X_MAX, YAIPS_MAIN_SIZE_Y_MAX); // minimum window size
}

/************************************************************************************
 * main
 */

int main(int argc, char **argv)
{
  int TextLen, RetVal;
  char *pStartupFile;
  int ierr, FullScreenWasActive;
  char TempFileName[ MAX_FILENAME_LEN]; // Path of current directory for video file browsers

  FOPEN();    // Log-file open

  FPRINTF0( "YaIPS main: TP 1\n");    // Log-file support

#ifdef WIN32

  //
  // Test for shared memory file.
  // If there is already a IqeBrowser running, there
  // exists a shared memory file. In this case copy
  // a file name argument to this shared memory.
  //

  {
    HANDLE hMapFile;
    char *pBuf;

    hMapFile = OpenFileMapping(
    FILE_MAP_ALL_ACCESS,       // read/write access
        FALSE,                 // do not inherit the name
        WIN_ENGLISH_TITLE);    // name of mapping object

    FPRINTF1( "YaIPS main: TP 1a Mapfile %s\n", hMapFile != NULL ? "OK" : "Error");    // Log-file support

    if( hMapFile != NULL) {   // There is an other application running

      FPRINTF1( "YaIPS main: TP 1b argc %d\n", argc);    // Log-file support

      if (argc > 1) {         // Is there a file argument

        pBuf = (LPTSTR) MapViewOfFile(hMapFile, // handle to map object
            FILE_MAP_ALL_ACCESS,  // read/write permission
            0, 0, MultipleRunning_SizeData);

        if (pBuf != NULL) {   // Got the mapping

          memset(pBuf, 0, MultipleRunning_SizeData);            // Reset memory
          strncpy(pBuf, argv[1], MultipleRunning_SizeData - 1); // Copy file argument

          UnmapViewOfFile(pBuf);
        }
      }

      CloseHandle(hMapFile);

      return (0);
    }
  }

  //
  // Create a memory mapped memory to get
  //

  MultipleRunning_pData = NULL;            // preset no mapping

  MultipleRunning_hMapFile = CreateFileMapping(
  INVALID_HANDLE_VALUE,     // use paging file
      NULL,                     // default security
      PAGE_READWRITE,           // read/write access
      0,                        // maximum object size (high-order DWORD)
      MultipleRunning_SizeData, // maximum object size (low-order DWORD)
      WIN_ENGLISH_TITLE);       // name of mapping object

  if (MultipleRunning_hMapFile != NULL) {  // Got it

    MultipleRunning_pData = (LPTSTR) MapViewOfFile(MultipleRunning_hMapFile, // handle to map object
        FILE_MAP_ALL_ACCESS, // read/write permission
        0, 0, MultipleRunning_SizeData);

    if (MultipleRunning_pData == NULL) {    // Got a problem

      CloseHandle(MultipleRunning_hMapFile);
      MultipleRunning_hMapFile = NULL;
    } else {

      memset(MultipleRunning_pData, 0, MultipleRunning_SizeData);
    }
  }
#endif

  FPRINTF0( "YaIPS main: TP 2\n");    // Log-file support

  // Use multimedia timer with 1 ms resolution.
  // Is needed for use of timeGetTime().

  timeBeginPeriod( 1);

  // Prepare FLTK things

  Fl::visual(FL_RGB8);

  Fl_Image::RGB_scaling(FL_RGB_SCALING_BILINEAR);           // set bilinear image scaling method

  FPRINTF0( "YaIPS main: TP 3\n");    // Log-file support

  fl_register_images();       // required preview of known image formats

  Fl::get_system_colors();
  //x/Fl::foreground(  185, 236, 255);   // adapt color scheme

  Fl::background( YAIPS_COLOR_MAIN_BGND);
  //x//Fl::background2( 255, 255, 200);
  Fl::set_color( FL_SELECTION_COLOR, YAIPS_COLOR_SELECTION);
  //x///Fl::set_color( FL_INACTIVE_COLOR, 0, 64, 0);

  // Pick background color, is used by OpenGL view.
  GUI_BackgroundColor = Fl::get_color(FL_BACKGROUND_COLOR);

  // load preference data from file

  FPRINTF0( "YaIPS main: TP 4\n");    // Log-file support

  IqeB_PreferencesGetFromFile();

  FPRINTF0( "YaIPS main: TP 5\n");    // Log-file support

  FullScreenWasActive = FullScreenActive;          // Latch FullScreenActive for later use. Is reset to 0 during startup of tool windows.

  // Add additional symbols (for buttons, ...)

  YaIPS_Utils_AddSympols();

  // Enumerate known fonts on this PC

  YaIPS_Utils_FontsEnum();

  FPRINTF0( "YaIPS main: TP 6\n");    // Log-file support

  // Working directory not set

  if ( YaIPS_WorkingDirectory[0] == '\0') {   // Current directory not set

    int StrLen;

    fl_getcwd(  YaIPS_WorkingDirectory, sizeof( YaIPS_WorkingDirectory) - 256);

    // Ensure path characters are normalized
    IqeB_FileNormalizePathChars( YaIPS_WorkingDirectory);

    // Ensure proper working directory if started by double clicking the .exe

    StrLen = strlen( YaIPS_WorkingDirectory);

    if( StrLen > 3 + 5 &&
        strnicmp( YaIPS_WorkingDirectory + StrLen - 5, "Debug", 5) == 0) {

      YaIPS_WorkingDirectory[ StrLen - 5 - 1] = '\0';
      fl_chdir( YaIPS_WorkingDirectory);

    } if( StrLen > 3 + 7 &&
        strnicmp( YaIPS_WorkingDirectory + StrLen - 7, "Release", 7) == 0) {

      YaIPS_WorkingDirectory[ StrLen - 7 - 1] = '\0';
      fl_chdir( YaIPS_WorkingDirectory);
    }
  }

  // Clean not used clipboard subdirectories

  FPRINTF1( "YaIPS main: TP 7, WDir %s\n", YaIPS_WorkingDirectory);    // Log-file support

  IqeB_PresetCleanClipboard();

  FPRINTF0( "YaIPS main: TP 8\n");    // Log-file support

  // Ensure clip board directory is created

  // Ensure path characters are normalized
  strcpy( TempFileName, YaIPS_CLIPBOARD_PATH);
  IqeB_FileNormalizePathChars( TempFileName);

  if( ! IqeB_DirExsits( TempFileName)) {   // If clip board directory does not exist

    FPRINTF1( "YaIPS main: TP 8a, Dir %s\n", TempFileName);    // Log-file support

    IqeB_FileMakePath( TempFileName);      // Create it

    FPRINTF0( "YaIPS main: TP 8b\n");    // Log-file support
  }

  FPRINTF0( "YaIPS main: TP 9\n");    // Log-file support

  // get current directory for the file browser

  if (YaIPS_BrowserDirectory[0] == '\0') {   // Current directory not set

    strcpy( YaIPS_BrowserDirectory, YaIPS_WorkingDirectory);
  }

  if (YaIPS_BrowserDirVideos[0] == '\0') {   // Current directory not set

    strcpy( YaIPS_BrowserDirVideos, YaIPS_WorkingDirectory);
  }

  // Load translations for last selected language
  Lang_Init();

  // Handle startup file

  pStartupFile = NULL;    // Preset no startup file

  if (argc > 1) {

    pStartupFile = argv[1];
  }

  if( pStartupFile != NULL) {

    char *p, SaveC;

    fl_filename_absolute(GUI_StartupFile, sizeof(GUI_StartupFile) - 256,
        pStartupFile);

    p = strrchr(GUI_StartupFile, '/');
    if (!p)
      p = strrchr(GUI_StartupFile, '\\');
    if (p) {     // point to last path slash

      SaveC = *p;              // save this char
      *p = '\0';               // set end of string

      strcpy(YaIPS_BrowserDirectory, GUI_StartupFile);

      *p = SaveC;              // restore
    }
  }

  // ensure we have a path character at the end of the string

  TextLen = strlen(YaIPS_BrowserDirectory);
  if (TextLen > 0 && YaIPS_BrowserDirectory[TextLen - 1] != '/'
      && YaIPS_BrowserDirectory[TextLen - 1] != '\\') {

    strcat(YaIPS_BrowserDirectory, "/");                   // add it
    IqeB_FileNormalizePathChars(YaIPS_BrowserDirectory);
  }

  TextLen = strlen(YaIPS_BrowserDirVideos);
  if (TextLen > 0 && YaIPS_BrowserDirVideos[TextLen - 1] != '/'
      && YaIPS_BrowserDirVideos[TextLen - 1] != '\\') {

    strcat(YaIPS_BrowserDirVideos, "/");                   // add it
    IqeB_FileNormalizePathChars(YaIPS_BrowserDirVideos);
  }

  // construct GUI

  // clip window sizes

  if (YaIPS_Main_WinSizeX < YAIPS_MAIN_SIZE_X_MIN)
    YaIPS_Main_WinSizeX = YAIPS_MAIN_SIZE_X_MIN;
  if (YaIPS_Main_WinSizeX > YAIPS_MAIN_SIZE_X_MAX)
    YaIPS_Main_WinSizeX = YAIPS_MAIN_SIZE_X_MAX;

  if (YaIPS_Main_WinSizeY < YAIPS_MAIN_SIZE_Y_MIN)
    YaIPS_Main_WinSizeY = YAIPS_MAIN_SIZE_Y_MIN;
  if (YaIPS_Main_WinSizeY > YAIPS_MAIN_SIZE_Y_MAX)
    YaIPS_Main_WinSizeY = YAIPS_MAIN_SIZE_Y_MAX;

  // create the window

  if (YaIPS_Main_WinPosX != IQE_GUI_NO_WINPOS_X && YaIPS_Main_WinPosY != IQE_GUI_NO_WINPOS_Y) { // have last window position

    pGUI_Main = new Fl_Double_Window(YaIPS_Main_WinPosX, YaIPS_Main_WinPosY, YaIPS_Main_WinSizeX, YaIPS_Main_WinSizeY, LangStringLookup( WIN_DEFAULT_TITLE));

  } else {

    pGUI_Main = new Fl_Double_Window( YaIPS_Main_WinSizeX, YaIPS_Main_WinSizeY, LangStringLookup( WIN_DEFAULT_TITLE));
  }

  if (pGUI_Main == NULL) {  // security test

    return (0);
  }

  IqeB_MainWindow_GUI_Setup(pGUI_Main);         // Main Window GUI setup

  pGUI_Main->callback( MainExitCallback);       // Press the close button --> close program

  // Set icon for window
#ifdef WIN32
  pGUI_Main->icon((char*) LoadIcon(fl_display, MAKEINTRESOURCE( 101)));
#endif

  pGUI_Main->show( argc, argv);

  if( FullScreenWasActive) {                    // Full screen was active on last exit

    pGUI_Main->fullscreen();

    // Reconstruct right side with

    if( RightSideWidth != RightSideWidthFS) {   // If sizes for full screen and std window are different

      int BigImage_bx, BigImage_by, BigImage_bw, BigImage_bh;

      BigImage_bx = pTile_RightSide->x();
      BigImage_by = pTile_RightSide->y();
      BigImage_bw = pTile_RightSide->w();
      BigImage_bh = pTile_RightSide->h();

      // Check size of right side

      if( BigImage_bw - RightSideWidthFS < WITH_MIN_IMAGE_BOX) {

        RightSideWidthFS = BigImage_bw - WITH_MIN_IMAGE_BOX;
      }

      if( RightSideWidthFS < WITH_MIN_TOOL_BOX) {

        RightSideWidthFS = WITH_MIN_TOOL_BOX;
      }

      YaIPS_BigImageDisp.pImage_Box->resize( BigImage_bx, BigImage_by, BigImage_bw - RightSideWidthFS, BigImage_bh);

      pGUI_Main_RightSide->resize( BigImage_bx + BigImage_bw - RightSideWidthFS, BigImage_by, RightSideWidthFS, BigImage_bh);

      pTile_RightSide->redraw();
    }
  }

  Fl::add_check( IqeB_GUI_ToolsMyIdleAction);   // Check big image size change
  Fl::add_idle( IqeB_GUI_OpenGLIdleAction);     // Redraw OpenGL window during idle

  // Reset some state variables

  DUseDataChange = -1;                      // Reset monitor source image change
  YaIPS_BigImageDisp.Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA; // Set AOI changed flag bit

  // Show loading image

  ierr = YaIPS_ImageDispUpdateByFileName(&YaIPS_BigImageDisp,
      (char*) "Images/YaIPS/Icon-YaIPS.png",
      YAIPS_WIN_ID_DROP_MAIN, false);

  if (ierr < 0) {   // Error loading image

    // Try other location
    ierr = YaIPS_ImageDispUpdateByFileName(&YaIPS_BigImageDisp,
        (char*) "../Images/YaIPS/Icon-YaIPS.png",
        YAIPS_WIN_ID_DROP_MAIN, false);
  }

  if (ierr >= 0) {       // Loading was OK

    pGUI_Main->show();
  }

  // Startup the windows from last session

  if( YaIPS_Setting_Startup_WinRestore) {

    YaIPS_BigImageDisp.ImageSourceID = YaIPS_Main_ImageSourceID_Last;
  }

  // Startup the windows from the last session

  Fl::check();                       // give fltk some cpu to update the screen
  pGUI_Main->show();                 // Ensure focus back to main window

  YaIPS_GUI_Main_Do_Startup = true;  // Startup phase of tool windows begin

  YaIPS_WindowsStartup( YaIPS_Setting_Startup_WinRestore,
                       pGUI_Main->x_root(), pGUI_Main->x_root() + pGUI_Main->decorated_w(),
                       pGUI_Main->y_root(), pGUI_Main->y_root() + pGUI_Main->decorated_h());

  pGUI_Main->show();                  // Ensure focus back to main window

  YaIPS_GUI_Main_Do_Startup = false;  // Startup phase of tool windows finished

  FPRINTF0( "YaIPS main: TP 99, before GUI loop\n");    // Log-file support

  FCLOSE(); // Log-file close

  // Run main window loop

  RetVal = Fl::run();

  IqeB_GUI_MainEndWorks();      // Do work on shutting down the program.

#ifdef use_again
 IQE_DEBUG_LOGFILE_CLOSE();     // close debug logfile
#endif

  return (RetVal);
}

/************************************************************************************
 * IqeB_GUI_MainMakeTopWindow
 *
 * Make the main window the top window
 *
 */

void IqeB_GUI_MainMakeTopWindow()
{

  if( pGUI_Main == NULL) {     // security test, no main window

    return;
  }

  //x/Fl::first_window( pGUI_Main);     // Select the main window as top window

  pGUI_Main->show();
}

/************************************************************************************
 * IqeB_GUI_SetWindowTitle
 */

void IqeB_GUI_SetWindowTitle( char *pTitle)
{

  pGUI_Main->label( pTitle);
}

/************************************************************************************
 * IqeB_GUI_WidgetActivate
 *
 * Change activation state of a widget
 *
 * return:   true   changed the activation of a widget
 *          false   no change
 *
 */

int IqeB_GUI_WidgetActivate( void *wArg, int ActivateIt)
{
  Fl_Widget *w;
  int RetVal;

  RetVal = false;

  w = (Fl_Widget *)wArg;

  if( w == NULL) {   // security test

    return( RetVal);
  }

  if( ActivateIt) {
    if( ! w->active()) {
      w->activate();
      RetVal = true;
    }
  } else {
    if( w->active()) {
      w->deactivate();
      RetVal = true;
    }
  }

  return( RetVal);
}

/************************************************************************************
 * IqeB_GUI_WidgetColor
 *
 * Change  color of widget
 *
 * return:   true   changed the color
 *          false   no change
 *
 */

int IqeB_GUI_WidgetColor( void *wArg, Fl_Color NewColor)
{
  Fl_Widget *w;
  int RetVal;

  RetVal = false;

  w = (Fl_Widget *)wArg;

  if( w == NULL) {   // security test

    return( RetVal);
  }

  if( w->color() != NewColor) {

    w->color( NewColor);

    w->redraw();
    RetVal = true;
  }

  return( RetVal);
}

/************************************************************************************
 * IqeB_GUI_WidgetLabelColor
 *
 * Change label color of widget
 *
 * return:   true   changed the color
 *          false   no change
 *
 */

int IqeB_GUI_WidgetLabelColor( void *wArg, Fl_Color NewColor)
{
  Fl_Widget *w;
  int RetVal;

  RetVal = false;

  w = (Fl_Widget *)wArg;

  if( w == NULL) {   // security test

    return( RetVal);
  }

  if( w->labelcolor() != NewColor) {

    w->labelcolor( NewColor);

    w->redraw();
    RetVal = true;
  }

  return( RetVal);
}

/************************************************************************************
 * IqeB_GUI_ItemActivate
 *
 *
 * return:   true   changed the activation of a widget
 *          false   no change
 *
 */

static int IqeB_GUI_ItemActivate( void *wArg, int ActivateIt)
{
  Fl_Menu_Item *w;
  int RetVal;

  RetVal = false;

  w = (Fl_Menu_Item *)wArg;

  if( w == NULL) {   // security test

    return( RetVal);
  }

  if( ActivateIt) {
    if( ! w->active()) {
      w->activate();
      RetVal = true;
    }
  } else {
    if( w->active()) {
      w->deactivate();
      RetVal = true;
    }
  }

  return( RetVal);
}

/************************************************************************************
 * Update GUI of this tool window
 *
 */

static void MyWinUpdate()
{
  int HaveAnyImage, Plot3D_Active;
#ifdef use_again  // Maybe used later
  int IsColImage, HaveAlpha;
#endif
  int i, AnyWidgetChanged;

  // ...

  AnyWidgetChanged = false;    // Catch change of any widget

  // Get states

  if( YaIPS_BigImageDisp.pImage_Img != NULL &&      // Have any image
      YaIPS_BigImageDisp.pImage_Img->d() > 0) {     // with minimum one byte per pixel

    HaveAnyImage = true;

#ifdef use_again  // Maybe used later
    IsColImage    = YaIPS_BigImageDisp.pImage_Img->d() >= 3;
    HaveAlpha     = YaIPS_BigImageDisp.pImage_Img->d() == 2 || YaIPS_BigImageDisp.pImage_Img->d() == 4;
#endif

    Plot3D_Active = YaIPS_BigImageDisp.Plot3D_Active;

  } else {

    HaveAnyImage  = false;
#ifdef use_again  // Maybe used later
    IsColImage    = false;
    HaveAlpha     = false;
#endif
    Plot3D_Active = false;
  }

  // To periodically updates first

  // Update menu items

  Fl_Menu_Item *pTempItem;

  // Enable 'Save image' if we have an image to save
  pTempItem = (Fl_Menu_Item *)main_menubar->find_item( IqeB_Main_Save_Callback);
  if( pTempItem != NULL) {

    AnyWidgetChanged |= IqeB_GUI_ItemActivate( pTempItem,
                           YaIPS_BigImageDisp.pImage_Img != NULL &&                                // Have an image loaded
                           (YaIPS_BigImageDisp.Flags & YAIPS_IDISP_FLAG_DO_DISP_MODIFY) != 0);      // and is not not the startup image
  }

  // Enable 'paste image' if we have an image in the clip board
  pTempItem = (Fl_Menu_Item *)main_menubar->find_item( MainMenuPasteCallback);
  if( pTempItem != NULL) {

    AnyWidgetChanged |= IqeB_GUI_ItemActivate( pTempItem,
                           Fl::clipboard_contains(Fl::clipboard_image) != 0 /*&&                      // Have an image in the clip board
                           (YaIPS_BigImageDisp.Flags & YAIPS_IDISP_FLAG_DO_DISP_MODIFY) != 0*/);      // and is not not the startup image
  }

  // Enable 'copy image' if we have an image to save
  pTempItem = (Fl_Menu_Item *)main_menubar->find_item( IqeB_Main_Copy_Callback);
  if( pTempItem != NULL) {

    AnyWidgetChanged |= IqeB_GUI_ItemActivate( pTempItem,
                           YaIPS_BigImageDisp.pImage_Img != NULL &&                                // Have an image loaded
                           (YaIPS_BigImageDisp.Flags & YAIPS_IDISP_FLAG_DO_DISP_MODIFY) != 0);      // and is not not the startup image
  }

  // Update image enlargement button
  // Current selection must be set

  if( ResolutionButtons[ YaIPS_BigImageDisp.DisplayResolution]->value() == 0) {   // This one is not selected

    AnyWidgetChanged = true;    // Catch change of any widget

    // Update all buttons
    for( i = 0; i < YAIPS_DISP_RESOLUTION_MAX + 1; i++) {

      ResolutionButtons[ i]->value( i == YaIPS_BigImageDisp.DisplayResolution);   // Set value

    }
  }

  // Update color modification button
  // Current selection must be set

  if( ColModButtons[ YaIPS_BigImageDisp.DisplayColMod.Style]->value() == 0) {   // This one is not selected

    AnyWidgetChanged = true;    // Catch change of any widget

    // Update all buttons
    for( i = 0; i < YAIPS_DISP_COLMOD_MAX + 1; i++) {

      ColModButtons[ i]->value( i == YaIPS_BigImageDisp.DisplayColMod.Style);   // Set value
    }
  }

  // Update image overlay button
  // Current selection must be set

  if( OverlayButtons[ OverlayStyle]->value() == 0) {   // This one is not selected

    AnyWidgetChanged = true;    // Catch change of any widget

    // Update all buttons
    for( i = 0; i < YAIPS_OVERLAY_MAX + 1; i++) {

      OverlayButtons[ i]->value( i == OverlayStyle);   // Set value
    }
  }

  // Update image overlay color

  if( OverlayColor != (int)pBut_OverlayColor->color()) {

    pBut_OverlayColor->color( OverlayColor);                // Update color
    pBut_OverlayColor->redraw();
  }

  // Update color selection things

  if( pChoice_Col_Palette->value() != YaIPS_BigImageDisp.DisplayColMod.FalseColor) {

    AnyWidgetChanged = true;    // Catch change of any widget

    pChoice_Col_Palette->value( YaIPS_BigImageDisp.DisplayColMod.FalseColor);
  }

  // Update show measurement/info modes button
  // Current selection must be set

  if( ShowButtons[ YaIPS_BigImageDisp.ShowInfoMode]->value() == 0 ||       // This one is not selected
      ShowButtons[ 0]->active() != !Plot3D_Active) {    // or active has to be changed

    AnyWidgetChanged = true;    // Catch change of any widget

    // Update all buttons
    for( i = 0; i < YAIPS_SHOW_INFO_MAX + 1; i++) {

      ShowButtons[ i]->value( i == YaIPS_BigImageDisp.ShowInfoMode);   // Set value
      AnyWidgetChanged |= IqeB_GUI_WidgetActivate( ShowButtons[ i], !Plot3D_Active);
    }
  }

  // Update teach symbol for first button

  int MouseTeachState;
  Fl_Color TeachStateColor;

  MouseTeachState = YaIPS_ToolWinMouseCallbackState( YaIPS_BigImageDisp.ImageSourceID);

  if( MouseTeachState == 3) {

    TeachStateColor = FL_GREEN;

  } else if( MouseTeachState == 2) {

    TeachStateColor = YAIPS_BCOL_BUTTON;

  } else {

    TeachStateColor = FL_BLACK;
  }

  IqeB_GUI_WidgetLabelColor( ShowButtons[ YAIPS_SHOW_INFO_OFF], TeachStateColor);

  // 3D things

  AnyWidgetChanged |= IqeB_GUI_WidgetActivate( pCBox_3D_Active, HaveAnyImage);
  AnyWidgetChanged |= IqeB_GUI_WidgetActivate( pCBox_3D_DrawGrid, Plot3D_Active);
  AnyWidgetChanged |= IqeB_GUI_WidgetActivate( pCBox_3D_Inverted, Plot3D_Active);
  AnyWidgetChanged |= IqeB_GUI_WidgetActivate( pSlider_3D_Azimuth, Plot3D_Active);
  AnyWidgetChanged |= IqeB_GUI_WidgetActivate( pSlider_3D_Elevation, Plot3D_Active);

  for( i = 0; i < YAIPS_OVERLAY_MAX + 1; i++) {  // Update all buttons

    AnyWidgetChanged |= IqeB_GUI_WidgetActivate( OverlayButtons[ i], ! Plot3D_Active);
  }
  AnyWidgetChanged |= IqeB_GUI_WidgetActivate( pBut_OverlayColor, ! Plot3D_Active);

  // Check for image changed.

  YaIPS_GUI_DiffUsesCalcThings();

  // Update GUI Elements

  if( pSlider_3D_Azimuth != NULL) {

    // Update slider if value has changed elsewhere
    if( (int)pSlider_3D_Azimuth->value() != YaIPS_BigImageDisp.Plot3D_Azimuth) {

      AnyWidgetChanged = true;    // Catch change of any widget

      pSlider_3D_Azimuth->value( YaIPS_BigImageDisp.Plot3D_Azimuth);
    }
  }

  if( pSlider_3D_Elevation != NULL) {

    // Update slider if value has changed elsewhere
    if( (int)pSlider_3D_Elevation->value() != YaIPS_BigImageDisp.Plot3D_Elevation) {

      AnyWidgetChanged = true;    // Catch change of any widget

      pSlider_3D_Elevation->value( YaIPS_BigImageDisp.Plot3D_Elevation);
    }
  }

  // Keep image background color up to date. May be changed by load of a preset.

  if( YaIPS_Color_IMG_BGND != YaIPS_BigImageDisp.pImage_Box->color()) {     // Changed color for main window image backgroudn

    YaIPS_BigImageDisp.pImage_Box->color( YaIPS_Color_IMG_BGND);            // Background color
    YaIPS_BigImageDisp.pImage_Box->redraw();
  }

  // Keep right side color up to date. May be changed by load of a preset.

  if( YaIPS_Color_RIGHT_BGND != pGUI_Main_RightSide->color()) {     // Changed color for main window right side box

    pGUI_Main_RightSide->color( YaIPS_Color_RIGHT_BGND);
    pGUI_Main_RightSide->redraw();
  }

  // If any widget has changed

  if( AnyWidgetChanged) {    // Catch change of any widget

    //x/Sleep( 15);         // Give up some time
  }

  // Check for move of tool windows inside right side

  if( pGUI_Main != NULL &&               // Have the main window
      pGUI_Main_RightSide != NULL) {     // Have the right side box

    int RightSide_X, RightSide_Y;
    int DeltaX, DeltaY;

    RightSide_X = pGUI_Main->x_root() + pGUI_Main_RightSide->x();
    RightSide_Y = pGUI_Main->y_root() + pGUI_Main_RightSide->y();

    if( RightSide_LastSkip <= 0) {

      DeltaX = RightSide_X - RightSide_LastX;
      DeltaY = RightSide_Y - RightSide_LastY;

    } else {                                 // Position not latched until now

      DeltaX = 0;
      DeltaY = 0;

      RightSide_LastSkip -= 1;               // Position delta test is skipped
    }

    if( DeltaX != 0 || DeltaY != 0)  {       // Right side has been moved

      YaIPS_WindowsToolWinAddPosDelta( RightSide_LastX, RightSide_LastY, pGUI_Main_RightSide->w(), pGUI_Main_RightSide->h(),
                                       DeltaX, DeltaY);

    }

    RightSide_LastX = RightSide_X;
    RightSide_LastY = RightSide_Y;
  }
}

/************************************************************************************
 * IqeB_GUI_ToolsMyIdleAction
 *
 * Manage display if big image in image box and GUI update.
 */

static void IqeB_GUI_ToolsMyIdleAction( void *)
{
  unsigned int TimeTemp;
  static unsigned int TimeLastCalled = 0;

  // Wait for all tool windows to started up
  if( YaIPS_GUI_Main_Do_Startup) {  // Startup phase of tool windows

    return;
  }

  TimeTemp = GetTickCount();           // Get current time

  //
  // some timed actions (not each call)
  //

  if( TimeTemp - TimeLastCalled >= 50) {   // 50 ms gone since last call

    TimeLastCalled = TimeTemp;             // Remember last time called

    // some time gone, do ...

    //  Check for GUI updates

    MyWinUpdate();                          // Update GUI of this tool window

    // Hack to get info area update
    // redraw() calls on other places don't work.

    if( pGUI_Box_DiffUses != NULL) {        // Not zero until now

      pGUI_Box_DiffUses->redraw();          // Force redraw
    }
  }


  // Check for size change

  YaIPS_ImageDispDrawUpdate( &YaIPS_BigImageDisp, false);

  return;
}

/************************************************************************************
 * IqeB_GUI_OpenGLIdleAction
 *
 * Periodically save presets and ...
 */

static void IqeB_GUI_OpenGLIdleAction( void *)
{
  unsigned int TimeTemp;
  static unsigned int TimeLastCalled_100 = 0;

  // Wait for all tool windows to started up
  if( YaIPS_GUI_Main_Do_Startup) {  // Startup phase of tool windows

    return;
  }

  TimeTemp = GetTickCount();

  //
  // some timed actions (not each call)
  //

  if( TimeTemp - TimeLastCalled_100 >= 100) {  // 100 ms gone since last call

    TimeLastCalled_100 = TimeTemp;             // Remember last time called

    // some time gone, do ...

    // Update full screen of main window

    FullScreenActive = pGUI_Main->fullscreen_active();

    // update window size

    if( FullScreenActive == 0) {    // full screen is not set

      // Update size only if full screen is NOT active

      YaIPS_Main_WinSizeX = pGUI_Main->w();  // update window size
      YaIPS_Main_WinSizeY = pGUI_Main->h();  // update window size

      // Update right side box width

      RightSideWidth = pGUI_Main_RightSide->w();

    } else {

      // Update right side box width for full screen

      RightSideWidthFS = pGUI_Main_RightSide->w();
    }

    // Update others
    YaIPS_Main_ImageSourceID_Last = YaIPS_BigImageDisp.ImageSourceID;

#ifdef use_again
#ifdef _DEBUG
    // Dump the window table to the console
    YaIPS_ToolWinDumpWindows();
#endif
#endif

    IqeB_PreferencesUpdateChanges();  // update preferences
  }

  // update preferences all 2 seconds

  if( TimeTemp - IqeB_PreferencesUpdateChanges_TimeLastCalled >= 2000) {  // more than 2 seconds gone

    IqeB_PreferencesUpdateChanges();  // update preferences
  }

  // have to redraw OpenGL window

#ifdef WIN32

  // Got a file from an second IqeBrowser application
  // Test the shared memory interface

  if( MultipleRunning_pData != NULL &&       // Have a file mapping
      MultipleRunning_pData[ 0] != '\0') {   // There is something in this memory

    char *p, SaveC;
    char TempFile[ MAX_FILENAME_LEN];       // file to load

    // Ensure we have an absolute file name

    fl_filename_absolute( TempFile, sizeof( TempFile) - 256, MultipleRunning_pData);

    // Test for a file with a file path before
    p = strrchr( TempFile, '/');                   // Test for last path delimiter
    if( !p) p = strrchr( TempFile, '\\');
    if( p) {                                       // point to last path slash

      // Change to this directory

      p = p + 1;               // Point after path delimiter (YaIPS_BrowserDirectory needs this)

      SaveC = *p;              // save this char
      *p = '\0';               // set end of string

      strcpy( YaIPS_BrowserDirectory, TempFile);

      *p = SaveC;              // restore

      IqeB_FileNormalizePathChars( YaIPS_BrowserDirectory);

#ifdef use_again
      // Refresh file browser, selecte file (if it is listed)
      FileBrowserRefresh( true, p);

      // and load the file

      IqeB_DispPrepareNewModel( TempFile, NULL); // load this file
#endif

      // Bring window to foreground

      pGUI_Main->show();
    }

    MultipleRunning_pData[ 0] = '\0';        // Flag, data is processed
  }
#endif
}

/************************* End Of File *************************/
