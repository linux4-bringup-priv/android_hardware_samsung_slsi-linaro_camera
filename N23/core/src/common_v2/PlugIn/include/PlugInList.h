/*
 * Copyright (C) 2017, Samsung Electronics Co. LTD
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
#ifndef PLUG_IN_LIST_H
#define PLUG_IN_LIST_H

#include "PlugInCommon.h"
#include "ExynosCameraCommonInclude.h" /* for pipeId */

#define PLUGIN_LIB_NM_FAKEFUSION            "libexynoscamera_fakefusion_plugin.so"
#define PLUGIN_LIB_NM_ARCSOFTFUSION         "libexynoscamera_arcsoftfusion_plugin.so"
#define PLUGIN_LIB_NM_LOWLIGHTSHOT          "libexynoscamera_lowlightshot_plugin.so"
#define PLUGIN_LIB_NM_VDIS                  "libexynoscamera_vdis_plugin.so"
#define PLUGIN_LIB_NM_HIFILLS               "libexynoscamera_hifills_plugin.so"
#define PLUGIN_LIB_NM_HIFI                  "libexynoscamera_hifi_plugin.so"
#define PLUGIN_LIB_NM_COMBINE_PREVIEW       "libexynoscamera_combine_preview_plugin.so"
#define PLUGIN_LIB_NM_COMBINE_REPROCESSING  "libexynoscamera_combine_reprocessing_plugin.so"
#define PLUGIN_LIB_NM_VPL                   "libexynoscamera_vpl_plugin.so"
#define PLUGIN_LIB_NM_LEC                   "libexynoscamera_exynoslec_plugin.so"

typedef struct PlugInList {
    int id;
    const char *plugInLibName;
} PlugInList_t;

PlugInList_t g_plugInList[] = {
#ifdef USE_DUAL_CAMERA
#ifdef USES_DUAL_CAMERA_SOLUTION_FAKE
    { PLUGIN_SCENARIO_FAKEFUSION_PREVIEW,      PLUGIN_LIB_NM_FAKEFUSION },
    { PLUGIN_SCENARIO_FAKEFUSION_REPROCESSING, PLUGIN_LIB_NM_FAKEFUSION },
#endif
#ifdef USES_DUAL_CAMERA_SOLUTION_ARCSOFT
    { PLUGIN_SCENARIO_ZOOMFUSION_PREVIEW,      PLUGIN_LIB_NM_ARCSOFTFUSION },
    { PLUGIN_SCENARIO_ZOOMFUSION_REPROCESSING, PLUGIN_LIB_NM_ARCSOFTFUSION },
    { PLUGIN_SCENARIO_BOKEHFUSION_PREVIEW,     PLUGIN_LIB_NM_ARCSOFTFUSION },
    { PLUGIN_SCENARIO_BOKEHFUSION_REPROCESSING,PLUGIN_LIB_NM_ARCSOFTFUSION },
#endif
#endif // USE_DUAL_CAMERA

#ifdef SLSI_LLS_REPROCESSING
    { PLUGIN_SCENARIO_BAYERLLS_REPROCESSING,   PLUGIN_LIB_NM_LOWLIGHTSHOT},
#endif

#ifdef USES_CAMERA_SOLUTION_VDIS
    { PLUGIN_SCENARIO_SWVDIS_PREVIEW,          PLUGIN_LIB_NM_VDIS },
#endif
#ifdef USES_HIFI_LLS
    { PLUGIN_SCENARIO_HIFILLS_REPROCESSING,    PLUGIN_LIB_NM_HIFILLS },
#endif
#ifdef USES_HIFI
    { PLUGIN_SCENARIO_HIFI_REPROCESSING,       PLUGIN_LIB_NM_HIFI },
#endif

#ifdef USES_COMBINE_PLUGIN
    { PLUGIN_SCENARIO_COMBINE_PREVIEW,              PLUGIN_LIB_NM_COMBINE_PREVIEW },
    { PLUGIN_SCENARIO_COMBINE_REPROCESSING,         PLUGIN_LIB_NM_COMBINE_REPROCESSING },
    { PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING,   PLUGIN_LIB_NM_COMBINE_REPROCESSING },
#endif

#ifdef USES_CAMERA_EXYNOS_VPL
    { PIPE_NFD,              PLUGIN_LIB_NM_VPL },
    { PIPE_NFD_REPROCESSING, PLUGIN_LIB_NM_VPL },
#endif
#ifdef USES_CAMERA_EXYNOS_LEC
    { PIPE_PLUGIN_LEC_REPROCESSING, PLUGIN_LIB_NM_LEC },
#endif
};

#endif
