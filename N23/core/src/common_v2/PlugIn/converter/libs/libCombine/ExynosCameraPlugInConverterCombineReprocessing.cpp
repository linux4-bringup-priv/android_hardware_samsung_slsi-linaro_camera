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

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCameraPlugInConverterCombineReprocessing"

#include "ExynosCameraPlugInConverterCombineReprocessing.h"

namespace android {

/*********************************************/
/*  protected functions                      */
/*********************************************/
status_t ExynosCameraPlugInConverterCombineReprocessing::m_init(void)
{
    strncpy(m_name, "CoverterCombineReprocessing", (PLUGIN_NAME_STR_SIZE - 1));

    for (int i = 0; i < 4; i++) {
        m_aeRegion[i] = 0;
        m_gyro[i] = 0.0f;
    }

    m_frameQeueue.release();
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_deinit(void)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_create(Map_t *map)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_setup(Map_t *map)
{
    return NO_ERROR;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_make(Map_t *map)
{
    status_t ret = NO_ERROR;

    enum PLUGIN_CONVERT_TYPE_T type;
    ExynosCameraFrameSP_sptr_t frame = NULL;
    ExynosCameraBuffer buffer;
    ExynosCameraParameters *parameter = NULL;
    ExynosCameraBufferSupplier  *bufferSupplier = NULL;
    int scenario = (int)(*map)[PLUGIN_SCENARIO];

    type = (enum PLUGIN_CONVERT_TYPE_T)(unsigned long)(*map)[PLUGIN_CONVERT_TYPE];
    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    parameter = (ExynosCameraParameters *)(*map)[PLUGIN_CONVERT_PARAMETER];
    bufferSupplier = (ExynosCameraBufferSupplier *)(*map)[PLUGIN_CONVERT_BUFFERSUPPLIER];

    switch (type) {
    case PLUGIN_CONVERT_PROCESS_BEFORE:
    {
        ////////////////////////////////////////////////
        // common
        ret = m_makeProcessBeforeCommon(map, scenario);
        if (ret != NO_ERROR) {
            CLOGE("m_makeProcessBeforeCommon() fail");
            goto func_exit;
        }

        ////////////////////////////////////////////////
        // night shot bayer
        ret = m_makeProcessBeforeNightShotBayer(map);
        if (ret != NO_ERROR) {
            CLOGE("m_makeProcessBeforeNightShotBayer() fail");
            goto func_exit;
        }

        ////////////////////////////////////////////////
        // night shot yuv
        ret = m_makeProcessBeforeNightShotYuv(map);
        if (ret != NO_ERROR) {
            CLOGE("m_makeProcessBeforeNightShotYuv() fail");
            goto func_exit;
        }

        ////////////////////////////////////////////////
        // super night shot bayer
        ret = m_makeProcessBeforeSuperNightShotBayer(map);
        if (ret != NO_ERROR) {
            CLOGE("m_makeProcessBeforeSuperNightShotBayer() fail");
            goto func_exit;
        }

        ////////////////////////////////////////////////
        // hdr bayer
        ret = m_makeProcessBeforeHdrBayer(map);
        if (ret != NO_ERROR) {
            CLOGE("m_makeProcessBeforeHdrBayer() fail");
            goto func_exit;
        }

        ////////////////////////////////////////////////
        // hdr yuv
        ret = m_makeProcessBeforeHdrYuv(map);
        if (ret != NO_ERROR) {
            CLOGE("m_makeProcessBeforeHdrYuv() fail");
            goto func_exit;
        }

        ////////////////////////////////////////////////
        //
        ret = m_makeProcessBeforeFlashMultiFrameDenoiseYuv(map);
        if (ret != NO_ERROR) {
            CLOGE("m_makeProcessBeforeFlashMultiFrameDenoiseYuv() fail");
            goto func_exit;
        }

        ////////////////////////////////////////////////
        // beauty face
        ret = m_makeProcessBeforeBeautyFaceYuv(map);
        if (ret != NO_ERROR) {
            CLOGE("m_makeProcessBeforeBeautyFaceYuv() fail");
            goto func_exit;
        }

        ////////////////////////////////////////////////
        // super resolution
        ret = m_makeProcessBeforeSuperResolution(map);
        if (ret != NO_ERROR) {
            CLOGE("m_makeProcessBeforeSuperResolution() fail");
            goto func_exit;
        }

        ///////////////////////////////////////////////
        // fusion
        if (scenario == PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING) {
            ret = m_makeProcessBokehFusionCapture(map);
            if (ret != NO_ERROR) {
                CLOGE("m_makeProcessBokehFusionCapture() fail");
                goto func_exit;
            }
        }

        /* ois denoise yuv */
        ret = m_makeProcessBeforeOisDenoiseYuv(map);
        if (ret != NO_ERROR) {
            CLOGE("m_makeProcessBeforeOisDenoiseYuv() fail");
            goto func_exit;
        }

        ////////////////////////////////////////////////
        // sports yuv
        ret = m_makeProcessBeforeSportsYuv(map);
        if (ret != NO_ERROR) {
            CLOGE("m_makeProcessBeforeSportsYuv() fail");
            goto func_exit;
        }

        ////////////////////////////////////////////////
        break;
    }
    case PLUGIN_CONVERT_PROCESS_AFTER:
    {
        if (frame->getMode(FRAME_MODE_DUAL_BOKEH_ANCHOR) == true) {
            ret = m_makeProcessAfterSuperNightShotBayer(map);
            if (ret != NO_ERROR) {
                CLOGE("m_makeProcessAfterSuperNightShotBayer() fail");
                goto func_exit;
            }
        } else {
            m_frameQeueue.pushProcessQ(&frame);
            if (checkLastFrameForMultiFrameCapture(frame) == true) {
                CLOGD("last frame free frameSize(%d)", m_frameQeueue.getSizeOfProcessQ());
                while (m_frameQeueue.getSizeOfProcessQ() > 0) {
                    ExynosCameraFrameSP_sptr_t oldFrame = NULL;
                    m_frameQeueue.popProcessQ(&oldFrame);

                    ret = oldFrame->getSrcBuffer(m_pipeId, &buffer);
                    if (ret < NO_ERROR) {
                        CLOGE("getBuffer fail, pipeId(%d), ret(%d), frame(%d)",
                                m_pipeId, ret, frame->getFrameCount());
                    } else {
                        CFLOGD(oldFrame, "free buffer(idx%d,%s) frameSize(%d)",
                                buffer.index, buffer.bufMgrNm,
                                m_frameQeueue.getSizeOfProcessQ());
                        bufferSupplier->putBuffer(buffer);
                    }

                    if (frame->getDualOperationMode() == DUAL_OPERATION_MODE_SYNC) {
                        ret = oldFrame->getSrcBuffer(m_pipeId, &buffer, OUTPUT_NODE_2);
                        if (ret < NO_ERROR) {
                            CLOGE("getBuffer fail, pipeId(%d), ret(%d), frame(%d)",
                                    m_pipeId, ret, frame->getFrameCount());
                        } else {
                            CFLOGD(oldFrame, "free buffer(idx%d,%s) frameSize(%d)",
                                    buffer.index, buffer.bufMgrNm,
                                    m_frameQeueue.getSizeOfProcessQ());
                            bufferSupplier->putBuffer(buffer);
                        }
                    }
                }

                if (buffer.index >=0)
                    bufferSupplier->decreaseOnDemandBuffers(buffer.tag);

#ifdef USES_COMBINE_PLUGIN_CONTROL_REQUEST
                sp<ExynosCameraVendorMetaData> vendorMeta = frame->getVendorMeta();
                if (vendorMeta != NULL) {
                    camera_metadata_entry_t entry = vendorMeta->find(SLSI_MF_STILL_CAPTURE);
                    if (entry.count > 0) {
                        uint8_t mf_still_result = entry.data.u8[0];
                        vendorMeta->update(SLSI_MF_STILL_RESULT, &mf_still_result, 1);
                        CLOGD("[R%d F%d D%d] SLSI_MF_STILL_RESULT count(%zu) result(%d)", frame->getRequestKey(), frame->getFrameCount(), frame->getMetaFrameCount(), entry.count, mf_still_result);
                    }
                }
#endif
            }
        }
        break;
    }
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

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBeforeCommon(Map_t *map, int pluginScenario)
{
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t frame = NULL;
    sp<ExynosCameraVendorMetaData> vendorMeta = NULL;
    ExynosCameraBuffer srcBuffer;
    ExynosCameraBuffer dstBuffer;

    ////////////////////////////////////////////////
    // reset scenario
    ExynosCameraPlugInConverterCombine::resetScenario(map);

    ////////////////////////////////////////////////
    // buffer index
    int frameCurIndex = 0;
    int frameMaxIndex = 0;

    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    if (frame == NULL) {
        CLOGE("frame is NULL!! pipeId(%d)", m_pipeId);
        return INVALID_OPERATION;
    }

    bool isDualMode = false;
    ExynosCameraConfigurations *configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];
    if (configurations) {
        int scenario = -1;

        isDualMode = configurations->getMode(CONFIGURATION_DUAL_MODE);
        switch (configurations->getScenario()) {
        case SCENARIO_DUAL_REAR_ZOOM:
            scenario = PLUGIN_DUAL_SCENARIO_ZOOM;
            break;
        case SCENARIO_DUAL_REAR_PORTRAIT:
        case SCENARIO_DUAL_FRONT_PORTRAIT:
            scenario = PLUGIN_DUAL_SCENARIO_BOKEH;
            break;
        default:
            break;
        }

        (*map)[PLUGIN_DUAL_MODE] = (Map_data_t)(&isDualMode);
        (*map)[PLUGIN_DUAL_SCENARIO] = (Map_data_t)(&scenario);
    }


    /* Update face info */
    const int masterNodeIndex = OUTPUT_NODE_1, slaveNodeIndex = OUTPUT_NODE_2;
    int numOfDetectedFaces = 0;
    const camera2_shot_ext *metaData[MAX_OUTPUT_NODE] = {NULL};

    metaData[masterNodeIndex] = frame->getConstMeta(masterNodeIndex);
    (*map)[PLUGIN_MASTER_FACE_RECT] = (Map_data_t)(&metaData[masterNodeIndex]->shot.dm.stats.faceRectangles);

    for (int i = 0; i < NUM_OF_DETECTED_FACES; i++) {
        if (metaData[masterNodeIndex]->shot.dm.stats.faceIds[i] <= 0) {
            continue;
         }

        numOfDetectedFaces++;
        CLOGV("faceId %d, faceScores %d, Rectangles %d, %d, %d, %d",
                metaData[masterNodeIndex]->shot.dm.stats.faceIds[i],
                metaData[masterNodeIndex]->shot.dm.stats.faceScores[i],
                metaData[masterNodeIndex]->shot.dm.stats.faceRectangles[i][0],
                metaData[masterNodeIndex]->shot.dm.stats.faceRectangles[i][1],
                metaData[masterNodeIndex]->shot.dm.stats.faceRectangles[i][2],
                metaData[masterNodeIndex]->shot.dm.stats.faceRectangles[i][3]);
     }

    if (isDualMode == true && frame->getDualOperationMode() == DUAL_OPERATION_MODE_SYNC) {
        metaData[slaveNodeIndex] = frame->getConstMeta(slaveNodeIndex);
        (*map)[PLUGIN_SLAVE_FACE_RECT] = (Map_data_t)(&metaData[slaveNodeIndex]->shot.dm.stats.faceRectangles);

        int slaveNumOfDetectedFaces = 0;
        for (int i = 0; i < NUM_OF_DETECTED_FACES; i++) {
            if (metaData[slaveNodeIndex]->shot.dm.stats.faceIds[i] <= 0) {
                continue;
            }

            slaveNumOfDetectedFaces++;
            CLOGV("Slave: faceId %d, faceScores %d, Rectangles %d, %d, %d, %d",
                metaData[slaveNodeIndex]->shot.dm.stats.faceIds[i],
                metaData[slaveNodeIndex]->shot.dm.stats.faceScores[i],
                metaData[slaveNodeIndex]->shot.dm.stats.faceRectangles[i][0],
                metaData[slaveNodeIndex]->shot.dm.stats.faceRectangles[i][1],
                metaData[slaveNodeIndex]->shot.dm.stats.faceRectangles[i][2],
                metaData[slaveNodeIndex]->shot.dm.stats.faceRectangles[i][3]);
        }

        numOfDetectedFaces = MAX(numOfDetectedFaces, slaveNumOfDetectedFaces);
    }

    (*map)[PLUGIN_FACE_NUM] = (Map_data_t)numOfDetectedFaces;


    ////////////////////////////////////////////////
    // get srcBuffer
    ret = frame->getSrcBuffer(m_pipeId, &srcBuffer);
    if (ret < 0 || srcBuffer.index < 0) {
        CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
        return ret;
    }

    ////////////////////////////////////////////////
    // get dstBuffer
    if (pluginScenario != PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING) {
        ret = frame->getDstBuffer(m_pipeId, &dstBuffer);
        if (ret < 0 || dstBuffer.index < 0) {
            CLOGE("getDstBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
            return ret;
        }
    }

    ////////////////////////////////////////////////
    // frameIndex of MFStill
    frameCurIndex = frame->getFrameIndex();
    frameMaxIndex = frame->getMaxFrameIndex();

    (*map)[PLUGIN_SRC_FRAMECOUNT] = (Map_data_t)frame->getMetaFrameCount();

    (*map)[PLUGIN_SRC_BUF_USED] = (Map_data_t)-1;
    (*map)[PLUGIN_DST_BUF_VALID] = (Map_data_t)1;

    (*map)[PLUGIN_HIFI_TOTAL_BUFFER_NUM] = (Map_data_t)frameMaxIndex;
    (*map)[PLUGIN_HIFI_CUR_BUFFER_NUM] = (Map_data_t)frameCurIndex;

#if 0
    ////////////////////////////////////////////////
    // dump
    dumpToFile("/data/camera/dump.raw", buffer.addr[0], buffer.size[0]);
#endif

    ////////////////////////////////////////////////
    // log
    CLOGD("[Common] [F%d] count(%d/%d) srcBuffer([B%d] / %d / %p) dstBuffer([B%d] / %d / %p)",
        frame->getMetaFrameCount(),
        frameMaxIndex, frameCurIndex,
        srcBuffer.index,
        srcBuffer.fd[0],
        srcBuffer.addr[0],
        dstBuffer.index,
        dstBuffer.fd[0],
        dstBuffer.addr[0]);

    return ret;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBeforeNightShotBayer(Map_t *map)
{
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t frame = NULL;
    struct camera2_shot_ext *metaData;
    ExynosCameraBuffer buffer;
    ExynosCameraConfigurations *configurations = NULL;

    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];

    ////////////////////////////////////////////////
    // check on / off
    int flagModeEnable = 0;
    bool flagOn = false;
    flagModeEnable = ExynosCameraPlugInConverterCombine::getVendorTagValue(frame, EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_BAYER);

    switch (flagModeEnable) {
    case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON:
        flagOn = true;
        break;
    case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_NONE:
    default:
        flagOn = false;
        break;
    }

    ExynosCameraPlugInConverterCombine::setVendorScenario(
        map,
        ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_NIGHT_SHOT_BAYER,
        flagOn);

    if (flagOn == false) {
        return ret;
    }

    ////////////////////////////////////////////////
    // get meta
    ret = frame->getSrcBuffer(m_pipeId, &buffer);
    if (ret < 0 || buffer.index < 0) {
        CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
        return INVALID_OPERATION;
    }

    metaData = (struct camera2_shot_ext *)buffer.addr[buffer.getMetaPlaneIndex()];

    ////////////////////////////////////////////////
    // shutter time
    Data_int64_t shutterTime = (Data_int64_t)metaData->shot.dm.sensor.exposureTime;
    (*map)[PLUGIN_EXPOSURE_TIME]            = (Map_data_t)shutterTime;

    ////////////////////////////////////////////////
    // digitalGain
    unsigned int digitalGain = metaData->shot.udm.sensor.digitalGain;
    (*map)[PLUGIN_DIGITAL_GAIN]             = (Map_data_t)digitalGain;

    ////////////////////////////////////////////////
    // Jpeg orientation
    uint32_t jpegOrientation = metaData->shot.ctl.jpeg.orientation;
    (*map)[PLUGIN_JPEG_ORIENTATION]         = (Map_data_t)jpegOrientation;

    ////////////////////////////////////////////////
    // log
    CLOGD("[NightShotBayer] [F%d] shutterTime(%lld) digitalGain(%d) jpegOrientation(%d) buffer(%d / %p)",
        frame->getMetaFrameCount(),
        shutterTime,
        digitalGain,
        jpegOrientation,
        buffer.fd[0],
        buffer.addr[0]);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBeforeNightShotYuv(Map_t *map)
{
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t frame = NULL;
    struct camera2_shot_ext *metaData;
    ExynosCameraBuffer buffer;
    ExynosCameraConfigurations *configurations = NULL;

    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];

    ////////////////////////////////////////////////
    // check on / off
    int flagModeEnable = 0;
    bool flagOn = false;
    flagModeEnable = ExynosCameraPlugInConverterCombine::getVendorTagValue(frame, EXYNOS_ANDROID_VENDOR_NIGHT_SHOT);

    switch (flagModeEnable) {
    case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON:
        flagOn = true;
        break;
    case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_NONE:
    default:
        flagOn = false;
        break;
    }

    ExynosCameraPlugInConverterCombine::setVendorScenario(
        map,
        ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_NIGHT_SHOT_YUV,
        flagOn);

    if (flagOn == false) {
        return ret;
    }

    ////////////////////////////////////////////////
    // get meta
    ret = frame->getSrcBuffer(m_pipeId, &buffer);
    if (ret < 0 || buffer.index < 0) {
        CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
        return INVALID_OPERATION;
    }

    metaData = (struct camera2_shot_ext *)buffer.addr[buffer.getMetaPlaneIndex()];

    ////////////////////////////////////////////////
    // iso
    int iso = (int)metaData->shot.dm.aa.vendor_isoValue;
    (*map)[PLUGIN_ISO]                      = (Map_data_t)iso;

    ////////////////////////////////////////////////
    // digitalGain
    unsigned int digitalGain = metaData->shot.udm.sensor.digitalGain;
    (*map)[PLUGIN_DIGITAL_GAIN]             = (Map_data_t)digitalGain;

    ////////////////////////////////////////////////
    // flashMode
    int flashMode = configurations->getModeValue(CONFIGURATION_FLASH_MODE);
    int isFlash = 1;

    switch (flashMode) {
    case FLASH_MODE_OFF:
        isFlash = 0;
        break;
    case FLASH_MODE_AUTO:
    case FLASH_MODE_ON:
    case FLASH_MODE_RED_EYE:
    case FLASH_MODE_TORCH:
    default:
        isFlash = 1;
        break;
    }

    (*map)[PLUGIN_FLASH_MODE]               = (Map_data_t)isFlash;

    ////////////////////////////////////////////////
    // camera type
    int rearOrFront = frame->getCameraId() % 2; /* facing back 0 front 1 */
    (*map)[PLUGIN_HIFI_CAMERA_TYPE]         = (Map_data_t)rearOrFront;

    ////////////////////////////////////////////////
    // llsState
    int llsState = NONE;
    (*map)[PLUGIN_LLS_INTENT] = (Map_data_t)llsState; // PLUGIN_LLS_INTENT_ENUM

    ////////////////////////////////////////////////
    // analog gain
    unsigned int analogGain = metaData->shot.udm.sensor.analogGain;
    (*map)[PLUGIN_ANALOG_GAIN]              = (Map_data_t)analogGain;

    ////////////////////////////////////////////////
    // log
    CLOGD("[NightShotYuv] [F%d] iso(%d) digitalGain(%d) flashMode(%d) facing(%d) llsState(%d) analogGain(%d) buffer(%d / %p)",
        frame->getMetaFrameCount(),
        iso,
        digitalGain,
        flashMode,
        rearOrFront,
        llsState,
        analogGain,
        buffer.fd[0],
        buffer.addr[0]);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBeforeSuperNightShotBayer(Map_t *map)
{
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t frame = NULL;
    struct camera2_shot_ext *metaData;
    ExynosCameraBuffer buffer;
    ExynosCameraConfigurations *configurations = NULL;

    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];

    ////////////////////////////////////////////////
    // check on / off
    int flagModeEnable = 0;
    bool flagOn = false;
    flagModeEnable = ExynosCameraPlugInConverterCombine::getVendorTagValue(frame, EXYNOS_ANDROID_VENDOR_SUPER_NIGHT_SHOT_BAYER);

    switch (flagModeEnable) {
    case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_ON:
        flagOn = true;
        break;
    case EXYNOS_ANDROID_VENDOR_NIGHT_SHOT_NONE:
    default:
        flagOn = false;
        break;
    }

    ExynosCameraPlugInConverterCombine::setVendorScenario(
        map,
        ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_SUPER_NIGHT_SHOT_BAYER,
        flagOn);

    if (flagOn == false) {
        return ret;
    }

    ////////////////////////////////////////////////
    // get meta
    ret = frame->getSrcBuffer(m_pipeId, &buffer);
    if (ret < 0 || buffer.index < 0) {
        CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
        return INVALID_OPERATION;
    }

    metaData = (struct camera2_shot_ext *)buffer.addr[buffer.getMetaPlaneIndex()];

    ////////////////////////////////////////////////
    // iso
    int iso = (int)metaData->shot.dm.aa.vendor_isoValue;
    (*map)[PLUGIN_ISO]                      = (Map_data_t)iso;

    ////////////////////////////////////////////////
    // shutter time
    Data_int64_t shutterTime = (Data_int64_t)metaData->shot.dm.sensor.exposureTime;
    (*map)[PLUGIN_EXPOSURE_TIME]            = (Map_data_t)shutterTime;

    ////////////////////////////////////////////////
    // digitalGain
    unsigned int digitalGain = metaData->shot.udm.sensor.digitalGain;
    (*map)[PLUGIN_DIGITAL_GAIN]             = (Map_data_t)digitalGain;

    ////////////////////////////////////////////////
    // Jpeg orientation
    uint32_t jpegOrientation = metaData->shot.ctl.jpeg.orientation;
    (*map)[PLUGIN_JPEG_ORIENTATION]         = (Map_data_t)jpegOrientation;

    (*map)[PLUGIN_ANCHOR_FRAME_INDEX]       = (Data_int32_t)-1;

    ////////////////////////////////////////////////
    // log
    CLOGD("[SuperNightShotBayer] [F%d] iso(%d) shutterTime(%lld) digitalGain(%d) jpegOrientation(%d) buffer(%p)",
        frame->getMetaFrameCount(),
        iso,
        shutterTime,
        digitalGain,
        jpegOrientation,
        buffer.fd[0],
        buffer.addr[0]);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessAfterSuperNightShotBayer(Map_t *map)
{
    status_t ret = NO_ERROR;

    if (ExynosCameraPlugInConverterCombine::getVendorScenario(
        map, ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_SUPER_NIGHT_SHOT_BAYER) == false) {
        CLOGD("skip to after processing");
        return NO_ERROR;
    }

    ExynosCameraFrameSP_sptr_t frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    if (frame->getMode(FRAME_MODE_DUAL_BOKEH_ANCHOR) == false
        || frame->getMode(FRAME_MODE_MF_STILL) == false) {
        CLOGD("skip to after processing. Anchor(%d), MultiFrmae(%d)",
                frame->getMode(FRAME_MODE_DUAL_BOKEH_ANCHOR), frame->getMode(FRAME_MODE_MF_STILL));
        return NO_ERROR;
    }

    if (checkLastFrameForMultiFrameCapture(frame) == true) {
        int anchorFrameIndex = (Data_int32_t)(*map)[PLUGIN_ANCHOR_FRAME_INDEX];
        if (anchorFrameIndex >= 0) {
            ret = frame->setBokehAnchorFrameIndex(anchorFrameIndex);
            if (ret != NO_ERROR) {
                CLOGE("[F%d T%d] fail to setBokehAnchorFrameIndex(%d)",
                    frame->getFrameCount(), frame->getFrameType(), frame->getBokehAnchorFrameIndex());
                return ret;
            }
        }
        CLOGD("[F%d T%d] BokehAnchorFrameIndex(%d)",
                frame->getFrameCount(), frame->getFrameType(), frame->getBokehAnchorFrameIndex());
    }

    return ret;
}


status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBeforeHdrBayer(Map_t *map)
{
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t frame = NULL;
    struct camera2_shot_ext *metaData;
    ExynosCameraBuffer buffer;
    ExynosCameraConfigurations *configurations = NULL;

    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];

    ////////////////////////////////////////////////
    // check on / off
    int flagModeEnable = 0;
    bool flagOn = false;
    flagModeEnable = ExynosCameraPlugInConverterCombine::getVendorTagValue(frame, EXYNOS_ANDROID_VENDOR_HDR_BAYER);

    switch (flagModeEnable) {
    case EXYNOS_ANDROID_VENDOR_HDR_ON:
        flagOn = true;
        break;
    case EXYNOS_ANDROID_VENDOR_HDR_NONE:
    default:
        flagOn = false;
        break;
    }

    ExynosCameraPlugInConverterCombine::setVendorScenario(
        map,
        ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_HDR_BAYER,
        flagOn);

    if (flagOn == false) {
        return ret;
    }

    ////////////////////////////////////////////////
    // get meta
    ret = frame->getSrcBuffer(m_pipeId, &buffer);
    if (ret < 0 || buffer.index < 0) {
        CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
        return INVALID_OPERATION;
    }

    metaData = (struct camera2_shot_ext *)buffer.addr[buffer.getMetaPlaneIndex()];

    ////////////////////////////////////////////////
    // analog gain
    unsigned int analogGain = metaData->shot.udm.sensor.analogGain;

    ////////////////////////////////////////////////
    // shutter time
    Data_int64_t shutterTime = (Data_int64_t)metaData->shot.dm.sensor.exposureTime;
    (*map)[PLUGIN_EXPOSURE_TIME]            = (Map_data_t)shutterTime;

    ////////////////////////////////////////////////
    // digitalGain
    unsigned int digitalGain = metaData->shot.udm.sensor.digitalGain;
    (*map)[PLUGIN_DIGITAL_GAIN]             = (Map_data_t)digitalGain;

    ////////////////////////////////////////////////
    // Jpeg orientation
    uint32_t jpegOrientation = metaData->shot.ctl.jpeg.orientation;
    (*map)[PLUGIN_JPEG_ORIENTATION]         = (Map_data_t)jpegOrientation;

    ////////////////////////////////////////////////
    // log
    CLOGD("[HdrBayer] [F%d] analogGain(%d) shutterTime(%lld) digitalGain(%d) jpegOrientation(%d) buffer(%d / %p)",
        frame->getMetaFrameCount(),
        analogGain,
        shutterTime,
        digitalGain,
        jpegOrientation,
        buffer.fd[0],
        buffer.addr[0]);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBeforeHdrYuv(Map_t *map)
{
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t frame = NULL;
    struct camera2_shot_ext *metaData;
    ExynosCameraBuffer buffer;
    ExynosCameraConfigurations *configurations = NULL;

    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];

    ////////////////////////////////////////////////
    // check on / off
    int flagModeEnable = 0;
    bool flagOn = false;
    flagModeEnable = ExynosCameraPlugInConverterCombine::getVendorTagValue(frame, EXYNOS_ANDROID_VENDOR_HDR_YUV);

    switch (flagModeEnable) {
    case EXYNOS_ANDROID_VENDOR_HDR_ON:
        flagOn = true;
        break;
    case EXYNOS_ANDROID_VENDOR_HDR_NONE:
    default:
        flagOn = false;
        break;
    }

    ExynosCameraPlugInConverterCombine::setVendorScenario(
        map,
        ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_HDR_YUV,
        flagOn);

    if (flagOn == false) {
        return ret;
    }

    ////////////////////////////////////////////////
    // get meta
    ret = frame->getSrcBuffer(m_pipeId, &buffer);
    if (ret < 0 || buffer.index < 0) {
        CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
        return INVALID_OPERATION;
    }

    metaData = (struct camera2_shot_ext *)buffer.addr[buffer.getMetaPlaneIndex()];

    ////////////////////////////////////////////////
    // iso
    int iso = (int)metaData->shot.dm.aa.vendor_isoValue;
    (*map)[PLUGIN_ISO]                      = (Map_data_t)iso;

    ////////////////////////////////////////////////
    // camera type
    int rearOrFront = frame->getCameraId() % 2; /* facing back 0 front 1 */
    (*map)[PLUGIN_HIFI_CAMERA_TYPE]         = (Map_data_t)rearOrFront;

    ////////////////////////////////////////////////
    // analog gain
    unsigned int analogGain = metaData->shot.udm.sensor.analogGain;
    (*map)[PLUGIN_ANALOG_GAIN]              = (Map_data_t)analogGain;

    ////////////////////////////////////////////////
    // shutter time
    Data_int64_t shutterTime = (Data_int64_t)metaData->shot.dm.sensor.exposureTime;
    (*map)[PLUGIN_EXPOSURE_TIME]            = (Map_data_t)shutterTime;

    ////////////////////////////////////////////////
    // Jpeg orientation
    uint32_t jpegOrientation = metaData->shot.ctl.jpeg.orientation;
    (*map)[PLUGIN_JPEG_ORIENTATION]         = (Map_data_t)jpegOrientation;

    ////////////////////////////////////////////////
    // ae region
    for (int i = 0; i < 4; i++) {
        m_aeRegion[i] = metaData->shot.dm.aa.aeRegions[4];
    }
    (*map)[PLUGIN_AE_REGION]                = (Map_data_t)m_aeRegion;

    ////////////////////////////////////////////////
    CLOGD("[HdrYuv] [F%d] iso(%d) facing(%d) analogGain(%d) shutterTime(%lld) jpegOrientation(%d) aeRegion(%d, %d, %d, %d) buffer(%d / %p)",
        frame->getMetaFrameCount(),
        iso,
        rearOrFront,
        analogGain,
        shutterTime,
        jpegOrientation,
        m_aeRegion[0], m_aeRegion[1], m_aeRegion[2], m_aeRegion[3],
        buffer.fd[0],
        buffer.addr[0]);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBeforeFlashMultiFrameDenoiseYuv(Map_t *map)
{
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t frame = NULL;
    struct camera2_shot_ext *metaData;
    ExynosCameraBuffer buffer;
    ExynosCameraConfigurations *configurations = NULL;

    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];

    ////////////////////////////////////////////////
    // check on / off
    int flagModeEnable = 0;
    bool flagOn = false;
    flagModeEnable = ExynosCameraPlugInConverterCombine::getVendorTagValue(frame, EXYNOS_ANDROID_VENDOR_FLASH_MULTI_FRAME_DENOISE_YUV);

    switch (flagModeEnable) {
    case EXYNOS_ANDROID_VENDOR_MODE_ON:
        flagOn = true;
        break;
    case EXYNOS_ANDROID_VENDOR_MODE_NONE:
    default:
        flagOn = false;
        break;
    }

    ExynosCameraPlugInConverterCombine::setVendorScenario(
        map,
        ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_FLASH_MULTI_FRAME_DENOISE_YUV,
        flagOn);

    if (flagOn == false) {
        return ret;
    }

    ////////////////////////////////////////////////
    // get meta
    ret = frame->getSrcBuffer(m_pipeId, &buffer);
    if (ret < 0 || buffer.index < 0) {
        CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
        return INVALID_OPERATION;
    }

    metaData = (struct camera2_shot_ext *)buffer.addr[buffer.getMetaPlaneIndex()];

    ////////////////////////////////////////////////
    // iso
    int iso = (int)metaData->shot.dm.aa.vendor_isoValue;
    (*map)[PLUGIN_ISO]                      = (Map_data_t)iso;

    ////////////////////////////////////////////////
    // digitalGain
    unsigned int digitalGain = metaData->shot.udm.sensor.digitalGain;
    (*map)[PLUGIN_DIGITAL_GAIN]             = (Map_data_t)digitalGain;

    ////////////////////////////////////////////////
    // flashMode
    int flashMode = configurations->getModeValue(CONFIGURATION_FLASH_MODE);
    int isFlash = 1;

    switch (flashMode) {
    case FLASH_MODE_OFF:
        isFlash = 0;
        break;
    case FLASH_MODE_AUTO:
    case FLASH_MODE_ON:
    case FLASH_MODE_RED_EYE:
    case FLASH_MODE_TORCH:
    default:
        isFlash = 1;
        break;
    }

    (*map)[PLUGIN_FLASH_MODE]               = (Map_data_t)isFlash;

    ////////////////////////////////////////////////
    // camera type
    int rearOrFront = frame->getCameraId() % 2; /* facing back 0 front 1 */
    (*map)[PLUGIN_HIFI_CAMERA_TYPE]         = (Map_data_t)rearOrFront;

    ////////////////////////////////////////////////
    // llsState
    int llsState = NONE;
    (*map)[PLUGIN_LLS_INTENT] = (Map_data_t)llsState; // PLUGIN_LLS_INTENT_ENUM

    ////////////////////////////////////////////////
    // analog gain
    unsigned int analogGain = metaData->shot.udm.sensor.analogGain;
    (*map)[PLUGIN_ANALOG_GAIN]              = (Map_data_t)analogGain;

    ////////////////////////////////////////////////
    // log
    CLOGD("[FlashMultiFrameDenoiseYuv] [F%d] iso(%d) digitalGain(%d) flashMode(%d) facing(%d) llsState(%d) analogGain(%d) buffer(%d / %p)",
        frame->getMetaFrameCount(),
        iso,
        digitalGain,
        flashMode,
        rearOrFront,
        llsState,
        analogGain,
        buffer.fd[0],
        buffer.addr[0]);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBeforeBeautyFaceYuv(Map_t *map)
{
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t frame = NULL;
    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];

    ////////////////////////////////////////////////
    // check on / off
    int flagModeEnable = 0;
    bool flagOn = false;
    flagModeEnable = ExynosCameraPlugInConverterCombine::getVendorTagValue(frame, EXYNOS_ANDROID_VENDOR_COMBINE_PREVIEW_PLUGIN);

    switch (flagModeEnable) {
    case EXYNOS_COMBINE_PREVIEW_PLUGIN_ENABLE:
        flagOn = true;
        break;
    case EXYNOS_COMBINE_PREVIEW_PLUGIN_DISABLE:
    default:
        flagOn = false;
        break;
    }

    ExynosCameraPlugInConverterCombine::setVendorScenario(
        map,
        ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_BEAUTY_FACE_YUV,
        flagOn);

    if (flagOn == false) {
        return ret;
    }

    ////////////////////////////////////////////////
    // log
    CLOGD("[BeautyFaceYuv] [F%d] flagModeEnable : %d",
        frame->getMetaFrameCount(),
        flagModeEnable);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBeforeSuperResolution(Map_t *map)
{
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t frame = NULL;
    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];

    ////////////////////////////////////////////////
    // check on / off
    int flagModeEnable = 0;
    bool flagOn = false;
    flagModeEnable = ExynosCameraPlugInConverterCombine::getVendorTagValue(frame, EXYNOS_ANDROID_VENDOR_CONTROL_SUPER_RESOLUTION);

    switch (flagModeEnable) {
    case EXYNOS_ANDROID_VENDOR_SUPER_RESOLUTION_ON:
        flagOn = true;
        break;
    case EXYNOS_ANDROID_VENDOR_SUPER_RESOLUTION_OFF:
    default:
        flagOn = false;
        break;
    }

    ExynosCameraPlugInConverterCombine::setVendorScenario(
        map,
        ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_SUPER_RESOLUTION,
        flagOn);

    if (flagOn == false) {
        return ret;
    }

    ////////////////////////////////////////////////
    // log
    CLOGD("[SuperResolution] [F%d] flagModeEnable : %d",
        frame->getMetaFrameCount(),
        flagModeEnable);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBokehFusionCapture(Map_t *map)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    bool enableFusionCapture = false;

    if ((frame->getFrameType() == FRAME_TYPE_REPROCESSING_DUAL_MASTER)
        || (frame->getFrameType() == FRAME_TYPE_REPROCESSING
                && frame->getDualOperationMode() == DUAL_OPERATION_MODE_MASTER)) {
        enableFusionCapture = true;
    }

    ExynosCameraPlugInConverterCombine::setVendorScenario(
        map,
        ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_BOKEH_FUSION_CAPTURE,
        enableFusionCapture);

    CLOGD("[BokehFusionCapture] [F%d] flagModeEnable : %d",
        frame->getMetaFrameCount(),
        enableFusionCapture);

    return ret;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBeforeSWRemosaicBayer(Map_t *map)
{
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t frame = NULL;
    struct camera2_shot_ext *metaData;
    ExynosCameraBuffer buffer;
    ExynosCameraConfigurations *configurations = NULL;

    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];

    ////////////////////////////////////////////////
    // check on / off
    int flagModeEnable = 0;
    bool flagOn = false;
    flagModeEnable = ExynosCameraPlugInConverterCombine::getVendorTagValue(frame, EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION);

    switch (flagModeEnable) {
    case EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_ON_SW:
        flagOn = true;
        break;
    case EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_NONE:
    case EXYNOS_ANDROID_VENDOR_REMOSAIC_FUNCTION_ON_HW:
    default:
        flagOn = false;
        break;
    }

    ExynosCameraPlugInConverterCombine::setVendorScenario(
        map,
        ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_SW_REMOSAIC,
        flagOn);

    if (flagOn == false) {
        return ret;
    }

    ////////////////////////////////////////////////
    // get meta
    ret = frame->getSrcBuffer(m_pipeId, &buffer);
    if (ret < 0 || buffer.index < 0) {
        CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
        return INVALID_OPERATION;
    }

    metaData = (struct camera2_shot_ext *)buffer.addr[buffer.getMetaPlaneIndex()];

    ////////////////////////////////////////////////
    // shutter time
    Data_int64_t shutterTime = (Data_int64_t)metaData->shot.dm.sensor.exposureTime;
    (*map)[PLUGIN_EXPOSURE_TIME]            = (Map_data_t)shutterTime;

    ////////////////////////////////////////////////
    // digitalGain
    unsigned int digitalGain = metaData->shot.udm.sensor.digitalGain;
    (*map)[PLUGIN_DIGITAL_GAIN]             = (Map_data_t)digitalGain;

    ////////////////////////////////////////////////
    // Jpeg orientation
    uint32_t jpegOrientation = metaData->shot.ctl.jpeg.orientation;
    (*map)[PLUGIN_JPEG_ORIENTATION]         = (Map_data_t)jpegOrientation;

    ////////////////////////////////////////////////
    // log
    CLOGD("[SW Remosaic] [F%d] shutterTime(%lld) digitalGain(%d) jpegOrientation(%d) buffer(%d / %p)",
        frame->getMetaFrameCount(),
        shutterTime,
        digitalGain,
        jpegOrientation,
        buffer.fd[0],
        buffer.addr[0]);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBeforeOisDenoiseYuv(Map_t *map)
{
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t frame = NULL;
    struct camera2_shot_ext *metaData;
    ExynosCameraBuffer buffer;
    ExynosCameraConfigurations *configurations = NULL;

    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];

    ////////////////////////////////////////////////
    // check on / off
    int flagModeEnable = 0;
    bool flagOn = false;
    flagModeEnable = ExynosCameraPlugInConverterCombine::getVendorTagValue(frame, EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV);

    switch (flagModeEnable) {
    case EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV_ON:
        flagOn = true;
        break;
    case EXYNOS_ANDROID_VENDOR_OIS_DENOISE_YUV_OFF:
    default:
        flagOn = false;
        break;
    }

    ExynosCameraPlugInConverterCombine::setVendorScenario(
        map,
        ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_OIS_DENOISE_YUV,
        flagOn);

    if (flagOn == false) {
        return ret;
    }

    ////////////////////////////////////////////////
    // get meta
    ret = frame->getSrcBuffer(m_pipeId, &buffer);
    if (ret < 0 || buffer.index < 0) {
        CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
        return INVALID_OPERATION;
    }

    metaData = (struct camera2_shot_ext *)buffer.addr[buffer.getMetaPlaneIndex()];

    ////////////////////////////////////////////////
    // iso
    int iso = (int)metaData->shot.dm.aa.vendor_isoValue;
    (*map)[PLUGIN_ISO]                      = (Map_data_t)iso;

    ////////////////////////////////////////////////
    // camera type
    int rearOrFront = frame->getCameraId() % 2; /* facing back 0 front 1 */
    (*map)[PLUGIN_HIFI_CAMERA_TYPE]         = (Map_data_t)rearOrFront;

    ////////////////////////////////////////////////
    // analog gain
    unsigned int analogGain = metaData->shot.udm.sensor.analogGain;
    (*map)[PLUGIN_ANALOG_GAIN]              = (Map_data_t)analogGain;

    ////////////////////////////////////////////////
    // shutter time
    Data_int64_t shutterTime = (Data_int64_t)metaData->shot.dm.sensor.exposureTime;
    (*map)[PLUGIN_EXPOSURE_TIME]            = (Map_data_t)shutterTime;

    ////////////////////////////////////////////////
    // Jpeg orientation
    uint32_t jpegOrientation = metaData->shot.ctl.jpeg.orientation;
    (*map)[PLUGIN_JPEG_ORIENTATION]         = (Map_data_t)jpegOrientation;

    ////////////////////////////////////////////////
    // ae region
    for (int i = 0; i < 4; i++) {
        m_aeRegion[i] = metaData->shot.dm.aa.aeRegions[4];
    }
    (*map)[PLUGIN_AE_REGION]                = (Map_data_t)m_aeRegion;

    ////////////////////////////////////////////////
    CLOGD("[OisDenoiseYuv] [F%d] iso(%d) facing(%d) analogGain(%d) shutterTime(%lld) jpegOrientation(%d) aeRegion(%d, %d, %d, %d) buffer(%d / %p)",
        frame->getMetaFrameCount(),
        iso,
        rearOrFront,
        analogGain,
        shutterTime,
        jpegOrientation,
        m_aeRegion[0], m_aeRegion[1], m_aeRegion[2], m_aeRegion[3],
        buffer.fd[0],
        buffer.addr[0]);

    ////////////////////////////////////////////////

    return ret;
}

status_t ExynosCameraPlugInConverterCombineReprocessing::m_makeProcessBeforeSportsYuv(Map_t *map)
{
    status_t ret = NO_ERROR;

    ExynosCameraFrameSP_sptr_t frame = NULL;
    struct camera2_shot_ext *metaData;
    ExynosCameraBuffer buffer;
    ExynosCameraConfigurations *configurations = NULL;

    frame = (ExynosCameraFrame *)(*map)[PLUGIN_CONVERT_FRAME];
    configurations = (ExynosCameraConfigurations *)(*map)[PLUGIN_CONVERT_CONFIGURATIONS];

    ////////////////////////////////////////////////
    // check on / off
    int flagModeEnable = 0;
    bool flagOn = false;
    flagModeEnable = ExynosCameraPlugInConverterCombine::getVendorTagValue(frame, EXYNOS_ANDROID_VENDOR_SPORTS_YUV);

    switch (flagModeEnable) {
    case EXYNOS_ANDROID_VENDOR_MODE_ON:
        flagOn = true;
        break;
    case EXYNOS_ANDROID_VENDOR_MODE_NONE:
    default:
        flagOn = false;
        break;
    }

    ExynosCameraPlugInConverterCombine::setVendorScenario(
        map,
        ExynosCameraPlugInConverterCombine::VENDOR_SCENARIO_SPORTS_YUV,
        flagOn);

    if (flagOn == false) {
        return ret;
    }

    ////////////////////////////////////////////////
    // get meta
    ret = frame->getSrcBuffer(m_pipeId, &buffer);
    if (ret < 0 || buffer.index < 0) {
        CLOGE("getSrcBuffer fail, pipeId(%d), ret(%d)", m_pipeId, ret);
        return INVALID_OPERATION;
    }

    metaData = (struct camera2_shot_ext *)buffer.addr[buffer.getMetaPlaneIndex()];

    ////////////////////////////////////////////////
    // iso
    int iso = (int)metaData->shot.dm.aa.vendor_isoValue;
    (*map)[PLUGIN_ISO]                      = (Map_data_t)iso;

    ////////////////////////////////////////////////
    // digitalGain
    unsigned int digitalGain = metaData->shot.udm.sensor.digitalGain;
    (*map)[PLUGIN_DIGITAL_GAIN]             = (Map_data_t)digitalGain;

    ////////////////////////////////////////////////
    // flashMode
    int flashMode = configurations->getModeValue(CONFIGURATION_FLASH_MODE);
    int isFlash = 1;

    switch (flashMode) {
    case FLASH_MODE_OFF:
        isFlash = 0;
        break;
    case FLASH_MODE_AUTO:
    case FLASH_MODE_ON:
    case FLASH_MODE_RED_EYE:
    case FLASH_MODE_TORCH:
    default:
        isFlash = 1;
        break;
    }

    (*map)[PLUGIN_FLASH_MODE]               = (Map_data_t)isFlash;

    ////////////////////////////////////////////////
    // camera type
    int rearOrFront = frame->getCameraId() % 2; /* facing back 0 front 1 */
    (*map)[PLUGIN_HIFI_CAMERA_TYPE]         = (Map_data_t)rearOrFront;

    ////////////////////////////////////////////////
    // llsState
    int llsState = NONE;
    (*map)[PLUGIN_LLS_INTENT] = (Map_data_t)llsState; // PLUGIN_LLS_INTENT_ENUM

    ////////////////////////////////////////////////
    // analog gain
    unsigned int analogGain = metaData->shot.udm.sensor.analogGain;
    (*map)[PLUGIN_ANALOG_GAIN]              = (Map_data_t)analogGain;

    ////////////////////////////////////////////////
    // gyro
    m_gyro[0] = metaData->shot.uctl.aaUd.gyroInfo.x;
    m_gyro[1] = metaData->shot.uctl.aaUd.gyroInfo.y;
    m_gyro[2] = metaData->shot.uctl.aaUd.gyroInfo.z;
    (*map)[PLUGIN_GYRO_FLOAT_ARRAY]         = (Map_data_t)m_gyro;

    ////////////////////////////////////////////////
    // aecSettled
    int aecSettled = getVendorMetaAecSettled(metaData);
    (*map)[PLUGIN_AEC_SETTLED] = (Map_data_t)aecSettled;

    ////////////////////////////////////////////////
    // log
    CLOGD("[SportsYuv] [F%d] iso(%d) digitalGain(%d) flashMode(%d) facing(%d) llsState(%d) analogGain(%d) m_gyro(%.2f, %.2f, %.2f) aecSettled(%d) buffer(%d / %p)",
        frame->getMetaFrameCount(),
        iso,
        digitalGain,
        flashMode,
        rearOrFront,
        llsState,
        analogGain,
        m_gyro[0],
        m_gyro[1],
        m_gyro[2],
        aecSettled,
        buffer.fd[0],
        buffer.addr[0]);

    ////////////////////////////////////////////////

    return ret;
}

}; /* namespace android */
