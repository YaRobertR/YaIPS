/****************************************************************************

  YaIPS_Utils_Misc.cpp

  03.01.2025 RR: First edition of this file.
  03.09.2026 RR: * New function IqeB_FileNormPathCharsAndCWD().
                   Normalize path characters and current working directory.
                   Replace the begin of the path string with '.'
                   if the begin is equal to the 'YaIPS_WorkingDirectory'.
                 * New function IqeB_DirExsits().
                   Test for a directory to exists.
  08.09.2026 RR: * New function IqeB_FileCopyFilesInDir().
                   Copy files in a directory to an other directory.
                 * New function IqeB_FileDelFilesInDir().
                   Delete files in a directory.

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

#include <FL/Fl_Copy_Surface.H>
#include <FL/Fl_Image_Surface.H>

// ---------------------------------------------------------------------------------
// Math utilities
// ---------------------------------------------------------------------------------

/************************************************************************************
 * GreatestcommonDivisor
 *
 * Get the greatest common divisor of two numbers.
 */
int GreatestcommonDivisor( int a, int b)
{
  if (b == 0) {

    return a;
  }

  return GreatestcommonDivisor( b, a % b);
}

// ---------------------------------------------------------------------------------
// File system utilities
// ---------------------------------------------------------------------------------

/************************************************************************************
 * IqeB_FileNormalizePathChars()
 *
 * Normalize the path delimiter characters to
 *   \  for WIN32
 *   /  for other operting systems.
 */

void IqeB_FileNormalizePathChars( char *pPath)
{
  char *p, WrongPathChar, NormPathChar;

  // Normalize path slasches

#ifdef _WIN32
    WrongPathChar = '/';
    NormPathChar = '\\';
#else
    iWrongPathChar = '\\';
    NormPathChar = '/';
#endif

  p = pPath;
  while( *p != '\0') {

    if( *p == WrongPathChar) *p = NormPathChar;
    p++;
  }
}

/************************************************************************************
 * IqeB_FileNormPathCharsAndCWD()
 *
 * Normalize path characters and current working directory.
 *
 * Normalize the path delimiter characters to
 *   \  for WIN32
 *   /  for other operating systems.
 * and replace the begin of the path string with '.'
 * if the begin is equal to the 'YaIPS_WorkingDirectory'.
 *
 * Note: We rely on YaIPS_WorkingDirectory having normalized path characters.
 */

void IqeB_FileNormPathCharsAndCWD( char *pPath)
{
  char *p, WrongPathChar, NormPathChar;
  int CWDlen;

  // Normalize path slashes

#ifdef _WIN32
    WrongPathChar = '/';
    NormPathChar = '\\';
#else
    iWrongPathChar = '\\';
    NormPathChar = '/';
#endif

  p = pPath;
  while( *p != '\0') {

    if( *p == WrongPathChar) *p = NormPathChar;
    p++;
  }

  // Normalize current working directory

  CWDlen = strlen( YaIPS_WorkingDirectory);    // Length of current working directory string

  if( CWDlen > 0 &&                            // Have a current working directory string
      (int)strlen( pPath) > CWDlen + 1 &&      // and path is longer than current working director string
      pPath[ CWDlen] == NormPathChar) {        // and has an other sub path after it.

    if( strnicmp( pPath, YaIPS_WorkingDirectory, CWDlen) == 0) {  // Begin of path is equal to ...

      pPath[ 0] = '.';                         // Set '.' as current working directory

      strcpy( pPath + 1, pPath + CWDlen);      // Copy down part after current working directory
    }
  }
}

/************************************************************************************
 * IqeB_FileGetBaseName()
 *
 * Get basename from filename width any path and extension
 *   \  for WIN32
 *   /  for other operting systems.
 */

void IqeB_FileGetBaseName( char *pFilename, char *pBasename, int SizeBasename)
{
  char *p;

	p = strrchr( pFilename, '/');
	if (!p) p = strrchr( pFilename, '\\');
	if (!p) p = pFilename; else p++;

	memset( pBasename, 0, SizeBasename);       // Zero destination

	strncpy( pBasename, p, SizeBasename - 1);
	p = strrchr( pBasename, '.');   // remove file extension
	if (p) *p = 0;
}

/************************************************************************************
 * IqeB_FileGetFileName()
 *
 * Get filename width out any path
 *   \  for WIN32
 *   /  for other operating systems.
 */

void IqeB_FileGetFileName( char *pFilename, char *pOut, int SizeOut)
{
  char *p;

  p = strrchr( pFilename, '/');
  if (!p) p = strrchr( pFilename, '\\');
  if (!p) p = pFilename; else p++;

  memset( pOut, 0, SizeOut);       // Zero destination

  strncpy( pOut, p, SizeOut - 1);
}

/************************************************************************************
 * IqeB_FileGetPath()
 *
 * Get path part from file name with path.
 *   \  for WIN32
 *   /  for other operating systems.
 *
 * Keep slash at end of path.
 */

void IqeB_FileGetPath( char *pFilename, char *pOut, int SizeOut)
{
  char *p;
  int n;

  p = strrchr( pFilename, '/');
  if (!p) p = strrchr( pFilename, '\\');
  if (!p) p = pFilename; else p++;

  n = p - pFilename;

  if( n > SizeOut - 1) {

    n = SizeOut - 1;
  }

  memset( pOut, 0, SizeOut);       // Zero destination

  strncpy( pOut, pFilename, n);
}

/************************************************************************************
 * IqeB_FileEnsureExtension()
 *
 * Ensure a specific file extension.
 *
 * If pExtension is NULL the extension is removed. The point for the extension stays.
 *
 */

void IqeB_FileEnsureExtension( char *pFilename, char *pExtension, char *pOut, int SizeOut)
{
  char *p;
  int i, SizeString;

  memset( pOut, 0, SizeOut);                // Zero destination

  strncpy( pOut, pFilename, SizeOut - 1);

  SizeString = strlen( pOut);

  // look for latest point

  p = pOut + SizeString;          // Preset with end of string

  for( i = 0; i < 6; i++) {

    if( SizeString - i - 1 >= 0 &&
        pOut[ SizeString - i - 1] == '.') {

      p = pOut + (SizeString - i - 1);

      break;
    }
  }

  // Append point before extension extension

  if( p - pOut > 1) {

    *p++ = '.';
  }

  // Append file extension

  if( pExtension != NULL) {         // Have an extension

    while( p - pOut > 1 && *pExtension != 0) {

      *p++ = *pExtension++;
    }
  }

  *p++ = 0;      // Add end of string
}

/************************************************************************************
 * IqeB_FileCheckExtension()
 *
 * Check a file name to have a specific extension.
 *
 * NOTE: Only extensions up to a length of 5 characters are supported.
 *
 * pFilename       Point to file name
 * pExtensionList  List of comma separated extensions
 *
 * Return:   true   OK. File name have one of the given extensions.
 *           false  No match
 *           < 0    Error
 */

int IqeB_FileCheckExtension( char *pFilename, const char *pExtensionList)
{
  int LenName, LenExtension, LenCheck;
  char *pExtension, *p;
  char *pBegin, *pNext, *pToken;

  if( pFilename == NULL || pFilename[ 0] == '\0') {    // Check for filename argument

    return( -1);   // Return ERROR
  }

  LenName = strlen( pFilename);   // Length of file name

  if( LenName < 4) {              // Name is too short

    return( -2);   // Return ERROR
  }

  // Get length of extension

  p = strrchr( pFilename, '.');   // Check for last point
  if( p == NULL) {                // No point

    return( -3);   // Return ERROR
  }

  LenExtension = strlen( p + 1);  // Length of extension

  if( LenExtension > 5) {         // Above max reasonable length

    return( -4);   // Return ERROR
  }

  pExtension = pFilename + LenName - LenExtension;   // Point to begin of extension

  // Check for a known image file type
  // The image type list are comma separated file extensions.

  pBegin = (char *)pExtensionList;
  pNext  = (char *)pExtensionList;

  for( ; ; ) {                                // Loop over all file extensions

    pToken = strchr( pNext, ',');             // Get next delimiter

    if( pToken == NULL) {                     // No next token delimiter

      LenCheck = strlen( pBegin);             // Is last token

      pNext = NULL;
    } else {

      LenCheck = pToken - pBegin;             // Length of this extension

      pNext = pToken + 1;
    }

    if( LenCheck == LenExtension) {          // Length extension must math

      if( strnicmp( pExtension, pBegin, LenExtension) == 0) {  // Have a matching file extension

        return( true);   // Return OK
      }
    }

    // Last extension was processed

    if( pNext == NULL) {                      // End of list

      break;
    }

    pBegin = pNext;                           // Point after this extension
  }

  return( false);    // Return none found
}

/************************************************************************************
 * IqeB_FileExsits()
 *
 * Test for a file to exists.
 *
 * Return:   true   file exists
 *           false  file is not existing
 */

int IqeB_FileExsits( char *pFilename)
{
  FILE *pFile;

  pFile = fl_fopen( pFilename, "rb");    // Try to open file

  if( pFile == NULL) {                   // File not found ?

    return( false);
  }

  fclose( pFile);                        // Close file

  return( true);
}

/************************************************************************************
 * IqeB_DirExsits()
 *
 * Test for a directory to exists.
 *
 * Return:   directory exists
 *           directory is not existing
 */

int IqeB_DirExsits( char *pPath)
{
  struct stat info;

//#include <sys/stat.h>
//#include <stdio.h>

  if( stat( pPath, &info) != 0) {

    return( false);
  }

  return( info.st_mode & S_IFDIR) != 0;
}

/************************************************************************************
 * IqeB_FileDelete()
 *
 * Delete file
 *
 * Return:
 */

void IqeB_FileDelete( char *pFilename)
{

  DeleteFile( pFilename);
}

/************************************************************************************
 * IqeB_FileMakePath()
 *
 * recursively create a path in the file system.
 *
 * NOTE: Argument must be a writable string.
 *       Path characters are overwritten!
 *
 * Return:
 */

void IqeB_FileMakePath( char *pPath)
{
  char *p, SavePathChar;

  if( IqeB_DirExsits( pPath)) {     // If this directory exists, return

    return;
  }

  p = strrchr( pPath, '/');        // Test for unix style path character
  if( p == NULL) {

    p = strrchr( pPath, '\\');     // Test for windows style path character
  }

  if( p == NULL) {                 // None found

    return;
  }

  SavePathChar = *p;               // Save path character

  *p = '\0';                       // Set end of path string

  IqeB_FileMakePath( pPath);       // Recursive test

  *p = SavePathChar;               // Restore path character

  fl_mkdir( pPath, 0700);
}

/************************************************************************************
 * IqeB_FileCopyFilesInDir()
 *
 * Copy files in a directory to an other directory.
 *
 * Return:
 */

void IqeB_FileCopyFilesInDir( char *pDirDst, char *pDirSrc)
{
  char FileSrc[ MAX_FILENAME_LEN];
  char FileDst[ MAX_FILENAME_LEN];
  int  numFiles, i, LenName;
  dirent **list;
  char *pName;

  // test for language files

  numFiles = fl_filename_list( pDirSrc, &list, fl_alphasort);

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

    // Construct path source path

    strcpy( FileSrc, pDirSrc);
    strcat( FileSrc, "/");
    strcat( FileSrc, pName);

    IqeB_FileNormalizePathChars( FileSrc);

    // Construct destinationn path for file

    strcpy( FileDst, pDirDst);
    strcat( FileDst, "/");
    strcat( FileDst, pName);

    IqeB_FileNormalizePathChars( FileDst);

    // Copy file

    CopyFile( FileSrc, FileDst, false);
  }

  // Free the file list

  fl_filename_free_list( &list, numFiles);

}

/************************************************************************************
 * IqeB_FileDelFilesInDir()
 *
 * Delete files in a directory.
 *
 * NOTE: Sub directories are not deleted.
 *
 * Return:
 */

void IqeB_FileDelFilesInDir( char *pDir)
{
  char FileSrc[ MAX_FILENAME_LEN];
  int  numFiles, i, LenName;
  dirent **list;
  char *pName;

  // test for language files

  numFiles = fl_filename_list( pDir, &list, fl_alphasort);

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

    // Construct path source path

    strcpy( FileSrc, pDir);
    strcat( FileSrc, "/");
    strcat( FileSrc, pName);

    IqeB_FileNormalizePathChars( FileSrc);

    // Delete file

    fl_unlink( FileSrc);
  }

  // Free the file list

  fl_filename_free_list( &list, numFiles);

}

// ---------------------------------------------------------------------------------
// string utilities
// ---------------------------------------------------------------------------------

/************************************************************************************
 * IqeB_strcasecmp()
 *
 * Compare strings S1 and S2, ignoring case, returning less than, equal to or
 * greater than zero if S1 is lexicographically less than, equal to or greater
 * than S2.
 */

int IqeB_strcasecmp( const char *s1, const char *s2)
{
  const unsigned char *p1, *p2;
  unsigned char c1, c2;

  if (s1 == s2)
    return 0;

  /* Be careful not to look at the entire extent of s1 or s2 until needed.
     This is useful because when two strings differ, the difference is
     most often already in the very few first characters.  */

  p1 = (const unsigned char *)s1;
  p2 = (const unsigned char *)s2;

  do {

    c1 = *p1;
    if( isupper( c1)) c1 = tolower( c1);

    c2 = *p2;
    if( isupper( c2)) c2 = tolower( c2);

    if (c1 == '\0')
      break;

    ++p1;
    ++p2;

  }  while( c1 == c2);

  return c1 - c2;
}

/************************************************************************************
 * IqeB_strncasecmp()
 *
 * Compare no more than N bytes of strings S1 and S2,
 * ignoring case, returning less than, equal to or
 * greater than zero if S1 is lexicographically less
 * than, equal to or greater than S2.
*/

int IqeB_strncasecmp( const char *s1, const char *s2, int n)
{
  const unsigned char *p1 = (const unsigned char *) s1;
  const unsigned char *p2 = (const unsigned char *) s2;
  unsigned char c1, c2;

  if (p1 == p2 || n == 0)
    return 0;

  do {

    c1 = *p1;
    if( isupper( c1)) c1 = tolower( c1);

    c2 = *p2;
    if( isupper( c2)) c2 = tolower( c2);

    if (--n == 0 || c1 == '\0') {
      break;
    }

    ++p1;
    ++p2;

  } while (c1 == c2);

  return c1 - c2;
}

/************************************************************************************
 * IqeB_strcasestr()
 *
 * Find string s2 in s1.
 *
*/

char *IqeB_strcasestr( char *s1, char *s2)
{
  int i, l1, l2;

  l1 = strlen( s1);
  l2 = strlen( s2);

  if( l1 < l2) {        // First is shorter

    return( NULL);      // This never may match
  }

  for( i = 0; i < l1 - l2 + 1; i++) {

    if( IqeB_strncasecmp( s1 + i, s2, l2) == 0) {  // Have a match

      return( s1 + i);     // return match point
    }
  }

  return( NULL);          // No match
}

/************************************************************************************
 * IqeB_strdup()
 *
 * Get copy of string.
 * Allocated memory for it.
*/

char *IqeB_strdup( const char *s)
{
  char *d;

  if( s == NULL) {                     // Argument is a NULL pointer

    return( NULL);                     // Also return NULL
  }

  d = (char *)malloc (strlen (s) + 1); // Space for length plus nul byte
  if (d == NULL) {                     // No memory

    return NULL;                       // return NULL
  }

  strcpy( d, s);                       // Copy the characters
  return d;                            // Return the new string
}

/************************************************************************************
 * YaIPS_Utils_AddSympols()
 *
 * Add additional symbols (for buttons, ...)
*/

// Some help stuff

#define BP fl_begin_polygon()
#define EP fl_end_polygon()
#define BCP fl_begin_complex_polygon()
#define ECP fl_end_complex_polygon()
#define BL fl_begin_line()
#define EL fl_end_line()
#define BC fl_begin_loop()
#define EC fl_end_loop()
#define vv(x,y) fl_vertex(x,y)

//for the outline color
static void set_outline_color(Fl_Color c) {
  fl_color(fl_darker(c));
}

static void rectangle(double x,double y,double x2,double y2,Fl_Color col) {
  fl_color(col);
  BP; vv(x,y); vv(x2,y); vv(x2,y2); vv(x,y2); EP;
  set_outline_color(col);
  BC; vv(x,y); vv(x2,y); vv(x2,y2); vv(x,y2); EC;
}

static void draw_menu2( Fl_Color col)
{
  fl_color(col);
  BL; vv(-0.65, 0.5); vv(0.65, 0.5); EL;
  BL; vv(-0.65, 0.0); vv(0.65, 0.0); EL;
  BL; vv(-0.65,-0.5); vv(0.65,-0.5); EL;
}

static void draw_cross( Fl_Color col)
{
  rectangle(-1.0, -0.2, 1.0, 0.2, col);
  rectangle(-0.2, -1.0, 0.2, 1.0, col);
}

static void draw_line2( Fl_Color col)
{
  rectangle(-0.9, -0.1, 0.9, 0.1, col);
}

static void draw_cross2( Fl_Color col)
{
  rectangle(-0.9, -0.1, 0.9, 0.1, col);
  rectangle(-0.1, -0.9, 0.1, 0.9, col);
}

static void draw_pencil( Fl_Color col)
{
  fl_color(fl_color_average(col, FL_WHITE, 0.25f));
  BP;
    vv(0.4, -1.0);
    vv(1.0, -0.4);
    vv(-0.4, 1.0);
    vv(-1.0, 1.0);
    vv(-1.0, 0.4);
  EP;

  fl_color(fl_darker(col));
  BC;
    vv(0.4, -1.0);
    vv(1.0, -0.4);
    vv(-0.4, 1.0);
    vv(-1.0, 1.0);
    vv(-1.0, 0.4);
  EC;

  BL;
    vv(-0.4, 1.0);
    vv(-1.0, 0.4);
  EL;

  BP;
    vv(-0.6, 1.0);
    vv(-1.0, 1.0);
    vv(-1.0, 0.6);
  EP;
}

static void draw_camera( Fl_Color col)
{

  // Camera body
  fl_color(fl_color_average(col, FL_WHITE, 0.25f));
  BP;
    vv(-1.0, -0.6);
    vv( 0.2, -0.6);
    vv( 0.2,  0.6);
    vv(-1.0,  0.6);
  EP;

  // Lens
  BP;
    vv( 0.2, -0.3);
    vv( 1.0, -0.5);
    vv( 1.0,  0.5);
    vv( 0.2,  0.3);
  EP;

  // Camera body
  fl_color(fl_darker(col));
  BC;
    vv(-1.0, -0.6);
    vv( 0.2, -0.6);
    vv( 0.2,  0.6);
    vv(-1.0,  0.6);
  EC;

  // Lens
  BL;
    vv( 0.2, -0.3);
    vv( 1.0, -0.5);
    vv( 1.0,  0.5);
    vv( 0.2,  0.3);
  EL;
}

// Copied form draw_filesave(). Bright parts are brighter.
static void draw_filesave2(Fl_Color c) {
  fl_color(c);
  BP;
    vv(-0.9, -1.0);
    vv(0.9, -1.0);
    vv(1.0, -0.9);
    vv(1.0, 0.9);
    vv(0.9, 1.0);
    vv(-0.9, 1.0);
    vv(-1.0, 0.9);
    vv(-1.0, -0.9);
  EP;

#ifdef use_again
  fl_color( fl_lighter(c));
#else
  fl_color( fl_color_average(c, FL_WHITE, .3f));
#endif
  BP;
    vv(-0.7, -1.0);
    vv(0.7, -1.0);
    vv(0.7, -0.4);
    vv(-0.7, -0.4);
  EP;

  BP;
    vv(-0.7, 0.0);
    vv(0.7, 0.0);
    vv(0.7, 1.0);
    vv(-0.7, 1.0);
  EP;

  fl_color(c);
  BP;
    vv(-0.5, -0.9);
    vv(-0.3, -0.9);
    vv(-0.3, -0.5);
    vv(-0.5, -0.5);
  EP;

#ifdef use_again
  fl_color(fl_darker(c));
#else
  fl_color( fl_color_average(c, FL_BLACK, .5f));
#endif
  BC;
    vv(-0.9, -1.0);
    vv(0.9, -1.0);
    vv(1.0, -0.9);
    vv(1.0, 0.9);
    vv(0.9, 1.0);
    vv(-0.9, 1.0);
    vv(-1.0, 0.9);
    vv(-1.0, -0.9);
  EC;
}

// Draw double arrow used for horizontal spacing
static void draw_SpacingH(Fl_Color c) {
  fl_color(c);
  BL;
    vv(-1.0, 0.0);
    vv( 1.0, 0.0);
  EL;
  BL;
    vv(-0.6,  0.6);
    vv(-1.0,  0.0);
    vv(-0.6, -0.6);
  EL;
  BL;
    vv( 0.6,  0.6);
    vv( 1.0,  0.0);
    vv( 0.6, -0.6);
  EL;
}

// Draw double arrow used for vertical spacing
static void draw_SpacingV(Fl_Color c) {
  fl_color(c);

  // Double arrow
  BL;
    vv( 0.0,-1.0);
    vv( 0.0, 1.0);
  EL;
  BL;
    vv( 0.6, -0.6);
    vv( 0.0, -1.0);
    vv(-0.6, -0.6);
  EL;
  BL;
    vv( 0.6,  0.6);
    vv( 0.0,  1.0);
    vv(-0.6,  0.6);
  EL;

  // Lines
  BL;
    vv( 1.2, 0.4);
    vv( 2.5, 0.4);
  EL;
  BL;
    vv( 1.2,-0.4);
    vv( 3.0,-0.4);
  EL;
}

// Align horizontal left
static void draw_AlignHL(Fl_Color c) {
  fl_color(c);
  BL;
    vv(-1.0,-1.0);
    vv( 1.0,-1.0);
  EL;
  BL;
    vv(-1.0,-0.33);
    vv( 0.0,-0.33);
  EL;
  BL;
    vv(-1.0, 0.33);
    vv( 1.0, 0.33);
  EL;
  BL;
    vv(-1.0, 1.0);
    vv( 0.0, 1.0);
  EL;
}

// Align horizontal center
static void draw_AlignHC(Fl_Color c) {
  fl_color(c);
  BL;
    vv(-1.0,-1.0);
    vv( 1.0,-1.0);
  EL;
  BL;
    vv(-0.5,-0.33);
    vv( 0.5,-0.33);
  EL;
  BL;
    vv(-1.0, 0.33);
    vv( 1.0, 0.33);
  EL;
  BL;
    vv(-0.5, 1.0);
    vv( 0.5, 1.0);
  EL;
}

// Align horizontal right
static void draw_AlignHR(Fl_Color c) {
  fl_color(c);
  BL;
    vv(-1.0,-1.0);
    vv( 1.0,-1.0);
  EL;
  BL;
    vv( 0.0,-0.33);
    vv( 1.0,-0.33);
  EL;
  BL;
    vv(-1.0, 0.33);
    vv( 1.0, 0.33);
  EL;
  BL;
    vv( 0.0, 1.0);
    vv( 1.0, 1.0);
  EL;
}

// Align vertical top
static void draw_AlignVT(Fl_Color c) {
  fl_color(c);
  BL;
    vv(-1.0,-1.0);
    vv( 1.0,-1.0);
  EL;
  BL;
    vv(-0.6,-0.5);
    vv( 0.6,-0.5);
  EL;
}

// Align vertical center
static void draw_AlignVC(Fl_Color c) {
  fl_color(c);
  BL;
    vv(-1.0,-0.25);
    vv( 1.0,-0.25);
  EL;
  BL;
    vv(-0.6, 0.25);
    vv( 0.6, 0.25);
  EL;
}

// Align vertical bottom
static void draw_AlignVB(Fl_Color c) {
  fl_color(c);
  BL;
    vv(-0.6, 1.0);
    vv( 0.6, 1.0);
  EL;
  BL;
    vv(-1.0, 0.5);
    vv( 1.0, 0.5);
  EL;
}

// Closed padlock
static void draw_PadlockC(Fl_Color c) {

  // Lock body
  fl_color(c);
  BP;
    vv(-0.6,-0.2);
    vv( 0.6,-0.2);
    vv( 0.6, 1.0);
    vv(-0.6, 1.0);
    vv(-0.6,-0.2);
  EP;

  // Lock clip
  fl_line_style( 0, 2);
  BL;
#ifdef use_again
    vv(-0.45,-0.20);
    vv(-0.45,-0.30);
    vv(-0.35,-0.55);
    vv(-0.20,-0.65);
    vv( 0.00,-0.70);
    vv( 0.20,-0.65);
    vv( 0.35,-0.55);
    vv( 0.45,-0.30);
    vv( 0.45,-0.20);
#else
    vv(-0.40,-0.20);
    vv(-0.40,-0.30);
    vv(-0.30,-0.55);
    vv(-0.15,-0.65);
    vv( 0.00,-0.70);
    vv( 0.15,-0.65);
    vv( 0.30,-0.55);
    vv( 0.40,-0.30);
    vv( 0.40,-0.20);
#endif
  EL;
  fl_line_style( 0);

  // Lock hole
  fl_color(FL_BACKGROUND_COLOR);
  BP;
    vv(-0.15, 0.1);
    vv( 0.15, 0.1);
    vv( 0.15, 0.7);
    vv(-0.15, 0.7);
    vv(-0.15, 0.1);
  EP;
}

// Open padlock
static void draw_PadlockO(Fl_Color c) {

  // Lock body
  fl_color(c);
  BP;
    vv(-0.6,-0.2);
    vv( 0.6,-0.2);
    vv( 0.6, 1.0);
    vv(-0.6, 1.0);
    vv(-0.6,-0.2);
  EP;

  // Lock clip
  fl_line_style( 0, 2);
  BL;
#ifdef use_again
    vv(-0.45,-0.20);
    vv(-0.45,-0.60);
    vv(-0.35,-0.85);
    vv(-0.20,-0.95);
    vv( 0.00,-1.00);
    vv( 0.20,-0.95);
    vv( 0.35,-0.85);
    vv( 0.45,-0.60);
    //x/vv( 0.45,-0.50);
#else
    vv(-0.40,-0.20);
    vv(-0.40,-0.60);
    vv(-0.30,-0.85);
    vv(-0.15,-0.95);
    vv( 0.00,-1.00);
    vv( 0.15,-0.95);
    vv( 0.30,-0.85);
    vv( 0.40,-0.60);
    //x/vv( 0.40,-0.50);
#endif
  EL;
  fl_line_style( 0);

  // Lock hole
  fl_color(FL_BACKGROUND_COLOR);
  BP;
    vv(-0.15, 0.1);
    vv( 0.15, 0.1);
    vv( 0.15, 0.7);
    vv(-0.15, 0.7);
    vv(-0.15, 0.1);
  EP;
}

// Double bar 2. This is a variant of the '||' symbol.
// The gap between the bars is bigger.
static void draw_doublebar2(Fl_Color col) {
  rectangle(-0.7,-0.8,-.2,.8,col);
  rectangle(.2,-0.8,.7,.8,col);
}

void YaIPS_Utils_AddSympols()
{

  fl_add_symbol(     "menu2",     draw_menu2, 1);
  fl_add_symbol(     "cross",     draw_cross, 1);
  fl_add_symbol(     "line2",     draw_line2, 1);
  fl_add_symbol(    "cross2",    draw_cross2, 1);
  fl_add_symbol(    "pencil",    draw_pencil, 1);
  fl_add_symbol(    "camera",    draw_camera, 1);
  fl_add_symbol( "filesave2", draw_filesave2, 1);   // Copied form draw_filesave(). Bright parts are brighter.
  fl_add_symbol(  "SpacingH",  draw_SpacingH, 1);   // Draw double arrow used for horizontal spacing
  fl_add_symbol(  "SpacingV",  draw_SpacingV, 1);   // Draw double arrow used for vertical spacing
  fl_add_symbol(   "AlignHL",   draw_AlignHL, 1);   // Align horizontal left
  fl_add_symbol(   "AlignHC",   draw_AlignHC, 1);   // Align horizontal center
  fl_add_symbol(   "AlignHR",   draw_AlignHR, 1);   // Align horizontal right
  fl_add_symbol(   "AlignVT",   draw_AlignVT, 1);   // Align vertical top
  fl_add_symbol(   "AlignVC",   draw_AlignVC, 1);   // Align vertical center
  fl_add_symbol(   "AlignVB",   draw_AlignVB, 1);   // Align vertical bottom
  fl_add_symbol(  "PadlockC",   draw_PadlockC, 1);  // Closed padlock
  fl_add_symbol(  "PadlockO",   draw_PadlockO, 1);  // Open padlock
  fl_add_symbol(     "Dbar2", draw_doublebar2, 1);  // Double bar 2. This is a variant of the '||' symbol.
}

/************************************************************************************
 * pYaIPS_PixelDepth_to_string()
 *
 * Convert Pixel depth to string
 *
 * PixelDepth   # bytes per pixel, range is 1 .. 4
 *
*/
char *pYaIPS_PixelDepth_to_string( int PixelDepth)
{
  char *p;

  p = (char *)"???";   // Preset default

  switch( PixelDepth) {

  case 1:
    p = LANGDEF_PIXDEPTH_1_TXT;
    break;

  case 2:
    p = LANGDEF_PIXDEPTH_2_TXT;
    break;

  case 3:
    p = LANGDEF_PIXDEPTH_3_TXT;
    break;

  case 4:
    p = LANGDEF_PIXDEPTH_4_TXT;
    break;
  }

  return( p);
}

/************************************************************************************
 * YaIPS_Utils_FontsEnum()
 *
 * Enumerate known fonts on this PC.
 *
 * NOTE: Depending on the argument, Fl::set_fonts() returns a different number of fonts.
 *       For  "*"   count is 1336
 *       For "-*"   count is 1336
 *       For NULL   count is 1230_
*/

YaIPS_FontBase_t *pYaIPS_FontBase; // Point to font face pool

int YaIPS_nFontBase;       // Number of base fonts
int YaIPS_FontsCount;      // Number of enumerated fonts

static int FontBaseCompare( const void *arg1, const void *arg2 )
{
  YaIPS_FontBase_t *p1;
  YaIPS_FontBase_t *p2;

  p1 = (YaIPS_FontBase_t *)arg1;
  p2 = (YaIPS_FontBase_t *)arg2;

  return stricmp( p1->FontName, p2->FontName);
}

void YaIPS_Utils_FontsEnum()
{
  int i, nFontBase;
  const char *pName;
  char NameLast[ 512];
  YaIPS_FontBase_t *pFontBase;

  // Get the number of fonts installed on this computer

  YaIPS_FontsCount = Fl::set_fonts( "*");

  // Count the fonts

  if( pYaIPS_FontBase != NULL) {     // Multiple calles

    free( pYaIPS_FontBase);
    pYaIPS_FontBase = NULL;
  }

  // Count number of difference base fonts

  nFontBase = 0;
  YaIPS_nFontBase = 0;

  for( i = 0; i < YaIPS_FontsCount; i++) {

    pName = Fl::get_font((Fl_Font)i);

    if( i == 0 ||                          // First time
        strcmp( NameLast, pName + 1) != 0) {   // or font name is different

      nFontBase += 1;             // Have one more base font

      strcpy( NameLast, pName + 1);    // Remember last name
    }
  }

  // Allocate memory for base fonts

  if( nFontBase > 0) {

    pYaIPS_FontBase = (YaIPS_FontBase_t *)malloc( sizeof( YaIPS_FontBase_t) * nFontBase);
  }

  if( pYaIPS_FontBase == NULL) {    // Security test

    return;
  }

  // Fill table with base fonts

  memset( pYaIPS_FontBase, 0, sizeof( YaIPS_FontBase_t) * nFontBase);   // Zero memory

  YaIPS_nFontBase = 0;

  pFontBase = pYaIPS_FontBase;

  for( i = 0; i < YaIPS_FontsCount; i++) {

    pName = Fl::get_font((Fl_Font)i);

    if( i == 0 ||                          // First time
        strcmp( NameLast, pName + 1) != 0) {   // or font name is different

      if( YaIPS_nFontBase >= nFontBase) {   // Security test, something not OK

        break;
      }

      pFontBase = pYaIPS_FontBase + YaIPS_nFontBase;

      pFontBase->FontNr_regular     = -1;    // Preset not valid
      pFontBase->FontNr_bold        = -1;    // Preset not valid
      pFontBase->FontNr_italic      = -1;    // Preset not valid
      pFontBase->FontNr_bold_italic = -1;    // Preset not valid

#ifdef use_again
      if( pName[ 1] == '@') {

        strcpy( pFontBase->FontName, pName + 2);    // Copy name
        strcat( pFontBase->FontName, "*");          // Mark this
      } else {
        strcpy( pFontBase->FontName, pName + 1);    // RCopy name
      }
#else
      strcpy( pFontBase->FontName, pName + 1);    // RCopy name
#endif
      pFontBase->FontName[ 0] = toupper( pFontBase->FontName[ 0]);  // Ensure first character is an upper

      YaIPS_nFontBase += 1;                // Have one more base font

      strcpy( NameLast, pName + 1);    // Remember last name
    }

    switch( pName[ 0]) {      // Font face is coded in first character

    case ' ':       // Regular face

      pFontBase->FontNr_regular = i;
      break;

    case 'B':       // Bold face

      pFontBase->FontNr_bold = i;
      break;

    case 'I':       // Italic face

      pFontBase->FontNr_italic = i;
      break;

    case 'P':       // Bold Italic face

      pFontBase->FontNr_bold_italic = i;
      break;

    default:       // Bold Italic face

      if( pFontBase->FontNr_regular == 0) {   // not set until now

        pFontBase->FontNr_regular = i;
      }
      break;
    }
  }

  // Sort the fonts by name

  if( YaIPS_nFontBase > 1) {

    qsort( pYaIPS_FontBase, YaIPS_nFontBase, sizeof( YaIPS_FontBase_t), FontBaseCompare);
  }
}

/************************************************************************************
 * YaIPS_Utils_FontsLoookup()
 *
 * Lookup a known base font
 *
 * Return: NULL   Name not known
 *         else   pointer to base font base data.
*/

YaIPS_FontBase_t *pYaIPS_Utils_FontsLookup( char *pFontName)
{
  YaIPS_FontBase_t *pSearchResult;

  pSearchResult = NULL;

  if( pFontName == NULL ||        // Have no font name
      pFontName[ 0] == '\0') {

    pFontName = (char *)"Arial";          // Fall back to a default font
  }

  if( YaIPS_nFontBase > 0) {

    pSearchResult = (YaIPS_FontBase_t *)bsearch( (void *)pFontName, (void *)pYaIPS_FontBase, YaIPS_nFontBase,
                                  sizeof( YaIPS_FontBase_t), FontBaseCompare);
  }

  return( pSearchResult);
  }

/************************************************************************************
 * YaIPS_ImageDispEmpty
 *
 * Empty a display image.
 * Release all allocated data, reset variable and reset display box.
 */
void YaIPS_ImageDispEmpty( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, int KeepInfoStrings)
{
  Fl_RGB_Image *oldimage;                   // Old image

  // Delete the image

  if( pYaIPS_ImageDisp->pImage_Img != NULL) {           // Have an image

    pYaIPS_ImageDisp->pImage_Img->release();            // Release the image
    pYaIPS_ImageDisp->pImage_Img = NULL;                // Also reset pointer to image
  }

  // Delete box display image

  if( pYaIPS_ImageDisp->pImage_Box != NULL) {           // Have a display box

    // Remove image displayed in box
    oldimage = (Fl_RGB_Image *)pYaIPS_ImageDisp->pImage_Box->image();

    if( oldimage != NULL) {                           // There is an image

      oldimage->release();                            // Release date
      oldimage = NULL;
      pYaIPS_ImageDisp->pImage_Box->image((Fl_Image *)oldimage);  // Set the image to the display

      // Refresh
      pYaIPS_ImageDisp->pImage_Box->redraw();
    }
  }

  // Reset image data

  pYaIPS_ImageDisp->ImageName[ 0] = '\0';               // Reset name of image
  pYaIPS_ImageDisp->ImageSourceID    = YAIPS_WIN_ID_NONE; // Reset image source
  pYaIPS_ImageDisp->ImageName[ 0]    = '\0';            // Reset image name
  pYaIPS_ImageDisp->BigImage_Calc_OK = false;           // Reset size calculations
  pYaIPS_ImageDisp->ImageChanged = 0;                   // Reset Count

  if( ! KeepInfoStrings) {                              // Reset info strings
    pYaIPS_ImageDisp->StrInfo[ 0] = '\0';               // Reset user info
    pYaIPS_ImageDisp->StrDebug[ 0] = '\0';              // Reset debug info
  }
}

/************************************************************************************
 * YaIPS_ImageDispReleaseBeforeClose
 *
 * Before close of an window release all allocated data
 * in an image display structure and reset data.
 */
void YaIPS_ImageDispReleaseBeforeClose( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp)
{
#ifdef use_again
  if( pYaIPS_ImageDisp->pImage_Img) {

    pYaIPS_ImageDisp->pImage_Img->release();
  }

   // Zero the data
  memset( pYaIPS_ImageDisp, 0, sizeof( Fl_YaIPS_ImageDisp_t));
#else
  // This should work, but keeps the pointer to the box's GUI element.
  YaIPS_ImageDispEmpty( pYaIPS_ImageDisp);
#endif
}

/************************************************************************************
 * YaIPS_ImageDispCalcSizes
 *
 * Calculate sizes for drawing big image in a box
 *
 * MouseX, MouseY: Mouse position over image. Used for resolution changes.
 *                 Both have the value -99999 if not used as argument.
 *
 * return:     -1: No image box (no drawing area).
 *          false: No big image to display or no image source set
 *                 NOTE: The BigImage_bX variables are set
 *           true: OK, sizes are calculated
 */
int YaIPS_ImageDispCalcSizes( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, int NewFlagBits, int MouseX, int MouseY)
{
  int lw, lh;                // last width/height
  int ResolutionChanged;     // True if resolution has changed since last call
  int DoDispModify, DisplayResolution_This;
  float SizeFactor_New;

  // Check image box
  if( pYaIPS_ImageDisp->pImage_Box == NULL) {                     // of have no big image box

    // No image box (no drawing area).
    pYaIPS_ImageDisp->BigImage_Calc_OK = -1;                     // Size calculations are NOT OK

    return( pYaIPS_ImageDisp->BigImage_Calc_OK);
  }

  // Check for no image display modifications

  DoDispModify = (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_DO_DISP_MODIFY) != 0;

  // Set info about image box

  pYaIPS_ImageDisp->BigImage_bx = pYaIPS_ImageDisp->pImage_Box->x();         // Big image box
  pYaIPS_ImageDisp->BigImage_by = pYaIPS_ImageDisp->pImage_Box->y();
  pYaIPS_ImageDisp->BigImage_bw = pYaIPS_ImageDisp->pImage_Box->w();
  pYaIPS_ImageDisp->BigImage_bh = pYaIPS_ImageDisp->pImage_Box->h();

  // Check things needed
  if( pYaIPS_ImageDisp->pImage_Img == NULL ||                     // Have a NO big image
      pYaIPS_ImageDisp->ImageSourceID == YAIPS_WIN_ID_NONE) {    // or no image source set

    pYaIPS_ImageDisp->BigImage_Calc_OK = false;                   // Size calculations are NOT OK

    return( pYaIPS_ImageDisp->BigImage_Calc_OK);
  }

  // Update flag bits

  pYaIPS_ImageDisp->Flags |= NewFlagBits;                         // Or in new flag bits

  // Calculate display image size

  pYaIPS_ImageDisp->BigImage_iw = pYaIPS_ImageDisp->pImage_Img->data_w();    // Size of image
  pYaIPS_ImageDisp->BigImage_ih = pYaIPS_ImageDisp->pImage_Img->data_h();

  // Check for changed resolution

  DisplayResolution_This = pYaIPS_ImageDisp->DisplayResolution_Last;                 // Get current setting

  if( pYaIPS_ImageDisp->Plot3D_Active) {                                             // 3D plot active
    DisplayResolution_This |= 0x0100;                                               // or in this bit
  }

  ResolutionChanged = DisplayResolution_This != pYaIPS_ImageDisp->DisplayResolution;

  // Calculate ...

  lw = pYaIPS_ImageDisp->BigImage_bw - YAIPS_IMAGE_DISP_BORDER;
  lh = pYaIPS_ImageDisp->BigImage_bh - YAIPS_IMAGE_DISP_BORDER;

  if( DoDispModify && pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_1_1) {

    int SizeShift;
    float ImageCenter_X_Before, ImageCenter_Y_Before, ImageCenter_X_New, ImageCenter_Y_New;
    float Delta_Mouse_X, Delta_Mouse_Y, SizeFactor_Before;

    if( ResolutionChanged) {

      if( pYaIPS_ImageDisp->DisplayResolution_Last <= YAIPS_DISP_RESOLUTION_AUTO) {

        float TempW, TempH;

        // Enlargement factor before
        if( pYaIPS_ImageDisp->BigImage_iw > lw ||
            pYaIPS_ImageDisp->BigImage_ih > lh) {

          TempW = lw;
          TempH = TempW * (float)pYaIPS_ImageDisp->BigImage_ih / (float)pYaIPS_ImageDisp->BigImage_iw;

          if( TempH > lh) {

            TempH = lh;
            TempW = TempH * (float)pYaIPS_ImageDisp->BigImage_iw / (float)pYaIPS_ImageDisp->BigImage_ih;
          }
        } else {

          TempW = (float)pYaIPS_ImageDisp->BigImage_iw;
          TempH = (float)pYaIPS_ImageDisp->BigImage_ih;
        }

        // Size factor = pixel on screen / image pixel = one image pixel is how many screen pixels.
        if( pYaIPS_ImageDisp->BigImage_iw > pYaIPS_ImageDisp->BigImage_ih) {

          SizeFactor_Before =  (float)TempW / (float)pYaIPS_ImageDisp->BigImage_iw;
        } else {

          SizeFactor_Before =  (float)TempH / (float)pYaIPS_ImageDisp->BigImage_ih;
        }

        ImageCenter_X_Before = pYaIPS_ImageDisp->BigImage_iw * 0.5;
        ImageCenter_Y_Before = pYaIPS_ImageDisp->BigImage_ih * 0.5;

      } else {

        // Enlargement factor before
        SizeShift = pYaIPS_ImageDisp->DisplayResolution_Last - 1;          // Shift is enlargement by power of 2
        SizeFactor_Before =  (float)(1 << SizeShift);

        ImageCenter_X_Before = (int)(pYaIPS_ImageDisp->SubImage_x + 0.5) + pYaIPS_ImageDisp->SubImage_w * 0.5;
        ImageCenter_Y_Before = (int)(pYaIPS_ImageDisp->SubImage_y + 0.5) + pYaIPS_ImageDisp->SubImage_h * 0.5;
      }

      // Mouse offset adjusts center of image
      if( MouseX == -99999 && MouseY == -99999) {       // No mouse coordinates given

        Delta_Mouse_X = 0;
        Delta_Mouse_Y = 0;

      } else {

        // Delta mouse from center
        Delta_Mouse_X = (MouseX - (pYaIPS_ImageDisp->BigImage_sx + pYaIPS_ImageDisp->BigImage_sw * 0.5)) / SizeFactor_Before;
        Delta_Mouse_Y = (MouseY - (pYaIPS_ImageDisp->BigImage_sy + pYaIPS_ImageDisp->BigImage_sh * 0.5)) / SizeFactor_Before;

        ImageCenter_X_Before += Delta_Mouse_X;
        ImageCenter_Y_Before += Delta_Mouse_Y;
      }

    }

    if( pYaIPS_ImageDisp->DisplayResolution > YAIPS_DISP_RESOLUTION_X_16) {  // Security test: clip maximum value

      pYaIPS_ImageDisp->DisplayResolution = YAIPS_DISP_RESOLUTION_X_16;
    }

    // Enlargement factor new
    SizeShift = pYaIPS_ImageDisp->DisplayResolution - 1;          // Shift is enlargement by power of 2

    // Size factor = pixel on screen / image pixel = one image pixel is how many screen pixels.
    SizeFactor_New =  (float)(1 << SizeShift);

    // Horizontal size
    if( (pYaIPS_ImageDisp->BigImage_iw << SizeShift) > lw) {

      pYaIPS_ImageDisp->SubImage_w = lw >> SizeShift;
    } else {

      pYaIPS_ImageDisp->SubImage_w = pYaIPS_ImageDisp->BigImage_iw;
    }

    pYaIPS_ImageDisp->SubImage_x_max = pYaIPS_ImageDisp->BigImage_iw - pYaIPS_ImageDisp->SubImage_w;

    // Vertical size
    if( (pYaIPS_ImageDisp->BigImage_ih << SizeShift) > lh) {

      pYaIPS_ImageDisp->SubImage_h = lh >> SizeShift;
    } else {

      pYaIPS_ImageDisp->SubImage_h = pYaIPS_ImageDisp->BigImage_ih;
    }

    pYaIPS_ImageDisp->SubImage_y_max = pYaIPS_ImageDisp->BigImage_ih - pYaIPS_ImageDisp->SubImage_h;

    // Width of image shown in box
    pYaIPS_ImageDisp->BigImage_sw = pYaIPS_ImageDisp->SubImage_w << SizeShift;
    pYaIPS_ImageDisp->BigImage_sh = pYaIPS_ImageDisp->SubImage_h << SizeShift;

    // Try to center image on resolution change

    if( ResolutionChanged) {

      ImageCenter_X_New = ImageCenter_X_Before;
      ImageCenter_Y_New = ImageCenter_Y_Before;

      ImageCenter_X_New -= Delta_Mouse_X * (SizeFactor_Before / SizeFactor_New);
      ImageCenter_Y_New -= Delta_Mouse_Y * (SizeFactor_Before / SizeFactor_New);

      pYaIPS_ImageDisp->SubImage_x = (ImageCenter_X_New - pYaIPS_ImageDisp->SubImage_w * 0.5);
      pYaIPS_ImageDisp->SubImage_y = (ImageCenter_Y_New - pYaIPS_ImageDisp->SubImage_h * 0.5);
    }

    // Clip image sources

    if( pYaIPS_ImageDisp->SubImage_x > pYaIPS_ImageDisp->SubImage_x_max) {    // Clip image source start
      pYaIPS_ImageDisp->SubImage_x = pYaIPS_ImageDisp->SubImage_x_max;
    }
    if( pYaIPS_ImageDisp->SubImage_x < 0) {
      pYaIPS_ImageDisp->SubImage_x = 0;
    }

    if( pYaIPS_ImageDisp->SubImage_y > pYaIPS_ImageDisp->SubImage_y_max) {    // Clip image source start
      pYaIPS_ImageDisp->SubImage_y = pYaIPS_ImageDisp->SubImage_y_max;
    }
    if( pYaIPS_ImageDisp->SubImage_y < 0) {
      pYaIPS_ImageDisp->SubImage_y = 0;
    }

  } else {    // Must be YAIPS_DISP_RESOLUTION_AUTO

    // Reset some variables used for 1:1 or enlarged resolution

    pYaIPS_ImageDisp->SubImage_x = 0;
    pYaIPS_ImageDisp->SubImage_y = 0;
    pYaIPS_ImageDisp->SubImage_w = pYaIPS_ImageDisp->BigImage_iw;
    pYaIPS_ImageDisp->SubImage_h = pYaIPS_ImageDisp->BigImage_ih;
    pYaIPS_ImageDisp->SubImage_x_max = 0;
    pYaIPS_ImageDisp->SubImage_y_max = 0;

    // Auto calculations

    if( pYaIPS_ImageDisp->BigImage_iw > lw ||
        pYaIPS_ImageDisp->BigImage_ih > lh) {

      pYaIPS_ImageDisp->BigImage_sw = lw;
      pYaIPS_ImageDisp->BigImage_sh = pYaIPS_ImageDisp->BigImage_sw * pYaIPS_ImageDisp->BigImage_ih / pYaIPS_ImageDisp->BigImage_iw;

      if( pYaIPS_ImageDisp->BigImage_sh > lh) {

        pYaIPS_ImageDisp->BigImage_sh = lh;
        pYaIPS_ImageDisp->BigImage_sw = pYaIPS_ImageDisp->BigImage_sh * pYaIPS_ImageDisp->BigImage_iw / pYaIPS_ImageDisp->BigImage_ih;
      }
    } else {

      pYaIPS_ImageDisp->BigImage_sw = pYaIPS_ImageDisp->BigImage_iw;
      pYaIPS_ImageDisp->BigImage_sh = pYaIPS_ImageDisp->BigImage_ih;
    }

    // Size factor = pixel on screen / image pixel = one image pixel is how many screen pixels.
    if( pYaIPS_ImageDisp->BigImage_iw > pYaIPS_ImageDisp->BigImage_ih) {

      SizeFactor_New =  (float)pYaIPS_ImageDisp->BigImage_sw / (float)pYaIPS_ImageDisp->BigImage_iw;
    } else {

      SizeFactor_New =  (float)pYaIPS_ImageDisp->BigImage_sh / (float)pYaIPS_ImageDisp->BigImage_ih;
    }
  }

  pYaIPS_ImageDisp->DisplayResolution_Last = pYaIPS_ImageDisp->DisplayResolution;     // Copy for resolution change check
  if( pYaIPS_ImageDisp->Plot3D_Active) {                                             // 3D plot active
    pYaIPS_ImageDisp->DisplayResolution_Last |= 0x0100;                              // or in this bit
  }

  pYaIPS_ImageDisp->PixelImageToScreen = SizeFactor_New;

  pYaIPS_ImageDisp->BigImage_sx = pYaIPS_ImageDisp->BigImage_bx + (pYaIPS_ImageDisp->BigImage_bw - pYaIPS_ImageDisp->BigImage_sw) / 2;      // left upper corner on display
  pYaIPS_ImageDisp->BigImage_sy = pYaIPS_ImageDisp->BigImage_by + (pYaIPS_ImageDisp->BigImage_bh - pYaIPS_ImageDisp->BigImage_sh) / 2;

  pYaIPS_ImageDisp->BigImage_Calc_OK = true;          // Size calculations are OK

  return( pYaIPS_ImageDisp->BigImage_Calc_OK);
}

/************************************************************************************
 * YaIPS_ImageDispDrawUpdate
 *
 * Update display image
 *
 * ForceUpdate: false: only check for size change
 *               true: always update the display image
 */

// extern declaration
void YaIPS_ImageDispPlot3D( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, Fl_RGB_Image *pDrawImage);

void YaIPS_ImageDispDrawUpdate( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, int ForceUpdate)
{
  Fl_RGB_Image *oldimage;                   // Old image
  int ierr, SizeInBytes, DoDispModify, DoLUTmodify, Src_d, Dst_d;
  int Src_w, Dst_w, Src_ld, Dst_ld, DisplayColModStyle;
  uchar *pDataDst, *pDataSrc, r, g, b;

  ierr = YaIPS_ImageDispCalcSizes( pYaIPS_ImageDisp); // Check sizes

  if( ierr != true) {                      // Something not OK

    goto ExitPoint;                        // Return to caller
  }

  if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_FORCE_UPDATE) != 0) {  // Have a forced update set ?

    ForceUpdate = true;

    pYaIPS_ImageDisp->Flags &= ~YAIPS_IDISP_FLAG_FORCE_UPDATE;           // Reset this bit
  }

  // Get pointer to source data

  // Need a table of pointers to image data.
  // See use of 'pNewimage->data()[ 0]'.
  // Check count() to have minimum one pointer

  if( pYaIPS_ImageDisp->pImage_Img->count() < 1) {

    goto ExitPoint;
  }

  //
  // Check display image for proper size
  //

  Src_d = pYaIPS_ImageDisp->pImage_Img->d();

  if( Src_d >= 3) {               // Have a color image

    Dst_d = Src_d;                // Box image has same number of bytes

  } else {                        // Have a BW image

    Dst_d = Src_d + 2;            // Box image should be an color image
  }

  // Remove image displayed in box
  oldimage = (Fl_RGB_Image *)pYaIPS_ImageDisp->pImage_Box->image();

  SizeInBytes = pYaIPS_ImageDisp->BigImage_sw * pYaIPS_ImageDisp->BigImage_sh * Dst_d;

  if( oldimage == NULL ||                          // Have NO image
      oldimage->data_w() != pYaIPS_ImageDisp->BigImage_sw ||          // Something different
      oldimage->data_h() != pYaIPS_ImageDisp->BigImage_sh ||
      oldimage->d() != Dst_d ||
      oldimage->alloc_array == 0) {                // or no data allocated

    if( oldimage) {                                // Have an image

      oldimage->release();                         // Release date
      oldimage = NULL;
    }

    // Allocate memory for image

    pDataDst = new uchar[ SizeInBytes];

    if( pDataDst == NULL) {                        // Security test

      goto ExitPoint;
    }

    // Create image envelope
    oldimage = new Fl_RGB_Image( pDataDst, pYaIPS_ImageDisp->BigImage_sw, pYaIPS_ImageDisp->BigImage_sh, Dst_d);

    if( oldimage == NULL) {                       // Security test
      return;
    }

    oldimage->alloc_array = 1;                    // Flag, data is allocated

    pYaIPS_ImageDisp->pImage_Box->image((Fl_Image *)oldimage);  // Set the image to the display

  } else {                                        // Sizes are not changed

    if( ! ForceUpdate) {                          // NO forced update

      return;
    }

    oldimage->uncache();                          // Uncache all chached data

    // Get pointer to allocated data

    pDataDst = (uchar *)oldimage->array;
  }

  // Check for no image display modifications

  DoDispModify = (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_DO_DISP_MODIFY) != 0;

  // Recalculate LUT if an update is needed

  YaIPS_ColModCheckLUT( pYaIPS_ImageDisp, ForceUpdate);

  // Check for use of LUT

  DoLUTmodify  = (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_LUT_ACTIVE) != 0;

  // Try 3D plot

  if( DoDispModify && pYaIPS_ImageDisp->Plot3D_Active) {

    // Draw as 3D plot
    YaIPS_ImageDispPlot3D( pYaIPS_ImageDisp, oldimage);

    goto ExitPoint;
  }

  // Histograms, column or row sums, ... must be updated

  pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA; // Set AOI changed flag bit

  //
  // Copy image data
  //

  Src_w  = pYaIPS_ImageDisp->pImage_Img->data_w();
  Src_ld = pYaIPS_ImageDisp->pImage_Img->ld() ?  pYaIPS_ImageDisp->pImage_Img->ld() :  Src_w * Src_d;

  Dst_w  = oldimage->data_w();
  Dst_ld = oldimage->ld() ?  oldimage->ld() :  oldimage->data_w() * Dst_d;

  pDataSrc = (uchar *)pYaIPS_ImageDisp->pImage_Img->data()[ 0];

  // Check alpha usage
  DisplayColModStyle = pYaIPS_ImageDisp->DisplayColMod.Style;

  if( DisplayColModStyle == YAIPS_DISP_COLMOD_A &&    // use alpha
      pYaIPS_ImageDisp->pImage_Img->d() != 2 &&       // and no alpha component
      pYaIPS_ImageDisp->pImage_Img->d() != 4) {

    DisplayColModStyle = YAIPS_DISP_COLMOD_NORMAL;    // Use normal channel
  }

  // Optimize the simple copy where the width and height are the same,
  // or when we are copying an empty image...
  if( ((! DoDispModify) ||                                                  // No display modification
        (( pYaIPS_ImageDisp->DisplayResolution == YAIPS_DISP_RESOLUTION_AUTO ||   // Auto resolution or
           pYaIPS_ImageDisp->DisplayResolution == YAIPS_DISP_RESOLUTION_1_1) &&   // 1:1 resolution
          (! DoLUTmodify) &&                                                   // and need no LUT
          (Src_w < 3 || DisplayColModStyle == YAIPS_DISP_COLMOD_NORMAL))) && // an BW image or color image with color display
        pYaIPS_ImageDisp->BigImage_sw == pYaIPS_ImageDisp->BigImage_iw &&      // and image fits into display box
        pYaIPS_ImageDisp->BigImage_sh == pYaIPS_ImageDisp->BigImage_ih) {

    if( Src_d == Dst_d &&                // Have a color image
        ( Dst_d <= 3 ||                  // and no alpha output
          pYaIPS_ImageDisp->NoAlphaDisplay == false)) {  // or no skip alpha display

      // Copy color image data
      memcpy( pDataDst, pDataSrc, SizeInBytes);

    } else {                                       // Have a BW image

      int x, y, xx, yy;
      uchar *pSrcLine2, *pDstLine2;

      xx = oldimage->data_w();
      yy = oldimage->data_h();

      for( y = 0; y < yy; y++) {

        pSrcLine2 = pDataSrc;
        pDstLine2 = pDataDst;

        if( DisplayColModStyle == YAIPS_DISP_COLMOD_A) {        // Show alpha channel and image has an alpha channel

          for( x = 0; x < xx; x++) {

             // Copy Alpha to color
             *pDstLine2++ = pSrcLine2[ 1];
             *pDstLine2++ = pSrcLine2[ 1];
             *pDstLine2++ = pSrcLine2[ 1];

             *pDstLine2++ = 0xff;              // Alpha full brightness

             pSrcLine2 += Src_d;
          }

        } else if( Src_d >= 3) {               // Show color image

          for( x = 0; x < xx; x++) {

            // Copy color to color
            *pDstLine2++ = pSrcLine2[ 0];
            *pDstLine2++ = pSrcLine2[ 1];
            *pDstLine2++ = pSrcLine2[ 2];

            if( Src_d == 4) {      // Image has an alpha part

              if( pYaIPS_ImageDisp->NoAlphaDisplay) {

                *pDstLine2++ = 0xff;              // Alpha full brightness

                if( pYaIPS_ImageDisp->NoAlphaDisplay == 2 &&          // Hack to visualize contour
                    (pSrcLine2[ 3] & YAIPS_REFMASK_CONTOUR) != 0) {

                  pDstLine2[ -4] = 0;
                  pDstLine2[ -3] = 255;
                  pDstLine2[ -2] = 255;
                }

              } else {

                *pDstLine2++ = pSrcLine2[ 3];
              }
            }

            pSrcLine2 += Src_d;
          }

        } else {                               // Show black/white image

          for( x = 0; x < xx; x++) {

            // Copy BW to color
            *pDstLine2++ = pSrcLine2[ 0];
            *pDstLine2++ = pSrcLine2[ 0];
            *pDstLine2++ = pSrcLine2[ 0];

            if( Src_d == 2) {      // Image has an alpha part

              if( pYaIPS_ImageDisp->NoAlphaDisplay) {

                *pDstLine2++ = 0xff;              // Alpha full brightness

                if( pYaIPS_ImageDisp->NoAlphaDisplay == 2 &&          // Hack to visualize contour
                    (pSrcLine2[ 3] & YAIPS_REFMASK_CONTOUR) != 0) {

                  pDstLine2[ -4] = 0;
                  pDstLine2[ -3] = 255;
                  pDstLine2[ -2] = 255;
                }

              } else {

                *pDstLine2++ = pSrcLine2[ 1];
              }
            }

            pSrcLine2 += Src_d;
          }
        }

        pDataSrc += Src_ld;
        pDataDst += Dst_ld;
      }
    }

  } else {

    if( pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_1_1) {  // 1:1 or enlarged

      int SizeShift, SizeFactor, x, y, BigImage_sw, BigImage_sh;
      int SubImage_x_int, SubImage_y_int;
      int Small_h, Small_w, x2, y2;
      uchar *pSrcLine, *pDstLine, *pSrcLine2, *pDstLine2;
      uchar *pLookupR;                   // Point to red lookup table ( 256 entries)
      uchar *pLookupG;                   // Point to green lookup table ( 256 entries)
      uchar *pLookupB;                   // Point to blue lookup table ( 256 entries)

      // Prepare lookup table for color invert or false color

      pLookupR = pYaIPS_ImageDisp->DisplayColMod.LookupR;
      pLookupG = pYaIPS_ImageDisp->DisplayColMod.LookupG;
      pLookupB = pYaIPS_ImageDisp->DisplayColMod.LookupB;

      // ...

      BigImage_sw = pYaIPS_ImageDisp->BigImage_sw;
      BigImage_sh = pYaIPS_ImageDisp->BigImage_sh;

      SizeShift  = pYaIPS_ImageDisp->DisplayResolution - 1;       // Shift is enlargement by power of 2
      SizeFactor = 1 << SizeShift;                               // Enlargement factor

      SubImage_x_int = (int)(pYaIPS_ImageDisp->SubImage_x + 0.5);
      SubImage_y_int = (int)(pYaIPS_ImageDisp->SubImage_y + 0.5);

      pSrcLine = pDataSrc + SubImage_y_int * Src_ld + SubImage_x_int * Src_d;
      pDstLine = pDataDst;

      Small_h = BigImage_sh >> SizeShift;
      Small_w = BigImage_sw >> SizeShift;

      for (y = 0; y < Small_h; y++) {

        // Build first line in destination

        pSrcLine2 = pSrcLine;
        pDstLine2 = pDstLine;

        for (x = 0; x < Small_w; x++) {

          if (Src_d == Dst_d) {               // Have a color image

            r = pSrcLine2[ 0];
            g = pSrcLine2[ 1];
            b = pSrcLine2[ 2];

            switch( DisplayColModStyle) {

            case YAIPS_DISP_COLMOD_NORMAL:     // Normal RGB image
            default:

              r = pLookupR[ r];
              g = pLookupG[ g];
              b = pLookupB[ b];

              break;

            case YAIPS_DISP_COLMOD_BW:         // BW image

              // A fast black white conversion. Intensity part of an IHS conversion.
              b = (r * 76 + g * 150 + b * 30) >> 8;

              r = pLookupR[ b];
              g = pLookupG[ b];
              b = pLookupB[ b];

              break;

            case YAIPS_DISP_COLMOD_R:          // Red component

              g = pLookupG[ r];
              b = pLookupB[ r];
              r = pLookupR[ r];

              break;

            case YAIPS_DISP_COLMOD_G:          // Green component

              r = pLookupR[ g];
              b = pLookupB[ g];
              g = pLookupG[ g];

              break;

            case YAIPS_DISP_COLMOD_B:          // Blue component

              r = pLookupR[ b];
              g = pLookupG[ b];
              b = pLookupB[ b];

              break;

            case YAIPS_DISP_COLMOD_A:          // Alpha component

              r = pLookupR[ pSrcLine2[ 3]];
              g = pLookupG[ pSrcLine2[ 3]];
              b = pLookupB[ pSrcLine2[ 3]];

              break;

            } // end switch

            for (x2 = 0; x2 < SizeFactor; x2++) {

              *pDstLine2++ = r;
              *pDstLine2++ = g;
              *pDstLine2++ = b;

              if (Src_d == 4) {      // Color with alpha

                if( DisplayColModStyle == YAIPS_DISP_COLMOD_A ||  // Show alpha channel and image has an alpha channel
                    pYaIPS_ImageDisp->NoAlphaDisplay) {           // or no alpha display

                  *pDstLine2++ = 0xff;              // Alpha full brightness


                  if( pYaIPS_ImageDisp->NoAlphaDisplay == 2 &&          // Hack to visualize contour
                      (pSrcLine2[ 3] & YAIPS_REFMASK_CONTOUR) != 0) {

                    pDstLine2[ -4] = 0;
                    pDstLine2[ -3] = 255;
                    pDstLine2[ -2] = 255;
                  }

                } else {

                  *pDstLine2++ = pSrcLine2[3];
                }
              }
            }

            pSrcLine2 += Src_d;

          } else {                                       // Have a BW image

            // Copy BW to color

            if( DisplayColModStyle == YAIPS_DISP_COLMOD_A) {  // Show alpha channel and image has an alpha channel

              for (x2 = 0; x2 < SizeFactor; x2++) {

                 // Copy Alpha to color
                 r = pSrcLine2[1];
                 *pDstLine2++ = pLookupR[r];
                 *pDstLine2++ = pLookupG[r];
                 *pDstLine2++ = pLookupB[r];

                 *pDstLine2++ = 0xff;              // Alpha full brightness
              }

            } else {

              for (x2 = 0; x2 < SizeFactor; x2++) {

                r = pSrcLine2[0];
                *pDstLine2++ = pLookupR[r];
                *pDstLine2++ = pLookupG[r];
                *pDstLine2++ = pLookupB[r];

                if (Src_d == 2) {      // BW with alpha

                  if( pYaIPS_ImageDisp->NoAlphaDisplay) {

                    *pDstLine2++ = 0xff;              // Alpha full brightness

                    if( pYaIPS_ImageDisp->NoAlphaDisplay == 2 &&          // Hack to visualize contour
                        (pSrcLine2[ 3] & YAIPS_REFMASK_CONTOUR) != 0) {

                      pDstLine2[ -4] = 0;
                      pDstLine2[ -3] = 255;
                      pDstLine2[ -2] = 255;
                    }

                  } else {

                    *pDstLine2++ = pSrcLine2[1];
                  }
                }
              }
            }

            pSrcLine2 += Src_d;
          }
        }

        // Copy first line

        pSrcLine2 = pDstLine;

        pDstLine2 = pDstLine;
        pDstLine += Dst_ld;

        for (y2 = 1; y2 < SizeFactor; y2++) {

          memcpy(pDstLine, pDstLine2, Dst_w * Dst_d);

          pDstLine += Dst_ld;
        }

        pSrcLine += Src_ld;
      }


    } else {    // Must be YAIPS_DISP_RESOLUTION_AUTO

      // OK, need to resize the image data
      uchar         *new_ptr;       // Pointer into new array
      int           dx, dy;         // Destination coordinates


#ifdef use_again

      // 13.02.2025 RR: Nearest neighbor scaling tested. It works.
      // Nearest neighbor scaling (FL_RGB_SCALING_NEAREST)

      const uchar   *old_ptr;       // Pointer into old array
      int         c,              // Channel number
                  sy,             // Source coordinate
                  xerr, yerr,     // X & Y errors
                  xmod, ymod,     // X & Y moduli
                  xstep, ystep;   // X & Y step increments

      // Figure out Bresenham step/modulus values...
      xmod   = pYaIPS_ImageDisp->BigImage_iw % pYaIPS_ImageDisp->BigImage_sw;
      xstep  = (pYaIPS_ImageDisp->BigImage_iw / pYaIPS_ImageDisp->BigImage_sw) * Src_d;
      ymod   = pYaIPS_ImageDisp->BigImage_ih % pYaIPS_ImageDisp->BigImage_sh;
      ystep  = pYaIPS_ImageDisp->BigImage_ih / pYaIPS_ImageDisp->BigImage_sh;

      // Scale the image using a nearest-neighbor algorithm...
      for (dy = pYaIPS_ImageDisp->BigImage_sh, sy = 0, yerr = pYaIPS_ImageDisp->BigImage_sh, new_ptr = pDataDst; dy > 0; dy --) {
        for (dx = pYaIPS_ImageDisp->BigImage_sw, xerr = pYaIPS_ImageDisp->BigImage_sw, old_ptr = pDataSrc + sy * Src_ld; dx > 0; dx --) {

          for( c = 0; c < d; c ++) *new_ptr++ = old_ptr[c];

          old_ptr += xstep;
          xerr    -= xmod;

          if (xerr <= 0) {
            xerr    += pYaIPS_ImageDisp->BigImage_sw;
            old_ptr += d;
          }
        }

        sy   += ystep;
        yerr -= ymod;
        if (yerr <= 0) {
          yerr += pYaIPS_ImageDisp->BigImage_sh;
          sy ++;
        }
      }

#else

      // Bilinear scaling (FL_RGB_SCALING_BILINEAR)

      uchar *pLookupR;                   // Point to red lookup table ( 256 entries)
      uchar *pLookupG;                   // Point to green lookup table ( 256 entries)
      uchar *pLookupB;                   // Point to blue lookup table ( 256 entries)

      // Prepare lookup table for color invert or false color

      pLookupR = pYaIPS_ImageDisp->DisplayColMod.LookupR;
      pLookupG = pYaIPS_ImageDisp->DisplayColMod.LookupG;
      pLookupB = pYaIPS_ImageDisp->DisplayColMod.LookupB;

      // ...

      const float xscale = (pYaIPS_ImageDisp->BigImage_iw - 1) / (float) pYaIPS_ImageDisp->BigImage_sw;
      const float yscale = (pYaIPS_ImageDisp->BigImage_ih - 1) / (float) pYaIPS_ImageDisp->BigImage_sh;

      for( dy = 0; dy < pYaIPS_ImageDisp->BigImage_sh; dy++) {

        float oldy = dy * yscale;
        if (oldy >= pYaIPS_ImageDisp->BigImage_ih)
          oldy = float( pYaIPS_ImageDisp->BigImage_ih - 1);
        const float yfract = oldy - (unsigned) oldy;

        for( dx = 0; dx < pYaIPS_ImageDisp->BigImage_sw; dx++) {

          new_ptr = pDataDst + (dy * pYaIPS_ImageDisp->BigImage_sw + dx) * Dst_d;

          float oldx = dx * xscale;
          if (oldx >= pYaIPS_ImageDisp->BigImage_iw)
            oldx = float( pYaIPS_ImageDisp->BigImage_iw - 1);
          const float xfract = oldx - (unsigned) oldx;

          const unsigned leftx = (unsigned)oldx;
          const unsigned lefty = (unsigned)oldy;
          const unsigned rightx = (unsigned)(oldx + 1 >= pYaIPS_ImageDisp->BigImage_iw ? oldx : oldx + 1);
          const unsigned righty = (unsigned)oldy;
          const unsigned dleftx = (unsigned)oldx;
          const unsigned dlefty = (unsigned)(oldy + 1 >= pYaIPS_ImageDisp->BigImage_ih ? oldy : oldy + 1);
          const unsigned drightx = (unsigned)rightx;
          const unsigned drighty = (unsigned)dlefty;

          uchar left[4], right[4], downleft[4], downright[4];
          memcpy(     left, pDataSrc + lefty   * Src_ld + leftx   * Src_d, Src_d);
          memcpy(    right, pDataSrc + righty  * Src_ld + rightx  * Src_d, Src_d);
          memcpy( downleft, pDataSrc + dlefty  * Src_ld + dleftx  * Src_d, Src_d);
          memcpy(downright, pDataSrc + drighty * Src_ld + drightx * Src_d, Src_d);

          const float leftf = 1 - xfract;
          const float rightf = xfract;
          const float upf = 1 - yfract;
          const float downf = yfract;

          if( Src_d == Dst_d) {               // Have a color image

            r = (uchar)((left[0] * leftf + right[0] * rightf) * upf + (downleft[0] * leftf + downright[0] * rightf) * downf);
            g = (uchar)((left[1] * leftf + right[1] * rightf) * upf + (downleft[1] * leftf + downright[1] * rightf) * downf);
            b = (uchar)((left[2] * leftf + right[2] * rightf) * upf + (downleft[2] * leftf + downright[2] * rightf) * downf);

            switch( DisplayColModStyle) {

            case YAIPS_DISP_COLMOD_NORMAL:     // Normal RGB image
            default:

              r = pLookupR[ r];
              g = pLookupG[ g];
              b = pLookupB[ b];

              break;

            case YAIPS_DISP_COLMOD_BW:         // BW image

              // A fast black white conversion. Intensity part of an IHS conversion.
              b = (r * 76 + g * 150 + b * 30) >> 8;

              r = pLookupR[ b];
              g = pLookupG[ b];
              b = pLookupB[ b];

              break;

            case YAIPS_DISP_COLMOD_R:          // Red component

              g = pLookupG[ r];
              b = pLookupB[ r];
              r = pLookupR[ r];

              break;

            case YAIPS_DISP_COLMOD_G:          // Green component

              r = pLookupR[ g];
              b = pLookupB[ g];
              g = pLookupG[ g];

              break;

            case YAIPS_DISP_COLMOD_B:          // Blue component

              r = pLookupR[ b];
              g = pLookupG[ b];
              b = pLookupB[ b];

              break;

            case YAIPS_DISP_COLMOD_A:          // Alpha component

              r = (uchar)((left[3] * leftf + right[3] * rightf) * upf + (downleft[3] * leftf + downright[3] * rightf) * downf);

              r = pLookupR[ r];
              g = pLookupG[ r];
              b = pLookupB[ r];

              break;

            } // end switch

            new_ptr[ 0] = r;
            new_ptr[ 1] = g;
            new_ptr[ 2] = b;

            if (Src_d == 4) {      // Color with alpha

              if( DisplayColModStyle == YAIPS_DISP_COLMOD_A ||  // Show alpha channel and image has an alpha channel
                  pYaIPS_ImageDisp->NoAlphaDisplay) {           // or no alpha display

                new_ptr[ 3] = 0xff;              // Alpha full brightness

                if( pYaIPS_ImageDisp->NoAlphaDisplay == 2 &&          // Hack to visualize contour
                    (left[ 3] & YAIPS_REFMASK_CONTOUR) != 0) {

                  new_ptr[ 0] = 0;
                  new_ptr[ 1] = 255;
                  new_ptr[ 2] = 255;
                }

              } else {

                new_ptr[ 3] = (uchar)((left[3] * leftf + right[3] * rightf) * upf + (downleft[3] * leftf + downright[3] * rightf) * downf);
              }
            }

          } else {                   // Have a BW image

            if( DisplayColModStyle == YAIPS_DISP_COLMOD_A ||  // Show alpha channel and image has an alpha channel
                pYaIPS_ImageDisp->NoAlphaDisplay) {           // or no alpha display

              r = (uchar)((left[1] * leftf + right[1] * rightf) * upf + (downleft[1] * leftf + downright[1] * rightf) * downf);

              new_ptr[ 0] = pLookupR[ r];
              new_ptr[ 1] = pLookupG[ r];
              new_ptr[ 2] = pLookupB[ r];

              new_ptr[ 3] = 0xff;              // Alpha full brightness

              if( pYaIPS_ImageDisp->NoAlphaDisplay == 2) {          // Hack to visualize contour

                if( Src_d == 2) {

                  if( (left[ 1] & YAIPS_REFMASK_CONTOUR) != 0) {
                    new_ptr[ 0] = 0;
                    new_ptr[ 1] = 255;
                    new_ptr[ 2] = 255;
                  }

                } else {

                  if( (left[ 3] & YAIPS_REFMASK_CONTOUR) != 0) {
                    new_ptr[ 0] = 0;
                    new_ptr[ 1] = 255;
                    new_ptr[ 2] = 255;
                  }
                }
              }

            } else {

              r = (uchar)((left[0] * leftf + right[0] * rightf) * upf + (downleft[0] * leftf + downright[0] * rightf) * downf);

              new_ptr[ 0] = pLookupR[ r];
              new_ptr[ 1] = pLookupG[ r];
              new_ptr[ 2] = pLookupB[ r];

              if( Src_d == 2) {

                new_ptr[3] = (uchar)((left[1] * leftf + right[1] * rightf) * upf + (downleft[1] * leftf + downright[1] * rightf) * downf);
              }
            }

          }
        }
      }
    }
#endif
  }

ExitPoint:

  if( pYaIPS_ImageDisp->pImage_Box != NULL) {      // Security test, have a big image box

    pYaIPS_ImageDisp->pImage_Box->redraw();                    // Redraw after image change
  }

  return;
}

/************************************************************************************
 * YaIPS_ImageDispDrawBefore_cb
 *
 * Additional actions before the image is drawn.
 * NOTE: This hack updates the image in the box during resize of window.
 *
 * pArg: must point to a 'Fl_YaIPS_ImageDisp_t'.
 *
 */
void YaIPS_ImageDispDrawBefore_cb( Fl_Widget *pW, void *pArg)
{

  // Check for size change.
  // Doing these here also shows a correct image during resize of the window.

  YaIPS_ImageDispDrawUpdate( (Fl_YaIPS_ImageDisp_t *)pArg, false);

}

/************************************************************************************
 * YaIPS_ImageDispDrawAfter_Common
 *
 * Additional actions after the image was drawn.
 * Can be called from other DrawAfterCallback functions.
 * NOTE: This hack displays the 'StrDebug' string and maybe other things
 *
 * pYaIPS_ImageDisp: point to image display data
 * DrawFlags:        0 draws all. Else drawing parts must be flagged
 * DoClip:           if true (> 0) handle clipping of draw region
 *                   else this must be done in the calling function
 *
 */
void YaIPS_ImageDispDrawAfter_Common( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // point to image display data
                                      int DrawFlags,                           // 0 draws all. Else drawing parts must be flagged
                                      int DoClip)                              // if true (> 0) handle clipping of draw region else caller must do it
{
  int x1, y1, FontSize, Ident;

  // Start in left upper corner
  x1 = pYaIPS_ImageDisp->BigImage_bx + 8;
  y1 = pYaIPS_ImageDisp->BigImage_by + 8;

  // Draw user info

  if( pYaIPS_ImageDisp->StrInfo[ 0] &&             // Have user info
      DrawFlags == 0) {                            // draw all

    char *pTextBegin, *pTextNewLine;
    int mw, mh;

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( pYaIPS_ImageDisp->BigImage_bx, pYaIPS_ImageDisp->BigImage_by, pYaIPS_ImageDisp->BigImage_bw, pYaIPS_ImageDisp->BigImage_bh);
    }


    if( pYaIPS_ImageDisp->ColInfoTextSize > 0) {     // Have text size set

      FontSize = pYaIPS_ImageDisp->ColInfoTextSize; // Use it
    } else {

      FontSize = 18;                    // Use default
    }

    fl_font( FL_HELVETICA, FontSize);

    Ident = FontSize / 2 + 1;

    if( pYaIPS_ImageDisp->ColInfoText == 0) {   // No text color set until now

      // Set default color
      pYaIPS_ImageDisp->ColInfoText = FL_BLACK;
    }

    pTextBegin = pYaIPS_ImageDisp->StrInfo;

    for( ; ; ) {

      pTextNewLine = strchr( pTextBegin, '\n');     // Check for next end of line

      if( pTextNewLine != NULL) {                   // Have an end of line

        *pTextNewLine = '\0';                       // Overwrite with end of string
      }

      mw = 0;
      mh = 0;
      fl_measure( pTextBegin, mw, mh);              // Measure string

      // Background rectangle

      if( pYaIPS_ImageDisp->ColInfoBgnd != 0xffffffff) {    // Draw background

        if( pYaIPS_ImageDisp->ColInfoBgnd == 0) {           // No color set until now

          // Set default color
          pYaIPS_ImageDisp->ColInfoBgnd = fl_rgb_color( 128, 230, 128);
        }
        fl_color( pYaIPS_ImageDisp->ColInfoBgnd);
        fl_rectf( x1, y1, mw + Ident * 2, mh + Ident);
      }

      // Text

      fl_color( pYaIPS_ImageDisp->ColInfoText);
      fl_draw( pTextBegin, x1 + Ident, y1 + mh);

      if( pTextNewLine != NULL) {                   // Have an end of line

        *pTextNewLine = '\n';                       // Restore end of line

        pTextBegin = pTextNewLine + 1;              // Point to next line

        y1 += mh + Ident + 2;          // Increment for next string output

      } else {

        y1 += mh + Ident + 2;          // Increment for next string output

        break;
      }
    }

    //fl_line_style( 0);   // Reset to default
  }

  // Draw debug info

  if( pYaIPS_ImageDisp->StrDebug[ 0] &&           // Have debug info
      DrawFlags == 0) {                            // draw all

    int mw = 0, mh = 0;

    if( DoClip > 0) {      // The the draw clipping

      DoClip = -1;         // Need to pop clipping

      fl_push_clip( pYaIPS_ImageDisp->BigImage_bx, pYaIPS_ImageDisp->BigImage_by, pYaIPS_ImageDisp->BigImage_bw, pYaIPS_ImageDisp->BigImage_bh);
    }

    fl_font( FL_HELVETICA, 18);
    fl_measure( pYaIPS_ImageDisp->StrDebug, mw, mh);

    // Black rect
    fl_color( FL_BLACK);
    fl_rectf( x1, y1, mw + 10, mh + 10);
    // White text
    fl_color( FL_WHITE);

    fl_draw( pYaIPS_ImageDisp->StrDebug, x1 + 5, y1 + mh);

    y1 += mh + 12;          // Increment for next string output

    //fl_line_style( 0);   // Reset to default
  }

  // Finish up

  if( DoClip < 0) {      // Need to pop clipping

    fl_pop_clip();
  }
}

/************************************************************************************
 * YaIPS_ImageDispDrawAfter_cb
 *
 * Additional actions after the image was drawn.
 * NOTE: This hack displays the 'StrDebug' string and maybe other things
 *
 * pArg: must point to a 'Fl_YaIPS_ImageDisp_t'.
 *
 */
void YaIPS_ImageDispDrawAfter_cb( Fl_Widget *pW,
                                  void *pArg1,        // Pointer to Fl_YaIPS_ImageDisp_t
                                  void *pArg2)        // Optional pointer to ToolData
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;

  pYaIPS_ImageDisp = (Fl_YaIPS_ImageDisp_t *)pArg1;

  // Draw additional common drawings

  YaIPS_ImageDispDrawAfter_Common( pYaIPS_ImageDisp, 0, true);
}

/************************************************************************************
 * YaIPS_ImageDispUpdateByChangedImage
 *
 * An image display has changed.
 *
 */
void YaIPS_ImageDispUpdateByChangedImage( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  int ImageSourceID, char *pFileName)
{

  pYaIPS_ImageDisp->ImageSourceID = ImageSourceID;         // Set new image source
  pYaIPS_ImageDisp->BigImage_Calc_OK = false;              // Reset size calculations

  pYaIPS_ImageDisp->ImageChanged += 1;                     // Count has changed in any case
  pYaIPS_ImageDisp->ImageChanged &= 0x7FFFFFFF;            // Handle wrap around
  if( pYaIPS_ImageDisp->ImageChanged <= 0) {               // NO value of 0 allowed
    pYaIPS_ImageDisp->ImageChanged = 1;
  }
  pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA; // Set AOI changed flag bit

  IqeB_FileGetFileName( pFileName, pYaIPS_ImageDisp->ImageName, sizeof( pYaIPS_ImageDisp->ImageName)); // Get filename without path

  memset( pYaIPS_ImageDisp->FileName, 0, sizeof( pYaIPS_ImageDisp->FileName));
  strncpy( pYaIPS_ImageDisp->FileName, pFileName, sizeof( pYaIPS_ImageDisp->FileName) - 1);

  // Update image display

  pYaIPS_ImageDisp->Flags = pYaIPS_ImageDisp->Flags | YAIPS_IDISP_FLAG_DO_DISP_MODIFY;
}

/************************************************************************************
 * YaIPS_ImageDispUpdateByNewImage
 *
 * Update an image display by a new image.
 *
 */
void YaIPS_ImageDispUpdateByNewImage( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, Fl_RGB_Image *pNewimage,
                                      int ImageSourceID, char *pFileName,
                                      int SetDoDispModify, int SetScaleAuto, int NoAlphaDisplay)
{

  if( pYaIPS_ImageDisp != &YaIPS_BigImageDisp &&           // Is NOT the big image display
      ImageSourceID != YAIPS_WIN_ID_KEEP) {                // and NOT keep source ID
    pYaIPS_ImageDisp->ImageSourceID = YAIPS_WIN_ID_NONE;   // Reset image source
  }
  pYaIPS_ImageDisp->ImageName[ 0]    = '\0';               // Reset image name
  pYaIPS_ImageDisp->FileName[ 0]    = '\0';                // Reset file name
  pYaIPS_ImageDisp->BigImage_Calc_OK = false;              // Reset size calculations

  pYaIPS_ImageDisp->ImageChanged += 1;                     // Count has changed in any case
  pYaIPS_ImageDisp->ImageChanged &= 0x7FFFFFFF;            // Handle wrap around
  if( pYaIPS_ImageDisp->ImageChanged <= 0) {               // NO value of 0 allowed
    pYaIPS_ImageDisp->ImageChanged = 1;
  }
  pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA; // Set AOI changed flag bit

  // Release previous big image
  if( pYaIPS_ImageDisp->pImage_Img != NULL) {      // Have a big image

    pYaIPS_ImageDisp->pImage_Img->release();       // Release the image

    pYaIPS_ImageDisp->pImage_Img = NULL;           // Set pointer to NULL
  }

  if( pYaIPS_ImageDisp->pImage_Box == NULL) {      // Security test, have no big image box

    goto ExitPoint;
  }

  // Need a table of pointers to image data.
  // See use of 'pNewimage->data()[ 0]'.
  // Check count() to have minimum one pointer

  if( pNewimage == NULL ||                  // Security test
      pNewimage->count() < 1) {

    return;
  }

  // ...

  int SizeInBytes;
  uchar *pDataImg;

  SizeInBytes = pNewimage->data_w() * pNewimage->data_h() * pNewimage->d();

  // Allocate memory for image

  pDataImg = new uchar[ SizeInBytes];

  if( pDataImg == NULL) {                        // Security test
    return;
  }

  // Create image envelope
  pYaIPS_ImageDisp->pImage_Img = new Fl_RGB_Image( pDataImg, pNewimage->data_w(), pNewimage->data_h(), pNewimage->d());

  if( pYaIPS_ImageDisp->pImage_Img == NULL) {              // Load failed

    goto ExitPoint;
  }

  pYaIPS_ImageDisp->pImage_Img->alloc_array = 1;           // Flag, data is allocated

  // Copy data

  memcpy( pDataImg, pNewimage->data()[ 0], SizeInBytes);

  if (pYaIPS_ImageDisp->pImage_Img && ( (pYaIPS_ImageDisp->pImage_Img->data_w() <= 0) ||  // Check image for error
                 (pYaIPS_ImageDisp->pImage_Img->data_h() <= 0) ||
                 (pYaIPS_ImageDisp->pImage_Img->d() < 0)  ||
                 (pYaIPS_ImageDisp->pImage_Img->count() <= 0))) {
    pYaIPS_ImageDisp->pImage_Img ->release();

    pYaIPS_ImageDisp->pImage_Img = NULL;           // Set pointer to NULL

    goto ExitPoint;
  }

  if( ImageSourceID != YAIPS_WIN_ID_KEEP) {            // NOT keep source ID

    pYaIPS_ImageDisp->ImageSourceID = ImageSourceID;   // Set new image source
  } else {

    if( pYaIPS_ImageDisp->ImageSourceID <= YAIPS_WIN_ID_NONE) {  // Check for invalid source ID

      pYaIPS_ImageDisp->ImageSourceID = YAIPS_WIN_ID_IS_VALID;   // Set a valid source ID
    }
  }

  IqeB_FileGetFileName( pFileName, pYaIPS_ImageDisp->ImageName, sizeof( pYaIPS_ImageDisp->ImageName)); // Get filename without path

  memset( pYaIPS_ImageDisp->FileName, 0, sizeof( pYaIPS_ImageDisp->FileName));
  strncpy( pYaIPS_ImageDisp->FileName, pFileName, sizeof( pYaIPS_ImageDisp->FileName) - 1);

  // Update image display

  if( SetDoDispModify) {            // Flag display modification on

    pYaIPS_ImageDisp->Flags = pYaIPS_ImageDisp->Flags | YAIPS_IDISP_FLAG_DO_DISP_MODIFY;
  }

  if( SetScaleAuto) {    // Set display resolution to auto scale

    pYaIPS_ImageDisp->DisplayResolution = YAIPS_DISP_RESOLUTION_AUTO;
  }

  pYaIPS_ImageDisp->NoAlphaDisplay = NoAlphaDisplay;    // Set no alpha display just before display

  YaIPS_ImageDispDrawUpdate( pYaIPS_ImageDisp, true);

ExitPoint:

  if( pYaIPS_ImageDisp->pImage_Box != NULL) {      // Security test, have a big image box

    pYaIPS_ImageDisp->pImage_Box->redraw();
  }

  return;
}

/************************************************************************************
 * YaIPS_ImageDispUpdateByFileName
 *
 * Update an image display by an image file.
 *
 * return:   0: OK
 *         < 0: error
 *
 */
int YaIPS_ImageDispUpdateByFileName( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp, char *pFileName,
                                     int ImageSourceID, int SetDoDispModify, int SetScaleAuto, int NoAlphaDisplay)
{
  Fl_RGB_Image *pTempImage;

  // load and show file

  pTempImage = YaIPS_Image_Read( pFileName);   // Try to load an image

  if( pTempImage == NULL) {                                        // No image loaded

    return( -1);
  }

  // Load the image to the display
  YaIPS_ImageDispUpdateByNewImage( pYaIPS_ImageDisp, pTempImage,
                                   ImageSourceID, pFileName, SetDoDispModify, SetScaleAuto, NoAlphaDisplay);

  pTempImage->release();        // Release

  return( 0);                   // Return OK
}

/************************************************************************************
 * YaIPS_ImageDispPasteToOut_cb
 *
 * Common paste image callback. Paste image from clipboard direct to output image.
 */

void YaIPS_ImageDispPasteToOut_cb( void *pYaIPS_ImageDispArg, Fl_RGB_Image *pRGB_Arg)
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  Fl_RGB_Image *pRGB;
  Fl_RGB_Image *pBW_Image = NULL;
  int MyWinID;

  pYaIPS_ImageDisp = (Fl_YaIPS_ImageDisp_t *)pYaIPS_ImageDispArg;   // Convert argument

  pRGB = pRGB_Arg;

  if( pYaIPS_ImageDisp == NULL) {      // Security test

    return;
  }

  // Check for a RGB image which has identical colors
  if( YaIPS_Setting_PasteImgCol2BW && // Support conversion from pseudo BW color images to a BW images
      pRGB->d() >= 3) {               // have an

    uchar *pD, *s8;
    int x, y, xx, yy, d, ld, IsBW, ierr;

    d  = pRGB->d();
    xx = pRGB->data_w();
    yy = pRGB->data_h();
    ld = pRGB->ld() ? pRGB->ld() : xx * d;
    pD = (uchar *)pRGB->data()[ 0];

    IsBW = true;       // Preset black white image

    for( y = 0; y < yy; y++) {

      s8 = pD;
      pD += ld;

      for( x = 0; x < xx; x++, s8 += d) {

        if( s8[ 0] != s8[ 1] || s8[ 0] != s8[ 2]) {  // Color components are different

          IsBW = false;           // Is color image
          break;
        }

        if( ! IsBW) {             // Break loop for color images

          break;
        }
      }
    }

    if( IsBW) {

      ierr = YaIPS_RGB_Color_ConvSimple( &pBW_Image, pRGB, YAIPS_COLOR_RGB_2_R);

      if( ierr == 0 && pBW_Image != NULL) {    // Have a converted image

        pRGB = pBW_Image;
      }
    }
  }

  // Load the image to the display

  MyWinID = pYaIPS_ImageDisp->MyWinID;

  if( pYaIPS_ImageDisp == &YaIPS_BigImageDisp ||   // Is big image display
      MyWinID < YAIPS_WIN_ID_IMG_WIN) {            // or ID is not valid

    MyWinID = YAIPS_WIN_ID_KEEP;
  }

  YaIPS_ImageDispUpdateByNewImage( pYaIPS_ImageDisp, pRGB,
                                   MyWinID, LANGDEF_CLIPBOARD, true, true);

  if( pYaIPS_ImageDisp != &YaIPS_BigImageDisp &&       // Is not the big image
      YaIPS_BigImageDisp.ImageSourceID == MyWinID) {   // and display this on the big image

    YaIPS_ImageDispUpdateByNewImage( &YaIPS_BigImageDisp, pRGB,
                                     MyWinID, LANGDEF_CLIPBOARD);   // Load the image to the display
  }

  if( pBW_Image != NULL) {     // Used a temporary image

    pBW_Image->release();      // Release image data
  }
}

/************************************************************************************
 * YaIPS_ImageDispPasteToOutFunc_cb
 *
 * Common paste image callback. Paste image from clipboard using 'ToolChangeOutput' function.
 */

void YaIPS_ImageDispPasteToOutFunc_cb( void *pYaIPS_ImageDispArg, Fl_RGB_Image *pRGB_Arg)
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  Fl_RGB_Image *pRGB;
  Fl_RGB_Image *pBW_Image = NULL;
  int MyWinID;

  pYaIPS_ImageDisp = (Fl_YaIPS_ImageDisp_t *)pYaIPS_ImageDispArg;   // Convert argument

  pRGB = pRGB_Arg;

  if( pYaIPS_ImageDisp == NULL) {      // Security test

    return;
  }

  // Check for a RGB image which has identical colors
  if( YaIPS_Setting_PasteImgCol2BW && // Support conversion from pseudo BW color images to a BW images
      pRGB->d() >= 3) {               // have an

    uchar *pD, *s8;
    int x, y, xx, yy, d, ld, IsBW, ierr;

    d  = pRGB->d();
    xx = pRGB->data_w();
    yy = pRGB->data_h();
    ld = pRGB->ld() ? pRGB->ld() : xx * d;
    pD = (uchar *)pRGB->data()[ 0];

    IsBW = true;       // Preset black white image

    for( y = 0; y < yy; y++) {

      s8 = pD;
      pD += ld;

      for( x = 0; x < xx; x++, s8 += d) {

        if( s8[ 0] != s8[ 1] || s8[ 0] != s8[ 2]) {  // Color components are different

          IsBW = false;           // Is color image
          break;
        }

        if( ! IsBW) {             // Break loop for color images

          break;
        }
      }
    }

    if( IsBW) {

      ierr = YaIPS_RGB_Color_ConvSimple( &pBW_Image, pRGB, YAIPS_COLOR_RGB_2_I);

      if( ierr == 0 && pBW_Image != NULL) {    // Have a converted image

        pRGB = pBW_Image;
      }
    }
  }

  // Call the tool change output function

  MyWinID = pYaIPS_ImageDisp->MyWinID;

  YaIPS_ToolChangeOutputCall( MyWinID, pRGB);

  if( pBW_Image != NULL) {     // Used a temporary image

    pBW_Image->release();      // Release image data
  }
}

/************************************************************************************
 * YaIPS_ImageDispCopyImage_cb
 *
 * May be called from extern (set pWidget to NULL for this) or may be used
 * a button or shortcut callback.
 *
 * Copy image to clipboard.
 */

void YaIPS_ImageDispCopyImage_cb( Fl_Widget *pWidget, void *pYaIPS_ImageDispArg)
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  Fl_Copy_Surface *copy_surf;
  Fl_RGB_Image *pTempImage = NULL;

  pYaIPS_ImageDisp = (Fl_YaIPS_ImageDisp_t *)pYaIPS_ImageDispArg;   // Convert argument

  if( pYaIPS_ImageDisp == NULL) {                  // Security test

    return;
  }

  if( pYaIPS_ImageDisp->pImage_Img == NULL) {     // No image set

    return;
  }

#ifdef use_again
  // Copy visible part of the image on the display.
  // This shows size and shift effects of the image on the display.
  copy_surf = new Fl_Copy_Surface( pYaIPS_ImageDisp->BigImage_sw, pYaIPS_ImageDisp->BigImage_sh);
  Fl_Surface_Device::push_current( copy_surf);
  copy_surf->draw( pYaIPS_ImageDisp->pImage_Box,
                   pYaIPS_ImageDisp->BigImage_bx - pYaIPS_ImageDisp->BigImage_sx,
                   pYaIPS_ImageDisp->BigImage_by - pYaIPS_ImageDisp->BigImage_sy);
#else

  // HACK: Copy complete image usage a box with an image.
  copy_surf = new Fl_Copy_Surface( pYaIPS_ImageDisp->BigImage_iw, pYaIPS_ImageDisp->BigImage_ih);
  Fl_Surface_Device::push_current( copy_surf);

  Fl_Box *b = new Fl_Box( FL_NO_BOX, 0, 0, pYaIPS_ImageDisp->BigImage_iw, pYaIPS_ImageDisp->BigImage_ih, 0);


  if( pYaIPS_ImageDisp->pImage_Img->d() == 2 ||       // Image has an alpha channel
      pYaIPS_ImageDisp->pImage_Img->d() == 4) {

    // Make copy of image without alpha channel
    YaIPS_RGB_MixChannels( &pTempImage, pYaIPS_ImageDisp->pImage_Img);
  }

  b->image( pTempImage != NULL ? pTempImage : pYaIPS_ImageDisp->pImage_Img);

  copy_surf->draw( b, 0, 0);

  if( pTempImage != NULL) {               // Release temporary image

    pTempImage->release();
  }

  delete b;
#endif

  delete copy_surf;
  Fl_Surface_Device::pop_current();
}

/************************************************************************************
 * YaIPS_ImageDispMouse_CommonEntry
 *
 * Mouse event callback for image displays, common entry work.
 *
 * return:   0 After return to exit work
 *           1 OK, process events
 *         < 0 Error or problem, return to caller
 *
 */

int YaIPS_ImageDispMouse_CommonEntry( Fl_Widget *pW,                           // Widget calling the event
                                      int event,                               // Event code
                                      Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Image display data
                                      int *pMouseX, int *pMouseY,              // Point to unprocessed mouse coordinates
                                      int *pLast_x, int *pLast_y)              // Point to latched mouse position
{

  if( pYaIPS_ImageDisp == NULL) {                      // Security test

    pYaIPS_ImageDisp->Latched_AoiDeltaAdd = 0;         // Reset latched AOI mouse modification

    pYaIPS_ImageDisp->Flags &= YAIPS_IDISP_FLAG_MASK_OUT_OF_WIDGET;  // Out of image

    return( -1);                                       // Problem, return to caller
  }

  pYaIPS_ImageDisp->pParentWindow = pW->window();      // Get parent windows

  if( pYaIPS_ImageDisp->pParentWindow == NULL) {       // Security test

    pYaIPS_ImageDisp->Latched_AoiDeltaAdd = 0;         // Reset latched AOI mouse modification

    pYaIPS_ImageDisp->Flags &= YAIPS_IDISP_FLAG_MASK_OUT_OF_WIDGET;  // Out of image

    return( -1);                                       // Problem, return to caller
  }

  if( ! pYaIPS_ImageDisp->pParentWindow->visible() ||                    // Window not visible
      ! pYaIPS_ImageDisp->pParentWindow->active() ||                     // Window not active
      (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_DO_DISP_MODIFY) == 0 || // or no display modification
      event == FL_LEAVE) {

    pYaIPS_ImageDisp->Latched_AoiDeltaAdd = 0;                              // Reset latched AOI mouse modification

    pYaIPS_ImageDisp->Flags &= YAIPS_IDISP_FLAG_MASK_OUT_OF_WIDGET;  // Out of image

    pYaIPS_ImageDisp->pParentWindow->cursor( FL_CURSOR_DEFAULT);

    return( -1);                                       // Problem, return to caller
  }

  // Preset variables and get mouse position

  pYaIPS_ImageDisp->RedrawOnExit   = false;                     // Set no redraw on exit
  pYaIPS_ImageDisp->CursorShape    = 0;                         // Preset default cursor shape, cross cursor if mouse is in image
  pYaIPS_ImageDisp->AoiDeltaAdd    = 0;                         // No AOI selection
  pYaIPS_ImageDisp->MyWinID        = 0;                         // Set my tool window ID, used for BigImageUpdate Flag
  pYaIPS_ImageDisp->BigImageUpdate = 0;                         // NO update of big image

  *pMouseX = Fl::event_x();
  *pMouseY = Fl::event_y();
  pYaIPS_ImageDisp->mouseleft  = (Fl::event_state() & FL_BUTTON1) != 0;
  pYaIPS_ImageDisp->mouseright = (Fl::event_state() & FL_BUTTON3) != 0;

  // Catch mouse button pressed or released

  if( pYaIPS_ImageDisp->mouseleft || pYaIPS_ImageDisp->mouseright ) {       // Any mouse button pressed

    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_BUTT_ANY) == 0) { // NO mouse button pressed before

      pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_BUTT_ANY;           // Any mouse button pressed

      pYaIPS_ImageDisp->RedrawOnExit = true;                                // Set redraw on exit
    }
  } else {                                                                  // NO mouse button pressed

    pYaIPS_ImageDisp->Latched_AoiDeltaAdd = 0;                              // Reset latched AOI mouse modification

    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_BUTT_ANY) != 0) { // Any mouse button pressed befoer

      pYaIPS_ImageDisp->Flags &= ~YAIPS_IDISP_FLAG_MOUSE_BUTT_ANY;          // No mouse button pressed

      pYaIPS_ImageDisp->RedrawOnExit = true;                                // Set redraw on exit
    }
  }

  if( event == FL_ENTER) {                                                  // Enter widget

    *pLast_x = *pMouseX;
    *pLast_y = *pMouseY;

    pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_IN_WIDGET;            // Mouse is in widget
  }

  // outside inner image

  if( *pMouseX < pYaIPS_ImageDisp->BigImage_sx ||
      *pMouseX >= pYaIPS_ImageDisp->BigImage_sx + pYaIPS_ImageDisp->BigImage_sw ||
      *pMouseY < pYaIPS_ImageDisp->BigImage_sy ||
      *pMouseY >= pYaIPS_ImageDisp->BigImage_sy + pYaIPS_ImageDisp->BigImage_sh) {

    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_IN_IMAGE) != 0) { // Mouse in image before

      pYaIPS_ImageDisp->Flags &= ~YAIPS_IDISP_FLAG_MOUSE_IN_IMAGE; // Mouse not in image

      pYaIPS_ImageDisp->RedrawOnExit = true;                       // Set redraw on exit
    }

    return( 0);             // After return to exit work
  }

  // Mouse delta

  pYaIPS_ImageDisp->Delta_x = *pMouseX - *pLast_x;
  pYaIPS_ImageDisp->Delta_y = *pMouseY - *pLast_y;

  *pLast_x = *pMouseX;
  *pLast_y = *pMouseY;

  pYaIPS_ImageDisp->MouseX = *pMouseX - pYaIPS_ImageDisp->BigImage_sx;
  pYaIPS_ImageDisp->MouseY = *pMouseY - pYaIPS_ImageDisp->BigImage_sy;

  if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_IN_IMAGE) == 0) { // Mouse not in image before

    pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_IN_IMAGE;    // Mouse is in image

    pYaIPS_ImageDisp->RedrawOnExit = true;                         // Set redraw on exit
  }

  return( 1);             // OK, process events
}

/************************************************************************************
 * YaIPS_ImageDispMouse_CommonMouseWheel
 *
 * Mouse event callback for image displays.
 * Manage display resolution change by mouse wheel event.
 *
 */

void YaIPS_ImageDispMouse_CommonMouseWheel( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Image display data
                                            int x, int y)                            // unprocessed mouse coordinates
{
  int Delta_MouseWheel;

  Delta_MouseWheel = Fl::event_dy();                             // Get direction of wheel

  if( Delta_MouseWheel > 0) {

    if( pYaIPS_ImageDisp->DisplayResolution > YAIPS_DISP_RESOLUTION_AUTO) {  // Not at lower limit

      pYaIPS_ImageDisp->DisplayResolution -= 1;                  // Decrease resolution
    }

    YaIPS_ImageDispCalcSizes( pYaIPS_ImageDisp, YAIPS_IDISP_FLAG_FORCE_UPDATE, x, y);  // Check sizes

    pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;   // Set AOI changed flag bit

  } else if( Delta_MouseWheel < 0) {

    if( pYaIPS_ImageDisp->DisplayResolution < YAIPS_DISP_RESOLUTION_MAX) {   // Not at upper limit

      pYaIPS_ImageDisp->DisplayResolution += 1;                  // Increase resolution
    }

    YaIPS_ImageDispCalcSizes( pYaIPS_ImageDisp, YAIPS_IDISP_FLAG_FORCE_UPDATE, x, y);  // Check sizes

    pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA;   // Set AOI changed flag bit
  }

}

/************************************************************************************
 * YaIPS_ImageDispMouse_CommonMouseWheel
 *
 * Mouse event callback for image displays.
 * Manage Left/right mouse button pressed on image background.
 *
 */

void YaIPS_ImageDispMouse_CommonMouseBackGnd( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Image display data
                                              int *pLast_x, int *pLast_y)              // Point to latched mouse position
{
  float DScale_x, DScale_y;

  if( pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_1_1 &&    // No auto resolution
      pYaIPS_ImageDisp->mouseleft &&                                         // and left button pressed
      (pYaIPS_ImageDisp->Delta_x != 0 || pYaIPS_ImageDisp->Delta_y != 0)) {  // and mouse has moved

    // Move the image will change the image covered by the AOI

    pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA; // Set AOI changed flag bit

    // Move delta depends on scale

    DScale_x = pYaIPS_ImageDisp->Delta_x / pYaIPS_ImageDisp->PixelImageToScreen;
    DScale_y = pYaIPS_ImageDisp->Delta_y / pYaIPS_ImageDisp->PixelImageToScreen;

    // Move image

    if (pYaIPS_ImageDisp->Plot3D_Active) {   // 3D plot active

      float angle, sa, ca;

      // Plot3D_Azimuth has the range -180 .. 180
      angle = ((pYaIPS_ImageDisp->Plot3D_Azimuth) * M_PI) / 180.0;

      ca = cos(angle);
      sa = sin(angle);

      pYaIPS_ImageDisp->SubImage_x -= (DScale_x * ca + DScale_y * sa);
      pYaIPS_ImageDisp->SubImage_y -= (DScale_y * ca - DScale_x * sa);

    } else {

      pYaIPS_ImageDisp->SubImage_x -= DScale_x;
      pYaIPS_ImageDisp->SubImage_y -= DScale_y;
    }

    if( pYaIPS_ImageDisp->SubImage_x > pYaIPS_ImageDisp->SubImage_x_max) {    // Clip image source start
      pYaIPS_ImageDisp->SubImage_x = pYaIPS_ImageDisp->SubImage_x_max;
    }
    if( pYaIPS_ImageDisp->SubImage_x < 0) {
      pYaIPS_ImageDisp->SubImage_x = 0;
    }

    if( pYaIPS_ImageDisp->SubImage_y > pYaIPS_ImageDisp->SubImage_y_max) {    // Clip image source start
      pYaIPS_ImageDisp->SubImage_y = pYaIPS_ImageDisp->SubImage_y_max;
    }
    if( pYaIPS_ImageDisp->SubImage_y < 0) {
      pYaIPS_ImageDisp->SubImage_y = 0;
    }

    YaIPS_ImageDispCalcSizes( pYaIPS_ImageDisp, YAIPS_IDISP_FLAG_FORCE_UPDATE);  // Update screen

    return;   // Done
  }

  // Right mouse button pressed for 3D plot active

  if( pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_AUTO &&  // Any resolution
      pYaIPS_ImageDisp->mouseright &&                                       // right left button pressed
      pYaIPS_ImageDisp->Plot3D_Active &&                                    // 3D plot active
      (pYaIPS_ImageDisp->Delta_x != 0 || pYaIPS_ImageDisp->Delta_y != 0)) { // and mouse has moved

    int Small_x, Small_y, Remain_x, Remain_y;

    Small_x = (pYaIPS_ImageDisp->Delta_x + 3) / 6;
    Remain_x = pYaIPS_ImageDisp->Delta_x - Small_x * 6;

    Small_y = (pYaIPS_ImageDisp->Delta_y + 2) / 4;
    Remain_y = pYaIPS_ImageDisp->Delta_y - Small_y * 4;

    *pLast_x -= Remain_x;      // Add remainder to be used for next time
    *pLast_y -= Remain_y;

#ifdef use_again
#ifdef _DEBUG
    printf("Delta %3d/%3d, Small %3d/%3d, Remain %3d/%3d\n", Delta_x, Delta_y,
        Small_x, Small_y, Remain_x, Remain_y);
#endif
#endif

    if( abs( pYaIPS_ImageDisp->Delta_x) >= abs( pYaIPS_ImageDisp->Delta_y)) {  // Prefer main direction

      pYaIPS_ImageDisp->Plot3D_Azimuth -= Small_x;
      if (pYaIPS_ImageDisp->Plot3D_Azimuth < YAIPS_3DPLOT_AZIMUT_MIN) {
        //pYaIPS_ImageDisp->Plot3D_Azimuth = YAIPS_3DPLOT_AZIMUT_MIN;
        pYaIPS_ImageDisp->Plot3D_Azimuth += 360;            // Handle wrap around
      }
      if (pYaIPS_ImageDisp->Plot3D_Azimuth > YAIPS_3DPLOT_AZIMUT_MAX) {
        //pYaIPS_ImageDisp->Plot3D_Azimuth = YAIPS_3DPLOT_AZIMUT_MAX;
        pYaIPS_ImageDisp->Plot3D_Azimuth -= 360;            // Handle wrap around
      }

    } else {

      pYaIPS_ImageDisp->Plot3D_Elevation += Small_y;
      if (pYaIPS_ImageDisp->Plot3D_Elevation < YAIPS_3DPLOT_ELEVATION_MIN) {
        pYaIPS_ImageDisp->Plot3D_Elevation = YAIPS_3DPLOT_ELEVATION_MIN;
      }
      if (pYaIPS_ImageDisp->Plot3D_Elevation > YAIPS_3DPLOT_ELEVATION_MAX) {
        pYaIPS_ImageDisp->Plot3D_Elevation = YAIPS_3DPLOT_ELEVATION_MAX;
      }
    }

    YaIPS_ImageDispCalcSizes( pYaIPS_ImageDisp, YAIPS_IDISP_FLAG_FORCE_UPDATE);  // Update screen

    return;   // Done
  }
}

/************************************************************************************
 * YaIPS_ImageDispMouse_CommonExit
 *
 * Mouse event callback for image displays, common exit work.
 *
 */

void YaIPS_ImageDispMouse_CommonExit( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp)  // Image display data
{

  // AOI selected

  if( pYaIPS_ImageDisp->AoiDeltaAdd) {                            // AOI selected

    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) == 0) { // NOT set until now

      pYaIPS_ImageDisp->RedrawOnExit = true;                        // Set redraw on exit
      pYaIPS_ImageDisp->BigImageUpdate = pYaIPS_ImageDisp->MyWinID; // Update big image

      pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_SEL;    // Set it
    }

  } else {                                                          // AOI NOT selected

    if( (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_AOI_SEL) != 0) { // Set until now

      pYaIPS_ImageDisp->RedrawOnExit = true;                        // Set redraw on exit
      pYaIPS_ImageDisp->BigImageUpdate = pYaIPS_ImageDisp->MyWinID; // Update big image

      pYaIPS_ImageDisp->Flags &= ~YAIPS_IDISP_FLAG_MOUSE_AOI_SEL;   // Reset it
      pYaIPS_ImageDisp->AoiIdNr = -1;                               // Invalidate AOI selection
    }
  }

  // Redraw

  if( pYaIPS_ImageDisp->RedrawOnExit) {                                          // Redraw on exit

    pYaIPS_ImageDisp->pImage_Box->redraw();                                      // Redraw after image change

    if( pYaIPS_ImageDisp == &YaIPS_BigImageDisp) {                               // Was updating the big image

      YaIPS_ToolWinDrawAfterRedraw( YaIPS_BigImageDisp.ImageSourceID);           // Redraw linked to this

    } else {                                                                     // Update other than the big image

      if( pYaIPS_ImageDisp->BigImageUpdate != 0 &&                               // Update the big image ?
          YaIPS_BigImageDisp.ImageSourceID == pYaIPS_ImageDisp->MyWinID) {       // and display this on the big image

        if( YaIPS_BigImageDisp.pImage_Box != NULL) {
          YaIPS_BigImageDisp.pImage_Box->redraw();
        }
      }
    }
  }

  // Set cursor shape

  if( pYaIPS_ImageDisp->CursorShape > 0) {                                        // Cursor shape set

    pYaIPS_ImageDisp->pParentWindow->cursor( (Fl_Cursor)pYaIPS_ImageDisp->CursorShape);

  } else if( pYaIPS_ImageDisp->CursorShape == 0 &&                                // Cross cursor if mouse is in image
             (pYaIPS_ImageDisp->Flags & YAIPS_IDISP_FLAG_MOUSE_IN_IMAGE) != 0) {  // Mouse is in image

    pYaIPS_ImageDisp->pParentWindow->cursor( FL_CURSOR_CROSS);

  } else {                                                                        // Mouse not in image

    pYaIPS_ImageDisp->pParentWindow->cursor( FL_CURSOR_DEFAULT);
  }
}

/************************************************************************************
 * YaIPS_ImageDispMouse_cb
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

int YaIPS_ImageDispMouse_cb( Fl_Widget *pW, int event,
                              void *pArg1,        // Pointer to Fl_YaIPS_ImageDisp_t
                              void *pArg2)        // Optional pointer to ToolData
{
  Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp;
  int ierr, x, y;
  static int Last_x = -9999, Last_y = -9999;           // Must be static

  pYaIPS_ImageDisp = (Fl_YaIPS_ImageDisp_t *)pArg1;    // Get pointer to image display data

  // Support for big image display to modify AOIs of tool windows

  if( pYaIPS_ImageDisp == &YaIPS_BigImageDisp &&       // Called for big image display
      pYaIPS_ImageDisp->Plot3D_Active == false &&      // and NO 3D plot active
      pYaIPS_ImageDisp->ShowInfoMode == YAIPS_SHOW_INFO_OFF) { // and no measurement/info modes

    ierr = YaIPS_ToolWinMouseCallbackCall( pYaIPS_ImageDisp, pYaIPS_ImageDisp->ImageSourceID, pW, event);

    if( ierr == 0) {     // Mouse callback of tool window was called

      return( 0);        // Mouse events are processed, return to caller
    }

    // continue ...
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

  if( pYaIPS_ImageDisp->ShowInfoMode == YAIPS_SHOW_INFO_CU_VAL &&           // At cursor show pixel value
      ! pYaIPS_ImageDisp->mouseleft && ! pYaIPS_ImageDisp->mouseright &&    // and no mouse button pressed
      (pYaIPS_ImageDisp->Delta_x != 0 || pYaIPS_ImageDisp->Delta_y != 0)) { // and any move

    pYaIPS_ImageDisp->RedrawOnExit = true;                                  // Set redraw on exit
  }

  // Process mouse events

  switch( event) {

  case FL_MOUSEWHEEL:    // The user has moved the mouse wheel.

    // Manage display resolution change by mouse wheel event
    YaIPS_ImageDispMouse_CommonMouseWheel( pYaIPS_ImageDisp, x, y);

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

    if( pYaIPS_ImageDisp->DisplayResolution >= YAIPS_DISP_RESOLUTION_AUTO &&  // Any resolution
        ! pYaIPS_ImageDisp->Plot3D_Active) {                                  // and NO 3D plot active

      if( pYaIPS_ImageDisp->ShowInfoMode >= YAIPS_SHOW_INFO_RE_HISTO_AOI) {   // Need an AOI

        YaIPS_ImageDispAoiClipAndCheck( pYaIPS_ImageDisp, NULL, NULL, &pYaIPS_ImageDisp->CursorShape, &pYaIPS_ImageDisp->AoiDeltaAdd);
      }

      if( pYaIPS_ImageDisp->mouseleft) {                                             // Left mouse button pressed

        if( pYaIPS_ImageDisp->AoiDeltaAdd != 0 && pYaIPS_ImageDisp->Latched_AoiDeltaAdd == 0) {  // Latch AOI mouse modification

          pYaIPS_ImageDisp->Latched_AoiDeltaAdd = pYaIPS_ImageDisp->AoiDeltaAdd;
          pYaIPS_ImageDisp->Latched_CursorShape = pYaIPS_ImageDisp->CursorShape;
        }

        if( pYaIPS_ImageDisp->Latched_AoiDeltaAdd != 0) {                  // Have latched AOI mouse modification

          pYaIPS_ImageDisp->AoiDeltaAdd = pYaIPS_ImageDisp->Latched_AoiDeltaAdd; // Use it
          pYaIPS_ImageDisp->CursorShape = pYaIPS_ImageDisp->Latched_CursorShape;
        }

      } else {                                                     // Left mouse button is NOT pressed

        pYaIPS_ImageDisp->Latched_AoiDeltaAdd = 0;                 // Reset latched data
        pYaIPS_ImageDisp->Latched_CursorShape = 0;
      }

      // Check for change of AOI

      if( pYaIPS_ImageDisp->mouseleft &&                                        // and left button pressed
          pYaIPS_ImageDisp->AoiDeltaAdd != 0 &&                                 // and add deltas
          (pYaIPS_ImageDisp->Delta_x != 0 || pYaIPS_ImageDisp->Delta_y != 0)) { // and mouse has moved

        pYaIPS_ImageDisp->RedrawOnExit = true;                                  // Set redraw on exit

        pYaIPS_ImageDisp->Flags |= YAIPS_IDISP_FLAG_MOUSE_AOI_CHA; // Set AOI changed flag bit

        if( pYaIPS_ImageDisp->AoiDeltaAdd == 0x0f) {           // Move

          // Add to the points

          pYaIPS_ImageDisp->AoiP1x += pYaIPS_ImageDisp->Delta_x;
          pYaIPS_ImageDisp->AoiP1y += pYaIPS_ImageDisp->Delta_y;
          pYaIPS_ImageDisp->AoiP2x += pYaIPS_ImageDisp->Delta_x;
          pYaIPS_ImageDisp->AoiP2y += pYaIPS_ImageDisp->Delta_y;

          // Clip points

          if( pYaIPS_ImageDisp->AoiP1x < 0) {

            pYaIPS_ImageDisp->AoiP2x -= pYaIPS_ImageDisp->AoiP1x;
            pYaIPS_ImageDisp->AoiP1x -= pYaIPS_ImageDisp->AoiP1x;
          }

          if( pYaIPS_ImageDisp->AoiP1y < 0) {

            pYaIPS_ImageDisp->AoiP2y -= pYaIPS_ImageDisp->AoiP1y;
            pYaIPS_ImageDisp->AoiP1y -= pYaIPS_ImageDisp->AoiP1y;
          }

          if( pYaIPS_ImageDisp->AoiP2x >= pYaIPS_ImageDisp->BigImage_sw) {

            pYaIPS_ImageDisp->AoiP1x -= pYaIPS_ImageDisp->AoiP2x - pYaIPS_ImageDisp->BigImage_sw + 1;
            pYaIPS_ImageDisp->AoiP2x = pYaIPS_ImageDisp->BigImage_sw - 1;
          }

          if( pYaIPS_ImageDisp->AoiP2y >= pYaIPS_ImageDisp->BigImage_sh) {

            pYaIPS_ImageDisp->AoiP1y -= pYaIPS_ImageDisp->AoiP2y - pYaIPS_ImageDisp->BigImage_sh + 1;
            pYaIPS_ImageDisp->AoiP2y = pYaIPS_ImageDisp->BigImage_sh - 1;
          }

        } else {                                // change side or edges

          if( (pYaIPS_ImageDisp->AoiDeltaAdd & 0x01) != 0) {           // Move p1x

            pYaIPS_ImageDisp->AoiP1x += pYaIPS_ImageDisp->Delta_x;
            if( pYaIPS_ImageDisp->AoiP1x < 0) {

              pYaIPS_ImageDisp->AoiP1x = 0;

            } else if( pYaIPS_ImageDisp->AoiP1x > pYaIPS_ImageDisp->AoiP2x - YAIPS_IDISP_AOI_MIN_SIZE) {

              pYaIPS_ImageDisp->AoiP1x = pYaIPS_ImageDisp->AoiP2x - YAIPS_IDISP_AOI_MIN_SIZE;
            }
          }

          if( (pYaIPS_ImageDisp->AoiDeltaAdd & 0x02) != 0) {           // Move p1y

            pYaIPS_ImageDisp->AoiP1y += pYaIPS_ImageDisp->Delta_y;
            if( pYaIPS_ImageDisp->AoiP1y < 0) {

              pYaIPS_ImageDisp->AoiP1y = 0;

            } else if( pYaIPS_ImageDisp->AoiP1y > pYaIPS_ImageDisp->AoiP2y - YAIPS_IDISP_AOI_MIN_SIZE) {

              pYaIPS_ImageDisp->AoiP1y = pYaIPS_ImageDisp->AoiP2y - YAIPS_IDISP_AOI_MIN_SIZE;
            }
          }

          if( (pYaIPS_ImageDisp->AoiDeltaAdd & 0x04) != 0) {           // Move p2x

            pYaIPS_ImageDisp->AoiP2x += pYaIPS_ImageDisp->Delta_x;
            if( pYaIPS_ImageDisp->AoiP2x >= pYaIPS_ImageDisp->BigImage_sw) {

              pYaIPS_ImageDisp->AoiP2x = pYaIPS_ImageDisp->BigImage_sw - 1;

            } else if( pYaIPS_ImageDisp->AoiP2x < pYaIPS_ImageDisp->AoiP1x + YAIPS_IDISP_AOI_MIN_SIZE) {

              pYaIPS_ImageDisp->AoiP2x = pYaIPS_ImageDisp->AoiP1x + YAIPS_IDISP_AOI_MIN_SIZE;
            }
          }

          if( (pYaIPS_ImageDisp->AoiDeltaAdd & 0x08) != 0) {           // Move p2y

            pYaIPS_ImageDisp->AoiP2y += pYaIPS_ImageDisp->Delta_y;
            if( pYaIPS_ImageDisp->AoiP2y >= pYaIPS_ImageDisp->BigImage_sh) {

              pYaIPS_ImageDisp->AoiP2y = pYaIPS_ImageDisp->BigImage_sh - 1;

            } else if( pYaIPS_ImageDisp->AoiP2y < pYaIPS_ImageDisp->AoiP1y + YAIPS_IDISP_AOI_MIN_SIZE) {

              pYaIPS_ImageDisp->AoiP2y = pYaIPS_ImageDisp->AoiP1y + YAIPS_IDISP_AOI_MIN_SIZE;
            }
          }
        }

        break;
      }
    }

    // Left/right mouse button pressed on image background
    YaIPS_ImageDispMouse_CommonMouseBackGnd( pYaIPS_ImageDisp, &Last_x, &Last_y);

    break;
  }

ExitPoint:

  // Mouse callback common exit work. Manage AOI selection and setting cursor shape

  YaIPS_ImageDispMouse_CommonExit( pYaIPS_ImageDisp);

  return( 0);        // Mouse events are processed
}

/************************************************************************************
 * YaIPS_ImageDispAoiClipAndCheck
 *
 * Clip AOI points and check for mouse selection.
 *
 * This points are relative to 'BigImage_sx/-y', the image part displayed on screen.
 * - Ensures that the 1. point is the left top point.
 * - Ensures that the AOI is inside the image part displayed on screen.
 *
 *   pAoiXX     if != NULL return X distance of the points.
 *   pAoiYY     if != NULL return X distance of the points.
 *   pCursor    if != NULL return selection cursor depending on distance to points
 *   pDeltaAdd  if != NULL return bit mask where to add the delta
 *
 * return:  < 0  Error
 *            0  OK
 */

int YaIPS_ImageDispAoiClipAndCheck( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                                   int *pAoiXX, int *pAoiYY,
                                   int *pCursor, int *pDeltaAdd)
{
  int Temp;

  if( pYaIPS_ImageDisp->pImage_Box == NULL) {      // Security test, have no big image box

    return( -1);    // Return error
  }

  // Ensures 1. point is left top point.

  if( pYaIPS_ImageDisp->AoiP2x < pYaIPS_ImageDisp->AoiP1x) {

    Temp = pYaIPS_ImageDisp->AoiP1x;
    pYaIPS_ImageDisp->AoiP1x = pYaIPS_ImageDisp->AoiP2x;
    pYaIPS_ImageDisp->AoiP2x = Temp;
  }

  if( pYaIPS_ImageDisp->AoiP2y < pYaIPS_ImageDisp->AoiP1y) {

    Temp = pYaIPS_ImageDisp->AoiP1y;
    pYaIPS_ImageDisp->AoiP1y = pYaIPS_ImageDisp->AoiP2y;
    pYaIPS_ImageDisp->AoiP2y = Temp;
  }

  // Ensures that the AOI is inside the image part displayed on screen.

  if( pYaIPS_ImageDisp->AoiP1x < 0) {

    pYaIPS_ImageDisp->AoiP1x = 0;
  }

  if( pYaIPS_ImageDisp->AoiP2x >= pYaIPS_ImageDisp->BigImage_sw) {

    pYaIPS_ImageDisp->AoiP2x = pYaIPS_ImageDisp->BigImage_sw - 1;
  }

  if( pYaIPS_ImageDisp->AoiP1y < 0) {

    pYaIPS_ImageDisp->AoiP1y = 0;
  }

  if( pYaIPS_ImageDisp->AoiP2y >= pYaIPS_ImageDisp->BigImage_sh) {

    pYaIPS_ImageDisp->AoiP2y = pYaIPS_ImageDisp->BigImage_sh - 1;
  }

  // Check for minimum AOI size

  Temp = pYaIPS_ImageDisp->AoiP2x - pYaIPS_ImageDisp->AoiP1x;                       // Distance

  if( Temp < YAIPS_IDISP_AOI_MIN_SIZE) {                                           // Below minimum size

    pYaIPS_ImageDisp->AoiP2x = pYaIPS_ImageDisp->AoiP1x + YAIPS_IDISP_AOI_MIN_SIZE;  // Adapt position of second point

    Temp = pYaIPS_ImageDisp->AoiP2x - pYaIPS_ImageDisp->BigImage_sw;                // Is too big
    if( Temp >= 0) {

      pYaIPS_ImageDisp->AoiP2x = pYaIPS_ImageDisp->BigImage_sw - 1;                 // Clip second point
      pYaIPS_ImageDisp->AoiP1x -= Temp + 1;                                        // Adapt position of first point
    }

    // Clip points again

    if( pYaIPS_ImageDisp->AoiP1x < 0) {

      pYaIPS_ImageDisp->AoiP1x = 0;
    }

    if( pYaIPS_ImageDisp->AoiP2x >= pYaIPS_ImageDisp->BigImage_sw) {

      pYaIPS_ImageDisp->AoiP2x = pYaIPS_ImageDisp->BigImage_sw - 1;
    }
  }

  Temp = pYaIPS_ImageDisp->AoiP2y - pYaIPS_ImageDisp->AoiP1y;                       // Distance

  if( Temp < YAIPS_IDISP_AOI_MIN_SIZE) {                                           // Below minimum size

    pYaIPS_ImageDisp->AoiP2y = pYaIPS_ImageDisp->AoiP1y + YAIPS_IDISP_AOI_MIN_SIZE;  // Adapt position of second point

    Temp = pYaIPS_ImageDisp->AoiP2y - pYaIPS_ImageDisp->BigImage_sh;                // Is too big
    if( Temp >= 0) {

      pYaIPS_ImageDisp->AoiP2y = pYaIPS_ImageDisp->BigImage_sh - 1;                 // Clip second point
      pYaIPS_ImageDisp->AoiP1y -= Temp + 1;                                        // Adapt position of first point
    }

    // Clip points again

    if( pYaIPS_ImageDisp->AoiP1y < 0) {

      pYaIPS_ImageDisp->AoiP1y = 0;
    }

    if( pYaIPS_ImageDisp->AoiP2y >= pYaIPS_ImageDisp->BigImage_sh) {

      pYaIPS_ImageDisp->AoiP2y = pYaIPS_ImageDisp->BigImage_sh - 1;
    }
  }

  // return

  if( pAoiXX != NULL) {    // Pointer to return value

    *pAoiXX = pYaIPS_ImageDisp->AoiP2x  - pYaIPS_ImageDisp->AoiP1x + 1;
  }

  if( pAoiYY != NULL) {    // Pointer to return value

    *pAoiYY = pYaIPS_ImageDisp->AoiP2y  - pYaIPS_ImageDisp->AoiP1y + 1;
  }

  if( pCursor != NULL || pDeltaAdd != NULL) {

    int DeltaP1x, DeltaP1y, DeltaP2x, DeltaP2y, Cursor, DeltaAdd;

    Cursor   = 0;
    DeltaAdd = 0;

    DeltaP1x = pYaIPS_ImageDisp->MouseX - pYaIPS_ImageDisp->AoiP1x;
    if( DeltaP1x < 0) DeltaP1x = - DeltaP1x;

    DeltaP1y = pYaIPS_ImageDisp->MouseY - pYaIPS_ImageDisp->AoiP1y;
    if( DeltaP1y < 0) DeltaP1y = - DeltaP1y;

    DeltaP2x = pYaIPS_ImageDisp->MouseX - pYaIPS_ImageDisp->AoiP2x;
    if( DeltaP2x < 0) DeltaP2x = - DeltaP2x;

    DeltaP2y = pYaIPS_ImageDisp->MouseY - pYaIPS_ImageDisp->AoiP2y;
    if( DeltaP2y < 0) DeltaP2y = - DeltaP2y;

    // What frame side are the nearest

    int FramedistX, FramedistY;

    FramedistX = -1;
    FramedistY = -1;

    if( pYaIPS_ImageDisp->MouseX >= pYaIPS_ImageDisp->AoiP1x - YAIPS_AOI_FRAME_DIST &&
        pYaIPS_ImageDisp->MouseX <= pYaIPS_ImageDisp->AoiP2x + YAIPS_AOI_FRAME_DIST) {

      // Check outside near top side to be near the frame
      if( pYaIPS_ImageDisp->MouseY >= pYaIPS_ImageDisp->AoiP1y - YAIPS_AOI_FRAME_DIST &&
          pYaIPS_ImageDisp->MouseY <= pYaIPS_ImageDisp->AoiP1y &&
          DeltaP1y <= YAIPS_AOI_FRAME_DIST) {

        DeltaAdd |= 0x02;      // Modify p1y

      } else

      // Check outside near bottom side to be near the frame
      if( pYaIPS_ImageDisp->MouseY <= pYaIPS_ImageDisp->AoiP2y + YAIPS_AOI_FRAME_DIST &&
          pYaIPS_ImageDisp->MouseY >= pYaIPS_ImageDisp->AoiP2y &&
          DeltaP2y <= YAIPS_AOI_FRAME_DIST) {

        DeltaAdd |= 0x08;      // Modify p2y

      } else

      // Check inside the frame to be near one of the borders
      if( pYaIPS_ImageDisp->MouseY >= pYaIPS_ImageDisp->AoiP1y &&
          pYaIPS_ImageDisp->MouseY <= pYaIPS_ImageDisp->AoiP2y) {

        FramedistY = pYaIPS_ImageDisp->AoiP2y - pYaIPS_ImageDisp->AoiP1y;    // Distance of frames

        if( FramedistY >= YAIPS_AOI_FRAME_DIST * 4) {     // More than this distance

          FramedistY = YAIPS_AOI_FRAME_DIST;              // Use STD check distance

        } else {

          // Use lower frame check distance
          FramedistY = FramedistY / 6;
        }

        if( DeltaP1y < DeltaP2y && DeltaP1y <= FramedistY) {

          DeltaAdd |= 0x02;      // Modify p1y

        } else if( DeltaP2y < DeltaP1y && DeltaP2y <= FramedistY) {

          DeltaAdd |= 0x08;      // Modify p2y
        }
      }
    }

    if( pYaIPS_ImageDisp->MouseY >= pYaIPS_ImageDisp->AoiP1y - YAIPS_AOI_FRAME_DIST &&
        pYaIPS_ImageDisp->MouseY <= pYaIPS_ImageDisp->AoiP2y + YAIPS_AOI_FRAME_DIST) {

      // Check outside near left side to be near the frame
      if( pYaIPS_ImageDisp->MouseX >= pYaIPS_ImageDisp->AoiP1x - YAIPS_AOI_FRAME_DIST &&
          pYaIPS_ImageDisp->MouseX <= pYaIPS_ImageDisp->AoiP1x &&
          DeltaP1x <= YAIPS_AOI_FRAME_DIST) {

        DeltaAdd |= 0x01;      // Modify p1x

      } else

      // Check outside near right side to be near the frame
      if( pYaIPS_ImageDisp->MouseX <= pYaIPS_ImageDisp->AoiP2x + YAIPS_AOI_FRAME_DIST &&
          pYaIPS_ImageDisp->MouseX >= pYaIPS_ImageDisp->AoiP2x &&
          DeltaP2x <= YAIPS_AOI_FRAME_DIST) {

        DeltaAdd |= 0x04;      // Modify p2x

      } else

      // Check inside the frame to be near one of the borders

      if( pYaIPS_ImageDisp->MouseX >= pYaIPS_ImageDisp->AoiP1x &&
          pYaIPS_ImageDisp->MouseX <= pYaIPS_ImageDisp->AoiP2x) {

        FramedistX = pYaIPS_ImageDisp->AoiP2x - pYaIPS_ImageDisp->AoiP1x;    // Distance of frames

        if( FramedistX >= YAIPS_AOI_FRAME_DIST * 4) {     // More than this distance

          FramedistX = YAIPS_AOI_FRAME_DIST;              // Use STD check distance

        } else {

          // Use lower frame check distance
          FramedistX = FramedistX / 6;
        }

        if( DeltaP1x < DeltaP2x && DeltaP1x <= FramedistX) {

          DeltaAdd |= 0x01;      // Modify p1x

        } else if( DeltaP2x < DeltaP1y && DeltaP2x <= FramedistX) {

          DeltaAdd |= 0x04;      // Modify p2x
        }
      }
    }

    // Decide for a cursor

    if( pCursor != NULL) {

      switch( DeltaAdd) {

      case 0x00:                   // not near a frame border

        // Check for inside the frame
        if( pYaIPS_ImageDisp->MouseX >= pYaIPS_ImageDisp->AoiP1x &&
            pYaIPS_ImageDisp->MouseX <= pYaIPS_ImageDisp->AoiP2x &&
            pYaIPS_ImageDisp->MouseY >= pYaIPS_ImageDisp->AoiP1y &&
            pYaIPS_ImageDisp->MouseY <= pYaIPS_ImageDisp->AoiP2y) {

          Cursor = FL_CURSOR_MOVE;  //x/ FL_CURSOR_HAND;

          DeltaAdd = 0x0f;
        }
        break;

      case 0x01:                   // left/right resize: ⇔
      case 0x04:
        Cursor = FL_CURSOR_WE;
        break;

      case 0x02:                   // up/down resize: ⇕
      case 0x08:
        Cursor = FL_CURSOR_NS;
        break;

      case 0x03:                   // diagonal resize: ⤡
      case 0x0c:
        Cursor = FL_CURSOR_NWSE;
        break;

      case 0x09:                   // diagonal resize: ⤢
      case 0x06:
        Cursor = FL_CURSOR_NESW;
        break;
      } // end switch( DeltaAdd)

      *pCursor = Cursor;
    }

    if( pDeltaAdd != NULL) {

      *pDeltaAdd = DeltaAdd;
    }
  }

  return( 0);   // return OK
}

/************************************************************************************
* ImageDispAoiRectDraw
*
* Draw an AOI
*
* NOTE: Make a fl_push_clip() call before calling this function
*
*/

void ImageDispAoiRectDraw( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                           Fl_YaIPS_AOI_t *pAOI,                    // Pointer to AOI data
                           int Teach_mode,                          // 0 = inspection mode, 1 = teach mode
                           int IsSelected,                          // True if window is selected
                           int InspError,                           // != 0 if error in inspection. Color AOI red if Teach_mode is zero.
                           char *pText)                             // If != NULL, draw this text
{
  int LineWidth;
  int x1, y1, x, y, xxo, yyo, OffX, OffY, DrawText;
  Fl_Color DrawColor;

  // Check for valid output image displayed

  if( pYaIPS_ImageDisp->pImage_Img == NULL) {

    return;
  }

  // Preparations

  x1 = pYaIPS_ImageDisp->BigImage_sx;
  y1 = pYaIPS_ImageDisp->BigImage_sy;

  // Points relative to image

  OffX = (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);
  OffY = (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

  LineWidth = YaIPS_Setting_Wide_Graphic_Lines ? YAIPS_LINE_WIDTH_WIDE : YAIPS_LINE_WIDTH_SMALL;

  // Prepare font size

  DrawText = false;                                            // Preset, do not draw text

  if( pYaIPS_ImageDisp->PixelImageToScreen >= 0.33) {          // Screen resolution is NOT to tiny

    int TempFontSize;

    DrawText = true;                                           // Draw crcdf text

    TempFontSize = (int)(pYaIPS_ImageDisp->PixelImageToScreen * 16.0 + 0.5);  // * 12.0

    if( TempFontSize < 10) {
      TempFontSize = 10;
    }

    fl_font( FL_HELVETICA, TempFontSize);
  }

  // Draw AOIs

  int AOI_XX, AOI_YY;

  // Get AOI

  if( pAOI->XSize >= pYaIPS_ImageDisp->pImage_Img->w()) {

    AOI_XX = pYaIPS_ImageDisp->pImage_Img->w();
  } else {
    AOI_XX = pAOI->XSize;
  }

  if( pAOI->YSize >= pYaIPS_ImageDisp->pImage_Img->h()) {

    AOI_YY = pYaIPS_ImageDisp->pImage_Img->h();
  } else {
    AOI_YY = pAOI->YSize;
  }

  // Color

  if( Teach_mode) {                // Teach mode

    if( IsSelected) {     // Teach mode and mouse is over this AOI

      DrawColor = FL_RED;
    } else {

      DrawColor = FL_GREEN - 2;
    }

  } else {                                // Inspection mode

    if( InspError != 0) {

      DrawColor = FL_RED;

    } else {

      DrawColor = FL_GREEN - 2;
    }
  }

  fl_line_style( 0, LineWidth);   // Set line width
  fl_color( DrawColor);           // Color

  fl_line_style( 0, LineWidth);   // Set line width

  // Draw rectangle

  x = (int)( (pAOI->XPos - OffX) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
  y = (int)( (pAOI->YPos - OffY) * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

  xxo = (int)( AOI_XX * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);
  yyo = (int)( AOI_YY * pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

  fl_rect( x1 + x, y1 + y, xxo, yyo);

  // Draw text

  if( DrawText &&            // Draw text
      pText != NULL &&
      pText[ 0] != '\0') {

    int mdx, mdy, mw, mh;

    fl_text_extents( pText, mdx, mdy, mw, mh);

    if( y + mdy < 4) {         // To near to upper border

      y += mh + 5;             // Show below upper frame
      x += 4;

    } else {                   // Fits above upper frame

      y -= 4;
    }

    fl_draw( pText, x1 + x, y1 + y);
  }

  // Finish up

  fl_line_style( 0);   // Reset to default
}

/************************************************************************************
 * YaIPS_ImageDispAoiRectClip
 *
 * AOI rectangle clip against image size
 *
 *   ImgXX, ImgYY       Size of image
 *   pAOI               Point to AOI to test
 *
 * return:    0  OK
 *            1  Something clipped
 */

int YaIPS_ImageDispAoiRectClip( int ImgXX, int ImgYY,       // Size of image
                                Fl_YaIPS_AOI_t *pAOI)       // Point to AOI to test
{
  int RedrawOnExit;

  RedrawOnExit = false;

  // Clip size x
  if( pAOI->XSize < YAIPS_IDISP_AOI_MIN_SIZE) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->XSize = YAIPS_IDISP_AOI_MIN_SIZE;
  }

  if( pAOI->XSize > ImgXX){
    RedrawOnExit = true;                                 // Something clipped
    pAOI->XSize = ImgXX;
  }

  // Clip position x
  if( pAOI->XPos < 0) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->XPos = 0;
  }

  if( pAOI->XPos > ImgXX - pAOI->XSize) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->XPos = ImgXX - pAOI->XSize;
  }

  // Clip size y
  if( pAOI->YSize < YAIPS_IDISP_AOI_MIN_SIZE){
    RedrawOnExit = true;                                 // Something clipped
    pAOI->YSize = YAIPS_IDISP_AOI_MIN_SIZE;
  }

  if( pAOI->YSize > ImgYY){
    RedrawOnExit = true;                                 // Something clipped
    pAOI->YSize = ImgYY;
  }

  // Clip position y
  if( pAOI->YPos < 0) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->YPos = 0;
  }

  if( pAOI->YPos > ImgYY - pAOI->YSize) {
    RedrawOnExit = true;                                 // Something clipped
    pAOI->YPos = ImgYY - pAOI->YSize;
  }

  return( RedrawOnExit);
}

/************************************************************************************
 * YaIPS_ImageDispAoiRectClip
 *
 * AOI rectangle clip to image displayed on the screen
 *
 * The AOI rectangle is relative to image displayed on the screen 'BigImage_iw/-ih'.
 *
 * return:  < 0  Error
 *            0  OK
 */

int YaIPS_ImageDispAoiRectClip( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                                Fl_YaIPS_AOI_t *pAOI)       // Point to AOI to test

{

  if( pYaIPS_ImageDisp->pImage_Box == NULL ||          // Security test, have no big image box
      pYaIPS_ImageDisp->BigImage_Calc_OK == false) {   // Size calculations failed

    return( -1);    // Return error
  }

  // Ensures that the AOI is inside the image part displayed on screen.

  if( pAOI->XSize < YAIPS_IDISP_AOI_MIN_SIZE) {            // Clip min with

    pAOI->XSize = YAIPS_IDISP_AOI_MIN_SIZE;
  }

  if( pAOI->XSize > pYaIPS_ImageDisp->BigImage_iw) {       // Clip max with

    pAOI->XSize = pYaIPS_ImageDisp->BigImage_iw;
  }

  if( pAOI->XPos < 0) {                                    // Clip left

    pAOI->XPos = 0;
  }

  if( pAOI->XPos + pAOI->XSize > pYaIPS_ImageDisp->BigImage_iw) {

    pAOI->XPos = pYaIPS_ImageDisp->BigImage_iw - pAOI->XSize;
  }

  if( pAOI->YSize < YAIPS_IDISP_AOI_MIN_SIZE) {            // Clip min height

    pAOI->YSize = YAIPS_IDISP_AOI_MIN_SIZE;
  }

  if( pAOI->YSize > pYaIPS_ImageDisp->BigImage_ih) {       // Clip max height

    pAOI->YSize = pYaIPS_ImageDisp->BigImage_ih;
  }

  if( pAOI->YPos < 0) {                                    // Clip upper

    pAOI->YPos = 0;
  }

  if( pAOI->YPos + pAOI->YSize > pYaIPS_ImageDisp->BigImage_ih) {

    pAOI->YPos = pYaIPS_ImageDisp->BigImage_ih - pAOI->YSize;
  }

  return( 0);       // OK
}

/************************************************************************************
 * YaIPS_ImageDispAoiRectDeltaAdd
 *
 * Add position change to AOI
 *
 * The AOI rectangle is relative to image displayed on the screen 'BigImage_iw/-ih'.
 *
 *   pAOI            Point to AOI
 *   AoiDeltaAdd     Where to add mouse delta
 *   Delta_x         Delta in X direction
 *   Delta_y         Delta in y direction
 *
 * return:  < 0  Error
 *            0  OK
 */

int YaIPS_ImageDispAoiRectDeltaAdd( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
                                    Fl_YaIPS_AOI_t *pAOI,       // Point to AOI to test
                                    int AoiDeltaAdd,            // Where to add mouse delta
                                    int Delta_x,                // Delta in X direction
                                    int Delta_y)                // Delta in y direction
{

  if( pYaIPS_ImageDisp->pImage_Box == NULL ||          // Security test, have no big image box
      pYaIPS_ImageDisp->BigImage_Calc_OK == false) {   // Size calculations failed

    return( -1);    // Return error
  }

  if( AoiDeltaAdd == 0x0f) {           // Move

    // Add to the points

    pAOI->XPos += Delta_x;
    pAOI->YPos += Delta_y;

  } else {                                // change side or edges

    if( (AoiDeltaAdd & 0x01) != 0) {           // Move p1x

      if( Delta_x >= 0) {

        if ((pAOI->XSize - YAIPS_IDISP_AOI_MIN_SIZE) < Delta_x) {
          Delta_x = pAOI->XSize - YAIPS_IDISP_AOI_MIN_SIZE;
        }
        pAOI->XSize -= Delta_x;
        pAOI->XPos  += Delta_x;

      } else {

        Delta_x = 0 - Delta_x;
        if (pAOI->XPos < Delta_x) {
          Delta_x = pAOI->XPos;
        }
        pAOI->XSize += Delta_x;
        pAOI->XPos  -= Delta_x;
      }
    }

    if( (AoiDeltaAdd & 0x02) != 0) {           // Move p1y

      if( Delta_y >= 0) {

        if ((pAOI->YSize - YAIPS_IDISP_AOI_MIN_SIZE) < Delta_y) {
          Delta_y = pAOI->YSize - YAIPS_IDISP_AOI_MIN_SIZE;
        }
        pAOI->YSize -= Delta_y;
        pAOI->YPos  += Delta_y;

      } else {

        Delta_y = 0 - Delta_y;
        if (pAOI->YPos < Delta_y) {
          Delta_y = pAOI->YPos;
        }
        pAOI->YSize += Delta_y;
        pAOI->YPos  -= Delta_y;
      }
    }

    if( (AoiDeltaAdd & 0x04) != 0) {           // Move p2x

      if( Delta_x < 0) {

        Delta_x = 0 - Delta_x;
        if ((pAOI->XSize - YAIPS_IDISP_AOI_MIN_SIZE) < Delta_x) {
          Delta_x = pAOI->XSize - YAIPS_IDISP_AOI_MIN_SIZE;
        }
        pAOI->XSize -= Delta_x;
      } else {
        if ((pYaIPS_ImageDisp->BigImage_iw - (pAOI->XPos + pAOI->XSize)) < Delta_x) {
          Delta_x = pYaIPS_ImageDisp->BigImage_iw - (pAOI->XPos + pAOI->XSize);
        }
        pAOI->XSize += Delta_x;
      }
    }

    if( (AoiDeltaAdd & 0x08) != 0) {           // Move p2y

      if( Delta_y < 0) {

        Delta_y = 0 - Delta_y;
        if ((pAOI->YSize - YAIPS_IDISP_AOI_MIN_SIZE) < Delta_y) {
          Delta_y = pAOI->YSize - YAIPS_IDISP_AOI_MIN_SIZE;
        }
        pAOI->YSize -= Delta_y;
      } else {
        if ((pYaIPS_ImageDisp->BigImage_ih - (pAOI->YPos + pAOI->YSize)) < Delta_y) {
          Delta_y = pYaIPS_ImageDisp->BigImage_ih - (pAOI->YPos + pAOI->YSize);
        }
        pAOI->YSize += Delta_y;
      }
    }
  }

  YaIPS_ImageDispAoiRectClip( pYaIPS_ImageDisp, pAOI);

  return( 0);       // OK
}

/************************************************************************************
 * YaIPS_ImageDispAoiRectCC
 *
 * AOI rectangle clip and check for mouse selection.
 *
 * The AOI rectangle is relative to image displayed on the screen 'BigImage_iw/-ih'.
 *
 *   pAOI            Point to AOI to test
 *   distanceToBeat  For first call must be set to -1.
 *                   An exit with new best distance, this distance is stored to
 *                   this variable. A successive call with a other aoi must
 *                   beat this one to get selected.
 *   pCursor         Return selection cursor depending on distance to points
 *   pDeltaAdd       Return bit mask where to add the delta
 *
 * return:  < 0  Error
 *            0  Mouse not inside window or not nearer than best distance
 *            1  New best distance
 */

int YaIPS_ImageDispAoiRectCC( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,
    Fl_YaIPS_AOI_t *pAOI,       // Point to AOI to test
                              int *distanceToBeat,        // In Out: Distance to beat
                              int *pCursor,               // Out: Cursor shape
                              int *pDeltaAdd)             // Out: Where to add mouse delta
{
  int xMouse, yMouse, dist;
  int AoiP1x, AoiP1y, AoiP2x, AoiP2y;
  int DeltaP1x, DeltaP1y, DeltaP2x, DeltaP2y, Cursor, DeltaAdd;
  int FramedistX, FramedistY;

  if( pYaIPS_ImageDisp->pImage_Box == NULL ||          // Security test, have no big image box
      pYaIPS_ImageDisp->BigImage_Calc_OK == false) {   // Size calculations failed

    return( -1);    // Return error
  }

  // Ensures that the AOI is inside the image part displayed on screen.

  if( pAOI->XSize < YAIPS_IDISP_AOI_MIN_SIZE) {            // Clip min with

    pAOI->XSize = YAIPS_IDISP_AOI_MIN_SIZE;
  }

  if( pAOI->XSize > pYaIPS_ImageDisp->BigImage_iw) {       // Clip max with

    pAOI->XSize = pYaIPS_ImageDisp->BigImage_iw;
  }

  if( pAOI->XPos < 0) {                                    // Clip left

    pAOI->XPos = 0;
  }

  if( pAOI->XPos + pAOI->XSize > pYaIPS_ImageDisp->BigImage_iw) {

    pAOI->XPos = pYaIPS_ImageDisp->BigImage_iw - pAOI->XSize;
  }

  if( pAOI->YSize < YAIPS_IDISP_AOI_MIN_SIZE) {            // Clip min height

    pAOI->YSize = YAIPS_IDISP_AOI_MIN_SIZE;
  }

  if( pAOI->YSize > pYaIPS_ImageDisp->BigImage_ih) {       // Clip max height

    pAOI->YSize = pYaIPS_ImageDisp->BigImage_ih;
  }

  if( pAOI->YPos < 0) {                                    // Clip upper

    pAOI->YPos = 0;
  }

  if( pAOI->YPos + pAOI->YSize > pYaIPS_ImageDisp->BigImage_ih) {

    pAOI->YPos = pYaIPS_ImageDisp->BigImage_ih - pAOI->YSize;
  }

  // Check for inside window

  xMouse = (int)( pYaIPS_ImageDisp->MouseX / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);  // Mouse relative to displayed screen part
  yMouse = (int)( pYaIPS_ImageDisp->MouseY / pYaIPS_ImageDisp->PixelImageToScreen + 0.5);

  xMouse += (int)( pYaIPS_ImageDisp->SubImage_x + 0.5);   // Add Offset to displayed screen part
  yMouse += (int)( pYaIPS_ImageDisp->SubImage_y + 0.5);

  // Check for inside or near the AOI

  AoiP1x = pAOI->XPos;
  AoiP1y = pAOI->YPos;
  AoiP2x = pAOI->XPos + pAOI->XSize - 1;
  AoiP2y = pAOI->YPos + pAOI->YSize - 1;

  if( xMouse > AoiP1x - YAIPS_AOI_FRAME_DIST && xMouse < AoiP2x + YAIPS_AOI_FRAME_DIST &&
      yMouse > AoiP1y - YAIPS_AOI_FRAME_DIST && yMouse < AoiP2y + YAIPS_AOI_FRAME_DIST) {

    // Get distance to the frames

    DeltaP1x = xMouse - AoiP1x;
    if( DeltaP1x < 0) DeltaP1x = - DeltaP1x;

    DeltaP1y = yMouse - AoiP1y;
    if( DeltaP1y < 0) DeltaP1y = - DeltaP1y;

    DeltaP2x = xMouse - AoiP2x;
    if( DeltaP2x < 0) DeltaP2x = - DeltaP2x;

    DeltaP2y = yMouse - AoiP2y;
    if( DeltaP2y < 0) DeltaP2y = - DeltaP2y;

    // Get nearest distance to frame

    dist = DeltaP1x;
    if( DeltaP1y < dist) dist = DeltaP1y;
    if( DeltaP2x < dist) dist = DeltaP2x;
    if( DeltaP2y < dist) dist = DeltaP2y;

    if( *distanceToBeat == -1 || dist < *distanceToBeat) {    /* position and border            */

      // save new distance to beat
      *distanceToBeat = dist;

      Cursor   = 0;
      DeltaAdd = 0;

      // What frame side are the nearest

      FramedistX = -1;
      FramedistY = -1;

      if( xMouse >= AoiP1x - YAIPS_AOI_FRAME_DIST &&
          xMouse <= AoiP2x + YAIPS_AOI_FRAME_DIST) {

        // Check outside near top side to be near the frame
        if( yMouse >= AoiP1y - YAIPS_AOI_FRAME_DIST &&
            yMouse <= AoiP1y &&
            DeltaP1y <= YAIPS_AOI_FRAME_DIST) {

          DeltaAdd |= 0x02;      // Modify p1y

        } else

        // Check outside near bottom side to be near the frame
        if( yMouse <= AoiP2y + YAIPS_AOI_FRAME_DIST &&
            yMouse >= AoiP2y &&
            DeltaP2y <= YAIPS_AOI_FRAME_DIST) {

          DeltaAdd |= 0x08;      // Modify p2y

        } else

        // Check inside the frame to be near one of the borders
        if( yMouse >= AoiP1y &&
            yMouse <= AoiP2y) {

          FramedistY = AoiP2y - AoiP1y;    // Distance of frames

          if( FramedistY >= YAIPS_AOI_FRAME_DIST * 4) {     // More than this distance

            FramedistY = YAIPS_AOI_FRAME_DIST;              // Use STD check distance

          } else {

            // Use lower frame check distance
            FramedistY = FramedistY / 6;
          }

          if( DeltaP1y < DeltaP2y && DeltaP1y <= FramedistY) {

            DeltaAdd |= 0x02;      // Modify p1y

          } else if( DeltaP2y < DeltaP1y && DeltaP2y <= FramedistY) {

            DeltaAdd |= 0x08;      // Modify p2y
          }
        }
      }

      if( yMouse >= AoiP1y - YAIPS_AOI_FRAME_DIST &&
          yMouse <= AoiP2y + YAIPS_AOI_FRAME_DIST) {

        // Check outside near left side to be near the frame
        if( xMouse >= AoiP1x - YAIPS_AOI_FRAME_DIST &&
            xMouse <= AoiP1x &&
            DeltaP1x <= YAIPS_AOI_FRAME_DIST) {

          DeltaAdd |= 0x01;      // Modify p1x

        } else

        // Check outside near right side to be near the frame
        if( xMouse <= AoiP2x + YAIPS_AOI_FRAME_DIST &&
            xMouse >= AoiP2x &&
            DeltaP2x <= YAIPS_AOI_FRAME_DIST) {

          DeltaAdd |= 0x04;      // Modify p2x

        } else

        // Check inside the frame to be near one of the borders

        if( xMouse >= AoiP1x &&
            xMouse <= AoiP2x) {

          FramedistX = AoiP2x - AoiP1x;    // Distance of frames

          if( FramedistX >= YAIPS_AOI_FRAME_DIST * 4) {     // More than this distance

            FramedistX = YAIPS_AOI_FRAME_DIST;              // Use STD check distance

          } else {

            // Use lower frame check distance
            FramedistX = FramedistX / 6;
          }

          if( DeltaP1x < DeltaP2x && DeltaP1x <= FramedistX) {

            DeltaAdd |= 0x01;      // Modify p1x

          } else if( DeltaP2x < DeltaP1y && DeltaP2x <= FramedistX) {

            DeltaAdd |= 0x04;      // Modify p2x
          }
        }
      }

      // Decide for a cursor

      switch( DeltaAdd) {

      case 0x00:                   // not near a frame border

        // Check for inside the frame
        if( xMouse >= AoiP1x &&
            xMouse <= AoiP2x &&
            yMouse >= AoiP1y &&
            yMouse <= AoiP2y) {

          Cursor = FL_CURSOR_MOVE;  //x/ FL_CURSOR_HAND;

          DeltaAdd = 0x0f;
        }
        break;

      case 0x01:                   // left/right resize: ⇔
      case 0x04:
        Cursor = FL_CURSOR_WE;
        break;

      case 0x02:                   // up/down resize: ⇕
      case 0x08:
        Cursor = FL_CURSOR_NS;
        break;

      case 0x03:                   // diagonal resize: ⤡
      case 0x0c:
        Cursor = FL_CURSOR_NWSE;
        break;

      case 0x09:                   // diagonal resize: ⤢
      case 0x06:
        Cursor = FL_CURSOR_NESW;
        break;
      } // end switch( DeltaAdd)

      *pCursor = Cursor;
      *pDeltaAdd = DeltaAdd;

      return( 1);   // return new best distance
    }
  }

  return( 0);       // Mouse not inside window or not nearer than best distance
}

/************************************************************************************
 * YaIPS_ImageDispAoiRectIGuiUpdate
 *
 * AOI rectangle clip against image size and update GUI input elements of the AOI.
 *
 * The AOI rectangle is relative to image displayed on the screen 'BigImage_iw/-ih'.
 *
 *   pAOI               Point to AOI to test
 *   mgXX, ImgYY        Size of image
 *   pAOI_X, pAOI_Y     GUI input elements
 *   pAOI_XX, pAOI_YY
 *
 * return:    0  OK
 *            1  One of the GUI elements have been changed.
 */

int YaIPS_ImageDispAoiRectIGuiUpdate( Fl_YaIPS_AOI_t *pAOI,       // Point to AOI to test
                                      int ImgXX, int ImgYY,       // Size of image
                                      void *pAOI_X_Arg,           // GUI input elements, must be a IqeFl_Int_Input pointer
                                      void *pAOI_Y_Arg,
                                      void *pAOI_XX_Arg,
                                      void *pAOI_YY_Arg)
{
  IqeFl_Int_Input *pAOI_X, *pAOI_Y, *pAOI_XX, *pAOI_YY;
  int RedrawOnExit;
  int AOI_Xo, AOI_Yo, AOI_XXo, AOI_YYo;
  int AOI_Xn, AOI_Yn, AOI_XXn, AOI_YYn;

  pAOI_X  = (IqeFl_Int_Input *)pAOI_X_Arg;
  pAOI_Y  = (IqeFl_Int_Input *)pAOI_Y_Arg;
  pAOI_XX = (IqeFl_Int_Input *)pAOI_XX_Arg;
  pAOI_YY = (IqeFl_Int_Input *)pAOI_YY_Arg;

  RedrawOnExit = false;

  // Maximums AOI Inputs

  if( pAOI_X->Max != ImgXX - 1) {       // Maximum is not correct
    pAOI_X->Max = ImgXX - 1;
  }

  if( pAOI_Y->Max != ImgYY - 1) {       // Maximum is not correct
    pAOI_Y->Max = ImgYY - 1;
  }

  if( pAOI_XX->Max != ImgXX) {          // Maximum is not correct
    pAOI_XX->Max = ImgXX;
  }

  if( pAOI_YY->Max != ImgYY) {          // Maximum is not correct
    pAOI_YY->Max = ImgYY;
  }

  // Clipping AOI values

  AOI_Xn  = AOI_Xo  = pAOI_X->GetValue();
  AOI_Yn  = AOI_Yo  = pAOI_Y->GetValue();
  AOI_XXn = AOI_XXo = pAOI_XX->GetValue();
  AOI_YYn = AOI_YYo = pAOI_YY->GetValue();

  // Clip size x
  if( AOI_XXn < pAOI_XX->Min){
    AOI_XXn = pAOI_XX->Min;
  }

  if( AOI_XXn > ImgXX){
    AOI_XXn = ImgXX;
  }

  // Clip position x
  if( AOI_Xn < 0) {

    AOI_Xn = 0;
  }

  if( AOI_Xn > ImgXX - AOI_XXn) {

    AOI_Xn = ImgXX - AOI_XXn;
  }

  // Clip size y
  if( AOI_YYn < pAOI_YY->Min){
    AOI_YYn = pAOI_YY->Min;
  }

  if( AOI_YYn > ImgYY){
    AOI_YYn = ImgYY;
  }

  // Clip position y
  if( AOI_Yn < 0) {

    AOI_Yn = 0;
  }

  if( AOI_Yn > ImgYY - AOI_YYn) {

    AOI_Yn = ImgYY - AOI_YYn;
  }

  // Update changed values

  if( AOI_Xn != AOI_Xo) {

    RedrawOnExit = true;                                 // Redraw camera image to show corrected line

    pAOI->XPos = AOI_Xn;
    pAOI_X->SetValue( AOI_Xn);                        // Update on GUI
    pAOI_X->redraw();
  }

  if( AOI_Yn != AOI_Yo) {

    RedrawOnExit = true;                                 // Redraw camera image to show corrected line

    pAOI->YPos = AOI_Yn;
    pAOI_Y->SetValue( AOI_Yn);                        // Update on GUI
    pAOI_Y->redraw();
  }

  if( AOI_XXn != AOI_XXo) {

    RedrawOnExit = true;                                 // Redraw camera image to show corrected line

    pAOI->XSize = AOI_XXn;
    pAOI_XX->SetValue(  AOI_XXn);                     // Update on GUI
    pAOI_XX->redraw();
  }

  if( AOI_YYn != AOI_YYo) {

    RedrawOnExit = true;                                // Redraw camera image to show corrected line

    pAOI->YSize = AOI_YYn;
    pAOI_YY->SetValue(  AOI_YYn);                    // Update on GUI
    pAOI_YY->redraw();
  }

  return( RedrawOnExit);
}

/************************************************************************************
 * YaIPS_ImageDispStrInfo
 *
 * Empty 'StrInfo' string.
 *
 * pYaIPS_ImageDisp  Point to image display data
 *
 */

void YaIPS_ImageDispStrInfo( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp)  // Point to image display data
{

  pYaIPS_ImageDisp->StrInfo[ 0] = '\0';   // Empty string
}

/************************************************************************************
 * YaIPS_ImageDispStrInfo
 *
 * Empty 'StrInfo' string and set colors.
 *
 * pYaIPS_ImageDisp  Point to image display data
 * ColInfoBgnd       Color for background of info string. 0 is used for default
 * ColInfoText       Color for text of info string. 0 is used for default color.
 *
 */

void YaIPS_ImageDispStrInfo( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                             Fl_Color ColInfoBgnd,                    // Color for background of info string. 0 is used for default
                             Fl_Color ColInfoText)                    // Color for text of info string. 0 is used for default color.
{

  pYaIPS_ImageDisp->StrInfo[ 0] = '\0';   // Empty string

  pYaIPS_ImageDisp->ColInfoBgnd = ColInfoBgnd;
  pYaIPS_ImageDisp->ColInfoText = ColInfoText;
}

/************************************************************************************
 * YaIPS_ImageDispStrInfo
 *
 * Copy to 'StrInfo' string.
 *
 * pYaIPS_ImageDisp  Point to image display data
 * pString           Pointer in string to set
 *
 */

void YaIPS_ImageDispStrInfo( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                             char *pString)                           // Pointer in string to set
{

  if( pString == NULL || *pString == '\0') {  // Null pointer or string is empty

    pYaIPS_ImageDisp->StrInfo[ 0] = '\0';    // Empty string

    return;
  }

  // Copy string
  strncpy( pYaIPS_ImageDisp->StrInfo, pString, sizeof( pYaIPS_ImageDisp->StrInfo) - 1);

  // Ensure proper end of string
  pYaIPS_ImageDisp->StrInfo[ sizeof( pYaIPS_ImageDisp->StrInfo) - 1] = '\0';
}

/************************************************************************************
 * YaIPS_ImageDispStrInfo
 *
 * Copy to 'StrInfo' string and set colors.
 *
 * pYaIPS_ImageDisp  Point to image display data
 * ColInfoBgnd       Color for background of info string. 0 is used for default
 * ColInfoText       Color for text of info string. 0 is used for default color.
 * pString           Pointer in string to set
 *
 */

void YaIPS_ImageDispStrInfo( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                             Fl_Color ColInfoBgnd,                    // Color for background of info string. 0 is used for default
                             Fl_Color ColInfoText,                    // Color for text of info string. 0 is used for default color.
                             char *pString)                           // Pointer in string to set
{

  YaIPS_ImageDispStrInfo( pYaIPS_ImageDisp, pString);

  pYaIPS_ImageDisp->ColInfoBgnd = ColInfoBgnd;
  pYaIPS_ImageDisp->ColInfoText = ColInfoText;
}

/************************************************************************************
 * YaIPS_ImageDispStrInfo
 *
 * Print to 'StrInfo' string.
 *
 * pYaIPS_ImageDisp  Point to image display data
 * format            Point to format string
 * ...               Additional arguments
 *
 */

void YaIPS_ImageDispStrInfo( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                             const char * format,                     // Point to format string
                             ...)                                     // Additional arguments
{
  char buffer[ 512];                     // A big string buffer

  va_list args;                          // Print arguments to string
  va_start (args, format);
  vsprintf (buffer,format, args);
  va_end (args);

  // Set string in image display data
  YaIPS_ImageDispStrInfo( pYaIPS_ImageDisp, buffer);
}

/************************************************************************************
 * YaIPS_ImageDispStrInfo
 *
 * Print to 'StrInfo' string and set colors.
 *
 * pYaIPS_ImageDisp  Point to image display data
 * format            Point to format string
 * ...               Additional arguments
 *
 */

void YaIPS_ImageDispStrInfo( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                             Fl_Color ColInfoBgnd,                    // Color for background of info string. 0 is used for default
                             Fl_Color ColInfoText,                    // Color for text of info string. 0 is used for default color.
                             const char * format,                     // Point to format string
                             ...)                                     // Additional arguments
{
  char buffer[ 512];                     // A big string buffer

  va_list args;                          // Print arguments to string
  va_start (args, format);
  vsprintf (buffer,format, args);
  va_end (args);

  // Set string in image display data
  YaIPS_ImageDispStrInfo( pYaIPS_ImageDisp, buffer);

  pYaIPS_ImageDisp->ColInfoBgnd = ColInfoBgnd;
  pYaIPS_ImageDisp->ColInfoText = ColInfoText;
}

/************************************************************************************
 * YaIPS_ImageDispStrDebug
 *
 * Empty 'StrDebug' string.
 *
 * pYaIPS_ImageDisp: Point to image display data
 *
 */

void YaIPS_ImageDispStrDebug( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp)  // Point to image display data
{

  pYaIPS_ImageDisp->StrDebug[ 0] = '\0';   // Empty string
}

/************************************************************************************
 * YaIPS_ImageDispStrDebug
 *
 * Copy to 'StrDebug' string.
 *
 * pYaIPS_ImageDisp: Point to image display data
 * pString:          Pointer in string to set
 *
 */

void YaIPS_ImageDispStrDebug( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                              char *pString)                           // Pointer in string to set
{

  if( pString == NULL || *pString == '\0') {  // Null pointer or string is empty

    pYaIPS_ImageDisp->StrDebug[ 0] = '\0';    // Empty string

    return;
  }

  // Copy string
  strncpy( pYaIPS_ImageDisp->StrDebug, pString, sizeof( pYaIPS_ImageDisp->StrDebug) - 1);

  // Ensure proper end of string
  pYaIPS_ImageDisp->StrDebug[ sizeof( pYaIPS_ImageDisp->StrDebug) - 1] = '\0';
}

/************************************************************************************
 * YaIPS_ImageDispStrDebug
 *
 * Print to 'StrDebug' string.
 *
 * pYaIPS_ImageDisp: Point to image display data
 * format:           Point to format string
 * ...               Additional arguments
 *
 */

void YaIPS_ImageDispStrDebug( Fl_YaIPS_ImageDisp_t *pYaIPS_ImageDisp,  // Point to image display data
                              const char * format,                     // Point to format string
                              ...)                                     // Additional arguments
{
  char buffer[ 512];                     // A big string buffer

  va_list args;                          // Print arguments to string
  va_start (args, format);
  vsprintf (buffer,format, args);
  va_end (args);

  // Set string in image display data
  YaIPS_ImageDispStrDebug( pYaIPS_ImageDisp, buffer);
}

/************************************************************************************
 * YaIPS_DialogAddCloseButton()
 *
 * Hack: Add close button to window caption.
 * At a close button to the right side of an caption.
 * Use this for none resizable modal windows used by YaIPS for parameter dialogs.
 * NOTE: MUST be called after show() call of this window.
 *
 */

#include <windows.h>

class FL_EXPORT Fl_X {
public:
  fl_uintptr_t xid;
  Fl_Window* w;
  Fl_Region region;
  Fl_X *next;
  // static variables, static functions and member functions
  static Fl_X* first;
  static Fl_X* flx(const Fl_Window* w) {return w ? (Fl_X*)w->flx_ : 0;}
#  if defined(FLTK_USE_X11) && FLTK_USE_X11 // for backward compatibility
  static void make_xid(Fl_Window*, XVisualInfo* =fl_visual, Colormap=fl_colormap);
  static Fl_X* set_xid(Fl_Window*, Window);
  static inline Fl_X* i(const Fl_Window* w) {return flx(w);}
#  endif
};
static HWND fl_xid (const Fl_Window* w)
{
  Fl_X *xTemp = Fl_X::flx(w);

  return xTemp ? (HWND)xTemp->xid : 0;
}

void YaIPS_DialogAddCloseButton( Fl_Window *pFlWin)
{
  HWND hwnd = (HWND)fl_xid( pFlWin);

  LONG style = GetWindowLong(hwnd, GWL_STYLE);

  style |= WS_SYSMENU;

  SetWindowLong(hwnd, GWL_STYLE, style);

  SetWindowPos(hwnd, nullptr, 0,0,0,0,
      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

/************************************************************************************
 * YaIPS_DialogRemoveMinMaxButton()
 *
 * Hack: Remove minimize and maximize buttons from the window caption.
 * REmove the minimize and maximize buttons from the right side of an caption.
 * Use this for resizable windows used by YaIPS for tool windows.
 * NOTE: MUST be called after show() call of this window.
 *
 */
void YaIPS_DialogRemoveMinMaxButton( Fl_Window *pFlWin)
{
  HWND hwnd = (HWND)fl_xid( pFlWin);

  LONG style = GetWindowLong(hwnd, GWL_STYLE);

  style &= ~ (WS_MAXIMIZEBOX | WS_MINIMIZEBOX);

  SetWindowLong(hwnd, GWL_STYLE, style);

  SetWindowPos(hwnd, nullptr, 0,0,0,0,
      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

/****************************** End Of File ******************************/




