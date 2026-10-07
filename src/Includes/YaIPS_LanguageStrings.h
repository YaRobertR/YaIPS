/****************************************************************************

  YaIPS_LanguageStrings.h

  24.07.2025 RR: First edition of this file.

 Centralized language defines.

 This defines are placed her for use my the source modules.
 The call of LangStringLookup() is needed to get translation
 support.

*****************************************************************************
*/

#ifndef YAIPS_LANGUAGE_STRINGS_H_
#define YAIPS_LANGUAGE_STRINGS_H_

/************************************************************************************
 * Needed external definition
 */

// Lookup a language string
char * LangStringLookup( char *pString);
char * LangStringLookup( const char *pString);

/************************************************************************************
 * Strings for GUI
 */

// Defines for GUI buttons

#define LANGDEF_BUTTON_CLOSE          LangStringLookup( "&Button_Close=Close")      // Button close
#define LANGDEF_BUTTON_OK             LangStringLookup( "&Button_OK=OK")            // Button OK
#define LANGDEF_BUTTON_RESET          LangStringLookup( "&Button_Reset=Reset")      // Button Reset
#define LANGDEF_BUTTON_RESET_SHORT    LangStringLookup( "&Button_Reset_Short=R")    // Button Reset short
#define LANGDEF_BUTTON_CANCEL         LangStringLookup( "&Button_Cancel=Cancel")    // Button Cancel
#define LANGDEF_BUTTON_NO             LangStringLookup( "&Button_No=No")            // Button No
#define LANGDEF_BUTTON_YES            LangStringLookup( "&Button_Yes=Yes")          // Button Yes
#define LANGDEF_BUTTON_APPLY          LangStringLookup( "&Button_Apply=Apply")      // Button Apply

// Other defines

#define LANGDEF_SETTINGS              LangStringLookup( "&Text_Settings=Settings")
#define LANGDEF_SETTINGS_POINTS       LangStringLookup( "&Text_SettingsPoints=Settings ...")
#define LANGDEF_CLIPBOARD             LangStringLookup( "&Text_Clipboard=Clipboard")
#define LANGDEF_GRAY_VALUE_SHORT      LangStringLookup( "&Text_GrayValueShort=Gv")     // Grey value
#define LANGDEF_ACTIVE_SHORT          LangStringLookup( "&Text_ActiveShort=A")         // Used for active checkbox
#define LANGDEF_REVERSE               LangStringLookup( "&Text_Reverse=Reverse")
#define LANGDEF_ALL                   LangStringLookup( "&Text_All=All")
#define LANGDEF_X                     LangStringLookup( "&Text_X=X")
#define LANGDEF_Y                     LangStringLookup( "&Text_Y=Y")
#define LANGDEF_PIXEL_UCF             LangStringLookup( "&Text_Pixel_UCF=Pixel")       // Upper case first
#define LANGDEF_PIXEL_LC              LangStringLookup( "&Text_Pixel_LC=pixel")        // Lower case

// Color
#define LANGDEF_COLOR                 LangStringLookup( "&Text_Color=Color")
#define LANGDEF_COLOR_R               LangStringLookup( "&Text_ColorR=R")
#define LANGDEF_COLOR_G               LangStringLookup( "&Text_ColorG=G")
#define LANGDEF_COLOR_B               LangStringLookup( "&Text_ColorB=B")
#define LANGDEF_COLOR_A               LangStringLookup( "&Text_ColorA=A")
#define LANGDEF_COLOR_BW              LangStringLookup( "&Text_ColorBW=BW")     // Black white

#define LANGDEF_COLOR_ALPHA           LangStringLookup( "&Text_ColorAlpha=Alpha")

#define LANGDEF_COLOR_RED_FOR_ALL     LangStringLookup( "&Text_ColorRedForAll=If set, the red value is used\nfor all three channels.")


// Pixel depth

#define LANGDEF_PIXDEPTH_1_TXT        LangStringLookup( "&Text_PixDepth1txt=BW")
#define LANGDEF_PIXDEPTH_2_TXT        LangStringLookup( "&Text_PixDepth2txt=BW+A")
#define LANGDEF_PIXDEPTH_3_TXT        LangStringLookup( "&Text_PixDepth3txt=RGB")
#define LANGDEF_PIXDEPTH_4_TXT        LangStringLookup( "&Text_PixDepth4txt=RGB+A")

// Color channel
#define LANGDEF_COL_CHANNEL           LangStringLookup( "&Text_ColChannel=Color channel")
#define LANGDEF_COL_CHANNEL_DPOINT    LangStringLookup( "&Text_ColChannelDP=Color channel:")
#define LANGDEF_COL_CHANNEL_R         LangStringLookup( "&Text_ColChannelR=Red color channel")
#define LANGDEF_COL_CHANNEL_G         LangStringLookup( "&Text_ColChannelG=Green color channel")
#define LANGDEF_COL_CHANNEL_B         LangStringLookup( "&Text_ColChannelB=Blue color channel")
#define LANGDEF_COL_CHANNEL_A         LangStringLookup( "&Text_ColChannelA=Alpha channel")

#define LANGDEF_COL_MULT_R1           LangStringLookup( "&Text_ColMultR1=+ R *")
#define LANGDEF_COL_MULT_G1           LangStringLookup( "&Text_ColMultG1=+ G *")
#define LANGDEF_COL_MULT_B1           LangStringLookup( "&Text_ColMultB1=+ B *")
#define LANGDEF_COL_MULT_R2           LangStringLookup( "&Text_ColMultR2=Multiplier for the red color channel")
#define LANGDEF_COL_MULT_G2           LangStringLookup( "&Text_ColMultG2=Multiplier for the green color channel")
#define LANGDEF_COL_MULT_B2           LangStringLookup( "&Text_ColMultB2=Multiplier for the blue color channel")

// Short text before some image selection pull down elements
// Use only one letter!

#define LANGDEF_IMGSEL_PDS_1           LangStringLookup( "&Text_ImgSel_PDS_1=1")
#define LANGDEF_IMGSEL_PDS_2           LangStringLookup( "&Text_ImgSel_PDS_2=2")
#define LANGDEF_IMGSEL_PDS_3           LangStringLookup( "&Text_ImgSel_PDS_3=3")
#define LANGDEF_IMGSEL_PDS_INPUT       LangStringLookup( "&Text_ImgSel_PDS_Input=I")
#define LANGDEF_IMGSEL_PDS_REF         LangStringLookup( "&Text_ImgSel_PDS_Ref=R")
#define LANGDEF_IMGSEL_PDS_SCENE       LangStringLookup( "&Text_ImgSel_PDS_Scene=S")
#define LANGDEF_IMGSEL_PDS_OBJECT      LangStringLookup( "&Text_ImgSel_PDS_Object=O")

// Test input and reference image to exist and have same sizes.
#define LANGDEF_IMGSEL_ERR_NO_INP      LangStringLookup( "&Text_ImgSel_Err_NoInp='Input image' missing.")
#define LANGDEF_IMGSEL_ERR_NO_REF      LangStringLookup( "&Text_ImgSel_Err_NoRef='Reference image' missing.")
#define LANGDEF_IMGSEL_ERR_NONE        LangStringLookup( "&Text_ImgSel_Err_None=Both images missing.")
#define LANGDEF_IMGSEL_ERR_SIZES       LangStringLookup( "&Text_ImgSel_Err_ErrSizes=Image sizes are different.")
#define LANGDEF_IMGSEL_ERR_BPP         LangStringLookup( "&Text_ImgSel_Err_ErrBPP=Images color/BW mismatch")

// AOI
#define LANGDEF_AOI_LEFT              LangStringLookup( "&Text_AoiLeft=AOI Left")
#define LANGDEF_AOI_LEFT_TOOLTIP      LangStringLookup( "&Text_AoiLeftTooltip=Position left edge AOI")
#define LANGDEF_AOI_TOP               LangStringLookup( "&Text_AoiTop=Top")
#define LANGDEF_AOI_TOP_TOOLTIP       LangStringLookup( "&Text_AoiTopTooltip=Position top edge AOI")
#define LANGDEF_AOI_HEIGHT            LangStringLookup( "&Text_AoiHeight=Height")
#define LANGDEF_AOI_HEIGHT_TOOLTIP    LangStringLookup( "&Text_AoiHeightTooltip=Height of AOI")
#define LANGDEF_AOI_WIDTH             LangStringLookup( "&Text_AoiWidth=Width")
#define LANGDEF_AOI_WIDTH_TOOLTIP     LangStringLookup( "&Text_AoiWidthTooltip=Width of AOI")
#define LANGDEF_AOI_TEACH_TOOLTIP     LangStringLookup( "&Text_AoiTeachTooltip="                                     \
                                                        "AOI = Area of Interest.\n"                             \
                                                        "Display AOI in scene view and change AOI parameters.\n"  \
                                                        "The AOI area can be adjusted with the mouse.")

// LUT
#define LANGDEF_LUT_SHORT             LangStringLookup( "&Text_LUT=LUT")
#define LANGDEF_LUT_TOOLTIP           LangStringLookup( "&Text_LUT_Tooltip="                   \
                                                        "False color representation of a\n"    \
                                                        "black and white or color image.")

// File load/save

#define LANGDEF_FILE_LOAD_IMAGE       LangStringLookup( "&Text_File_Load_Image=Load image")
#define LANGDEF_FILE_LOAD_VIDEO       LangStringLookup( "&Text_File_Load_Video=Load video")

// Calibration

#define LANGDEF_CALIB_UNIT_UNLNOWN   LangStringLookup( "&Text_Calib_Unit_Unknown=???")
#define LANGDEF_CALIB_UNIT_PIXEL     LangStringLookup( "&Text_Calib_Unit_Pixel=Pixel")
#define LANGDEF_CALIB_UNIT_MM        LangStringLookup( "&Text_Calib_Unit_mm=mm")
#define LANGDEF_CALIB_UNIT_CM        LangStringLookup( "&Text_Calib_Unit_cm=cm")
#define LANGDEF_CALIB_UNIT_INCH      LangStringLookup( "&Text_Calib_Unit_inch=inch")

// Inspection tools

#define LANGDEF_INSP_WIN_INP         LangStringLookup( "&Text_Insp_Win_Inp="                     \
                                                       "Input image. This image is processed.\n" \
                                                       "BOTH images must have the same size.")

#define LANGDEF_INSP_WIN_REF         LangStringLookup( "&Text_Insp_Win_Ref="                               \
                                                       "Reference image. Holds reference AOI positions.\n" \
                                                       "BOTH images must have the same size.")

// Others
#define LANGDEF_INPUT                 LangStringLookup( "&Text_Input=Input")
#define LANGDEF_IMAGES                LangStringLookup( "&Text_Images=Images")
#define LANGDEF_VIDEOS                LangStringLookup( "&Text_Videos=Videos")
#define LANGDEF_IMAGE_NOT_CHANGED     LangStringLookup( "&Text_ImageNotChanged=Image unchanged")
#define LANGDEF_SHOW_ON_BIG_IMAGE     LangStringLookup( "&Text_ShowOnBigImage=Display image on big window.")
#define LANGDEF_SELECT_INPUT_IMAGE    LangStringLookup( "&Text_SelectInputImage=Selecting an input image.")
#define LANGDEF_SWITCH_TEACH_INSPECT  LangStringLookup( "&Text_SwitchTeachInspect=Switching between teach mode and inspection mode.")
#define LANGDEF_ROTATION              LangStringLookup( "&Text_Rotation=Rotation")
#define LANGDEF_INVERTED              LangStringLookup( "&Text_Inverted=Inverted")

#define LANGDEF_ERROR_CODE            LangStringLookup( "&Text_Error_Code=Error %d!")

/************************************************************************************
 * Error strings for image processing functions.
 */

#define ERR_IPS_PARAM_ILLEGAL          LangStringLookup( "&ErrIps_ParamIllegal=Illegal parameter")
#define ERR_IPS_PARAM_BAD              LangStringLookup( "&ErrIps_ParamBad=Bad parameter")
#define ERR_IPS_NO_SUPP                LangStringLookup( "&ErrIpsNoSupp=Not supported!")   // "Nicht unterstützt!"

#define ERR_IPS_FB_NO_FB               LangStringLookup( "&ErrIps_fbnofb =No such FB")
#define ERR_IPS_FB_CLOSE               LangStringLookup( "&ErrIps_fbclose=FB not open")
#define ERR_IPS_FB_OFF                 LangStringLookup( "&ErrIps_fboff=FB is off")
#define ERR_IPS_FB_NO_SEL              LangStringLookup( "&ErrIps_fbnsel=no FB selected")
#define ERR_IPS_FB_NO_SUPP             LangStringLookup( "&ErrIps_fbsupp=FB not supported by standard SIP")

#define ERR_IPS_DATA_COMP_NO_SUPP      LangStringLookup( "&ErrIps_support=This data type combination is not supported")
#define ERR_IPS_NO_NAME                LangStringLookup( "&ErrIps_noname=No name defined")
#define ERR_IPS_DEVICE_NOT_FOR_THIS_OP LangStringLookup( "&ErrIps_DdviceNotForThisOp=Device not allowed for this operation")

#define ERR_IPS_DATA_TYPE_NO_STD       LangStringLookup( "&ErrIps_DataTyeNoStd=No standard data type")
#define ERR_IPS_DATA_TYPE_ILLEGAL      LangStringLookup( "&ErrIps_DataTyeIllegal=Illegal data type")
#define ERR_IPS_DATA_TYPE_CONFLICT     LangStringLookup( "&ErrIps_DataTyeConflict=Data type conflict")

#define ERR_IPS_NO_MEM                 LangStringLookup( "&ErrIps_nomem=Out of memory")
#define ERR_IPS_MEM_OVERFLOW           LangStringLookup( "&ErrIps_MemOverflow=Memory overflow")
#define ERR_IPS_MEM_BAD_SIZE           LangStringLookup( "&ErrIps_MemBadSizhe=Bad sizes for memory alloc")

#define ERR_IPS_IMG_CREATE             LangStringLookup( "&ErrIps_ImgCreate=Failed to create image")
#define ERR_IPS_IMG_PTR_NULL           LangStringLookup( "&ErrIps_ImgUnknown=Image pointer is zero")
#define ERR_IPS_IMG_BAD_TYPE           LangStringLookup( "&ErrIps_ImgBadType=Image is of bad data type")
#define ERR_IPS_IMG_NOT_INSIDE_MEM     LangStringLookup( "&ErrIps_ImgNotIsideMem=Image not inside memory")

#define ERR_IPS_VEC_CREATE             LangStringLookup( "&ErrIps_vcreate=Failed to create vector")
#define ERR_IPS_VEC_ALLOC              LangStringLookup( "&ErrIps_valloc=Error allocating vector data")
#define ERR_IPS_VEC_EMPTY              LangStringLookup( "&ErrIps_vempty=Vector is empty")
#define ERR_IPS_VEC_BAD_NO             LangStringLookup( "&ErrIps_vbadno=Wrong number of items in vector")
#define ERR_IPS_VEC_ILL_ID             LangStringLookup( "&ErrIps_villid=Illegal vector ID")
#define ERR_IPS_VEC_PTR_NULL           LangStringLookup( "&ErrIps_VecUnknown=Vector pointer is zero")
#define ERR_IPS_VEC_DEVICE_WRONG       LangStringLookup( "&ErrIps_VecDeviceWrong=Vector on wrong device")
#define ERR_IPS_VEC_BAD_TYPE           LangStringLookup( "&ErrIps_VecBadType=Vector is of bad data type")
#define ERR_IPS_VEC_TOO_SHORT          LangStringLookup( "&ErrIps_VecTooShort=Vector too short")
#define ERR_IPS_VEC_MORE_32676         LangStringLookup( "&ErrIps_VecMore32676=Vector has more than 32676 values")

#define ERR_IPS_XS_DST_OVERFLOW        LangStringLookup( "&ErrIps_XSizeDstOverflow=X-size dst overflow")
#define ERR_IPS_XS_SRC_OVERFLOW        LangStringLookup( "&ErrIps_XSizeSrcOverflow=X-size src overflow")

#define ERR_IPS_YSIZE_SRC_DST_DIFF     LangStringLookup( "&ErrIps_YSizeSrcDstDifferent=Y-size of src and dst different")
#define ERR_IPS_XA_XE_RLC_ILLEGAL      LangStringLookup( "&ErrIps_XaXeRlcIllegal=Illegal xa, xe address in runlength code")
#define ERR_IPS_LABEL_NOT_FOUND        LangStringLookup( "&ErrIps_LabelNotFound=No label found")
#define ERR_IPS_OBJ_BUF_OVERFLOW       LangStringLookup( "&ErrIps_ObjBufOverflow=Object buffer overflow")

#endif /* YAIPS_LANGUAGE_STRINGS_H_ */

/****************************** End Of File ******************************/
