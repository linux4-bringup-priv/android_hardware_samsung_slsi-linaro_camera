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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraRequestManagerSec"

#include "ExynosCameraRequestManager.h"

namespace android {

ExynosCameraRequestSP_sprt_t ExynosCameraRequestManager::registerToServiceList(camera3_capture_request_t *srcRequest,
                                                                                            bool skipRequest)
{
    status_t ret = NO_ERROR;
    ExynosCameraRequestSP_sprt_t request = NULL;
    CameraMetadata *meta;
    CameraMetadata *metaPhysCam = NULL;
    uint32_t bufferCnt = 0;
    camera3_stream_buffer_t *inputbuffer = NULL;
    const camera3_stream_buffer_t *outputbuffer = NULL;
    ExynosCameraStream *stream = NULL;
    ExynosCameraFrameFactory *factory = NULL;
    int32_t streamID = 0;
    int32_t factoryID = 0;
    int reqId;
    bool isZslInput = false;
    int32_t *physCamIDs;
    struct camera2_shot_ext *shot_ext = NULL;
    int previewPortId = MCSC_PORT_NONE;
    int yuvPortId = MCSC_PORT_NONE;
    int tempPortId = MCSC_PORT_NONE;
    int jpegPortId = MCSC_PORT_NONE;
    int yuvStallPortId = MCSC_PORT_NONE;

    /* Check whether the input buffer (ZSL input) is specified.
       Use zslFramFactory in the following section if ZSL input is used
    */
    request = new ExynosCameraRequest(srcRequest, m_previousMeta, m_previousMetaPhysCam);

    ////////////////////////////////////////////////
    // get current meta, using m_previousMeta
    m_converter->setPreviousMeta(&m_previousMeta);

    meta = request->getServiceMeta();
    if(meta->isEmpty()) {
        CLOGD("meta is EMPTY");
    } else {
        CLOGV("meta is NOT EMPTY");
    }

    ret = m_converter->convertRequestToShot(request, &reqId);
    if (ret != NO_ERROR){
        CLOGE("convertRequestToShot([R%d]) fail", request->getKey());
        return NULL;
    }

    request->setRequestId(reqId);

    for (size_t i = 0; i < request->getNumOfPhysCamSettings(); i++) {
        physCamIDs = request->getAllPhysCamInternalIDs();
        if ((physCamIDs[i] < CAMERA_ID_MAX) && (physCamIDs[i] >= 0))
            m_converter->setPreviousMetaPhysCam(&m_previousMetaPhysCam[physCamIDs[i]], physCamIDs[i]);
        ret = m_converter->convertRequestToShot(request, &reqId, physCamIDs[i]);
        if (ret != OK) {
            CLOGE("convertRequestToShot is failed : physCamId = %d", physCamIDs[i]);
            //fall back
        }

        if ((physCamIDs[i] < CAMERA_ID_MAX) && (physCamIDs[i] >= 0)) {
            metaPhysCam = request->getServiceMetaPhysCam(physCamIDs[i]);
            m_previousMetaPhysCam[physCamIDs[i]] = *metaPhysCam;
        }
    }

    m_previousMeta = *meta;

    shot_ext = request->getServiceShot();
    if (shot_ext == NULL){
        CLOGE("shot_ext == NULL. request[R%d]->getServiceShot() fail", request->getKey());
        return NULL;
    }

    ////////////////////////////////////////////////
    // get each frameFactory
    bufferCnt = request->getNumOfInputBuffer();
    inputbuffer = request->getInputBuffer();
    for(uint32_t i = 0 ; i < bufferCnt ; i++) {
        stream = static_cast<ExynosCameraStream*>(inputbuffer[i].stream->priv);
        stream->getID(&streamID);
        factoryID = streamID % HAL_STREAM_ID_MAX;

        /* Stream ID validity */
        if(factoryID == HAL_STREAM_ID_ZSL_INPUT
           || factoryID == HAL_STREAM_ID_YUV_INPUT) {
            isZslInput = true;
        } else {
            /* Ignore input buffer */
            CLOGE("Invalid input streamID. streamID(%d)", streamID);
        }
        request->pushRequestInputStreams(streamID);
    }

    bool vendorYuvStall = false;
#ifdef SUPPORT_VENDOR_YUV_STALL
    vendorYuvStall = getVendorYUVStallMeta(request);
    if (isZslInput || vendorYuvStall) {
        isZslInput = true;
    }
#endif

    bufferCnt = request->getNumOfOutputBuffer();
    outputbuffer = request->getOutputBuffers();
    for(uint32_t i = 0 ; i < bufferCnt ; i++) {
        stream = static_cast<ExynosCameraStream*>(outputbuffer[i].stream->priv);
        stream->getID(&streamID);
        factoryID = streamID % HAL_STREAM_ID_MAX;

        /* If current request has ZSL Input stream buffer,
         * CALLBACK stream must be processed by reprocessing stream.
         */
        if (isZslInput == true && factoryID == HAL_STREAM_ID_CALLBACK
#ifdef ENABLE_YUV_STALL_FOR_SECOND_YUV
                // HACK: for second yuv
                // TODO: need more condition for specific vendor tag
                && (streamID != HAL_STREAM_ID_CALLBACK)
#endif
           ) {
            CLOGV("[R%d]CALLBACK stream will be replaced with CALLBACK_STALL stream", request->getKey());

            factoryID = HAL_STREAM_ID_CALLBACK_STALL;
        }
        ret = m_getFactory(factoryID, &factory, &m_factoryMap, &m_factoryMapLock);
        if (ret < 0) {
            CLOGD("[R%d]m_getFactory is failed. streamID(%d)",
                request->getKey(), streamID);
        }

        stream->getOutputPortId(&tempPortId);

        switch(factoryID % HAL_STREAM_ID_MAX) {
        case HAL_STREAM_ID_PREVIEW:
            if (previewPortId < 0) {
                previewPortId = tempPortId;
            } else {
                previewPortId = (previewPortId > tempPortId) ? tempPortId : previewPortId;
            }
            break;
        case HAL_STREAM_ID_VIDEO:
        case HAL_STREAM_ID_CALLBACK:
        case HAL_STREAM_ID_CALLBACK_PHYSICAL:
            if (yuvPortId < 0) {
                yuvPortId = tempPortId;
            } else {
                yuvPortId = (yuvPortId > tempPortId) ? tempPortId : yuvPortId;
            }
            break;
        case HAL_STREAM_ID_CALLBACK_STALL:
            if (yuvStallPortId < 0) {
                yuvStallPortId = tempPortId;
                yuvStallPortId = yuvStallPortId + MCSC_PORT_MAX;
            } else {
                yuvStallPortId = yuvStallPortId - MCSC_PORT_MAX;
                yuvStallPortId = (yuvStallPortId > tempPortId) ? tempPortId : yuvStallPortId;
                yuvStallPortId = yuvStallPortId + MCSC_PORT_MAX;
            }
            break;
        case HAL_STREAM_ID_JPEG:
            jpegPortId = MCSC_PORT_3 + MCSC_PORT_MAX;
            break;
        }

        if (request->findFrameFactory(streamID) == false) {
            request->pushFrameFactory(streamID, factory);
        }

        request->pushRequestOutputStreams(streamID);
#if 0
        ////////////////////////////////////////////////
        // setFlip info on meta.
        bool flipH = request->getFlipHorizontal(streamID);
        bool flipV = request->getFlipVertical(streamID);

        CLOGV("[R%d] flip info : bufferCnt[%d]:i[%d] streamID(%d), tempPortId(%d), flipH(%d), flipV(%d)",
            request->getKey(),
            bufferCnt, i, streamID, tempPortId, flipH, flipV);

        if (tempPortId <= MCSC_PORT_NONE || MCSC_PORT_MAX <= tempPortId) {
            CLOGE("[R%d] Invalid tempPortId(%d). so, fail. just skip flip : bufferCnt[%d]:i[%d] streamID(%df), flipH(%d), flipV(%d)",
                request->getKey(),
                tempPortId,
                bufferCnt,
                i,
                streamID,
                flipH,
                flipV);
        } else {
            enum camera_flip_mode flipMode = CAM_FLIP_MODE_NORMAL;

            if (flipH == true && flipV == true) {
                flipMode = CAM_FLIP_MODE_HORIZONTAL_VERTICAL;
            } else if (flipH == true) {
                flipMode = CAM_FLIP_MODE_HORIZONTAL;
            } else if(flipV == true) {
                flipMode = CAM_FLIP_MODE_VERTICAL;
            } else {
                flipMode = CAM_FLIP_MODE_NORMAL;
            }

            setMetaMcscFlip(shot_ext, (enum mcsc_port)tempPortId, flipMode);
        }
#endif
        ////////////////////////////////////////////////
    }

    CLOGV("m_currReqeustList size(%zu), fn(%d)",
        m_serviceRequests.size(), request->getFrameCount());

    if (shot_ext->shot.ctl.stats.faceDetectMode == FACEDETECT_MODE_OFF) {
        request->setDsInputPortId(MCSC_PORT_NONE);
    } else {
        if (previewPortId >= 0) {
            request->setDsInputPortId(previewPortId);
        } else if (yuvPortId >= 0) {
            request->setDsInputPortId(yuvPortId);
        }
#ifndef CAPTURE_FD_SYNC_WITH_PREVIEW
        else if (jpegPortId >= 0) {
            request->setDsInputPortId(jpegPortId);
        } else {
            request->setDsInputPortId(yuvStallPortId);
        }
#endif
    }

    if (skipRequest == true) {
        return request;
    }

    if (m_configurations->getRestartStream() == false) {
        ret = m_pushBack(request, &m_serviceRequests, &m_requestLock);
        if (ret < 0){
            CLOGE("request m_pushBack is failed request(%d)", request->getFrameCount());
            request = NULL;
            return NULL;
        }
    } else {
        CLOGD("restartStream flag checked, request[R%d]", request->getKey());
    }

    if (m_getFlushFlag() == false && m_resultNotifyCallbackThread->isRunning() == false) {
        m_resultNotifyCallbackThread->run();
    }

    if (m_getFlushFlag() == false && m_resultStreamCallbackThread->isRunning() == false) {
        m_resultStreamCallbackThread->run();
    }

    if (m_getFlushFlag() == false && m_resultMetaCallbackThread->isRunning() == false) {
        m_resultMetaCallbackThread->run();
    }

    if (m_getFlushFlag() == false && m_callbackThread->isRunning() == false) {
        m_callbackThread->run();
    }

    return request;
}

void ExynosCameraRequest::m_copyAndEraseTagFromeServiceMetaDataU8(uint32_t tag, CameraMetadata &resultMeta)
{
    camera_metadata_entry_t entry;

    entry = m_serviceMeta.find(tag);
    if (entry.count > 0) {
        resultMeta.update(tag, entry.data.u8, entry.count);

        for (size_t i = 0; i < entry.count; i++) {
            CLOGV2("%s[%zu/%zu] is (%d)", get_camera_metadata_tag_name(tag), i, entry.count, entry.data.u8[i]);
        }
    }
    // should remove the duplication tag frome Servivce meta
    m_serviceMeta.erase(tag);
}

void ExynosCameraRequest::m_copyAndEraseTagFromeServiceMetaDataI32(uint32_t tag, CameraMetadata &resultMeta)
{
    camera_metadata_entry_t entry;

    entry = m_serviceMeta.find(tag);
    if (entry.count > 0) {
        resultMeta.update(tag, entry.data.i32, entry.count);

        for (size_t i = 0; i < entry.count; i++) {
            CLOGV2("%s[%zu/%zu] is (%d)", get_camera_metadata_tag_name(tag), i, entry.count, entry.data.i32[i]);
        }
    }
    // should remove the duplication tag frome Servivce meta
    m_serviceMeta.erase(tag);
}

void ExynosCameraRequest::get3AAResultMetaVendor(CameraMetadata &minimal_resultMeta)
{
    m_copyAndEraseTagFromeServiceMetaDataU8(ANDROID_CONTROL_AE_MODE, minimal_resultMeta);
    m_copyAndEraseTagFromeServiceMetaDataU8(ANDROID_CONTROL_AF_TRIGGER, minimal_resultMeta);

    m_copyAndEraseTagFromeServiceMetaDataI32(ANDROID_CONTROL_AF_REGIONS, minimal_resultMeta);
    m_copyAndEraseTagFromeServiceMetaDataI32(ANDROID_CONTROL_AE_REGIONS, minimal_resultMeta);
    m_copyAndEraseTagFromeServiceMetaDataI32(ANDROID_CONTROL_AWB_REGIONS, minimal_resultMeta);

    return;
}
}; /* namespace android */
