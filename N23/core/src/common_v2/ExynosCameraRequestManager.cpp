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
#define LOG_TAG "ExynosCameraRequestManager"

#include "ExynosCameraRequestManager.h"

namespace android {

ExynosCameraRequestResult::ExynosCameraRequestResult(uint32_t key, uint32_t requestKey, uint32_t frameCount, EXYNOS_REQUEST_RESULT::TYPE type)
{
    m_init();

    m_key = key;
    m_requestKey = requestKey;
    m_type = type;
    m_frameCount = frameCount;
}

ExynosCameraRequestResult::~ExynosCameraRequestResult()
{
    m_deinit();
}

status_t ExynosCameraRequestResult::m_init()
{
    status_t ret = NO_ERROR;
    m_key = 0;
    m_requestKey = 0;
    m_type = EXYNOS_REQUEST_RESULT::CALLBACK_INVALID;
    m_frameCount = 0;
    m_streamTimeStamp = 0;
    m_streamId = -1;
    memset(&m_captureResult, 0x00, sizeof(camera3_capture_result_t));
    memset(&m_notityMsg, 0x00, sizeof(camera3_notify_msg_t));
    memset(&m_streamBuffer, 0x00, sizeof(camera3_stream_buffer_t));
    memset(&m_physCamIds, 0x00, sizeof(m_physCamIds));

    for (int i = 0; i < MAX_NUM_SENSORS; i++) {
        m_physCamIdsPointer[i] = NULL;
        m_physCamMetadata[i] = NULL;
    }

    return ret;
}

status_t ExynosCameraRequestResult::m_deinit()
{
    status_t ret = NO_ERROR;
    m_key = 0;
    m_requestKey = 0;
    m_type = EXYNOS_REQUEST_RESULT::CALLBACK_INVALID;
    m_frameCount = 0;
    m_streamId = -1;
    memset(&m_captureResult, 0x00, sizeof(camera3_capture_result_t));
    memset(&m_notityMsg, 0x00, sizeof(camera3_notify_msg_t));
    memset(&m_streamBuffer, 0x00, sizeof(camera3_stream_buffer_t));

    for (int i = 0; i < MAX_NUM_SENSORS; i++) {
        m_physCamIdsPointer[i] = NULL;
        m_physCamMetadata[i] = NULL;
    }

    return ret;
}

uint32_t ExynosCameraRequestResult::getKey()
{
    return m_key;
}

uint32_t ExynosCameraRequestResult::getFrameCount()
{
    return m_frameCount;
}

uint32_t ExynosCameraRequestResult::getRequestKey()
{
    return m_requestKey;
}

EXYNOS_REQUEST_RESULT::TYPE ExynosCameraRequestResult::getType()
{
    EXYNOS_REQUEST_RESULT::TYPE ret = EXYNOS_REQUEST_RESULT::CALLBACK_INVALID;
    switch (m_type) {
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT:
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA:
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ERROR:
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_SHUTTER:
        ret = m_type;
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_INVALID:
    default:
        CLOGE2("getResultType type have INVALID value type(%d) key(%u) frameCount(%u) ",
            m_type, m_key, m_frameCount);
        break;
    }
    return ret;
}

int ExynosCameraRequestResult::getStreamId()
{
    int ret = -1;
    switch (m_type) {
    case EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY:
        if (m_streamId < 0) {
            ExynosCameraStream *stream = static_cast<ExynosCameraStream *>(m_streamBuffer.stream->priv);
            stream->getID(&m_streamId);
        }
        ret = m_streamId;
        break;
     default:
        CLOGE2("getResultType type have INVALID value type(%d) key(%u) frameCount(%u) ",
            m_type, m_key, m_frameCount);
        break;
    }

    return ret;
}

camera3_capture_result_t* ExynosCameraRequestResult::getCaptureResult()
{
    return &m_captureResult;
}

camera3_notify_msg_t* ExynosCameraRequestResult::getNotifyMsg()
{
    return &m_notityMsg;
}

camera3_stream_buffer_t* ExynosCameraRequestResult::getStreamBuffer()
{
    return &m_streamBuffer;
}

void ExynosCameraRequestResult::setStreamTimeStamp(uint64_t timestamp)
{
    m_streamTimeStamp = timestamp;
}

uint64_t ExynosCameraRequestResult::getStreamTimeStamp()
{
    return m_streamTimeStamp;
}

void ExynosCameraRequestResult::setupForPhysCamMeta(camera3_capture_result_t *captureResult)
{
    if (!captureResult)
        return;

    for (int i = 0; i < MAX_NUM_SENSORS; i++) {
        m_physCamIdsPointer[i] = (char *)m_physCamIds[i];
    }

    captureResult->physcam_ids = (const char **)m_physCamIdsPointer;
    captureResult->physcam_metadata = (const camera_metadata_t **)m_physCamMetadata;

    return;
}

ExynosCameraRequest::ExynosCameraRequest(camera3_capture_request_t* request,
                                                CameraMetadata &previousMeta,
                                                CameraMetadata *previousMetaPhysCam)
{
    ExynosCameraStream *stream = NULL;
    int streamId = -1;
    cameraId_Info camIdInfo;
    int physCamId = -1;

    if (request == NULL) {
        ALOGE("ERR(%s[%d]):capture_request is NULL", __FUNCTION__, __LINE__);
        return;
    }

    m_init();

    m_request = new camera3_capture_request_t;
    memcpy(m_request, request, sizeof(camera3_capture_request_t));

    /* Deep copy the input buffer object, because the Camera sevice can reuse it
       in successive request with different contents.
    */
    if(request->input_buffer != NULL) {
        CLOGD2("Allocating input buffer (%p)", request->input_buffer);
        m_request->input_buffer = new camera3_stream_buffer_t();
        memcpy(m_request->input_buffer, request->input_buffer, sizeof(camera3_stream_buffer_t));
    }

    m_key = m_request->frame_number;
    m_numOfOutputBuffers = request->num_output_buffers;

    /* Deep copy the output buffer objects as well */
    if ((request->output_buffers != NULL) && (m_numOfOutputBuffers > 0)) {
        camera3_stream_buffer_t* newOutputBuffers = NULL;
        newOutputBuffers = new camera3_stream_buffer_t[m_numOfOutputBuffers];
        memcpy(newOutputBuffers, request->output_buffers, sizeof(camera3_stream_buffer_t) * m_numOfOutputBuffers);
        /* Nasty casting to assign a value to const pointer */
        *(camera3_stream_buffer_t**)(&m_request->output_buffers) = newOutputBuffers;

        for (int i = 0; i < m_numOfOutputBuffers; i++) {
            streamId = -1;
            stream = static_cast<ExynosCameraStream *>(request->output_buffers[i].stream->priv);
            stream->getID(&streamId);
            m_streamIdList[i] = streamId;
        }
    }

    if (request->settings != NULL) {
        m_serviceMeta = request->settings;
    } else {
        CLOGV2("serviceMeta is NULL, use previousMeta");
        if (previousMeta.isEmpty()) {
            CLOGE2("previous meta is empty, ERROR ");
        } else {
            m_serviceMeta = previousMeta;
        }
    }

    m_numPhysCamSettings = request->num_physcam_settings;
    if (m_numPhysCamSettings > MAX_NUM_SENSORS) {
        CLOGE2("m_numPhysCamSettings (%d) are more than the capacity (%d)", m_numPhysCamSettings, MAX_NUM_SENSORS);
        m_numPhysCamSettings = MAX_NUM_SENSORS;
    }

#ifdef USE_DUAL_CAMERA
    // Below code is not required if Dual camera is not neabled

    for (int i = 0; i < m_numPhysCamSettings; i++) {
        camIdInfo.serviceCameraId = atoi(request->physcam_id[i]);
        getCameraIdInfo(&camIdInfo);
        m_physCamInternalID[i] = camIdInfo.cameraId[MAIN_CAM];
        physCamId = camIdInfo.cameraId[MAIN_CAM];
        m_physCamInternalIDMap.insert(pair<int32_t, int32_t>(physCamId, i));
        std::strncpy(m_physCamIDString[i], request->physcam_id[i], PHYSCAM_ID_SIZE - 1);

        /*
         * Assignment clones metadata buffer.
         * CameraMetadata &operator=(const camera_metadata_t *buffer);
         */
        if (request->physcam_settings[i] != NULL) {
            m_serviceMetaPhysCam[i] = request->physcam_settings[i];
        } else {
            m_serviceMetaPhysCam[i] = previousMetaPhysCam[physCamId];
        }
        m_serviceShotPhysCam[i] = m_serviceShot;
        m_metaParamPhysCam[i] = m_metaParameters;
        CLOGV2("key(%d), request->frame_count(%d), physCamId(%d) idx(%d)",
                m_key, request->frame_number, physCamId, i);
    }
    if (m_numPhysCamSettings <= 0) {
        m_numPhysCamSettings = 0;
        /*
         * NOTE: It is not documented anywhere.
         * But according to CTS:
         * Even if the request->num_physcam_settings is zero when the physical streams are present,
         * Physical_meta result should be sent to the framework.
         */
         for (int i = 0; i < m_numOfOutputBuffers; i++) {
            if (request->output_buffers != NULL) {
                camera3_stream_t *serviceStream = request->output_buffers[i].stream;
                stream = static_cast<ExynosCameraStream *>(serviceStream->priv);
                physCamId = stream->getPhysCameraID();
                if (physCamId >= 0) {
                    int id_idx = MIN(m_numPhysCamSettings, MAX_NUM_SENSORS - 1);

                    m_physCamInternalID[id_idx] = physCamId;
                    m_physCamInternalIDMap.insert(pair<int32_t, int32_t>(physCamId, id_idx));
                    std::strncpy(m_physCamIDString[id_idx], serviceStream->physical_camera_id, PHYSCAM_ID_SIZE - 1);
                    if (previousMetaPhysCam[physCamId].isEmpty()) {
                        m_serviceMetaPhysCam[id_idx] = m_serviceMeta;
                        //clone the logicalCam meta for the physical streams.
                    } else {
                        m_serviceMetaPhysCam[id_idx] = previousMetaPhysCam[physCamId];
                    }
                    m_serviceShotPhysCam[id_idx] = m_serviceShot;
                    m_metaParamPhysCam[id_idx] = m_metaParameters;
                    m_numPhysCamSettings++;
                     CLOGV2("[NO_PHYSCAM_Settings]key(%d), request->frame_count(%d), physCamId(%d) idx(%d)",
                            m_key, request->frame_number, physCamId, id_idx);
                }
            }
        }
    }
#endif

    m_vendorMeta = new ExynosCameraVendorMetaData(m_key, m_frameCount);

    CLOGV2("key(%d), request->frame_count(%d), num_output_buffers(%d) m_numPhysCamSettings (%d)",
            m_key, request->frame_number, request->num_output_buffers, m_numPhysCamSettings);

    m_previewFactoryAddrList.clear();
    m_captureFactoryAddrList.clear();

/*    m_serviceMeta = request->settings;*/
}

ExynosCameraRequest::~ExynosCameraRequest()
{
    m_deinit();
}

uint32_t ExynosCameraRequest::getKey()
{
    return m_key;
}

void ExynosCameraRequest::setFrameCount(uint32_t frameCount)
{
    m_frameCount = frameCount;
}

status_t ExynosCameraRequest::m_init()
{
    status_t ret = NO_ERROR;

    m_key = 0;
    m_request = NULL;
    m_requestId = 0;
    m_frameCount = 0;
    m_serviceMeta.clear();
    memset(&m_serviceShot, 0x00, sizeof(struct camera2_shot_ext));
    memset(&m_metaParameters, 0x00, sizeof(struct CameraMetaParameters));
    memset(&m_ysumValue, 0x00, sizeof(struct ysum_data));
    m_metaParameters.m_zoomRatio = 1.0f;
    m_metaParameters.m_prevZoomRatio = 1.0f;

    for (int i = 0; i < INTERFACE_TYPE_MAX; i++) {
        m_serviceShot.shot.uctl.scalerUd.mcsc_sub_blk_port[i] = MCSC_PORT_NONE;
    }

    m_requestState = EXYNOS_REQUEST::STATE_SERVICE;
    m_factoryMap.clear();
    m_acquireFenceDoneMap.clear();
    m_numOfCompleteBuffers = 0;
    m_pipelineDepth = 0;

    for (int i = 0 ; i < EXYNOS_REQUEST_RESULT::CALLBACK_MAX ; i++) {
        m_resultStatus[i] = false;
    }

    m_inputStreamStatus = CAMERA3_BUFFER_STATUS_OK;
    for (int i = 0; i < HAL_STREAM_ID_MAX; i++) {
        m_outputStreamStatus[i] = CAMERA3_BUFFER_STATUS_OK;
    }
    m_sensorTimeStampBoot = 0;

    m_inputStreamDone = false;
    for (int i = 0 ; i < HAL_STREAM_ID_MAX; i++) {
        m_streamIdList[i] = -1;
        m_outputStreamDone[i] = false;
        m_streamPipeId[i] = -1;
        m_streamParentPipeId[i] = -1;

        m_horizontalFlip[i] = false;
        m_verticalFlip[i] = false;

        m_rotation[i] = false;
    }

    m_isSkipMetaResult = false;
    m_isSkipCaptureResult = false;
    m_dsInputPortId = MCSC_PORT_NONE;

    m_displayCameraId = -1;
    m_slaveCamId = -1;
    m_slaveCamIdx = DEFAULT_SLAVE_CAM_IDX;
    m_reprocessSlaveCamId = -1;
    m_reprocessSlaveCamIdx = DEFAULT_SLAVE_CAM_IDX;

    return ret;
}

status_t ExynosCameraRequest::m_deinit()
{
    status_t ret = NO_ERROR;

    if (m_request != NULL) {
        if (m_request->input_buffer != NULL) {
            delete m_request->input_buffer;
        }
        if (m_request->output_buffers != NULL) {
            delete[] m_request->output_buffers;
        }

        delete m_request;
        m_request = NULL;
    }

    m_frameCount = 0;
    m_serviceMeta.clear();
    memset(&m_serviceShot, 0x00, sizeof(struct camera2_shot_ext));
    memset(&m_metaParameters, 0x00, sizeof(struct CameraMetaParameters));
    m_requestState = EXYNOS_REQUEST::STATE_SERVICE;
    resetCompleteBufferCount();

    for (int i = 0; i < m_numPhysCamSettings; i++) {
        m_serviceMetaPhysCam[i].clear();
    }
    m_physCamInternalIDMap.clear();

    for (int i = 0 ; i < EXYNOS_REQUEST_RESULT::CALLBACK_MAX ; i++) {
        m_resultStatus[i] = false;
    }

    m_previewFactoryAddrList.clear();
    m_captureFactoryAddrList.clear();

    m_vendorMeta = NULL;

    return ret;
}


uint32_t ExynosCameraRequest::getFrameCount()
{
    return m_frameCount;
}

camera3_capture_request_t* ExynosCameraRequest::getServiceRequest()
{
    if (m_request == NULL) {
        CLOGE2("getServiceRequest is NULL m_request(%p)", m_request);
    }
    return m_request;
}

CameraMetadata* ExynosCameraRequest::getServiceMeta()
{
    return &m_serviceMeta;
}

CameraMetadata* ExynosCameraRequest::getServiceMetaPhysCam(int32_t physCamInternalID)
{
    map<int32_t, int32_t>::iterator it;

    it = m_physCamInternalIDMap.find(physCamInternalID);
    if (it == m_physCamInternalIDMap.end()) {
        CLOGE2("getServiceMetaPhysCam is failed:  physCamID (%d) it (%d)", physCamInternalID, it);
        return NULL;
    }

   return &m_serviceMetaPhysCam[it->second];
}

int32_t *ExynosCameraRequest::getAllPhysCamInternalIDs(void) {
    return m_physCamInternalID;
}

void ExynosCameraRequest::getPhysCamIDString(char **physCamIDString)
{
    for (int i = 0; i < m_numPhysCamSettings && i < MAX_NUM_SENSORS; i++) {
        std::strncpy (physCamIDString[i], m_physCamIDString[i], PHYSCAM_ID_SIZE);
    }
    return;
}

struct camera2_shot_ext* ExynosCameraRequest::getServiceShot()
{
    return &m_serviceShot;
}

struct camera2_shot_ext* ExynosCameraRequest::getServiceShotPhysCam(int32_t physCamInternalID)
{

    map<int32_t, int32_t>::iterator it;

    it = m_physCamInternalIDMap.find(physCamInternalID);
    if (it == m_physCamInternalIDMap.end()) {
        CLOGV2("getServiceShotPhysCam is failed:  physCamID (%d) it (%d)", physCamInternalID, it);
        return NULL;
    }

    return &m_serviceShotPhysCam[it->second];
}

struct CameraMetaParameters* ExynosCameraRequest::getMetaParameters()
{
    return &m_metaParameters;
}


struct CameraMetaParameters* ExynosCameraRequest::getMetaParamPhysCam(int32_t physCamInternalID)
{
    map<int32_t, int32_t>::iterator it;

    it = m_physCamInternalIDMap.find(physCamInternalID);
    if (it == m_physCamInternalIDMap.end()) {
        CLOGE2("getMetaParameters is failed:  physCamID (%d) it (%d)", physCamInternalID, it);
        return NULL;
    }

    return &m_metaParamPhysCam[it->second];
}

status_t ExynosCameraRequest::setSlaveCamId(int32_t slaveCamId,
                                            int32_t slaveCamIdx,
                                            bool isReprocessing)
{
    if (!isReprocessing) {
        m_slaveCamId = slaveCamId;
        m_slaveCamIdx = slaveCamIdx;
    } else {
        m_reprocessSlaveCamId = slaveCamId;
        m_reprocessSlaveCamIdx = slaveCamIdx;
    }
    return NO_ERROR;
}

int32_t ExynosCameraRequest::getSlaveCamId(bool isReprocessing)
{
    if (!isReprocessing) {
        return m_slaveCamId;
    } else {
        return m_reprocessSlaveCamId;
    }
}

void ExynosCameraRequest::setRequestLock()
{
    m_requestLock.lock();
}

void ExynosCameraRequest::setRequestUnlock()
{
    m_requestLock.unlock();
}

status_t ExynosCameraRequest::setRequestState(EXYNOS_REQUEST::STATE state)
{
    status_t ret = NO_ERROR;
    switch (state) {
    case EXYNOS_REQUEST::STATE_ERROR:
    case EXYNOS_REQUEST::STATE_SERVICE:
    case EXYNOS_REQUEST::STATE_RUNNING:
        m_requestState = state;
        break;
    default:
        CLOGE2("setState is invalid newstate(%d) ", state);
        break;
    }

    return ret;
}

EXYNOS_REQUEST::STATE ExynosCameraRequest::getRequestState(void)
{
    EXYNOS_REQUEST::STATE ret = EXYNOS_REQUEST::STATE_INVALID;
    switch (m_requestState) {
    case EXYNOS_REQUEST::STATE_ERROR:
    case EXYNOS_REQUEST::STATE_SERVICE:
    case EXYNOS_REQUEST::STATE_RUNNING:
        ret = m_requestState;
        break;
    case EXYNOS_REQUEST::STATE_INVALID:
    default:
        CLOGE2("getState is invalid curstate(%d) ", m_requestState);
        break;
    }

    return ret;
}

uint32_t ExynosCameraRequest::getNumOfInputBuffer()
{
    uint32_t numOfInputBuffer = 0;
    if (m_request->input_buffer != NULL) {
        numOfInputBuffer = 1;
    }
    return numOfInputBuffer;
}

camera3_stream_buffer_t* ExynosCameraRequest::getInputBuffer()
{
    if (m_request == NULL){
        CLOGE2("getInputBuffer m_request is NULL m_request(%p) ", m_request);
        return NULL;
    }

    if (m_request->input_buffer == NULL){
        CLOGV2("getInputBuffer input_buffer is NULL m_request(%p) ", m_request->input_buffer);
    }

    return m_request->input_buffer;
}

uint64_t ExynosCameraRequest::getSensorTimestamp()
{
    return m_sensorTimeStampBoot;
}

void ExynosCameraRequest::setSensorTimestamp(uint64_t timeStamp)
{
    m_sensorTimeStampBoot = timeStamp;
}

uint32_t ExynosCameraRequest::getNumOfOutputBuffer()
{
    return m_numOfOutputBuffers;
}

uint32_t ExynosCameraRequest::getNumOfPhysCamSettings()
{
    return m_numPhysCamSettings;
}

void ExynosCameraRequest::increaseCompleteBufferCount(void)
{
    m_resultStatusLock.lock();
    m_numOfCompleteBuffers++;
    m_resultStatusLock.unlock();
}

void ExynosCameraRequest::resetCompleteBufferCount(void)
{
    m_resultStatusLock.lock();
    m_numOfCompleteBuffers = 0;
    m_resultStatusLock.unlock();
}

int ExynosCameraRequest::getCompleteBufferCount(void)
{
     return m_numOfCompleteBuffers;
}

bool ExynosCameraRequest::isAllBufferCallbackDone(void)
{
    return ((int) this->getNumOfOutputBuffer() == this->getCompleteBufferCount());
}

const camera3_stream_buffer_t* ExynosCameraRequest::getOutputBuffers()
{
    if (m_request == NULL){
        CLOGE2("utputBuffer m_request is NULL m_request(%p) ", m_request);
        return NULL;
    }

    if (m_request->output_buffers == NULL){
        CLOGE2("getNumOfOutputBuffer output_buffers is NULL m_request(%p) ",
            m_request->output_buffers);
        return NULL;
    }

    CLOGV2("m_request->output_buffers(%p)", m_request->output_buffers);

    return m_request->output_buffers;
}

const camera3_stream_buffer_t* ExynosCameraRequest::getOutputBuffer(int pStreamId, bool exact)
{
    int count = getNumOfOutputBuffer();

    if (!count) return nullptr;

    const camera3_stream_buffer_t *bufferList = getOutputBuffers();
    for (uint32_t index = 0; index < count; index++) {
        const camera3_stream_buffer_t *streamBuffer = &(bufferList[index]);
        int streamId = getStreamIdwithBufferIdx(index) % (exact ? 1 : HAL_STREAM_ID_MAX);
        if (streamId == pStreamId) {
            return streamBuffer;
        }
    }

    return nullptr;
}

void ExynosCameraRequest::setRequestId(int reqId) {
    m_requestId = reqId;
}

int ExynosCameraRequest::getRequestId(void) {
    return m_requestId;
}

status_t ExynosCameraRequest::m_push(int key, ExynosCameraFrameFactory* item, FrameFactoryMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<FrameFactoryMap::iterator,bool> listRet;
    lock->lock();
    listRet = list->insert( pair<uint32_t, ExynosCameraFrameFactory*>(key, item));
    if (listRet.second == false) {
        ret = INVALID_OPERATION;
        CLOGE2("m_push failed, request already exist!! Request frameCnt( %d )", key);
    }
    lock->unlock();

    return ret;
}

void ExynosCameraRequest::setSizeInfo(frame_size_scenario_t scenario, const frame_size_info_t &info, int32_t cameraId)
{
    Mutex::Autolock l(m_sizeInfoLock);

    m_frameSizeInfoMap[cameraId][scenario] = info;
}

bool ExynosCameraRequest::getSizeInfo(frame_size_scenario_t scenario, frame_size_info_t &info, int32_t cameraId)
{
    Mutex::Autolock l(m_sizeInfoLock);

    if (m_frameSizeInfoMap.count(cameraId) == 0) return false;
    auto &subMap = m_frameSizeInfoMap[cameraId];

    if (subMap.count(scenario) == 0) return false;

    info = subMap[scenario];

    return true;
}

status_t ExynosCameraRequest::m_pop(int key, ExynosCameraFrameFactory** item, FrameFactoryMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<FrameFactoryMap::iterator,bool> listRet;
    FrameFactoryMapIterator iter;
    ExynosCameraFrameFactory *factory = NULL;

    lock->lock();
    iter = list->find(key);
    if (iter != list->end()) {
        factory = iter->second;
        *item = factory;
        list->erase( iter );
    } else {
        CLOGE2("m_pop failed, factory is not EXIST Request frameCnt( %d )", key);
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequest::m_get(int streamID, ExynosCameraFrameFactory** item, FrameFactoryMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<FrameFactoryMap::iterator,bool> listRet;
    FrameFactoryMapIterator iter;
    ExynosCameraFrameFactory *factory = NULL;

    lock->lock();
    iter = list->find(streamID);
    if (iter != list->end()) {
        factory = iter->second;
        *item = factory;
    } else {
        CLOGE2("m_pop failed, request is not EXIST Request streamID( %d )", streamID);
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return ret;
}

bool ExynosCameraRequest::m_find(int streamID, FrameFactoryMap *list, Mutex *lock)
{
    bool ret = false;
    pair<FrameFactoryMap::iterator,bool> listRet;
    FrameFactoryMapIterator iter;

    lock->lock();
    iter = list->find(streamID);
    if (iter != list->end()) {
        ret = true;
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequest::m_getList(FrameFactoryList *factorylist, FrameFactoryMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    FrameFactoryMapIterator iter;
    ExynosCameraFrameFactory *factory = NULL;

    lock->lock();
    for (iter = list->begin(); iter != list->end() ; iter++) {
        factory = iter->second;
        factorylist->push_back(factory);
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequest::pushFrameFactory(int StreamID, ExynosCameraFrameFactory* factory)
{
    status_t ret = NO_ERROR;
    ret = m_push(StreamID % HAL_STREAM_ID_MAX, factory, &m_factoryMap, &m_factoryMapLock);
    if (ret < 0) {
        CLOGE2("pushFrameFactory is failed StreamID(%d) factory(%p)", StreamID, factory);
    }

    return ret;
}

ExynosCameraFrameFactory* ExynosCameraRequest::getFrameFactory(int streamID)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameFactory* factory = NULL;

    ret = m_get(streamID % HAL_STREAM_ID_MAX, &factory, &m_factoryMap, &m_factoryMapLock);
    if (ret < 0) {
        CLOGE2("getFrameFactory is failed StreamID(%d) factory(%p)", streamID, factory);
    }

    return factory;
}

bool ExynosCameraRequest::findFrameFactory(int streamID)
{
    bool ret = false;

    ret = m_find(streamID % HAL_STREAM_ID_MAX, &m_factoryMap, &m_factoryMapLock);

    return ret;
}

status_t ExynosCameraRequest::getFrameFactoryList(FrameFactoryList *list)
{
    status_t ret = NO_ERROR;

    ret = m_getList(list, &m_factoryMap, &m_factoryMapLock);
    if (ret < 0) {
        CLOGE2("getFrameFactoryList is failed");
    }

    return ret;
}

status_t ExynosCameraRequest::setFactoryAddrList(ExynosCameraFrameFactory *factoryAddr[FRAME_FACTORY_TYPE_MAX])
{
    status_t ret = NO_ERROR;

    for (int factoryTypeIdx = 0; factoryTypeIdx < FRAME_FACTORY_TYPE_MAX; factoryTypeIdx++) {
        if (factoryAddr[factoryTypeIdx] != NULL) {
            if (factoryTypeIdx == FRAME_FACTORY_TYPE_REPROCESSING) {
                m_captureFactoryAddrList.push_back(factoryAddr[factoryTypeIdx]);
            } else if (factoryTypeIdx == FRAME_FACTORY_TYPE_CAPTURE_PREVIEW
                       || factoryTypeIdx == FRAME_FACTORY_TYPE_VISION) {
                m_previewFactoryAddrList.push_back(factoryAddr[factoryTypeIdx]);
            }
        }
    }

    return ret;
}

status_t ExynosCameraRequest::getFactoryAddrList(enum FRAME_FACTORY_TYPE factoryType, FrameFactoryList *list)
{
    status_t ret = NO_ERROR;
    FrameFactoryListIterator iter;
    ExynosCameraFrameFactory *factory = NULL;

    if (factoryType == FRAME_FACTORY_TYPE_REPROCESSING) {
        for (iter = m_captureFactoryAddrList.begin(); iter != m_captureFactoryAddrList.end(); ) {
            factory = *iter;
            list->push_back(factory);
            iter++;
        }
    } else if (factoryType == FRAME_FACTORY_TYPE_CAPTURE_PREVIEW
               || factoryType == FRAME_FACTORY_TYPE_VISION) {
        for (iter = m_previewFactoryAddrList.begin(); iter != m_previewFactoryAddrList.end(); ) {
            factory = *iter;
            list->push_back(factory);
            iter++;
        }
    }

    return ret;
}

int32_t ExynosCameraRequest::getSizeOfFactory(enum FRAME_FACTORY_TYPE factoryType)
{
    if (factoryType == FRAME_FACTORY_TYPE_REPROCESSING) {
        return m_captureFactoryAddrList.size();
    } else if (factoryType == FRAME_FACTORY_TYPE_CAPTURE_PREVIEW
               || factoryType == FRAME_FACTORY_TYPE_VISION) {
        return m_previewFactoryAddrList.size();
    }

    return 0;
}

status_t ExynosCameraRequest::getAllRequestOutputStreams(List<int> **list)
{
    status_t ret = NO_ERROR;

    CLOGV2("m_requestOutputStreamList.size(%zu)", m_requestOutputStreamList.size());

    /* lock->lock(); */
    *list = &m_requestOutputStreamList;
    /* lock->unlock(); */

    return ret;
}

status_t ExynosCameraRequest::pushRequestOutputStreams(int requestStreamId)
{
    status_t ret = NO_ERROR;

    /* lock->lock(); */
    m_requestOutputStreamList.push_back(requestStreamId);
    /* lock->unlock(); */

    return ret;
}

status_t ExynosCameraRequest::getAllRequestInputStreams(List<int> **list)
{
    status_t ret = NO_ERROR;

    CLOGV2("m_requestOutputStreamList.size(%zu)", m_requestInputStreamList.size());

    /* lock->lock(); */
    *list = &m_requestInputStreamList;
    /* lock->unlock(); */

    return ret;
}

status_t ExynosCameraRequest::pushRequestInputStreams(int requestStreamId)
{
    status_t ret = NO_ERROR;

    /* lock->lock(); */
    m_requestInputStreamList.push_back(requestStreamId);
    /* lock->unlock(); */

    return ret;
}

status_t ExynosCameraRequest::setAcquireFenceDone(buffer_handle_t *handle, bool done)
{
    status_t ret = NO_ERROR;
    pair<map<buffer_handle_t *, bool>::iterator, bool> listRet;
    Mutex::Autolock l(m_acquireFenceDoneLock);
    listRet = m_acquireFenceDoneMap.insert(pair<buffer_handle_t *, bool>(handle, done));
    if (listRet.second == false) {
        map<buffer_handle_t *, bool>::iterator iter = m_acquireFenceDoneMap.find(handle);
        bool fenceDone = false;

        if (iter != m_acquireFenceDoneMap.end()) {
            fenceDone = iter->second;
            if (fenceDone != done) {
                m_acquireFenceDoneMap.at(handle) = done;
                CLOGW2("[R%d F%d] duplicate KEY, value old: %s/ new: %s",
                        m_key, m_frameCount,
                        fenceDone == true ? "Done" : "Wait",
                        done == true ? "Done" : "Wait");
            }
        } else {
            ret = INVALID_OPERATION;
            CLOGE2("[R%d F%d] m_pop failed, handle is not EXIST",
                    m_key, m_frameCount);
        }
    }

    return ret;
}

bool ExynosCameraRequest::getAcquireFenceDone(buffer_handle_t *handle)
{
    bool fenceDone = false;
    pair<map<buffer_handle_t *, bool>::iterator, bool> listRet;
    map<buffer_handle_t *, bool>::iterator iter;
    ExynosCameraRequestSP_sprt_t request = NULL;

    Mutex::Autolock l(m_acquireFenceDoneLock);
    iter = m_acquireFenceDoneMap.find(handle);
    if (iter != m_acquireFenceDoneMap.end()) {
        fenceDone = iter->second;
    } else {
        CLOGE2("[R%d F%d] m_pop failed, handle is not EXIST",
                m_key, m_frameCount);
        fenceDone = false;
    }

    return fenceDone;
}

void ExynosCameraRequest::setFlipHorizontal(int streamId, bool flip)
{
    if (streamId < HAL_STREAM_ID_RAW) {
        CLOG_ASSERT2("invalid streamId(%d)). so, assert!!!!", streamId);
    }

    m_horizontalFlip[streamId % HAL_STREAM_ID_MAX] = flip;
}

bool ExynosCameraRequest::getFlipHorizontal(int streamId)
{
    if (streamId < HAL_STREAM_ID_RAW) {
        CLOG_ASSERT2("invalid streamId(%d)). so, assert!!!!", streamId);
    }

    return m_horizontalFlip[streamId % HAL_STREAM_ID_MAX];
}

void ExynosCameraRequest::setFlipVertical(int streamId, bool flip)
{
    if (streamId < HAL_STREAM_ID_RAW) {
        CLOG_ASSERT2("invalid streamId(%d). so, assert!!!!", streamId);
    }

    m_verticalFlip[streamId % HAL_STREAM_ID_MAX] = flip;
}

bool ExynosCameraRequest::getFlipVertical(int streamId)
{
    if (streamId < HAL_STREAM_ID_RAW) {
        CLOG_ASSERT2("invalid streamId(%d)). so, assert!!!!", streamId);
    }

    return m_verticalFlip[streamId % HAL_STREAM_ID_MAX];
}

void ExynosCameraRequest::setRotation(int streamId, bool rotation)
{
    if (streamId < HAL_STREAM_ID_RAW) {
        CLOG_ASSERT2("invalid streamId(%d). so, assert!!!!", streamId);
    }

    m_rotation[streamId % HAL_STREAM_ID_MAX] = rotation;
}

bool ExynosCameraRequest::getRotation(int streamId)
{
    if (streamId < HAL_STREAM_ID_RAW) {
        CLOG_ASSERT2("invalid streamId(%d)). so, assert!!!!", streamId);
    }

    return m_rotation[streamId % HAL_STREAM_ID_MAX];
}

status_t ExynosCameraRequest::setCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType, bool flag)
{
    status_t ret = NO_ERROR;
    ret = m_setCallbackDone(reqType, flag, &m_resultStatusLock);
    if (ret < 0) {
        CLOGE2("m_get request is failed, request type(%d) ", reqType);
    }

    return ret;
}

bool ExynosCameraRequest::getCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType)
{
    bool ret = false;
    ret = m_getCallbackDone(reqType, &m_resultStatusLock);
    return ret;
}

status_t ExynosCameraRequest::setCallbackStreamDone(int streamId, bool flag)
{
    status_t ret = NO_ERROR;
    ret = m_setCallbackStreamDone(streamId, flag, &m_resultStatusLock);
    if (ret < 0) {
        CLOGE2("[R%d F%d S%d] setCallbackStreamDone is failed.", m_key, m_frameCount, streamId);
    }

    return ret;
}

bool ExynosCameraRequest::getCallbackStreamDone(int streamId)
{
    bool ret = false;
    ret = m_getCallbackStreamDone(streamId, &m_resultStatusLock);
    return ret;
}

bool ExynosCameraRequest::isComplete()
{
    bool ret = false;
    bool notify = false;
    bool partial = false;
    bool capture = false;

    notify = m_getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY, &m_resultStatusLock);
    partial = m_getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA, &m_resultStatusLock);
    capture = m_getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT, &m_resultStatusLock);

    if (notify == true && capture == true && partial == true) {
        ret = true;
    }

    return ret;
}

int ExynosCameraRequest::getStreamIdwithBufferIdx(int bufferIndex)
{
    if (bufferIndex < 0 || bufferIndex >= m_numOfOutputBuffers) {
        CLOGE2("Invalid buffer index(%d), outputBufferCount(%d)",
                bufferIndex, m_numOfOutputBuffers);
        return -1;
    }

    if (m_streamIdList[bufferIndex] < 0) {
        CLOGE2("m_streamIdList is NULL, index(%d)", bufferIndex);
        return -1;
    }

    return m_streamIdList[bufferIndex];
}

bool ExynosCameraRequest::hasStream(int streamIdx)
{
    if (streamIdx >= HAL_STREAM_ID_MAX) {
        CLOGE2("Invalid Stream id (%d)", streamIdx);
        return false;
    }

    for (int i = 0; i < m_numOfOutputBuffers; i++) {
        if ((m_streamIdList[i] % HAL_STREAM_ID_MAX) == streamIdx) {
            return true;
        }
    }

    return false;
}

int ExynosCameraRequest::getStreamIdwithStreamIdx(int streamIdx)
{
    if (streamIdx >= HAL_STREAM_ID_MAX) {
        CLOGE2("Invalid Stream id (%d)", streamIdx);
        return -1;
    }

    for (int i = 0; i < m_numOfOutputBuffers; i++) {
        if ((m_streamIdList[i] % HAL_STREAM_ID_MAX) == streamIdx) {
            return m_streamIdList[i];
        }
    }

    return -1;
}

int ExynosCameraRequest::getBufferIndex(int streamId)
{
    int index = -1;

    if (streamId % HAL_STREAM_ID_MAX >= HAL_STREAM_ID_MAX) {
        CLOGE2("Invalid Stream id (%d)", streamId);
        return -1;
    }

    for (int i = 0; i < m_numOfOutputBuffers; i++) {
        if (streamId == m_streamIdList[i]) {
            index = i;
            break;
        }
    }

    return index;
}

void ExynosCameraRequest::setStreamPipeId(int streamId, int pipeId)
{
    int index = m_getOutputBufferIndex(streamId);

    if (index == -1 || index >= HAL_STREAM_ID_MAX) {
        CLOGE2("Invalid Stream id (%d)", streamId);
        return;
    }

    m_streamPipeId[index] = pipeId;
}

int ExynosCameraRequest::getStreamPipeId(int streamId)
{
    int index = m_getOutputBufferIndex(streamId);

    if (index == -1 || index >= HAL_STREAM_ID_MAX) {
        CLOGE2("Invalid Stream id (%d)", streamId);
        return -1;
    }

    return m_streamPipeId[index];
}

void ExynosCameraRequest::setParentStreamPipeId(int streamId, int pipeId)
{
    int index = m_getOutputBufferIndex(streamId);

    if (index == -1 || index >= HAL_STREAM_ID_MAX) {
        CLOGE2("Invalid Stream id (%d)", streamId);
        return;
    }

    m_streamParentPipeId[index] = pipeId;
}

int ExynosCameraRequest::getParentStreamPipeId(int streamId)
{
    int index = m_getOutputBufferIndex(streamId);

    if (index == -1 || index >= HAL_STREAM_ID_MAX) {
        CLOGE2("Invalid Stream id (%d)", streamId);
        return -1;
    }

    return m_streamParentPipeId[index];
}

void ExynosCameraRequest::increasePipelineDepth(void)
{
    m_pipelineDepth++;
}

void ExynosCameraRequest::updatePipelineDepth(void)
{
    const uint8_t pipelineDepth = m_pipelineDepth;

    m_serviceShot.shot.dm.request.pipelineDepth = m_pipelineDepth;
    m_serviceMeta.update(ANDROID_REQUEST_PIPELINE_DEPTH, &pipelineDepth, 1);

    for (int i = 0; i < m_numPhysCamSettings; i++) {
        m_serviceShotPhysCam[i].shot.dm.request.pipelineDepth = m_pipelineDepth;
        m_serviceMetaPhysCam[i].update(ANDROID_REQUEST_PIPELINE_DEPTH, &pipelineDepth, 1);
    }
    CLOGV2("ANDROID_REQUEST_PIPELINE_DEPTH(%d)", pipelineDepth);
}

CameraMetadata ExynosCameraRequest::get3AAResultMeta(void)
{
    uint8_t entry;
    CameraMetadata minimal_resultMeta;

    m_copyAndEraseTagFromeServiceMetaDataU8(ANDROID_CONTROL_AF_MODE, minimal_resultMeta);
    m_copyAndEraseTagFromeServiceMetaDataU8(ANDROID_CONTROL_AWB_MODE, minimal_resultMeta);
    m_copyAndEraseTagFromeServiceMetaDataU8(ANDROID_CONTROL_AE_STATE, minimal_resultMeta);
    m_copyAndEraseTagFromeServiceMetaDataU8(ANDROID_CONTROL_AF_STATE, minimal_resultMeta);
    m_copyAndEraseTagFromeServiceMetaDataU8(ANDROID_CONTROL_AWB_STATE, minimal_resultMeta);

    get3AAResultMetaVendor(minimal_resultMeta);

    return minimal_resultMeta;
}

CameraMetadata ExynosCameraRequest::getShutterMeta()
{
    __unused uint8_t entry = 1;
    CameraMetadata minimal_resultMeta;

    return minimal_resultMeta;
}

void ExynosCameraRequest::setSkipMetaResult(bool skip)
{
    Mutex::Autolock l(m_SkipMetaResultLock);
    m_isSkipMetaResult = skip;
}

bool ExynosCameraRequest::getSkipMetaResult(void)
{
    Mutex::Autolock l(m_SkipMetaResultLock);
    return m_isSkipMetaResult;
}

void ExynosCameraRequest::setSkipCaptureResult(bool skip)
{
    Mutex::Autolock l(m_SkipCaptureResultLock);
    m_isSkipCaptureResult = skip;
}

bool ExynosCameraRequest::getSkipCaptureResult(void)
{
    Mutex::Autolock l(m_SkipCaptureResultLock);
    return m_isSkipCaptureResult;
}

void ExynosCameraRequest::setDsInputPortId(int dsInputPortId)
{
    m_dsInputPortId = dsInputPortId;
}

int ExynosCameraRequest::getDsInputPortId(void)
{
    return m_dsInputPortId;
}

void ExynosCameraRequest::setYsumValue(struct ysum_data *ysumdata)
{
    m_ysumValue.higher_ysum_value = ysumdata->higher_ysum_value;
    m_ysumValue.lower_ysum_value = ysumdata->lower_ysum_value;
}

void ExynosCameraRequest::getYsumValue(struct ysum_data *ysumdata)
{
    ysumdata->higher_ysum_value = m_ysumValue.higher_ysum_value;
    ysumdata->lower_ysum_value = m_ysumValue.lower_ysum_value;
}

sp<ExynosCameraVendorMetaData> ExynosCameraRequest::getVendorMeta()
{
    return m_vendorMeta;
}

bool ExynosCameraRequest::m_isInputStreamId(int streamId)
{
    camera3_stream_buffer_t *inputBuffer = NULL;
    camera3_stream_t *serviceStream = NULL;
    ExynosCameraStream *stream = NULL;
    int id = 0;

    if (streamId < 0) {
        ALOGE("ERR(%s[%d]):streamId is INVALID", __FUNCTION__, __LINE__);
        return -1;
    }

    inputBuffer = getInputBuffer();
    if (inputBuffer == NULL) {
        ALOGV("DEBUG(%s[%d]):inputBuffer is NULL", __FUNCTION__, __LINE__);
        return false;
    }

    serviceStream = inputBuffer->stream;
    if (serviceStream == NULL) {
        ALOGE("ERR(%s[%d]):serviceStream is NULL", __FUNCTION__, __LINE__);
        return false;
    }

    stream = static_cast<ExynosCameraStream *>(serviceStream->priv);
    if (stream == NULL) {
        ALOGE("ERR(%s[%d]):stream is NULL", __FUNCTION__, __LINE__);
        return false;
    }

    stream->getID(&id);
    if (id != streamId) {
         ALOGV("DEBUG(%s[%d]):not inputStreamId", __FUNCTION__, __LINE__);
         return false;
    }

    return true;
}

int ExynosCameraRequest::m_getOutputBufferIndex(int streamId)
{
    if (streamId < 0) {
        ALOGE("ERR(%s[%d]):streamId is INVALID", __FUNCTION__, __LINE__);
        return -1;
    }

    for (int streamIndex = 0 ; streamIndex < m_numOfOutputBuffers; streamIndex++) {
        if (m_streamIdList[streamIndex] == streamId) {
            ALOGV("DEBUG(%s[%d]):Find output stream index. streamId(%d), streamIndex(%d)",
                    __FUNCTION__, __LINE__, streamId, streamIndex);
            return streamIndex;
        }
    }

    ALOGV("DEBUG(%s[%d]):Stream index not found from stream ID list. streamId(%d)",
            __FUNCTION__, __LINE__, streamId);

    return -1;
}

status_t ExynosCameraRequest::m_setCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType, bool flag, Mutex *lock)
{
    status_t ret = NO_ERROR;
    if (reqType >= EXYNOS_REQUEST_RESULT::CALLBACK_MAX) {
        CLOGE2("m_setCallback failed, status erray out of bounded reqType(%d)", reqType);

        ret = INVALID_OPERATION;
        return ret;
    }

    lock->lock();
    m_resultStatus[reqType] = flag;
    lock->unlock();
    return ret;
}

bool ExynosCameraRequest::m_getCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType, Mutex *lock)
{
    bool ret = false;
    if (reqType >= EXYNOS_REQUEST_RESULT::CALLBACK_MAX) {
        CLOGE2("m_getCallback failed, status erray out of bounded reqType(%d)", reqType);

        return ret;
    }

    lock->lock();
    ret = m_resultStatus[reqType];
    lock->unlock();
    return ret;
}

status_t ExynosCameraRequest::m_setCallbackStreamDone(int streamId, bool flag, Mutex *lock)
{
    int index = m_getOutputBufferIndex(streamId);
    if (index == -1) {
        if (m_isInputStreamId(streamId) == true) {
            Mutex::Autolock l(lock);
            m_inputStreamDone = flag;
        } else {
            ALOGE("ERR(%s[%d]):streamId(%d) is mismatched", __FUNCTION__, __LINE__, streamId);
            return NAME_NOT_FOUND;
        }
    } else {
        Mutex::Autolock l(lock);
        m_outputStreamDone[index] = flag;
    }

    return NO_ERROR;
}

bool ExynosCameraRequest::m_getCallbackStreamDone(int streamId, Mutex *lock)
{
    bool ret = false;
    int index = m_getOutputBufferIndex(streamId);
    if (index == -1) {
        if (m_isInputStreamId(streamId) == true) {
            Mutex::Autolock l(lock);
            ret = m_inputStreamDone;
        } else {
            ALOGE("ERR(%s[%d]):streamId(%d) is mismatched", __FUNCTION__, __LINE__, streamId);
            return false;
        }
    } else {
        Mutex::Autolock l(lock);
        ret = m_outputStreamDone[index];
    }

    return ret;
}

status_t ExynosCameraRequest::m_setStreamBufferStatus(int streamId, camera3_buffer_status_t status, Mutex *lock)
{
    int index = m_getOutputBufferIndex(streamId);
    if (index == -1) {
        if (m_isInputStreamId(streamId) == true) {
            Mutex::Autolock l(lock);
            m_inputStreamStatus = status;
        } else {
            ALOGE("ERR(%s[%d]):streamId(%d) is mismatched", __FUNCTION__, __LINE__, streamId);
            return NAME_NOT_FOUND;
        }
    } else {
        Mutex::Autolock l(lock);
        m_outputStreamStatus[index] = status;
    }

    return NO_ERROR;
}

camera3_buffer_status_t ExynosCameraRequest::m_getStreamBufferStatus(int streamId, Mutex *lock)
{
    camera3_buffer_status_t status = CAMERA3_BUFFER_STATUS_ERROR;

    int index = m_getOutputBufferIndex(streamId);
    if (index == -1) {
        if (m_isInputStreamId(streamId) == true) {
            Mutex::Autolock l(lock);
            status = m_inputStreamStatus;
        } else {
            ALOGE("ERR(%s[%d]):streamId(%d) is mismatched", __FUNCTION__, __LINE__, streamId);
            return CAMERA3_BUFFER_STATUS_ERROR;
        }
    } else {
        Mutex::Autolock l(lock);
        status = m_outputStreamStatus[index];
    }

    return status;
}

void ExynosCameraRequest::printCallbackDoneState()
{
    for (int i = 0 ; i < EXYNOS_REQUEST_RESULT::CALLBACK_MAX ; i++)
        CLOGD2("m_key(%d), m_resultStatus[%d](%d)", m_key, i, m_resultStatus[i]);
}

status_t ExynosCameraRequest::setStreamBufferStatus(int streamId, camera3_buffer_status_t bufferStatus)
{
    status_t ret = NO_ERROR;

    ret = m_setStreamBufferStatus(streamId, bufferStatus, &m_streamStatusLock);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):[R%d F%d S%d] setCallbackStreamDone is failed.",
                __FUNCTION__, __LINE__, m_key, m_frameCount, streamId);
    }

    return ret;
}

camera3_buffer_status_t ExynosCameraRequest::getStreamBufferStatus(int streamId)
{
    return m_getStreamBufferStatus(streamId, &m_streamStatusLock);
}

ExynosCameraRequestManager::ExynosCameraRequestManager(cameraId_Info *camIdInfo, ExynosCameraConfigurations *configurations)
{
    CLOGD("Create-ID(%d)", camIdInfo->cameraId[MAIN_CAM]);

    m_camIdInfo = camIdInfo;
    setCameraId(m_camIdInfo->cameraId[MAIN_CAM]);
    m_configurations = configurations;
    m_converter = NULL;
    m_callbackOps = NULL;
    m_requestResultKey = 0;
    m_setFlushFlag(false);
    m_resultRenew = 0;
    m_printInterval = 0;
    m_callbackCounts.fill(0);

    memset(m_name, 0x00, sizeof(m_name));

    for (int i = 0; i < CAMERA3_TEMPLATE_COUNT; i++) {
        m_defaultRequestTemplate[i] = NULL;
    }

    m_factoryMap.clear();

    m_notifySequencer = new ExynosCameraCallbackSequencer("CB NOTIFY", getCameraId());
    m_partialSequencer = new ExynosCameraCallbackSequencer("CB PARTIAL", getCameraId());
    m_allMetaSequencer = new ExynosCameraCallbackSequencer("CB META", getCameraId());

    m_resultNotifyCallbackThread        = new callbackThread(this, &ExynosCameraRequestManager::m_resultNotifyCallbackThreadFunc, "m_resultNotifyCallbackThread");
    m_resultStreamCallbackThread        = new callbackThread(this, &ExynosCameraRequestManager::m_resultStreamCallbackThreadFunc, "m_resultStreamCallbackThread");
    m_resultMetaCallbackThread          = new callbackThread(this, &ExynosCameraRequestManager::m_resultMetaCallbackThreadFunc, "m_resultMetaCallbackThread");
    m_callbackThread                    = new callbackThread(this, &ExynosCameraRequestManager::m_resultCallbackThreadFunc, "m_resultCallbackThreadFunc");

    for (int i = 0; i < EXYNOS_REQUEST_RESULT::CALLBACK_MAX; i++)
        m_lastResultKey[i] = 0;

    for (int i = 0; i < CAMERA_ID_MAX; i++)
        m_previousMetaPhysCam[i].clear();

    memset(&m_faceDetectMeta, 0x00, sizeof(m_faceDetectMeta));

    for (int i = 0; i < MAX_NUM_SENSORS; i++) {
        m_physicalIdsMetaData[i] = NULL;
    }
}

ExynosCameraRequestManager::~ExynosCameraRequestManager()
{
    CLOGD("");

    m_setFlushFlag(false);
    m_resultRenew = 0;

    stopThreadAndInputQ(m_resultNotifyCallbackThread, 1, &m_resultNotifyCallbackQ);
    stopThreadAndInputQ(m_resultStreamCallbackThread, 1, &m_resultStreamCallbackQ);
    stopThreadAndInputQ(m_resultMetaCallbackThread, 1, &m_resultMetaCallbackQ);
    stopThreadAndInputQ(m_callbackThread, 1, &m_callbackQ);

    if (m_notifySequencer != NULL) {
        m_notifySequencer->flush();
        m_notifySequencer = NULL;
    }

    if (m_partialSequencer != NULL) {
        m_partialSequencer->flush();
        m_partialSequencer = NULL;
    }

    if (m_allMetaSequencer != NULL) {
        m_allMetaSequencer->flush();
        m_allMetaSequencer = NULL;
    }

    for (int i = 0; i < CAMERA3_TEMPLATE_COUNT; i++) {
        if (m_defaultRequestTemplate[i]) {
            free_camera_metadata(m_defaultRequestTemplate[i]);
            m_defaultRequestTemplate[i] = NULL;
        }
    }

    m_serviceRequests.clear();
    m_runningRequests.clear();

    m_requestFrameCountMap.clear();

    m_previousMeta.clear();
    for (int i = 0; i < CAMERA_ID_MAX; i++)
        m_previousMetaPhysCam[i].clear();

    m_factoryMap.clear();

}

status_t ExynosCameraRequestManager::setMetaDataConverter(ExynosCameraMetadataConverter *converter)
{
    status_t ret = NO_ERROR;
    if (m_converter != NULL)
        CLOGD("m_converter is not NULL(%p)", m_converter);

    m_converter = converter;
    return ret;
}

ExynosCameraMetadataConverter* ExynosCameraRequestManager::getMetaDataConverter()
{
    if (m_converter == NULL)
        CLOGD("m_converter is NULL(%p)", m_converter);

    return m_converter;
}

status_t ExynosCameraRequestManager::setRequestsInfo(int key, ExynosCameraFrameFactory *factory)
{
    status_t ret = NO_ERROR;
    if (factory == NULL) {
        CLOGE("m_factory is NULL key(%d) factory(%p)", key, factory);

        ret = INVALID_OPERATION;
        return ret;
    }

    ret = m_pushFactory(key ,factory, &m_factoryMap, &m_factoryMapLock);
    if (ret < 0) {
        CLOGE("m_pushFactory is failed key(%d) factory(%p)", key, factory);

        ret = INVALID_OPERATION;
        return ret;
    }

    return ret;
}

void ExynosCameraRequestManager::clearFrameFactory(void)
{
    m_factoryMapLock.lock();
    m_factoryMap.clear();
    m_factoryMapLock.unlock();
}

status_t ExynosCameraRequestManager::setStreamInfos(list<int> &streamIdList)
{
    Mutex::Autolock lock(m_streamsSequencerListLock);
    callback_sequence_t sequence = NULL;
    for(list<int>::iterator iter = streamIdList.begin(); iter != streamIdList.end(); iter++) {
        int streamId = *iter;
        String8 name;
        name.appendFormat("CB STREAM %d", streamId);
        sequence = new ExynosCameraCallbackSequencer(name.c_str(),getCameraId());
        m_streamsSequencerList[streamId] = sequence;
    }
    return NO_ERROR;
}

void ExynosCameraRequestManager::flushStreamSequencer(void)
{
    Mutex::Autolock lock(m_streamsSequencerListLock);
    callback_sequence_t sequence = NULL;
    for(stream_callback_sequence_iter_t iter = m_streamsSequencerList.begin(); iter != m_streamsSequencerList.end(); iter++) {
        sequence = iter->second;
        sequence->flush();
    }
}

void ExynosCameraRequestManager::clearStreamSequencer(void)
{
    Mutex::Autolock lock(m_streamsSequencerListLock);
    m_streamsSequencerList.clear();
}

ExynosCameraRequestSP_sprt_t ExynosCameraRequestManager::registerToServiceList(ExynosCameraRequestSP_sprt_t request)
{
    status_t ret = NO_ERROR;

    m_waitFlushDone();

    ret = m_pushBack(request, &m_serviceRequests, &m_requestLock);
    if (ret < 0){
        CLOGE("request m_pushBack is failed request[R%d]", request->getKey());

        request = NULL;
        return NULL;
    }

    return request;
}

status_t ExynosCameraRequestManager::isPrevRequest()
{
    if (m_previousMeta.isEmpty())
        return BAD_VALUE;
    else
        return OK;
}

status_t ExynosCameraRequestManager::clearPrevRequest()
{
    m_previousMeta.clear();
    for (int i = 0; i < MAX_NUM_SENSORS; i++)
        m_previousMetaPhysCam[i].clear();
    return OK;
}

status_t ExynosCameraRequestManager::constructDefaultRequestSettings(int type, camera_metadata_t **request)
{
    Mutex::Autolock l(m_requestLock);

    CLOGD("Type = %d", type);

    if (m_defaultRequestTemplate[type]) {
        *request = m_defaultRequestTemplate[type];
        return OK;
    }

    m_converter->constructDefaultRequestSettings(type, request);

    m_defaultRequestTemplate[type] = *request;

    CLOGD("Registered default request template(%d)", type);
    return OK;
}

status_t ExynosCameraRequestManager::m_pushBack(ExynosCameraRequestSP_sprt_t item, RequestInfoList *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    lock->lock();
    list->push_back(item);
    lock->unlock();
    return ret;
}

status_t ExynosCameraRequestManager::m_popBack(ExynosCameraRequestSP_dptr_t item, RequestInfoList *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    lock->lock();
    if (list->size() > 0) {
        item = list->back();
        list->pop_back();
    } else {
        CLOGE("m_popBack failed, size(%zu)", list->size());
        ret = INVALID_OPERATION;
    }
    lock->unlock();
    return ret;
}

status_t ExynosCameraRequestManager::m_pushFront(ExynosCameraRequestSP_sprt_t item, RequestInfoList *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    lock->lock();
    list->push_front(item);
    lock->unlock();
    return ret;
}

status_t ExynosCameraRequestManager::m_popFront(ExynosCameraRequestSP_dptr_t item, RequestInfoList *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    lock->lock();
    if (list->size() > 0) {
        item = list->front();
        list->pop_front();
    } else {
        CLOGE("m_popFront failed, size(%zu)", list->size());
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_get(uint32_t frameCount,
                                        ExynosCameraRequestSP_dptr_t item,
                                        RequestInfoList *list,
                                        Mutex *lock)
{
    status_t ret = INVALID_OPERATION;
    RequestInfoListIterator iter;
    ExynosCameraRequestSP_sprt_t request = NULL;

    if (item == NULL) {
        CLOGE("item is NULL");
        return BAD_VALUE;
    }

    if (list == NULL) {
        CLOGE("list is NULL");
        return BAD_VALUE;
    }

    if (lock == NULL) {
        CLOGE("lock is NULL");
        return BAD_VALUE;
    }

    lock->lock();
    for (iter = list->begin(); iter != list->end(); ++iter) {
        request = *iter;
        if (request->getKey() == frameCount) {
            ret = NO_ERROR;
            break;
        }
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_push(ExynosCameraRequestSP_sprt_t request, RequestInfoMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<RequestInfoMap::iterator,bool> listRet;
    lock->lock();
    listRet = list->insert( pair<uint32_t, ExynosCameraRequestSP_sprt_t>(request->getKey(), request));
    if (listRet.second == false) {
        ret = INVALID_OPERATION;
        CLOGE("m_push failed, request already exist!! Request frameCnt( %d )", request->getFrameCount());
    }
    lock->unlock();

    //m_printAllRequestInfo(list, lock);

    return ret;
}

status_t ExynosCameraRequestManager::m_pop(uint32_t key,
                                            ExynosCameraRequestSP_dptr_t item,
                                            RequestInfoMap *list,
                                            Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<RequestInfoMap::iterator,bool> listRet;
    RequestInfoMapIterator iter;
    ExynosCameraRequestSP_sprt_t request = NULL;

    lock->lock();
    iter = list->find(key);
    if (iter != list->end()) {
        request = iter->second;
        item = request;
        list->erase( iter );
    } else {
        CLOGE("m_pop failed, request is not EXIST Request key(%d)", key);
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_get(uint32_t key,
                                           ExynosCameraRequestSP_dptr_t item,
                                           RequestInfoMap *list,
                                           Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<RequestInfoMap::iterator,bool> listRet;
    RequestInfoMapIterator iter;
    ExynosCameraRequestSP_sprt_t request = NULL;

    lock->lock();
    iter = list->find(key);
    if (iter != list->end()) {
        request = iter->second;
        item = request;
    } else {
        CLOGE("m_pop failed, request is not EXIST Request key(%d)", key);
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    //m_printAllRequestInfo(list, lock);

    return ret;
}

bool ExynosCameraRequestManager::m_exist(uint32_t key,
                                           RequestInfoMap *list,
                                           Mutex *lock)
{
    bool ret = false;
    pair<RequestInfoMap::iterator,bool> listRet;
    RequestInfoMapIterator iter;
    ExynosCameraRequestSP_sprt_t request = NULL;

    lock->lock();
    iter = list->find(key);
    if (iter != list->end()) {
        ret = true;
    }
    lock->unlock();

    return ret;
}

void ExynosCameraRequestManager::m_printAllServiceRequestInfo(void)
{
    Mutex::Autolock l(m_requestLock);

    RequestInfoListIterator iter;
    ExynosCameraRequestSP_sprt_t request;
    camera3_capture_request_t *serviceRequest;

    for (iter = m_serviceRequests.begin(); iter != m_serviceRequests.end(); ++iter) {
        request = *iter;
        serviceRequest = request->getServiceRequest();
        CLOGI("key(%d), serviceFrameCount(%d), (%p) frame_number(%d), outputNum(%d)",
                request->getKey(),
                request->getFrameCount(),
                serviceRequest,
                serviceRequest->frame_number,
                serviceRequest->num_output_buffers);

    }
}

void ExynosCameraRequestManager::m_printAllRequestInfo(RequestInfoMap *map, Mutex *lock)
{
    RequestInfoMapIterator iter;
    ExynosCameraRequestSP_sprt_t request = NULL;
    ExynosCameraRequestSP_sprt_t item = NULL;
    camera3_capture_request_t *serviceRequest = NULL;

    lock->lock();
    iter = map->begin();

    while(iter != map->end()) {
        request = iter->second;
        item = request;

        serviceRequest = item->getServiceRequest();

        CLOGI("key(%d), serviceFrameCount(%d), (%p) frame_number(%d), outputNum(%d)",
            request->getKey(),
            request->getFrameCount(),
            serviceRequest,
            serviceRequest->frame_number,
            serviceRequest->num_output_buffers);

        iter++;
    }
    lock->unlock();
}

void ExynosCameraRequestManager::m_printAllResultInfo(camera3_capture_result_t *result, EXYNOS_REQUEST_RESULT::TYPE type)
{
    ////////////////////////////////////////////////
    // meta check : if you want to meta check, please turn on here.
    camera_metadata_t *metadata = (camera_metadata_t *)result->result;
    camera_metadata_entry_t entry;
    int tag = 0;
    char *tagName = NULL;

    int64_t timeStamp = 0LL;
    int aeState = -1;
    int afState = -1;
    int flashState = -1;

    ////////////////////////////////////////////////
    // TIMESTAMP
    tag = ANDROID_SENSOR_TIMESTAMP; // you may change this.
    tagName = (char *)"ANDROID_SENSOR_TIMESTAMP"; // you may change this.

    if (find_camera_metadata_entry(metadata, tag, &entry) == 0) {
        if (entry.count > 0) {
            timeStamp = entry.data.i64[0];
        } else {
            CLOGV("[R:%d] no entry for %s", result->frame_number, tagName);
        }
    } else {
        CLOGV("[R:%d] find_camera_metadata_entry(%s), fail", result->frame_number, tagName);
    }

    ////////////////////////////////////////////////
    // AE
    tag = ANDROID_CONTROL_AE_STATE; // you may change this.
    tagName = (char *)"ANDROID_CONTROL_AE_STATE"; // you may change this.

    if (find_camera_metadata_entry(metadata, tag, &entry) == 0) {
        if(entry.count > 0) {
            aeState = entry.data.u8[0];
        } else {
            CLOGV("[R:%d] no entry for %s", result->frame_number, tagName);
        }
    } else {
        CLOGV("[R:%d] find_camera_metadata_entry(%s), fail", result->frame_number, tagName);
    }

    ////////////////////////////////////////////////
    // AE
    tag = ANDROID_CONTROL_AF_STATE; // you may change this.
    tagName = (char *)"ANDROID_CONTROL_AF_STATE"; // you may change this.

    if (find_camera_metadata_entry(metadata, tag, &entry) == 0) {
        if(entry.count > 0) {
            afState = entry.data.u8[0];
        } else {
            CLOGV("[R:%d] no entry for %s", result->frame_number, tagName);
        }
    } else {
        CLOGV("[R:%d] find_camera_metadata_entry(%s), fail", result->frame_number, tagName);
    }

    ////////////////////////////////////////////////
    // FLASH_STATE
    tag = ANDROID_FLASH_STATE; // you may change this.
    tagName = (char *)"ANDROID_FLASH_STATE"; // you may change this.

    if (find_camera_metadata_entry(metadata, tag, &entry) == 0) {
        if(entry.count > 0) {
            flashState = entry.data.u8[0];
        } else {
            CLOGV("[R:%d] no entry for %s", result->frame_number, tagName);
        }
    } else {
        CLOGV("[R:%d] find_camera_metadata_entry(%s), fail", result->frame_number, tagName);
    }


    ////////////////////////////////////////////////
    // Print it
    if (0 < aeState &&
        0 < afState) {
        char *callbackName = NULL;

        switch (type) {
        case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY:
            callbackName = (char *)"CALLBACK_NOTIFY_ONLY";
            break;
        case EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY:
            callbackName = (char *)"CALLBACK_BUFFER_ONLY";
            break;
        case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA:
            callbackName = (char *)"CALLBACK_PARTIAL_3AA";
            break;
        case EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT:
            callbackName = (char *)"CALLBACK_ALL_RESULT";
            break;
        case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ERROR:
            callbackName = (char *)"CALLBACK_NOTIFY_ERROR";
            break;
        case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_SHUTTER:
            callbackName = (char *)"CALLBACK_PARTIAL_SHUTTER";
            break;
        default:
            callbackName = (char *)"CALLBACK_UNKNOWN";
            break;
        }

        if (type == EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT) {
            // refer system/media//camera/include/system/camera_metadata_tags.h

            CLOGD("[R%d][TS%ju] ae(%d) af(%d) flashState(%d) : %s",
                result->frame_number, timeStamp, aeState, afState, flashState, callbackName);
        }
    }

    ////////////////////////////////////////////////
}

status_t ExynosCameraRequestManager::m_pushFactory(int key,
                                                    ExynosCameraFrameFactory* item,
                                                    FrameFactoryMap *list,
                                                    Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<FrameFactoryMap::iterator,bool> listRet;
    lock->lock();
    listRet = list->insert(pair<uint32_t, ExynosCameraFrameFactory*>(key, item));
    if (listRet.second == false) {
        ret = INVALID_OPERATION;
        CLOGE("m_push failed, request already exist!! Request key( %d )", key);
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_popFactory(int key, ExynosCameraFrameFactory** item, FrameFactoryMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<FrameFactoryMap::iterator,bool> listRet;
    FrameFactoryMapIterator iter;
    ExynosCameraFrameFactory *factory = NULL;

    lock->lock();
    iter = list->find(key);
    if (iter != list->end()) {
        factory = iter->second;
        *item = factory;
        list->erase( iter );
    } else {
        CLOGE("m_pop failed, factory is not EXIST Request key( %d )", key);
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_getFactory(int key, ExynosCameraFrameFactory** item, FrameFactoryMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<FrameFactoryMap::iterator,bool> listRet;
    FrameFactoryMapIterator iter;
    ExynosCameraFrameFactory *factory = NULL;
    lock->lock();
    iter = list->find(key);
    if (iter != list->end()) {
        factory = iter->second;
        *item = factory;
    } else {
        CLOGE("m_pop failed, request is not EXIST Request key( %d )", key);
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return ret;
}

uint32_t ExynosCameraRequestManager::getAllRequestCount(void)
{
    Mutex::Autolock l(m_requestLock);
    return m_serviceRequests.size() + m_runningRequests.size();
}

uint32_t ExynosCameraRequestManager::getServiceRequestCount(void)
{
    Mutex::Autolock lock(m_requestLock);
    return m_serviceRequests.size();
}

uint32_t ExynosCameraRequestManager::getRunningRequestCount(void)
{
    Mutex::Autolock lock(m_requestLock);
    return m_runningRequests.size();
}

void ExynosCameraRequestManager::getRunningRequestKeyList(CallbackListkeys &list)
{
    Mutex::Autolock lock(m_requestLock);
    for (RequestInfoMapIterator iter = m_runningRequests.begin(); iter != m_runningRequests.end(); iter++) {
        list.push_back(iter->first);
    }
    list.sort();
    return;
}

ExynosCameraRequestSP_sprt_t ExynosCameraRequestManager::eraseFromServiceList(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraRequestSP_sprt_t request = NULL;

    ret = m_popFront(request, &m_serviceRequests, &m_requestLock);
    if (ret < 0){
        CLOGE("request m_popFront is failed request");
        ret = INVALID_OPERATION;
        return NULL;
    }

    return request;
}

status_t ExynosCameraRequestManager::registerToRunningList(ExynosCameraRequestSP_sprt_t request_in)
{
    status_t ret = NO_ERROR;
    List<int> *streamList = NULL;

    m_waitFlushDone();

    ret = m_push(request_in, &m_runningRequests, &m_requestLock);
    if (ret < 0){
        CLOGE("request m_push is failed request");
        ret = INVALID_OPERATION;
        return ret;
    }

    ret = m_increasePipelineDepth(&m_runningRequests, &m_requestLock);
    if (ret != NO_ERROR)
        CLOGE("Failed to increase the pipeline depth");

    m_notifySequencer->pushKey(request_in->getKey());
    m_partialSequencer->pushKey(request_in->getKey());
    m_allMetaSequencer->pushKey(request_in->getKey());

    request_in->getAllRequestOutputStreams(&streamList);
    for (List<int>::iterator iter = streamList->begin(); iter != streamList->end(); iter++) {
        stream_callback_sequence_iter_t findIter;
        callback_sequence_t sequence = NULL;
        int streamId = *iter;
        findIter = m_streamsSequencerList.find(streamId);
        if (findIter != m_streamsSequencerList.end()) {
            sequence = findIter->second;
            sequence->pushKey(request_in->getKey());
        }
    }

    return ret;
}

status_t ExynosCameraRequestManager::m_removeFromRunningList(uint32_t requestKey)
{
    status_t ret = NO_ERROR;
    ExynosCameraRequestSP_sprt_t request = NULL;
    bool find = false;

    find = m_exist(requestKey, &m_runningRequests, &m_requestLock);
    if (find) {
        CLOGV("[R%d] Delete m_removeFromRunningList.", requestKey);
        ret = m_pop(requestKey, request, &m_runningRequests, &m_requestLock);
        if (ret < 0) {
            ret = INVALID_OPERATION;
            CLOGE("request m_popFront is failed request");
            return ret;
        }
    } else {
        CLOGV("[R%d] Already Delete m_removeFromRunningList.", requestKey);
        return ret;
    }

    if (m_getFlushFlag() == false) {
        uint32_t key = 0;
        ret = m_popKey(&key, request->getFrameCount());
        if (ret < NO_ERROR) {
            CLOGE("Failed to m_popKey. frameCount %d", request->getFrameCount());
            return INVALID_OPERATION;
        }
    }

    return ret;
}

ExynosCameraRequestSP_sprt_t ExynosCameraRequestManager::getRunningRequest(uint32_t frameCount)
{
    status_t ret = NO_ERROR;
    uint32_t key = 0;
    ExynosCameraRequestSP_sprt_t request = NULL;

    ret = m_getKey(&key, frameCount);
    if (ret < NO_ERROR) {
        CLOGE("Failed to m_popKey. frameCount %d", frameCount);
        return NULL;
    }

    ret = m_get(key, request, &m_runningRequests, &m_requestLock);
    if (ret < 0) {
        ret = INVALID_OPERATION;
        CLOGE("request m_popFront is failed request");
    }

    return request;
}

status_t ExynosCameraRequestManager::getRunningRequest(uint32_t requestCount, ExynosCameraRequestSP_dptr_t request)
{
    status_t ret = NO_ERROR;

    ret = m_get(requestCount, request, &m_runningRequests, &m_requestLock);
    if (ret < 0) {
        ret = INVALID_OPERATION;
        CLOGE("request m_popFront is failed request");
    }

    return ret;
}

void ExynosCameraRequestManager::notifyDeviceError(void)
{
    camera3_notify_msg_t notifyMsg;

    memset(&notifyMsg, 0, sizeof(notifyMsg));

    notifyMsg.type = CAMERA3_MSG_ERROR;
    notifyMsg.message.error.error_code = CAMERA3_MSG_ERROR_DEVICE;

    m_callbackOpsNotify(&notifyMsg);
}

status_t ExynosCameraRequestManager::flush(bool reconfigure)
{
    status_t ret = NO_ERROR;
    uint32_t requestKey = 0;
    ExynosCameraRequestSP_sprt_t request = NULL;
    ResultRequest resultRequest = NULL;

    camera3_notify_msg_t *notifyMsg = NULL;
    camera3_capture_result_t *requestResult = NULL;
    camera3_stream_buffer_t *streamBuffer = NULL;
    const camera3_stream_buffer_t *outputBuffer = NULL;

    CameraMetadata resultMeta;

    int32_t streamId = 0;

    uint32_t bufferCnt = 0;
    ExynosCameraStream *stream = NULL;

    EXYNOS_REQUEST_RESULT::TYPE cbType = EXYNOS_REQUEST_RESULT::CALLBACK_INVALID;

    m_setFlushFlag(true);
    m_resultRenew = 0;
    m_printInterval = 0;

    stopThreadAndInputQ(m_resultNotifyCallbackThread, 1, &m_resultNotifyCallbackQ);
    stopThreadAndInputQ(m_resultStreamCallbackThread, 1, &m_resultStreamCallbackQ);
    stopThreadAndInputQ(m_resultMetaCallbackThread, 1, &m_resultMetaCallbackQ);

    int maxWaitTry = 50;
    while(m_callbackQ.getSizeOfProcessQ() > 0 && maxWaitTry > 0) {
        usleep(2000);
        if (maxWaitTry%10 == 0)
            CLOGD("Wait for callbackQ flush size(%d)", m_callbackQ.getSizeOfProcessQ());

        maxWaitTry--;
    }
    stopThreadAndInputQ(m_callbackThread, 1, &m_callbackQ);

    CLOGD("ALL Callback Thread Exited");

    m_callbackFlushTimer.start();

    CLOGD("IN+++");
    CLOGD("AllRequestCount(%d), ServiceRequestCount(%d), RunningRequestCount(%d)",
            getAllRequestCount(), getServiceRequestCount(), getRunningRequestCount());

    do {
        /*
         * If the flush is operated, the frameCount in request may be an inavlid value.
         * Because, the request before matched with frameCount will be transfer to RunningList from ServiceList.
         */

        if (reconfigure) {
            while (getServiceRequestCount() > 0) {
                request = eraseFromServiceList();
                if (request == NULL) {
                    CLOGE("createRequest failed ");
                }

                if (request != NULL) {
                    if (registerToRunningList(request) != NO_ERROR) {
                        CLOGE("[R%d] registerToRunningList is failed", request->getKey());
                    }
                }
            }
        } else {
            for (uint32_t i = 0; i < getServiceRequestCount(); i++) {
                eraseFromServiceList();
            }
        }

        CallbackListkeys keyList;
        getRunningRequestKeyList(keyList);

        for(CallbackListkeysIter iter = keyList.begin(); iter != keyList.end(); iter++) {
            request = NULL;
            requestKey = *iter;

            ret = getRunningRequest(requestKey, request);
            if (ret < 0) {
                CLOGD("Request is NULL(%d) ret(%d)", requestKey, ret);
                continue;
            }

            notifyMsg = NULL;
            cbType = EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY;
            if (request->getCallbackDone(cbType) == false) {
                resultRequest = createResultRequest(requestKey, request->getFrameCount(), cbType);
                if (resultRequest == NULL) {
                    CLOGE("[R%d] createResultRequest fail. Notify Callback", request->getKey());
                    continue;
                }

                notifyMsg = resultRequest->getNotifyMsg();
                if (notifyMsg == NULL) {
                    CLOGE("[R%d] getNotifyResult fail. Notify Callback", request->getKey());
                    continue;
                }

                request->setRequestState(EXYNOS_REQUEST::STATE_ERROR);
                request->setSkipMetaResult(true);

                notifyMsg->type = CAMERA3_MSG_ERROR;
                notifyMsg->message.error.frame_number = requestKey;

                if (request->getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA) == true
                        || (request->getCompleteBufferCount() > 0)) {
                    notifyMsg->message.error.error_code = CAMERA3_MSG_ERROR_RESULT;
                } else {
                    notifyMsg->message.error.error_code = CAMERA3_MSG_ERROR_REQUEST;
                }

                m_notifyErrorCallback(request, resultRequest, true);
                request->setCallbackDone(cbType, true);
            }

            requestResult = NULL;
            cbType = EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA;
            if (request->getCallbackDone(cbType) == false) {
                resultRequest = createResultRequest(requestKey, request->getFrameCount(), cbType);
                if (resultRequest == NULL) {
                    CLOGE("[R%d] createResultRequest fail. 3AA_META", request->getKey());
                    continue;
                }

                requestResult = resultRequest->getCaptureResult();
                if (requestResult == NULL) {
                    CLOGE("[R%d] getCaptureResult fail. 3AA_META", request->getKey());
                    continue;
                }

                if (request->getSkipMetaResult() == false) {
                    request->setRequestLock();
                    resultMeta = request->get3AAResultMeta();
                    request->setRequestUnlock();
                    requestResult->result = resultMeta.release();
                } else {
                    requestResult->result = NULL;
                }

                requestResult->frame_number = requestKey;
                requestResult->num_output_buffers = 0;
                requestResult->output_buffers = NULL;
                requestResult->input_buffer = NULL;
                requestResult->partial_result = 1;

                m_partialMetaCallback(request, resultRequest, true);
            }

            requestResult = NULL;
            cbType = EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT;
            if (request->getCallbackDone(cbType) == false) {
                resultRequest = createResultRequest(requestKey, request->getFrameCount(), cbType);
                if (resultRequest == NULL) {
                    CLOGE("[R%d] createResultRequest fail. 3AA_META", request->getKey());
                    continue;
                }

                requestResult = resultRequest->getCaptureResult();
                if (requestResult == NULL) {
                    CLOGE("[R%d] getCaptureResult fail. 3AA_META", request->getKey());
                    continue;
                }

                request->setRequestLock();
                request->updatePipelineDepth();
                request->setRequestUnlock();

                requestResult->frame_number = requestKey;
                requestResult->result = NULL; /* Will be updated by m_releaseCameraMetadata */
                requestResult->num_output_buffers = 0;
                requestResult->output_buffers = NULL;
                requestResult->input_buffer = NULL;
                requestResult->partial_result = 2;

                m_allMetaCallback(request, resultRequest, true);
            }

            cbType = EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY;

            outputBuffer = NULL;
            bufferCnt = request->getNumOfOutputBuffer();
            outputBuffer = request->getOutputBuffers();
            for(uint32_t i = 0 ; i < bufferCnt ; i++) {
                stream = static_cast<ExynosCameraStream*>(outputBuffer[i].stream->priv);
                stream->getID(&streamId);

                requestResult = NULL;
                if (request->getCallbackStreamDone(streamId) == false) {
                    resultRequest = createResultRequest(requestKey, request->getFrameCount(), cbType);
                    if (resultRequest == NULL) {
                        CLOGE("[R%d S%d] createResultRequest fail.", request->getKey(), streamId);
                        continue;
                    }

                    requestResult = resultRequest->getCaptureResult();
                    if (requestResult == NULL) {
                        CLOGE("[R%d S%d] getCaptureResult fail.", request->getKey(), streamId);
                        continue;
                    }

                    streamBuffer = resultRequest->getStreamBuffer();
                    if (streamBuffer == NULL) {
                        CLOGE("[R%d S%d] getStreamBuffer fail.", request->getKey(), streamId);

                        /* Continue to handle other streams */
                        continue;
                    }

                    ret = stream->getStream(&(streamBuffer->stream));
                    if (ret != NO_ERROR) {
                        CLOGE("[R%d S%d]Failed to getStream.", request->getKey(), streamId);

                        /* Continue to handle other streams */
                        continue;
                    }

                    streamBuffer->buffer = outputBuffer[i].buffer;

                    streamBuffer->acquire_fence = -1;

                    if (request->getAcquireFenceDone(streamBuffer->buffer) == true) {
                        streamBuffer->release_fence = -1;
                    } else {
                        int err = sync_wait(outputBuffer[i].acquire_fence, 1000 /* msec */);
                        if (err >= 0) {
                            streamBuffer->release_fence = outputBuffer[i].acquire_fence;
                        } else {
                            streamBuffer->release_fence = -1;
                        }
                    }
                    CLOGD("[R%d S%d] acquire_fence(%d), release_fence(%d)",
                            request->getKey(), streamId, streamBuffer->acquire_fence, streamBuffer->release_fence);

                    streamBuffer->status = CAMERA3_BUFFER_STATUS_ERROR;

                    /* construct result for service */
                    requestResult->frame_number = requestKey;
                    requestResult->result = NULL;
                    requestResult->num_output_buffers = 1;
                    requestResult->output_buffers = streamBuffer;
                    requestResult->input_buffer = request->getInputBuffer();
                    requestResult->partial_result = 0;

                    m_bufferOnlyCallback(request, resultRequest, true);
                }
            }
        }
    } while (getServiceRequestCount() > 0);

    m_serviceRequests.clear();
    m_runningRequests.clear();

    m_requestFrameCountMap.clear();

    m_callbackFlushTimer.stop();

    long long FlushTime = m_callbackFlushTimer.durationMsecs();
    CLOGD("flush time(%lld)", FlushTime);

    stopThreadAndInputQ(m_resultNotifyCallbackThread, 1, &m_resultNotifyCallbackQ);
    stopThreadAndInputQ(m_resultStreamCallbackThread, 1, &m_resultStreamCallbackQ);
    stopThreadAndInputQ(m_resultMetaCallbackThread, 1, &m_resultMetaCallbackQ);
    stopThreadAndInputQ(m_callbackThread, 1, &m_callbackQ);

    m_callbackCounts.fill(0);

    if (m_notifySequencer != NULL) {
        m_notifySequencer->flush();
    }

    if (m_partialSequencer != NULL) {
        m_partialSequencer->flush();
    }

    if (m_allMetaSequencer != NULL) {
        m_allMetaSequencer->flush();
    }

    flushStreamSequencer();

    /* dump */
    if (getAllRequestCount() > 0)
        dump();

    m_setFlushFlag(false);

    CLOGD(" OUT---");

    return ret;
}

status_t ExynosCameraRequestManager::m_getKey(uint32_t *key, uint32_t frameCount)
{
    status_t ret = NO_ERROR;
    RequestFrameCountMapIterator iter;

    m_requestFrameCountMapLock.lock();
    iter = m_requestFrameCountMap.find(frameCount);
    if (iter != m_requestFrameCountMap.end()) {
        *key = iter->second;
    } else {
        CLOGE("get request key is failed. request for framecount(%d) is not EXIST", frameCount);
        ret = INVALID_OPERATION;
    }
    m_requestFrameCountMapLock.unlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_popKey(uint32_t *key, uint32_t frameCount)
{
    status_t ret = NO_ERROR;
    RequestFrameCountMapIterator iter;

    m_requestFrameCountMapLock.lock();
    iter = m_requestFrameCountMap.find(frameCount);
    if (iter != m_requestFrameCountMap.end()) {
        *key = iter->second;
        m_requestFrameCountMap.erase(iter);
    } else {
        CLOGE("get request key is failed. request for framecount(%d) is not EXIST", frameCount);
        ret = INVALID_OPERATION;
    }
    m_requestFrameCountMapLock.unlock();

    return ret;
}

uint32_t ExynosCameraRequestManager::m_generateResultKey()
{
    m_requestResultKeyLock.lock();
    uint32_t key = m_requestResultKey++;
    m_requestResultKeyLock.unlock();
    return key;
}

ResultRequest ExynosCameraRequestManager::createResultRequest(uint32_t requestKey,
                                                              uint32_t frameCount,
                                                              EXYNOS_REQUEST_RESULT::TYPE type)
{
    ResultRequest result;

    result = new ExynosCameraRequestResult(m_generateResultKey(), requestKey, frameCount, type);

    return result;
}

status_t ExynosCameraRequestManager::setCallbackOps(const camera3_callback_ops *callbackOps)
{
    status_t ret = NO_ERROR;
    m_callbackOps = callbackOps;
    return ret;
}

status_t ExynosCameraRequestManager::setFrameCount(uint32_t frameCount, uint32_t requestKey)
{
    status_t ret = NO_ERROR;
    pair<RequestFrameCountMapIterator, bool> listRet;
    ExynosCameraRequestSP_sprt_t request = NULL;

    m_requestFrameCountMapLock.lock();
    listRet = m_requestFrameCountMap.insert(pair<uint32_t, uint32_t>(frameCount, requestKey));
    if (listRet.second == false) {
        ret = INVALID_OPERATION;
        CLOGE("Failed, requestKey(%d) already exist!!", frameCount);
        m_requestFrameCountMapLock.unlock();
        return ret;
    }
    m_requestFrameCountMapLock.unlock();

    ret = m_get(requestKey, request, &m_runningRequests, &m_requestLock);
    if (ret < 0)
        CLOGE("m_get is failed. requestKey(%d)", requestKey);

    request->setFrameCount(frameCount);

    return ret;
}

// Count number of invocation and print FPS for every 30 frames.
void ExynosCameraRequestManager::m_debugCallbackFPS() {
#ifdef CALLBACK_FPS_CHECK
    m_callbackFrameCnt++;
    if(m_callbackFrameCnt % 30 == 0) {
        // Initial invocation
        m_callbackDurationTimer.start();
    } else if(m_callbackFrameCnt % 30 == 29) {
        m_callbackDurationTimer.stop();
        long long durationTime = m_callbackDurationTimer.durationMsecs();

        CLOGI("CALLBACK_FPS_CHECK, duration %lld", durationTime);
        //m_callbackFrameCnt = 0;
    }
#endif
}

status_t ExynosCameraRequestManager::m_releaseCameraMetadata(ExynosCameraRequestSP_sprt_t request, ResultRequest result)
{
    status_t ret = NO_ERROR;
    CameraMetadata resultMeta;
    camera3_capture_result_t *requestResult = NULL;
    int32_t *physCamIDs;
    static char temp_physCamIDs[3][3] = {0,};

    request->setRequestLock();
    resultMeta = *(request->getServiceMeta());
    request->setRequestUnlock();

    requestResult = result->getCaptureResult();
    if (requestResult == NULL) {
        CLOGE("[R%d F%d]Failed to getCaptureResult.",
                result->getRequestKey(), result->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    requestResult->result = resultMeta.release();
    if (requestResult->result == NULL) {
        CLOGE("[R%d F%d]Failed to release CameraMetadata",
                request->getKey(), request->getFrameCount());
        ret = INVALID_OPERATION;
        return ret;
    }

    request->setRequestLock();
    if (request->getNumOfPhysCamSettings() > 0) {
        requestResult->physcam_metadata = m_physicalIdsMetaData;
    }
    physCamIDs = request->getAllPhysCamInternalIDs();

    CLOGV("NumOfPhysCamSettings = %d", request->getNumOfPhysCamSettings());
    requestResult->num_physcam_metadata = request->getNumOfPhysCamSettings();
    if (requestResult->num_physcam_metadata) {
        result->setupForPhysCamMeta(requestResult);
        request->getPhysCamIDString((char **)requestResult->physcam_ids);
    }

    for (int i = 0; i < (int)request->getNumOfPhysCamSettings(); i++) {
        resultMeta = *(request->getServiceMetaPhysCam(physCamIDs[i]));
        requestResult->physcam_metadata[i] = resultMeta.release();
        if (requestResult->result == NULL) {
            CLOGE("[R%d F%d]Failed to release physcam_metadata[%d]",
                    request->getKey(), request->getFrameCount(), i);
            //fall back
            break;
        }
    }
    request->setRequestUnlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_sendCallbackResult(ExynosCameraRequestSP_sprt_t request, ResultRequest result)
{
    status_t ret = NO_ERROR;
    camera3_capture_result_t *capture_result = NULL;
    camera3_notify_msg_t *notify_msg = NULL;

    CLOGV("[R%d F%d T%d] m_sendCallbackResult",
            result->getRequestKey(), result->getFrameCount(), result->getType());

    switch(result->getType()){
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY:
        notify_msg = result->getNotifyMsg();
        if (request->getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ERROR) == true) {
            CLOGV("[R%d] skip NOTIFY_SHUTTER.", result->getRequestKey());
        } else {
            m_callbackOpsNotify(notify_msg);
        }

        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY:
        capture_result = result->getCaptureResult();
        m_callbackOpsCaptureResult(capture_result, result->getType());
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA:
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_SHUTTER:
        capture_result = result->getCaptureResult();

        /* Due to the limitations of the service, can not update the meta callback if update the notify error. */
        if (request->getSkipMetaResult() == true) {
            CLOGV("[R%d] skip CALLBACK_PARTIAL_3AA.", result->getRequestKey());
        } else {
            m_callbackOpsCaptureResult(capture_result, result->getType());
        }

        if (capture_result->result != NULL) {
            free_camera_metadata((camera_metadata_t *)(capture_result->result));
            capture_result->result = NULL;
        }
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT:
        capture_result = result->getCaptureResult();

        /* Due to the limitations of the service, can not update the meta callback if update the notify error. */
        if (request->getSkipMetaResult() == true) {
            CLOGV("[R%d] skip CALLBACK_ALL_RESULT.", result->getRequestKey());
        } else {
            m_callbackOpsCaptureResult(capture_result, result->getType());
        }

        if (capture_result->result != NULL) {
            free_camera_metadata((camera_metadata_t *)(capture_result->result));
            capture_result->result = NULL;
        }
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ERROR:
        notify_msg = result->getNotifyMsg();
        m_callbackOpsNotify(notify_msg);
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_INVALID:
    default:
        ret = BAD_VALUE;
        CLOGE("[R%d F%d T%d] callback type have INVALID value ",
                 result->getRequestKey(), result->getFrameCount(), result->getType());
        break;
    }

    m_lastResultKey[result->getType()] = result->getRequestKey();

    return ret;
}

status_t ExynosCameraRequestManager::m_callbackOpsCaptureResult(camera3_capture_result_t *result, __unused EXYNOS_REQUEST_RESULT::TYPE type)
{
    status_t ret = NO_ERROR;

#ifdef DEBUG_STREAM_CONFIGURATIONS
    CLOGD("DEBUG_STREAM_CONFIGURATIONS:frame number(%d), type(%d), #out(%d)",
             result->frame_number, type, result->num_output_buffers);
#else
    CLOGV("frame number(%d), #out(%d)",
             result->frame_number, result->num_output_buffers);
#endif

#ifdef DEBUG_IRIS_LEAK
#endif

    TIME_LOGGER_UPDATE(m_cameraId, result->frame_number, result->partial_result, INTERVAL, UPDATE_RESULT, 0);


    if (result->result != NULL) {
        ret = validate_camera_metadata_structure(result->result, NULL);
        if (ret) CLOGE("ERR!!!");
    }

    CLOG_PERFRAME(PATH, m_cameraId, m_name, nullptr, nullptr, result->frame_number,
            "RST(%d), outbufs(%d), partial(%d)",
            type, result->num_output_buffers, result->partial_result);

    m_callbackOps->process_capture_result(m_callbackOps, result);

    ////////////////////////////////////////////////
    // enable when you need debug.
    //m_printAllResultInfo(result, type);

    return ret;
}

status_t ExynosCameraRequestManager::m_callbackOpsNotify(camera3_notify_msg_t *msg)
{
    status_t ret = NO_ERROR;

#ifdef DEBUG_IRIS_LEAK
#endif

    switch (msg->type) {
    case CAMERA3_MSG_ERROR:
        CLOGW("msg frame(%d) type(%d) errorCode(%d)",
                 msg->message.error.frame_number, msg->type, msg->message.error.error_code);
        m_callbackOps->notify(m_callbackOps, msg);
        break;
    case CAMERA3_MSG_SHUTTER:
        CLOGV("msg frame(%d) type(%d) timestamp(%ju)",
                 msg->message.shutter.frame_number, msg->type, msg->message.shutter.timestamp);

        CLOG_PERFRAME(PATH, m_cameraId, m_name, nullptr, nullptr, msg->message.shutter.frame_number,
                "notify(%d), time(%lld)",
                msg->type,
                msg->message.shutter.timestamp);

        m_callbackOps->notify(m_callbackOps, msg);
        break;
    default:
        CLOGE("Msg type is invalid (%d)", msg->type);
        ret = BAD_VALUE;
        break;
    }
    return ret;
}

/* Increase the pipeline depth value from each request in running request map */
status_t ExynosCameraRequestManager::m_increasePipelineDepth(RequestInfoMap *map, Mutex *lock)
{
    status_t ret = NO_ERROR;
    RequestInfoMapIterator requestIter;
    ExynosCameraRequestSP_sprt_t request = NULL;

    lock->lock();
    if (map->size() < 1) {
        CLOGV("map is empty. Skip to increase the pipeline depth");
        ret = NO_ERROR;
        goto func_exit;
    }

    requestIter = map->begin();
    while (requestIter != map->end()) {
        request = requestIter->second;

        request->increasePipelineDepth();
        requestIter++;
    }

func_exit:
    lock->unlock();
    return ret;
}

status_t ExynosCameraRequestManager::pushResultRequest(ResultRequest result)
{
    ExynosCameraRequestSP_sprt_t request = NULL;
    status_t ret = NO_ERROR;
    ResultList list;
    uint32_t requestCount = 0;
    int32_t resultStreamId = -1;
    callback_sequence_t sequencer = NULL;

    if (m_resultNotifyCallbackThread->isRunning() == false) {
        m_resultNotifyCallbackThread->run();
    }

    if (m_resultStreamCallbackThread->isRunning() == false) {
        m_resultStreamCallbackThread->run();
    }

    if (m_resultMetaCallbackThread->isRunning() == false) {
        m_resultMetaCallbackThread->run();
    }

    if (m_callbackThread->isRunning() == false) {
        m_callbackThread->run();
    }

    if (result == NULL) {
        CLOGE("Result is NULL");
        return BAD_VALUE;
    }

    requestCount = result->getRequestKey();

    CLOGV("[R%d F%d T%d] %d %d %d sequence",
            result->getRequestKey(), result->getFrameCount(), result->getType(), m_resultNotifyCallbackQ.getSizeOfProcessQ(), m_resultStreamCallbackQ.getSizeOfProcessQ(), m_resultMetaCallbackQ.getSizeOfProcessQ());

    switch(result->getType()){
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY:
        m_notifySequencer->pushResult(requestCount, result);
        m_resultNotifyCallbackQ.pushProcessQ(&requestCount);
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ERROR:
        ret = m_get(result->getRequestKey(), request, &m_runningRequests, &m_requestLock);
        if (ret < NO_ERROR) {
            CLOGE("[R%d F%d] m_get is failed.", result->getRequestKey(), result->getFrameCount());
            return ret;
        }
        m_notifyErrorCallback(request, result);
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY:
        resultStreamId = result->getStreamId();
        sequencer = m_streamsSequencerList[resultStreamId];
        sequencer->pushResult(requestCount, result);
        m_resultStreamCallbackQ.pushProcessQ(&requestCount);
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA:
        m_partialSequencer->pushResult(requestCount, result);
        m_resultStreamCallbackQ.pushProcessQ(&requestCount);
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_SHUTTER:
        m_partialSequencer->pushResult(requestCount, result);
        m_resultStreamCallbackQ.pushProcessQ(&requestCount);
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT:
        m_allMetaSequencer->pushResult(requestCount, result);
        m_resultMetaCallbackQ.pushProcessQ(&requestCount);
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_INVALID:
    default:
        CLOGE("[R%d F%d T%d] callback type have INVALID value ",
                 result->getRequestKey(), result->getFrameCount(), result->getType());
        break;
    }

    return NO_ERROR;
}

void ExynosCameraRequestManager::m_setFlushFlag(bool flag)
{
    Mutex::Autolock l(m_flushLock);
    m_flushFlag = flag;
}

bool ExynosCameraRequestManager::m_getFlushFlag(void)
{
    Mutex::Autolock l(m_flushLock);
    return m_flushFlag;
}

void ExynosCameraRequestManager::m_waitFlushDone(void)
{
    int32_t retryCount = 100;
    while (m_getFlushFlag() == true && retryCount-- > 0) {
        CLOGD("Wait flush done. retryCount %d", retryCount);
        usleep(1000); //1msec
    }
}

int32_t ExynosCameraRequestManager::getResultRenew(void)
{
    return m_resultRenew;
}

void ExynosCameraRequestManager::incResultRenew(void)
{
    if (m_getFlushFlag() == true) {
        CLOGV("flush flag is set. Do not increase the result renew count");
    } else if (getAllRequestCount() == 0) {
        if (m_printInterval++ % 5 == 0)
            CLOGW("No request! resultRenew %d", m_resultRenew);
    } else {
        m_resultRenew++;
    }
}

void ExynosCameraRequestManager::resetResultRenew(void)
{
    m_resultRenew = 0;
}

void ExynosCameraRequestManager::dump(void)
{
    CLOGD("AllRequestCount(%d), ServiceRequestCount(%d), RunningRequestCount(%d)",
            getAllRequestCount(), getServiceRequestCount(), getRunningRequestCount());

    CLOGD("----- Last Result Key -----");
    for (int i = 0; i < EXYNOS_REQUEST_RESULT::CALLBACK_MAX; i++)
        CLOGI("Type[%d] = Last Key(%d)", i, m_lastResultKey[i]);

    CLOGD("----- All Remained Request Info (m_serviceRequests-----");
    m_printAllServiceRequestInfo();

    CLOGD("----- All Remained Request Info (m_runningRequests-----");
    m_printAllRequestInfo(&m_runningRequests, &m_requestLock);
}

bool ExynosCameraRequestManager::m_requestDeleteFunc(ExynosCameraRequestSP_sprt_t curRequest)
{
    status_t ret = NO_ERROR;

    if (curRequest == NULL) {
        CLOGE("Request is NULL");
        ret = INVALID_OPERATION;
        return ret;
    }

    CLOGV("[R%d] Try to delete request. RunningRequestList size(%d)",
        curRequest->getKey(), getRunningRequestCount());

    if (curRequest->isComplete()
        && curRequest->isAllBufferCallbackDone() == true) {
        CLOGV("[R%d] m_removeFromRunningList.", curRequest->getKey());
        m_removeFromRunningList(curRequest->getKey());
    } else {
        CLOGV("[R%d] Request is not deleted yet.", curRequest->getKey());
    }

    return ret;
}

bool ExynosCameraRequestManager::m_resultNotifyCallbackThreadFunc(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraRequestSP_sprt_t request = NULL;
    uint32_t requestCount = 0;
    uint32_t wishCount = 0;
    uint32_t callbackCount = 0;

    ret = m_resultNotifyCallbackQ.waitAndPopProcessQ(&requestCount);
    if (ret == TIMED_OUT) {
        CLOGV("queue wait timeout");
        goto func_exit;
    } else if (ret != NO_ERROR) {
        CLOGE("queue wait and pop fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        goto func_exit;
    }

    ret = m_notifySequencer->getFrontKey(wishCount);
    if (ret < 0) {
        CLOGE("Sequencer getFrontKey failed ret(%d) key(%d)", ret, wishCount);
        goto func_exit;
    }

    ret = m_get(wishCount, request, &m_runningRequests, &m_requestLock);
    if (ret < NO_ERROR) {
        CLOGE("[R%d T%d] m_get is failed.", requestCount);
        goto func_exit;
    }

    CLOGV("[R%d] callbackQ(%d) request(%d)", wishCount, m_resultNotifyCallbackQ.getSizeOfProcessQ(), request->getKey());

    m_notifyCallback(request);
    if (ret != NO_ERROR) {
        CLOGE("ResultCallback fail, ret(%d)", ret);
        /* TODO: doing exception handling */
    } else {
        m_resultRenew = 0;
    }

    callbackCount = m_callbackCounts[EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY];

    if (callbackCount == 0) {
        m_resultNotifyCallbackQ.pushProcessQ(&requestCount);
    } else {
        for (int i = 1; i < callbackCount ; i++) {
            if (m_resultNotifyCallbackQ.getSizeOfProcessQ() > 0) {
                ret = m_resultNotifyCallbackQ.popProcessQ(&requestCount);
            } else {
                CLOGE("don't have enough notify callbackQ(%d) callback(%d)", m_resultNotifyCallbackQ.getSizeOfProcessQ(), callbackCount);
                ret = m_resultNotifyCallbackQ.waitAndPopProcessQ(&requestCount);
            }
            if (ret == TIMED_OUT) {
                CLOGV("queue wait timeout");
                return ret;
            } else if (ret != NO_ERROR) {
                CLOGE("queue wait and pop fail, ret(%d)", ret);
                /* TODO: doing exception handling */
                return ret;
            }
        }
    }
    m_callbackCounts[EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY] = 0;

func_exit:
    return true;
}

bool ExynosCameraRequestManager::m_resultStreamCallbackThreadFunc(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraRequestSP_sprt_t request = NULL;
    uint32_t requestCount = 0;
    uint32_t wishCount = 0;
    CallbackListkeys keyList;
    CallbackListkeys streamKeyList;
    CallbackListkeys streamIdList;
    callback_sequence_t sequencer = NULL;
    uint32_t callbackCount = 0;

    ret = m_resultStreamCallbackQ.waitAndPopProcessQ(&requestCount);
    if (ret == TIMED_OUT) {
        CLOGV("queue wait timeout");
        return ret;
    } else if (ret != NO_ERROR) {
        CLOGE("queue wait and pop fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        return ret;
    }

    if (m_partialSequencer->getKeySize() > 0) {
        m_partialSequencer->getKeyList(&keyList);

        for (CallbackListkeysIter iter = keyList.begin(); iter != keyList.end(); iter++) {
            wishCount = *iter;
            CLOGV("[R%d] callbackQ(%d) keySize(%d)", wishCount, m_resultStreamCallbackQ.getSizeOfProcessQ(), keyList.size());
            ret = m_get(wishCount, request, &m_runningRequests, &m_requestLock);
            if (ret < NO_ERROR) {
                CLOGE("[R%d] m_get is failed.", wishCount);
                return ret;
            }

            m_partialMetaCallback(request);
            if (ret != NO_ERROR) {
                CLOGE("ResultCallback fail, ret(%d)", ret);
                /* TODO: doing exception handling */
            } else {
                m_resultRenew = 0;
            }
        }
    }

    keyList.clear();

    for(stream_callback_sequence_iter_t iter = m_streamsSequencerList.begin() ; iter != m_streamsSequencerList.end(); iter++) {
        streamIdList.push_back(iter->first);
        sequencer = iter->second;
        sequencer->getKeyList(&keyList);
        streamKeyList.insert(streamKeyList.begin(), keyList.begin(), keyList.end());
    }

    streamKeyList.sort();
    streamKeyList.unique();

    for (CallbackListkeysIter iter = streamKeyList.begin(); iter != streamKeyList.end(); iter++) {
        wishCount = *iter;
        CLOGV("[R%d] callbackQ(%d)", wishCount, m_resultStreamCallbackQ.getSizeOfProcessQ());

        ret = m_get(wishCount, request, &m_runningRequests, &m_requestLock);
        if (ret < NO_ERROR) {
            CLOGE("[R%d] m_get is failed.", wishCount);
            return ret;
        }

        ret = m_bufferOnlyCallback(request);
        if (ret != NO_ERROR) {
            CLOGE("ResultCallback fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        } else {
            m_resultRenew = 0;
        }
    }

    callbackCount = m_callbackCounts[EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY] + m_callbackCounts[EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA];

    if (callbackCount == 0) {
        m_resultStreamCallbackQ.pushProcessQ(&requestCount);
    } else {
        for (int i = 1; i < callbackCount ; i++) {
            if (m_resultStreamCallbackQ.getSizeOfProcessQ() > 0) {
                ret = m_resultStreamCallbackQ.popProcessQ(&requestCount);
            } else {
                CLOGD("don't have enough notify callbackQ(%d) callback(%d)", m_resultStreamCallbackQ.getSizeOfProcessQ(), callbackCount);
                ret = m_resultStreamCallbackQ.waitAndPopProcessQ(&requestCount);
            }
            if (ret == TIMED_OUT) {
                CLOGV("queue wait timeout");
                return ret;
            } else if (ret != NO_ERROR) {
                CLOGE("queue wait and pop fail, ret(%d)", ret);
                /* TODO: doing exception handling */
                return ret;
            }
        }
    }

    m_callbackCounts[EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY] = 0;
    m_callbackCounts[EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA] = 0;

    return true;
}

bool ExynosCameraRequestManager::m_resultMetaCallbackThreadFunc(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraRequestSP_sprt_t request = NULL;
    uint32_t requestCount = 0;
    uint32_t wishCount = 0;
    uint32_t callbackCount = 0;

    ret = m_resultMetaCallbackQ.waitAndPopProcessQ(&requestCount);
    if (ret == TIMED_OUT) {
        CLOGV("queue wait timeout");
        goto func_exit;
    } else if (ret != NO_ERROR) {
        CLOGE("queue wait and pop fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        goto func_exit;
    }

    ret = m_allMetaSequencer->getFrontKey(wishCount);
    if (ret < 0) {
        CLOGE("Sequencer getFrontKey failed ret(%d) key(%d)", ret, wishCount);
        goto func_exit;
    }

    ret = m_get(wishCount, request, &m_runningRequests, &m_requestLock);
    if (ret < NO_ERROR) {
        CLOGE("[R%d T%d] m_get is failed.", requestCount);
        goto func_exit;
    }

    CLOGV("[R%d] callbackQ(%d)", wishCount, m_resultMetaCallbackQ.getSizeOfProcessQ());

    m_allMetaCallback(request);
    if (ret != NO_ERROR) {
        CLOGE("ResultCallback fail, ret(%d)", ret);
        /* TODO: doing exception handling */
    } else {
        m_resultRenew = 0;
    }

    callbackCount = m_callbackCounts[EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT];

    if (callbackCount == 0) {
        m_resultMetaCallbackQ.pushProcessQ(&requestCount);
    } else {
        for (int i = 1; i < callbackCount ; i++) {
            if (m_resultMetaCallbackQ.getSizeOfProcessQ() > 0) {
                ret = m_resultMetaCallbackQ.popProcessQ(&requestCount);
            } else {
                CLOGD("don't have enough notify callbackQ(%d) callback(%d)", m_resultMetaCallbackQ.getSizeOfProcessQ(), callbackCount);
                ret = m_resultMetaCallbackQ.waitAndPopProcessQ(&requestCount);
            }
            if (ret == TIMED_OUT) {
                CLOGV("queue wait timeout");
                return ret;
            } else if (ret != NO_ERROR) {
                CLOGE("queue wait and pop fail, ret(%d)", ret);
                /* TODO: doing exception handling */
                return ret;
            }
        }
    }

    m_callbackCounts[EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT] = 0;

func_exit:
    return true;
}

bool ExynosCameraRequestManager::m_resultCallbackThreadFunc(void)
{
    CLOGV("-IN-");
    status_t ret = NO_ERROR;

    ExynosCameraRequestSP_sprt_t request = NULL;
    ResultRequest result = NULL;
    ResultItem item = NULL;

    ret = m_callbackQ.waitAndPopProcessQ(&item);
    if (ret == TIMED_OUT) {
        CLOGV("m_callbackQ wait timeout");
        return ret;
    } else if (ret != NO_ERROR) {
        CLOGE("m_callbackQ wait and pop fail, ret(%d)", ret);
        /* TODO: doing exception handling */
        return ret;
    }

    if (item == NULL) {
        CLOGE("item is NULL");
        return BAD_VALUE;
    }

    request = item->getRequest();
    if (request == NULL) {
        CLOGE("request is NULL");
        return BAD_VALUE;
    }

    result = item->getResult();
    if (result == NULL) {
        CLOGE("result is NULL");
        return BAD_VALUE;
    }

    CLOGV("[R%d T%d] resultCallbackQ(%d)",
            request->getKey(), result->getType(), m_callbackQ.getSizeOfProcessQ());

    switch(result->getType()){
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA:
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_SHUTTER:
    case EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT:
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ERROR:
        m_sendCallbackResult(request, result);
        m_requestDeleteFunc(request);
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_INVALID:
    default:
        CLOGE("[R%d T%d] callback type is INVALID.", request->getKey(), result->getType());
        break;
    }

    CLOGV("-OUT-");

    return true;
}

bool ExynosCameraRequestManager::m_notifyCallback(ExynosCameraRequestSP_sprt_t curRequest, ResultRequest result)
{
    CLOGV("-IN-");
    status_t ret = NO_ERROR;
    uint32_t wishCount = 0;
    static int printNotifyCount = 0;
    ResultList resultList;
    ResultListIter iter;
    ResultRequest newResult = NULL;

    EXYNOS_REQUEST_RESULT::TYPE cbType = EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY;

    if (curRequest == NULL) {
        CLOGE("request is NULL");
        ret = INVALID_OPERATION;
        return ret;
    }

    ret = m_notifySequencer->getFrontKey(wishCount);
    if (ret < 0) {
        CLOGE("Sequencer getFrontKey failed ret(%d) key(%d)", ret, wishCount);
        return ret;
    }

    CLOGV("[R%d F%d W%d] NotifyCallback state(%d)", curRequest->getKey(),
            curRequest->getFrameCount(), wishCount,
            curRequest->getCallbackDone(cbType));

    if (curRequest->getCallbackDone(cbType) == false) {
        if (curRequest->getKey() == wishCount
            && (m_notifySequencer->isExistResult(wishCount) > 0 || result != NULL)) {
            CLOGV("[R%d F%d W%d] NotifyCallback state(%d) resultSize(%d)", curRequest->getKey(),
                    curRequest->getFrameCount(), wishCount,
                    curRequest->getCallbackDone(cbType), m_notifySequencer->isExistResult(wishCount));
            if (result != NULL) {
                resultList.push_back(result);
            } else {
                ResultRequest result;
                m_notifySequencer->popResult(wishCount, result);
                resultList.push_back(result);
            }

            for(iter = resultList.begin(); iter != resultList.end(); iter++) {
                newResult = *iter;
                ResultItem item = NULL;
                item = new ResultCallback(curRequest, newResult);
                CLOGV("[R%d T%d] pushQ notify result callback", newResult->getRequestKey(), newResult->getType());
                m_callbackQLock.lock();
                curRequest->setCallbackDone(cbType, true);
                m_callbackQ.pushProcessQ(&item);
                m_callbackQLock.unlock();
                printNotifyCount = 0;
            }
            m_callbackCounts[cbType] += resultList.size();
            m_notifySequencer->deleteKey(wishCount);
        } else if (curRequest->getKey() > wishCount) {
            if ((printNotifyCount % 100) == 0) {
                printNotifyCount++;
                CLOGW("[T%d R%d F%d W%d] Notify result is pushed back in resultCallbackQ(%d)",
                        cbType, curRequest->getKey(), curRequest->getFrameCount(),
                        wishCount, m_resultNotifyCallbackQ.getSizeOfProcessQ());
            }
            {
                /* time delay to prevent busy waiting */
                uint32_t minFps = 0;
                uint32_t maxFps = 0;

                m_configurations->getPreviewFpsRange(&minFps, &maxFps);
                if (maxFps <= 30) {
                    CLOGV("usleep(%d, %d)", minFps, maxFps);
                    usleep(500); /* 500us */
                }
            }

            if ((m_notifySequencer->getKeySize() == 0) && (getRunningRequestCount() > 0)) {
                dump();
                android_printAssert(NULL, LOG_TAG,
                            "[R%d F%d]ASSERT(%s):wish Meta Count Size(%d), runningRequests size(%d)",
                            curRequest->getKey(), curRequest->getFrameCount(), __FUNCTION__,
                            m_notifySequencer->getKeySize(), getRunningRequestCount());
            }
        }
    } else {
        CLOGV("[R%d W%d] Notify result is already sent", curRequest->getKey(), wishCount);
    }

    CLOGV("-END-");

    return ret;
}

bool ExynosCameraRequestManager::m_partialMetaCallback(ExynosCameraRequestSP_sprt_t curRequest, ResultRequest result, bool flagFlush)
{
    CLOGV("-IN-");
    status_t ret = NO_ERROR;
    ResultList resultList;
    ResultListIter iter;
    uint32_t requestCount = 0;
    ResultRequest newResult = NULL;
    uint32_t wishCount = 0;
    uint32_t callbackCount = 0;
    EXYNOS_REQUEST_RESULT::TYPE checkCbType = EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY;

    EXYNOS_REQUEST_RESULT::TYPE cbType = EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA;

    if (curRequest == NULL) {
        CLOGE("request is NULL");
        ret = INVALID_OPERATION;
        return ret;
    }

    CLOGV("[R%d] 3aaMetaCallback state(%d)", curRequest->getKey(), curRequest->getCallbackDone(cbType));

    if (curRequest->getCallbackDone(checkCbType)
        && curRequest->getCallbackDone(cbType) == false
        && (m_partialSequencer->isExistResult(curRequest->getKey()) > 0 || result != NULL)) {
        if (result != NULL) {
            resultList.push_back(result);
        } else {
            ResultRequest result;
            m_partialSequencer->popResult(curRequest->getKey(), result);
            resultList.push_back(result);
        }

        for(iter = resultList.begin(); iter != resultList.end(); iter++) {
            newResult = *iter;

            if (flagFlush == true) {
                m_sendCallbackResult(curRequest, result);
                curRequest->setCallbackDone(cbType, true);
                m_requestDeleteFunc(curRequest);
            } else {
                ResultItem item = NULL;
                item = new ResultCallback(curRequest, newResult);
                m_callbackQLock.lock();
                curRequest->setCallbackDone(cbType, true);
                m_callbackQ.pushProcessQ(&item);
                m_callbackQLock.unlock();
            }

            CLOGV("[R%d T%d] pushQ partial result callback", newResult->getRequestKey(), newResult->getType());

            CLOG_PERFORMANCE(FPS, m_cameraId, 0, DURATION, PATIAL_RESULT, 0, curRequest->getKey(), nullptr);
        }
        m_callbackCounts[cbType] += resultList.size();
        m_partialSequencer->deleteKey(curRequest->getKey());
    } else {
        CLOGV("[R%d] 3aaMeta result is already sent or OutOfOrder notify(%d)", curRequest->getKey(), curRequest->getCallbackDone(checkCbType));
    }

    CLOGV("-END-");

    return ret;
}

bool ExynosCameraRequestManager::m_allMetaCallback(ExynosCameraRequestSP_sprt_t curRequest, ResultRequest result, bool flagFlush)
{
    CLOGV("-IN-");
    status_t ret = NO_ERROR;
    uint32_t wishCount = 0;
    static int printAllMetaCount = 0;
    ResultList resultList;
    ResultListIter iter;
    uint32_t requestCount = 0;
    ResultRequest newResult = NULL;

    EXYNOS_REQUEST_RESULT::TYPE cbType = EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT;

    if (curRequest == NULL) {
        CLOGE("request is NULL");
        ret = INVALID_OPERATION;
        return ret;
    }

    ret = m_allMetaSequencer->getFrontKey(wishCount);
    if (ret < 0) {
        CLOGE("Sequencer getFrontKey failed ret(%d) key(%d)", ret, wishCount);
        return ret;
    }

    CLOGV("[R%d F%d W%d] AllMetaCallback state(%d)", curRequest->getKey(),
            curRequest->getFrameCount(), wishCount,
            curRequest->getCallbackDone(cbType));

    if (curRequest->getCallbackDone(cbType) == false) {
        if (curRequest->getKey() == wishCount
            && curRequest->getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA) == true
            && (m_allMetaSequencer->isExistResult(wishCount) > 0 || result != NULL)) {

            if (result != NULL) {
                resultList.push_back(result);
            } else {
                ResultRequest result;
                m_allMetaSequencer->popResult(wishCount, result);
                resultList.push_back(result);
            }

            for(iter = resultList.begin(); iter != resultList.end(); iter++) {
                newResult = *iter;
                /* Adjust face detection metadata for the order of request and frame drop */
                m_adjustFaceDetectMetadata(curRequest);

                ret = m_releaseCameraMetadata(curRequest, newResult);
                if (ret != NO_ERROR) {
                    CLOGE("[R%d]Failed to releaseCameraMetadata. ret %d",
                            curRequest->getKey(), ret);
                    return ret;
                }

                CLOG_PERFORMANCE(FPS, m_cameraId, 0, DURATION, ALL_RESULT, 0, curRequest->getKey(), nullptr);
                CLOG_PERFORMANCE(FPS, m_cameraId, 0, INTERVAL, ALL_RESULT, 0, curRequest->getKey(), nullptr);

                if ((uint32_t)curRequest->getCompleteBufferCount() != curRequest->getNumOfOutputBuffer()) {
                    CLOGV("[R%d F%d]. AllMeta result is pushed but not complete output buffer(%u / %d)", curRequest->getKey(), curRequest->getFrameCount(), curRequest->getNumOfOutputBuffer(), curRequest->getCompleteBufferCount());
                }

                if (flagFlush == true) {
                    m_sendCallbackResult(curRequest, newResult);
                    curRequest->setCallbackDone(cbType, true);
                    m_requestDeleteFunc(curRequest);
                } else {
                    ResultItem item = NULL;
                    item = new ResultCallback(curRequest, newResult);
                    m_callbackQLock.lock();
                    curRequest->setCallbackDone(cbType, true);
                    m_callbackQ.pushProcessQ(&item);
                    m_callbackQLock.unlock();

                    /* This sleep to prevent too many allMeta results blocked all other service call */
                    usleep(5000);
                }

                CLOGV("[R%d T%d] pushQ allMeta result callback", newResult->getRequestKey(), newResult->getType());
            }
            m_callbackCounts[cbType] += resultList.size();
            m_allMetaSequencer->deleteKey(wishCount);
        } else if (curRequest->getKey() > wishCount
                   || curRequest->getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA) == false) {
            if ((printAllMetaCount % 100) == 0) {
                printAllMetaCount++;
                CLOGW("[R%d F%d W%d 3S%d]. AllMeta result is pushed back in resultCallbackQ(%d)",
                        curRequest->getKey(), curRequest->getFrameCount(), wishCount,
                        curRequest->getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA),
                        m_resultMetaCallbackQ.getSizeOfProcessQ());
            }
            {
                /* time delay to prevent busy waiting */
                uint32_t minFps = 0;
                uint32_t maxFps = 0;

                m_configurations->getPreviewFpsRange(&minFps, &maxFps);
                if (maxFps <= 30) {
                    CLOGV("usleep(%d, %d)", minFps, maxFps);
                    usleep(500); /* 500us */
                }
            }

            if ((m_allMetaSequencer->getKeySize() == 0) && (getRunningRequestCount() > 0)) {
                dump();
                android_printAssert(NULL, LOG_TAG,
                            "[R%d F%d]ASSERT(%s):wish Meta Count Size(%d), runningRequests size(%d)",
                            curRequest->getKey(), curRequest->getFrameCount(), __FUNCTION__,
                            m_allMetaSequencer->getKeySize(), getRunningRequestCount());
            }
        }
    } else {
        CLOGV("[R%d W%d] AllMeta result is already sent", curRequest->getKey(), wishCount);
    }

    CLOGV("-END-");

    return ret;
}

bool ExynosCameraRequestManager::m_bufferOnlyCallback(ExynosCameraRequestSP_sprt_t curRequest, ResultRequest result, bool flagFlush)
{
    CLOGV("-IN-");
    status_t ret = NO_ERROR;

    camera3_stream_t *resultStream = NULL;
    camera3_stream_buffer_t *streamBuffer = NULL;
    ExynosCameraStream *privStreamInfo = NULL;
    camera3_capture_result_t *capture_result = NULL;
    int32_t resultStreamId = -1;
    __unused uint32_t max_fps = 0, min_fps = 0;
    int32_t inputStreamId = -1;
    ResultRequest newResult = NULL;
    uint32_t wishCount = 0;
    callback_sequence_t sequencer = NULL;

    EXYNOS_REQUEST_RESULT::TYPE cbType = EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY;

    if (curRequest == NULL) {
        CLOGE("request is NULL");
        ret = INVALID_OPERATION;
        return ret;
    }

    if (curRequest->getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY)) {
        ResultList resultList;
        if (result == NULL) {
            for (size_t i = 0; i < curRequest->getNumOfOutputBuffer(); i++) {
                resultStreamId = curRequest->getStreamIdwithBufferIdx(i);
                sequencer = m_streamsSequencerList[resultStreamId];
                ret = sequencer->getFrontKey(wishCount);
                if (ret < 0) {
                    CLOGE("Sequencer getFrontKey failed ret(%d) key(%d)", ret, wishCount);
                    continue;
                }
                if (curRequest->getCallbackStreamDone(resultStreamId) == false
                    && wishCount == curRequest->getKey()
                    && sequencer->isExistResult(wishCount)) {
                    ResultRequest item;
                    sequencer->popResult(wishCount, item);
                    resultList.push_back(item);
                } else {
                    continue;
                }
            }
        } else {
            resultStreamId = result->getStreamId();
            sequencer = m_streamsSequencerList[resultStreamId];

            ret = sequencer->getFrontKey(wishCount);
            if (ret < 0) {
                CLOGE("Sequencer getFrontKey failed ret(%d) key(%d)", ret, wishCount);
            }

            if (curRequest->getCallbackStreamDone(resultStreamId) == false
                && wishCount == curRequest->getKey()) {
                resultList.push_back(result);
            }
        }

        for (ResultListIter iter = resultList.begin(); iter != resultList.end(); iter++) {
            newResult = *iter;

            resultStreamId = newResult->getStreamId();
            sequencer = m_streamsSequencerList[resultStreamId];

            TIME_LOGGER_UPDATE(m_cameraId, curRequest->getKey(), 0, CUMULATIVE_CNT, RESULT_CALLBACK, 0);
#ifdef DEBUG_PREVIEW_STREAM_PROFLIE
            if (resultStreamId % HAL_STREAM_ID_MAX == HAL_STREAM_ID_PREVIEW) {
                if (streamBuffer->status == CAMERA3_BUFFER_STATUS_OK) {
                    CLOGD("[R%d S%d(%d)] BufferOnlyCallback state(%d) trace prevew buffer",
                            curRequest->getKey(), resultStreamId, resultStreamId % HAL_STREAM_ID_MAX,
                            streamBuffer->status);
                } else {
                    CLOGD("[R%d S%d(%d)] BufferOnlyCallback state(%d) error trace prevew buffer",
                            curRequest->getKey(), resultStreamId, resultStreamId % HAL_STREAM_ID_MAX,
                            streamBuffer->status);
                }
            }
#endif
            capture_result = newResult->getCaptureResult();
            if (capture_result->input_buffer != NULL) {
                List<int> *list = NULL;
                List<int>::iterator iter;
                curRequest->getAllRequestInputStreams(&list);
                for(iter = list->begin() ; iter != list->end(); iter++) {
                    inputStreamId = *iter;
                    if (curRequest->getCallbackStreamDone(*iter) == true) {
                        capture_result->input_buffer = NULL;
                    }
                }
            }

            CLOGV("[R%d T%d] pushQ BufferOnly result callback", newResult->getRequestKey(), newResult->getType());

            curRequest->increaseCompleteBufferCount();
            curRequest->setCallbackStreamDone(resultStreamId, true);
            if (capture_result->input_buffer != NULL) {
                curRequest->setCallbackStreamDone(inputStreamId, true);
            }

            if (flagFlush == true) {
                m_sendCallbackResult(curRequest, newResult);
            } else {
                m_callbackQLock.lock();
                if (curRequest->isAllBufferCallbackDone() == true) {
                    curRequest->setCallbackDone(cbType, true);
                }
                ResultItem item = new ResultCallback(curRequest, newResult);
                m_callbackQ.pushProcessQ(&item);
                m_callbackQLock.unlock();
            }

            CLOG_PERFRAME(BUF, m_cameraId, m_name, nullptr, (void *)(streamBuffer),
                    curRequest->getKey(), "[CALLBACK]");

            if (flagFlush == true) {
                m_requestDeleteFunc(curRequest);
            }

            sequencer->deleteKey(newResult->getRequestKey());
            CLOGV("[R%d T%d] pushQ BufferOnly result callback Done", newResult->getRequestKey(), newResult->getType());
        }
        m_callbackCounts[cbType] += resultList.size();
    } else {
        CLOGV("[R%d S%d(%d)] BufferOnly result is already sent.",
            curRequest->getKey(), resultStreamId, resultStreamId % HAL_STREAM_ID_MAX);
    }

    CLOGV("-END-");

    return ret;
}

bool ExynosCameraRequestManager::m_notifyErrorCallback(ExynosCameraRequestSP_sprt_t curRequest, ResultRequest result, bool flagFlush)
{
    CLOGV("-IN-");
    status_t ret = NO_ERROR;
    ResultRequest newResult = NULL;
    EXYNOS_REQUEST_RESULT::TYPE cbType = EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ERROR;

    if (curRequest == NULL) {
        CLOGE("request is NULL");
        ret = INVALID_OPERATION;
        return ret;
    }

    CLOGV("[R%d F%d] notifyErrorCallback state(%d)",
            curRequest->getKey(), curRequest->getFrameCount(),
            curRequest->getCallbackDone(cbType));

    if (curRequest->isAllBufferCallbackDone() == false) {
        curRequest->setSkipMetaResult(true);
    }

    if (flagFlush == true) {
        m_sendCallbackResult(curRequest, result);
        curRequest->setCallbackDone(cbType, true);
        m_requestDeleteFunc(curRequest);
    } else {
        ResultItem item = NULL;
        item = new ResultCallback(curRequest, result);
        m_callbackQLock.lock();
        curRequest->setCallbackDone(cbType, true);
        m_callbackQ.pushProcessQ(&item);
        m_callbackQLock.unlock();
    }

    CLOGV("-END-");

    return ret;
}


status_t ExynosCameraRequestManager::waitforRequestflush()
{
    int count = 0;

    CLOGD("m_serviceRequest size(%zu) m_runningRequest size(%zu)",
           m_serviceRequests.size(), m_runningRequests.size());

    while (true) {
        if (m_serviceRequests.size() == 0 && m_runningRequests.size() == 0)
            break;

        usleep(50000);

        if (count++ == 200)
            break;
    }

    if (count > 200) {
        CLOGW("m_serviceRequest size(%zu) m_runningRequest size(%zu), count(%d)",
               m_serviceRequests.size(), m_runningRequests.size(), count);
    } else {
        CLOGD("Done : count(%d)", count);
    }

    return 0;
}

void ExynosCameraRequestManager::m_adjustFaceDetectMetadata(ExynosCameraRequestSP_sprt_t request)
{
    struct camera2_shot_ext *shot_ext = NULL;

    shot_ext = request->getServiceShot();
    if (shot_ext == NULL) {
        CLOGE("[R%d F%d]Failed to getServiceShot.",
                request->getKey(), request->getFrameCount());
        return;
    }

    if (shot_ext->shot.ctl.stats.faceDetectMode > FACEDETECT_MODE_OFF) {
        if (m_faceDetectMeta.frameCount > shot_ext->shot.dm.request.frameCount
            && m_camIdInfo->scenario == SCENARIO_NORMAL) {
            shot_ext->shot.dm.stats.faceDetectMode = m_faceDetectMeta.faceDetectMode;
            memcpy(shot_ext->shot.dm.stats.faceIds, m_faceDetectMeta.faceIds, sizeof(m_faceDetectMeta.faceIds));
            memcpy(shot_ext->shot.dm.stats.faceLandmarks, m_faceDetectMeta.faceLandmarks, sizeof(m_faceDetectMeta.faceLandmarks));
            memcpy(shot_ext->shot.dm.stats.faceRectangles, m_faceDetectMeta.faceRectangles, sizeof(m_faceDetectMeta.faceRectangles));
            memcpy(shot_ext->shot.dm.stats.faceScores, m_faceDetectMeta.faceScores, sizeof(m_faceDetectMeta.faceScores));
            memcpy(&(shot_ext->vra_ext), &(m_faceDetectMeta.vraExt), sizeof(struct vra_ext_meta));

            m_converter->updateFaceDetectionMetaData(request);

            CLOGV("[R%d F%d(%d/%d)]Older Meta. Change FaceDetectMeta.",
                    request->getKey(),
                    request->getFrameCount(),
                    shot_ext->shot.dm.request.frameCount,
                    m_faceDetectMeta.frameCount);
        } else {
            m_faceDetectMeta.faceDetectMode = shot_ext->shot.dm.stats.faceDetectMode;
            m_faceDetectMeta.frameCount = shot_ext->shot.dm.request.frameCount;

            memcpy(m_faceDetectMeta.faceIds, shot_ext->shot.dm.stats.faceIds, sizeof(shot_ext->shot.dm.stats.faceIds));
            memcpy(m_faceDetectMeta.faceLandmarks, shot_ext->shot.dm.stats.faceLandmarks, sizeof(shot_ext->shot.dm.stats.faceLandmarks));
            memcpy(m_faceDetectMeta.faceRectangles, shot_ext->shot.dm.stats.faceRectangles, sizeof(shot_ext->shot.dm.stats.faceRectangles));
            memcpy(m_faceDetectMeta.faceScores, shot_ext->shot.dm.stats.faceScores, sizeof(shot_ext->shot.dm.stats.faceScores));
            memcpy(&(m_faceDetectMeta.vraExt), &(shot_ext->vra_ext), sizeof(struct vra_ext_meta));

            CLOGV("[R%d F%d(%d)]Keep new FaceDetectMeta. %d,%d %dx%d",
                    request->getKey(),
                    request->getFrameCount(),
                    shot_ext->shot.dm.request.frameCount,
                    shot_ext->shot.dm.stats.faceRectangles[0][X1],
                    shot_ext->shot.dm.stats.faceRectangles[0][Y1],
                    shot_ext->shot.dm.stats.faceRectangles[0][X2],
                    shot_ext->shot.dm.stats.faceRectangles[0][Y2]);
        }
    }
}

ResultCallback::ResultCallback(ExynosCameraRequestSP_sprt_t request, ResultRequest result)
{
    m_request   = request;
    m_result    = result;
}

ResultCallback::~ResultCallback()
{

}

ExynosCameraRequestSP_sprt_t ResultCallback::getRequest()
{
    return m_request;
}

ResultRequest ResultCallback::getResult()
{
    return m_result;
}

ExynosCameraCallbackSequencer::ExynosCameraCallbackSequencer(const char *name, int cameraId)
{
    m_init();
    strncpy(m_name,  name,  EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_cameraId = cameraId;
    CLOGD("Create Callback Sequencer");
}

ExynosCameraCallbackSequencer::~ExynosCameraCallbackSequencer()
{
    if (m_runningRequestKeys.size() > 0
        || m_resultMap.size() > 0) {
        CLOGE("destructor size is not ZERO keySize(%zu) ResultSize(%zu)",
                 m_runningRequestKeys.size(), m_resultMap.size());
    }

    m_deinit();
}

status_t ExynosCameraCallbackSequencer::pushKey(uint32_t key)
{
    status_t ret = NO_ERROR;

    ret = m_pushKey(EXYNOS_LIST_OPER::SINGLE_BACK, key, m_runningRequestKeys, &m_requestCbListLock);
    if (ret < 0){
        CLOGE2("m_push failed, key(%d)", key);
    }

    return ret;
}

status_t ExynosCameraCallbackSequencer::getFrontKey(uint32_t &key)
{
    status_t ret = NO_ERROR;

    ret = m_getKey(EXYNOS_LIST_OPER::SINGLE_FRONT, m_runningRequestKeys, key, &m_requestCbListLock);
    if (ret < 0){
        CLOGE2("m_get failed(%d)", ret);
        return 0;
    }

    return ret;
}

status_t ExynosCameraCallbackSequencer::pushResult(uint32_t key, ResultRequest result)
{
    status_t ret = NO_ERROR;

    ret = m_pushResult(key, result, m_resultMap, &m_requestCbListLock);
    if (ret < 0){
        CLOGE("m_pushResult failed, key(%d)", key);
    }

    return ret;
}

status_t ExynosCameraCallbackSequencer::popResult(uint32_t key, ResultRequest_dptr_t result)
{
    status_t ret = NO_ERROR;

    ret = m_popResult(key, result, m_resultMap, &m_requestCbListLock);
    if (ret < 0){
        CLOGE("m_pushResult failed, key(%d)", key);
    }

    return ret;
}

status_t ExynosCameraCallbackSequencer::getResult(uint32_t key, ResultRequest_dptr_t result)
{
    status_t ret = NO_ERROR;

    ret = m_getResult(key, result, m_resultMap, &m_requestCbListLock);
    if (ret < 0){
        CLOGE("m_getResult failed, key(%d)", key);
    }

    return ret;
}


uint32_t ExynosCameraCallbackSequencer::getKeySize()
{
    Mutex::Autolock lock(m_requestCbListLock);
    return m_runningRequestKeys.size();
}

uint32_t ExynosCameraCallbackSequencer::getResultSize()
{
    Mutex::Autolock lock(m_requestCbListLock);
    return m_resultMap.size();
}

bool ExynosCameraCallbackSequencer::isExistResult(uint32_t key)
{
    return m_findResult(key, m_resultMap, &m_requestCbListLock);
}

status_t ExynosCameraCallbackSequencer::getKeyList(CallbackListkeys *list)
{
    status_t ret = NO_ERROR;

    list->clear();

    m_requestCbListLock.lock();
    std::copy(m_runningRequestKeys.begin(), m_runningRequestKeys.end(), std::back_inserter(*list));
    m_requestCbListLock.unlock();
    return ret;
}

status_t ExynosCameraCallbackSequencer::deleteKey(uint32_t key)
{
    status_t ret = NO_ERROR;

    ret = m_delete(key, m_runningRequestKeys, m_resultMap, &m_requestCbListLock);
    if (ret < 0){
        CLOGE("m_delete failed, key(%d)", key);
    }

    return ret;
}

void ExynosCameraCallbackSequencer::dumpList()
{
    CallbackListkeysIter iter;
    CallbackListkeys *list = &m_runningRequestKeys;

    m_requestCbListLock.lock();

    if (list->size() > 0) {
        for (iter = list->begin(); iter != list->end();) {
            CLOGE("key(%d), size(%zu)", *iter, list->size());
            iter++;
        }
    } else {
        CLOGE("m_getCallbackResults failed, size is ZERO, size(%zu)",
                 list->size());
    }

    m_requestCbListLock.unlock();

}

status_t ExynosCameraCallbackSequencer::flush()
{
    Mutex::Autolock lock(m_requestCbListLock);
    status_t ret = NO_ERROR;

    m_runningRequestKeys.clear();
    m_resultMap.clear();
    return ret;
}

status_t ExynosCameraCallbackSequencer::m_init()
{
    Mutex::Autolock lock(m_requestCbListLock);
    status_t ret = NO_ERROR;

    m_runningRequestKeys.clear();
    m_resultMap.clear();
    return ret;
}

status_t ExynosCameraCallbackSequencer::m_deinit()
{
    Mutex::Autolock lock(m_requestCbListLock);
    status_t ret = NO_ERROR;

    m_runningRequestKeys.clear();
    m_resultMap.clear();

    return ret;
}

status_t ExynosCameraCallbackSequencer::m_pushKey(EXYNOS_LIST_OPER::MODE operMode, uint32_t key, CallbackListkeys &keyList, Mutex *lock)
{
    status_t ret = NO_ERROR;

    lock->lock();

    switch (operMode) {
    case EXYNOS_LIST_OPER::SINGLE_BACK:
        keyList.push_back(key);
        break;
    case EXYNOS_LIST_OPER::SINGLE_FRONT:
        keyList.push_front(key);
        break;
    case EXYNOS_LIST_OPER::SINGLE_ORDER:
    case EXYNOS_LIST_OPER::MULTI_GET:
    default:
        ret = INVALID_OPERATION;
        CLOGE("m_push failed, mode(%d) size(%zu)", operMode, keyList.size());
        break;
    }

    lock->unlock();

    CLOGV("m_push(%d), size(%zu)", key, keyList.size());

    return ret;
}

status_t ExynosCameraCallbackSequencer::m_popKey(EXYNOS_LIST_OPER::MODE operMode, CallbackListkeys &keyList, uint32_t &key, Mutex *lock)
{
    status_t ret = NO_ERROR;
    CallbackListkeysIter iter;

    lock->lock();

    switch (operMode) {
    case EXYNOS_LIST_OPER::SINGLE_BACK:
        if (keyList.size() > 0) {
            key = keyList.back();
            keyList.pop_back();
        } else {
            CLOGE("m_pop failed, size(%zu)", keyList.size());
            ret = INVALID_OPERATION;
        }
        break;
    case EXYNOS_LIST_OPER::SINGLE_FRONT:
        if (keyList.size() > 0) {
            key = keyList.front();
            keyList.pop_front();
        } else {
            CLOGE("m_pop failed, size(%zu)", keyList.size());
            ret = INVALID_OPERATION;
        }
        break;
    case EXYNOS_LIST_OPER::SINGLE_ORDER:
    case EXYNOS_LIST_OPER::MULTI_GET:
    default:
        ret = INVALID_OPERATION;
        CLOGE("m_push failed, mode(%d) size(%zu)", operMode, keyList.size());
        break;
    }

    lock->unlock();

    CLOGV("m_pop(%d), size(%zu)", key, keyList.size());

    return ret;
}

status_t ExynosCameraCallbackSequencer::m_getKey(EXYNOS_LIST_OPER::MODE operMode, CallbackListkeys &keyList, uint32_t &key, Mutex *lock)
{
    status_t ret = NO_ERROR;
    CallbackListkeysIter iter;

    lock->lock();

    switch (operMode) {
    case EXYNOS_LIST_OPER::SINGLE_BACK:
        if (keyList.size() > 0) {
            key = keyList.back();
        } else {
            CLOGE("m_get failed, size(%zu)", keyList.size());
            ret = INVALID_OPERATION;
        }
        break;
    case EXYNOS_LIST_OPER::SINGLE_FRONT:
        if (keyList.size() > 0) {
            key = keyList.front();
        } else {
            CLOGE("m_get failed, size(%zu)", keyList.size());
            ret = INVALID_OPERATION;
        }
        break;
    case EXYNOS_LIST_OPER::SINGLE_ORDER:
    case EXYNOS_LIST_OPER::MULTI_GET:
    default:
        ret = INVALID_OPERATION;
        CLOGE("m_get failed, mode(%d) size(%zu)", operMode, keyList.size());
        break;
    }

    lock->unlock();

    CLOGV("m_get(%d), size(%zu)", key, keyList.size());

    return ret;
}

status_t ExynosCameraCallbackSequencer::m_pushResult(uint32_t key, ResultRequest item, ResultMapS &resultMap, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<ResultMapS::iterator,bool> listRet;
    ResultMapSIter iter;

    lock->lock();

    iter = resultMap.find(key);
    if (iter != resultMap.end()) {
        CLOGE("m_pushResult already exist key(%d)", key);
    } else {
        resultMap[key] = item;
    }

    lock->unlock();

    return ret;
}

status_t ExynosCameraCallbackSequencer::m_popResult(uint32_t key, ResultRequest_dptr_t item, ResultMapS &resultMap, Mutex *lock)
{
    status_t ret = NO_ERROR;
    ResultMapSIter iter;

    lock->lock();

    iter = resultMap.find(key);
    if (iter != resultMap.end()) {
        item = iter->second;
        resultMap.erase(iter);
    }

    lock->unlock();

    return ret;
}

status_t ExynosCameraCallbackSequencer::m_getResult(uint32_t key, ResultRequest_dptr_t item, ResultMapS &resultMap, Mutex *lock)
{
    status_t ret = NO_ERROR;
    ResultMapSIter iter;

    lock->lock();

    iter = resultMap.find(key);
    if (iter != resultMap.end()) {
        item = iter->second;
    }

    lock->unlock();

    return ret;
}

bool ExynosCameraCallbackSequencer::m_findResult(uint32_t key, ResultMapS &resultMap, Mutex *lock)
{
    bool ret = false;
    ResultMapSIter iter;

    lock->lock();
    iter = resultMap.find(key);
    if (iter != resultMap.end()) {
        ret = true;
    }

    lock->unlock();
    return ret;
}

status_t ExynosCameraCallbackSequencer::m_delete(uint32_t key, CallbackListkeys &keyList, ResultMapS &resultMap, Mutex *lock)
{
    status_t ret = NO_ERROR;
    CallbackListkeysIter iter;
    ResultMapSIter resultIter;

    lock->lock();

    if (keyList.size() > 0) {
        iter = find(keyList.begin(), keyList.end(), key);
        if (iter != keyList.end()) {
            keyList.erase(iter);
            CLOGV("key(%d), size(%zu)", key, keyList.size());
        }

        resultIter = resultMap.find(key);
        if (resultIter != resultMap.end()) {
            resultMap.erase(resultIter);
        }
    } else {
        ret = INVALID_OPERATION;
        CLOGE("m_getCallbackResults failed, size is ZERO, size(%zu)", keyList.size());
    }

    lock->unlock();

    CLOGV("size(%zu)", keyList.size());

    return ret;
}

}; /* namespace android */
