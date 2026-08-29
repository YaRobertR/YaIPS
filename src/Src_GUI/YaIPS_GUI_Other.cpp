/****************************************************************************

  YaIPS_GUI_Other.cpp

  Other image processing things.

  22.08.2025 RR: First edition of this file.

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
#define MY_WIN_ID     YAIPS_WIN_ID_OTHER          // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_OTHER         // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_Other_Title=Other"                // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "WinOther"            // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_Other_Tool  // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       221  //x/ YAIPS_WIN_SIZE_S1_X_MIN
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

/************************************************************************************
* Global variables for this window
*/

#define WIN_OTHER_FIFO_MAX   4      // Max size of FIFO

//-----------------------------------------------------------------------------------
// Manage multiple tool windows
//-----------------------------------------------------------------------------------

typedef struct {

  // Parameters
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

  // Support for image FIFO. The images are only allocated, if FIFO is selected.

  int FIFO_idx;                         // Next FIFO location to update
  Fl_RGB_Image *FIFO_img[ WIN_OTHER_FIFO_MAX]; // Allocated FIFO images.

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int Tab_Group_Selected;               // Number of last selected tab group.
  int SelectionType;                    // What selection to use

  // section: IFO
  int FIFO_Size;                        // Size of image FIFO

  // section: chroma key processing

  int Chroma_Color;                     // Chroma key color
  int Chroma_HueThres;                  // Chroma hue threshold
  int Chroma_HueFade;                   // Chroma hue fade transition
  int Chroma_SatThres;                  // Chroma saturation threshold
  int Chroma_SatFade;                   // Chroma saturation fade transition
  int Chroma_IDaThres;                  // Chroma intensity dark threshold
  int Chroma_IDaFade;                   // Chroma intensity dark fade transition
  int Chroma_IBrThres;                  // Chroma intensity bright threshold
  int Chroma_IBrFade;                   // Chroma intensity bright fade transition
  int Chroma_Flags;                     // Chroma optional processing steps
  int Chroma_Output;                    // Chroma type of output image
  int Chroma_BlendColor;                // Chroma the color used for 'YAIPS_RGB_CHROMA_OUT_COL_B'

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

  // Dialog is open
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

  // Hold last selected tab

  { PREF_T_INT,    "Group_Selected",  "0", &YaIPS_ToolData_info[0].Tab_Group_Selected},
  { PREF_T_INT,     "SelectionType",  "0", &YaIPS_ToolData_info[0].SelectionType},

  // section: IFO
  { PREF_T_INT,         "FIFO_Size",  "0", &YaIPS_ToolData_info[0].FIFO_Size},

  // section: chroma key processing
  { PREF_T_INT,      "Chroma_Color",   "2", &YaIPS_ToolData_info[0].Chroma_Color},
  { PREF_T_INT,   "Chroma_HueThres",  "30", &YaIPS_ToolData_info[0].Chroma_HueThres},
  { PREF_T_INT,    "Chroma_HueFade",  "10", &YaIPS_ToolData_info[0].Chroma_HueFade},
  { PREF_T_INT,   "Chroma_SatThres",  "20", &YaIPS_ToolData_info[0].Chroma_SatThres},
  { PREF_T_INT,    "Chroma_SatFade",  "10", &YaIPS_ToolData_info[0].Chroma_SatFade},
  { PREF_T_INT,   "Chroma_IDaThres",  "20", &YaIPS_ToolData_info[0].Chroma_IDaThres},
  { PREF_T_INT,    "Chroma_IDaFade",  "10", &YaIPS_ToolData_info[0].Chroma_IDaFade},
  { PREF_T_INT,   "Chroma_IBrThres",  "10", &YaIPS_ToolData_info[0].Chroma_IBrThres},
  { PREF_T_INT,    "Chroma_IBrFade",  "10", &YaIPS_ToolData_info[0].Chroma_IBrFade},
  { PREF_T_INT,      "Chroma_Flags",   "0", &YaIPS_ToolData_info[0].Chroma_Flags},
  { PREF_T_INT,     "Chroma_Output",   "0", &YaIPS_ToolData_info[0].Chroma_Output},
  { PREF_T_INT, "Chroma_BlendColor", "248", &YaIPS_ToolData_info[0].Chroma_BlendColor},

};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_OtherWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp);

//-----------------------------------------------------------------------------------
// Parameter dialog
//
// This is a modal dialog. Therefore we can use global variables to hold
// info about the data.
//-----------------------------------------------------------------------------------

// defines for function selection

#define YAIPS_SELECTION_FIFO        0       // Image FIFO
#define YAIPS_SELECTION_CHROMA_KEY  1       // Chroma key processing

#define YAIPS_SELECTION_BUTTON_MAX    (YAIPS_SELECTION_CHROMA_KEY + 1)  // Number of selection radio buttons

// ...

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolData;     // NOTE: Is used by all parameter dialog functions

static IqeFl_Tabs      *pTab_Groups;          // Point to tabulator GUI element
static Fl_Radio_Round_Button *SelectionButtons[ YAIPS_SELECTION_BUTTON_MAX]; // Table of filter buttons
static int SelectionType_Last;                // Catch filter change

// YAIPS_SELECTION_FIFO

static IqeFl_Int_Input *pInt_FIFO_Size;

// YAIPS_SELECTION_CHROMA_KEY

static Fl_Button       *pBut_Chroma_Color, *pBut_Chroma_BlendColor;    // Pointer to color button
static IqeFl_Int_Input *pInt_Chroma_HueThres,  *pInt_Chroma_SatThres, *pInt_Chroma_IDaThres, *pInt_Chroma_IBrThres; // Pointer to GUI input element
static IqeFl_Int_Input *pInt_Chroma_HueFade,  *pInt_Chroma_SatFade, *pInt_Chroma_IDaFade, *pInt_Chroma_IBrFade;     // Pointer to GUI input element
static Fl_Radio_Round_Button *pRadio_Chroma_Output_0, *pRadio_Chroma_Output_1, *pRadio_Chroma_Output_2, *pRadio_Chroma_Output_3, *pRadio_Chroma_Output_4;
static IqeFl_Check_Bit *pCheck_Bit_Chrom_Smootstep, *pCheck_Bit_Chrom_Despill_O, *pCheck_Bit_Chrom_Despill_I;

/************************************************************************************
 * FIFO support function.
 *
 */

#define FIFO_RESET       0       // Reset FIFO, also frees FIFO buffer
#define FIFO_CHECK_SIZE  1       // Reset FIFO, also frees FIFO buffer

static void FIFO_support( YaIPS_ToolData_info_t *pToolData, int WhatToDo, Fl_RGB_Image *pImgInput = NULL)
{
  int iFIFO;
  Fl_RGB_Image *pTempImg;

  switch( WhatToDo) {

  case FIFO_RESET:
  default:

    for( iFIFO = 0; iFIFO < WIN_OTHER_FIFO_MAX; iFIFO++) {

      if( pToolData->FIFO_img[ iFIFO] != NULL) {       // Have an image

        pToolData->FIFO_img[ iFIFO]->release();        // Release the image
        pToolData->FIFO_img[ iFIFO] = NULL;            // Also reset pointer to image
      }
    }

    pToolData->FIFO_idx = 0;

    break;

  case FIFO_CHECK_SIZE:

    if( pToolData->SelectionType != YAIPS_SELECTION_FIFO ||  // FIFO mode is NOT selected
        pImgInput == NULL) {                                 // or no input image

      FIFO_support( pToolData, FIFO_RESET);                  // Reset

      break;
    }

    // Clip FIFO size

    if( pToolData->FIFO_Size < 0) {

      pToolData->FIFO_Size = 0;

    }  else if( pToolData->FIFO_Size > WIN_OTHER_FIFO_MAX) {

      pToolData->FIFO_Size = WIN_OTHER_FIFO_MAX;
    }

    // Remove not used images

    for( iFIFO = pToolData->FIFO_Size; iFIFO < WIN_OTHER_FIFO_MAX; iFIFO++) {

      // Don't need this image

      pTempImg = pToolData->FIFO_img[ iFIFO];

      if( pTempImg != NULL) {                  // Have an image

        pToolData->FIFO_img[ iFIFO]->release();        // Release the image
        pToolData->FIFO_img[ iFIFO] = NULL;            // Also reset pointer to image
      }
    }

    // Check FIFO index

    if( pToolData->FIFO_idx >= pToolData->FIFO_Size) {

      pToolData->FIFO_idx = 0;
    }

    break;

  }  // end switch()

  return;
}

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{
  int i, ValThis, TempEnable;

  // Get last selected tab group

  pToolData->Tab_Group_Selected = pTab_Groups->GetTabGroup();

  // Update filter button
  // Current selection must be set

  // Update all buttons
  if( SelectionType_Last != pToolData->SelectionType) {                // Filter type has change

    SelectionType_Last = pToolData->SelectionType;

    for( i = 0; i < YAIPS_SELECTION_BUTTON_MAX; i++) {

      ValThis = SelectionButtons[ i]->value();

      if( ValThis != (i == pToolData->SelectionType)) {             // Not what we expected

        SelectionButtons[ i]->value( i == pToolData->SelectionType);   // Set value
        SelectionButtons[ i]->redraw();
      }
    }
  }

  // YAIPS_SELECTION_FIFO

  TempEnable = pToolData->SelectionType == YAIPS_SELECTION_FIFO;

  IqeB_GUI_WidgetActivate( pInt_FIFO_Size, TempEnable);

  // YAIPS_SELECTION_CHROMA_KEY

  TempEnable = pToolData->SelectionType == YAIPS_SELECTION_CHROMA_KEY;

  IqeB_GUI_WidgetActivate( pBut_Chroma_Color, TempEnable);
  IqeB_GUI_WidgetActivate( pInt_Chroma_HueThres, TempEnable);
  IqeB_GUI_WidgetActivate( pInt_Chroma_SatThres, TempEnable);
  IqeB_GUI_WidgetActivate( pInt_Chroma_IDaThres, TempEnable);
  IqeB_GUI_WidgetActivate( pInt_Chroma_IBrThres, TempEnable);
  IqeB_GUI_WidgetActivate( pInt_Chroma_HueFade, TempEnable);
  IqeB_GUI_WidgetActivate( pInt_Chroma_SatFade, TempEnable);
  IqeB_GUI_WidgetActivate( pInt_Chroma_IDaFade, TempEnable);
  IqeB_GUI_WidgetActivate( pInt_Chroma_IBrFade, TempEnable);
  IqeB_GUI_WidgetActivate( pRadio_Chroma_Output_0, TempEnable);
  IqeB_GUI_WidgetActivate( pRadio_Chroma_Output_1, TempEnable);
  IqeB_GUI_WidgetActivate( pRadio_Chroma_Output_2, TempEnable);
  IqeB_GUI_WidgetActivate( pRadio_Chroma_Output_3, TempEnable);
  IqeB_GUI_WidgetActivate( pRadio_Chroma_Output_4, TempEnable);
  IqeB_GUI_WidgetActivate( pBut_Chroma_BlendColor, TempEnable && pToolData->Chroma_Output == YAIPS_RGB_CHROMA_OUT_COL_B);
  IqeB_GUI_WidgetActivate( pCheck_Bit_Chrom_Smootstep, TempEnable);
  IqeB_GUI_WidgetActivate( pCheck_Bit_Chrom_Despill_O, TempEnable);
  IqeB_GUI_WidgetActivate( pCheck_Bit_Chrom_Despill_I, TempEnable);
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
 * YaIPS_Filter_Callback
 *
 * Filter will change
 */

static void YaIPS_Filter_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                       // get value to set

  if( pToolData->SelectionType == Value) {         // Value will not change

    return;                                        // Exit, nothing to do
  }

  pToolData->SelectionType = Value;                // Set new value

  pToolData->Input1_Change = 0;                    // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_Float_SetValue_Callback
 *
 * Callback, set a float or double value
 */

#ifdef use_again
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

  pToolData->Input1_Change = 0;                    // Force recalculation output
}
#endif

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

  pToolData->Input1_Change = 0;                    // Force recalculation output
}

/************************************************************************************
 * YaIPS_Chroma_Output_Callback
 *
 * Chroma output image type will change will change
 */

static void YaIPS_Chroma_Output_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                   // get value to set

  if( pToolData->Chroma_Output == Value) {     // Value will not change

    return;                                    // Exit, nothing to do
  }

  pToolData->Chroma_Output = Value;            // Set new value

  // Force recalculation of processing

  pToolData->Input1_Change = 0;                // Force recalculation output
}

/************************************************************************************
 * IqeB_GUI_CBox_SetValue_Callback
 */

#ifdef use_again
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

  pToolData->Input1_Change = 0;          // Force recalculation output
}
#endif

/************************************************************************************
 * IqeB_GUI_But_Color_SetValue_Callback
 */

static void IqeB_GUI_But_Color_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  Fl_Color ColorBefore, ColorAfter;
  Fl_Button *pThis;
  int *pValue;

  // ...

  pThis  = (Fl_Button *)w;
  pValue = (int *)pValueArg;               // get pointer to associated variable

  ColorBefore = *pValue;

  ColorAfter = IqeB_GUI_ColorChooser( ColorBefore);

  if( ColorBefore != ColorAfter) {

    *pValue = ColorAfter;

    pThis->color( ColorAfter);
    pThis->parent()->redraw();

    pToolData->Input1_Change = 0;                    // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_CBox_SetValue_Callback
 */

static void IqeB_GUI_Check_Bit_Callback( Fl_Widget *w, void *pValueArg)
{
  int *pValue, HasChanged;
  IqeFl_Check_Bit *pThis;

  pThis  = (IqeFl_Check_Bit *)w;
  pValue = (int *)pValueArg;             // get pointer to associated variable

  if( pThis == NULL ||                   // security test
      pValue == NULL) {

    return;
  }

  // Set value

  HasChanged = pThis->GetValue();      // Update the variable

  if( HasChanged) {                    // Value has changed

    pToolData->Input1_Change = 0;          // Force recalculation output
  }
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

  pMyParWin = new Fl_Window( xPos, yPos, 359 /*IQE_GUI_TOOLS_STD_WITDH*/, 164, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  SelectionType_Last = -1;         // Reset last filter type

  //
  //  GUI things
  //

  int x1, y, yy, xx1;
  //x/int xx1, xc;
  int yGroup;
  char TempBuffer[ 256];

  //x/Fl_Check_Button *pCheckTemp;
  Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  //x/IqeFl_Float_Input  *pFloatTemp;
  IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group,*pTemp_Group2;
  Fl_Button       *pTemp_Button;
  //x/Fl_Choice       *pTemp_Choice;
  Fl_Radio_Round_Button *pRadioButTemp;
  IqeFl_Check_Bit *pTemp_Check_Bit;

  x1  = 4;
  //x/xx1 = pMyParWin->w() - 16;
  //x/xx2 = xx1 / 2;
  //x/xc  = pMyToolWin->w() / 2;          // x center
  yy  = 20;

  y = 4;

  //
  // Tabs
  //

  pTemp_Tabs = new IqeFl_Tabs( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4);
  pTemp_Tabs->selection_color( YAIPS_COLOR_SELECTION);
  pTab_Groups = pTemp_Tabs;

  y += 26;

  //
  // Group 'FIFO'
  //

  yGroup = y;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Other_TabA1=FIFO"));

    y += 8;

    xx1 = (pMyParWin->w() - 8) / 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Other_TabA2=FIFO"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Other_TabA2a="
                            "Image FIFO (First In First Out)\n"
                            "This FIFO delays images."));

    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_SELECTION_FIFO);
    SelectionButtons[ YAIPS_SELECTION_FIFO] = pRadioButTemp;

    x1 += xx1;

    xx1 = 40;
    x1 = pMyParWin->w() - xx1 - 8;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Other_TabA3=Delay"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Other_TabA3a="
                        "Size of the FIFO or number of image delays.\n"
                        "0 = no delay."));
    pTemp_Int->SetValue( pToolData->FIFO_Size);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->FIFO_Size);
    pTemp_Int->SetModifyData( 0, WIN_OTHER_FIFO_MAX, 1, 0);
    pInt_FIFO_Size = pTemp_Int;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group 'XXX'
  //

  y = yGroup;

  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Other_TabB1=Chroma key"));

    y += 8;

    x1 = 4 + 4;

    xx1 = (pMyParWin->w() - 8) / 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Other_TabB1=Chroma key"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Other_TabB1a="
                                              "Chroma key processing.\n"
                                              "Generate an alpha mask by comparing\n"
                                              "it to a specific color."));

    pRadioButTemp->callback( YaIPS_Filter_Callback, (void *)YAIPS_SELECTION_CHROMA_KEY);
    SelectionButtons[ YAIPS_SELECTION_CHROMA_KEY] = pRadioButTemp;

    x1 += xx1 + 2;

    xx1 = 50;

    pTemp_Box = new Fl_Box( x1, y, xx1, yy, LangStringLookup( "&GUI_Other_TabB3=Color"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    x1 += xx1 + 2;

    xx1 = 34;

    pTemp_Button = new Fl_Button( x1, y, xx1, yy);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Other_TabB4a="
                                             "Press to change the color"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Button->color( pToolData->Chroma_Color);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &pToolData->Chroma_Color);
    pBut_Chroma_Color = pTemp_Button;

    // Next Line

    y += yy + 4;

    x1 = 4 + 4;

    xx1 = 40;
    x1 += 60;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Other_TabB5=Hue T"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Other_TabB5a="
                                          "Hue threshold.\n"
                                          "Default = 30."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Chroma_HueThres);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Chroma_HueThres);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_Chroma_HueThres = pTemp_Int;

    x1 += xx1 + 4;

    xx1 = 40;
    x1 += 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Other_TabB6=F"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Other_TabB6a="
                                          "Hue fade out range.\n"
                                          "0 = off. Default = 10."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Chroma_HueFade);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Chroma_HueFade);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_Chroma_HueFade = pTemp_Int;

    x1 += xx1 + 2;

    x1 += 8;               // Separate group of input elements

    xx1 = 40;
    x1 += 60;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Other_TabB7=Sat T"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Other_TabB7a="
                                          "Saturation threshold.\n"
                                          "0 = off. Default = 20."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Chroma_SatThres);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Chroma_SatThres);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_Chroma_SatThres = pTemp_Int;

    x1 += xx1 + 4;

    xx1 = 40;
    x1 += 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Other_TabB8=F"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Other_TabB8a="
                                          "Saturation fade out range.\n"
                                          "0 = off. Default = 10."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Chroma_SatFade);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Chroma_SatFade);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_Chroma_SatFade = pTemp_Int;

    // Next Line

    y += yy + 4;

    x1 = 4 + 4;

    xx1 = 40;
    x1 += 60;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Other_TabB9=IDa T"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Other_TabB9a="
                                          "Dark intensity threshold.\n"
                                          "0 = off. Default = 20."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Chroma_IDaThres);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Chroma_IDaThres);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_Chroma_IDaThres = pTemp_Int;

    x1 += xx1 + 4;

    xx1 = 40;
    x1 += 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Other_TabB10=F"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Other_TabB10a="
                                          "Dark intensity fade out range.\n"
                                          "0 = off. Default = 10."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Chroma_IDaFade);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Chroma_IDaFade);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_Chroma_IDaFade = pTemp_Int;

    x1 += xx1 + 2;

    x1 += 8;               // Separate group of input elements

    xx1 = 40;
    x1 += 60;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Other_TabB11=IBr T"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Other_TabB11a="
                                          "Bright intensity threshold.\n"
                                          "0 = off. Default = 10."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Chroma_IBrThres);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Chroma_IBrThres);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_Chroma_IBrThres = pTemp_Int;

    x1 += xx1 + 4;

    xx1 = 40;
    x1 += 12;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx1, yy, LangStringLookup( "&GUI_Other_TabB12=F"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Other_TabB12a="
                                          "Bright intensity fade out range.\n"
                                          "0 = off. Default = 10."));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    sprintf( TempBuffer, "%d", pToolData->Chroma_IBrFade);
    pTemp_Int->value( TempBuffer);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Chroma_IBrFade);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pInt_Chroma_IBrFade = pTemp_Int;

    // Next Line

    y += yy + 4;

    x1 = 4 + 4;

    xx1 = pMyParWin->w() - x1 - 4;

    pTemp_Group2 = new Fl_Group( x1, y, xx1, yy);  // Group around the radio buttons
    pTemp_Group2->align( FL_ALIGN_INSIDE | FL_ALIGN_TOP_LEFT);
    pTemp_Group2->vertical_label_margin( 3);

    xx1 = 60;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Other_TabB21=Inp+A"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Other_TabB21a="
                                              "Output image is processed input\n"
                                              "image + Alpha mask."));
    pRadioButTemp->callback( YaIPS_Chroma_Output_Callback, (void *)YAIPS_RGB_CHROMA_OUT_INP_A);
    pRadioButTemp->value( pToolData->Chroma_Output == YAIPS_RGB_CHROMA_OUT_INP_A);   // Set value
    pRadio_Chroma_Output_0 = pRadioButTemp;

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Other_TabB22=Alpha"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Other_TabB22a="
                                              "Output image is alpha mask."));
    pRadioButTemp->callback( YaIPS_Chroma_Output_Callback, (void *)YAIPS_RGB_CHROMA_OUT_ALPHA);
    pRadioButTemp->value( pToolData->Chroma_Output == YAIPS_RGB_CHROMA_OUT_ALPHA);   // Set value
    pRadio_Chroma_Output_1 = pRadioButTemp;

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Other_TabB23=Overl"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Other_TabB23a="
                                              "Overlay onto background.\n"
                                              "Input image 1 is the overlay.\n"
                                              "Input Image 2 is the background."));
    pRadioButTemp->callback( YaIPS_Chroma_Output_Callback, (void *)YAIPS_RGB_CHROMA_OUT_BLEND);
    pRadioButTemp->value( pToolData->Chroma_Output == YAIPS_RGB_CHROMA_OUT_BLEND);   // Set value
    pRadio_Chroma_Output_2 = pRadioButTemp;

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Other_TabB24=Inp-A"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Other_TabB24a="
                                              "Output image is processed input\n"
                                              "image (without alpha)."));
    pRadioButTemp->callback( YaIPS_Chroma_Output_Callback, (void *)YAIPS_RGB_CHROMA_OUT_INP_P);
    pRadioButTemp->value( pToolData->Chroma_Output == YAIPS_RGB_CHROMA_OUT_INP_P);   // Set value
    pRadio_Chroma_Output_3 = pRadioButTemp;

    x1 += xx1;
    x1 += 2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx1 - 2, yy, LangStringLookup( "&GUI_Other_TabB25=Color"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Other_TabB25a="
                                              "Output image is processed input\n"
                                              "image alpha blended to color."));
    pRadioButTemp->callback( YaIPS_Chroma_Output_Callback, (void *)YAIPS_RGB_CHROMA_OUT_COL_B);
    pRadioButTemp->value( pToolData->Chroma_Output == YAIPS_RGB_CHROMA_OUT_COL_B);   // Set value
    pRadio_Chroma_Output_4 = pRadioButTemp;

    x1 += xx1;
    x1 += 2;

    xx1 = 32;

    pTemp_Button = new Fl_Button( x1, y, xx1, yy);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Other_TabB26a="
                                             "Color used for blending.\n"
                                             "Press to change the color"));
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Button->color( pToolData->Chroma_BlendColor);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &pToolData->Chroma_BlendColor);
    pBut_Chroma_BlendColor = pTemp_Button;

    // End of the group around the radio buttons
    pTemp_Group2->end();

    // Next Line

    y += yy + 4;

    x1 = 4 + 4;

    xx1 = 96;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, xx1, yy,
                                           &pToolData->Chroma_Flags, YAIPS_RGB_CHROMA_FLAG_SMOOTHSTEP,
                                           LangStringLookup( "&GUI_Other_TabB30=Smoothstep"));
    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Other_TabB30a="
                                                 "Apply smoothstep processing.\n"
                                                 "Improves edge transitions."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &pToolData->Chroma_Flags);

    pCheck_Bit_Chrom_Smootstep = pTemp_Check_Bit;

    x1 += xx1 + 4;

    xx1 = 88;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, xx1, yy,
                                           &pToolData->Chroma_Flags, YAIPS_RGB_CHROMA_FLAG_DESPILL_O,
                                           LangStringLookup( "&GUI_Other_TabB31=Despill Out"));
    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Other_TabB31a="
                                                "Removes color residue from the outside\n"
                                                "of the mask and the edges."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &pToolData->Chroma_Flags);

    pCheck_Bit_Chrom_Despill_O = pTemp_Check_Bit;

    x1 += xx1 + 4;

    xx1 = 88;

    pTemp_Check_Bit = new IqeFl_Check_Bit( x1, y, xx1, yy,
                                           &pToolData->Chroma_Flags, YAIPS_RGB_CHROMA_FLAG_DESPILL_I,
                                           LangStringLookup( "&GUI_Other_TabB32=Despill In"));
    pTemp_Check_Bit->tooltip( LangStringLookup( "&GUI_Other_TabB32a="
                                                "Removes color residue from the inside\n"
                                                "of the mask."));
    pTemp_Check_Bit->callback( IqeB_GUI_Check_Bit_Callback, &pToolData->Chroma_Flags);

    pCheck_Bit_Chrom_Despill_I = pTemp_Check_Bit;

    // Finish things for this group

    pTemp_Group->end();

  // finish up

  pTemp_Tabs->end();

  pTemp_Tabs->SetTabGroup( pToolData->Tab_Group_Selected);    // Select tab group from last session

  // finish up

  pMyParWin->end();
  pMyParWin->set_modal();
  pMyParWin->callback( close_Par_cb, &pMyParWin);
  pMyParWin->show();

  // Hack: Add close button to window caption
  YaIPS_DialogAddCloseButton( pMyParWin);

  // Add idle action for this window

  #ifdef YAIPS_IDLE_CALLBACK_USE  // Use the idle callbacks in tool windows
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

  // Create the window

  CLASS_WIN_TOOL( int X, int Y, int W, int H, const char *l, int iToolDataArg) : Fl_Double_Window( X, Y, W, H, l)
  {
    YaIPS_ToolData_info_t *pToolData;
    Fl_Button *pTemp_Button;

    iToolData = iToolDataArg;                    // Index of info data element, see YaIPS_ToolData_info
    pToolData = YaIPS_ToolData_info + iToolData;  // Point to info data, user data is index to info data

    // Initialize some data

    memset( &pToolData->YaIPS_ImageDisp, 0, sizeof( Fl_YaIPS_ImageDisp_t)); // Zero data

    pToolData->Input1_Change = 0;                // Reset image change check
    pToolData->Input2_Change = 0;                // Reset image change check

    // ...

    Fl_Group *pGUI_GroupTopSide;                 // Top side of window
    Fl_Box   *pTemp_Box;
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
    pTemp_Box = new Fl_Box( x1, y + yy - yy2 - yy2 + 1, xx, yy2, LANGDEF_IMGSEL_PDS_1);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);     // align for label

    // 2. Input element
    pTemp_Box = new Fl_Box( x1, y + yy - yy2, xx, yy2, LANGDEF_IMGSEL_PDS_2);
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

    // Pull down buttons

    xx = yy - 10;

    // 1. Input element
    pBut_Input1 = new Fl_Button( x1, y + yy - yy2 - yy2 + 1, xx, yy2, "@2>");
    pBut_Input1->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pBut_Input1->tooltip( LangStringLookup( "&GUI_Combine_Tools2a=Select 1. input image"));
    pBut_Input1->labelcolor( YAIPS_BCOL_BUTTON);
    pBut_Input1->box( FL_BORDER_BOX);

    // 2. Input element
    pBut_Input2 = new Fl_Button( x1, y + yy - yy2, xx, yy2, "@2>");
    pBut_Input2->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pBut_Input2->tooltip( LangStringLookup( "&GUI_Combine_Tools3a=Select 2. input image"));
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
    pToolData->YaIPS_ImageDisp.pImage_Box->pDrawAfterCallback  = YaIPS_ImageDispDrawAfter_cb;  // Draw after callback
    pToolData->YaIPS_ImageDisp.pImage_Box->DrawCallbackArg1    = &pToolData->YaIPS_ImageDisp;  /// Pointer to Fl_YaIPS_ImageDisp_t
    pToolData->YaIPS_ImageDisp.pImage_Box->DrawCallbackArg2    = pToolData;                    // Optional pointer to ToolData

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

  // Remove FIFO images

  FIFO_support( pToolData, FIFO_RESET);

  // ...

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

      pToolData->Input1_Change = 0;                            // Force recalculation output
    }

  } else if( w == pMyToolWin->pBut_Input2) {                   // Select 1. input

    int WinIdNr_Before;

    WinIdNr_Before = pToolData->Input2_WinIdNr;

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + iToolData, &pToolData->Input2_WinIdNr, pMyToolWin->pBut_Input1, pMyToolWin->pBox_Input2);

    if( WinIdNr_Before != pToolData->Input2_WinIdNr) {         // Image source selection as changed

      pToolData->Input2_Change = 0;                            // Force recalculation output
    }

  } else if( w == pMyToolWin->pGUI_Parameter) {                // Open parameter dialog

    YaIPS_GUI_ParameterWin( pMyToolWin->x() + 16, pMyToolWin->y() + 16, iToolData);
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
  int TempEnable;

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

  if( DoEnable == 2) {                                                      // Was periodically updates only

    return;
  }

  // ...

  IqeB_GUI_WidgetActivate( pMyToolWin->pGUI_Img_ShowOnBig,
                             DoEnable &&                                     // Enable GUI elements
                             pToolData->YaIPS_ImageDisp.pImage_Img != NULL); // and have an image loaded

  // Check input image and visualize state

  // Check for enable of frist image

  IqeB_GUI_WidgetActivate( pMyToolWin->pBox_Input1, DoEnable);               // Enable GUI elements
  IqeB_GUI_WidgetActivate( pMyToolWin->pBut_Input1, DoEnable);               // Enable GUI elements

  YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr, pMyToolWin->pBox_Input1);

  // Check for enable of second image

  TempEnable = DoEnable && pToolData->SelectionType == YAIPS_SELECTION_CHROMA_KEY && pToolData->Chroma_Output == YAIPS_RGB_CHROMA_OUT_BLEND;

  IqeB_GUI_WidgetActivate( pMyToolWin->pBox_Input2, TempEnable);             // Enable GUI elements
  IqeB_GUI_WidgetActivate( pMyToolWin->pBut_Input2, TempEnable);             // Enable GUI elements

  if( TempEnable) {

    YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input2_WinIdNr, pMyToolWin->pBox_Input2);

  } else {

    YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, -1, pMyToolWin->pBox_Input2);
  }

  // ...

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

    int Input1_Check, Input1_ImageChanged, Input2_Check, Input2_ImageChanged, ForceUpdate;
    Fl_RGB_Image *pImgIn1, *pImgIn2;

    ForceUpdate = false;                         // Preset: NO Force update of output image

    // Check input image and visualize state
    Input1_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr,
                                            NULL, &pImgIn1, &Input1_ImageChanged);

    // Need second input image

    if( pToolData->SelectionType == YAIPS_SELECTION_CHROMA_KEY && pToolData->Chroma_Output == YAIPS_RGB_CHROMA_OUT_BLEND) {

      Input2_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input2_WinIdNr,
                                              NULL, &pImgIn2, &Input2_ImageChanged);

      if( Input2_Check != 0) {      // Image is not valid

        // Image is not valid
        Input2_Check = 0;
        Input2_ImageChanged = 0;
        pToolData->Input2_Change = 0;
        pImgIn2 = NULL;

      }
    } else {

      // Image is not valid
      Input2_Check = 0;
      Input2_ImageChanged = 0;
      pToolData->Input2_Change = 0;
      pImgIn2 = NULL;
    }

    if( Input1_Check != 0) {                                 // Input image is not valid

      // Empty a display image
      YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);

    } else
      if( Input1_ImageChanged != pToolData->Input1_Change ||  // Image counts are different
          Input2_ImageChanged != pToolData->Input2_Change) {

      pToolData->Input1_Change = Input1_ImageChanged;        // Image is processed
      pToolData->Input2_Change = Input2_ImageChanged;        // Image is processed

      // Do the image processing

      ierr = 0;                                              // Reset error
      errstring = NULL;                                      // Reset error string

      FIFO_support( pToolData, FIFO_CHECK_SIZE, pImgIn1);

      YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, FL_GREEN, FL_BLACK);  // Empty info string

      switch( pToolData->SelectionType) {

      case YAIPS_SELECTION_FIFO:
      default:

        if( pToolData->FIFO_Size <= 0) {               // FIFO size is zero

          YaIPS_RGB_CopyImg( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1);  // Simply copy image

        } else {                                           // FIFO is filled

          if( pToolData->FIFO_idx >= pToolData->FIFO_Size) {

            pToolData->FIFO_idx = 0;
          }

          YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, FL_GREEN, FL_BLACK, "FIFO %d. / %d", pToolData->FIFO_idx + 1, pToolData->FIFO_Size);

          // Output current slot

          if( pToolData->FIFO_img[ pToolData->FIFO_idx] == NULL) {  // Slot is empty

            ierr = 4711;                                            // FIFO entry is without image

          } else {

            YaIPS_RGB_CopyImg( &pToolData->YaIPS_ImageDisp.pImage_Img, pToolData->FIFO_img[ pToolData->FIFO_idx]);  // Copy image
          }

          // Replace with new input image

          YaIPS_RGB_CopyImg( pToolData->FIFO_img + pToolData->FIFO_idx, pImgIn1);  // Simply copy image

          pToolData->FIFO_idx += 1;             // Point to next slot
        }

        break;

      case YAIPS_SELECTION_CHROMA_KEY:
        {

          ierr = YaIPS_RGB_ChromaKey( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pToolData->Chroma_Color,
                                      pToolData->Chroma_HueThres, pToolData->Chroma_HueFade, pToolData->Chroma_SatThres, pToolData->Chroma_SatFade,
                                      pToolData->Chroma_IDaThres, pToolData->Chroma_IDaFade, pToolData->Chroma_IBrThres, pToolData->Chroma_IBrFade,
                                      pToolData->Chroma_Flags, pToolData->Chroma_Output, pImgIn2, pToolData->Chroma_BlendColor);

          if( ierr == 0) {       // Is OK

            YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, FL_GREEN, FL_BLACK, LangStringLookup( "&GUI_Other_Chroma_OK=Chroma Key"));

          } else if( ierr > 0) { // Source image is no color image

            YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, FL_GREEN, FL_BLACK, LangStringLookup( "&GUI_Other_Chroma_ErrCol=Chroma Key: No color image"));

          } else {

            YaIPS_ImageDispStrInfo( &pToolData->YaIPS_ImageDisp, FL_GREEN, FL_BLACK, LangStringLookup( "&GUI_Other_Chroma_ErrOther=Chroma Key: Error"));
          }

        }
        break;
      } // end switch

      // Has a valid output image

      if( ierr == 0) {                                  // Have a result image

        YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, (char *)MY_WIN_GUI_NAME);

        YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp); // Reset error message

        ForceUpdate = true;                             // Force update of output image

      } else if( ierr == 4711) {                        // FIFO entry was without image

        // Empty display image
        YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp, true);

        YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp); // Reset error message

      } else {                                          // Processing error

        pToolData->Input1_Change = 0;                   // Force recalculation output

        // Empty display image
        YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);

        if( errstring != NULL) {                           // Have a error message
          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp, errstring);
        } else {
          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp, LANGDEF_ERROR_CODE, ierr);
        }
      }
    }

    // ...

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
 * IqeB_GUI_OtherWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_OtherWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
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

  pToolData->IsOpen = true;           // Flag image as open

  // Reset FIFO images

  FIFO_support( pToolData, FIFO_RESET);

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
 * IqeB_GUI_OtherWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_OtherWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

	  IqeB_GUI_OtherWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  IqeB_GUI_OtherWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


