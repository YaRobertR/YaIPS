README-ReleaseHistory.txt
-------------------------

Release history of YaIPS.

------------------------------------------------------
 V  1.01  16.09.2026

* Reworked clipboard handling
  The “Images\YaIPS\Clipboard” subdirectory is used as the clipboard. 
  Pasted images, text or other tool window-specific data are stored here.
  When a preset is saved, the clipboard contents are copied to a 
  subdirectory of “Presets\.Clipboard”. This subdirectory has the same 
  name as the saved preset.
  Loading a preset file restores the clipboard files.
  
* File paths
  File paths are stored relative to the application's working directory,
  if possible.
  This makes preset files more portable. Now it makes sense to use them
  on another computer.
  
* Create a poster
  A new tool has been added to the “Other” tool window.

* A few emojis from OpenMoji were added to the images in the “Image”
  directory.
     https://openmoji.org
  Open source emojis for designers, developers and everyone else!

------------------------------------------------------

16.09.2026 RR: * YaIPS_RGB_Posterization.cpp
                 Done with final improvements.    
                 
               * YaIPS_GUI_Settings.cpp
                 The code for converting an image into a poster is now complete.
                 
11.09.2026 RR * YaIPS_RGB_Posterization.cpp
                Convert an image into a poster.
                First edition of this file.

08.09.2026 RR * YaIPS_Utils_Misc.cpp
                * New function IqeB_FileCopyFilesInDir().
                  Copy files in a directory to an other directory.
                * New function IqeB_FileDelFilesInDir().
                  Delete files in a directory.
                  
              * YaIPS_Utils_Pref_Win_Manag.cpp
                * IqeB_PresetSave_cb()
                  Added coded to save the clip board files. The files
                  are stored in a sub directory of the "Preset" directory.
                  This sub directory has the name of the saved preset file.
                * IqeB_PresetLoad_cb()
                  * Restore clipboard files.
                  * if 'pValueArg' is != 0 reset presets.
                * Added IqeB_PresetCleanClipboard()
                  Clean not used clipboard subdirectories.
                  
              * YaIPS.h
                * Added reference to
                  * IqeB_FileCopyFilesInDir()
                  * IqeB_FileDelFilesInDir()
                  * IqeB_PresetCleanClipboard()
                  
              * YaIPS_GUI_Main.cpp
                * Added menu entry 'File/Reset presets'.
                  Call the call back IqeB_Main_PresetLoad() with the
                  argument 'pValueArg' set to 1.
                * IqeB_Main_PresetLoad()
                  Handle 'pValueArg' over to call of function IqeB_PresetLoad_cb().
                * Function main()
                  Added call to IqeB_PresetCleanClipboard().
                  Clean not used clipboard subdirectories.
  
03.09.2026 RR * Rework use of current working directory. Replace
                use of the working directory string
                'YaIPS_WorkingDirectory' by '.'.
                The goal is to store paths to files relative to
                the local working directory, whenever possible.
                Note: The application's working directory is always
                      located at the root of the YaIPS file system.
                Changed files: 
                * YaIPS_GUI_CustomColors.cpp
                * YaIPS_GUI_Image_File.cpp
                * YaIPS_GUI_Image_Generate.cpp
                * YaIPS_GUI_InspRefImage.cpp
                * YaIPS_GUI_Overlay.cpp
                * YaIPS_GUI_Settings.cpp
                * YaIPS_Utils_Pref_Win_Manag.cpp
                * YaIPS_Utils_Language.cpp
                
              * YaIPS_GUI_Main.cpp
                * Function main()
                  * Ensure that 'YaIPS_WorkingDirectory' has normalized
                    path characters.
                  * Ensure clip board directory is created
                * IqeB_Main_SaveCopy_Callback()
                  Ensure normalized path characters and working directory
                  for files to save.
                
              * YaIPS_Utils_Misc.cpp
                * New function IqeB_FileNormPathCharsAndCWD().
                  Normalize path characters and current working directory.
                  Replace the begin of the path string with '.'
                  if the begin is equal to the 'YaIPS_WorkingDirectory'.
                * New function IqeB_DirExsits().
                  Test for a directory to exists.
                
              * YaIPS.h
                * Added reference to IqeB_FileNormPathCharsAndCWD().
                * Added reference to IqeB_DirExsits().
                * Reworked clipboard handling
                  Added defines YaIPS_CLIPBOARD_DIR and YaIPS_CLIPBOARD_PATH.
                
              * YaIPS_GUI_Image_File.cpp
                * Replace use of the working directory string
                  'YaIPS_WorkingDirectory' by '.'.
                * Function Load_cb()
                * Function DropFile_cb()
                * Function FilePrev_cb()
                * Function FileNext_cb()
                * Function YaIPS_GUI_MyChangeOutput()
                  Ensure normalized path characters and working directory
                  for last loaded file.
                * Reworked clipboard handling
                  Replaced patch './Images/YaIPS' with YaIPS_CLIPBOARD_PATH.

              * YaIPS_GUI_VideoRead.cpp
                * Function VideoLoadFile()
                  Ensure normalized path characters and working directory
                  for last loaded file.
                  
              * YaIPS_GUI_VideoWrite.cpp
                * Function VideoWriteFileOpen()
                  Ensure normalized path characters and working directory
                  for last written file.
                  
              * YaIPS_GUI_Overlay.cpp
                * Replace use of the working directory string
                  'YaIPS_WorkingDirectory' by '.'.
                * IqeB_GUI_BGndFileLoad_Callback()
                * DropFile_cb()
                  Ensure normalized path characters and working directory
                  for last loaded background file.
                * Reworked clipboard handling
                  Replaced patch './Images/YaIPS' with YaIPS_CLIPBOARD_PATH.
                  
              * YaIPS_GUI_InspRefImage.cpp
                * Replace use of the working directory string
                  'YaIPS_WorkingDirectory' by '.'.
                * Reworked clipboard handling
                  Replaced patch './Images/YaIPS' with YaIPS_CLIPBOARD_PATH.
                  
              * YaIPS_GUI_Image_Generate.cpp
                * Replace use of the working directory string
                  'YaIPS_WorkingDirectory' by '.'.
                * Reworked clipboard handling
                  Replaced patch './Images/YaIPS' with YaIPS_CLIPBOARD_PATH.
             
------------------------------------------------------
 V  1.00  21.08.2026

          First release of this package on GitHub.
------------------------------------------------------

                 
                  