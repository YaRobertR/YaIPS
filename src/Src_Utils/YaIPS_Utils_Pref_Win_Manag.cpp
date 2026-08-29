/****************************************************************************

  YaIPS_Utils_Pref_Win_Manag.cpp

  Manage preferences and windows.

  03.01.2025 RR: First edition of this file.

*****************************************************************************
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <sysinfoapi.h>

#include <iostream>
using namespace std;

#include "YaIPS.h"

//-----------------------------------------------------------------------------------
// Preferences data things
//-----------------------------------------------------------------------------------

unsigned int IqeB_PreferencesUpdateChanges_TimeLastCalled;  // remember time last called

// hold preferences data

static Fl_Preferences IqeB_PreferencesData( Fl_Preferences::USER, "ya3dag.de", "YaIPS");

// Remember picked up preference settings, used to save a program close

typedef struct {
  const char *pGroupName;               // Name of this group
  Fl_Preferences *pGroup;               // Group for this preference
  T_GUI_PreferenceEntry *pSettings;     // pointer to seting table
  int nSettings;                        // Number of settings for this groupd
  void **ppMyToolWin;                   // If != NULL, pointer to Fl_Double_Window pointer of a tool window
  int *pWinPosX, *pWinPosY;             // point to variable for windows position
  int LWinPosX, LWinPosY;               // last position of tool window
} T_GUI_PreferenceGroup;

static T_GUI_PreferenceGroup IqeB_PreferenceGroup[ PREFERENCE_MAX_GROUPS];

static int IqeB_PreferenceGroupN = 0;  // number of preference groups

//-----------------------------------------------------------------------------------
// Windows manager data
//-----------------------------------------------------------------------------------

// There is one entry for each window which can be open.
// Index in this table is one of the YAIPS_WIN_ID_XXX defines.
// Sub-windows has each on entries.

typedef struct {
  void **ppMyToolWin;                   // If != NULL, pointer to Fl_Double_Window pointer of a tool window
  char WinName[ WIN_ID_NAME_SIZE];      // GUI name of window
  int SubWinIDx;                        // Sub-window index. Starts with 0.
  int *pIsOpen;                         // Point to variable for 'window is open' flag
  YaIPS_WinStartup_Callback *pStartup;  // Point to optional startup function
  Fl_Callback *pClose;                  // Point to optional close function
  Fl_YaIPS_ImageDisp_t *pOutImage;      // Point to optional output image

  // Options. Must be registered by tool window at startup of window
  YaIPS_ToolWinDrawAfterCb *pBigDrawAfter;         // If != 0, pointer to optional draw after for big image display
  YaIPS_ToolWinChangeOutputCb *pChangeOutput;  // If != 0, pointer to optional change output for big image display
} T_YaIPS_WinManagerData;

static T_YaIPS_WinManagerData WinManagerTable[ YAIPS_WIN_MANAGER_MAX];

/************************************************************************************
* IqeB_PreferencesAddGroup
*
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
*  nTableElements         Number of multiple windows/data table
*                         Must be > 0 for multiple windows
*  SizeOfTableElement     Size of data table elements
*                         Must be > 0 for multiple windows
*
* NOTE: pGroupName must point to a constant string.
*/

static void IqeB_PreferencesAddGroup( const char *pPrefName, T_GUI_PreferenceEntry *pSettings, int nSettings,
                                      void **ppMyToolWin, int *pWinPosX, int *pWinPosY,
                                      int nTableElements, int SizeOfTableElement)
{
  int iSetting;
  T_GUI_PreferenceEntry *pSetting;

  // add to table of preference groups

  if( nTableElements <= 1 || SizeOfTableElement <= 0) {      // Has a single table

    if( IqeB_PreferenceGroupN >= PREFERENCE_MAX_GROUPS) {   // Security test, table overflows

      return;
    }

    IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pGroupName = pPrefName;
    IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pGroup = NULL;   // is set later
    IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pSettings = pSettings;
    IqeB_PreferenceGroup[IqeB_PreferenceGroupN].nSettings = nSettings;

    IqeB_PreferenceGroup[IqeB_PreferenceGroupN].ppMyToolWin = ppMyToolWin;
    IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pWinPosX = pWinPosX;
    IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pWinPosY = pWinPosY;

    IqeB_PreferenceGroupN += 1;   // one more

    // get default value for this settings

    pSetting = pSettings;

    for (iSetting = 0; iSetting < nSettings; iSetting++, pSetting++) { // loop over all settings

      switch (pSetting->Type) {

      case PREF_T_INT:

        if( pSetting->pDefaultValue[ 0] == '0' && pSetting->pDefaultValue[ 1] == 'x') {  // Hexadecimal string

          sscanf( pSetting->pDefaultValue, "0x%x", &pSetting->DVal_Int);

        } else {       // Decimal string

          pSetting->DVal_Int = atoi(pSetting->pDefaultValue);
        }
        break;

      case PREF_T_FLOAT:

        pSetting->DVal_Float = atof(pSetting->pDefaultValue);
        break;

      case PREF_T_DOUBLE:

        pSetting->DVal_Double = atof(pSetting->pDefaultValue);
        break;

      } // end switch
    } // for( iSetting

  } else {          // Has multiple tables

    int iTable, DataPointerAdd, SizeOfSettings;
    char TempString[ 256];
    char *pGroupNameTemp;

    for( iTable = 0; iTable < nTableElements; iTable++) {

      if( IqeB_PreferenceGroupN >= PREFERENCE_MAX_GROUPS) {   // Security test, table overflows

        return;
      }

      DataPointerAdd = SizeOfTableElement * iTable;

      sprintf( TempString, "%s-%d", pPrefName, iTable + 1);
      pGroupNameTemp = (char *)malloc( strlen( TempString + 1));
      strcpy( pGroupNameTemp, (const char *)TempString);

      // Clone settings info for each incarnation

      if( iTable == 0) {                 // First table element holds settings

        pSetting = pSettings;

      } else {                           // Other table elements

        // Copy setting from first

        SizeOfSettings = sizeof( T_GUI_PreferenceEntry) * nSettings;

        pSetting = (T_GUI_PreferenceEntry*)malloc( SizeOfSettings);

        memcpy( pSetting, pSettings, SizeOfSettings);
      }

      // Fill setting group

      IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pGroupName = pGroupNameTemp;
      IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pGroup = NULL;   // is set later
      IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pSettings = pSetting;
      IqeB_PreferenceGroup[IqeB_PreferenceGroupN].nSettings = nSettings;

      IqeB_PreferenceGroup[IqeB_PreferenceGroupN].ppMyToolWin = ppMyToolWin;
      if( IqeB_PreferenceGroup[IqeB_PreferenceGroupN].ppMyToolWin != NULL) {

        IqeB_PreferenceGroup[IqeB_PreferenceGroupN].ppMyToolWin = (void **)(((char *)IqeB_PreferenceGroup[IqeB_PreferenceGroupN].ppMyToolWin) + DataPointerAdd);
      }

      IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pWinPosX = pWinPosX;
      if( IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pWinPosX != NULL) {

        IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pWinPosX = (int *)(((char *)IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pWinPosX) + DataPointerAdd);
      }

      IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pWinPosY = pWinPosY;
      if( IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pWinPosY != NULL) {

        IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pWinPosY = (int *)(((char *)IqeB_PreferenceGroup[IqeB_PreferenceGroupN].pWinPosY) + DataPointerAdd);
      }

      IqeB_PreferenceGroupN += 1;   // one more

      // get default value for this settings

      for (iSetting = 0; iSetting < nSettings; iSetting++, pSetting++) { // loop over all settings

        if( pSetting->pValue != NULL) {

          pSetting->pValue = (void *)((char *)pSetting->pValue + DataPointerAdd);
        }

        switch (pSetting->Type) {

        case PREF_T_INT:

          if( pSetting->pDefaultValue[ 0] == '0' && pSetting->pDefaultValue[ 1] == 'x') {  // Hexadecimal string

            sscanf( pSetting->pDefaultValue, "0x%x", &pSetting->DVal_Int);

          } else {       // Decimal string

            pSetting->DVal_Int = atoi(pSetting->pDefaultValue);
          }
          break;

        case PREF_T_FLOAT:

          pSetting->DVal_Float = atof(pSetting->pDefaultValue);
          break;

        case PREF_T_DOUBLE:

          pSetting->DVal_Double = atof(pSetting->pDefaultValue);
          break;

        } // end switch
      } // for( iSetting

    }
  }

  return;
}

/************************************************************************************
 * Helper class IqeB_PreferencesGroup
 * Automatic add preference settings at startup of the program.
 */

/**
  The constructor adds a preference setting group.
*/
IqeB_PreferencesGroup::IqeB_PreferencesGroup( const char *pPrefName, T_GUI_PreferenceEntry *pSettings, int nSettings,
                                              void **ppMyToolWin, int *pWinPosX, int *pWinPosY,
                                              int WinSrcID, int nWindows, int SizeOfTableElement,
                                              int *pIsOpen, YaIPS_WinStartup_Callback *pStartup, Fl_Callback *pClose,
                                              const char *pGuiName, Fl_YaIPS_ImageDisp_t *pOutImage)
{
  int SubWinID, DataPointerAdd;
  T_YaIPS_WinManagerData *pWinManagData;
  char TempString[ 256];

  // Add preference data
  IqeB_PreferencesAddGroup( pPrefName, pSettings, nSettings, ppMyToolWin, pWinPosX, pWinPosY, nWindows, SizeOfTableElement);

  // Add to windows manager data

  if( WinSrcID <= 0)  {              // Security test need windows source ID

    return;
  }

  if( nWindows < 1) {               // Security test, minimum must be 1

    return;
  }

  if( nWindows > 1 && SizeOfTableElement <= 0) {  // More than one window, need size of table element

    return;
  }

  if( WinSrcID + nWindows > YAIPS_WIN_MANAGER_MAX) {  // Test for table overflow

    return;
  }

  pWinManagData = WinManagerTable + WinSrcID;       // Point to first window

  for( SubWinID = 0; SubWinID < nWindows; SubWinID++, pWinManagData++) {

    DataPointerAdd = SizeOfTableElement * SubWinID;

    if( pGuiName != NULL) {                         // Have a GUI name
      if( nWindows > 1) {                           // more than one window
        sprintf( TempString, "%d %s", SubWinID + 1, pGuiName);
      } else {
        sprintf( TempString, "%s", pGuiName);
      }
    } else {                                        // Use the preference name

      if( nWindows > 1) {                           // more than one window
        sprintf( TempString, "%d %s", SubWinID + 1, pPrefName);
      } else {
        sprintf( TempString, "%s", pPrefName);
      }
    }

    pWinManagData->ppMyToolWin = ppMyToolWin;
    if( pWinManagData->ppMyToolWin != NULL) {
      pWinManagData->ppMyToolWin = (void **)(((char *)pWinManagData->ppMyToolWin) + DataPointerAdd);
    }

    strncpy( pWinManagData->WinName, TempString, sizeof( pWinManagData->WinName) - 1);
    pWinManagData->SubWinIDx = SubWinID;

    pWinManagData->pIsOpen = pIsOpen;
    if( pIsOpen != NULL) {
      pWinManagData->pIsOpen = (int *)((char *)pIsOpen + DataPointerAdd);
    }

    pWinManagData->pStartup  = pStartup;
    pWinManagData->pClose    = pClose;
    pWinManagData->pOutImage = pOutImage;
    if( pOutImage != NULL) {
      pWinManagData->pOutImage = (Fl_YaIPS_ImageDisp_t *)((char *)pOutImage + DataPointerAdd);
    }
  }
}

/**
  The destructor has nothing to do.
*/
IqeB_PreferencesGroup::~IqeB_PreferencesGroup()
{
}

/************************************************************************************
 * IqeB_PreferencesUpdateChanges
 *
 * Update changed settings to preferenes and
 * flush the preferences database.
 * Is called on program close and maybe on value changes.
 */

void IqeB_PreferencesUpdateChanges()
{
  int iGroup, iSetting, AnyValueChanged;
  T_GUI_PreferenceGroup *pSettingGroup;
  T_GUI_PreferenceEntry *pSetting;

  AnyValueChanged = false;
  IqeB_PreferencesUpdateChanges_TimeLastCalled = GetTickCount();  // rember time last called

  // ...

  pSettingGroup = IqeB_PreferenceGroup;

  for( iGroup = 0; iGroup < IqeB_PreferenceGroupN; iGroup++, pSettingGroup++) {  // loop over all groups

    if( pSettingGroup->pGroup == NULL) {  // Security test

      continue;
    }

    // update window positions

    if( pSettingGroup->ppMyToolWin != NULL &&   // have pointer to pointer
        *pSettingGroup->ppMyToolWin != NULL) {  // and have pointer

      Fl_Double_Window *pThisWin;

      pThisWin = (Fl_Double_Window *)*pSettingGroup->ppMyToolWin; // get pointer to tool window

      if( pThisWin != pGUI_Main ||                     // If this is NOT the main windows
          pGUI_Main->fullscreen_active() == 0) {       // or main window is NOT full screen

        // update window positions

        if( pSettingGroup->pWinPosX != NULL) {   // X position

          *pSettingGroup->pWinPosX = pThisWin->x_root();  // update window position

          if( pSettingGroup->LWinPosX != *pSettingGroup->pWinPosX) {   // Value has changed

            AnyValueChanged = true;

            pSettingGroup->LWinPosX = *pSettingGroup->pWinPosX;

            pSettingGroup->pGroup->set( "WinPosX", *pSettingGroup->pWinPosX);
          }
        }

        if( pSettingGroup->pWinPosY != NULL) {   // Y position

          *pSettingGroup->pWinPosY = pThisWin->y_root();  // update window position

          if( pSettingGroup->LWinPosY != *pSettingGroup->pWinPosY) {   // Value has changed

            AnyValueChanged = true;

            pSettingGroup->LWinPosY = *pSettingGroup->pWinPosY;

            pSettingGroup->pGroup->set( "WinPosY", *pSettingGroup->pWinPosY);
          }
        }
      }
    }

   // update other settings

    pSetting = pSettingGroup->pSettings;

    for( iSetting = 0; iSetting < pSettingGroup->nSettings; iSetting++, pSetting++) {  // loop over all settings

      switch( pSetting->Type) {

      case PREF_T_INT:
        { int *pValue;

          pValue = (int *)pSetting->pValue;

          if( pSetting->LVal_Int != *pValue) {   // Value has changed

            AnyValueChanged = true;

            pSetting->LVal_Int = *pValue;

            pSettingGroup->pGroup->set( pSetting->pName, *pValue);
          }
        }
        break;

      case PREF_T_FLOAT:
        { float *pValue;

          pValue = (float *)pSetting->pValue;

          if( pSetting->LVal_Float != *pValue) {   // Value has changed

            AnyValueChanged = true;

            pSetting->LVal_Float = *pValue;

            pSettingGroup->pGroup->set( pSetting->pName, *pValue);
          }
        }
        break;

      case PREF_T_DOUBLE:
        { double *pValue;

          pValue = (double *)pSetting->pValue;

          if( pSetting->LVal_Double != *pValue) {   // Value has changed

            AnyValueChanged = true;

            pSetting->LVal_Double = *pValue;

            pSettingGroup->pGroup->set( pSetting->pName, *pValue);
          }
        }
        break;

      case PREF_T_STRING:
        { char *pValue;
          int CRC_Value, i, ThisLen;

          pValue = (char *)pSetting->pValue;

          // Calculate CRC

          CRC_Value = 4711;

          ThisLen = strlen( pValue);

          for( i = 0; i < ThisLen; i++) {

            CRC_Value ^= (pValue[ i] & 0xff) << ((i & 3) * 8);
          }

          if( pSetting->LVal_Int != CRC_Value) {   // Value has changed

            AnyValueChanged = true;

            pSettingGroup->pGroup->set( pSetting->pName, pValue);
          }
        }
        break;

      } // end switch
    } // for( iSetting
  } // for( iGroup

  if( AnyValueChanged) {           // Any value has changed ?

    IqeB_PreferencesData.flush();  // Flush the data base
  }

  return;
}

/************************************************************************************
 * IqeB_PreferencesGetFromFile
 *
 * Get the preferences from the file and store to the variables.
 * Only call this once at program start.
 * All preference groups must have been added.
 */

void IqeB_PreferencesGetFromFile()
{
  int iGroup, iSetting;
  T_GUI_PreferenceGroup *pSettingGroup;
  T_GUI_PreferenceEntry *pSetting;

  pSettingGroup = IqeB_PreferenceGroup;

  for( iGroup = 0; iGroup < IqeB_PreferenceGroupN; iGroup++, pSettingGroup++) {  // loop over all groups

    if( pSettingGroup->pGroup == NULL) {

      pSettingGroup->pGroup = new Fl_Preferences( IqeB_PreferencesData, pSettingGroup->pGroupName);

      if( pSettingGroup->pGroup == NULL) {   // Security test

        continue;
      }
    }

    // get window positions

    if( pSettingGroup->pWinPosX != NULL) {   // X position
      int ThisValue;

      pSettingGroup->pGroup->get( "WinPosX", ThisValue, IQE_GUI_NO_WINPOS_X);

      pSettingGroup->LWinPosX  = ThisValue;
      *pSettingGroup->pWinPosX = ThisValue;
    }

    if( pSettingGroup->pWinPosY != NULL) {   // Y position
      int ThisValue;

      pSettingGroup->pGroup->get( "WinPosY", ThisValue, IQE_GUI_NO_WINPOS_Y);

      pSettingGroup->LWinPosY  = ThisValue;
      *pSettingGroup->pWinPosY = ThisValue;
    }

    // get other settings

    pSetting = pSettingGroup->pSettings;

    for( iSetting = 0; iSetting < pSettingGroup->nSettings; iSetting++, pSetting++) {  // loop over all settings

      switch( pSetting->Type) {

      case PREF_T_INT:
        { int *pValue, ThisValue;

          pValue = (int *)pSetting->pValue;

          pSettingGroup->pGroup->get( pSetting->pName, ThisValue, pSetting->DVal_Int);

          pSetting->LVal_Int = ThisValue;
          *pValue = ThisValue;
        }
        break;

      case PREF_T_FLOAT:
        { float *pValue, ThisValue;

          pValue = (float *)pSetting->pValue;

          pSettingGroup->pGroup->get( pSetting->pName, ThisValue, pSetting->DVal_Float);

          pSetting->LVal_Float = ThisValue;
          *pValue = ThisValue;
        }
        break;

      case PREF_T_DOUBLE:
        { double *pValue, ThisValue;

          pValue = (double *)pSetting->pValue;

          pSettingGroup->pGroup->get( pSetting->pName, ThisValue, pSetting->DVal_Double);

          pSetting->LVal_Double = ThisValue;
          *pValue = ThisValue;
        }
        break;

      case PREF_T_STRING:
        { char *pValue;
          char *pDefault;
          int defaultSize, maxSize, i, ThisLen;

          pValue   = (char *)pSetting->pValue;
          pDefault = (char *)pSetting->pDefaultValue;
          if( pDefault == NULL) {

            pDefault = (char *)"";
          }

          defaultSize = strlen( pDefault) + 1;
          maxSize = pSetting->SizeOfString;

          if( maxSize > 0 && defaultSize > maxSize) { // Test default size not OK

            pDefault = (char *)"";
            defaultSize = strlen( pDefault) + 1;
          }

          if( maxSize > 0) {   // Need enough space for string

            pSettingGroup->pGroup->get( pSetting->pName, pValue, pDefault, maxSize);
          }

          // Calculate a CRC-Sum to test for Changes

          pSetting->LVal_Int = 4711;

          ThisLen = strlen( pValue);

          for( i = 0; i < ThisLen; i++) {

            pSetting->LVal_Int ^= (pValue[ i] & 0xff) << ((i & 3) * 8);
          }
        }
        break;

      } // end switch
    } // for( iSetting
  } // for( iGroup

  return;
}

/************************************************************************************
 * IqeB_PresetSave_cb
 *
 * Save the current references to a preset file
 *
 */

void IqeB_PresetSave_cb( Fl_Widget *pWidget, void *pValueArg)
{
  Fl_Native_File_Chooser fc;
  char *pFileName;
  int ierr;
  char FileNameSrc[ MAX_FILENAME_LEN];
  char PathPresets[ MAX_FILENAME_LEN];
  char TempFileName[ MAX_FILENAME_LEN];

  // Ensure all presets are saved to file
  IqeB_PreferencesUpdateChanges();

  // Get file name of preset file
  ierr = IqeB_PreferencesData.filename( FileNameSrc, sizeof( FileNameSrc));
  if( ierr < 0) {                 // Error ?

    return;
  }

  // Construct path to preset directory
  strcpy( PathPresets, YaIPS_WorkingDirectory);

  strcat( PathPresets, "/Presets");
  IqeB_FileNormalizePathChars( PathPresets);

  // Initialize the file chooser. Only can save prefs images
  strcpy( TempFileName, LangStringLookup( "&Utils_Preset_FileType=Presets"));
  strcat( TempFileName, "\t*.{prefs}\n");
  fc.filter( TempFileName);

  fc.options( Fl_Native_File_Chooser::SAVEAS_CONFIRM | Fl_Native_File_Chooser::USE_FILTER_EXT);
  fc.directory( PathPresets);

  strcpy( TempFileName, PathPresets);
  strcat( TempFileName, "/");
  strcat( TempFileName, LangStringLookup( "&Utils_Preset_Save1=Preset.prefs"));
  IqeB_FileNormalizePathChars( TempFileName);
  fc.preset_file( TempFileName);

  fc.title( LangStringLookup( "&Utils_Preset_Save2=Save preset"));
  fc.type( Fl_Native_File_Chooser::BROWSE_SAVE_FILE);  // need this if file doesn't exist yet
  ierr = fc.show();                                    // Open file chooser dialog

  if( ierr != 0) {      // User cancelled or error

    goto ExitPoint;
  }

  // Have a filename here. Ensure a prefs file extension.

  pFileName = (char *)fc.filename();

  IqeB_FileEnsureExtension( pFileName, (char *)"prefs", TempFileName, sizeof( TempFileName));

  ierr = CopyFile( FileNameSrc, TempFileName, false);

  // ...

ExitPoint: ;

}

/************************************************************************************
 * DumpPreferenceGroup
 *
 * Dump preference group
 *
 */

#ifdef use_again
#ifdef _DEBUG
static void DumpPreferenceGroup( Fl_Preferences *pSrc, int Level, char *pTitle)
{
  int i, iLevel, groups, entries;
  char *name, *data;

  groups  = pSrc->groups();
  entries = pSrc->entries();

  if( Level == 0) {

    printf( "-------------------------------\n");
    printf( "%s Groups %2d  Entries %d\n", pTitle, groups, entries);
  }

  for( i = 0; i < groups; ++i) {

    name = (char *)pSrc->group( i);

    Fl_Preferences s( pSrc, i);

    for( iLevel = 0; iLevel < Level; iLevel++) printf( "  ");

    printf( "Group %2d: %s\n", i + 1, name);

    DumpPreferenceGroup( &s, Level + 1, (char *)"");
  }

  for( i = 0; i < entries; ++i) {

    name = (char *)pSrc->entry(i);

    if( name != NULL) {

      pSrc->get( name, data, "");

      for( iLevel = 0; iLevel < Level; iLevel++) printf( "  ");

      printf( "Entry %2d: %s = %s\n", i + 1, name, data);

      ::free(data);
    }
  }
}
#endif
#endif

static void CopyPreferenceGroup( Fl_Preferences *pSrc, Fl_Preferences *pDst)
{
  int i, groups, entries;
  char *name, *data;

  groups  = pSrc->groups();
  entries = pSrc->entries();

  for( i = 0; i < groups; ++i) {

    name = (char *)pSrc->group( i);

    Fl_Preferences s( pSrc, i);
    Fl_Preferences d( pDst, name);
    CopyPreferenceGroup( &s, &d);
  }

  for( i = 0; i < entries; ++i) {

    name = (char *)pSrc->entry(i);

    if( name != NULL) {

      pSrc->get( name, data, "");
      pDst->set( name, data);
      ::free(data);
    }
  }
}

/************************************************************************************
 * IqeB_PresetLoad_cb
 *
 * Load a preset file from file
 *
 */

void IqeB_PresetLoad_cb( Fl_Widget *pWidget, void *pValueArg)
{
  Fl_Native_File_Chooser fc;
  char *pFileName;
  int ierr;
  char FileNameSrc[ MAX_FILENAME_LEN];
  char PathPresets[ MAX_FILENAME_LEN];
  char TempFileName[ MAX_FILENAME_LEN];
  Fl_Preferences *pPreferences = NULL;

  // Ensure all presets are saved to file
  IqeB_PreferencesUpdateChanges();

  // Get file name of preset file
  ierr = IqeB_PreferencesData.filename( FileNameSrc, sizeof( FileNameSrc));
  if( ierr < 0) {                 // Error ?

    return;
  }

  // Construct path to preset directory
  strcpy( PathPresets, YaIPS_WorkingDirectory);

  strcat( PathPresets, "/Presets");
  IqeB_FileNormalizePathChars( PathPresets);

  // Initialize the file chooser. Only can save prefs images
  strcpy( TempFileName, LangStringLookup( "&Utils_Preset_FileType=Presets"));
  strcat( TempFileName, "\t*.{prefs}\n");
  fc.filter( TempFileName);

  fc.directory( PathPresets);

  strcpy( TempFileName, PathPresets);
  strcat( TempFileName, "/---");
  IqeB_FileNormalizePathChars( TempFileName);
  fc.preset_file( TempFileName);

  fc.title( LangStringLookup( "&Utils_Preset_Load1=Load preset"));
  fc.type( Fl_Native_File_Chooser::BROWSE_FILE);  // only picks files that exist

  ierr = fc.show();                                    // Open file chooser dialog

  if( ierr != 0) {      // User cancelled or error

    goto ExitPoint;
  }

  // Have a filename here. Ensure a prefs file extension.

  pFileName = (char *)fc.filename();

  pPreferences = new Fl_Preferences( pFileName, "ya3dag.de", NULL, (Fl_Preferences::Root)0);   // create a temporary preference file

  if( pPreferences == NULL) {     // Load Error

    goto ExitPoint;
  }

  // Creating and loading preferences was OK

  IqeB_PreferencesUpdateChanges();     // update preferences database and save to to file

  YaIPS_WindowsShutDown( false);       // Close all open tool windows

  Fl::check();                         // give fltk some cpu to update the screen

  IqeB_PreferencesData.clear();        // Empty the data base

  CopyPreferenceGroup( pPreferences, &IqeB_PreferencesData);   // Copy the preferences

  // Reset the pointer in the settings table. Was overwritten before.

  int iGroup;
  T_GUI_PreferenceGroup *pSettingGroup;

  pSettingGroup = IqeB_PreferenceGroup;

  for( iGroup = 0; iGroup < IqeB_PreferenceGroupN; iGroup++, pSettingGroup++) {  // loop over all groups

    pSettingGroup->pGroup = NULL;
  }

  // Get the preferences from the file and store to the variables.
  IqeB_PreferencesGetFromFile();

  delete pPreferences;                 // Release temporary loaded preferences

  // Startup the windows from last session

  YaIPS_BigImageDisp.ImageSourceID = YaIPS_Main_ImageSourceID_Last;

  // Startup the windows from the last session

  YaIPS_GUI_Main_Do_Startup = true;  // Startup phase of tool windows begin

  Fl::check();                  // give fltk some cpu to update the screen

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

  if( YaIPS_Main_WinPosX == IQE_GUI_NO_WINPOS_X || YaIPS_Main_WinPosY == IQE_GUI_NO_WINPOS_Y) { // have NO last window position

    YaIPS_Main_WinPosX = 100;
    YaIPS_Main_WinPosY = 100;
  }

  YaIPS_WindowsStartup( true,
                        YaIPS_Main_WinPosX, YaIPS_Main_WinPosX + YaIPS_Main_WinSizeX,
                        YaIPS_Main_WinPosY, YaIPS_Main_WinPosY + YaIPS_Main_WinSizeY);

  Fl::check();                  // give fltk some cpu to update the screen

  pGUI_Main->show();                 // Ensure focus back to main window

  YaIPS_GUI_Main_Do_Startup = false;  // Startup phase of tool windows finished

  // ...

ExitPoint: ;

}

/************************************************************************************
 * YaIPS_WindowsStartup
 *
 * Startup the windows from last session
 */

void YaIPS_WindowsStartup( int DoStartup, int xLeft, int xRight, int yTop, int yBotton)
{
  T_YaIPS_WinManagerData *pWinManagData;
  int i;

  pWinManagData = WinManagerTable;

  for( i = 0; i < YAIPS_WIN_MANAGER_MAX; i++, pWinManagData++) {

    if( pWinManagData->pIsOpen == NULL) {     // Have no pointer to 'window is open' flag

      continue;
    }

    if( ! DoStartup) {                        // No startup configured

      *pWinManagData->pIsOpen = false;        // Disable startup for next call

      continue;
    }

    if( ! *pWinManagData->pIsOpen) {          // Window was not open on last exit of application

      continue;
    }

    // Open this window dialog

    if( pWinManagData->pStartup != NULL) {    // Security test

      pGUI_Main->show();                 // Ensure focus back to main window

      Sleep(  50);                            // Wait some time, so window pop up one after the other

      pWinManagData->pStartup( xLeft, xRight, yTop, yBotton, pWinManagData->SubWinIDx);

      Fl::check();                            // give fltk some cpu to update the screen

      Sleep(  50);                            // Wait some time, so window pop up one after the other
    }
  }
}

/************************************************************************************
 * YaIPS_WindowsShutDown
 *
 * Called on shut down of the application. Close the current open windows.
 * The windows must have configured to use this feature.
 *
 * NOTE: The close function is called with NULL pointers
 *       as arguments.
 * NOTE: This is only necessary for windows doing critical things
 *       in the close function
 */

void YaIPS_WindowsShutDown( int DoStartup)
{
  T_YaIPS_WinManagerData *pWinManagData;
  int i;

  pWinManagData = WinManagerTable;

  for( i = 0; i < YAIPS_WIN_MANAGER_MAX; i++, pWinManagData++) {

    if( pWinManagData->pIsOpen == NULL) {     // Have no pointer to 'window is open' flag

      continue;
    }

    if( ! *pWinManagData->pIsOpen) {          // Window is not open

      continue;
    }

    // Open this window dialog

    if( pWinManagData->pClose != NULL) {      // Security test

      // NOTE: This function will reset the 'IsOpen' flag.
      pWinManagData->pClose( NULL, (void *)(long long)pWinManagData->SubWinIDx);

      if( DoStartup) {                        // Startup configured

        *pWinManagData->pIsOpen = true;       // Ensure open on next startup
      }
    }
  }
}

/************************************************************************************
 * YaIPS_WindowsToolWinAddPosDelta
 *
 * Add offset to tool windows inside right side if right side
 * position has changed.
 */

void YaIPS_WindowsToolWinAddPosDelta( int RightSide_X, int RightSide_Y,  // Position main window right side
                                      int RightSide_W, int RightSide_H,  // Size main window right side
                                      int DeltaX, int DeltaY)            // Move tool windows inside right side
{
  T_YaIPS_WinManagerData *pWinManagData;
  Fl_Double_Window *pWin;
  int i, WinPosX, WinPosY;

  pWinManagData = WinManagerTable;

  for( i = 0; i < YAIPS_WIN_MANAGER_MAX; i++, pWinManagData++) {

    if( pWinManagData->pIsOpen == NULL) {     // Have no pointer to 'window is open' flag

      continue;
    }

    if( ! *pWinManagData->pIsOpen) {          // Window is not open

      continue;
    }

    if( pWinManagData->ppMyToolWin == NULL) {  // Have no pointer to 'window data pointer'

      continue;
    }

    // Get pointer to Fl_Double_Window pointer of a tool window

    pWin = (Fl_Double_Window *)*pWinManagData->ppMyToolWin; // get pointer to tool window

    // Check pointer to position

    WinPosX = pWin->x();
    WinPosY = pWin->y();

    WinPosX = pWin->x_root();
    WinPosY = pWin->y_root();

    // Check tool window for being inside right side

    if( WinPosX >= RightSide_X &&
        WinPosY >= RightSide_Y &&
        WinPosX + pWin->w() <= RightSide_X + RightSide_W &&
        WinPosY + pWin->h() <= RightSide_Y + RightSide_H ) {

      // Move tool windows
      pWin->position( WinPosX + DeltaX, WinPosY + DeltaY);
    }
  }
}

/************************************************************************************
 * YaIPS_ToolWinIsOpen
 *
 * Check for a specific dialog to be open.
 *
 * Return:   1: Dialog is open
 *           0: Dialog is closed
 *         < 0: Error
 */

int YaIPS_ToolWinIsOpen( int WinIdNr)                      // Tool window number
{
  T_YaIPS_WinManagerData *pWinManagData;

  if( WinIdNr < 0 || WinIdNr >= YAIPS_WIN_MANAGER_MAX) {   // Security test

    return( -1);
  }

  pWinManagData = WinManagerTable + WinIdNr;


  if( pWinManagData->ppMyToolWin == NULL ||                // have NO pointer to pointer
      *pWinManagData->ppMyToolWin == NULL) {               // and NO have pointer

    return( -2);
  }

  // Check for window data for this window
  if( pWinManagData->pIsOpen == NULL) {                    // Security test: Have no pointer to 'window is open' flag

    return( -3);
  }

  if( ! *pWinManagData->pIsOpen) {                         // Window is closed

    return( 0);        // Return: window is closed
  }

  return( 1);        // Return: window is open
}

/************************************************************************************
 * YaIPS_ToolWinInputCheck
 *
 * Check the output image connected to this image input.
 *
 * HACK: If pWinIdName is NULL
 *
 * Return:   0: Have a valid output image
 *         > 0: Have a valid output image but associated
 *              tool window is not open or no output
 *              window set until now
 *         < 0: Error
 */

int YaIPS_ToolWinInputCheck( int DstWinIdNr,          // Window nr of calling tool window
                            int SrcWinIdNr,         // IN: selected WinIdNr
                            Fl_Box *pInp_Input,      // Optional: Place for output image name
                            Fl_RGB_Image **ppImgOut, // Optional: For a valid output image return pointer to output image
                            int *pImageChanged,      // Optional: For a valid output image return 'ImageChanged'
                            char *pFileName,         // Optional: Place file name (if any) here
                            int SizeOfFileName)      // Size of string for file name. Must be > 1.
{
  T_YaIPS_WinManagerData *pWinManagData;
  int RetVal;
  char *pWinName;
  Fl_Color ColBgnd;

  pWinName = NULL;                                    // No window name until now

  if( ppImgOut != NULL) {                             // Return pointer to output image
    *ppImgOut = NULL;                                 // Reset pointer
  }

  if( pImageChanged != NULL) {                        // Return image changed count
    *pImageChanged = 0;                               // Reset count
  }

  if( pFileName != NULL) {                            // Pointer to file name
    *pFileName = '\0';                                // Empty file name
  }

  if( SrcWinIdNr < 0 ||                                // Security test range
      SrcWinIdNr >= YAIPS_WIN_MANAGER_MAX) {

    RetVal = -1;
    goto ExitPoint;
  }

  if( SrcWinIdNr == DstWinIdNr) {                     // Never use myself

    RetVal = -2;
    goto ExitPoint;
  }

  pWinManagData = WinManagerTable + SrcWinIdNr;       // Get pointer to info element

  if( pWinManagData->pIsOpen == NULL) {               // Have no pointer to 'window is open' flag

    RetVal = -3;
    goto ExitPoint;
  }

  if( pWinManagData->pOutImage == NULL) {             // Must have an output image

    RetVal = -4;
    goto ExitPoint;
  }


  pWinName = pWinManagData->WinName;                  // Have a window name here

  if( pWinName[ 0] == '&') {                          // After startup, this can be a language string

    // Convert to language selected at startup
    strncpy( pWinManagData->WinName, LangStringLookup( pWinName), sizeof( pWinManagData->WinName) - 1);

  } else if( pWinName[ 2] == '&') {                   // After startup, this can be a language string

    // Convert to language selected at startup with leading one digit
    strncpy( pWinManagData->WinName + 2, LangStringLookup( pWinName + 2), sizeof( pWinManagData->WinName) - 3);
  }

  if( ! *pWinManagData->pIsOpen) {                    // Window is not open

    RetVal = 1;
    goto ExitPoint;
  }

  if( pWinManagData->pOutImage->pImage_Img == NULL) {  // Have no output image until now

    RetVal = 2;
    goto ExitPoint;
  }

  // Have a valid output image here

  RetVal = 0;

  if( ppImgOut != NULL) {                             // Return pointer to output image
    *ppImgOut = pWinManagData->pOutImage->pImage_Img; // Set pointer to output image
  }

  if( pImageChanged != NULL) {                        // Return image changed count

    *pImageChanged = pWinManagData->pOutImage->ImageChanged;
  }

  if( pFileName != NULL &&                            // Pointer to file name
      SizeOfFileName > 1) {                           // and have size of file name

    memset( pFileName, 0, SizeOfFileName);            // Zero all
    strncpy( pFileName, pWinManagData->pOutImage->FileName, SizeOfFileName - 1);
  }

ExitPoint:

  // Color the background of output image name
  // and set name of image window

#define COL2      96                                    // Shade color

  if( pInp_Input != NULL)  {                            // Have a pointer to this GUI element

    // Update name of image window

    if( pWinName == NULL) {                             // Name is not valid

      pWinName = (char *)"---";
    }

    if( strcmp( pWinName, pInp_Input->label()) != 0) {  // Name is different

      pInp_Input->copy_label( pWinName);
      pInp_Input->redraw();
    }

    // Color background

    ColBgnd = fl_rgb_color( COL2, 255, COL2);           // Preset ready

    if( RetVal < 0) {                                   // Error

      ColBgnd = fl_rgb_color( 255, COL2, COL2);

    } else if( RetVal > 0) {                            // Input is OK but there is no image

      ColBgnd = fl_rgb_color( 255, 176, 96);            // Orange
    }

    if( pInp_Input->color() != ColBgnd) {               // Color is different

      pInp_Input->color( ColBgnd);
      pInp_Input->redraw();
    }
  }

  return( RetVal);
}

/************************************************************************************
 * YaIPS_ToolWinInputSelect
 *
 * Select an input image. Also sets variables to latch image and color the window name field.
 *
 * Popup a menu at place of the button.
 * Select an output of an other image as input.
 *
 * Return:   0: Have a valid output image
 *         > 0: Have a valid output image but associated
 *              tool window is not open or no output
 *              window set until now
 *         < 0: Error
 */

int YaIPS_ToolWinInputSelect( int DstWinIdNr,            // Window nr of calling tool window
                             int *pWinIdNr,             // OUT: selected WinIdNr
                             Fl_Button *pBut_Input,     // Button to popup the menu
                             Fl_Box *pInp_Input)        // Place for output image name
{
  const Fl_Menu_Item *pPicked;
  T_YaIPS_WinManagerData *pWinManagData;
  int i, SrcWinIdNr;

  // Post dynamically created context menu, get user's choice
  Fl_Menu_Button menu( pBut_Input->x(), pBut_Input->y() + pBut_Input->h(), 80, 1);

  // Loop through windows and collect the output images

  menu.add( "---", 0, NULL, (void *)-1);       // Add this for now output selected

  pWinManagData = WinManagerTable;

  for( i = 0; i < YAIPS_WIN_MANAGER_MAX; i++, pWinManagData++) {

    if( i == DstWinIdNr) {                    // Never use myself

      continue;
    }

    if( pWinManagData->pIsOpen == NULL) {     // Have no pointer to 'window is open' flag

      continue;
    }

    if( ! *pWinManagData->pIsOpen) {          // Window is not open

      continue;
    }

    if( pWinManagData->pOutImage != NULL) {   // This has an output image

      if( pWinManagData->WinName[ 0] == '&') {   // After startup, this can be a language string

        // Convert to language selected at startup
        strncpy( pWinManagData->WinName, LangStringLookup( pWinManagData->WinName), sizeof( pWinManagData->WinName) - 1);

      } else if( pWinManagData->WinName[ 2] == '&') {                   // After startup, this can be a language string

        // Convert to language selected at startup with leading one digit
        strncpy( pWinManagData->WinName + 2, LangStringLookup( pWinManagData->WinName + 2), sizeof( pWinManagData->WinName) - 3);
      }

      menu.add( pWinManagData->WinName, 0, NULL, (void *)(long long)i);
    }
  }

  // Pop up the menu

  pPicked = menu.popup();

  // Do we have any selection

  if( pPicked != NULL) {                     // Have picked one

    SrcWinIdNr = -1;                         // Preset, none selected

    pInp_Input->copy_label( pPicked->text);  // Set the selected text

    SrcWinIdNr = (int)(uintptr_t)pPicked->user_data_;   // Get WinIdNr from selected

    // Store the data
    *pWinIdNr = SrcWinIdNr;
  }

  // Check input image settings
  i = YaIPS_ToolWinInputCheck( DstWinIdNr, *pWinIdNr, pInp_Input);

  return( i);
}

/************************************************************************************
 * YaIPS_ToolWinDrawAfterSet
 *
 * Set a draw after function for the big image display
 *
 * This will be called from big image display do make specialized drawings
 * for an image displayed from an specific tool window.
 *
 * WinIdNr:       Tool window number
 * pBigDrawAfter: Pointer to draw after function. Can be NULL to reset pointer.
 *
 * Return:   0: OK
 *         < 0: Error
 */

int YaIPS_ToolWinDrawAfterSet( int WinIdNr,                              // Tool window number
                               YaIPS_ToolWinDrawAfterCb *pBigDrawAfter)  // Pointer to draw after function
{
  T_YaIPS_WinManagerData *pWinManagData;

  if( WinIdNr < 0 || WinIdNr >= YAIPS_WIN_MANAGER_MAX) {   // Security test

    return( -1);
  }

  pWinManagData = WinManagerTable + WinIdNr;

  // Check for window data for this window
  if( pWinManagData->pIsOpen == NULL) {          // Security test: Have no pointer to 'window is open' flag

    return( -2);
  }

  pWinManagData->pBigDrawAfter = pBigDrawAfter;  // Set the pointer

  return( 0);                                    // Return OK
}

/************************************************************************************
 * YaIPS_ToolWinDrawAfterCall
 *
 * Call a draw after function from a tool window to draw into the big image display.
 * This is called from big image display do make specialized drawings
 * for an image displayed from an specific tool window.
 *
 * pYaIPS_ImageDisp: Point to big image display data
 * WinIdNr:          Tool window number
 *
 */

void YaIPS_ToolWinDrawAfterCall( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                                 int WinIdNr)                             // Tool window number
{
  T_YaIPS_WinManagerData *pWinManagData;

  if( WinIdNr < 0 || WinIdNr >= YAIPS_WIN_MANAGER_MAX) {   // Security test

    return;
  }

  pWinManagData = WinManagerTable + WinIdNr;

  if( pWinManagData->pBigDrawAfter == NULL) {              // NO draw after function set

    return;
  }

  if( pWinManagData->ppMyToolWin == NULL ||                // have NO pointer to pointer
      *pWinManagData->ppMyToolWin == NULL) {               // and NO have pointer

    return;
  }

  // Check for window data for this window
  if( pWinManagData->pIsOpen == NULL) {                    // Security test: Have no pointer to 'window is open' flag

    return;
  }

  if( ! *pWinManagData->pIsOpen) {                         // Window is closed

    return;
  }


  pWinManagData->pBigDrawAfter( pYaIPS_ImageDisp, pWinManagData->SubWinIDx);
}

/************************************************************************************
 * YaIPS_ToolWinDrawAfterRedraw
 *
 * Refresh other than big image if linked to big image
 *
 * WinIdNr:          Tool window number
 *
 */

void YaIPS_ToolWinDrawAfterRedraw( int WinIdNr)                             // Tool window number
{
  T_YaIPS_WinManagerData *pWinManagData;
  Fl_YaIPS_ImageDisp_t *pOutImage;      // Point to optional output image

  if( WinIdNr < 0 || WinIdNr >= YAIPS_WIN_MANAGER_MAX) {   // Security test

    return;
  }

  pWinManagData = WinManagerTable + WinIdNr;

  if( pWinManagData->pBigDrawAfter == NULL) {              // NO draw after function set

    return;
  }

  if( pWinManagData->ppMyToolWin == NULL ||                // have NO pointer to pointer
      *pWinManagData->ppMyToolWin == NULL) {               // and NO have pointer

    return;
  }

  // Check for window data for this window
  if( pWinManagData->pIsOpen == NULL) {                    // Security test: Have no pointer to 'window is open' flag

    return;
  }

  if( ! *pWinManagData->pIsOpen) {                         // Window is closed

    return;
  }

  pOutImage = pWinManagData->pOutImage;                   // Get output iamge


  if( pOutImage == NULL) {                                // NO output image set

    return;
  }

  if( YaIPS_BigImageDisp.ImageSourceID != WinIdNr) {      // and NOT display this on the big image

    return;
  }

  if( pOutImage->pImage_Box != NULL) {
    pOutImage->pImage_Box->redraw();
  }
}

/************************************************************************************
 * YaIPS_ToolChangeOutputSet
 *
 * Set a change output function for the big image display
 *
 * This will be called from big image display do allow change of the
 * output image from outside the source module.
 *
 * WinIdNr:       Tool window number
 * pChangeOutput: Pointer to change output function.
 *
 * Return:   0: OK
 *         < 0: Error
 */

int YaIPS_ToolChangeOutputSet( int WinIdNr,                                     // Tool window number
                                   YaIPS_ToolWinChangeOutputCb *pChangeOutput)  // Pointer to change output function
{
  T_YaIPS_WinManagerData *pWinManagData;

  if( WinIdNr < 0 || WinIdNr >= YAIPS_WIN_MANAGER_MAX) {   // Security test

    return( -1);
  }

  pWinManagData = WinManagerTable + WinIdNr;

  // Check for window data for this window
  if( pWinManagData->pIsOpen == NULL) {          // Security test: Have no pointer to 'window is open' flag

    return( -2);
  }

  pWinManagData->pChangeOutput = pChangeOutput;  // Set the pointer

  return( 0);                                    // Return OK
}

/************************************************************************************
 * YaIPS_ToolChangeOutputTest
 *
 * Test for a change output function is set.
 *
 * WinIdNr:       Tool window number
 * pChangeOutput: Pointer to change output function.
 *
 * Return:   0: NOT set
 *           1: Set
 *         < 0: Error
 */

int YaIPS_ToolChangeOutputTest( int WinIdNr)                                     // Tool window number
{
  T_YaIPS_WinManagerData *pWinManagData;

  if( WinIdNr < 0 || WinIdNr >= YAIPS_WIN_MANAGER_MAX) {   // Security test

    return( -1);
  }

  pWinManagData = WinManagerTable + WinIdNr;

  // Check for window data for this window
  if( pWinManagData->pIsOpen == NULL) {          // Security test: Have no pointer to 'window is open' flag

    return( -2);
  }

  return( pWinManagData->pChangeOutput != NULL);  // Return true if the pointer is set
}

/************************************************************************************
 * YaIPS_ToolChangeOutputCall
 *
 * Call the change output callback. This sets a new output image.
 *
 * WinIdNr:       Tool window number
 * pNewimage:     Replace output image with this window
 */

void YaIPS_ToolChangeOutputCall( int WinIdNr,                             // Tool window number
                                 Fl_RGB_Image *pNewimage)                 // Replace output image with this window
{
  T_YaIPS_WinManagerData *pWinManagData;

  if( WinIdNr < 0 || WinIdNr >= YAIPS_WIN_MANAGER_MAX) {   // Security test

    return;
  }

  pWinManagData = WinManagerTable + WinIdNr;

  if( pWinManagData->pChangeOutput == NULL) {              // NO change output function set

    return;
  }

  if( pWinManagData->ppMyToolWin == NULL ||                // have NO pointer to pointer
      *pWinManagData->ppMyToolWin == NULL) {               // and NO have pointer

    return;
  }

  // Check for window data for this window
  if( pWinManagData->pIsOpen == NULL) {                    // Security test: Have no pointer to 'window is open' flag

    return;
  }

  if( ! *pWinManagData->pIsOpen) {                         // Window is closed

    return;
  }


  pWinManagData->pChangeOutput( pWinManagData->SubWinIDx, pNewimage);
}

/************************************************************************************
 * YaIPS_ToolWinMouseCallbackCall
 *
 * Call a mouse callback function from a tool window.
 * This is called from big image display do make specialized mouse event processing
 * from a tool window also on the big image display.
 *
 * pYaIPS_ImageDisp: Point to big image display data
 * WinIdNr:          Tool window number
 *
 * Return:   0: OK, event was processed
 *         < 0: Error
 */

int YaIPS_ToolWinMouseCallbackCall( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                                    int WinIdNr,                             // Tool window number
                                    Fl_Widget *pW,                           // Calling widget
                                    int event)                               // Mouse event
{
  int ierr;
  T_YaIPS_WinManagerData *pWinManagData;
  Fl_YaIPS_ImageDisp_t *pOutImage;

  if( WinIdNr < 0 || WinIdNr >= YAIPS_WIN_MANAGER_MAX) {   // Security test

    return( -1);
  }

  pWinManagData = WinManagerTable + WinIdNr;

  if( pWinManagData->pBigDrawAfter == NULL) {              // NO draw after function set

    return( -2);
  }

  if( pWinManagData->ppMyToolWin == NULL ||                // have NO pointer to pointer
      *pWinManagData->ppMyToolWin == NULL) {               // and NO have pointer

    return( -3);
  }

  // Check for window data for this window
  if( pWinManagData->pIsOpen == NULL) {                    // Security test: Have no pointer to 'window is open' flag

    return( -4);
  }

  if( ! *pWinManagData->pIsOpen) {                         // Window is closed

    return( -5);
  }

  pOutImage = pWinManagData->pOutImage;                    // Get output image
  if( pOutImage == NULL) {                                 // Need an output image

    return( -5);
  }

  if( pOutImage->pImage_Box == NULL) {                     // Need an image box

    return( -6);
  }

  if( pOutImage->pImage_Box->pMouseCallback == NULL) {     // Need a mouse callback

    return( -7);
  }

  // Call the mouse callback. Replace pArg1 but keep pArg2

  if( pOutImage->pImage_Box->MouseTeachState == 3) {      // Check for enabled teach mode

    // Only call for enabled teach mode

    ierr = pOutImage->pImage_Box->pMouseCallback( pW, event, pYaIPS_ImageDisp, pOutImage->pImage_Box->MouseCallbackArg2);

  } else {

    ierr = -10;      // Return event not processed
  }

  return( ierr);     // OK
}

/************************************************************************************
 * YaIPS_ToolWinDropCallbackCall
 *
 * Call a drop callback function from a tool window.
 * This is called from big image display do make specialized drop processing
 * from a tool window also on the big image display.
 *
 * WinIdNr:          Tool window number
 * ImageDispArg: Pointer to image display of original file drop
 *
 * Return:   0: OK
 *         < 0: Error
 */

int YaIPS_ToolWinDropCallbackCall( int WinIdNr,                             // Tool window number
                                   Fl_Widget *pW,                           // Calling widget
                                   void *pArg,                              // File name argument
                                   void *pImageDispArg)

{
  int ierr;
  T_YaIPS_WinManagerData *pWinManagData;
  Fl_YaIPS_ImageDisp_t *pOutImage;

  if( WinIdNr < 0 || WinIdNr >= YAIPS_WIN_MANAGER_MAX) {   // Security test

    return( -1);
  }

  pWinManagData = WinManagerTable + WinIdNr;

  if( pWinManagData->ppMyToolWin == NULL ||                // have NO pointer to pointer
      *pWinManagData->ppMyToolWin == NULL) {               // and NO have pointer

    return( -3);
  }

  // Check for window data for this window
  if( pWinManagData->pIsOpen == NULL) {                    // Security test: Have no pointer to 'window is open' flag

    return( -4);
  }

  if( ! *pWinManagData->pIsOpen) {                         // Window is closed

    return( -5);
  }

  pOutImage = pWinManagData->pOutImage;                    // Get output image
  if( pOutImage == NULL) {                                 // Need an output image

    return( -5);
  }

  if( pOutImage->pImage_Box == NULL) {                     // Need an image box

    return( -6);
  }

  if( pOutImage->pImage_Box->pDropCallback == NULL) {     // Need a mouse callback

    return( -7);
  }

  // Call the mouse callback. Replace pArg1 but keep pArg2

  ierr = pOutImage->pImage_Box->pDropCallback( pW, pArg, pImageDispArg, pWinManagData->SubWinIDx);

  return( ierr);     // OK
}

/************************************************************************************
 * YaIPS_ToolWinMouseCallbackState
 *
 * Check for a mouse callback / teach mode.
 * This is called from big image GUI to color the Teach pencil symbol
 * in the image section.
 *
 * pYaIPS_ImageDisp: Point to big image display data
 * WinIdNr:          Tool window number
 *
 * Return:   0: NO mouse callback set
 *           1: Teach mode NOT available for current settings.
 *           2: Teach mode disabled
 *           3: Teach mode enabled
 *         < 0: Error
 */

int YaIPS_ToolWinMouseCallbackState( int WinIdNr)                             // Tool window number
{
  int ierr;
  T_YaIPS_WinManagerData *pWinManagData;
  Fl_YaIPS_ImageDisp_t *pOutImage;

  if( WinIdNr <= YAIPS_WIN_ID_NONE || WinIdNr >= YAIPS_WIN_MANAGER_MAX) {   // Security test

    return( -1);
  }

  pWinManagData = WinManagerTable + WinIdNr;

  if( pWinManagData->pBigDrawAfter == NULL) {              // NO draw after function set

    return( -2);
  }

  if( pWinManagData->ppMyToolWin == NULL ||                // have NO pointer to pointer
      *pWinManagData->ppMyToolWin == NULL) {               // and NO have pointer

    return( -3);
  }

  // Check for window data for this window
  if( pWinManagData->pIsOpen == NULL) {                    // Security test: Have no pointer to 'window is open' flag

    return( -4);
  }

  if( ! *pWinManagData->pIsOpen) {                         // Window is closed

    return( -5);
  }

  pOutImage = pWinManagData->pOutImage;                    // Get output image
  if( pOutImage == NULL) {                                 // Need an output image

    return( -5);
  }

  if( pOutImage->pImage_Box == NULL) {                     // Need an image box

    return( -6);
  }

  if( pOutImage->pImage_Box->pMouseCallback == NULL) {     // Need a mouse callback

    return( 0);                                            // NO mouse callback set
  }

  // Return the mouse teach state

  ierr = pOutImage->pImage_Box->MouseTeachState;

  return( ierr);     // OK
}

/************************************************************************************
 * YaIPS_ToolWinCountChildren
 *
 * Count the children widgets.
 */

//x/#define USE_COUNT_DEBUG_PRINTS 1   // Define this for count debug prints

static int YaIPS_ToolWinCountChildren2( Fl_Group *pParent, int Level)
{

  int i, nChildren, SumAll;
  Fl_Widget *pChild;
  Fl_Group *gGroup;
#ifdef USE_COUNT_DEBUG_PRINTS    // Count debug prints
#ifdef _DEBUG
  int iLevel, nThis;
  char *pLabel;
#endif
#endif

  // Number of children
  nChildren = pParent->children();

  // Get number of grandchildren
  SumAll = 0;

  for( i = 0; i < nChildren; i++) {

    SumAll += 1;                         // Count widget itself

    pChild = pParent->child( i);

    gGroup = pChild->as_group();

#ifdef USE_COUNT_DEBUG_PRINTS    // Count debug prints
#ifdef _DEBUG
    if( gGroup != NULL) {

      nThis = gGroup->children();

    } else {

      nThis = 0;
    }

    pLabel = (char *)pChild->label();

    for( iLevel = 0; iLevel < Level; iLevel++) {
      printf( "  ");
    }

    printf( " %2d: Type 0x%02x, Group %s, Childs %2d: %s\n", i + 1, pChild->type(), gGroup != NULL ? "YES" : "no ", nThis, pLabel);
#endif
#endif

    if( gGroup != NULL) {

      SumAll += YaIPS_ToolWinCountChildren2( gGroup, Level + 1);  // Sum up children

    }

  }

  return( SumAll);
}

int YaIPS_ToolWinCountChildren( Fl_Double_Window *pWin)
{
  int SumAll;
#ifdef USE_COUNT_DEBUG_PRINTS    // Count debug prints
#ifdef _DEBUG
  int nChildren;
  char *pLabel;
#endif
#endif

  // Number of children
#ifdef USE_COUNT_DEBUG_PRINTS    // Count debug prints
#ifdef _DEBUG
  nChildren = pWin->children();

  pLabel = (char *)pWin->label();
  printf( "Win '%s' Childs %d S %d V %d Vr %d\n", pLabel, nChildren, pWin->shown(), pWin->visible(), pWin->visible_r());
#endif
#endif

  SumAll = YaIPS_ToolWinCountChildren2( pWin, 0);

#ifdef USE_COUNT_DEBUG_PRINTS    // Count debug prints
#ifdef _DEBUG
  printf( "  -------------------\n");
  printf( "  Summ all childs %d\n", SumAll);
#endif
#endif

  return( SumAll);
}

/************************************************************************************
 * YaIPS_ToolWinDumpWindows
 *
 * Dump the window table to the console.
 */

void YaIPS_ToolWinDumpWindows()
{
  T_YaIPS_WinManagerData *pWinManagData;
  Fl_Double_Window *pWin;
  int i;

  printf( "Tool window table\n");
  printf( "-----------------\n");

  pWinManagData = WinManagerTable;

  for( i = 0; i < YAIPS_WIN_MANAGER_MAX; i++, pWinManagData++) {

    if( pWinManagData->ppMyToolWin == NULL) {  // Have no pointer to 'window data pointer'

      continue;
    }

    pWin = (Fl_Double_Window *)*pWinManagData->ppMyToolWin; // get pointer to tool window

    printf( " %2d: %-20s SWI %1d Op %d", i, pWinManagData->WinName, pWinManagData->SubWinIDx,
                                            pWinManagData->pIsOpen == NULL ? -9 : *pWinManagData->pIsOpen);

    if( pWin == NULL) {

      printf( " NO Win pointer\n");
    } else {

      printf( " Cs %d = %d S %d V %d Vr %d\n", pWin->children(), pWin->parent() != NULL, pWin->shown(), pWin->visible(), pWin->visible_r());
    }
  }

  printf( "Win order\n");
  printf( "---------\n");

  Fl_Window *pWinS = Fl::first_window();

  Fl_Double_Window *pWinThis;

  while( pWinS) {

    if( !pWinS->menu_window()) {

      // Locate it in our tool win table

      pWinThis = NULL;

      pWinManagData = WinManagerTable;

      for( i = 0; i < YAIPS_WIN_MANAGER_MAX; i++, pWinManagData++) {

        if( pWinManagData->ppMyToolWin == NULL) {  // Have no pointer to 'window data pointer'

          continue;
        }

        pWin = (Fl_Double_Window *)*pWinManagData->ppMyToolWin; // get pointer to tool window

        if( pWin != NULL) {

          if( pWin == pWinS) {        // Got it

            pWinThis = pWin;
            break;
          }
        }
      }

      if( pWinThis != NULL) {

        printf( " %08llx  %2d: %-20s Op %d S %d V %d P %08llx W %08llx\n",
                                (long long)pWinThis, i, pWinManagData->WinName,
                                pWinManagData->pIsOpen == NULL ? -9 : *pWinManagData->pIsOpen,
                                pWinThis->shown(), pWinThis->visible(),
                                (long long)pWinThis->parent(), (long long)pWinThis->window());
      } else {

        if( pWinThis == NULL) {

          printf( " %08llx, Must be main Window %08llx W %08llx\n",
                               (long long)pWinThis, (long long)pGUI_Main, (long long)pGUI_Main->window());

        } else {

          printf( " %08llx\n", (long long)pWinThis);
        }
      }

    }

    pWinS = Fl::next_window( pWinS);
  }

}

/************************************************************************************
 * YaIPS_ToolWinTestAction
 *
 * Do test actions with the windows in the window table
 *
 * Action              Type of action
 * MainMakeTopWindow   If true: at exit make main the top window
 */

void YaIPS_ToolWinTestAction( int Action, int MainMakeTopWindow)
{
  T_YaIPS_WinManagerData *pWinManagData;
  Fl_Double_Window *pWin;
  int i;

#ifdef use_again

  pWinManagData = WinManagerTable;

  for( i = 0; i < YAIPS_WIN_MANAGER_MAX; i++, pWinManagData++) {

    if( pWinManagData->ppMyToolWin == NULL) {  // Have no pointer to 'window data pointer'

      continue;
    }

    pWin = (Fl_Double_Window *)*pWinManagData->ppMyToolWin; // get pointer to tool window

    if( pWin != NULL) {                        // Have a pointer to the window

      switch( Action) {

      case YAIPS_TWIN_ACTION_SHOW_ALL:
        pWin->show();
        break;

      case YAIPS_TWIN_ACTION_HIDE_ALL:
        pWin->hide();
        break;

      case YAIPS_TWIN_ACTION_ICONIZE_ALL:
        pWin->iconize();
        break;

      case YAIPS_TWIN_ACTION_HIDE_SHOW_ALL:
        pWin->hide();
        pWin->show();
        break;

      case YAIPS_TWIN_ACTION_REDRAW_ALL:
        pWin->redraw();
        break;
      }
    }
  }
#else

  // Walk the windows on the screen

  Fl_Window *pWinS = Fl::first_window();
  Fl_Double_Window *pWinThis;

  Fl_Double_Window *WinInScreen[ YAIPS_WIN_MANAGER_MAX];

  int nWindows;

  nWindows = 0;

  while( pWinS) {

    if( !pWinS->menu_window()) {

      // Locate it in our tool win table

      pWinThis = NULL;

      pWinManagData = WinManagerTable;

      for( i = 0; i < YAIPS_WIN_MANAGER_MAX; i++, pWinManagData++) {

        if( pWinManagData->ppMyToolWin == NULL) {  // Have no pointer to 'window data pointer'

          continue;
        }

        pWin = (Fl_Double_Window *)*pWinManagData->ppMyToolWin; // get pointer to tool window

        if( pWin != NULL) {

          if( pWin == pWinS) {        // Got it

            pWinThis = pWin;
            break;
          }
        }
      }

      if( pWinThis != NULL) {

        WinInScreen[ nWindows] = pWinThis;
        nWindows += 1;
      }
    }

    pWinS = Fl::next_window( pWinS);
  }

  // Now walk the windows backwards

  while( nWindows > 0) {

    nWindows -= 1;

    pWin = WinInScreen[ nWindows];

    if( pWin != NULL) {                        // Have a pointer to the window

      switch( Action) {

      case YAIPS_TWIN_ACTION_SHOW_ALL:
        pWin->show();
        break;

      case YAIPS_TWIN_ACTION_HIDE_ALL:
        pWin->hide();
        break;

      case YAIPS_TWIN_ACTION_ICONIZE_ALL:
        pWin->iconize();
        break;

      case YAIPS_TWIN_ACTION_HIDE_SHOW_ALL:
        pWin->hide();
        pWin->show();
        break;

      case YAIPS_TWIN_ACTION_REDRAW_ALL:
        pWin->redraw();
        break;
      }
    }
  }
#endif

  Fl::check();                            // give fltk some cpu to update the screen

  // Make the main window the top window
  if( MainMakeTopWindow) {                // Make main the top window

    IqeB_GUI_MainMakeTopWindow();
  }
}

#ifdef _DEBUG
void YaIPS_ToolWinTestAction_cb( Fl_Widget *pWidget, void *pValueArg)
{
  int ArgInt;

  ArgInt = (long long)pValueArg;

  YaIPS_ToolWinTestAction( ArgInt, true);
}
#endif

/****************************** End Of File ******************************/


