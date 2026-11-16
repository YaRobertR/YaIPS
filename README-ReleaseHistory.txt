README-ReleaseHistory.txt
-------------------------

Release history of YaIPS.

------------------------------------------------------
 V  1.03  16.11.2026

* Reworked behavior of tool window parameter dialogs.
  The parameter dialogs of the tool windows can be operated
  parallel to other tool windows and the main window.
  Only one parameter dialog from a tool window can be open
  at a time. If a new parameter dialog is opened while
  another parameter dialog is still open, this dialog
  is automatically closed.
  This approach results in a more efficient workflow
  that requires less user interaction.
  
* Support for check masks in image comparison.
  Each reference image holds three check masks.
  For these masks, the 'inspection reference image' tool window
  has an editor for creating check regions for image
  comparison. The 'inspection compare' tool window has three
  checkboxes to take the check masks into account.
  For every set pixel within a checked check mask,
  the image comparison is done.
  
* Reworked color channel selection on the left area of the
  main window.
  The selectable color channels now depend on the color
  channels of the displayed image.
  
* Reworked the measurement tools on the left area of the
  main window.
  
  * Histogram
    Switch between RGB and IHS histograms
  * Brighness profiles
    Optional smooth brightness profiles
    Option to display the slope
    The measured profiles depends from the color channel selection
  * All
    Switch between thin and thick lines for the drawings
    
  The options can be accessed either via a menu button in
  the 'Info/Measurement' group or via a context menu when
  an AOI is selected.
  
------------------------------------------------------

11.11.2026 RR: * All tool windows.
                 * MyWinUpdate()
                   Parameter dialog button is disabled if the parameter dialog
                   is open.
                   
09.11.2026 RR: * YaIPS.h
               * YaIPS_GUI_InspCompare.cpp
               * YaIPS_GUI_InspRefImage.cpp
               * YaIPS_Utils_ImageCompare.cpp
               * YaIPS_Utils_Misc.cpp
                 * Added support for check masks in image comparison.
                   See: one of the defines YAIPS_INSP_COMP_FLAG_MASK,
                        YAIPS_REFMASK_CHECK_MASK_ or YAIPS_NOALPHADISP_MASK_
                        
24.10.2026 RR: * YaIPS_GUI_InspRefImage.cpp
                 * Begin with support for check masks in image comparison.
                   The plan is to use the alpha channel of the reference image
                   for check masks. There will be three check
                   masks. Each will be stored in its own bit plane in the alpha
                   channel. These masks will be made with an editor. 

               * YaIPS_RGB_Graphic_2D.cpp
                 * First edition of this file.
                   Draw graphics into image. Is used to draw check masks
                   into a bit layer of an reference image.
                                     
18.10.2026 RR: * YaIPS_GUI_Main.cpp
                 * Reworked behavior of tool window parameter dialogs.
                   New functions to allow tool window parameter dialogs
                   to be operable side by side with other tool window dialogs
                   and the main window.
                   Only one tool window parameter dialog can be open at
                   a time.
                   * IqeB_GUI_ToolParCheckOpen()
                     Called at begin of the function to create a tool window
                     parameter dialog.
                     This close an other open tool window parameter dialog.
                   * IqeB_GUI_ToolParCheckReset()
                     Called at the begin of a tool window parameter dialog
                     close function.
                     Marks no tool window parameter dialog is open.
                   * IqeB_GUI_ToolParCheckClose()
                     This is called from the close function of a tool windows.
                     Closes a parameter dialog associated with this
                     tool window.
                     
               * All tool windows.
                 * Reworked behavior of tool window parameter dialogs.
                   See functions IqeB_GUI_ToolParCheckOpen(),
                   IqeB_GUI_ToolParCheckReset() and IqeB_GUI_ToolParCheckClose().
                     
15.10.2026 RR: * YaIPS_GUI_Main.cpp
                 * YaIPS_GUI_pMouseContext_cb()
                   New call back function. This function pops up a menu
                   to change features for pixel value, brightness profiles,
                   histograms an thick line drawings.
                   This can be called either as a context menu when the
                   mouse pointer is positioned over a selection rectangle
                   within the image in the central area, or by clicking
                   the button in the 'Info/Measurement' group.

14.10.2026 RR: * YaIPS_Utils_Histo.cpp
                 * YaIPS_Histo_Measure()
                 * YaIPS_Histo_Measure2()
                   If the 'YAIPS_HISTO_FLAG_IHS' flag is set,
                   an IHS histogram is generated instead of an RGB histogram.
                 * YaIPS_Histo_Draw()
                   Added code to also draw an IHS histogram.
                   
               * YaIPS_RGB_Color.cpp
                 * Bugfix YaIPS_RGB_Color_IHS2RGB()
                    Accessing the tables YaIPS_ColMod_sintab and YaIPS_ColMod_costab
                    with a value of ch = 64 us an index of 64 (A180 - 64 = 128 - 64 = 64)
                    access one element after then end of the tables.
                    HACK: Added one element to the tables replicating the last element.

12.10.2026 RR: *  YaIPS_Utils_ColRowSum.cpp
                  YaIPS_ColRowSum_SingleDraw()
                  Added coded to draw the brightness profile smoothed
                  and optional draw the slopes of the profiles.
                   
09.10.2026 RR: * YaIPS_Utils_ColRowSum.cpp
                 * YaIPS_ColRowSum_Measure()
                   Added argument color channel to function.
                  This also allows to make profiles from one color channel
                  or from the alpha channel.
                  
               * YaIPS_GUI_Main.cpp
                 * YaIPS_GUI_DiffUsesCalcThings()
                   To call of YaIPS_ColRowSum_Measure() at the current selected
                   color channel to the arguments.
                   If a brightness profile is shown, the measured profiles
                   depends from the color channel selection.
                 * MyWinUpdate()
                   Enables for color channel selection depends from image
                   type. Only for color images the color channels are
                   enabled.

------------------------------------------------------
 V  1.02  07.10.2026

* The "Calculation" tool window has been updated with the feature
  "Combine Image with Constants".
  
* The "Geometry" tool window has been updated with the features
  "Trapezoidal distortion" and "Perspective transformation".
  
* Improvements to image downsizing.
  Whenever appropriate, the source image is scaled down
  by powers of two. As a result, the subsequent
  nearest-neighbor resizing produces better results.
  This enhancement is used by:
  * Tool 'Geometry', resize image by % and parallel projection.
  * Tool 'New Image' when using an image as a background.
  * Tool 'Video writer' when using large input images.
  * Tool 'Overlay' when using images or tool windows as overlays.

* A "Reset" button has been added to the upper-right corner of the
  left area of the main window.
  Clicking this button resets the display settings.
  
* Relaxed criteria for moving tool windows in the right area of the
  main window along with the main window.
  Previously, the entire tool window had to be located in the right
  area of the main window. Now, it is sufficient for the center of
  the tool window to be located in the right area.
  
------------------------------------------------------

07.10.2026 RR: * YaIPS_RGB_GeoTransform.cpp
                 * YaIPS_RGB_Geo_Resize2()
                   Optimize speed by eliminate inner loop of resize code.

05.10.2026 RR: * YaIPS_RGB_GeoTransform.cpp
                 * Finished coding for YaIPS_RGB_Geo_Warp_4_Points().
                   Perspective transformation with 4 points in source
                   and 4 points in destination.

               * YaIPS_GUI_GeoTransform.cpp
                 * Finished coding for tools
                   * Trapezoidal distortion
                   * Warp, applies a perspective transformation to an image.

30.09.2026 RR: * YaIPS_RGB_ShapeGen.cpp
                 * YaIPS_RGB_CopyBgndToOverlay()
                   Optimization when resizing images.
                   Whenever appropriate, the source image is scaled down
                   by powers of two. As a result, the subsequent
                   nearest-neighbor resizing produces better results.

29.09.2026 RR: * YaIPS_RGB_GeoTransform.cpp
                 * YaIPS_RGB_Geo_Resize2()
                   New function with separate arguments for X and Y scaling.
                 * YaIPS_RGB_Geo_Transform()
                   Optimization when resizing images.
                   Whenever appropriate, the source image is scaled down
                   by powers of two. As a result, the subsequent
                   nearest-neighbor resizing produces better results.

27.09.2026 RR: * YaIPS_GUI_Combine.cpp
                 Finished coding for calculation with constants.
                 
               * YaIPS_RGB_Combine.cpp
                 Finished coding for calculation with constants.
                 See: YaIPS_RGB_CalcConst()

23.09.2026 RR: * YaIPS_GUI_Main.cpp
                 * IqeB_MainWindow_GUI_Setup().
                   Added 'Reset' button into right upper corner of
                   the windows left area.
                   Pressing the button resets the display settings.
                   
               * YaIPS_Utils_Pref_Win_Manag.cpp
                 * YaIPS_WindowsToolWinAddPosDelta()
                   Relaxed criteria for moving tool windows in the right
                   area of the main window along with the main window.
                   Previously, the entire tool window had to be located
                   in the right area of the main window. Now, it is
                   sufficient for the center of the tool window to be
                   located in the right area.

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

                 
                  