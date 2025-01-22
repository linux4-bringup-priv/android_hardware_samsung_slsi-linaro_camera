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

#include "ExynosCameraCommonInclude.h"

#define MAX_NUM_OF_CAMERA 13

namespace android {

enum CAMERA_STATE {
    CAMERA_NONE,
    CAMERA_OPENED,
    CAMERA_RELEASED,
    CAMERA_CLOSED,
    CAMERA_PREVIEW,
    CAMERA_PREVIEWSTOPPED,
    CAMERA_RECORDING,
    CAMERA_RECORDINGSTOPPED,
#ifdef SUPPORT_FACTORY_CHECK_ACTIVE_CAMERA
    CAMERA_NON_ACTIVE,
#endif
};

static const char *camera_state_enum2str[40] = {
    "NONE",
    "OPENED",
    "RELEASED",
    "CLOSED",
    "PREVIEW_RUNNING",
    "PREVIEW_STOPPED",
    "RECORDING_RUNNING",
    "RECORDING_STOPPED",
#ifdef SUPPORT_FACTORY_CHECK_ACTIVE_CAMERA
    "NON_ACTIVE"
#endif
};

static CAMERA_STATE     cam_state[CAMERA_ID_MAX];
static Mutex            cam_stateLock[CAMERA_ID_MAX];

static int check_camera_state(CAMERA_STATE state, int cameraId)
{
    bool ret = false;
    cam_stateLock[cameraId].lock();

    ALOGD("DEBUG(%s):camera(%d) state(%d) checking...", __FUNCTION__, cameraId, state);

    int numOfActiveCam = 0;

    if (state == CAMERA_OPENED) {
        for (int i = 0; i < CAMERA_ID_MAX; i++) {
            if ((cam_state[i] != CAMERA_NONE)
                && (cam_state[i] != CAMERA_CLOSED)) {
                numOfActiveCam++;
                //TODO: need to exclude TOF Standalone cameras.
            }
        }
#ifndef PIP_CAMERA_SUPPORTED
        if (numOfActiveCam > 0)
#else
        if (numOfActiveCam > 1) //maximum 2 cameras are allowed to open simultaneously.
#endif
        {
            ALOGE("ERR(%s):camera(%d) DUAL camera not supported\n", __FUNCTION__, cameraId);
            cam_stateLock[cameraId].unlock();
            return ret;
        }
    }

    switch (state) {
    case CAMERA_NONE:
        ret = true;
        break;
    case CAMERA_OPENED:
        if (cam_state[cameraId] == CAMERA_NONE ||
            cam_state[cameraId] == CAMERA_CLOSED)
            ret = true;
        break;
    case CAMERA_RELEASED:
        if (cam_state[cameraId] == state ||
            cam_state[cameraId] == CAMERA_OPENED ||
            cam_state[cameraId] == CAMERA_PREVIEWSTOPPED)
            ret = true;
        break;
    case CAMERA_CLOSED:
        if (cam_state[cameraId] == state ||
            cam_state[cameraId] == CAMERA_OPENED ||
            cam_state[cameraId] == CAMERA_PREVIEWSTOPPED ||
            cam_state[cameraId] == CAMERA_RELEASED)
            ret = true;
        break;
    case CAMERA_PREVIEW:
        if (cam_state[cameraId] == CAMERA_OPENED ||
            cam_state[cameraId] == CAMERA_PREVIEWSTOPPED)
            ret = true;
        break;
    case CAMERA_PREVIEWSTOPPED:
        if (cam_state[cameraId] == state ||
            cam_state[cameraId] == CAMERA_OPENED ||
            cam_state[cameraId] == CAMERA_PREVIEW ||
            cam_state[cameraId] == CAMERA_RECORDINGSTOPPED)
            ret = true;
        break;
    case CAMERA_RECORDING:
        if (cam_state[cameraId] == CAMERA_PREVIEW ||
            cam_state[cameraId] == CAMERA_RECORDINGSTOPPED)
            ret = true;
        break;
    case CAMERA_RECORDINGSTOPPED:
        if (cam_state[cameraId] == state ||
            cam_state[cameraId] == CAMERA_RECORDING)
            ret = true;
        break;
#ifdef SUPPORT_FACTORY_CHECK_ACTIVE_CAMERA
    case CAMERA_NON_ACTIVE:
        ret = true;
        break;
#endif
    default:
        ALOGE("ERR(%s):camera(%d) state(%s) is unknown value",
            __FUNCTION__, cameraId, camera_state_enum2str[state]);
        ret = false;
        break;
    }

    if (ret == true) {
        ALOGD("DEBUG(%s):camera(%d) state(%d:%s->%d:%s) is valid",
                __FUNCTION__, cameraId,
                cam_state[cameraId], camera_state_enum2str[cam_state[cameraId]],
                state, camera_state_enum2str[state]);
    } else {
        ALOGE("ERR(%s):camera(%d) state(%d:%s->%d:%s) is INVALID",
                __FUNCTION__, cameraId,
                cam_state[cameraId], camera_state_enum2str[cam_state[cameraId]],
                state, camera_state_enum2str[state]);
    }

    cam_stateLock[cameraId].unlock();
    return ret;
}

#ifdef SUPPORT_FACTORY_CHECK_ACTIVE_CAMERA
static bool get_camera_active(int cameraId)
{
    bool ret = true;
    if (cam_state[cameraId] == CAMERA_NON_ACTIVE) {
        ret = false;
    }
    return ret;
}
#endif

}; /* namespace android */
