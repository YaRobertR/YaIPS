/****************************************************************************

  YaIPS_Utils_Language.cpp

  Language handling.

 15.07.2025 RR: First edition of this file.

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

#include "FL/filename.H"

#include "YaIPS.h"

/*===========================================================================

	defines, externals

=============================================================================
*/

#define MAX_LAGUAGE_TRANSLATION_FILES      512  // Max number of translation files

typedef struct {
  char *pTag;                 // point to tag
  char *pText;                // point to buffer
} T_LangTranslations;

static T_LangTranslations *pLangTranslations;   // point to buffer for language translations
static char *pLangTagsTextSpace;                // point to buffer for tags and text strings

static int nLangTranslationsAlloced;
static int nLangTranslationsUsed;
static int nLangTagsTextSpaceAlloced;
static int nLangTagsTextSpaceUsed;

/*===========================================================================

	LangLoadTranslationFile

  NewLineCharForMultipleStringLines:
     If set, insert new line characters at end of multiple string lines.
       Example: tag_xyz  "Line 1"
                         "Line 2"
                         "Line 3"

=============================================================================
*/
static int LangLoadTranslationFile( char *pFileName, int SaveData,
                                    int *pCountTranslations, int *pData,
                                    int NewLineCharForMultipleStringLines)
{
  int  ThisLength;
  FILE *pFile;
  char *buf, *pTag, *pTextBegin, *pText;
  char line[ 1024];

  // load the file

  pFile = fl_fopen ( pFileName, "rt"); // Try to open file

  if( pFile == NULL) {                   // File not found ?

    return( -1);
	}

  // parse the file

RestartParse:

  for( ; ; ) {

    // Try to read next line

    if( ! fgets( line, sizeof line, pFile)) {
      break;
    }

    // Skip comment lines

    if( line[ 0] == '\0' ||
        line[ 0] == ';' ||
        line[ 0] == '/') {

      continue;
    }

    buf = line;

    // parse until begin of tag

    while( *buf == ' ') {

      buf++;                // skip this char
    }

    if( *buf == '\0' ||     // end of line
        *buf == ';') {      // comment

      continue;             // rescan
    }

    // must be begin of tag

    if( *buf == '"') {              // quote is begin of string

      pTag = NULL;                  // have no tag

      buf++;                        // skip this char

      if( *buf == '\0') {           // beyond end of line

        goto RestartParse;          // something not OK, restart parsing
      }

    } else {                        // Is begin of tag

      pTag = buf;

      for( ; ; ) {

        if( *buf == '\0') {           // beyond end of line

          goto RestartParse;          // something not OK, restart parsing
        }

        if( *buf <= ' ') {            // end of tag

          *buf = '\0';                // set end of tag
          buf++;                      // skip this char
          break;
        }

        buf++;                        // skip this char
      }

      // parse until begin of text


      for( ; ; ) {

        if( *buf == '\0' ) {          // end of line

          goto RestartParse;          // something not OK, restart parsing
        }

        if( *buf <= ' ') {            // Is blank
          buf++;                      // skip this char
          continue;
        }

        if( *buf == '"') {            // double quote is begin of text
          buf++;                      // skip this char
          break;
        }

        buf++;                        // skip this char

        if( *buf == '\0') {           // beyond end of line

          goto RestartParse;          // something not OK, restart parsing
        }
      }
    }

    // must be begin of text

    pText      = buf;
    pTextBegin = buf;

    // parse until end of text

    for( ; ; ) {

      if( *buf == '\0') {           // end of line

        goto RestartParse;          // something not OK, restart parsing
      }

      if( *buf == '"') {            // double quote is end of text
        *pText = '\0';              // set end of text
        buf++;                      // skip this char
        break;
      }

      if( buf[ 0] == '\\' && buf[1] == 'n') {          // new line
        *pText++ = '\n';
        buf++;
        buf++;
      } else {
        *pText ++ = *buf++;           // copy this char
      }

      if( *buf == '\0') {           // beyond end of line

        goto RestartParse;          // something not OK, restart parsing
      }
    }

    if( pTag != NULL) {

      // got tag and text

      *pCountTranslations += 1;

      ThisLength = strlen( (char *)pTag) + 1 + strlen( (char *)pTextBegin) + 1;
      *pData += ThisLength;

      if( SaveData) {

        if( nLangTranslationsUsed + 1 <= nLangTranslationsAlloced &&
          nLangTagsTextSpaceUsed + ThisLength <= nLangTagsTextSpaceAlloced) {    // security test

          pLangTranslations[ nLangTranslationsUsed].pTag = pLangTagsTextSpace + nLangTagsTextSpaceUsed;

          strcpy( pLangTagsTextSpace + nLangTagsTextSpaceUsed, (char *)pTag);
          nLangTagsTextSpaceUsed += strlen( (char *)pTag) + 1;

          pLangTranslations[ nLangTranslationsUsed].pText = pLangTagsTextSpace + nLangTagsTextSpaceUsed;

          strcpy( pLangTagsTextSpace + nLangTagsTextSpaceUsed, (char *)pTextBegin);
          nLangTagsTextSpaceUsed += strlen( (char *)pTextBegin) + 1;

          nLangTranslationsUsed += 1;
        }
      }
    } else {

      // have one more line to the last string

      if( *pCountTranslations > 0 && *pData > 0) {        // must have one to append

        ThisLength = strlen( (char *)pTextBegin) + 1;

        if( ! NewLineCharForMultipleStringLines) {        // NO insert of new line character at end for multiple string lines.

          ThisLength -= 1;                                // Need one character less
          nLangTagsTextSpaceUsed -= 1;                    // Overwrite last end of string to concatenate the strings.
        }


        *pData += ThisLength;

        if( SaveData) {

          if( nLangTagsTextSpaceUsed + ThisLength <= nLangTagsTextSpaceAlloced) {     // security test

            if( NewLineCharForMultipleStringLines) {                                  // Insert of new line character at end for multiple string lines.
                pLangTagsTextSpace[ nLangTagsTextSpaceUsed - 1] = '\n';               // Last end of string goes to end of line
            }
            strcpy( pLangTagsTextSpace + nLangTagsTextSpaceUsed, (char *)pTextBegin); // append string
            nLangTagsTextSpaceUsed += strlen( (char *)pTextBegin) + 1;
          }
        }
      }
    }
  }

  // free the file

  fclose( pFile);                        // Close file

  return( 0);
}

/*===========================================================================

  Lang_FreeData()

  Free up all the memory allocated for the most
  recently loaded language data.

  Return:

    None

=============================================================================
*/

void Lang_FreeData()
{
  // free translation buffers

  if( pLangTranslations != NULL) {
    free( pLangTranslations);
    pLangTranslations = NULL;
  }

  if( pLangTagsTextSpace != NULL) {
    free( pLangTagsTextSpace);
    pLangTagsTextSpace = NULL;
  }

  nLangTranslationsAlloced = 0;
  nLangTranslationsUsed    = 0;

  nLangTagsTextSpaceAlloced = 0;
  nLangTagsTextSpaceUsed    = 0;
}

/*===========================================================================

  LangLoadTranslations()

  Load the language data for a specific language

  Arguments:

    char *pLanguage        Name of the directory containing the language data

    int NewLineCharForMultipleStringLines
                           If set, insert new line characters at end of multiple string lines.
                                Example: tag_xyz  "Line 1"
                                "Line 2"
                                "Line 3"

  Return:

    0      OK
    != 0   Error

=============================================================================
*/

static int LangTranslationCompare( const void *arg1, const void *arg2 )
{
  T_LangTranslations *p1;
  T_LangTranslations *p2;

  p1 = (T_LangTranslations *)arg1;
  p2 = (T_LangTranslations *)arg2;

  return strcmp( p1->pTag, p2->pTag);
}

int LangLoadTranslations( char *pLanguage,
                          int NewLineCharForMultipleStringLines)
{
  char Files_path[ MAX_FILENAME_LEN];
  int  numFiles, i, j, LenName;
  int  nTranslations, nData;
  dirent **list;
  char *pName;

  // free translation buffers

  Lang_FreeData();

  // Construct path to language files
  strcpy( Files_path, YaIPS_WorkingDirectory);
  strcat( Files_path, "/Languages/");
  strcat( Files_path, pLanguage);

  IqeB_FileNormalizePathChars( Files_path);

  // test for language files

	numFiles = fl_filename_list( Files_path, &list, fl_alphasort);

  // pass 1, get the amount of data we need

  nTranslations = 0;
  nData = 0;

  for( i = 0; i < numFiles; i++) {

    pName = list[i]->d_name;

    LenName = strlen( pName);

    if( pName[ 0] == '\0' ||   // End of string
        pName[ 0] == '.' ) {   // current dir or dir up

      continue;
    }

    // Skip directories. Directories have a '/' as last character
    if( LenName > 0 && pName[ LenName - 1] == '/') { // Is a directory

      continue;
    }

    // Check for .txt extension

    if( LenName <= 4 ||
        stricmp( pName + LenName - 4, ".txt") != 0) {

      continue;
    }

    // Construct path to language file

    strcpy( Files_path, YaIPS_WorkingDirectory);
    strcat( Files_path, "/Languages/");
    strcat( Files_path, pLanguage);
    strcat( Files_path, "/");
    strcat( Files_path, pName);

    IqeB_FileNormalizePathChars( Files_path);

    // Load translations

    LangLoadTranslationFile( Files_path, false, &nTranslations, &nData, NewLineCharForMultipleStringLines);
  }

  // allocate buffer for translations

  pLangTranslations  = (T_LangTranslations *)malloc( sizeof( T_LangTranslations) * nTranslations + 1);
  pLangTagsTextSpace = (char *)malloc( nData + 1);

  if( pLangTranslations == NULL || pLangTagsTextSpace == NULL) {        // got problem with malloc
    return( -2);
  }

  nLangTranslationsAlloced = nTranslations;
  nLangTranslationsUsed    = 0;

  nLangTagsTextSpaceAlloced = nData;
  nLangTagsTextSpaceUsed    = 0;

  // pass 2, save the translations

  nTranslations = 0;
  nData = 0;

  for( i = 0; i < numFiles; i++) {

    pName = list[i]->d_name;

    LenName = strlen( pName);

    if( pName[ 0] == '\0' ||   // End of string
        pName[ 0] == '.' ) {   // current dir or dir up

      continue;
    }

    // Skip directories. Directories have a '/' as last character
    if( LenName > 0 && pName[ LenName - 1] == '/') { // Is a directory

      continue;
    }

    // Check for .txt extension

    if( LenName <= 4 ||
        stricmp( pName + LenName - 4, ".txt") != 0) {

      continue;
    }

    // Construct path to language file

    strcpy( Files_path, YaIPS_WorkingDirectory);
    strcat( Files_path, "/Languages/");
    strcat( Files_path, pLanguage);
    strcat( Files_path, "/");
    strcat( Files_path, pName);

    IqeB_FileNormalizePathChars( Files_path);

    // Load translations

    LangLoadTranslationFile( Files_path, true, &nTranslations, &nData, NewLineCharForMultipleStringLines);
  }

  // Free the file list

  fl_filename_free_list( &list, numFiles);

  // pass 3, Test for entries with same tag
  //         Later ones overwrites the text from before.

  for( i = 0; i < nLangTranslationsUsed - 1; i++) {

RestartCleanDouble:

    for( j = i + 1; j < nLangTranslationsUsed; j++) {

      if( strcmp( pLangTranslations[ i].pTag, pLangTranslations[ j].pTag) == 0) { // Got one

        int nCopyDown;

        nCopyDown = nLangTranslationsUsed - i - 1;

        if( nCopyDown <= 0) {    // security test
          break;                 // This should not happen
        }

        memcpy( pLangTranslations + i, pLangTranslations + i + 1, nCopyDown * sizeof( T_LangTranslations));

        nLangTranslationsUsed -= 1;    // Have one less

        goto RestartCleanDouble;       // Restart search
      }
    }
  }

  // pass 4, sort the entries by tag

  qsort( (void *)pLangTranslations, (size_t)nLangTranslationsUsed, sizeof( T_LangTranslations), LangTranslationCompare);

  // ...

  // Update common language strings used by FLTK

  fl_cancel = LANGDEF_BUTTON_CANCEL;
  fl_close  = LANGDEF_BUTTON_CLOSE;
  fl_ok     = LANGDEF_BUTTON_OK;
  fl_no     = LANGDEF_BUTTON_NO;
  fl_yes    = LANGDEF_BUTTON_YES;

  return( 0);
}


/*===========================================================================

	Lang_Init()

  Load the language files for the last selected language.
  Use this after starting your application.

  Arguments:

    int NewLineCharForMultipleStringLines
                           If set, insert new line characters at end of multiple string lines.
                                Example: tag_xyz  "Line 1"
                                "Line 2"
                                "Line 3"

  NOTE: The last language used is stored in a global variable

  Return:

    None

=============================================================================
*/
void Lang_Init( int NewLineCharForMultipleStringLines)
{

  LangLoadTranslations( YaIPS_Setting_Language, NewLineCharForMultipleStringLines);
}

/*===========================================================================

  LangStringLookup()

  Look up a language definition.

  Arguments:

    char *pString       Look up this language definition.
                        It should be in the format "&tag=string".

  Return:

    NULL                This only happens if the 'pString' argument is 'NULL'.

    pointer to string   * If the first character does not start with '&',
                          the 'pString' argument string will be returned.
                        * If a translation is found, this will be returned.
                        * If no translation is found, the text following the '='
                          character in the 'pString' argument will be returned.

  NOTE: The translations remain valid as long as no language data is loaded.
        This means that after loading language data, the application must be
        closed and restarted. For example, after selecting a new language.

=============================================================================
*/

char * LangStringLookup( const char *pString)
{
  char *p;

  p = LangStringLookup( (char *)pString);

  return( p);
}

char * LangStringLookup( char *pString)
{
  char *p, *pTag;
  char Tag[ 256];
  int i;
  T_LangTranslations SearchSetup;
  T_LangTranslations *pSearchResult;

  if( pString == NULL) {           // points to Null
    return( pString);              // nothing to do
  }

  if( pString[ 0] != '&') {        // is no language string
    return( pString);              // nothing to do
  }

  if( pLangTranslations == NULL || pLangTagsTextSpace == NULL) {   // no language data allocated
    goto GetInLineTranslation;
  }

  // extract Tag

  pTag = Tag;
  p = pString + 1;
  i = 0;

  for( ; ; ) {

    if( *p == '=' || *p == '\0' || i >= (int)sizeof( Tag) - 1) {
      *pTag++ = '\0';
      break;
    }

    *pTag++ = *p++;
    i += 1;
  }

  // lookup in language pool

#ifdef use_again
  for( i = 0; i < nLangTranslationsUsed; i++) {

    if( strcmp( Tag, pLangTranslations[ i].pTag) == 0) {

      return( pLangTranslations[ i].pText);
    }

  }
#else

  SearchSetup.pTag = Tag;

  pSearchResult = (T_LangTranslations *)bsearch( (void *)&SearchSetup, (void *)pLangTranslations, nLangTranslationsUsed,
                              sizeof( T_LangTranslations), LangTranslationCompare);

  if( pSearchResult != NULL) {

    return( pSearchResult->pText);
  }

#endif

  // no translation found

GetInLineTranslation:

  p = strchr( pString, '=');
  if( p != NULL) {

    return( p + 1);
  }

  return( pString);
}


/************************************* End Of File ***************************/
