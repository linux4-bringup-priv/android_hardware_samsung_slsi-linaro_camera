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
 * \file      ExynosCameraActivitySpecialCapture.h
 * \brief     hearder file for CAMERA HAL MODULE
 * \author    Pilsun Jang(pilsun.jang@samsung.com)
 * \date      2012/12/19
 *
 */

#ifndef EXYNOS_CAMERA_ACTIVITY_SPECIAL_CAPTURE_H__
#define EXYNOS_CAMERA_ACTIVITY_SPECIAL_CAPTURE_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utils/threads.h>

#include <linux/videodev2.h>
#include "videodev2_exynos_camera.h"
#include <linux/vt.h>

#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/List.h>
#include "cutils/properties.h"

#include "exynos_format.h"
#include "ExynosBuffer.h"
#include "ExynosRect.h"
#include "ExynosJpegEncoderForCamera.h"
#include "ExynosExif.h"
#include "exynos_v4l2.h"
#include "ExynosCameraActivityBase.h"

#include "fimc-is-metadata.h"

#define CAPTURE_SKIP_COUNT              (1)

#define HDR_WAITING_SLEEP_TIME          (15000)     /* 15 msec */
#define HDR_MAX_WAITING_TIME            (1000000)
#define HDR_TIMEOUT_COUNT               (30)        /* 30 fps * 1 sec */
#define HDR_BESTSHOT_MAX_WAITING_TIME   (100000)    /* 100 msec */

/* #define HDR_WAIT_COUNT    (3) */
#define HDR_WAIT_COUNT      (0)
#define HDR_FRAME_COUNT     (4)
#define BAYER_LOCK          (2)
#define HDR_REPROCESSING_COUNT (3)

namespace android {

class ExynosCameraActivitySpecialCapture : public ExynosCameraActivityBase {
public:
    enum SCAPTURE_DUMMY {
        SCAPTURE_DUMMY,
        SCAPTURE_DUMMY1
    };

    enum SCAPTURE_MODE {
        SCAPTURE_MODE_NONE,
        SCAPTURE_MODE_HDR,
        SCAPTURE_MODE_LLL,
        SCAPTURE_MODE_OIS,
        SCAPTURE_MODE_RAW,
        SCAPTURE_MODE_DYNAMIC_PICK,    // Dynamic meta Picking
        SCAPTURE_MODE_END
    };

    enum SCAPTURE_STEP {
        SCAPTURE_STEP_OFF,
        SCAPTURE_STEP_START,
        SCAPTURE_STEP_MINUS_SET,
        SCAPTURE_STEP_ZERO_DELAY_SET,
        SCAPTURE_STEP_ZERO_SET,
        SCAPTURE_STEP_PLUS_SET,
        SCAPTURE_STEP_RESTORE,
        SCAPTURE_STEP_WAIT_CAPTURE_DELAY,
        SCAPTURE_STEP_WAIT_CAPTURE,
        SCAPTURE_STEP_END
    };

    enum DYNAMIC_PICK_MODE {
        DYNAMIC_PICK_NONE,
        DYNAMIC_PICK_OUTFOCUS,
        DYNAMIC_PICK_END
    };

public:
    ExynosCameraActivitySpecialCapture(int cameraId,
                                       cameraId_Info *camIdInfo,
                                       struct ExynosCameraSensorInfoBase *staticInfo);
    virtual ~ExynosCameraActivitySpecialCapture();

protected:
    int t_funcNull(void *args);
    int t_funcSensorBefore(void *args);
    int t_funcSensorAfter(void *args);
    int t_func3ABefore(void *args);
    int t_func3AAfter(void *args);
    int t_func3ABeforeHAL3(void *args);
    int t_func3AAfterHAL3(void *args);
    int t_funcISPBefore(void *args);
    int t_funcISPAfter(void *args);
    int t_funcVRABefore(void *args);
    int t_funcVRAAfter(void *args);

public:
    int setCaptureMode(enum SCAPTURE_MODE sCaptureModeVal);
    ExynosCameraActivitySpecialCapture::SCAPTURE_MODE getCaptureMode(void);

    int setCaptureMode(enum SCAPTURE_MODE sCaptureModeVal, int ModeVal);
    int setCaptureStep(enum SCAPTURE_STEP sCaptureStepVal);
    int getIsHdr();
    unsigned int getHdrStartFcount(int index);
    unsigned int getHdrDropFcount(void);
    int resetHdrStartFcount();
    int getHdrWaitFcount();
    void setHdrBuffer(ExynosCameraBuffer *secondBuffer, ExynosCameraBuffer *thirdBuffer);
    unsigned int getDynamicPickCaptureFcount(void);
    void resetDynamicPickCaptureFcount();
    void setMultiCaptureMode(bool enable);
    bool getMultiCaptureMode(void);
    unsigned int getOISCaptureFcount(void);
    void resetOISCaptureFcount();
    void waitShutterCallback();
    ExynosCameraBuffer *getHdrBuffer(int index);

    void setCaptureIntent(enum aa_capture_intent captureIntent);
    enum aa_capture_intent getCaptureIntent(void);

    void setCaptureCount(int captureCount);
    int  getCaptureCount(void);

private:
    enum SCAPTURE_MODE m_specialCaptureMode;
    enum SCAPTURE_STEP m_specialCaptureStep;
    enum DYNAMIC_PICK_MODE m_dynamicPickMode;

    unsigned int m_hdrFcount;
    unsigned int m_currentInputFcount;
    unsigned int m_hdrStartFcount[3];
    unsigned int m_hdrDropFcount[3];

    int                 m_backupAeExpCompensation;
    enum aa_scene_mode  m_backupSceneMode;
    enum aa_mode        m_backupAaMode;
    enum aa_ae_lock     m_backupAeLock;
    int                 m_backupAeTargetFpsRange[2];
    long                m_backupFrameDuration;
    int                 m_delay;
    bool                m_check;

    ExynosCameraBuffer  *m_hdrBuffer[2];

    bool                m_multiCaptureMode;
    bool                m_waitAvailable;
    unsigned int        m_OISCaptureFcount;
    Mutex               m_SignalMutex;
    mutable Condition   m_SignalCondition;
    uint64_t            m_waitSignalTime;

    unsigned int        m_DynamicPickCaptureFcount;

    enum aa_capture_intent m_captureIntent;
    int                 m_captureCount;
    int                 m_currentCaptureCount;
};

}

#endif /* EXYNOS_CAMERA_ACTIVITY_SPECIAL_CAPTURE_H__ */
