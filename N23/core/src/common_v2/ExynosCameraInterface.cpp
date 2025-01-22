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
#define LOG_TAG "ExynosCameraInterface"
#include <log/log.h>

#include "ExynosCameraInterface.h"
#include "ExynosCameraAutoTimer.h"
#include "ExynosCameraTimeLogger.h"
#ifdef USE_SLSI_VENDOR_TAGS
#include "ExynosCameraVendorTags.h"
#endif

namespace android {

/* The global lock is intended to avoid racing problem between open/close */
static Mutex   gCamOpenCloseLock;

void createWaitingThreadForOfflineCapture(int, int);

static int HAL3_camera_device_open(const struct hw_module_t* module,
                                    const char *id,
                                    struct hw_device_t** device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int cameraId = atoi(id);
    enum CAMERA_STATE state;
    FILE *fp = NULL;
    int ret = 0;

    Mutex::Autolock lock(gCamOpenCloseLock);

    CameraMetadata metadata;
    camera_metadata_entry flashAvailable;
    bool hasFlash = false;
    char flashFilePath[100] = {'\0',};
    int numOfSensors = 1;
    cameraId_Info camIdInfo;

    for (int i = 0; i < MAX_NUM_SENSORS; i++) {
        camIdInfo.cameraId[i] = -1;
    }

    camIdInfo.serviceCameraId = cameraId;
    numOfSensors = getCameraIdInfo(&camIdInfo);
    /*
     * NOTE: camIdInfo.serviceCameraId is same as camIdInfo.camInfoIndex for the Public cameras.
     * It could be different for the hidden cameras.
     */

    /* Validation check */
    ALOGI("INFO(%s[%d]):camera(%d) master(%d), slave(%d) camInfoIndex(%d) in ======",
        __FUNCTION__, __LINE__, cameraId, camIdInfo.cameraId[MAIN_CAM], camIdInfo.cameraId[SUB_CAM], camIdInfo.camInfoIndex);

    if (numOfSensors < 1 || camIdInfo.cameraId[MAIN_CAM] < 0 || camIdInfo.camInfoIndex < 0) {
        ALOGE("ERR(%s[%d]):Invalid camera ID%d and numOf Sensors(%d)",
            __FUNCTION__, __LINE__, camIdInfo.cameraId[MAIN_CAM], numOfSensors);
        return -EINVAL;
    }

#ifdef TIME_LOGGER_LAUNCH_ENABLE
    TIME_LOGGER_INIT(camIdInfo.cameraId[MAIN_CAM]);
#endif
    TIME_LOGGER_UPDATE(camIdInfo.cameraId[MAIN_CAM], 0, 0, CUMULATIVE_CNT, OPEN_START, 0);

    waitForInitThread();

    /* Setting status and check current status */
    state = CAMERA_OPENED;
    for (int i = 0; i < camIdInfo.numOfSensors; i++) {
        if (check_camera_state(state, camIdInfo.cameraId[i]) == false) {
            ALOGE("ERR(%s[%d]):camera(%d) state(%d) is INVALID", __FUNCTION__, __LINE__, camIdInfo.cameraId[i], state);
            return -EUSERS;
        }
    }

    /* Create camera device */
    if (g_cam_device3[camIdInfo.camInfoIndex]) {
        ALOGE("ERR(%s[%d]):returning existing camera Idx(%d)", __FUNCTION__, __LINE__, camIdInfo.camInfoIndex);
        *device = (hw_device_t *)g_cam_device3[camIdInfo.camInfoIndex];
        goto done;
    }

#ifdef USE_DEBUG_PROPERTY
    {
        bool first = true;
        for (int i = 0; i < CAMERA_INDEX_ID_MAX; i++) {
            if (g_cam_device3[i]) first = false;
        }
        if (first) LOGMGR_INIT();
    }
#endif

    g_cam_device3[camIdInfo.camInfoIndex] = (camera3_device_t *)malloc(sizeof(camera3_device_t));
    if (!g_cam_device3[camIdInfo.camInfoIndex])
        return -ENOMEM;

    g_cam_openLock[camIdInfo.camInfoIndex].lock();
    g_cam_device3[camIdInfo.camInfoIndex]->common.tag     = HARDWARE_DEVICE_TAG;
    g_cam_device3[camIdInfo.camInfoIndex]->common.version = CAMERA_DEVICE_API_VERSION_3_5;
    g_cam_device3[camIdInfo.camInfoIndex]->common.module  = const_cast<hw_module_t *>(module);
    g_cam_device3[camIdInfo.camInfoIndex]->common.close   = HAL3_camera_device_close;
    g_cam_device3[camIdInfo.camInfoIndex]->ops            = &camera_device3_ops;

    ALOGV("DEBUG(%s[%d]):open camera(%d)", __FUNCTION__, __LINE__, camIdInfo.camInfoIndex);
    g_cam_device3[camIdInfo.camInfoIndex]->priv = new ExynosCamera(&camIdInfo, &g_cam_info[camIdInfo.camInfoIndex]);
    *device = (hw_device_t *)g_cam_device3[camIdInfo.camInfoIndex];

    ALOGI("INFO(%s[%d]):camera(%d) out from new g_cam_device3[%d]->priv()",
        __FUNCTION__, __LINE__, camIdInfo.cameraId[MAIN_CAM], camIdInfo.camInfoIndex);

    g_cam_openLock[camIdInfo.camInfoIndex].unlock();

done:

    camera3_device_t *cam_device = (camera3_device_t *)(*device);
    obj(cam_device)->createDevice(&camIdInfo, &g_cam_info[camIdInfo.camInfoIndex]);

#ifdef PIP_CAMERA_SUPPORTED
    /*
     * check if any other camera is already opened.
     * If so,
     * 1) set PIP mode for previously opened camera and the current camera device.
     * 2) set PIP_SUB_CAM_MODE mode for the current camera (second camera).
     * TODO: need to check the camera conflicting_devices & resource_cost.
     */
    bool needPIPConfig = false;
    int PIPMainCamIdx = -1;

    for (int i = 0; i < CAMERA_INDEX_ID_MAX; i++) {
        if (g_cam_device3[i]) {
            camera3_device_t *cam_device = (camera3_device_t *)g_cam_device3[i];
            int prevLocalCamId = obj(cam_device)->getCameraId();
            if ((cam_state[prevLocalCamId] != CAMERA_NONE)
                && (cam_state[prevLocalCamId] != CAMERA_CLOSED)) {
                 camera3_device_t *cam_device = (camera3_device_t *)*device;
                 int localCamId = obj(cam_device)->getCameraId();
                 if ((localCamId != CAMERA_ID_BACK_TOF)
                     && (localCamId != CAMERA_ID_FRONT_TOF)) {
                     needPIPConfig = true;
                     PIPMainCamIdx = i;
                     break;
                 }
            }
        }
    }

    if (needPIPConfig && PIPMainCamIdx >= 0 && numOfSensors == 1) {
        /* Set PIP mode into Rear Camera */
        ret = obj(g_cam_device3[PIPMainCamIdx])->setPIPMode(true);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):camera(%d) set pipe mode fail. ret %d",
                    __FUNCTION__, __LINE__, PIPMainCamIdx, ret);
        } else {
            ALOGI("INFO(%s[%d]):camera(%d) set pip mode. Restart stream.",
                    __FUNCTION__, __LINE__, PIPMainCamIdx);
        }

        ret = obj(g_cam_device3[camIdInfo.camInfoIndex])->setPIPMode(true, true);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):camera(%d) set pip mode fail, ret(%d)",
                __FUNCTION__, __LINE__, camIdInfo.camInfoIndex, ret);
        } else {
            ALOGI("INFO(%s[%d]):camera(%d) set pip mode",
                __FUNCTION__, __LINE__, camIdInfo.camInfoIndex);
        }
    }
#endif

    for (int i = 0; i < camIdInfo.numOfSensors; i++) {
        cam_stateLock[camIdInfo.cameraId[i]].lock();
        cam_state[camIdInfo.cameraId[i]] = state;
        cam_stateLock[camIdInfo.cameraId[i]].unlock();
    }

#ifdef USE_DUAL_CAMERA
    if (numOfSensors > 1) {
        int32_t sensorCnt = 0;
        int32_t sensorIdx = 0;
        while ((sensorIdx < MAX_NUM_SENSORS)
            && (sensorCnt < numOfSensors)) {
            if ((camIdInfo.cameraId[sensorIdx] > -1)
                && (camIdInfo.cameraId[sensorIdx] != CAMERA_ID_BACK_TOF)
                 && (camIdInfo.cameraId[sensorIdx] != CAMERA_ID_FRONT_TOF)) {
                 // TOF sensors are excluded from the sensor count.
                sensorCnt++;
            }
            sensorIdx++;
        }

        if (sensorCnt > 1) {
            obj(cam_device)->setDualMode(true);
        }
    }
#endif

    if (g_cam_info[camIdInfo.camInfoIndex]) {
        metadata = g_cam_info[camIdInfo.camInfoIndex];
        flashAvailable = metadata.find(ANDROID_FLASH_INFO_AVAILABLE);

        ALOGV("INFO(%s[%d]): cameraId(%d), flashAvailable.count(%zu), flashAvailable.data.u8[0](%d)",
            __FUNCTION__, __LINE__, camIdInfo.camInfoIndex, flashAvailable.count, flashAvailable.data.u8[0]);

        if (flashAvailable.count == 1 && flashAvailable.data.u8[0] == 1) {
            hasFlash = true;
        } else {
            hasFlash = false;
        }
    }

    for (int i = 0; i < camIdInfo.numOfSensors; i++) {
        /* Turn off torch and update torch status */
        if(hasFlash && g_cam_torchEnabled[camIdInfo.cameraId[i]]) {
            if (isBackCamera(camIdInfo.cameraId[i])) {
                snprintf(flashFilePath, sizeof(flashFilePath), TORCH_REAR_FILE_PATH);
            } else {
                snprintf(flashFilePath, sizeof(flashFilePath), TORCH_FRONT_FILE_PATH);
            }

            fp = fopen(flashFilePath, "w+");
            if (fp == NULL) {
                ALOGE("ERR(%s[%d]):torch file open fail", __FUNCTION__, __LINE__);
            } else {
                fwrite("0", sizeof(char), 1, fp);
                fflush(fp);
                fclose(fp);

                g_cam_torchEnabled[camIdInfo.cameraId[i]] = false;
            }
        }
    }

#ifdef USE_CAMERA_MEMBOOST
    fp = fopen(MEMBOOST_FILE_PATH, "w+");
    if (fp == NULL) {
        ALOGE("ERR(%s[%d]):MEMBOOST_FILE_PATH open fail", __FUNCTION__, __LINE__);
    } else {
        fwrite("2", sizeof(char), 1, fp);
        fflush(fp);
        fclose(fp);
        ALOGI("INFO(%s[%d]):MEMBOOST start", __FUNCTION__, __LINE__);
    }
#endif

    /* Set TORCH_MODE_STATUS_NOT_AVAILABLE for all physical cameras */
    if (camIdInfo.numOfSensors == 1) {
        g_callbacks->torch_mode_status_change(g_callbacks, id, TORCH_MODE_STATUS_NOT_AVAILABLE);
    } else {
        /* Logical camera Scenario */
        for (int i = 0; i < camIdInfo.numOfSensors; i++) {
            int phyCamId;
            phyCamId = getPhysicalIdFromInternalId(camIdInfo.cameraId[i]);
            if (phyCamId >= 0) {
                char camID[10];
                snprintf(camID, sizeof(camID), "%d\n", phyCamId);
                g_callbacks->torch_mode_status_change(g_callbacks, camID, TORCH_MODE_STATUS_NOT_AVAILABLE);
            }
        }
    }

    ALOGI("INFO(%s[%d]):camera(%d) out =====", __FUNCTION__, __LINE__, camIdInfo.camInfoIndex);

    TIME_LOGGER_UPDATE(camIdInfo.cameraId[MAIN_CAM], 0, 0, CUMULATIVE_CNT, OPEN_END, 0);

    return 0;
}

static int HAL3_camera_device_close(struct hw_device_t* device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int serviceCameraId = -1;
    int mainCameraId = -1;
#ifdef USE_DUAL_CAMERA
    int subCameraId = -1;
#endif
    int ret = OK;
    enum CAMERA_STATE state;
    char camid[10];
    cameraId_Info camIdInfo;
    int numOfSensors = 1;

    for (int i = 0; i < MAX_NUM_SENSORS; i++) {
        camIdInfo.cameraId[i] = -1;
    }

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    Mutex::Autolock lock(gCamOpenCloseLock);

    if (device) {
        camera3_device_t *cam_device = (camera3_device_t *)device;
        mainCameraId = obj(cam_device)->getCameraId();
        serviceCameraId = obj(cam_device)->getServiceCameraId();

        camIdInfo.serviceCameraId = serviceCameraId;
        numOfSensors = getCameraIdInfo(&camIdInfo);
        if (numOfSensors < 1 || camIdInfo.cameraId[MAIN_CAM] < 0 || camIdInfo.camInfoIndex < 0) {
            ALOGE("ERR(%s[%d]):Invalid camera ID%d and numOf Sensors(%d)",
                __FUNCTION__, __LINE__, camIdInfo.cameraId[MAIN_CAM], numOfSensors);
            return -EINVAL;
        }

        ALOGV("DEBUG(%s[%d]):close camera(%d)", __FUNCTION__, __LINE__, serviceCameraId);

#ifdef TIME_LOGGER_CLOSE_ENABLE
        TIME_LOGGER_INIT(mainCameraId);
#endif
        TIME_LOGGER_UPDATE(mainCameraId, 0, 0, CUMULATIVE_CNT, CLOSE_START, 0);

        ret = obj(cam_device)->releaseDevice();
        if (ret) {
            ALOGE("ERR(%s[%d]):releaseDevice error!!", __FUNCTION__, __LINE__);
            ret = BAD_VALUE;
        }

        state = CAMERA_CLOSED;
        for (int i = 0; i < camIdInfo.numOfSensors; i++) {
            if (check_camera_state(state, camIdInfo.cameraId[i]) == false) {
                ALOGE("ERR(%s[%d]):camera(%d) state(%d) is INVALID", __FUNCTION__, __LINE__, camIdInfo.cameraId[i], state);
                return -1;
            }
        }

        if(obj(cam_device)->isOfflineCaptureRunning()) {
            ret = obj(cam_device)->destroyDevice();
            if (ret) {
                ALOGE("ERR(%s[%d]):destroyDevice error!!", __FUNCTION__, __LINE__);
                ret = BAD_VALUE;
            }

            int cameraSessinoId = obj(cam_device)->getCameraSessionId();
            createWaitingThreadForOfflineCapture(camIdInfo.camInfoIndex, cameraSessinoId);

            ALOGI("INFO(%s[%d]) : [OFFLINE] offline capture is running", __FUNCTION__, __LINE__);
        } else {

            g_cam_openLock[camIdInfo.camInfoIndex].lock();
            ALOGV("INFO(%s[%d]):camera(%d) open locked..", __FUNCTION__, __LINE__, camIdInfo.serviceCameraId);
            g_cam_device3[camIdInfo.camInfoIndex] = NULL;
            g_cam_openLock[camIdInfo.camInfoIndex].unlock();
            ALOGV("INFO(%s[%d]):camera(%d) open unlocked..", __FUNCTION__, __LINE__, camIdInfo.serviceCameraId);

            delete static_cast<ExynosCamera *>(cam_device->priv);
            free(cam_device);
        }

        for (int i = 0; i < camIdInfo.numOfSensors; i++) {
            cam_stateLock[camIdInfo.cameraId[i]].lock();
            cam_state[camIdInfo.cameraId[i]] = state;
            cam_stateLock[camIdInfo.cameraId[i]].unlock();
        }

        ALOGI("INFO(%s[%d]):close camera(%d)", __FUNCTION__, __LINE__, camIdInfo.serviceCameraId);

        /* Set TORCH_MODE_STATUS_AVAILABLE_OFF for all physical cameras */
        if (camIdInfo.numOfSensors == 1) {
            snprintf(camid, sizeof(camid), "%d\n", camIdInfo.serviceCameraId);
            g_callbacks->torch_mode_status_change(g_callbacks, camid, TORCH_MODE_STATUS_AVAILABLE_OFF);
            g_cam_torchEnabled[camIdInfo.cameraId[MAIN_CAM]] = false;
        } else {
            /* Logical camera Scenario */
            for (int i = 0; i < camIdInfo.numOfSensors; i++) {
                int phyCamId;
                phyCamId = getPhysicalIdFromInternalId(camIdInfo.cameraId[i]);
                if (phyCamId >= 0) {
                    snprintf(camid, sizeof(camid), "%d\n", phyCamId);
                    g_callbacks->torch_mode_status_change(g_callbacks, camid, TORCH_MODE_STATUS_AVAILABLE_OFF);
                }
                /* Update torch status */
                g_cam_torchEnabled[camIdInfo.cameraId[i]] = false;
            }
        }

#ifdef TIME_LOGGER_CLOSE_ENABLE
        TIME_LOGGER_UPDATE(mainCameraId, 0, 0, CUMULATIVE_CNT, CLOSE_END, 0);
        TIME_LOGGER_SAVE(mainCameraId);
#endif
    }
#ifdef USE_INTERNAL_ALLOC_DEBUG
    alloc_info_print();
#endif
    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static int HAL3_camera_device_initialize(const struct camera3_device *dev,
                                        const camera3_callback_ops_t *callback_ops)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = OK;
    int mainCameraId = obj(dev)->getCameraId();
    int camInfoIndex = obj(dev)->getCameraInfoIdx();

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    TIME_LOGGER_UPDATE(mainCameraId, 0, 0, CUMULATIVE_CNT, INITIALIZE_START, 0);

    g_cam_configLock[camInfoIndex].lock();

    ALOGI("INFO(%s[%d]): cam_state[0](%d)", __FUNCTION__, __LINE__, cam_state[mainCameraId]);

    ret = obj(dev)->initializeDevice(callback_ops);
    if (ret) {
        ALOGE("ERR(%s[%d]):initialize error!!", __FUNCTION__, __LINE__);
        ret = NO_INIT;
    }
    g_cam_configLock[camInfoIndex].unlock();

    ALOGV("DEBUG(%s):set callback ops - %p", __FUNCTION__, callback_ops);
    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

    TIME_LOGGER_UPDATE(mainCameraId, 0, 0, CUMULATIVE_CNT, INITIALIZE_END, 0);

    return ret;
}

static int HAL3_camera_device_configure_streams(const struct camera3_device *dev,
                                                camera3_stream_configuration_t *stream_list)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = OK;
    int mainCameraId = obj(dev)->getCameraId();
    int camInfoIndex = obj(dev)->getCameraInfoIdx();
    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    TIME_LOGGER_UPDATE(mainCameraId, 0, 0, CUMULATIVE_CNT, CONFIGURE_STREAM_START, 0);

    Mutex::Autolock lock(gCamOpenCloseLock);

    g_cam_configLock[camInfoIndex].lock();
    ret = obj(dev)->configureStreams(stream_list);
    if (ret) {
        ALOGE("ERR(%s[%d]):configure_streams error!!", __FUNCTION__, __LINE__);
        /* if NO_MEMORY is returned, cameraserver makes assert intentionally to recover iris buffer leak */
        if (ret != NO_INIT) { /* NO_INIT = -ENODEV */
            ret = BAD_VALUE;
        }
    }
    g_cam_configLock[camInfoIndex].unlock();
    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

    TIME_LOGGER_UPDATE(mainCameraId, 0, 0, CUMULATIVE_CNT, CONFIGURE_STREAM_END, 0);

    return ret;
}

static const camera_metadata_t* HAL3_camera_device_construct_default_request_settings(
                                                                const struct camera3_device *dev,
                                                                int type)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    camera_metadata_t *request = NULL;
    status_t res;
    int camInfoIndex = obj(dev)->getCameraInfoIdx();

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    g_cam_configLock[camInfoIndex].lock();
    res = obj(dev)->construct_default_request_settings(&request, type);
    if (res) {
        ALOGE("ERR(%s[%d]):constructDefaultRequestSettings error!!", __FUNCTION__, __LINE__);
        g_cam_configLock[camInfoIndex].unlock();
        return NULL;
    }
    g_cam_configLock[camInfoIndex].unlock();
    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return request;
}

static int HAL3_camera_device_process_capture_request(const struct camera3_device *dev,
                                                        camera3_capture_request_t *request)
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */

    int ret = OK;
    __unused int mainCameraId = obj(dev)->getCameraId();

    ALOGV("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    TIME_LOGGER_UPDATE(mainCameraId, 0, 0, CUMULATIVE_CNT, PROCESS_CAPTURE_REQUEST_START, 0);
#ifdef TIME_LOGGER_LAUNCH_ENABLE
    if (request->frame_number == 0) {
        TIME_LOGGER_SAVE(mainCameraId);
    }
#endif

    ret = obj(dev)->processCaptureRequest(request);
    if (ret) {
        ALOGE("ERR(%s[%d]):process_capture_request error(%d)!!", __FUNCTION__, __LINE__, ret);
        if (ret != NO_INIT) /* NO_INIT = -ENODEV */
            ret = BAD_VALUE;
    }
    ALOGV("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static int HAL3_camera_device_flush(const struct camera3_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = 0;
    int mainCameraId = obj(dev)->getCameraId();
    int camInfoIndex = obj(dev)->getCameraInfoIdx();

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    g_cam_configLock[camInfoIndex].lock();
    ret = obj(dev)->flush();
    if (ret) {
        ALOGE("ERR(%s[%d]):flush error(%d)!!", __FUNCTION__, __LINE__, ret);
        if (ret != NO_INIT) /* NO_INIT = -ENODEV */
            ret = BAD_VALUE;
    }
    g_cam_configLock[camInfoIndex].unlock();

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static void HAL3_camera_device_get_metadata_vendor_tag_ops(const struct camera3_device *dev,
                                                            vendor_tag_query_ops_t* ops)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    if (dev == NULL)
        ALOGE("ERR(%s[%d]):dev is NULL", __FUNCTION__, __LINE__);

    if (ops == NULL)
        ALOGE("ERR(%s[%d]):ops is NULL", __FUNCTION__, __LINE__);

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
}

static void HAL3_camera_device_dump(const struct camera3_device *dev, int fd)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    if (dev == NULL) {
        ALOGE("ERR(%s[%d]):dev is NULL", __FUNCTION__, __LINE__);
        return;
    }

    if (fd < 0)
        ALOGE("ERR(%s[%d]):fd is Negative Value", __FUNCTION__, __LINE__);

    int mainCameraId = obj(dev)->getCameraId();
    int camInfoIndex = obj(dev)->getCameraInfoIdx();

    g_cam_configLock[camInfoIndex].lock();
    obj(dev)->dump(fd);
    g_cam_configLock[camInfoIndex].unlock();

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
}

/***************************************************************************
 * FUNCTION   : get_camera_info
 *
 * DESCRIPTION: static function to query the numner of cameras
 *
 * PARAMETERS : none
 *
 * RETURN     : the number of cameras pre-defined
 ***************************************************************************/
static int HAL_getNumberOfCameras()
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */
    int i = 0;
    int getNumOfCamera = 0;
    int numOfSensor = -1;
    int size = ExynosCameraMetadataConverter::getExynosCameraDeviceInfoSize();
    cameraId_Info camIdInfo;

#ifdef SUPPORT_FACTORY_CHECK_ACTIVE_CAMERA
    waitForInitThread();
#endif

    for (i = 0; i < size; i++) {
        if (g_HAL_CameraInfo[i] == NULL)
            g_HAL_CameraInfo[i] = ExynosCameraMetadataConverter::getExynosCameraDeviceInfoByCamIndex(i);

        camIdInfo.serviceCameraId = g_HAL_CameraInfo[i]->cameraId;
        if (camIdInfo.serviceCameraId < CAMERA_SERVICE_ID_OPEN_CAMERA_MAX) {
            numOfSensor = getCameraIdInfo(&camIdInfo);
            if (numOfSensor > 0 && camIdInfo.cameraId[MAIN_CAM] != -1) {
#ifdef SUPPORT_FACTORY_CHECK_ACTIVE_CAMERA
                if (get_camera_active(camIdInfo.cameraId[MAIN_CAM]) == true) {
                    getNumOfCamera++;
                }
#else
                getNumOfCamera++;
#endif
            }
        }
    }
    ALOGD("DEBUG(%s[%d]):Number of cameras(%d)", __FUNCTION__, __LINE__, getNumOfCamera);
    gNumOfCameras = getNumOfCamera;
    return getNumOfCamera;
}

static int HAL_getCameraInfo(int serviceCameraId, struct camera_info *info)
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */
    status_t ret = NO_ERROR;
    int numOfSensors = 1;
    cameraId_Info camIdInfo;

    camIdInfo.serviceCameraId = serviceCameraId;
    numOfSensors = getCameraIdInfo(&camIdInfo);

    ALOGI("INFO(%s[%d]):in(%d) =====", __FUNCTION__, __LINE__, serviceCameraId);

    /* set facing and orientation */
    /*  set service arbitration (resource_cost, conflicting_devices, conflicting_devices_length) */
    if (numOfSensors < 1 || camIdInfo.cameraId[MAIN_CAM] < 0 || camIdInfo.camInfoIndex < 0) {
            ALOGE("ERR(%s[%d]):Invalid camera ID %d/%d camInfoIndex(%d)",
                __FUNCTION__, __LINE__, serviceCameraId, camIdInfo.cameraId[MAIN_CAM], camIdInfo.camInfoIndex);
            return NO_INIT; /* NO_INIT = -ENODEV */
    }

    /* set device API version */
    info->device_version = CAMERA_DEVICE_API_VERSION_3_5;

    /* set camera_metadata_t if needed */
    if (info->device_version >= HARDWARE_DEVICE_API_VERSION(2, 0)) {
        if (g_cam_info[camIdInfo.camInfoIndex] == NULL) {
            ALOGV("DEBUG(%s[%d]):Return static information (%d)", __FUNCTION__, __LINE__, camIdInfo.cameraId[MAIN_CAM]);
            ret = ExynosCameraMetadataConverter::constructStaticInfo(&camIdInfo,
                                                                     &g_cam_info[camIdInfo.camInfoIndex], &g_HAL_CameraInfo[camIdInfo.camInfoIndex]);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]): static information is NULL", __FUNCTION__, __LINE__);
                return BAD_VALUE;
            }
            info->static_camera_characteristics = g_cam_info[camIdInfo.camInfoIndex];

            if (g_HAL_CameraInfo[camIdInfo.camInfoIndex] != NULL) {
                info->facing = g_HAL_CameraInfo[camIdInfo.camInfoIndex]->facing_info;
                info->orientation = g_HAL_CameraInfo[camIdInfo.camInfoIndex]->orientation;
                info->resource_cost = g_HAL_CameraInfo[camIdInfo.camInfoIndex]->resource_cost;
                info->conflicting_devices = g_HAL_CameraInfo[camIdInfo.camInfoIndex]->conflicting_devices;
                info->conflicting_devices_length = g_HAL_CameraInfo[camIdInfo.camInfoIndex]->conflicting_devices_length;
            } else {
                ALOGE("ERR(%s[%d]): HAL_CameraInfo[%d] is NULL!", __FUNCTION__, __LINE__, camIdInfo.camInfoIndex);
                return BAD_VALUE;
            }
        } else {
            ALOGV("DEBUG(%s[%d]):Reuse Return static information ( serviceCameraId (%d) mainCameraId (%d) camInfoIndex(%d) )",
                    __FUNCTION__, __LINE__, serviceCameraId, camIdInfo.cameraId[MAIN_CAM], camIdInfo.camInfoIndex);
            info->static_camera_characteristics = g_cam_info[camIdInfo.camInfoIndex];
            info->facing = g_HAL_CameraInfo[camIdInfo.camInfoIndex]->facing_info;
            info->orientation = g_HAL_CameraInfo[camIdInfo.camInfoIndex]->orientation;
            info->resource_cost = g_HAL_CameraInfo[camIdInfo.camInfoIndex]->resource_cost;
            info->conflicting_devices = g_HAL_CameraInfo[camIdInfo.camInfoIndex]->conflicting_devices;
            info->conflicting_devices_length = g_HAL_CameraInfo[camIdInfo.camInfoIndex]->conflicting_devices_length;
        }
    }

    ALOGD("DEBUG(%s info->resource_cost = %d ", __FUNCTION__, info->resource_cost);

    if (info->conflicting_devices_length) {
        for (size_t i = 0; i < info->conflicting_devices_length; i++) {
            ALOGD("DEBUG(%s info->conflicting_devices = %s ", __FUNCTION__, info->conflicting_devices[i]);
        }
    } else {
        ALOGV("DEBUG(%s info->conflicting_devices_length is zero ", __FUNCTION__);
    }
    return NO_ERROR;
}

static int HAL_set_callbacks(const camera_module_callbacks_t *callbacks)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    if (callbacks == NULL)
        ALOGE("ERR(%s[%d]):dev is NULL", __FUNCTION__, __LINE__);

    g_callbacks = callbacks;

    return OK;
}

int control_torch_flash(enum CAMERA_TORCH_FLASH flash_type, bool enabled)
{
    FILE *fp = NULL;
    int ret = 0;
    char flashFilePath[100] = {'\0',};
    char flashVal[5] = {'\0',};
    int flashValSize = 0;

    switch (flash_type) {
        case CAMERA_TORCH_FLASH_REAR:
#if MOT_9609_SENSORS_KANE
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_REAR_FILE_PATH_1);
#else
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_REAR_FILE_PATH);
#endif
            break;
#ifdef TORCH_REAR2_FILE_PATH
        case CAMERA_TORCH_FLASH_REAR2:
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_REAR2_FILE_PATH);
            break;
#endif
#ifdef TORCH_REAR3_FILE_PATH
        case CAMERA_TORCH_FLASH_REAR3:
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_REAR3_FILE_PATH);
            break;
#endif
        case CAMERA_TORCH_FLASH_FRONT:
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_FRONT_FILE_PATH);
            break;
        default:
            ALOGE("WRN(%s[%d]):Invalid flash_type(%d)", __FUNCTION__, __LINE__, flash_type);
            goto func_err;
            break;
    }

    ALOGI("INFO(%s[%d]):flashFilePath(%s)", __FUNCTION__, __LINE__, flashFilePath);

    fp = fopen(flashFilePath, "w+");
    if (fp == NULL) {
        ALOGE("ERR(%s[%d]):torch file open(%s) fail",
            __FUNCTION__, __LINE__, flashFilePath);
        return -ENOSYS;
    }
#if MOT_9609_SENSORS_KANE
    if (enabled) {
        fwrite("1 1", sizeof(char), 3, fp);
        fseek(fp, 0, SEEK_SET);
        fwrite("2 1", sizeof(char), 3, fp);
    } else {
        fwrite("1 0", sizeof(char), 3, fp);
        fseek(fp, 0, SEEK_SET);
        fwrite("2 0", sizeof(char), 3, fp);
    }
    fflush(fp);

    ret = fclose(fp);
    if (ret != 0) {
        ALOGE("ERR(%s[%d]): file close failed(%d)", __FUNCTION__, __LINE__, ret);
    }

    fp = fopen(TORCH_REAR_CURRENT_PATH_1, "w+");
    if (fp == NULL) {
        ALOGE("ERR(%s[%d]):torch current file open(%s) fail",
            __FUNCTION__, __LINE__, TORCH_REAR_CURRENT_PATH_1);
        return -ENOSYS;
    }

    if (enabled) {
#ifdef MOT_9609_SENSORS_TROIKA
        fwrite("1 150", sizeof(char), 5, fp);
#else
        fwrite("1 130", sizeof(char), 5, fp);
#endif
        fseek(fp, 0, SEEK_SET);
        fwrite("2 50", sizeof(char), 4, fp);
    } else {
        fwrite("1 0", sizeof(char), 3, fp);
        fseek(fp, 0, SEEK_SET);
        fwrite("2 0", sizeof(char), 3, fp);
    }
#else
    if (enabled) {
#ifdef FLASH_ON_VAL
        snprintf(flashVal, sizeof(flashVal), FLASH_ON_VAL);
        flashValSize = sizeof(FLASH_ON_VAL);
#else
        snprintf(flashVal, sizeof(flashVal), "1");
        flashValSize = 1;
#endif
    } else {
#ifdef FLASH_OFF_VAL
        snprintf(flashVal, sizeof(flashVal), FLASH_OFF_VAL);
        flashValSize = sizeof(FLASH_OFF_VAL);
#else
        snprintf(flashVal, sizeof(flashVal), "0");
        flashValSize = 1;
#endif
    }
    fwrite(flashVal, sizeof(char), flashValSize, fp);
#endif
    fflush(fp);

    ret = fclose(fp);
    if (ret != 0) {
        ALOGE("ERR(%s[%d]): file close failed(%d)", __FUNCTION__, __LINE__, ret);
    }

func_err:

    return ret;
}

static int HAL_set_torch_mode(const char* serviceCameraId, bool enabled)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int cameraId = atoi(serviceCameraId);
    CameraMetadata metadata;
    camera_metadata_entry flashAvailable;
    int numOfSensors = 1;
    cameraId_Info camIdInfo;

    camIdInfo.serviceCameraId = cameraId;
    numOfSensors = getCameraIdInfo(&camIdInfo);

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    if (numOfSensors < 1 || camIdInfo.cameraId[MAIN_CAM] < 0 || camIdInfo.camInfoIndex < 0) {
        ALOGE("ERR(%s[%d]):Invalid camera ID%d and numOf Sensors(%d)",
            __FUNCTION__, __LINE__, camIdInfo.cameraId[MAIN_CAM], numOfSensors);
        return -EINVAL;
    }

    /* Check the android.flash.info.available */
    /* If this camera device does not support flash, It have to return -ENOSYS */
    metadata = g_cam_info[camIdInfo.camInfoIndex];
    flashAvailable = metadata.find(ANDROID_FLASH_INFO_AVAILABLE);

    if (flashAvailable.count == 1 && flashAvailable.data.u8[0] == 1) {
        ALOGV("DEBUG(%s[%d]): Flash metadata exist", __FUNCTION__, __LINE__);
    } else {
        ALOGE("ERR(%s[%d]): Can not find flash metadata", __FUNCTION__, __LINE__);
        return -ENOSYS;
    }

    ALOGI("INFO(%s[%d]): Current Camera State (state = %d)", __FUNCTION__, __LINE__, cam_state[camIdInfo.cameraId[MAIN_CAM]]);

    /* Add the check the camera state that camera in use or not */
    if (cam_state[camIdInfo.cameraId[MAIN_CAM]] > CAMERA_CLOSED) {
        ALOGE("ERR(%s[%d]): Camera Device is busy (state = %d)", __FUNCTION__, __LINE__, cam_state[camIdInfo.cameraId[MAIN_CAM]]);
        g_callbacks->torch_mode_status_change(g_callbacks, serviceCameraId, TORCH_MODE_STATUS_AVAILABLE_OFF);
        return -EBUSY;
    }

    if (isBackCamera(camIdInfo.cameraId[MAIN_CAM])) {
        control_torch_flash(CAMERA_TORCH_FLASH_REAR, enabled);

#if defined(BOARD_SUPPORT_MULTI_FLASH_2)
        control_torch_flash(CAMERA_TORCH_FLASH_REAR2, enabled);
#endif

#if defined(BOARD_SUPPORT_MULTI_FLASH_3)
        control_torch_flash(CAMERA_TORCH_FLASH_REAR3, enabled);
#endif
    } else if (isFrontCamera(camIdInfo.cameraId[MAIN_CAM])) {
        control_torch_flash(CAMERA_TORCH_FLASH_FRONT, enabled);
    }

    if (enabled) {
        g_cam_torchEnabled[camIdInfo.cameraId[MAIN_CAM]] = true;
        g_callbacks->torch_mode_status_change(g_callbacks,
            serviceCameraId, TORCH_MODE_STATUS_AVAILABLE_ON);
    } else {
        g_cam_torchEnabled[camIdInfo.cameraId[MAIN_CAM]] = false;
        g_callbacks->torch_mode_status_change(g_callbacks,
            serviceCameraId, TORCH_MODE_STATUS_AVAILABLE_OFF);
    }

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

void waitForInitThread()
{
    g_threadLock.lock();
    int ret = 0;

    /* Check init thread state */
    if (g_thread) {
        ret = pthread_join(g_thread, NULL);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):pthread_join failed with error code %d",  __FUNCTION__, __LINE__, ret);
        }
        g_thread = 0;
    }

    g_threadLock.unlock();
}

#ifdef SUPPORT_FACTORY_CHECK_ACTIVE_CAMERA
int update_active_sensor(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    int activeCameraCount = 0;

    for (int i = 0; i < ExynosCameraMetadataConverter::getExynosCameraDeviceInfoSize(); i++) {
        if (g_HAL_CameraInfo[i] == NULL)
            g_HAL_CameraInfo[i] = ExynosCameraMetadataConverter::getExynosCameraDeviceInfoByCamIndex(i);

        cameraId_Info camIdInfo;
        camIdInfo.serviceCameraId = g_HAL_CameraInfo[i]->cameraId;
        if (camIdInfo.serviceCameraId < CAMERA_SERVICE_ID_OPEN_CAMERA_MAX) {
            enum CAMERA_STATE state = CAMERA_NONE;
            int numOfSensor = getCameraIdInfo(&camIdInfo);
            int mainCamId = camIdInfo.cameraId[MAIN_CAM];

            if (numOfSensor > 0 && mainCamId != -1) {
                status_t ret = NO_ERROR;
                int value = 0;
                int sensorNodeNum = getFliteNodenum(mainCamId);
                int setInput = 0;

                state = CAMERA_NON_ACTIVE;
                cam_stateLock[mainCamId].lock();
                cam_state[mainCamId] = state;
                cam_stateLock[mainCamId].unlock();

                ExynosCameraNode *node = new ExynosCameraNode();

                ret = node->create("SENSOR", mainCamId);
                if (ret < 0) {
                    ALOGE("[CAM_ID(%d)]-ERR(%s[%d]): create(FLITE) fail, ret(%d)",
                        mainCamId, __FUNCTION__, __LINE__, ret);
                    SAFE_DELETE(node);
                    continue;
                }

                ret = node->open(sensorNodeNum);
                if (ret < 0) {
                    ALOGE("[CAM_ID(%d)]-ERR(%s[%d]): open(%d) fail, ret(%d)",
                        mainCamId, __FUNCTION__, __LINE__, sensorNodeNum, ret);
                    SAFE_DELETE(node);
                    continue;
                }

                unsigned int reprocessingBit = 0;
                unsigned int leaderBit = 1;
                unsigned int sensorPosition = mainCamId;
                unsigned int sensorScenario = SENSOR_SCENARIO_ACTIVE_SENSOR_FACTORY;
                unsigned int connectionMode = 0;

                if (sensorScenario < 0 || SENSOR_SCENARIO_MAX <= sensorScenario) {
                    ALOGE("[CAM_ID(%d)]-ERR(%s[%d]): invalid sensorScenario(%d) scenarioMax(%d)",
                            mainCamId, __FUNCTION__, __LINE__, sensorScenario, SENSOR_SCENARIO_MAX);
                    goto func_close;
                }

                setInput = ((sensorScenario     << INPUT_SENSOR_SHIFT)   & INPUT_SENSOR_MASK)   |
                           ((reprocessingBit    << INPUT_STREAM_SHIFT)   & INPUT_STREAM_MASK)   |
                           ((sensorPosition     << INPUT_POSITION_SHIFT) & INPUT_POSITION_MASK) |
                           ((sensorNodeNum      << INPUT_VINDEX_SHIFT)   & INPUT_VINDEX_MASK)   |
                           ((connectionMode     << INPUT_MEMORY_SHIFT)   & INPUT_MEMORY_MASK)   |
                           ((leaderBit          << INPUT_LEADER_SHIFT)   & INPUT_LEADER_MASK);

                ret = node->setInput(setInput);
                if (ret < 0) {
                    ALOGE("[CAM_ID(%d)]-ERR(%s[%d]): Node(%d) setInput(sensorIds : %d fail, ret(%d)",
                            mainCamId, __FUNCTION__, __LINE__, sensorNodeNum, setInput, ret);
                }

                ret = node->getControl(V4L2_CID_IS_G_ACTIVE_CAMERA, &value);
                if (value > 0) {
                    state = CAMERA_NONE;
                    if (check_camera_state(state, mainCamId) == false) {
                        ALOGE("ERR(%s[%d]):camera(%d) state(%d) is INVALID", __FUNCTION__, __LINE__, mainCamId, state);
                        goto func_close;
                    }
                    cam_stateLock[mainCamId].lock();
                    cam_state[mainCamId] = state;
                    cam_stateLock[mainCamId].unlock();

                    activeCameraCount++;
                }

func_close:
                if (node->close() != NO_ERROR) {
                    ALOGE("[CAM_ID(%d)]-DEBUG(%s[%d]): close fail Node(%d)",
                            mainCamId, __FUNCTION__, __LINE__, sensorNodeNum);
                }

                SAFE_DELETE(node);
            }
        }
    }
    return activeCameraCount;
}
#endif

void *init_func(__unused void *data)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    FILE *fp = NULL;
    char name[64];

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    fp = fopen(INIT_MODULE_PATH, "r");
    if (fp == NULL) {
        ALOGI("INFO(%s[%d]):module init file open fail", __FUNCTION__, __LINE__);
        goto check_sensor;
    }

    if (fgets(name, sizeof(name), fp) == NULL) {
        ALOGI("INFO(%s[%d]):failed to read init sysfs", __FUNCTION__, __LINE__);
    }

    fclose(fp);

check_sensor:

#ifdef SUPPORT_FACTORY_CHECK_ACTIVE_CAMERA
    update_active_sensor();
#endif

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

    return NULL;
}

static int HAL_init()
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = 0;

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    ret = pthread_create(&g_thread, NULL, init_func, NULL);
    if (ret) {
        ALOGE("ERR(%s[%d]):pthread_create failed with error code %d", __FUNCTION__, __LINE__, ret);
    }

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

    return OK;
}

static void HAL_get_vendor_tag_ops(__unused vendor_tag_ops_t* ops)
{
    ALOGV("INFO(%s):", __FUNCTION__);

#ifdef USE_SLSI_VENDOR_TAGS
    ExynosCameraVendorTags::getVendorTagOps(ops);
#else
    ALOGW("WARN(%s[%d]):empty operation", __FUNCTION__, __LINE__);
#endif

}

pthread_t g_offlineThread;
typedef struct offlineCameraInfo {
    int cameraId;
    int cameraSessionId;
} offlineCameraInfo_t;
static offlineCameraInfo_t g_offlineCameraInfo = {-1, 0};

void *waitForOfflineCaptureDone(__unused void *data)
{
    int retryCnt = 100;
    offlineCameraInfo_t mainCameraInfo = *((offlineCameraInfo_t*)data);

    int mainCameraId = mainCameraInfo.cameraId;
    int mainCameraSessionId = mainCameraInfo.cameraSessionId;
    bool threadExit = false;

    ALOGI("INFO(%s[%d]) : [OFFLINE] mainCameraId(%d)", __FUNCTION__, __LINE__, mainCameraId);

    camera3_device_t *cam_device = (camera3_device_t *)g_cam_device3[mainCameraId];

    while ( (obj(cam_device)->isOfflineCaptureRunning(mainCameraSessionId) == true) && (retryCnt > 0) ) {
        retryCnt--;
        ALOGI("INFO(%s[%d]) : [OFFLINE] offline capture is running retryCnt(%d)", __FUNCTION__, __LINE__, retryCnt);

        cam_stateLock[mainCameraId].lock();
        if (cam_state[mainCameraId] == CAMERA_OPENED) {
            threadExit = true;
            cam_stateLock[mainCameraId].unlock();
            break;
        }
        cam_stateLock[mainCameraId].unlock();

        usleep(100000);
    }

    if (threadExit == true) {
        ALOGI("INFO(%s[%d]) : [OFFLINE] offline capture is running, But camera is re-opend!", __FUNCTION__, __LINE__);
        return NULL;
    }

    ALOGI("INFO(%s[%d]) : [OFFLINE] offline capture is done!", __FUNCTION__, __LINE__);

    delete static_cast<ExynosCamera *>(cam_device->priv);
    free(cam_device);

    g_cam_openLock[mainCameraId].lock();
    ALOGV("INFO(%s[%d]):camera(%d) open locked..", __FUNCTION__, __LINE__, mainCameraId);
    g_cam_device3[mainCameraId] = NULL;
    g_cam_openLock[mainCameraId].unlock();
    ALOGV("INFO(%s[%d]):camera(%d) open unlocked..", __FUNCTION__, __LINE__, mainCameraId);

    return NULL;
}

void createWaitingThreadForOfflineCapture(int cameraId, int cameraSessionId)
{
    int ret = 0;

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    g_offlineCameraInfo.cameraId = cameraId;
    g_offlineCameraInfo.cameraSessionId = cameraSessionId;

    ret = pthread_create(&g_offlineThread, NULL, waitForOfflineCaptureDone, (void*)&g_offlineCameraInfo);
    if (ret) {
        ALOGE("ERR(%s[%d]):pthread_create failed with error code %d", __FUNCTION__, __LINE__, ret);
    }

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

}

}; /* namespace android */
