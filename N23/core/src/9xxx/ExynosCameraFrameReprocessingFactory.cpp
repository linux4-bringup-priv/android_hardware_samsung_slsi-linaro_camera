/*
**
** Copyright 2017, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraFrameReprocessingFactory"
#include <log/log.h>

#include "ExynosCameraFrameReprocessingFactory.h"

namespace android {

ExynosCameraFrameReprocessingFactory::~ExynosCameraFrameReprocessingFactory()
{
    status_t ret = NO_ERROR;

    if (m_shot_ext != NULL) {
        delete m_shot_ext;
        m_shot_ext = NULL;
    }

    ret = destroy();
    if (ret != NO_ERROR)
        CLOGE("destroy fail");
}

status_t ExynosCameraFrameReprocessingFactory::create(void)
{
    Mutex::Autolock lock(ExynosCameraStreamMutex::getInstance()->getStreamMutex());
    CLOGI("");

    status_t ret = NO_ERROR;

    ret = ExynosCameraFrameFactoryBase::create();
    if (ret != NO_ERROR) {
        CLOGE("Pipe create fail, ret(%d)", ret);
        return ret;
    }

    /* EOS */
    ret = m_pipes[INDEX(PIPE_3AA_REPROCESSING)]->setControl(V4L2_CID_IS_END_OF_STREAM, 1);
    if (ret != NO_ERROR) {
        CLOGE("PIPE_%s V4L2_CID_IS_END_OF_STREAM fail, ret(%d)",
                 m_pipes[INDEX(PIPE_3AA_REPROCESSING)]->getPipeName(), ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    ret = m_transitState(FRAME_FACTORY_STATE_CREATE);

    return ret;
}

status_t ExynosCameraFrameReprocessingFactory::initPipes(void)
{
    CLOGI("");

    status_t ret = NO_ERROR;
    camera_pipe_info_t pipeInfo[MAX_NODE];
    camera_pipe_info_t nullPipeInfo;

    int pipeId = -1;
    enum NODE_TYPE nodeType = INVALID_NODE;
    enum NODE_TYPE leaderNodeType = OUTPUT_NODE;

    int32_t nodeNums[MAX_NODE];
    int32_t sensorIds[MAX_NODE];
    int32_t secondarySensorIds[MAX_NODE];
    for (int i = 0; i < MAX_NODE; i++) {
        nodeNums[i] = -1;
        sensorIds[i] = -1;
        secondarySensorIds[i] = -1;
    }

    ExynosRect tempRect;
    ExynosRect bnsSize;
    ExynosRect bayerCropSize;
    int hwSensorW = 0, hwSensorH = 0;
    int maxPreviewW = 0, maxPreviewH = 0, hwPreviewW = 0, hwPreviewH = 0;
    int maxPictureW = 0, maxPictureH = 0, hwPictureW = 0, hwPictureH = 0;
    int maxThumbnailW = 0, maxThumbnailH = 0;
    int yuvWidth[ExynosCameraParameters::YUV_MAX] = {0};
    int yuvHeight[ExynosCameraParameters::YUV_MAX] = {0};
    int bayerFormat = m_parameters->getBayerFormat(PIPE_3AA_REPROCESSING);
    camera_pixel_size yuvPixelSize[ExynosCameraParameters::YUV_MAX] = {CAMERA_PIXEL_SIZE_8BIT};
    int yuvFormat[ExynosCameraParameters::YUV_MAX] = {0};
    int dsWidth = MAX_VRA_INPUT_WIDTH;
    int dsHeight = MAX_VRA_INPUT_HEIGHT;
    int dsFormat = m_parameters->getHwVraInputFormat();
    int pictureFormat = m_parameters->getHwPictureFormat();
    camera_pixel_size picturePixelSize = m_parameters->getHwPicturePixelSize();
    struct ExynosConfigInfo *config = m_configurations->getConfig();
    int perFramePos = 0;
    int yuvIndex = -1;
    bool supportJpeg = false;

    memset(&nullPipeInfo, 0, sizeof(camera_pipe_info_t));


    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING) {
        m_parameters->getSize(HW_INFO_HW_REMOSAIC_SENSOR_SIZE, (uint32_t *)&hwSensorW, (uint32_t *)&hwSensorH);
    } else {
        m_parameters->getSize(HW_INFO_HW_SENSOR_SIZE, (uint32_t *)&hwSensorW, (uint32_t *)&hwSensorH);
    }
    m_parameters->getSize(HW_INFO_MAX_PREVIEW_SIZE, (uint32_t *)&maxPreviewW, (uint32_t *)&maxPreviewH);
    m_parameters->getSize(HW_INFO_HW_PREVIEW_SIZE, (uint32_t *)&hwPreviewW, (uint32_t *)&hwPreviewH);
#ifdef SUPPORT_MULTI_STREAM_CAPTURE
    if (m_configurations->getScenario() == SCENARIO_DUAL_REAR_ZOOM) {
        m_configurations->getSize(CONFIGURATION_MAX_PICTURE_SIZE_OF_MULTISTREAM, (uint32_t *)&maxPictureW, (uint32_t *)&maxPictureH);
        hwPictureW = maxPictureW;
        hwPictureH = maxPictureH;
    } else
#endif
    {
        m_parameters->getSize(HW_INFO_MAX_PICTURE_SIZE, (uint32_t *)&maxPictureW, (uint32_t *)&maxPictureH);
        m_parameters->getSize(HW_INFO_HW_MAX_PICTURE_SIZE, (uint32_t *)&hwPictureW, (uint32_t *)&hwPictureH);
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
        // remosaic capture will be available in this configureStream
        if (m_configurations->isSupportedFunction(SUPPORTED_FUNCTION_REMOSAIC_BY_SESSION) == true) {
            // if remosaic mode is available, setFmt size is always smaller than full size
            // setFmt : 12M, perframeControl : 48M
            m_configurations->getSize(CONFIGURATION_MAX_NOT_REMOSAIC_SENSOR_SIZE, (uint32_t *)&maxPreviewW, (uint32_t *)&maxPreviewH);
            m_configurations->getSize(CONFIGURATION_MAX_NOT_REMOSAIC_SENSOR_SIZE, (uint32_t *)&maxPictureW, (uint32_t *)&maxPictureH);
            m_configurations->getSize(CONFIGURATION_MAX_NOT_REMOSAIC_SENSOR_SIZE, (uint32_t *)&hwPictureW, (uint32_t *)&hwPictureH);
        }
#endif
    }
    m_parameters->getSize(HW_INFO_MAX_THUMBNAIL_SIZE, (uint32_t *)&maxThumbnailW, (uint32_t *)&maxThumbnailH);
    m_parameters->getPreviewBayerCropSize(&bnsSize, &bayerCropSize, false);

    CLOGI(" MaxPreviewSize(%dx%d), HwPreviewSize(%dx%d)", maxPreviewW, maxPreviewH, hwPreviewW, hwPreviewH);
    CLOGI(" MaxPixtureSize(%dx%d), HwPixtureSize(%dx%d)", maxPictureW, maxPictureH, hwPictureW, hwPictureH);
    CLOGI(" MaxThumbnailSize(%dx%d), HwSensor(%dx%d)", maxThumbnailW, maxThumbnailH, hwSensorW, hwSensorH);
    CLOGI(" PreviewBayerCropSize(%dx%d)", bayerCropSize.w, bayerCropSize.h);
    CLOGI("DS Size %dx%d Format %x Buffer count %d",
            dsWidth, dsHeight, dsFormat, config->current->bufInfo.num_vra_buffers);

    for (int i = ExynosCameraParameters::YUV_STALL_0; i < ExynosCameraParameters::YUV_STALL_MAX; i++) {
        yuvIndex = i % ExynosCameraParameters::YUV_MAX;
        m_parameters->getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)&yuvWidth[yuvIndex], (uint32_t *)&yuvHeight[yuvIndex], i);
        yuvFormat[yuvIndex] = m_configurations->getYuvFormat(i);
        yuvPixelSize[yuvIndex] = m_configurations->getYuvPixelSize(i);

        if (m_configurations->getModeValue(CONFIGURATION_YUV_STALL_PORT) == yuvIndex) {
            if (yuvWidth[yuvIndex] == 0 && yuvHeight[yuvIndex] == 0) {
#ifdef SUPPORT_MULTI_STREAM_CAPTURE
                if (m_configurations->getScenario() == SCENARIO_DUAL_REAR_ZOOM) {
                    m_configurations->getSize(CONFIGURATION_MAX_PICTURE_SIZE_OF_MULTISTREAM,
                                              (uint32_t *)&yuvWidth[yuvIndex], (uint32_t *)&yuvHeight[yuvIndex]);
                } else
#endif
                {
                    m_parameters->getSize(HW_INFO_HW_MAX_PICTURE_SIZE, (uint32_t *)&yuvWidth[yuvIndex], (uint32_t *)&yuvHeight[yuvIndex]);
                }
                yuvFormat[yuvIndex] = V4L2_PIX_FMT_NV21;
            }
        }
        CLOGI("YUV_STALL[%d] Size %dx%d Format %x PixelSizeNum %d",
                i, yuvWidth[yuvIndex], yuvHeight[yuvIndex], yuvFormat[yuvIndex], yuvPixelSize[yuvIndex]);
    }

#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    if (m_parameters->getNumOfMcscOutputPorts() == 3 || m_parameters->getNumOfMcscOutputPorts() == 1) {
        for (int i = ExynosCameraParameters::YUV_STALL_0; i < ExynosCameraParameters::YUV_STALL_MAX; i++) {
            yuvIndex = i % ExynosCameraParameters::YUV_MAX;
            yuvWidth[yuvIndex] = VIRTUALFD_SIZE_W;
            yuvHeight[yuvIndex] = VIRTUALFD_SIZE_H;
            yuvFormat[yuvIndex] = pictureFormat;
        }
    }
#endif

    /*
     * 3AA for Reprocessing
     */
    if (m_supportPureBayerReprocessing == true) {
        pipeId = PIPE_3AA_REPROCESSING;
#ifdef USE_PAF
        if (m_flagPaf3aaOTF == HW_CONNECTION_MODE_OTF) {
            /* 3PAF */

            nodeType = getNodeType(PIPE_PAF_REPROCESSING);
            bayerFormat = m_parameters->getBayerFormat(PIPE_3AA_REPROCESSING);

            /* set v4l2 buffer size */
            tempRect.fullW = hwSensorW;
            tempRect.fullH = hwSensorH;
            tempRect.colorFormat = bayerFormat;

            /* set v4l2 video node buffer count */
            pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;
            pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat);

            /* Set output node default info */
            SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_PURE_REPROCESSING_3AA);
        }
#endif

        /* 3AS */
        nodeType = getNodeType(PIPE_3AA_REPROCESSING);
        bayerFormat = m_parameters->getBayerFormat(PIPE_3AA_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = hwSensorW;
        tempRect.fullH = hwSensorH;
        tempRect.colorFormat = bayerFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;
        pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat);

        if (m_flagPaf3aaOTF == HW_CONNECTION_MODE_OTF) {
            SET_CAPTURE_DEVICE_BASIC_INFO();
        } else {
            /* Set output node default info */
            SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_PURE_REPROCESSING_3AA);
        }

        /* 3AC */
        nodeType = getNodeType(PIPE_3AC_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_3AC_POS;
#ifdef USE_3AG_CAPTURE
        bayerFormat = m_parameters->getBayerFormat(PIPE_3AG_REPROCESSING);
#else
        bayerFormat = m_parameters->getBayerFormat(PIPE_3AC_REPROCESSING);
#endif

        /* set v4l2 buffer size */
        tempRect.fullW = hwSensorW;
        tempRect.fullH = hwSensorH;
        tempRect.colorFormat = bayerFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;
        pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat);

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        /* 3AP */
        nodeType = getNodeType(PIPE_3AP_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_3AP_POS;
        bayerFormat = m_parameters->getBayerFormat(PIPE_3AP_REPROCESSING);

        /* set v4l2 buffer size */
        if (m_configurations->getDynamicMode(DYNAMIC_HIGHSPEED_RECORDING_MODE)) {
            tempRect.fullW = hwSensorW;
            tempRect.fullH = hwSensorH;
        } else {
            if (maxPictureW > hwSensorW || maxPictureH > hwSensorH) {
                //3AP reprocessing size should be less than or equal to hwSensor size.
                tempRect.fullW = hwSensorW;
                tempRect.fullH = hwSensorH;
            } else {
                tempRect.fullW = maxPictureW;
                tempRect.fullH = maxPictureH;
            }
        }
        tempRect.colorFormat = bayerFormat;

        /* set v4l2 video node bytes per plane */
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);
        pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat);

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_reprocessing_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

#ifdef SUPPORT_3AF
        /* 3AF */
        nodeType = getNodeType(PIPE_3AF_REPROCESSING);
        perFramePos = 0; //it is dummy info
        // TODO: need to remove perFramePos from the SET_CAPTURE_DEVICE_BASIC_INFO

        /* set v4l2 buffer size */
        tempRect.fullW = MAX_VRA_INPUT_WIDTH;
        tempRect.fullH = MAX_VRA_INPUT_HEIGHT;
        tempRect.colorFormat = m_parameters->getHW3AFdFormat();

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_vra_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();
#endif

        if (m_parameters->isUse3aaDNG()) {
            /* 3AG */
            nodeType = getNodeType(PIPE_3AG_REPROCESSING);
            perFramePos = 0;
            // TODO: need to remove perFramePos from the SET_CAPTURE_DEVICE_BASIC_INFO
            bayerFormat = m_parameters->getBayerFormat(PIPE_3AG_REPROCESSING);

            /* set v4l2 buffer size */
            tempRect.fullW = hwPictureW;
            tempRect.fullH = hwPictureH;
            tempRect.colorFormat = bayerFormat;

            /* set v4l2 video node bytes per plane */
            pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);

            /* set v4l2 video node buffer count */
            pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

            /* Set capture node default info */
            SET_CAPTURE_DEVICE_BASIC_INFO();
        }

        /* setup pipe info to 3AA pipe */
        if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
            ret = m_pipes[INDEX(pipeId)]->setupPipe(pipeInfo, m_sensorIds[INDEX(pipeId)]);
            if (ret != NO_ERROR) {
                CLOGE("3AA setupPipe fail, ret(%d)", ret);
                /* TODO: exception handling */
                return INVALID_OPERATION;
            }

            /* clear pipeInfo for next setupPipe */
            for (int i = 0; i < MAX_NODE; i++)
                pipeInfo[i] = nullPipeInfo;
        }
    } else {
        /*
        * 3A video node is opened for dirty bayer.
        * So, we have to do setinput to 3A video node.
        */
        pipeId = PIPE_3AA_REPROCESSING;

        /* setup pipe info to 3AA pipe */
        ret = m_pipes[INDEX(pipeId)]->setupPipe(pipeInfo, m_sensorIds[INDEX(pipeId)]);
        if (ret != NO_ERROR) {
            CLOGE("3AA setupPipe for dirty bayer reprocessing  fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;
    }


    /*
     * ISP for Reprocessing
     */

    /* ISP */
    if (m_supportPureBayerReprocessing == false
        || m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_ISP_REPROCESSING;
        nodeType = getNodeType(PIPE_ISP_REPROCESSING);
        bayerFormat = m_parameters->getBayerFormat(PIPE_ISP_REPROCESSING);

        /* set v4l2 buffer size */
        if (m_supportPureBayerReprocessing == true) {
            if (m_configurations->getDynamicMode(DYNAMIC_HIGHSPEED_RECORDING_MODE)) {
                tempRect.fullW = hwSensorW;
                tempRect.fullH = hwSensorH;
            } else {
                if (maxPictureW > hwSensorW || maxPictureH > hwSensorH) {
                    //ISP reprocessing size should be less than or equal to hwSensor size.
                    tempRect.fullW = hwSensorW;
                    tempRect.fullH = hwSensorH;
                } else {
                    tempRect.fullW = maxPictureW;
                    tempRect.fullH = maxPictureH;
                }
            }
        } else {
            // Dirtybayer input is bCrop size
            tempRect.fullW = bayerCropSize.w;
            tempRect.fullH = bayerCropSize.h;
        }
        tempRect.colorFormat = bayerFormat;

        /* set v4l2 video node bytes per plane */
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);
        pipeInfo[nodeType].pixelSize = getPixelSizeFromBayerFormat(bayerFormat);

        /* set v4l2 video node buffer count */
        if(m_supportPureBayerReprocessing) {
            pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_reprocessing_buffers;
        } else if (m_parameters->isSupportZSLInput()) {
            pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;
        } else {
            pipeInfo[nodeType].bufInfo.count = m_configurations->maxNumOfSensorBuffer();
        }

        /* Set output node default info */
        int ispPerframeInfoIndex = m_supportPureBayerReprocessing ? PERFRAME_INFO_PURE_REPROCESSING_ISP : PERFRAME_INFO_DIRTY_REPROCESSING_ISP;
        SET_OUTPUT_DEVICE_BASIC_INFO(ispPerframeInfoIndex);
    }

    /* setup pipe info to ISP pipe */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        /* ISPC */
        nodeType = getNodeType(PIPE_ISPC_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_ISPC_POS;

    /* set v4l2 buffer size */
        if (m_configurations->getDynamicMode(DYNAMIC_HIGHSPEED_RECORDING_MODE)) {
            tempRect.fullW = hwSensorW;
            tempRect.fullH = hwSensorH;
        } else {
            tempRect.fullW = maxPictureW;
            tempRect.fullH = maxPictureH;
        }
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = picturePixelSize;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_hwdis_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        /* ISPP */
        nodeType = getNodeType(PIPE_ISPP_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_ISPP_POS;

        /* set v4l2 buffer size */
        if (m_configurations->getDynamicMode(DYNAMIC_HIGHSPEED_RECORDING_MODE)) {
            tempRect.fullW = hwSensorW;
            tempRect.fullH = hwSensorH;
        } else {
            tempRect.fullW = maxPictureW;
            tempRect.fullH = maxPictureH;
        }
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = picturePixelSize;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_hwdis_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        ret = m_pipes[INDEX(pipeId)]->setupPipe(pipeInfo, m_sensorIds[INDEX(pipeId)]);
        if (ret != NO_ERROR) {
            CLOGE("ISP setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;
    }

    /*
     * MCSC for Reprocessing
     */

    /* MCSC */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_MCSC_REPROCESSING;
        nodeType = getNodeType(PIPE_MCSC_REPROCESSING);

        /* set v4l2 buffer size */
        if (m_configurations->getDynamicMode(DYNAMIC_HIGHSPEED_RECORDING_MODE)) {
            tempRect.fullW = hwSensorW;
            tempRect.fullH = hwSensorH;
        } else {
            tempRect.fullW = maxPictureW;
            tempRect.fullH = maxPictureH;
        }
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = picturePixelSize;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_hwdis_buffers;

        /* Set output node default info */
        int mcscPerframeInfoIndex = m_supportPureBayerReprocessing ? PERFRAME_INFO_PURE_REPROCESSING_MCSC : PERFRAME_INFO_DIRTY_REPROCESSING_MCSC;
        SET_OUTPUT_DEVICE_BASIC_INFO(mcscPerframeInfoIndex);
    }

    switch (m_parameters->getNumOfMcscOutputPorts()) {
    case 5:
        /*
         * If the number of output was lower than 5,
         * try to use another reprocessing factory for jpeg
         * to support full device
         */
        /* MCSC2 */
        nodeType = getNodeType(PIPE_MCSC2_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_MCSC2_POS;
        yuvIndex = ExynosCameraParameters::YUV_STALL_2 % ExynosCameraParameters::YUV_MAX;

        /* set v4l2 buffer size */
        tempRect.fullW = yuvWidth[yuvIndex];
        tempRect.fullH = yuvHeight[yuvIndex];
        tempRect.colorFormat = yuvFormat[yuvIndex];

        /* set v4l2 format pixel size */
        pipeInfo[nodeType].pixelSize = yuvPixelSize[yuvIndex];

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;
        /* pipeInfo[nodeType].bytesPerPlane[0] = tempRect.fullW; */

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        /* MCSC1 */
        nodeType = getNodeType(PIPE_MCSC1_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_MCSC1_POS;
        yuvIndex = ExynosCameraParameters::YUV_STALL_1 % ExynosCameraParameters::YUV_MAX;

        /* set v4l2 buffer size */
        tempRect.fullW = yuvWidth[yuvIndex];
        tempRect.fullH = yuvHeight[yuvIndex];
        tempRect.colorFormat = yuvFormat[yuvIndex];

        /* set v4l2 format pixel size */
        pipeInfo[nodeType].pixelSize = yuvPixelSize[yuvIndex];

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;
        /* pipeInfo[nodeType].bytesPerPlane[0] = tempRect.fullW; */

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        /* Not break; */
    case 3:

        supportJpeg = true;

        /* Jpeg Thumbnail */
        nodeType = getNodeType(PIPE_MCSC_THUMB_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_MCSC_THUMB_POS;

        /* set v4l2 buffer size */
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        tempRect.fullW = VIRTUALFD_SIZE_W;
        tempRect.fullH = VIRTUALFD_SIZE_H;
#else
        tempRect.fullW = maxThumbnailW;
        tempRect.fullH = maxThumbnailH;
#endif
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = picturePixelSize;

        /* set v4l2 video node buffer count */
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;
#else
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_reprocessing_buffers;
#endif

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        /* Jpeg Main */
        nodeType = getNodeType(PIPE_MCSC_JPEG_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_MCSC_JPEG_POS;

        /* set v4l2 buffer size */
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        tempRect.fullW = VIRTUALFD_SIZE_W;
        tempRect.fullH = VIRTUALFD_SIZE_H;
#else
        tempRect.fullW = hwPictureW;
        tempRect.fullH = hwPictureH;
#endif
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = picturePixelSize;

        /* set v4l2 video node buffer count */
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;
#else
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_reprocessing_buffers;
#endif

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        /* Not break; */
    case 1:
        /* MCSC0 */
        nodeType = getNodeType(PIPE_MCSC0_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_MCSC0_POS;
        yuvIndex = ExynosCameraParameters::YUV_STALL_0 % ExynosCameraParameters::YUV_MAX;

        /* set v4l2 buffer size */
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        tempRect.fullW = VIRTUALFD_SIZE_W;
        tempRect.fullH = VIRTUALFD_SIZE_H;
#else
        tempRect.fullW = yuvWidth[yuvIndex];
        tempRect.fullH = yuvHeight[yuvIndex];
#endif
        tempRect.colorFormat = yuvFormat[yuvIndex];

        /* set v4l2 format pixel size */
        pipeInfo[nodeType].pixelSize = yuvPixelSize[yuvIndex];

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;
        /* pipeInfo[nodeType].bytesPerPlane[0] = tempRect.fullW; */

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();
        break;
    default:
        CLOG_ASSERT("invalid MCSC output(%d)", m_parameters->getNumOfMcscOutputPorts());
        break;
    }

    if (supportJpeg == true && m_flagHWFCEnabled == true) {
#ifdef SUPPORT_HWFC_SERIALIZATION
        /* Do serialized Q/DQ operation to guarantee the H/W flow control sequence limitation */
        m_pipes[INDEX(pipeId)]->needSerialization(true);
#endif

        /* JPEG Src */
        nodeType = getNodeType(PIPE_HWFC_JPEG_SRC_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = hwPictureW;
        tempRect.fullH = hwPictureH;
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = picturePixelSize;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_reprocessing_buffers;

        /* Set capture node default info */
        pipeInfo[nodeType].rectInfo = tempRect;
        pipeInfo[nodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        pipeInfo[nodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;

        /* Thumbnail Src */
        nodeType = getNodeType(PIPE_HWFC_THUMB_SRC_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = maxThumbnailW;
        tempRect.fullH = maxThumbnailH;
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = picturePixelSize;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_reprocessing_buffers;

        /* Set capture node default info */
        pipeInfo[nodeType].rectInfo = tempRect;
        pipeInfo[nodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        pipeInfo[nodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;

        /* JPEG Dst */
        nodeType = getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = hwPictureW;
        tempRect.fullH = hwPictureH;
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = picturePixelSize;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

        /* Set capture node default info */
        pipeInfo[nodeType].rectInfo = tempRect;
        pipeInfo[nodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        pipeInfo[nodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;

        /* Thumbnail Dst */
        nodeType = getNodeType(PIPE_HWFC_THUMB_DST_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = maxThumbnailW;
        tempRect.fullH = maxThumbnailH;
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = picturePixelSize;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_reprocessing_buffers;

        /* Set capture node default info */
        pipeInfo[nodeType].rectInfo = tempRect;
        pipeInfo[nodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        pipeInfo[nodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
    }

#ifdef SUPPORT_VIRTUALFD_REPROCESSING
#if defined(NUM_OF_M2M_MCSC_OUTPUT_PORTS) && (NUM_OF_M2M_MCSC_OUTPUT_PORTS == 1)
    ret = m_pipes[INDEX(pipeId)]->setShreadNode(getNodeType(PIPE_MCSC0_REPROCESSING), getNodeType(PIPE_MCSC_JPEG_REPROCESSING));
    ret = m_pipes[INDEX(pipeId)]->setShreadNode(getNodeType(PIPE_MCSC0_REPROCESSING), getNodeType(PIPE_MCSC_THUMB_REPROCESSING));
#else
    ret = m_pipes[INDEX(pipeId)]->setShreadNode(getNodeType(PIPE_MCSC_JPEG_REPROCESSING), getNodeType(PIPE_MCSC1_REPROCESSING));
    ret = m_pipes[INDEX(pipeId)]->setShreadNode(getNodeType(PIPE_MCSC_THUMB_REPROCESSING), getNodeType(PIPE_MCSC2_REPROCESSING));
#endif
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
    ret = m_pipes[INDEX(pipeId)]->setShreadNode(getNodeType(PIPE_MCSC_JPEG_REPROCESSING), getNodeType(PIPE_MCSC_PP_REPROCESSING));
#endif
    frame_queue_t *frameQ = NULL;
    ExynosCameraNode **nodes = NULL;
    sp<ReorderMCSCHandler> handler = new ReorderMCSCHandler(getCameraId(), pipeId, m_configurations, m_parameters, PIPE_HANDLER::SCENARIO_REORDER);
    handler->setName("MCSC Reorder Handler");
    handler->addUsage(PIPE_HANDLER::USAGE_PRE_PUSH_FRAMEQ);
    handler->addUsage(PIPE_HANDLER::USAGE_PRE_QBUF);
    handler->addUsage(PIPE_HANDLER::USAGE_POST_DQBUF);

    m_pipes[INDEX(pipeId)]->getInputFrameQ(&frameQ);
    handler->setInputFrameQ(frameQ);
    m_pipes[INDEX(pipeId)]->getOutputFrameQ(&frameQ);
    handler->setOutputFrameQ(frameQ);
    m_pipes[INDEX(pipeId)]->getFrameDoneQ(&frameQ);
    handler->setFrameDoneQ(frameQ);
    handler->setPipeInfo(m_deviceInfo[INDEX(pipeId)].pipeId, m_deviceInfo[INDEX(pipeId)].virtualNodeNum);
    handler->setPerframeIndex(pipeInfo[OUTPUT_NODE].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex);
    handler->setBufferSupplier(m_bufferSupplier);

    m_pipes[INDEX(pipeId)]->getNodes(&nodes);
    handler->setNodes(&nodes);

    m_pipes[INDEX(pipeId)]->setHandler(handler);
#endif

#ifdef USE_VRA_FD
    if (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) {
        /* MCSC5 */
        nodeType = getNodeType(PIPE_MCSC5_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_MCSC5_POS;

        /* set v4l2 buffer size */
        tempRect.fullW = dsWidth;
        tempRect.fullH = dsHeight;
        tempRect.colorFormat = dsFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_vra_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        ret = m_pipes[INDEX(pipeId)]->setupPipe(pipeInfo, m_sensorIds[INDEX(pipeId)]);
        if (ret != NO_ERROR) {
            CLOGE("MCSC setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;

        pipeId = PIPE_VRA_REPROCESSING;
        nodeType = getNodeType(PIPE_VRA_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = dsWidth;
        tempRect.fullH = dsHeight;
        tempRect.colorFormat = dsFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_vra_buffers;

        /* Set output node default info */
        SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_VRA);
    }
#endif

    ret = m_pipes[INDEX(pipeId)]->setupPipe(pipeInfo, m_sensorIds[INDEX(pipeId)]);
    if (ret != NO_ERROR) {
        CLOGE("ISP setupPipe fail, ret(%d)", ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

#ifdef USE_VRA_FD
    /* VRA */
    if ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) &&
        (m_flagMcscVraOTF != HW_CONNECTION_MODE_M2M)) {
        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;

        pipeId = PIPE_VRA_REPROCESSING;
        nodeType = getNodeType(PIPE_VRA_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = MAX_VRA_INPUT_WIDTH;
        tempRect.fullH = MAX_VRA_INPUT_HEIGHT;
        tempRect.colorFormat = dsFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_vra_buffers;

        /* Set output node default info */
        SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_VRA);

        ret = m_pipes[INDEX(pipeId)]->setupPipe(pipeInfo, m_sensorIds[INDEX(pipeId)]);
        if (ret != NO_ERROR) {
            CLOGE("MCSC setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

#ifdef USE_CLAHE_REPROCESSING
    /*
     * CLAHE for Reprocessing
     */

    /* CLAHE */
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_CLAHE_REPROCESSING;
        nodeType = getNodeType(PIPE_CLAHE_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = yuvWidth[yuvIndex];
        tempRect.fullH = yuvHeight[yuvIndex];
        tempRect.colorFormat = yuvFormat[yuvIndex];

        /* set v4l2 format pixel size */
        pipeInfo[nodeType].pixelSize = yuvPixelSize[yuvIndex];

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_reprocessing_buffers;

        /* Set output node default info */
        SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_CLAHE);
    }

    /* setup pipe info to CLAHE pipe */
    /* CLAHEC */
    nodeType = getNodeType(PIPE_CLAHEC_REPROCESSING);

    /* set v4l2 buffer size */
    tempRect.fullW = yuvWidth[yuvIndex];
    tempRect.fullH = yuvHeight[yuvIndex];
    tempRect.colorFormat = yuvFormat[yuvIndex];

    /* set v4l2 format pixel size */
    pipeInfo[nodeType].pixelSize = yuvPixelSize[yuvIndex];

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_reprocessing_buffers;

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();

    ret = m_pipes[INDEX(pipeId)]->setupPipe(pipeInfo, m_sensorIds[INDEX(pipeId)]);
    if (ret != NO_ERROR) {
        CLOGE("CLAHE setupPipe fail, ret(%d)", ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    /* clear pipeInfo for next setupPipe */
    for (int i = 0; i < MAX_NODE; i++)
        pipeInfo[i] = nullPipeInfo;
#endif

#if defined(USES_HIFI) || \
    defined(USES_COMBINE_PLUGIN)
    {
    Map_t map;
    pipeId = PIPE_PLUGIN_PRE1_REPROCESSING;

    ret = m_pipes[INDEX(pipeId)]->setupPipe(&map);
    if (ret != NO_ERROR) {
        CLOGE("PRE1 setupPipe fail, ret(%d)", ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    }
#endif

#if defined(USES_HIFI_LLS) || \
    defined(USES_COMBINE_PLUGIN)
    Map_t map;
    pipeId = PIPE_PLUGIN_POST1_REPROCESSING;

    /* POST1 */
    ret = m_pipes[INDEX(pipeId)]->setupPipe(&map);
    if (ret != NO_ERROR) {
        CLOGE("PIPE_PLUGIN_POST1_REPROCESSING setupPipe fail, ret(%d)", ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
#endif

#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
     /* nfd */
     if (m_pipes[INDEX(PIPE_NFD_REPROCESSING)] != NULL) {
         Map_t map;
         ret = m_pipes[INDEX(PIPE_NFD_REPROCESSING)]->setupPipe(&map);
         if (ret != NO_ERROR) {
             CLOGE("nfd setupPipe fail, ret(%d)", ret);
             return INVALID_OPERATION;
         }
     }
#endif

#if defined(USES_CAMERA_EXYNOS_LEC)
     /* LEC */
     if (m_pipes[INDEX(PIPE_PLUGIN_LEC_REPROCESSING)] != NULL) {
         Map_t map;
         ret = m_pipes[INDEX(PIPE_PLUGIN_LEC_REPROCESSING)]->setupPipe(&map);
         if (ret != NO_ERROR) {
             CLOGE("LEC setupPipe fail, ret(%d)", ret);
             return INVALID_OPERATION;
         }
     }
#endif

    m_frameCount = 0;

    ret = m_transitState(FRAME_FACTORY_STATE_INIT);

    return ret;
}

status_t ExynosCameraFrameReprocessingFactory::preparePipes(void)
{
    return NO_ERROR;
}

status_t ExynosCameraFrameReprocessingFactory::startPipes(void)
{
    status_t ret = NO_ERROR;
    CLOGI("");

#if defined(USES_HIFI_LLS) || \
    defined(USES_COMBINE_PLUGIN)
    if (m_pipes[INDEX(PIPE_PLUGIN_POST1_REPROCESSING)] != NULL) {
        ret = m_pipes[INDEX(PIPE_PLUGIN_POST1_REPROCESSING)]->start();
        if (ret < 0) {
            CLOGE("PIPE_PLUGIN_POST1_REPROCESSING start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

#if defined(USES_HIFI) || \
    defined(USES_COMBINE_PLUGIN)
    if (m_pipes[INDEX(PIPE_PLUGIN_PRE1_REPROCESSING)] != NULL) {
        ret = m_pipes[INDEX(PIPE_PLUGIN_PRE1_REPROCESSING)]->start();
        if (ret < 0) {
            CLOGE("PIPE_PLUGIN_PRE1_REPROCESSING start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
    if (m_pipes[INDEX(PIPE_NFD_REPROCESSING)] != NULL) {
        ret = m_pipes[INDEX(PIPE_NFD_REPROCESSING)]->start();
        if (ret < 0) {
            CLOGE("NFD start fail, ret(%d)", ret);
            return INVALID_OPERATION;
        }
    }
#endif

#if defined(USES_CAMERA_EXYNOS_LEC)
    if (m_pipes[INDEX(PIPE_PLUGIN_LEC_REPROCESSING)] != NULL) {
        ret = m_pipes[INDEX(PIPE_PLUGIN_LEC_REPROCESSING)]->start();
        if (ret < 0) {
            CLOGE("LEC start fail, ret(%d)", ret);
            return INVALID_OPERATION;
        }
    }
#endif

#ifdef USE_VRA_FD
    /* VRA Reprocessing */
    if ((m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) ||
			(m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M)) {
        ret = m_pipes[INDEX(PIPE_VRA_REPROCESSING)]->start();
        if (ret != NO_ERROR) {
            CLOGE("VRA start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

#ifdef USE_CLAHE_REPROCESSING
    /* CLAHE Reprocessing */
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[INDEX(PIPE_CLAHE_REPROCESSING)]->start();
        if (ret != NO_ERROR) {
            CLOGE("CLAHE start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

    /* MCSC Reprocessing */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[INDEX(PIPE_MCSC_REPROCESSING)]->start();
        if (ret != NO_ERROR) {
            CLOGE("MCSC start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    /* ISP Reprocessing */
    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[INDEX(PIPE_ISP_REPROCESSING)]->start();
        if (ret != NO_ERROR) {
            CLOGE("ISP start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    /* 3AA Reprocessing */
    if (m_supportPureBayerReprocessing == true) {
        ret = m_pipes[INDEX(PIPE_3AA_REPROCESSING)]->start();
        if (ret != NO_ERROR) {
            CLOGE("ISP start fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    ret = m_transitState(FRAME_FACTORY_STATE_RUN);
    if (ret != NO_ERROR) {
        CLOGE("Failed to transitState. ret %d", ret);
        return ret;
    }

    CLOGI("Starting Reprocessing [SCC>ISP] Success!");

    return NO_ERROR;
}

status_t ExynosCameraFrameReprocessingFactory::stopPipes(void)
{
    status_t ret = NO_ERROR;
    status_t funcRet = NO_ERROR;
    CLOGI("");

    /* 3AA Reprocessing Thread stop */
    if (m_supportPureBayerReprocessing == true) {
        ret = m_pipes[INDEX(PIPE_3AA_REPROCESSING)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("ISP stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    /* ISP Reprocessing Thread stop */
    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[INDEX(PIPE_ISP_REPROCESSING)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("ISP stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    /* MCSC Reprocessing Thread stop */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[INDEX(PIPE_MCSC_REPROCESSING)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("MCSC stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

#ifdef USE_VRA_FD
    if (m_pipes[INDEX(PIPE_VRA_REPROCESSING)] &&
        m_pipes[INDEX(PIPE_VRA_REPROCESSING)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_VRA_REPROCESSING)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("VRA stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

#ifdef USE_CLAHE_REPROCESSING
    /* CLAHE Reprocessing Thread stop */
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[INDEX(PIPE_CLAHE_REPROCESSING)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("CLAHE stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

#if defined(USES_HIFI) || \
    defined(USES_COMBINE_PLUGIN)
    if (m_pipes[INDEX(PIPE_PLUGIN_PRE1_REPROCESSING)] != NULL
        && m_pipes[INDEX(PIPE_PLUGIN_PRE1_REPROCESSING)]->isThreadRunning() == true) {
        ret = stopThread(INDEX(PIPE_PLUGIN_PRE1_REPROCESSING));
        if (ret < 0) {
            CLOGE("PIPE_PLUGIN_PRE1_REPROCESSING stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
    if (m_pipes[INDEX(PIPE_NFD_REPROCESSING)] &&
        m_pipes[INDEX(PIPE_NFD_REPROCESSING)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_NFD_REPROCESSING)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("NFD stopThread fail. ret(%d)", ret);
            funcRet |= ret;
        }
    }
#endif

#if defined(USE_SW_MCSC_REPROCESSING) && (USE_SW_MCSC_REPROCESSING == true)
    if (m_pipes[INDEX(PIPE_SW_MCSC_REPEOCESSING)] != NULL
        && m_pipes[INDEX(PIPE_SW_MCSC_REPEOCESSING)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_SW_MCSC_REPEOCESSING)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("PIPE_SW_MCSC_REPEOCESSING stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

#if defined(USES_HIFI_LLS) || \
    defined(USES_COMBINE_PLUGIN)
    if (m_pipes[INDEX(PIPE_PLUGIN_POST1_REPROCESSING)] != NULL &&
            m_pipes[INDEX(PIPE_PLUGIN_POST1_REPROCESSING)]->isThreadRunning() == true) {
        ret = stopThread(INDEX(PIPE_PLUGIN_POST1_REPROCESSING));
        if (ret < 0) {
            CLOGE("PIPE_PLUGIN_POST1_REPROCESSING stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

    if (m_pipes[INDEX(PIPE_GSC_REPROCESSING2)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_GSC_REPROCESSING2)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("PIPE_GSC_REPROCESSING2 stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    if (m_pipes[INDEX(PIPE_GSC_REPROCESSING3)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_GSC_REPROCESSING3)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("PIPE_GSC_REPROCESSING3 stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    /* 3AA Reprocessing stop */
    if (m_supportPureBayerReprocessing == true) {
        ret = m_pipes[INDEX(PIPE_3AA_REPROCESSING)]->stop();
        if (ret != NO_ERROR) {
            CLOGE("ISP stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    /* ISP Reprocessing stop */
    if (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[INDEX(PIPE_ISP_REPROCESSING)]->stop();
        if (ret != NO_ERROR) {
            CLOGE("ISP stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    /* MCSC Reprocessing stop */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[INDEX(PIPE_MCSC_REPROCESSING)]->stop();
        if (ret != NO_ERROR) {
            CLOGE("MCSC stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

#ifdef USE_VRA_FD
    if (m_pipes[INDEX(PIPE_VRA_REPROCESSING)] &&
        m_pipes[INDEX(PIPE_VRA_REPROCESSING)]->flagStart() == true) {
        ret = m_pipes[INDEX(PIPE_VRA_REPROCESSING)]->stop();
        if (ret != NO_ERROR) {
            CLOGE("VRA stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

#ifdef USE_CLAHE_REPROCESSING
    /* CLAHE Reprocessing stop */
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) {
        ret = m_pipes[INDEX(PIPE_CLAHE_REPROCESSING)]->stop();
        if (ret != NO_ERROR) {
            CLOGE("CLAHE stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

#if defined(USES_HIFI) || \
    defined(USES_COMBINE_PLUGIN)
    if (m_pipes[INDEX(PIPE_PLUGIN_PRE1_REPROCESSING)] != NULL) {
        ret = m_pipes[INDEX(PIPE_PLUGIN_PRE1_REPROCESSING)]->stop();
        if (ret < 0) {
            CLOGE("PIPE_PLUGIN_PRE1_REPROCESSING stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
    if (m_pipes[INDEX(PIPE_NFD_REPROCESSING)] != NULL) {
        ret = m_pipes[INDEX(PIPE_NFD_REPROCESSING)]->stop();
        if (ret != NO_ERROR) {
            CLOGE("NFD stop fail. ret(%d)", ret);
            funcRet |= ret;
        }
    }
#endif

#if defined(USES_CAMERA_EXYNOS_LEC)
    if (m_pipes[INDEX(PIPE_PLUGIN_LEC_REPROCESSING)] != NULL) {
        ret = m_pipes[INDEX(PIPE_PLUGIN_LEC_REPROCESSING)]->stop();
        if (ret != NO_ERROR) {
            CLOGE("LEC stop fail. ret(%d)", ret);
            funcRet |= ret;
        }
    }
#endif

#if defined(USE_SW_MCSC_REPROCESSING) && (USE_SW_MCSC_REPROCESSING == true)
    if (m_pipes[INDEX(PIPE_SW_MCSC_REPEOCESSING)] != NULL) {
        ret = m_pipes[INDEX(PIPE_SW_MCSC_REPEOCESSING)]->stop();
        if (ret < 0) {
            CLOGE("PIPE_SW_MCSC_REPEOCESSING stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

#if defined(USES_HIFI_LLS) || \
    defined(USES_COMBINE_PLUGIN)
    if (m_pipes[INDEX(PIPE_PLUGIN_POST1_REPROCESSING)] != NULL) {
        ret = m_pipes[INDEX(PIPE_PLUGIN_POST1_REPROCESSING)]->stop();
        if (ret < 0) {
            CLOGE("PIPE_PLUGIN_POST1_REPROCESSING stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

    if (m_flagHWFCEnabled == false || (m_flagHWFCEnabled && m_parameters->isHWFCOnDemand())) {
        /* JPEG Reprocessing stop */
        ret = m_pipes[INDEX(PIPE_JPEG_REPROCESSING)]->stop();
        if (ret != NO_ERROR) {
            CLOGE("JPEG stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    /* GSC2 Reprocessing stop */
    ret = m_pipes[INDEX(PIPE_GSC_REPROCESSING2)]->stop();
    if (ret != NO_ERROR) {
        CLOGE("PIPE_GSC_REPROCESSING2 stop fail, ret(%d)", ret);
        /* TODO: exception handling */
        funcRet |= ret;
    }

    /* GSC3 Reprocessing stop */
    ret = m_pipes[INDEX(PIPE_GSC_REPROCESSING3)]->stop();
    if (ret != NO_ERROR) {
        CLOGE("PIPE_GSC_REPROCESSING3 stop fail, ret(%d)", ret);
        /* TODO: exception handling */
        funcRet |= ret;
    }

    ret = m_transitState(FRAME_FACTORY_STATE_CREATE);
    if (ret != NO_ERROR) {
        CLOGE("Failed to transitState. ret %d",
                 ret);
        funcRet |= ret;
    }

    CLOGI("Stopping Reprocessing [3AA>MCSC] Success!");

    return funcRet;
}

status_t ExynosCameraFrameReprocessingFactory::startInitialThreads(void)
{
    CLOGI("start pre-ordered initial pipe thread");

    return NO_ERROR;
}

status_t ExynosCameraFrameReprocessingFactory::setStopFlag(void)
{
    CLOGI("");

    status_t ret = NO_ERROR;

    if (m_pipes[INDEX(PIPE_3AA_REPROCESSING)]->flagStart() == true)
        ret = m_pipes[INDEX(PIPE_3AA_REPROCESSING)]->setStopFlag();

    if (m_pipes[INDEX(PIPE_ISP_REPROCESSING)]->flagStart() == true)
        ret = m_pipes[INDEX(PIPE_ISP_REPROCESSING)]->setStopFlag();

    if (m_pipes[INDEX(PIPE_MCSC_REPROCESSING)]->flagStart() == true)
        ret = m_pipes[INDEX(PIPE_MCSC_REPROCESSING)]->setStopFlag();

#ifdef USE_VRA_FD
    if (m_pipes[INDEX(PIPE_VRA_REPROCESSING)] &&
        m_pipes[INDEX(PIPE_VRA_REPROCESSING)]->flagStart() == true)
        ret = m_pipes[INDEX(PIPE_VRA_REPROCESSING)]->setStopFlag();
#endif

#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
    if (m_pipes[INDEX(PIPE_NFD_REPROCESSING)] &&
        m_pipes[INDEX(PIPE_NFD_REPROCESSING)]->flagStart() == true)
        ret = m_pipes[INDEX(PIPE_NFD_REPROCESSING)]->setStopFlag();
#endif

#if defined(USES_CAMERA_EXYNOS_LEC)
    if (m_pipes[INDEX(PIPE_PLUGIN_LEC_REPROCESSING)] &&
        m_pipes[INDEX(PIPE_PLUGIN_LEC_REPROCESSING)]->flagStart() == true)
        ret = m_pipes[INDEX(PIPE_PLUGIN_LEC_REPROCESSING)]->setStopFlag();
#endif

#if defined(USE_SW_MCSC_REPROCESSING) && (USE_SW_MCSC_REPROCESSING == true)
    if (m_pipes[INDEX(PIPE_SW_MCSC_REPEOCESSING)] != NULL
        && m_pipes[INDEX(PIPE_SW_MCSC_REPEOCESSING)]->flagStart() == true)
        ret = m_pipes[INDEX(PIPE_SW_MCSC_REPEOCESSING)]->setStopFlag();
#endif

    return NO_ERROR;
}

status_t ExynosCameraFrameReprocessingFactory::m_fillNodeGroupInfo(ExynosCameraFrameSP_sptr_t frame)
{
    camera2_node_group node_group_info_3aa;
    camera2_node_group node_group_info_isp;
    camera2_node_group node_group_info_mcsc;
    camera2_node_group node_group_info_vra;
    camera2_node_group node_group_info_clahe;
    camera2_node_group *node_group_info_temp = NULL;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    camera2_virtual_node_group virtual_node_group_info;
    camera2_virtual_node_group *virtual_node_group_temp = NULL;
#endif

    status_t ret = NO_ERROR;
    float zoomRatio = m_configurations->getZoomRatio();
    int pipeId = -1;
    int nodePipeId = -1;
    uint32_t perframePosition = 0;

    int yuvFormat[ExynosCameraParameters::YUV_MAX] = {0};
    int yuvWidth[ExynosCameraParameters::YUV_MAX] = {0};
    int yuvHeight[ExynosCameraParameters::YUV_MAX] = {0};
    int yuvIndex = -1;

    for (int i = ExynosCameraParameters::YUV_STALL_0; i < ExynosCameraParameters::YUV_STALL_MAX; i++) {
        yuvIndex = i % ExynosCameraParameters::YUV_MAX;
        m_parameters->getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)&yuvWidth[yuvIndex], (uint32_t *)&yuvHeight[yuvIndex], i);
        yuvFormat[yuvIndex] = m_configurations->getYuvFormat(i);
    }

    memset(&node_group_info_3aa, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_isp, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_mcsc, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_vra, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_clahe, 0x0, sizeof(camera2_node_group));
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    memset(&virtual_node_group_info, 0x0, sizeof(camera2_virtual_node_group));
#endif

    /* 3AA for Reprocessing */
    if (m_supportPureBayerReprocessing == true) {
        pipeId = INDEX(PIPE_3AA_REPROCESSING);
        node_group_info_temp = &node_group_info_3aa;
        nodePipeId = PIPE_3AA_REPROCESSING;

#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        virtual_node_group_temp = &virtual_node_group_info;
#endif

        node_group_info_temp->leader.request = 1;
        node_group_info_temp->leader.pixelformat = m_parameters->getBayerFormat(nodePipeId);

        nodePipeId = PIPE_3AC_REPROCESSING;
        if (m_request[INDEX(nodePipeId)] == true) {
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                        pipeId, nodePipeId, perframePosition, m_parameters->getBayerFormat(nodePipeId));
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }

        nodePipeId = PIPE_3AG_REPROCESSING;
        if (m_request[INDEX(nodePipeId)] == true) {
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                        pipeId, nodePipeId, perframePosition, m_parameters->getBayerFormat(nodePipeId));
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }

        nodePipeId = PIPE_3AP_REPROCESSING;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getBayerFormat(nodePipeId));
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;


#ifdef SUPPORT_3AF
        nodePipeId = PIPE_3AF_REPROCESSING;
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                    pipeId, nodePipeId, perframePosition, m_parameters->getHW3AFdFormat());
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;
#endif

    }


    /* ISP for Reprocessing */
    if (m_supportPureBayerReprocessing == false
        || m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_ISP_REPROCESSING;
        perframePosition = 0;
        node_group_info_temp = &node_group_info_isp;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        virtual_node_group_temp = &virtual_node_group_info;
#endif
        node_group_info_temp->leader.request = 1;

        int perFrameFormat = 0;
        camera_pixel_size perFramePixelSize = CAMERA_PIXEL_SIZE_8BIT;
        getMetaV4l2Format(pipeId, getNodeType(pipeId), &perFrameFormat, &perFramePixelSize);
        if (perFrameFormat > 0) {
            node_group_info_temp->leader.pixelformat = perFrameFormat;
#ifdef META_USE_NODE_PIXELSIZE
            node_group_info_temp->leader.pixelsize = perFramePixelSize;
#endif
        } else {
            node_group_info_temp->leader.pixelformat = m_parameters->getBayerFormat(pipeId);
#ifdef META_USE_NODE_PIXELSIZE
            node_group_info_temp->leader.pixelsize = getPixelSizeFromBayerFormat(node_group_info_temp->leader.pixelformat);
#endif
        }

        pipeId = INDEX(PIPE_ISP_REPROCESSING);
    }

    /* MCSC for Reprocessing */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        nodePipeId = PIPE_ISPC_REPROCESSING;
        if (m_request[INDEX(nodePipeId)] == true && node_group_info_temp != NULL) {
            nodePipeId = PIPE_ISPC_REPROCESSING;
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                        pipeId, nodePipeId, perframePosition, m_parameters->getHwPictureFormat());
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }

        nodePipeId = PIPE_ISPP_REPROCESSING;
        if (m_request[INDEX(nodePipeId)] == true && node_group_info_temp != NULL) {
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                        pipeId, nodePipeId, perframePosition, m_parameters->getHwPictureFormat());
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }

        pipeId = INDEX(PIPE_MCSC_REPROCESSING);
        perframePosition = 0;
        node_group_info_temp = &node_group_info_mcsc;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        virtual_node_group_temp = &virtual_node_group_info;
#endif
        node_group_info_temp->leader.request = 1;
        node_group_info_temp->leader.pixelformat = m_parameters->getHwPictureFormat();
    }

    if (node_group_info_temp != NULL) {
        nodePipeId = PIPE_MCSC0_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                                    pipeId, nodePipeId, perframePosition,
                                    yuvFormat[ExynosCameraParameters::YUV_STALL_0 % ExynosCameraParameters::YUV_MAX]);
#else
        ret = m_setPerframeCaptureNodeInfo(node_group_info_temp, pipeId, nodePipeId, perframePosition,
                                    yuvFormat[ExynosCameraParameters::YUV_STALL_0 % ExynosCameraParameters::YUV_MAX]);
#endif
        if (ret != NO_ERROR) {
            CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
            return ret;
        }
        perframePosition++;

        if (m_parameters->getNumOfMcscOutputPorts() > 1) {
            nodePipeId = PIPE_MCSC_JPEG_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                    pipeId, nodePipeId, perframePosition, m_parameters->getHwPictureFormat());
#else
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                    pipeId, nodePipeId, perframePosition, m_parameters->getHwPictureFormat());
#endif
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;

            nodePipeId = PIPE_MCSC_THUMB_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                    pipeId, nodePipeId, perframePosition, m_parameters->getHwPictureFormat());
#else
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                    pipeId, nodePipeId, perframePosition, m_parameters->getHwPictureFormat());
#endif
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        else {
            nodePipeId = PIPE_MCSC_JPEG_REPROCESSING;
            ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                    pipeId, nodePipeId, perframePosition, m_parameters->getHwPictureFormat());
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;

            nodePipeId = PIPE_MCSC_THUMB_REPROCESSING;
            ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                    pipeId, nodePipeId, perframePosition, m_parameters->getHwPictureFormat());
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }
#endif

        if (m_parameters->getNumOfMcscOutputPorts() > 3) {
            nodePipeId = PIPE_MCSC1_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                                    pipeId, nodePipeId, perframePosition,
                                    yuvFormat[ExynosCameraParameters::YUV_STALL_1 % ExynosCameraParameters::YUV_MAX]);
#else
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp, pipeId, nodePipeId, perframePosition,
                                    yuvFormat[ExynosCameraParameters::YUV_STALL_1 % ExynosCameraParameters::YUV_MAX]);
#endif
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;

            nodePipeId = PIPE_MCSC2_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                                    pipeId, nodePipeId, perframePosition,
                                    yuvFormat[ExynosCameraParameters::YUV_STALL_2 % ExynosCameraParameters::YUV_MAX]);
#else
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp, pipeId, nodePipeId, perframePosition,
                                    yuvFormat[ExynosCameraParameters::YUV_STALL_2 % ExynosCameraParameters::YUV_MAX]);
#endif
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        else {
            nodePipeId = PIPE_MCSC1_REPROCESSING;
            ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                                    pipeId, nodePipeId, perframePosition,
                                    yuvFormat[ExynosCameraParameters::YUV_STALL_1 % ExynosCameraParameters::YUV_MAX]);
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;

            nodePipeId = PIPE_MCSC2_REPROCESSING;
            ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                                    pipeId, nodePipeId, perframePosition,
                                    yuvFormat[ExynosCameraParameters::YUV_STALL_2 % ExynosCameraParameters::YUV_MAX]);
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }
#endif

#ifdef USE_VRA_FD
        if (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) {
            nodePipeId = PIPE_MCSC5_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                                                pipeId, nodePipeId, perframePosition,
                                                m_parameters->getHwVraInputFormat());
#else
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                        pipeId, nodePipeId, perframePosition, m_parameters->getHwVraInputFormat());
#endif
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }
#endif
    }

#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
    nodePipeId = PIPE_MCSC_PP_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    ret = m_setPerframeVirtualFdCaptureNodeInfo(node_group_info_temp, virtual_node_group_temp,
                                pipeId, nodePipeId, perframePosition,
                                yuvFormat[ExynosCameraParameters::YUV_STALL_0 % ExynosCameraParameters::YUV_MAX]);
#else
    ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                pipeId, nodePipeId, perframePosition,
                                yuvFormat[ExynosCameraParameters::YUV_STALL_0 % ExynosCameraParameters::YUV_MAX]);
#endif
    if (ret != NO_ERROR) {
        CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
        return ret;
    }
    perframePosition++;
#endif

#ifdef USE_VRA_FD
    /* VRA */
    if ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) ||
        (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M)) {
        pipeId = INDEX(PIPE_VRA_REPROCESSING);
        perframePosition = 0;
        node_group_info_temp = &node_group_info_vra;
        node_group_info_temp->leader.request = 1;
        node_group_info_temp->leader.pixelformat = m_parameters->getHwVraInputFormat();
    }
#endif

#ifdef USE_CLAHE_REPROCESSING
    /* CLAHE */
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = INDEX(PIPE_CLAHE_REPROCESSING);
        perframePosition = 0;
        node_group_info_temp = &node_group_info_clahe;
        node_group_info_temp->leader.request = 1;
        node_group_info_temp->leader.pixelformat = m_parameters->getHwPictureFormat();

        nodePipeId = PIPE_CLAHEC_REPROCESSING;
        if (m_request[INDEX(nodePipeId)] == true && node_group_info_temp != NULL) {
            ret = m_setPerframeCaptureNodeInfo(node_group_info_temp,
                                        pipeId, nodePipeId, perframePosition, m_parameters->getHwPictureFormat());
            if (ret != NO_ERROR) {
                CLOGE("setCaptureNodeInfo is fail. ret = %d", ret);
                return ret;
            }
            perframePosition++;
        }
    }
#endif

    frame->setZoomRatio(zoomRatio);

    updateNodeGroupInfo(
            PIPE_3AA_REPROCESSING,
            frame,
            &node_group_info_3aa);
    frame->storeNodeGroupInfo(&node_group_info_3aa, PERFRAME_INFO_PURE_REPROCESSING_3AA);
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_PURE_REPROCESSING_3AA);
#endif

    if (m_supportPureBayerReprocessing == false
        || m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        updateNodeGroupInfo(
                PIPE_ISP_REPROCESSING,
                frame,
                &node_group_info_isp);
        if (m_supportPureBayerReprocessing == true) {
            frame->storeNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_PURE_REPROCESSING_ISP);
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_PURE_REPROCESSING_ISP);
#endif
        } else {
            frame->storeNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_DIRTY_REPROCESSING_ISP);
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_DIRTY_REPROCESSING_ISP);
#endif
        }
    }

//#if defined(NUM_OF_M2M_MCSC_OUTPUT_PORTS) && (NUM_OF_M2M_MCSC_OUTPUT_PORTS == 1)
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        updateNodeGroupInfo(
                PIPE_MCSC_REPROCESSING,
                frame,
                &node_group_info_mcsc);
        if (m_supportPureBayerReprocessing == true) {
            frame->storeNodeGroupInfo(&node_group_info_mcsc, PERFRAME_INFO_PURE_REPROCESSING_MCSC);
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_PURE_REPROCESSING_MCSC);
#endif
        } else {
            frame->storeNodeGroupInfo(&node_group_info_mcsc, PERFRAME_INFO_DIRTY_REPROCESSING_MCSC);
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_DIRTY_REPROCESSING_MCSC);
#endif
        }
    }
//#endif

#ifdef USE_VRA_FD
    if ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) || (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M)) {
        updateNodeGroupInfo(
                PIPE_VRA_REPROCESSING,
                frame,
                &node_group_info_vra);
        if (m_supportPureBayerReprocessing == true) {
            frame->storeNodeGroupInfo(&node_group_info_vra, PERFRAME_INFO_PURE_REPROCESSING_VRA);
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_PURE_REPROCESSING_VRA);
#endif
        } else {
            frame->storeNodeGroupInfo(&node_group_info_vra, PERFRAME_INFO_DIRTY_REPROCESSING_VRA);
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_DIRTY_REPROCESSING_VRA);
#endif
        }
    }
#endif

#ifdef USE_CLAHE_REPROCESSING
    if (m_flagMcscClaheOTF == HW_CONNECTION_MODE_M2M) {
        updateNodeGroupInfo(
                PIPE_CLAHE_REPROCESSING,
                frame,
                &node_group_info_clahe);
        frame->storeNodeGroupInfo(&node_group_info_clahe, PERFRAME_INFO_CLAHE);
    }
#endif

#if defined(USE_SW_MCSC_REPROCESSING) && (USE_SW_MCSC_REPROCESSING == true)
    nodePipeId = PIPE_SW_MCSC_REPEOCESSING;
    if (m_request[INDEX(nodePipeId)]) {
        camera2_node_group node_group_info_sw_mcsc;
        int count = 5;
        memset(&node_group_info_sw_mcsc, 0x0, sizeof(camera2_node_group));
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        memset(&virtual_node_group_info, 0x0, sizeof(camera2_virtual_node_group));
#endif
        perframePosition = 0;
        int swMCSCyuvFormat[FIMC_IS_VIDEO_M5P_NUM] = {0};
        swMCSCyuvFormat[0] = yuvFormat[0];
        swMCSCyuvFormat[1] = yuvFormat[1];
        swMCSCyuvFormat[2] = yuvFormat[2];
        swMCSCyuvFormat[3] = m_parameters->getHwPictureFormat();
        swMCSCyuvFormat[4] = m_parameters->getHwPictureFormat();

        node_group_info_sw_mcsc.leader.pixelformat = V4L2_PIX_FMT_NV21;

        for (int i = 0; i < count; i++) {
            node_group_info_sw_mcsc.capture[perframePosition].vid = (FIMC_IS_VIDEO_M0P_NUM + i) - FIMC_IS_VIDEO_BAS_NUM;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            virtual_node_group_info.virtualVid[perframePosition] = (FIMC_IS_VIDEO_M0P_NUM + i) - FIMC_IS_VIDEO_BAS_NUM;
#endif
            node_group_info_sw_mcsc.capture[perframePosition].pixelformat = swMCSCyuvFormat[i];
            perframePosition++;
        }

        updateNodeGroupInfo(
                PIPE_SW_MCSC_REPEOCESSING,
                frame,
                &node_group_info_sw_mcsc);
        frame->storeNodeGroupInfo(&node_group_info_sw_mcsc, PERFRAME_INFO_REPROCESSING_SWMCSC);
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_REPROCESSING_SWMCSC);
#endif
    }
#endif

    return NO_ERROR;
}

void ExynosCameraFrameReprocessingFactory::m_init(void)
{
    m_flagReprocessing = true;
    m_flagHWFCEnabled = m_parameters->isUseHWFC();

    m_shot_ext = new struct camera2_shot_ext;
}

}; /* namespace android */
