/*
 * Copyright (C) 2019, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_PLUGIN_CONVERTER_COMBINE_H__
#define EXYNOS_CAMERA_PLUGIN_CONVERTER_COMBINE_H__

#include "ExynosCameraPlugInConverter.h"

namespace android {

class ExynosCameraPlugInConverterCombine {
public:
    ////////////////////////////////////////////////
    // this should same with ExynosCameraPlugInCombine.h
    enum VENDOR_SCENARIO {
        VENDOR_SCENARIO_BASE = 0,
        VENDOR_SCENARIO_NIGHT_SHOT_BAYER,
        VENDOR_SCENARIO_NIGHT_SHOT_YUV,
        VENDOR_SCENARIO_SUPER_NIGHT_SHOT_BAYER,
        VENDOR_SCENARIO_HDR_BAYER,
        VENDOR_SCENARIO_HDR_YUV,
        VENDOR_SCENARIO_FLASH_MULTI_FRAME_DENOISE_YUV,
        VENDOR_SCENARIO_BEAUTY_FACE_YUV,
        VENDOR_SCENARIO_SUPER_RESOLUTION,
        VENDOR_SCENARIO_BOKEH_FUSION_CAPTURE,
        VENDOR_SCENARIO_SW_REMOSAIC,
        VENDOR_SCENARIO_OIS_DENOISE_YUV,
        VENDOR_SCENARIO_SPORTS_YUV,
        VENDOR_SCENARIO_MAX,
    };

public:
    static int  getVendorTagValue(ExynosCameraFrameSP_sptr_t frame, int vendorTagName);

    static void setMapValue(Map_t *map, int tagName, int value);
    static int  getMapValue(Map_t *map, int tagName);

    static void resetScenario(Map_t *map);
    static void setScenario(Map_t *map, int scenario);
    static int  getScenario(Map_t *map);

    static void setVendorScenario(Map_t *map, enum VENDOR_SCENARIO vendorScenario, bool on);
    static bool getVendorScenario(Map_t *map, enum VENDOR_SCENARIO vendorScenario);
};

}; /* namespace android */
#endif
