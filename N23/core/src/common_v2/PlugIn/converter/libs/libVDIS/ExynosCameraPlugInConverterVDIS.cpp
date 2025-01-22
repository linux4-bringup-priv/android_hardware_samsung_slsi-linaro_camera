/*
 * Copyright (C) 2014, Samsung Electronics Co. LTD
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
#define LOG_TAG "ExynosCameraPlugInConverterVDIS"

#include "ExynosCameraPlugInConverterVDIS.h"
#include "ExynosCameraDefine.h"

namespace android {

/*********************************************/
/*  protected functions                      */
/*********************************************/
status_t ExynosCameraPlugInConverterVDIS::m_init(void)
{
    strncpy(m_name, "ConverterVDIS", (PLUGIN_NAME_STR_SIZE - 1));

    // For EIS lib's view
    m_recordingBufPos = 0;
    m_previewBufPos = 1;

    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterVDIS::m_deinit(void)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterVDIS::m_create(__unused Map_t *map)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterVDIS::m_setup(Map_t *map)
{
    ExynosCameraParameters *parameters = NULL;
    ExynosCameraConfigurations *configurations = NULL;
    PLUGIN_CONVERT_TYPE_T type = PLUGIN_CONVERT_BASE;

    uint32_t hwWidth = 0, hwHeight = 0, videoW = 1920, videoH = 1080;
    uint32_t previewW = 0, previewH = 0;
    int fps = 0;
    bool superEis = false;
    ExynosCameraSensorInfoBase *sensorInfo = NULL;

    type = (enum PLUGIN_CONVERT_TYPE_T)(unsigned long)(*map)[PLUGIN_CONVERT_TYPE];

    switch(type) {
    case PLUGIN_CONVERT_SETUP_BEFORE:
        parameters = (ExynosCameraParameters *)(*map)[PLUGIN_CONVERT_PARAMETER];
        configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];

        configurations->getSize(CONFIGURATION_PREVIEW_SIZE, (uint32_t *)&previewW, (uint32_t *)&previewH);
        configurations->getSize(CONFIGURATION_VIDEO_SIZE, (uint32_t *)&videoW, (uint32_t *)&videoH);
        fps = configurations->getModeValue(CONFIGURATION_RECORDING_FPS);
        superEis = configurations->getMode(CONFIGURATION_SUPER_EIS_MODE);

#ifdef USE_SW_VDIS_UHD_RECORDING
        if (videoW == 3840 && videoH == 2160) {
            hwWidth = VIDEO_MARGIN_UHD_W;
            hwHeight = VIDEO_MARGIN_UHD_H;
        } else
#endif
        if (videoW == 1920 && videoH == 1080) {
#ifdef USE_SUPER_EIS
            if (superEis) {
                hwWidth = VIDEO_SUPER_EIS_MARGIN_FHD_W;
                hwHeight = VIDEO_SUPER_EIS_MARGIN_FHD_H;
            } else
#endif
            {
                hwWidth = VIDEO_MARGIN_FHD_W;
                hwHeight = VIDEO_MARGIN_FHD_H;
            }
        } else if (videoW == 1280 && videoH == 720){
            hwWidth = VIDEO_MARGIN_HD_W;
            hwHeight = VIDEO_MARGIN_HD_H;
#ifdef USE_SUPER_EIS
            if (superEis) {
                hwWidth = VIDEO_SUPER_EIS_MARGIN_HD_W;
                hwHeight = VIDEO_SUPER_EIS_MARGIN_HD_H;
            } else
#endif
            {
                hwWidth = VIDEO_MARGIN_HD_W;
                hwHeight = VIDEO_MARGIN_HD_H;
            }
        } else if (videoW == 1920 && videoH == 816){
            hwWidth = VIDEO_MARGIN_1920_W;
            hwHeight = VIDEO_MARGIN_816_H;
        } else {
            videoW = 1920;
            videoH = 1080;
#ifdef USE_SUPER_EIS
            if (superEis) {
                hwWidth = VIDEO_SUPER_EIS_MARGIN_FHD_W;
                hwHeight = VIDEO_SUPER_EIS_MARGIN_FHD_H;
            } else
#endif
            {
                hwWidth = VIDEO_MARGIN_FHD_W;
                hwHeight = VIDEO_MARGIN_FHD_H;
            }
        }

        (*map)[PLUGIN_ARRAY_RECT_X] = (Map_data_t)previewW;
        (*map)[PLUGIN_ARRAY_RECT_Y] = (Map_data_t)previewH;
        (*map)[PLUGIN_ARRAY_RECT_W] = (Map_data_t)videoW;
        (*map)[PLUGIN_ARRAY_RECT_H] = (Map_data_t)videoH;
        (*map)[PLUGIN_ARRAY_RECT_FULL_W] = (Map_data_t)hwWidth;
        (*map)[PLUGIN_ARRAY_RECT_FULL_H] = (Map_data_t)hwHeight;
        (*map)[PLUGIN_FPS] = (Map_data_t)fps;

        m_previewSizeW   = previewW;
        m_previewSizeH   = previewH;
        m_recordingSizeW = videoW;
        m_recordingSizeH = videoH;
        m_inputSizeW     = hwWidth;
        m_inputSizeH     = hwHeight;

        sensorInfo = parameters->getSensorStaticInfo();
        if (sensorInfo == NULL) {
            CLOG_ASSERT("sensorInfo == NULL. please check it");
        } else if (sensorInfo->sensorId <= SENSOR_NAME_NOTHING || SENSOR_NAME_END <= sensorInfo->sensorId) {
            CLOG_ASSERT("Invalid sensorInfo->sensorId(%d). please check it", sensorInfo->sensorId);
        } else {
            (*map)[PLUGIN_SENSOR_ID] = (Map_data_t)sensorInfo->sensorId;
        }

        CLOGD("Init sensorId(%d) margin(%d x %d), video target(%d x %d) preview target(%d x %d) fps(%d)",
            sensorInfo->sensorId, hwWidth, hwHeight, videoW, videoH, previewW, previewH, fps);

        memset(&m_bcrop, 0, sizeof(plugin_rect_t));
        memset(&m_previewCrop, 0, sizeof(plugin_rect_t));
        break;
    default:
        break;
    }

    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterVDIS::m_make(Map_t *map)
{
    status_t ret = NO_ERROR;
    enum PLUGIN_CONVERT_TYPE_T type;
    ExynosCameraFrameSP_sptr_t frame = NULL;
    struct camera2_shot_ext *metaData;
    ExynosCameraBuffer buffer;
    ExynosCameraParameters *parameters = NULL;
    ExynosCameraConfigurations *configurations = NULL;
    ExynosCameraSensorInfoBase *sensorInfo = NULL;

    type = (enum PLUGIN_CONVERT_TYPE_T)(unsigned long)(*map)[PLUGIN_CONVERT_TYPE];
    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];

    /* meta data setting */
    ret = frame->getSrcBuffer(m_pipeId, &buffer, OUTPUT_NODE_1);
    if (ret < 0 || buffer.index < 0) {
        CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
        return BAD_VALUE;
    }
    metaData = (struct camera2_shot_ext *)buffer.addr[buffer.getMetaPlaneIndex()];
    // TODO: It should be depend on dispCameraId
    int32_t cameraId = frame->getDisplayCameraId();
    parameters = configurations->getParameters(cameraId);

#ifdef USE_MSL_VDIS_GYRO
    camera_gyro_data_t* gyroData;
    Array_pointer_gyro_data_t pGyroData_t;
#endif

    camera2_node_group node_group_info_bcrop;

#if 0
    char filePath[70];
    time_t rawtime;
    struct tm* timeinfo;
    int meW = 320;
    int meH = 240;
#endif

    switch (type) {
    case PLUGIN_CONVERT_PROCESS_BEFORE:
        if (frame == NULL) {
            CLOGE("frame is NULL!! type(%d), pipeId(%d)", type, m_pipeId);
            goto func_exit;
        }

        (*map)[PLUGIN_SRC_FRAMECOUNT] = (Map_data_t)frame->getMetaFrameCount();
        (*map)[PLUGIN_ME_FRAMECOUNT ] = (Map_data_t)frame->getMetaFrameCount();
        (*map)[PLUGIN_TIMESTAMP     ] = (Map_data_t)frame->getTimeStamp();
        (*map)[PLUGIN_EXPOSURE_TIME ] = (Map_data_t)metaData->shot.dm.sensor.exposureTime;
        (*map)[PLUGIN_TIMESTAMPBOOT ] = (Map_data_t)metaData->shot.udm.sensor.timeStampBoot;
        (*map)[PLUGIN_FRAME_DURATION] = (Map_data_t)metaData->shot.ctl.sensor.frameDuration;
        (*map)[PLUGIN_ROLLING_SHUTTER_SKEW] = (Map_data_t)metaData->shot.dm.sensor.rollingShutterSkew;
        (*map)[PLUGIN_OPTICAL_STABILIZATION_MODE_CTRL] = (Map_data_t)metaData->shot.ctl.lens.opticalStabilizationMode;
        (*map)[PLUGIN_OPTICAL_STABILIZATION_MODE_DM] = (Map_data_t)metaData->shot.dm.lens.opticalStabilizationMode;
        (*map)[PLUGIN_SRC_BUF_USED]   = (Map_data_t)-1;
        (*map)[PLUGIN_DST_BUF_VALID]  = (Map_data_t)1;

        sensorInfo = parameters->getSensorStaticInfo();
        if (sensorInfo == NULL) {
            CLOG_ASSERT("sensorInfo == NULL. please check it");
        } else if (sensorInfo->sensorId <= SENSOR_NAME_NOTHING || SENSOR_NAME_END <= sensorInfo->sensorId) {
            CLOG_ASSERT("Invalid sensorInfo->sensorId(%d). please check it", sensorInfo->sensorId);
        } else {
            (*map)[PLUGIN_SENSOR_ID] = (Map_data_t)sensorInfo->sensorId;
        }

        {
            ExynosRect srcRect, dstRect;
            dstRect.fullW = dstRect.w = m_inputSizeW;
            dstRect.fullH = dstRect.h = m_inputSizeH;

            std::vector<frame_size_scenario_t> scenarios;
            scenarios.push_back(FRAME_SIZE_FUSION_CROP  ); // Fusion trans
            scenarios.push_back(FRAME_SIZE_MCP_OUT      ); // HW chain trans
            scenarios.push_back(FRAME_SIZE_MCP_CROP     ); // HW chain trans
            scenarios.push_back(FRAME_SIZE_MCS_CROP_IN  ); // HW chain trans
            scenarios.push_back(FRAME_SIZE_ISP_CROP_IN  ); // HW chain trans
            scenarios.push_back(FRAME_SIZE_BDS          ); // HW chain trans
            scenarios.push_back(FRAME_SIZE_BCROP_OUT    ); // HW chain trans

            frame_size_info_t info;
            for (auto scenario : scenarios) {
                // calculate from display camera
                if (frame->getSizeInfo(scenario, info, cameraId)) {
                    srcRect = info.rect;
                    CFLOGV(frame, "[TRANS][CAM%d][Sensor%d][S%d][P%d] src[%d,%d,%d,%d(%dx%d)] dst[%d,%d,%d,%d(%dx%d)]",
                            cameraId, sensorInfo->sensorId,
                            scenario, info.pipeId,
                            srcRect.x, srcRect.y, srcRect.w, srcRect.h, srcRect.fullW, srcRect.fullH,
                            dstRect.x, dstRect.y, dstRect.w, dstRect.h, dstRect.fullW, dstRect.fullH);
                    dstRect = convertDstRectBySrcRect(srcRect, dstRect);
                }
            }

            m_bcrop.x = dstRect.x;
            m_bcrop.y = dstRect.y;
            m_bcrop.w = dstRect.w;
            m_bcrop.h = dstRect.h;
            CFLOGV(frame, "[TRANS][CAM%d][Sensor%d] bcrop[%d,%d,%d,%d]",
                    cameraId, sensorInfo->sensorId,
                    m_bcrop.x,
                    m_bcrop.y,
                    m_bcrop.w,
                    m_bcrop.h);
        }

        //CLOGD("bcrop (%d %d %d %d)", m_bcrop.x, m_bcrop.y, m_bcrop.w, m_bcrop.h);
        (*map)[PLUGIN_BCROP_RECT] = (Map_data_t)&m_bcrop;
        (*map)[PLUGIN_EIS_PREVIEW_CROP] = (Map_data_t)&m_previewCrop;

        //m_zoomRatio[0] = configurations->getZoomRatio();
        //(*map)[PLUGIN_ZOOM_RATIO] = (Map_data_t)m_zoomRatio;

#ifdef USE_MSL_VDIS_GYRO
        gyroData = frame->getGyroData();
        pGyroData_t = (Array_pointer_gyro_data_t)gyroData;
        (*map)[PLUGIN_GYRO_DATA] = (Map_data_t)pGyroData_t;
        (*map)[PLUGIN_GYRO_DATA_SIZE] = (Map_data_t)VDIS_GYRO_DATA_SIZE;

        for (int i=0; i<VDIS_GYRO_DATA_SIZE; i++) {
            CLOGE("gyro %x-%x-%x %llu",
                    pGyroData_t[i].x,
                    pGyroData_t[i].y,
                    pGyroData_t[i].z,
                    pGyroData_t[i].timestamp);
        }
#endif

#if 0
        CLOGD("timeStamp(%lu), timeStampBoot(%lu), exposureTime(%lu), frameDuration(%d), RollingShutterSkew(%d), OpticalStabilizationModeCtrl(%d), OpticalStabilizationModeDm(%d)",
                frame->getTimeStamp(),
                metaData->shot.udm.sensor.timeStampBoot, metaData->shot.dm.sensor.exposureTime, metaData->shot.ctl.sensor.frameDuration,
                metaData->shot.dm.sensor.rollingShutterSkew, metaData->shot.ctl.lens.opticalStabilizationMode, metaData->shot.dm.lens.opticalStabilizationMode);
#endif

#ifdef SUPPORT_ME
        ret = frame->getDstBuffer(m_pipeId, &buffer, OTF_NODE_1);
        if (ret < 0 || buffer.index < 0) {
            CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
        } else {
            (*map)[PLUGIN_ME_DOWNSCALED_BUF] = (Map_data_t)buffer.addr[0];
            (*map)[PLUGIN_MOTION_VECTOR_BUF] = (Map_data_t)&(metaData->shot.udm.me.motion_vector);
            (*map)[PLUGIN_CURRENT_PATCH_BUF] = (Map_data_t)&(metaData->shot.udm.me.current_patch);
        }

#if 0
        memset(filePath, 0, sizeof(filePath));
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        snprintf(filePath, sizeof(filePath), "/data/misc/cameraserver/ME_%d_%d_%02d%02d%02d_%02d%02d%02d.y12",
        m_cameraId, frame->getFrameCount(), timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday,
        timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

        CLOGD("[ME_DUMP] size (%d x %d) : %s", meW, meH, filePath);

        ret = dumpToFile((char *)filePath, buffer.addr[0], meW * meH);

        if (ret != true) {
            CLOGE("couldn't make a raw file");
        }
#endif

#endif
        // 1) normal case (recoring only or recording + preview)
        if (frame->hasServiceBuffer(HAL_STREAM_ID_VIDEO) == true) {
            break;
        }

        // 2) invalid case (no service buffer..)
        if (frame->hasServiceBuffer(HAL_STREAM_ID_PREVIEW) == false) {
            CFLOGE(frame, "invalid buffer situation..");
            break;
        }

        // 3) preview only
        //  - Preview buffer located at recordingBuf's position for now.
        //  - So need to migrate preview buffer to right position.
        ExynosCameraPlugInConverter::m_migrateDstBufferInfo(m_recordingBufPos, m_previewBufPos);

        break;
    case PLUGIN_CONVERT_PROCESS_AFTER:
        if (m_previewCrop.w && m_previewCrop.h) {
            frame_size_info_t info;
            info.pipeId = m_pipeId;
            info.rect.x = m_previewCrop.x;
            info.rect.y = m_previewCrop.y;
            info.rect.w = m_previewCrop.w;
            info.rect.h = m_previewCrop.h;
            info.rect.fullW = m_inputSizeW;
            info.rect.fullH = m_inputSizeH;

            frame->setSizeInfo(FRAME_SIZE_VDIS_CROP, info);
            int slaveCameraId = frame->getCameraId(OUTPUT_NODE_2);
            if (slaveCameraId < 0)
                frame->setSizeInfo(FRAME_SIZE_VDIS_CROP, info, slaveCameraId);
        }
#if 0 // for debug
        {
            frame_size_info_t info[9];
            frame->getSizeInfo(FRAME_SIZE_BCROP_OUT,   info[0]);
            frame->getSizeInfo(FRAME_SIZE_BDS,         info[1]);
            frame->getSizeInfo(FRAME_SIZE_ISP_CROP_IN, info[2]);
            frame->getSizeInfo(FRAME_SIZE_MCS_CROP_IN, info[3]);
            frame->getSizeInfo(FRAME_SIZE_MCP_CROP,    info[4]);
            frame->getSizeInfo(FRAME_SIZE_MCP_OUT,     info[5]);
            frame->getSizeInfo(FRAME_SIZE_FUSION_CROP, info[6]);
            frame->getSizeInfo(FRAME_SIZE_FUSION_SCALEUP,info[7]);
            frame->getSizeInfo(FRAME_SIZE_VDIS_CROP,   info[8]);

            CFLOGD(frame, "[VDIS] time:%lld, bcrop (%d %d %d %d), src[addr%p, size:%d, fd:%d] dst[addr%p, size:%d, fd:%d]",
                    frame->getTimeStamp(),
                    m_bcrop.x, m_bcrop.y, m_bcrop.w, m_bcrop.h,
                    m_srcBuf[0][0], m_srcBufSize[0][0], m_srcBufFd[0][0],
                    m_dstBuf[0][0], m_dstBufSize[0][0], m_dstBufFd[0][0]);
            CFLOGD(frame, "[VDIS] bcrop[%d:%d,%d,%d,%d -> %d:%d,%d,%d,%d], bds[%d:%d,%d,%d,%d], isp[%d:%d,%d,%d,%d], mcs[%d:%d,%d,%d,%d]," \
                    "mcp[%d:%d,%d,%d,%d] -> [%d:%d,%d,%d,%d], fusion[%d:%d,%d,%d,%d], vdis[%d:%d,%d,%d,%d]",
                    info[0].pipeId, info[0].rect.x, info[0].rect.y, info[0].rect.w, info[0].rect.h,
                    info[1].pipeId, info[1].rect.x, info[1].rect.y, info[1].rect.w, info[1].rect.h,
                    info[2].pipeId, info[2].rect.x, info[2].rect.y, info[2].rect.w, info[2].rect.h,
                    info[3].pipeId, info[3].rect.x, info[3].rect.y, info[3].rect.w, info[3].rect.h,
                    info[4].pipeId, info[4].rect.x, info[4].rect.y, info[4].rect.w, info[4].rect.h,
                    info[5].pipeId, info[5].rect.x, info[5].rect.y, info[5].rect.w, info[5].rect.h,
                    info[6].pipeId, info[6].rect.x, info[6].rect.y, info[6].rect.w, info[6].rect.h,
                    info[7].pipeId, info[7].rect.x, info[7].rect.y, info[7].rect.w, info[7].rect.h,
                    info[8].pipeId, info[8].rect.x, info[8].rect.y, info[8].rect.w, info[8].rect.h);
        }
#endif
        break;
    case PLUGIN_CONVERT_SETUP_AFTER:
        (*map)[PLUGIN_SRC_FRAMECOUNT] = (Map_data_t)frame->getMetaFrameCount();
        break;
    default:
        CLOGE("invalid convert type(%d)!! pipeId(%d)", type, m_pipeId);
        goto func_exit;
    }

    func_exit:

    return NO_ERROR;

}
}; /* namespace android */
