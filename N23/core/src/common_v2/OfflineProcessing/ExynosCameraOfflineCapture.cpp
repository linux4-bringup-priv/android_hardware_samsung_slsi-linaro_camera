/*
 * Copyright 2017, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "OFFLINE"

#include <log/log.h>

#include "ExynosCameraVendorTags.h"
#include "ExynosCameraRequestManager.h"
#include "ExynosCameraCounter.h"
#include "ExynosCameraConfigurations.h"
#include "ExynosCameraRequestManager.h"
#include "ExynosCameraOfflineCapture.h"

#ifdef ADDED_VOP_HIDL
#include "CameraVOPIf.h"
#endif

#define STR_IMAGE_READER_ID ((char*)"imageReaderId")
#define STR_CAMERA_SESSION_ID ((char*)"sessionId")

#define RESULT_NEXT_OPERATION_ALWAYS_ON (true)

namespace android {

ExynosCameraIdentifier::ExynosCameraIdentifier()
{
    CLOGD2("");
}

ExynosCameraIdentifier::~ExynosCameraIdentifier()
{
    CLOGD2("");

    m_customIdList.clear();
}

status_t ExynosCameraIdentifier::registerCustomIdentification(char* id_name, uint64_t value)
{
    std::string strCustomId(id_name);
    m_customIdList.insert(pair<std::string, uint64_t>(strCustomId, value));

    return NO_ERROR;
}

uint64_t ExynosCameraIdentifier::getCustomIdentification(char* id_name)
{
    uint64_t value = 0;

    std::string strCustomId(id_name);

    map<std::string, uint64_t>::iterator iter;

    iter = m_customIdList.find(strCustomId);
    if (iter != m_customIdList.end()) {
        value = iter->second;
    }

    return value;
}

ExynosCameraOfflineCapture::ExynosCameraDummyRequest::ExynosCameraDummyRequest(camera3_capture_request_t* request,
                                                                                            CameraMetadata &meta)
                                                                                            : ExynosCameraRequest(request, meta, nullptr)
{
    CLOGD2("");
}

ExynosCameraOfflineCapture::ExynosCameraDummyRequest::~ExynosCameraDummyRequest()
{
    CLOGD2("");
}

ExynosCameraOfflineCapture::ExynosCameraOfflineCapture(int cameraId)
{
    m_cameraId = cameraId;
    CLOGD("");
        
    strncpy(m_name, "ExynosCameraOfflineCapture", (EXYNOS_CAMERA_NAME_STR_SIZE-1));

    m_dummyRequest = NULL;

    m_cameraSessionIdMap.clear();
    m_requestKeyMap.clear();
    m_requestInfoMap.clear();

    for (int i = 0; i < CAMERA_SESSION_MAX; i++) {
        m_flagSessionFlushed[i] = false;
    }
}

ExynosCameraOfflineCapture::~ExynosCameraOfflineCapture()
{
    CLOGD("");

    m_cameraSessionIdMap.clear();
    m_requestKeyMap.clear();
    m_requestInfoMap.clear();
}

int ExynosCameraOfflineCapture::configureCameraSession(const camera_metadata_t * session_param)
{
    CLOGD("");

    camera_metadata_entry_t entry;
    uint32_t enable = 0;
    uint32_t imageReaderId = 0;

    CameraMetadata sessionParam;

    const exynos_ext_tags sessionKey_enable = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_SESSION_ENABLE;
    const exynos_ext_tags sessionKey_imageReaderId = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_SESSION_IMAGE_READER_ID;

    sessionParam = session_param;

    entry = sessionParam.find(sessionKey_enable);
    if (entry.count > 0) {
        enable = entry.data.i32[0];
    } else {
        CLOGD("vendorTag(OFFLINE_CAPTURE_SESSION_ENABLE) is NOT defined");
    }

    /* increase cameraSessionId */
    m_cameraSessionId.incCount();

    if (m_cameraSessionId.getCount() >= 5) {
        m_cameraSessionId.setCount(1);
    }

    m_flagSessionFlushed[m_cameraSessionId.getCount()] = false;

    if (enable > 0) {
        entry = sessionParam.find(sessionKey_imageReaderId);
        if (entry.count > 0) {
            imageReaderId = entry.data.i32[0];
        } else {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):imageReaderId is not defined", __FUNCTION__, __LINE__);
        }

        CLOGI("[S(%d)] offline capture is enabled (imageReaderId(%d))", m_cameraSessionId.getCount(), imageReaderId);

        /* create identifier for current cameraSession */
        CLOGI("[S(%d)] create identifier", m_cameraSessionId.getCount());

        sp<ExynosCameraIdentifier> cameraIdentifier = new ExynosCameraIdentifier();

        /* register imageReaderId to identifier */
        cameraIdentifier->registerCustomIdentification(STR_IMAGE_READER_ID, (uint64_t)imageReaderId);
        cameraIdentifier->registerCustomIdentification(STR_CAMERA_SESSION_ID, (uint64_t)m_cameraSessionId.getCount());

        /* create map for cameraSessionId and identifier */
        m_cameraSessionIdMap.insert(pair<uint32_t, sp<ExynosCameraIdentifier>>(m_cameraSessionId.getCount(), cameraIdentifier));

    } else {
        CLOGI("This cameraSession is NOT for offline");
    }

    return m_cameraSessionId.getCount();
}

status_t ExynosCameraOfflineCapture::processCaptureRequest(ExynosCameraRequestSP_sprt_t request)
{
    CLOGD("");

    status_t ret = NO_ERROR;
    uint32_t requestKey = request->getKey();
    CameraMetadata* meta = request->getServiceMeta();
    camera_metadata_entry_t entry;
    int64_t vendorRequestKey = -1;

    const exynos_ext_tags requestTag = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_REQUEST_ID;

    if (m_cameraSessionId.getCount() <= 0) {
        CLOGW("CameraSessiontId is invalid(%d)", m_cameraSessionId.getCount());
        return ret;
    }

    vendorRequestKey = m_getVendorRequestKey(requestTag, request);

    /* create map for vendorRequestKey and cameraIdentifer */
    if (vendorRequestKey > 0) {
        map<uint32_t, sp<ExynosCameraIdentifier>>::iterator iter;

        iter = m_cameraSessionIdMap.find(m_cameraSessionId.getCount());
        if (iter != m_cameraSessionIdMap.end()) {
            sp<ExynosCameraIdentifier> identifier = iter->second;

            int cameraSessionId = (int)identifier->getCustomIdentification(STR_CAMERA_SESSION_ID);
            int32_t ImgReaderId = (int32_t)identifier->getCustomIdentification(STR_IMAGE_READER_ID);
            CLOGD("[S(%d)] ImgReaderId(%d)", cameraSessionId, ImgReaderId);

            if (m_cameraSessionId.getCount() != cameraSessionId) {
                android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):cameraSessino mismatch", __FUNCTION__, __LINE__);
            }

            m_requestKeyMap.insert(pair<int64_t, sp<ExynosCameraIdentifier>>(vendorRequestKey, identifier));
        } else {
            CLOGW("End of m_cameraSessionIdMap");
        }

        /* increase capture request count on current captureSession */
        m_nCaptureRequested[m_cameraSessionId.getCount()].incCount();

        /* register vendorRequestKey to vendor metadata of request*/
        entry = request->getServiceMeta()->find(requestTag);
        if (entry.count > 0) {
            request->getVendorMeta()->update(entry.tag, entry.data.i64, entry.count);
        }
    }

    CLOGD("[S(%d)R(%d)] vendorRequestKey(%lld), nRequested(%d)", m_cameraSessionId.getCount(),
                                                                requestKey,
                                                                (long long)vendorRequestKey,
                                                                m_nCaptureRequested[m_cameraSessionId.getCount()].getCount());

    return ret;
}

status_t ExynosCameraOfflineCapture::resultHandler(ExynosCameraRequestManager* requestMgr,
                                                    ExynosCameraRequestSP_sprt_t request,
                                                    EXYNOS_REQUEST_RESULT::TYPE type)
{
    CLOGD2("");

    status_t ret = OK;

    ResultRequest resultRequest = NULL;
    camera3_capture_result_t *requestResult = NULL;
    CameraMetadata resultMeta;
    int64_t vendorRequestKey = 0;

    bool alwaysOn = RESULT_NEXT_OPERATION_ALWAYS_ON;

    const exynos_ext_tags requestTag = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_REQUEST_ID;
    const exynos_ext_tags resultTag = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_RESULT_NEXT_OPERATION;

    if (request == NULL) {
        CLOGW("request is NULL");

        //TODO : consider internal frame
        return ret;
    }

    if (request->getServiceMeta()->exists(requestTag) == false && (!alwaysOn)) {
        CLOGI("Skip send result since this request(%d) is not offline, alwaysOn(%d)", request->getKey(), alwaysOn);
        return ret;
    }

    switch(type) {
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA:
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_SHUTTER:

        vendorRequestKey = m_getVendorRequestKey(requestTag, request);
        if (vendorRequestKey > 0 || (alwaysOn)) {
            //resultMeta = request->get3AAResultMeta();
            camera_metadata_entry_t entry;
            int32_t offline_next_op_flag = 0x01;

            resultRequest = requestMgr->createResultRequest(request->getKey(), request->getFrameCount(), type);
            if (resultRequest == NULL) {
                CLOGE("[S(%d)R(%d)F(%d)] createResultRequest fail. PARTIAL_VENDOR_TAG",
                        m_cameraSessionId.getCount(),
                        request->getKey(), request->getFrameCount());
                ret = INVALID_OPERATION;
                return ret;
            }

            requestResult = resultRequest->getCaptureResult();
            if (requestResult == NULL) {
                CLOGE("[S(%d)R(%d)F(%d)] getCaptureResult fail. PARTIAL_VENDOR_TAG",
                        m_cameraSessionId.getCount(),
                        request->getKey(), request->getFrameCount());

                ret = INVALID_OPERATION;
                return ret;
            }

            /* create map for vendorRequestKey and requestInfo(serviceRequestKey, frameCount) */
            offlineRequestInfo_t requestInfo = make_pair(request->getKey(), request->getFrameCount());
            m_requestInfoMap.insert(pair<uint64_t, offlineRequestInfo_t>(vendorRequestKey, requestInfo));

            CLOGD("[S(%d)R(%d)F(%d)] vendorRequestKey(%lld)", m_cameraSessionId.getCount(),
                                                              request->getKey(),
                                                              request->getFrameCount(),
                                                              vendorRequestKey);

            request->setRequestLock();
            resultMeta.update(resultTag, &offline_next_op_flag, 1);
            request->setRequestUnlock();

            requestResult->frame_number = request->getKey();
            requestResult->result = resultMeta.release();
            requestResult->num_output_buffers = 0;
            requestResult->output_buffers = NULL;
            requestResult->input_buffer = NULL;
            requestResult->partial_result = 1;
        } else {
            CLOGD("This request(%d) doesn't have offline requestId", request->getKey());
            return ret;
        }

        break;

    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT:
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ERROR:
    default:
        break;
    }

    requestMgr->pushResultRequest(resultRequest);

    return ret;

}

ExynosCameraRequestSP_sprt_t ExynosCameraOfflineCapture::getRequest()
{
    CLOGD2("");

    return m_getDummyRequest();
}

int ExynosCameraOfflineCapture::getCameraSessionId(ExynosCameraRequestSP_sprt_t request)
{
    int64_t vendorRequestKey = 0;
    int cameraSessionId = 0;

    const exynos_ext_tags requestTag = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_REQUEST_ID;

    if (request == NULL) {
        CLOGW("request is NULL");

        //TODO : consider internal frame
        return 0;
    }

    if (request->getServiceMeta()->exists(requestTag) == false) {
        CLOGV("[R(%d)] Skip send result since this request is not offline", request->getKey());
        return 0;
    }

    vendorRequestKey = m_getVendorRequestKey(requestTag, request);

    CLOGD("[R(%d)] vendorRequestKey(%lld)", request->getKey(), vendorRequestKey);

    /* get cameraSessionId and ImageReaderId from identifier */
    vendorRequestKeyMap_t::iterator requestKeyIter;
    requestKeyIter = m_requestKeyMap.find(vendorRequestKey);
    if (requestKeyIter != m_requestKeyMap.end()) {
        sp<ExynosCameraIdentifier> identifier = requestKeyIter->second;
        cameraSessionId = (int)identifier->getCustomIdentification(STR_CAMERA_SESSION_ID);
    } else {
        CLOGW("End of m_RequestInfoMap");
        return 0;
    }

    CLOGD2("cameraSessionId(%d)", cameraSessionId);

    return cameraSessionId;
}

int ExynosCameraOfflineCapture::getCameraSessionId(ExynosCameraFrameSP_sptr_t frame)
{
    CLOGD("");

    int64_t vendorRequestKey = 0;
    int cameraSessionId = 0;

    const exynos_ext_tags requestTag = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_REQUEST_ID;

    /* find vendorRequestKey from frame */
    camera_metadata_entry_t entry = frame->getVendorMeta()->find(requestTag);
    if (entry.count > 0) {
        vendorRequestKey = entry.data.i64[0];
    } else {
        CLOGE("NOT defined: vendorTag (%s)", get_camera_metadata_tag_name(requestTag));
        return 0;
    }

    CLOGD("[F(%d)] vendorRequestKey(%lld)", frame->getFrameCount(), vendorRequestKey);

    /* get cameraSessionId and ImageReaderId from identifier */
    vendorRequestKeyMap_t::iterator requestKeyIter;
    requestKeyIter = m_requestKeyMap.find(vendorRequestKey);
    if (requestKeyIter != m_requestKeyMap.end()) {
        sp<ExynosCameraIdentifier> identifier = requestKeyIter->second;
        cameraSessionId = (int)identifier->getCustomIdentification(STR_CAMERA_SESSION_ID);
    } else {
        CLOGW("End of m_RequestInfoMap");
        return 0;
    }

    CLOGD("cameraSessionId(%d)", cameraSessionId);

    return cameraSessionId;
}

ExynosCameraRequestSP_sprt_t ExynosCameraOfflineCapture::m_getDummyRequest()
{
    CameraMetadata metadata;

    if (m_dummyRequest == NULL) {
        m_dummyRequest = new ExynosCameraOfflineCapture::ExynosCameraDummyRequest(nullptr, metadata);
    }

    m_dummyRequest->setRequestState(EXYNOS_REQUEST::STATE_ERROR);

    return m_dummyRequest;
}

bool ExynosCameraOfflineCapture::callbackHandler(ExynosCameraFrameSP_sptr_t frame, uint32_t result_status)
{
    CLOGD("");

    bool bLastCapture = false;

    int     cameraSessionId = 0;
    int32_t ImgReaderId = 0;
    int64_t vendorRequestKey = 0;
    int32_t frameNumber = 0;

    const exynos_ext_tags requestTag = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_REQUEST_ID;

    /* find vendorRequestKey from frame */
    camera_metadata_entry_t entry = frame->getVendorMeta()->find(requestTag);
    if (entry.count > 0) {
        vendorRequestKey = entry.data.i64[0];
    } else {
        CLOGE("NOT defined: vendorTag (%s)", get_camera_metadata_tag_name(requestTag));
    }

    /* get requestKey(frameNumber of service request) */
    requestInfo_t::iterator requestInfoIter;
    requestInfoIter = m_requestInfoMap.find(vendorRequestKey);
    if (requestInfoIter != m_requestInfoMap.end()) {
        frameNumber = requestInfoIter->second.first;

        m_requestInfoMap.erase(requestInfoIter);
    } else {
        CLOGW("End of m_RequestInfoMap");
    }

    CLOGD("vendorRequestKey(%lld)", vendorRequestKey);

    /* get cameraSessionId and ImageReaderId from identifier */
    vendorRequestKeyMap_t::iterator requestKeyIter;
    requestKeyIter = m_requestKeyMap.find(vendorRequestKey);
    if (requestKeyIter != m_requestKeyMap.end()) {
        sp<ExynosCameraIdentifier> identifier = requestKeyIter->second;
        cameraSessionId = (int)identifier->getCustomIdentification(STR_CAMERA_SESSION_ID);
        ImgReaderId = (int32_t)identifier->getCustomIdentification(STR_IMAGE_READER_ID);

        m_requestKeyMap.erase(requestKeyIter);
    } else {
        CLOGW("End of m_RequestInfoMap");
    }

    CLOGD("[S(%d)] ImgReaderId(%d), vendorRequestKey(%lld), frameNumber(%d), status(%d)",
            cameraSessionId, ImgReaderId, vendorRequestKey, frameNumber, result_status);

#ifdef ADDED_VOP_HIDL
    CameraVOPIf::getInstance()->CallCBFunc(ImgReaderId, vendorRequestKey, frameNumber, result_status);
#endif

    m_nCaptureRequested[cameraSessionId].decCount();
    if ( m_nCaptureRequested[cameraSessionId].getCount() <= 0 ) {
        CLOGD("[S(%d)] Last callback of this session nCaptureRequested(%d)", cameraSessionId,
                                                                    m_nCaptureRequested[cameraSessionId].getCount());
        if (m_flagSessionFlushed[cameraSessionId] == true) {
            bLastCapture = true;
        }
    } else {
        CLOGD("[S(%d)] nCaptureRequested = %d", cameraSessionId, m_nCaptureRequested[cameraSessionId].getCount());
    }

    CLOGD("[S(%d)] bLastCapture(%d)", cameraSessionId, bLastCapture);

    return bLastCapture;
}

status_t ExynosCameraOfflineCapture::flush(int cameraSessionId)
{
    int cameraSession_id = 0;

    if (cameraSessionId == 0) {
        /* checking latest cameraSessionId */
        cameraSession_id = m_cameraSessionId.getCount();
    } else {
        cameraSession_id = cameraSessionId;
    }

    CLOGD("[S(%d)] flushed!!", cameraSession_id);

    m_flagSessionFlushed[cameraSession_id] = true;

    return NO_ERROR;
}

bool ExynosCameraOfflineCapture::isOfflineCaptureFrame(ExynosCameraFrameSP_sptr_t frame)
{
    bool bOfflineCapture = false;
    int64_t vendorRequestKey = 0;

    const exynos_ext_tags requestTag = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_REQUEST_ID;

    /* find vendorRequestKey from frame */
    camera_metadata_entry_t entry = frame->getVendorMeta()->find(requestTag);
    if (entry.count > 0) {
        vendorRequestKey = entry.data.i64[0];
        if (vendorRequestKey > 0)
            bOfflineCapture = true;
        else {
            CLOGW("[S(%d)F(%d)] vendorRequestKey(%d)",
                    m_cameraSessionId.getCount(), frame->getFrameCount(), vendorRequestKey);
        }
    } else {
        CLOGE("NOT defined: vendorTag (%s)", get_camera_metadata_tag_name(requestTag));
    }

    CLOGD("[F:%d] bOfflineCapture(%d)", frame->getFrameCount(), bOfflineCapture);

    return bOfflineCapture;
}

bool ExynosCameraOfflineCapture::isOfflineCaptureRequest(ExynosCameraRequestSP_sptr_t request)
{
    bool bOfflineCapture = false;

    const exynos_ext_tags requestTag = EXYNOS_ANDROID_VENDOR_OFFLINE_CAPTURE_REQUEST_ID;

    if (m_getVendorRequestKey(requestTag, request) > 0) {
        bOfflineCapture = true;
    }

    CLOGD("[F:%d] bOfflineCapture(%d)", request->getFrameCount(), bOfflineCapture);

    return bOfflineCapture;
}

bool ExynosCameraOfflineCapture::isOfflineCaptureRunning(int cameraSessionId)
{
    bool bRunning = false;
    int cameraSessing_id = 0;

    if (cameraSessionId == 0) {
        /* checking latest cameraSessionId */
        cameraSessing_id = m_cameraSessionId.getCount();
    } else {
        cameraSessing_id = cameraSessionId;
    }

    if (m_nCaptureRequested[cameraSessing_id].getCount() > 0) {
        bRunning = true;
    }

    CLOGD("[S(%d)] offlineCaptureRunning(%d) nCaptureRequested = %d",
            cameraSessing_id, bRunning, m_nCaptureRequested[cameraSessing_id].getCount());

    return bRunning;
}

uint64_t ExynosCameraOfflineCapture::m_getVendorRequestKey(exynos_ext_tags tag, ExynosCameraRequestSP_sprt_t request)
{
    uint64_t vendorRequestKey = 0;
    camera_metadata_entry_t entry;

    entry = request->getServiceMeta()->find(tag);
    if (entry.count > 0) {
        vendorRequestKey = entry.data.i64[0];
    } else {
        CLOGE("NOT defined: vendorTag (%s)", get_camera_metadata_tag_name(tag));
    }

    return vendorRequestKey;
}

};
