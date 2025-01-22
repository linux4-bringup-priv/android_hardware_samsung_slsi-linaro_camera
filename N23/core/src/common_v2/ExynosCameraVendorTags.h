/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef EXYNOS_CAMERA_SLSI_VENDOR_TAGS_H
#define EXYNOS_CAMERA_SLSI_VENDOR_TAGS_H


#include <system/camera_metadata.h>
#include <system/camera_vendor_tags.h>

namespace android {

#define EXYNOS_SECTION VENDOR_SECTION

enum exynos_ext_section {
    EXYNOS_ANDROID_EXTENSION_SECTION_START = EXYNOS_SECTION,
    EXYNOS_ANDROID_CONTROL = EXYNOS_SECTION,
    EXYNOS_ANDROID_VENDOR_SENSOR,
    EXYNOS_ANDROID_VENDOR_ENVINFO,
    EXYNOS_ANDROID_VENDOR_CONTROL,
    EXYNOS_ANDROID_VENDOR_STATS,
    EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP,
    EXYNOS_ANDROID_VENDOR_SNAPSHOT,
    EXYNOS_ANDROID_VENDOR_FACTORY,
    EXYNOS_ANDROID_VENDOR_DUALCAM,
    EXYNOS_ANDROID_VENDOR_REMOSAIC,
    EXYNOS_ANDROID_VENDOR_BEAUTY_FACE,
    EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE,
    EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE,
    EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE,
    EXYNOS_ANDROID_EXTENSION_SECTION_END,
};

typedef struct camera_metadata_struct_exynos_android_scene_detection_info {
    int64_t timestamp;
    int64_t scene_index;
    int64_t confidence_score;
    int64_t object_roi_left;
    int64_t object_roi_top;
    int64_t object_roi_width;
    int64_t object_roi_height;
} __attribute__((packed)) scene_detection_info_t;

const int EXYNOS_ANDROID_EXTENSION_SECTION_COUNT = EXYNOS_ANDROID_EXTENSION_SECTION_END - EXYNOS_ANDROID_EXTENSION_SECTION_START;

enum exynos_ext_section_ranges {
    EXYNOS_ANDROID_CONTROL_START            = EXYNOS_ANDROID_CONTROL << 16,
    EXYNOS_ANDROID_VENDOR_SENSOR_START      = EXYNOS_ANDROID_VENDOR_SENSOR << 16,
    EXYNOS_ANDROID_VENDOR_ENVINFO_START     = EXYNOS_ANDROID_VENDOR_ENVINFO << 16,
    EXYNOS_ANDROID_VENDOR_CONTROL_START     = EXYNOS_ANDROID_VENDOR_CONTROL << 16,
    EXYNOS_ANDROID_VENDOR_STATS_START       = EXYNOS_ANDROID_VENDOR_STATS << 16,
    EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_START = EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP << 16,
    EXYNOS_ANDROID_VENDOR_SNAPSHOT_START    = EXYNOS_ANDROID_VENDOR_SNAPSHOT << 16,
    EXYNOS_ANDROID_VENDOR_FACTORY_START     = EXYNOS_ANDROID_VENDOR_FACTORY << 16,
    EXYNOS_ANDROID_VENDOR_DUALCAM_START     = EXYNOS_ANDROID_VENDOR_DUALCAM << 16,
    EXYNOS_ANDROID_VENDOR_REMOSAIC_START     = EXYNOS_ANDROID_VENDOR_REMOSAIC << 16,
    EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_START = EXYNOS_ANDROID_VENDOR_BEAUTY_FACE << 16,
    EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_START = EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE << 16,
    EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE_START = EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE << 16,
    EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_START = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE << 16,
};

typedef enum enum_exynos_mf_still_mode {
	SLSI_MF_STILL_MODE_OFF,
    SLSI_MF_STILL_MODE_ONDEMAND
} enum_exynos_mf_still_mode;

typedef enum enum_exynos_mf_still_capture {
    SLSI_MF_STILL_FUNCTIONS_NONE,
    SLSI_MF_STILL_FUNCTIONS_LLS,
    SLSI_MF_STILL_FUNCTIONS_SR,
    SLSI_MF_STILL_FUNCTIONS_LLS_FAILED,
    SLSI_MF_STILL_FUNCTIONS_SR_FAILED
} enum_exynos_mf_still_capture_t;

static uint8_t AVAILABLE_VENDOR_MF_STILL_MODE[] = {
	SLSI_MF_STILL_MODE_OFF,
    SLSI_MF_STILL_MODE_ONDEMAND,
};

static uint8_t AVAILABLE_VENDOR_MF_STILL_CAPTURE[] = {
    SLSI_MF_STILL_FUNCTIONS_NONE,
    SLSI_MF_STILL_FUNCTIONS_LLS,
    SLSI_MF_STILL_FUNCTIONS_SR,
};

static uint8_t AVAILABLE_VENDOR_MF_STILL_RESULT[] = {
    SLSI_MF_STILL_FUNCTIONS_NONE,
    SLSI_MF_STILL_FUNCTIONS_LLS,
    SLSI_MF_STILL_FUNCTIONS_SR,
    SLSI_MF_STILL_FUNCTIONS_LLS_FAILED,
    SLSI_MF_STILL_FUNCTIONS_SR_FAILED,
};

typedef enum enum_exynos_remosaic_capture {
    EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_NONE,
    EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_ON_HW, /* HW remosaic */
    EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_ON_SW, /* SW remosaic */
} enum_exynos_remosaic_capture_t;

typedef enum enum_exynos_night_shot {
    EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_NONE,
    EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON,
} enum_exynos_night_shot_t;

typedef enum enum_exynos_hdr {
    EXYNOS_ANDROID_VENDOR_HDR_NONE,
    EXYNOS_ANDROID_VENDOR_HDR_ON,
} enum_exynos_hdr_t;

typedef enum enum_exynos_ois {
    EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV_OFF,
    EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV_ON,
} enum_exynos_ois_t;

typedef enum enum_exynos_mode {
    EXYNOS_ANDROID_VENDOR_MODE_NONE,
    EXYNOS_ANDROID_VENDOR_MODE_ON,
} enum_exynos_mode_t;

typedef enum camera_metadata_enum_exynos_control_hdr_mode {
    EXYNOS_CONTROL_HDR_MODE_OFF = 0,
    EXYNOS_CONTROL_HDR_MODE_ON,
    EXYNOS_CONTROL_HDR_MODE_AUTO
} camera_metadata_enum_exynos_control_hdr_mode_t;

typedef enum enum_exynos_control_long_exposure_capture {
    EXYNOS_CONTROL_LONG_EXPOSURE_CAPTURE_OFF = 0,
    EXYNOS_CONTROL_LONG_EXPOSURE_CAPTURE_ON
} enum_exynos_control_long_exposure_capture_t;

typedef enum enum_exynos_session_mode {
    EXYNOS_SESSION_MODE_OFF = 0,
    EXYNOS_SESSION_MODE_PRO,
    EXYNOS_SESSION_MODE_REMOSAIC,
    EXYNOS_SESSION_MODE_LED_CAL, // LED calibration
} enum_exynos_session_mode_t;

typedef enum enum_exynos_combine_preview_plugin {
    EXYNOS_COMBINE_PREVIEW_PLUGIN_DISABLE = 0,
    EXYNOS_COMBINE_PREVIEW_PLUGIN_ENABLE = 1,
} enum_exynos_combine_preview_plugin_t;

typedef enum enum_exynos_super_resolution {
    EXYNOS_ANDROID_VENDOR_SUPER_RESOLUTION_OFF = 0,
    EXYNOS_ANDROID_VENDOR_SUPER_RESOLUTION_ON,
} enum_exynos_super_resolution_t;

typedef enum enum_exynos_super_eis {
    EXYNOS_ANDROID_VENDOR_SUPER_EIS_OFF = 0,
    EXYNOS_ANDROID_VENDOR_SUPER_EIS_ON,
} enum_exynos_super_eis_t;

typedef enum enum_exynos_clahe_capture {
    EXYNOS_ANDROID_VENDOR_CLAHE_CAPTURE_OFF = 0,
    EXYNOS_ANDROID_VENDOR_CLAHE_CAPTURE_ON,
} enum_exynos_clahe_capture_t;

typedef enum enum_exynos_combine_single_capture {
    EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE_OFF,
    EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE_ON,
} enum_exynos_combine_single_capture_t;

enum exynos_ext_tags {
    EXYNOS_ANDROID_CONTROL_BASE                     = EXYNOS_ANDROID_CONTROL_START,
    EXYNOS_ANDROID_CONTROL_SCENE_DETECTION_INFO     = EXYNOS_ANDROID_CONTROL_START,             // int64[]      Control meta
    SLSI_MF_STILL_MODE,                                                                         // TYPE_BYTE
    SLSI_MF_STILL_CAPTURE,                                                                      // TYPE_BYTE
    SLSI_MF_STILL_RESULT,                                                                       // TYPE_BYTE
    SLSI_MF_STILL_VERSION,                                                                      // TYPE_BYTE
    SLSI_MF_STILL_PARAMETERS,                                                                   // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_SCENE_TYPE,
    EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_BAYER,                                                     // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_NIGHT_SHOT,                                                           // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_SUPER_NIGHT_SHOT_BAYER,                                               // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_HDR_BAYER,                                                            // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_HDR_YUV,                                                              // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_FLASH_MULTI_FRAME_DENOISE_YUV,                                        // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV,                                                              // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_SPORTS_YUV,                                                           // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_SPORTS_YUV_MOTION_LEVEL,                                              // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE,                                                           // TYPE_BYTE
    EXYNOS_ANDROID_CONTROL_END,

    EXYNOS_ANDROID_VENDOR_SENSOR_BASE               = EXYNOS_ANDROID_VENDOR_SENSOR_START,
    EXYNOS_ANDROID_VENDOR_SENSOR_INFO               = EXYNOS_ANDROID_VENDOR_SENSOR_START,       // TYPE_BYTE    static meta
    EXYNOS_ANDROID_VENDOR_SENSOR_INFO_ARCSOFT_DUAL_CALIB_BLOB,                                  // TYPE_BYTE    static meta
    EXYNOS_ANDROID_VENDOR_SENSOR_INFO_3D_HDR_SUPPORTED,                                         // TYPE_BYTE    static meta
    EXYNOS_ANDROID_VENDOR_SENSOR_VENDOR_EXPOSURE_SUPPORT_RANGE,
    EXYNOS_ANDROID_VENDOR_SENSOR_VENDOR_FPS_SUPPORT_RANGE,                                      // TYPE_INT32   static meta
    EXYNOS_ANDROID_VENDOR_SENSOR_QUAD_PIXEL_SUPPORT,                                            // TYPE_BYTE    static meta
    EXYNOS_ANDROID_VENDOR_SENSOR_END,

    EXYNOS_ANDROID_VENDOR_ENVINFO_BASE              = EXYNOS_ANDROID_VENDOR_ENVINFO_START,
    EXYNOS_ANDROID_VENDOR_ENVINFO_LUX_STD           = EXYNOS_ANDROID_VENDOR_ENVINFO_START,      // TYPE_FLOAT   control meta
    EXYNOS_ANDROID_VENDOR_ENVINFO_LUX_IDX,                                                      // TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_ENVINFO_ISO100_GAIN,                                                  //TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_ENVINFO_ANALOG_GAIN,                                                  // TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_ENVINFO_LINECOUNT,                                                    //TYPE_INT32
    EXYNOS_ANDROID_VENDOR_ENVINFO_AWB_CCT,                                                      // TYPE_INT32    control meta
    EXYNOS_ANDROID_VENDOR_ENVINFO_AWB_DEC,                                                      // TYPE_INT32    control meta
    EXYNOS_ANDROID_VENDOR_ENVINFO_LENS_POS,                                                     // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_ENVINFO_AFD_SUBMODE,                                                  // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_ENVINFO_THERMAL_LEVEL,                                                // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_ENVINFO_FLICKER_DETECT,                                               // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_ENVINFO_END,

    EXYNOS_ANDROID_VENDOR_CONTROL_BASE              = EXYNOS_ANDROID_VENDOR_CONTROL_START,
    EXYNOS_ANDROID_VENDOR_CONTROL_EXP_PRI           = EXYNOS_ANDROID_VENDOR_CONTROL_START,      // TYPE_INT64
    EXYNOS_ANDROID_VENDOR_CONTROL_ISO_PRI,                                                      // TYPE_INT64
    EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_STIL,                                                    // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_VIDEO,                                                   // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_CONTROL_ROTATION_STILL,                                               // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_CONTROL_3DHDR,                                                        // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_CONTROL_LONG_EXPOSURE_CAPTURE,                                        // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_CONTROL_LONG_EXPOSURE_COUNT,                                          // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_SESSION_MODE,                                                         // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_COMBINE_PREVIEW_PLUGIN,                                               // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_CONTROL_VSTAB_MAX_SIZE,                                               // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_CONTROL_VSTAB_MAX_FPS,                                                // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_CONTROL_EV,                                                           // TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_CONTROL_CAMERA_ID,                                                    // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_CONTROL_ISO_TOTAL,                                                    // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_CONTROL_HDR_GAIN,                                                     // TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_CONTROL_HDR_SHUTTER,                                                  // TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_CONTROL_SENSOR_GAIN,                                                  // TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_RESOLUTION,                                             // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_CONTROL_ZERO_CAMERA,                                                  // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_EIS,                                                    // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_CONTROL_CLAHE_CAPTURE,                                                // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_CONTROL_END,

    EXYNOS_ANDROID_VENDOR_STATS_BASE                = EXYNOS_ANDROID_VENDOR_STATS_START,
    EXYNOS_ANDROID_VENDOR_STATS_BLINK_DEGREE        = EXYNOS_ANDROID_VENDOR_STATS_START,        // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_STATS_BLINK_DETECTED,                                                 // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_STATS_SMILE_CONFIDENCE,                                               // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_STATS_SMILE_DEGREE,                                                   // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_STATS_GAZE_ANGLE,                                                     // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_STATS_GAZE_DIRECTION,                                                 // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_STATS_AEC_AECLUX,                                                     // TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_AWB_CCT_VALUE,                                         // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_AEC_AEC_STATUS,                                        // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_AF_STATUS,                                             // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_LENS_SHIFT_MM,                                         // TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_OBJECT_DISTANCE_MM,                                    // TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_NEAR_FIELD_MM,                                         // TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_FAR_FIELD_MM,                                          // TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_STATS_END,

    EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_BASE     = EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_START,
    EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_ENABLE   = EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_START, // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_RECT,                                                // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_ROI,                                                 // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_END,

    EXYNOS_ANDROID_VENDOR_SNAPSHOT_BASE             = EXYNOS_ANDROID_VENDOR_SNAPSHOT_START,
    EXYNOS_ANDROID_VENDOR_SNAPSHOT_APPX             = EXYNOS_ANDROID_VENDOR_SNAPSHOT_START,     // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_SNAPSHOT_EXIF,                                                        // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_SNAPSHOT_MAKER_NOTE,                                                  // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_SNAPSHOT_END,

    EXYNOS_ANDROID_VENDOR_FACTORY_BASE              = EXYNOS_ANDROID_VENDOR_FACTORY_START,
    EXYNOS_ANDROID_VENDOR_FACTORY_FOCUS_POS         = EXYNOS_ANDROID_VENDOR_FACTORY_START,      // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_FACTORY_CALIBRATION_STATUS,                                           // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_FACTORY_MODULE_ID,                                                    // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_FACTORY_OIS_GEA,                                                      // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_FACTORY_OIS_HEA,                                                      // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_FACTORY_OIS_FW_VER,                                                   // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_STROBE_CURRENTS,                              // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_ENABLED,                                      // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_DATA,                                         // TYPE_DOUBLE
    EXYNOS_ANDROID_VENDOR_FACTORY_END,

    EXYNOS_ANDROID_VENDOR_DUALCAM_BASE       = EXYNOS_ANDROID_VENDOR_DUALCAM_START,
    EXYNOS_ANDROID_VENDOR_DUALCAM_IS_AUX_BAYER         = EXYNOS_ANDROID_VENDOR_DUALCAM_START,      // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_DUALCAM_IS_AUX_MASTER,    //TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_DUALCAM_IS_VIDEO_ACTION,  //TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_DUALCAM_END,

    EXYNOS_ANDROID_VENDOR_REMOSAIC_BASE       = EXYNOS_ANDROID_VENDOR_REMOSAIC_START,
    EXYNOS_ANDROID_VENDOR_REMOSAIC_IS_REMOSAIC_SENSOR       = EXYNOS_ANDROID_VENDOR_REMOSAIC_START,      // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_REMOSAIC_IS_BY_HW,            // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_REMOSAIC_RAW_DIM,             // TYPE_INT32[2]
    EXYNOS_ANDROID_VENDOR_REMOSAIC_MAX_YUV_IN_OUT_SIZE, // TYPE_INT32[4]
    EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION,                                                       // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_REMOSAIC_HIGH_RESOLUTION_SIZES,                                            //TYPE_INT32[6]
    EXYNOS_ANDROID_VENDOR_REMOSAIC_SUPPORTED,                                            //TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_REMOSAIC_END,

    EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_BASE      = EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_START,
    EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_STRENGTH  = EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_START,      // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_STRENGTH_RANGE,   // int32[]
    EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_MODE,             // enum
    EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_AVAILABLE_MODES,  // byte[]
    EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_END,

    EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_BASE    = EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_START,
    EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_ENABLE  = EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_START,   // TYPE_BYTE
    EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_END,

    EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE_BASE      = EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE_START,
    EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE_EXPINDEX  = EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE_START,   // TYPE_FLOAT
    EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE_END,

    EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_BASE    = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_START,
    EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_SESSION_ENABLE  = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_START,   // TYPE_INT32
    EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_SESSION_IMAGE_READER_ID,                                             //TYPE_INT32
    EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_REQUEST_ID,                                                    //TYPE_INT64
    EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_RESULT_NEXT_OPERATION,                                         //TYPE_INT32
    EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_RESULT_IMAGE_READER_ID,                                        //TYPE_INT32
    EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_RESULT_REQUEST_ID,                                             //TYPE_INT64
    EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_END,
};

typedef struct vendor_tag_info {
    const char *tag_name;
    uint8_t     tag_type;
} vendor_tag_info_t;

class ExynosCameraVendorTags {
public:
    static void getVendorTagOps(vendor_tag_ops_t* ops);

    /* Gets the number of vendor tags supported on this platform */
    static int getTagCount(const vendor_tag_ops_t *ops);

    /* Fills an array with all of the supported vendor tags on this platform */
    static void getAllTags(const vendor_tag_ops_t *ops,
                            uint32_t *tag_array);

    /* Gets the vendor section name for a vendor-specified entry tag.*/
    static const char* getSectionName(const vendor_tag_ops_t *ops,
                            uint32_t tag);

    /* Gets the tag name for a vendor-specified entry tag. */
    static const char* getTagName(const vendor_tag_ops_t *ops,
                            uint32_t tag);

    /* Gets tag type for a vendor-specified entry tag.*/
    static int getTagType(const vendor_tag_ops_t *ops,
                            uint32_t tag);

    static const vendor_tag_ops_t *vOps;

};

}; // namespace android
#endif


