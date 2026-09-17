/****************************************************************************

  YaIPS_GUI_Settings.cpp

  27.03.2025 RR: First edition of this file.
  03.09.2026 RR: Replace use of the working directory string
                 'YaIPS_WorkingDirectory' by '.'.
  16.09.2026 RR: * Replace use of the working directory string

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

#define MY_WIN_PREF_NAME  "Settings"              // Name used for the preference data

/************************************************************************************
 * Globals
 *
 */

#define MAX_NUM_LANGUAGE_DIRS            64  // Max number of language directories
#define MAX_LEN_LANGUAGE_NAME            64  // Max length of a language name

//
// Global setting
//

// Common settings

char YaIPS_Setting_Language[ MAX_LEN_LANGUAGE_NAME];  // Language to load at startup
int  YaIPS_Setting_Startup_WinRestore = true;    // If true, open windows from last session
int  YaIPS_Setting_Wide_Graphic_Lines = false;   // If true, use wide lines for graphics
int  YaIPS_Setting_PasteImgCol2BW     = true;    // If true, support conversion from pseudo BW color images to a BW images
int  YaIPS_Setting_File_JPEG_Quality  = 90;      // JPEG quality used for writing IPEG files

// Colors

unsigned int YaIPS_Color_IMG_BGND   =  37; // Color for background of image boxes. 37: a gray value
unsigned int YaIPS_Color_GRA_FRAME  = 131; // For some graphics (row sum, ..) color for the frame
unsigned int YaIPS_Color_RIGHT_BGND = 140; // Right side box background (used to place tool windows)

unsigned int YaIPS_Setting_ChooseColorPredef[ YAIPS_CUSTOM_COLORS_N]; // Array of custom colors for choose color dialog

unsigned int YaIPS_Setting_CopyPasteColor;    // Use this variable to copy/paste color values for the color button

// Inform user to restart after change

static int BackupSettingsDone = 0;                     // if true backup on first dialog open was done

static char BACKUP_Language[ MAX_LEN_LANGUAGE_NAME];   // Save this to check for change

/************************************************************************************
 * Statics
 */

static  Fl_Window *pMyToolWin;
static int MyWinPosX = IQE_GUI_NO_WINPOS_X, MyWinPosY = IQE_GUI_NO_WINPOS_Y; // last window position

static IqeFl_Tabs      *pTab_Groups;         // Point to tabulator GUI element
static int Tab_Group_Selected;               // Number of last selected tab group.

static char LanguageDirs[ MAX_NUM_LANGUAGE_DIRS][MAX_LEN_LANGUAGE_NAME]; // The language directories
static int nLanguageDirs;                   // Number of language directories

/************************************************************************************
 * Presets for this tools window
 *
 */

 static T_GUI_PreferenceEntry MyPreferences[] =
 {

  // Hold last selected tab

  { PREF_T_INT,    "Group_Selected",            "0", &Tab_Group_Selected},

  // Common settings

  { PREF_T_STRING, "Language",            "English", &YaIPS_Setting_Language, sizeof( YaIPS_Setting_Language) - 1},
  { PREF_T_INT,    "Startup_WinRestore",        "1", &YaIPS_Setting_Startup_WinRestore},
  { PREF_T_INT,    "Wide_Graphic_Lines",        "0", &YaIPS_Setting_Wide_Graphic_Lines},
  { PREF_T_INT,        "PasteImgCol2BW",        "1", &YaIPS_Setting_PasteImgCol2BW},
  { PREF_T_INT,    "File_JPEG_Quality",        "90", &YaIPS_Setting_File_JPEG_Quality},

  // Colors

  { PREF_T_INT,     "Color_IMG_BGND",          "37", &YaIPS_Color_IMG_BGND},
  { PREF_T_INT,    "Color_GRA_FRAME",         "131", &YaIPS_Color_GRA_FRAME},
  { PREF_T_INT,   "Color_RIGHT_BGND",         "140", &YaIPS_Color_RIGHT_BGND},

  // Predefine colors for choose color dialog

  { PREF_T_INT,    "Color_PreDef_00",    "0x0000ff", YaIPS_Setting_ChooseColorPredef +  0},      // Red
  { PREF_T_INT,    "Color_PreDef_01",    "0x00ff00", YaIPS_Setting_ChooseColorPredef +  1},      // Green
  { PREF_T_INT,    "Color_PreDef_02",    "0xff0000", YaIPS_Setting_ChooseColorPredef +  2},      // Blue
  { PREF_T_INT,    "Color_PreDef_03",    "0x00ffff", YaIPS_Setting_ChooseColorPredef +  3},      // Yellow
  { PREF_T_INT,    "Color_PreDef_04",    "0xffff00", YaIPS_Setting_ChooseColorPredef +  4},      // Cyan
  { PREF_T_INT,    "Color_PreDef_05",    "0xff00ff", YaIPS_Setting_ChooseColorPredef +  5},      // Magenta
  { PREF_T_INT,    "Color_PreDef_06",    "0xffffff", YaIPS_Setting_ChooseColorPredef +  6},      // White
  { PREF_T_INT,    "Color_PreDef_07",    "0x000000", YaIPS_Setting_ChooseColorPredef +  7},      // Black
  { PREF_T_INT,    "Color_PreDef_08",    "0x000080", YaIPS_Setting_ChooseColorPredef +  8},      // Dark red
  { PREF_T_INT,    "Color_PreDef_09",    "0x008000", YaIPS_Setting_ChooseColorPredef +  9},      // Dark green
  { PREF_T_INT,    "Color_PreDef_10",    "0x800000", YaIPS_Setting_ChooseColorPredef + 10},      // Dark blue
  { PREF_T_INT,    "Color_PreDef_11",    "0x008080", YaIPS_Setting_ChooseColorPredef + 11},      // Dark yellow
  { PREF_T_INT,    "Color_PreDef_12",    "0x808000", YaIPS_Setting_ChooseColorPredef + 12},      // Dark cyan
  { PREF_T_INT,    "Color_PreDef_13",    "0x800080", YaIPS_Setting_ChooseColorPredef + 13},      // Dark magenta
  { PREF_T_INT,    "Color_PreDef_14",    "0xaaaaaa", YaIPS_Setting_ChooseColorPredef + 14},      // Light gray
  { PREF_T_INT,    "Color_PreDef_15",    "0x555555", YaIPS_Setting_ChooseColorPredef + 15},      // Dark gray

  // Predefine colors for choose color dialog

  { PREF_T_INT,    "CopyPasteColor",    "0x000000", &YaIPS_Setting_CopyPasteColor},      // Black
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&pMyToolWin), &MyWinPosX, &MyWinPosY);

/************************************************************************************
 * IqeB_GUI_CBox_SetValue_Callback
 */

static void IqeB_GUI_CBox_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  int *pValue;
  Fl_Check_Button *pThis;

  // ...

  pThis  = (Fl_Check_Button *)w;
  pValue = (int *)pValueArg;             // get pointer to associated variable

  *pValue = pThis->value();              // update the variable
}

/************************************************************************************
 * IqeB_GUI_But_Color_SetValue_Callback
 */

static void IqeB_GUI_But_Color_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  unsigned int *pColor;
  Fl_Button *pThis;

  // ...

  pThis  = (Fl_Button *)w;
  pColor = (unsigned int *)pValueArg;    // get pointer to associated variable

  *pColor = IqeB_GUI_ColorChooser( *pColor);
  pThis->color( *pColor);
  pThis->parent()->redraw();

  if( pValueArg == &YaIPS_Color_RIGHT_BGND) {     // Changed color for maind window right side box

    pGUI_Main_RightSide->color( *pColor);
    pGUI_Main_RightSide->redraw();
  }
}

/************************************************************************************
 * IqeB_GUI_Choice_SetValue_Callback
 */

static void IqeB_GUI_Choice_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  int Value;
  Fl_Choice *pThis;

  // ...

  pThis  = (Fl_Choice *)w;
  Value  = pThis->value();

  //
  // Test for direct value change
  //

  if( pValueArg == YaIPS_Setting_Language) {  // Have assign to language variable

    if( Value >= 0 && Value <= MAX_NUM_LANGUAGE_DIRS) {  // Security test index

      memset( YaIPS_Setting_Language, 0, sizeof( YaIPS_Setting_Language));

      strncpy( YaIPS_Setting_Language, LanguageDirs[ Value], sizeof( YaIPS_Setting_Language) - 1);
    }

    return;
  }
}

/************************************************************************************
 * IqeB_GUI_Int_SetValue_Callback
 */

static void IqeB_GUI_Int_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  int Value, *pValue, WasClipped;
  IqeFl_Int_Input *pThis;
  char TempBuffer[ 256];

  // ...

  pThis  = (IqeFl_Int_Input *)w;
  pValue = (int *)pValueArg;               // get pointer to associated variable

  Value = atoi( pThis->value());           // get the value

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

      sprintf( TempBuffer, "%d", Value);   // Update on GUI
      pThis->value( TempBuffer);
    }
  }

//x/ExitPoint:

  *pValue = Value;         // update the variable
}

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyWinUpdate()
{

  // Get last selected tab group

  Tab_Group_Selected = pTab_Groups->GetTabGroup();
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

    // update window size

    //x/MyWinSizeX = pMyToolWin->w();  // update window size
    //x/MyWinSizeY = pMyToolWin->h();  // update window size

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

  //

  if( BackupSettingsDone == true) {  // Backup of settings was done

    // Test for change
    if( strcmp( BACKUP_Language, YaIPS_Setting_Language) != 0) {

      // One of the settings is different

      fl_message( LangStringLookup( "&GUI_Settings_NeedRestart=Settings that require a restart\nof the application have been changed!"));

      BackupSettingsDone = 2;    // Flag user is informed about change only once.
    }
  }

  // ...

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::remove_idle( IqeB_GUI_ToolsMyIdleAction);      // Redraw window during idle
#endif
  Fl::remove_check( IqeB_GUI_ToolsMyIdleAction);     // Check small image size change

  IqeB_GUI_CloseToolWindow( (void **)&pMyToolWin);
}

/************************************************************************************
 * YaIPS_GUI_SettingsWin
 *
 * Open dialog
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void YaIPS_GUI_SettingsWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{

  //
  // window already created --> show it
  //

  if( pMyToolWin != NULL) {         // already have tool window

    // Show invisible window or bring visible window to foreground

    pMyToolWin->show();          // show it

    return;
  }

  // Backup some settings to allow check for need of restart

  if( ! BackupSettingsDone) {     // No Backup done until now

    BackupSettingsDone = true;    // Backup on first open of dialog is done

    strncpy( BACKUP_Language, YaIPS_Setting_Language, sizeof( BACKUP_Language) - 1);
  }

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

    pMyToolWin = new Fl_Window( xPos, yPos, 248, 174, LANGDEF_SETTINGS);

    if( pMyToolWin == NULL) {  // security test

      return;
    }
  }

  //
  //  GUI things
  //

  int x1, x2, xx1, xx2, y, yy;
  //x/int xx2, xc;
  int yGroup;
  //x/char TempBuffer[ 256];

  Fl_Check_Button *pTemp_Check_Button;
  //x/Fl_Float_Input  *pTemp_Float_Input;
  Fl_Box          *pTemp_Box;
  IqeFl_Int_Input *pTemp_Int;
  IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group;
  Fl_Button       *pTemp_Button;
  Fl_Choice       *pTemp_Choice;
  //x/Fl_Radio_Round_Button *pRadioButTemp;

  // Scan the language sup directory

  char Files_path[ MAX_FILENAME_LEN];
  int  numFiles, i, LenName;
  dirent **list;
  char *pName;

  // Construct path to language directory
  strcpy( Files_path, "./Languages");

  IqeB_FileNormalizePathChars( Files_path);

  numFiles = fl_filename_list( Files_path, &list, fl_alphasort);

  // pass 1, get the amount of data we need

  nLanguageDirs = 0;

  for( i = 0; i < numFiles; i++) {

    if( nLanguageDirs > MAX_NUM_LANGUAGE_DIRS) {  // Table is full

      break;
    }

    pName = list[i]->d_name;

    LenName = strlen( pName);

    if( pName[ 0] == '\0' ||   // End of string
        pName[ 0] == '.' ) {   // current dir or dir up

      continue;
    }

    // Directories have a '/' as last character
    if( LenName < 2 || pName[ LenName - 1] != '/') { // Is no directory

      continue;
    }

    // Have a sub directory here

    if( strnicmp( pName, "YaIPS", 5) == 0) {  // Skip files starting with 'YaIPS'

      continue;
    }

    if( LenName >= MAX_LEN_LANGUAGE_NAME - 1) {   // Name is to long to store

      continue;
    }

    strncpy( LanguageDirs[ nLanguageDirs], pName, LenName - 1); // Store to table

    nLanguageDirs += 1;
  }

  // Free the file list
  fl_filename_free_list( &list, numFiles);

  // ...

  x1  = 4;
  xx1 = pMyToolWin->w() - 16;
  //x/xx2 = xx1 / 2;
  //x/xc  = pMyToolWin->w() / 2;          // x center
  yy  = 26;

  y = 4;

  //
  // Tabs
  //

  pTemp_Tabs = new IqeFl_Tabs( x1, y, pMyToolWin->w() - x1 - 4, pMyToolWin->h() - y - 4);
  pTemp_Tabs->selection_color( YAIPS_COLOR_SELECTION);
  pTab_Groups = pTemp_Tabs;

  y += 26;

  //
  // Group 'Settings'
  //

  yGroup = y;

  pTemp_Group = new Fl_Group( x1, y, pMyToolWin->w() - x1 - 4, pMyToolWin->h() - y - 4, LANGDEF_SETTINGS);
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Settings_TabA1a=Basic settings"));

    // checkbox

    y += 4;

    y += 20;

    pTemp_Choice = new Fl_Choice( x1 + 4, y, xx1, yy, LangStringLookup( "&GUI_Settings_TabA2=Language"));
    pTemp_Choice->align( FL_ALIGN_TOP_LEFT);     // align for label
    pTemp_Choice->callback( IqeB_GUI_Choice_SetValue_Callback, YaIPS_Setting_Language);
    pTemp_Choice->tooltip( LangStringLookup( "&GUI_Settings_TabA2a="
                           "Select language for the user interface\n"
                           "ATTENTION: Close the application and restart it!"));

    for( i = 0; i < nLanguageDirs; i++) {

      pTemp_Choice->add( LanguageDirs[ i]);    // Add entry

      if( YaIPS_Setting_Language[ 0] != 0 &&    // Test for last selected entry
          stricmp( YaIPS_Setting_Language, LanguageDirs[ i]) == 0) {

        pTemp_Choice->value( i);
      }
    };

    y += yy;

    y += 4;

    yy = 20;   // Check boxes are not so high

    pTemp_Check_Button = new Fl_Check_Button( x1 + 4, y, xx1, yy, LangStringLookup( "&GUI_Settings_TabA3=Restore windows"));
    pTemp_Check_Button->tooltip( LangStringLookup( "&GUI_Settings_TabA3a="
                                 "The next time the application is started, the\n"
                                 "windows from the last session will be reopened."));
    pTemp_Check_Button->value( YaIPS_Setting_Startup_WinRestore);
    pTemp_Check_Button->callback( IqeB_GUI_CBox_SetValue_Callback, &YaIPS_Setting_Startup_WinRestore);

    y += yy;

    pTemp_Check_Button = new Fl_Check_Button( x1 + 4, y, xx1, yy, LangStringLookup( "&GUI_Settings_TabA4=Thick lines for graphics"));
    pTemp_Check_Button->tooltip( LangStringLookup( "&GUI_Settings_TabA4a="
                                 "Some graphics use thicker lines\n"
                                 "for better visibility."));
    pTemp_Check_Button->value( YaIPS_Setting_Wide_Graphic_Lines);
    pTemp_Check_Button->callback( IqeB_GUI_CBox_SetValue_Callback, &YaIPS_Setting_Wide_Graphic_Lines);

    y += yy;

    pTemp_Check_Button = new Fl_Check_Button( x1 + 4, y, xx1, yy, LangStringLookup( "&GUI_Settings_TabA5=Paste converts color -> BW"));
    pTemp_Check_Button->tooltip( LangStringLookup( "&GUI_Settings_TabA5a="
                                 "Paste an image with 'Ctrl+V' supports converting\n"
                                 "color images to black and white images.\n"
                                 "For this to happen, the R G B components of all\n"
                                 "pixels must have the same values."));
    pTemp_Check_Button->value( YaIPS_Setting_PasteImgCol2BW);
    pTemp_Check_Button->callback( IqeB_GUI_CBox_SetValue_Callback, &YaIPS_Setting_PasteImgCol2BW);

    y += yy + 2;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1 + 4, y, xx2, yy, LangStringLookup( "&GUI_Settings_TabA10=JPEG quality"));
    pTemp_Int->align( FL_ALIGN_RIGHT);     // align for label
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Settings_TabA10a="
                                          "Used for writing JPEG image files.\n"
                                          "Quality is between 10 and 100.\n"
                                          "Higher quality looks better but\n"
                                          "results in a bigger image files."));
    pTemp_Int->SetValue( YaIPS_Setting_File_JPEG_Quality);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &YaIPS_Setting_File_JPEG_Quality);
    pTemp_Int->SetModifyData( 10, 100, 10, 1);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'Colors'
  //

  y = yGroup;

  pTemp_Group = new Fl_Group( x1, y, pMyToolWin->w() - x1 - 4, pMyToolWin->h() - y - 4, LangStringLookup( "&GUI_Settings_TabB1=Colors"));

    // enable checkbox

    y += 4;
    y += 2;

    x2 = pMyToolWin->w() - yy * 2 - 12;

    pTemp_Box = new Fl_Box( x2, y, 1, yy - 4, LangStringLookup( "&GUI_Settings_TabB2=Background of images"));
    pTemp_Box->align( FL_ALIGN_LEFT);
    pTemp_Box->box( FL_NO_BOX);

    pTemp_Button = new Fl_Button( x2 + 4, y, yy * 2, yy - 4, "");
    pTemp_Button->color( YaIPS_Color_IMG_BGND);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &YaIPS_Color_IMG_BGND);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Settings_TabB2a="
                           "Background of image display windows.\n"
                           "ATTENTION: Close the application and restart it!"));

    y += yy;

    pTemp_Box = new Fl_Box( x2, y, 1, yy - 4, LangStringLookup( "&GUI_Settings_TabB3=Frame color"));
    pTemp_Box->align( FL_ALIGN_LEFT);
    pTemp_Box->box( FL_NO_BOX);

    pTemp_Button = new Fl_Button( x2 + 4, y, yy * 2, yy - 4, "");
    pTemp_Button->color( YaIPS_Color_GRA_FRAME);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &YaIPS_Color_GRA_FRAME);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Settings_TabB3a="
                                             "Colors of the frames for some graphics.\n"
                                             "Column/row sum, ..."));

    y += yy;

    pTemp_Box = new Fl_Box( x2, y, 1, yy - 4, LangStringLookup( "&GUI_Settings_TabB4=Background of right side"));
    pTemp_Box->align( FL_ALIGN_LEFT);
    pTemp_Box->box( FL_NO_BOX);

    pTemp_Button = new Fl_Button( x2 + 4, y, yy * 2, yy - 4, "");
    pTemp_Button->color( YaIPS_Color_RIGHT_BGND);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &YaIPS_Color_RIGHT_BGND);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Settings_TabB4a="
                                             "Background of the right side of the main window.\n"
                                             "This area is used for placing tool windows."));

    // Finish things for this group

    pTemp_Group->end();

  // finish up

  pTemp_Tabs->end();

  pTemp_Tabs->SetTabGroup( Tab_Group_Selected);    // Select tab group from last session

  // finish up

  pMyToolWin->end();
  pMyToolWin->set_modal();
  pMyToolWin->callback( close_cb, &pMyToolWin);
  pMyToolWin->show();

  // Hack: Add close button to window caption
  YaIPS_DialogAddCloseButton( pMyToolWin);

  // Add idle action for this window

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::add_idle( IqeB_GUI_ToolsMyIdleAction);      // Redraw window during idle
#endif
  Fl::add_check( IqeB_GUI_ToolsMyIdleAction);     // Check small image size change
}

/********************************** End Of File **********************************/
