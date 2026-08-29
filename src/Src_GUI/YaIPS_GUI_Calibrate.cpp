/****************************************************************************

  YaIPS_GUI_Calibrate.cpp

  Calibration window

  30.03.2025 RR: First edition of this file.

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
#define MY_WIN_ID     YAIPS_WIN_ID_CALIBRATE    // Source specific windows ID
#define MY_WIN_MAX    1                         // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_Calibrate_Title=Calibration"      // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinCalibration"      // Name used for the preference data

// Define for window sizes: This window is not resizable
#define MYWIN_SIZE_X_MIN       314
#define MYWIN_SIZE_X_MAX       MYWIN_SIZE_X_MIN
#define MYWIN_SIZE_X_DEFAULT   MYWIN_SIZE_X_MIN

#define MYWIN_SIZE_Y_MIN       238
#define MYWIN_SIZE_Y_MAX       MYWIN_SIZE_Y_MIN
#define MYWIN_SIZE_Y_DEFAULT   MYWIN_SIZE_Y_MIN

/************************************************************************************
* forwards
*/

static void close_cb( Fl_Widget *w, long int iToolData);

/************************************************************************************
* Data managed by this windows
*/

static int   YaIPS_Calib_do_XandY = true;   // If set, calibrate X an X together
static float YaIPS_Calib_DistX = 10.0;      // Calibration distance X in [Units]
static float YaIPS_Calib_DistY = 10.0;      // Calibration distance Y in [Units]

// Defines for GUI element strings

#define YAIPS_CALIP_FORMAT_UPP       "%.6f"     // Format string for calibration factors
#define YAIPS_CALIP_FORMAT_DIST      "%.4f"     // Format string for calibration distances

/************************************************************************************
* Local variables for this window
*/

static  Fl_Double_Window *pMyToolWin;
static int IsOpen;                           // True if this window is open.
static int MyWinPosX  = IQE_GUI_NO_WINPOS_X, MyWinPosY = IQE_GUI_NO_WINPOS_Y; // last window position
static int MyWinSizeX = MYWIN_SIZE_X_DEFAULT, MyWinSizeY = MYWIN_SIZE_Y_DEFAULT; // last window size

// GUI elements
static Fl_Output       *pOutput_UPP_X;          // Units per pixel
static Fl_Output       *pOutput_UPP_Y;
static Fl_Check_Button *pCheck_do_XandY;       // Check button do x and y calibration
static Fl_Button       *pBut_Calibrate;        // Button calibrate
static Fl_Float_Input  *pFloat_DistX;          // Calibration distance
static Fl_Float_Input  *pFloat_DistY;
static IqeFl_Int_Input *pInt_DPI;
static Fl_Menu_Button  *pMBut_DPI;
static Fl_Radio_Round_Button *pRadio_Mode1, *pRadio_Mode2;

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

  // Global calibration variables
  { PREF_T_DOUBLE, "UnitsPerPixel_X",   "0.1", &YaIPS_Calib_UPP_X},
  { PREF_T_DOUBLE, "UnitsPerPixel_Y",   "0.1", &YaIPS_Calib_UPP_Y},
  { PREF_T_DOUBLE,   "Calib_Image_X",   "0.1", &YaIPS_Calib_Image_X},
  { PREF_T_DOUBLE,   "Calib_Image_Y",   "0.1", &YaIPS_Calib_Image_Y},
  { PREF_T_INT,         "Calib_Mode",     "0", &YaIPS_Calib_Mode},
  { PREF_T_INT,         "Calib_Unit",     "0", &YaIPS_Calib_Unit},
  { PREF_T_INT,          "Calib_DPI",    "96", &YaIPS_Calib_DPI},

  // Local variables
  { PREF_T_INT,    "Do_X_and_Y",          "1", &YaIPS_Calib_do_XandY},
  { PREF_T_FLOAT,  "Distance_X",       "10.0", &YaIPS_Calib_DistX},
  { PREF_T_FLOAT,  "Distance_Y",       "10.0", &YaIPS_Calib_DistX},
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&pMyToolWin), &MyWinPosX, &MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, 0,
                                               &IsOpen, IqeB_GUI_CalibrationWin, (Fl_Callback *)close_cb);

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyWinUpdate()
{
  int RedrawWindow, TempEnable;
  char TempString[ 256];

  RedrawWindow = false;                      // No redraw of window

  // To periodically updates first

  // Update enables

  TempEnable = YaIPS_Calib_Unit > YAIPS_CALIB_UNIT_PIXEL;  // No pixel values

  IqeB_GUI_WidgetActivate( pRadio_Mode1, TempEnable);
  IqeB_GUI_WidgetActivate( pRadio_Mode2, TempEnable);

  TempEnable = YaIPS_Calib_Mode == YAIPS_CALIB_MODE_DPI && YaIPS_Calib_Unit > YAIPS_CALIB_UNIT_PIXEL;  // Mode DPI and no pixel values

  IqeB_GUI_WidgetActivate( pInt_DPI, TempEnable);
  IqeB_GUI_WidgetActivate( pMBut_DPI, TempEnable);

  TempEnable = YaIPS_Calib_Mode == YAIPS_CALIB_MODE_IMAGE && YaIPS_Calib_Unit > YAIPS_CALIB_UNIT_PIXEL;  // Mode Image and no pixel values

  IqeB_GUI_WidgetActivate( pFloat_DistX, TempEnable);
  IqeB_GUI_WidgetActivate( pFloat_DistY, TempEnable);
  IqeB_GUI_WidgetActivate( pCheck_do_XandY, TempEnable);

  // Calibration button is only enabled if we make a distance measurement

  TempEnable =
      YaIPS_Calib_Mode == YAIPS_CALIB_MODE_IMAGE &&                          // Mode Image
      YaIPS_Calib_Unit > YAIPS_CALIB_UNIT_PIXEL &&                           // and no pixel values
      (YaIPS_BigImageDisp.Flags & YAIPS_IDISP_FLAG_DO_DISP_MODIFY) != 0 &&   // Have a valid image
      ! YaIPS_BigImageDisp.Plot3D_Active &&                                  // NO 3D plot active
      YaIPS_Main_Measured_Distance > 0 &&                                    // and have a measured distance
      ( YaIPS_BigImageDisp.ShowInfoMode == YAIPS_SHOW_INFO_2P_DIST_HOR ||    // and any of the distance measurements
        YaIPS_BigImageDisp.ShowInfoMode == YAIPS_SHOW_INFO_2P_DIST_VER);

  IqeB_GUI_WidgetActivate( pBut_Calibrate, TempEnable);

  // Recalculate calibration factors YaIPS_Calib_UPP_X and YaIPS_Calib_UPP_Y
  if( YaIPS_Calib_Recaclc_Factors() != 0) {        // Any of the calibration factors has changed

    if( YaIPS_BigImageDisp.pImage_Box != NULL) {  // Security test

      // Redraw big image window to reflect changed measured value
      YaIPS_BigImageDisp.pImage_Box->redraw();
    }
  }

  // Update GUI elements which use the unit string
  if( YaIPS_Calib_Unit <= YAIPS_CALIB_UNIT_PIXEL) {

    sprintf( TempString, LangStringLookup( "&GUI_Calibrate_LabelX_Pix=Factor X"));
  } else {

    sprintf( TempString, LangStringLookup( "&GUI_Calibrate_LabelX_Unit=Factor X [%s/Pixel]"), pYaIPS_Calib_Unit2String());
  }
  if( strcmp( TempString, pOutput_UPP_X->label()) != 0) {

    RedrawWindow = true;                      // NEED redraw of window

    pOutput_UPP_X->copy_label( TempString);
  }

  if( YaIPS_Calib_Unit <= YAIPS_CALIB_UNIT_PIXEL) {

    sprintf( TempString, LangStringLookup( "&GUI_Calibrate_LabelY_Pix=Factor Y"));
  } else {

    sprintf( TempString, LangStringLookup( "&GUI_Calibrate_LabelY_Unit=Factor Y [%s/Pixel]"), pYaIPS_Calib_Unit2String());
  }
  if( strcmp( TempString, pOutput_UPP_Y->label()) != 0) {

    RedrawWindow = true;                      // NEED redraw of window

    pOutput_UPP_Y->copy_label( TempString);
  }

  sprintf( TempString, YAIPS_CALIP_FORMAT_UPP, YaIPS_Calib_UPP_X);
  if( strcmp( TempString, pOutput_UPP_X->value()) != 0) {

    RedrawWindow = true;                      // NEED redraw of window

    pOutput_UPP_X->value( TempString);
  }

  sprintf( TempString, YAIPS_CALIP_FORMAT_UPP, YaIPS_Calib_UPP_Y);
  if( strcmp( TempString, pOutput_UPP_Y->value()) != 0) {

    RedrawWindow = true;                      // NEED redraw of window

    pOutput_UPP_Y->value( TempString);
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
 * IqeB_GUI_Float_SetValue_Callback
 *
 * Callback, set a float or double value
 */

static void IqeB_GUI_Float_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  void *pValue;
  double Value;
  Fl_Float_Input *pThis;

  pThis  = (Fl_Float_Input *)w;
  pValue = (float *)pValueArg;             // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pValue == NULL) {

    return;
  }

  // Set value

  Value  = atof( pThis->value());         // Get the value

  // Set Value to variable
  if( pValue == &YaIPS_Calib_UPP_X ||     // Assign to double variable
      pValue == &YaIPS_Calib_UPP_Y) {

    *(double *)pValue = Value;

  } else {                               // Assign to float variable

    *(float *)pValue = Value;
  }

  // ...

  if( YaIPS_BigImageDisp.pImage_Box != NULL) {  // Security test

    // Redraw big image window to reflect changed measured value
    YaIPS_BigImageDisp.pImage_Box->redraw();
  }

  MyWinUpdate();                               // Update the GUI
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

  if( *pValue != Value) {                  // Value is different

    *pValue = Value;                       // update the variable

    // ...

    if( YaIPS_BigImageDisp.pImage_Box != NULL) {  // Security test

      // Redraw big image window to reflect changed measured value
      YaIPS_BigImageDisp.pImage_Box->redraw();
    }

    MyWinUpdate();                               // Update the GUI
  }
}

/************************************************************************************
 * IqeB_GUI_DPI_Button_Callback
 *
 * Callback, Set one of the DPI presets
 */

static void IqeB_GUI_DPI_Button_Callback( Fl_Widget *w)
{
  int DPI_New;
  Fl_Menu_Button *pMenu_Button;

  pMenu_Button = (Fl_Menu_Button *)w;

  // Return a pointer to the last menu item that was picked
  const Fl_Menu_Item *m = pMenu_Button->mvalue();

  if( m != NULL) {       // A menu line was selected

    DPI_New = (int)(uintptr_t)(m->user_data_);

    if( DPI_New != YaIPS_Calib_DPI) {

      YaIPS_Calib_DPI = DPI_New;

      // Update DPI input field
      pInt_DPI->value( YaIPS_Calib_DPI);

      // ...

      if( YaIPS_BigImageDisp.pImage_Box != NULL) {  // Security test

        // Redraw big image window to reflect changed measured value
        YaIPS_BigImageDisp.pImage_Box->redraw();
      }

      MyWinUpdate();                               // Update the GUI
    }
  }
}

/************************************************************************************
 * IqeB_GUI_CBox_SetValue_Callback
 */

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

  // ...

  if( YaIPS_BigImageDisp.pImage_Box != NULL) {  // Security test

    // Redraw big image window to reflect changed measured value
    YaIPS_BigImageDisp.pImage_Box->redraw();
  }

  MyWinUpdate();                               // Update the GUI
}

/************************************************************************************
 * IqeB_GUI_But_Calibrate_Callback
 *
 * Do the calibration.
 *
 */

static void IqeB_GUI_But_Calibrate_Callback( Fl_Widget *w, void *pValueArg)
{

  // Check requirements
  if( YaIPS_Calib_Mode == YAIPS_CALIB_MODE_IMAGE &&                          // Mode Image
      YaIPS_Calib_Unit > YAIPS_CALIB_UNIT_PIXEL &&                           // and no pixel values
      (YaIPS_BigImageDisp.Flags & YAIPS_IDISP_FLAG_DO_DISP_MODIFY) != 0 &&   // Have a valid image
      ! YaIPS_BigImageDisp.Plot3D_Active &&                                  // and NO 3D plot active
      YaIPS_Main_Measured_Distance > 0 &&                                    // and have a measured distance
      ( YaIPS_BigImageDisp.ShowInfoMode == YAIPS_SHOW_INFO_2P_DIST_HOR ||    // and any of the distance measurements
        YaIPS_BigImageDisp.ShowInfoMode == YAIPS_SHOW_INFO_2P_DIST_VER)) {

    // Requirements are OK

  } else {

    // Requirements are NOT OK

    return;   // Do nothing
  }

  switch( YaIPS_BigImageDisp.ShowInfoMode) {     // Calculations depend from measurement mode

  case YAIPS_SHOW_INFO_2P_DIST_HOR:

    YaIPS_Calib_Image_X = YaIPS_Calib_DistX / YaIPS_Main_Measured_Distance;    // Calculate calibration factor X

    if( YaIPS_Calib_do_XandY) {                 // Have square pixel

      YaIPS_Calib_Image_Y = YaIPS_Calib_Image_X;     // Copy to calibration factor X
    }

    break;

  case YAIPS_SHOW_INFO_2P_DIST_VER:

    YaIPS_Calib_Image_Y = YaIPS_Calib_DistY / YaIPS_Main_Measured_Distance;    // Calculate calibration factor Y

    if( YaIPS_Calib_do_XandY) {                 // Have square pixel

      YaIPS_Calib_Image_X = YaIPS_Calib_Image_Y;     // Copy to calibration factor X
    }

    break;
  }

  // Copy to current valid calibration values

  YaIPS_Calib_UPP_X = YaIPS_Calib_Image_X;
  YaIPS_Calib_UPP_Y = YaIPS_Calib_Image_Y;

  // ...

  if( YaIPS_BigImageDisp.pImage_Box != NULL) {  // Security test

    // Redraw big image window to reflect changed measured value
    YaIPS_BigImageDisp.pImage_Box->redraw();
  }

  MyWinUpdate();                               // Update the GUI
}

/************************************************************************************
 * YaIPS_Calib_Mode_Callback
 *
 * Calibration mode setting will change
 */

static void YaIPS_Calib_Mode_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                       // get value to set

  if( YaIPS_Calib_Mode == Value) {                 // Value will not change

    return;                                        // Exit, nothing to do
  }

  YaIPS_Calib_Mode = Value;                        // Set new value

  // ...

  if( YaIPS_BigImageDisp.pImage_Box != NULL) {  // Security test

    // Redraw big image window to reflect changed measured value
    YaIPS_BigImageDisp.pImage_Box->redraw();
  }

  MyWinUpdate();                               // Update the GUI
}

/************************************************************************************
 * IqeB_GUI_Misc_SetValue_Callback
 *
 * This is usable for Fl_Valuator, Fl_Choice, Fl_Check_Button
 */

static void IqeB_GUI_Misc_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{

  if( w == NULL ||                         // security test
      pValueArg == NULL) {

    return;
  }

  if( pValueArg == &YaIPS_Calib_Unit) {    // Changed new unit

    // Fl_Choice

    Fl_Choice *pThis;
    int *pValue, NewValue;

    pThis  = (Fl_Choice *)w;
    pValue = (int *)pValueArg;             // get pointer to associated variable

    NewValue = pThis->value();             // Get new value

    if( *pValue != NewValue) {             // Value is different

      *pValue = NewValue;                  // Update the variable
    }
  }

  // ...

  if( YaIPS_BigImageDisp.pImage_Box != NULL) {  // Security test

    // Redraw big image window to reflect changed measured value
    YaIPS_BigImageDisp.pImage_Box->redraw();
  }

  MyWinUpdate();                               // Update the GUI
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
 * IqeB_GUI_CalibrationWin
 *
 * Open calibration window
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_CalibrationWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
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

  Fl_Box          *pTemp_Box;
  IqeFl_Int_Input *pTemp_Int;
  Fl_Output       *pOutputTemp;
  Fl_Float_Input  *pFloatTemp;
  Fl_Check_Button *pCheckTemp;
  //x/Fl_Input        *pInputTemp;
  Fl_Button       *pButtonTemp;
  Fl_Radio_Round_Button *pRadioButTemp;
  Fl_Choice       *pTemp_Choice;

  char TempString[ 256];

  int x, x1, y, y1, y2, xx1, xx2, yy, wWin;
  //x/int xx, hWin;

  //x/hWin = pMyToolWin->h();
  wWin = pMyToolWin->w();

  //
  // Group, top side
  //

  x = 4;
  y = 24;

  yy = 24;

  pTemp_Box = new Fl_Box( x, y, wWin - 8, yy * 2 + 14, LangStringLookup( "&GUI_Calibrate_Group1=Current calibration factors"));
  pTemp_Box->box( FL_DOWN_FRAME);
  pTemp_Box->align(FL_ALIGN_TOP_LEFT);     // align for label
  pTemp_Box->vertical_label_margin( 2);    // gap distance

  y1 = pTemp_Box->y() + 6;                 // Remember begin of this group

  y = pTemp_Box->y() + pTemp_Box->h() + 24;

  pTemp_Box = new Fl_Box( x, y, wWin - 8, yy * 5 + 4, LangStringLookup( "&GUI_Calibrate_Group2=Calibrate"));
  pTemp_Box->box( FL_DOWN_FRAME);
  pTemp_Box->align(FL_ALIGN_TOP_LEFT);     // align for label
  pTemp_Box->vertical_label_margin( 2);    // gap distance

  y2 = pTemp_Box->y() + 6;                 // Remember begin of this group

  // Draw top part

  x1 = x + 4;
  y = y1;

  xx1 = 196;
  xx2 = 100;

  pOutputTemp = new Fl_Output( x1 + xx1, y, xx2, yy);
  pOutputTemp->copy_label( "xxx");
  pOutputTemp->value( "xxx");
  pOutputTemp->color( YAIPS_COLOR_RONLY_BGND);
  pOutput_UPP_X = pOutputTemp;

  y += yy + 4;

  pOutputTemp = new Fl_Output( x1 + xx1, y, xx2, yy);
  pOutputTemp->copy_label( "xxx");
  pOutputTemp->value( "xxx");
  pOutputTemp->color( YAIPS_COLOR_RONLY_BGND);
  pOutput_UPP_Y = pOutputTemp;

  y += yy + 4;

  // Draw left bottom side

  x1 = x + 4;
  y = y2;

  xx1 =  80;

  pTemp_Box = new Fl_Box( x1, y, xx1, yy, LangStringLookup( "&GUI_Calibrate_Group20=Mode"));
  pTemp_Box->box( FL_NO_BOX);
  pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

  x1 += xx1 + 6;

  xx1 =  (wWin - 8 - x1 ) / 2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LangStringLookup( "&GUI_Calibrate_Group21=Image"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Calibrate_Group21a="
                         "A file or camera image is used for calibration."));
  pRadioButTemp->value( YaIPS_Calib_Mode == YAIPS_CALIB_MODE_IMAGE);
  pRadioButTemp->callback( YaIPS_Calib_Mode_Callback, (void *)YAIPS_CALIB_MODE_IMAGE);
  pRadio_Mode1 = pRadioButTemp;

  x1 += xx1 + 2;

  pRadioButTemp = new Fl_Radio_Round_Button( x1, y + 2, xx1 - 2, yy - 4, LangStringLookup( "&GUI_Calibrate_Group22=DPI"));
  pRadioButTemp->tooltip( LangStringLookup( "&GUI_Calibrate_Group22a="
                         "A DPI setting is used for calibration."));
  pRadioButTemp->value( YaIPS_Calib_Mode == YAIPS_CALIB_MODE_DPI);
  pRadioButTemp->callback( YaIPS_Calib_Mode_Callback, (void *)YAIPS_CALIB_MODE_DPI);
  pRadio_Mode2 = pRadioButTemp;

  y += yy + 6;
  x1 = x + 4;

  xx1 =  80;
  xx2 =  80;

  pTemp_Choice = new Fl_Choice( x1 + xx1, y, xx2, yy, LangStringLookup( "&GUI_Calibrate_Group23=Unit"));
  pTemp_Choice->tooltip( LangStringLookup( "&GUI_Calibrate_Group23a="
                                           "Select calibration unit."));
  pTemp_Choice->callback( IqeB_GUI_Misc_SetValue_Callback, &YaIPS_Calib_Unit);
  pTemp_Choice->menu_box( FL_BORDER_BOX);

  pTemp_Choice->add( LANGDEF_CALIB_UNIT_PIXEL);
  pTemp_Choice->add( LANGDEF_CALIB_UNIT_MM);
  pTemp_Choice->add( LANGDEF_CALIB_UNIT_CM);
  pTemp_Choice->add( LANGDEF_CALIB_UNIT_INCH);
  pTemp_Choice->value( YaIPS_Calib_Unit);

  x1 += xx1 + xx2 + 40;

  pTemp_Int = new IqeFl_Int_Input( x1, y, xx2 - yy, yy, LangStringLookup( "&GUI_Calibrate_Group24=DPI"));
  pTemp_Int->tooltip( LangStringLookup( "&GUI_Calibrate_Group24a="
                                        "Dots per inch.\n"
                                        "Used to convert mm or inch\n"
                                        "sizes into pixel sizes."));
  pTemp_Int->SetValue( YaIPS_Calib_DPI);
  pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &YaIPS_Calib_DPI);
  pTemp_Int->SetModifyData( YAIPS_CALIB_MIN_VAL_DPI, YAIPS_CALIB_MAX_VAL_DPI, 10, 1);
  pInt_DPI = pTemp_Int;

  x1 += xx2 - yy;

  pMBut_DPI = new Fl_Menu_Button( x1, y, yy, yy);
  pMBut_DPI->callback( IqeB_GUI_DPI_Button_Callback);
  pMBut_DPI->tooltip( LangStringLookup( "&GUI_Calibrate_Group25a="
                                          "Some typical DPI values."));

  pMBut_DPI->add( LangStringLookup( "&GUI_Calibrate_DPI_96=  96 Screen"), 0, NULL, (void *)(fl_intptr_t)( 96));
  pMBut_DPI->add( LangStringLookup( "&GUI_Calibrate_DPI_120=120"), 0, NULL, (void *)(fl_intptr_t)( 120));
  pMBut_DPI->add( LangStringLookup( "&GUI_Calibrate_DPI_150=150 Draft prints"), 0, NULL, (void *)(fl_intptr_t)( 150));
  pMBut_DPI->add( LangStringLookup( "&GUI_Calibrate_DPI_200=200"), 0, NULL, (void *)(fl_intptr_t)( 200));
  pMBut_DPI->add( LangStringLookup( "&GUI_Calibrate_DPI_254=254 10 Pixel = 1 mm"), 0, NULL, (void *)(fl_intptr_t)( 254));
  pMBut_DPI->add( LangStringLookup( "&GUI_Calibrate_DPI_300=300 High-quality prints"), 0, NULL, (void *)(fl_intptr_t)( 300));

  // Next line

  y += yy + 8;
  x1 = x + 4;

  y2 = y;                                          // Latch y begin

  pFloatTemp = new Fl_Float_Input( x1 + xx1, y, xx2, yy, LangStringLookup( "&GUI_Calibrate_Group30=Distance X"));
  pFloatTemp->type( FL_FLOAT_INPUT);
  pFloatTemp->tooltip( LangStringLookup( "&GUI_Calibrate_Group30a=Distance between calibration marks in X"));
  sprintf( TempString, YAIPS_CALIP_FORMAT_DIST, YaIPS_Calib_DistX);
  pFloatTemp->value( TempString);
  pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &YaIPS_Calib_DistX);
  pFloat_DistX = pFloatTemp;

  y += yy + 4;

  pFloatTemp = new Fl_Float_Input( x1 + xx1, y, xx2, yy, LangStringLookup( "&GUI_Calibrate_Group31=Distance Y"));
  pFloatTemp->type( FL_FLOAT_INPUT);
  pFloatTemp->tooltip( LangStringLookup( "&GUI_Calibrate_Group31a=Distance between calibration marks in Y"));
  sprintf( TempString, YAIPS_CALIP_FORMAT_DIST, YaIPS_Calib_DistY);
  pFloatTemp->value( TempString);
  pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &YaIPS_Calib_DistY);
  pFloat_DistY = pFloatTemp;

  y += yy + 4;

  // Draw right bottom side

  x1= xx1 + xx2 + 16;
  y = y2;                              // restore top y

  pCheckTemp = new Fl_Check_Button( x1, y, 64, yy, LangStringLookup( "&GUI_Calibrate_Group32=Pixel ² "));
  pCheckTemp->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label
  pCheckTemp->tooltip( LangStringLookup( "&GUI_Calibrate_Group32a="
                       "Set for square pixels. The calibration\n"
                       "factors then have the same value."));
  pCheckTemp->value( YaIPS_Calib_do_XandY);
  pCheckTemp->callback( IqeB_GUI_CBox_SetValue_Callback, &YaIPS_Calib_do_XandY);
  pCheck_do_XandY = pCheckTemp;

  y += yy + 4;

  pButtonTemp = new Fl_Button( x1, y, 128, yy, LangStringLookup( "&GUI_Calibrate_Group33=Calibrate"));
  pButtonTemp->tooltip( LangStringLookup( "&GUI_Calibrate_Group33a="
                        "To perform a calibration, a distance\n"
                        "measurement must be active in the main window.\n"
                        "Place the distance measurement on a\n"
                        "structure of known size.\n"
                        "If ‘Pixel ²’ is not set, X and Y must be\n"
                        "calibrated separately."));
  pButtonTemp->callback( IqeB_GUI_But_Calibrate_Callback, NULL);
  pBut_Calibrate = pButtonTemp;

  //
  // Layout end work
  //

  pMyToolWin->end();

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
