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

#define LOG_TAG "ExynosCameraVendorTags"
#include <log/log.h>

#include "ExynosCameraVendorTags.h"
#include "ExynosCameraConfig.h"
#include "ExynosCameraCommonDefine.h"
#include "ExynosCameraCommonEnum.h"
#include "ExynosCameraVendorTagsCommon.h"

namespace android {

const vendor_tag_ops_t* ExynosCameraVendorTags::vOps = NULL;

const char *exynos_extension_section_names[EXYNOS_ANDROID_EXTENSION_SECTION_COUNT] = {
    vendor_section_name[0],
    vendor_section_name[1],
    vendor_section_name[2],
    vendor_section_name[3],
    vendor_section_name[4],
    vendor_section_name[5],
    vendor_section_name[6],
    vendor_section_name[7],
    vendor_section_name[8],
    vendor_section_name[9],
    vendor_section_name[10],
    vendor_section_name[11],
    vendor_section_name[12],
    vendor_section_name[13],
};

uint32_t exynos_extension_section_bounds[EXYNOS_ANDROID_EXTENSION_SECTION_COUNT][2] = {
    { (uint32_t) EXYNOS_ANDROID_CONTROL_START,              (uint32_t) (EXYNOS_ANDROID_CONTROL_END)           },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_SENSOR_START,        (uint32_t) (EXYNOS_ANDROID_VENDOR_SENSOR_END)     },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_ENVINFO_START,       (uint32_t) (EXYNOS_ANDROID_VENDOR_ENVINFO_END)    },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_CONTROL_START,       (uint32_t) (EXYNOS_ANDROID_VENDOR_CONTROL_END)    },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_STATS_START,         (uint32_t) (EXYNOS_ANDROID_VENDOR_STATS_END)      },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_START, (uint32_t) (EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_END)   },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_SNAPSHOT_START,      (uint32_t) (EXYNOS_ANDROID_VENDOR_SNAPSHOT_END)   },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_FACTORY_START,       (uint32_t) (EXYNOS_ANDROID_VENDOR_FACTORY_END)    },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_DUALCAM_START,       (uint32_t) (EXYNOS_ANDROID_VENDOR_DUALCAM_END)    },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_REMOSAIC_START,      (uint32_t) (EXYNOS_ANDROID_VENDOR_REMOSAIC_END)   },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_START,      (uint32_t) (EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_END)   },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_START,    (uint32_t) (EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_END) },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE_START,  (uint32_t) (EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE_END) },
    { (uint32_t) EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_START,    (uint32_t) (EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_END) },
};

vendor_tag_info_t exynos_extension_android_control[EXYNOS_ANDROID_CONTROL_END - EXYNOS_ANDROID_CONTROL_START] =
{
    { "sceneDetectionInfo",                 TYPE_INT64  },
    { "mf_still.mode",                      TYPE_BYTE   },
    { "mf_still.capture",                   TYPE_BYTE   },
    { "mf_still.result",                    TYPE_BYTE   },
    { "mf_still.version",                   TYPE_BYTE   },
    { "mf_still.parameter",                 TYPE_BYTE   },
    { "sceneType",                          TYPE_INT32  },
    { "nightshot_bayer",                    TYPE_BYTE   },
    { "nightshot",                          TYPE_BYTE   },
    { "super_nightshot_bayer",              TYPE_BYTE   },
    { "hdr_bayer",                          TYPE_BYTE   },
    { "hdr_yuv",                            TYPE_BYTE   },
    { "flash_multi-frame",                  TYPE_BYTE   },
    { "ois_denoise_yuv",                    TYPE_BYTE   },
    { "sports_yuv",                         TYPE_BYTE   },
    { "sports_yuv_motion_level",            TYPE_BYTE   },
    { "ar_sticker",                         TYPE_BYTE   },
};

vendor_tag_info_t exynos_extension_android_vendor_sensor[EXYNOS_ANDROID_VENDOR_SENSOR_END - EXYNOS_ANDROID_VENDOR_SENSOR_START] =
{
    { "info.name",                          TYPE_BYTE   },
    { "info.arcsoft_dual_calib_blob",       TYPE_BYTE   },
    { "info.3d_hdr_supported",              TYPE_BYTE   },
    { "info.exposureTimeRange",             TYPE_INT64  },
    { "info.fps_range",                     TYPE_INT32  },
    { "info.quad_pixel",                    TYPE_BYTE   },
};

vendor_tag_info_t exynos_extension_android_vendor_envinfo[EXYNOS_ANDROID_VENDOR_ENVINFO_END - EXYNOS_ANDROID_VENDOR_ENVINFO_START] =
{
    { "lux_std",                            TYPE_FLOAT  },
    { "lux_idx",                            TYPE_FLOAT  },
    { "iso100_gain",                        TYPE_FLOAT	},
    { "analog_gain",                        TYPE_FLOAT  },
    { "linecount",                          TYPE_INT32	},
    { "awb_cct",                            TYPE_INT32  },
    { "awb_dec",                            TYPE_INT32  },
    { "lens_pos",                           TYPE_INT32  },
    { "afd_submode",                        TYPE_BYTE	},
    { "thermal_level",                      TYPE_BYTE	},
    { "flicker_detect",                     TYPE_BYTE	},
};

vendor_tag_info_t exynos_extension_android_vendor_control[EXYNOS_ANDROID_VENDOR_CONTROL_END - EXYNOS_ANDROID_VENDOR_CONTROL_START] =
{
    {"exp_pri",                             TYPE_INT64  },
    {"iso_pri",                             TYPE_INT64  },
    {"flip_still",                          TYPE_INT32  },
    {"flip_video",                          TYPE_INT32  },
    {"rotation_still",                      TYPE_INT32  },
    {"3d_hdr_mode",                         TYPE_BYTE   },
    {"long_exposure_capture",               TYPE_BYTE   },
    {"long_exposure_count",                 TYPE_INT32  },
    {"session_mode",                        TYPE_BYTE   },
    {"combine_preview_plugin",              TYPE_BYTE   },
    { "vstab_available_max_res",            TYPE_INT32  },
    { "vstab_available_max_fps",            TYPE_INT32  },
    { "ev",                                 TYPE_FLOAT  },
    { "camera_id",                          TYPE_INT32  },
    { "iso_total",                          TYPE_INT32  },
    { "hdr_gain",                           TYPE_FLOAT  },
    { "hdr_shutter",                        TYPE_FLOAT  },
    { "sensor_gain",                        TYPE_FLOAT  },
    { "super_resolution",                   TYPE_BYTE   },
    { "ZeroCamera",                         TYPE_BYTE   },
    { "super_eis",                          TYPE_BYTE   },
    { "clahe_capture",                      TYPE_BYTE   },
};

vendor_tag_info_t exynos_extension_android_vendor_stats[EXYNOS_ANDROID_VENDOR_STATS_END - EXYNOS_ANDROID_VENDOR_STATS_START] =
{
    {"blink_degree",                        TYPE_BYTE   },
    {"blink_detected",                      TYPE_BYTE   },
    {"smile_confidence",                    TYPE_BYTE   },
    {"smile_degree",                        TYPE_BYTE   },
    {"gaze_angle",                          TYPE_BYTE   },
    {"gaze_direction",                      TYPE_INT32  },
    {"aec.AecLux",                          TYPE_FLOAT  },
    {"3rd.info.awb_cct_value",              TYPE_INT32  },
    {"3rd.info.aec_aec_status",             TYPE_INT32  },
    {"3rd.info.af_status",                  TYPE_INT32  },
    {"3rd.info.lens_shift_mm",              TYPE_FLOAT  },
    {"3rd.info.object_distance_mm",         TYPE_FLOAT  },
    {"3rd.info.near_field_mm",              TYPE_FLOAT  },
    {"3rd.info.far_field_mm",               TYPE_FLOAT  },
};

vendor_tag_info_t exynos_extension_android_vendor_jpeg_encode_crop[EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_END - EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_START] =
{
    { "enable",                             TYPE_BYTE   },
    { "rect",                               TYPE_INT32  },
    { "roi",                                TYPE_INT32  },
};

vendor_tag_info_t exynos_extension_android_vendor_snapshot[EXYNOS_ANDROID_VENDOR_SNAPSHOT_END - EXYNOS_ANDROID_VENDOR_SNAPSHOT_START] =
{
    { "appx",                               TYPE_BYTE  },
    { "exif",                               TYPE_BYTE  },
    { "makernote",                          TYPE_BYTE  },
};

vendor_tag_info_t exynos_extension_android_vendor_factory[EXYNOS_ANDROID_VENDOR_FACTORY_END - EXYNOS_ANDROID_VENDOR_FACTORY_START] =
{
    { "focus_pos",                          TYPE_INT32  },
    { "calibration_status",                 TYPE_BYTE   },
    { "module_id",                          TYPE_BYTE   },
    { "ois_gea",                            TYPE_BYTE   },
    { "ois_hea",                            TYPE_INT32  },
    { "ois_get_fw_rev",                     TYPE_BYTE   },
    { "strobe_currents",                    TYPE_INT32  },
    { "led_calibration_enabled",            TYPE_BYTE   },
    { "led_calibration_data",               TYPE_DOUBLE },
};

vendor_tag_info_t exynos_extension_android_vendor_dualcam[EXYNOS_ANDROID_VENDOR_DUALCAM_END - EXYNOS_ANDROID_VENDOR_DUALCAM_START] =
{
    { "is_aux_bayer",                          TYPE_BYTE  },
    { "is_aux_master",                          TYPE_BYTE  },
    { "is_video_action",                        TYPE_BYTE  },
};

vendor_tag_info_t exynos_extension_android_vendor_remosaic[EXYNOS_ANDROID_VENDOR_REMOSAIC_END - EXYNOS_ANDROID_VENDOR_REMOSAIC_START] =
{
    { "is_qcfa_sensor",                          TYPE_BYTE  },
    { "is_remosaic_by_hw",                       TYPE_BYTE  },
    { "qcfa_dimension",                          TYPE_INT32  },
    { "max_yuv_input_output_size",               TYPE_INT32  },
    { "capture",                                 TYPE_BYTE   },
    { "high_resolution_sizes",                   TYPE_INT32  },
    { "supported",                               TYPE_BYTE  },
};

vendor_tag_info_t exynos_extension_android_vendor_beauty_face[EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_END - EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_START] =
{
    { "beauty_face_strength",                   TYPE_INT32 },
    { "beauty_face_strength_range",             TYPE_INT32 },
    { "beauty_face_mode",                       TYPE_BYTE },
    { "beauty_face_available_mode",             TYPE_BYTE },
};

vendor_tag_info_t exynos_extension_android_vendor_abort_capture[EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_END - EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_START] =
{
    { "enable",                   TYPE_BYTE },
};

vendor_tag_info_t exynos_extension_android_vendor_chi_override[EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE_END - EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE_START] =
{
    { "expIndex",                               TYPE_FLOAT },
};

vendor_tag_info_t exynos_extension_android_vendor_offline_capture[EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_END - EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_START] =
{
    { "control.vop",                            TYPE_INT32 },
    { "parameter.vop.imagereadid",				TYPE_INT32 },
    { "control.requestid",				        TYPE_INT64 },
    { "feedback.vop.nextcapture",				TYPE_INT32 },
    { "result.vop.imagereadid",				    TYPE_INT32 },
    { "result.vop.requestid",				    TYPE_INT64 },
};

vendor_tag_info_t *exynos_extension_tag_info[EXYNOS_ANDROID_EXTENSION_SECTION_COUNT] = {
    exynos_extension_android_control,
    exynos_extension_android_vendor_sensor,
    exynos_extension_android_vendor_envinfo,
    exynos_extension_android_vendor_control,
    exynos_extension_android_vendor_stats,
    exynos_extension_android_vendor_jpeg_encode_crop,
    exynos_extension_android_vendor_snapshot,
    exynos_extension_android_vendor_factory,
    exynos_extension_android_vendor_dualcam,
    exynos_extension_android_vendor_remosaic,
    exynos_extension_android_vendor_beauty_face,
    exynos_extension_android_vendor_abort_capture,
    exynos_extension_android_vendor_chi_override,
    exynos_extension_android_vendor_offline_capture
};

/* It gets the vendor tag function pointers */
void ExynosCameraVendorTags::getVendorTagOps(
                                vendor_tag_ops_t* v)
{
    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    vOps = v;

    v->get_tag_count = getTagCount;
    v->get_all_tags = getAllTags;
    v->get_section_name = getSectionName;
    v->get_tag_name = getTagName;
    v->get_tag_type = getTagType;
    v->reserved[0] = NULL;

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return;
}


/* Gets the number of vendor tags supported on this platform */
int ExynosCameraVendorTags::getTagCount(
                const vendor_tag_ops_t * v)
{
    int count = 0;
    int section;
    uint32_t start, end;

    if (v != vOps)
        return 0;

    for (section = 0; section < EXYNOS_ANDROID_EXTENSION_SECTION_COUNT; section++) {
        start   = exynos_extension_section_bounds[section][0];
        end     = exynos_extension_section_bounds[section][1];
        count += end - start;
    }

    return count;

}

/* Fills an array with all of the supported vendor tags on this platform */
void ExynosCameraVendorTags::getAllTags(const vendor_tag_ops_t *v, uint32_t *tag_array) {
    int section;
    uint32_t start, end, tag;

    if ((v != vOps) || (tag_array == NULL))
        return;

    for (section = 0; section < EXYNOS_ANDROID_EXTENSION_SECTION_COUNT; section++) {
        start   = exynos_extension_section_bounds[section][0];
        end     = exynos_extension_section_bounds[section][1];
        for (tag = start; tag < end; tag++) {
            *tag_array++ = tag;
        }
    }
}

/* Gets the vendor section name for a vendor-specified entry tag.*/
const char * ExynosCameraVendorTags::getSectionName(const vendor_tag_ops_t *v,
        uint32_t tag) {

    int tag_section;

    if (v != vOps)
        return NULL;

    tag_section = (tag >> 16) - EXYNOS_SECTION;
    if ((tag_section < 0) ||
            (tag_section >= EXYNOS_ANDROID_EXTENSION_SECTION_COUNT)) {
        return NULL;
    }

    return exynos_extension_section_names[tag_section];
}

/* Gets the tag name for a vendor-specified entry tag. */
const char * ExynosCameraVendorTags::getTagName(const vendor_tag_ops_t *v,
        uint32_t tag) {

    int tag_section;
    int tag_index;

    if (v != vOps)
        return NULL;

    tag_section = (tag >> 16) - EXYNOS_ANDROID_EXTENSION_SECTION_START;
    if ((tag_section < 0)
            || (tag_section >= EXYNOS_ANDROID_EXTENSION_SECTION_COUNT)
            || (tag >= exynos_extension_section_bounds[tag_section][1])) {
        return NULL;
    }

    tag_index = tag & 0xFFFF;
    return exynos_extension_tag_info[tag_section][tag_index].tag_name;
}

/* Gets tag type for a vendor-specified entry tag.*/
int ExynosCameraVendorTags::getTagType(const vendor_tag_ops_t *v,
        uint32_t tag) {

    int tag_section;
    int tag_index;

    if (v != vOps)
        return -1;

    tag_section = (tag >> 16) - EXYNOS_ANDROID_EXTENSION_SECTION_START;
    if ((tag_section < 0)
            || (tag_section >= EXYNOS_ANDROID_EXTENSION_SECTION_COUNT)
            || (tag >= exynos_extension_section_bounds[tag_section][1])) {
        return -1;
    }

    tag_index = tag & 0xFFFF;

    return exynos_extension_tag_info[tag_section][tag_index].tag_type;
}

}

