/****************************************************************************

  YaIPS_GUI_CustomColors.cpp

  Custom color manager

  15.03.2026 RR: First edition of this file.

*****************************************************************************
*/

#include <sysinfoapi.h>
#include <synchapi.h>

// Other includes
#include "YaIPS.h"

/************************************************************************************
* Defines for this source file.
*/

// Defines for windows ID
#define MY_WIN_ID     YAIPS_WIN_ID_CUSTOM_COLORS  // Source specific windows ID
#define MY_WIN_MAX    1                           // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_CustomColors_Title=Custom colors"      // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinCustomColors"      // Name used for the preference data

// Define for window sizes: This window is not resizable
#define MYWIN_SIZE_X_MIN       120
#define MYWIN_SIZE_X_MAX       314
#define MYWIN_SIZE_X_DEFAULT   (MYWIN_SIZE_X_MIN + MYWIN_SIZE_X_MIN) / 2

#define MYWIN_SIZE_Y_MIN       474
#define MYWIN_SIZE_Y_MAX       MYWIN_SIZE_Y_MIN
#define MYWIN_SIZE_Y_DEFAULT   MYWIN_SIZE_Y_MIN

/************************************************************************************
* forwards
*/

static void close_cb( Fl_Widget *w, long int iToolData);

/************************************************************************************
* Data managed by this windows
*/

// The custom color values are defined in this table defined in YaIPS.h

// unsigned int YaIPS_Setting_ChooseColorPredef[ YAIPS_CUSTOM_COLORS_N];

// Names for the custom colors
static char CustomColorNames[ YAIPS_CUSTOM_COLORS_N][ 32];

// Name of last loaded file
static char CustomColorFileName[ 256]; // Filename without path and extension

/************************************************************************************
* Local variables for this window
*/

static  Fl_Double_Window *pMyToolWin;
static int IsOpen;                           // True if this window is open.
static int MyWinPosX  = IQE_GUI_NO_WINPOS_X, MyWinPosY = IQE_GUI_NO_WINPOS_Y; // last window position
static int MyWinSizeX = MYWIN_SIZE_X_DEFAULT, MyWinSizeY = MYWIN_SIZE_Y_DEFAULT; // last window size

// GUI elements

static Fl_Button *pGUI_File_Load;              // Load color presets from file
static Fl_Button *pGUI_File_Save;              // Save color presets to file
static Fl_Input  *pColFileName;
static Fl_Button *pColorButton[ YAIPS_CUSTOM_COLORS_N];   // Color buttons
static Fl_Input  *pColorName[ YAIPS_CUSTOM_COLORS_N];     // Color names

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

  // Color names
  { PREF_T_STRING, "FileName",  "", CustomColorFileName, sizeof( CustomColorFileName) - 1},

  // Color names
  { PREF_T_STRING, "Name_00",   "", CustomColorNames +  0, sizeof( CustomColorNames[ 0]) - 1},
  { PREF_T_STRING, "Name_01",   "", CustomColorNames +  1, sizeof( CustomColorNames[ 1]) - 1},
  { PREF_T_STRING, "Name_02",   "", CustomColorNames +  2, sizeof( CustomColorNames[ 2]) - 1},
  { PREF_T_STRING, "Name_03",   "", CustomColorNames +  3, sizeof( CustomColorNames[ 3]) - 1},
  { PREF_T_STRING, "Name_04",   "", CustomColorNames +  4, sizeof( CustomColorNames[ 4]) - 1},
  { PREF_T_STRING, "Name_05",   "", CustomColorNames +  5, sizeof( CustomColorNames[ 5]) - 1},
  { PREF_T_STRING, "Name_06",   "", CustomColorNames +  6, sizeof( CustomColorNames[ 6]) - 1},
  { PREF_T_STRING, "Name_07",   "", CustomColorNames +  7, sizeof( CustomColorNames[ 7]) - 1},
  { PREF_T_STRING, "Name_08",   "", CustomColorNames +  8, sizeof( CustomColorNames[ 8]) - 1},
  { PREF_T_STRING, "Name_09",   "", CustomColorNames +  9, sizeof( CustomColorNames[ 9]) - 1},
  { PREF_T_STRING, "Name_10",   "", CustomColorNames + 10, sizeof( CustomColorNames[ 10]) - 1},
  { PREF_T_STRING, "Name_11",   "", CustomColorNames + 11, sizeof( CustomColorNames[ 11]) - 1},
  { PREF_T_STRING, "Name_12",   "", CustomColorNames + 12, sizeof( CustomColorNames[ 12]) - 1},
  { PREF_T_STRING, "Name_13",   "", CustomColorNames + 13, sizeof( CustomColorNames[ 13]) - 1},
  { PREF_T_STRING, "Name_14",   "", CustomColorNames + 14, sizeof( CustomColorNames[ 14]) - 1},
  { PREF_T_STRING, "Name_15",   "", CustomColorNames + 15, sizeof( CustomColorNames[ 15]) - 1},

};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&pMyToolWin), &MyWinPosX, &MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, 0,
                                               &IsOpen, IqeB_GUI_CustomColorWin, (Fl_Callback *)close_cb);

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyWinUpdate()
{
  int RedrawWindow, iColor;
  Fl_Widget *pCurrFocus;

  RedrawWindow = false;                      // No redraw of window

  // To periodically updates first

  // Check for update of GUI Elements if values have changed after file load

  pCurrFocus = Fl::focus();

  for( iColor = 0; iColor < YAIPS_CUSTOM_COLORS_N; iColor++) {

    // Check button color

    uchar r,g,b;

    r = YaIPS_Setting_ChooseColorPredef[ iColor] & 0xff;
    g = (YaIPS_Setting_ChooseColorPredef[ iColor] >> 8) & 0xff;
    b = (YaIPS_Setting_ChooseColorPredef[ iColor] >> 16) & 0xff;

    if( pColorButton[ iColor]->color() != fl_rgb_color( r, g, b)) {     // color is different

      pColorButton[ iColor]->color( fl_rgb_color( r, g, b));

      RedrawWindow = true;
    }

    // Check color name

    if( pCurrFocus == pColorName[ iColor]) {               // Input element has keyboard focus ?

      continue;   // Skip, otherwise typing inputs will be changed
    }

    if( strcmp( pColorName[ iColor]->value(), CustomColorNames[ iColor]) != 0) {  // Name is different

      pColorName[ iColor]->value( CustomColorNames[ iColor]);
      pColorName[ iColor]->insert_position( 0);                    // Position to begin of text

      RedrawWindow = true;
    }
  }

  if( RedrawWindow) {                        // NEED redraw of window ?

    pMyToolWin->redraw();
  }
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

    MyWinSizeX = pMyToolWin->w();  // update window size
    MyWinSizeY = pMyToolWin->h();  // update window size

    // Periodically updates on GUI
    MyWinUpdate();
  }

  return;
}

/************************************************************************************
 * IqeB_GUI_But_Color_SetValue_Callback
 */

static void IqeB_GUI_But_Color_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  unsigned int *pColor;
  Fl_Color ColorBefore, ColorAfter;
  Fl_Button *pThis;
  uchar r,g,b;

  // ...

  pThis  = (Fl_Button *)w;
  pColor = (unsigned int *)pValueArg;    // get pointer to associated variable

  r = *pColor & 0xff;
  g = (*pColor >> 8) & 0xff;
  b = (*pColor >> 16) & 0xff;

  ColorBefore = fl_rgb_color( r, g, b);

  ColorAfter = IqeB_GUI_ColorChooser( ColorBefore);

  if( ColorBefore != ColorAfter) {

    Fl::get_color( ColorAfter, r, g, b);

    *pColor = r | (g << 8) | (b << 16);

    pThis->color( ColorAfter);
    pThis->parent()->redraw();
  }

}

/************************************************************************************
 * IqeB_GUI_Input_SetValue_Callback
 *
 * Callback, set one of the color names
 */

static void IqeB_GUI_Input_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  char *pString, *pValue;
  Fl_Input *pThis;
  int SizeOfString;

  pThis   = (Fl_Input *)w;
  pString = (char *)pValueArg;             // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pString == NULL) {

    return;
  }

  // Set value

  pValue = (char*)pThis->value();

  SizeOfString = sizeof( CustomColorNames[ 0]);   // Size of color name field

  memset( pString, 0, SizeOfString);  // Zero string
  strncpy( pString, pValue, SizeOfString - 1);
}

/************************************************************************************
 * CustomColor_Load_cb
 *
 * Load custom color file
 */

static void CustomColor_Load_cb( Fl_Widget *w)
{
  Fl_Native_File_Chooser fc;
  char FileFilter[ 1024];
  char *pFileName;
  char TempFileName[ FILENAME_MAX + 16];
  char FileName[ 256];
  int ierr, iColor, StringLen;
  FILE *pFile;
  int r,g,b;
  char line[ 1024], CustomColorName[ 256], *pInLine;

  // Initialize the file chooser. Only can save txt images

  strcpy( FileFilter, "*.txt\n");

  fc.filter( FileFilter);
  fc.options( Fl_Native_File_Chooser::SAVEAS_CONFIRM | Fl_Native_File_Chooser::USE_FILTER_EXT);

  pFileName = NULL;      // NO preset file name

  if( CustomColorFileName[ 0] != 0) {

    sprintf( TempFileName, "%s.txt", CustomColorFileName);

  } else {

    sprintf( TempFileName, "%s.txt", LangStringLookup( "&GUI_CustomColors_File_Preset=File name"));
  }

  fc.preset_file( TempFileName);

  fc.title( LangStringLookup( "&GUI_CustomColors_Load1=Load custom color file"));
  fc.type( Fl_Native_File_Chooser::BROWSE_FILE);  // only picks files that exist

  // Path for directory with custom colors
  sprintf( TempFileName, "%s/CustomColors", YaIPS_WorkingDirectory);
  IqeB_FileNormalizePathChars( TempFileName);
  fc.directory( TempFileName);                         // Set browser directory

  ierr = fc.show();                                    // Open file chooser dialog

  if( ierr != 0) {      // User cancelled or error

    return;
  }

  // Have a filename here

  // Have a filename here. Ensure a txt file extension.

  pFileName = (char *)fc.filename();

  IqeB_FileEnsureExtension( pFileName, (char *)"txt", TempFileName, sizeof( TempFileName));

  pFileName = TempFileName;

  // Set file name GUI element

  IqeB_FileGetBaseName( pFileName, FileName, sizeof( FileName));

  memset( CustomColorFileName, 0, sizeof( CustomColorFileName));
  strncpy( CustomColorFileName, FileName, sizeof( CustomColorFileName));

  pColFileName->value( CustomColorFileName);
  pColFileName->insert_position( 0);            // Position to begin of text

  // Load from file

  IqeB_FileGetFileName( pFileName, FileName, sizeof( FileName));

  ierr = 0;                              // Preset no error

  pFile = fl_fopen( pFileName, "rt");    // Try to open file

  if( pFile == NULL) {                   // Could not create the file

    ierr = 1;                            // Set error code

    goto ErrorExit;
  }

  // File holds only comments or color entries

  iColor = 0;

  ierr = 0;                          // Preset no error

  for( ; ; ) {

    // Try to read next line

    if( !fgets( line, sizeof line, pFile)) {

      break;
    }

    // Skip comment lines

    if( line[ 0] == '/' && line[ 1] == '/') {   // is comment line

      continue;
    }

    // remove line feed and space at end of line

    StringLen = strlen( line);

    while( StringLen > 0 && (line[ StringLen - 1] & 0x0ff) <= ' ') {

      line[ StringLen - 1] = '\0';  //  Set end of string

      StringLen -= 1;
    }

    // Skip empty lines

    if( line[ 0] == '\0') {   // is empty line

      continue;
    }

    // Data lines have a number with a double point at begin. Check for the double point.

    pInLine = strchr( line, ':');

    if( pInLine == NULL) {     // No double point

      continue;                // Simply skip the line
    }

    // Read in color data

    r = g = b = 0;

    memset( CustomColorName, 0, sizeof( CustomColorName));

    pInLine += 1;

    if( sscanf( pInLine, "%d %d %d", &r, &g, &b) < 3) {   // Minimum need the numbers

      ierr = 2;                            // Set error code

      goto ErrorExit;
    }

    if( iColor == 0) {                 // First time we come to here

      // Zero custom colors and color names

      memset( YaIPS_Setting_ChooseColorPredef, 0, sizeof( YaIPS_Setting_ChooseColorPredef));

      memset( CustomColorNames, 0, sizeof( CustomColorNames));
    }

    // Skip numbers

    while( *pInLine == ' ') pInLine++;       // Skip spaces before first number
    while( isdigit( *pInLine )) pInLine++;   // Skip first number
    while( *pInLine == ' ') pInLine++;       // Skip spaces before second number
    while( isdigit( *pInLine )) pInLine++;   // Skip second number
    while( *pInLine == ' ') pInLine++;       // Skip spaces before third number
    while( isdigit( *pInLine )) pInLine++;   // Skip third number
    while( *pInLine == ' ') pInLine++;       // Skip spaces after third number

    // Get color name

    strncpy( CustomColorName, pInLine, sizeof( CustomColorName) - 1);

    // Clip colors

    if( r <   0) r =   0;
    if( r > 255) r = 255;
    if( g <   0) g =   0;
    if( g > 255) g = 255;
    if( b <   0) b =   0;
    if( b > 255) b = 255;

    // Set color

    YaIPS_Setting_ChooseColorPredef[ iColor] = r | (g << 8) | (b << 16);

    // Set color name

    strncpy( CustomColorNames[ iColor], CustomColorName, sizeof( CustomColorNames[ iColor]) - 1);

    // ...

    iColor += 1;                  // Have one more

    if( iColor >= YAIPS_CUSTOM_COLORS_N) {   // Was last line

      break;
    }
  }

ErrorExit :

  if( pFile != NULL) {                   // File is open

    fclose( pFile);                      // Close file
  }

  return;
}

/************************************************************************************
 * CustomColor_Save_cb
 *
 * Save custom colors file
 */

static void CustomColor_Save_cb( Fl_Widget *w)
{
  Fl_Native_File_Chooser fc;
  char FileFilter[ 1024];
  char *pFileName;
  char TempFileName[ FILENAME_MAX + 16];
  char FileName[ 256];
  int ierr, iColor;
  FILE *pFile;
  uchar r,g,b;

  // Initialize the file chooser. Only can save txt images

  strcpy( FileFilter, "*.txt\n");

  fc.filter( FileFilter);
  fc.options( Fl_Native_File_Chooser::SAVEAS_CONFIRM | Fl_Native_File_Chooser::USE_FILTER_EXT);

  pFileName = NULL;      // NO preset file name

  if( CustomColorFileName[ 0] != 0) {

    sprintf( TempFileName, "%s.txt", CustomColorFileName);

  } else {

    sprintf( TempFileName, "%s.txt", LangStringLookup( "&GUI_CustomColors_File_Preset=File name"));
  }

  fc.preset_file( TempFileName);

  fc.title( LangStringLookup( "&GUI_CustomColors_Save1=Save custom color file"));
  fc.type( Fl_Native_File_Chooser::BROWSE_SAVE_FILE);  // need this if file doesn't exist yet

  // Path for directory with custom colors
  sprintf( TempFileName, "%s/CustomColors", YaIPS_WorkingDirectory);
  IqeB_FileNormalizePathChars( TempFileName);
  fc.directory( TempFileName);                         // Set browser directory

  ierr = fc.show();                                    // Open file chooser dialog

  if( ierr != 0) {      // User cancelled or error

    return;
  }

  // Have a filename here

  // Have a filename here. Ensure a txt file extension.

  pFileName = (char *)fc.filename();

  IqeB_FileEnsureExtension( pFileName, (char *)"txt", TempFileName, sizeof( TempFileName));

  pFileName = TempFileName;

  // Set file name GUI element

  IqeB_FileGetBaseName( pFileName, FileName, sizeof( FileName));

  memset( CustomColorFileName, 0, sizeof( CustomColorFileName));
  strncpy( CustomColorFileName, FileName, sizeof( CustomColorFileName));

  pColFileName->value( CustomColorFileName);
  pColFileName->insert_position( 0);            // Position to begin of text

  // Save to file

  IqeB_FileGetFileName( pFileName, FileName, sizeof( FileName));

  ierr = 0;                              // Preset no error

  pFile = fl_fopen( pFileName, "wt");    // Try to open file

  if( pFile == NULL) {                   // Could not create the file

    ierr = 1;                            // Set error code

    goto ErrorExit;
  }

  fprintf( pFile, "// %s\n", FileName);
  fprintf( pFile, "//\n");
  fprintf( pFile, "// A YaIPS custom color file\n");
  fprintf( pFile, "//\n");
  fprintf( pFile, "//    R   G   B Color name\n");
  fprintf( pFile, "//\n");

  for( iColor = 0; iColor < YAIPS_CUSTOM_COLORS_N; iColor++) {

    r = YaIPS_Setting_ChooseColorPredef[ iColor] & 0xff;
    g = (YaIPS_Setting_ChooseColorPredef[ iColor] >> 8) & 0xff;
    b = (YaIPS_Setting_ChooseColorPredef[ iColor] >> 16) & 0xff;

    fprintf( pFile, "%2d: %3d %3d %3d %s\n", iColor + 1, r, g, b, CustomColorNames[ iColor]);
  }

  fprintf( pFile, "//\n");
  fprintf( pFile, "// End of file\n");
  fprintf( pFile, "//\n");

ErrorExit :

  if( pFile != NULL) {                   // File is open

    fclose( pFile);                      // Close file
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

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::remove_idle( IqeB_GUI_ToolsMyIdleAction);      // Redraw window during idle
#endif
  Fl::remove_check( IqeB_GUI_ToolsMyIdleAction);     // Check small image size change

  IsOpen = false;                                    // Flag info data is not in use

  IqeB_GUI_CloseToolWindow( (void **)&pMyToolWin);
}

/************************************************************************************
 * IqeB_GUI_CustomColorWin
 *
 * Open calibration window
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_CustomColorWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  //
  // window already created --> show it
  //

  if( pMyToolWin != NULL) {         // already have tool window

    // Show invisible window or bring visible window to foreground

    pMyToolWin->show();          // show it

    return;
  }

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

  Fl_Group        *pTemp_Group;
  Fl_Box          *pTemp_Box;
  Fl_Button       *pTemp_Button;
  Fl_Input        *pTemp_Input;

  char TempString[ 256];

  int x1, y, xx1, yy, ySave, wWin, iColor;
  //x/int xx, hWin;

  //x/hWin = pMyToolWin->h();
  wWin = pMyToolWin->w();

  y = 4;

  // File load/save buttons

  yy = 28;

  x1 = 4;

  pTemp_Group = new Fl_Group( x1, y, yy * 2 + 24, yy);

  pGUI_File_Load = new Fl_Button( x1, y, yy, yy, "@+1fileopen");
  pGUI_File_Load->callback( CustomColor_Load_cb);
  pGUI_File_Load->tooltip( LangStringLookup( "&GUI_CustomColors_1a=Load custom colors from file"));
  pGUI_File_Load->labelcolor( YAIPS_BCOL_BUTTON);

  x1 += yy + 5;

  pGUI_File_Save = new Fl_Button( x1, y, yy, yy, "@+1filesave2");
  pGUI_File_Save->callback( CustomColor_Save_cb);
  pGUI_File_Save->tooltip( LangStringLookup( "&GUI_CustomColors_2a=Save custom colors to file"));
  pGUI_File_Save->labelcolor( YAIPS_BCOL_BUTTON);

  pTemp_Group->end();                       // end this group
  pTemp_Group->resizable( 0);

  y += yy + 2;

  // Filename

  yy = 22;

  y += 12;

  x1 = 4;
  xx1 = wWin - 8;

  pColFileName = new Fl_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_CustomColors_3=Name"));
  pColFileName->align( FL_ALIGN_TOP_LEFT);     // align for label
  pColFileName->labelsize( 10);
  pColFileName->tooltip( LangStringLookup( "&GUI_CustomColors_3a="
                                            "Name of last loaded custom color file\n"
                                            "NOTE: is read only"));
  pColFileName->readonly( 1);    // Set read only. String is better visible then deactivate()
  pColFileName->color( YAIPS_COLOR_RONLY_BGND);
  pColFileName->value( CustomColorFileName);
  pColFileName->insert_position( 0);            // Position to begin of text

  y += yy + 2;
  y += 12;

  // Color button

  x1 = 2;

  ySave = y;

  for( iColor = 0; iColor < YAIPS_CUSTOM_COLORS_N; iColor++) {

    uchar r,g,b;

    r = YaIPS_Setting_ChooseColorPredef[ iColor] & 0xff;
    g = (YaIPS_Setting_ChooseColorPredef[ iColor] >> 8) & 0xff;
    b = (YaIPS_Setting_ChooseColorPredef[ iColor] >> 16) & 0xff;

    x1 = 2;

    xx1 = 25;
    sprintf( TempString, "%d:", iColor + 1);

    pTemp_Box = new Fl_Box( x1, y, xx1, yy);
    pTemp_Box->copy_label( TempString);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    x1 += xx1;

    xx1 = yy + yy / 2;

    pTemp_Button = new Fl_Button( x1, y, xx1, yy);
    if( iColor == 0) {
      pTemp_Button->align( FL_ALIGN_TOP_LEFT);     // align for label
      pTemp_Button->labelsize( 10);
      pTemp_Button->label( LANGDEF_COLOR);
    }
    pTemp_Button->color( fl_rgb_color( r, g, b));
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, YaIPS_Setting_ChooseColorPredef + iColor);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_CustomColors_10a="
                                             "Press to change the color"));

    pColorButton[ iColor] = pTemp_Button;

    x1 += xx1 + 4;

    y += yy + 2;

    if( iColor == 7) {             // A bigger gap to separate the to lines of the custom colors

      y += 6;
    }
  }

  // Color names

  y = ySave;

  xx1 = wWin - x1 - 4;

  pTemp_Group = new Fl_Group( x1, y, xx1, (yy + 2) * YAIPS_CUSTOM_COLORS_N + 6);

  for( iColor = 0; iColor < YAIPS_CUSTOM_COLORS_N; iColor++) {

    pTemp_Input = new Fl_Input( x1, y, xx1, yy);
    if( iColor == 0) {
      pTemp_Input->align( FL_ALIGN_TOP_LEFT);     // align for label
      pTemp_Input->labelsize( 10);
      pTemp_Input->label( LangStringLookup( "&GUI_CustomColors_11=Name"));
    }
    pTemp_Input->tooltip( LangStringLookup( "&GUI_CustomColors_11a=Name for this color"));
    pTemp_Input->maximum_size( sizeof( CustomColorNames[ 0]) - 10);
    pTemp_Input->value( CustomColorNames[ iColor]);
    pTemp_Input->callback( IqeB_GUI_Input_SetValue_Callback, CustomColorNames + iColor);
    pTemp_Input->insert_position( 0);                    // Position to begin of text

    pColorName[ iColor] = pTemp_Input;

    y += yy + 2;

    if( iColor == 7) {             // A bigger gap to separate the to lines of the custom colors

      y += 6;
    }
  }

  pTemp_Group->end();                       // end this group

  //
  // Layout end work
  //

  pMyToolWin->end();

  pMyToolWin->resizable( pTemp_Group);     // nothing in this group is resizable


  if( MYWIN_SIZE_X_MIN != MYWIN_SIZE_X_MAX ||           // If window min/max sizes are different
      MYWIN_SIZE_Y_MIN != MYWIN_SIZE_Y_MAX) {

    // make window resizable
    pMyToolWin->size_range( MYWIN_SIZE_X_MIN, MYWIN_SIZE_Y_MIN, MYWIN_SIZE_X_MAX, MYWIN_SIZE_Y_MAX); // minimum window size
  }

  // finish up

  //x/pBut_Calibrate->take_focus();

  pMyToolWin->set_non_modal();
  pMyToolWin->callback( close_cb, 0);
  pMyToolWin->show();

  // Hack: Remove minimize and maximize buttons from the window caption
  YaIPS_DialogRemoveMinMaxButton( pMyToolWin);

  // Add idle action for this window

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::add_idle( IqeB_GUI_ToolsMyIdleAction);      // Redraw window during idle
#endif
  Fl::add_check( IqeB_GUI_ToolsMyIdleAction);     // Check small image size change

  MyWinUpdate();                                  // Update the GUI
}

/************************* End Of File *************************/
