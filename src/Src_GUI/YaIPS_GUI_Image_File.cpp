/****************************************************************************

  YaIPS_GUI_Image_File.cpp

  Load image from file for further processing.
  Creates an window which shows the loaded image.

  06.02.2025 RR: First edition of this file.

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
#define MY_WIN_ID     YAIPS_WIN_ID_IMG_WIN          // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_IMG_FILES       // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_Image_Title=Image viewer"          // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinImage"               // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_Image_Tool      // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       YAIPS_WIN_SIZE_S1_X_MIN
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
static void Save_cb( Fl_Widget *w, long int iToolData);
static  int DropFile_cb( Fl_Widget *w, void *pFileNameArg, void *pImageDispArg, int SubWinIDx);
static void ShowOnBig_cb( Fl_Widget *w, long int iToolData);
static void CopyBig_cb( Fl_Widget *w, long int iToolData);
static void Delete_cb( Fl_Widget *w, long int iToolData);
static void FilePrev_cb( Fl_Widget *w, long int iToolData);
static void FileNext_cb( Fl_Widget *w, long int iToolData);
static void PasteImg_cb( Fl_Widget *w, long int iToolData);
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

  char LastFileName[ FILENAME_MAX];     // File name of last loaded image file. This is inclusive path and file extension.

  int WriteFileType_Last;               // File type of last saved image file

  // Used for intern data management

  Fl_YaIPS_ImageDisp_t YaIPS_ImageDisp;   // Info image display

  // Handle files in directory of last loaded file

  char LastDirName[ FILENAME_MAX];               // Path of last loaded image file.
  struct dirent **files;                // Pointer to file list
  int num_files;                        // Number of files in file list

} YaIPS_ToolData_info_t;

static int nYaIPS_ToolData_info;       // Number of image files windows open

static YaIPS_ToolData_info_t YaIPS_ToolData_info[ MY_WIN_MAX];

//-----------------------------------------------------------------------------------
// Presets for this tools window
//-----------------------------------------------------------------------------------

static T_GUI_PreferenceEntry MyPreferences[] =

{
  // File is open
  { PREF_T_INT,    "IsOpen",      "0", &YaIPS_ToolData_info[0].IsOpen},

  // Window size

  { PREF_T_INT,    "WinSizeX", "100", &YaIPS_ToolData_info[0].MyWinSizeX }, // NOTE: values will be clipped against MYWIN_SIZE_X_MIN / MYWIN_SIZE_Y_MIN
  { PREF_T_INT,    "WinSizeY", "100", &YaIPS_ToolData_info[0].MyWinSizeY },

  // ...

  { PREF_T_STRING,       "LastFileName",   "", &YaIPS_ToolData_info[0].LastFileName, sizeof( YaIPS_ToolData_info[0].LastFileName) - 1 },
  { PREF_T_INT,    "WriteFileType_Last",  "0", &YaIPS_ToolData_info[0].WriteFileType_Last },
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_ImageFileWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp);

//-----------------------------------------------------------------------------------
// Create a specialized window class for image load and display
//-----------------------------------------------------------------------------------

class CLASS_WIN_TOOL : public Fl_Double_Window {

public:

  int iToolData;                         // Index of info data element, see YaIPS_ToolData_info

  Fl_Button *pGUI_Img_ShowOnBig;         // Show this image on big display
  Fl_Button *pGUI_Img_Load;              // Load an image
  Fl_Button *pGUI_Img_Save;              // Save image
  Fl_Button *pGUI_Img_CopyBig;           // Copy image from big display
  Fl_Button *pGUI_Img_Delete;            // Delete the image
  Fl_Button *pGUI_Img_File_Prev;         // Previous file from directory
  Fl_Button *pGUI_Img_File_Next;         // Next file in directory

  // Create the window

  CLASS_WIN_TOOL( int X, int Y, int W, int H, const char *l, int iToolDataArg) : Fl_Double_Window( X, Y, W, H, l)
  {
    YaIPS_ToolData_info_t *pToolData;
    Fl_Button       *pTemp_Button;

    iToolData = iToolDataArg;                    // Index of info data element, see YaIPS_ToolData_info
    pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

    // Initialize some data

    memset( &pToolData->YaIPS_ImageDisp, 0, sizeof( Fl_YaIPS_ImageDisp_t)); // Zero data

    // ...

    Fl_Group *pGUI_GroupTopSide;                 // Top side of window
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
    pGUI_Img_ShowOnBig->callback( ShowOnBig_cb, (long int)iToolData);
    pGUI_Img_ShowOnBig->tooltip( LANGDEF_SHOW_ON_BIG_IMAGE);
    pGUI_Img_ShowOnBig->labelcolor( YAIPS_BCOL_SHOW_OTHER);

    x1 += xx + 5;

    // Button load image

    pGUI_Img_Load = new Fl_Button( x1, y, xx, yy, "@+3fileopen");
    pGUI_Img_Load->callback( Load_cb, (long int)iToolData);
    pGUI_Img_Load->tooltip( LangStringLookup( "&GUI_Image_Tools2a=Load image from file."));
    pGUI_Img_Load->labelcolor( YAIPS_BCOL_BUTTON);

    x1 += xx + 5;

    // Button copy image from big image

    pGUI_Img_CopyBig = new Fl_Button( x1, y, xx, yy, "@+3import");
    pGUI_Img_CopyBig->callback( CopyBig_cb, (long int)iToolData);
    pGUI_Img_CopyBig->tooltip( LangStringLookup( "&GUI_Image_Tools3a=Copy image from large window."));
    pGUI_Img_CopyBig->labelcolor( YAIPS_BCOL_BUTTON);

    x1 += xx + 5;

    // 4 smaller buttons

    xx = xx0 / 2 - 1;

    // Button save image

    pGUI_Img_Save = new Fl_Button( x1, y, xx, xx, "@-2filesave2");
    pGUI_Img_Save->callback( Save_cb, (long int)iToolData);
    pGUI_Img_Save->tooltip( LangStringLookup( "&GUI_Image_Tools4a=Save image."));
    pGUI_Img_Save->labelcolor( YAIPS_BCOL_BUTTON);

    // Button reset image

    pGUI_Img_Delete = new Fl_Button( x1 + xx + 2, y, xx, xx, "@-23cross");
    pGUI_Img_Delete->callback( Delete_cb, (long int)iToolData);
    pGUI_Img_Delete->tooltip( LangStringLookup( "&GUI_Image_Tools5a=Delete image."));
    pGUI_Img_Delete->labelcolor( YAIPS_BCOL_BUTTON);

    pGUI_Img_File_Prev = new Fl_Button( x1, y + xx + 2, xx, xx, "@-24>");
    pGUI_Img_File_Prev->callback( FilePrev_cb, (long int)iToolData);
    pGUI_Img_File_Prev->tooltip( LangStringLookup( "&GUI_Image_Tools6a=Load previous file."));
    pGUI_Img_File_Prev->labelcolor( YAIPS_BCOL_BUTTON);
    pGUI_Img_File_Prev->shortcut( FL_COMMAND+'p');       // Short cut key

    pGUI_Img_File_Next = new Fl_Button( x1 + xx + 2, y + xx + 2, xx, xx, "@-2>");
    pGUI_Img_File_Next->callback( FileNext_cb, (long int)iToolData);
    pGUI_Img_File_Next->tooltip( LangStringLookup( "&GUI_Image_Tools7a=Load next file."));
    pGUI_Img_File_Next->labelcolor( YAIPS_BCOL_BUTTON);
    pGUI_Img_File_Next->shortcut( FL_COMMAND+'n');       // Short cut key

    // ...

    xx = xx0;
    x1 += xx + 5;

    // Paste image from clipboard button
    // NOTE: we place the button outside the window.
    //       This makes the button invisible.
    //       The shortcut still can be used.

    pTemp_Button = new Fl_Button( x1, y - 100, xx, yy, "Paste");
    pTemp_Button->callback( PasteImg_cb, (long int)iToolData);
    pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
    pTemp_Button->shortcut( FL_COMMAND+'v');       // Short cut key

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

    pToolData->YaIPS_ImageDisp.pImage_Box->pDropCallback = DropFile_cb;    // Accept file drops
#ifdef use_again
    pToolData->YaIPS_ImageDisp.pImage_Box->pPasteImgCallback    = YaIPS_ImageDispPasteToOut_cb; // Common paste image callback
#else
    // Use the YaIPS_GUI_MyChangeOutput() function to display a pasted image.
    // This also saves the pasted image so it is reloaded on next open of this dialog.
    pToolData->YaIPS_ImageDisp.pImage_Box->pPasteImgCallback    = YaIPS_ImageDispPasteToOutFunc_cb; // Common paste image callback
#endif
    pToolData->YaIPS_ImageDisp.pImage_Box->PasteImgCallbackArg1 = &pToolData->YaIPS_ImageDisp;  // Pointer to Fl_YaIPS_ImageDisp_t

    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawBeforeCallback = YaIPS_ImageDispDrawBefore_cb; // Draw before callback
    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawAfterCallback  = YaIPS_ImageDispDrawAfter_cb;  // Draw after callback
    pToolData->YaIPS_ImageDisp.pImage_Box->DrawCallbackArg1    = &pToolData->YaIPS_ImageDisp;  // Pointer to Fl_YaIPS_ImageDisp_t
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
};

/************************************************************************************
 * close_cb, close this window
 *
 * pValueArg is a pointer to the widget. Set this pointer to NULL on deletion.
 */

static void close_cb( Fl_Widget *w, long int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  int DeleteFile;
  char TempFileName[ 256 + 16];

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

  if( pToolData->pMyToolWin == NULL) {  // security test, has main window

    return;
  }

  // Ensure not used clipboard images are deleted

  DeleteFile = false;         // Preset no delete

  // Construct a file name for the clipboard image

  sprintf( TempFileName, "%s/Images/YaIPS/Clipboard-ImageView-%d.png", YaIPS_WorkingDirectory, (int)iToolData + 1);
  IqeB_FileNormalizePathChars( TempFileName);

  // Check for clipboard image

  if( strcmp( pToolData->LastFileName, TempFileName) != 0) {

      DeleteFile = true;        // Ensure clipboard image is deleted
  }

  if( DeleteFile) {             // Check delete of clipboard image

    // If existing, delete the file
    IqeB_FileDelete( TempFileName);
  }

  // ...

  YaIPS_ImageDispReleaseBeforeClose( &pToolData->YaIPS_ImageDisp);

  pToolData->IsOpen = false;           // Flag info data is not in use

  IqeB_GUI_CloseToolWindow( (void **)&pToolData->pMyToolWin);

  // Free data of last loaded directory

  if( pToolData->num_files >= 0 && pToolData->files != NULL) {

    fl_filename_free_list( &pToolData->files, pToolData->num_files);
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
 * Load_cb
 *
 * Handle a load file request from the button
 */

static void Load_cb( Fl_Widget *w, long int iToolData)
{
  Fl_Native_File_Chooser fc;
  YaIPS_ToolData_info_t *pToolData;
  //x/CLASS_WIN_TOOL *pMyToolWin;
  Fl_RGB_Image *pTempImage;
  char FileFilter[ 1024];
  char *pFileName;
  char TempString1[ FILENAME_MAX], TempString2[ FILENAME_MAX];
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

  if( pTempImage != NULL) {           // Got an image

    // Copy to display images
    YaIPS_ImageDispUpdateByNewImage( &pToolData->YaIPS_ImageDisp, pTempImage,
                                     MY_WIN_ID + iToolData, pFileName);   // Load the image to the display

    if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {      // Display this on the big image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pTempImage,
                                       MY_WIN_ID + iToolData, pFileName); // Load the image to the display
    }

    // Remember last loaded file name
    memset( pToolData->LastFileName, 0, sizeof( pToolData->LastFileName));
    strncpy( pToolData->LastFileName, pFileName, sizeof( pToolData->LastFileName) - 1);

    // Remember last used directory
    IqeB_FileGetPath( pFileName, YaIPS_BrowserDirectory, sizeof( YaIPS_BrowserDirectory));

    // Set windows title

    IqeB_FileGetFileName( pFileName, TempString1, sizeof( TempString1));   // Get filename without path

#ifdef use_again
    sprintf( TempString2, "%d %s: %s (%d x %d)", (int)iToolData + 1, MY_WIN_GUI_NAME, TempString1, pTempImage->data_w(), pTempImage->data_h());
    pMyToolWin->copy_label( TempString2);
#else
    sprintf( TempString2, "%s %d x %d", TempString1, pTempImage->data_w(), pTempImage->data_h());
    YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, TempString2);
#endif

    pTempImage->release();                        // Release temporary image
  }

  MyWinUpdate( iToolData, true);                 // Update the GUI
}

/************************************************************************************
 * Save_cb
 *
 * Save image to file
 */

static void Save_cb( Fl_Widget *w, long int iToolData)
{
  Fl_Native_File_Chooser fc;
  YaIPS_ToolData_info_t *pToolData;
  //x/CLASS_WIN_TOOL *pMyToolWin;
  char FileFilter[ 1024];
  char *pFileName;
  char TempFileName[ FILENAME_MAX + 16];
  int ierr, iFileType;
  static char *FileTypes[ YAIPS_IMAGE_FILES_WRITE_TAB_N] = { YAIPS_IMAGE_FILES_WRITE_TAB_DATA };

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  //x/pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  // Initialize the file chooser. Only can save png images

  strcpy( FileFilter, YAIPS_IMAGE_FILES_WRITE_BROWSER);

  fc.filter( FileFilter);
  fc.options( Fl_Native_File_Chooser::SAVEAS_CONFIRM | Fl_Native_File_Chooser::USE_FILTER_EXT);

  iFileType = pToolData->WriteFileType_Last;

  if( iFileType < 0 || iFileType >= YAIPS_IMAGE_FILES_WRITE_TAB_N) {    // Security test out of range

    iFileType = 0;
    pToolData->WriteFileType_Last = 0;
  }

  fc.filter_value( pToolData->WriteFileType_Last);  // What file type to use

  pFileName = NULL;      // NO preset file name

  if( pToolData->YaIPS_ImageDisp.ImageName[ 0] != 0) {

    pFileName = pToolData->YaIPS_ImageDisp.ImageName;

  } else {

    pFileName = LangStringLookup( "&GUI_Image_Save5=Image.png");
  }

  if( pFileName != NULL) {

    // Remove the file extension of the preset file name.
    IqeB_FileEnsureExtension( pFileName, NULL /*FileTypes[ iFileType]*/, TempFileName, sizeof( TempFileName));
    fc.preset_file( TempFileName);
  }

  fc.title( LangStringLookup( "&GUI_Image_Save6=Save image"));
  fc.type( Fl_Native_File_Chooser::BROWSE_SAVE_FILE);  // need this if file doesn't exist yet
  fc.directory( YaIPS_BrowserDirectory);          // Set browser directory
  ierr = fc.show();                                    // Open file chooser dialog

  if( ierr != 0) {      // User cancelled or error

    return;
  }

  // Have a filename here

  // Have a filename here. Ensure a png file extension.

  pFileName = (char *)fc.filename();
  iFileType = fc.filter_value();        // What file to save

  if( iFileType >= 0 && iFileType < YAIPS_IMAGE_FILES_WRITE_TAB_N) {

    IqeB_FileEnsureExtension( pFileName, FileTypes[ iFileType], TempFileName, sizeof( TempFileName));

    pToolData->WriteFileType_Last = iFileType;
  } else {

    IqeB_FileEnsureExtension( pFileName, (char *)"png", TempFileName, sizeof( TempFileName));
    pToolData->WriteFileType_Last = 0;
  }

  pFileName = TempFileName;

  // Remember last used directory
  IqeB_FileGetPath( pFileName, YaIPS_BrowserDirectory, sizeof( YaIPS_BrowserDirectory));

  // Save latched display image
  if( pToolData->YaIPS_ImageDisp.pImage_Img != NULL) {   // Have a latched image

    YaIPS_Image_Write_PNG( pFileName, pToolData->YaIPS_ImageDisp.pImage_Img);
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

static int DropFile_cb( Fl_Widget *w, void *pFileNameArg, void *pImageDispArg, int SubWinIDx)
{
  YaIPS_ToolData_info_t *pToolData;
  CLASS_WIN_TOOL *pMyToolWin;
  Fl_RGB_Image *pTempImage;
  char *pFileName;
  char TempString1[ FILENAME_MAX], TempString2[ FILENAME_MAX];
  int iToolData;

  // Must locate the associated window so search ...

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

    // load and show file

    pTempImage = YaIPS_Image_Read( pFileName);   // Try to load an image

    if( pTempImage != NULL) {           // Got an image

      YaIPS_ImageDispUpdateByNewImage( &pToolData->YaIPS_ImageDisp, pTempImage,
                                      MY_WIN_ID + iToolData, pFileName);   // Load the image to the display

      if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {   // Display this on the big image

        YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pTempImage,
                                        MY_WIN_ID + iToolData, pFileName);   // Load the image to the display
      }

      // Remember last loaded file name
      memset( pToolData->LastFileName, 0, sizeof( pToolData->LastFileName));
      strncpy( pToolData->LastFileName, pFileName, sizeof( pToolData->LastFileName) - 1);

      // Set windows title

      IqeB_FileGetFileName( pFileName, TempString1, sizeof( TempString1));   // Get filename without path

#ifdef use_again
      sprintf( TempString2, "%d %s: %s (%d x %d)", iToolData + 1, MY_WIN_GUI_NAME, TempString1, pTempImage->data_w(), pTempImage->data_h());
      pMyToolWin->copy_label( TempString2);
#else
      sprintf( TempString2, "%s %d x %d", TempString1, pTempImage->data_w(), pTempImage->data_h());
      YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, TempString2);
#endif
    }

    // Done, can exit here

    MyWinUpdate( iToolData, true);                 // Update the GUI

    return( 0);    // OK Processed drop
  }

//x/ErrorExit:

  return( -1);   // Error on processing drop
}

/************************************************************************************
 * ShowOnBig_cb
 *
 * Show loaded image to the big display
 */

static void ShowOnBig_cb( Fl_Widget *w, long int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  //x/CLASS_WIN_TOOL *pMyToolWin;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  //x/pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  if( pToolData->YaIPS_ImageDisp.pImage_Img != NULL) {           // Got an image

    YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                     MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName);   // Load the image to the display
  }
}

/************************************************************************************
 * CopyBig_cb
 *
 * Copy image from big display
 */

static void CopyBig_cb( Fl_Widget *w, long int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  //x/CLASS_WIN_TOOL *pMyToolWin;
  Fl_RGB_Image *pTempImage;
  char TempString2[ FILENAME_MAX];

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  //x/pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  if( YaIPS_BigImageDisp.pImage_Img == NULL) {     // Have no big image

    return;                                       // Need this !
  }

  pTempImage = YaIPS_BigImageDisp.pImage_Img;      // The last displayed big image

  if( pTempImage != NULL) {                       // Got an image

    YaIPS_ImageDispUpdateByNewImage( &pToolData->YaIPS_ImageDisp, pTempImage,
                                     MY_WIN_ID + iToolData, YaIPS_BigImageDisp.FileName);   // Load the image to the display

    // Set windows title

#ifdef use_again
    sprintf( TempString2, "%d %s: %s (%d x %d)", (int)iToolData + 1, MY_WIN_GUI_NAME, pToolData->YaIPS_ImageDisp.ImageName, pTempImage->data_w(), pTempImage->data_h());
    pMyToolWin->copy_label( TempString2);
#else
    sprintf( TempString2, "%s %d x %d", pToolData->YaIPS_ImageDisp.ImageName, pTempImage->data_w(), pTempImage->data_h());
    YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, TempString2);
#endif

    // Reset last loaded file name
    pToolData->LastFileName[ 0] = '\0';
  }
}

/************************************************************************************
 * Delete_cb
 *
 * Delete the image
 */

static void Delete_cb( Fl_Widget *w, long int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  //x/CLASS_WIN_TOOL *pMyToolWin;
  char TempString2[ FILENAME_MAX];

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  //x/pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  // Empty a display image
  YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);

  pToolData->YaIPS_ImageDisp.MyWinID = MY_WIN_ID + iToolData;  // Remember my image ID. Is needed for paste image.

  // Set windows title

#ifdef use_again
  sprintf( TempString2, "%d %s: ", (int)iToolData + 1, MY_WIN_GUI_NAME);
  pMyToolWin->copy_label( TempString2);
#else
  strcpy( TempString2, "");
  YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, TempString2);
#endif

  // Reset last loaded file name
  pToolData->LastFileName[ 0] = '\0';
}

/************************************************************************************
 * FilePrev_cb
 *
 * Previous file in directory
 */

static void FilePrev_cb( Fl_Widget *w, long int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  //x/CLASS_WIN_TOOL *pMyToolWin;
  char TempString1[ FILENAME_MAX], TempString2[ FILENAME_MAX];
  int i;
  Fl_RGB_Image *pTempImage;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  //x/pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  if( pToolData->LastFileName[ 0] == '\0') {    // Security test, no file name

    return;
  }

  if( pToolData->num_files <= 1) {              // No or only one file in the file list

    return;
  }

  // Have two or more files in the file list
  // Search for next file.

  IqeB_FileGetFileName( pToolData->LastFileName, TempString1, sizeof( TempString1));   // Get filename without path

  for( i = 0; i < pToolData->num_files; i++) {

    if( stricmp( TempString1, pToolData->files[ i]->d_name) == 0) {  // Got it

      i -= 1;                             // Previous file
      if( i < 0) {                        // Handle wrap around

        i = pToolData->num_files - 1;
      }

      // Construct new file name
      strcpy( TempString1, pToolData->LastDirName);
      strcat( TempString1, pToolData->files[ i]->d_name);

      // load and show file

      pTempImage = YaIPS_Image_Read( TempString1);   // Try to load an image

      if( pTempImage != NULL) {           // Got an image

        // Copy to display images
        YaIPS_ImageDispUpdateByNewImage( &pToolData->YaIPS_ImageDisp, pTempImage,
                                         MY_WIN_ID + iToolData, TempString1);   // Load the image to the display

        if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {      // Display this on the big image

          YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pTempImage,
                                           MY_WIN_ID + iToolData, TempString1); // Load the image to the display
        }

        // Remember last loaded file name
        memset( pToolData->LastFileName, 0, sizeof( pToolData->LastFileName));
        strncpy( pToolData->LastFileName, TempString1, sizeof( pToolData->LastFileName) - 1);

        // Remember last used directory
        IqeB_FileGetPath( TempString1, YaIPS_BrowserDirectory, sizeof( YaIPS_BrowserDirectory));

        // Set windows title

        IqeB_FileGetFileName( pToolData->LastFileName, TempString1, sizeof( TempString1));   // Get filename without path

#ifdef use_again
        sprintf( TempString2, "%d %s: %s (%d x %d)", (int)iToolData + 1, MY_WIN_GUI_NAME, TempString1, pTempImage->data_w(), pTempImage->data_h());
        pMyToolWin->copy_label( TempString2);
#else
        sprintf( TempString2, "%s %d x %d", TempString1, pTempImage->data_w(), pTempImage->data_h());
        YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, TempString2);
#endif

        pTempImage->release();                        // Release temporary image

        MyWinUpdate( iToolData, true);                 // Update the GUI
      }
    }
  }

}

/************************************************************************************
 * FileNext_cb
 *
 * Next file in directory
 */

static void FileNext_cb( Fl_Widget *w, long int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  //x/CLASS_WIN_TOOL *pMyToolWin;
  char TempString1[ FILENAME_MAX], TempString2[ FILENAME_MAX];
  int i;
  Fl_RGB_Image *pTempImage;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  //x/pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  if( pToolData->LastFileName[ 0] == '\0') {    // Security test, no file name

    return;
  }

  if( pToolData->num_files <= 1) {              // No or only one file in the file list

    return;
  }

  // Have two or more files in the file list
  // Search for next file.

  IqeB_FileGetFileName( pToolData->LastFileName, TempString1, sizeof( TempString1));   // Get filename without path

  for( i = 0; i < pToolData->num_files; i++) {

    if( stricmp( TempString1, pToolData->files[ i]->d_name) == 0) {  // Got it

      i += 1;                             // Next file
      if( i >= pToolData->num_files) {    // Handle wrap around

        i = 0;
      }

      // Construct new file name
      strcpy( TempString1, pToolData->LastDirName);
      strcat( TempString1, pToolData->files[ i]->d_name);

      // load and show file

      pTempImage = YaIPS_Image_Read( TempString1);   // Try to load an image

      if( pTempImage != NULL) {           // Got an image

        // Copy to display images
        YaIPS_ImageDispUpdateByNewImage( &pToolData->YaIPS_ImageDisp, pTempImage,
                                         MY_WIN_ID + iToolData, TempString1);   // Load the image to the display

        if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {      // Display this on the big image

          YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pTempImage,
                                           MY_WIN_ID + iToolData, TempString1); // Load the image to the display
        }

        // Remember last loaded file name
        memset( pToolData->LastFileName, 0, sizeof( pToolData->LastFileName));
        strncpy( pToolData->LastFileName, TempString1, sizeof( pToolData->LastFileName) - 1);

        // Remember last used directory
        IqeB_FileGetPath( TempString1, YaIPS_BrowserDirectory, sizeof( YaIPS_BrowserDirectory));

        // Set windows title

        IqeB_FileGetFileName( pToolData->LastFileName, TempString1, sizeof( TempString1));   // Get filename without path

#ifdef use_again
        sprintf( TempString2, "%d %s: %s (%d x %d)", (int)iToolData + 1, MY_WIN_GUI_NAME, TempString1, pTempImage->data_w(), pTempImage->data_h());
        pMyToolWin->copy_label( TempString2);
#else
        sprintf( TempString2, "%s %d x %d", TempString1, pTempImage->data_w(), pTempImage->data_h());
        YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, TempString2);
#endif

        pTempImage->release();                        // Release temporary image

        MyWinUpdate( iToolData, true);                 // Update the GUI
      }
    }
  }

}

/************************************************************************************
 * PasteImg_cb
 *
 * Control + V was pressed on keyboard. Check clip board.
 */

static void PasteImg_cb( Fl_Widget *w, long int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  //x/CLASS_WIN_TOOL *pMyToolWin;

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  //x/pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  if( Fl::clipboard_contains(Fl::clipboard_image) == 0) {  // NO image in the clipboard

    return;
  }

  if( pToolData->YaIPS_ImageDisp.pImage_Box == NULL) {     // Security test

    return;
  }

  Fl::paste( *pToolData->YaIPS_ImageDisp.pImage_Box, 1, Fl::clipboard_image); // try to find image in the clipboard

#ifdef use_again  // NOT needed if pPasteImgCallback = YaIPS_ImageDispPasteToOutFunc_cb
  // Set windows title

  char TempString2[ FILENAME_MAX];

#ifdef use_again
  sprintf( TempString2, "%d %s: %s", (int)iToolData + 1, MY_WIN_GUI_NAME, LANGDEF_CLIPBOARD);
  pMyToolWin->copy_label( TempString2);
#else
  sprintf( TempString2, "%s", LANGDEF_CLIPBOARD);
  YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, TempString2);
#endif
#endif

  return;
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

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  if( pToolData->IsOpen == false) {      // Security test, must be open

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

  if( DoEnable == 2) {                                          // Was periodically updates only

    return;
  }

  // ...

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_ShowOnBig,
                             DoEnable &&                            // Enable GUI elements
                             pToolData->YaIPS_ImageDisp.pImage_Img != NULL); // and have an image loaded

  // ...

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_Load,
                             DoEnable);                             // Enable GUI elements

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_CopyBig,
                             DoEnable &&
                             YaIPS_BigImageDisp.pImage_Img != NULL); // Enable GUI elements

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_Save,
                             DoEnable &&
                             pToolData->YaIPS_ImageDisp.pImage_Img != NULL); // and have an image loaded

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_Delete,
                             DoEnable &&
                             pToolData->YaIPS_ImageDisp.pImage_Img != NULL); // and have an image loaded

  // Keep last loaded directory up to date.
  // Check for change of directory.

  if( pToolData->LastFileName[ 0] != '\0') {            // Have a file name set

    char ThisDirName[ FILENAME_MAX];                    // Path of last loaded image file.

    // Get path name out of last loaded file
    IqeB_FileGetPath( pToolData->LastFileName, ThisDirName, sizeof( ThisDirName));

    if( strcmp( ThisDirName, pToolData->LastDirName) != 0) {  // Is different than last file name

      // Has to change last loaded directory

      // Free data of last loaded directory

      if( pToolData->num_files >= 0 && pToolData->files != NULL) {

        fl_filename_free_list( &pToolData->files, pToolData->num_files);
      }

      pToolData->LastDirName[ 0] = '\0';        // Reset last loaded directory
      pToolData->files = NULL;
      pToolData->num_files = 0;

      strcpy( pToolData->LastDirName, ThisDirName);

      YaIPS_Image_GetFilesInDir( ThisDirName, &pToolData->files, &pToolData->num_files);
    }
  }

  // ...

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_File_Prev,
                             DoEnable &&
                             pToolData->YaIPS_ImageDisp.pImage_Img != NULL && // and have an image loaded
                             pToolData->num_files > 1);                       // and have more then one file in the directory list

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_File_Next,
                             DoEnable &&
                             pToolData->YaIPS_ImageDisp.pImage_Img != NULL && // and have an image loaded
                             pToolData->num_files > 1);                       // and have more then one file in the directory list
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

    // Check an image display for size change and redisplay if size has changed.
    YaIPS_ImageDispDrawUpdate( &pToolData->YaIPS_ImageDisp, false);

    MyWinUpdate( iToolData, true);               // Update the GUI

  }  // end: for( iToolData ...

}

/************************************************************************************
 * YaIPS_GUI_MyChangeOutput
 *
 * Called from outside this module.
 * Replace output image by an other image.
 *
 * SubWinIDx:        What sub window to use
 * pNewimage:        Replace output image with this window
 */

static void YaIPS_GUI_MyChangeOutput( int iToolData,                           // What sub window to use
                                      Fl_RGB_Image *pNewimage)                 // Replace output image with this window
{
  YaIPS_ToolData_info_t *pToolData;
  char TempFileName[ FILENAME_MAX + 16];
  char TempString1[ FILENAME_MAX], TempString2[ FILENAME_MAX];

  if( pNewimage == NULL) {                                   // Security test

    return;
  }

  pToolData = YaIPS_ToolData_info + iToolData;               // Point to info data

  if( pToolData->YaIPS_ImageDisp.pImage_Box == NULL) {       // Security test

    return;
  }

  // Construct a file name for the image

  sprintf( TempFileName, "%s/Images/YaIPS/Clipboard-ImageView-%d.png", YaIPS_WorkingDirectory, iToolData + 1);

  IqeB_FileNormalizePathChars( TempFileName);

  // Load the image to the display
  YaIPS_ImageDispUpdateByNewImage( &pToolData->YaIPS_ImageDisp, pNewimage,
                                   MY_WIN_ID + iToolData, TempFileName, true, true);

  if( pToolData->YaIPS_ImageDisp.pImage_Img != NULL) {   // Have a latched image

    if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {   // and display this on the big image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                       MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName);   // Load the image to the display
    }

    // Set windows title

    IqeB_FileGetFileName( TempFileName, TempString1, sizeof( TempString1));   // Get filename without path

    sprintf( TempString2, "%s %d x %d", TempString1, pNewimage->data_w(), pNewimage->data_h());
    YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, TempString2);

    // Save image so it is reloaded at next program start

    // Remember last loaded file name
    memset( pToolData->LastFileName, 0, sizeof( pToolData->LastFileName));
    strncpy( pToolData->LastFileName, TempFileName, sizeof( pToolData->LastFileName) - 1);

    YaIPS_Image_Write_PNG( TempFileName, pToolData->YaIPS_ImageDisp.pImage_Img);
  }
}

/************************************************************************************
 * IqeB_GUI_ImageFileWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_ImageFileWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
{
  YaIPS_ToolData_info_t *pToolData;
  CLASS_WIN_TOOL *pMyToolWin;
  int xPos, yPos, nTabelOnEntry;
  char TempString[ FILENAME_MAX];

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

  pToolData->LastDirName[ 0] = '\0';  // Reset last loaded directory
  pToolData->files = NULL;
  pToolData->num_files = 0;

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

  if( pToolData->pMyToolWin == NULL) {  // security test

    // Window creation has failed

    pToolData->IsOpen = false;           // Flag info data is not in use

    if( iToolData == nYaIPS_ToolData_info - 1) {  // Was the last table element

      nYaIPS_ToolData_info -= 1;         // Decrease table size by one
    }

    return;
  }

  // Set window title

  pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  sprintf( TempString, "%d %s", iToolData + 1, MY_WIN_GUI_NAME);
  pMyToolWin->copy_label( TempString);

  pToolData->YaIPS_ImageDisp.ColInfoTextSize = 12;   // Size of text field

  // Info string: White text and no background
#ifdef use_again
  YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, 0xffffffff, FL_WHITE);
#endif

  // ...

  if( nTabelOnEntry == 0) {                         // Table was empty before

    // Add idle action for this window

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
    Fl::add_idle( IqeB_GUI_ToolsMyIdleAction);      // Redraw window during idle
#endif
    Fl::add_check( IqeB_GUI_ToolsMyIdleAction);     // Check small image size change
  }

  // Restore last loaded image

  if( pToolData->LastFileName[ 0] != '\0') {

    Fl_RGB_Image *pTempImage;
    char *pFileName;
    char TempString1[ FILENAME_MAX], TempString2[ FILENAME_MAX];

    pFileName  = pToolData->LastFileName;

    // load and show file

    pTempImage = YaIPS_Image_Read( pFileName);   // Try to load an image

    if( pTempImage != NULL) {           // Got an image

      // Copy to display images
      YaIPS_ImageDispUpdateByNewImage( &pToolData->YaIPS_ImageDisp, pTempImage,
                                      MY_WIN_ID + iToolData, pFileName);   // Load the image to the display

      if( YaIPS_GUI_Main_Do_Startup &&                                        // Startup phase of tool windows
          YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + iToolData) {        // and this tool window is selected

        // Show also on big image
        YaIPS_ImageDispUpdateByNewImage(          &YaIPS_BigImageDisp, pTempImage,
                                          MY_WIN_ID + iToolData, pFileName);   // Load the image to the display
      }

      // Set windows title

      // Get filename without path
      IqeB_FileGetFileName( pFileName, TempString1, sizeof( TempString1));

      sprintf( TempString2, "%s %d x %d", TempString1, pTempImage->data_w(), pTempImage->data_h());
      YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, TempString2);

      pTempImage->release();                        // Release temporary image

      //x/MyWinUpdate( iToolData, true);                 // Update the GUI
    }
  }
}

/************************************************************************************
 * IqeB_GUI_ImageFileWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_ImageFileWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

    // Register change output function for big image display
    YaIPS_ToolChangeOutputSet( MY_WIN_ID + SubWinIDx, YaIPS_GUI_MyChangeOutput);

	  IqeB_GUI_ImageFileWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  // Register change output function for big image display
  YaIPS_ToolChangeOutputSet( MY_WIN_ID + iUnused, YaIPS_GUI_MyChangeOutput);

  IqeB_GUI_ImageFileWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


