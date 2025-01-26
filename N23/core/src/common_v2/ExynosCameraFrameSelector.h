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

#ifndef EXYNOS_CAMERA_BAYER_SELECTOR_H
#define EXYNOS_CAMERA_BAYER_SELECTOR_H

#include <array>
#include <list>
#include "ExynosCameraParameters.h"
#include "ExynosCameraBuffer.h"
#include "ExynosCameraBufferManager.h"
#include "ExynosCameraBufferSupplier.h"
#include "ExynosCameraList.h"
#include "ExynosCameraActivityControl.h"
#include "ExynosCameraFrame.h"
#include "ExynosCameraFrameManager.h"
#ifdef SUPPORT_DEPTH_MAP
#include "ExynosCameraPipe.h"
#endif

typedef ExynosCameraList<ExynosCameraFrameSP_sptr_t> frame_queue_t;

typedef list<ExynosCameraFrameSP_sptr_t>                         FrameList;
typedef list<ExynosCameraFrameSP_sptr_t>::iterator               FrameListIterator;

namespace android{

using namespace std;

class ExynosCameraFrameSelector {
public:
    /* Frame Select result */
    typedef enum result {
        RESULT_BASE,
        /* HAS_FRAME : The frameSelector select a frame without any problems */
        RESULT_HAS_FRAME,
        /* NO_FRAME : There's any frames in frameSelector */
        RESULT_NO_FRAME,
        /*
         * SKIP_FRAME : There's any frames in frameSelector also.
         * But capture sequence should be done without frame.
         * (eg. in switch mode, there's a slave frame and no
         *  master frame. but capture sequence should done in
         *  master camera)
         */
        RESULT_SKIP_FRAME,
        RESULT_MAX,
    } result_t;

    /* Frame Select state */
    typedef enum state {
        STATE_BASE,
        /* in this state, frameSelector can return HAS_FRAME or NO_FRAME */
        STATE_NORMAL,
        /* in this state, frameSelector can return SKIP_FRAME */
        STATE_STANDBY,
        /* in this state, frameSelector skip push Frame if m_frameHoldCount > m_frameHoldList.getSizeOfProcessQ() */
        STATE_OLD_FRAME_LOCK,
        /* in this state, frameSelector use to lockList for old frame */
        STATE_MAX,
    } state_t;

    /* Selector ID */
    typedef enum SELECTOR_ID {
        SELECTOR_ID_BASE = 0,
        SELECTOR_ID_CAPTURE,
        SELECTOR_ID_MAX,
    } SELECTOR_ID_t;

    ExynosCameraFrameSelector (int cameraId,
                            ExynosCameraConfigurations *configurations,
                            ExynosCameraParameters *param,
                            ExynosCameraBufferSupplier *bufferSupplier,
                            ExynosCameraFrameManager *manager = NULL
                            );
    virtual ~ExynosCameraFrameSelector();
    virtual status_t release(void);
    status_t manageFrameHoldListHAL3(ExynosCameraFrameSP_sptr_t frame);
    status_t manageFrameHoldListForDynamicBayer(ExynosCameraFrameSP_sptr_t frame);
    status_t m_manageLockFrameHoldList(ExynosCameraFrameSP_sptr_t newFrame, bool &isLockFrame, bool &isHoldFrame);

    ExynosCameraFrameSP_sptr_t selectDynamicFrames(int count, int tryCount);
    ExynosCameraFrameSP_sptr_t selectCaptureFrames(int count, uint32_t frameCount, int tryCount);
    virtual status_t clearList(void);
    uint32_t getSizeOfHoldFrame(void);
    int getFrameHoldCount(void) { return m_frameHoldCount; };
    virtual status_t setFrameHoldCount(int32_t count);
    status_t cancelPicture(bool flagCancel);
    status_t wakeupQ(void);
    void setWaitTime(uint64_t waitTime);
#ifdef OIS_CAPTURE
    void setWaitTimeOISCapture(uint64_t waitTime);
#endif

    void setId(SELECTOR_ID_t selectorId);
    SELECTOR_ID_t getId(void);

    status_t    initLockFrameHoldCount(int maxOldBayerKeepCount);
    status_t    pushToLockFrameHoldList(ExynosCameraFrameSP_sptr_t frame);
    status_t    setLockFrameHoldCount(int32_t count);
    int32_t     getLockFrameHoldCount(void);
    status_t    lockFrameList(bool flagMigration = false, int num_of_frames = 0);
    status_t    waitAndGetFrames(FrameList &list, int num_of_frames);
    status_t    popLockFrames(FrameList &list);
    status_t    pushLockFrames(FrameList &list);
    status_t    releaseLockFrames(int remainFrameNum = 0);
    status_t    frameComplete(ExynosCameraFrameSP_sptr_t frame);
    status_t    unlockFrameList(void);
    status_t    setBayerFrameLock(bool lock);
    bool        getBayerFrameLock(void);
    status_t    releasePairFrameBuffer(ExynosCameraFrameSP_sptr_t frame);

    void        setBufferSupplier(ExynosCameraBufferSupplier *bufferSupplier);

protected:
    status_t m_manageNormalFrameHoldListHAL3(ExynosCameraFrameSP_sptr_t frame, int pipeID, bool isSrc, int32_t dstPos);
    status_t m_manageHdrFrameHoldList(ExynosCameraFrameSP_sptr_t frame, int pipeID, bool isSrc, int32_t dstPos);

    status_t m_list_release(frame_queue_t *list);
    int m_flagResetFrameHoldList;

    ExynosCameraFrameSP_sptr_t m_selectNormalFrame(int tryCount);
    ExynosCameraFrameSP_sptr_t m_selectFlashFrameV2(int tryCount);
    ExynosCameraFrameSP_sptr_t m_selectCaptureFrame(uint32_t frameCount, int tryCount);
    ExynosCameraFrameSP_sptr_t m_selectHdrFrame(int tryCount);
    ExynosCameraFrameSP_sptr_t m_selectRemosaicFrame(int tryCount);
    ExynosCameraFrameSP_sptr_t m_selectLockFrame(int tryCount);
    ExynosCameraFrameSP_sptr_t m_selectLockAndNormalFrame(int tryCount);

#ifdef OIS_CAPTURE
    status_t m_manageOISFrameHoldList(ExynosCameraFrameSP_sptr_t frame, int pipeID, bool isSrc, int32_t dstPos);
    status_t m_manageOISFrameHoldListHAL3(ExynosCameraFrameSP_sptr_t frame, int pipeID, bool isSrc, int32_t dstPos);
    ExynosCameraFrameSP_sptr_t m_selectOISNormalFrameHAL3(int tryCount);
#endif

    status_t m_getBufferFromFrame(ExynosCameraFrameSP_sptr_t frame, int pipeID, bool isSrc, ExynosCameraBuffer *outBuffer, int32_t dstPos);
    status_t m_pushQ(frame_queue_t *list, ExynosCameraFrameSP_sptr_t inframe, bool lockflag);
    status_t m_popQ(frame_queue_t *list, ExynosCameraFrameSP_dptr_t outframe, bool unlockflag, int tryCount);
    status_t m_waitAndpopQ(frame_queue_t *list, ExynosCameraFrameSP_dptr_t outframe, bool unlockflag, int tryCount);
    status_t m_frameComplete(ExynosCameraFrameSP_sptr_t frame, bool isForcelyDelete = false, bool flagReleaseBuf = false);
    status_t m_LockedFrameComplete(ExynosCameraFrameSP_sptr_t frame);
    status_t m_clearList(frame_queue_t *list);
    status_t m_release(frame_queue_t *list);
    status_t m_releaseBuffer(ExynosCameraFrameSP_sptr_t frame);

    void    m_setLockFrameCaptureCount(int captureCount) { Mutex::Autolock lock(m_lockFrameCaptureCountLock); m_lockFrameCaptureCount = captureCount; };
    int     m_getLockFrameCaptureCount() { Mutex::Autolock lock(m_lockFrameCaptureCountLock); return m_lockFrameCaptureCount; };

    bool m_isFrameMetaTypeShotExt(void);
    status_t m_waitPushFrameToList(frame_queue_t *list, int tryCount);
    void m_adjustLockFrameList(void);
    status_t m_updateLatestFrameToLockFrameList(void);


protected:
    frame_queue_t m_frameHoldList;
    frame_queue_t m_hdrFrameHoldList;
    frame_queue_t m_OISFrameHoldList;
    frame_queue_t m_lockFrameHoldList;
    ExynosCameraFrameManager *m_frameMgr;
    ExynosCameraConfigurations *m_configurations;
    ExynosCameraParameters *m_parameters;
    ExynosCameraBufferSupplier *m_bufferSupplier;
    ExynosCameraActivityControl *m_activityControl;

    bool m_bayerFramelock;
    int m_reprocessingCount;
    int32_t m_frameHoldCount;
    int32_t m_lockFrameHoldCount;
    int32_t m_lockFrameCaptureCount;

    mutable Mutex m_listLock;
    mutable Mutex m_lockFrameCaptureCountLock;
    mutable Mutex m_stateLock;

    bool m_isCanceled;
    bool m_isFirstFrame;

    state_t m_state;
    SELECTOR_ID_t m_selectorId;
    int32_t m_lastFrameType;

    int  m_cameraId;
    char m_name[EXYNOS_CAMERA_NAME_STR_SIZE];
};

}
#endif

