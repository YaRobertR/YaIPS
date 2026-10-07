/****************************************************************************

  YaIPS_GUI_InspCompare.cpp

  Compare image parts.

  15.09.2025 RR: First edition of this file.

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
#define MY_WIN_ID     YAIPS_WIN_ID_INSP_COMPARE         // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_INSP_COMPARE        // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_InspCompare_Title=Compare"                // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinInspCompare"            // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_ClassInspCompare_Tool   // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       262
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
static void YaIPS_GUI_MyDrawAfter_cb( Fl_Widget *pW, void *pArg1, void *pArg2);
static int YaIPS_GUI_MyMouse_cb( Fl_Widget *pW, int event, void *pArg1, void *pArg2);

/************************************************************************************
* Global variables for this window
*/

//-----------------------------------------------------------------------------------
// Manage multiple tool windows
//-----------------------------------------------------------------------------------

typedef struct {

  // Parameters
  int iToolData;                        // Sub window number
  int IsOpen;                           // True if this window is open.
  void *pMyToolWin;                     // Pointer to window data ( is pointer to CLASS_WIN_TOOL)

  int MyWinPosX, MyWinPosY;             // last window position
  int MyWinSizeX, MyWinSizeY;           // last window size

  int Input1_WinIdNr;                   // Window ID nr of 1. input
  int Input2_WinIdNr;                   // Window ID nr of 2. input

  // Used for intern data management

  Fl_YaIPS_ImageDisp_t YaIPS_ImageDisp;   // Info image output

  // Used to catch a change of the input image
  int Input1_Change;                    // Last processed 'ImageChanged' from input image
  int Input2_Change;                    // Last processed 'ImageChanged' from 2. input image

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int   Teach_mode;                     // 0 = inspection mode, 1 = teach mode

  YaIPS_CompareAOI_t Cmp;               // Compare data

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
  { PREF_T_INT,    "Input2_WinIdNr", "-1", &YaIPS_ToolData_info[0].Input2_WinIdNr },

  //
  // Parameter Dialog
  //

  { PREF_T_INT,    "MyParPosX",  IQE_GUI_NO_WINPOS_X_STRING, &YaIPS_ToolData_info[0].MyParPosX }, // NOTE: values will be clipped against MYWIN_SIZE_X_MIN / MYWIN_SIZE_Y_MIN
  { PREF_T_INT,    "MyParPosY",  IQE_GUI_NO_WINPOS_Y_STRING, &YaIPS_ToolData_info[0].MyParPosY },

  // GUI Parameter
  { PREF_T_INT,        "Teach_mode",    "0", &YaIPS_ToolData_info[0].Teach_mode},

  { PREF_T_INT,             "AOI_X",    "0", &YaIPS_ToolData_info[0].Cmp.AOI.XPos},
  { PREF_T_INT,             "AOI_Y",    "0", &YaIPS_ToolData_info[0].Cmp.AOI.YPos},
  { PREF_T_INT,            "AOI_XX",  "100", &YaIPS_ToolData_info[0].Cmp.AOI.XSize},
  { PREF_T_INT,            "AOI_YY",  "100", &YaIPS_ToolData_info[0].Cmp.AOI.YSize},
  { PREF_T_INT,     "GWToleranceHi",   "25", &YaIPS_ToolData_info[0].Cmp.GWToleranceHi},
  { PREF_T_INT,     "GWToleranceLo",   "25", &YaIPS_ToolData_info[0].Cmp.GWToleranceLo},
  { PREF_T_FLOAT,     "MinBlobArea",  "1.0", &YaIPS_ToolData_info[0].Cmp.MinBlobArea},
  { PREF_T_FLOAT,    "MinLBlobArea",  "5.0", &YaIPS_ToolData_info[0].Cmp.MinLBlobArea},
  { PREF_T_FLOAT,     "SumBlobArea", "10.0", &YaIPS_ToolData_info[0].Cmp.SumBlobArea},
  { PREF_T_INT,        "FlagDarken",    "1", &YaIPS_ToolData_info[0].Cmp.FlagDarken},
  { PREF_T_INT,  "FlagShowBlobArea",    "0", &YaIPS_ToolData_info[0].Cmp.FlagShowBlobArea},
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_InspCompareWin, (Fl_Callback *)close_cb,
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

static IqeFl_Int_Input *pAOI_X, *pAOI_Y, *pAOI_XX, *pAOI_YY;
static Fl_Button *pTeachToggle;

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{
  Fl_RGB_Image *pImgIn1;
  int ImgXX, ImgYY, RedrawOnExit;
  Fl_Widget *pCurrFocus;

  RedrawOnExit = false;

  pCurrFocus = Fl::focus();

  // Enable AOI pos/size input buttons
  IqeB_GUI_WidgetActivate( pAOI_X, pToolData->Teach_mode);
  IqeB_GUI_WidgetActivate( pAOI_Y, pToolData->Teach_mode);
  IqeB_GUI_WidgetActivate( pAOI_XX, pToolData->Teach_mode);
  IqeB_GUI_WidgetActivate( pAOI_YY, pToolData->Teach_mode);

  // Color teach toggle button

  IqeB_GUI_WidgetLabelColor( pTeachToggle, pToolData->Teach_mode ? FL_GREEN : YAIPS_BCOL_BUTTON);

  // Check Scene input

  pImgIn1 = NULL;       // Will be set if there is a valid scene image

  YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Input1_WinIdNr,
                           NULL, &pImgIn1, NULL);

  if( pImgIn1 != NULL) {

    ImgXX = pImgIn1->w();
    ImgYY = pImgIn1->h();

  } else {

    ImgXX = 1024;
    ImgYY = 1024;
  }

  // Check AOI

  if( pCurrFocus != pAOI_X  && pCurrFocus != pAOI_Y &&               // Input element has NO keyboard focus ?
      pCurrFocus != pAOI_XX && pCurrFocus != pAOI_YY) {

    if( YaIPS_ImageDispAoiRectIGuiUpdate( &pToolData->Cmp.AOI, ImgXX, ImgYY,
                                          pAOI_X, pAOI_Y, pAOI_XX, pAOI_YY) > 0) {

      RedrawOnExit = true;                                 // Redraw on exit
    }
  }

  // Redraw

  if( RedrawOnExit) {                                           // Redraw on exit

    pToolData->Input1_Change = 0;                // Reset image change check
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

  // ...

  pToolData->Input1_Change = 0;                    // Force recalculation output
  pToolData->Input2_Change = 0;                    // Force recalculation output
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

    if( WasClipped) {                      // Value was clipped

      pThis->SetValue( Value);
    }
  }

  *pValue = Value;         // update the variable

  // ...

  pToolData->Input1_Change = 0;                    // Force recalculation output
  pToolData->Input2_Change = 0;                    // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_Misc_SetValue_Callback
 *
 * This is usable for Fl_Valuator, Fl_Choice, Fl_Check_Button
 */

static void IqeB_GUI_Misc_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  int *pValue;

  pValue = (int *)pValueArg;             // get pointer to associated variable

  if( w == NULL ||                       // security test
      pValue == NULL) {

    return;
  }

  if( w == pTeachToggle) {                   // Toggle Teach / Inspection button

    pToolData->Teach_mode = ! pToolData->Teach_mode;

  } else {

    Fl_Button *pThis;

    pThis  = (Fl_Check_Button *)w;
    *pValue = pThis->value();                // update the variable
  }

  pToolData->Input1_Change = 0;          // Force recalculation output
  pToolData->Input2_Change = 0;                    // Force recalculation output
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

  pMyParWin = new Fl_Window( xPos, yPos, 355 /*IQE_GUI_TOOLS_STD_WITDH*/, 147, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  //
  //  GUI things
  //

  int x1, y, yy, xx1, xx2, ySave;
  //x/int xx1, xc;
  char TempBuffer[ 256];

  Fl_Check_Button *pCheckTemp;
  Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  IqeFl_Float_Input  *pFloatTemp;
  Fl_Group        *pTemp_Group;
  Fl_Button       *pTemp_Button;
  //x/Fl_Choice       *pTemp_Choice;
  //x/Fl_Radio_Round_Button *pRadioButTemp;

  yy  = 20;

  y = 4;

  // Toggle teach/inspection mode

  x1 = pMyParWin->w() - 28 - 4;

  pTemp_Button = new Fl_Button( x1, y, 28, 28, "@+1pencil");
  pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Teach_mode);
  pTemp_Button->tooltip( LANGDEF_SWITCH_TEACH_INSPECT);
  pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
  pTemp_Button->shortcut( FL_COMMAND+'t');       // Short cut key
  pTeachToggle = pTemp_Button;

  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 28 - 8, yy + 8);
  pTemp_Group->box( FL_UP_BOX);

    // Next line

    y += 4;

    x1  = 4;
    xx2 = 46;                                              // With of position/size input element

    pTemp_Box = new Fl_Box( x1, y, 60, yy, LangStringLookup( "&GUI_InspCompare_TabAa=AOI"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += 60;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_X);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_InspCompare_TabA2a=Position left edge AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Cmp.AOI.XPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Cmp.AOI.XPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pAOI_X = pTemp_Int;

    x1 += xx2;
    x1 += 18;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_Y);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_InspCompare_TabA3a=Position upper edge AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Cmp.AOI.YPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Cmp.AOI.YPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pAOI_Y = pTemp_Int;

    x1 += xx2;
    x1 += 28;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_InspCompare_TabA4=W"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_InspCompare_TabA4a=Width of the AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Cmp.AOI.XSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Cmp.AOI.XSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pAOI_XX = pTemp_Int;

    x1 += xx2;
    x1 += 18;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_InspCompare_TabA6=H"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_InspCompare_TabA5a=Height of the AOI"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->Cmp.AOI.YSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Cmp.AOI.YSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pAOI_YY = pTemp_Int;

    // Next line

    y += yy;

  // Finish things for this group

  pTemp_Group->end();

  x1  = 4;
  y += 8;

  // Tolerance group

  xx1 = 153;     // With of legend
  xx2 = 54;      // With of input element

  x1  = 4;

  ySave = y;    // Remember y position

  pTemp_Group = new Fl_Group( x1, y, xx1 + xx2 + 41, 5 * yy + 8);
  pTemp_Group->box( FL_UP_BOX);

    // Next line

    y += 4;
    x1 = 4;

    pTemp_Box = new Fl_Box( x1, y, xx1, yy, LangStringLookup( "&GUI_InspCompare_GroupB1=Dark error tolerance"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += xx1;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_GRAY_VALUE_SHORT);
    pTemp_Int->align( FL_ALIGN_RIGHT);     // align for label
    pTemp_Int->tooltip( LangStringLookup( "&GUI_InspCompare_GroupB1a="
                         "Image pixels with a brightness deviation larger\n"
                         "than this dark tolerance are marked as error pixels."));
    pTemp_Int->SetValue( pToolData->Cmp.GWToleranceHi);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Cmp.GWToleranceHi);
    pTemp_Int->SetModifyData( 0, 255, 10, 1);

    // Next line

    y += yy;
    x1 = 4;

    pTemp_Box = new Fl_Box( x1, y, xx1, yy, LangStringLookup( "&GUI_InspCompare_GroupB2=Bright error tolerance"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += xx1;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_GRAY_VALUE_SHORT);
    pTemp_Int->align( FL_ALIGN_RIGHT);     // align for label
    pTemp_Int->tooltip( LangStringLookup( "&GUI_InspCompare_GroupB2a="
                         "Image pixels with a brightness deviation larger\n"
                         "than this bright tolerance are marked as error pixels."));
    pTemp_Int->SetValue( pToolData->Cmp.GWToleranceLo);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Cmp.GWToleranceLo);
    pTemp_Int->SetModifyData( 0, 255, 10, 1);

    // Next line

    y += yy;
    x1 = 4;

    pTemp_Box = new Fl_Box( x1, y, xx1, yy, LangStringLookup( "&GUI_InspCompare_GroupB3=Error area small blobs"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += xx1;

    sprintf( TempBuffer, "%s²", pYaIPS_Calib_Unit2String());

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy);
    pFloatTemp->copy_label( TempBuffer);
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_InspCompare_GroupB3a="
                         "Area threshold for small blobs.\n"
                         "Blobs with an area bigger than this threshold. Blobs\n"
                         "below this (colored blue) are discarded.\n"
                         "An area higher than this and smaller than\n"
                         "'Error area big blobs' is a small blob (colored green).\n"));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->Cmp.MinBlobArea);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->Cmp.MinBlobArea);
    pFloatTemp->SetModifyData( 0.0, 100.0, 1.0, 0.1);

    // Next line

    y += yy;
    x1 = 4;

    pTemp_Box = new Fl_Box( x1, y, xx1, yy, LangStringLookup( "&GUI_InspCompare_GroupB4=Error area big blobs"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += xx1;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy);
    pFloatTemp->copy_label( TempBuffer);
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_InspCompare_GroupB4a="
                         "Area threshold for large blobs (colored red).\n"
                         "Area must be larger than this.\n"
                         "A bad inspection result occurs when\n"
                         "there are one or more large blobs."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->Cmp.MinLBlobArea);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->Cmp.MinLBlobArea);
    pFloatTemp->SetModifyData( 0.0, 100.0, 1.0, 0.1);

    // Next line

    y += yy;
    x1 = 4;

    pTemp_Box = new Fl_Box( x1, y, xx1, yy, LangStringLookup( "&GUI_InspCompare_GroupB5=Sum area small blobs"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);     // align for label

    x1 += xx1;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy);
    pFloatTemp->copy_label( TempBuffer);
    pFloatTemp->align( FL_ALIGN_RIGHT);     // align for label
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_InspCompare_GroupB5a="
                                 "A bad inspection result occurs when the area\n"
                                 "sum area of all small blobs this threshold."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->Cmp.SumBlobArea);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->Cmp.SumBlobArea);
    pFloatTemp->SetModifyData( 0.0, 100.0, 1.0, 0.1);


  // Finish things for this group

  pTemp_Group->end();

  // Checkbox group

  xx1 = 160;     // With of legend
  xx2 = 54;      // With of input element

  x1  = 4;

  y = ySave;    // Restore y position

  x1 = pTemp_Group->x() + pTemp_Group->w() + 4;   // Position near next group

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, 2 * yy + 8);
  pTemp_Group->box( FL_UP_BOX);

    // Next line

    x1 += 4;                                        // Left side for check buttons
    xx2 = pMyParWin->w() - x1 - 8;

    y += 4;

    pCheckTemp = new Fl_Check_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_InspCompare_GroupC1=Darken"));
    pCheckTemp->tooltip( LangStringLookup( "&GUI_InspCompare_GroupC1a="
                         "If set, darken the image.\n"
                         "This makes error markings more visible."));
    pCheckTemp->value( pToolData->Cmp.FlagDarken);
    pCheckTemp->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Cmp.FlagDarken);

    // Next line

    y += yy;

    pCheckTemp = new Fl_Check_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_InspCompare_GroupC2=Area"));
    pCheckTemp->tooltip( LangStringLookup( "&GUI_InspCompare_GroupC2a="
                         "If set, show area\n"
                         "of small and large blobs."));
    pCheckTemp->value( pToolData->Cmp.FlagShowBlobArea);
    pCheckTemp->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Cmp.FlagShowBlobArea);

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

  #ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows_
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
  Fl_Box *pBox_Input1;                   // Text for selected 1. input
  Fl_Button *pBut_Input1;                // Select 1. input
  Fl_Box *pBox_Input2;                   // Text for selected 2. input
  Fl_Button *pBut_Input2;                // Select 2. input
  Fl_Button *pGUI_Parameter;             // Open the parameter dialog
  Fl_Button *pGUI_TeachToggle;           // Toggle Teach / Inspection

  // Create the window

  CLASS_WIN_TOOL( int X, int Y, int W, int H, const char *l, int iToolDataArg) : Fl_Double_Window( X, Y, W, H, l)
  {
    YaIPS_ToolData_info_t *pToolData;
    Fl_Button *pTemp_Button;
    Fl_Box    *pTemp_Box;

    iToolData = iToolDataArg;                    // Index of info data element, see YaIPS_ToolData_info
    pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

    // Initialize some data

    memset( &pToolData->YaIPS_ImageDisp, 0, sizeof( Fl_YaIPS_ImageDisp_t)); // Zero data

    pToolData->Input1_Change = 0;                // Reset image change check
    pToolData->Input2_Change = 0;

    // ...

    Fl_Group *pGUI_GroupTopSide;                 // Top side of window
    int x, x1, y, xx, xx0, yy, yy2, hWin, wWin;

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

    yy2 = yy / 2 + 2;            // Height for input element

    // Legend of image inputs

    xx = 16;

    // 1. Input element
    pTemp_Box = new Fl_Box( x1, y + yy - yy2 - yy2 + 1, xx, yy2, LANGDEF_IMGSEL_PDS_INPUT);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label

    // 2. Input element
    pTemp_Box = new Fl_Box( x1, y + yy - yy2, xx, yy2, LANGDEF_IMGSEL_PDS_REF);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label

    x1 += xx;

    // Selected image input

    xx = yy * 2 + yy / 2;

    // 1. Input element
    pBox_Input1 = new Fl_Box( x1, y + yy - yy2 - yy2 + 1, xx, yy2);
    pBox_Input1->box( FL_BORDER_BOX);
    pBox_Input1->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pBox_Input1->labelsize( 18);
    pBox_Input1->copy_label( "---");

    // 2. Input element
    pBox_Input2 = new Fl_Box( x1, y + yy - yy2, xx, yy2);
    pBox_Input2->box( FL_BORDER_BOX);
    pBox_Input2->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pBox_Input2->labelsize( 18);
    pBox_Input2->copy_label( "---");

    x1 += xx - 1;

    xx = yy - 10;

    // Pull down buttons

    // 1. Input element
    pBut_Input1 = new Fl_Button( x1, y + yy - yy2 - yy2 + 1, xx, yy2, "@2>");
    pBut_Input1->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pBut_Input1->tooltip( LANGDEF_INSP_WIN_INP);
    pBut_Input1->labelcolor( YAIPS_BCOL_BUTTON);
    pBut_Input1->box( FL_BORDER_BOX);

    // 2. Input element
    pBut_Input2 = new Fl_Button( x1, y + yy - yy2, xx, yy2, "@2>");
    pBut_Input2->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pBut_Input2->tooltip( LANGDEF_INSP_WIN_REF);
    pBut_Input2->labelcolor( YAIPS_BCOL_BUTTON);
    pBut_Input2->box( FL_BORDER_BOX);

    x1 += xx + 5;

    // Open setting dialog

    xx = xx0;

    pGUI_Parameter = new Fl_Button( x1, y, xx, yy, "@+4menu2");
    pGUI_Parameter->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_Parameter->tooltip( LANGDEF_SETTINGS_POINTS);
    pGUI_Parameter->labelcolor( YAIPS_BCOL_BUTTON);
    pGUI_Parameter->shortcut( FL_COMMAND+'p');       // Short cut key

    x1 += xx + 5;

    // Toggle Teach / Inspection

    xx = xx0;

    pGUI_TeachToggle = new Fl_Button( x1, y, xx, yy, "@+3pencil");
    pGUI_TeachToggle->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_TeachToggle->tooltip( LANGDEF_SWITCH_TEACH_INSPECT);
    pGUI_TeachToggle->labelcolor( YAIPS_BCOL_BUTTON);
    pGUI_TeachToggle->shortcut( FL_COMMAND+'t');       // Short cut key

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
    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawAfterCallback  = YaIPS_GUI_MyDrawAfter_cb;     // Draw after callback
    pToolData->YaIPS_ImageDisp.pImage_Box->DrawCallbackArg1    = &pToolData->YaIPS_ImageDisp;  // Pointer to Fl_YaIPS_ImageDisp_t
    pToolData->YaIPS_ImageDisp.pImage_Box->DrawCallbackArg2    = pToolData;                    // Optional pointer to ToolData

    pToolData->YaIPS_ImageDisp.pImage_Box->pMouseCallback      = YaIPS_GUI_MyMouse_cb;         // Mouse event callback
    pToolData->YaIPS_ImageDisp.pImage_Box->MouseCallbackArg1   = &pToolData->YaIPS_ImageDisp;  // Pointer to Fl_YaIPS_ImageDisp_t
    pToolData->YaIPS_ImageDisp.pImage_Box->MouseCallbackArg2   = pToolData;                    // Optional pointer to ToolData

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

  // Ensure AOI from this tool window on big image display is removed if this tool window is closed.
  if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

    if( YaIPS_BigImageDisp.pImage_Box != NULL) {
      YaIPS_BigImageDisp.pImage_Box->redraw();
    }
  }

  // Free allocated work data at close of window
  YaIPS_ImageCompare_WorkDataFree( &pToolData->Cmp);
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

  pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data
  pMyToolWin = (CLASS_WIN_TOOL *)pToolData->pMyToolWin;  // Convert type of pointer

  // Show output image on the big display

  if( w == pMyToolWin->pGUI_Img_ShowOnBig) {                   // Show on big image

    if( pToolData->
        YaIPS_ImageDisp.pImage_Img != NULL) {                   // Got an image

      YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pToolData->YaIPS_ImageDisp.pImage_Img,
                                      MY_WIN_ID + iToolData, pToolData->YaIPS_ImageDisp.FileName);   // Load the image to the display
    }

  } else if( w == pMyToolWin->pBut_Input1) {                   // Select 1. input

    int WinIdNr_Before;

    WinIdNr_Before = pToolData->Input1_WinIdNr;

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + iToolData, &pToolData->Input1_WinIdNr, pMyToolWin->pBut_Input1, pMyToolWin->pBox_Input1);

    if( WinIdNr_Before != pToolData->Input1_WinIdNr) {         // Image source selection as changed

      pToolData->Input1_Change = 0;                           // Force recalculation output
    }

  } else if( w == pMyToolWin->pBut_Input2) {                   // Select 2. input

    int WinIdNr_Before;

    WinIdNr_Before = pToolData->Input2_WinIdNr;

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + iToolData, &pToolData->Input2_WinIdNr, pMyToolWin->pBut_Input2, pMyToolWin->pBox_Input2);

    if( WinIdNr_Before != pToolData->Input2_WinIdNr) {         // Image source selection as changed

      pToolData->Input2_Change = 0;                            // Force recalculation output
    }

  } else if( w == pMyToolWin->pGUI_Parameter) {                // Open parameter dialog

    YaIPS_GUI_ParameterWin( pMyToolWin->x() + 16, pMyToolWin->y() + 16, iToolData);

  } else if( w == pMyToolWin->pGUI_TeachToggle) {              // Toggle Teach / Inspection

    pToolData->Teach_mode = ! pToolData->Teach_mode;

    pToolData->Input1_Change = 0;          // Force recalculation output
    pToolData->Input2_Change = 0;                    // Force recalculation output
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

  if( DoEnable == 2) {                                          // Was periodically updates only

    return;
  }

  // ...

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_ShowOnBig,
                             DoEnable &&                                    // Enable GUI elements
                             pToolData->YaIPS_ImageDisp.pImage_Img != NULL); // and have an image loaded


  // Check input image and visualize state
  YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr, pMyToolWin->pBox_Input1);

  YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input2_WinIdNr, pMyToolWin->pBox_Input2);

  // Update color of Toggle Teach / Inspection button

  int MouseTeachState;

  MouseTeachState = 1;                                            // We have a mouse callback state

  // Only usable for brightness correction
  if( 1) {                                                       // Teach mode always available

    MouseTeachState = pToolData->Teach_mode ? 3 : 2;
  }

  pToolData->YaIPS_ImageDisp.pImage_Box->MouseTeachState = MouseTeachState;  // Shadow setting of teach mode button

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_TeachToggle, MouseTeachState >= 2);   // Teach mode available

  IqeB_GUI_WidgetLabelColor( pMyToolWin->pGUI_TeachToggle, MouseTeachState == 3 ? FL_GREEN : YAIPS_BCOL_BUTTON);

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

    int Input1_Check, Input2_Check, Input1_ImageChanged, Input2_ImageChanged, ForceUpdate;
    Fl_RGB_Image *pImgIn1, *pImgIn2, *pImgUsed;
    char *pInspErrText;

    ForceUpdate = false;                         // Preset: NO Force update of output image

    // Check input image and visualize state
    Input1_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr,
                                            NULL, &pImgIn1, &Input1_ImageChanged);


    Input2_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input2_WinIdNr,
                                            NULL, &pImgIn2, &Input2_ImageChanged);

    pImgUsed = NULL;
    pInspErrText = NULL;

    {

      ierr = 0;                                            // Reset error
      errstring = NULL;                                    // Reset error string

      // Check inputs

      if( Input1_Check != 0 || Input2_Check != 0) {            // Any input image is not valid

        if( Input1_Check == 0 && Input2_Check != 0) {          // Input 1 is valid, input 2 is not valid

          errstring = LANGDEF_IMGSEL_ERR_NO_REF;

          ierr = -100;

          if( Input1_ImageChanged != pToolData->Input1_Change) { // if image counts are different

            // Show scene image to adjust AOI parameter

            pToolData->Input1_Change = Input1_ImageChanged;        // Image is processed
          }

        } else if( Input1_Check != 0 && Input2_Check == 0) {   // Input 1 is not valid, input 2 is valid

          errstring = LANGDEF_IMGSEL_ERR_NO_INP;

          ierr = -101;

          if( Input2_ImageChanged != pToolData->Input2_Change) { // if image counts are different

            // Show scene image to adjust AOI parameter

            pToolData->Input2_Change = Input2_ImageChanged;        // Image is processed
          }

        } else {                                               // Both inputs are not valid

          errstring = LANGDEF_IMGSEL_ERR_NONE;

          ierr = -103;

          pToolData->Input1_Change = 0;                    // Force recalculation output
          pToolData->Input2_Change = 0;                    // Force recalculation output

        }

      } else {

        // Have both images

        // Check image size to be the same

        if( pImgIn1->data_w() != pImgIn2->data_w()) {         // Image size is different

          errstring = LANGDEF_IMGSEL_ERR_SIZES;

          ierr = -104;

        } else if( (pImgIn1->d() >= 3) != (pImgIn2->d() >= 3)) {  // Image color/BW is different

          errstring = LANGDEF_IMGSEL_ERR_BPP;

          ierr = -105;
        }
      }

      if( ierr == 0 &&                                        // No error until now
          Input1_ImageChanged == pToolData->Input1_Change &&  // Both image counts are same
          Input2_ImageChanged == pToolData->Input2_Change) {

        // Skip inspection because no image has changed
        goto SkipInspection;
      } else {

        if( ierr != 0 &&           // Have an error
            errstring != NULL) {   // and an error text

          pInspErrText = errstring;   // Copy to Info string

          errstring = NULL;
        }
      }

      pToolData->Input1_Change = Input1_ImageChanged;        // Image is processed
      pToolData->Input2_Change = Input2_ImageChanged;        // Image is processed

      if( ierr == 0) {                // No error until now

        // Depending from teach mode process one of the input images

        if( pImgIn2 != NULL && pToolData->Teach_mode) {     // Have a reference image and teach mode on

          pImgUsed = pImgIn2;   // Use reference image

        } else {

          pImgUsed = pImgIn1;   // Use input image
        }

        // Do the image processing

        ierr = 0;                                              // Reset error
        errstring = NULL;                                      // Reset error string

        // Always copy the input image

        if( ierr == 0 &&                   // No error until now
            pToolData->Teach_mode == 0) {  // and inspection mode

          ierr = YaIPS_ImageCompare_Inspect( &pToolData->YaIPS_ImageDisp.pImage_Img, // Output image
                                             pImgIn1,                // Input image
                                             pImgIn2,                // Reference image
                                             &pToolData->Cmp,        // Point to image compare data
                                             pToolData->Teach_mode); // 0 = inspection mode, 1 = teach mode

          pInspErrText = pToolData->Cmp.InspText;

        } else {

          // Copy input image to output image
          YaIPS_RGB_MixChannels( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgUsed, NULL, NULL, NULL);   // Remove alpha channel from input image
        }
      }

      // End processing

      // Has a valid output image

      if( ierr == 0 ||                                  // Have a result image
          ierr > 0) {                                   // or an inspection error

        YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, (char *)MY_WIN_GUI_NAME);

        if( ierr == 0) {

          YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, FL_GREEN, FL_BLACK, pInspErrText);  // Display info message
        } else {

          YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, FL_RED, FL_WHITE, pInspErrText);  // Display info message
        }

        YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp);                          // Reset error message

        ForceUpdate = true;                        // Force update of output image

      } else {                                          // Processing error

        //x/pToolData->Input1_Change = 0;                   // Force recalculation output
        //x/pToolData->Input2_Change = 0;                   // Force recalculation output

        // Empty display image

        // Copy input image to output image
        if( pImgUsed != NULL) {

        } else {

          // No image to output

          YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);
        }

        YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, (char *)MY_WIN_GUI_NAME);

        ForceUpdate = true;                        // Force update of output image

        // Info / error text display

        if( pInspErrText != NULL && pInspErrText[ 0] != '\0') {             // Have a check text

          YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, FL_RED, FL_WHITE, pInspErrText);  // Display error message

        } else {

          YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp);  // Reset info message
        }

#ifdef _DEBUG
        if( errstring != NULL) {                           // Have a error message

          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp, errstring);

        } else {

#ifdef use_again
          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp, LANGDEF_ERROR_CODE, ierr);
#else
          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp);
#endif
        }
#else
        YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp);
#endif
      }
    }

    // ...

SkipInspection:

    MyWinUpdate( iToolData, true);               // Update the GUI

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
 * IqeB_GUI_InspCompareWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_InspCompareWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
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

  pToolData->iToolData = iToolData;                       // Set sub window number

  pToolData->IsOpen = true;                               // Flag image as open

  pToolData->Cmp.InspText[ 0] = '\0';                     // Reset check text at entry
  pToolData->Cmp.pWorkData = NULL;

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
  int InspError, IsSelected;
  char *pInspText;

  // Check for valid output image displayed

  if( pToolData->YaIPS_ImageDisp.pImage_Img == NULL) {

    return;
  }

  //
  // Draw AOI
  //

  Fl_YaIPS_AOI_t *pAOI;

  pAOI = &pToolData->Cmp.AOI;
  InspError = 0;
  pInspText = NULL;

  if( pToolData->Teach_mode) {                // Adjust AOI


  } else {      // Inspection mode

    InspError = pToolData->Cmp.InspError;
    pInspText = pToolData->Cmp.InspText;

  }

  if( pAOI != NULL) {

    IsSelected = pToolData->Teach_mode &&
                (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL);      // Mouse is over any AOI

    // Clipping ?

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( pYaIPS_ImageDisp->BigImage_sx,
                    pYaIPS_ImageDisp->BigImage_sy,
                    pYaIPS_ImageDisp->BigImage_sw,
                    pYaIPS_ImageDisp->BigImage_sh);
    }

    ImageDispAoiRectDraw( pYaIPS_ImageDisp, pAOI, pToolData->Teach_mode, IsSelected, InspError, pInspText);

    if( ! pToolData->Teach_mode &&          // If inspection mode
        pToolData->Cmp.FlagShowBlobArea) {  // and draw the blobs

      // Draw the blob areas
      YaIPS_ImageCompare_DrawArea( pYaIPS_ImageDisp, &pToolData->Cmp, pToolData->Teach_mode);
    }
  }

//x/ExitPoint:

  // Finish up

  fl_line_style( 0);   // Reset to default

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
  YaIPS_ToolData_info_t *pToolData;

  pToolData = YaIPS_ToolData_info + SubWinIDx;               // Point to info data

  YaIPS_GUI_MyDrawAfter_Func( pYaIPS_ImageDisp, pToolData, true);
}

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
  int minAoiDist, CursorShapeTest, AoiDeltaAddTest, IsBigImageDisp;
  YaIPS_ToolData_info_t *pToolData;
  Fl_YaIPS_AOI_t *pAOI_Best, *pAOI_This;
  static int Last_x = -9999, Last_y = -9999;           // Must be static
  static int Pressed_x, Pressed_y;                     // Used for move with pressed mouse button
  static Fl_YaIPS_AOI_t Pressed_AOI;                   // AOI on press of mouse button
  static Fl_YaIPS_AOI_t *pPressed_AOI_Best;            // What AOI to modify

  pYaIPS_ImageDisp = (Fl_YaIPS_ImageDisp_t *)pArg1;    // Get pointer to image display data

  // Get pointer to tool data

  pToolData = (YaIPS_ToolData_info_t *)pArg2;          // Need pointer to tool data
  if( pToolData == NULL) {                             // Security test, have a pointer

    return( -1);                                       // Return error
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

  pYaIPS_ImageDisp->CursorShape = IsBigImageDisp ? 0 : -1;       // Overwrite: Preset default cursor shape
  pYaIPS_ImageDisp->MyWinID = MY_WIN_ID + pToolData->iToolData;  // Overwrite: Update big image, set my tool window ID
  minAoiDist     = -1;                                // Needed for section of nearest AOI
  pAOI_Best      = NULL;                              // Modify this AOI

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

      if( pToolData->Teach_mode != 0) {                                       // Can be changed with the mouse

        pAOI_This = &pToolData->Cmp.AOI;

        ierr = YaIPS_ImageDispAoiRectCC( pYaIPS_ImageDisp, pAOI_This, &minAoiDist, &CursorShapeTest, &AoiDeltaAddTest);

        if( ierr == true) {     // Got one (or a better one)

          // nearer aoi found
          pYaIPS_ImageDisp->CursorShape = CursorShapeTest;
          pYaIPS_ImageDisp->AoiDeltaAdd = AoiDeltaAddTest;
          pAOI_Best   = pAOI_This;
        }
      }

      if( pYaIPS_ImageDisp->mouseleft) {                    // Left mouse button pressed

        if( pYaIPS_ImageDisp->AoiDeltaAdd != 0 && pYaIPS_ImageDisp->Latched_AoiDeltaAdd == 0) {   // Latch AOI mouse modification

          pYaIPS_ImageDisp->Latched_AoiDeltaAdd = pYaIPS_ImageDisp->AoiDeltaAdd;
          pYaIPS_ImageDisp->Latched_CursorShape = pYaIPS_ImageDisp->CursorShape;

          pPressed_AOI_Best = pAOI_Best;                    // Modify this AOI
          Pressed_x = Last_x + pYaIPS_ImageDisp->Delta_x;   // Latch position at button press
          Pressed_y = Last_y + pYaIPS_ImageDisp->Delta_y;

          memcpy( &Pressed_AOI, pAOI_Best, sizeof( Fl_YaIPS_AOI_t)); // Remember AOI data a button press
        }

        if( pYaIPS_ImageDisp->Latched_AoiDeltaAdd != 0) {        // Have latched AOI mouse modification

          pYaIPS_ImageDisp->AoiDeltaAdd = pYaIPS_ImageDisp->Latched_AoiDeltaAdd;   // Use it
          pYaIPS_ImageDisp->CursorShape = pYaIPS_ImageDisp->Latched_CursorShape;
          pAOI_Best   = pPressed_AOI_Best;
        }

      } else {                                                   // Left mouse button is NOT pressed

        pYaIPS_ImageDisp->Latched_AoiDeltaAdd = 0;               // Reset latched data
        pYaIPS_ImageDisp->Latched_CursorShape = 0;
      }

      if( pYaIPS_ImageDisp->mouseleft &&                                        // and left button pressed
          pYaIPS_ImageDisp->AoiDeltaAdd != 0 &&                                 // and add deltas
          (pYaIPS_ImageDisp->Delta_x != 0 || pYaIPS_ImageDisp->Delta_y != 0)) { // and mouse has moved

        memcpy( pAOI_Best, &Pressed_AOI, sizeof( Fl_YaIPS_AOI_t)); // Restore AOI data from button press

        pYaIPS_ImageDisp->Delta_x = dto32( (Last_x - Pressed_x) / pYaIPS_ImageDisp->PixelImageToScreen);
        pYaIPS_ImageDisp->Delta_y = dto32( (Last_y - Pressed_y) / pYaIPS_ImageDisp->PixelImageToScreen);

        YaIPS_ImageDispAoiRectDeltaAdd( pYaIPS_ImageDisp, pAOI_Best,
                                        pYaIPS_ImageDisp->AoiDeltaAdd, pYaIPS_ImageDisp->Delta_x, pYaIPS_ImageDisp->Delta_y);

        pToolData->Input1_Change = 0;                                 // Force recalculation output
        pToolData->Input2_Change = 0;                                 // Force recalculation output

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
 * IqeB_GUI_InspCompareWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_InspCompareWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

    // Register draw after function for big image display
    YaIPS_ToolWinDrawAfterSet( MY_WIN_ID + SubWinIDx, YaIPS_GUI_MyDrawAfter_Other);

    IqeB_GUI_InspCompareWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  // Register draw after function for big image display
  YaIPS_ToolWinDrawAfterSet( MY_WIN_ID + iUnused, YaIPS_GUI_MyDrawAfter_Other);

  IqeB_GUI_InspCompareWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


