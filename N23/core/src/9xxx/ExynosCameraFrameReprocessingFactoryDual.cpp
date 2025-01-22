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
#define LOG_TAG "ExynosCameraFrameReprocessingFactoryDual"
#include <log/log.h>

#include "ExynosCameraFrameReprocessingFactoryDual.h"

namespace android {
ExynosCameraFrameReprocessingFactoryDual::~ExynosCameraFrameReprocessingFactoryDual()
{
    status_t ret = NO_ERROR;

    ret = destroy();
    if (ret != NO_ERROR)
        CLOGE("destroy fail");
}

status_t ExynosCameraFrameReprocessingFactoryDual::initPipes(void)
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
    uint32_t hwSensorW = 0, hwSensorH = 0;
    uint32_t maxPreviewW = 0, maxPreviewH = 0, hwPreviewW = 0, hwPreviewH = 0;
    uint32_t maxPictureW = 0, maxPictureH = 0, hwPictureW = 0, hwPictureH = 0;
    uint32_t maxThumbnailW = 0, maxThumbnailH = 0;
    uint32_t yuvWidth[ExynosCameraParameters::YUV_MAX] = {0};
    uint32_t yuvHeight[ExynosCameraParameters::YUV_MAX] = {0};
    int bayerFormat = m_parameters->getBayerFormat(PIPE_3AA_REPROCESSING);
    int yuvFormat[ExynosCameraParameters::YUV_MAX] = {0};
    camera_pixel_size yuvPixelSize[ExynosCameraParameters::YUV_MAX] = {CAMERA_PIXEL_SIZE_8BIT};
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

#ifdef DEBUG_RAWDUMP
    if (m_configurations->checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    m_parameters->getSize(HW_INFO_HW_SENSOR_SIZE, &hwSensorW, &hwSensorH);
    m_parameters->getSize(HW_INFO_MAX_PREVIEW_SIZE, &maxPreviewW, &maxPreviewH);
#ifdef SUPPORT_MULTI_STREAM_CAPTURE
    if (m_configurations->getScenario() == SCENARIO_DUAL_REAR_ZOOM) {
        m_configurations->getSize(CONFIGURATION_MAX_PICTURE_SIZE_OF_MULTISTREAM, &maxPictureW, &maxPictureH);
        hwPictureW = maxPictureW;
        hwPictureH = maxPictureH;
    } else
#endif
    {
    m_parameters->getSize(HW_INFO_MAX_PICTURE_SIZE, &maxPictureW, &maxPictureH);
    m_parameters->getSize(HW_INFO_HW_MAX_PICTURE_SIZE, &hwPictureW, &hwPictureH);
    }

    m_parameters->getSize(HW_INFO_MAX_THUMBNAIL_SIZE, &maxThumbnailW, &maxThumbnailH);
    m_parameters->getPreviewBayerCropSize(&bnsSize, &bayerCropSize, false);

    CLOGI(" MaxPreviewSize(%dx%d), HwPreviewSize(%dx%d)", maxPreviewW, maxPreviewH, hwPreviewW, hwPreviewH);
    CLOGI(" MaxPictureSize(%dx%d), HwPictureSize(%dx%d)", maxPictureW, maxPictureH, hwPictureW, hwPictureH);
    CLOGI(" MaxThumbnailSize(%dx%d)", maxThumbnailW, maxThumbnailH);
    CLOGI(" PreviewBayerCropSize(%dx%d)", bayerCropSize.w, bayerCropSize.h);
    CLOGI(" DS Size %dx%d Format %x Buffer count %d",
            dsWidth, dsHeight, dsFormat, config->current->bufInfo.num_vra_buffers);

    for (int i = ExynosCameraParameters::YUV_STALL_0; i < ExynosCameraParameters::YUV_STALL_MAX; i++) {
        yuvIndex = i % ExynosCameraParameters::YUV_MAX;
        m_configurations->getSize(CONFIGURATION_YUV_SIZE, &yuvWidth[yuvIndex], &yuvHeight[yuvIndex], i);
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
    if (m_parameters->getNumOfMcscOutputPorts() == 3) {
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
        tempRect.fullW = maxPictureW;
        tempRect.fullH = maxPictureH;
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
            tempRect.fullW = hwSensorW;
            tempRect.fullH = hwSensorH;
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
            tempRect.fullW = maxPictureW;
            tempRect.fullH = maxPictureH;
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
        perFramePos = m_supportPureBayerReprocessing ? PERFRAME_INFO_PURE_REPROCESSING_ISP : PERFRAME_INFO_DIRTY_REPROCESSING_ISP;
        SET_OUTPUT_DEVICE_BASIC_INFO(perFramePos);
    }

    /* setup pipe info to ISP pipe */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        /* ISPC */
        nodeType = getNodeType(PIPE_ISPC_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_ISPC_POS;

        /* set v4l2 buffer size */
        tempRect.fullW = maxPictureW;
        tempRect.fullH = maxPictureH;
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
        tempRect.fullW = maxPictureW;
        tempRect.fullH = maxPictureH;
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
        tempRect.fullW = maxPictureW;
        tempRect.fullH = maxPictureH;
        tempRect.colorFormat = V4L2_PIX_FMT_NV16M;

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
        break;
    default:
        CLOG_ASSERT("invalid MCSC output(%d)", m_parameters->getNumOfMcscOutputPorts());
        break;
    }

    if(supportJpeg == true && m_flagHWFCEnabled == true) {
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
    ret = m_pipes[INDEX(pipeId)]->setShreadNode(getNodeType(PIPE_MCSC_JPEG_REPROCESSING), getNodeType(PIPE_MCSC1_REPROCESSING));
    ret = m_pipes[INDEX(pipeId)]->setShreadNode(getNodeType(PIPE_MCSC_THUMB_REPROCESSING), getNodeType(PIPE_MCSC2_REPROCESSING));
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
            CLOGE("ISP setupPipe fail, ret(%d)", ret);
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

        ret = m_pipes[pipeId]->setupPipe(pipeInfo, m_sensorIds[pipeId]);
        if (ret != NO_ERROR) {
            CLOGE("MCSC setupPipe fail, ret(%d)", ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }
#endif

#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
     /* nfd */
     if (m_pipes[INDEX(PIPE_NFD_REPROCESSING)] != NULL) {
         Map_t map;
         ret = m_pipes[INDEX(PIPE_NFD_REPROCESSING)]->setupPipe(&map);
         if (ret != NO_ERROR) {
             CLOGE("NFD setupPipe fail, ret(%d)", ret);
             return INVALID_OPERATION;
         }
     }
#endif

#if defined(USES_HIFI) || \
    defined(USES_COMBINE_PLUGIN)
    Map_t map;
    pipeId = PIPE_PLUGIN_PRE1_REPROCESSING;

    ret = m_pipes[INDEX(pipeId)]->setupPipe(&map);
    if (ret != NO_ERROR) {
        CLOGE("PRE1 setupPipe fail, ret(%d)", ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
#endif

#if defined(USE_DUAL_CAMERA) && defined(USE_SLSI_PLUGIN)
    if (m_configurations->getDualReprocessingMode() == DUAL_REPROCESSING_MODE_SW) {
        int pluginId = 0;
#ifdef USES_COMBINE_PLUGIN
        pluginId = PIPE_PLUGIN_POST1_REPROCESSING;
#else
        pluginId = PIPE_FUSION_REPROCESSING;
#endif
        if (m_pipes[INDEX(pluginId)] != NULL) {
            Map_t map;
            ret = m_pipes[INDEX(pluginId)]->setupPipe(&map);
            if(ret != NO_ERROR) {
                CLOGE("Fusion setupPipe fail, ret(%d)", ret);
                /* TODO: exception handling */
                return INVALID_OPERATION;
            }
        }
    }
#endif

    m_frameCount = 0;

    ret = m_transitState(FRAME_FACTORY_STATE_INIT);

    return ret;
}

status_t ExynosCameraFrameReprocessingFactoryDual::startPipes(void)
{
    status_t ret = NO_ERROR;
    CLOGI("");

#ifdef USE_SLSI_PLUGIN
#ifdef USES_COMBINE_PLUGIN
    int pluginId = PIPE_PLUGIN_POST1_REPROCESSING;
#else
    int pluginId = PIPE_FUSION_REPROCESSING;
#endif
    if (m_pipes[INDEX(pluginId)] != NULL &&
            m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true) {
        ret = m_pipes[INDEX(pluginId)]->start();
        if (ret != NO_ERROR) {
            CLOGE("PIPE_FUSION_REPROCESSING start fail, ret(%d)", ret);
            return INVALID_OPERATION;
        }
    }
#endif

    if (m_pipes[INDEX(PIPE_SYNC_REPROCESSING)] != NULL &&
            m_configurations->getMode(CONFIGURATION_DUAL_MODE) == true) {
        ret = m_pipes[INDEX(PIPE_SYNC_REPROCESSING)]->start();
        if (ret != NO_ERROR) {
            CLOGE("SYNC_REPROCESSING start fail, ret(%d)", ret);
            return INVALID_OPERATION;
        }
    }

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

#ifdef USE_VRA_FD
    /* VRA Reprocessing */
    if ((m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) ||
        (m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M)){
        ret = m_pipes[INDEX(PIPE_VRA_REPROCESSING)]->start();
        if (ret != NO_ERROR) {
            CLOGE("VRA start fail, ret(%d)", ret);
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

status_t ExynosCameraFrameReprocessingFactoryDual::stopPipes(void)
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
    if (m_supportPureBayerReprocessing == false
        || m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
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
    if (m_pipes[INDEX(PIPE_VRA_REPROCESSING)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_VRA_REPROCESSING)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("VRA stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
    if (m_pipes[INDEX(PIPE_NFD_REPROCESSING)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_NFD_REPROCESSING)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("NFD stopThread fail. ret(%d)", ret);
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

    if (m_pipes[INDEX(PIPE_SYNC_REPROCESSING)] != NULL
        && m_pipes[INDEX(PIPE_SYNC_REPROCESSING)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_SYNC_REPROCESSING)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("SYNC_REPROCESSING stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    if (m_configurations->getDualReprocessingMode() == DUAL_REPROCESSING_MODE_SW) {
#ifdef USES_COMBINE_PLUGIN
        int pluginId = PIPE_PLUGIN_POST1_REPROCESSING;
#else
        int pluginId = PIPE_FUSION_REPROCESSING;
#endif
        if (m_pipes[INDEX(pluginId)] != NULL
            && m_pipes[INDEX(pluginId)]->isThreadRunning() == true) {
            ret = m_pipes[INDEX(pluginId)]->stopThread();
            if (ret != NO_ERROR) {
                CLOGE("FUSION_REPROCESSING stopThread fail, ret(%d)", ret);
                /* TODO: exception handling */
                funcRet |= ret;
            }
        }
    }

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
    if (m_supportPureBayerReprocessing == false
        || m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
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

#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
    if (m_pipes[INDEX(PIPE_NFD_REPROCESSING)] != NULL) {
        ret = m_pipes[INDEX(PIPE_NFD_REPROCESSING)]->stop();
        if (ret != NO_ERROR) {
            CLOGE("NFD stop fail. ret(%d)", ret);
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

    if (m_pipes[INDEX(PIPE_SYNC_REPROCESSING)] != NULL) {
        ret = m_pipes[INDEX(PIPE_SYNC_REPROCESSING)]->stop();
        if (ret != NO_ERROR) {
            CLOGE("SYNC_REPROCESSING stop fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }

    if (m_configurations->getDualReprocessingMode() == DUAL_REPROCESSING_MODE_SW) {
#ifdef USES_COMBINE_PLUGIN
        int pluginId = PIPE_PLUGIN_POST1_REPROCESSING;
#else
        int pluginId = PIPE_FUSION_REPROCESSING;
#endif
        if (m_pipes[INDEX(pluginId)] != NULL) {
            ret = m_pipes[INDEX(pluginId)]->stop();
            if (ret < 0) {
                CLOGE("FUSION_REPROCESSING stop fail, ret(%d)", ret);
                /* TODO: exception handling */
                funcRet |= ret;
            }
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

ExynosCameraFrameSP_sptr_t ExynosCameraFrameReprocessingFactoryDual::createNewFrame(uint32_t frameCount, bool useJpegFlag)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *newEntity[MAX_NUM_PIPES] = {0};
    if (frameCount <= 0) {
        frameCount = m_frameCount;
    }
    ExynosCameraFrameSP_sptr_t frame =  m_frameMgr->createFrame(m_configurations, frameCount, FRAME_TYPE_REPROCESSING);
    int requestEntityCount = 0;
    int pipeId = -1;
    int parentPipeId = PIPE_3AA_REPROCESSING;

    if (frame == NULL) {
        CLOGE("frame is NULL");
        return NULL;
    }

    m_setBaseInfoToFrame(frame);

    ret = m_initFrameMetadata(frame);
    if (ret != NO_ERROR)
        CLOGE("frame(%d) metadata initialize fail", m_frameCount);

    /* PLUGIN_PRE1 */
    pipeId = PIPE_PLUGIN_PRE1_REPROCESSING;
    if (m_request[INDEX(pipeId)] == true) {
        ////////////////////////////////////////////////
        // FRAME_MODE_MF_STILL means.... it is multi frame mode.
        if (1 < m_configurations->getModeValue(CONFIGURATION_CAPTURE_COUNT)) {
            frame->setMode(FRAME_MODE_MF_STILL, true);
        }

        ////////////////////////////////////////////////

        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_PARTIAL, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }

    /* set 3AA pipe to linkageList */
    if (m_supportPureBayerReprocessing == true) {
        pipeId = PIPE_3AA_REPROCESSING;
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);

        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_PARTIAL, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
        parentPipeId = pipeId;
    }

    /* set ISP pipe to linkageList */
    if (m_supportPureBayerReprocessing == false
        || m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_ISP_REPROCESSING;

        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);

        if (m_supportPureBayerReprocessing == false) {
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_PARTIAL, (enum pipeline)pipeId);
        }

        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
        parentPipeId = pipeId;
    }

    /* set MCSC pipe to linkageList */
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_MCSC_REPROCESSING;
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }

#ifdef USE_VRA_FD
    /* set VRA pipe to linkageList */
    if (((m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M)
        && m_request[INDEX(PIPE_MCSC5_REPROCESSING)] == true && m_request[INDEX(PIPE_VRA_REPROCESSING)] == true) ||
        ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M)
        && m_request[INDEX(PIPE_3AF_REPROCESSING)] == true && m_request[INDEX(PIPE_VRA_REPROCESSING)] == true)) {
        pipeId = PIPE_VRA_REPROCESSING;
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        requestEntityCount++;
    }
#endif

#if defined(USES_CAMERA_EXYNOS_VPL) && defined(USE_EARLY_FD_REPROCES)
    if (m_request[INDEX(PIPE_NFD_REPROCESSING)] == true) {
        pipeId = PIPE_NFD_REPROCESSING;
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        requestEntityCount++;
    }
#endif

    /* set Sync pipe to linkageList */
    pipeId = PIPE_SYNC_REPROCESSING;
    if (m_request[INDEX(pipeId)] == true) {
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }

    if (m_configurations->getDualReprocessingMode() == DUAL_REPROCESSING_MODE_SW) {
#ifdef USES_COMBINE_PLUGIN
        pipeId = PIPE_PLUGIN_POST1_REPROCESSING;
#else
        pipeId = PIPE_FUSION_REPROCESSING;
#endif

        if (m_request[INDEX(pipeId)] == true) {
#ifdef USES_COMBINE_PLUGIN
            frame->setMode(FRAME_MODE_MF_STILL, (1 < m_configurations->getModeValue(CONFIGURATION_CAPTURE_COUNT)));
#endif
            newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
            frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
            requestEntityCount++;
        }
    }

    /* set GSC pipe to linkageList */
    pipeId = PIPE_GSC_REPROCESSING;
    newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
    if (m_parameters->needGSCForCapture(m_cameraId) == true) {
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;
    }

    /* set JPEG pipe to linkageList */
    pipeId = PIPE_JPEG_REPROCESSING;
    newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
    if (useJpegFlag == true && isMcscRequest()) {
        if ((m_flagHWFCEnabled == true && m_configurations->getModeValue(CONFIGURATION_YUV_STALL_PORT_USAGE) == YUV_STALL_USAGE_PICTURE)
            || (m_flagHWFCEnabled == false)) {
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
            frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
            requestEntityCount++;

            for (int pipeId = PIPE_JPEG0_REPROCESSING ; pipeId <= MAX_PIPE_NUM_JPEG_DST_REPROCESSING ; pipeId++) {
                newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
                frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
            }
        }
    }

    /* set GSC pipe to linkageList */
    pipeId = PIPE_GSC_REPROCESSING2;
    newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);

    /* set GSC pipe to linkageList */
    pipeId = PIPE_GSC_REPROCESSING3;
    newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);

    ret = m_initPipelines(frame);
    if (ret != NO_ERROR) {
        CLOGE("m_initPipelines fail, ret(%d)", ret);
    }

    frame->setNumRequestPipe(requestEntityCount);

    m_fillNodeGroupInfo(frame);

    m_frameCount++;

    return frame;
}

status_t ExynosCameraFrameReprocessingFactoryDual::m_setupConfig(void)
{
    CLOGI("");

    int pipeId = -1;
    int node3Paf = -1;
    int node3aa = -1, node3ac = -1, node3ap = -1, node3af = -1, node3ag = -1;
    int nodeIsp = -1, nodeIspc = -1, nodeIspp = -1;
    int nodeMcsc = -1, nodeMcscp0 = -1, nodeMcscp1 = -1;
    int nodeMcscp2 = -1, nodeMcscp3 = -1, nodeMcscp4 = -1;
    int nodeMcscpDs = -1;
    int nodeVra = -1;
    int previousPipeId = -1;
    int mcscSrcPipeId = -1;
    int vraSrcPipeId = -1;
    enum NODE_TYPE nodeType = INVALID_NODE;
    bool flagStreamLeader = false;
    bool supportJpeg = false;
    enum pipeline pipeId3Paf = (enum pipeline) - 1;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    int virtualNodeMCSC[MCSC_PORT_MAX] = {-1};
    int virtualNodeDS = -1;
#endif
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
    int virtualNodeNv21 = -1;
#endif

    m_flagFlite3aaOTF = m_parameters->getHwConnectionMode(PIPE_FLITE, PIPE_3AA);
#ifdef USE_PAF
    pipeId3Paf = PIPE_PAF_REPROCESSING;
    m_flagPaf3aaOTF = m_parameters->getHwConnectionMode(pipeId3Paf, PIPE_3AA_REPROCESSING);
#endif
    m_flag3aaIspOTF = m_parameters->getHwConnectionMode(PIPE_3AA_REPROCESSING, PIPE_ISP_REPROCESSING);
    m_flagIspMcscOTF = m_parameters->getHwConnectionMode(PIPE_ISP_REPROCESSING, PIPE_MCSC_REPROCESSING);
    m_flag3aaVraOTF = m_parameters->getHwConnectionMode(PIPE_3AA_REPROCESSING, PIPE_VRA_REPROCESSING);
    if (m_flag3aaVraOTF == HW_CONNECTION_MODE_NONE)
        m_flagMcscVraOTF = m_parameters->getHwConnectionMode(PIPE_MCSC_REPROCESSING, PIPE_VRA_REPROCESSING);


    m_supportReprocessing = m_parameters->isReprocessing();
    if (getFactoryType() == FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING) {
        m_supportPureBayerReprocessing = m_parameters->getUsePureBayerRemosaicReprocessing();
    } else {
        m_supportPureBayerReprocessing = m_parameters->getUsePureBayerReprocessing();
    }

    m_request[INDEX(PIPE_3AP_REPROCESSING)] = (m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M);
    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        m_request[INDEX(PIPE_ISPC_REPROCESSING)] = true;
    }

    /* jpeg can be supported in this factory depending on mcsc output port */
    if (m_parameters->getNumOfMcscOutputPorts() > 3) {
        m_request[INDEX(PIPE_MCSC_JPEG_REPROCESSING)] = true;
        m_request[INDEX(PIPE_MCSC_THUMB_REPROCESSING)] = true;
        if (m_flagHWFCEnabled == true) {
            m_request[INDEX(PIPE_HWFC_JPEG_SRC_REPROCESSING)] = true;
            m_request[INDEX(PIPE_HWFC_JPEG_DST_REPROCESSING)] = true;
            m_request[INDEX(PIPE_HWFC_THUMB_SRC_REPROCESSING)] = true;
        }
    }

    switch (m_parameters->getHwChainType()) {
    case HW_CHAIN_TYPE_DUAL_CHAIN:
#ifdef USE_PAF
        node3Paf = FIMC_IS_VIDEO_PAF1S_NUM;
#endif
        node3aa = FIMC_IS_VIDEO_31S_NUM;
        node3ac = FIMC_IS_VIDEO_31C_NUM;
        node3ap = FIMC_IS_VIDEO_31P_NUM;
        node3af = FIMC_IS_VIDEO_31F_NUM;
        nodeIsp = FIMC_IS_VIDEO_I1S_NUM;
        nodeIspc = FIMC_IS_VIDEO_I1C_NUM;
        nodeIspp = FIMC_IS_VIDEO_I1P_NUM;
        nodeMcsc = FIMC_IS_VIDEO_M0S_NUM;
        break;
    case HW_CHAIN_TYPE_SEMI_DUAL_CHAIN:
#if defined(FRONT_CAMERA_3AA_NUM)
        if (isFrontCamera(getCameraId())) {
            if (FRONT_CAMERA_3AA_NUM != FIMC_IS_VIDEO_30S_NUM) {
#ifdef USE_PAF
                node3Paf = FIMC_IS_VIDEO_PAF0S_NUM;
#endif
                node3aa = FIMC_IS_VIDEO_30S_NUM;
                node3ac = FIMC_IS_VIDEO_30C_NUM;
                node3ap = FIMC_IS_VIDEO_30P_NUM;
                node3af = FIMC_IS_VIDEO_31F_NUM;
            } else {
#ifdef USE_PAF
                node3Paf = FIMC_IS_VIDEO_PAF1S_NUM;
#endif
                node3aa = FIMC_IS_VIDEO_31S_NUM;
                node3ac = FIMC_IS_VIDEO_31C_NUM;
                node3ap = FIMC_IS_VIDEO_31P_NUM;
                node3af = FIMC_IS_VIDEO_31F_NUM;
            }
        } else
#endif
        {
#ifdef USE_PAF
            node3Paf = FIMC_IS_VIDEO_PAF1S_NUM;
#endif
            node3aa = FIMC_IS_VIDEO_31S_NUM;
            node3ac = FIMC_IS_VIDEO_31C_NUM;
            node3ap = FIMC_IS_VIDEO_31P_NUM;
            node3af = FIMC_IS_VIDEO_31F_NUM;
        }
        nodeIsp = FIMC_IS_VIDEO_I0S_NUM;
        nodeIspc = FIMC_IS_VIDEO_I0C_NUM;
        nodeIspp = FIMC_IS_VIDEO_I0P_NUM;
        break;
    default:
        CLOGE("invalid hw chain type(%d)", m_parameters->getHwChainType());
        break;
    }

#ifdef USE_3AG_CAPTURE
    //HACK: normal scenario : use dng port instead of 3AP
    //      remosaic : use dng port instead of 3AC
    if (getFactoryType() != FRAME_FACTORY_TYPE_REMOSAIC_REPROCESSING) {
        if (node3ap == FIMC_IS_VIDEO_30P_NUM ||
            node3ap == FIMC_IS_VIDEO_31P_NUM)
            node3ap = CONVERT_3AP_TO_3AG(node3ap);
    } else {
        if (node3ac == FIMC_IS_VIDEO_30C_NUM ||
            node3ac == FIMC_IS_VIDEO_31C_NUM)
            node3ac = CONVERT_3AC_TO_3AG(node3ac);
    }
#endif

    if (m_parameters->getNumOfMcscInputPorts() > 1)
#if 0//def USE_DUAL_CAMERA // HACK??
        nodeMcsc = FIMC_IS_VIDEO_M0S_NUM;
#else
        nodeMcsc = FIMC_IS_VIDEO_M1S_NUM;
#endif
    else
        nodeMcsc = FIMC_IS_VIDEO_M0S_NUM;

    nodeMcscp0 = FIMC_IS_VIDEO_M0P_NUM;
    nodeMcscp1 = FIMC_IS_VIDEO_M1P_NUM;
    nodeMcscp2 = FIMC_IS_VIDEO_M2P_NUM;
    nodeMcscp3 = FIMC_IS_VIDEO_M3P_NUM;
    nodeMcscp4 = FIMC_IS_VIDEO_M4P_NUM;
    nodeVra = FIMC_IS_VIDEO_VRA_NUM;

#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    virtualNodeMCSC[MCSC_PORT_0] = nodeMcscp0;
    virtualNodeMCSC[MCSC_PORT_1] = nodeMcscp1;
    virtualNodeMCSC[MCSC_PORT_2] = nodeMcscp2;
    virtualNodeMCSC[MCSC_PORT_3] = nodeMcscp3;
    virtualNodeMCSC[MCSC_PORT_4] = nodeMcscp4;
    virtualNodeDS = FIMC_IS_VIDEO_M5P_NUM;
#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
    virtualNodeNv21 = FIMC_IS_VIRTUAL_VIDEO_PP;
#endif
#endif

    switch (m_parameters->getNumOfMcscOutputPorts()) {
    case 5:
        nodeMcscpDs = FIMC_IS_VIDEO_M5P_NUM;
        break;
    case 3:
        nodeMcscp3 = FIMC_IS_VIDEO_M1P_NUM;
        nodeMcscp4 = FIMC_IS_VIDEO_M2P_NUM;
        nodeMcscpDs = FIMC_IS_VIDEO_M3P_NUM;
        break;
    default:
        CLOGE("invalid output port(%d)", m_parameters->getNumOfMcscOutputPorts());
        break;
    }

    m_initDeviceInfo(INDEX(PIPE_3AA_REPROCESSING));
    m_initDeviceInfo(INDEX(PIPE_ISP_REPROCESSING));
    m_initDeviceInfo(INDEX(PIPE_MCSC_REPROCESSING));
#ifdef USE_VRA_FD
    m_initDeviceInfo(INDEX(PIPE_VRA_REPROCESSING));
#endif
#if defined(USE_SW_MCSC_REPROCESSING) && (USE_SW_MCSC_REPROCESSING == true)
    m_initDeviceInfo(INDEX(PIPE_SW_MCSC_REPEOCESSING));
#endif
    m_initDeviceInfo(INDEX(PIPE_JPEG_REPROCESSING));

    /*
     * 3AA for Reprocessing
     */
    pipeId = INDEX(PIPE_3AA_REPROCESSING);
    previousPipeId = PIPE_FLITE;

#ifdef USE_PAF
    if (m_flagPaf3aaOTF == HW_CONNECTION_MODE_OTF
        || m_flagPaf3aaOTF == HW_CONNECTION_MODE_M2M) {

        /*
         * If dirty bayer is used for reprocessing, the ISP video node is leader in the reprocessing stream.
         */
        if (m_supportPureBayerReprocessing == false) {
            flagStreamLeader = false;
        } else {
            flagStreamLeader = true;
        }

        /* 3PAF */
        nodeType = getNodeType(pipeId3Paf);
        m_deviceInfo[pipeId].pipeId[nodeType]  = pipeId3Paf;
        m_deviceInfo[pipeId].nodeNum[nodeType] = node3Paf;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_3PAF_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        int fliteNodeNum = getFliteNodenum(m_cameraId);
        m_sensorIds[pipeId][nodeType]  = m_getSensorId(getFliteCaptureNodenum(m_cameraId, fliteNodeNum), false, flagStreamLeader, m_flagReprocessing);
        previousPipeId = INDEX(pipeId3Paf);
    }
#endif

    /*
     * If dirty bayer is used for reprocessing, the ISP video node is leader in the reprocessing stream.
     */
    if (m_supportPureBayerReprocessing == false
        || m_flagPaf3aaOTF == HW_CONNECTION_MODE_OTF) {
        flagStreamLeader = false;
    } else {
        flagStreamLeader = true;
    }

    /* 3AS */
    nodeType = getNodeType(PIPE_3AA_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_3AA_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3aa;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_3AA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    if (m_flagPaf3aaOTF == HW_CONNECTION_MODE_OTF
        || m_flagPaf3aaOTF == HW_CONNECTION_MODE_M2M) {
        m_sensorIds[pipeId][nodeType]  = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(pipeId3Paf)], m_flagPaf3aaOTF, flagStreamLeader, m_flagReprocessing);
    } else {
        int fliteNodeNum = getFliteNodenum(m_cameraId);
        m_sensorIds[pipeId][nodeType]  = m_getSensorId(getFliteCaptureNodenum(m_cameraId, fliteNodeNum), false, flagStreamLeader, m_flagReprocessing);
    }

    // Restore leader flag
    flagStreamLeader = false;

    /* 3AC */
    nodeType = getNodeType(PIPE_3AC_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AC_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3ac;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_3AA_CAPTURE_OPT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* 3AP */
    nodeType = getNodeType(PIPE_3AP_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AP_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3ap;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_3AA_CAPTURE_MAIN", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    if (m_parameters->isUse3aaDNG()) {
        /* 3AG */
        node3ag = FIMC_IS_VIDEO_31G_NUM;
        nodeType = getNodeType(PIPE_3AG_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AG_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = node3ag;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_3AA_DNG", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AG_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);
    }

#ifdef SUPPORT_3AF
    /* 3AF */
    nodeType = getNodeType(PIPE_3AF_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AF_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3af;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_3AA_FD", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);
#endif

#ifdef USE_VRA_FD
    if (m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) {
        int vraPipeId = INDEX(PIPE_VRA_REPROCESSING);

        /*
         * VRA
         */
        previousPipeId = pipeId;

        vraSrcPipeId = INDEX(PIPE_3AF_REPROCESSING);
        nodeType = getNodeType(PIPE_VRA_REPROCESSING);
        m_deviceInfo[vraPipeId].pipeId[nodeType]  = PIPE_VRA;
        m_deviceInfo[vraPipeId].nodeNum[nodeType] = nodeVra;
        m_deviceInfo[vraPipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[vraPipeId].nodeName[nodeType], "VRA_OUTPUT_REPROCESSING", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[vraPipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(vraSrcPipeId)], m_flag3aaVraOTF, flagStreamLeader, m_flagReprocessing);
    }
#endif

    /*
     * ISP for Reprocessing
     */
    previousPipeId = pipeId;
    if (m_supportPureBayerReprocessing == false
        || m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = INDEX(PIPE_ISP_REPROCESSING);
    }

    if (m_supportPureBayerReprocessing == false) {
        /* Set leader flag on ISP pipe for Dirty bayer reprocessing */
        flagStreamLeader = true;
    }
    /* ISPS */
    nodeType = getNodeType(PIPE_ISP_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_ISP_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeIsp;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_ISP_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(PIPE_3AP_REPROCESSING)], m_flag3aaIspOTF, flagStreamLeader, m_flagReprocessing);

    // Restore leader flag
    flagStreamLeader = false;

    /*
     * MCSC for Reprocessing
     */
    previousPipeId = pipeId;
    mcscSrcPipeId = PIPE_ISP_REPROCESSING;

    if (m_flagIspMcscOTF == HW_CONNECTION_MODE_M2M) {
        /* ISPC */
        nodeType = getNodeType(PIPE_ISPC_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_ISPC_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeIspc;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_ISP_CAPTURE_M2M", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_ISP_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /* ISPP */
        nodeType = getNodeType(PIPE_ISPP_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_ISPP_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeIspp;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_ISP_CAPTURE_OTF", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_ISP_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        pipeId = INDEX(PIPE_MCSC_REPROCESSING);
        mcscSrcPipeId = PIPE_ISPC_REPROCESSING;
    }

    /* MCSC */
    nodeType = getNodeType(PIPE_MCSC_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMcsc;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(mcscSrcPipeId)], m_flagIspMcscOTF, flagStreamLeader, m_flagReprocessing);

    /* MCSC0 */
    nodeType = getNodeType(PIPE_MCSC0_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC0_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC0_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_0];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_CAPTURE_YUV_0", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC1 */
    nodeType = getNodeType(PIPE_MCSC1_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC1_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC1_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_1];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_CAPTURE_YUV_1", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC2 */
    nodeType = getNodeType(PIPE_MCSC2_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC2_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC2_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_2];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_CAPTURE_YUV_2", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC3 */
    nodeType = getNodeType(PIPE_MCSC_JPEG_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_JPEG_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC_JPEG_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_3];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_CAPTURE_MAIN", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC4 */
    nodeType = getNodeType(PIPE_MCSC_THUMB_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_THUMB_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC_THUMB_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_4];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_CAPTURE_THUMBNAIL", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

#ifdef USE_RESERVED_NODE_PPJPEG_MCSCPORT
    /* MCSC NV21 */
    nodeType = getNodeType(PIPE_MCSC_PP_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_PP_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC_PP_REPROCESSING)] = virtualNodeNv21;
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_MCSC_CAPTURE_PP", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);
#endif

    switch (m_parameters->getNumOfMcscOutputPorts()) {
    case 5:
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC2_REPROCESSING)] = nodeMcscp2;
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC1_REPROCESSING)] = nodeMcscp1;
        /* Not break; */
    case 3:
        supportJpeg = true;
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_THUMB_REPROCESSING)] = nodeMcscp4;
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_JPEG_REPROCESSING)] = nodeMcscp3;
        /* Not break; */
    case 1:
        m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC0_REPROCESSING)] = nodeMcscp0;
        break;
    default:
        CLOG_ASSERT("invalid MCSC output(%d)", m_parameters->getNumOfMcscOutputPorts());
        break;
    }

    if (supportJpeg == true && m_flagHWFCEnabled == true) {
        /* JPEG Src */
        nodeType = getNodeType(PIPE_HWFC_JPEG_SRC_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_HWFC_JPEG_SRC_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_HWFC_JPEG_NUM;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_HWFC_JPEG_SRC", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_HWFC_JPEG_SRC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /* Thumbnail Src */
        nodeType = getNodeType(PIPE_HWFC_THUMB_SRC_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_HWFC_THUMB_SRC_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_HWFC_THUMB_NUM;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_HWFC_THUMBNAIL_SRC", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_HWFC_THUMB_SRC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /* JPEG Dst */
        nodeType = getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_HWFC_JPEG_DST_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_HWFC_JPEG_NUM;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_HWFC_JPEG_DST", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /* Thumbnail Dst  */
        nodeType = getNodeType(PIPE_HWFC_THUMB_DST_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_HWFC_THUMB_DST_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_HWFC_THUMB_NUM;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REPROCESSING_HWFC_THUMBNAIL_DST", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_HWFC_THUMB_DST_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);
    }

#ifdef USE_VRA_FD
    if (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) {
        /* MCSC5 */
        nodeType = getNodeType(PIPE_MCSC5_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC5_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC5_REPROCESSING)] = virtualNodeDS;
#endif
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMcscpDs;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "MCSC_DS_REPROCESSING", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /*
         * VRA
         */
        previousPipeId = pipeId;
        vraSrcPipeId = PIPE_MCSC5_REPROCESSING;
        pipeId = INDEX(PIPE_VRA_REPROCESSING);

        nodeType = getNodeType(PIPE_VRA_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_VRA_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeVra;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "VRA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(vraSrcPipeId)], m_flagMcscVraOTF, flagStreamLeader, m_flagReprocessing);
    }
#endif

#if defined(USE_SW_MCSC_REPROCESSING) && (USE_SW_MCSC_REPROCESSING == true)
    pipeId = INDEX(PIPE_SW_MCSC_REPEOCESSING);

    nodeType = getNodeType(PIPE_SW_MCSC_REPEOCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_SW_MCSC_REPEOCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "SW_MCSC_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC0_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC0_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "SW_MCSC_YUV0", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC1_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC1_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "SW_MCSC_YUV1", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC2_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC2_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "SW_MCSC_YUV2", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC3_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC3_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "SW_MCSC_MAIN", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_MCSC4_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC4_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = PREVIEW_GSC_NODE_NUM;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "SW_MCSC_THUMBNAIL", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
#endif

    /* JPEG for Reprocessing */
    pipeId = INDEX(PIPE_JPEG_REPROCESSING);

    nodeType = getNodeType(PIPE_JPEG_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_JPEG_REPROCESSING;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "MASTER_JPEG_REPROCESSING_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_JPEG0_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_JPEG0_REPROCESSING;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "MASTER_JPEG_REPROCESSING_CAPTURE0", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_JPEG1_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_JPEG1_REPROCESSING;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "MASTER_JPEG_REPROCESSING_CAPTURE1", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    /* GSC for Reprocessing */
    m_nodeNums[INDEX(PIPE_GSC_REPROCESSING)][OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;

    /* GSC2 for Reprocessing */
    m_nodeNums[INDEX(PIPE_GSC_REPROCESSING2)][OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;

    /* GSC3 for Reprocessing */
    m_nodeNums[INDEX(PIPE_GSC_REPROCESSING3)][OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;

    return NO_ERROR;
}

status_t ExynosCameraFrameReprocessingFactoryDual::m_constructPipes(void)
{
    CLOGI("");

    status_t ret = NO_ERROR;
    int pipeId = -1;

    ret = ExynosCameraFrameReprocessingFactory::m_constructPipes();
    if (ret != NO_ERROR) {
        CLOGE("Construct Pipes fail! ret(%d)", ret);
        return ret;
    }

    /* SYNC Pipe for Dual */
    pipeId = PIPE_SYNC_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeSync(m_cameraId, m_configurations, m_parameters, m_flagReprocessing, m_nodeNums[INDEX(pipeId)], m_camIdInfo);    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    m_pipes[INDEX(pipeId)]->setPipeName("PIPE_SYNC_REPROCESSING");

    CLOGI("m_pipes[%d] : PipeId(%d)" , INDEX(pipeId), m_pipes[INDEX(pipeId)]->getPipeId());

    PlugInScenario fusionPlugin = m_configurations->getFusionCapturePluginScenario();
    if (fusionPlugin != PLUGIN_SCENARIO_COMBINEFUSION_REPROCESSING) {
        /* Fusion Pipe for Dual */
        pipeId = PIPE_FUSION_REPROCESSING;
        ExynosCameraPipePlugIn *fusionPipe = new ExynosCameraPipePlugIn(m_cameraId,
            m_configurations,
            m_parameters,
            m_flagReprocessing,
            m_nodeNums[INDEX(pipeId)],
            m_camIdInfo,
            fusionPlugin);
        m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)fusionPipe;
        m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_FUSION_REPROCESSING");

        CLOGI("m_pipes[%d] : PipeId(%d)", INDEX(pipeId), m_pipes[INDEX(pipeId)]->getPipeId());
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameReprocessingFactoryDual::m_fillNodeGroupInfo(ExynosCameraFrameSP_sptr_t frame)
{
    camera2_node_group node_group_info_3aa;
    camera2_node_group node_group_info_isp;
    camera2_node_group node_group_info_mcsc;
    camera2_node_group node_group_info_vra;
    camera2_node_group *node_group_info_temp = NULL;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    camera2_virtual_node_group virtual_node_group_info;
    camera2_virtual_node_group *virtual_node_group_temp = NULL;;
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
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    memset(&virtual_node_group_info, 0x0, sizeof(camera2_virtual_node_group));
#endif

    /* 3AA for Reprocessing */
    if (m_supportPureBayerReprocessing == true) {
        pipeId = INDEX(PIPE_3AA_REPROCESSING);
        nodePipeId = PIPE_3AA_REPROCESSING;
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
                                    pipeId, nodePipeId, perframePosition, m_parameters->getHwVraInputFormat());
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

#ifdef USE_VRA_FD
    if (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) {
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

    return NO_ERROR;
}

void ExynosCameraFrameReprocessingFactoryDual::m_init(void)
{
    m_flagReprocessing = true;
}

}; /* namespace android */
