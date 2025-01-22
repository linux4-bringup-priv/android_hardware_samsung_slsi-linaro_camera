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
#define LOG_TAG "ExynosCameraFrameSelectorSec"

#include "ExynosCameraFrameSelector.h"

#define FLASHED_LLS_COUNT 4

namespace android {


#define ENABLE_DEBUG_BUFFERQ
#ifdef ENABLE_DEBUG_BUFFERQ
#define BUFFERQ_LOG CLOGD
#else
#define BUFFERQ_LOG CLOGV
#endif

status_t ExynosCameraFrameSelector::release(void)
{
    int ret = 0;
    ret = m_release(&m_frameHoldList);
    if (ret != NO_ERROR) {
        CLOGE("m_frameHoldList release failed ");
    }
    ret = m_release(&m_hdrFrameHoldList);
    if (ret != NO_ERROR) {
        CLOGE("m_hdrFrameHoldList release failed ");
    }

#ifdef OIS_CAPTURE
    ret = m_release(&m_OISFrameHoldList);
    if (ret != NO_ERROR) {
        CLOGE("m_OISFrameHoldList release failed ");
    }
#endif

    ret = m_release(&m_lockFrameHoldList);
    if (ret != NO_ERROR) {
        CLOGE("m_lockFrameHoldList release failed ");
    }

    m_isCanceled = false;

    return NO_ERROR;
}

#ifdef OIS_CAPTURE
void ExynosCameraFrameSelector::setWaitTimeOISCapture(uint64_t waitTime)
{
    m_OISFrameHoldList.setWaitTime(waitTime);
}
#endif

status_t ExynosCameraFrameSelector::manageFrameHoldListHAL3(ExynosCameraFrameSP_sptr_t frame)
{
    int ret = 0;
    int pipeID;
    bool isSrc;
    int32_t dstPos;

    /* get first tag to match with this selector */
    ExynosCameraFrameSelectorTag compareTag;
    compareTag.selectorId = m_selectorId;
    if (frame->findSelectorTag(&compareTag) == true) {
        pipeID = compareTag.pipeId;
        isSrc = compareTag.isSrc;
        dstPos = compareTag.bufPos;

    } else {
        CLOGE("can't find selectorTag(id:%d) from frame(%d)",
                m_selectorId, frame->getFrameCount());
        ret = INVALID_OPERATION;
        goto p_err;
    }

    if (getBayerFrameLock() == true) {
        bool isLockFrame = false, isHoldFrame = true;
        ret = m_manageLockFrameHoldList(frame, isLockFrame, isHoldFrame);
        if (ret != NO_ERROR) {
            CLOGE("[F%d T%d] Failed to _manageLockFrameHoldList",
                    frame->getFrameCount(), frame->getFrameType());
            return ret;
        }

        BUFFERQ_LOG("isLockFrame(%d), isHoldFrame(%d)", isLockFrame, isHoldFrame);

        if (isLockFrame == true) {
            goto func_exit;
        } else if (isHoldFrame == false) {
            m_LockedFrameComplete(frame);
            goto func_exit;
        } else {
            CLOGV("[F%d T%d] use to other frame hold list",
                    frame->getFrameCount(), frame->getFrameType());
        }
    }

    if (m_configurations->getMode(CONFIGURATION_HDR_MODE) == true) {
        ret = m_manageHdrFrameHoldList(frame, pipeID, isSrc, dstPos);
    } else if (m_parameters->getOldBayerFrameLockCount() > 0) {

#ifdef OIS_CAPTURE
        if (m_activityControl->getOISCaptureMode() == true) {
            BUFFERQ_LOG("m_manageOISFrameHoldListHAL3(%d)", frame->getFrameCount());
            ret = m_manageOISFrameHoldListHAL3(frame, pipeID, isSrc, dstPos);
        } else
#endif
        {
            BUFFERQ_LOG("m_manageNormalFrameHoldListHAL3(%d)", frame->getFrameCount());
            ret = m_manageNormalFrameHoldListHAL3(frame, pipeID, isSrc, dstPos);
        }
    }
#ifdef OIS_CAPTURE
    else if (m_activityControl->getOISCaptureMode() == true) {
        if (m_flagResetFrameHoldList == false) {
            m_setLockFrameCaptureCount(0);
            m_list_release(&m_frameHoldList);
            m_list_release(&m_lockFrameHoldList);
            m_flagResetFrameHoldList = true;
            CLOGI("delete frameHoldList.size(%d), lockFrameHoldList.size(%d)",
                    m_frameHoldList.getSizeOfProcessQ(),
                    m_lockFrameHoldList.getSizeOfProcessQ());
        }

        {
            ret = m_manageOISFrameHoldListHAL3(frame, pipeID, isSrc, dstPos);
        }
    }
#endif
    else {
#ifdef OIS_CAPTURE
        if(m_flagResetFrameHoldList == true) {
            m_setLockFrameCaptureCount(0);
            m_list_release(&m_OISFrameHoldList);
            m_flagResetFrameHoldList = false;
            CLOGI("m_OISFrameHoldList delete(%d)", m_OISFrameHoldList.getSizeOfProcessQ());
        }
#endif
        ret = m_manageNormalFrameHoldListHAL3(frame, pipeID, isSrc, dstPos);
    }

func_exit:
    if (ret == NO_ERROR)
        frame->setStateInSelector(FRAME_STATE_IN_SELECTOR_PUSHED);

p_err:
    return ret;
}

status_t ExynosCameraFrameSelector::m_manageNormalFrameHoldListHAL3(ExynosCameraFrameSP_sptr_t newFrame,
                                                                                __unused int pipeID,
                                                                                __unused bool isSrc,
                                                                                __unused int32_t dstPos)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t oldFrame = NULL;
    ExynosCameraBuffer buffer;

    /* Skip INITIAL_SKIP_FRAME only FastenAeStable is disabled */
    /* This previous condition check is useless because almost framecount for capture is over than skip frame count */
    /*
     * if (m_parameters->getUseFastenAeStable() == true ||
     *     newFrame->getFrameCount() > INITIAL_SKIP_FRAME) {
     */

#ifdef SUPPORT_REMOSAIC_CAPTURE
    if (newFrame->getFrameType() == FRAME_TYPE_INTERNAL_SENSOR_TRANSITION) {
        if (m_lastFrameType != newFrame->getFrameType()) {
            CLOGD("[REMOSAIC][F%d D%d T%d P%d] frameListSize(%d/%d) lockFrameListSize(%d/%d)",
                    newFrame->getFrameCount(), newFrame->getMetaFrameCount(),
                    newFrame->getFrameType(), m_lastFrameType,
                    m_frameHoldList.getSizeOfProcessQ(), getFrameHoldCount(),
                    m_lockFrameHoldList.getSizeOfProcessQ(), m_lockFrameHoldCount);
            m_list_release(&m_frameHoldList);
            m_list_release(&m_lockFrameHoldList);
        }
    }
#endif

    CLOGV("[F%d D%d] push Frame Size(%d) holdCount(%d) lockFrameHoldList(%d)",
            newFrame->getFrameCount(), newFrame->getMetaFrameCount(),
            m_frameHoldList.getSizeOfProcessQ(), getFrameHoldCount(), m_lockFrameHoldList.getSizeOfProcessQ());
    m_pushQ(&m_frameHoldList, newFrame, true);

    while (m_frameHoldList.getSizeOfProcessQ() > getFrameHoldCount()) {
        if (m_popQ(&m_frameHoldList, oldFrame, true, 1) != NO_ERROR ) {
            CLOGE("getBufferToManageQ fail");
        } else {
            if (getLockFrameHoldCount() > 0) {
                ret = pushToLockFrameHoldList(oldFrame);
                if (ret != NO_ERROR) {
                    CLOGE("[F%d T%d]pushToLockFrameHoldList is fail",
                            newFrame->getFrameCount(), newFrame->getFrameType());
                }
            } else {
                /*
                 * Frames in m_frameHoldList and m_hdrFrameHoldList are locked
                 * when they are inserted on the list.
                 * So we need to use m_LockedFrameComplete() to remove those frames.
                 */
                m_LockedFrameComplete(oldFrame);
                oldFrame = NULL;
            }
        }
    }

    return ret;
}

status_t ExynosCameraFrameSelector::m_manageLockFrameHoldList(ExynosCameraFrameSP_sptr_t newFrame, bool &isLockFrame, bool &isHoldFrame)
{
    status_t ret = NO_ERROR;

    if (getBayerFrameLock() == false) {
        CLOGV("skip to lock frame hold");
        return ret;
    }

    Mutex::Autolock lock(m_stateLock);

    int oldFrameLockCount = getLockFrameHoldCount();
    bool pushToLockFrameList = ((m_getLockFrameCaptureCount() + m_lockFrameHoldList.getSizeOfProcessQ()) < oldFrameLockCount)
                                    && (m_state == STATE_OLD_FRAME_LOCK);

    BUFFERQ_LOG("[F%d] pushToLockFrameList(%d), LockFrameCaptureCnt(%d), lockFrameHoldList.size(%d), oldFrameLockCount(%d)",
            newFrame->getFrameCount(), pushToLockFrameList,
            m_getLockFrameCaptureCount(), m_lockFrameHoldList.getSizeOfProcessQ(), oldFrameLockCount);

    ret = m_updateLatestFrameToLockFrameList();
    if (ret != NO_ERROR) {
        CLOGE("Failed to update latest frame to lockFrameList()");
    }

    if (pushToLockFrameList == true) {
        ret = pushToLockFrameHoldList(newFrame);
        if (ret != NO_ERROR) {
            CLOGE("[F%d T%d]pushToLockFrameHoldList is fail",
                        newFrame->getFrameCount(), newFrame->getFrameType());
            return ret;
        }
        isLockFrame = true;

        CLOGD("[F%d D%d]isLockFrame(%d) lockCaptureCount(%d) lockFrameHoldList(%d)",
            newFrame->getFrameCount(), newFrame->getMetaFrameCount(),
            isLockFrame, m_getLockFrameCaptureCount(), m_lockFrameHoldList.getSizeOfProcessQ());
    } else {
        frame_queue_t *holdList = NULL;
#ifdef OIS_CAPTURE
        if (m_activityControl->getOISCaptureMode() == true) {
            holdList = &m_OISFrameHoldList;
        } else
#endif
        {
            holdList = &m_frameHoldList;
        }

        if (holdList->getSizeOfProcessQ() >= getFrameHoldCount()) {
            isHoldFrame = false;
        }

        CLOGD("[F%d D%d]isHoldFrame(%d) holdListSize(%d) holdCount(%d)",
            newFrame->getFrameCount(), newFrame->getMetaFrameCount(),
            isHoldFrame, holdList->getSizeOfProcessQ(), getFrameHoldCount());
    }

    return ret;
}

status_t ExynosCameraFrameSelector::m_list_release(frame_queue_t *list)
{
    ExynosCameraFrameSP_sptr_t frame  = NULL;

    while (list->getSizeOfProcessQ() > 0) {
        if (m_popQ(list, frame, true, 1) != NO_ERROR) {
            CLOGE("getBufferToManageQ fail");
#if 0
            m_bufMgr->printBufferState();
            m_bufMgr->printBufferQState();
#endif
        } else {
            m_LockedFrameComplete(frame);
            frame = NULL;
        }
    }

    return NO_ERROR;
}

#ifdef OIS_CAPTURE
status_t ExynosCameraFrameSelector::m_manageOISFrameHoldListHAL3(ExynosCameraFrameSP_sptr_t newFrame,
                                                                  int pipeID, bool isSrc, int32_t dstPos)
{
    int ret = 0;
    ExynosCameraBuffer buffer;
    ExynosCameraFrameSP_sptr_t oldFrame = NULL;
    ExynosCameraActivitySpecialCapture *m_sCaptureMgr = NULL;
    unsigned int OISFcount = 0;
    unsigned int fliteFcount = 0;
    m_sCaptureMgr = m_activityControl->getSpecialCaptureMgr();
    OISFcount = m_sCaptureMgr->getOISCaptureFcount();

    ret = m_getBufferFromFrame(newFrame, pipeID, isSrc, &buffer, dstPos);
    if( ret != NO_ERROR ) {
        CLOGE("m_getBufferFromFrame fail pipeID(%d) BufferType(%s)",
               pipeID, (isSrc)?"Src":"Dst");
    }

    if (m_parameters->getUsePureBayerReprocessing() == true) {
        camera2_shot_ext *shot_ext = NULL;
        shot_ext = (camera2_shot_ext *)(buffer.addr[buffer.getMetaPlaneIndex()]);
        if (shot_ext != NULL)
            fliteFcount = shot_ext->shot.dm.request.frameCount;
        else
            CLOGE("fliteReprocessingBuffer is null");
    } else {
        camera2_stream *shot_stream = NULL;
        shot_stream = (camera2_stream *)(buffer.addr[buffer.getMetaPlaneIndex()]);
        if (shot_stream != NULL)
            fliteFcount = shot_stream->fcount;
        else
            CLOGE("fliteReprocessingBuffer is null");
    }

    BUFFERQ_LOG("OISFcount(%d), fliteFcount(%d)", OISFcount, fliteFcount);

    if (OISFcount <= 0 || fliteFcount < OISFcount) {
        CLOGI("Fcount %d, fliteFcount (%d) halcount(%d) OIS size(%d), Normal size(%d), Lock size(%d)",
                OISFcount, fliteFcount, newFrame->getFrameCount(),
                m_OISFrameHoldList.getSizeOfProcessQ(),
                m_frameHoldList.getSizeOfProcessQ(),
                m_lockFrameHoldList.getSizeOfProcessQ());

        BUFFERQ_LOG("m_flagResetFrameHoldList(%d)", m_flagResetFrameHoldList);

        if (m_flagResetFrameHoldList == false) {
            ret = pushToLockFrameHoldList(newFrame);
            if (ret != NO_ERROR) {
                CLOGE("[F%d T%d]pushToLockFrameHoldList is fail",
                            newFrame->getFrameCount(), newFrame->getFrameType());
                return ret;
            }
        } else {
            m_LockedFrameComplete(newFrame);
        }

        newFrame = NULL;
    } else {
        int captureCount = m_configurations->getModeValue(CONFIGURATION_CAPTURE_COUNT);
        if ((OISFcount + captureCount) <= fliteFcount) {
            ret = m_manageNormalFrameHoldListHAL3(newFrame, pipeID, isSrc, dstPos);
            CLOGE("[F%d]The frame count of the Bayer buffer(DF%d) can not be used for OIS capture(DF%d ~ DF%d).",
                    newFrame->getFrameCount(), fliteFcount, OISFcount, (OISFcount + (captureCount - 1)));

            m_activityControl->setOISCaptureMode(false);
            CLOGE("[F%d]So, Forces OIS mode %d", newFrame->getFrameCount(), m_activityControl->getOISCaptureMode());
        } else if (OISFcount <= fliteFcount) {
            CLOGI("zsl-like Fcount(%d), fliteFcount(%d), halcount(%d), captureCount(%d)",
                    OISFcount, fliteFcount, newFrame->getFrameCount(), captureCount);
            if (getBayerFrameLock() == true
                && m_OISFrameHoldList.getSizeOfProcessQ() > getFrameHoldCount()) {
                CLOGD("[F%d]Complete frame", newFrame->getFrameCount());
                m_LockedFrameComplete(newFrame);
            } else {
                m_pushQ(&m_OISFrameHoldList, newFrame, true);

                BUFFERQ_LOG("push OISFrame(%d) to OISFrameHoldList", newFrame->getFrameCount());
                if (m_OISFrameHoldList.getSizeOfProcessQ() > getFrameHoldCount()) {
                    if (m_popQ(&m_OISFrameHoldList, oldFrame, true, 1) != NO_ERROR) {
                        CLOGE("getBufferToManageQ fail");
                    } else {
                        m_LockedFrameComplete(oldFrame);
                        oldFrame = NULL;
                    }
                }
            }
        } else {
            CLOGE("unexpected case. OISFcount(%d), fliteFcount(%d), halcount(%d)",
                    OISFcount, fliteFcount, newFrame->getFrameCount());
            m_LockedFrameComplete(newFrame);
        }
    }

    return ret;
}
#endif

ExynosCameraFrameSP_sptr_t ExynosCameraFrameSelector::selectCaptureFrames(int count,
                                                                  uint32_t frameCount,
                                                                  int tryCount)
{
    ExynosCameraFrameSP_sptr_t selectedFrame = NULL;
    ExynosCameraActivityFlash *m_flashMgr = NULL;
    int oldFrameLockCount = m_parameters->getOldBayerFrameLockCount();
    int lockFrameCaptureCount = m_getLockFrameCaptureCount();
    bool useOldFrame = false;

    if (getBayerFrameLock() == true
        && lockFrameCaptureCount < oldFrameLockCount) {
        useOldFrame = true;
        CLOGD("use old bayer Frame(%d), lockCaptureCount %d / oldFrameLockCount %d",
                useOldFrame, lockFrameCaptureCount, oldFrameLockCount);
    }

    m_reprocessingCount = count;
    m_flashMgr = m_activityControl->getFlashMgr();

    if (m_flashMgr->getNeedCaptureFlash() == true
#ifdef OIS_CAPTURE
        && (m_activityControl->getOISCaptureMode() == false)
#endif
        ) {
        unlockFrameList();
        selectedFrame = m_selectFlashFrameV2(tryCount);
        if (selectedFrame == NULL && !m_isCanceled) {
            CLOGE("Failed to selectFlashFrame");
            selectedFrame = m_selectNormalFrame(tryCount);
        }
    } else if (useOldFrame == true) {
#ifdef OIS_CAPTURE
        if (m_activityControl->getOISCaptureMode() == true
                && m_flagResetFrameHoldList == false) {

            if (lockFrameCaptureCount == 0) {
                CLOGD("Waiting OISFrameList updating count(%d), frameCount(%d) tryCount(%d)",
                        count, frameCount, tryCount);

                if (m_waitPushFrameToList(&m_OISFrameHoldList, tryCount*10) != NO_ERROR) {
                    CLOGD("Failed to get OIS capture frame");
                }
            }
        }
#endif

        selectedFrame = m_selectLockAndNormalFrame(tryCount);
    }
#ifdef OIS_CAPTURE
    else if (m_activityControl->getOISCaptureMode() == true) {
        selectedFrame = m_selectOISNormalFrameHAL3(tryCount);
        if (selectedFrame == NULL && !m_isCanceled) {
            CLOGE("Failed to selectCaptureFrame");
            selectedFrame = m_selectNormalFrame(tryCount);
        }
    }
#endif
    else {
        switch(m_parameters->getReprocessingBayerMode()) {
            case REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON:
            case REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON:
                if (m_configurations->getMode(CONFIGURATION_REMOSAIC_CAPTURE_MODE)) {
                    selectedFrame = m_selectRemosaicFrame(tryCount);
                } else {
                    selectedFrame = m_selectNormalFrame(tryCount);
                    m_clearList(&m_lockFrameHoldList);
                }
                break;
            case REPROCESSING_BAYER_MODE_PURE_DYNAMIC:
            case REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC:
                selectedFrame = m_selectCaptureFrame(frameCount, tryCount);
                break;
            default:
                CLOGE("reprocessing is not valid");
                break;
        }

        if (selectedFrame == NULL && !m_isCanceled) {
            CLOGE("Failed to selectCaptureFrame");
            selectedFrame = m_selectNormalFrame(tryCount);
        }
    }

    return selectedFrame;
}

ExynosCameraFrameSP_sptr_t ExynosCameraFrameSelector::m_selectNormalFrame(int tryCount)
{
    int ret = 0;
    ExynosCameraFrameSP_sptr_t selectedFrame = NULL;

    ret = m_waitAndpopQ(&m_frameHoldList, selectedFrame, false, tryCount);
    if (ret < 0 || selectedFrame == NULL) {
        CLOGD("getFrame Fail ret(%d)", ret);
        return NULL;
    } else if (m_isCanceled == true) {
        CLOGD("m_isCanceled");
        if (selectedFrame != NULL) {
            m_LockedFrameComplete(selectedFrame);
        }
        return NULL;
    }
    CLOGD("Frame Count(%d)", selectedFrame->getFrameCount());

    return selectedFrame;
}

/**
**      USAGE : HAL3, LCD_FLASH
**
**      This function does not include main flash control code.
**
**/
ExynosCameraFrameSP_sptr_t ExynosCameraFrameSelector::m_selectFlashFrameV2(int tryCount)
{
    int ret = 0;
    ExynosCameraFrameSP_sptr_t selectedFrame = NULL;
    ExynosCameraBuffer selectedBuffer;
    int bufferFcount = 0;
    int waitFcount = 0;
    int totalWaitingCount = 0;
    ExynosCameraActivityFlash *flashMgr = NULL;
    flashMgr = m_activityControl->getFlashMgr();
    int pipeID;
    bool isSrc;
    int32_t dstPos;

    /* Choose bayerBuffer to process reprocessing */
    while (totalWaitingCount <= (FLASH_MAIN_SELECT_TIMEOUT_COUNT + m_parameters->getReprocessingBayerHoldCount())) {
        ret = m_waitAndpopQ(&m_frameHoldList, selectedFrame, false, tryCount);
        if (ret < 0 ||  selectedFrame == NULL) {
            CLOGD("getFrame Fail ret(%d)", ret);
            flashMgr->resetShotFcount();
            return NULL;
        } else if (m_isCanceled == true) {
            CLOGD("m_isCanceled");
            if (selectedFrame != NULL) {
                m_LockedFrameComplete(selectedFrame);
            }
            flashMgr->resetShotFcount();
            return NULL;
        }
        CLOGD("Frame Count(%d)", selectedFrame->getFrameCount());

        if (waitFcount == 0) {
            waitFcount = flashMgr->getBestFlashShotFcount();
            CLOGD("[Flash] best frame count for flash capture : [F%d]", waitFcount);
        }

        /* Handling exception cases : like preflash is not processed */
        if (waitFcount < 0) {
            CLOGW("waitFcount is negative : preflash is not processed");

            flashMgr->resetShotFcount();
            return NULL;
        }

        if (m_isCanceled == true) {
            CLOGD("m_isCanceled");

            flashMgr->resetShotFcount();
            return NULL;
        }

        ExynosCameraFrameSelectorTag compareTag;
        compareTag.selectorId = m_selectorId;
        /* get first tag to match with this selector */
        if (selectedFrame->findSelectorTag(&compareTag) == true) {
            pipeID = compareTag.pipeId;
            isSrc = compareTag.isSrc;
            dstPos = compareTag.bufPos;
        }

        ret = m_getBufferFromFrame(selectedFrame, pipeID, isSrc, &selectedBuffer, dstPos);
        if (ret != NO_ERROR) {
            CLOGE("m_getBufferFromFrame fail pipeID(%d) BufferType(%s) bufferPtr(%p)",
                    pipeID, (isSrc)? "Src" : "Dst", &selectedBuffer);
        }

        if (m_isFrameMetaTypeShotExt() == true) {
            camera2_shot_ext *shot_ext = NULL;
            shot_ext = (camera2_shot_ext *)(selectedBuffer.addr[selectedBuffer.getMetaPlaneIndex()]);
            if (shot_ext != NULL)
                bufferFcount = shot_ext->shot.dm.request.frameCount;
            else
                CLOGE("selectedBuffer is null");
        } else {
            camera2_stream *shot_stream = NULL;
            shot_stream = (camera2_stream *)(selectedBuffer.addr[selectedBuffer.getMetaPlaneIndex()]);
            if (shot_stream != NULL)
                bufferFcount = shot_stream->fcount;
            else
                CLOGE("selectedBuffer is null");
        }

        /* Put mismatched buffer */
        if (waitFcount != bufferFcount) {
            if (m_bufferSupplier == NULL) {
                CLOGE("m_bufferSupplier is NULL");
                flashMgr->resetShotFcount();
                return NULL;
            } else {
                m_frameComplete(selectedFrame, false, true);
                selectedFrame = NULL;
            }
        }

        /* On HAL3, frame selector must wait the waitFcount to be updated by Flash Manager */
        if (waitFcount > 1 && waitFcount <= bufferFcount)
            break;
        else
            waitFcount = 0;

        totalWaitingCount++;
        CLOGD(" (totalWaitingCount %d)", totalWaitingCount);
    }

    if (totalWaitingCount > FLASH_MAIN_TIMEOUT_COUNT) {
        CLOGW("fail to get bayer frame count for flash capture (totalWaitingCount %d)",
                totalWaitingCount);
    }

    CLOGD("waitFcount : %d, bufferFcount : %d",
            waitFcount, bufferFcount);

    flashMgr->resetShotFcount();
    return selectedFrame;
}


#ifdef OIS_CAPTURE
ExynosCameraFrameSP_sptr_t ExynosCameraFrameSelector::m_selectOISNormalFrameHAL3(int tryCount)
{
    int ret = 0;
    ExynosCameraFrameSP_sptr_t selectedFrame = NULL;

    ret = m_waitAndpopQ(&m_OISFrameHoldList, selectedFrame, false, tryCount);
    if (ret < 0 || selectedFrame == NULL) {
        ALOGD("DEBUG(%s[%d]):getFrame Fail ret(%d)", __FUNCTION__, __LINE__, ret);
        m_activityControl->setOISCaptureMode(false);
        if (selectedFrame != NULL) {
            m_LockedFrameComplete(selectedFrame);
        }
        return NULL;
    } else if (m_isCanceled == true) {
        ALOGD("DEBUG(%s[%d]):m_isCanceled", __FUNCTION__, __LINE__);
        m_activityControl->setOISCaptureMode(false);
        if (selectedFrame != NULL) {
            m_LockedFrameComplete(selectedFrame);
        }
        return NULL;
    }

    return selectedFrame;
}
#endif

ExynosCameraFrameSP_sptr_t ExynosCameraFrameSelector::m_selectLockFrame(int tryCount)
{
    int ret = 0;
    ExynosCameraFrameSP_sptr_t selectedFrame = NULL;

    m_adjustLockFrameList();

    ret = m_waitAndpopQ(&m_lockFrameHoldList, selectedFrame, false, tryCount);
    if (ret < 0 || selectedFrame == NULL) {
        CLOGD("getFrame Fail ret(%d)", ret);
        return NULL;
    } else if (m_isCanceled == true) {
        CLOGD("m_isCanceled");
        if (selectedFrame != NULL) {
            m_LockedFrameComplete(selectedFrame);
        }
        return NULL;
    } else if (m_lockFrameHoldList.getSizeOfProcessQ() >= getLockFrameHoldCount()) {
        CLOGE("[F%d]Although lock frame is selected, lockFrameHoldList(%d) cannot be larger than lockFrameHoldCount(%d)",
                selectedFrame->getFrameCount(), m_lockFrameHoldList.getSizeOfProcessQ(), getLockFrameHoldCount());
        m_LockedFrameComplete(selectedFrame);
        return NULL;
    }

    CLOGD("[F%d D%d] select lock frame", selectedFrame->getFrameCount(), selectedFrame->getMetaFrameCount());

    return selectedFrame;
}

void ExynosCameraFrameSelector::m_adjustLockFrameList(void)
{
    if (m_parameters->getOldBayerFrameLockCount() < getLockFrameHoldCount()) {
        if (m_lockFrameHoldList.getSizeOfProcessQ() != m_parameters->getOldBayerFrameLockCount()) {
            releaseLockFrames(m_parameters->getOldBayerFrameLockCount());
        }
    } else {
        releaseLockFrames(getLockFrameHoldCount());
    }
}

status_t ExynosCameraFrameSelector::m_updateLatestFrameToLockFrameList(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameSP_sptr_t latestFrame = NULL;

    while (m_frameHoldList.getSizeOfProcessQ() > 0) {
        if (m_popQ(&m_frameHoldList, latestFrame, true, 1) != NO_ERROR ) {
            CLOGE("getBufferToManageQ fail");
        } else {
            BUFFERQ_LOG("getLockFrameHoldCount(%d), latestFrame(%d)", getLockFrameHoldCount(), latestFrame->getFrameCount());
            if (getLockFrameHoldCount() > 0) {
                ret = pushToLockFrameHoldList(latestFrame);
                if (ret != NO_ERROR) {
                    CLOGE("[F%d T%d]pushToLockFrameHoldList is fail",
                            latestFrame->getFrameCount(), latestFrame->getFrameType());
                }
            } else {
                /*
                * Frames in m_frameHoldList and m_hdrFrameHoldList are locked
                * when they are inserted on the list.
                * So we need to use m_LockedFrameComplete() to remove those frames.
                */
                m_LockedFrameComplete(latestFrame);
                latestFrame = NULL;
            }
        }
    }

    return ret;
}

ExynosCameraFrameSP_sptr_t ExynosCameraFrameSelector::m_selectRemosaicFrame(int tryCount)
{
    int ret = 0;
    ExynosCameraFrameSP_sptr_t selectedFrame = NULL;
    int totalWaitingCount = 0;
    bool find = false;

    /* Choose bayerBuffer to process reprocessing */
    while (totalWaitingCount <= CAPTURE_WAITING_COUNT) {
        ret = m_waitAndpopQ(&m_frameHoldList, selectedFrame, false, tryCount);
        if (ret < 0 ||  selectedFrame == NULL) {
            CLOGD("getFrame Fail ret(%d)", ret);
            return NULL;
        }
        CLOGD("Frame Count(%d)", selectedFrame->getFrameCount());

        /* Put mismatched buffer */
#ifdef SUPPORT_REMOSAIC_CAPTURE
        if (selectedFrame->getFrameType() != FRAME_TYPE_INTERNAL_SENSOR_TRANSITION) {
            m_frameComplete(selectedFrame, false, true);
            selectedFrame = NULL;
        } else {
            find = true;
            break;
        }
#endif
        totalWaitingCount++;
        CLOGD(" (totalWaitingCount %d)", totalWaitingCount);
    }

    if (totalWaitingCount > FLASH_MAIN_TIMEOUT_COUNT) {
        CLOGW("fail to get bayer frame count for flash capture (totalWaitingCount %d)",
                totalWaitingCount);
    }

    CLOGD("[REMOSAIC][F%d D%d S%d T%d P%d] select remosaic best frame", selectedFrame->getFrameCount(), selectedFrame->getMetaFrameCount(), m_state, selectedFrame->getFrameType(), m_lastFrameType);

    return selectedFrame;
}

ExynosCameraFrameSP_sptr_t ExynosCameraFrameSelector::m_selectLockAndNormalFrame(int tryCount)
{
    ExynosCameraFrameSP_sptr_t selectedFrame = NULL;

    {
        Mutex::Autolock lock(m_stateLock);
        m_state = STATE_OLD_FRAME_LOCK;
    }

    int lockFrameCaptureCount = m_getLockFrameCaptureCount();
    m_setLockFrameCaptureCount(++lockFrameCaptureCount);

    if (lockFrameCaptureCount <= getLockFrameHoldCount()) {
        selectedFrame = m_selectLockFrame(tryCount);
        if (selectedFrame == NULL) {
            selectedFrame = m_selectNormalFrame(tryCount);
        }
    }

    return selectedFrame;
}

status_t ExynosCameraFrameSelector::m_waitAndpopQ(frame_queue_t *list, ExynosCameraFrameSP_dptr_t outframe, bool unlockflag, int tryCount)
{
    status_t ret = NO_ERROR;
    int iter = 0;

    do {
        ret = list->waitAndPopProcessQ(&outframe);

        if (m_isCanceled == true) {
            CLOGD("m_isCanceled");

            return NO_ERROR;
        }

        if( ret < 0 ) {
            if( ret == TIMED_OUT ) {
                CLOGD("waitAndPopQ Time out -> retry[max cur](%d %d) in %s", tryCount, iter, list->getName());
                iter++;
                continue;
            }
#ifdef OIS_CAPTURE
            else if (ret == INVALID_OPERATION && list == &m_OISFrameHoldList) {
                CLOGE("m_OISFrameHoldList is empty");
                return ret;
            }
#endif
        }

        if (outframe != NULL) {
            CFLOGD(outframe, "frame poped in %s", list->getName());
        }
    } while (ret != OK && tryCount > iter);

    if(ret != OK) {
        CLOGE("wait for popQ fail(%d) in %s", ret, list->getName());
        return ret;
    }

    if(outframe == NULL) {
        CLOGE("wait for popQ frame = NULL in %s", list->getName());
        return ret;
    }

    if(unlockflag) {
        outframe->frameUnlock();
    }
    return ret;
}

status_t ExynosCameraFrameSelector::m_waitPushFrameToList(frame_queue_t *list, int tryCount)
{
    status_t ret = INVALID_OPERATION;
    int iter = 0;
    int sizeOfList = 0;

    do {
        sizeOfList = list->getSizeOfProcessQ();

        iter++;

        usleep(10000);

    } while (sizeOfList <= 0 && tryCount > iter);

    if (tryCount <= iter) {
        CLOGD("TIME OUT waiting list");
        ret = TIMED_OUT;
    }

    if (sizeOfList > 0) {
        CLOGD("sizeOflist(%d)", sizeOfList);
        ret = NO_ERROR;
    }

    return ret;
}

status_t ExynosCameraFrameSelector::clearList(void)
{
    int ret = 0;
    ExynosCameraFrameSP_sptr_t frame = NULL;

    if (m_frameHoldList.isWaiting() == false) {
        ret = m_clearList(&m_frameHoldList);
        if( ret < 0 ) {
            CLOGE("m_frameHoldList clear failed");
        }
    } else {
        CLOGE("Cannot clear frameHoldList cause waiting for pop frame");
    }

    if (m_hdrFrameHoldList.isWaiting() == false) {
        ret = m_clearList(&m_hdrFrameHoldList);
        if( ret < 0 ) {
            CLOGE("m_hdrFrameHoldList clear failed");
        }
    } else {
        CLOGE("Cannot clear hdrFrameHoldList cause waiting for pop frame");
    }

#ifdef OIS_CAPTURE
    if (m_OISFrameHoldList.isWaiting() == false) {
        ret = m_clearList(&m_OISFrameHoldList);
        if( ret < 0 ) {
            CLOGE("m_OISFrameHoldList clear failed");
        }
    } else {
        CLOGE("Cannot clear m_OISFrameHoldList cause waiting for pop frame");
    }
#endif

    if (m_lockFrameHoldList.isWaiting() == false) {
        ret = m_clearList(&m_lockFrameHoldList);
        if( ret < 0 ) {
            CLOGE("m_lockFrameHoldList clear failed");
        }
    } else {
        CLOGE("Cannot clear m_lockFrameHoldList cause waiting for pop frame");
    }

    m_isCanceled = false;

    return NO_ERROR;
}

status_t ExynosCameraFrameSelector::m_releaseBuffer(ExynosCameraFrameSP_sptr_t frame)
{
    selector_tag_queue_t::iterator r;
    int ret = 0;
    ExynosCameraBuffer buffer;
    ExynosCameraBuffer bayerBuffer;
    int pipeID;
    bool isSrc;
    int32_t dstPos;
    selector_tag_queue_t *selectorTagList = frame->getSelectorTagList();

    if (m_bufferSupplier == NULL) {
        CLOGE("m_bufferSupplier is NULL");
        return INVALID_OPERATION;
    }
#ifdef USE_DUAL_BAYER_SYNC
    if (frame->getDualOperationMode() == DUAL_OPERATION_MODE_SYNC
        && frame->getPairFrame() != NULL
        && frame->isSlaveFrame() == true) {
        return NO_ERROR;
    }
#endif

#ifdef SUPPORT_DEPTH_MAP
    if (frame->getRequest(PIPE_VC1) == true
        && frame->getStreamRequested(STREAM_TYPE_DEPTH) == false
        ) {
        int pipeId = PIPE_3AA;

        if (m_parameters->getHwConnectionMode(PIPE_FLITE, PIPE_3AA) == HW_CONNECTION_MODE_M2M) {
            pipeId = PIPE_FLITE;
        }

        ExynosCameraBuffer depthMapBuffer;

        depthMapBuffer.index = -2;

        ret = frame->getDstBuffer(pipeId, &depthMapBuffer, CAPTURE_NODE_2);
        if (ret != NO_ERROR) {
            CLOGE("Failed to get DepthMap buffer");
        }

        ret = m_bufferSupplier->putBuffer(depthMapBuffer);
        if (ret != NO_ERROR) {
            CLOGE("[F%d B%d]Failed to putBuffer. ret %d",
                    frame->getFrameCount(), depthMapBuffer.index, ret);
        }
    }
#endif

    /* lock for selectorTag */
    frame->lockSelectorTagList();
    frame->setRawStateInSelector(FRAME_STATE_IN_SELECTOR_REMOVE);

    if (selectorTagList->empty())
        goto func_exit;

    r = selectorTagList->begin()++;
    do {
        /* only removed the tag matched with selectorId */
        if (r->selectorId == m_selectorId) {
            pipeID = r->pipeId;
            isSrc = r->isSrc;
            dstPos = r->bufPos;

#ifdef DEBUG_RAWDUMP
    /* TODO: Do not use dstPos to check processed bayer reprocessing mode */
    if (dstPos != CAPTURE_NODE_1 && frame->getRequest(PIPE_VC0) == true) {
        bayerBuffer.index = -2;
        ret = m_getBufferFromFrame(frame, pipeID, isSrc, &bayerBuffer, CAPTURE_NODE_1);
        if (ret != NO_ERROR) {
            CLOGE("[F%d B%d]Failed to getBufferFromFrame for RAWDUMP. pipeID %d ret %d",
                    frame->getFrameCount(), bayerBuffer.index, pipeID, ret);
            /* continue */
        } else {
            ret = m_bufferSupplier->putBuffer(bayerBuffer);
            if (ret != NO_ERROR) {
                CLOGE("[F%d B%d]Failed to putBuffer. ret %d",
                        frame->getFrameCount(), bayerBuffer.index, ret);
                /* continue */
            }
        }
    }
#endif

            ret = m_getBufferFromFrame(frame, pipeID, isSrc, &buffer, dstPos);
            if( ret != NO_ERROR ) {
                CLOGE("m_getBufferFromFrame fail pipeID(%d) BufferType(%s) bufferPtr(%p)",
                        pipeID, (isSrc)? "Src" : "Dst", &buffer);
            }

            ret = m_bufferSupplier->putBuffer(buffer);
            if (ret < 0) {
                CLOGE("putIndex is %d", buffer.index);
#if 0
                m_bufMgr->printBufferState();
                m_bufMgr->printBufferQState();
#endif
            }

            r = selectorTagList->erase(r);
        } else {
            r++;
        }
    } while (r != selectorTagList->end());

func_exit:
    /* unlock for selectorTag */
    frame->unlockSelectorTagList();

#ifdef USE_DUAL_BAYER_SYNC
    if (frame->getDualOperationMode() == DUAL_OPERATION_MODE_SYNC
        && frame->getPairFrame() != NULL
        && frame->isSlaveFrame() == false) {
        ret = releasePairFrameBuffer(frame);
        if (ret != NO_ERROR) {
            CLOGE("fail to release pair frame buffer. ret(%d)", ret);
            return ret;
        }
    }
#endif

    return NO_ERROR;
}

status_t ExynosCameraFrameSelector::releasePairFrameBuffer(ExynosCameraFrameSP_sptr_t frame)
{
    int sizeOfMasterPairFrameQ = frame->getSizeOfPairFrameQ();
    while (sizeOfMasterPairFrameQ-- > 0) {
        ExynosCameraFrameSP_sptr_t pairFrame = frame->popPairFrame();
        pairFrame->releasePairFrameQ();
        m_releaseBuffer(pairFrame);
    }

    if (frame->getSizeOfPairFrameQ() > 0) {
        CLOGE("invalid pairFrameQ size(%d)", frame->getSizeOfPairFrameQ());
        frame->releasePairFrameQ();
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

}
