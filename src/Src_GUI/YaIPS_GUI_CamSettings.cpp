/****************************************************************************

  YaIPS_GUI_CamSettings.cpp

  Camera settings.

  23.04.2025 RR: First edition of this file.

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

#define MY_WIN_PREF_NAME  "CamSettings"              // Name used for the preference data

/************************************************************************************
 * Globals
 *
 */

//
// Global setting
//

// Camera acquisition mode
int YaIPS_CamPar_Acq_Mode;               // Acquisition mode. Is also number of last selected tab group.

// Area camera settings
int YaIPS_CamPar_AMode_Square = false;   // Area mode: If true, make a square image output
int YaIPS_CamPar_AMode_ColMod = 0;       // All cameras: Color space of output: RGB, BW, R, G or B
int YaIPS_CamPar_AMode_SkipFirst = 0;    // All cameras: Skip first camera.
int YaIPS_CamPar_AMode_StartAcqOn = 0;   // All cameras: Start with 'continuous acquire on' after open of dialog.

// Line scan camera simulation
int YaIPS_CamPar_LMode_Adjust;           // Line mode: If true, show lines to average
int YaIPS_CamPar_LMode_LinesAcq = 256;   // Line mode: height of acquired image, # of lines to acquire
int YaIPS_CamPar_LMode_nAvgLines = 1;    // Line mode: # area lines to average
int YaIPS_CamPar_LMode_nRefresh = 10;    // Line mode: Refresh image after # lines acquired

// Height camera simulation
int YaIPS_CamPar_HMode_Adjust;           // Height mode: If true, adjust top/lower line in area camera
int YaIPS_CamPar_HMode_LinesAcq  = 256;  // Height mode: height of acquired image, # of lines to acquire
int YaIPS_CamPar_HMode_Top_Line  = 100;  // Height mode: Top line in area camera
int YaIPS_CamPar_HMode_Base_Line = 400;  // Height mode: Base/lower line in area camera
int YaIPS_CamPar_HMode_Val_Thres =  32;  // Height mode: Brightness threshold

/************************************************************************************
 * Statics
 */

static  Fl_Window *pMyToolWin;
static int MyWinPosX = IQE_GUI_NO_WINPOS_X, MyWinPosY = IQE_GUI_NO_WINPOS_Y; // last window position

static IqeFl_Tabs      *pTab_Groups;         // Point to tabulator GUI element
static IqeFl_Int_Input *pInt_LM_LinesAcq;    // Line mode: height of acquired image, # of lines to acquire
static IqeFl_Int_Input *pInt_LM_nAvgLines;   // Line mode: # area lines to average
static IqeFl_Int_Input *pInt_LM_nRefresh ;   // Line mode: # area lines to average
static Fl_Button *pTeachToggle_LMode;        // Line mode: Toggle teach / inspection mode
static IqeFl_Int_Input *pInt_HM_TopLine;     // Height mode: Top line in area camera
static IqeFl_Int_Input *pInt_HM_BotLine;     // Height mode: Base/lower line in area camera
static Fl_Button *pTeachToggle_HMode;        // Height mode: Toggle teach / inspection mode

/************************************************************************************
 * Presets for this tools window
 *
 */

 static T_GUI_PreferenceEntry MyPreferences[] =
 {

  // Hold last selected tab

  { PREF_T_INT,       "Acq_Mode",      "0", &YaIPS_CamPar_Acq_Mode},

  // Area camera settings

  { PREF_T_INT,     "AMode_Square",   "0", &YaIPS_CamPar_AMode_Square},
  { PREF_T_INT,     "AMode_ColMod",   "0", &YaIPS_CamPar_AMode_ColMod},
  { PREF_T_INT,  "AMode_SkipFirst",   "0", &YaIPS_CamPar_AMode_SkipFirst},
  { PREF_T_INT, "AMode_StartAcqOn",   "0", &YaIPS_CamPar_AMode_StartAcqOn},

  // Line scan camera simulation

  { PREF_T_INT,     "LMode_Adjust",   "0", &YaIPS_CamPar_LMode_Adjust},
  { PREF_T_INT,   "LMode_LinesAcq", "256", &YaIPS_CamPar_LMode_LinesAcq},
  { PREF_T_INT,  "LMode_nAvgLines",   "1", &YaIPS_CamPar_LMode_nAvgLines},
  { PREF_T_INT,  "LMode_nRefresch",  "10", &YaIPS_CamPar_LMode_nRefresh},

  // Height camera simulation

  { PREF_T_INT,     "HMode_Adjust",   "0", &YaIPS_CamPar_HMode_Adjust},
  { PREF_T_INT,   "HMode_LinesAcq", "256", &YaIPS_CamPar_HMode_LinesAcq},
  { PREF_T_INT,   "HMode_Top_Line", "100", &YaIPS_CamPar_HMode_Top_Line},
  { PREF_T_INT,  "HMode_Base_Line", "400", &YaIPS_CamPar_HMode_Base_Line},
  { PREF_T_INT,  "HMode_Val_Thres",  "32", &YaIPS_CamPar_HMode_Val_Thres},
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&pMyToolWin), &MyWinPosX, &MyWinPosY);

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyWinUpdate()
{
  int Enable, ValueTop, ValueBot, ValueTopNew, ValueBotNew, RedrawOnExit;
  char TempBuffer[ 256];

  RedrawOnExit = false;

  // Enables for line camera mode

  Enable = YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_LINE &&      // Enable for line camera mode
           YaIPS_CamPar_LMode_Adjust;

  IqeB_GUI_WidgetActivate( pInt_LM_nAvgLines, Enable);              // Enable GUI elements
  IqeB_GUI_WidgetActivate( pInt_LM_nRefresh, Enable);               // Enable GUI elements

  // Height mode: Color teach toggle button

  IqeB_GUI_WidgetLabelColor( pTeachToggle_LMode, YaIPS_CamPar_LMode_Adjust ? FL_GREEN : YAIPS_BCOL_BUTTON);

  // Enables for height camera mode

  Enable = YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT &&    // Enable for height camera mode
           YaIPS_CamPar_HMode_Adjust;

  IqeB_GUI_WidgetActivate( pInt_HM_TopLine, Enable);                // Enable GUI elements
  IqeB_GUI_WidgetActivate( pInt_HM_BotLine, Enable);                // Enable GUI elements

  // Height mode: Color teach toggle button

  IqeB_GUI_WidgetLabelColor( pTeachToggle_HMode, YaIPS_CamPar_HMode_Adjust ? FL_GREEN : YAIPS_BCOL_BUTTON);

  // Adjust HM_TopLine/HM_BotLine
  ValueTop = atoi( pInt_HM_TopLine->value());                       // get the value
  ValueBot = atoi( pInt_HM_BotLine->value());

  ValueTopNew = ValueTop;                                           // Set for update check
  ValueBotNew = ValueBot;

  // Clip minimum maximum
  if( YaIPS_Camera_YY > 0) {                                         // Camera height is known

    if( pInt_HM_TopLine->Min != 0) {                                // Minimum is not correct
      pInt_HM_TopLine->Min = 0;
    }
    if( pInt_HM_TopLine->Max != YaIPS_Camera_YY - HMODE_ACQ_REGION_MIN_HEIGHT - 1) { // Maximum is not correct
      pInt_HM_TopLine->Max = YaIPS_Camera_YY - HMODE_ACQ_REGION_MIN_HEIGHT - 1;
    }

    if( pInt_HM_BotLine->Min != HMODE_ACQ_REGION_MIN_HEIGHT - 1) {                  // Minimum is not correct
      pInt_HM_BotLine->Min = HMODE_ACQ_REGION_MIN_HEIGHT - 1;
    }
    if( pInt_HM_BotLine->Max != YaIPS_Camera_YY - 1) {               // Maximum is not correct
      pInt_HM_BotLine->Max = YaIPS_Camera_YY - 1;
    }

  } else {

    if( pInt_HM_TopLine->Min != 0) {                                // Minimum is not correct
      pInt_HM_TopLine->Min = 0;
    }
    if( pInt_HM_TopLine->Max != 2024 - HMODE_ACQ_REGION_MIN_HEIGHT - 1) {  // Maximum is not correct
      pInt_HM_TopLine->Max = 2024 - HMODE_ACQ_REGION_MIN_HEIGHT - 1;
    }

    if( pInt_HM_BotLine->Max != 2024 - 1) {                         // Maximum is not correct
      pInt_HM_BotLine->Max = 2024 - 1;
    }
  }

  if( ValueTopNew < pInt_HM_TopLine->Min) {                         // Clip to current minimum
    ValueTopNew = pInt_HM_TopLine->Min;
  }
  if( ValueTopNew > pInt_HM_TopLine->Max) {                         // Clip to current maximum
    ValueTopNew = pInt_HM_TopLine->Max;
  }

  if( ValueBotNew < pInt_HM_BotLine->Min) {                         // Clip to current minimum
    ValueBotNew = pInt_HM_BotLine->Min;
  }
  if( ValueBotNew > pInt_HM_BotLine->Max) {                         // Clip to current maximum
    ValueBotNew = pInt_HM_BotLine->Max;
  }

  if( ValueTopNew != ValueTop) {                                    // Have to update

    RedrawOnExit = true;                                            // Redraw camera image to show corrected line

    YaIPS_CamPar_HMode_Top_Line = ValueTopNew;
    sprintf( TempBuffer, "%d", ValueTopNew);                        // Update on GUI
    pInt_HM_TopLine->value( TempBuffer);
    pInt_HM_TopLine->redraw();
  }

  if( ValueBotNew != ValueBot) {                                    // Have to update

    RedrawOnExit = true;                                            // Redraw camera image to show corrected line

    YaIPS_CamPar_HMode_Base_Line = ValueBotNew;
    sprintf( TempBuffer, "%d", ValueBotNew);                        // Update on GUI
    pInt_HM_BotLine->value( TempBuffer);
    pInt_HM_BotLine->redraw();
  }

  // Get last selected tab group

  if( YaIPS_CamPar_Acq_Mode != pTab_Groups->GetTabGroup()) {         // Tab group will change

    RedrawOnExit = true;                                            // Redraw camera image. Other Mode has other drawing.

    YaIPS_CamPar_Acq_Mode = pTab_Groups->GetTabGroup();
  }

  // Redraw

  if( RedrawOnExit) {                                          // Redraw on exit

    IqeB_GUI_CameraImageRedraw();                              // Redraw camera image
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

  if( YaIPS_CamPar_AMode_ColMod == Value) {                 // Value will not change

    return;                                                // Exit, nothing to do
  }

  YaIPS_CamPar_AMode_ColMod = Value;                        // Set new value
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

  if( w == pTeachToggle_LMode) {                   // Toggle Teach / Inspection button or height mode show lines

    YaIPS_CamPar_LMode_Adjust = ! YaIPS_CamPar_LMode_Adjust;

    IqeB_GUI_CameraImageRedraw();               // Redraw camera image to show changes of lines

  } else if( w == pTeachToggle_HMode) {                   // Toggle Teach / Inspection button or height mode show lines

    YaIPS_CamPar_HMode_Adjust = ! YaIPS_CamPar_HMode_Adjust;

    IqeB_GUI_CameraImageRedraw();               // Redraw camera image to show changes of lines
  }
}

/************************************************************************************
 * IqeB_GUI_CBox_SetValue_Callback
 */

static void IqeB_GUI_CBox_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  int *pValue;
  Fl_Check_Button *pThis;

  // ...

  pThis  = (Fl_Check_Button *)w;
  pValue = (int *)pValueArg;                    // get pointer to associated variable

  *pValue = pThis->value();                     // update the variable

  if( pValue == &YaIPS_CamPar_LMode_Adjust ||    // Was line mode show lines
      pValue == &YaIPS_CamPar_HMode_Adjust) {    // or was height mode show lines

    IqeB_GUI_CameraImageRedraw();               // Redraw camera image to show changes of lines
  }
}

/************************************************************************************
 * IqeB_GUI_Choice_SetValue_Callback
 */

#ifdef use_again   // May be used later
static void IqeB_GUI_Choice_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  int Value;
  int *pValue;
  Fl_Choice *pThis;

  // ...

  pThis  = (Fl_Choice *)w;
  Value  = pThis->value();

  //
  // Test for direct value change
  //

  if( pValueArg != NULL) {           // Have assign to variable

    pValue = (int *)pValueArg;       // get pointer to associated variable

    *pValue = Value;                 // update the variable

    return;
  }
}
#endif

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
  }

  if( pValue == &YaIPS_CamPar_HMode_Top_Line) {      // Has changed top line

    int ValueBot, ValueTop, ValueBotIn;

    ValueTop = Value;
    ValueBot = atoi( pInt_HM_BotLine->value());;

    ValueBotIn = ValueBot;

    if( ValueBot - ValueTop < HMODE_ACQ_REGION_MIN_HEIGHT - 1) {  // Below minimum distance

      ValueBot = ValueTop + HMODE_ACQ_REGION_MIN_HEIGHT - 1;

      if( ValueBot > pInt_HM_TopLine->Max) {

        ValueBot = pInt_HM_BotLine->Max;
        ValueTop = ValueBot - HMODE_ACQ_REGION_MIN_HEIGHT + 1;
      }
    }

    if( ValueBotIn != ValueBot) {         // Value top has changed

      YaIPS_CamPar_HMode_Base_Line = ValueBot;
      sprintf( TempBuffer, "%d", ValueBot);    // Update on GUI
      pInt_HM_BotLine->value( TempBuffer);
      pInt_HM_BotLine->redraw();
    }

    if( Value != ValueTop) {
      Value = ValueTop;
      WasClipped = true;                 // Value was clipped
    }

  } else
  if( pValue == &YaIPS_CamPar_HMode_Base_Line) {     // Has changed bottom line

    int ValueBot, ValueTop, ValueTopIn;

    ValueTop = atoi( pInt_HM_TopLine->value());
    ValueBot = Value;

    ValueTopIn = ValueTop;

    if( ValueBot - ValueTop < HMODE_ACQ_REGION_MIN_HEIGHT - 1) {  // Below minimum distance

      ValueTop = ValueBot - HMODE_ACQ_REGION_MIN_HEIGHT + 1;

      if( ValueTop < pInt_HM_TopLine->Min) {

        ValueTop = pInt_HM_TopLine->Min;
        ValueBot = pInt_HM_TopLine->Min + HMODE_ACQ_REGION_MIN_HEIGHT - 1;
      }
    }

    if( ValueTopIn != ValueTop) {         // Value top has changed

      YaIPS_CamPar_HMode_Top_Line = ValueTop;
      sprintf( TempBuffer, "%d", ValueTop);    // Update on GUI
      pInt_HM_TopLine->value( TempBuffer);
      pInt_HM_TopLine->redraw();
    }

    if( Value != ValueBot) {
      Value = ValueBot;
      WasClipped = true;                 // Value was clipped
    }
  }

  if( WasClipped) {                      // Value was clipped

    sprintf( TempBuffer, "%d", Value);   // Update on GUI
    pThis->value( TempBuffer);
  }

  // Value has changed. Need redraw of image if adjust line is active.
  if( (YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_LINE && YaIPS_CamPar_LMode_Adjust) ||
      (YaIPS_CamPar_Acq_Mode == YAIPS_CAM_ACQ_MODE_HEIGHT && YaIPS_CamPar_HMode_Adjust)) {

    IqeB_GUI_CameraImageRedraw();                              // Redraw camera image
  }

//x/ExitPoint:

  *pValue = Value;         // update the variable
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

#ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
  Fl::remove_idle( IqeB_GUI_ToolsMyIdleAction);      // Redraw window during idle
#endif
  Fl::remove_check( IqeB_GUI_ToolsMyIdleAction);     // Check small image size change

  IqeB_GUI_CloseToolWindow( (void **)&pMyToolWin);
}

/************************************************************************************
 * YaIPS_GUI_CamSettingsWin
 *
 * Open dialog
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void YaIPS_GUI_CamSettingsWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
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

  if( pMyToolWin == NULL) {  // no tool window until now

    int xPos, yPos;

    xPos = xRight;
    yPos = yTop;

    if( MyWinPosX != IQE_GUI_NO_WINPOS_X && MyWinPosY != IQE_GUI_NO_WINPOS_Y) { // have last window position

      xPos = MyWinPosX;
      yPos = MyWinPosY;
    }

    pMyToolWin = new Fl_Window( xPos, yPos, IQE_GUI_TOOLS_STD_WITDH, 168, LANGDEF_SETTINGS);

    if( pMyToolWin == NULL) {  // security test

      return;
    }
  }

  //
  //  GUI things
  //

  int x1, xx1, y, yy;
  //x/int xx2, xc;
  int yGroup;
  char TempBuffer[ 256];

  Fl_Check_Button *pTemp_Check_Button;
  //x/Fl_Float_Input  *pTemp_Float_Input;
  //x/Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group, *pTemp_Group2;
  Fl_Button       *pTemp_Button;
  //x/Fl_Choice       *pTemp_Choice;
  Fl_Radio_Round_Button *pRadioButTemp;

  x1  = 4;
  xx1 = pMyToolWin->w() - 16;
  //x/xx2 = xx1 / 2;
  //x/xc  = pMyToolWin->w() / 2;          // x center
  yy  = 26;

  y = 4;

  //
  // Tabs
  //

  y += 18;

  pTemp_Tabs = new IqeFl_Tabs( x1, y, pMyToolWin->w() - x1 - 4, pMyToolWin->h() - y - 4, LangStringLookup( "&GUI_CamSettings_TabA0=Camera operating modes"));
  pTemp_Tabs->selection_color( YAIPS_COLOR_SELECTION);
  pTab_Groups = pTemp_Tabs;

  y += 26;

  //
  // Group 'Einstellungen'
  //

  yGroup = y;

  //
  // Group 'XXX'
  //

  y = yGroup;
  x1  = 4;
  pTemp_Group = new Fl_Group( x1, y, pMyToolWin->w() - x1 - 4, pMyToolWin->h() - y - 4, LangStringLookup( "&GUI_CamSettings_TabA1=Area"));

    // Square cut out

    y += 4;
    y += 4;

    xx1 = pMyToolWin->w() - 16;

    pTemp_Check_Button = new Fl_Check_Button( x1 + 4, y, xx1, yy, LangStringLookup( "&GUI_CamSettings_TabA2=Square cutout"));
    pTemp_Check_Button->tooltip( LangStringLookup( "&GUI_CamSettings_TabA2a=The camera image is cropped to a square shape."));
    pTemp_Check_Button->value( YaIPS_CamPar_AMode_Square);
    pTemp_Check_Button->callback( IqeB_GUI_CBox_SetValue_Callback, &YaIPS_CamPar_AMode_Square);

    // Next line

    y += yy;

    //
    // Color channel selection
    //

    x1 = 4;
    xx1 = pMyToolWin->w() - x1 - 4;

    pTemp_Group2 = new Fl_Group( x1, y, xx1, yy + 14, LANGDEF_COL_CHANNEL_DPOINT);  // Group around this radio buttons
    pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT);
    pTemp_Group2->labelsize( 12);

    y += 12;

    // ...

    xx1 = 62;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LANGDEF_COLOR);
    pRadioButTemp->tooltip( LANGDEF_IMAGE_NOT_CHANGED);
    pRadioButTemp->callback( YaIPS_BigImageColMod_Callback, (void *)YAIPS_DISP_COLMOD_NORMAL);
    pRadioButTemp->value( YaIPS_CamPar_AMode_ColMod == YAIPS_DISP_COLMOD_NORMAL);   // Set value

    x1 += xx1;
    x1 += 2;

    xx1 = 46;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LANGDEF_COLOR_BW);
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_CamSettings_TabA3a=Black and white"));
    pRadioButTemp->callback( YaIPS_BigImageColMod_Callback, (void *)YAIPS_DISP_COLMOD_BW);
    pRadioButTemp->value( YaIPS_CamPar_AMode_ColMod == YAIPS_DISP_COLMOD_BW);   // Set value

    x1 += xx1;
    x1 += 2;

    xx1 = 36;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LANGDEF_COLOR_R);
    pRadioButTemp->tooltip( LANGDEF_COL_CHANNEL_R);
    pRadioButTemp->callback( YaIPS_BigImageColMod_Callback, (void *)YAIPS_DISP_COLMOD_R);
    pRadioButTemp->value( YaIPS_CamPar_AMode_ColMod == YAIPS_DISP_COLMOD_R);   // Set value

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LANGDEF_COLOR_G);
    pRadioButTemp->tooltip( LANGDEF_COL_CHANNEL_G);
    pRadioButTemp->callback( YaIPS_BigImageColMod_Callback, (void *)YAIPS_DISP_COLMOD_G);
    pRadioButTemp->value( YaIPS_CamPar_AMode_ColMod == YAIPS_DISP_COLMOD_G);   // Set value

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LANGDEF_COLOR_B);
    pRadioButTemp->tooltip( LANGDEF_COL_CHANNEL_B);
    pRadioButTemp->callback( YaIPS_BigImageColMod_Callback, (void *)YAIPS_DISP_COLMOD_B);
    pRadioButTemp->value( YaIPS_CamPar_AMode_ColMod == YAIPS_DISP_COLMOD_B);   // Set value

    pTemp_Group2->end();

    // Next line

    y += yy - 3;
    x1 = 4;

    // Skip first camera

    xx1 = pMyToolWin->w() - 16;

    pTemp_Check_Button = new Fl_Check_Button( x1 + 4, y, xx1, yy - 3, LangStringLookup( "&GUI_CamSettings_TabA4=Ignore first camera"));
    pTemp_Check_Button->tooltip( LangStringLookup( "&GUI_CamSettings_TabA4a="
                                                   "With camera detection, the first camera is ignored.\n"
                                                   "This can be useful for laptops with built-in cameras.\n"
                                                   "This camera is then ignored."));
    pTemp_Check_Button->value( YaIPS_CamPar_AMode_SkipFirst);
    pTemp_Check_Button->callback( IqeB_GUI_CBox_SetValue_Callback, &YaIPS_CamPar_AMode_SkipFirst);

    // Start with 'continuous acquire on' after open of dialog
    y += yy - 3;
    x1 = 4;

    xx1 = pMyToolWin->w() - 16;

    pTemp_Check_Button = new Fl_Check_Button( x1 + 4, y, xx1, yy - 3, LangStringLookup( "&GUI_CamSettings_TabA5=Start with acquire on"));
    pTemp_Check_Button->tooltip( LangStringLookup( "&GUI_CamSettings_TabA5a="
                                                   "After opening the dialog, set\n"
                                                   "'Continuous acquire on' to on."));
    pTemp_Check_Button->value( YaIPS_CamPar_AMode_StartAcqOn);
    pTemp_Check_Button->callback( IqeB_GUI_CBox_SetValue_Callback, &YaIPS_CamPar_AMode_StartAcqOn);

    // Finish things for this group

    pTemp_Group->end();

  y = yGroup;
  x1  = 4;
  pTemp_Group = new Fl_Group( x1, y, pMyToolWin->w() - x1 - 4, pMyToolWin->h() - y - 4, LangStringLookup( "&GUI_CamSettings_TabB1=Lines"));
  //x/pTemp_Group->tooltip( "Allgemeine Einstellungen\n");

    y += 4;
    y += 4;

    x1  = 4;

    pTemp_Button = new Fl_Button( x1 + 4, y, yy, yy, "@+1pencil");
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &YaIPS_CamPar_LMode_Adjust);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_CamSettings_TabB3a="
                           "Display and change the acquire area.\n"
                           "The area can be adjusted with the mouse."));
    pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
    pTemp_Button->shortcut( FL_COMMAND+'t');       // Short cut key
    pTeachToggle_LMode = pTemp_Button;

    //x/y += yy;

    xx1 = 40;
    x1 = pMyToolWin->w() - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_CamSettings_TabB2=# Lines to acquire"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    //x/pTemp_Int->labelsize( 10);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_CamSettings_TabB2a="
                        "Number of lines to be acquired\n"
                        "or height of the recorded image."));
    sprintf( TempBuffer, "%d", YaIPS_CamPar_LMode_LinesAcq );
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &YaIPS_CamPar_LMode_LinesAcq);
    pTemp_Int->SetModifyData( 32, 2048, 16, 1);
    pInt_LM_LinesAcq = pTemp_Int;

    y += yy;

    xx1 = 40;
    x1 = pMyToolWin->w() - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_CamSettings_TabB4=# lines averaged"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    //x/pTemp_Int->labelsize( 10);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_CamSettings_TabB4a="
                        "This number of lines from the area camera\n"
                        "are combined into one recording line.\n"
                        "The lines are centered in the area image."));
    sprintf( TempBuffer, "%d", YaIPS_CamPar_LMode_nAvgLines );
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &YaIPS_CamPar_LMode_nAvgLines);
    pTemp_Int->SetModifyData( 1, 32);
    pInt_LM_nAvgLines = pTemp_Int;

    y += yy;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_CamSettings_TabB5=Refresh rows"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    //x/pTemp_Int->labelsize( 10);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_CamSettings_TabB5a="
                        "The image display is updated after\n"
                        "this number of lines have been recorded.\n"
                        "With a value of 0, each new line is displayed."));
    sprintf( TempBuffer, "%d", YaIPS_CamPar_LMode_nRefresh );
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &YaIPS_CamPar_LMode_nRefresh);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_LM_nRefresh = pTemp_Int;

    // Finish things for this group

    pTemp_Group->end();

  y = yGroup;
  x1  = 4;
  pTemp_Group = new Fl_Group( x1, y, pMyToolWin->w() - x1 - 4, pMyToolWin->h() - y - 4, LangStringLookup( "&GUI_CamSettings_TabC1=Height"));

    // Square cut out

    y += 4;
    y += 4;

    x1  = 4;

    pTemp_Button = new Fl_Button( x1 + 4, y, yy, yy, "@+1pencil");
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &YaIPS_CamPar_HMode_Adjust);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_CamSettings_TabC3a="
                           "Display and change the acquire area.\n"
                           "The area can be adjusted with the mouse."));
    pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
    pTemp_Button->shortcut( FL_COMMAND+'t');       // Short cut key
    pTeachToggle_HMode = pTemp_Button;

    xx1 = 40;
    x1 = pMyToolWin->w() - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_CamSettings_TabC2=# Lines to acquire"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    //x/pTemp_Int->labelsize( 10);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_CamSettings_TabC2a="
                        "Number of lines to be acquired\n"
                        "or height of the recorded image."));
    sprintf( TempBuffer, "%d", YaIPS_CamPar_HMode_LinesAcq );
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &YaIPS_CamPar_HMode_LinesAcq);
    pTemp_Int->SetModifyData( 32, 2048, 16, 1);

    y += yy;

    xx1 = 40;
    x1 = pMyToolWin->w() - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_CamSettings_TabC4=Acquire area top"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->tooltip( LangStringLookup( "&GUI_CamSettings_TabC4a="
                        "This line in the area camera image corresponds\n"
                        "to the highest measurable height."));
    sprintf( TempBuffer, "%d", YaIPS_CamPar_HMode_Top_Line );
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &YaIPS_CamPar_HMode_Top_Line);
    pTemp_Int->SetModifyData( 0, 2048, 10, 1);
    pInt_HM_TopLine = pTemp_Int;

    y += yy;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_CamSettings_TabC5=Acquire area bottom"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->tooltip( LangStringLookup( "&GUI_CamSettings_TabC5a="
                        "This line in the area camera image corresponds\n"
                        "to the lowest measurable height."));
    sprintf( TempBuffer, "%d", YaIPS_CamPar_HMode_Base_Line );
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &YaIPS_CamPar_HMode_Base_Line);
    pTemp_Int->SetModifyData( 100, 2048, 10, 1);
    pInt_HM_BotLine = pTemp_Int;

    y += yy;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_CamSettings_TabC6=Brightness threshold"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->tooltip( LangStringLookup( "&GUI_CamSettings_TabC6a="
                        "The brightest spot in a column is used to determine the\n"
                        "height. The brightness must be at least above this threshold."));
    sprintf( TempBuffer, "%d", YaIPS_CamPar_HMode_Val_Thres );
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &YaIPS_CamPar_HMode_Val_Thres);
    pTemp_Int->SetModifyData( 16, 240, 8, 1);

    // Finish things for this group

    pTemp_Group->end();

  // finish up

  pTemp_Tabs->end();

  pTemp_Tabs->SetTabGroup( YaIPS_CamPar_Acq_Mode);    // Select tab group from last session

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
