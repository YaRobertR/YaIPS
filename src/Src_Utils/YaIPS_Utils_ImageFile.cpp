/****************************************************************************

  YaIPS_Utils_ImageFile.cpp

  Interface to read image from file or to write them to file.

 09.08.2025 RR: First edition of this file.

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

#ifdef USE_STB_IMAGE_FILES  // Use stb functions to read/write images

// Includes for image reading

#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Includes for image writing

#define STBIW_WINDOWS_UTF8
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#endif

/************************************************************************************
 * YaIPS_Image_Read
 *
 * Read image from file.
 *
 * Return:   NULL no image loaded
 *
 *
 */
Fl_RGB_Image * YaIPS_Image_Read( char *pFileName)
{
  Fl_RGB_Image *pTempImage;

#ifdef USE_STB_IMAGE_FILES  // Use stb functions to read/write images

  int x, y, n;
  unsigned char *data, *pNewImageData;

  pTempImage = NULL;                             // Preset no image loaded

  data = stbi_load( pFileName, &x, &y, &n, 0);   // Try to load image from file

  if( data == NULL) {                            // Have any error

    return( NULL);                               // Return error
  }

  if( n < 1 || n > 4) {                          // Security test for number of components

    stbi_image_free( data);                      // Release temporary image data

    return( NULL);                               // Return error
  }

  YaIPS_RGB_ImageSetSize( &pTempImage, x, y, n); // Create an empty image

  if( pTempImage == NULL) {                      // Error creating empty image

    stbi_image_free( data);                      // Release temporary image data

    return( NULL);                               // Return error
  }

  pNewImageData = (uchar *)pTempImage->data()[ 0];  // Pointer to new image data

  if( pNewImageData == NULL) {                   // Security test

    stbi_image_free( data);                      // Release temporary image data

    return( NULL);                               // Return error
  }

  memcpy( (void *)pNewImageData, data, x * y * n); // Copy the data

  stbi_image_free( data);                        // Release temporary image data

#else //  Use FLTK functions to read/write images

  pTempImage = (Fl_RGB_Image *)Fl_Shared_Image::get( pFileName);   // Try to load an image

#endif

  return( pTempImage);
}

/************************************************************************************
 * YaIPS_Image_Write
 *
 * Write image to file.
 *
 * Return:     0   OK
 *         NOT 0   Error code
 *
 */
int YaIPS_Image_Write( char *pFileName, Fl_RGB_Image *pImg)
{
  int ierr;

#ifdef USE_STB_IMAGE_FILES  // Use stb functions to read/write images

  int xx, yy, d;
  unsigned char *pD;

  // Get image data
  d  = pImg->d();
  xx = pImg->data_w();
  yy = pImg->data_h();
  //x/ld = pImg->ld() ? pImg->ld() : xx * d;
  pD = (uchar *)pImg->data()[ 0];

  ierr = YaIPS_Image_Write( pFileName, pD, xx, yy, d);

#else //  Use FLTK functions to read/write images

  ierr = fl_write_png( pFileName, pImg);

#endif

  return( ierr);
}

/************************************************************************************
 * YaIPS_Image_Write
 *
 * Write image to file.
 *
 * Return:     0   OK
 *         NOT 0   Error code
 *
 */
int YaIPS_Image_Write( char *pFileName, const unsigned char *pixels, int w, int h, int d)
{
  int ierr;

#ifdef USE_STB_IMAGE_FILES  // Use stb functions to read/write images

  char *pExtenstion;

  if( d < 1 || d > 4) {          // Check for reasonable size

    return( -102);
  }

  // Check extension.

  pExtenstion = strrchr( pFileName, '.');     // Point to last point

  if( pExtenstion == NULL) {                  // Have no point

    return( -103);
  }

  pExtenstion += 1;                           // Point after point

  if( stricmp( pExtenstion, "png") == 0) {

    ierr = stbi_write_png( pFileName, w, h, d, (void *)pixels, 0);

  } else if( stricmp( pExtenstion, "jpg") == 0) {

    ierr = stbi_write_jpg( pFileName, w, h, d, (void *)pixels, YaIPS_Setting_File_JPEG_Quality);

  } else if( stricmp( pExtenstion, "tga") == 0) {

    ierr = stbi_write_tga( pFileName, w, h, d, (void *)pixels);

  } else {

    return( -104);                             // File extension not known
  }

#else //  Use FLTK functions to read/write images

  ierr = fl_write_png( pFileName, pixels, w, h, d, ld);

#endif

  return( ierr);
}

/************************************************************************************
 * YaIPS_Image_Write_PNG
 *
 * Write PNG image to file.
 *
 * Return:     0   OK
 *         NOT 0   Error code
 *
 */
int YaIPS_Image_Write_PNG( char *pFileName, Fl_RGB_Image *pImg)
{
  int ierr;

#ifdef USE_STB_IMAGE_FILES  // Use stb functions to read/write images

  int xx, yy, d, ld;
  unsigned char *pD;

  // Get image data
  d  = pImg->d();
  xx = pImg->data_w();
  yy = pImg->data_h();
  ld = pImg->ld() ? pImg->ld() : xx * d;
  pD = (uchar *)pImg->data()[ 0];

  if( d < 1 || d > 4) {          // Check for reasonable size

    return( -102);
  }

  ierr = stbi_write_png( pFileName, xx, yy, d, (void *)pD, ld);


#else //  Use FLTK functions to read/write images

  ierr = fl_write_png( pFileName, pImg);

#endif

  return( ierr);
}

/************************************************************************************
 * YaIPS_Image_Write_PNG
 *
 * Write PNG image to file.
 *
 * Return:     0   OK
 *         NOT 0   Error code
 *
 */
int YaIPS_Image_Write_PNG( char *pFileName, const unsigned char *pixels, int w, int h, int d, int ld)
{
  int ierr;

#ifdef USE_STB_IMAGE_FILES  // Use stb functions to read/write images

  ierr = stbi_write_png( pFileName, w, h, d, pixels, ld);


#else //  Use FLTK functions to read/write images

  ierr = fl_write_png( pFileName, pixels, w, h, d, ld);

#endif

  return( ierr);
}

/************************************************************************************
 * YaIPS_Image_GetFilesInDir
 *
 * Get all image files in a directory
 *
 * Return:     0   OK
 *         NOT 0   Error code
 *
 */

// Helper functions for explorer like natural sor

static int read_number(const char **s) {
    int num = 0;
    while (isdigit(**s)) {
        num = num * 10 + (**s - '0');
        (*s)++;
    }
    return num;
}

static int strnatcmp(const char *a, const char *b) {
    while (*a && *b) {
        if (isdigit(*a) && isdigit(*b)) {
            int num1 = read_number(&a);
            int num2 = read_number(&b);
            if (num1 != num2)
                return num1 - num2;
        } else {
            if (*a != *b)
                return (unsigned char)*a - (unsigned char)*b;
            a++;
            b++;
        }
    }

#ifdef use_again
    if( *a == 0 && *b == 0) {

      return( 0);

    } else if( *a == 0) {

      return( 1);

    } else if( *b == 0) {

      return( -1);
    }
#endif

    return (unsigned char)*a - (unsigned char)*b;
}

int YaIPS_Image_GetFilesInDir( char *pDirName, struct dirent ***pFileList, int *pNumFiles)
{
  int num_files, i, LenName, AnySwap;
  char *pName;
  struct dirent *pDirEntry;

  if( fl_filename_isdir( pDirName) == 0) {       // Check path to be a directory

    return( -1);                                 // No directory
  }

  // Get file list in this directory

  *pNumFiles = 0;                                // Rest number of files found

  num_files = fl_filename_list( pDirName, pFileList);

  if( num_files >= 0) {   // No error

    // Scan for image files with known extension

    for( i = 0; i < num_files; i++) {

      pDirEntry = (*pFileList)[ i];

      pName = pDirEntry->d_name;

      if( pName[ 0] == '.') {                   // Is current directory or directory up

        // Free entry
        free( pDirEntry);

        continue;
      }

      if( IqeB_FileCheckExtension( pName, YAIPS_IMAGE_FILES_READ_KNOWN) != true) {

        // Extension is not known

        // Free entry
        free( pDirEntry);

        continue;
      }

      // Got a matching file extension
      // Keep this file name

      // Copy down pointer to file entry
      (*pFileList)[ *pNumFiles] = (*pFileList)[ i];

      // One more file
      *pNumFiles += 1;
    }

    if( *pNumFiles > 1) {            // Have tow or more files

      // Bubble sort the file names

      for( ; ; ) {

        AnySwap = false;

        for( i = 0; i < *pNumFiles - 1; i++) {

#ifdef use_again
          LenName = strnatcmp( pToolData->files[ i]->d_name, pToolData->files[ i + 1]->d_name);
#else
          char Name1[ FILENAME_MAX], Name2[ FILENAME_MAX];

          pDirEntry = (*pFileList)[ i];
          strcpy( Name1, pDirEntry->d_name);
          LenName = strlen( Name1);
          if( LenName >= 4 && Name1[ LenName - 4] == '.') {

            Name1[ LenName - 4] = '\0';
          }
          strupr( Name1);

          pDirEntry = (*pFileList)[ i + 1];
          strcpy( Name2, pDirEntry->d_name);
          LenName = strlen( Name2);
          if( LenName >= 4 && Name2[ LenName - 4] == '.') {

            Name2[ LenName - 4] = '\0';
          }
          strupr( Name2);

          LenName = strnatcmp( Name1, Name2);
#endif

          if( LenName > 0) {

            // Swap the entries

            AnySwap = true;

            pDirEntry = (*pFileList)[ i];
            (*pFileList)[ i] = (*pFileList)[ i + 1];
            (*pFileList)[ i + 1] = pDirEntry;
          }
        }

        if( ! AnySwap) {   // All sorted

          break;
        }
      }
    }
  }

  return( 0);    // return OK
}

/****************************** End Of File ******************************/
