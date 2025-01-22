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

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCameraMetadataConverterVendor"

#include "ExynosCameraMetadataConverter.h"
#include "ExynosCameraRequestManager.h"
#ifdef USE_SLSI_VENDOR_TAGS
#include "ExynosCameraVendorUtils.h"
#include "ExynosCameraVendorTags.h"
#endif
#include "ExynosCameraUtils.h"

#ifdef SUPPORT_VENDOR_TAG_FACTORY_LED_CALIBRATION
#include "ExynosCameraLEDCalibrationMap.h"
#endif

namespace android {

void ExynosCameraMetadataConverter::m_constructVendorDefaultRequestSettings(__unused int type, CameraMetadata *settings)
{
    /** android.stats */
    uint8_t faceDetectMode = ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;

    settings->update(ANDROID_STATISTICS_FACE_DETECT_MODE, &faceDetectMode, 1);

#ifdef SUPPORT_VENDOR_TAG_MF_STILL
    uint8_t mfmode = SLSI_MF_STILL_MODE_ONDEMAND;
    settings->update(SLSI_MF_STILL_MODE, &mfmode, 1);

    uint8_t mfcapture = SLSI_MF_STILL_FUNCTIONS_NONE;
    settings->update(SLSI_MF_STILL_CAPTURE, &mfcapture, 1);

    uint8_t mfresult = SLSI_MF_STILL_FUNCTIONS_NONE;
    settings->update(SLSI_MF_STILL_RESULT, &mfresult, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_NIGHT_SHOT
    uint8_t nightShot = 0;
    settings->update(EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_BAYER, &nightShot, 1);
    settings->update(EXYNOS_ANDROID_VENDOR_NIGHT_SHOT, &nightShot, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_SUPER_NIGHT_SHOT
    uint8_t superNightShot = 0;
    settings->update(EXYNOS_ANDROID_VENDOR_SUPER_NIGHT_SHOT_BAYER, &superNightShot, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_HDR
    uint8_t hdr = 0;
    settings->update(EXYNOS_ANDROID_VENDOR_HDR_BAYER, &hdr, 1);
    settings->update(EXYNOS_ANDROID_VENDOR_HDR_YUV, &hdr, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_MULTI_FRAME_DENOISE
    uint8_t multiFrameDenoise = 0;
    settings->update(EXYNOS_ANDROID_VENDOR_FLASH_MULTI_FRAME_DENOISE_YUV, &multiFrameDenoise, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_SPORT_SHOT
    uint8_t sportYuv = 0;
    settings->update(EXYNOS_ANDROID_VENDOR_SPORTS_YUV, &sportYuv, 1);

    uint8_t sportYuvMotionLevel = 0;
    settings->update(EXYNOS_ANDROID_VENDOR_SPORTS_YUV_MOTION_LEVEL, &sportYuvMotionLevel, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_COMBINE_SINGLE_CAPTURE
    uint8_t arSticker = 0;
    settings->update(EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE, &arSticker, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_REMOSAIC
    if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_REMOSAIC)) {
        uint8_t remosaicCapture = EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_NONE;
        settings->update(EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION, &remosaicCapture, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_3DHDR
    if (m_sensorStaticInfo->supported_sensor_ex_mode & (1 << EXTEND_SENSOR_MODE_3DHDR)) {
        uint8_t hdrMode = EXYNOS_CONTROL_HDR_MODE_OFF;
        settings->update(EXYNOS_ANDROID_VENDOR_CONTROL_3DHDR, &hdrMode, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_LONG_EXPOSURE_CAPTURE
    uint8_t longExposureCapture = EXYNOS_CONTROL_LONG_EXPOSURE_CAPTURE_OFF;
    settings->update(EXYNOS_ANDROID_VENDOR_CONTROL_LONG_EXPOSURE_CAPTURE, &longExposureCapture, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_SCENE_DETECTION
    const int64_t sceneDetectInfo[7] = {0, 0, 0, 0, 0, 0, 0};
    settings->update(EXYNOS_ANDROID_CONTROL_SCENE_DETECTION_INFO, sceneDetectInfo, ARRAY_LENGTH(sceneDetectInfo));
#endif

#ifdef USES_COMBINE_PLUGIN
    const int32_t sceneType = 0;
    settings->update(EXYNOS_ANDROID_VENDOR_SCENE_TYPE, &sceneType, 1);

    uint8_t oisDenoiseYuv = EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV_OFF;
    settings->update(EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV, &oisDenoiseYuv, 1);
#endif

#ifdef USE_HW_BEAUTY_CONTROL
    if (m_cameraId == CAMERA_ID_BACK) {
        int32_t beautyFaceStrengthDefault = BEAUTY_FACE_STRENGTH_DEFAULT;
        settings->update(EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_STRENGTH, &beautyFaceStrengthDefault, 1);
    }
#endif
#ifdef USES_SUPER_RESOLUTION
    uint8_t superResolution = EXYNOS_ANDROID_VENDOR_SUPER_RESOLUTION_OFF;
    settings->update(EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_RESOLUTION, &superResolution, 1);
#endif

#ifdef USE_SUPER_EIS
    uint8_t superResolution = EXYNOS_ANDROID_VENDOR_SUPER_EIS_OFF;
    settings->update(EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_EIS, &superResolution, 1);
#endif

    return;
}

void ExynosCameraMetadataConverter::m_constructVendorStaticInfo(struct ExynosCameraSensorInfoBase *sensorStaticInfo,
                                                                CameraMetadata *info, cameraId_Info *camIdInfo, int camIdx)
{
    Vector<int32_t> i32Vector;
    Vector<uint8_t> ui8Vector;
    status_t ret = NO_ERROR;
    int cameraId = camIdInfo->cameraId[camIdx];

#ifdef SUPPORT_VENDOR_TAG_MF_STILL
    String8 mfstillVersion;
    mfstillVersion.append(PLUGIN_HIFILLS_VERSION);
    ret = info->update(SLSI_MF_STILL_VERSION, mfstillVersion);
    if (ret < 0) {
        CLOGD2("SLSI_MF_STILL_VERSION update failed(%d)", ret);
    }
#endif

    String8 sensorName;
    sensorName.clear();

#ifdef SUPPORT_VENDOR_TAG_SENSOR_NAME
    sensorName.append(sensorStaticInfo->name);
    ret = info->update(EXYNOS_ANDROID_VENDOR_SENSOR_INFO, sensorName);
    if (ret < 0) {
        CLOGD2("EXYNOS_ANDROID_VENDOR_SENSOR_INFO update failed(%d)", ret);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_SENSOR_INFO_ARCSOFT_DUAL_CALIB_BLOB
    /* Arcsoft dual calibration is only for rear main camera */
    if (isBackCamera(cameraId)) {
        uint8_t arcsoftDualCalibBlob[ARCSOFT_DUAL_CALIB_BLOB_SIZE];

        /* Check whether re-calibration data exist.
           If re-calibration data exist, check whether module id match with installed module.
           If matched, apply re-calibration data to APP. Otherwise, apply calibration data in module*/
#if defined(SENSOR_FW_GET_FROM_FILE)
        uint8_t read_module_id[DUMP_SERIAL_NUMBER_SIZE] = {0};
        FILE *fp = fopen(ARCSOFT_DUAL_RE_CALIB_MODULE_ID_PATH, "r");
        if (fp != NULL) {
            fread(read_module_id, 1, DUMP_SERIAL_NUMBER_SIZE, fp);
            CLOGD2("Reading from %s", ARCSOFT_DUAL_RE_CALIB_MODULE_ID_PATH);
            fclose(fp);
        } else {
            CLOGD2("Cannot open file name: %s", ARCSOFT_DUAL_RE_CALIB_MODULE_ID_PATH);
        }

        int     sensorfwSize = 0;
        char    *eepromData = NULL;
        eepromData = (char *)getSensorFWFromFile(sensorStaticInfo, cameraId, &sensorfwSize);
        memcpy(sensorStaticInfo->sensor_fw, eepromData, sensorfwSize);
#endif
        char *pSeriesBuff = &sensorStaticInfo->sensor_fw[DUMP_SERIAL_NUMBER_OFFSET];
        for (int i = 0; i < DUMP_SERIAL_NUMBER_SIZE; i++)
            CLOGV2("Serial Number buffer[%d]:0x%02x == 0x%02x", i, read_module_id[i], pSeriesBuff[i]);
        CLOGD2("Serial Number compare result:%d", memcmp(read_module_id, pSeriesBuff, DUMP_SERIAL_NUMBER_SIZE));

        FILE *file = fopen(ARCSOFT_DUAL_RE_CALIB_BLOB_PATH, "r");
        if (file != NULL && !memcmp(read_module_id, pSeriesBuff, DUMP_SERIAL_NUMBER_SIZE)) {
            uint8_t *dual_buffer;
            fseek(file, 0, SEEK_END);
            int fileLen = ftell(file);
            fseek(file, 0, SEEK_SET);
            CLOGD2("Loading arcsoft dual cal data from phone memory fileLen=%d", fileLen);
            dual_buffer = (uint8_t *)malloc(fileLen);
            if (dual_buffer != NULL) {
                if (ARCSOFT_DUAL_CALIB_BLOB_SIZE == fileLen) {
                    fread(dual_buffer, 1, fileLen, file);
                    /* copy to eeprom custom data for HAL */
                    memcpy(arcsoftDualCalibBlob, dual_buffer,
                            ARCSOFT_DUAL_CALIB_BLOB_SIZE);
                    CLOGD2("arcsoft dual calibration data file is fine!!!");
                } else {
                    memset(arcsoftDualCalibBlob, 0x00,
                            ARCSOFT_DUAL_CALIB_BLOB_SIZE);
                    CLOGD2("arcsoft dual calibration data file is damaged!!!");
                }
            } else {
                CLOGD2("dual_buffer is null");
            }

            if (dual_buffer) {
                free(dual_buffer);
            }
            
        } else {
            CLOGD2("Loading arcsoft dual cal data from camera module");
            char filePath[70];
            memset(filePath, 0, sizeof(filePath));
            snprintf(filePath, sizeof(filePath), "%s", ARCSOFT_DUAL_CALIB_BLOB_PATH);
            ret = readFromFile(filePath, (char *)arcsoftDualCalibBlob, sizeof(arcsoftDualCalibBlob));
            if (ret != NO_ERROR) {
                CLOGD2("Failed to readFromFile(dual_calib data file)\n");
            }
        }

        if(file != NULL)
            fclose(file);

        ret = info->update(EXYNOS_ANDROID_VENDOR_SENSOR_INFO_ARCSOFT_DUAL_CALIB_BLOB,
            arcsoftDualCalibBlob, ARCSOFT_DUAL_CALIB_BLOB_SIZE);
        if (ret < 0) {
            CLOGD2("EXYNOS_ANDROID_VENDOR_SENSOR_INFO_ARCSOFT_DUAL_CALIB_BLOB(size : %d) update failed(%d)",
                ARCSOFT_DUAL_CALIB_BLOB_SIZE, ret);
        }
    }
#endif

#ifdef SUPPORT_VEDNOR_TAG_SENSOR_FPS_SUPPORT_RANGE
    if (sensorStaticInfo->hiddenfpsRangeList != NULL) {
        Vector<int32_t> fpsRanges;
        int (*fpsRangesList)[2] = NULL;
        size_t fpsRangesLength = 0;

        fpsRangesList = sensorStaticInfo->hiddenfpsRangeList;
        fpsRangesLength = sensorStaticInfo->hiddenfpsRangeListMax;

        fpsRanges.setCapacity(fpsRangesLength * 2);

        for (size_t i = 0; i < fpsRangesLength; i++) {
            fpsRanges.add(fpsRangesList[i][0]/1000);
            fpsRanges.add(fpsRangesList[i][1]/1000);
        }

        ret = info->update(EXYNOS_ANDROID_VENDOR_SENSOR_VENDOR_FPS_SUPPORT_RANGE,
                fpsRanges.array(), fpsRanges.size());
        if (ret < 0) {
            CLOGD2("EXYNOS_ANDROID_VENDOR_SENSOR_VENDOR_FPS_SUPPORT_RANGE update failed(%d)", ret);
        }
    }
#endif

    int sensorId = getSensorId(cameraId);
#ifdef USE_DUAL_CAMERA
#ifdef SUPPORT_VENDOR_TAG_AUX_INFO
    //the vendor tag is for aux sensor, if the sensor is not aux sensor,
    //do not add the vendor tag it.
    //the question is how to know it is aux sensor
    if (sensorId < 0) {
        ALOGE("[CAM_ID(%d)]-ERR(%s[%d]):Inavalid sensorId %d",
                cameraId, __FUNCTION__, __LINE__, sensorId);
    }
    if (sensorId == SENSOR_NAME_S5K5E9) {
        uint8_t is_aux_bayer = 1;
        uint8_t is_aux_master = 0;
        info->update(EXYNOS_ANDROID_VENDOR_DUALCAM_IS_AUX_BAYER, &is_aux_bayer, 1);
        info->update(EXYNOS_ANDROID_VENDOR_DUALCAM_IS_AUX_MASTER, &is_aux_master, 1);
    }
#endif
#endif
#ifdef SUPPORT_VENDOR_TAG_REMOSAIC
    if (sensorId == SENSOR_NAME_S5K2X5SP) {
        uint8_t is_remosaic_sensor = 1;
        uint8_t is_remosaic_by_hw = 1;
        int32_t max_dim[2];
        int32_t yuv_in_out_size[4];
        int32_t high_res_size[6];
        max_dim[0] = sensorStaticInfo->maxPictureW;
        max_dim[1] = sensorStaticInfo->maxPictureH;
        high_res_size[0] = 5760;
        high_res_size[1] = 4320;
        high_res_size[2] = 5760;
        high_res_size[3] = 3240;
        high_res_size[4] = 5760;
        high_res_size[5] = 2464;
        yuv_in_out_size[0] = 2880; //sensorStaticInfo->yuvList[0][0];
        yuv_in_out_size[1] = 2160; //sensorStaticInfo->yuvList[0][1];
        yuv_in_out_size[2] = 2880; //sensorStaticInfo->yuvList[0][0];
        yuv_in_out_size[3] = 2160; //sensorStaticInfo->yuvList[0][1];
        info->update(EXYNOS_ANDROID_VENDOR_REMOSAIC_IS_REMOSAIC_SENSOR, &is_remosaic_sensor, 1);
        info->update(EXYNOS_ANDROID_VENDOR_REMOSAIC_IS_BY_HW, &is_remosaic_by_hw, 1);
        info->update(EXYNOS_ANDROID_VENDOR_REMOSAIC_RAW_DIM, max_dim, 2);
        info->update(EXYNOS_ANDROID_VENDOR_REMOSAIC_MAX_YUV_IN_OUT_SIZE, yuv_in_out_size, 4);
        info->update(EXYNOS_ANDROID_VENDOR_REMOSAIC_HIGH_RESOLUTION_SIZES, high_res_size, 6);
    }

    int supportRemosaicMode = 0;
    supportRemosaicMode = sensorStaticInfo->supported_remosaic_mode;
    if (supportRemosaicMode > 0) {
        uint8_t supportedMode[2];
        int index = 0;
        if (supportRemosaicMode & (1 << SUPPORT_REMOSAIC_MODE_HW))
            supportedMode[index++] = EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_ON_HW;

        if (supportRemosaicMode & (1 << SUPPORT_REMOSAIC_MODE_SW))
            supportedMode[index++] = EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_ON_SW;

        info->update(EXYNOS_ANDROID_VENDOR_REMOSAIC_SUPPORTED, supportedMode, index);
    }
#endif
#ifdef SUPPORT_VENDOR_TAG_3DHDR
    if (sensorId == SENSOR_NAME_S5K2X5SP) {
        uint8_t sensor_3d_hdr_supported = 1;
        info->update(EXYNOS_ANDROID_VENDOR_SENSOR_INFO_3D_HDR_SUPPORTED, &sensor_3d_hdr_supported, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_VIDEO_ACTION
    if (sensorId == SENSOR_NAME_OV16885C) {
        uint8_t is_video_action = 1;
        info->update(EXYNOS_ANDROID_VENDOR_DUALCAM_IS_VIDEO_ACTION, &is_video_action, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_QUAD_PIXEL
    if (sensorId == SENSOR_NAME_S5KGM1SP) {
        uint8_t sensor_quad_pixel = 1;
        info->update(EXYNOS_ANDROID_VENDOR_SENSOR_QUAD_PIXEL_SUPPORT, &sensor_quad_pixel, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_VSTAB
    int32_t vstabMaxSize[2] = {0, 0};
    int32_t vstabMaxFps = 0;
    if (CAMERA_ID_BACK == cameraId || CAMERA_ID_BACK_2 == cameraId) {
        vstabMaxSize[0] = 1920;
        vstabMaxSize[1] = 1080;
        vstabMaxFps = 60;
    }
    info->update(EXYNOS_ANDROID_VENDOR_CONTROL_VSTAB_MAX_SIZE, vstabMaxSize, 2);
    info->update(EXYNOS_ANDROID_VENDOR_CONTROL_VSTAB_MAX_FPS, &vstabMaxFps, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_LONG_EXPOSURE_CAPTURE
    ret = info->update(EXYNOS_ANDROID_VENDOR_SENSOR_VENDOR_EXPOSURE_SUPPORT_RANGE,
            sensorStaticInfo->vendorExposureTimeRange,
            ARRAY_LENGTH(sensorStaticInfo->vendorExposureTimeRange));
    if (ret < 0)
        CLOGD2("EXYNOS_ANDROID_VENDOR_SENSOR_VENDOR_EXPOSURE_SUPPORT_RANGE update failed(%d)", ret);
#endif

#ifdef USE_HW_BEAUTY_CONTROL
    if (isBackCamera(cameraId)) {
        int32_t beautyFaceStrengthRange[RANGE_TYPE_MAX] = {BEAUTY_FACE_STRENGTH_MIN, BEAUTY_FACE_STRENGTH_MAX};

        ret = info->update(EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_STRENGTH_RANGE,
            beautyFaceStrengthRange,  ARRAY_LENGTH(beautyFaceStrengthRange));
        if (ret < 0)
            CLOGD2("EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_STRENGTH_RANGE update is failed(%d)", ret);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_ABORT_CAPTURE
    uint8_t enableDefault = 1;
    ret = info->update(EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_ENABLE, &enableDefault, 1);
    if (ret < 0)
        CLOGD2("EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_ENABLE update is failed(%d)", ret);
#endif

    return;
}

void ExynosCameraMetadataConverter::m_setSessionVendorParams()
{
#ifdef SUPPORT_SESSION_PARAMETERS
    camera_metadata_entry_t entry;

    /* check fps range for high speed */
    /* ANDROID_CONTROL_AE_TARGET_FPS_RANGE */
    entry = m_sessionParams.find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
    if (entry.count > 0) {
        uint32_t fps;
        fps = entry.data.i32[1];
        m_configurations->setModeValue(CONFIGURATION_RECORDING_FPS, fps);
        CLOGD("ANDROID_CONTROL_AE_TARGET_FPS_RANGE(%d-%d) fps(%d)", entry.data.i32[0], entry.data.i32[1], fps);
    }

    // init
    m_configurations->setMode(CONFIGURATION_SESSION_MODE, false);

    entry = m_sessionParams.find(EXYNOS_ANDROID_VENDOR_SESSION_MODE);
    for (int i = 0; i < entry.count; i++) {
        int mode = entry.data.u8[i];
        int value = 0;

        switch(mode) {
        case EXYNOS_SESSION_MODE_PRO:
            value = 1;
            break;
        case EXYNOS_SESSION_MODE_REMOSAIC:
#ifdef SUPPORT_VENDOR_DYNAMIC_SENSORMODE_BY_SESSION_PARAMETER
            m_configurations->setMode(CONFIGURATION_FULL_SIZE_SENSOR_LUT_MODE, true);
            m_configurations->setModeValue(CONFIGURATION_FULL_SIZE_SENSOR_LUT_FPS_VALUE, FRONT_0_DYNAMIC_FULL_SENSOR_FPS);
#endif
#ifdef SUPPORT_REMOSAIC_MODE_BY_SESSION_PARAMETER
            value = 1;
#endif
            break;
        case EXYNOS_SESSION_MODE_LED_CAL:
            value = 1;
            break;
        default:
            CLOGE("invalid mode(%d)", mode);
            break;
        }

        if (value) {
            m_configurations->setMode(CONFIGURATION_SESSION_MODE, true);
            m_configurations->setModeMultiValue(CONFIGURATION_MULTI_SESSION_MODE_VALUE, mode, value);
            CLOGD("[%d] CONFIGURATION_SESSION_MODE(%d,%d)", i, mode, value);
        }
    }

#ifdef USE_SLSI_VENDOR_TAGS
    entry = m_sessionParams.find(EXYNOS_ANDROID_VENDOR_CONTROL_ZERO_CAMERA);
    if (entry.count > 0) {
        m_configurations->setMode(CONFIGURATION_ZERO_CAMERA_MODE, entry.data.u8[0] == 1 ? true : false);
        CLOGD("EXYNOS_ANDROID_VENDOR_CONTROL_ZERO_CAMERA (%d)", entry.data.u8[0]);
    }
#endif

#ifdef USES_COMBINE_PLUGIN
    entry = m_sessionParams.find(EXYNOS_ANDROID_VENDOR_COMBINE_PREVIEW_PLUGIN);
    if (entry.count > 0) {
        switch(entry.data.u8[0]) {
        case EXYNOS_COMBINE_PREVIEW_PLUGIN_DISABLE:
            m_configurations->setMode(CONFIGURATION_COMBINE_PREVIEW_PLUGIN, false);
            m_configurations->setModeValue(CONFIGURATION_COMBINE_PREVIEW_PLUGIN_VALUE, entry.data.u8[0]);
            break;
        case EXYNOS_COMBINE_PREVIEW_PLUGIN_ENABLE:
            m_configurations->setMode(CONFIGURATION_COMBINE_PREVIEW_PLUGIN, true);
            m_configurations->setModeValue(CONFIGURATION_COMBINE_PREVIEW_PLUGIN_VALUE, entry.data.u8[0]);
            break;
        }
        CLOGD("CONFIGURATION_PICTURE_FORMAT(%d)", entry.data.u8[0]);
    }
#endif

#ifdef USE_SUPER_EIS
    bool newEisMode = false;
    entry = m_sessionParams.find(EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_EIS);
    if (entry.count > 0) {
        switch (entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_SUPER_EIS_OFF:
            CLOGV("EXYNOS_ANDROID_VENDOR_SUPER_EIS_OFF(%d)", entry.data.u8[0]);
            newEisMode = false;
            break;
        case EXYNOS_ANDROID_VENDOR_SUPER_EIS_ON:
            CLOGV("EXYNOS_ANDROID_VENDOR_SUPER_EIS_ON(%d)", entry.data.u8[0]);
            newEisMode = true;
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_EIS(%d", entry.data.u8[0]);
            break;
        }

        CLOGD("EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_EIS (%d)", newEisMode);
        m_configurations->setMode(CONFIGURATION_SUPER_EIS_MODE, newEisMode);
    }
#endif

#endif
}

status_t ExynosCameraMetadataConverter::initShotVendorData(struct camera2_shot *shot)
{
    /* utrl */
    shot->uctl.isModeUd.paf_mode = CAMERA_PAF_OFF;
    shot->uctl.isModeUd.wdr_mode = CAMERA_WDR_OFF;
#ifdef SUPPORT_DEPTH_MAP
    if (m_configurations->getMode(CONFIGURATION_DEPTH_MAP_MODE)) {
        shot->uctl.isModeUd.disparity_mode = CAMERA_DISPARITY_CENSUS_CENTER;
    } else
#endif
    {
        shot->uctl.isModeUd.disparity_mode = CAMERA_DISPARITY_OFF;
    }

        shot->uctl.opMode = CAMERA_OP_MODE_HAL3_GED;

    shot->uctl.vtMode = (enum camera_vt_mode)m_configurations->getModeValue(CONFIGURATION_VT_MODE);

#ifdef USE_HW_BEAUTY_CONTROL
    shot->ctl.facebeauty.strength = BEAUTY_FACE_STRENGTH_DEFAULT;
#endif

    return OK;
}

void ExynosCameraMetadataConverter::translatePreVendorControlControlData(__unused CameraMetadata *settings,
                                                                         struct camera2_shot_ext *dst_ext,
                                                                         CameraMetadata *prevMeta,
                                                                         int32_t physCamID)
{
    struct camera2_shot *dst = NULL;

    dst = &dst_ext->shot;

    return;
}

void ExynosCameraMetadataConverter::translateVendorControlControlData(ExynosCameraRequestSP_sprt_t request,
                                                                      __unused CameraMetadata *settings,
                                                                      struct camera2_shot_ext *dst_ext,
                                                                      CameraMetadata *prevMeta,
                                                                      int32_t physCamID)
{
    struct camera2_shot *dst = NULL;
    bool isMetaExist = false;
    uint8_t prev_valueU8 = 0;

    dst = &dst_ext->shot;

    camera_metadata_entry_t entry;
    camera_metadata_entry_t prev_entry;

    /* EXYNOS_ANDROID_CONTROL_SCENE_DETECTION_INFO */
#ifdef SUPPORT_VENDOR_TAG_SCENE_DETECTION
    entry = settings->find(EXYNOS_ANDROID_CONTROL_SCENE_DETECTION_INFO);
    if (entry.count > 0) {
        ExynosRect2 sceneObjectRegion;

        dst->uctl.sceneDetectInfoUd.timeStamp = (uint64_t)(entry.data.i64[0]);
        dst->uctl.sceneDetectInfoUd.scene_index = (enum camera2_scene_index)(entry.data.i64[1]);
        dst->uctl.sceneDetectInfoUd.confidence_score = (uint32_t)(entry.data.i64[2]);

        /* entry.data.i32[3] : left */
        /* entry.data.i32[4] : top */
        /* entry.data.i32[5] : width */
        /* entry.data.i32[6] : height */
        if (entry.data.i32[5] > 0 && entry.data.i32[6] > 0) {
            sceneObjectRegion.x1 = (uint32_t)(entry.data.i64[3]);      /* left */
            sceneObjectRegion.y1 = (uint32_t)(entry.data.i64[4]);      /* top */
            sceneObjectRegion.x2 = (uint32_t)(sceneObjectRegion.x1 + entry.data.i32[5] - 1);       /* right */
            sceneObjectRegion.y2 = (uint32_t)(sceneObjectRegion.y1 + entry.data.i32[6] - 1);       /* bottom */

            m_convertActiveArrayTo3AARegion(&sceneObjectRegion, "OBJ");
            dst->uctl.sceneDetectInfoUd.object_roi[0] = sceneObjectRegion.x1;
            dst->uctl.sceneDetectInfoUd.object_roi[1] = sceneObjectRegion.y1;
            dst->uctl.sceneDetectInfoUd.object_roi[2] = sceneObjectRegion.x2;
            dst->uctl.sceneDetectInfoUd.object_roi[3] = sceneObjectRegion.y2;
        } else {
            dst->uctl.sceneDetectInfoUd.object_roi[0] = 0;   /* left */
            dst->uctl.sceneDetectInfoUd.object_roi[1] = 0;   /* top */
            dst->uctl.sceneDetectInfoUd.object_roi[2] = 0;   /* right */
            dst->uctl.sceneDetectInfoUd.object_roi[3] = 0;   /* bottom */
        }

        prev_entry = m_prevMeta->find(EXYNOS_ANDROID_CONTROL_SCENE_DETECTION_INFO);
        if (prev_entry.count > 0) {
            if (prev_entry.data.i64[1] != dst->uctl.sceneDetectInfoUd.scene_index) {
                    CLOGD("timestamp(%lld->%lld),scene_index(%lld->%lld),confidence_score(%lld->%lld),\
                                    object_roi(%lld,%lld,%lld,%lld->%lld,%lld,%lld,%lld)",
                        (long long)prev_entry.data.i64[0],
                        (long long)entry.data.i64[0],
                        (long long)prev_entry.data.i64[1],
                        (long long)entry.data.i64[1],
                        (long long)prev_entry.data.i64[2],
                        (long long)entry.data.i64[2],
                        (long long)prev_entry.data.i64[3], (long long)prev_entry.data.i64[4],
                        (long long)prev_entry.data.i64[5], (long long)prev_entry.data.i64[6],
                        (long long)entry.data.i64[3], (long long)entry.data.i64[4],
                        (long long)entry.data.i64[5], (long long)entry.data.i64[6]);
            }
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_FLIP_STILL
    bool flipStilH = false;
    bool flipStilV = false;

    entry = settings->find(EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_STIL);
    if (entry.count > 0) {
        switch (entry.data.i32[0]) {
        case 0:
            break;
        case 1:
            flipStilH = true;
            flipStilV = false;
            break;
        case 2:
            flipStilH = false;
            flipStilV = true;
            break;
        case 3:
            flipStilH = true;
            flipStilV = true;
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_STIL value(%d)",
                    entry.data.i32[0]);
            break;
        }

        prev_entry = m_prevMeta->find(EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_STIL);
        if (prev_entry.count > 0) {
            if (prev_entry.data.i32[0] != entry.data.i32[0]) {
                    CLOGD("EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_STIL(%d->%d)",
                        prev_entry.data.i32[0],
                        entry.data.i32[0]);
            }
        }
    }

    if (flipStilH == true || flipStilV == true) {
        for (size_t i = 0; i < request->getNumOfOutputBuffer(); i++) {
            int id = request->getStreamIdwithBufferIdx(i);

            ////////////////////////////////////////////////
            // set only jpeg, according to customer scenario.
            switch (id % HAL_STREAM_ID_MAX) {
            case HAL_STREAM_ID_JPEG:
                request->setFlipHorizontal(id, flipStilH);
                request->setFlipVertical(id, flipStilV);

                CLOGD("[R%d] EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_STIL -> HAL_STREAM_ID_%d, flipStilH(%d), flipStilV(%d)",
                    request->getKey(), request->getStreamIdwithBufferIdx(i) % HAL_STREAM_ID_MAX, flipStilH, flipStilV);
                break;
            case HAL_STREAM_ID_RAW:
            case HAL_STREAM_ID_ZSL_INPUT:
            case HAL_STREAM_ID_ZSL_OUTPUT:
            case HAL_STREAM_ID_CALLBACK_STALL:
            case HAL_STREAM_ID_THUMBNAIL_CALLBACK:
            case HAL_STREAM_ID_YUV_INPUT:
            default:
                break;
            }
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_FLIP_VIDEO
    bool flipVideoH = false;
    bool flipVideoV = false;

    entry = settings->find(EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_VIDEO);
    if (entry.count > 0) {
        switch (entry.data.i32[0]) {
        case 0:
            break;
        case 1:
            flipVideoH = true;
            flipVideoV = false;
            break;
        case 2:
            flipVideoH = false;
            flipVideoV = true;
            break;
        case 3:
            flipVideoH = true;
            flipVideoV = true;
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_VIDEO value(%d)",
                    entry.data.i32[0]);
            break;
        }

        prev_entry = m_prevMeta->find(EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_VIDEO);
        if (prev_entry.count > 0) {
            if (prev_entry.data.i32[0] != entry.data.i32[0]) {
                    CLOGD("EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_VIDEO(%d->%d)",
                        prev_entry.data.i32[0],
                        entry.data.i32[0]);
            }
        }
    }

    ////////////////////////////////////////////////
    // we need to set on all capture related stream, even no flip.

    //request->setFlipHorizontal(HAL_STREAM_ID_PREVIEW,        flipVideoH);
    request->setFlipHorizontal(HAL_STREAM_ID_VIDEO,          flipVideoH);
    //request->setFlipHorizontal(HAL_STREAM_ID_CALLBACK,       flipVideoH);
    //request->setFlipHorizontal(HAL_STREAM_ID_DEPTHMAP,       flipVideoH);
    //request->setFlipHorizontal(HAL_STREAM_ID_DEPTHMAP_STALL, flipVideoH);
    //request->setFlipHorizontal(HAL_STREAM_ID_VISION,         flipVideoH);
    request->setFlipHorizontal(HAL_STREAM_ID_PREVIEW_VIDEO,  flipVideoH);

    //request->setFlipVertical(HAL_STREAM_ID_PREVIEW,        flipVideoV);
    request->setFlipVertical(HAL_STREAM_ID_VIDEO,          flipVideoV);
    //request->setFlipVertical(HAL_STREAM_ID_CALLBACK,       flipVideoV);
    //request->setFlipVertical(HAL_STREAM_ID_DEPTHMAP,       flipVideoV);
    //request->setFlipVertical(HAL_STREAM_ID_DEPTHMAP_STALL, flipVideoV);
    //request->setFlipVertical(HAL_STREAM_ID_VISION,         flipVideoV);
    request->setFlipVertical(HAL_STREAM_ID_PREVIEW_VIDEO,  flipVideoV);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_ROTATION_STILL
    bool jpegRotation = false;

    entry = settings->find(EXYNOS_ANDROID_VENDOR_CONTROL_ROTATION_STILL);
    if (entry.count > 0) {
        if (entry.data.i32[0] == 1) {
            jpegRotation = true;
            CLOGD("EXYNOS_ANDROID_VENDOR_CONTROL_ROTATION_STILL(%d)", entry.data.i32[0]);
        }
    }

#ifdef USE_ROTATION_STILL_ALWAYS
    if (jpegRotation == false) {
        jpegRotation = true;
        CLOGV("EXYNOS_ANDROID_VENDOR_CONTROL_ROTATION_STILL ALWAYS!!");
    }
#endif

    if (jpegRotation == true) {
        for (size_t i = 0; i < request->getNumOfOutputBuffer(); i++) {
            int id = request->getStreamIdwithBufferIdx(i);

            ////////////////////////////////////////////////
            // set only jpeg, according to customer scenario.
            switch (id % HAL_STREAM_ID_MAX) {
            case HAL_STREAM_ID_JPEG:
                request->setRotation(id, jpegRotation);

                CLOGD("[R%d] EXYNOS_ANDROID_VENDOR_CONTROL_ROTATION_STILL -> HAL_STREAM_ID_%d, jpegRotation(%d)",
                    request->getKey(), request->getStreamIdwithBufferIdx(i) % HAL_STREAM_ID_MAX, jpegRotation);
                break;
            case HAL_STREAM_ID_RAW:
            case HAL_STREAM_ID_ZSL_INPUT:
            case HAL_STREAM_ID_ZSL_OUTPUT:
            case HAL_STREAM_ID_CALLBACK_STALL:
            case HAL_STREAM_ID_THUMBNAIL_CALLBACK:
            case HAL_STREAM_ID_YUV_INPUT:
            default:
                break;
            }
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_MF_STILL
    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();

    entry = settings->find(SLSI_MF_STILL_MODE);
    if (entry.count > 0) {
        switch (entry.data.u8[0]) {
        case SLSI_MF_STILL_MODE_OFF ... SLSI_MF_STILL_MODE_ONDEMAND:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid SLSI_MF_STILL_MODE value(%d)", entry.data.u8[0]);
            break;
        }
    }

    entry = settings->find(SLSI_MF_STILL_CAPTURE);
    if (entry.count > 0) {
        switch (entry.data.u8[0]) {
        case SLSI_MF_STILL_FUNCTIONS_LLS:
        case SLSI_MF_STILL_FUNCTIONS_SR:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            vendorMeta->set(VENDORMETA::YUV_STALL_ON, checkVendorYUVStallMeta(request));
            CLOGD("[R%d] MFStillCapture(%d) VendorYUVStall(%d)", request->getKey(), entry.data.u8[0], getVendorYUVStallMeta(request));
            break;
        case SLSI_MF_STILL_FUNCTIONS_NONE:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid SLSI_MF_STILL_CAPTURE value(%d)", entry.data.u8[0]);
            break;
        }
    }

    entry = settings->find(SLSI_MF_STILL_PARAMETERS);
    if (entry.count > 0) {
        vendorMeta->update(entry.tag, entry.data.u8, entry.count);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_REMOSAIC
    entry = settings->find(EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        switch (entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_ON_HW:
        case EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_ON_SW:
            CLOGD("[R%d] RemosaicStillCapture(%d)", request->getKey(), entry.data.u8[0]);
            m_configurations->setModeValue(CONFIGURATION_REMOSAIC_CAPTURE_MODE_VALUE, entry.data.u8[0]);
            /* no break */
        case EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_NONE:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION value(%d)", entry.data.u8[0]);
            break;
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_NIGHT_SHOT
    entry = settings->find(EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_BAYER);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();

        switch (entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON:
            CLOGD("[R%d] EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_BAYER(%d)", request->getKey(), entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_NONE:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_BAYER value(%d)", entry.data.u8[0]);
            break;
        }
    }

    entry = settings->find(EXYNOS_ANDROID_VENDOR_NIGHT_SHOT);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();

        switch (entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON:
            CLOGD("[R%d] EXYNOS_ANDROID_VENDOR_NIGHT_SHOT(%d)", request->getKey(), entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_NONE:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_NIGHT_SHOT value(%d)", entry.data.u8[0]);
            break;
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_SUPER_NIGHT_SHOT
    entry = settings->find(EXYNOS_ANDROID_VENDOR_SUPER_NIGHT_SHOT_BAYER);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();

        switch (entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON:
            CLOGD("[R%d] EXYNOS_ANDROID_VENDOR_SUPER_NIGHT_SHOT_BAYER(%d)", request->getKey(), entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_NONE:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_SUPER_NIGHT_SHOT_BAYER value(%d)", entry.data.u8[0]);
            break;
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_HDR
    entry = settings->find(EXYNOS_ANDROID_VENDOR_HDR_BAYER);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();

        switch (entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_HDR_ON:
            CLOGD("[R%d] EXYNOS_ANDROID_VENDOR_HDR_BAYER(%d)", request->getKey(), entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        case EXYNOS_ANDROID_VENDOR_HDR_NONE:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_HDR_BAYER value(%d)", entry.data.u8[0]);
            break;
        }
    }

    entry = settings->find(EXYNOS_ANDROID_VENDOR_HDR_YUV);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();

        switch (entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_HDR_ON:
            CLOGD("[R%d] EXYNOS_ANDROID_VENDOR_HDR_YUV(%d)", request->getKey(), entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        case EXYNOS_ANDROID_VENDOR_HDR_NONE:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_HDR_YUV value(%d)", entry.data.u8[0]);
            break;
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_MULTI_FRAME_DENOISE
    entry = settings->find(EXYNOS_ANDROID_VENDOR_FLASH_MULTI_FRAME_DENOISE_YUV);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();

        switch (entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_MODE_ON:
            CLOGD("[R%d] EXYNOS_ANDROID_VENDOR_FLASH_MULTI_FRAME_DENOISE_YUV(%d)", request->getKey(), entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        case EXYNOS_ANDROID_VENDOR_MODE_NONE:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_FLASH_MULTI_FRAME_DENOISE_YUV value(%d)", entry.data.u8[0]);
            break;
        }
    }
#endif

#ifdef USES_COMBINE_PLUGIN
    entry = settings->find(EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        switch(entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV_OFF:
            CLOGV("[R%d] EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV(%d)", request->getKey(), entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        case EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV_ON:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV value(%d)", entry.data.u8[0]);
            break;
        }
    }
#endif

#ifdef USES_COMBINE_PLUGIN
    entry = settings->find(EXYNOS_ANDROID_VENDOR_SPORTS_YUV);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        switch (entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_MODE_ON:
            CLOGD("[R%d] EXYNOS_ANDROID_VENDOR_SPORTS_YUV(%d)", request->getKey(), entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        case EXYNOS_ANDROID_VENDOR_MODE_NONE:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_SPORTS_YUV value(%d)", entry.data.u8[0]);
            break;
        }

        entry = settings->find(EXYNOS_ANDROID_VENDOR_SPORTS_YUV_MOTION_LEVEL);
        if (entry.count > 0) {
            int motionLevel = 0;

            motionLevel = (int)entry.data.u8[0];

            if (motionLevel < 0) {
                CLOGW("force motionLevel from %d to 0", motionLevel);
                entry.data.u8[0] = 0;
            } else if (2 < motionLevel) {
                CLOGW("force motionLevel from %d to 2", motionLevel);
                entry.data.u8[0] = 2;
            }

            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
        } else {
            CLOGW("cannot find EXYNOS_ANDROID_VENDOR_SPORTS_YUV_MOTION_LEVEL. just use 0");
        }
    }
#endif

#ifdef USES_COMBINE_PLUGIN
    entry = settings->find(EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        switch(entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE_OFF:
            CLOGV("[R%d] EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE(%d)", request->getKey(), entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        case EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE_ON:
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_COMBINE_SINGLE_CAPTURE value(%d)", entry.data.u8[0]);
            break;
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_3DHDR
    entry = settings->find(EXYNOS_ANDROID_VENDOR_CONTROL_3DHDR);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        uint8_t hdr_mode = entry.data.u8[0];

        m_configurations->setModeValue(CONFIGURATION_3DHDR_MODE, hdr_mode);
        m_configurations->checkExtSensorMode();
        dst->uctl.isModeUd.wdr_mode = (enum camera2_wdr_mode) FIMC_IS_METADATA(hdr_mode);

        prev_entry = m_prevMeta->find(EXYNOS_ANDROID_VENDOR_CONTROL_3DHDR);
        if (prev_entry.count > 0) {
            prev_valueU8 = prev_entry.data.i32[0];
            isMetaExist = true;
        }

        if (m_configurations->getRestartStream() == false) {
            if (isMetaExist && prev_valueU8 != hdr_mode) {
                if ((prev_valueU8 == 0) || (hdr_mode == 0)) {
                    m_configurations->setRestartStream(true);
                    CLOGD("setRestartStream(HDR_MODE)");
                }
            }
        }

        if (!isMetaExist || prev_valueU8 != hdr_mode) {
            CLOGD("EXYNOS_CONTROL_HDR_MODE(%d)", hdr_mode);
        }
        isMetaExist = false;
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_OIS_FW_VER
    entry = settings->find(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_FW_VER);
    if (entry.count > 0) {
        m_configurations->setOisTestReqired(CONFIGURATION_OIS_TEST_FW_VER, true);
        CLOGI("[MotFactory] get vendor tag of ois fw_rev");
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_OIS_GEA
    entry = settings->find(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_GEA);
    if (entry.count > 0) {
        m_configurations->setOisTestReqired(CONFIGURATION_OIS_TEST_GEA, true);
        CLOGI("[MotFactory] get vendor tag of ois gea");
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_OIS_HEA
    entry = settings->find(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_HEA);
    if (entry.count > 0) {
        m_configurations->setOisTestReqired(CONFIGURATION_OIS_TEST_HEA, true);
        CLOGI("[MotFactory] get vendor tag of ois hea");
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_LED_CALIBRATION
    entry = settings->find(EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_STROBE_CURRENTS);
    if (entry.count > 0) {
        m_configurations->setLedCurrent(0, entry.data.i32[0]);
        m_configurations->setLedCurrent(1, entry.data.i32[1]);

        CLOGI("[MotFactory] EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_STROBE_CURRENTS(%d, %d)",
            entry.data.i32[0],
            entry.data.i32[1]);

        prev_entry = m_prevMeta->find(EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_STROBE_CURRENTS);
        if (prev_entry.count > 0) {
            if (prev_entry.data.i32[0] != entry.data.i32[0] ||
                prev_entry.data.i32[1] != entry.data.i32[1]) {
                    CLOGI("[MotFactory] EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_STROBE_CURRENTS(%d->%d, %d->%d)",
                        prev_entry.data.i32[0], entry.data.i32[0],
                        prev_entry.data.i32[1], entry.data.i32[1]);
            }
        }
    }

    entry = settings->find(EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_ENABLED);
    if (entry.count > 0) {
        bool enable = (0 < entry.data.u8[0]) ? true : false;

        // "set false" means, it does not need to turn off led calibration.
        // so, just check only "set true".
        if (enable == true) {
            m_configurations->setLedCalibrationEnable(enable);
        }

        CLOGI("[MotFactory] EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_ENABLED(%d)",
            entry.data.u8[0]);

        prev_entry = m_prevMeta->find(EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_ENABLED);
        if (prev_entry.count > 0) {
            if (prev_entry.data.u8[0] != entry.data.u8[0]) {
                    CLOGI("[MotFactory] EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_ENABLED(%d->%d)",
                        prev_entry.data.u8[0],
                        entry.data.u8[0]);
            }
        }
    }
#endif

#ifdef USE_HW_BEAUTY_CONTROL
    entry = settings->find(EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_STRENGTH);
    if (entry.count > 0) {
        int32_t beautyFaceStrength = entry.data.i32[0];
        switch (beautyFaceStrength) {
            case BEAUTY_FACE_STRENGTH_MIN ... BEAUTY_FACE_STRENGTH_MAX:
                dst->ctl.facebeauty.strength = beautyFaceStrength;
                break;
            default:
                CLOGW("Invalid BEAUTY_FACE_STRENGTH value(%d)", beautyFaceStrength);
                break;

        }
    }
#endif

#ifdef USES_SUPER_RESOLUTION
    entry = settings->find(EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_RESOLUTION);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        switch (entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_SUPER_RESOLUTION_OFF:
            CLOGV("EXYNOS_ANDROID_VENDOR_SUPER_RESOLUTION_OFF(%d)", entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        case EXYNOS_ANDROID_VENDOR_SUPER_RESOLUTION_ON:
            CLOGV("EXYNOS_ANDROID_VENDOR_SUPER_RESOLUTION_ON(%d)", entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_RESOLUTION(%d", entry.data.u8[0]);
            break;
        }
    }
#endif

#ifdef USE_CLAHE_REPROCESSING
    entry = settings->find(EXYNOS_ANDROID_VENDOR_CONTROL_CLAHE_CAPTURE);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        switch (entry.data.u8[0]) {
        case EXYNOS_ANDROID_VENDOR_CLAHE_CAPTURE_OFF:
            CLOGV("EXYNOS_ANDROID_VENDOR_CLAHE_CAPTURE_OFF(%d)", entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        case EXYNOS_ANDROID_VENDOR_CLAHE_CAPTURE_ON:
            CLOGV("EXYNOS_ANDROID_VENDOR_CLAHE_CAPTURE_ON(%d)", entry.data.u8[0]);
            vendorMeta->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_CONTROL_CLAHE_CAPTURE(%d", entry.data.u8[0]);
            break;
        }
    }
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorAeControlControlData(struct camera2_shot_ext *dst_ext,
                                                                    __unused uint32_t vendorAeMode,
                                                                    __unused aa_aemode *aeMode,
                                                                    __unused ExynosCameraActivityFlash::FLASH_REQ *flashReq,
                                                                    __unused struct CameraMetaParameters *metaParameters)
{
    struct camera2_shot *dst = NULL;

    dst = &dst_ext->shot;
}

void ExynosCameraMetadataConverter::translateVendorAfControlControlData(struct camera2_shot_ext *dst_ext,
							                                         uint32_t vendorAfMode)
{
    struct camera2_shot *dst = NULL;

    dst = &dst_ext->shot;

    switch(vendorAfMode) {
    default:
        break;
    }
}

void ExynosCameraMetadataConverter::translateVendorLensControlData(__unused CameraMetadata *settings,
                                                                    struct camera2_shot_ext *dst_ext,
                                                                    __unused struct CameraMetaParameters *metaParameters,
                                                                    CameraMetadata *prevMeta)
{
    __unused camera_metadata_entry_t entry;
    __unused camera_metadata_entry_t prev_entry;
    __unused int32_t prev_value;
    __unused bool isMetaExist = false;
    struct camera2_shot *dst = NULL;

    dst = &dst_ext->shot;

    return;
}

void ExynosCameraMetadataConverter::translateVendorSensorControlData(__unused ExynosCameraRequestSP_sprt_t request,
                                                                    __unused CameraMetadata *settings,
                                                                    struct camera2_shot_ext *dst_ext,
                                                                    CameraMetadata *prevMeta)
{
    __unused camera_metadata_entry_t entry;
    __unused camera_metadata_entry_t entryMode;
    __unused camera_metadata_entry_t prev_entry;
    struct camera2_shot *dst = NULL;
    __unused int32_t prev_value;
    __unused bool isMetaExist = false;

    dst = &dst_ext->shot;

#ifdef SUPPORT_VENDOR_TAG_CONTROL_EXP_PRI
    entry = settings->find(EXYNOS_ANDROID_VENDOR_CONTROL_EXP_PRI);
    if (entry.count > 0) {
        if (entry.data.i64[0] != 0L
            && entry.data.i64[0] <= m_sensorStaticInfo->exposureTimeRange[MAX]) {
            setMetaCtlExposureTime(dst_ext, (int64_t)(entry.data.i64[0]));
        } else if (entry.data.i64[0] != 0L){
            setMetaCtlExposureTime(dst_ext, m_sensorStaticInfo->exposureTimeRange[MAX]);
        }

        prev_entry = m_prevMeta->find(EXYNOS_ANDROID_VENDOR_CONTROL_EXP_PRI);
        if (prev_entry.count > 0) {
            if (prev_entry.data.i64[0] != entry.data.i64[0]) {
                    CLOGD("EXYNOS_ANDROID_VENDOR_CONTROL_EXP_PRI(%lld->%lld)",
                        (long long)prev_entry.data.i64[0],
                        (long long)entry.data.i64[0]);
            }
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_EXP_PRI
    entryMode = settings->find(EXYNOS_ANDROID_VENDOR_CONTROL_LONG_EXPOSURE_CAPTURE);
    if (entryMode.count > 0) {
        switch (entryMode.data.u8[0]) {
        case EXYNOS_CONTROL_LONG_EXPOSURE_CAPTURE_OFF:
            break;
        case EXYNOS_CONTROL_LONG_EXPOSURE_CAPTURE_ON:
        {
            entry = settings->find(EXYNOS_ANDROID_VENDOR_CONTROL_EXP_PRI);
            if (entry.count > 0) {
                sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
                vendorMeta->update(entry.tag, entry.data.i64, entry.count);

                if (entry.data.i64[0] > m_sensorStaticInfo->exposureTimeRange[MAX]) {
                    /*
                     * It is intended to reduce the delay for LONG_EXPOSURE capture.
                     * It try to force flush the pending requests in the pipeline.
                     * worst case: restart_stream costs about 300 ms.
                     *  Average pending requests in the pipeline would be around 7.
                     */
                    if (dst->ctl.sensor.exposureTime * 7 > 300000000L) {
                        if (m_configurations->getRestartStream() == false) {
                            m_configurations->setRestartStream(true);
                        }

                        m_configurations->setMode(CONFIGURATION_RESTART_FORCE_FLUSH, true);
                        CLOGD("LONG_EXPOSURE: setRestartStream(LONG_EXPOSURE_CAPTURE_ON), exposureTime (%lld)", dst->ctl.sensor.exposureTime);
                    }

                    dst->ctl.aa.vendor_captureExposureTime = (entry.data.i64[0] / 1000);
                    dst->ctl.aa.captureIntent = AA_CAPTURE_INTENT_STILL_CAPTURE_EXPOSURE_DYNAMIC_SHOT;
                    m_configurations->setCaptureExposureTime(dst->ctl.aa.vendor_captureExposureTime);
                    int32_t count = m_configurations->getLongExposureShotCount();
                    dst->ctl.aa.vendor_captureCount = count;

                    vendorMeta->update(EXYNOS_ANDROID_VENDOR_CONTROL_LONG_EXPOSURE_COUNT, &count, 1);
                    CLOGD("LONG_EXPOSURE mode(%d) exposure(%lld) vendorExposure(%u) count(%d)", entryMode.data.u8[0], (long long)entry.data.i64[0], dst->ctl.aa.vendor_captureExposureTime, count);
                } else {
                    CLOGD("LONG_EXPOSURE mode(%d) exposure(%lld) vendorExposure(%u)", entryMode.data.u8[0], (long long)entry.data.i64[0], dst->ctl.aa.vendor_captureExposureTime);
                }
            }
            break;
        }
        default:
            CLOGW("Invalid EXYNOS_ANDROID_VENDOR_CONTROL_LONG_EXPOSURE_CAPTURE mode(%d)", entryMode.data.u8[0]);
            break;
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_ISO_PRI
    entry = settings->find(EXYNOS_ANDROID_VENDOR_CONTROL_ISO_PRI);
    if (entry.count > 0) {
        if (entry.data.i64[0] == 0L) {
            setMetaCtlIso(dst_ext, AA_ISOMODE_AUTO, 0);
        } else {
            setMetaCtlIso(dst_ext, AA_ISOMODE_MANUAL, (uint32_t)(entry.data.i64[0]));
        }

        prev_entry = m_prevMeta->find(EXYNOS_ANDROID_VENDOR_CONTROL_ISO_PRI);
        if (prev_entry.count > 0) {
            if (prev_entry.data.i64[0] != entry.data.i64[0]) {
                    CLOGD("EXYNOS_ANDROID_VENDOR_CONTROL_ISO_PRI(%lld->%lld)",
                        (long long)prev_entry.data.i64[0],
                        (long long)entry.data.i64[0]);
            }
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_LONG_EXPOSURE_CAPTURE
    if (m_configurations->getMode(CONFIGURATION_SESSION_MODE)) {
        if (m_configurations->getModeMultiValue(
                    CONFIGURATION_MULTI_SESSION_MODE_VALUE, EXYNOS_SESSION_MODE_PRO)) {
            enum aa_scene_mode sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
            sceneMode = AA_SCENE_MODE_PRO_MODE;
            setMetaCtlSceneMode(dst_ext, sceneMode);
        }
    }

    entry = settings->find(EXYNOS_ANDROID_VENDOR_CONTROL_LONG_EXPOSURE_CAPTURE);
    if (entry.count > 0) {
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();
        vendorMeta->update(entry.tag, entry.data.u8, entry.count);
    }
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorLedControlData(__unused CameraMetadata *settings,
                                                                  struct camera2_shot_ext *dst_ext,
                                                                  CameraMetadata *prevMeta)
{
    __unused camera_metadata_entry_t entry;
    __unused camera_metadata_entry_t prev_entry;
    __unused int32_t prev_value;
    __unused bool isMetaExist = false;

    struct camera2_shot *dst = NULL;
    dst = &dst_ext->shot;

    return;
}

void ExynosCameraMetadataConverter::translateVendorControlMetaData(__unused CameraMetadata *settings,
                                                                   __unused struct camera2_shot_ext *src_ext,
                                                                   __unused ExynosCameraRequestSP_sprt_t request)
{
    struct camera2_shot *src = NULL;
    __unused camera_metadata_entry_t entry;
    sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();

    src = &src_ext->shot;

#ifdef SUPPORT_VENDOR_TAG_SCENE_DETECTION
    /* EXYNOS_ANDROID_CONTROL_SCENE_DETECTION_INFO */
    int64_t sceneDetectionInfo[7] = {0, };

    sceneDetectionInfo[0] = (int64_t)src->uctl.sceneDetectInfoUd.timeStamp;
    sceneDetectionInfo[1] = (int64_t)src->udm.scene_index;  /* UDM */
    sceneDetectionInfo[2] = (int64_t)src->uctl.sceneDetectInfoUd.confidence_score;
    sceneDetectionInfo[3] = (int64_t)src->uctl.sceneDetectInfoUd.object_roi[0];
    sceneDetectionInfo[4] = (int64_t)src->uctl.sceneDetectInfoUd.object_roi[1];
    sceneDetectionInfo[5] = (int64_t)src->uctl.sceneDetectInfoUd.object_roi[2];
    sceneDetectionInfo[6] = (int64_t)src->uctl.sceneDetectInfoUd.object_roi[3];

    settings->update(EXYNOS_ANDROID_CONTROL_SCENE_DETECTION_INFO,
                    sceneDetectionInfo,
                    sizeof(sceneDetectionInfo)/sizeof(int64_t));
    CLOGV("sceneDetectionInfo : scene index(%lld)", (long long)sceneDetectionInfo[1]);
#endif
#ifdef USES_COMBINE_PLUGIN
    entry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_SCENE_TYPE);
    if (entry.count > 0) {
        int32_t sceneType = (int32_t)entry.data.i32[0];
        settings->update(EXYNOS_ANDROID_VENDOR_SCENE_TYPE, &sceneType, entry.count);
        CLOGV("[scene] meta data converter: %d, %d, %d", entry.tag, sceneType, entry.count);
    }
#endif
#ifdef SUPPORT_VENDOR_TAG_MF_STILL
    entry = vendorMeta->find(SLSI_MF_STILL_RESULT);
    if (entry.count > 0) {
        switch (entry.data.u8[0]) {
        case SLSI_MF_STILL_FUNCTIONS_NONE ... SLSI_MF_STILL_FUNCTIONS_SR_FAILED:
            CLOGD("[R%d] MFStillCaptureResult(%d)", request->getKey(), entry.data.u8[0]);
            settings->update(entry.tag, entry.data.u8, entry.count);
            break;
        default:
            CLOGW("Invalid SLSI_MF_STILL_RESULT value(%d)", entry.data.u8[0]);
            break;
        }
    }
#endif

#ifdef USE_HW_BEAUTY_CONTROL
    entry = settings->find(EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_STRENGTH);
    if (entry.count > 0) {
        int32_t beautyFaceStrength = src->dm.facebeauty.strength;

        if (beautyFaceStrength != entry.data.i32[0]) {
            switch (beautyFaceStrength) {
                case BEAUTY_FACE_STRENGTH_MIN ... BEAUTY_FACE_STRENGTH_MAX:
                    settings->update(EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_STRENGTH,
                                &beautyFaceStrength, 1);
                    break;
                default:
                    CLOGW("Invalid BEAUTY_FACE_STRENGTH value(%d)", beautyFaceStrength);
                    break;
            }
        }
    }
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorJpegMetaData(ExynosCameraRequestSP_sprt_t requestInfo,
                                                                __unused CameraMetadata *settings)
{
#ifdef SUPPORT_VENDOR_TAG_JPEG_ENCODE_CROP_ENABLE
    uint8_t jpegEncodeCropEnable = 0; // TODO : need to update

    settings->update(EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_ENABLE, &jpegEncodeCropEnable, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_JPEG_ENCODE_CROP_RECT
    int32_t jpegEncodeCropRect[4]; // TODO : need to update

    settings->update(EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_RECT, jpegEncodeCropRect, 4);
#endif

#ifdef SUPPORT_VENDOR_TAG_JPEG_ENCODE_CROP_ROI
    int32_t jpegEncodeCropRoi[4]; // TODO : need to update

    settings->update(EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_ROI, jpegEncodeCropRoi, 4);
#endif

#ifdef SUPPORT_VENDOR_TAG_SNAPSHOT_APPX
    debug_attribute_t *debugInfo = m_parameters[m_cameraId]->getDebugAttribute();
    uint8_t *appX = (uint8_t *)debugInfo->debugData[APP_MARKER_4];
    settings->update(EXYNOS_ANDROID_VENDOR_SNAPSHOT_APPX, appX, debugInfo->debugSize[APP_MARKER_4]);
#endif

#ifdef SUPPORT_VENDOR_TAG_SNAPSHOT_EXIF
    exif_attribute_t exifInfo;
    m_parameters[m_cameraId]->getFixedExifInfo(&exifInfo);
    uint8_t *exif = (uint8_t *)(&exifInfo);
    settings->update(EXYNOS_ANDROID_VENDOR_SNAPSHOT_EXIF, exif, sizeof(exif_attribute_t));
#endif

#ifdef SUPPORT_VENDOR_TAG_SNAPSHOT_MAKERNOTE
    uint8_t *makerNote = (uint8_t *)exifInfo.maker_note;
    settings->update(EXYNOS_ANDROID_VENDOR_SNAPSHOT_MAKER_NOTE, makerNote, exifInfo.maker_note_size);
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorLensMetaData(ExynosCameraRequestSP_sprt_t requestInfo,
                                                                __unused CameraMetadata *settings,
                                                                struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;

    return;
}

void ExynosCameraMetadataConverter::translateVendorScalerControlData(__unused CameraMetadata *settings,
                                                                 __unused struct camera2_shot_ext *src_ext,
                                                                 CameraMetadata *prevMeta,
                                                                 struct CameraMetaParameters *metaParameters)
{
    __unused camera_metadata_entry_t entry;
    __unused camera_metadata_entry_t prev_entry;
    __unused bool isMetaExist = false;

    return;
}

void ExynosCameraMetadataConverter::translateVendorSensorMetaData(ExynosCameraRequestSP_sprt_t request,
                                                                    CameraMetadata *settings,
                                                                    struct camera2_shot_ext *src_ext,
                                                                    int32_t physCamID)
{
    struct camera2_shot *src = NULL;
    camera_metadata_entry_t entry;

    src = &src_ext->shot;

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_LUX_STD
    float luxStd = 0.0F;
    luxStd = getVendorMetaLuxStd(src_ext);

    settings->update(EXYNOS_ANDROID_VENDOR_ENVINFO_LUX_STD, &luxStd, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_LUX_IDX
    float luxIdx = 0.0F;
    luxIdx = getVendorMetaLuxIdx(src_ext);

    if (luxIdx == 0.0F) {
        float oldLuxIdx = getVendorMetaLuxIdx(&m_oldShotExt);
        CLOGV("[B%d]Invalid VENDOR_TAG value : luxIdx(%f -> %f)", getMetaDmRequestFrameCount(src_ext), luxIdx, oldLuxIdx);
        luxStd = oldLuxIdx;
    } else {
        setVendorMetaLuxIdx(&m_oldShotExt, luxIdx);
    }

    settings->update(EXYNOS_ANDROID_VENDOR_ENVINFO_LUX_IDX, &luxIdx, 1);
#endif

    /* ANALOG_GAIN */
#ifdef SUPPORT_VENDOR_TAG_ENVINFO_ANALOG_GAIN
    float analogGain = 0.0F;
    analogGain = getVendorMetaAnalogGain(src_ext);

    if (analogGain == 0.0F) {
        float oldAnalogGain = getVendorMetaAnalogGain(&m_oldShotExt);
        CLOGV("[B%d]Invalid VENDOR_TAG value : analogGain(%f -> %f)", getMetaDmRequestFrameCount(src_ext), analogGain, oldAnalogGain);
        analogGain = oldAnalogGain;
    } else {
        setVendorMetaAnalogGain(&m_oldShotExt, analogGain);
    }

    settings->update(EXYNOS_ANDROID_VENDOR_ENVINFO_ANALOG_GAIN, &analogGain, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_THERMAL_LEVEL
    enum camera_thermal_mode thermalLevel = CAM_THERMAL_NORMAL;
    thermalLevel = getVendorMetaThermalLevel(src_ext);

    switch (thermalLevel) {
    case CAM_THERMAL_NORMAL:
        CLOGV("EXYNOS_ANDROID_VENDOR_ENVINFO_THERMAL_LEVEL is CAM_THERMAL_NORMAL. OK");
        break;
    case CAM_THERMAL_THROTTLING:
        CLOGW("EXYNOS_ANDROID_VENDOR_ENVINFO_THERMAL_LEVEL is CAM_THERMAL_THROTTLING. HOT");
        break;
    case CAM_THERMAL_TRIPPING:
        CLOGW("EXYNOS_ANDROID_VENDOR_ENVINFO_THERMAL_LEVEL is CAM_THERMAL_TRIPPING. VERY HOT. camera will be turn off");
        break;
    default:
        CLOGE("Invalid thermalLevel(%d). weird...", thermalLevel);
        break;
    }

    uint8_t thermal_level = (uint8_t)thermalLevel;
    settings->update(EXYNOS_ANDROID_VENDOR_ENVINFO_THERMAL_LEVEL, &thermal_level, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_ISO100_GAIN
    float iso100Gain = 2.0F;
    settings->update(EXYNOS_ANDROID_VENDOR_ENVINFO_ISO100_GAIN, &iso100Gain, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_AWB_CCT
    int32_t awbCct = 0;
    awbCct = getVendorMetaAwbCct(src_ext);

    if (awbCct == 0) {
        int32_t oldAwbCct = getVendorMetaAwbCct(&m_oldShotExt);
        CLOGV("[B%d]Invalid VENDOR_TAG value : awbCct(%d -> %d)", getMetaDmRequestFrameCount(src_ext), awbCct, oldAwbCct);
        awbCct = oldAwbCct;
    } else {
        setVendorMetaAwbCct(&m_oldShotExt, awbCct);
    }

    settings->update(EXYNOS_ANDROID_VENDOR_ENVINFO_AWB_CCT, &awbCct, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_AWB_DEC
    int32_t awbDec = 0;
    awbDec = getVendorMetaAwbDec(src_ext);

    if(awbDec == 0) {
        int32_t oldAwbDec = getVendorMetaAwbDec(&m_oldShotExt);
        CLOGV("[B%d]Invalid VENDOR_TAG value : awbDec(%d -> %d)", getMetaDmRequestFrameCount(src_ext), awbDec, oldAwbDec);
        awbDec = oldAwbDec;
    } else {
        setVendorMetaAwbDec(&m_oldShotExt, awbDec);
    }

    settings->update(EXYNOS_ANDROID_VENDOR_ENVINFO_AWB_DEC, &awbDec, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_LENS_POS
    int32_t lensPos = 0;
    lensPos = getVendorMetaLensPos(src_ext);

    settings->update(EXYNOS_ANDROID_VENDOR_ENVINFO_LENS_POS, &lensPos, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_LINECOUNT
    int32_t lineCount = 0;//TBD
    settings->update(EXYNOS_ANDROID_VENDOR_ENVINFO_LINECOUNT, &lineCount, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_AFD_SUBMODE
    uint8_t afdSubmode = 0;
    afdSubmode = (uint8_t)getVendorMetaAfdSubmode(src_ext);

    settings->update(EXYNOS_ANDROID_VENDOR_ENVINFO_AFD_SUBMODE, &afdSubmode, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_FLICKER_DETECT
    uint8_t flickerDetect = 0;
    flickerDetect = (uint8_t)getVendorMetaFlickerDetect(src_ext);

    // 0: no flicker, 1 : flicker
    settings->update(EXYNOS_ANDROID_VENDOR_ENVINFO_FLICKER_DETECT, &flickerDetect, 1);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_EV
    {
        float ev = 0.0F;
        ev = (float)getVendorMetaEv(src_ext);
        CLOGV("ev(%f)", ev);
        settings->update(EXYNOS_ANDROID_VENDOR_CONTROL_EV, &ev, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_CAMERA_ID
    {
        int32_t logicaCameraId = 0;

        HAL_CameraInfo_t *halCameraInfo = ExynosCameraMetadataConverter::getExynosCameraDeviceInfoByCamIndex(m_cameraId);
        logicaCameraId = halCameraInfo->facing_info;
        CLOGV("logicaCameraId(%d)", logicaCameraId);

        settings->update(EXYNOS_ANDROID_VENDOR_CONTROL_CAMERA_ID, &logicaCameraId, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_ISO_TOTAL
    {
        int32_t isoTotal = 0;
        isoTotal = (int32_t)getVendorMetaVendorIsoValue(src_ext);
        CLOGV("isoTotal(%d)", isoTotal);
        settings->update(EXYNOS_ANDROID_VENDOR_CONTROL_ISO_TOTAL, &isoTotal, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_HDR_GAIN
    {
        float hdrGain = 0.0F;
        float analogGain = getVendorMetaAnalogGain(src_ext);
        float digitalGain = getVendorMetaDigitalGain(src_ext);
        hdrGain = analogGain * digitalGain;
        CLOGV("analogGain(%f) * digitalGain(%f) = hdrGain(%f)", analogGain, digitalGain, hdrGain);
        settings->update(EXYNOS_ANDROID_VENDOR_CONTROL_HDR_GAIN, &hdrGain, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_HDR_SHUTTER
    {
        float hdrShutter = 0.0F;

        settings->update(EXYNOS_ANDROID_VENDOR_CONTROL_HDR_SHUTTER, &hdrShutter, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_SENSOR_GAIN
    {
        float sensorGain = 0.0F;
        float analogGain = getVendorMetaAnalogGain(src_ext);
        float digitalGain = getVendorMetaDigitalGain(src_ext);
        sensorGain = analogGain * digitalGain;
        CLOGV("analogGain(%f) * digitalGain(%f) = sensorGain(%f)", analogGain, digitalGain, sensorGain);
        settings->update(EXYNOS_ANDROID_VENDOR_CONTROL_SENSOR_GAIN, &sensorGain, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_CHI_OVERRIDE_EXPINDEX
    {
        float expIndex = 0.0F;

        settings->update(EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE_EXPINDEX, &expIndex, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_FOCUS_POS
    if(settings->exists(EXYNOS_ANDROID_VENDOR_FACTORY_FOCUS_POS)) {
        camera_metadata_entry entry = settings->find(EXYNOS_ANDROID_VENDOR_FACTORY_FOCUS_POS);
        if (entry.count == 1 && entry.data.i32[0]) {

            int32_t focusPos[4];

            focusPos[0] = getVendorMetaFocusTargetPositionSupported(src_ext);
            focusPos[1] = getVendorMetaFocusTargetPosition(src_ext);
            focusPos[2] = getVendorMetaFocusActualPositionSupported(src_ext);
            focusPos[3] = getVendorMetaFocusActualPosition(src_ext);

            if (focusPos[1] == 0) {
                int32_t oldFocusPos[4];
                oldFocusPos[0] = getVendorMetaFocusTargetPositionSupported(&m_oldShotExt);
                oldFocusPos[1] = getVendorMetaFocusTargetPosition         (&m_oldShotExt);

                CLOGV("[B%d]Invalid VENDOR_TAG value : focusPos[0](%d -> %d), focusPos[1](%d -> %d)",
                    getMetaDmRequestFrameCount(src_ext),
                    focusPos[0], oldFocusPos[0],
                    focusPos[1], oldFocusPos[1]);

                focusPos[0] = oldFocusPos[0];
                focusPos[1] = oldFocusPos[1];
            } else {
                setVendorMetaFocusTargetPositionSupported(&m_oldShotExt, focusPos[0]);
                setVendorMetaFocusTargetPosition         (&m_oldShotExt, focusPos[1]);
            }

            if (focusPos[3] == 0) {
                int32_t oldFocusPos[4];
                oldFocusPos[2] = getVendorMetaFocusActualPositionSupported(&m_oldShotExt);
                oldFocusPos[3] = getVendorMetaFocusActualPosition         (&m_oldShotExt);

                CLOGV("[B%d]Invalid VENDOR_TAG value : focusPos[2](%d -> %d), focusPos[3](%d -> %d)",
                    getMetaDmRequestFrameCount(src_ext),
                    focusPos[2], oldFocusPos[2],
                    focusPos[3], oldFocusPos[3]);

                focusPos[2] = oldFocusPos[2];
                focusPos[3] = oldFocusPos[3];
            } else {
                setVendorMetaFocusActualPositionSupported(&m_oldShotExt, focusPos[2]);
                setVendorMetaFocusActualPosition         (&m_oldShotExt, focusPos[3]);
            }
            CLOGI("[MotFactory] focusPos: %d, %d, %d, %d",
                    focusPos[0], focusPos[1], focusPos[2], focusPos[3]);

            settings->update(EXYNOS_ANDROID_VENDOR_FACTORY_FOCUS_POS, focusPos, 4);
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_CALIBRATION_STATUS
    if(settings->exists(EXYNOS_ANDROID_VENDOR_FACTORY_CALIBRATION_STATUS)) {
        camera_metadata_entry entry = settings->find(EXYNOS_ANDROID_VENDOR_FACTORY_CALIBRATION_STATUS);
        if (entry.count == 1 && entry.data.u8[0]) {
            /*
             * if (mnf == 1 || af == 1 || awb == 1 || pdaf==1 || dual==1)
             * print "pass"
             * else if (mnf == 0 && af == 0 && awb == 0 && pdaf == 0 && dual == 1)
             * print "fail mnf:1, af:0, awb:0, pdaf:0, dual:0"
             */
            uint8_t calibrationStatus[10];
            int     calibrationStatusCount = getVendorMetaCalibStatusCount(src_ext);

            bool    flagPass = true;
            String8 strBuf;

            for (int i = 0; i < calibrationStatusCount; i++) {
                calibrationStatus[i] = getVendorMetaCalibStatus(src_ext, i);

                if (calibrationStatus[i] != 0) {
                    flagPass = false;
                }
            }

            if (flagPass == true) {
                strBuf.append("pass");
            } else {
                strBuf.appendFormat("fail mnf:%d, af:%d, awb:%d, pdaf:%d, dual:%d",
                    getVendorMetaCalibStatusMnf(src_ext),
                    getVendorMetaCalibStatusAf(src_ext),
                    getVendorMetaCalibStatusAwb(src_ext),
                    getVendorMetaCalibStatusPdaf(src_ext),
                    getVendorMetaCalibStatusDual(src_ext));
            }
            CLOGI("[MotFactory] calibrationStatus=%s", strBuf.c_str());

            settings->update(EXYNOS_ANDROID_VENDOR_FACTORY_CALIBRATION_STATUS, strBuf);
        }
    }
#endif

    /* MODULE_ID */
#ifdef SUPPORT_VENDOR_TAG_FACTORY_MODULE_ID
    if(settings->exists(EXYNOS_ANDROID_VENDOR_FACTORY_MODULE_ID)) {
        camera_metadata_entry entry = settings->find(EXYNOS_ANDROID_VENDOR_FACTORY_MODULE_ID);
        if (entry.count == 1 && entry.data.u8[0]) {
            uint8_t moduleId[MAX_CALIBRATION_STRING];
            memset(moduleId, 0, MAX_CALIBRATION_STRING);
            uint8_t *serialNumber;
            int      serialNumberSize;

#if defined(SENSOR_FW_GET_FROM_FILE)
            ExynosCameraEEPRomMap *eepromMap = m_parameters[m_cameraId]->getEEPRomMap();
            if (eepromMap != NULL) {
                if (eepromMap->flagCreated() == true) {
                    serialNumber = (uint8_t *)eepromMap->getSerialNumber(&serialNumberSize);
                    memcpy(moduleId, serialNumber, serialNumberSize);
                } else {
                    CLOGE("eepromMap->flagCreated() == false. so, fail. you cannot get the exact eeprom value");
                }
            } else
                CLOGE("eepromMap == NULL. so, fail. you cannot get serial number");
#endif
            CLOGI("[MotFactory] moduleId=%s", moduleId);

            settings->update(EXYNOS_ANDROID_VENDOR_FACTORY_MODULE_ID, moduleId,
                strnlen((char *) moduleId, MAX_CALIBRATION_STRING));
        }
    }
#endif

    /* OIS_GEA */
#ifdef SUPPORT_VENDOR_TAG_FACTORY_OIS_GEA
    if (settings->exists(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_GEA)) {
        ////////////////////////////////////////////////
        // check vendor meta tag
        sp<ExynosCameraVendorMetaData> vendorMeta = request->getVendorMeta();

        if (vendorMeta) {
            camera_metadata_entry vendorEntry = vendorMeta->find(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_GEA);

            ////////////////////////////////////////////////
            // update to app
            if (0 < vendorEntry.count) {
                uint8_t oisGea = vendorEntry.data.u8[0];

                // 0x01 : OK
                // 0xFF : NG
                // 0xFE : Gyro communation error;
                CLOGI("[MotFactory] ois: gea result: 0x%X", oisGea);
                settings->update(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_GEA, &oisGea, 1);
            }
        }

        ////////////////////////////////////////////////
    }
#endif

    /* OIS_HEA */
#ifdef SUPPORT_VENDOR_TAG_FACTORY_OIS_HEA
    if(settings->exists(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_HEA)) {
        camera_metadata_entry entry = settings->find(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_HEA);
        if (entry.count == 1) {
            int data[8];

#if defined(SENSOR_FW_GET_FROM_FILE)
            ExynosCameraEEPRomMap *eepromMap = m_parameters[m_cameraId]->getEEPRomMap();
            if (eepromMap != NULL) {
                if (eepromMap->flagCreated() == true) {
                    int oisHeaTargetSize = 0;
                    int8_t* heaTarget = (int8_t *)eepromMap->getOisHeaTarget(&oisHeaTargetSize);
                    data[0] = heaTarget[0];
                    data[1] = heaTarget[1];
                    data[2] = heaTarget[2];
                    data[3] = heaTarget[3];
                } else {
                    CLOGE("[MotFactory] ois hea eepromMap->flagCreated() == false.");
                }
            } else {
                CLOGE("[MotFactory] ois hea eepromMap == NULL. so, fail.");
            }
#endif
            int oisHeaActual[4];
            m_configurations->getOisTestHea(oisHeaActual);
            data[4] = oisHeaActual[0];
            data[5] = oisHeaActual[1];
            data[6] = oisHeaActual[2];
            data[7] = oisHeaActual[3];
            CLOGI("[MotFactory] ois hea target: (%d %d %d %d), actual: (%d %d %d %d)",
                    data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

            settings->update(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_HEA, data, ARRAY_LENGTH(data));
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_OIS_FW_VER
    if(settings->exists(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_FW_VER)) {
        camera_metadata_entry entry = settings->find(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_FW_VER);
        if (entry.count == 1) {
            uint8_t data[4];
            uint32_t mOisFWRev = m_configurations->getOisTestFwVer();
            data[3] = (mOisFWRev >> 24) & 0xFF;
            data[2] = (mOisFWRev >> 16) & 0xFF;
            data[1] = (mOisFWRev >> 8) & 0xFF;
            data[0] =  mOisFWRev & 0xFF;
            CLOGI("[MotFactory] ois fw version: (%d %d %d %d)",
                    data[0], data[1], data[2], data[3]);
            settings->update(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_FW_VER, data, ARRAY_LENGTH(data));
        }
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_LED_CALIBRATION
    if (settings->exists(EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_DATA)) {

        ////////////////////////////////////////////////
        // read value
        status_t ret = NO_ERROR;

        struct ExynosCameraLEDCalibrationMap ledCalibrationMap;
        memset(&ledCalibrationMap, 0, sizeof(struct ExynosCameraLEDCalibrationMap));

        char *fileName = (char *)LED_CALIBRATION_FILE_PATH;
        char *bufAddr = (char *)&ledCalibrationMap;
        int   bufSize = sizeof(struct ExynosCameraLEDCalibrationMap);

        ret = readFromFile(fileName, (char*)bufAddr, bufSize);
        if (ret != NO_ERROR) {
            CLOGE("[MotFactory] readFromFile(%s, %p, %d) fail", fileName, (char*)bufAddr, bufSize);
        }

        ////////////////////////////////////////////////
        // return value
        int dataSize = 6;
        double ledCalibrationData[6];

        ledCalibrationData[0] = (double)ledCalibrationMap.Cool_LED_Current_R_Avg;
        ledCalibrationData[1] = (double)ledCalibrationMap.Cool_LED_Current_b_Avg;

        ledCalibrationData[2] = (double)ledCalibrationMap.Warm_LED_Current_R_Avg;
        ledCalibrationData[3] = (double)ledCalibrationMap.Warm_LED_Current_b_Avg;

        ledCalibrationData[4] = (double)ledCalibrationMap.Cool_Warm_LED_Current_R_Avg;
        ledCalibrationData[5] = (double)ledCalibrationMap.Cool_Warm_LED_Current_b_Avg;

        CLOGI("[MotFactory] EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_DATA(COOL) (LED1 R(%.5f) Gr(%.5f) Gb(%.5f) b(%.5f)",
            (double)ledCalibrationMap.Cool_LED_Current_R_Avg,
            (double)ledCalibrationMap.Cool_LED_Current_Gr_Avg,
            (double)ledCalibrationMap.Cool_LED_Current_Gb_Avg,
            (double)ledCalibrationMap.Cool_LED_Current_b_Avg);

        CLOGI("[MotFactory] EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_DATA(WARM) (LED2 R(%.5f) Gr(%.5f) Gb(%.5f) b(%.5f)",
            (double)ledCalibrationMap.Warm_LED_Current_R_Avg,
            (double)ledCalibrationMap.Warm_LED_Current_Gr_Avg,
            (double)ledCalibrationMap.Warm_LED_Current_Gb_Avg,
            (double)ledCalibrationMap.Warm_LED_Current_b_Avg);

        CLOGI("[MotFactory] EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_DATA(BOTH) (LED3 R(%.5f) Gr(%.5f) Gb(%.5f) b(%.5f)",
            (double)ledCalibrationMap.Cool_Warm_LED_Current_R_Avg,
            (double)ledCalibrationMap.Cool_Warm_LED_Current_Gr_Avg,
            (double)ledCalibrationMap.Cool_Warm_LED_Current_Gb_Avg,
            (double)ledCalibrationMap.Cool_Warm_LED_Current_b_Avg);

        settings->update(EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_DATA, ledCalibrationData, dataSize);

        ////////////////////////////////////////////////
    }
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorStatisticsMetaData(__unused CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;
    camera_metadata_entry_t entry;

    src = &src_ext->shot;

#ifdef SUPPORT_VENDOR_TAG_STATS_AEC_AECLUX
    {
        float aecLux = 0.0F;
        aecLux = getVendorMetaLuxStd(src_ext);
        CLOGV("aecLux(%f)", aecLux);
        settings->update(EXYNOS_ANDROID_VENDOR_STATS_AEC_AECLUX, &aecLux, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_AWB_CCT_VALUE
    {
        int32_t awbCctValue = 0;
        awbCctValue = getVendorMetaAwbCct(src_ext);
        CLOGV("awbCctValue(%d)", awbCctValue);
        settings->update(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_AWB_CCT_VALUE, &awbCctValue, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_AEC_AEC_STATUS
    {
        int32_t aecAecStatus = 0; // 1 : AECSettled  0 : AECUnSettled
        aecAecStatus = getVendorMetaAecAecStatus(src_ext);
        CLOGV("aecAecStatus(%d)", aecAecStatus);
        settings->update(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_AEC_AEC_STATUS, &aecAecStatus, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_AF_STATUS
    {
        int32_t afStatus = 0;
        afStatus = getVendorMetaAfStatus(src_ext);
        CLOGV("afStatus(%d)", afStatus);
        settings->update(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_AF_STATUS, &afStatus, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_LENS_SHIFT_MM
    {
        float lensShiftMM = 0.0F;

        settings->update(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_LENS_SHIFT_MM, &lensShiftMM, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_OBJECT_DISTANCE_MM
    {
        float objectDistanceMM = 0.0F;
        objectDistanceMM = getVendorMetaVendorObjectDistanceCm(src_ext) * 10.0f;
        CLOGV("objectDistanceMM(%f)", objectDistanceMM);
        settings->update(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_OBJECT_DISTANCE_MM, &objectDistanceMM, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_NEAR_FIELD_MM
    {
        float nearFieldMM = 0.0F;

        settings->update(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_NEAR_FIELD_MM, &nearFieldMM, 1);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_FAR_FIELD_MM
    {
        float farFieldMM = 0.0F;

        settings->update(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_FAR_FIELD_MM, &farFieldMM, 1);
    }
#endif

#if defined(SUPPORT_VENDOR_TAG_STATS_BLINK_DEGREE)     || \
    defined(SUPPORT_VENDOR_TAG_STATS_BLINK_DETECTED)   || \
    defined(SUPPORT_VENDOR_TAG_STATS_SMILE_CONFIDENCE) || \
    defined(SUPPORT_VENDOR_TAG_STATS_SMILE_DEGREE)     || \
    defined(SUPPORT_VENDOR_TAG_STATS_GAZE_ANGLE)       || \
    defined(SUPPORT_VENDOR_TAG_STATS_GAZE_DIRECTION)

    struct facial_score *facialScore = getVendorMetaFacialScore(src_ext);
    struct facial_angle *facialAngle = getVendorMetaFacialAngle(src_ext);
    int numOfDetectedFaces = getVendorMetaNumOfDetectedFaces(src_ext);

    if (NUM_OF_DETECTED_FACES < numOfDetectedFaces) {
        CLOGV("NUM_OF_DETECTED_FACES(%d) <  numOfDetectedFaces(%d). so, just use numOfDetectedFaces", NUM_OF_DETECTED_FACES, numOfDetectedFaces);
        numOfDetectedFaces = NUM_OF_DETECTED_FACES;
    }
    if (numOfDetectedFaces <= 0){
        CLOGV("NUM_OF_DETECTED_FACES(%d),so no need to update metadata", numOfDetectedFaces);
        return;
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_BLINK_DEGREE
    uint8_t blinkDegree[NUM_OF_DETECTED_FACES * 2];

    for (int i = 0; i < numOfDetectedFaces; i++) {
        // face[0][0] : 1'st face's left  blink
        // face[0][1] : 2'nd face's right blink

        blinkDegree[(i * 2)]     = facialScore[i].left_blink;
        blinkDegree[(i * 2) + 1] = facialScore[i].right_blink;

        //CLOGV("blinkDegree[%d] : %d / %d", i, blinkDegree[i], blinkDegree[i + 1]);
    }

    settings->update(EXYNOS_ANDROID_VENDOR_STATS_BLINK_DEGREE, blinkDegree, numOfDetectedFaces * 2);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_BLINK_DETECTED
    uint8_t blinkDetected[NUM_OF_DETECTED_FACES];

    for (int i = 0; i < numOfDetectedFaces; i++) {
        // face[0] : 1'st face's blink detection

        blinkDetected[i] = ((65 < facialScore[i].left_blink) || (65 < facialScore[i].right_blink)) ? 1 : 0;

    }

    settings->update(EXYNOS_ANDROID_VENDOR_STATS_BLINK_DETECTED, blinkDetected, numOfDetectedFaces);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_SMILE_CONFIDENCE
    uint8_t smileConfidence[NUM_OF_DETECTED_FACES];

    for (int i = 0; i < numOfDetectedFaces; i++) {
        smileConfidence[i] = (0 < facialScore[i].smile) ? 100 : 0;

        //CLOGV("smileConfidence[%d] : %d", i, smileConfidence[i]);
    }

    settings->update(EXYNOS_ANDROID_VENDOR_STATS_SMILE_CONFIDENCE, smileConfidence, numOfDetectedFaces);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_SMILE_DEGREE
    uint8_t smileDegree[NUM_OF_DETECTED_FACES];

    for (int i = 0; i < numOfDetectedFaces; i++) {
        smileDegree[i] = facialScore[i].smile;

        if (smileDegree[i] < 0) {
            smileDegree[i] = 0;
        }

        if (100 < smileDegree[i]) {
            smileDegree[i] = 100;
        }

        //CLOGV("smileDegree[%d] : %d", i, smileDegree[i]);
    }

    settings->update(EXYNOS_ANDROID_VENDOR_STATS_SMILE_DEGREE, smileDegree, numOfDetectedFaces);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_GAZE_ANGLE
    uint8_t gazeAngle[NUM_OF_DETECTED_FACES];

    for (int i = 0; i < numOfDetectedFaces; i++) {
        int yaw = (int)facialAngle[i].yaw;

        if (yaw <= 180) {
            yaw = 0 - yaw;
        } else {
            yaw = 360 - yaw;
        }

        gazeAngle[i] = yaw;

        //CLOGV("facialAngle[%d].yaw : %d -> yaw : %d -> gazeAngle[%d] : %d", i, facialAngle[i].yaw, yaw, i, gazeAngle[i]);
    }

    settings->update(EXYNOS_ANDROID_VENDOR_STATS_GAZE_ANGLE, gazeAngle, numOfDetectedFaces);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_GAZE_DIRECTION
    int32_t gazeDirection[(NUM_OF_DETECTED_FACES * 3) + 2] = {0};

    for (int i = 0; i < numOfDetectedFaces; i++) {
        int index = (i * 3) + 2;

        gazeDirection[index] = (int32_t)facialAngle[i].roll;
        if (gazeDirection[index] <= 180) {
            // nop
        } else {
            gazeDirection[index] = gazeDirection[index] - 360;
        }

        //CLOGV("facialAngle[%d].roll : %d -> gazeDirection[%d] : %d", i, facialAngle[i].roll, i, gazeDirection[index]);
    }

    settings->update(EXYNOS_ANDROID_VENDOR_STATS_GAZE_DIRECTION, gazeDirection, numOfDetectedFaces * 3);
#endif

    return;
}

void ExynosCameraMetadataConverter::translateVendorPartialLensMetaData(__unused CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;
}

void ExynosCameraMetadataConverter::translateVendorPartialControlMetaData(__unused CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;
}

void ExynosCameraMetadataConverter::translateVendorPartialMetaData(ExynosCameraRequestSP_sprt_t requestInfo,
                                                                 __unused CameraMetadata *settings,
                                                                 struct camera2_shot_ext *src_ext, __unused enum metadata_type metaType)
{
    struct camera2_shot *src = NULL;

    src = &src_ext->shot;

    return;
}

status_t ExynosCameraMetadataConverter::checkRangeOfValid(__unused int32_t tag, __unused int32_t value)
{
    status_t ret = NO_ERROR;
    __unused camera_metadata_entry_t entry;

    __unused const int32_t *i32Range = NULL;

    return ret;
}

void ExynosCameraMetadataConverter::setShootingMode(__unused int shotMode, struct camera2_shot_ext *dst_ext)
{
    __unused enum aa_scene_mode sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
    __unused bool changeSceneMode = true;
    enum aa_mode mode = dst_ext->shot.ctl.aa.mode;

    if (mode == AA_CONTROL_USE_SCENE_MODE) {
        return;
    }
}

void ExynosCameraMetadataConverter::setSceneMode(int value, struct camera2_shot_ext *dst_ext)
{
    enum aa_scene_mode sceneMode = AA_SCENE_MODE_FACE_PRIORITY;

    if (dst_ext->shot.ctl.aa.mode != AA_CONTROL_USE_SCENE_MODE) {
        return;
    }

    switch (value) {
    case ANDROID_CONTROL_SCENE_MODE_PORTRAIT:
        sceneMode = AA_SCENE_MODE_PORTRAIT;
        break;
    case ANDROID_CONTROL_SCENE_MODE_LANDSCAPE:
        sceneMode = AA_SCENE_MODE_LANDSCAPE;
        break;
    case ANDROID_CONTROL_SCENE_MODE_NIGHT:
        sceneMode = AA_SCENE_MODE_NIGHT;
        break;
    case ANDROID_CONTROL_SCENE_MODE_BEACH:
        sceneMode = AA_SCENE_MODE_BEACH;
        break;
    case ANDROID_CONTROL_SCENE_MODE_SNOW:
        sceneMode = AA_SCENE_MODE_SNOW;
        break;
    case ANDROID_CONTROL_SCENE_MODE_SUNSET:
        sceneMode = AA_SCENE_MODE_SUNSET;
        break;
    case ANDROID_CONTROL_SCENE_MODE_FIREWORKS:
        sceneMode = AA_SCENE_MODE_FIREWORKS;
        break;
    case ANDROID_CONTROL_SCENE_MODE_SPORTS:
        sceneMode = AA_SCENE_MODE_SPORTS;
        break;
    case ANDROID_CONTROL_SCENE_MODE_PARTY:
        sceneMode = AA_SCENE_MODE_PARTY;
        break;
    case ANDROID_CONTROL_SCENE_MODE_CANDLELIGHT:
        sceneMode = AA_SCENE_MODE_CANDLELIGHT;
        break;
    case ANDROID_CONTROL_SCENE_MODE_STEADYPHOTO:
        sceneMode = AA_SCENE_MODE_STEADYPHOTO;
        break;
    case ANDROID_CONTROL_SCENE_MODE_ACTION:
        sceneMode = AA_SCENE_MODE_ACTION;
        break;
    case ANDROID_CONTROL_SCENE_MODE_NIGHT_PORTRAIT:
        sceneMode = AA_SCENE_MODE_NIGHT_PORTRAIT;
        break;
    case ANDROID_CONTROL_SCENE_MODE_THEATRE:
        sceneMode = AA_SCENE_MODE_THEATRE;
        break;
    case ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY:
        sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
        break;
    case ANDROID_CONTROL_SCENE_MODE_HDR:
        sceneMode = AA_SCENE_MODE_HDR;
        break;
    case ANDROID_CONTROL_SCENE_MODE_DISABLED:
    default:
        sceneMode = AA_SCENE_MODE_DISABLED;
        break;
    }

    setMetaCtlSceneMode(dst_ext, sceneMode);
}

enum aa_afstate ExynosCameraMetadataConverter::translateVendorAfStateMetaData(enum aa_afstate mainAfState)
{
    enum aa_afstate resultAfState = mainAfState;

    return resultAfState;
}

void ExynosCameraMetadataConverter::translateVendorScalerMetaData(struct camera2_shot_ext *src_ext)
{
    struct camera2_shot *src = NULL;
    float appliedZoomRatio = 1.0f;
    float userZoomRatio = 1.0f;
    ExynosRect zoomRect = {0, };
    int sensorMaxW = 0, sensorMaxH = 0;

    src = &(src_ext->shot);

    appliedZoomRatio = src->udm.zoomRatio;
    userZoomRatio = src->uctl.zoomRatio;

    {
        src->dm.scaler.cropRegion[0] = src->ctl.scaler.cropRegion[0];
        src->dm.scaler.cropRegion[1] = src->ctl.scaler.cropRegion[1];
        src->dm.scaler.cropRegion[2] = src->ctl.scaler.cropRegion[2];
        src->dm.scaler.cropRegion[3] = src->ctl.scaler.cropRegion[3];
    }

    CLOGV("CropRegion(%f)(%d,%d,%d,%d)->(%f/%f)(%d,%d,%d,%d)",
            userZoomRatio,
            src->ctl.scaler.cropRegion[0],
            src->ctl.scaler.cropRegion[1],
            src->ctl.scaler.cropRegion[2],
            src->ctl.scaler.cropRegion[3],
            appliedZoomRatio,
            (float) sensorMaxW / (float)src->dm.scaler.cropRegion[2],
            src->dm.scaler.cropRegion[0],
            src->dm.scaler.cropRegion[1],
            src->dm.scaler.cropRegion[2],
            src->dm.scaler.cropRegion[3]
    );
}

status_t ExynosCameraMetadataConverter::m_createVendorAvailableKeys(
        __unused const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        __unused Vector<int32_t> *session, __unused Vector<int32_t> *request, __unused Vector<int32_t> *result, __unused Vector<int32_t> *characteristics, __unused int cameraId)
{
    status_t ret = NO_ERROR;

#ifdef SUPPORT_VENDOR_TAG_MF_STILL
    request->add(SLSI_MF_STILL_MODE);
    result->add(SLSI_MF_STILL_MODE);

    request->add(SLSI_MF_STILL_CAPTURE);
    result->add(SLSI_MF_STILL_CAPTURE);

    request->add(SLSI_MF_STILL_RESULT);
    result->add(SLSI_MF_STILL_RESULT);

    request->add(SLSI_MF_STILL_PARAMETERS);
    result->add(SLSI_MF_STILL_PARAMETERS);

    characteristics->add(SLSI_MF_STILL_VERSION);
#endif

#ifdef SUPPORT_VEDNOR_TAG_SENSOR_FPS_SUPPORT_RANGE
    if (sensorStaticInfo->hiddenfpsRangeList != NULL) {
        characteristics->add(EXYNOS_ANDROID_VENDOR_SENSOR_VENDOR_FPS_SUPPORT_RANGE);
    }
#endif

#ifdef SUPPORT_REMOSAIC_CAPTURE
#ifdef SUPPORT_VENDOR_TAG_REMOSAIC
    if (sensorStaticInfo->previewHighResolutionSizeLut != NULL) {
        request->add(EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION);
    }
#endif
#endif

#ifdef SUPPORT_VENDOR_TAG_3DHDR
    if (sensorStaticInfo->supported_sensor_ex_mode & (1 << EXTEND_SENSOR_MODE_3DHDR)) {
        request->add(EXYNOS_ANDROID_VENDOR_CONTROL_3DHDR);
    }
#endif

#ifdef USE_SLSI_VENDOR_TAGS
    session->add(EXYNOS_ANDROID_VENDOR_SESSION_MODE);
    request->add(EXYNOS_ANDROID_VENDOR_SESSION_MODE);
#endif

#ifdef SUPPORT_VENDOR_TAG_LONG_EXPOSURE_CAPTURE
    request->add(EXYNOS_ANDROID_VENDOR_CONTROL_LONG_EXPOSURE_CAPTURE);

    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_LONG_EXPOSURE_CAPTURE);
    characteristics->add(EXYNOS_ANDROID_VENDOR_SENSOR_VENDOR_EXPOSURE_SUPPORT_RANGE);
#endif

#ifdef USE_SLSI_VENDOR_TAGS
    session->add(EXYNOS_ANDROID_VENDOR_CONTROL_ZERO_CAMERA);
    request->add(EXYNOS_ANDROID_VENDOR_CONTROL_ZERO_CAMERA);
#endif

#ifdef SUPPORT_VENDOR_TAG_SCENE_DETECTION
    /* scene detection */
    request->add(EXYNOS_ANDROID_CONTROL_SCENE_DETECTION_INFO);
    result->add(EXYNOS_ANDROID_CONTROL_SCENE_DETECTION_INFO);
#endif
#ifdef USES_COMBINE_PLUGIN
    result->add(EXYNOS_ANDROID_VENDOR_SCENE_TYPE);
    session->add(EXYNOS_ANDROID_VENDOR_COMBINE_PREVIEW_PLUGIN);
    request->add(EXYNOS_ANDROID_VENDOR_COMBINE_PREVIEW_PLUGIN);
    request->add(EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV);
    result->add(EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV);
#endif

#ifdef USES_SUPER_RESOLUTION
    request->add(EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_RESOLUTION);
    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_RESOLUTION);
#endif
#ifdef SUPPORT_VENDOR_TAG_SENSOR_NAME
    /* sensor info */
    characteristics->add(EXYNOS_ANDROID_VENDOR_SENSOR_INFO);
#endif

#ifdef USE_SUPER_EIS
    session->add(EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_EIS);
    request->add(EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_EIS);
#endif

#ifdef USES_CLAHE_REPROCESSING
    request->add(EXYNOS_ANDROID_VENDOR_CONTROL_CLAHE_CAPTURE);
    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_CLAHE_CAPTURE);
#endif

#ifdef SUPPORT_VENDOR_TAG_SENSOR_INFO_ARCSOFT_DUAL_CALIB_BLOB
    //Don't need to add vendor tag to characteristics
    //characteristics->add(EXYNOS_ANDROID_VENDOR_SENSOR_INFO_ARCSOFT_DUAL_CALIB_BLOB);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_LUX_STD
    result->add(EXYNOS_ANDROID_VENDOR_ENVINFO_LUX_STD);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_LUX_IDX
    result->add(EXYNOS_ANDROID_VENDOR_ENVINFO_LUX_IDX);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_ANALOG_GAIN
    /* env info */
    result->add(EXYNOS_ANDROID_VENDOR_ENVINFO_ANALOG_GAIN);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_THERMAL_LEVEL
    result->add(EXYNOS_ANDROID_VENDOR_ENVINFO_THERMAL_LEVEL);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_ISO100_GAIN
    result->add(EXYNOS_ANDROID_VENDOR_ENVINFO_ISO100_GAIN);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_AWB_CCT
    result->add(EXYNOS_ANDROID_VENDOR_ENVINFO_AWB_CCT);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_AWB_DEC
    result->add(EXYNOS_ANDROID_VENDOR_ENVINFO_AWB_DEC);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_LENS_POS
    result->add(EXYNOS_ANDROID_VENDOR_ENVINFO_LENS_POS);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_LINECOUNT
    result->add(EXYNOS_ANDROID_VENDOR_ENVINFO_LINECOUNT);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_AFD_SUBMODE
    result->add(EXYNOS_ANDROID_VENDOR_ENVINFO_AFD_SUBMODE);
#endif

#ifdef SUPPORT_VENDOR_TAG_ENVINFO_FLICKER_DETECT
    result->add(EXYNOS_ANDROID_VENDOR_ENVINFO_FLICKER_DETECT);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_EXP_PRI
    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_EXP_PRI);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_ISO_PRI
    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_ISO_PRI);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_FLIP_STILL
    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_STIL);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_FLIP_VIDEO
    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_FLIP_VIDEO);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_ROTATION_STILL
    request->add(EXYNOS_ANDROID_VENDOR_CONTROL_ROTATION_STILL);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_EV
    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_EV);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_CAMERA_ID
    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_CAMERA_ID);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_ISO_TOTAL
    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_ISO_TOTAL);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_HDR_GAIN
    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_HDR_GAIN);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_HDR_SHUTTER
    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_HDR_SHUTTER);
#endif

#ifdef SUPPORT_VENDOR_TAG_CONTROL_SENSOR_GAIN
    result->add(EXYNOS_ANDROID_VENDOR_CONTROL_SENSOR_GAIN);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_BLINK_DEGREE
    result->add(EXYNOS_ANDROID_VENDOR_STATS_BLINK_DEGREE);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_BLINK_DETECTED
    result->add(EXYNOS_ANDROID_VENDOR_STATS_BLINK_DETECTED);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_SMILE_CONFIDENCE
    result->add(EXYNOS_ANDROID_VENDOR_STATS_SMILE_CONFIDENCE);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_SMILE_DEGREE
    result->add(EXYNOS_ANDROID_VENDOR_STATS_SMILE_DEGREE);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_GAZE_ANGLE
    result->add(EXYNOS_ANDROID_VENDOR_STATS_GAZE_ANGLE);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_GAZE_DIRECTION
    result->add(EXYNOS_ANDROID_VENDOR_STATS_GAZE_DIRECTION);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_AEC_AECLUX
    result->add(EXYNOS_ANDROID_VENDOR_STATS_AEC_AECLUX);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_AWB_CCT_VALUE
    result->add(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_AWB_CCT_VALUE);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_AEC_AEC
    result->add(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_AEC_AEC_STATUS);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_AF_STAT
    result->add(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_AF_STATUS);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_LENS_SHIFT_MM
    result->add(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_LENS_SHIFT_MM);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_OBJECT_DISTANCE_MM
    result->add(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_OBJECT_DISTANCE_MM);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_NEAR_FIELD_MM
    result->add(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_NEAR_FIELD_MM);
#endif

#ifdef SUPPORT_VENDOR_TAG_STATS_3RD_INFO_FAR_FIELD_MM
    result->add(EXYNOS_ANDROID_VENDOR_STATS_3RD_INFO_FAR_FIELD_MM);
#endif

#ifdef SUPPORT_VENDOR_TAG_JPEG_ENCODE_CROP_ENABLE
    result->add(EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_ENABLE);
#endif

#ifdef SUPPORT_VENDOR_TAG_JPEG_ENCODE_CROP_RECT
    result->add(EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_RECT);
#endif

#ifdef SUPPORT_VENDOR_TAG_JPEG_ENCODE_CROP_ROI
    result->add(EXYNOS_ANDROID_VENDOR_JPEG_ENCODE_CROP_ROI);
#endif

#ifdef SUPPORT_VENDOR_TAG_SNAPSHOT_APPX
    result->add(EXYNOS_ANDROID_VENDOR_SNAPSHOT_APPX);
#endif

#ifdef SUPPORT_VENDOR_TAG_SNAPSHOT_EXIF
    result->add(EXYNOS_ANDROID_VENDOR_SNAPSHOT_EXIF);
#endif

#ifdef SUPPORT_VENDOR_TAG_SNAPSHOT_MAKERNOTE
    result->add(EXYNOS_ANDROID_VENDOR_SNAPSHOT_MAKER_NOTE);
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_FOCUS_POS
    result->add(EXYNOS_ANDROID_VENDOR_FACTORY_FOCUS_POS);
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_CALIBRATION_STATUS
    result->add(EXYNOS_ANDROID_VENDOR_FACTORY_CALIBRATION_STATUS);
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_MODULE_ID
    result->add(EXYNOS_ANDROID_VENDOR_FACTORY_MODULE_ID);
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_OIS_GEA
    result->add(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_GEA);
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_OIS_HEA
    result->add(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_HEA);
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_OIS_FW_VER
    result->add(EXYNOS_ANDROID_VENDOR_FACTORY_OIS_FW_VER);
#endif

#ifdef SUPPORT_VENDOR_TAG_FACTORY_LED_CALIBRATION
    result->add(EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_STROBE_CURRENTS);

    result->add(EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_ENABLED);

    result->add(EXYNOS_ANDROID_VENDOR_FACTORY_LED_CALIBRATION_DATA);
#endif

#ifdef USE_HW_BEAUTY_CONTROL
    if (isBackCamera(cameraId)) {
        characteristics->add(EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_STRENGTH_RANGE);
        request->add(EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_STRENGTH);
        result->add(EXYNOS_ANDROID_VENDOR_BEAUTY_FACE_STRENGTH);
    }
#endif

#ifdef SUPPORT_VENDOR_TAG_ABORT_CAPTURE
    characteristics->add(EXYNOS_ANDROID_VENDOR_ABORT_CAPTURE_ENABLE);
#endif

#ifdef SUPPORT_VENDOR_TAG_CHI_OVERRIDE_EXPINDEX
    result->add(EXYNOS_ANDROID_VENDOR_CHI_OVERRIDE_EXPINDEX);
#endif

#ifdef USES_OFFLINE_CAPTURE
    session->add(EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_SESSION_ENABLE);
    session->add(EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_SESSION_IMAGE_READER_ID);
    request->add(EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_SESSION_ENABLE);
    request->add(EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_SESSION_IMAGE_READER_ID);

    request->add(EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_REQUEST_ID);

    result->add(EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_RESULT_NEXT_OPERATION);
#endif

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailablePreviewConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs,
        enum sensor_stream_type streamType)
{
    status_t ret = NO_ERROR;
    int (*yuvSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int yuvSizeListLength = 0;
    int (*hiddenPreviewSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int hiddenPreviewSizeListLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->yuvList == NULL) {
        CLOGI2("VendorYuvList is NULL");
        return BAD_VALUE;
    }

    yuvSizeList = sensorStaticInfo->yuvList;
    yuvSizeListLength = sensorStaticInfo->yuvListMax;

    /* HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream supported size list */
    for (int i = 0; i < yuvSizeListLength; i++) {
        streamConfigs->add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        streamConfigs->add(yuvSizeList[i][0]);
        streamConfigs->add(yuvSizeList[i][1]);
        streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
    }

    if (sensorStaticInfo->hiddenPreviewList != NULL) {
        hiddenPreviewSizeList = sensorStaticInfo->hiddenPreviewList;
        hiddenPreviewSizeListLength = sensorStaticInfo->hiddenPreviewListMax;

        /* add hidden size list */
        for (int i = 0; i < hiddenPreviewSizeListLength; i++) {
            streamConfigs->add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
            streamConfigs->add(hiddenPreviewSizeList[i][0]);
            streamConfigs->add(hiddenPreviewSizeList[i][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailablePictureConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs,
        enum sensor_stream_type streamType)
{
    status_t ret = NO_ERROR;
    int (*jpegSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int jpegSizeListLength = 0;
    int (*hiddenPictureSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int hiddenPictureSizeListLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->jpegList == NULL) {
        CLOGI2("VendorJpegList is NULL");
        return BAD_VALUE;
    }

    jpegSizeList = sensorStaticInfo->jpegList;
    jpegSizeListLength = sensorStaticInfo->jpegListMax;

    /* Stall output stream supported size list */
    for (size_t i = 0; i < ARRAY_LENGTH(STALL_FORMATS); i++) {
        for (int j = 0; j < jpegSizeListLength; j++) {
            streamConfigs->add(STALL_FORMATS[i]);
            streamConfigs->add(jpegSizeList[j][0]);
            streamConfigs->add(jpegSizeList[j][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

    if (sensorStaticInfo->hiddenPictureList != NULL) {
        hiddenPictureSizeList = sensorStaticInfo->hiddenPictureList;
        hiddenPictureSizeListLength = sensorStaticInfo->hiddenPictureListMax;

        /* add hidden size list */
        for (size_t i = 0; i < ARRAY_LENGTH(STALL_FORMATS); i++) {
            for (int j = 0; j < hiddenPictureSizeListLength; j++) {
                streamConfigs->add(STALL_FORMATS[i]);
                streamConfigs->add(hiddenPictureSizeList[j][0]);
                streamConfigs->add(hiddenPictureSizeList[j][1]);
                streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
            }
        }
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableVideoConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int (*availableVideoSizeList)[7] = NULL;
    int availableVideoSizeListLength = 0;
    int cropRatio = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->availableVideoList == NULL) {
        CLOGI2("VendorVideoList is NULL");
        return BAD_VALUE;
    }

    availableVideoSizeList = sensorStaticInfo->availableVideoList;
    availableVideoSizeListLength = sensorStaticInfo->availableVideoListMax;

    for (int i = 0; i < availableVideoSizeListLength; i++) {
        streamConfigs->add(availableVideoSizeList[i][0]);
        streamConfigs->add(availableVideoSizeList[i][1]);
        streamConfigs->add(availableVideoSizeList[i][2]/1000);
        streamConfigs->add(availableVideoSizeList[i][3]/1000);
        cropRatio = 0;
        /* cropRatio = vdisW * 1000 / nVideoW;
         * cropRatio = ((cropRatio - 1000) + 5) / 10 */
        if (availableVideoSizeList[i][4] != 0) {
            cropRatio = (int)(availableVideoSizeList[i][4] * 1000) / (int)availableVideoSizeList[i][0];
            cropRatio = ((cropRatio - 1000) + 5) / 10;
        }
        streamConfigs->add(cropRatio);
        streamConfigs->add(availableVideoSizeList[i][6]);
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableHighSpeedVideoConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int (*availableHighSpeedVideoList)[5] = NULL;
    int availableHighSpeedVideoListLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->availableHighSpeedVideoList == NULL) {
        CLOGI2("availableHighSpeedVideoList is NULL");
        return BAD_VALUE;
    }

    availableHighSpeedVideoList = sensorStaticInfo->availableHighSpeedVideoList;
    availableHighSpeedVideoListLength = sensorStaticInfo->availableHighSpeedVideoListMax;

    for (int i = 0; i < availableHighSpeedVideoListLength; i++) {
        streamConfigs->add(availableHighSpeedVideoList[i][0]);
        streamConfigs->add(availableHighSpeedVideoList[i][1]);
        streamConfigs->add(availableHighSpeedVideoList[i][2]/1000);
        streamConfigs->add(availableHighSpeedVideoList[i][3]/1000);
        streamConfigs->add(availableHighSpeedVideoList[i][4]);
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableAeModeConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<uint8_t> *vendorAeModes)
{
    status_t ret = NO_ERROR;
    __unused uint8_t (*baseAeModes) = NULL;
    __unused int baseAeModesLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (vendorAeModes == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->aeModes == NULL) {
        CLOGE2("aeModes is NULL");
        return BAD_VALUE;
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableAfModeConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<uint8_t> *vendorAfModes)
{
    status_t ret = NO_ERROR;
    uint8_t (*baseAfModes) = NULL;
    int baseAfModesLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (vendorAfModes == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->afModes == NULL) {
        CLOGE2("availableafModes is NULL");
        return BAD_VALUE;
    }

    baseAfModes = sensorStaticInfo->afModes;
    baseAfModesLength = sensorStaticInfo->afModesLength;

    /* default af modes */
    for (int i = 0; i < baseAfModesLength; i++) {
        vendorAfModes->add(baseAfModes[i]);
    }

    return ret;
}

#ifdef SUPPORT_DEPTH_MAP
status_t ExynosCameraMetadataConverter::m_createVendorDepthAvailableDepthConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int availableDepthSizeListMax = 0;
    int (*availableDepthSizeList)[2] = NULL;
    int availableDepthFormatListMax = 0;
    int *availableDepthFormatList = NULL;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }

    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    availableDepthSizeList = sensorStaticInfo->availableDepthSizeList;
    availableDepthSizeListMax = sensorStaticInfo->availableDepthSizeListMax;
    availableDepthFormatList = sensorStaticInfo->availableDepthFormatList;
    availableDepthFormatListMax = sensorStaticInfo->availableDepthFormatListMax;

    for (int i = 0; i < availableDepthFormatListMax; i++) {
        for (int j = 0; j < availableDepthSizeListMax; j++) {
            streamConfigs->add(availableDepthFormatList[i]);
            streamConfigs->add(availableDepthSizeList[j][0]);
            streamConfigs->add(availableDepthSizeList[j][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

#ifdef DEBUG_STREAM_CONFIGURATIONS
    const int32_t* streamConfigArray = NULL;
    streamConfigArray = streamConfigs->array();
    for (size_t i = 0; i < streamConfigs->size(); i = i + 4) {
        CLOGD2("Size %4dx%4d Format %2x %6s",
                streamConfigArray[i+1], streamConfigArray[i+2],
                streamConfigArray[i],
                (streamConfigArray[i+3] == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT)?
                "OUTPUT" : "INPUT");
    }
#endif

    return ret;
}
#endif

status_t ExynosCameraMetadataConverter::m_createVendorScalerAvailableThumbnailConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int availableThumbnailCallbackSizeListMax = 0;
    int (*availableThumbnailCallbackSizeList)[2] = NULL;
    int availableThumbnailCallbackFormatListMax = 0;
    int *availableThumbnailCallbackFormatList = NULL;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }

    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    availableThumbnailCallbackSizeList = sensorStaticInfo->availableThumbnailCallbackSizeList;
    availableThumbnailCallbackSizeListMax = sensorStaticInfo->availableThumbnailCallbackSizeListMax;
    availableThumbnailCallbackFormatList = sensorStaticInfo->availableThumbnailCallbackFormatList;
    availableThumbnailCallbackFormatListMax = sensorStaticInfo->availableThumbnailCallbackFormatListMax;

    for (int i = 0; i < availableThumbnailCallbackFormatListMax; i++) {
        for (int j = 0; j < availableThumbnailCallbackSizeListMax; j++) {
            streamConfigs->add(availableThumbnailCallbackFormatList[i]);
            streamConfigs->add(availableThumbnailCallbackSizeList[j][0]);
            streamConfigs->add(availableThumbnailCallbackSizeList[j][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorScalerAvailableIrisConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    status_t ret = NO_ERROR;
    int availableIrisSizeListMax = 0;
    int (*availableIrisSizeList)[2] = NULL;
    int availableIrisFormatListMax = 0;
    int *availableIrisFormatList = NULL;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }

    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    availableIrisSizeList = sensorStaticInfo->availableIrisSizeList;
    availableIrisSizeListMax = sensorStaticInfo->availableIrisSizeListMax;
    availableIrisFormatList = sensorStaticInfo->availableIrisFormatList;
    availableIrisFormatListMax = sensorStaticInfo->availableIrisFormatListMax;

    for (int i = 0; i < availableIrisFormatListMax; i++) {
        for (int j = 0; j < availableIrisSizeListMax; j++) {
            streamConfigs->add(availableIrisFormatList[i]);
            streamConfigs->add(availableIrisSizeList[j][0]);
            streamConfigs->add(availableIrisSizeList[j][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableEffectModesConfigurations(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<uint8_t> *vendorEffectModes)
{
    status_t ret = NO_ERROR;
    __unused uint8_t (*baseEffectModes) = NULL;
    __unused int baseEffectModesLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (vendorEffectModes == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->effectModes == NULL) {
        CLOGE2("effectModes is NULL");
        return BAD_VALUE;
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorEffectAeAvailableFpsRanges(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *fpsRanges)
{
    int ret = OK;
    int (*fpsRangesList)[2] = NULL;
    size_t fpsRangesLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (fpsRanges == NULL) {
        CLOGE2("FPS ranges is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->effectFpsRangesList == NULL) {
        CLOGI2("effectFpsRangesList is NULL");
        return BAD_VALUE;
    }

    fpsRangesList = sensorStaticInfo->effectFpsRangesList;
    fpsRangesLength = sensorStaticInfo->effectFpsRangesListMax;

    for (size_t i = 0; i < fpsRangesLength; i++) {
        fpsRanges->add(fpsRangesList[i][0]/1000);
        fpsRanges->add(fpsRangesList[i][1]/1000);
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorAvailableThumbnailSizes(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs)
{
    int ret = OK;
    int (*hiddenThumbnailList)[2] = NULL;
    int hiddenThumbnailListMax = 0;
    int (*thumbnailList)[SIZE_OF_RESOLUTION] = NULL;
    int thumbnailListMax = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }

    if (streamConfigs == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->thumbnailList == NULL) {
        CLOGI2("thumbnailList is NULL");
        return BAD_VALUE;
    }

    thumbnailList = sensorStaticInfo->thumbnailList;
    thumbnailListMax = sensorStaticInfo->thumbnailListMax;

    for (int i = 0; i < thumbnailListMax; i++) {
        streamConfigs->add(thumbnailList[i][0]);
        streamConfigs->add(thumbnailList[i][1]);
    }

    if (sensorStaticInfo->hiddenThumbnailList != NULL) {
        hiddenThumbnailList = sensorStaticInfo->hiddenThumbnailList;
        hiddenThumbnailListMax = sensorStaticInfo->hiddenThumbnailListMax;

        for (int i = 0; i < hiddenThumbnailListMax; i++) {
            streamConfigs->add(hiddenThumbnailList[i][0]);
            streamConfigs->add(hiddenThumbnailList[i][1]);
        }
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_createVendorControlAvailableFeatures(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *availableFeatures, cameraId_Info *camIdInfo)
{
    status_t ret = NO_ERROR;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }
    if (availableFeatures == NULL) {
        CLOGE2("Stream configs is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->availableBasicFeaturesList == NULL) {
        CLOGE2("availableBasicFeaturesList is NULL");
        return BAD_VALUE;
    }

    int *basicFeatures = sensorStaticInfo->availableBasicFeaturesList;
    int basicFeaturesLength = sensorStaticInfo->availableBasicFeaturesListLength;
    int *sensorFeatures = sensorStaticInfo->availableOptionalFeaturesList;
    int sensorFeaturesLength = sensorStaticInfo->availableOptionalFeaturesListLength;

    /* basic features */
    for (int i = 0; i < basicFeaturesLength; i++) {
        availableFeatures->add(basicFeatures[i]);
    }

    /* sensor feature */
    if (sensorFeatures) {
        for (int i = 0; i < sensorFeaturesLength; i++) {
            availableFeatures->add(sensorFeatures[i]);
        }
    }

    return ret;
}

status_t ExynosCameraMetadataConverter::m_getMaxPictureSize(
        const struct ExynosCameraSensorInfoBase *sensorStaticInfo,
        int *maxPictureW, int *maxPictureH)
{
    status_t ret = NO_ERROR;
    int width = 0, height = 0;
    int (*jpegSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int jpegSizeListLength = 0;
    int (*hiddenPictureSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int hiddenPictureSizeListLength = 0;

    if (sensorStaticInfo == NULL) {
        CLOGE2("Sensor static info is NULL");
        return BAD_VALUE;
    }

    if (sensorStaticInfo->jpegList == NULL) {
        CLOGI2("VendorJpegList is NULL");
        return BAD_VALUE;
    }

    jpegSizeList = sensorStaticInfo->jpegList;
    jpegSizeListLength = sensorStaticInfo->jpegListMax;
    for (int i = 0; i < jpegSizeListLength; i++) {
        int listWidth = 0, listHeight = 0;

        listWidth = jpegSizeList[i][0];
        if (listWidth > width) {
            width = listWidth;
        }

        listHeight = jpegSizeList[i][1];
        if (listHeight > height) {
            height = listHeight;
        }
    }

    if (sensorStaticInfo->hiddenPictureList != NULL) {
        hiddenPictureSizeList = sensorStaticInfo->hiddenPictureList;
        hiddenPictureSizeListLength = sensorStaticInfo->hiddenPictureListMax;
    }
    for (int i = 0; i < hiddenPictureSizeListLength; i++) {
        int listWidth = 0, listHeight = 0;

        listWidth = hiddenPictureSizeList[i][0];
        if (listWidth > width) {
            width = listWidth;
        }

        listHeight = hiddenPictureSizeList[i][1];
        if (listHeight > height) {
            height = listHeight;
        }
    }

    *maxPictureW = width;
    *maxPictureH = height;

    return ret;
}

}; /* namespace android */
