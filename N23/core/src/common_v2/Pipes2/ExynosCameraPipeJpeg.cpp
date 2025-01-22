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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraPipeJpeg"

#include "ExynosCameraPipeJpeg.h"
#ifdef USE_GIANT_MSCL
#include "hardware/exynos/giant_mscl.h"
#endif
/* For test */
#include "ExynosCameraBuffer.h"

namespace android {

status_t ExynosCameraPipeJpeg::stop(void)
{
    CLOGD("");

    m_jpegEnc.destroy();

    ExynosCameraSWPipe::stop();

    return NO_ERROR;
}

status_t ExynosCameraPipeJpeg::m_destroy(void)
{
    if (m_shot_ext != NULL) {
        delete m_shot_ext;
        m_shot_ext = NULL;
    }

    if (m_gscWrapper != nullptr) {
        m_gscWrapper->destroy();
        m_gscWrapper = nullptr;
    }

    ExynosCameraSWPipe::m_destroy();

    return NO_ERROR;
}

status_t ExynosCameraPipeJpeg::m_run(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    status_t ret = 0;
    ExynosCameraFrameSP_sptr_t newFrame = NULL;
    ExynosCameraFrameEntity *entity = NULL;
    ExynosCameraBuffer srcBuf;
    ExynosCameraBuffer thumbBuf;
    ExynosRect srcRect;
    ExynosCameraBuffer yuvBuf, jpegBuf;
    camera2_stream *shot_stream = nullptr;

    exif_attribute_t exifInfo;
    m_parameters->getFixedExifInfo(&exifInfo);

    CLOGD("wait JPEG pipe inputFrameQ");
    ret = m_inputFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("wait timeout");
        } else {
            CLOGE("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return ret;
    }

    if (newFrame == NULL) {
        CLOGE("new frame is NULL");
        return NO_ERROR;
    }

    memset(m_shot_ext, 0x00, sizeof(struct camera2_shot_ext));

    /* get metaData for make exif info */
    newFrame->getMetaData(m_shot_ext);
    newFrame->getDynamicMeta(m_shot_ext);
    newFrame->getUserDynamicMeta(m_shot_ext);

    /* JPEG Quality, Thumbnail Quality Setting */
    ExynosCameraFrame::jpeg_pipe_info jpegPipeInfo = newFrame->getJpegPipeInfo(getPipeId());
    int jpegQuality = (int)jpegPipeInfo.jpegQuality;
    int thumbnailQuality = (int)jpegPipeInfo.thumbnailQuality;

    /* JPEG, Thumbnail Size Setting */
    ExynosRect pictureRect = jpegPipeInfo.jpegRect;
    ExynosRect thumbnailRect = jpegPipeInfo.thumbnailSize;

    pictureRect.colorFormat = (newFrame->getFrameYuvStallPortUsage() == YUV_STALL_USAGE_PICTURE) ? V4L2_PIX_FMT_NV21 : JPEG_INPUT_COLOR_FMT;
    int jpegformat = (pictureRect.colorFormat == V4L2_PIX_FMT_YUYV) ? V4L2_PIX_FMT_JPEG_422 : V4L2_PIX_FMT_JPEG_420;

    CFLOGD(newFrame, "JPEG pipe inputFrameQ output done. srcFormat %#x, jpegFormat %#x",
            pictureRect.colorFormat, jpegformat);

    ret = newFrame->getSrcBuffer(getPipeId(), &srcBuf);
    if (ret < 0) {
        CLOGE("frame get src buffer fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        goto func_exit;
    }

    yuvBuf = srcBuf;
    shot_stream = (camera2_stream*)srcBuf.addr[srcBuf.getMetaPlaneIndex()];
    srcRect.x = shot_stream->output_crop_region[0];
    srcRect.y = shot_stream->output_crop_region[1];
    srcRect.w = shot_stream->output_crop_region[2];
    srcRect.h = shot_stream->output_crop_region[3];
    srcRect.colorFormat = pictureRect.colorFormat;

    if (srcRect.w == 0 || srcRect.h == 0) {
        CFLOGE(newFrame, "invalid shot_stream info[%d,%d,%d,%d]",
                srcRect.x, srcRect.y, srcRect.w, srcRect.h);
        srcRect = pictureRect;
        CFLOGE(newFrame, "So, set to srcRect as pictureRect [%d,%d,%d,%d]",
                srcRect.x, srcRect.y, srcRect.w, srcRect.h);
    }

    if ((thumbnailRect.w != 0 && thumbnailRect.h != 0) &&
            (pictureRect.w < 320 || pictureRect.h < 240)) {
        thumbnailRect.w = 160;
        thumbnailRect.h = 120;
    }
    thumbnailRect.colorFormat = pictureRect.colorFormat;

    CLOGD("src(%dx%d), picture size(%dx%d), thumbnail size(%dx%d)",
            srcRect.w, srcRect.h,
            pictureRect.w, pictureRect.h, thumbnailRect.w, thumbnailRect.h);

    m_prepareYuv(newFrame, srcBuf, &yuvBuf, &thumbBuf, srcRect, &pictureRect, &thumbnailRect);

    entity = newFrame->searchEntityByPipeId(getPipeId());
    if (entity == NULL || entity->getSrcBufState() == ENTITY_BUFFER_STATE_ERROR) {
        CLOGE("frame(%d) entityState(ENTITY_BUFFER_STATE_ERROR), skip jpeg", newFrame->getFrameCount());
        goto func_exit;
    }

    ret = newFrame->getDstBuffer(getPipeId(), &jpegBuf);
    if (ret < 0) {
        CLOGE("frame get dst buffer fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        goto func_exit;
    }

    if (m_jpegEnc.create()) {
        CLOGE("m_jpegEnc.create() fail");
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    m_jpegEnc.setExtScalerNum(m_parameters->getScalerNodeNumPicture());

    {
        if (m_jpegEnc.setQuality(jpegQuality)) {
            CLOGE("m_jpegEnc.setQuality() fail");
            ret = INVALID_OPERATION;
            goto jpeg_encode_done;
        }
    }

    if (m_jpegEnc.setSize(pictureRect.w, pictureRect.h)) {
        CLOGE("m_jpegEnc.setSize() fail");
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if (m_jpegEnc.setColorFormat(pictureRect.colorFormat)) {
        CLOGE("m_jpegEnc.setColorFormat() fail");
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if (m_jpegEnc.setJpegFormat(jpegformat)) {
        CLOGE("m_jpegEnc.setJpegFormat() fail");
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if ((thumbnailRect.w != 0 && thumbnailRect.h != 0)) {
        exifInfo.enableThumb = true;
        if (m_jpegEnc.setThumbnailSize(thumbnailRect.w, thumbnailRect.h)) {
            CLOGE("m_jpegEnc.setThumbnailSize(%d, %d) fail", thumbnailRect.w, thumbnailRect.h);
            ret = INVALID_OPERATION;
            goto jpeg_encode_done;
        }
        if (0 < thumbnailQuality && thumbnailQuality <= 100) {
            if (m_jpegEnc.setThumbnailQuality(thumbnailQuality)) {
                ret = INVALID_OPERATION;
                CLOGE("m_jpegEnc.setThumbnailQuality(%d) fail", thumbnailQuality);
            }
        }

#ifdef MAKE_THUMBNAIL
        if (thumbBuf.index >= 0) {
            if (m_jpegEnc.setInBuf2((int *)&(thumbBuf.fd), (int *)thumbBuf.size)) {
                CLOGE("m_jpegEnc.setInBuf2() fail");
                exifInfo.enableThumb = false;
            }
        }
#endif

    } else {
        exifInfo.enableThumb = false;
    }

    /* wait for medata update */
    if(newFrame->getMetaDataEnable() == false) {
        CLOGD(" Waiting for update jpeg metadata failed (%d) ", ret);
    }

    m_parameters->setExifChangedAttribute(&exifInfo, &pictureRect, &thumbnailRect, m_shot_ext);

    if (m_jpegEnc.setInBuf((int *)&(yuvBuf.fd), (int *)yuvBuf.size)) {
        CLOGE("m_jpegEnc.setInBuf() fail");
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if (m_jpegEnc.setOutBuf(jpegBuf.fd[0], jpegBuf.size[0])) {
        CLOGE("m_jpegEnc.setOutBuf() fail");
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if (m_jpegEnc.updateConfig()) {
        CLOGE("m_jpegEnc.updateConfig() fail");
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    if (m_jpegEnc.encode((int *)&jpegBuf.size, &exifInfo, (char **)jpegBuf.addr, m_parameters->getDebugAttribute())) {
        CLOGE("m_jpegEnc.encode() fail");
        ret = INVALID_OPERATION;
        goto jpeg_encode_done;
    }

    newFrame->setJpegEncodeSize(getPipeId(), jpegBuf.size[0]);

    ret = newFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret < 0) {
        CLOGE("set entity state fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        goto func_exit;
    }

    m_outputFrameQ->pushProcessQ(&newFrame);

jpeg_encode_done:
    if (ret != NO_ERROR) {
        CLOGD("[jpegBuf.fd[0] %d][jpegBuf.size[0] %d", jpegBuf.fd[0], jpegBuf.size[0]);
        CLOGD("[pictureW %d][pictureH %d][pictureFormat %d]",
            pictureRect.w, pictureRect.h, pictureRect.colorFormat);
    }

    if (m_jpegEnc.flagCreate() == true)
        m_jpegEnc.destroy();

    if (yuvBuf.reserved.u8[0] == 1) {
        ret = m_bufferSupplier->putBuffer(yuvBuf);
        if (ret != NO_ERROR) {
            CLOGE("[YUV_ROT_BUF][B%d]Failed to putBuffer. ret %d",
                    yuvBuf.index, ret);
        }
    }

    if (thumbBuf.reserved.u8[0] == 1) {
        ret = m_bufferSupplier->putBuffer(thumbBuf);
        if (ret != NO_ERROR) {
            CLOGE("[YUV_ROT_BUF][B%d]Failed to putBuffer. ret %d",
                    thumbBuf.index, ret);
        }
        CLOGD("Release Buffer, index(%d), fd(%d)", thumbBuf.index, thumbBuf.fd[0]);
    }

    CLOGI(" -OUT-");
    return ret;

func_exit:

    ret = newFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret < 0) {
        CLOGE("set entity state fail, ret(%d)", ret);
        /* TODO: doing exception handling */
    }

    m_outputFrameQ->pushProcessQ(&newFrame);

    if (yuvBuf.reserved.u8[0] == 1) {
        ret = m_bufferSupplier->putBuffer(yuvBuf);
        if (ret != NO_ERROR) {
            CLOGE("[YUV_ROT_BUF][B%d]Failed to putBuffer. ret %d",
                    yuvBuf.index, ret);
        }
        CLOGD("Release Buffer, index(%d), fd(%d)", yuvBuf.index, yuvBuf.fd[0]);
    }

    if (thumbBuf.reserved.u8[0] == 1) {
        ret = m_bufferSupplier->putBuffer(thumbBuf);
        if (ret != NO_ERROR) {
            CLOGE("[YUV_ROT_BUF][B%d]Failed to putBuffer. ret %d",
                    thumbBuf.index, ret);
        }
        CLOGD("Release Buffer, index(%d), fd(%d)", thumbBuf.index, thumbBuf.fd[0]);
    }

    CLOGI(" -OUT-");
    return NO_ERROR;
}

void ExynosCameraPipeJpeg::m_init(void)
{
    m_reprocessing = 1;
    m_shot_ext = new struct camera2_shot_ext;

    if (m_parameters->supportJpegYuvRotation() == true) {
        /* TODO: FIXME: set proper gsc number */
        m_gscWrapper = new ExynosCameraGSCWrapper(PREVIEW_GSC_NODE_NUM);
        if (m_gscWrapper == nullptr) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Fail to new gscWrapper instance. so, assert!!!!",
                    __FUNCTION__, __LINE__);
        }

        if (m_gscWrapper->create() != NO_ERROR) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Fail to create gscWrapper so, assert!!!!",
                    __FUNCTION__, __LINE__);
        }
    }
}

status_t ExynosCameraPipeJpeg::m_prepareYuv(ExynosCameraFrameSP_dptr_t newFrame,
                                    ExynosCameraBuffer srcBuf,
                                    ExynosCameraBuffer *mainBuf,
                                    ExynosCameraBuffer *thumbBuf,
                                    ExynosRect srcRect,
                                    ExynosRect *mainRect,
                                    ExynosRect *thumbRect)
{
    status_t ret = 0;
    ExynosCameraFrame::jpeg_pipe_info jpegPipeInfo = newFrame->getJpegPipeInfo(getPipeId());

    ret = m_getMainImage(newFrame, srcBuf, mainBuf, srcRect, mainRect);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to m_getMainImage. ret %d",
                newFrame->getFrameCount(), ret);
        return false;
    }

#ifdef MAKE_THUMBNAIL
    ret = m_getThumbImage(newFrame, srcBuf, thumbBuf, srcRect, thumbRect);
    if (ret != NO_ERROR) {
        CLOGE("[F%d]Failed to m_getThumbImage. ret %d",
                newFrame->getFrameCount(), ret);
        return false;
    }
#endif

    if (jpegPipeInfo.rotation == true)
        m_shot_ext->shot.ctl.jpeg.orientation = 0;

    return ret;
}

status_t ExynosCameraPipeJpeg::m_getMainImage(ExynosCameraFrameSP_dptr_t newFrame,
                                    ExynosCameraBuffer srcBuf,
                                    ExynosCameraBuffer *dstBuf,
                                    ExynosRect srcRect,
                                    ExynosRect *dstRect)
{
    status_t ret = 0;
    ExynosCameraBuffer tmpBuf;
    ExynosRect tmpRect;
    bool rotated = false;
    bool isSameSize = true;
    ExynosCameraFrame::jpeg_pipe_info jpegPipeInfo = newFrame->getJpegPipeInfo(getPipeId());

    if (srcRect.w != dstRect->w || srcRect.h != dstRect->h) {
        CFLOGD(newFrame, "need to process scale up/down src(%dx%d)->dst(%dx%d)",
                srcRect.w, srcRect.h, dstRect->w, dstRect->h);
        isSameSize = false;
    }

    tmpBuf = srcBuf;
    tmpRect = srcRect;

    /* CASE1: scale up for 64MP */
    if (!isSameSize) {
        // need giant
        // TODO: need to change the size condition
        if (dstRect->w == 9216 && dstRect->h == 6912) {
#ifdef USE_GIANT_MSCL
            buffer_manager_tag_t bufTag;
            bufTag.pipeId[0] = getPipeId();
            bufTag.managerType = BUFFER_MANAGER_ONLY_HAL_USE_ION_TYPE;

            ret = m_bufferSupplier->getBuffer(bufTag, &tmpBuf);
            if (ret != NO_ERROR) {
                CFLOGE(newFrame, "Failed to getBuffer from BufferSupplier. ret %d", ret);
                return false;
            }
            /* set flag to release */
            tmpBuf.reserved.u8[0] = 1;
            CFLOGD(newFrame, "Aquire Buffer, index(%d), fd(%d)", tmpBuf.index, tmpBuf.fd[0]);

            m_giantScale(newFrame, srcBuf, tmpBuf, srcRect, dstRect);
            tmpRect = dstRect;
            isSameSize = true;
#else
            CFLOG_ASSERT(newFrame, "Not define GIANT_MSCL");
#endif
        }
    }

    /* CASE2: Rotation / Not Same Size / Flip if need */
    if (jpegPipeInfo.rotation == true
            || !isSameSize
            || newFrame->getFlipHorizontal(getPipeId()) == true
            || newFrame->getFlipVertical(getPipeId()) == true) {
        /* Rotate JPEG source buffer if needed */
        rotated = m_rotate(newFrame, &tmpBuf, dstBuf, &tmpRect, dstRect);
        if (rotated == true) {
            if (tmpBuf.reserved.u8[0] == 1) {
                ret = m_bufferSupplier->putBuffer(tmpBuf);
                if (ret != NO_ERROR) {
                    CLOGE("[YUV_ROT_BUF][B%d]Failed to putBuffer. ret %d",
                            tmpBuf.index, ret);
                }
                CLOGD("Release Buffer, index(%d), fd(%d)", tmpBuf.index, tmpBuf.fd[0]);
            }
        } else {
            *dstBuf = tmpBuf;
        }
    } else {
        *dstBuf = tmpBuf;
    }

    return ret;
}

status_t ExynosCameraPipeJpeg::m_getThumbImage(ExynosCameraFrameSP_dptr_t newFrame,
                                    ExynosCameraBuffer srcBuf,
                                    ExynosCameraBuffer *dstBuf,
                                    ExynosRect srcRect,
                                    ExynosRect *dstRect)
{
    status_t ret = 0;
    bool rotated = false;
    rotated = m_rotate(newFrame, &srcBuf, dstBuf, &srcRect, dstRect);

    return ret;
}

bool ExynosCameraPipeJpeg::m_rotate(ExynosCameraFrameSP_dptr_t newFrame,
                                    ExynosCameraBuffer *srcBuf,
                                    ExynosCameraBuffer *dstBuf,
                                    ExynosRect *srcRect,
                                    ExynosRect *dstRect)
{
    int ret = 0;
    int rotation = 0;
    int needRotation = newFrame->getRotation(getPipeId());
    int flipHorizontal = newFrame->getFlipHorizontal(getPipeId());
    int flipVertical = newFrame->getFlipVertical(getPipeId());

    int pipeId = getPipeId();
    struct ExynosCameraSensorInfoBase *sensorInfo = m_parameters->getSensorStaticInfo();

    if (m_gscWrapper == nullptr) {
        CLOGE("m_gscWrapper is not created");
        goto func_exit;
    }

    if (srcBuf == nullptr || dstBuf == nullptr || srcRect == nullptr) {
        CLOGE("Invalid arguments");
        goto func_exit;
    }

    srcRect->fullW = srcRect->w;
    srcRect->fullH = srcRect->h;

    dstRect->fullW = dstRect->w;
    dstRect->fullH = dstRect->h;

    if (needRotation) {
        if (sensorInfo->orientation == BACK_ROTATION) {
            rotation = m_shot_ext->shot.ctl.jpeg.orientation;
        } else {
            switch (m_shot_ext->shot.ctl.jpeg.orientation) {
                case 0:
                    rotation = 180;
                    break;
                case 180:
                    rotation = 0;
                    break;
                case 90:
                case 270:
                default:
                    rotation = m_shot_ext->shot.ctl.jpeg.orientation;
                    break;
            }
        }

        /* if need rotation */
        if (rotation == 0 && srcRect == dstRect) {
            CLOGI("skip, rotation(%d), srcRect and dstRect is same (%d %d %d %d)",
                    rotation, srcRect->x, srcRect->y, srcRect->w, srcRect->h);
            goto func_exit;
        }

        if (rotation == 90 || rotation == 270) {
            SWAP(int, dstRect->x, dstRect->y);
            SWAP(int, dstRect->w, dstRect->h);
            SWAP(int, dstRect->fullW, dstRect->fullH);
        }
    }

    {
        buffer_manager_tag_t bufTag;
        bufTag.pipeId[0] = pipeId;
        bufTag.managerType = BUFFER_MANAGER_ONLY_HAL_USE_ION_TYPE;
#ifdef SUPPORT_OPTIMIZED_REMOSAIC_BUFFER_ALLOCATION
        if (newFrame->getFrameType() == FRAME_TYPE_REPROCESSING_SENSOR_TRANSITION) {
            bufTag.managerType = BUFFER_MANAGER_REMOSAIC_ONLY_HAL_USE_ION_TYPE;
        }
#endif
        ret = m_bufferSupplier->getBuffer(bufTag, dstBuf);
        if (ret != NO_ERROR) {
            CLOGE("[F%d]Failed to getBuffer from BufferSupplier. ret %d",
                    newFrame->getFrameCount(), ret);
            goto func_exit;
        }
        CLOGD("Aquire Buffer, index(%d), fd(%d)", dstBuf->index, dstBuf->fd[0]);
        dstBuf->reserved.u8[0] = 1;
    }

    CLOGD("pipeId(%d):(rot:%d, flipH:%d, flipV:%d) " \
          "srcBuf.index(%d) dstBuf.index(%d) " \
          "(size:%d, %d, %d x %d) format(%c%c%c%c) actual(%x) fd(%d) addr(%p)-> " \
          "(size:%d, %d, %d x %d) format(%c%c%c%c) actual(%x) fd(%d) addr(%p)",
            pipeId,
            rotation,
            flipHorizontal, flipVertical,
            srcBuf->index, dstBuf->index,
            srcRect->x, srcRect->y, srcRect->w, srcRect->h,
            v4l2Format2Char(srcRect->colorFormat, 0),
            v4l2Format2Char(srcRect->colorFormat, 1),
            v4l2Format2Char(srcRect->colorFormat, 2),
            v4l2Format2Char(srcRect->colorFormat, 3),
            V4L2_PIX_2_HAL_PIXEL_FORMAT(srcRect->colorFormat),
            srcBuf->fd[0], srcBuf->addr[0],
            dstRect->x, dstRect->y, dstRect->w, dstRect->h,
            v4l2Format2Char(dstRect->colorFormat, 0),
            v4l2Format2Char(dstRect->colorFormat, 1),
            v4l2Format2Char(dstRect->colorFormat, 2),
            v4l2Format2Char(dstRect->colorFormat, 3),
            V4L2_PIX_2_HAL_PIXEL_FORMAT(dstRect->colorFormat),
            dstBuf->fd[0], dstBuf->addr[0]);

    // need giant
    // TODO: need to change the size condition
    if (srcRect->w == 9216 && srcRect->h == 6912) {
#ifdef USE_GIANT_MSCL
        m_giantScale(newFrame, *srcBuf, *dstBuf, srcRect, dstRect,
                    rotation ==  90 ? HAL_TRANSFORM_ROT_90:
                    rotation == 180 ? HAL_TRANSFORM_ROT_180:
                    rotation == 270 ? HAL_TRANSFORM_ROT_270:
                    0);
#else
        CFLOG_ASSERT(newFrame, "Not define GIANT_MSCL");
#endif
    } else {
        ret = m_gscWrapper->convertWithRotation(*srcRect, dstRect, *srcBuf, *dstBuf, rotation, flipHorizontal, flipVertical);
        if (ret != NO_ERROR) {
            CLOGE("convertWithRoration fail, ret(%d)", ret);
            goto func_exit;
        }
    }

    return true;

func_exit:
    return false;
}

#ifdef USE_GIANT_MSCL
bool ExynosCameraPipeJpeg::m_giantScale(ExynosCameraFrameSP_dptr_t newFrame,
                                            ExynosCameraBuffer srcBuf,
                                            ExynosCameraBuffer dstBuf,
                                            ExynosRect srcRect,
                                            ExynosRect dstRect,
                                            int rotation)
{
    status_t ret = 0;

    GiantMscl mscl;
    CLOGD("[64M] srcRect width : %d hight : %d", srcRect.w, srcRect.h);
    CLOGD("[64M] dstRect width : %d hight : %d", dstRect.w, dstRect.h);
    CLOGD("[64M] format 0x%x ,%d", V4L2_PIX_2_HAL_PIXEL_FORMAT(srcRect.colorFormat), V4L2_PIX_2_HAL_PIXEL_FORMAT(srcRect.colorFormat));

    if(!mscl){
        CLOGE("[64M] mscl construct fail");
        return false;
    }

    if(!mscl.setSrc(srcRect.w, srcRect.h, V4L2_PIX_2_HAL_PIXEL_FORMAT(srcRect.colorFormat), rotation)){
        CLOGE("[64M] mscl  setsrc fail");
        return false;
    }

    if(!mscl.setDst(dstRect.w, dstRect.h, V4L2_PIX_2_HAL_PIXEL_FORMAT(srcRect.colorFormat))){
        CLOGE( "[64M] mscl  setDst fail");
        return false;
    }

    if(!mscl.run(srcBuf.fd, dstBuf.fd)){
        CLOGE( "[64M] mscl  run fail");
        return false;
    }
    CLOGD("[64M] Good");

    return false;
}
#endif

}; /* namespace android */
