/*
 * Copyright @ 2019, Samsung Electronics Co. LTD
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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraPlugInCombineReprocessing"
#include <log/log.h>

#include "ExynosCameraPlugInCombineReprocessing.h"

namespace android {

/*********************************************/
/*  global definition                        */
/*********************************************/
volatile int32_t ExynosCameraPlugInCombineReprocessing::m_initCount = 0;

DECLARE_CREATE_PLUGIN_SYMBOL(ExynosCameraPlugInCombineReprocessing);

/*********************************************/
/*  protected functions                      */
/*********************************************/
status_t ExynosCameraPlugInCombineReprocessing::m_init(void)
{
    int count = android_atomic_inc(&m_initCount);

    mUser = NULL;
    m_fusion = NULL;

    PLUGIN_LOGD("count(%d)", count);

    if (count == 1) {
        for (int i = 0; i < NUM_OF_HDR_MERGE_FRAME; i++) {
            m_analogGain[i] = 0;
            m_shutterTime[i] = 0L;
        }
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombineReprocessing::m_deinit(void)
{
    int count = android_atomic_dec(&m_initCount);

    PLUGIN_LOGD("count(%d)", count);

    if (count == 0) {
        /* do nothing */
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombineReprocessing::m_destroy(void)
{
    PLUGIN_LOGD("");

    if (mUser) {
        Library* lib = (Library*)mUser;

        int ret = lib->destroy();
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[CombinePlugIn] Plugin deinit failed!!");
        }
        mUser = NULL;
    }

    if (m_fusion) {
        m_fusion->destroy();
        delete m_fusion;
        m_fusion = NULL;
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombineReprocessing::m_process(Map_t *map)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // frameCount & Index
    int frameCount    = m_getFrameCount(map);
    int frameMaxIndex = m_getFrameMaxIndex(map);
    int frameCurIndex = m_getFrameCurIndex(map);
    int scenario = -1;

    ////////////////////////////////////////////////
    // get common Infomation
    ret =  m_processCommon(map);
    if (ret != NO_ERROR) {
        PLUGIN_LOGE("[CombineReprocessingPlugIn] m_processCommon() fail");
        return ret;
    }

    ////////////////////////////////////////////////
    // process night shot bayer
    if (m_getVendorScenario(map, VENDOR_SCENARIO_NIGHT_SHOT_BAYER) == true) {
        ret = m_processNightShotBayer(map);
        if (ret != NO_ERROR) {
            CLOGE("m_processNightShotBayer() fail");
            return ret;
        }
    }

    ////////////////////////////////////////////////
    // process night shot yuv
    if (m_getVendorScenario(map, VENDOR_SCENARIO_NIGHT_SHOT_YUV) == true) {
        ret = m_processNightShotYuv(map);
        if (ret != NO_ERROR) {
            CLOGE("m_processNightShotYuv() fail");
            return ret;
        }
    }

    ////////////////////////////////////////////////
    // process super night shot bayer
    if (m_getVendorScenario(map, VENDOR_SCENARIO_SUPER_NIGHT_SHOT_BAYER) == true) {
        ret = m_processSuperNightShotBayer(map);
        if (ret != NO_ERROR) {
            CLOGE("m_processSuperNightShotBayer() fail");
            return ret;
        }
    }

    ////////////////////////////////////////////////
    // process hdr bayer
    if (m_getVendorScenario(map, VENDOR_SCENARIO_HDR_BAYER) == true) {
        ret = m_processHdrBayer(map);
        if (ret != NO_ERROR) {
            CLOGE("m_processHdrBayer() fail");
            return ret;
        }
    }

    ////////////////////////////////////////////////
    // process hdr yuv
    if (m_getVendorScenario(map, VENDOR_SCENARIO_HDR_YUV) == true) {
        ret = m_processHdrYuv(map);
        if (ret != NO_ERROR) {
            CLOGE("m_processHdrYuv() fail");
            return ret;
        }
    }

    ////////////////////////////////////////////////
    //
    if (m_getVendorScenario(map, VENDOR_SCENARIO_FLASH_MULTI_FRAME_DENOISE_YUV) == true) {
        ret = m_processFlashMultiFrameDenoiseYuv(map);
        if (ret != NO_ERROR) {
            CLOGE("m_processFlashMultiFrameDenoiseYuv() fail");
            return ret;
        }
    }

    ////////////////////////////////////////////////
    // process beauty face yuv
    if (m_getVendorScenario(map, VENDOR_SCENARIO_BEAUTY_FACE_YUV) == true) {
        ret = m_processBeautyFaceYuv(map);
        if (ret != NO_ERROR) {
            CLOGE("m_processBeautyFaceYuv() fail");
            return ret;
        }
    }

    if (m_getVendorScenario(map, VENDOR_SCENARIO_SUPER_RESOLUTION) == true) {
        ret = m_processSuperResolution(map);
        if (ret != NO_ERROR) {
            CLOGE("m_processSuperResolution() fail");
            return ret;
        }
    }

    if (m_getVendorScenario(map, VENDOR_SCENARIO_OIS_DENOISE_YUV) == true) {
        ret = m_processOisDenoiseYuv(map);
        if (ret != NO_ERROR) {
            CLOGE("m_processOisDenoisYuv() fail");
            return ret;
        }
    }

    ////////////////////////////////////////////////
    // process sports yuv
    if (m_getVendorScenario(map, VENDOR_SCENARIO_SPORTS_YUV) == true) {
        ret = m_processSportsYuv(map);
        if (ret != NO_ERROR) {
            CLOGE("m_processSportsYuv() fail");
            return ret;
        }
    }

    ////////////////////////////////////////////////
    // execute
    if (frameCurIndex <= (frameMaxIndex - 1)) {
        ////////////////////////////////////////////////
        // create
        ret = m_librariesCreate(map);
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[CombineReprocessingPlugIn] librariesCreate failed!!");
            return ret;
        }

        ret = m_librariesExecute(map);
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[CombineReprocessingPlugIn] librariesExecute failed!!");
            return ret;
        }
    }

    ////////////////////////////////////////////////
    // destroy
    if (frameCurIndex == (frameMaxIndex - 1)) { /* last frame */
        ret = m_librariesDestory(map);
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[CombineReprocessingPlugIn] librariesDestory failed!!");
            return ret;
        }
    }

    return ret;
}

status_t ExynosCameraPlugInCombineReprocessing::m_librariesCreate(Map_t *map)
{
    status_t ret = NO_ERROR;

    if (mUser == NULL) {
        PLUGIN_LOGD("[CombineReprocessingPlugIn] Plugin create");
        Library* lib = new Library();

        ret = lib->create();
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[CombineReprocessingPlugIn] Plugin init failed!!");
            SAFE_DELETE(lib);
        }

        mUser = (void *)lib;
        PLUGIN_LOGD("[CombineReprocessingPlugIn] Plugin create done");
    }

    if (((m_getVendorScenario(map, VENDOR_SCENARIO_BOKEH_FUSION_CAPTURE)) == true) && (m_fusion == NULL)) {
        m_fusion = new FakeFusion();
        ret = m_fusion->create();
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[CombineReprocessingPlugIn] Fusion init failed!!");
            SAFE_DELETE(m_fusion);
        }
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombineReprocessing::m_librariesDestory(Map_t *map)
{
    status_t ret = NO_ERROR;

    if (mUser != NULL) {
        PLUGIN_LOGD("[CombineReprocessingPlugIn] Plugin destroy");
        Library* lib = (Library*)mUser;

        ret = lib->destroy();
        PLUGIN_LOGD("[CombineReprocessingPlugIn] Plugin destroy done");
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[CombineReprocessingPlugIn] Plugin destroy failed!!");
        }

        SAFE_DELETE(mUser);
    }

    if (((m_getVendorScenario(map, VENDOR_SCENARIO_BOKEH_FUSION_CAPTURE)) == true) && (m_fusion != NULL)) {
        ret = m_fusion->destroy();
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[CombineReprocessingPlugIn] fusion destroy failed!!");
        }

        SAFE_DELETE(m_fusion);
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombineReprocessing::m_librariesExecute(Map_t *map)
{
    status_t ret = NO_ERROR;

    if (mUser != NULL) {
        Library* lib = (Library*)mUser;
        PLUGIN_LOGD("[CombineReprocessingPlugIn] Plugin execute");
        ret = lib->execute(map);
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[CombineReprocessingPlugIn] Plugin process failed!!");
            // run without any error sequence. only shows log.
            //return ret;
        }
        PLUGIN_LOGD("[CombineReprocessingPlugIn] Plugin execute done");
    }

    PLUGIN_LOGD("[CombineReprocessingPlugIn] VENDOR_SCENARIO_BOKEH_FUSION_CAPTURE = %d, fusion = %#p",
            m_getVendorScenario(map, VENDOR_SCENARIO_BOKEH_FUSION_CAPTURE), m_fusion);
    PLUGIN_LOGD("[CombineReprocessingPlugIn] scenario %#x", m_getScenario(map));

    if (((m_getVendorScenario(map, VENDOR_SCENARIO_BOKEH_FUSION_CAPTURE)) == true) && (m_fusion != NULL)) {
        PLUGIN_LOGD("[CombineReprocessingPlugIn] fusion execute");
        ret = m_fusion->execute(map);
        if (ret != NO_ERROR) {
            PLUGIN_LOGE("[CombineReprocessingPlugIn] fusion execute failed!!");
            return ret;
        }
        PLUGIN_LOGD("[CombineReprocessingPlugIn] fusion execute done");
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombineReprocessing::m_processNightShotBayer(Map_t *map)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // frameCount & Index
    int frameCount    = m_getFrameCount(map);
    int frameMaxIndex = m_getFrameMaxIndex(map);
    int frameCurIndex = m_getFrameCurIndex(map);

    ////////////////////////////////////////////////
    // shutter time
    int64_t shutterTime = 0L;
    shutterTime     = (Data_int64_t)(*map)[PLUGIN_EXPOSURE_TIME];

    ////////////////////////////////////////////////
    // digital gain
    unsigned int digitalGain = 0;
    digitalGain     = (Data_uint32_t)(*map)[PLUGIN_DIGITAL_GAIN];

    ////////////////////////////////////////////////
    // face detection
    int numOfDetectedFaces = 0;
    numOfDetectedFaces = (Data_int32_t)(*map)[PLUGIN_FACE_NUM];

    unsigned int(*Array_fd_master_rect_t)[4];
    Array_fd_master_rect_t = (unsigned int (*)[4])(*map)[PLUGIN_MASTER_FACE_RECT];

    for (int i = 0; i < numOfDetectedFaces; i++) {
        PLUGIN_LOGV("[NightShot] [%d / %d] : %d, %d, %d, %d",
            i,
            numOfDetectedFaces,
            Array_fd_master_rect_t[i][0],
            Array_fd_master_rect_t[i][1],
            Array_fd_master_rect_t[i][2],
            Array_fd_master_rect_t[i][3]);
    }

    ////////////////////////////////////////////////
    // Jpeg orientation
    uint32_t jpegOrientation = 0;
    jpegOrientation   = (Data_uint32_t)(*map)[PLUGIN_JPEG_ORIENTATION];

    ////////////////////////////////////////////////
    // log
    PLUGIN_LOGD("[NightShotBayer] frame(%d) count(%d/%d) shutterTime(%zu) digitalGain(%d) numOfDetectedFaces(%d) jpegOrientation(%d)",
            frameCount,
            frameMaxIndex, frameCurIndex,
            shutterTime,
            digitalGain,
            numOfDetectedFaces,
            jpegOrientation);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInCombineReprocessing::m_processNightShotYuv(Map_t *map)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // frameCount & Index
    int frameCount    = m_getFrameCount(map);
    int frameMaxIndex = m_getFrameMaxIndex(map);
    int frameCurIndex = m_getFrameCurIndex(map);

    ////////////////////////////////////////////////
    // iso
    int iso = 0;
    iso             = (Data_uint32_t)(*map)[PLUGIN_ISO];

    ////////////////////////////////////////////////
    // digital gain
    unsigned int digitalGain = 0;
    digitalGain     = (Data_uint32_t)(*map)[PLUGIN_DIGITAL_GAIN];

    ////////////////////////////////////////////////
    // flashMode
    int flashMode = 0;
    flashMode       = (Data_int32_t)(*map)[PLUGIN_FLASH_MODE];

    ////////////////////////////////////////////////
    // camera type
    int rearOrFront = 0;
    rearOrFront     = (Data_int32_t)(*map)[PLUGIN_HIFI_CAMERA_TYPE];

    ////////////////////////////////////////////////
    // llsState
    int llsState = 0;
    llsState        = (Data_int32_t)(*map)[PLUGIN_LLS_INTENT];

    ////////////////////////////////////////////////
    // analog gain
    unsigned int analogGain = 0;
    analogGain      = (Data_uint32_t)(*map)[PLUGIN_ANALOG_GAIN];

    ////////////////////////////////////////////////
    // face detection
    int numOfDetectedFaces = 0;
    numOfDetectedFaces = (Data_int32_t)(*map)[PLUGIN_FACE_NUM];

    unsigned int(*Array_fd_master_rect_t)[4];
    Array_fd_master_rect_t = (unsigned int (*)[4])(*map)[PLUGIN_MASTER_FACE_RECT];

    for (int i = 0; i < numOfDetectedFaces; i++) {
        PLUGIN_LOGV("[NightShotYuv] [%d / %d] : %d, %d, %d, %d",
            i,
            numOfDetectedFaces,
            Array_fd_master_rect_t[i][0],
            Array_fd_master_rect_t[i][1],
            Array_fd_master_rect_t[i][2],
            Array_fd_master_rect_t[i][3]);
    }

    ////////////////////////////////////////////////
    // log
    PLUGIN_LOGD("[NightShotYuv] frame(%d) count(%d/%d) iso(%d) digitalGain(%d) flashMode(%d) facing(%d) llsState(%d) analogGain(%d) numOfDetectedFaces(%d)",
            frameCount,
            frameMaxIndex, frameCurIndex,
            iso,
            digitalGain,
            flashMode,
            rearOrFront,
            llsState,
            analogGain,
            numOfDetectedFaces);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInCombineReprocessing::m_processSuperNightShotBayer(Map_t *map)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // frameCount & Index
    int frameCount    = m_getFrameCount(map);
    int frameMaxIndex = m_getFrameMaxIndex(map);
    int frameCurIndex = m_getFrameCurIndex(map);

    ////////////////////////////////////////////////
    // shutter time
    int64_t shutterTime = 0L;
    shutterTime     = (Data_int64_t)(*map)[PLUGIN_EXPOSURE_TIME];

    ////////////////////////////////////////////////
    // digital gain
    unsigned int digitalGain = 0;
    digitalGain     = (Data_uint32_t)(*map)[PLUGIN_DIGITAL_GAIN];

    ////////////////////////////////////////////////
    // face detection
    int numOfDetectedFaces = 0;
    numOfDetectedFaces = (Data_int32_t)(*map)[PLUGIN_FACE_NUM];

    unsigned int(*Array_fd_master_rect_t)[4];
    Array_fd_master_rect_t = (unsigned int (*)[4])(*map)[PLUGIN_MASTER_FACE_RECT];

    for (int i = 0; i < numOfDetectedFaces; i++) {
        PLUGIN_LOGV("[NightShot] [%d / %d] : %d, %d, %d, %d",
            i,
            numOfDetectedFaces,
            Array_fd_master_rect_t[i][0],
            Array_fd_master_rect_t[i][1],
            Array_fd_master_rect_t[i][2],
            Array_fd_master_rect_t[i][3]);
    }

    ////////////////////////////////////////////////
    // Jpeg orientation
    uint32_t jpegOrientation = 0;
    jpegOrientation   = (Data_uint32_t)(*map)[PLUGIN_JPEG_ORIENTATION];

    ////////////////////////////////////////////////
    // log
    PLUGIN_LOGD("[NightShotBayer] frame(%d) count(%d/%d) shutterTime(%zu) digitalGain(%d) numOfDetectedFaces(%d) jpegOrientation(%d)",
            frameCount,
            frameMaxIndex, frameCurIndex,
            shutterTime,
            digitalGain,
            numOfDetectedFaces,
            jpegOrientation);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInCombineReprocessing::m_processHdrBayer(Map_t *map)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // frameCount & Index
    int frameCount    = m_getFrameCount(map);
    int frameMaxIndex = m_getFrameMaxIndex(map);
    int frameCurIndex = m_getFrameCurIndex(map);

    ////////////////////////////////////////////////
    // shutter time
    int64_t shutterTime = 0L;
    shutterTime     = (Data_int64_t)(*map)[PLUGIN_EXPOSURE_TIME];

    ////////////////////////////////////////////////
    // digital gain
    unsigned int digitalGain = 0;
    digitalGain     = (Data_uint32_t)(*map)[PLUGIN_DIGITAL_GAIN];

    ////////////////////////////////////////////////
    // face detection
    int numOfDetectedFaces = 0;
    numOfDetectedFaces = (Data_int32_t)(*map)[PLUGIN_FACE_NUM];

    unsigned int(*Array_fd_master_rect_t)[4];
    Array_fd_master_rect_t = (unsigned int (*)[4])(*map)[PLUGIN_MASTER_FACE_RECT];

    for (int i = 0; i < numOfDetectedFaces; i++) {
        PLUGIN_LOGV("[HdrBayer] [%d / %d] : %d, %d, %d, %d",
            i,
            numOfDetectedFaces,
            Array_fd_master_rect_t[i][0],
            Array_fd_master_rect_t[i][1],
            Array_fd_master_rect_t[i][2],
            Array_fd_master_rect_t[i][3]);
    }

    ////////////////////////////////////////////////
    // Jpeg orientation
    uint32_t jpegOrientation = 0;
    jpegOrientation   = (Data_uint32_t)(*map)[PLUGIN_JPEG_ORIENTATION];

    ////////////////////////////////////////////////
    // log
    PLUGIN_LOGD("[HdrBayer] frame(%d) count(%d/%d) shutterTime(%zu) digitalGain(%d) numOfDetectedFaces(%d) jpegOrientation(%d)",
            frameCount,
            frameMaxIndex, frameCurIndex,
            shutterTime,
            digitalGain,
            numOfDetectedFaces,
            jpegOrientation);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInCombineReprocessing::m_processHdrYuv(Map_t *map)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // frameCount & Index
    int frameCount    = m_getFrameCount(map);
    int frameMaxIndex = m_getFrameMaxIndex(map);
    int frameCurIndex = m_getFrameCurIndex(map);

    ////////////////////////////////////////////////
    // iso
    int iso = 0;
    iso             = (Data_uint32_t)(*map)[PLUGIN_ISO];

    ////////////////////////////////////////////////
    // camera type
    int rearOrFront = 0;
    rearOrFront     = (Data_int32_t)(*map)[PLUGIN_HIFI_CAMERA_TYPE];

    ////////////////////////////////////////////////
    // face detection
    int numOfDetectedFaces = 0;
    numOfDetectedFaces = (Data_int32_t)(*map)[PLUGIN_FACE_NUM];

    unsigned int(*Array_fd_master_rect_t)[4];
    Array_fd_master_rect_t = (unsigned int (*)[4])(*map)[PLUGIN_MASTER_FACE_RECT];

    for (int i = 0; i < numOfDetectedFaces; i++) {
        PLUGIN_LOGV("[HdrYuv] [%d / %d] : %d, %d, %d, %d",
            i,
            numOfDetectedFaces,
            Array_fd_master_rect_t[i][0],
            Array_fd_master_rect_t[i][1],
            Array_fd_master_rect_t[i][2],
            Array_fd_master_rect_t[i][3]);
    }

    ret = m_setAnalogGain(frameCurIndex, (Data_uint32_t)(*map)[PLUGIN_ANALOG_GAIN]);
    if (ret != NO_ERROR) {
        PLUGIN_LOGE("fail to setAnalogGain. ret(%d)", ret);
        return ret;
    }

    ret = m_setShutterTime(frameCurIndex, (Data_int64_t)(*map)[PLUGIN_EXPOSURE_TIME]);
    if (ret != NO_ERROR) {
        PLUGIN_LOGE("fail to setShutterTime. ret(%d)", ret);
        return ret;
    }

    ////////////////////////////////////////////////
    // Jpeg orientation
    uint32_t jpegOrientation = 0;
    jpegOrientation   = (Data_uint32_t)(*map)[PLUGIN_JPEG_ORIENTATION];

    ////////////////////////////////////////////////
    // ae region
    Pointer_int_t aeRegion = (Pointer_int_t)(*map)[PLUGIN_AE_REGION];

    for (int i = 0; i < 4; i++) {
        PLUGIN_LOGV("[HdrYuv] aeRegion(%d):[%d, %d, %d, %d]",
            i,
            aeRegion[0],
            aeRegion[1],
            aeRegion[2],
            aeRegion[3]);
    }

    ////////////////////////////////////////////////
    // log
    PLUGIN_LOGD("[HdrYuv] frame(%d) count(%d/%d) iso(%d) facing(%d) numOfDetectedFaces(%d) jpegOrientation(%d) aeRegion(%d, %d, %d, %d)",
            frameCount,
            frameMaxIndex, frameCurIndex,
            iso,
            rearOrFront,
            numOfDetectedFaces,
            jpegOrientation,
            aeRegion[0], aeRegion[1], aeRegion[2], aeRegion[3]);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInCombineReprocessing::m_processFlashMultiFrameDenoiseYuv(Map_t *map)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // frameCount & Index
    int frameCount    = m_getFrameCount(map);
    int frameMaxIndex = m_getFrameMaxIndex(map);
    int frameCurIndex = m_getFrameCurIndex(map);

    ////////////////////////////////////////////////
    // iso
    int iso = 0;
    iso             = (Data_uint32_t)(*map)[PLUGIN_ISO];

    ////////////////////////////////////////////////
    // digital gain
    unsigned int digitalGain = 0;
    digitalGain     = (Data_uint32_t)(*map)[PLUGIN_DIGITAL_GAIN];

    ////////////////////////////////////////////////
    // flashMode
    int flashMode = 0;
    flashMode       = (Data_int32_t)(*map)[PLUGIN_FLASH_MODE];

    ////////////////////////////////////////////////
    // camera type
    int rearOrFront = 0;
    rearOrFront     = (Data_int32_t)(*map)[PLUGIN_HIFI_CAMERA_TYPE];

    ////////////////////////////////////////////////
    // llsState
    int llsState = 0;
    llsState        = (Data_int32_t)(*map)[PLUGIN_LLS_INTENT];

    ////////////////////////////////////////////////
    // analog gain
    unsigned int analogGain = 0;
    analogGain      = (Data_uint32_t)(*map)[PLUGIN_ANALOG_GAIN];

    ////////////////////////////////////////////////
    // face detection
    int numOfDetectedFaces = 0;
    numOfDetectedFaces = (Data_int32_t)(*map)[PLUGIN_FACE_NUM];

    unsigned int(*Array_fd_master_rect_t)[4];
    Array_fd_master_rect_t = (unsigned int (*)[4])(*map)[PLUGIN_MASTER_FACE_RECT];

    for (int i = 0; i < numOfDetectedFaces; i++) {
        PLUGIN_LOGV("[FlashMultiFrameDenoiseYuv] [%d / %d] : %d, %d, %d, %d",
            i,
            numOfDetectedFaces,
            Array_fd_master_rect_t[i][0],
            Array_fd_master_rect_t[i][1],
            Array_fd_master_rect_t[i][2],
            Array_fd_master_rect_t[i][3]);
    }

    ////////////////////////////////////////////////
    // log
    PLUGIN_LOGD("[FlashMultiFrameDenoiseYuv] frame(%d) count(%d/%d) iso(%d) digitalGain(%d) flashMode(%d) facing(%d) llsState(%d) analogGain(%d) numOfDetectedFaces(%d)",
            frameCount,
            frameMaxIndex, frameCurIndex,
            iso,
            digitalGain,
            flashMode,
            rearOrFront,
            llsState,
            analogGain,
            numOfDetectedFaces);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInCombineReprocessing::m_processBeautyFaceYuv(Map_t *map)
{
    status_t ret = NO_ERROR;

    // frameCount & Index
    int frameCount = m_getFrameCount(map);
    int frameMaxIndex = m_getFrameMaxIndex(map);
    int frameCurIndex = m_getFrameCurIndex(map);

    // log
    PLUGIN_LOGD("[BeautyFaceYuv] frame(%d) count(%d/%d)",
        frameCount,
        frameMaxIndex, frameCurIndex);

    return ret;
}

status_t ExynosCameraPlugInCombineReprocessing::m_processSuperResolution(Map_t *map)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // frameCount & Index
    int frameCount = m_getFrameCount(map);
    int frameMaxIndex = m_getFrameMaxIndex(map);
    int frameCurIndex = m_getFrameCurIndex(map);

    ////////////////////////////////////////////////
    // log
    PLUGIN_LOGD("[SuperResolution] frame(%d) count(%d/%d)",
        frameCount,
        frameMaxIndex, frameCurIndex);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInCombineReprocessing::m_processOisDenoiseYuv(Map_t *map)
{
    status_t ret = NO_ERROR;

    int frameCount    = m_getFrameCount(map);
    int frameMaxIndex = m_getFrameMaxIndex(map);
    int frameCurIndex = m_getFrameCurIndex(map);
    int iso = (Data_uint32_t)(*map)[PLUGIN_ISO];
    int rearOrFront = (Data_int32_t)(*map)[PLUGIN_HIFI_CAMERA_TYPE];
    uint32_t jpegOrientation = (Data_uint32_t)(*map)[PLUGIN_JPEG_ORIENTATION];
    Pointer_int_t aeRegion = (Pointer_int_t)(*map)[PLUGIN_AE_REGION];

    ret = m_setAnalogGain(frameCurIndex, (Data_uint32_t)(*map)[PLUGIN_ANALOG_GAIN]);
    if (ret != NO_ERROR) {
        PLUGIN_LOGE("fail to setAnalogGain. ret(%d)", ret);
        return ret;
    }

    ret = m_setShutterTime(frameCurIndex, (Data_int64_t)(*map)[PLUGIN_EXPOSURE_TIME]);
    if (ret != NO_ERROR) {
        PLUGIN_LOGE("fail to setShutterTime. ret(%d)", ret);
        return ret;
    }

    PLUGIN_LOGD("[OisDenoiseYuv] frame(%d) count(%d/%d) iso(%d) facing(%d) jpegOrientation(%d) aeRegion(%d, %d, %d, %d)",
            frameCount,
            frameMaxIndex,
            frameCurIndex,
            iso,
            rearOrFront,
            jpegOrientation,
            aeRegion[0], aeRegion[1], aeRegion[2], aeRegion[3]);

    return ret;
}

status_t ExynosCameraPlugInCombineReprocessing::m_processSportsYuv(Map_t *map)
{
    status_t ret = NO_ERROR;

    ////////////////////////////////////////////////
    // frameCount & Index
    int frameCount    = m_getFrameCount(map);
    int frameMaxIndex = m_getFrameMaxIndex(map);
    int frameCurIndex = m_getFrameCurIndex(map);

    ////////////////////////////////////////////////
    // iso
    int iso = 0;
    iso             = (Data_uint32_t)(*map)[PLUGIN_ISO];

    ////////////////////////////////////////////////
    // digital gain
    unsigned int digitalGain = 0;
    digitalGain     = (Data_uint32_t)(*map)[PLUGIN_DIGITAL_GAIN];

    ////////////////////////////////////////////////
    // flashMode
    int flashMode = 0;
    flashMode       = (Data_int32_t)(*map)[PLUGIN_FLASH_MODE];

    ////////////////////////////////////////////////
    // camera type
    int rearOrFront = 0;
    rearOrFront     = (Data_int32_t)(*map)[PLUGIN_HIFI_CAMERA_TYPE];

    ////////////////////////////////////////////////
    // llsState
    int llsState = 0;
    llsState        = (Data_int32_t)(*map)[PLUGIN_LLS_INTENT];

    ////////////////////////////////////////////////
    // analog gain
    unsigned int analogGain = 0;
    analogGain      = (Data_uint32_t)(*map)[PLUGIN_ANALOG_GAIN];

    ////////////////////////////////////////////////
    // face detection
    int numOfDetectedFaces = 0;
    numOfDetectedFaces = (Data_int32_t)(*map)[PLUGIN_FACE_NUM];

    unsigned int(*Array_fd_master_rect_t)[4];
    Array_fd_master_rect_t = (unsigned int (*)[4])(*map)[PLUGIN_MASTER_FACE_RECT];

    for (int i = 0; i < numOfDetectedFaces; i++) {
        PLUGIN_LOGV("[SportsYuv] [%d / %d] : %d, %d, %d, %d",
            i,
            numOfDetectedFaces,
            Array_fd_master_rect_t[i][0],
            Array_fd_master_rect_t[i][1],
            Array_fd_master_rect_t[i][2],
            Array_fd_master_rect_t[i][3]);
    }

    ////////////////////////////////////////////////
    // gyro
    Pointer_float_t gyro = (Pointer_float_t)(*map)[PLUGIN_GYRO_FLOAT_ARRAY];

    ////////////////////////////////////////////////
    // aecSettled
    int aecSettled = 0;
    aecSettled = (Data_uint32_t)(*map)[PLUGIN_AEC_SETTLED];

    ////////////////////////////////////////////////
    // log
    PLUGIN_LOGD("[SportsYuv] frame(%d) count(%d/%d) iso(%d) digitalGain(%d) flashMode(%d) facing(%d) llsState(%d) analogGain(%d) numOfDetectedFaces(%d) gyro(%.2f, %.2f, %.2f) aecSettled(%d)",
            frameCount,
            frameMaxIndex, frameCurIndex,
            iso,
            digitalGain,
            flashMode,
            rearOrFront,
            llsState,
            analogGain,
            numOfDetectedFaces,
            gyro[0],
            gyro[1],
            gyro[2],
            aecSettled);

    ////////////////////////////////////////////////

    return ret;
}

int ExynosCameraPlugInCombineReprocessing::m_getFrameMaxIndex(Map_t *map)
{
    int frameMaxIndex = 0;
    frameMaxIndex = (Data_int32_t)(*map)[PLUGIN_HIFI_TOTAL_BUFFER_NUM];

    return frameMaxIndex;
}

int ExynosCameraPlugInCombineReprocessing::m_getFrameCurIndex(Map_t *map)
{
    int frameCurIndex = 0;
    frameCurIndex = (Data_int32_t)(*map)[PLUGIN_HIFI_CUR_BUFFER_NUM];

    return frameCurIndex;
}

status_t ExynosCameraPlugInCombineReprocessing::m_setAnalogGain(int frameCurIndex, unsigned int analogGain)
{
    static int analogGainArraySize = (sizeof(m_analogGain) / sizeof(m_analogGain[0]));

    if (frameCurIndex >= analogGainArraySize) {
        PLUGIN_LOGE("invalid frameCurIndex(%d), analogGain index size(%d)",
                    frameCurIndex, analogGainArraySize);
        return INVALID_OPERATION;
    }

    m_analogGain[frameCurIndex] = analogGain;

    return NO_ERROR;
}

status_t ExynosCameraPlugInCombineReprocessing::m_setShutterTime(int frameCurIndex, int64_t shutterTime)
{
    static int shutterTimeArraySize = (sizeof(m_shutterTime) / sizeof(m_shutterTime[0]));

    if (frameCurIndex >= shutterTimeArraySize) {
        PLUGIN_LOGE("invalid frameCurIndex(%d), shutterTime index size(%d)",
                    frameCurIndex, shutterTimeArraySize);
        return INVALID_OPERATION;
    }

    m_shutterTime[frameCurIndex] = shutterTime;

    return NO_ERROR;
}

}
