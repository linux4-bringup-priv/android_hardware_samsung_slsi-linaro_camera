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
 * distributed under the License is distributed toggle an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file      ExynosCameraOfflineCapture.h
 * \brief     hearder file for ExynosCameraOfflineCapture
 *
 */

#ifndef EXYNOS_CAMERA_OFFLINE_CAPTURE_H__
#define EXYNOS_CAMERA_OFFLINE_CAPTURE_H__

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <utils/String8.h>
#include <utils/Mutex.h>
#include <map>
#include <hardware/camera3.h>
//#include <system/camera_metadata.h>


#include "ExynosCameraCounter.h"

#define CAMERA_SESSION_MAX 10

namespace android {

class ExynosCameraRequest;
class ExynosCameraRequestManager;

typedef sp<ExynosCameraRequest> ExynosCameraRequestSP_sptr_t;
typedef sp<ExynosCameraFrame>  ExynosCameraFrameSP_sptr_t;
typedef map<std::string, uint64_t> customIdMap_t;

class ExynosCameraIdentifier : public virtual RefBase {
public:
    ExynosCameraIdentifier();
    ~ExynosCameraIdentifier();

    status_t registerCustomIdentification(char* id_name, uint64_t value);
    uint64_t getCustomIdentification(char* id_name);

private:
    customIdMap_t m_customIdList;
};

class ExynosCameraOfflineCapture : public virtual RefBase {

public:
    class ExynosCameraDummyRequest : public ExynosCameraRequest {
    public:
        ExynosCameraDummyRequest(camera3_capture_request_t* request, CameraMetadata &meta);
        ~ExynosCameraDummyRequest();
    };

public:
    ExynosCameraOfflineCapture(int cameraId);
    ~ExynosCameraOfflineCapture();

    int configureCameraSession(const camera_metadata_t* session_param);
    status_t processCaptureRequest(ExynosCameraRequestSP_sptr_t request);
    status_t resultHandler(ExynosCameraRequestManager* requestMgr,
                        ExynosCameraRequestSP_sptr_t request,
                        EXYNOS_REQUEST_RESULT::TYPE type);
    bool callbackHandler(ExynosCameraFrameSP_sptr_t frame, uint32_t result_status);

    status_t flush(int cameraSessionId = 0);

    bool isOfflineCaptureFrame(ExynosCameraFrameSP_sptr_t frame);
    bool isOfflineCaptureRequest(ExynosCameraRequestSP_sptr_t request);

    int getCameraSessionId(ExynosCameraRequestSP_sprt_t request);
    int getCameraSessionId(ExynosCameraFrameSP_sptr_t frame);

    ExynosCameraRequestSP_sptr_t getRequest();

    bool isOfflineCaptureRunning(int cameraSessionId = 0);

private:
    uint64_t m_getVendorRequestKey(exynos_ext_tags tag, ExynosCameraRequestSP_sptr_t request);
    ExynosCameraRequestSP_sptr_t m_getDummyRequest();

private:
    typedef std::map<uint32_t, sp<ExynosCameraIdentifier>> cameraSessionIdMap_t;
    typedef std::map<int64_t, sp<ExynosCameraIdentifier>> vendorRequestKeyMap_t;
    typedef pair<uint32_t, uint32_t> offlineRequestInfo_t;
    typedef std::map<uint32_t, offlineRequestInfo_t> requestInfo_t;

    cameraSessionIdMap_t                     m_cameraSessionIdMap;
    vendorRequestKeyMap_t                    m_requestKeyMap;
    requestInfo_t                            m_requestInfoMap;

    ExynosCameraRequestSP_sptr_t m_dummyRequest;

    ExynosCameraCounter m_cameraSessionId;

    ExynosCameraCounter m_nCaptureRequested[CAMERA_SESSION_MAX];

    bool m_flagSessionFlushed[CAMERA_SESSION_MAX];

    int m_cameraId;
    char m_name[EXYNOS_CAMERA_NAME_STR_SIZE];
};
};

#endif /* EXYNOS_CAMERA_OFFLINE_CAPTURE_H__ */
