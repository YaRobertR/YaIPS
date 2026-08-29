/****************************************************************************

  YaIPS_GUI_Color.cpp

  Color processing.

  09.06.2025 RR: First edition of this file.

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
#define MY_WIN_ID     YAIPS_WIN_ID_COLOR         // Source specific windows ID
#define MY_WIN_MAX    YAIPS_WIN_MAX_COLOR        // Number of windows for this window type
#define MY_WIN_GUI_LD_NAME  "&GUI_Color_Title=Color"                // Language string used for GUI Name
#define MY_WIN_GUI_NAME     LangStringLookup( MY_WIN_GUI_LD_NAME)   // Name used for the windows caption
#define MY_WIN_PREF_NAME  "Win_Color"            // Name used for the preference data
#define CLASS_WIN_TOOL  YaIPS_Class_Color_Tool   // Use this as class name for the window class

// define for window sizes

#define MYWIN_SIZE_X_MIN       246
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

  // Used for intern data management

  Fl_YaIPS_ImageDisp_t YaIPS_ImageDisp;   // Info image output

  // Used to catch a change of the input image
  int Input1_Change;                    // Last processed 'ImageChanged' from input image

  int InputA_Change;                    // For mix color channels. Last processed alpha image.
  int InputG_Change;                    // For mix color channels. Last processed green image.
  int InputB_Change;                    // For mix color channels. Last processed blue image.

  Fl_YaIPS_ColMod_t TempLUT;            // Last LUT used for color processing.

  //
  // Parameter Dialog
  //

  int MyParPosX, MyParPosY;             // last window position

  int Tab_Group_Selected;               // Number of last selected tab group.
  int ColorType;                        // What color processing to use

  // LUT table
  int LUT_Table;                        // > 0 False color table type
  int LUT_Invert;                       // Flag: Invert color

  // Gain + offset change
  int GainCh1FA;                        // Gain change % checkbox 'one for all'. Use one value for all three channels.
  int OffsetR;                          // Offset change for red color channel
  int OffsetG;                          // Offset change for green color channel
  int OffsetB;                          // Offset change for blue color channel
  int GainChR;                          // Gain change % for red color channel
  int GainChG;                          // Gain change % for green color channel
  int GainChB;                          // Gain change % for blue color channel

  // Gamma
  int   Gamma1FA;                       // Gamma checkbox 'one for all'. Use one value for all three channels.
  float GammaR;                         // Gamma for red color channel
  float GammaG;                         // Gamma for green color channel
  float GammaB;                         // Gamma for blue color channel

  // Contrast
  int Contrast1in;                      // Contrast 1. point in value
  int Contrast1out;                     // Contrast 1. point out value
  int Contrast2in;                      // Contrast 2. point in value
  int Contrast2out;                     // Contrast 2. point out value

  // Duotone
  unsigned int Duotone_Br_Color;        // Duotone bright color
  unsigned int Duotone_Da_Color;        // Duotone dark color
  int Duotone_contrast;                 // Contrast range -100 ... 100 (0 = no change)
  int Duotone_brightness;               // Brightness range -100 ... 100 (0 = no change)
  int Duotone_RGB_Fade;                 // Used for color images only.
                                        // Range = 0 ... 100. 0 = use Color pixels. 100 = use BW pixels.

  // Matrix
  int R_Offset, G_Offset, B_Offset;     // Offsets for the three color channels
  float R_MultR, R_MultG, R_MultB;      // Multiplier for the red channel
  float G_MultR, G_MultG, G_MultB;      // Multiplier for the green channel
  float B_MultR, B_MultG, B_MultB;      // Multiplier for the blue channel

  // IHS changes
  int IHS_IntAdjust;                    // Intensity change %
  int IHS_HueAdjust;                    // Hue change %
  int IHS_SatAdjust;                    // Saturation change %

  // Tab group 'Brightness correction'

  int BC_TargetR, BC_TargetG, BC_TargetB;       // Brightness target values
  int BC_AOI_Teach;                             // Change AOI with mouse
  Fl_YaIPS_AOI_t BC_AOI;                        // Measurement AOI

  // Mix color channels

  int Mix_In_A_WinIdNr;                         // Window ID nr for alpha channel
  int Mix_In_G_WinIdNr;                         // Window ID nr for green channel
  int Mix_In_B_WinIdNr;                         // Window ID nr for blue channel

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

  //
  // Parameter Dialog
  //

  { PREF_T_INT,    "MyParPosX",  IQE_GUI_NO_WINPOS_X_STRING, &YaIPS_ToolData_info[0].MyParPosX }, // NOTE: values will be clipped against MYWIN_SIZE_X_MIN / MYWIN_SIZE_Y_MIN
  { PREF_T_INT,    "MyParPosY",  IQE_GUI_NO_WINPOS_Y_STRING, &YaIPS_ToolData_info[0].MyParPosY },

  // Hold last selected tab

  { PREF_T_INT,    "Group_Selected",   "0", &YaIPS_ToolData_info[0].Tab_Group_Selected},
  { PREF_T_INT,         "ColorType",   "0", &YaIPS_ToolData_info[0].ColorType},

  // Group 'LUT'
  { PREF_T_INT,         "LUT_Table",   "0", &YaIPS_ToolData_info[0].LUT_Table},
  { PREF_T_INT,        "LUT_Invert",   "0", &YaIPS_ToolData_info[0].LUT_Invert},


  // Gain + offset change
  { PREF_T_INT,         "GainCh1FA",   "0", &YaIPS_ToolData_info[0].GainCh1FA},
  { PREF_T_INT,           "OffsetR",   "0", &YaIPS_ToolData_info[0].OffsetR},
  { PREF_T_INT,           "OffsetG",   "0", &YaIPS_ToolData_info[0].OffsetG},
  { PREF_T_INT,           "OffsetB",   "0", &YaIPS_ToolData_info[0].OffsetB},
  { PREF_T_INT,           "GainChR",   "0", &YaIPS_ToolData_info[0].GainChR},
  { PREF_T_INT,           "GainChG",   "0", &YaIPS_ToolData_info[0].GainChG},
  { PREF_T_INT,           "GainChB",   "0", &YaIPS_ToolData_info[0].GainChB},

  // Gamma
  { PREF_T_INT,          "Gamma1FA",   "0", &YaIPS_ToolData_info[0].Gamma1FA},
  { PREF_T_FLOAT,          "GammaR", "1.0", &YaIPS_ToolData_info[0].GammaR},
  { PREF_T_FLOAT,          "GammaG", "1.0", &YaIPS_ToolData_info[0].GammaG},
  { PREF_T_FLOAT,          "GammaB", "1.0", &YaIPS_ToolData_info[0].GammaB},

  // Contrast
  { PREF_T_INT,       "Contrast1in",   "0", &YaIPS_ToolData_info[0].Contrast1in},
  { PREF_T_INT,      "Contrast1out",   "0", &YaIPS_ToolData_info[0].Contrast1out},
  { PREF_T_INT,       "Contrast2in", "255", &YaIPS_ToolData_info[0].Contrast2in},
  { PREF_T_INT,      "Contrast2out", "255", &YaIPS_ToolData_info[0].Contrast2out},

  // Duotone
  { PREF_T_INT,  "Duotone_Br_Color",   "6", &YaIPS_ToolData_info[0].Duotone_Br_Color},
  { PREF_T_INT,  "Duotone_Da_Color",  "11", &YaIPS_ToolData_info[0].Duotone_Da_Color},
  { PREF_T_INT,  "Duotone_contrast",   "0", &YaIPS_ToolData_info[0].Duotone_contrast},
  { PREF_T_INT,"Duotone_brightness",   "0", &YaIPS_ToolData_info[0].Duotone_brightness},
  { PREF_T_INT  ,"Duotone_RGB_Fade",   "0", &YaIPS_ToolData_info[0].Duotone_RGB_Fade},

  // Matrix
  { PREF_T_INT,          "R_Offset",   "0", &YaIPS_ToolData_info[0].R_Offset},
  { PREF_T_INT,          "G_Offset",   "0", &YaIPS_ToolData_info[0].G_Offset},
  { PREF_T_INT,          "B_Offset",   "0", &YaIPS_ToolData_info[0].B_Offset},
  { PREF_T_FLOAT,         "R_MultR", "1.0", &YaIPS_ToolData_info[0].R_MultR},
  { PREF_T_FLOAT,         "R_MultG", "0.0", &YaIPS_ToolData_info[0].R_MultG},
  { PREF_T_FLOAT,         "R_MultB", "0.0", &YaIPS_ToolData_info[0].R_MultB},
  { PREF_T_FLOAT,         "G_MultR", "0.0", &YaIPS_ToolData_info[0].G_MultR},
  { PREF_T_FLOAT,         "G_MultG", "1.0", &YaIPS_ToolData_info[0].G_MultG},
  { PREF_T_FLOAT,         "G_MultB", "0.0", &YaIPS_ToolData_info[0].G_MultB},
  { PREF_T_FLOAT,         "B_MultR", "0.0", &YaIPS_ToolData_info[0].B_MultR},
  { PREF_T_FLOAT,         "B_MultG", "0.0", &YaIPS_ToolData_info[0].B_MultG},
  { PREF_T_FLOAT,         "B_MultB", "1.0", &YaIPS_ToolData_info[0].B_MultB},

  // IHS
  { PREF_T_INT,     "IHS_IntAdjust",   "0", &YaIPS_ToolData_info[0].IHS_IntAdjust},
  { PREF_T_INT,     "IHS_HueAdjust",   "0", &YaIPS_ToolData_info[0].IHS_HueAdjust},
  { PREF_T_INT,     "IHS_SatAdjust",   "0", &YaIPS_ToolData_info[0].IHS_SatAdjust},

  // Brightness correction
  { PREF_T_INT,        "BC_TargetR", "200", &YaIPS_ToolData_info[0].BC_TargetR},
  { PREF_T_INT,        "BC_TargetG", "200", &YaIPS_ToolData_info[0].BC_TargetG},
  { PREF_T_INT,        "BC_TargetB", "200", &YaIPS_ToolData_info[0].BC_TargetB},
  { PREF_T_INT,        "BC_TargetB", "200", &YaIPS_ToolData_info[0].BC_TargetB},
  { PREF_T_INT,      "BC_AOI_Teach",   "0", &YaIPS_ToolData_info[0].BC_AOI_Teach},
  { PREF_T_INT,          "BC_AOI_X",   "0", &YaIPS_ToolData_info[0].BC_AOI.XPos},
  { PREF_T_INT,          "BC_AOI_Y",   "0", &YaIPS_ToolData_info[0].BC_AOI.YPos},
  { PREF_T_INT,         "BC_AOI_XX",  "30", &YaIPS_ToolData_info[0].BC_AOI.XSize},
  { PREF_T_INT,         "BC_AOI_YY",  "30", &YaIPS_ToolData_info[0].BC_AOI.YSize},

  // Mix color channels
  { PREF_T_INT,    "Mix_In_A_WinIdNr", "-1", &YaIPS_ToolData_info[0].Mix_In_A_WinIdNr },
  { PREF_T_INT,    "Mix_In_G_WinIdNr", "-1", &YaIPS_ToolData_info[0].Mix_In_G_WinIdNr },
  { PREF_T_INT,    "Mix_In_B_WinIdNr", "-1", &YaIPS_ToolData_info[0].Mix_In_B_WinIdNr },
};

// Automatic add this preference settings at startup of the program.
static IqeB_PreferencesGroup MyPreferencesAdd( MY_WIN_PREF_NAME, MyPreferences, sizeof( MyPreferences) / sizeof( T_GUI_PreferenceEntry),
                                               (void **)(&YaIPS_ToolData_info[ 0].pMyToolWin), &YaIPS_ToolData_info[ 0].MyWinPosX, &YaIPS_ToolData_info[ 0].MyWinPosY,
                                               MY_WIN_ID, MY_WIN_MAX, sizeof( YaIPS_ToolData_info_t),
                                               &YaIPS_ToolData_info[ 0].IsOpen, IqeB_GUI_ColorWin, (Fl_Callback *)close_cb,
                                               MY_WIN_GUI_LD_NAME, &YaIPS_ToolData_info[ 0].YaIPS_ImageDisp);

//-----------------------------------------------------------------------------------
// Parameter dialog
//
// This is a modal dialog. Therefore we can use global variables to hold
// info about the data.
//-----------------------------------------------------------------------------------

// defines for color conversions

// Simple color conversions
#define YAIPS_COLOR_GUI_RGB_2_R      YAIPS_COLOR_RGB_2_R   // Get red color component
#define YAIPS_COLOR_GUI_RGB_2_G      YAIPS_COLOR_RGB_2_G   // Get green color component
#define YAIPS_COLOR_GUI_RGB_2_B      YAIPS_COLOR_RGB_2_B   // Get blue color component
#define YAIPS_COLOR_GUI_GET_ALPHA    YAIPS_COLOR_GET_ALPHA // Get alpha from BW or RGB image
#define YAIPS_COLOR_GUI_RGB_2_I      YAIPS_COLOR_RGB_2_I   // Convert image RGB to intensity
#define YAIPS_COLOR_GUI_RGB_2_H      YAIPS_COLOR_RGB_2_H   // Convert image RGB to hue
#define YAIPS_COLOR_GUI_RGB_2_S      YAIPS_COLOR_RGB_2_S   // Convert image RGB to saturation
#define YAIPS_COLOR_GUI_RGB_MIN      YAIPS_COLOR_RGB_MIN   // Minimum of color components
#define YAIPS_COLOR_GUI_RGB_MAX      YAIPS_COLOR_RGB_MAX   // Maximum of color components
#define YAIPS_COLOR_GUI_RGB_2_BGR    YAIPS_COLOR_RGB_2_BGR // RGB <-> BGR conversion

// Other color conversions
#define YAIPS_COLOR_GUI_LUT_TABLE    (YAIPS_COLOR_RGB_N_CONV +  0)  // LUT conversion with predefined tables
#define YAIPS_COLOR_GUI_LUT_GAINCH   (YAIPS_COLOR_RGB_N_CONV +  1)  // LUT tables gain change
#define YAIPS_COLOR_GUI_LUT_GAMMA    (YAIPS_COLOR_RGB_N_CONV +  2)  // LUT tables gamma correction
#define YAIPS_COLOR_GUI_LUT_CONTRAST (YAIPS_COLOR_RGB_N_CONV +  3)  // LUT tables contrast correction
#define YAIPS_COLOR_GUI_LUT_DUOTONE  (YAIPS_COLOR_RGB_N_CONV +  4)  // LUT tables duotone coloring

#define YAIPS_COLOR_GUI_LUT_FIRST    YAIPS_COLOR_GUI_LUT_TABLE      // First color conversion working with LUTs
#define YAIPS_COLOR_GUI_LUT_LAST     YAIPS_COLOR_GUI_LUT_DUOTONE    // Last color conversion working with LUTs

#define YAIPS_COLOR_GUI_RGB_2_IHS    (YAIPS_COLOR_RGB_N_CONV +  5)  // Convert image RGB to IHS
#define YAIPS_COLOR_GUI_IHS_2_RGB    (YAIPS_COLOR_RGB_N_CONV +  6)  // Convert image IHS to RGB
#define YAIPS_COLOR_GUI_IHS_ADJUST   (YAIPS_COLOR_RGB_N_CONV +  7)  // Adjust IHS
#define YAIPS_COLOR_GUI_MATRIX       (YAIPS_COLOR_RGB_N_CONV +  8)  // Color conversion with matrix
#define YAIPS_COLOR_GUI_BRIGHT_CORR  (YAIPS_COLOR_RGB_N_CONV +  9)  // Brightness correction
#define YAIPS_COLOR_GUI_MIX_CHANNELS (YAIPS_COLOR_RGB_N_CONV + 10)  // Mix color channels to create an image

#define YAIPS_COLOR_GUI_BUTTON_MAX   (YAIPS_COLOR_GUI_MIX_CHANNELS + 1)  // Number of filter radio buttons

// ...

static  Fl_Window *pMyParWin;
static  YaIPS_ToolData_info_t *pToolData;     // NOTE: Is used by all parameter dialog functions
static Fl_YaIPS_Histo_RGB_t BC_AOI_Histo;     // Histogram for brightness correction AOI

static IqeFl_Tabs      *pTab_Groups;         // Point to tabulator GUI element
static Fl_Radio_Round_Button *ColorButtons[ YAIPS_COLOR_GUI_BUTTON_MAX]; // Table of color buttons
static int ColorType_Last;                  // Catch filter change

static IqeFl_Int_Input *pInt_OffsetR, *pInt_OffsetG, *pInt_OffsetB;
static IqeFl_Int_Input *pInt_GainChR, *pInt_GainChG, *pInt_GainChB;
static IqeFl_Float_Input *pFloat_GammaR, *pFloat_GammaG, *pFloat_GammaB;
static IqeFl_Int_Input *pIHS_IntAdjust, *pIHS_HueAdjust, *pIHS_SatAdjust;
static IqeFl_Int_Input *pBC_AOI_X, *pBC_AOI_Y, *pBC_AOI_XX, *pBC_AOI_YY;
static IqeFl_Int_Input *pBC_TargetR, *pBC_TargetG, *pBC_TargetB;
static Fl_Box *pMix_In_A_Box, *pMix_In_G_Box, *pMix_In_B_Box;    // Mix color channels
static Fl_Button *pMix_In_A_But, *pMix_In_G_But, *pMix_In_B_But; // Mix color channels
static Fl_Button *pTeachToggle;
static IqeFl_Int_Input *pInt_MatOffR, *pInt_MatOffG, *pInt_MatOffB;
static IqeFl_Float_Input *pFloat_MatMulRR, *pFloat_MatMulRG, *pFloat_MatMulRB;
static IqeFl_Float_Input *pFloat_MatMulGR, *pFloat_MatMulGG, *pFloat_MatMulGB;
static IqeFl_Float_Input *pFloat_MatMulBR, *pFloat_MatMulBG, *pFloat_MatMulBB;

static Fl_Button *pButCol_Br, *pButCol_Da;
static IqeFl_Int_Input *pIntColR_Br, *pIntColG_Br, *pIntColB_Br;
static IqeFl_Int_Input *pIntColR_Da, *pIntColG_Da, *pIntColB_Da;
static IqeFl_Int_Input *pIntDuoToneCo, *pIntDuoToneBr, *pIntDuoToneFade;

static Fl_Button *pBC_PickColor;

/************************************************************************************
 * update GUI of this tool window
 *
 */

static void MyParWinUpdate()
{
  int i, ValThis;
  Fl_RGB_Image *pImgIn1;
  int ImgXX, ImgYY, RedrawOnExit, TempEnable;
  Fl_Widget *pCurrFocus;

  RedrawOnExit = false;

  pCurrFocus = Fl::focus();

  // Get last selected tab group

  pToolData->Tab_Group_Selected = pTab_Groups->GetTabGroup();

  // Update filter button
  // Current selection must be set

  // Update all buttons
  if( ColorType_Last != pToolData->ColorType) {                // Filter type has change

    ColorType_Last = pToolData->ColorType;

    for( i = 0; i < YAIPS_COLOR_GUI_BUTTON_MAX; i++) {

      ValThis = ColorButtons[ i]->value();

      if( ValThis != (i == pToolData->ColorType)) {             // Not what we expected

        ColorButtons[ i]->value( i == pToolData->ColorType);   // Set value
        ColorButtons[ i]->redraw();
      }
    }
  }

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

  // ...

  IqeB_GUI_WidgetActivate( pInt_OffsetG, ! pToolData->GainCh1FA);
  IqeB_GUI_WidgetActivate( pInt_OffsetB, ! pToolData->GainCh1FA);
  IqeB_GUI_WidgetActivate( pInt_GainChG, ! pToolData->GainCh1FA);
  IqeB_GUI_WidgetActivate( pInt_GainChB, ! pToolData->GainCh1FA);

  IqeB_GUI_WidgetActivate( pFloat_GammaG, ! pToolData->Gamma1FA);
  IqeB_GUI_WidgetActivate( pFloat_GammaB, ! pToolData->Gamma1FA);

  IqeB_GUI_WidgetActivate( pIHS_IntAdjust, pToolData->ColorType == YAIPS_COLOR_GUI_IHS_ADJUST);
  IqeB_GUI_WidgetActivate( pIHS_HueAdjust, pToolData->ColorType == YAIPS_COLOR_GUI_IHS_ADJUST);
  IqeB_GUI_WidgetActivate( pIHS_SatAdjust, pToolData->ColorType == YAIPS_COLOR_GUI_IHS_ADJUST);

  // Duotone

  if( pButCol_Br->color() != pToolData->Duotone_Br_Color) {
    pButCol_Br->color( pToolData->Duotone_Br_Color);
    pButCol_Br->redraw();
  }

  if( pButCol_Da->color() != pToolData->Duotone_Da_Color) {
    pButCol_Da->color( pToolData->Duotone_Da_Color);
    pButCol_Da->redraw();
  }

#ifdef use_again
  TempEnable = pToolData->ColorType == YAIPS_COLOR_GUI_LUT_DUOTONE;

  IqeB_GUI_WidgetActivate( pButCol_Br, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColR_Br, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColG_Br, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColB_Br, TempEnable);

  IqeB_GUI_WidgetActivate( pButCol_Da, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColR_Da, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColG_Da, TempEnable);
  IqeB_GUI_WidgetActivate( pIntColB_Da, TempEnable);

  IqeB_GUI_WidgetActivate( pIntDuoToneBr, TempEnable);
  IqeB_GUI_WidgetActivate( pIntDuoToneCo, TempEnable);
  IqeB_GUI_WidgetActivate( pIntDuoToneFade, TempEnable);
#else
  // Allow Fade only for color images
  TempEnable =  pImgIn1 == NULL || pImgIn1->d() >= 3;         // Image has three or more color channels
  IqeB_GUI_WidgetActivate( pIntDuoToneFade, TempEnable);
#endif

  // Check mix color channels input images

  YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Mix_In_A_WinIdNr, pMix_In_A_Box);
  YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Mix_In_G_WinIdNr, pMix_In_G_Box);
  YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Mix_In_B_WinIdNr, pMix_In_B_Box);

  // Brightness correction: Pick color button

  IqeB_GUI_WidgetActivate( pBC_PickColor, pToolData->ColorType == YAIPS_COLOR_GUI_BRIGHT_CORR &&
                                          BC_AOI_Histo.nHistos > 0);

  // Brightness correction: Enable AOI pos/size input buttons

  IqeB_GUI_WidgetActivate( pBC_AOI_X, pToolData->ColorType == YAIPS_COLOR_GUI_BRIGHT_CORR && pToolData->BC_AOI_Teach);
  IqeB_GUI_WidgetActivate( pBC_AOI_Y, pToolData->ColorType == YAIPS_COLOR_GUI_BRIGHT_CORR && pToolData->BC_AOI_Teach);
  IqeB_GUI_WidgetActivate( pBC_AOI_XX, pToolData->ColorType == YAIPS_COLOR_GUI_BRIGHT_CORR && pToolData->BC_AOI_Teach);
  IqeB_GUI_WidgetActivate( pBC_AOI_YY, pToolData->ColorType == YAIPS_COLOR_GUI_BRIGHT_CORR && pToolData->BC_AOI_Teach);

  // Brightness correction: Color teach toggle button

  IqeB_GUI_WidgetActivate( pTeachToggle, pToolData->ColorType == YAIPS_COLOR_GUI_BRIGHT_CORR);// Only usable for brightness correction

  IqeB_GUI_WidgetLabelColor( pTeachToggle, pToolData->BC_AOI_Teach ? FL_GREEN : YAIPS_BCOL_BUTTON);

  // Check AOI

  if( pCurrFocus != pBC_AOI_X  && pCurrFocus != pBC_AOI_Y &&               // Input element has NO keyboard focus ?
      pCurrFocus != pBC_AOI_XX && pCurrFocus != pBC_AOI_YY) {

    if( YaIPS_ImageDispAoiRectIGuiUpdate( &pToolData->BC_AOI, ImgXX, ImgYY,
                                        pBC_AOI_X, pBC_AOI_Y, pBC_AOI_XX, pBC_AOI_YY) > 0) {

      RedrawOnExit = true;                                 // Redraw on exit
    }
  }

  // Redraw

  if( RedrawOnExit) {                                           // Redraw on exit

    if( pToolData->ColorType == YAIPS_COLOR_GUI_BRIGHT_CORR) {  // AOI is displayed on image

      if( pToolData->YaIPS_ImageDisp.pImage_Box != NULL) {
        pToolData->YaIPS_ImageDisp.pImage_Box->redraw();
      }

      if( YaIPS_BigImageDisp.ImageSourceID == MY_WIN_ID + pToolData->iToolData) {   // and display this on the big image

        if( YaIPS_BigImageDisp.pImage_Box != NULL) {
          YaIPS_BigImageDisp.pImage_Box->redraw();
        }
      }
    }
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
 * YaIPS_Color_Callback
 *
 * Color processing will change
 */

static void YaIPS_Color_Callback( Fl_Widget *w, void *data)
{
  int Value;

  // ...

  Value = (long long)(data);                       // get value to set

  if( pToolData->ColorType == Value) {             // Value will not change

    return;                                        // Exit, nothing to do
  }

  pToolData->ColorType = Value;                   // Set new value

  pToolData->Input1_Change = 0;                   // Force recalculation output
}

/************************************************************************************
 * IqeB_ColorFilter_PresetButton_Callback
 *
 * Callback, Set one of the preset color filters
 */

static void IqeB_ColorFilter_PresetButton_Callback( Fl_Widget *w)
{
  int i;
  Fl_Menu_Button *pMenu_Button;
  T_YaIPS_ColorMatrix *pColorMatrix;

  pMenu_Button = (Fl_Menu_Button *)w;

  // Return a pointer to the last menu item that was picked
  const Fl_Menu_Item *m = pMenu_Button->mvalue();

  if( m == NULL) {       // Security test

    return;
  }

  // A menu line was selected

  i = (int)(uintptr_t)(m->user_data_);

  if( i < 0 || i >= nColorMatrix_List) {   // Security test

    return;
  }

  // Point into color matrix list
  pColorMatrix = ColorMatrix_List + i;

  // Set data

  pToolData->R_Offset = pColorMatrix->R_Offset;
  pToolData->G_Offset = pColorMatrix->G_Offset;
  pToolData->B_Offset = pColorMatrix->B_Offset;

  pInt_MatOffR->SetValue( pColorMatrix->R_Offset);
  pInt_MatOffG->SetValue( pColorMatrix->G_Offset);
  pInt_MatOffB->SetValue( pColorMatrix->B_Offset);

  pToolData->R_MultR = pColorMatrix->R_MultR;
  pToolData->R_MultG = pColorMatrix->R_MultG;
  pToolData->R_MultB = pColorMatrix->R_MultB;

  pFloat_MatMulRR->SetValue( pColorMatrix->R_MultR);
  pFloat_MatMulRG->SetValue( pColorMatrix->R_MultG);
  pFloat_MatMulRB->SetValue( pColorMatrix->R_MultB);

  pToolData->G_MultR = pColorMatrix->G_MultR;
  pToolData->G_MultG = pColorMatrix->G_MultG;
  pToolData->G_MultB = pColorMatrix->G_MultB;

  pFloat_MatMulGR->SetValue( pColorMatrix->G_MultR);
  pFloat_MatMulGG->SetValue( pColorMatrix->G_MultG);
  pFloat_MatMulGB->SetValue( pColorMatrix->G_MultB);

  pToolData->B_MultR = pColorMatrix->B_MultR;
  pToolData->B_MultG = pColorMatrix->B_MultG;
  pToolData->B_MultB = pColorMatrix->B_MultB;

  pFloat_MatMulBR->SetValue( pColorMatrix->B_MultR);
  pFloat_MatMulBG->SetValue( pColorMatrix->B_MultG);
  pFloat_MatMulBB->SetValue( pColorMatrix->B_MultB);

  // ...

  if( pToolData->ColorType == YAIPS_COLOR_GUI_MATRIX) {  // Color matrix operation is selected

    pToolData->Input1_Change = 0;                        // Force recalculation output
  }
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

  if( pValue == &pToolData->GammaR &&      // Red gamma and 'one for all' checkbox is set
      pToolData->Gamma1FA != 0) {

    pToolData->GammaG = pToolData->GammaR;        // Copy red to green and blue
    pToolData->GammaB = pToolData->GammaR;
    pFloat_GammaG->SetValue( pToolData->GammaR);  // and update GUI
    pFloat_GammaB->SetValue( pToolData->GammaR);
  }

  // ...

  pToolData->Input1_Change = 0;                    // Force recalculation output
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

  if( pValue == &pToolData->OffsetR) {      // Red offset change and 'one for all' checkbox is set

    if( pToolData->GainCh1FA != 0) {

      pToolData->OffsetG = pToolData->OffsetR;        // Copy red to green and blue
      pToolData->OffsetB = pToolData->OffsetR;
      pInt_OffsetG->SetValue( pToolData->OffsetR);  // and update GUI
      pInt_OffsetB->SetValue( pToolData->OffsetR);
    }

  } else
  if( pValue == &pToolData->GainChR) {      // Red gain change and 'one for all' checkbox is set

    if( pToolData->GainCh1FA != 0) {

      pToolData->GainChG = pToolData->GainChR;        // Copy red to green and blue
      pToolData->GainChB = pToolData->GainChR;
      pInt_GainChG->SetValue( pToolData->GainChR);  // and update GUI
      pInt_GainChB->SetValue( pToolData->GainChR);
    }
  }

  // ...

  if( *pValue != Value) {                  // Value is different

    *pValue = Value;                       // update the variable

    pToolData->Input1_Change = 0;          // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_Int_SetValue_ColR_Callback
 *
 * Callback change only red color component.
 */

static void IqeB_GUI_Int_SetValue_ColR_Callback( Fl_Widget *w, void *pValueArg)
{
  unsigned int Value, *pValue;
  IqeFl_Int_Input *pThis;
  uchar r,g,b;

  // ...

  pThis  = (IqeFl_Int_Input *)w;
  pValue = (unsigned int *)pValueArg;               // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pValue == NULL) {

    return;
  }

  Fl::get_color( *pValue, r, g, b);

  Value = pThis->GetValue();               // get the value

  IqeB_GUI_Int_SetValue_Callback( w, &Value);

  if( r != Value) {                        // Value has changed

    // Set new changed value
    r = Value;

    *pValue = fl_rgb_color( r, g, b);

    pToolData->Input1_Change = 0;                    // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_Int_SetValue_ColG_Callback
 *
 * Callback change only red color component.
 */

static void IqeB_GUI_Int_SetValue_ColG_Callback( Fl_Widget *w, void *pValueArg)
{
  unsigned int Value, *pValue;
  IqeFl_Int_Input *pThis;
  uchar r,g,b;

  // ...

  pThis  = (IqeFl_Int_Input *)w;
  pValue = (unsigned int *)pValueArg;               // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pValue == NULL) {

    return;
  }

  Fl::get_color( *pValue, r, g, b);

  Value = pThis->GetValue();               // get the value

  IqeB_GUI_Int_SetValue_Callback( w, &Value);

  if( g != Value) {                        // Value has changed

    // Set new changed value
    g = Value;

    *pValue = fl_rgb_color( r, g, b);

    pToolData->Input1_Change = 0;                    // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_Int_SetValue_ColB_Callback
 *
 * Callback change only red color component.
 */

static void IqeB_GUI_Int_SetValue_ColB_Callback( Fl_Widget *w, void *pValueArg)
{
  unsigned int Value, *pValue;
  IqeFl_Int_Input *pThis;
  uchar r,g,b;

  // ...

  pThis  = (IqeFl_Int_Input *)w;
  pValue = (unsigned int *)pValueArg;               // get pointer to associated variable

  if( pThis == NULL ||       // security test
      pValue == NULL) {

    return;
  }

  Fl::get_color( *pValue, r, g, b);

  Value = pThis->GetValue();               // get the value

  IqeB_GUI_Int_SetValue_Callback( w, &Value);

  if( b != Value) {                        // Value has changed

    // Set new changed value
    b = Value;

    *pValue = fl_rgb_color( r, g, b);

    pToolData->Input1_Change = 0;                // Force recalculation output
  }
}

/************************************************************************************
 * IqeB_GUI_But_Color_SetValue_Callback
 */

static void IqeB_GUI_But_Color_SetValue_Callback( Fl_Widget *w, void *pValueArg)
{
  unsigned int *pColor, ColorBefore;
  Fl_Button *pThis;
  uchar r,g,b;

  // ...

  pThis  = (Fl_Button *)w;
  pColor = (unsigned int *)pValueArg;    // get pointer to associated variable
  ColorBefore = *pColor;

  *pColor = IqeB_GUI_ColorChooser( *pColor);

  if( ColorBefore != *pColor) {

    pThis->color( *pColor);
    pThis->parent()->redraw();

    // Update color RGB components
    Fl::get_color( *pColor, r, g, b);

    if( pColor == &pToolData->Duotone_Br_Color) {
      pIntColR_Br->SetValue( r);
      pIntColG_Br->SetValue( g);
      pIntColB_Br->SetValue( b);
    } else if( pColor == &pToolData->Duotone_Da_Color) {
      pIntColR_Da->SetValue( r);
      pIntColG_Da->SetValue( g);
      pIntColB_Da->SetValue( b);
    }

    pToolData->Input1_Change = 0;          // Force recalculation output
  }
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

    pToolData->BC_AOI_Teach = ! pToolData->BC_AOI_Teach;

  } else if( pValue == &pToolData->LUT_Table) {

    // Fl_Choice

    Fl_Choice *pThis;

    pThis  = (Fl_Choice *)w;
    *pValue = pThis->value();                // update the variable

  } else if( pValue == &pToolData->Mix_In_A_WinIdNr) {

    int WinIdNr_Before;

    WinIdNr_Before = pToolData->Mix_In_A_WinIdNr;

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + pToolData->iToolData, &pToolData->Mix_In_A_WinIdNr, pMix_In_A_But, pMix_In_A_Box);

    if( WinIdNr_Before != pToolData->Mix_In_A_WinIdNr) {    // Image source selection as changed

      pToolData->Input1_Change = 0;                         // Force recalculation output
    }

  } else if( pValue == &pToolData->Mix_In_G_WinIdNr) {

    int WinIdNr_Before;

    WinIdNr_Before = pToolData->Mix_In_G_WinIdNr;

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + pToolData->iToolData, &pToolData->Mix_In_G_WinIdNr, pMix_In_G_But, pMix_In_G_Box);

    if( WinIdNr_Before != pToolData->Mix_In_G_WinIdNr) {    // Image source selection as changed

      pToolData->Input1_Change = 0;                         // Force recalculation output
    }

  } else if( pValue == &pToolData->Mix_In_B_WinIdNr) {

    int WinIdNr_Before;

    WinIdNr_Before = pToolData->Mix_In_B_WinIdNr;

    // Select an input image
    YaIPS_ToolWinInputSelect( MY_WIN_ID + pToolData->iToolData, &pToolData->Mix_In_B_WinIdNr, pMix_In_B_But, pMix_In_B_Box);

    if( WinIdNr_Before != pToolData->Mix_In_B_WinIdNr) {    // Image source selection as changed

      pToolData->Input1_Change = 0;                         // Force recalculation output
    }

  } else {

    Fl_Button *pThis;

    pThis  = (Fl_Check_Button *)w;
    *pValue = pThis->value();                // update the variable


    if( pValue == &pToolData->GainCh1FA) {    // Gain change 'one for all' checkbox

      if( pToolData->GainCh1FA != 0) {        // and checkbox changed to on


        pToolData->OffsetG = pToolData->OffsetR;        // Copy red to green and blue
        pToolData->OffsetB = pToolData->OffsetR;
        pInt_OffsetG->SetValue( pToolData->OffsetR);  // and update GUI
        pInt_OffsetB->SetValue( pToolData->OffsetR);

        pToolData->GainChG = pToolData->GainChR;        // Copy red to green and blue
        pToolData->GainChB = pToolData->GainChR;
        pInt_GainChG->SetValue( pToolData->GainChR);  // and update GUI
        pInt_GainChB->SetValue( pToolData->GainChR);
      }

    } else
    if( pValue == &pToolData->Gamma1FA) {    // Gamma 'one for all' checkbox

      if( pToolData->Gamma1FA != 0) {        // and checkbox changed to on

        pToolData->GammaG = pToolData->GammaR;        // Copy red to green and blue
        pToolData->GammaB = pToolData->GammaR;
        pFloat_GammaG->SetValue( pToolData->GammaR);  // and update GUI
        pFloat_GammaB->SetValue( pToolData->GammaR);
      }
    }
  }

  pToolData->Input1_Change = 0;          // Force recalculation output
}

/************************************************************************************
 * YaIPS_GUI_PickColor
 *
 * Pick color from brightness correction AOI as new target value.
 */

static void YaIPS_GUI_PickColor( Fl_Widget *w, void *pValueArg)
{
  int NewR, NewG, NewB;

  // Check proper settings

  if( pToolData->ColorType != YAIPS_COLOR_GUI_BRIGHT_CORR ||   // Brightness correction
      BC_AOI_Histo.nHistos <= 0) {                             // and measured value

    return;
  }

  // Pick the values

  if( BC_AOI_Histo.nHistos >= 3) {            // Color image

    NewR = (int)(BC_AOI_Histo.R.Average + 0.5);
    NewG = (int)(BC_AOI_Histo.G.Average + 0.5);
    NewB = (int)(BC_AOI_Histo.B.Average + 0.5);

  } else if( BC_AOI_Histo.nHistos >= 1) {    // Black / white image

    NewR = (int)(BC_AOI_Histo.R.Average + 0.5);
    NewG = (int)(BC_AOI_Histo.R.Average + 0.5);
    NewB = (int)(BC_AOI_Histo.R.Average + 0.5);

  } else {

    // Change nothing

    NewR = pToolData->BC_TargetR;
    NewG = pToolData->BC_TargetG;
    NewB = pToolData->BC_TargetB;
  }

  // Changed any value ?

  if( NewR != pToolData->BC_TargetR ||
      NewG != pToolData->BC_TargetG ||
      NewB != pToolData->BC_TargetB) {

    // Set value to GUI
    pBC_TargetR->SetValue( NewR);
    pBC_TargetG->SetValue( NewG);
    pBC_TargetB->SetValue( NewB);

    // Get value from GUI picks the clipped values
    pToolData->BC_TargetR = pBC_TargetR->GetValue();
    pToolData->BC_TargetG = pBC_TargetG->GetValue();
    pToolData->BC_TargetB = pBC_TargetB->GetValue();

    // ...

    pToolData->Input1_Change = 0;                    // Force recalculation output
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

  pMyParWin = new Fl_Window( xPos, yPos, 297 /*IQE_GUI_TOOLS_STD_WITDH*/, 142 /* 162 */, LANGDEF_SETTINGS);

  if( pMyParWin == NULL) {  // security test

    return;
  }

  ColorType_Last = -1;         // Reset last filter type

  //
  //  GUI things
  //

  uchar r,g,b;
  int x1, y, yy, xx2;
  //x/int xx1, xc;
  int yGroup;
  //x/char TempBuffer[ 256];

  Fl_Check_Button *pCheckTemp;
  Fl_Box          *pTemp_Box;
  IqeFl_Int_Input    *pTemp_Int;
  IqeFl_Float_Input  *pFloatTemp;
  IqeFl_Tabs      *pTemp_Tabs;
  Fl_Group        *pTemp_Group;
  Fl_Button       *pTemp_Button;
  Fl_Choice       *pTemp_Choice;
  Fl_Radio_Round_Button *pRadioButTemp;
  Fl_Menu_Button  *pTemp_MenuButton;

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
  // Group 'Simple'
  //

  yGroup = y;
  x1 = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Color_TabA1=Simple"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Color_TabA1a=Simple conversions"));

    y += 8;

    xx2 = 70;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabA2=-> R"));
    pRadioButTemp->tooltip( LANGDEF_COL_CHANNEL_R);
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_RGB_2_R);
    ColorButtons[ YAIPS_COLOR_GUI_RGB_2_R] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabA3=-> G"));
    pRadioButTemp->tooltip( LANGDEF_COL_CHANNEL_G);
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_RGB_2_G);
    ColorButtons[ YAIPS_COLOR_GUI_RGB_2_G] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabA4=-> B"));
    pRadioButTemp->tooltip( LANGDEF_COL_CHANNEL_B);
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_RGB_2_B);
    ColorButtons[ YAIPS_COLOR_GUI_RGB_2_B] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabA5=-> A"));
    pRadioButTemp->tooltip(  LangStringLookup( "&GUI_Color_TabA5a="
                                               "Extracts the alpha channel from\n"
                                               "a black and white or RGB image."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_GET_ALPHA);
    ColorButtons[ YAIPS_COLOR_GUI_GET_ALPHA] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabA6=-> I"));
    pRadioButTemp->tooltip(  LangStringLookup( "&GUI_Color_TabA6a=Color -> Intensity of an IHS conversion."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_RGB_2_I);
    ColorButtons[ YAIPS_COLOR_GUI_RGB_2_I] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabA7=-> H"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Color_TabA7a=Color -> Color tone of an IHS conversion."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_RGB_2_H);
    ColorButtons[ YAIPS_COLOR_GUI_RGB_2_H] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabA8=-> S"));
    pRadioButTemp->tooltip(  LangStringLookup( "&GUI_Color_TabA8a=Color -> Saturation of an IHS conversion."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_RGB_2_S);
    ColorButtons[ YAIPS_COLOR_GUI_RGB_2_S] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    xx2 = 86;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabA9=RGB Min"));
    pRadioButTemp->tooltip(  LangStringLookup( "&GUI_Color_TabA9a=Smallest value of the color components."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_RGB_MIN);
    ColorButtons[ YAIPS_COLOR_GUI_RGB_MIN] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabA10=RGB Max"));
    pRadioButTemp->tooltip(  LangStringLookup( "&GUI_Color_TabA10a=Highest value of the color components."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_RGB_MAX);
    ColorButtons[ YAIPS_COLOR_GUI_RGB_MAX] = pRadioButTemp;

    x1 += xx2;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 + 24, yy, LangStringLookup( "&GUI_Color_TabA11=RGB <-> BGR"));
    pRadioButTemp->tooltip(  LangStringLookup( "&GUI_Color_TabA11a=RGB <-> BGR conversion."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_RGB_2_BGR);
    ColorButtons[ YAIPS_COLOR_GUI_RGB_2_BGR] = pRadioButTemp;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group LUT1
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Color_TabB1=LUT1"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Color_TabB1a=Look Up Tables"));

    y += 8;

    xx2 = 60;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LANGDEF_LUT_SHORT);
    pRadioButTemp->tooltip( LANGDEF_LUT_TOOLTIP);
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_LUT_TABLE);
    ColorButtons[ YAIPS_COLOR_GUI_LUT_TABLE] = pRadioButTemp;

    x1 += xx2;
    x1 += 8;

    xx2 = 140;

    pTemp_Choice = new Fl_Choice( x1, y, xx2 - 10, yy);
    pTemp_Choice->tooltip( LANGDEF_LUT_TOOLTIP);
    pTemp_Choice->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->LUT_Table);

    // Add predefined LUT tables
    for( int i = 0; i < YaIPS_ColMod_nLUT_tables; i++) {

      pTemp_Choice->add( LangStringLookup( YaIPS_ColMod_LUT_table[ i]));
    }

    pTemp_Choice->value( pToolData->LUT_Table);

    x1 += xx2;
    //x/x1 += 4;

    xx2 = 92;

    pCheckTemp = new Fl_Check_Button( x1, y, xx2 - 16, yy, LANGDEF_INVERTED);
    pCheckTemp->tooltip( LangStringLookup( "&GUI_Color_TabB4a=Inverts color, black and white image or color palette"));
    pCheckTemp->value( pToolData->LUT_Invert);
    pCheckTemp->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->LUT_Invert);

    // Next line

    x1  = 4;
    y += yy + 6;

    xx2 = 72;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabB5=Gain %"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Color_TabB5a="
                                              "Gain % + offset.\n"
                                              "Change brightness gain in %\n"
                                              "and brightness offset in gray values."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_LUT_GAINCH);
    ColorButtons[ YAIPS_COLOR_GUI_LUT_GAINCH] = pRadioButTemp;

    x1 += xx2;
    x1 += 16;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_R);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabB6a=Brightness for the red color channel."));
    pTemp_Int->SetValue( pToolData->GainChR);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->GainChR);
    pTemp_Int->SetModifyData( -100, 100, 10, 1);
    pInt_GainChR = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_G);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabB7a=Brightness for the green color channel."));
    pTemp_Int->SetValue( pToolData->GainChG);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->GainChG);
    pTemp_Int->SetModifyData( -100, 100, 10, 1);
    pInt_GainChG = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_B);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabB8a=Brightness for the blue color channel."));
    pTemp_Int->SetValue( pToolData->GainChB);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->GainChB);
    pTemp_Int->SetModifyData( -100, 100, 10, 1);
    pInt_GainChB = pTemp_Int;

    x1 += xx2;
    x1 += 8;

    xx2 = 50;

    pCheckTemp = new Fl_Check_Button( x1, y, xx2 - 16, yy, LANGDEF_ACTIVE_SHORT);
    pCheckTemp->tooltip( LangStringLookup( "&GUI_Color_TabB9a="
                                           "If set, the red value is used\n"
                                           "for all three channels."));
    pCheckTemp->value( pToolData->GainCh1FA);
    pCheckTemp->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->GainCh1FA);

    // Next line

    x1  = 4;
    y += yy + 2;

    x1 += 88;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabB10=Offset  R"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabB10a=Offset for the red color channel."));
    pTemp_Int->SetValue( pToolData->OffsetR);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->OffsetR);
    pTemp_Int->SetModifyData( -256, 256, 16, 1);
    pInt_OffsetR = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_G);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabB11a=Offset for the green color channel."));
    pTemp_Int->SetValue( pToolData->OffsetG);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->OffsetG);
    pTemp_Int->SetModifyData( -256, 256, 16, 1);
    pInt_OffsetG = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_B);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabB12a=Offset for the blue color channel."));
    pTemp_Int->SetValue( pToolData->OffsetB);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->OffsetB);
    pTemp_Int->SetModifyData( -256, 256, 16, 1);
    pInt_OffsetB = pTemp_Int;

    x1 += xx2;
    x1 += 8;

    // Next line

    x1  = 4;
    y += yy + 6;

    xx2 = 72;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabB13=Gamma"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Color_TabB13a="
                                              "Gamma correction.\n"
                                              "Adjusting brightness and contrast."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_LUT_GAMMA);
    ColorButtons[ YAIPS_COLOR_GUI_LUT_GAMMA] = pRadioButTemp;

    x1 += xx2;
    x1 += 16;

    xx2 = 40;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_COLOR_R);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Color_TabB14a="
                                           "Gamma factor for the red color channel."
                                           "No correction is made at 1.0."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->GammaR);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->GammaR);
    pFloatTemp->SetModifyData( 0.2, 5.0, 0.1, 0.01);
    pFloat_GammaR = pFloatTemp;

    x1 += xx2;
    x1 += 16;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_COLOR_G);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Color_TabB15a="
                                           "Gamma factor for the green color channel."
                                           "No correction is made at 1.0."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->GammaG);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->GammaG);
    pFloatTemp->SetModifyData( 0.2, 5.0, 0.1, 0.01);
    pFloat_GammaG = pFloatTemp;

    x1 += xx2;
    x1 += 16;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_COLOR_B);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LangStringLookup( "&GUI_Color_TabB16a="
                                           "Gamma factor for the blue color channel."
                                           "No correction is made at 1.0."));
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->GammaB);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->GammaB);
    pFloatTemp->SetModifyData( 0.2, 5.0, 0.1, 0.01);
    pFloat_GammaB = pFloatTemp;

    x1 += xx2;
    x1 += 8;

    xx2 = 50;

    pCheckTemp = new Fl_Check_Button( x1, y, xx2 - 16, yy, LANGDEF_ACTIVE_SHORT);
    pCheckTemp->tooltip( LangStringLookup( "&GUI_Color_TabB17a="
                                           "If set, the red value is used\n"
                                           "for all three channels."));
    pCheckTemp->value( pToolData->Gamma1FA);
    pCheckTemp->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Gamma1FA);

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group LUT2
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Color_TabM1=LUT2"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Color_TabM1a=Look Up Tables"));

    y += 8;

    xx2 = 80;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabM2=Contrast"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Color_TabM2a="
                                              "Change contrast by modifying lower and upper\n"
                                              "control points.\n"
                                              "In (input image) axis goes from left to right.\n"
                                              "Out (output image) axis goes from bottom to top.\n"
                                              "NOTE: Li must be smaller than Ui."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_LUT_CONTRAST);
    ColorButtons[ YAIPS_COLOR_GUI_LUT_CONTRAST] = pRadioButTemp;

    x1 += xx2;
    x1 += 21;

    xx2 = 30;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabM3=Li"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabM3a=Lower control point, in value."));
    pTemp_Int->SetValue( pToolData->Contrast1in);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Contrast1in);
    pTemp_Int->SetModifyData( 0, 255, 10, 1);

    x1 += xx2;
    x1 += 21;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabM4=Ui"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabM4a=Upper control point, in value."));
    pTemp_Int->SetValue( pToolData->Contrast2in);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Contrast2in);
    pTemp_Int->SetModifyData( 0, 255, 10, 1);

    x1 += xx2;
    x1 += 21;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabM5=Lo"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabM5a=Lower control point, out value."));
    pTemp_Int->SetValue( pToolData->Contrast1out);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Contrast1out);
    pTemp_Int->SetModifyData( 0, 255, 10, 1);

    x1 += xx2;
    x1 += 21;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabM6=Uo"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabM6a=Upper control point, out value."));
    pTemp_Int->SetValue( pToolData->Contrast2out);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Contrast2out);
    pTemp_Int->SetModifyData( 0, 255, 10, 1);

    // Next line

    x1  = 4;
    y += yy + 6;

    xx2 = 80;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabM10=Duotone"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Color_TabM10a="
                                              "Combine two colors as a filter."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_LUT_DUOTONE);
    ColorButtons[ YAIPS_COLOR_GUI_LUT_DUOTONE] = pRadioButTemp;

    x1 += xx2;
    x1 += 21;

    xx2 = 26;

    pTemp_Box = new Fl_Box( x1 - xx2, y, xx2, yy, LangStringLookup( "&GUI_Color_TabM11=Br"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    xx2 = 30;

    pTemp_Button = new Fl_Button( x1, y, xx2, yy, "");
    pTemp_Button->color( pToolData->Duotone_Br_Color);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &pToolData->Duotone_Br_Color);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Color_TabM11a="
                           "Bright color for highlighting."));
    pButCol_Br = pTemp_Button;

    x1 += xx2 + 20;

    xx2 = 30;

    Fl::get_color( pToolData->Duotone_Br_Color, r, g, b);

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_R);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_R);
    pTemp_Int->SetValue( r);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColR_Callback, &pToolData->Duotone_Br_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColR_Br= pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_G);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_G);
    pTemp_Int->SetValue( g);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColG_Callback, &pToolData->Duotone_Br_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColG_Br = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_B);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_B);
    pTemp_Int->SetValue( b);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColB_Callback, &pToolData->Duotone_Br_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColB_Br = pTemp_Int;

    // Next line

    x1  = 4;
    y += yy + 6;

    x1 += 80 + 21;

    xx2 = 26;

    pTemp_Box = new Fl_Box( x1 - xx2, y, xx2, yy, LangStringLookup( "&GUI_GenImage_TabM12=Da"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

    xx2 = 30;

    pTemp_Button = new Fl_Button( x1, y, xx2, yy, "");
    pTemp_Button->color( pToolData->Duotone_Da_Color);
    pTemp_Button->callback( IqeB_GUI_But_Color_SetValue_Callback, &pToolData->Duotone_Da_Color);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_GenImage_TabM12a="
                           "Dark color for shadows."));
    pButCol_Da = pTemp_Button;

    x1 += xx2 + 20;

    xx2 = 30;

    Fl::get_color( pToolData->Duotone_Da_Color, r, g, b);

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_R);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_R);
    pTemp_Int->SetValue( r);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColR_Callback, &pToolData->Duotone_Da_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColR_Da = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_G);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_G);
    pTemp_Int->SetValue( g);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColG_Callback, &pToolData->Duotone_Da_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColG_Da = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_B);
    pTemp_Int->tooltip( LANGDEF_COL_CHANNEL_B);
    pTemp_Int->SetValue( b);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_ColB_Callback, &pToolData->Duotone_Da_Color);
    pTemp_Int->SetModifyData( 0, 255, 16, 1);
    pIntColB_Da = pTemp_Int;

    // Next line

    x1  = 4;
    y += yy + 6;

    x1 += 80 + 21;

    xx2 = 38;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabM13=Contrast"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabM13a="
                                          "Contrast\n"
                                          "Range -100 ... 100.\n"
                                          "0 = no change."));
    pTemp_Int->SetValue( pToolData->Duotone_contrast);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Duotone_contrast);
    pTemp_Int->SetModifyData( -100, 100, 10, 1);
    pIntDuoToneCo = pTemp_Int;

    x1 += xx2 + 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabM14=Bri."));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabM14a="
                                          "Brightness\n"
                                          "Range -100 ... 100.\n"
                                          "0 = no change."));
    pTemp_Int->SetValue( pToolData->Duotone_brightness);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Duotone_brightness);
    pTemp_Int->SetModifyData( -100, 100, 10, 1);
    pIntDuoToneBr = pTemp_Int;


    x1 += xx2 + 40;

    xx2 = 30;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabM15=Fade"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabM15a="
                                          "Used for color images only.\n"
                                          "Fade between RGB and BW (saturation) pixels.\n"
                                          "Range 0 ... 100.\n"
                                          "0 = use RGB pixels. 100 = use BW pixels"));
    pTemp_Int->SetValue( pToolData->Duotone_RGB_Fade);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->Duotone_RGB_Fade);
    pTemp_Int->SetModifyData( 0, 100, 10, 1);
    pIntDuoToneFade = pTemp_Int;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group Matrix
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Color_TabC1=Matrix"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Color_TabC1a=Color conversion using a matrix"));

    y += 8;

    xx2 = 60;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabC2=Matrix"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Color_TabC2a=Color conversion using a matrix"));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_MATRIX);
    ColorButtons[ YAIPS_COLOR_GUI_MATRIX] = pRadioButTemp;

    x1 += xx2;
    x1 += 16;

    xx2 = 80;

    pTemp_MenuButton = new Fl_Menu_Button( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabC3=Preset"));
    pTemp_MenuButton->tooltip( LangStringLookup( "&GUI_Color_TabC3a="
                                              "Some typical color filters"));
    //pTemp_MenuButton->align( FL_ALIGN_TOP_LEFT /*FL_ALIGN_LEFT*/);     // align for label
    //pTemp_MenuButton->labelsize( 10);
    pTemp_MenuButton->callback( IqeB_ColorFilter_PresetButton_Callback);

    // Add predefined color filters

    for( int i = 0; i < nColorMatrix_List; i++) {

      pTemp_MenuButton->add( /*LangStringLookup(*/ ColorMatrix_List[ i].pName /* ) */, 0, NULL, (void *)(fl_intptr_t)i);
    }

    // Next line

    x1  = 4;
    y += yy + 6;

    x1 += 28;

    xx2 = 38;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabC4=R ="));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabC4a=Offset for the red color channel."));
    pTemp_Int->SetValue( pToolData->R_Offset);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->R_Offset);
    pTemp_Int->SetModifyData( -400, 400, 10, 1);
    pInt_MatOffR = pTemp_Int;

    x1 += xx2;
    x1 += 34;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_COL_MULT_R1);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LANGDEF_COL_MULT_R2);
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->R_MultR);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->R_MultR);
    pFloatTemp->SetModifyData( -2.0, 2.0, 0.1, 0.01);
    pFloat_MatMulRR = pFloatTemp;

    x1 += xx2;
    x1 += 34;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_COL_MULT_G1);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LANGDEF_COL_MULT_G2);
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->R_MultG);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->R_MultG);
    pFloatTemp->SetModifyData( -2.0, 2.0, 0.1, 0.01);
    pFloat_MatMulRG = pFloatTemp;

    x1 += xx2;
    x1 += 34;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_COL_MULT_B1);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LANGDEF_COL_MULT_B2);
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->R_MultB);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->R_MultB);
    pFloatTemp->SetModifyData( -2.0, 2.0, 0.1, 0.01);
    pFloat_MatMulRB = pFloatTemp;

    // Next line

    x1  = 4;
    y += yy + 6;

    x1 += 28;

    xx2 = 38;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabC7=G ="));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabC7a=Offset for the green color channel."));
    pTemp_Int->SetValue( pToolData->G_Offset);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->G_Offset);
    pTemp_Int->SetModifyData( -400, 400, 10, 1);
    pInt_MatOffG = pTemp_Int;

    x1 += xx2;
    x1 += 34;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_COL_MULT_R1);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LANGDEF_COL_MULT_R2);
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->G_MultR);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->G_MultR);
    pFloatTemp->SetModifyData( -2.0, 2.0, 0.1, 0.01);
    pFloat_MatMulGR = pFloatTemp;

    x1 += xx2;
    x1 += 34;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_COL_MULT_G1);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LANGDEF_COL_MULT_G2);
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->G_MultG);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->G_MultG);
    pFloatTemp->SetModifyData( -2.0, 2.0, 0.1, 0.01);
    pFloat_MatMulGG = pFloatTemp;

    x1 += xx2;
    x1 += 34;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_COL_MULT_B1);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LANGDEF_COL_MULT_B2);
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->G_MultB);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->G_MultB);
    pFloatTemp->SetModifyData( -2.0, 2.0, 0.1, 0.01);
    pFloat_MatMulGB = pFloatTemp;

    // Next line

    x1  = 4;
    y += yy + 6;

    x1 += 28;

    xx2 = 38;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabC11=B ="));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabC11a=Offset for the blue color channel."));
    pTemp_Int->SetValue( pToolData->B_Offset);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->B_Offset);
    pTemp_Int->SetModifyData( -400, 400, 10, 1);
    pInt_MatOffB = pTemp_Int;

    x1 += xx2;
    x1 += 34;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_COL_MULT_R1);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LANGDEF_COL_MULT_R2);
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->B_MultR);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->B_MultR);
    pFloatTemp->SetModifyData( -2.0, 2.0, 0.1, 0.01);
    pFloat_MatMulBR = pFloatTemp;

    x1 += xx2;
    x1 += 34;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_COL_MULT_G1);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LANGDEF_COL_MULT_G2);
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->B_MultG);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->B_MultG);
    pFloatTemp->SetModifyData( -2.0, 2.0, 0.1, 0.01);
    pFloat_MatMulBG = pFloatTemp;

    x1 += xx2;
    x1 += 34;

    pFloatTemp = new IqeFl_Float_Input( x1, y, xx2, yy, LANGDEF_COL_MULT_B1);
    pFloatTemp->type( FL_FLOAT_INPUT);
    pFloatTemp->tooltip( LANGDEF_COL_MULT_B2);
    pFloatTemp->SetFormat( "%.2f");
    pFloatTemp->SetValue( pToolData->B_MultB);
    pFloatTemp->callback( IqeB_GUI_Float_SetValue_Callback, &pToolData->B_MultB);
    pFloatTemp->SetModifyData( -2.0, 2.0, 0.1, 0.01);
    pFloat_MatMulBB = pFloatTemp;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Group IHS
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Color_TabD1=IHS"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Color_TabD1a=RGB <-> IHS conversions"));

    y += 8;

    xx2 = 95;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabD2=RGB -> IHS"));
    pRadioButTemp->tooltip(  LangStringLookup( "&GUI_Color_TabD2a=Converts an RGB image into an IHS image."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_RGB_2_IHS);
    ColorButtons[ YAIPS_COLOR_GUI_RGB_2_IHS] = pRadioButTemp;

    x1 += xx2;

    xx2 = 95;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabD3=IHS -> RGB"));
    pRadioButTemp->tooltip(  LangStringLookup( "&GUI_Color_TabD3a=Converts an IHS image to an RGB image."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_IHS_2_RGB);
    ColorButtons[ YAIPS_COLOR_GUI_IHS_2_RGB] = pRadioButTemp;

    x1 += xx2;

    xx2 = 95;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabD4=Adjust"));
    pRadioButTemp->tooltip(  LangStringLookup( "&GUI_Color_TabD4a="
                                               "Adjust intensity, hue, and saturation."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_IHS_ADJUST);
    ColorButtons[ YAIPS_COLOR_GUI_IHS_ADJUST] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 120;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabD5=Intensity %"));
    pTemp_Int->tooltip(  LangStringLookup( "&GUI_Color_TabD5a=Adjusting the intensity."));
    pTemp_Int->SetValue( pToolData->IHS_IntAdjust);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->IHS_IntAdjust);
    pTemp_Int->SetModifyData( -100, 100, 10, 1);
    pIHS_IntAdjust = pTemp_Int;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 120;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabD6=Hue %"));
    pTemp_Int->tooltip(  LangStringLookup( "&GUI_Color_TabD6a=Adjusting the hue."));
    pTemp_Int->SetValue( pToolData->IHS_HueAdjust);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->IHS_HueAdjust);
    pTemp_Int->SetModifyData( -100, 100, 10, 1);
    pIHS_HueAdjust = pTemp_Int;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 120;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabD7=Saturation %"));
    pTemp_Int->tooltip(  LangStringLookup( "&GUI_Color_TabD7a=Adjusting saturation."));
    pTemp_Int->SetValue( pToolData->IHS_SatAdjust);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->IHS_SatAdjust);
    pTemp_Int->SetModifyData( -100, 100, 10, 1);
    pIHS_SatAdjust = pTemp_Int;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Brightness correction
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Color_TabE1=BC"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Color_TabE1a=Brightness correction"));

    y += 8;

    xx2 = 180;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabE2=Brightness correction"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Color_TabE2a="
                                              "The brightness in the image is corrected according\n"
                                              "to the deviations in a measurement window (AOI)."));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_BRIGHT_CORR);
    ColorButtons[ YAIPS_COLOR_GUI_BRIGHT_CORR] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 86;

    xx2 = 40;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LangStringLookup( "&GUI_Color_TabE3=Target R"));
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabE3a="
                                          "Target value for the red color channel\n"
                                          "or for a black and white image."));
    pTemp_Int->SetValue( pToolData->BC_TargetR);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->BC_TargetR);
    pTemp_Int->SetModifyData( 100, 255, 10, 1);
    pBC_TargetR = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_G);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabE4a=Target value for the green color channel."));
    pTemp_Int->SetValue( pToolData->BC_TargetG);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->BC_TargetG);
    pTemp_Int->SetModifyData( 100, 255, 10, 1);
    pBC_TargetG = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_COLOR_B);
    pTemp_Int->tooltip( LangStringLookup( "&GUI_Color_TabE5a=Target value for the blue color channel."));
    pTemp_Int->SetValue( pToolData->BC_TargetB);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->BC_TargetB);
    pTemp_Int->SetModifyData( 100, 255, 10, 1);
    pBC_TargetB = pTemp_Int;

    x1 += xx2;
    x1 += 16;

    pTemp_Button = new Fl_Button( x1, y, yy, yy, LangStringLookup( "&GUI_Color_TabE6=#"));
    pTemp_Button->callback( YaIPS_GUI_PickColor);
    pTemp_Button->tooltip( LangStringLookup( "&GUI_Color_TabE6a="
                                             "Take the measured value from the measurement\n"
                                             "window (AOI) as the target value."));
    pBC_PickColor = pTemp_Button;

    // Next line

    x1  = 4;
    y += yy + 4;

    x1 += 69;

    xx2 = 44;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_AOI_LEFT);
    pTemp_Int->tooltip( LANGDEF_AOI_LEFT_TOOLTIP);
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->BC_AOI.XPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->BC_AOI.XPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pBC_AOI_X = pTemp_Int;

    x1 += xx2;
    x1 += 50;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_AOI_TOP);
    pTemp_Int->tooltip( LANGDEF_AOI_TOP_TOOLTIP);
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->BC_AOI.YPos);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->BC_AOI.YPos);
    pTemp_Int->SetModifyData( 0, 4096, 10, 1);
    pBC_AOI_Y = pTemp_Int;

    x1 += xx2;
    x1 += 8;

    pTemp_Button = new Fl_Button( x1, y, 28, 28, "@+1pencil");
    pTemp_Button->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->BC_AOI_Teach);
    pTemp_Button->tooltip( LANGDEF_AOI_TEACH_TOOLTIP);
    pTemp_Button->labelcolor( YAIPS_BCOL_BUTTON);
    pTemp_Button->shortcut( FL_COMMAND+'t');       // Short cut key
    pTeachToggle = pTemp_Button;

    // Next line

    x1  = 4;
    y += yy;

    x1 += 69;

    xx2 = 44;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_AOI_WIDTH);
    pTemp_Int->tooltip( LANGDEF_AOI_WIDTH_TOOLTIP);
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->BC_AOI.XSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->BC_AOI.XSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pBC_AOI_XX = pTemp_Int;

    x1 += xx2;
    x1 += 50;

    pTemp_Int = new IqeFl_Int_Input( x1, y, xx2, yy, LANGDEF_AOI_HEIGHT);
    pTemp_Int->tooltip( LANGDEF_AOI_HEIGHT_TOOLTIP);
    pTemp_Int->align( FL_ALIGN_LEFT);     // align for label
    pTemp_Int->SetValue( pToolData->BC_AOI.YSize);
    pTemp_Int->callback( IqeB_GUI_Int_SetValue_Callback, &pToolData->BC_AOI.YSize);
    pTemp_Int->SetModifyData( YAIPS_IDISP_AOI_MIN_SIZE, 1024, 10, 1);
    pBC_AOI_YY = pTemp_Int;

    // Finish things for this group

    pTemp_Group->end();

  //
  // Mix color channels
  //

  y = yGroup;
  x1  = 4;

  pTemp_Group = new Fl_Group( x1, y, pMyParWin->w() - x1 - 4, pMyParWin->h() - y - 4, LangStringLookup( "&GUI_Color_TabF1=Mix"));
  pTemp_Group->tooltip( LangStringLookup( "&GUI_Color_TabF1a=Mixing color channels to create a new image"));

    y += 8;

    xx2 = 76;

    pRadioButTemp = new Fl_Radio_Round_Button( x1, y, xx2 - 2, yy, LangStringLookup( "&GUI_Color_TabF2=Mixing"));
    pRadioButTemp->tooltip( LangStringLookup( "&GUI_Color_TabF2a="
                           "Mixing color channels to create a new image.\n"
                           "\n"
                           "* No image selected\n"
                           "  An alpha channel is removed from the input image\n"
                           "* G and B selected\n"
                           "  Input, G, and B must be black/white images.\n"
                           "  A color image is generated (input is R).\n"
                           "* Alpha selected\n"
                           "  Output image has an alpha channel.\n"
                           "  If available, the alpha channel is used.\n"
                           "  Otherwise, alpha must be a black/white image.\n"));
    pRadioButTemp->callback( YaIPS_Color_Callback, (void *)YAIPS_COLOR_GUI_MIX_CHANNELS);
    ColorButtons[ YAIPS_COLOR_GUI_MIX_CHANNELS] = pRadioButTemp;

    // Next line

    x1  = 4;
    y += yy + 4;

    // Input element

    x1 += 90;
    xx2 = 165;

    pMix_In_A_Box = new Fl_Box( x1, y, xx2, yy + 2);
    pMix_In_A_Box->box( FL_BORDER_BOX);
    pMix_In_A_Box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pMix_In_A_Box->labelsize( 18);
    pMix_In_A_Box->copy_label( "---");

    // Legend
    pTemp_Box = new Fl_Box( x1 - 80, y, 80, yy + 2, LangStringLookup( "&GUI_Color_TabF3=Alpha"));
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    //x/pTemp_Box->labelsize( 10);

    x1 += xx2 - 1;

    xx2 = yy;

    pMix_In_A_But = new Fl_Button( x1, y, xx2, yy + 2, "@2>");
    pMix_In_A_But->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Mix_In_A_WinIdNr);
    pMix_In_A_But->tooltip( LangStringLookup( "&GUI_Color_TabF3a=Select image for alpha channel"));
    pMix_In_A_But->labelcolor( YAIPS_BCOL_BUTTON);
    pMix_In_A_But->box( FL_BORDER_BOX);

    // Next line

    x1  = 4;
    y += yy + 6;

    // Input element

    x1 += 90;
    xx2 = 165;

    pMix_In_G_Box = new Fl_Box( x1, y, xx2, yy + 2);
    pMix_In_G_Box->box( FL_BORDER_BOX);
    pMix_In_G_Box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pMix_In_G_Box->labelsize( 18);
    pMix_In_G_Box->copy_label( "---");

    // Legend
    pTemp_Box = new Fl_Box( x1 - 80, y, 80, yy + 2, LANGDEF_COLOR_G);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    //x/pTemp_Box->labelsize( 10);

    x1 += xx2 - 1;

    xx2 = yy;

    pMix_In_G_But = new Fl_Button( x1, y, xx2, yy + 2, "@2>");
    pMix_In_G_But->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Mix_In_G_WinIdNr);
    pMix_In_G_But->tooltip( LangStringLookup( "&GUI_Color_TabF4a=Select image for the green channel"));
    pMix_In_G_But->labelcolor( YAIPS_BCOL_BUTTON);
    pMix_In_G_But->box( FL_BORDER_BOX);

    // Next line

    x1  = 4;
    y += yy + 4;

    // Input element

    x1 += 90;
    xx2 = 165;

    pMix_In_B_Box = new Fl_Box( x1, y, xx2, yy + 2);
    pMix_In_B_Box->box( FL_BORDER_BOX);
    pMix_In_B_Box->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pMix_In_B_Box->labelsize( 18);
    pMix_In_B_Box->copy_label( "---");

    // Legend
    pTemp_Box = new Fl_Box( x1 - 80, y, 80, yy + 2, LANGDEF_COLOR_B);
    pTemp_Box->box( FL_NO_BOX);
    pTemp_Box->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    //x/pTemp_Box->labelsize( 10);

    x1 += xx2 - 1;

    xx2 = yy;

    pMix_In_B_But = new Fl_Button( x1, y, xx2, yy + 2, "@2>");
    pMix_In_B_But->callback( IqeB_GUI_Misc_SetValue_Callback, &pToolData->Mix_In_B_WinIdNr);
    pMix_In_B_But->tooltip( LangStringLookup( "&GUI_Color_TabF5a=Select image for the blue channel"));
    pMix_In_B_But->labelcolor( YAIPS_BCOL_BUTTON);
    pMix_In_B_But->box( FL_BORDER_BOX);

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
  Fl_Button *pGUI_Parameter;             // Open the parameter dialog
  Fl_Button *pGUI_TeachToggle;           // Toggle Teach / Inspection

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

    pToolData->InputA_Change = 0;                // Reset image change check
    pToolData->InputG_Change = 0;
    pToolData->InputB_Change = 0;

    // ...

    Fl_Group *pGUI_GroupTopSide;                 // Top side of window
    Fl_Box   *pBoxTemp;
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
    pGUI_Img_ShowOnBig->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pGUI_Img_ShowOnBig->tooltip( LANGDEF_SHOW_ON_BIG_IMAGE);
    pGUI_Img_ShowOnBig->labelcolor( YAIPS_BCOL_SHOW_OTHER);

    x1 += xx + 5;

    xx = yy * 2 + yy / 2;

    // Input element
    pBox_Input1 = new Fl_Box( x1, y + 10, xx, yy - 10);
    pBox_Input1->box( FL_BORDER_BOX);
    pBox_Input1->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pBox_Input1->labelsize( 18);
    pBox_Input1->copy_label( "---");

    // Legend
    pBoxTemp = new Fl_Box( x1, y - 3, xx, 12, LANGDEF_INPUT);
    pBoxTemp->box( FL_NO_BOX);
    pBoxTemp->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    pBoxTemp->labelsize( 10);

    x1 += xx - 1;

    xx = yy - 10;

    pBut_Input1 = new Fl_Button( x1, y + 10, xx, yy - 10, "@2>");
    pBut_Input1->callback( YaIPS_ToolWin_GUI_Callback, (long int)iToolData);
    pBut_Input1->tooltip( LANGDEF_SELECT_INPUT_IMAGE);
    pBut_Input1->labelcolor( YAIPS_BCOL_BUTTON);
    pBut_Input1->box( FL_BORDER_BOX);

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

      pToolData->Input1_Change = 0;                    // Force recalculation output
    }

  } else if( w == pMyToolWin->pGUI_Parameter) {                // Open parameter dialog

    YaIPS_GUI_ParameterWin( pMyToolWin->x() + 16, pMyToolWin->y() + 16, iToolData);

  } else if( w == pMyToolWin->pGUI_TeachToggle) {              // Toggle Teach / Inspection

    if( pToolData->ColorType == YAIPS_COLOR_GUI_BRIGHT_CORR) { // Only usable for brightness correction

      pToolData->BC_AOI_Teach = ! pToolData->BC_AOI_Teach;

      pToolData->Input1_Change = 0;          // Force recalculation output
    }
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

  // Update color of Toggle Teach / Inspection button

  int MouseTeachState;

  MouseTeachState = 1;                                            // We have a mouse callback state

  // Is teach mode available
  if( DoEnable &&                                                 // Enable GUI elements
      pToolData->ColorType == YAIPS_COLOR_GUI_BRIGHT_CORR) {      // Only usable for brightness correction

    MouseTeachState = pToolData->BC_AOI_Teach ? 3 : 2;
  }

  pToolData->YaIPS_ImageDisp.pImage_Box->MouseTeachState = MouseTeachState;  // Shadow setting of teach

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

    int Input1_Check, Input1_ImageChanged, ForceUpdate;
    Fl_RGB_Image *pImgIn1;

    ForceUpdate = false;                         // Preset: NO Force update of output image

    // Check input image and visualize state
    Input1_Check = YaIPS_ToolWinInputCheck( MY_WIN_ID + iToolData, pToolData->Input1_WinIdNr,
                                            NULL, &pImgIn1, &Input1_ImageChanged);

    if( Input1_Check != 0) {                                 // Input image is not valid

      // Empty a display image
      YaIPS_ImageDispEmpty( &pToolData->YaIPS_ImageDisp);

    } else {

      Fl_RGB_Image *pImgMix_In_A, *pImgMix_In_G, *pImgMix_In_B;

      pImgMix_In_A = NULL;       // Will be set if there is a valid image
      pImgMix_In_G = NULL;
      pImgMix_In_B = NULL;

      if( pToolData->ColorType == YAIPS_COLOR_GUI_MIX_CHANNELS) {   // Check input images for Mix color channels

        int InputA_ImageChanged, InputG_ImageChanged, InputB_ImageChanged;


        YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Mix_In_A_WinIdNr, NULL, &pImgMix_In_A, &InputA_ImageChanged);
        YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Mix_In_G_WinIdNr, NULL, &pImgMix_In_G, &InputG_ImageChanged);
        YaIPS_ToolWinInputCheck( MY_WIN_ID + pToolData->iToolData, pToolData->Mix_In_B_WinIdNr, NULL, &pImgMix_In_B, &InputB_ImageChanged);

        if( InputA_ImageChanged != pToolData->InputA_Change ||     // Any of the images has changed since the last call
            InputG_ImageChanged != pToolData->InputG_Change ||
            InputB_ImageChanged != pToolData->InputB_Change) {

          pToolData->InputA_Change = InputA_ImageChanged;          // Copy new image count
          pToolData->InputG_Change = InputG_ImageChanged;
          pToolData->InputB_Change = InputB_ImageChanged;

          pToolData->Input1_Change = 0;                            // Fore new compute
        }
      }

      if( Input1_ImageChanged != pToolData->Input1_Change) { // Image count is different

        pToolData->Input1_Change = Input1_ImageChanged;        // Image is processed

        // Do the image processing

        ierr = 0;                                              // Reset error
        errstring = NULL;                                      // Reset error string
        BC_AOI_Histo.nHistos = 0;                              // No Histograms

        switch( pToolData->ColorType) {

        default:

          ierr = YaIPS_RGB_Color_ConvSimple( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, YAIPS_COLOR_RGB_2_I);
          break;

        case YAIPS_COLOR_GUI_RGB_2_R:
        case YAIPS_COLOR_GUI_RGB_2_G:
        case YAIPS_COLOR_GUI_RGB_2_B:
        case YAIPS_COLOR_GUI_GET_ALPHA:
        case YAIPS_COLOR_GUI_RGB_2_I:
        case YAIPS_COLOR_GUI_RGB_2_H:
        case YAIPS_COLOR_GUI_RGB_2_S:
        case YAIPS_COLOR_GUI_RGB_MIN:
        case YAIPS_COLOR_GUI_RGB_MAX:
        case YAIPS_COLOR_GUI_RGB_2_BGR:

          ierr = YaIPS_RGB_Color_ConvSimple( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1, pToolData->ColorType);
          break;

        case YAIPS_COLOR_GUI_LUT_TABLE:

          memset( &pToolData->TempLUT, 0, sizeof( pToolData->TempLUT));  // Zero all

          pToolData->TempLUT.FalseColor = pToolData->LUT_Table;          // Set LUT data
          pToolData->TempLUT.Invert     = pToolData->LUT_Invert;

          YaIPS_ColModCalcLUT( &pToolData->TempLUT, 0);              // Calculate predefined lookup table

          ierr = YaIPS_RGB_Color_ConvLUT( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                          pToolData->TempLUT.LookupR, pToolData->TempLUT.LookupG, pToolData->TempLUT.LookupB);
          break;

        case YAIPS_COLOR_GUI_LUT_GAINCH:

          memset( &pToolData->TempLUT, 0, sizeof( pToolData->TempLUT));  // Zero all

          YaIPS_RGB_Color_MakeLUT_OffsetGain( pToolData->TempLUT.LookupR, pToolData->TempLUT.LookupG, pToolData->TempLUT.LookupB,
                                              pToolData->OffsetR, pToolData->OffsetG, pToolData->OffsetB,
                                              pToolData->GainChR, pToolData->GainChG, pToolData->GainChB);

          ierr = YaIPS_RGB_Color_ConvLUT( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                          pToolData->TempLUT.LookupR, pToolData->TempLUT.LookupG, pToolData->TempLUT.LookupB);
          break;

        case YAIPS_COLOR_GUI_LUT_GAMMA:

          memset( &pToolData->TempLUT, 0, sizeof( pToolData->TempLUT));  // Zero all

          YaIPS_RGB_Color_MakeLUT_Gamma( pToolData->TempLUT.LookupR, pToolData->TempLUT.LookupG, pToolData->TempLUT.LookupB,
                                         pToolData->GammaR, pToolData->GammaG, pToolData->GammaB);

          ierr = YaIPS_RGB_Color_ConvLUT( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                          pToolData->TempLUT.LookupR, pToolData->TempLUT.LookupG, pToolData->TempLUT.LookupB);
          break;

        case YAIPS_COLOR_GUI_LUT_CONTRAST:

          memset( &pToolData->TempLUT, 0, sizeof( pToolData->TempLUT));  // Zero all

          YaIPS_RGB_Color_MakeLUT_Contrast( pToolData->TempLUT.LookupR, pToolData->TempLUT.LookupG, pToolData->TempLUT.LookupB,
                                            pToolData->Contrast1in, pToolData->Contrast1out, pToolData->Contrast2in, pToolData->Contrast2out);

          ierr = YaIPS_RGB_Color_ConvLUT( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                          pToolData->TempLUT.LookupR, pToolData->TempLUT.LookupG, pToolData->TempLUT.LookupB);
          break;

        case YAIPS_COLOR_GUI_LUT_DUOTONE:

          memset( &pToolData->TempLUT, 0, sizeof( pToolData->TempLUT));  // Zero all

          YaIPS_RGB_Color_MakeLUT_Duotone( pToolData->TempLUT.LookupR, pToolData->TempLUT.LookupG, pToolData->TempLUT.LookupB,
                                           pToolData->Duotone_Br_Color, pToolData->Duotone_Da_Color,
                                           pToolData->Duotone_contrast, pToolData->Duotone_brightness);

          ierr = YaIPS_RGB_Color_ConvLUT( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                          pToolData->TempLUT.LookupR, pToolData->TempLUT.LookupG, pToolData->TempLUT.LookupB,
                                          pToolData->Duotone_RGB_Fade);
          break;

        case YAIPS_COLOR_GUI_RGB_2_IHS:

          ierr = YaIPS_RGB_Color_RGB2IHS( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1);
          break;

        case YAIPS_COLOR_GUI_IHS_2_RGB:

          ierr = YaIPS_RGB_Color_IHS2RGB( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1);
          break;

        case YAIPS_COLOR_GUI_IHS_ADJUST:

          ierr = YaIPS_RGB_Color_IHS_Adjust( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                           pToolData->IHS_IntAdjust, pToolData->IHS_HueAdjust, pToolData->IHS_SatAdjust);
          break;

        case YAIPS_COLOR_GUI_MATRIX:

          ierr = YaIPS_RGB_Color_ConvMatrix( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                             pToolData->R_Offset, pToolData->R_MultR, pToolData->R_MultG, pToolData->R_MultB,
                                             pToolData->G_Offset, pToolData->G_MultR, pToolData->G_MultG, pToolData->G_MultB,
                                             pToolData->B_Offset, pToolData->B_MultR, pToolData->B_MultG, pToolData->B_MultB);
          break;

        case YAIPS_COLOR_GUI_BRIGHT_CORR:

          // Get histograms and statistical data

          YaIPS_Histo_Measure( pImgIn1, &BC_AOI_Histo,
                               pToolData->BC_AOI.XPos, pToolData->BC_AOI.YPos, pToolData->BC_AOI.XSize, pToolData->BC_AOI.YSize);

          // Brightness correction via LUT table

          {
            float GainChR, GainChG, GainChB;

            memset( &pToolData->TempLUT, 0, sizeof( pToolData->TempLUT));  // Zero all

            if( BC_AOI_Histo.nHistos >= 3) {            // Color image

              GainChR = (100.0 * pToolData->BC_TargetR / BC_AOI_Histo.R.Average) - 100.0;
              if( GainChR > 400.0) GainChR = 400.0;
              if( GainChR < -90.0) GainChR = -90.0;

              GainChG = (100.0 * pToolData->BC_TargetG / BC_AOI_Histo.G.Average) - 100.0;
              if( GainChG > 400.0) GainChG = 400.0;
              if( GainChG < -90.0) GainChG = -90.0;

              GainChB = (100.0 * pToolData->BC_TargetB / BC_AOI_Histo.B.Average) - 100.0;
              if( GainChB > 400.0) GainChB = 400.0;
              if( GainChB < -90.0) GainChB = -90.0;

            } else if( BC_AOI_Histo.nHistos >= 1) {    // Black / white image

              GainChR = (100.0 * pToolData->BC_TargetR / BC_AOI_Histo.R.Average) - 100.0;
              if( GainChR > 400.0) GainChR = 400.0;
              if( GainChR < -90.0) GainChR = -90.0;

              GainChG = GainChR;
              GainChB = GainChR;

            } else {

              // Change nothing

              GainChR = 0.0;
              GainChG = 0.0;
              GainChB = 0.0;
            }

            YaIPS_RGB_Color_MakeLUT_OffsetGain( pToolData->TempLUT.LookupR, pToolData->TempLUT.LookupG, pToolData->TempLUT.LookupB,
                                                0, 0, 0,
                                                GainChR, GainChG, GainChB);

            ierr = YaIPS_RGB_Color_ConvLUT( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,
                                            pToolData->TempLUT.LookupR, pToolData->TempLUT.LookupG, pToolData->TempLUT.LookupB);
          }

          break;

        case YAIPS_COLOR_GUI_MIX_CHANNELS:

          ierr = YaIPS_RGB_MixChannels( &pToolData->YaIPS_ImageDisp.pImage_Img, pImgIn1,   // Output image and common input image
                                        pImgMix_In_G, pImgMix_In_B, pImgMix_In_A);         // Optional green, blue and alpha channels
          break;

        } // end switch

        // Has a valid output image

        if( ierr == 0) {                                  // Have a result image

          YaIPS_ImageDispUpdateByChangedImage( &pToolData->YaIPS_ImageDisp, MY_WIN_ID + iToolData, (char *)MY_WIN_GUI_NAME);

          YaIPS_ImageDispStrDebug( &pToolData->YaIPS_ImageDisp); // Reset error message

          ForceUpdate = true;                        // Force update of output image

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
 * IqeB_GUI_ColorWinIntern
 *
 * Open a specific window
 */

static void IqeB_GUI_ColorWinIntern( int xLeft, int xRight, int yTop, int yBotton, int iToolData)
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

  pToolData->iToolData = iToolData;                        // Set sub window number

  pToolData->IsOpen = true;                                // Flag image as open

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

  // Draw LUT ?

  if( pToolData->ColorType >= YAIPS_COLOR_GUI_LUT_FIRST &&
      pToolData->ColorType <= YAIPS_COLOR_GUI_LUT_LAST) {

    // Draw LUT

    YaIPS_ColMod_LUT_Draw( pYaIPS_ImageDisp->pImage_Box,
                           &pToolData->TempLUT, 9, 6, true);
    return;
  }

  // Draw AOI for brightness correction ?

  if( pToolData->ColorType == YAIPS_COLOR_GUI_BRIGHT_CORR &&
      pYaIPS_ImageDisp->pImage_Img != NULL) {

    int AOI_XX, AOI_YY, LineWidth;
    int x1, y1, x, y, xx, yy, xxo, yyo, OffX, OffY, DrawText;
    char TempString[ 256];

    // Preparations

    x1 = pYaIPS_ImageDisp->BigImage_sx;
    y1 = pYaIPS_ImageDisp->BigImage_sy;
    xx = pYaIPS_ImageDisp->BigImage_sw;
    yy = pYaIPS_ImageDisp->BigImage_sh;

    // Points relative to image

    OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
    OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

    LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

    // Get AOI

    if( pToolData->BC_AOI.XSize >= pYaIPS_ImageDisp->pImage_Img->w()) {

      AOI_XX = pYaIPS_ImageDisp->pImage_Img->w();
    } else {
      AOI_XX = pToolData->BC_AOI.XSize;
    }

    if( pToolData->BC_AOI.YSize >= pYaIPS_ImageDisp->pImage_Img->h()) {

      AOI_YY = pYaIPS_ImageDisp->pImage_Img->h();
    } else {
      AOI_YY = pToolData->BC_AOI.YSize;
    }

    // Clipping ?

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( x1, y1, xx, yy);
    }

    // Prepare font size

    DrawText = false;                                            // Preset, do not draw text

    if( pYaIPS_ImageDisp->PixelImageToScreen >= 0.33) {              // Is NOT to tiny

      int TempFontSize;

      DrawText = true;                                           // Draw crcdf text

      TempFontSize = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 12.0 + 0.5);

      if( TempFontSize < 10) {
        TempFontSize = 10;
      }

      fl_font( FL_HELVETICA, TempFontSize);
    }

    // Draw AOI

    fl_line_style( 0, LineWidth);   // Set line width

    // Draw color for AOI
    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0) {  // Mouse is over the AOI

      fl_color( FL_RED);
    } else {

      fl_color( FL_GREEN - 2);
    }

    x = (int)( (pToolData->BC_AOI.XPos - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    y = (int)( (pToolData->BC_AOI.YPos - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

    xxo = (int)( AOI_XX * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
    yyo = (int)( AOI_YY * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

    fl_rect( x1 + x, y1 + y, xxo, yyo);

    // Draw label and quality of correlation to screen

    if( DrawText) {                                                 // Draw text

      int mdx, mdy, mw, mh;

      if( BC_AOI_Histo.nHistos == 3) {            // Color image

        sprintf( TempString, "%d/%d/%d", (int)(BC_AOI_Histo.R.Average + 0.5), (int)(BC_AOI_Histo.G.Average + 0.5), (int)(BC_AOI_Histo.B.Average + 0.5));

      } else if( BC_AOI_Histo.nHistos == 1) {    // Black / white image

        sprintf( TempString, "%d", (int)(BC_AOI_Histo.R.Average + 0.5));

      } else {

        sprintf( TempString, "---");
      }

      fl_text_extents( TempString, mdx, mdy, mw, mh);

      if( y + mdy < 4) {         // To near to upper border

        y += mh + 5;             // Show below upper frame
        x += 4;

      } else {                   // Fits above upper frame

        y -= 4;
      }

      fl_draw( TempString, x1 + x, y1 + y);
    }

    goto ExitPoint;
  }


ExitPoint:

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

  YaIPS_GUI_MyDrawAfter_Func( pYaIPS_ImageDisp, pToolData, false);
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

      if( pToolData->ColorType == YAIPS_COLOR_GUI_BRIGHT_CORR &&             // Need an AOI
          pToolData->BC_AOI_Teach != 0) {                                    // and can be changed with the mouse

        pAOI_This = &pToolData->BC_AOI;

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
 * IqeB_GUI_ColorWin
 *
 * Open a window to show images loaded from files
 *
 * SubWinIDx:  < 0 if called from menu
 *            >= 0 if called during startup of the application
 */

void IqeB_GUI_ColorWin( int xLeft, int xRight, int yTop, int yBotton, int SubWinIDx)
{
  int iToolData, iUnused;

  if( SubWinIDx >= 0) {        // Call a specific sub-window at startup

    // Register draw after function for big image display
    YaIPS_ToolWinDrawAfterSet( MY_WIN_ID + SubWinIDx, YaIPS_GUI_MyDrawAfter_Other);

	  IqeB_GUI_ColorWinIntern( xLeft, xRight, yTop, yBotton, SubWinIDx);

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

  IqeB_GUI_ColorWinIntern( xLeft, xRight, yTop, yBotton, iUnused);
}

/************************* End Of File *************************/


