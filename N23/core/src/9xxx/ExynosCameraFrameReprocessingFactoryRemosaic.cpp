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
#define LOG_TAG "ExynosCameraFrameReprocessingFactoryRemosaic"
#include <log/log.h>

#include "ExynosCameraFrameReprocessingFactoryRemosaic.h"
#include "ExynosCameraPipeGSC.h"

namespace android {

ExynosCameraFrameReprocessingFactoryRemosaic::~ExynosCameraFrameReprocessingFactoryRemosaic()
{
    status_t ret = NO_ERROR;

    ret = destroy();
    if (ret != NO_ERROR)
        CLOGE("destroy fail");
}

status_t ExynosCameraFrameReprocessingFactoryRemosaic::initPipes(void)
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

    m_parameters->getSize(HW_INFO_HW_REMOSAIC_SENSOR_SIZE, (uint32_t *)&hwSensorW, (uint32_t *)&hwSensorH);
    m_parameters->getSize(HW_INFO_MAX_PREVIEW_SIZE, (uint32_t *)&maxPreviewW, (uint32_t *)&maxPreviewH);
    m_parameters->getSize(HW_INFO_HW_PREVIEW_SIZE, (uint32_t *)&hwPreviewW, (uint32_t *)&hwPreviewH);
    m_parameters->getSize(HW_INFO_MAX_PICTURE_SIZE, (uint32_t *)&maxPictureW, (uint32_t *)&maxPictureH);
    m_parameters->getSize(HW_INFO_HW_MAX_PICTURE_SIZE, (uint32_t *)&hwPictureW, (uint32_t *)&hwPictureH);
    m_parameters->getSize(HW_INFO_MAX_THUMBNAIL_SIZE, (uint32_t *)&maxThumbnailW, (uint32_t *)&maxThumbnailH);
    m_parameters->getPreviewBayerCropSize(&bnsSize, &bayerCropSize, false);

#ifdef SUPPORT_REMOSAIC_ROTATION
    SWAP(int, hwSensorW, hwSensorH);
    SWAP(int, maxPictureW, maxPictureH);
    SWAP(int, hwPictureW, hwPictureH);

    SWAP(int, maxThumbnailW, maxThumbnailH); //TODO check
#endif //SUPPORT_REMOSAIC_ROTATION

    CLOGI(" HWSensorSize(%dx%d)", hwSensorW, hwSensorH);
    CLOGI(" MaxPreviewSize(%dx%d), HwPreviewSize(%dx%d)", maxPreviewW, maxPreviewH, hwPreviewW, hwPreviewH);
    CLOGI(" MaxPictureSize(%dx%d), HwPictureSize(%dx%d)", maxPictureW, maxPictureH, hwPictureW, hwPictureH);
    CLOGI(" MaxThumbnailSize(%dx%d)", maxThumbnailW, maxThumbnailH);
    CLOGI(" PreviewBayerCropSize(%dx%d)", bayerCropSize.w, bayerCropSize.h);
    CLOGI("DS Size %dx%d Format %x Buffer count %d",
            dsWidth, dsHeight, dsFormat, config->current->bufInfo.num_vra_buffers);

    for (int i = ExynosCameraParameters::YUV_STALL_0; i < ExynosCameraParameters::YUV_STALL_MAX; i++) {
        yuvIndex = i % ExynosCameraParameters::YUV_MAX;
        m_parameters->getSize(HW_INFO_HW_YUV_SIZE, (uint32_t *)&yuvWidth[yuvIndex], (uint32_t *)&yuvHeight[yuvIndex], i);
        yuvFormat[yuvIndex] = m_configurations->getYuvFormat(i);
        yuvPixelSize[yuvIndex] = m_configurations->getYuvPixelSize(i);

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
        if (m_flagPaf3aaOTF == HW_CONNECTION_MODE_OTF) {
            /* 3PAF */
            uint32_t pipeId3Paf = -1;
#ifdef USE_PAF
            pipeId3Paf = PIPE_PAF_REPROCESSING;
#endif
            nodeType = getNodeType(pipeId3Paf);
            bayerFormat = m_parameters->getBayerFormat(PIPE_3AA_REPROCESSING);

            /* set v4l2 buffer size */
            tempRect.fullW = hwSensorW;
            tempRect.fullH = hwSensorH;
            tempRect.colorFormat = bayerFormat;

            /* set v4l2 video node buffer count */
            pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;

            /* Set output node default info */
            SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_PURE_REPROCESSING_3AA);
        }

        /* 3AS */
        nodeType = getNodeType(PIPE_3AA_REPROCESSING);
        bayerFormat = m_parameters->getBayerFormat(PIPE_3AA_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = hwSensorW;
        tempRect.fullH = hwSensorH;
        tempRect.colorFormat = bayerFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;

        if (m_flagPaf3aaOTF == HW_CONNECTION_MODE_OTF) {
            SET_CAPTURE_DEVICE_BASIC_INFO();
        } else {
            /* Set output node default info */
            SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_PURE_REPROCESSING_3AA);
        }

        /* 3AC */
        nodeType = getNodeType(PIPE_3AC_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_3AC_POS;
        bayerFormat = m_parameters->getBayerFormat(PIPE_3AC_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = hwSensorW;
        tempRect.fullH = hwSensorH;
        tempRect.colorFormat = bayerFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;

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
        tempRect.fullW = maxPictureW;
        tempRect.fullH = maxPictureH;
        tempRect.colorFormat = bayerFormat;

        /* set v4l2 video node bytes per plane */
        pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);

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
        tempRect.fullW = maxPictureW;
        tempRect.fullH = maxPictureH;
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = CAMERA_PIXEL_SIZE_PACKED_10BIT;

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
        tempRect.colorFormat = pictureFormat;

        /* set YUV pixel size */
        pipeInfo[nodeType].pixelSize = picturePixelSize;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_hwdis_buffers;

        /* Set output node default info */
        int mcscPerframeInfoIndex = m_supportPureBayerReprocessing ? PERFRAME_INFO_PURE_REPROCESSING_MCSC : PERFRAME_INFO_DIRTY_REPROCESSING_MCSC;
        SET_OUTPUT_DEVICE_BASIC_INFO(mcscPerframeInfoIndex);
    }

#ifdef SUPPORT_REMOSAIC_ROTATION
    /* If rotate, the buffer size is 16 align, so the set format size should also be 16 align. */
    tempRect.fullW = ALIGN_UP(tempRect.fullW, CAMERA_16PX_ALIGN);
#endif

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

#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        ret = m_pipes[INDEX(pipeId)]->setShreadNode(getNodeType(PIPE_MCSC_JPEG_REPROCESSING), getNodeType(PIPE_MCSC1_REPROCESSING));
        ret = m_pipes[INDEX(pipeId)]->setShreadNode(getNodeType(PIPE_MCSC_THUMB_REPROCESSING), getNodeType(PIPE_MCSC2_REPROCESSING));
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

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;
    }
#endif

#ifdef USE_VRA_FD
    /* VRA */
    if (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M) {
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

    m_frameCount = 0;

    ret = m_transitState(FRAME_FACTORY_STATE_INIT);

    return ret;
}

status_t ExynosCameraFrameReprocessingFactoryRemosaic::stopPipes(void)
{
    status_t ret = NO_ERROR;
    status_t funcRet = NO_ERROR;
    CLOGI("");

#if defined(SUPPORT_SENSOR_REMOSAIC_SW)
    if (m_pipes[INDEX(PIPE_REMOSAIC_REPROCESSING)] != NULL
        && m_pipes[INDEX(PIPE_REMOSAIC_REPROCESSING)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_REMOSAIC_REPROCESSING)]->stopThread();
        if (ret != NO_ERROR) {
            CLOGE("PIPE_REMOSAIC_REPROCESSING stopThread fail, ret(%d)", ret);
            /* TODO: exception handling */
            funcRet |= ret;
        }
    }
#endif

    return ExynosCameraFrameReprocessingFactory::stopPipes();
}

void ExynosCameraFrameReprocessingFactoryRemosaic::setRequest(int pipeId,bool enable)
{
    CLOGW("SETREUEST(%d / %d)", pipeId, enable);
    return ExynosCameraFrameFactoryBase::setRequest(pipeId, enable);
}

ExynosCameraFrameSP_sptr_t ExynosCameraFrameReprocessingFactoryRemosaic::createNewFrame(uint32_t frameCount, bool useJpegFlag)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *newEntity[MAX_NUM_PIPES] = {0};
    if (frameCount <= 0) {
        frameCount = m_frameCount;
    }
    ExynosCameraFrameSP_sptr_t frame =  m_frameMgr->createFrame(m_configurations, frameCount, FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION);
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

    /* set 3AA pipe to linkageList */
    if (m_supportPureBayerReprocessing == true) {
#if defined(SUPPORT_REMOSAIC_ROTATION)
        pipeId = PIPE_REMOSAIC_REPROCESSING;
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        requestEntityCount++;
#endif
        pipeId = PIPE_3AA_REPROCESSING;
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        requestEntityCount++;
#endif //SUPPORT_VIRTUALFD_REPROCESSING
        parentPipeId = pipeId;
    }

    /* set ISP pipe to linkageList */
    if (m_supportPureBayerReprocessing == false
        || m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = PIPE_ISP_REPROCESSING;
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        if (m_supportPureBayerReprocessing == true) {
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
            frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
#else
            frame->addChildEntity(newEntity[INDEX(parentPipeId)], newEntity[INDEX(pipeId)], INDEX(PIPE_3AP_REPROCESSING));
#endif
        } else {
            frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        }

        parentPipeId = pipeId;
    }

    frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_PARTIAL, (enum pipeline)pipeId);
    frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
    frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
    requestEntityCount++;

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
        ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) &&
        m_request[INDEX(PIPE_3AF_REPROCESSING)] == true && m_request[INDEX(PIPE_VRA_REPROCESSING)] == true)) {
        pipeId = PIPE_VRA_REPROCESSING;
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        requestEntityCount++;
    }
#endif

#ifdef USE_DUAL_CAMERA
    /* set Sync pipe to linkageList */
    pipeId = PIPE_SYNC_REPROCESSING;
    if (m_request[INDEX(pipeId)] == true) {
        newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
        requestEntityCount++;
    }
#endif

    /* set GSC pipe to linkageList */
    pipeId = PIPE_GSC_REPROCESSING;
    newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);

    frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
    frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
#if defined(SUPPORT_REMOSAIC_ROTATION)
    requestEntityCount++;
#endif

    /* set JPEG pipe to linkageList */
    pipeId = PIPE_JPEG_REPROCESSING;
    newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
    if ((isMcscRequest() == true) && (m_flagHWFCEnabled == false || useJpegFlag == true)) {
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_BUFFER, (enum pipeline)pipeId);
        frame->setPipeIdForResultUpdate(ExynosCameraFrame::RESULT_UPDATE_TYPE_ALL, (enum pipeline)pipeId);
        requestEntityCount++;

        for (int pipeId = PIPE_JPEG0_REPROCESSING ; pipeId < MAX_PIPE_NUM_JPEG_DST_REPROCESSING ; pipeId++) {
            newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
            frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);
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
    frame->setParameters(m_parameters);
    frame->setCameraId(m_cameraId);

    m_fillNodeGroupInfo(frame);

    m_frameCount++;

    return frame;
}

status_t ExynosCameraFrameReprocessingFactoryRemosaic::m_setupConfig(void)
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
    enum pipeline pipeId3Paf = (enum pipeline)-1;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    int virtualNodeMCSC[MCSC_PORT_MAX] = {-1};
    int virtualNodeDS = -1;
#endif

    m_flagFlite3aaOTF = m_parameters->getHwConnectionMode(PIPE_FLITE, PIPE_3AA);
#ifdef USE_PAF
    pipeId3Paf = PIPE_PAF_REPROCESSING;
    m_flagPaf3aaOTF = m_parameters->getHwConnectionMode(pipeId3Paf, PIPE_3AA_REPROCESSING);
#endif
    m_flag3aaIspOTF = HW_CONNECTION_MODE_M2M;
    m_flagIspMcscOTF = m_parameters->getHwConnectionMode(PIPE_ISP_REPROCESSING, PIPE_MCSC_REPROCESSING);
    m_flag3aaVraOTF = m_parameters->getHwConnectionMode(PIPE_3AA_REPROCESSING, PIPE_VRA_REPROCESSING);
    if (m_flag3aaVraOTF == HW_CONNECTION_MODE_NONE)
        m_flagMcscVraOTF = m_parameters->getHwConnectionMode(PIPE_MCSC_REPROCESSING, PIPE_VRA_REPROCESSING);

    m_supportReprocessing = m_parameters->isReprocessing();
    m_supportPureBayerReprocessing = m_parameters->getUsePureBayerRemosaicReprocessing();

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
#ifdef USE_DUAL_CAMERA
            if (m_camIdInfo->cameraId[SUB_CAM] == getCameraId()
                || m_camIdInfo->cameraId[SUB_CAM2] == getCameraId()) {
#ifdef USE_PAF
                node3Paf = FIMC_IS_VIDEO_PAF0S_NUM;
#endif

                node3aa = FIMC_IS_VIDEO_30S_NUM;
                node3ac = FIMC_IS_VIDEO_30C_NUM;
                node3ap = FIMC_IS_VIDEO_30P_NUM;
                node3af = FIMC_IS_VIDEO_30F_NUM;
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
        }
        nodeIsp = FIMC_IS_VIDEO_I0S_NUM;
        nodeIspc = FIMC_IS_VIDEO_I0C_NUM;
        nodeIspp = FIMC_IS_VIDEO_I0P_NUM;
        break;
    default:
        CLOGE("invalid hw chain type(%d)", m_parameters->getHwChainType());
        break;
    }

    if (m_parameters->getNumOfMcscInputPorts() > 1) {
#if 0//def USE_DUAL_CAMERA // HACK??
        nodeMcsc = FIMC_IS_VIDEO_M0S_NUM;
#else
        nodeMcsc = FIMC_IS_VIDEO_M1S_NUM;
#endif
    } else {
        nodeMcsc = FIMC_IS_VIDEO_M0S_NUM;
	}

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

    m_initDeviceInfo(INDEX(PIPE_JPEG_REPROCESSING));

    /*
     * 3AA for Reprocessing
     */
    pipeId = INDEX(PIPE_3AA_REPROCESSING);
    previousPipeId = PIPE_FLITE;

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
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_3PAF_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        int fliteNodeNum = getFliteNodenum(m_cameraId);
        m_sensorIds[pipeId][nodeType]  = m_getSensorId(getFliteCaptureNodenum(m_cameraId, fliteNodeNum), false, flagStreamLeader, m_flagReprocessing);
        previousPipeId = INDEX(pipeId3Paf);
    }

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
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_3AA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    if (m_flagPaf3aaOTF == HW_CONNECTION_MODE_OTF || m_flagPaf3aaOTF == HW_CONNECTION_MODE_M2M) {
        m_sensorIds[pipeId][nodeType]  = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(pipeId3Paf)],
                                               m_flagPaf3aaOTF, flagStreamLeader, m_flagReprocessing);
    } else {
        int fliteNodeNum = getFliteNodenum(m_cameraId);
        m_sensorIds[pipeId][nodeType]  = m_getSensorId(getFliteCaptureNodenum(m_cameraId, fliteNodeNum),
                                               false, flagStreamLeader, m_flagReprocessing);
    }

    // Restore leader flag
    flagStreamLeader = false;

    /* 3AC */
    nodeType = getNodeType(PIPE_3AC_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AC_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3ac;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_3AA_CAPTURE_OPT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* 3AP */
    nodeType = getNodeType(PIPE_3AP_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AP_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3ap;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_3AA_CAPTURE_MAIN", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    if (m_parameters->isUse3aaDNG()) {
        /* 3AG */
        node3ag = FIMC_IS_VIDEO_31G_NUM;
        nodeType = getNodeType(PIPE_3AG_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AG_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = node3ag;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_3AA_DNG", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AG_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);
    }

#ifdef SUPPORT_3AF
    /* 3AF */
    nodeType = getNodeType(PIPE_3AF_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AF_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = node3af;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_3AA_FD", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
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
        strncpy(m_deviceInfo[vraPipeId].nodeName[nodeType], "REMOSAIC_VRA_OUTPUT_REPROCESSING", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
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
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_ISP_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
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
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_ISP_CAPTURE_M2M", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_ISP_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /* ISPP */
        nodeType = getNodeType(PIPE_ISPP_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_ISPP_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = nodeIspp;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_ISP_CAPTURE_OTF", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_ISP_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        pipeId = INDEX(PIPE_MCSC_REPROCESSING);
        mcscSrcPipeId = PIPE_ISPC_REPROCESSING;
    }

    /* MCSC */
    nodeType = getNodeType(PIPE_MCSC_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_REPROCESSING;
    m_deviceInfo[pipeId].nodeNum[nodeType] = nodeMcsc;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(mcscSrcPipeId)], m_flagIspMcscOTF, flagStreamLeader, m_flagReprocessing);

    /* MCSC0 */
    nodeType = getNodeType(PIPE_MCSC0_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC0_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC0_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_0];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_CAPTURE_YUV_0", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC1 */
    nodeType = getNodeType(PIPE_MCSC1_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC1_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC1_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_1];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_CAPTURE_YUV_1", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC2 */
    nodeType = getNodeType(PIPE_MCSC2_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC2_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC2_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_2];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_SERVICE_GRALLOC_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_CAPTURE_YUV_2", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC3 */
    nodeType = getNodeType(PIPE_MCSC_JPEG_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_JPEG_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC_JPEG_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_3];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_CAPTURE_MAIN", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

    /* MCSC4 */
    nodeType = getNodeType(PIPE_MCSC_THUMB_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC_THUMB_REPROCESSING;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    m_deviceInfo[pipeId].virtualNodeNum[getNodeType(PIPE_MCSC_THUMB_REPROCESSING)] = virtualNodeMCSC[MCSC_PORT_4];
#endif
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_MCSC_CAPTURE_THUMBNAIL", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_MCSC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

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
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_HWFC_JPEG_SRC", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_HWFC_JPEG_SRC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /* Thumbnail Src */
        nodeType = getNodeType(PIPE_HWFC_THUMB_SRC_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_HWFC_THUMB_SRC_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_HWFC_THUMB_NUM;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_HWFC_THUMBNAIL_SRC", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_HWFC_THUMB_SRC_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /* JPEG Dst */
        nodeType = getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_HWFC_JPEG_DST_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_HWFC_JPEG_NUM;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_HWFC_JPEG_DST", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING)], true, flagStreamLeader, m_flagReprocessing);

        /* Thumbnail Dst  */
        nodeType = getNodeType(PIPE_HWFC_THUMB_DST_REPROCESSING);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_HWFC_THUMB_DST_REPROCESSING;
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_HWFC_THUMB_NUM;
        m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_HWFC_THUMBNAIL_DST", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
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
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_MCSC_DS_REPROCESSING", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
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
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_REPROCESSING_VRA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(vraSrcPipeId)], m_flagMcscVraOTF, flagStreamLeader, m_flagReprocessing);
    }
#endif

    /* JPEG for Reprocessing */
    pipeId = PIPE_JPEG_REPROCESSING;

    nodeType = getNodeType(PIPE_JPEG_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_JPEG_REPROCESSING;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "REMOSAIC_JPEG_REPROCESSING_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_JPEG0_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_JPEG0_REPROCESSING;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REMOSAIC_JPEG_REPROCESSING_CAPTURE0", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    nodeType = getNodeType(PIPE_JPEG1_REPROCESSING);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_JPEG1_REPROCESSING;
    m_deviceInfo[pipeId].bufferManagerType[nodeType] = BUFFER_MANAGER_ION_TYPE;
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "REMOSAIC_JPEG_REPROCESSING_CAPTURE1", EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    /* GSC for Reprocessing */
    m_nodeNums[INDEX(PIPE_GSC_REPROCESSING)][OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;

    /* GSC2 for Reprocessing */
    m_nodeNums[INDEX(PIPE_GSC_REPROCESSING2)][OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;

    /* GSC3 for Reprocessing */
    m_nodeNums[INDEX(PIPE_GSC_REPROCESSING3)][OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;

    /* Remosaic for Reprocessing */
    m_nodeNums[INDEX(PIPE_REMOSAIC_REPROCESSING)][OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;

    return NO_ERROR;
}

status_t ExynosCameraFrameReprocessingFactoryRemosaic::m_constructPipes(void)
{
    CLOGI("");

    int pipeId = -1;

    /* Remosaic Reprocessing */
    pipeId = PIPE_REMOSAIC_REPROCESSING;
#ifdef SUPPORT_REMOSAIC_ROTATION
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    m_pipes[INDEX(pipeId)]->setPipeName("PIPE_REMOSAIC_REPROCESSING");
#endif

    /* 3AA for Reprocessing */
    pipeId = PIPE_3AA_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                &m_deviceInfo[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    m_pipes[INDEX(pipeId)]->setPipeName("PIPE_3AA_REPROCESSING");

    /* ISP for Reprocessing */
    pipeId = PIPE_ISP_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                &m_deviceInfo[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    m_pipes[INDEX(pipeId)]->setPipeName("PIPE_ISP_REPROCESSING");

    /* MCSC for Reprocessing */
    pipeId = PIPE_MCSC_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                &m_deviceInfo[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    m_pipes[INDEX(pipeId)]->setPipeName("PIPE_MCSC_REPROCESSING");

    /* VRA */
    pipeId = PIPE_VRA_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeVRA(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_deviceInfo[INDEX(pipeId)].nodeNum,
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    m_pipes[INDEX(pipeId)]->setPipeName("PIPE_VRA_REPROCESSING");

    /* GSC for Reprocessing */
    pipeId = PIPE_GSC_REPROCESSING;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    m_pipes[INDEX(pipeId)]->setPipeName("PIPE_GSC_REPROCESSING");

    /* GSC2 for Reprocessing */
    pipeId = PIPE_GSC_REPROCESSING2;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    m_pipes[INDEX(pipeId)]->setPipeName("PIPE_GSC_REPROCESSING2");

    /* GSC3 for Reprocessing */
    pipeId = PIPE_GSC_REPROCESSING3;
    m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(
                m_cameraId,
                m_configurations,
                m_parameters,
                m_flagReprocessing,
                m_nodeNums[INDEX(pipeId)],
                m_camIdInfo);
    m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    m_pipes[INDEX(pipeId)]->setPipeName("PIPE_GSC_REPROCESSING3");

    if (m_flagHWFCEnabled == false || (m_flagHWFCEnabled && m_parameters->isHWFCOnDemand())) {
        /* JPEG for Reprocessing */
        pipeId = PIPE_JPEG_REPROCESSING;
        m_pipes[INDEX(pipeId)] = (ExynosCameraPipe*)new ExynosCameraPipeMultipleJpeg(
                    m_cameraId,
                    m_configurations,
                    m_parameters,
                    m_flagReprocessing,
                    m_nodeNums[INDEX(pipeId)],
                    m_camIdInfo,
                    &m_deviceInfo[INDEX(pipeId)]);
        m_pipes[INDEX(pipeId)]->setPipeName("PIPE_JPEG_MULTIPLE_REPROCESSING");
        m_pipes[INDEX(pipeId)]->setPipeId(pipeId);
    }

    CLOGI("pipe ids for reprocessing");
    for (int i = 0; i < MAX_NUM_PIPES; i++) {
        if (m_pipes[i] != NULL) {
            CLOGI("m_pipes[%d] : PipeId(%d)" , i, m_pipes[i]->getPipeId());
        }
    }

    return NO_ERROR;
}



status_t ExynosCameraFrameReprocessingFactoryRemosaic::m_fillNodeGroupInfo(ExynosCameraFrameSP_sptr_t frame)
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
    if (m_supportPureBayerReprocessing == false || m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        pipeId = INDEX(PIPE_ISP_REPROCESSING);
        perframePosition = 0;
        node_group_info_temp = &node_group_info_isp;
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
        virtual_node_group_temp = &virtual_node_group_info;
#endif
        node_group_info_temp->leader.request = 1;
        node_group_info_temp->leader.pixelformat = m_parameters->getBayerFormat(pipeId);
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

#ifdef USE_VRA_FD
    /* VRA */
    if ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) || (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M)) {
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

#ifdef SUPPORT_REMOSAIC_ROTATION
    swapNodeGroupSize(&node_group_info_3aa);
#endif //SUPPORT_REMOSAIC_ROTATION

    frame->storeNodeGroupInfo(&node_group_info_3aa, PERFRAME_INFO_PURE_REPROCESSING_3AA);
#ifdef SUPPORT_VIRTUALFD_REPROCESSING
    frame->storeVirtualNodeInfo(&virtual_node_group_info, PERFRAME_INFO_PURE_REPROCESSING_3AA);
#endif

    if (m_supportPureBayerReprocessing == false || m_flag3aaIspOTF == HW_CONNECTION_MODE_M2M) {
        updateNodeGroupInfo(
                PIPE_ISP_REPROCESSING,
                frame,
                &node_group_info_isp);

#ifdef SUPPORT_REMOSAIC_ROTATION
        swapNodeGroupSize(&node_group_info_isp);
#endif //SUPPORT_REMOSAIC_ROTATION

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

#ifdef USE_VRA_FD
    if ((m_flag3aaVraOTF == HW_CONNECTION_MODE_M2M) || (m_flagMcscVraOTF == HW_CONNECTION_MODE_M2M)) {
        updateNodeGroupInfo(
                PIPE_VRA_REPROCESSING,
                frame,
                &node_group_info_vra);

#ifdef SUPPORT_REMOSAIC_ROTATION
        swapNodeGroupSize(&node_group_info_isp);
#endif //SUPPORT_REMOSAIC_ROTATION

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

}; /* namespace android */
